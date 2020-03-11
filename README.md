# nrf24-low-power-sensor


 
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
 
