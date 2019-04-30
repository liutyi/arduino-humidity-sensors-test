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
#define appname "Humidity sensors tester v2.1"
#define header "SHT2 SHT3 BME+ Si7x HDCx SHT8"
#define footer "https://wiki.liutyi.info/"

// Set header (sensor names) for csv file. leave empty if intend to substitute sensors time-to-time without firmware update
//#define csvheader "Time,SHT25,SHT35,BME680-1,SHT21,SHT35,BME680-1,SHT21,SHT31-2,BME280,SHT21-CJMCU,SHT31-2,BME280,SHT21-CJMCU,SHT31,BME280,SHT20,SHT31,DHT12,SHT20,SHT30,DHT12,SHT20,SHT30,DHT12,Si7021-1,Si7021-1,Si7021-2,Si7021-2,Si7021-3,Si7021-3,Si7021-4,HTU21d,HDC1080-CJMCU,SHT31-2,HDC1080-CJMCU,SHT31-2,HDC1080-CJMCU,SHT85,HDC1080-CJMCU,SHT85,HDC1080-CJMCU,SHT85,HDC1080,HDC1080,BME680-2,HDC1080,BME680-2"
#define csvheader "Time,Sensors"

// Variables for file rotation 00-99
#define t_base_name "tmprt"
#define rh_base_name "humdt"
const uint8_t t_base_name_size = sizeof(t_base_name);
const uint8_t rh_base_name_size = sizeof(rh_base_name);
char tFileName[] = t_base_name "000.csv";
char rhFileName[] = rh_base_name "000.csv";

// Define number and addresses of multiplexers
uint8_t multiplexer[3] = {112, 113, 114};

// Type of sensor
#define EMPTY 0 /* slot is empty or sensor disabled */
#define SHT2X 1 /* include SHT20, SHT21, SHT25, HTU21d*/
#define SI70XX 2 /* includes Si7021 */
#define HDC1X 3 /* includes HDC1080 */
#define SHT3X 4 /* include SHT30, SHT31, SHT35, SHT88*/
#define BME280 5 /* includes BME280 */
#define BME680 6 /* includes BME680 */
#define DHT1X 7 /* includes DHT12 */
#define DHT2X 8 /* includes DHT22 */
#define AHT1X 9 /* includes AHT10 */

