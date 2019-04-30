#include <SPI.h>
#include <SD.h>
#include <UTFT.h>
#include <Wire.h>

// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t SevenSegNumFont[];
extern uint8_t BigFont[];

// SCREEN Type and PINs
UTFT LCD(ILI9486, 38, 39, 40, 41);

// SD Card PIN
const int SDCARD = 53;
File logfile1;
File logfile2;

// Screen customiztion
const String appname = "Humidity sensors tester v2.1";
const String header = "SHT2 SHT3 BME+ Si7x HDCx SHT8";
const String footer = "https://wiki.liutyi.info/";

// Set header (sensor names) for csv file. leave empty if intend to substitute sensors time-to-time without firmware update
const String csvheader = "Time,SHT25,SHT35,BME680-1,SHT21,SHT35,BME680-1,SHT21,SHT31-2,BME280,SHT21-CJMCU,SHT31-2,BME280,SHT21-CJMCU,SHT31,BME280,SHT20,SHT31,DHT12,SHT20,SHT30,DHT12,SHT20,SHT30,DHT12,Si7021-1,Si7021-1,Si7021-2,Si7021-2,Si7021-3,Si7021-3,Si7021-4,HTU21d,HDC1080-CJMCU,SHT31-2,HDC1080-CJMCU,SHT31-2,HDC1080-CJMCU,SHT85,HDC1080-CJMCU,SHT85,HDC1080-CJMCU,SHT85,HDC1080,HDC1080,BME680-2,HDC1080,BME680-2";

// Variables for file rotation 00-99
#define t_base_name "temprt"
#define rh_base_name "humidt"
const uint8_t t_base_name_size = sizeof(t_base_name) - 1;
const uint8_t rh_base_name_size = sizeof(rh_base_name) - 1;
char tFileName[] = t_base_name "00.csv";
char rhFileName[] = rh_base_name "00.csv";

// Define number and addresses of multiplexers
uint8_t multiplexer[3] = {112, 113, 114};

// Type of sensor
const uint8_t EMPTY = 0; /* slot is empty or sensor disabled */
const uint8_t SHT2X = 1; /* include SHT20, SHT21, SHT25, HTU21d*/
const uint8_t SI70XX = 2; /* includes Si7021 */
const uint8_t HDC10xx = 3; /* includes HDC1080 */
const uint8_t SHT3X = 4; /* include SHT30, SHT31, SHT35, SHT88*/
const uint8_t BME280 = 5; /* includes BME280 */
const uint8_t BME680 = 6; /* includes BME680 */
const uint8_t DHT1X = 7; /* includes DHT12 */
const uint8_t DHT2X = 8; /* includes DHT22 */
const uint8_t AHT1X = 9; /* includes AHT10 */

// indexes name in sensor arrays
const uint8_t get_type = 0; /* indexes name in sensor arrays */
const uint8_t get_collumn = 1; /* indexes name in sensor arrays */
const uint8_t get_address = 2; /* indexes name in sensor arrays */
const uint8_t UNDEF = 255; /* sensor have no position on display */
const uint8_t NOCOLM = 254; /* do not display this sensor */

