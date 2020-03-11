# Arduino Low Power Sensor Template Project

This project is intended as a template project for low power ATmega328P based solutions collecting data from sensors and sending values to a data collection gateway using 2.4 GHz wireless communication based on NRF24L01 modules.

Some specific design criterias have been applied to make the project suitable for low power scenarios.  

First of all the design is based on using a DS3231 to allow the CPU to be waked up after being placed in deep sleep, and the NRF24L01 module is powered off while not actively transmitting sensor readings.  Likewise, the design uses a PIN to cut off power to sensors, like for example a DHT22 temperature and humidity sensor, to save power while the sensor is not being used.

The sketch uses the [nrf24-time-client](https://github.com/bjarne-hansen/nrf24-time-client) code to allow the sensor solution to receive date/time from a [nrf24-time-server](https://github.com/bjarne-hansen/nrf24-time-server). Basically, this allows a low power sensor solution to receive date/time using NRF24 communication much like internet connected solutions can receive date/time information via NTP.

The wiring below targets an Arduino Nano that is used for testing the sketch.  A real world implementation will use additional components such as voltage regulators, battery packs, solar cells, battery chargers, and can with good results be based on "bare-bone" ATmega328P configurations.  For further inspiration on such solutions, please refer to ...




 
    Wiring:

    I always use the same colour code for the wires connecting the RF24 modules. 
    It seems that the colours below are popular in many articles on the RF24 module.

    NRF24L01, PIN side.
    +---+----------------------
    |       *    *
    +---+
    |7|8|   purple |
    +-+-+
    |5|6|   green  |  blue
    +-+-+
    |3|4|   yellow | orange
    +-+-+   
    |1|2|   black  |  red
    +-+-+----------------------
 
    The following layout is for the Arduino NANO.
 
         RF24L01                 NANO
    -------------------------------------
    PIN DESC  COLOR           PIN   GPIO
    1   GND   black   <--->   GND    -
    2   3.3V  red     <--->   3V3    -
    3   CE    yellow  <--->   14     10 
    4   CSN   orange  <--->   13      9 
    5   SCKL  green   <--->   17     13    
    6   MOSI  blue    <--->   15     11 
    7   MISO  purple  <--->   16     12 
    8   IRQ           <--->   N/C    - 
    
      DS3231     NANO
    ----------------------
     GND  <-->   -  GND  
     VCC  <-->   -  3V3
     SDA  <-->  27   A4
     SCL  <-->  28   A5
 
