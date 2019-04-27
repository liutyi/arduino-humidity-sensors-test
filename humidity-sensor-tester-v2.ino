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
uint8_t multiplexer[4] = {112, 113, 114, 115};
// Type of sensor
const uint8_t SHT2X = 0; /* include SHT20, SHT21, SHT25, HTU21d*/
const uint8_t SI70XX = 1; /* includes Si7021 */
const uint8_t HDC10xx = 2; /* includes HDC1080 */
const uint8_t SHT3X = 3; /* include SHT30, SHT31, SHT35, SHT88*/
const uint8_t BME280 = 4; /* includes BME280 */
const uint8_t BME680 = 5; /* includes BME680 */
const uint8_t DHT1X = 6; /* includes DHT12 */
const uint8_t DHT2X = 7; /* includes DHT22 */


/*Create an I2CMultiplexer object, the address of I2CMultiplexer is 0x70*/
DFRobot_I2CMultiplexer I2CMultiplexer(multiplexer[0]);

// SCREEN Type and PINs
UTFT LCD(ILI9486, 38, 39, 40, 41);

// SD Card PIN
const int SDCARD = 53;
File logfile1;
File logfile2;

// Set header (sensor names) for csv file. leave empty if intend to substitute sensors time-to-time without firmware update
String csvheader = "Time,SHT25,SHT35,BME680-1,SHT21,SHT35,BME680-1,SHT21,SHT31-2,BME280,SHT21-CJMCU,SHT31-2,BME280,SHT21-CJMCU,SHT31,BME280,SHT20,SHT31,DHT12,SHT20,SHT30,DHT12,SHT20,SHT30,DHT12,";
// Variables for file rotation 00-99
#define T_FILE_BASE_NAME "temprt"
#define RH_FILE_BASE_NAME "humidt"
const uint8_t T_BASE_NAME_SIZE = sizeof(T_FILE_BASE_NAME) - 1;
const uint8_t RH_BASE_NAME_SIZE = sizeof(RH_FILE_BASE_NAME) - 1;
char tFileName[] = T_FILE_BASE_NAME "00.csv";
char rhFileName[] = RH_FILE_BASE_NAME "00.csv";


// Other Variables
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
  if (!SD.begin(SDCARD/*, SPI_HALF_SPEED*/)) {
    LCD.print("Card failed, or not present", LEFT, 18);
    return;
  } else {
    LCD.print("card initialized.", LEFT, 18);
    while (SD.exists(tFileName)) {
      if (tFileName[T_BASE_NAME_SIZE + 1] != '9') {
        tFileName[T_BASE_NAME_SIZE + 1]++;
      } else if (tFileName[T_BASE_NAME_SIZE] != '9') {
        tFileName[T_BASE_NAME_SIZE + 1] = '0';
        tFileName[T_BASE_NAME_SIZE]++;
      } else {
        LCD.print("Can't generate temperature file name", LEFT, 36);
        return;
      }
      while (SD.exists(rhFileName)) {
        if (rhFileName[RH_BASE_NAME_SIZE + 1] != '9') {
          rhFileName[RH_BASE_NAME_SIZE + 1]++;
        } else if (rhFileName[RH_BASE_NAME_SIZE] != '9') {
          rhFileName[RH_BASE_NAME_SIZE + 1] = '0';
          rhFileName[RH_BASE_NAME_SIZE]++;
        } else {
          LCD.print("Can't generate humidity file name", LEFT, 36);
          return;
        }
      }
    }
  }
  // CSV Header
  logfile1 = SD.open(rhFileName, FILE_WRITE );
  logfile1.println(csvheader);
  logfile1.close();
  logfile2 = SD.open(tFileName, FILE_WRITE );
  logfile2.println(csvheader);
  logfile2.close();
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
  LCD.print("Humidity sensors tester v2", CENTER, 1);
  // Footer Text (Yellow)
  LCD.setBackColor(64, 64, 64);
  LCD.setColor(255, 255, 0);
  LCD.print("https://wiki.liutyi.info/", CENTER, 307);
  // Table title
  LCD.setBackColor(0, 0, 0);
  LCD.setColor(150, 150, 150);
  LCD.setFont(BigFont);
  LCD.print("SHT2 SHT3 BME+ Si7x HDCx SHT8", CENTER, 18);
  // Gray Frame
  LCD.setColor(60, 60, 60);
  LCD.drawRect(0, 14, 479, 305);
  //Draw Grid and header text
  for (int y = 14; y < 270; y += 28)
    LCD.drawLine(1, y, 479, y);
  for (int x = 79; x < 479; x += 80)
    LCD.drawLine(x, 14, x, 266);
}

