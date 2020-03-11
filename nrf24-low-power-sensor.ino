//
// Example code for an ATmega328P processor conserving power by entering deep sleep and
// being woken up by an interrupt generated by an DS3231 RTC to read and report sensor
// values using a NRF24L01 transceiver.
//
// Please refer to GitHub reposiotory for further details:
//   https://github.com/bjarne-hansen/nrf24-low-power-sensor
// 
// In order to get the current date/time when booting it used a simple setup with an Raspberry Pi
// server that publishes date/time information via a NRF24L01 transceiver.
//
// Please refer to the following pages on github for further details:
//   https://github.com/bjarne-hansen/nrf24-time-server
//   https://github.com/bjarne-hansen/nrf24-time-client
// 
// When date/time has been synchronised the program will go to sleep and be woken up every 15 minutes
// (configurable) to read sensor values and send collected data to a gateway via a NRF24L01 transceiver.
// 
// The gateway will take the binary payload, convert it to JSON and publish it on a MQTT queue.
//
// Please refer to the GitHub repository for the gateway for further details:
//   https://github.com/bjarne-hansen/nrf24-mqtt-gateway
//

// NRF24L01 communication libraries.
//   https://tmrh20.github.io/RF24/
#include <printf.h>
#include <nRF24L01.h>
#include <RF24_config.h>
#include <RF24.h>

// Time library 
//   https://github.com/PaulStoffregen/Time
#include <TimeLib.h>

// DS3232RTC by Jack Christensen
//   https://github.com/JChristensen/DS3232RTC
#include <DS3232RTC.h>

// Low-Power by Rocket Scream Electronics
//   https://github.com/rocketscream/Low-Power
#include <LowPower.h>

// "DHT sensor library by Adafruit"
//   https://github.com/adafruit/DHT-sensor-library
#include <DHT.h>
#include <DHT_U.h>

#define PIN_RELAY     4 
#define PIN_DHT       5
#define PIN_MOISTURE A0
#define PIN_INT       2
const uint8_t PIN_INTERRUPT = 2;
const uint8_t PIN_RF24_CSN = 9;
const uint8_t PIN_RF24_CE = 10;

byte time_svr_addr[6] = "DTP01";        // Address of date/time server.
byte time_cln_addr[6] = "DTCLN";        // Client address of date/time client (this device).
byte payload[16];


byte gateway_addr[6] = "GW001";         // Address of the data gateway.
byte sensor_addr[6] = "SM105";          // Address of the sensor device (this device).

RF24 radio(PIN_RF24_CSN, PIN_RF24_CE);  // CSN, CE pins

/// ***

DHT dht(PIN_DHT, DHT22);

// protocol(1), id(3), reading(2), vcc(2), temperature(4), humidity(4)
byte protocol = 1;
unsigned int id = 0xBA01;
unsigned int reading = 0;

// ***

void setup() 
{
  Serial.begin(9600); 
  printf_begin();

  //
  // This part of the setup process receives the current date/time from a date/time server via NRF24L01.
  //
  // PLEASE NOTE: The code below will loop until a date/time has been received. If no date/time server
  // in range is publishing date/time information it will appear to hang.
  //
  // Initialize the real time clock (RTC)
  Serial.println("Initialize RTC ...");
  setupRTC();
  
  // Setup RF24 module for receiving date/time.
  Serial.println("Configure RF24 module for date/time reception ...");
  setupDateTimeClient();

  // Receive date/time being broadcast.
  Serial.println("Receiving date/time from server ...");
  time_t dt = receiveDateTime();

  // Update the RTC with current date/time
  updateRTCDateTime(dt);

  //
  // End of date/time synchronization.
  //
  
  // Initialize "relay" PIN to control power to sensors.
  // A transistor "relay" is used to turn on/off power to attached sensor to make sure that there
  // is only minimal power consumption while sleeping.
  //
  Serial.println("Initialize relay ...");
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);

  // Initialize the DHT sensor.
  // Different sensors can be initialized here.
  Serial.println("Initialize DHT sensor ...");
  dht.begin();

  // Setup and configure the RTC for generating alarm interrupts allowing the processeor to
  // wake up from deep sleep.
  
  // Initialize the interrupt pin for waking the ATmega328
  Serial.println("Configure RTC interrupt ...");
  pinMode(PIN_INTERRUPT, INPUT_PULLUP);
  digitalWrite(PIN_INTERRUPT, HIGH);  
  attachInterrupt(digitalPinToInterrupt(PIN_INTERRUPT), wakeUp, FALLING);
  
  // Set alarm to trigger interrupt.
  Serial.println("Configure first alarm ...");
  set_next_alarm();
  
  // Clear the alarm flag and activate interrupt for alarm 1.
  RTC.alarm(ALARM_1);
  RTC.alarmInterrupt(ALARM_1, true); 

  // Configure NRF24 module for sending data to gateway.
  setup_gateway_client();
  
}

