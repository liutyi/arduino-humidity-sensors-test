#include <SPI.h>
#include <SD.h>
#include <UTFT.h>
#include <DFRobot_I2CMultiplexer.h>
#include <Wire.h>
//SHT2x
#include <Sodaq_SHT2x.h>
//SHT3x
#include <SHTSensor.h>
SHTSensor sht3x(SHTSensor::SHT3X);
// Adafruit BME680
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
Adafruit_BME680 bme680l0;
Adafruit_BME680 bme680l1;
Adafruit_BME680 bme680l5;
Adafruit_BME680 bme680l6;
// Adafruit BME280
#include <Adafruit_BME280.h>
Adafruit_BME280 bme280l2;
Adafruit_BME280 bme280l3;
Adafruit_BME280 bme280l4;
// DHT12
#include <DHT12.h>
DHT12 dht12;


// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t SevenSegNumFont[];
extern uint8_t BigFont[];

/*Create an I2CMultiplexer object, the address of I2CMultiplexer is 0x70*/
DFRobot_I2CMultiplexer I2CMultiplexer(0x70);

// SCREEN Type and PINs
UTFT LCD(ILI9486,38,39,40,41);

// SD Card PIN
const int SDCARD = 53;

// Variables
int x;
int y;
int i2cPort;
char i;

void setupSD ()
{
  LCD.clrScr();
  LCD.setColor(255, 255, 255);
  LCD.setBackColor(0, 0, 0);
  LCD.setFont(BigFont);
// Setup SD
  LCD.print("Initializing SD card...", LEFT, 1);
  pinMode(SDCARD, OUTPUT);
  if (!SD.begin(SDCARD)) {
    LCD.print("Card failed, or not present", LEFT, 18);
    return;
  }
  LCD.print("card initialized.", LEFT, 18);
}

void drawTable ()
{
  LCD.setFont(SmallFont);
  LCD.clrScr();

  // RED header
  LCD.setColor(80, 80, 80);
  LCD.fillRect(0, 0, 479, 13);
  // Gray Footer
  LCD.setColor(60, 70, 80);
  LCD.fillRect(0, 306, 479, 319);
  // Header Text (White)
  LCD.setColor(255, 255, 255);
  LCD.setBackColor(80, 80, 80);
  LCD.print("RH/t sensor comparision sketch", CENTER, 1);
  // Footer Text (Yellow)
  LCD.setBackColor(64, 64, 64);
  LCD.setColor(255,255,0);
  LCD.print("https://wiki.liutyi.info/", CENTER, 307);
  // Table title
  LCD.setBackColor(0, 0, 0);
  LCD.setColor(150,150,150);
  LCD.setFont(BigFont);
  LCD.print("     SHT2 SHT3 BMEx DHTx OTHr", CENTER, 18);
  for (uint8_t port=1; port<9; port++) {
    x=20;
    y=18+(28*port);
    LCD.printNumI(port, x, y);
  }
  // Gray Frame
  LCD.setColor(60, 60, 60);
  LCD.drawRect(0, 14, 479, 305);
  //Draw Grid and header text
  for (int y=14; y<270; y+=28)
    LCD.drawLine(1, y, 479, y);
  for (int x=79; x<479; x+=80)
    LCD.drawLine(x, 14, x, 266);
}

void initSensors ()
{
  LCD.setBackColor(0, 0, 0);
  LCD.setColor(100,100,0);
  LCD.setFont(BigFont);
  sht3x.init();
  for (uint8_t port=0; port<8; port++) {
    uint8_t* dev = I2CMultiplexer.scan(port);
    while(*dev){
      i2cPort=*dev;
      // SHT2x
      if ( i2cPort == 64 ) {
       i=1; 
      }
      // SHT3x
      if ( i2cPort == 68 ) {
      i=2;
      sht3x.setAccuracy(SHTSensor::SHT_ACCURACY_HIGH);
      }
      // BMEx80
      if ( (i2cPort == 118) || (i2cPort == 119) ) {
       i=3;
       switch (port) {
         case 0:
           bme680l0.begin(i2cPort);
           break;
         case 1:
           bme680l1.begin(i2cPort);
           break;
         case 2:
           bme280l2.begin(i2cPort);
           break;
         case 3:
           bme280l3.begin(i2cPort);
           break;
         case 4:
           bme280l4.begin(i2cPort);
           break;
         case 5:
           bme680l5.begin(i2cPort);
           break;                                       
         case 6:
           bme680l6.begin(i2cPort);
           break;           
       }
      }
      // DHTxx
      if ( i2cPort == 92 ) {
       i=4;
       dht12.begin();
      }
      x=20+(i*80); y=46+(28*port);
      LCD.printNumI (i2cPort, x, y);
      dev++;
    }
  }  
}

void readSensors ()
{
  float hum;
  float temp;
    LCD.setFont(SmallFont);
    LCD.setBackColor(0, 0, 0);
    for (uint8_t port=0; port<8; port++) {
    uint8_t* dev = I2CMultiplexer.scan(port);
    while(*dev){
      i2cPort=*dev;
      hum=0;
      temp=0;
      // SHT2x
      if ( i2cPort == 64 ) {
       i=1;
       hum=SHT2x.GetHumidity();
       temp=SHT2x.GetTemperature(); 
      }
      // SHT3x
      if ( i2cPort == 68 ) {
      i=2;
      if (sht3x.readSample()) {
       hum=sht3x.getHumidity();
       temp=sht3x.getTemperature();
       }
      }
      // BMEx80
      if ( (i2cPort == 118) || (i2cPort == 119) ) {
       i=3;
       switch (port) {
         case 0:
           hum=bme680l0.readHumidity();
           temp=bme680l0.readTemperature();
           break;
         case 1:
           hum=bme680l1.readHumidity();
           temp=bme680l1.readTemperature();
           break;
         case 2:
           hum=bme280l2.readHumidity();
           temp=bme280l2.readTemperature();
           break;
         case 3:
           hum=bme280l3.readHumidity();
           temp=bme280l3.readTemperature();
           break;
         case 4:
           hum=bme280l4.readHumidity();
           temp=bme280l4.readTemperature();
           break;
         case 5:
           hum=bme680l5.readHumidity();
           temp=bme680l5.readTemperature();
           break;                                       
         case 6:
           hum=bme680l6.readHumidity();
           temp=bme680l6.readTemperature();
           break;           
       }
      }
      // DHTxx
      if ( i2cPort == 92 ) {
       i=4;
       hum=dht12.readHumidity();
       temp=dht12.readTemperature();
      }
      x=(i*80); y=44+(28*port);
      LCD.setColor(0,0,255);
      LCD.printNumF (hum, 2, x, y, '.', 5);
      x=38+(i*80); y=12+46+(28*port);
      LCD.setColor(255,255,0);
      LCD.printNumF (temp, 2, x, y, '.', 5);
      dev++;
    }
  }  
delay (2000);
}

void setup()
{
 int x;
 int y;
 char i;
// Setup the LCD
  LCD.InitLCD();

  setupSD ();
// Clear the screen and draw the frame
  drawTable ();
  sht3x.init ();
  initSensors ();
  delay(1000);
  drawTable ();

}

void loop()
{
  //drawTable ();
  readSensors ();
  delay(1000);
}