void initSensors ()
{
  LCD.setBackColor(0, 0, 0);
  LCD.setColor(100, 100, 0);
  LCD.setFont(BigFont);
  sht3x.init();
  for (uint8_t port = 0; port < 8; port++) {
    uint8_t* dev = I2CMultiplexer.scan(port);
    while (*dev) {
      i2cPort = *dev;
      // SHT2x
      if ( i2cPort == 64 ) {
        i = 0;
      }
      // SHT3x
      if ( i2cPort == 68 ) {
        i = 1;
        sht3x.setAccuracy(SHTSensor::SHT_ACCURACY_HIGH);
      }
      // BMEx80
      if ( (i2cPort == 118) || (i2cPort == 119) ) {
        i = 2;
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
        i = 2;
        dht12.begin();
      }
      x = 20 + (i * 80); y = 46 + (28 * port);
      LCD.printNumI (i2cPort, x, y);
      dev++;
    }
  }
}

void readSensors ()
{
  float hum = 0;
  float temp = 0;
  long seconds;
  String csvline1 = "";
  String csvline2 = "";

  //uptime in seconds
  seconds = millis() / 1000;
  LCD.setFont(SmallFont);
  LCD.setBackColor(64, 64, 64);
  LCD.setColor(0, 0, 0);
  LCD.printNumI (seconds, 2, 307);
  LCD.setBackColor(0, 0, 0);

  for (uint8_t port = 0; port < 8; port++) {
    uint8_t* dev = I2CMultiplexer.scan(port);
    while (*dev) {
      i2cPort = *dev;
      hum = 0;
      temp = 0;
      // SHT2x
      if ( i2cPort == 64 ) {
        i = 0;
        hum = SHT2x.GetHumidity();
        temp = SHT2x.GetTemperature();
      }
      // SHT3x
      if ( i2cPort == 68 ) {
        i = 1;
        if (sht3x.readSample()) {
          hum = sht3x.getHumidity();
          temp = sht3x.getTemperature();
        }
      }
      // BMEx80
      if ( (i2cPort == 118) || (i2cPort == 119) ) {
        i = 2;
        switch (port) {
          case 0:
            hum = bme680l0.readHumidity();
            temp = bme680l0.readTemperature();
            break;
          case 1:
            hum = bme680l1.readHumidity();
            temp = bme680l1.readTemperature();
            break;
          case 2:
            hum = bme280l2.readHumidity();
            temp = bme280l2.readTemperature();
            break;
          case 3:
            hum = bme280l3.readHumidity();
            temp = bme280l3.readTemperature();
            break;
          case 4:
            hum = bme280l4.readHumidity();
            temp = bme280l4.readTemperature();
            break;
          case 5:
            hum = bme680l5.readHumidity();
            temp = bme680l5.readTemperature();
            break;
          case 6:
            hum = bme680l6.readHumidity();
            temp = bme680l6.readTemperature();
            break;
        }
      }
      // DHTxx
      if ( i2cPort == 92 ) {
        i = 2;
        hum = dht12.readHumidity();
        temp = dht12.readTemperature();
      }
      x = (i * 80); y = 44 + (28 * port);
      LCD.setColor(0, 0, 255);
      LCD.printNumF (hum, 2, x, y, '.', 5);
      x = 38 + (i * 80); y = 12 + 46 + (28 * port);
      LCD.setColor(255, 255, 0);
      LCD.printNumF (temp, 2, x, y, '.', 5);
      csvline1  += String(hum) + ",";
      csvline2  += String(temp) + ",";
      dev++;
    }
  }
  Serial.println(csvline1);
  //Serial.println(csvline2);
  delay (200);
  // Write to humidity log file
  logfile1 = SD.open(rhFileName, FILE_WRITE );
  if (logfile1) {
    csvline1 = String(seconds) + "," + csvline1;
    logfile1.println(csvline1);
    LCD.print(csvline1, LEFT, 280);
    logfile1.close();
  } else {
    LCD.print("Cannot write RH to file", LEFT, 280);
  }
  // Write to temperatures log file
  logfile2 = SD.open(tFileName, FILE_WRITE );
  if (logfile2) {
    csvline2 = String(seconds) + "," + csvline2;
    logfile2.println(csvline2);
    LCD.print(csvline2, LEFT, 290);
    logfile2.close();
  } else {
    LCD.print("Cannot write t to file", LEFT, 290);
  }
  delay (800);
}

void stop_multiplexors()
{
  for (uint8_t port = 112; port < 119; port++) {
    Wire.beginTransmission(port);
    Wire.write(0);
    Wire.endTransmission();
  }
}


void setup()
{
  int x;
  int y;
  char i;
  // Setup the LCD
  LCD.InitLCD();
  //  LCD.lcdOff();
  //  LCD.lcdOn();
  //  LCD.setContrast(64);
  //  LCD.setBrightness(16);
  //  LCD.fillScr(90,90,90);

  Serial.begin(115200);
  setupSD ();
  stop_multiplexors();
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