// indexes name in sensor arrays
#define get_type 0 /* indexes name in sensor arrays */
#define get_collumn 1 /* indexes name in sensor arrays */
#define get_address 2 /* indexes name in sensor arrays */
#define UNDEF 255 /* sensor have no position on display */
#define NOCOLM 254 /* do not display this sensor */

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
    {  {SI70XX, 4, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {SI70XX, 4, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {SI70XX, 4, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {SI70XX, 4, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {SI70XX, 4, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {SI70XX, 4, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {SI70XX, 4, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {SI70XX, 4, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} }
  },
  {
    {  {HDC1X, 5, 64}, {BME680, UNDEF, 118}, {EMPTY, UNDEF, 0} },
    {  {HDC1X, 5, 64}, {BME680, UNDEF, 118}, {EMPTY, UNDEF, 0} },
    {  {HDC1X, 5, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {HDC1X, 5, 64}, {SHT3X, 6, 68}, {EMPTY, UNDEF, 0} },
    {  {HDC1X, 5, 64}, {SHT3X, 6, 68}, {EMPTY, UNDEF, 0} },
    {  {HDC1X, 5, 64}, {SHT3X, 6, 68}, {EMPTY, UNDEF, 0} },
    {  {HDC1X, 5, 64}, {SHT3X, 6, 68}, {EMPTY, UNDEF, 0} },
    {  {HDC1X, 5, 64}, {SHT3X, 6, 68}, {EMPTY, UNDEF, 0} }
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
// Sensor communication variables
#define DEFAULT_TIMEOUT 300
#define SHT2X_CMD_SIZE 2
#define SHT2X_DATA_SIZE 3
#define SHT3X_MEASUREMENT_DELAY 15  /* HIGH = 15 MID=6 LOW=4 */
#define SHT2X_MEASUREMENT_DELAY 10
#define SHT_RESET_DURATION 20 /* should be 15 */
#define SHT2X_READ_T 0xE3
#define SHT2X_READ_RH 0xE5
#define SHT2X_RESET 0xFE

#define SHT3X_CMD_SIZE 2
#define SHT3X_DATA_SIZE 6
#define SHT3X_CLOCK_STRETCH 0x2C
#define SHT3X_HRES_READ 0x06
#define SHT3X_CONTROL 0x30
#define SHT3X_RESET 0xA2
#define SHT3X_HEATER_OFF 0x66
#define SHT3X_CLEAR_STATUS 0x41

#define HDC1X_CONFIG_CMD_SIZE 3
#define HDC1X_DATA_SIZE 4
#define HDC1X_READ_T 0x00
#define HDC1X_READ_RH 0x01
#define HDC1X_CONFIG 0x02 /* MSB */
#define HDC1X_HRES 0x00
#define HDC1X_RESET 0x80 /* MSB */
#define HDC1X_MEASUREMENT_DELAY 20
#define HDC1X_RESET_DURATION 20

uint8_t readBuffer[6] = {0, 0, 0, 0, 0, 0}; //buffer for read from sensor
uint8_t writeBuffer[3] = {0, 0, 0};            //variable to devide long i2c command
uint32_t timeout;

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
      Serial.print("TRYING to switch i2c bus = ");
      Serial.print(ibus + 1);
      Serial.print("\t of mux = ");
      Serial.print(imux, HEX);
      Serial.println();
      choose_i2c_bus(imux, ibus + 1);
      Serial.println("switch done");
      for (idev = 0; idev < 3; idev++)
      {
        type = sensor[imux][ibus][idev][get_type];
        addr = sensor[imux][ibus][idev][get_address];
        colm = (sensor[imux][ibus][idev][get_collumn] - 1);
        if (type != EMPTY) {
          Serial.print("TRYING TO Init type = ");
          Serial.print(type);
          Serial.print("\t addr = ");
          Serial.print(addr, HEX);
          Serial.println();
          init_sensor(type, addr);

        }
        if (colm != NOCOLM) {
          x = 20 + (colm * 80); y = 46 + (28 * ibus);
          LCD.printNumI (type, x, y);
        }
      }
    }
  }
}


void choose_i2c_bus(uint8_t mux, uint8_t bus) {
  for (uint8_t i = 0; i < 3; i++) {
    uint8_t addr = multiplexer[i];
    Serial.print ("before begin transmition to:\t");
    Serial.print (addr);
    Serial.print ("\titerator:\t");
    Serial.print (i);
    Wire.beginTransmission(addr);
    if ( i == mux ) {
      Serial.println (bus);
      Wire.write(bus);
    } else {
      Serial.println (0);
      Wire.write(0);
    }
    Wire.endTransmission();
    delay (5);
  }
}

void clean_buffers () {
  for (uint8_t i = 0; i < sizeof(readBuffer); i++) {
    readBuffer[i] = 0;
  }
  for (uint8_t i = 0; i < sizeof (writeBuffer); i++) {
    writeBuffer[i] = 0;
  }
}

bool init_sensor (uint8_t type, uint8_t addr)
{
  clean_buffers();
  if (type == SHT2X) {
    Serial.print("SHT2X Init  ");
    Serial.print(type);
    Serial.print("\t addr = ");
    Serial.print(addr, HEX);
    Serial.print("\t com = ");
    Serial.print(SHT2X_RESET, HEX);
    Serial.println();
    Wire.beginTransmission(addr);
    Wire.write(SHT2X_RESET);
    Wire.endTransmission();
    delay(SHT_RESET_DURATION);
  }
  if (type == SHT3X) {
    Serial.print("SHT3X Init  ");
    Serial.print(type);
    Serial.print("\t addr = ");
    Serial.print(addr, HEX);
    Serial.print("\t com = ");
    Serial.print(SHT3X_CONTROL, HEX);
    Serial.print("\t");
    Serial.print(SHT3X_RESET, HEX);
    Serial.println();
    Wire.beginTransmission(addr);
    writeBuffer[0] = SHT3X_CONTROL;
    writeBuffer[1] = SHT3X_RESET;
    for (int i = 0; i < SHT3X_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
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
  if (type == HDC1X) {
    Serial.print("HDC1X Init  ");
    Serial.print(type);
    Serial.print("\t addr = ");
    Serial.print(addr, HEX);
    Serial.print("\t com = ");
    Serial.print(HDC1X_CONFIG, HEX);
    Serial.print("\t");
    Serial.print(HDC1X_RESET, HEX);
    Serial.print("\t");
    Serial.print(HDC1X_HRES, HEX);
    Serial.println();
    Wire.beginTransmission(addr);
    writeBuffer[0] = HDC1X_CONFIG;
    writeBuffer[1] = HDC1X_RESET;
    writeBuffer[2] = HDC1X_HRES;
    for (int i = 0; i < HDC1X_CONFIG_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    delay(HDC1X_RESET_DURATION);

  }
}

float get_humidity (uint8_t gtype, uint8_t gaddr)
{
  float xhum = 0;
  uint16_t result = 0;
  uint8_t z = 0;
  clean_buffers();
  if ((gtype == SHT2X) | (gtype == SI70XX)) {
    Wire.beginTransmission(gaddr);
    Wire.write(SHT2X_READ_RH);
    Wire.endTransmission();
    delay(SHT2X_MEASUREMENT_DELAY);
    z = Wire.requestFrom(gaddr, SHT2X_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < SHT2X_DATA_SIZE) {
        delay(SHT2X_MEASUREMENT_DELAY);
      } else {
        for (int i = 0; i < SHT2X_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        result = (readBuffer[0] << 8) + readBuffer[1];
        result &= ~0x0003;
        Serial.print("SHT2X rh result = ");
        Serial.println(result);
        xhum = (float)result;
        xhum *= 125;
        xhum /= 65536;
        xhum -= 6;
      }
    }
  }
  if (gtype == SHT3X) {
    writeBuffer[0] = SHT3X_CLOCK_STRETCH;
    writeBuffer[1] = SHT3X_HRES_READ;
    Wire.beginTransmission(gaddr);
    for (int i = 0; i < SHT3X_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    delay(SHT3X_MEASUREMENT_DELAY);
    z = Wire.requestFrom(gaddr, SHT3X_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if  (Wire.available() < SHT3X_DATA_SIZE) {
        delay(SHT3X_MEASUREMENT_DELAY);
      } else {
        for (int i = 0; i < SHT3X_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        result = (readBuffer[3] << 8) + readBuffer[4];
        Serial.print("SHT3X rh result = ");
        Serial.println(result);
        xhum = (float)result;
        xhum *= 100;
        xhum /= 65535;
      }
    }
  }
  if (gtype == HDC1X) {
    Wire.beginTransmission(gaddr);
    Wire.write(HDC1X_READ_RH);
    Wire.endTransmission();
    delay(HDC1X_MEASUREMENT_DELAY);
    z = Wire.requestFrom(gaddr, HDC1X_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < HDC1X_DATA_SIZE) {
        delay(HDC1X_MEASUREMENT_DELAY);
      } else {
        for (int i = 0; i < HDC1X_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        result =  (readBuffer[0] << 8) + readBuffer[1];
        result &= ~0x03;
        Serial.print("HDC1X rh result = ");
        Serial.println(result);
        xhum = (float)result;
        xhum *= 100;
        xhum /= 65536;
      }
    }
  }
  return xhum;
}

float get_temperature (uint8_t gtype, uint8_t gaddr) {
  float xtemp = 0;
  uint16_t result = 0;
  uint8_t z = 0;
  clean_buffers();
  if ((gtype == SHT2X) | (gtype == SI70XX)) {
    Wire.beginTransmission(gaddr);
    Wire.write(SHT2X_READ_T);
    Wire.endTransmission();
    delay(SHT2X_MEASUREMENT_DELAY);
    z = Wire.requestFrom(gaddr, SHT2X_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < SHT2X_DATA_SIZE) {
        delay(SHT2X_MEASUREMENT_DELAY);
      } else {
        for (int i = 0; i < SHT2X_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        result = (readBuffer[0] << 8) + readBuffer[1];
        result &= ~0x0003;
        Serial.print("t result = ");
        Serial.print(result);
        xtemp = (float)result;
        xtemp *= 175.72;
        xtemp /= 65536;
        xtemp -= 46.85;
      }
    }
  }
  if (gtype == SHT3X) {
    writeBuffer[0] = SHT3X_CLOCK_STRETCH;
    writeBuffer[1] = SHT3X_HRES_READ;
    Wire.beginTransmission(gaddr);
    for (int i = 0; i < SHT3X_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    delay(SHT3X_MEASUREMENT_DELAY);
    z = Wire.requestFrom(gaddr, SHT3X_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if  (Wire.available() < SHT3X_DATA_SIZE) {
        delay(SHT3X_MEASUREMENT_DELAY);
      } else {
        for (int i = 0; i < SHT3X_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        result = (readBuffer[0] << 8) + readBuffer[1];
        Serial.print("t result = ");
        Serial.print(result);
        xtemp = (float)result;
        xtemp *= 175;
        xtemp /= 65535;
        xtemp -= 45;
      }
    }
  }
  if (gtype == HDC1X) {
    Wire.beginTransmission(gaddr);
    Wire.write(HDC1X_READ_T);
    Wire.endTransmission();
    delay(HDC1X_MEASUREMENT_DELAY);
    z = Wire.requestFrom(gaddr, HDC1X_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < HDC1X_DATA_SIZE) {
        delay(HDC1X_MEASUREMENT_DELAY);
      } else {
        for (int i = 0; i < HDC1X_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        result =  (readBuffer[0] << 8) + readBuffer[1];
        result &= ~0x03;
        Serial.print("t result = ");
        Serial.print(result);
        xtemp = (float)result;
        xtemp *= 165;
        xtemp /= 65536;
        xtemp -= 40;
      }
    }
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
  //String csvline2 = "";

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
        Serial.print("TYPE =");
        Serial.print(type);
        Serial.print("\tMUX= 0x");
        Serial.print(multiplexer[imux], HEX);
        Serial.print("\tADDR= 0x");
        Serial.println(addr, HEX);
        hum = 0;
        temp = 0;
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
          //csvline2  += String(temp) + ",";
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
  /*
    logfile2 = SD.open(tFileName, FILE_WRITE );
    if (logfile2) {
    csvline2 = String(seconds) + "," + csvline2;
    logfile2.println(csvline2);
    LCD.print(csvline2, LEFT, 290);
    logfile2.close();
    } else {
    LCD.print("Cannot write t to file", LEFT, 290);
    }
  */
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
  Serial.println ("Serial begin done");
  setupSD ();
  Serial.println ("Setup SD done");
  Wire.begin();
  Serial.println ("Wire begin done");
  // Clear the screen and draw the frame
  drawTable ();
  Serial.println ("First table draw done");
  initSensors ();
  Serial.println ("Init Sensors done");
  delay(1000);
  drawTable ();
}

void loop()
{
  //drawTable ();
  readSensors ();
  delay(1000);
}