void loop() 
{
  // Go to deep sleep and wait for the alarm to trigger an interrupt that will wake
  // the processor.  
  Serial.println("Sleeping ...");
  delay(1000);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

  // If an alarm was triggered, continue with reading the sensors and sending the results
  // results to the gateway server.
  if (RTC.alarm(ALARM_1))
  {
    // Report date/time for alarm.
    print_alarm_date_time();

    // Read and send sensor measurements.
    read_sensors();

    // Set next alarm.
    set_next_alarm();
  }
}

//
// Function to read and report sensor values.
//
void read_sensors()
{
  float h, t;
  int vcc = 3300;
  int retries = 5;

  // Connect power to sensors.
  digitalWrite(PIN_RELAY, HIGH);

  // Let sensors settle to be ready for reading.
  // Serial.println("Read sensors ...");    
  delay(2000);

  // Read sensors.
  h = dht.readHumidity();
  t = dht.readTemperature();

  // Serial.print("Humidity: "); Serial.println(h);
  // Serial.print("Temperature: "); Serial.println(t);
  // Serial.print("Size: "); Serial.println(sizeof(payload));
  
  // Serial.println("Send results to gateway ...");  
  reading++;
  memcpy(payload + 1, (byte *)(&id), 2);
  memcpy(payload + 3, (byte *)(&reading), 2);
  memcpy(payload + 5, (byte *)(&vcc), 2);
  memcpy(payload + 7, (byte *)(&t), 4);
  memcpy(payload + 11, (byte *)(&h), 4);

  // for (int i = 0; i < sizeof(payload); i++)
  //   Serial.print(payload[i], 16); Serial.print(", ");
  // Serial.println();
  
  radio.powerUp();
  delay(50);

  while (retries > 0)
  {
    delay((5 - retries) * 50);      // 0, 50, 100, 150, 200 (max. 500ms)
    if (radio.write(payload, 15))
      break;
    retries--;      
  }
  
  if (retries == 0)
    Serial.println("Failed to send data to gateway."); 
  else
    Serial.print("Data sent to gatway. Retries="); Serial.println(5 - retries);
  
  
  // Power down 
  radio.powerDown();
  
  // Disconnect power from sensors.
  digitalWrite(PIN_RELAY, LOW);
}

//
// Code for setting up the data gateway client.
//
void setup_gateway_client()
{
  //radio.begin();  
  //delay(5);
  radio.enableDynamicPayloads();
  radio.setAutoAck(true);                 // Automatic acknowledgement.  
  radio.setDataRate(RF24_250KBPS);        // Default: RF24_1MBPS.   Value: RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  radio.setChannel(1);                    // Default: 76            Minimum 1. Maximum 125.  
  radio.setPayloadSize(32);
  //radio.setPALevel(RF24_PA_MAX);          // Maximum transmission power.
  //radio.setCRCLength(RF24_CRC_16);        // Default: RF24_CRC_16.  Other values: RF24_CRC_DISABLED = 0, RF24_CRC_8
  //radio.setRetries(5, 15);                // Default: 5, 15         5 usec delay, 15 retries.
  //radio.setPayloadSize(0);               // Default: 32.           Maximum 32 bytes. 0 means dynamic payload size.
  
  //delay(5);
  radio.openWritingPipe(gateway_addr);    // Send to address.
  radio.openReadingPipe(1, sensor_addr);  // Receive address.  
  radio.stopListening();                  
 
  Serial.println("RF24 Details:");
  radio.printDetails(); 

  payload[0] = protocol;
  radio.powerDown();
  
}

//
// Code for handling RTC alarm, interrupts, and wake-up from sleep code.
//
void print_alarm_date_time()
{
  time_t dt;

  dt = RTC.get();
  Serial.print("ALARM_1: ");     
  printDateTime(dt);
  Serial.println();  
}

void set_next_alarm()
{
    int nm, ns;

    // Get next minute and seconds;
    nm = next_minute();
    ns = next_second();
    Serial.print("Next alarm: *:"); Serial.print(nm); Serial.print(":"); Serial.print(ns); Serial.println();
    RTC.setAlarm(ALM1_MATCH_MINUTES, ns, nm, 0, 1);    
}


// Function called on interrupt from RTC.
void wakeUp() 
{  
  Serial.println("Interrupt!");
}