// Sensor properties by [multiplexor][i2c_bus][number][get_type/get_collumn/get_address]
const uint8_t sensor[3][8][3][3] =
{
  {
    {  {SHT2X, 1, 64}, {SHT3X, 2, 68}, {BME680, 3, 119} },
    {  {SHT2X, 1, 64}, {SHT3X, 2, 68}, {BME680, 3, 119} },
    {  {SHT2X, 1, 64}, {SHT3X, 2, 68}, {BME280, 3, 118} },
    {  {SHT2X, 1, 64}, {SHT3X, 2, 68}, {BME280, 3, 118} },
    {  {SHT2X, 1, 64}, {SHT3X, 2, 68}, {BME280, 3, 118} },
    {  {SHT2X, 1, 64}, {SHT3X, 2, 68}, {DHT1X, 3, 92} },
    {  {SHT2X, 1, 64}, {SHT3X, 2, 68}, {DHT1X, 3, 92} },
    {  {SHT2X, 1, 64}, {SHT3X, 2, 68}, {DHT1X, 3, 92} }
  },
  {
    {  {SHT2X, 4, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {SHT2X, 4, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {SHT2X, 4, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {SHT2X, 4, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {SHT2X, 4, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {SHT2X, 4, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {SHT2X, 4, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {SHT2X, 4, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} }
  },
  {
    {  {HDC10xx, 5, 64}, {BME680, UNDEF, 118}, {EMPTY, UNDEF, 0} },
    {  {HDC10xx, 5, 64}, {BME680, UNDEF, 118}, {EMPTY, UNDEF, 0} },
    {  {HDC10xx, 5, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {HDC10xx, 5, 64}, {SHT3X, 6, 68}, {EMPTY, UNDEF, 0} },
    {  {HDC10xx, 5, 64}, {SHT3X, 6, 68}, {EMPTY, UNDEF, 0} },
    {  {HDC10xx, 5, 64}, {SHT3X, 6, 68}, {EMPTY, UNDEF, 0} },
    {  {HDC10xx, 5, 64}, {SHT3X, 6, 68}, {EMPTY, UNDEF, 0} },
    {  {HDC10xx, 5, 64}, {SHT3X, 6, 68}, {EMPTY, UNDEF, 0} }
  }/*,
  {
    {  {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} }
  }*/
};


// Other Variables
int x;
int y;

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
      if (tFileName[t_base_name_size + 1] != '9') {
        tFileName[t_base_name_size + 1]++;
      } else if (tFileName[t_base_name_size] != '9') {
        tFileName[t_base_name_size + 1] = '0';
        tFileName[t_base_name_size]++;
      } else {
        LCD.print("Can't generate temperature file name", LEFT, 36);
        return;
      }
      while (SD.exists(rhFileName)) {
        if (rhFileName[rh_base_name_size + 1] != '9') {
          rhFileName[rh_base_name_size + 1]++;
        } else if (rhFileName[rh_base_name_size] != '9') {
          rhFileName[rh_base_name_size + 1] = '0';
          rhFileName[rh_base_name_size]++;
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
  LCD.print(appname, CENTER, 1);
  // Footer Text (Yellow)
  LCD.setBackColor(64, 64, 64);
  LCD.setColor(255, 255, 0);
  LCD.print(footer, CENTER, 307);
  // Table title
  LCD.setBackColor(0, 0, 0);
  LCD.setColor(150, 150, 150);
  LCD.setFont(BigFont);
  LCD.print(header, CENTER, 18);
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
  uint8_t type;
  uint8_t addr;
  uint8_t colm;
  uint8_t imux;
  uint8_t ibus;
  uint8_t idev;
  LCD.setBackColor(0, 0, 0);
  LCD.setColor(100, 100, 0);
  LCD.setFont(BigFont);
  for (imux = 0; imux < sizeof(multiplexer); imux++)
  {
    for (ibus = 0; ibus < 8; ibus++)
    {
      choose_i2c_bus(imux, ibus + 1);
      Serial.print ("choose_i2c_bus\t");
      Serial.print (imux);
      Serial.print (",");
      Serial.println (ibus + 1);
      for (idev = 0; idev < 3; idev++)
      {
        type = sensor[imux][ibus][idev][get_type];
        addr = sensor[imux][ibus][idev][get_address];
        colm = (sensor[imux][ibus][idev][get_collumn] - 1);
        if (type != EMPTY) {
          init_sensor(type, addr);
          Serial.print ("init_sensor\t");
          Serial.print (type);
          Serial.print (",");
          Serial.println (addr);
        }
        if (colm != NOCOLM) {
          x = 20 + (colm * 80); y = 46 + (28 * ibus);
          LCD.printNumI (type, x, y);
        }
      }
    }
  }
}

void choose_i2c_bus(uint8_t smux, uint8_t sbus) {
  for (uint8_t i = 0; i < sizeof(multiplexer); i++) {
    uint8_t addr = multiplexer[i];
    Wire.beginTransmission(addr);
    if ( i == smux ) {
      Wire.write(sbus);
    } else {
      Wire.write(0);
    }
    Wire.endTransmission();
    delay (10);
  }
}

bool init_sensor (uint8_t type, uint8_t addr)
{
  const uint16_t SHT3X_CMD_SIZE         = 2;
  const uint16_t SHT3X_RESET            = 0x30A2;
  const uint16_t SHT3X_HEATER_OFF       = 0x3066;
  const uint16_t SHT3X_CLEAR_STATUS     = 0x3041;
  const uint8_t SHT2X_RESET             = 0xFE;
  const uint8_t SHT_RESET_DURATION      = 15;
  Wire.beginTransmission(addr);
  if (type == SHT2X) {

    Wire.write(SHT2X_RESET);
    delay(SHT_RESET_DURATION);
  }
  if (type == SHT3X) {
    uint8_t cmd[SHT3X_CMD_SIZE];
    cmd[0] = SHT2X_RESET >> 8;
    cmd[1] = SHT2X_RESET & 0xff;
    for (int i = 0; i < SHT3X_CMD_SIZE; i++) {
      Wire.write(cmd[i]);
    }
    delay(SHT_RESET_DURATION);
    /*
      for (int i = 0; i < SHT3X_CMD_SIZE; i++) {
      Wire.write(*SHT3X_HEATER_OFF[i]);
      }
      for (int i = 0; i < SHT3X_CMD_SIZE; i++) {
      Wire.write(*SHT3X_CLEAR_STATUS[i]);
      }
    */

  }
  Wire.endTransmission();
}
const uint8_t  SHT2X_READ_T      = 0xE3;   // hold master
const uint8_t  SHT2X_READ_RH     = 0xE5; // hold master
const uint16_t SHT3X_READ_ALL    = 0x2400; /* HIGH=0x2400 MID=0x240b LOW=0x2416*/
const uint8_t  SHT2X_DATA_SIZE   = 3;
const uint8_t  SHT3X_DATA_SIZE   = 6;
const uint8_t  SHT3X_CMD_SIZE    = 2;
const uint8_t  SHT3X_MEASUREMENT_DELAY   = 15;     /* HIGH = 15 MID=6 LOW=4 */
const uint8_t  SHT2X_MEASUREMENT_DELAY   = 100;
uint8_t sht3data[SHT3X_DATA_SIZE] = {0, 0, 0, 0, 0, 0};
uint8_t sht3cmd[SHT3X_CMD_SIZE] = {0, 0};
uint8_t sht2data[SHT2X_DATA_SIZE] = {0, 0, 0};

float get_humidity (uint8_t type, uint8_t addr)
{
  float xhum = 0;
  uint16_t result = 0;
  if (type == SHT2X) {
    Wire.beginTransmission(addr);
    Serial.println ("SHT2X - case");
    Wire.write(SHT2X_READ_RH);
    Wire.endTransmission();
    delay(SHT2X_MEASUREMENT_DELAY);
    Wire.requestFrom(addr, SHT2X_DATA_SIZE);
    while (Wire.available() < SHT2X_DATA_SIZE) {
      delay(SHT2X_MEASUREMENT_DELAY);
    }
    for (int i = 0; i < SHT2X_DATA_SIZE; i++) {
      sht2data[i] = Wire.read();
    }
    result = (sht2data[1] << 8) + sht2data[2];
    result &= ~0x0003;
    xhum = (-6.0 + (125.0 / 65536.0) * (float)result);
    Serial.println ("SHT2X - case2");

    Serial.println ("SHT2X - case3");

  }
  if ( type == SHT3X) {
    Serial.println ("SHT3X - case");
    uint16_t val = 0;
    sht3cmd[0] = SHT3X_READ_ALL >> 8;
    sht3cmd[1] = SHT3X_READ_ALL & 0xff;
    Wire.beginTransmission(addr);
    for (int i = 0; i < SHT3X_CMD_SIZE; i++) {
      Wire.write(sht3cmd[i]);
    }
    Wire.endTransmission();
    delay(SHT3X_MEASUREMENT_DELAY);
    Wire.requestFrom(addr, SHT3X_DATA_SIZE);
    while (Wire.available() < SHT3X_DATA_SIZE) {
      delay(SHT3X_MEASUREMENT_DELAY);
    }

    for (int i = 0; i < SHT3X_DATA_SIZE; i++) {
      sht3data[i] = Wire.read();
    }
    xhum = (sht3data[3] << 8) + sht3data[4];
    xhum *= 100;
    xhum /= 65535;
  }
  return xhum;
}

float get_temperature (uint8_t type, uint8_t addr) {
  float xtemp = 0;
  uint16_t result = 0;
  if (type == SHT2X) {
    Wire.beginTransmission(addr);
    Wire.write(SHT2X_READ_T);
    Wire.endTransmission();
    delay(SHT2X_MEASUREMENT_DELAY);
    Wire.requestFrom(addr, SHT2X_DATA_SIZE);
    while (Wire.available() < SHT2X_DATA_SIZE) {
      delay(SHT2X_MEASUREMENT_DELAY);
    }
    for (int i = 0; i < SHT2X_DATA_SIZE; i++) {
      sht2data[i] = Wire.read();
    }
    result = (sht2data[1] << 8) + sht2data[2];
    result &= ~0x0003;
    xtemp = (-46.85 + (175.72 / 65536.0) * (float)result);
    return xtemp;

  }
  if (type == SHT3X) {
    uint16_t val;
    sht3cmd[0] = SHT3X_READ_ALL >> 8;
    sht3cmd[1] = SHT3X_READ_ALL & 0xff;
    Wire.beginTransmission(addr);
    for (int i = 0; i < SHT3X_CMD_SIZE; i++) {
      Wire.write(sht3cmd[i]);
    }
    Wire.endTransmission();
    delay(SHT3X_MEASUREMENT_DELAY);
    Wire.requestFrom(addr, SHT3X_DATA_SIZE);
    while (Wire.available() < SHT3X_DATA_SIZE) {
      delay(SHT3X_MEASUREMENT_DELAY);
    }
      for (int i = 0; i < SHT3X_DATA_SIZE; i++) {
        sht3data[i] = Wire.read();
      }
      xtemp = (sht3data[0] << 8) + sht3data[1];
      xtemp *= 175;
      xtemp /= 65535;
      xtemp -= 45;


  }

  return xtemp;
}
void readSensors ()
{
  float hum = 0;
  float temp = 0;
  uint8_t type;
  uint8_t addr;
  uint8_t colm;
  uint8_t imux;
  uint8_t ibus;
  uint8_t idev;
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
  for (imux = 0; imux < sizeof(multiplexer); imux++ )
  {
    for (ibus = 0; ibus < 8; ibus++)
    {
      choose_i2c_bus(imux, ibus + 1);
      for (idev = 0; idev < 3; idev++)
      {
        type = sensor[imux][ibus][idev][get_type];
        addr = sensor[imux][ibus][idev][get_address];
        colm = (sensor[imux][ibus][idev][get_collumn] - 1);
        Serial.print(type);
        Serial.print("\tMUX= 0x");
        Serial.print(multiplexer[imux], HEX);
        Serial.print("\tADDR= 0x");
        Serial.println(addr, HEX);
        hum = 0;
        temp = 0;
        Serial.print("hum = ");
        Serial.print(hum);
        Serial.print("\ttemp = ");
        Serial.println(temp);
        if (type != EMPTY) {
          hum = get_humidity(type, addr);
          temp = get_temperature(type, addr);
        }
        Serial.print("hum2 = ");
        Serial.print(hum);
        Serial.print("\ttemp2 = ");
        Serial.println(temp);
        delay(100);
        if (type != EMPTY) {
          if (colm != NOCOLM) {
            Serial.print("hum3 = ");
            Serial.print(hum);
            Serial.print("\ttemp3 = ");
            Serial.println(temp);
            x = 2 + (colm * 80); y = 44 + (28 * ibus);
            LCD.setColor(0, 0, 255);
            LCD.printNumF (hum, 2, x, y, '.', 5);
            x = 38 + (colm * 80); y = 12 + 46 + (28 * ibus);
            LCD.setColor(255, 255, 0);
            LCD.printNumF (temp, 2, x, y, '.', 5);
          }
          csvline1  += String(hum) + ",";
          csvline2  += String(temp) + ",";
        }
      }
    }
  }
  //Serial.println(csvline1);
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

void setup()
{
  int x;
  int y;
  char i;
  // Setup the LCD
  LCD.InitLCD();
  Serial.begin(115200);
  setupSD ();
  //disconnect all i2c busses
  Wire.begin();
  choose_i2c_bus(0, 0);
  // Clear the screen and draw the frame
  drawTable ();
  initSensors ();
  delay(1000);
  drawTable ();
}

void loop()
{
  //drawTable ();
  for (uint8_t m = 0; m < sizeof(multiplexer); m++) {
    for (uint8_t b = 0; b < 8; b++) {
      for (uint8_t n = 0; n < 3; n++) {
        for (uint8_t i = 0; i < 3; i++) {
          Serial.print(sensor[m][b][n][i]);
          Serial.print(", ");
        }
        Serial.print("\t");
      }
      Serial.println();
    }
  }
  readSensors ();
  delay(1000);
}