// Function to calculate next minute for the alarm.
int next_minute()
{  
  time_t dt;
  int m, nm;

  // Get time from RTC.
  dt = RTC.get();

  // Extract current minute.
  m = minute(dt);

  // Calculate next minute.
  // nm = ((int)((m + 1)/1.0 + 0.5)) % 60 * 1 - 1;       // Every minute
  // nm = ((int)((m + 2)/2.0 + 0.5)) % 30 * 2 - 1;    // Every 2 minutes
  // nm = ((int)((m + 5)/5.0 + 0.5)) % 12 * 5 - 1;    // Every 5 minutes
  // nm = ((int)((m + 10)/10.0 + 0.5)) % 6 * 10 - 1;  // Every 10 minutes
  nm = ((int)((m + 15)/15.0 + 0.5)) % 4 * 15 - 1;  // Every 15 minutes
  // nm = ((int)((m + 30)/30.0 + 0.5)) % 2 * 30 - 1;  // Every 30 minutes

  // Return next minute.
  return nm >= 0 ? nm : 60 + nm;
}

int next_second()
{
  // Always set the alarm 2 seconds before the minute to allow
  // us to read sensors and send values on the next minute.
  return 58;  
}

//
// Code for synchronizing the current date/time from a time server.
//

void setupDateTimeClient() 
{
  radio.begin();  
  radio.setPALevel(RF24_PA_MAX);        // Maximum transmission power.
  radio.setAutoAck(true);               // Automatic acknowledgement.
  
  radio.setDataRate(RF24_250KBPS);      // Default: RF24_1MBPS.   Value: RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  radio.setChannel(1);                  // Default: 76            Minimum 1. Maximum 125.  
  radio.setPayloadSize(10);             // Default: 32.           Maximum 32 bytes. 0 means dynamic payload size.
  radio.setCRCLength(RF24_CRC_16);      // Default: RF24_CRC_16.  Other values: RF24_CRC_DISABLED = 0, RF24_CRC_8
  radio.setRetries(5, 15);              // Default: 5, 15         5 usec delay, 15 retries.
  radio.openWritingPipe(time_svr_addr);
  radio.openReadingPipe(1, time_cln_addr);
  radio.stopListening();
  
  Serial.println("RF24 Details:");
  radio.printDetails();  
}

void setupRTC()
{
  // Setup RTC with basic defaults.
  RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
  RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_1, false);
  RTC.alarmInterrupt(ALARM_2, false);
  RTC.squareWave(SQWAVE_NONE);
}

time_t receiveDateTime() 
{
  tmElements_t tm;
  time_t result;
  boolean timed_out = false;
  unsigned long started;
  int timeouts = 0;

  // Start RF24 radio listining ...
  radio.startListening();
  
  while (true) 
  {

    // Prepare checking if data has been received.
    timed_out = false;
    started = millis();

    // Check if data has been received. Timeout after 200 ms.
    while (!radio.available()) 
    {
      if (millis() - started > 200) 
      {
          timed_out = true;
          break;
      }
    }

    if (!timed_out) 
    {
      // Data has been received, so copy them into payload buffer.
      radio.read(&payload, sizeof(payload));

      // Check the signature "tim" (0x74, 0x69, 0x6d)
      if (payload[0] == 0x74 && payload[1] == 0x69 && payload[2] == 0x6d) 
      {

        // Create date/time structure.
        tm.Year = (payload[3] << 8 | payload[4]) - 1970;
        tm.Month = payload[5];
        tm.Day = payload[6];
        tm.Hour = payload[7];
        tm.Minute = payload[8];
        tm.Second = payload[9];

        // Make time_t
        result = makeTime(tm);
        
        Serial.print("\nReceived: "); printDateTime(result);
        Serial.println();

        // Return result.
        return result;    
      } 
      else
      {
        Serial.println("\nInvalid payload received.");
      }
    }
    else
    {
      Serial.print(".");
      timeouts++;
      if (timeouts >= 25)
      {
        Serial.println();
        timeouts = 0;
      }
    }
  } 
} 

void updateRTCDateTime(time_t dt)
{
  // Set RTC clock to the specified date/time.
  RTC.set(dt);  
}

void printDateTime(time_t t)
{
    // Print year.
    Serial.print(year(t)); Serial.print("-");
    Serial.print(month(t) < 10 ? "0" : ""); Serial.print(month(t)); Serial.print("-");
    Serial.print(day(t) < 10 ? "0" : ""); Serial.print(day(t)); Serial.print(" ");

    // Print time.
    Serial.print(hour(t) < 10 ? "0" : ""); Serial.print(hour(t)); Serial.print(':');
    Serial.print(minute(t) < 10 ? "0" : ""); Serial.print(minute(t)); Serial.print(':');
    Serial.print(second(t) < 10 ? "0" : ""); Serial.print(second(t));
}