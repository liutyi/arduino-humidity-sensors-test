/*
   Multiple i2c humidity sensors tester.
   code for https://wiki.liutyi.info/display/ARDUINO/v3+7x8+Sensors+board+project
*/
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
#define appname "Humidity sensors tester v2.8"
#define header "SHT2 SHT3 BME+ Si7x HDCx SHT8"
#define footer "https://wiki.liutyi.info/"

// Set header (sensor names) for csv file. leave empty if intend to substitute sensors time-to-time without firmware update
//#define csvheader "Time,SHT25,SHT35,BME680-1,SHT21,SHT35,BME680-1,SHT21,SHT31-2,BME280,SHT21-CJMCU,SHT31-2,BME280,SHT21-CJMCU,SHT31,BME280,SHT20,SHT31,DHT12,SHT20,SHT30,DHT12,SHT20,SHT30,DHT12,Si7021-1,Si7021-1,Si7021-2,Si7021-2,Si7021-3,Si7021-3,Si7021-4,HTU21d,HDC1080-CJMCU,SHT31-2,HDC1080-CJMCU,SHT31-2,HDC1080-CJMCU,SHT85,HDC1080-CJMCU,SHT85,HDC1080-CJMCU,SHT85,HDC1080,HDC1080,BME680-2,HDC1080,BME680-2"
#define csvheader "Time,SHT25,SHT21,SHT21,SHT21-CJMCU,SHT21-CJMCU,SHT20,SHT20,SHT20,SHT35,SHT35,SHT31-2,SHT31-2,SHT31,SHT31,SHT30,SHT30,BME680-1,BME680-1,BME280,BME280,BME280,DHT12,DHT12,DHT12,HTU21d,Si7021-4,Si7021-3,Si7021-3,Si7021-2,Si7021-2,Si7021,Si7021,HDC1080,HDC1080,HDC1080,HDC1080-CJMCU,HDC1080-CJMCU,HDC1080-CJMCU,HDC1080-CJMCU,HDC1080-CJMCU,BME680-2,BME680-2,SHT85,SHT85,SHT85,SHT31-2,SHT31-2"

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
    {  {HDC1X, 5, 64}, {BME680, 6, 118}, {EMPTY, UNDEF, 0} },
    {  {HDC1X, 5, 64}, {BME680, 6, 118}, {EMPTY, UNDEF, 0} },
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
#define SHT2X_CMD_SIZE 1
#define SHT2X_DATA_SIZE 3
#define SHT3X_MEASUREMENT_DELAY 16  /* HIGH = 15 MID=6 LOW=4 */
#define SHT2X_T_MEASUREMENT_DELAY 85 /* 85 - 14bit,  43 - 13 bit, 22 - 12 bit, 11 - 11 bit */
#define SHT2X_RH_MEASUREMENT_DELAY 30 /* 29 - 12bit,  15 - 11 bit, 9 - 10 bit, 4 - 8 bit */
#define SHT_RESET_DURATION 20 /* should be 15 */
//#define SHT2X_READ_T 0xE3 /* HOLD the master */
#define SHT2X_READ_T 0xF3
//#define SHT2X_READ_RH 0xE5 /* HOLD the master */
#define SHT2X_READ_RH 0xF5
#define SHT2X_RESET 0xFE

#define SHT3X_CMD_SIZE 2
#define SHT3X_DATA_SIZE 6
//#define SHT3X_CLOCK_STRETCH 0x2C
#define SHT3X_CLOCK_STRETCH 0x24
//#define SHT3X_HRES_READ 0x06
#define SHT3X_HRES_READ 0x00
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
#define HDC1X_MEASUREMENT_DELAY 15 /* t 6.35 -14 bit 3.65 - 11bit; RH 14bit - 6.50, 11 bit - 3.85, 8bit - 2.50 */
#define HDC1X_RESET_DURATION 15 /*After power-up, the sensor needs at most 15 ms, to be ready*/

#define DHT12_CONFIG_CMD_SIZE 3
#define DHT12_DATA_SIZE 2
#define DHT12_READ_T 0x02
#define DHT12_READ_RH 0x00
#define DHT12_MEASUREMENT_DELAY 15

#define BME280_CMD_SIZE 1
#define BME280_CFG_SIZE 2
#define BME280_DATA_SIZE 2
#define BME280_T_DATA_SIZE 3
#define BME280_MEASUREMENT_DELAY 10  /* HIGH = 15 MID=6 LOW=4 */
#define BME280_RESET_DURATION 300 /* should be 15 */
#define BME280_READ_T 0xFA
#define BME280_READ_RH 0xFD
#define BME280_RESET_REGISTER 0xE0
#define BME280_RESET 0xB6
#define BME280_CONFIG_REG 0xF5
#define BME280_CONFIG 0x00 /* 0.5ms standby, filter off, no 3-wire SPI */
#define BME280_CONTROL_RH_REG 0xF2 /* need to update RH and M register both to apply RH */
#define BME280_CONTROL_RH 0x01 /* 001 - RH oversampling x1 */
#define BME280_CONTROL_M_REG 0xF4
#define BME280_CONTROL_M 0x27 /* 001 - t oversampling x1, 001 - p oversampling x1 11 - normal (not sleep) mode */
#define BME280_7BIT_MASK 0x7F
#define BME280_COEFF1_ADDR 0x88
#define BME280_COEFF2_ADDR 0xE1
#define BME280_COEFF3_ADDR 0xA1
#define BME280_COEFF1_SIZE 6
#define BME280_COEFF2_SIZE 7
#define BME280_COEFF3_SIZE 1

//COEF1
#define BME280_T1_LSB 0
#define BME280_T1_MSB 1
#define BME280_T2_LSB 2
#define BME280_T2_MSB 3
#define BME280_T3_LSB 4
#define BME280_T3_MSB 5
//COEF2
#define BME280_H2_LSB 0
#define BME280_H2_MSB 1
#define BME280_H3   2
#define BME280_H4_MSB 3
#define BME280_H4_LSB 4
#define BME280_H5_LSB 4
#define BME280_H5_MSB 5
#define BME280_H6   6
//COEF3
#define BME280_H1 0

#define BME680_CMD_SIZE 1
#define BME680_CFG_SIZE 2
#define BME680_MEASUREMENT_DELAY 15  /* HIGH = 15 MID=6 LOW=4 */
#define BME680_RESET_DURATION 15 /* should be 15 */
#define BME680_READ_ALL 0x1D
#define BME680_READ_ALL_SIZE 15
#define BME680_READ_T 0x22
#define BME680_READ_RH 0x25
#define BME680_RESET_REGISTER 0xE0
#define BME680_RESET 0xB6
#define BME680_CONFIG_REG 0x75
#define BME680_CONFIG 0x00 /* 0.5ms standby, filter off, no 3-wire SPI */
#define BME680_CONTROL_RH_REG 0x72 /* need to update RH and M register both to apply RH */
#define BME680_CONTROL_RH 0x01 /* 001 - RH oversampling x1 */
#define BME680_CONTROL_M_REG 0x74
#define BME680_CONTROL_M 0x20 /* 001 - t oversampling x1, 000 - p disabled (oversampling x0) 00 - sleep mode */
#define BME680_CONTROL_M_ON 0x21
//#define BME680_CONTROL_M 0x24 /* 001 - t oversampling x1, 001 - p oversampling x1, 00 - sleep mode */
//#define BME680_CONTROL_M_ON 0x25
#define BME680_CONTROL_GH_REG 0x70
#define BME680_CONTROL_GH 0x0 /*Gas heater off*/
#define BME680_COEFF1_ADDR 0x89
#define BME680_COEFF2_ADDR 0xE1
#define BME680_COEFF1_SIZE 4
#define BME680_COEFF2_SIZE 16

//COEF1
#define BME680_T2_LSB 1
#define BME680_T2_MSB 2
#define BME680_T3 3
//COEF2
#define BME680_H2_MSB 0
#define BME680_H2_LSB 1
#define BME680_H1_LSB 1
#define BME680_H1_MSB 2
#define BME680_H3   3
#define BME680_H4   4
#define BME680_H5   5
#define BME680_H6   6
#define BME680_H7   7
#define BME680_T1_LSB 8
#define BME680_T1_MSB 9

uint8_t readBuffer[17] = {0}; //buffer for read from sensor
uint8_t writeBuffer[3] = {0}; //variable to devide long i2c command
uint32_t timeout;

// Other Variables
int x;
int y;
uint8_t mux;
uint8_t bus;
uint8_t dev;
uint8_t type;
uint8_t addr;
uint8_t colm;
String csvline1 = "";
String csvline2 = "";
long seconds;
float hum = 0;
float temp = 0;
uint32_t temp_comp;
int32_t temp_comp_280;

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
  LCD.setBackColor(0, 0, 0);
  LCD.setColor(100, 100, 0);
  LCD.setFont(BigFont);
  for (mux = 0; mux < sizeof(multiplexer); mux++)
  {
    for (bus = 0; bus < 8; bus++)
    {
      choose_i2c_bus();
      for (dev = 0; dev < 3; dev++)
      {
        type = sensor[mux][bus][dev][get_type];
        addr = sensor[mux][bus][dev][get_address];
        colm = (sensor[mux][bus][dev][get_collumn] - 1);
        if (type != EMPTY) {
          init_sensor(type, addr);
        }
        if (colm != NOCOLM) {
          x = 20 + (colm * 80); y = 46 + (28 * bus);
          LCD.printNumI (addr, x, y);
        }
      }
    }
  }
}

void choose_i2c_bus() {
  for (uint8_t i = 0; i < 3; i++) {
    uint8_t addr = multiplexer[i];
    Wire.beginTransmission(addr);
    if ( i == mux ) {
      Wire.write(1 << bus);
    } else {
      Wire.write(0);
    }
    Wire.endTransmission();
  }
  delay (15);
}

void clean_buffers () {
  for (uint8_t i = 0; i < sizeof(readBuffer); i++) {
    readBuffer[i] = 0;
  }
  for (uint8_t i = 0; i < sizeof (writeBuffer); i++) {
    writeBuffer[i] = 0;
  }
}

void init_sensor (uint8_t itype, uint8_t iaddr)
{
  clean_buffers();
  if (itype == SHT2X) {
    Wire.beginTransmission(iaddr);
    Wire.write(SHT2X_RESET);
    Wire.endTransmission();
    //delay(SHT_RESET_DURATION); /* since there is no other configuration, skip delay */
  }
  if (itype == SHT3X) {
    Wire.beginTransmission(iaddr);
    writeBuffer[0] = SHT3X_CONTROL;
    writeBuffer[1] = SHT3X_RESET;
    for (int i = 0; i < SHT3X_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    //delay(SHT_RESET_DURATION); /* since there is no other configuration, skip delay */
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
    Wire.beginTransmission(addr);
    writeBuffer[0] = HDC1X_CONFIG;
    writeBuffer[1] = HDC1X_RESET;
    writeBuffer[2] = HDC1X_HRES;
    for (int i = 0; i < HDC1X_CONFIG_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    //delay(HDC1X_RESET_DURATION); /* since there is no other configuration, skip delay */
  }
  if (type == BME280 ) {
    Wire.beginTransmission(addr);
    writeBuffer[0] = BME280_RESET_REGISTER;
    writeBuffer[1] = BME280_RESET;
    for (int i = 0; i < BME280_CFG_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    delay(BME280_RESET_DURATION);
    Wire.beginTransmission(addr);
    writeBuffer[0] = BME280_CONTROL_RH_REG;
    writeBuffer[1] = BME280_CONTROL_RH;
    for (int i = 0; i < BME280_CFG_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    Wire.beginTransmission(addr);
    writeBuffer[0] = BME280_CONTROL_M_REG;
    writeBuffer[1] = BME280_CONTROL_M;
    for (int i = 0; i < BME280_CFG_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
  }
  if (type == BME680 ) {
    Wire.beginTransmission(addr);
    writeBuffer[0] = BME680_RESET_REGISTER;
    writeBuffer[1] = BME680_RESET;
    for (int i = 0; i < BME680_CFG_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    delay(BME680_RESET_DURATION);
    Wire.beginTransmission(addr);
    writeBuffer[0] = BME680_CONTROL_GH_REG;
    writeBuffer[1] = BME680_CONTROL_GH;
    for (int i = 0; i < BME680_CFG_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    Wire.beginTransmission(addr);
    writeBuffer[0] = BME680_CONTROL_RH_REG;
    writeBuffer[1] = BME680_CONTROL_RH;
    for (int i = 0; i < BME680_CFG_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    Wire.beginTransmission(addr);
    writeBuffer[0] = BME680_CONTROL_M_REG;
    writeBuffer[1] = BME680_CONTROL_M;
    for (int i = 0; i < BME680_CFG_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
  }
  if (type == DHT1X ) {
    //do nothing
  }
}


void get_humidity ()
{
  uint16_t result = 0;
  clean_buffers();
  if ((type == SHT2X) | (type == SI70XX)) {
    Wire.beginTransmission(addr);
    Wire.write(SHT2X_READ_RH);
    Wire.endTransmission();
    delay(SHT2X_RH_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)SHT2X_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < SHT2X_DATA_SIZE) {
        delay(SHT2X_RH_MEASUREMENT_DELAY / 2);
      } else {
        for (int i = 0; i < SHT2X_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        result = (readBuffer[0] << 8) + readBuffer[1];
        result &= ~0x0003;
        hum = (float)result;
        hum *= 125;
        hum /= 65536;
        hum -= 6;
      }
    }
  }
  if (type == SHT3X) {
    writeBuffer[0] = SHT3X_CLOCK_STRETCH;
    writeBuffer[1] = SHT3X_HRES_READ;
    Wire.beginTransmission(addr);
    for (int i = 0; i < SHT3X_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    delay(SHT3X_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)SHT3X_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if  (Wire.available() < SHT3X_DATA_SIZE) {
        delay(SHT3X_MEASUREMENT_DELAY);
      } else {
        for (int i = 0; i < SHT3X_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        result = (readBuffer[3] << 8) + readBuffer[4];
        hum = (float)result;
        hum *= 100;
        hum /= 65535;
      }
    }
  }
  if (type == HDC1X) {
    Wire.beginTransmission(addr);
    Wire.write(HDC1X_READ_RH);
    Wire.endTransmission();
    delay(HDC1X_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)HDC1X_DATA_SIZE);
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
        hum = (float)result;
        hum *= 100;
        hum /= 65536;
      }
    }
  }
  if (type == BME280 ) {
    uint8_t h1, h3;
    int16_t h2, h4, h5, result;
    int8_t h6;
    uint32_t xresult;
    clean_buffers();
    Wire.beginTransmission(addr);
    Wire.write(BME280_COEFF3_ADDR);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)addr, (uint8_t)BME280_COEFF3_SIZE);
    timeout = millis() + (DEFAULT_TIMEOUT / 2);
    while ( millis() < timeout) {
      if (Wire.available() < BME280_COEFF3_SIZE) {
        delay(BME280_MEASUREMENT_DELAY / 2);
      } else {
        for (int i = 0; i < BME280_COEFF3_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        h1 = (uint8_t) readBuffer[BME280_H1];
      }
    }
    Wire.beginTransmission(addr);
    Wire.write(BME280_COEFF2_ADDR);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)addr, (uint8_t)BME280_COEFF2_SIZE);
    timeout = millis() + (DEFAULT_TIMEOUT / 2);
    while ( millis() < timeout) {
      if (Wire.available() < BME280_COEFF2_SIZE) {
        delay(BME280_MEASUREMENT_DELAY / 2);
      } else {
        for (int i = 0; i < BME280_COEFF2_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        h2 = (uint16_t) (((uint16_t) readBuffer[BME280_H2_MSB] << 8) | (uint16_t)(readBuffer[BME280_H2_LSB]));
        h3 = (uint8_t) readBuffer[BME280_H3];
        h4 = (int16_t) (((uint16_t) readBuffer[BME280_H4_MSB] << 4) | ((uint16_t)(readBuffer[BME280_H4_LSB]) & 0x0F));
        h5 = (int16_t) (((uint16_t) readBuffer[BME280_H5_MSB] << 4) | ((uint16_t)(readBuffer[BME280_H5_LSB]) >> 4));
        h6 = (int8_t) readBuffer[BME280_H6];
      }
    }
    clean_buffers();
    Wire.endTransmission();
    Wire.beginTransmission(addr);
    Wire.write(BME280_READ_RH);
    Wire.endTransmission();
    delay(BME280_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)BME280_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < BME280_DATA_SIZE) {
        delay(BME280_MEASUREMENT_DELAY / 2);
      } else {
        for (int i = 0; i < BME280_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        xresult = (uint16_t)((readBuffer[0] << 8) | readBuffer[1]);
        int32_t var32;
        var32 = (temp_comp_280 - ((int32_t)76800));
        var32 = (((((xresult << 14) - (((int32_t)h4) << 20) -
                    (((int32_t)h5) * var32)) + ((int32_t)16384)) >> 15) *
                 (((((((var32 * ((int32_t)h6)) >> 10) *
                      (((var32 * ((int32_t)h3)) >> 11) + ((int32_t)32768))) >> 10) +
                    ((int32_t)2097152)) * ((int32_t)h2) + 8192) >> 14));
        var32 = (var32 - (((((var32 >> 15) * (var32 >> 15)) >> 7) *
                           ((int32_t)h1)) >> 4));
        hum = (float)(var32  >> 12);
        hum /= 1024;

      }
    }
  }
  if (type == BME680 ) {
    uint16_t h1, h2, result;
    int8_t h3, h4, h5, h7 = 1;
    uint8_t h6 = 1;
    clean_buffers();
    Wire.beginTransmission(addr);
    Wire.write(BME680_COEFF2_ADDR);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)addr, (uint8_t)BME680_COEFF2_SIZE);
    timeout = millis() + (DEFAULT_TIMEOUT / 2);
    while ( millis() < timeout) {
      if (Wire.available() < BME680_COEFF2_SIZE) {
        delay(BME680_MEASUREMENT_DELAY / 2);
      } else {
        for (int i = 0; i < BME680_COEFF2_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        h1 = (uint16_t) (((uint16_t) readBuffer[BME680_H1_MSB] << 4) | ((uint16_t)(readBuffer[BME680_H1_LSB]) & 0x0F));
        h2 = (uint16_t) (((uint16_t) readBuffer[BME680_H2_MSB] << 4) | ((uint16_t)(readBuffer[BME680_H2_LSB]) >> 4));
        h3 = (int8_t) readBuffer[BME680_H3];
        h4 = (int8_t) readBuffer[BME680_H4];
        h5 = (int8_t) readBuffer[BME680_H5];
        h6 = (uint8_t) readBuffer[BME680_H6];
        h7 = (int8_t) readBuffer[BME680_H7];
      }
    }
    clean_buffers();
    Wire.beginTransmission(addr);
    writeBuffer[0] = BME680_CONTROL_M_REG;
    writeBuffer[1] = BME680_CONTROL_M_ON;
    for (int i = 0; i < BME680_CFG_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    Wire.beginTransmission(addr);
    Wire.write(BME680_READ_ALL);
    Wire.endTransmission();
    delay(BME680_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)BME680_READ_ALL_SIZE);

    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < BME680_READ_ALL_SIZE) {
        delay(BME680_MEASUREMENT_DELAY);
      } else {
        for (int i = 0; i < BME680_READ_ALL_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        result =  (uint16_t) (((uint32_t) readBuffer[8] * 256) | (uint32_t) readBuffer[9]);
        int32_t var1, var2, var3, var4, var5, var6, temp_scaled, calc_hum;
        temp_scaled =  (((int32_t) temp_comp * 5) + 128) >> 8;
        var1 = (int32_t) (result - ((int32_t) ((int32_t) h1 * 16)))
               - (((temp_scaled * (int32_t) h3) / ((int32_t) 100)) >> 1);
        var2 = ((int32_t) h2
                * (((temp_scaled * (int32_t) h4) / ((int32_t) 100))
                   + (((temp_scaled * ((temp_scaled * (int32_t) h5) / ((int32_t) 100))) >> 6)
                      / ((int32_t) 100)) + (int32_t) (1 << 14))) >> 10;
        var3 = var1 * var2;
        var4 = (int32_t) h6 << 7;
        var4 = ((var4) + ((temp_scaled * (int32_t) h7) / ((int32_t) 100))) >> 4;
        var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
        var6 = (var4 * var5) >> 1;
        calc_hum = (((var3 + var6) >> 10) * ((int32_t) 1000)) >> 12;
        /* if (calc_hum > 100000)
          calc_hum = 100000;
          else if (calc_hum < 0)
          calc_hum = 0; */
        hum = (float)calc_hum;
        hum /= 1000;

      }
    }
  }
  if (type == DHT1X ) {
    Wire.beginTransmission(addr);
    Wire.write(DHT12_READ_RH);
    Wire.endTransmission();
    delay(DHT12_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)DHT12_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < DHT12_DATA_SIZE) {
        delay(DHT12_MEASUREMENT_DELAY / 2);
      } else {
        for (int i = 0; i < DHT12_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        hum = (readBuffer[0] + (float) readBuffer[1] / 10);
      }
    }
  }
}

void get_temperature () {
  uint16_t result = 0;
  clean_buffers();
  if ((type == SHT2X) | (type == SI70XX)) {
    Wire.beginTransmission(addr);
    Wire.write(SHT2X_READ_T);
    Wire.endTransmission();
    delay(SHT2X_T_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)SHT2X_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < SHT2X_DATA_SIZE) {
        delay(SHT2X_T_MEASUREMENT_DELAY / 2);
      } else {
        for (int i = 0; i < SHT2X_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        result = (readBuffer[0] << 8) + readBuffer[1];
        result &= ~0x0003;
        temp = (float)result;
        temp *= 175.72;
        temp /= 65536;
        temp -= 46.85;

      }
    }
  }
  if (type == SHT3X) {

    writeBuffer[0] = SHT3X_CLOCK_STRETCH;
    writeBuffer[1] = SHT3X_HRES_READ;
    Wire.beginTransmission(addr);
    for (int i = 0; i < SHT3X_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    delay(SHT3X_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)SHT3X_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if  (Wire.available() < SHT3X_DATA_SIZE) {
        delay(SHT3X_MEASUREMENT_DELAY / 2);
      } else {
        for (int i = 0; i < SHT3X_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        result = (readBuffer[0] << 8) + readBuffer[1];
        temp = (float)result;
        temp *= 175;
        temp /= 65535;
        temp -= 45;

      }
    }
  }
  if (type == HDC1X) {

    Wire.beginTransmission(addr);
    Wire.write(HDC1X_READ_T);
    Wire.endTransmission();
    delay(HDC1X_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)HDC1X_DATA_SIZE);
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
        temp = (float)result;
        temp *= 165;
        temp /= 65536;
        temp -= 40;
      }
    }
  }
  if (type == BME280 ) {
    uint16_t t1;
    int16_t t2, t3;
    uint32_t result = 0;
    clean_buffers();
    Wire.beginTransmission(addr);
    Wire.write(BME280_COEFF1_ADDR);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)addr, (uint8_t)BME280_COEFF1_SIZE);
    timeout = millis() + (DEFAULT_TIMEOUT / 2);
    while ( millis() < timeout) {
      if (Wire.available() < (BME280_COEFF1_SIZE)) {
        delay(BME280_MEASUREMENT_DELAY / 2);
      } else {
        for (int i = 0; i < BME280_COEFF1_SIZE; i++) {
          readBuffer[i] = Wire.read();

        }
        t1 = (uint16_t) (((uint16_t)readBuffer[BME280_T1_MSB] << 8) | (uint16_t)readBuffer[BME280_T1_LSB]);
        t2 = (int16_t)(((uint16_t)readBuffer[BME280_T2_MSB] << 8) | (uint16_t)readBuffer[BME280_T2_LSB]);
        t3 = (int16_t)(((uint16_t)readBuffer[BME280_T3_MSB] << 8) | (uint16_t)readBuffer[BME280_T3_LSB]);
      }
    }
    clean_buffers();
    Wire.beginTransmission(addr);
    Wire.write(BME280_READ_T);
    Wire.endTransmission();
    delay(BME280_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)BME280_T_DATA_SIZE);

    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < BME280_T_DATA_SIZE) {
        delay(BME280_MEASUREMENT_DELAY);
      } else {
        for (int i = 0; i < BME280_T_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();

        }

        result = (uint32_t) (((uint32_t) readBuffer[0] << 16) | ((uint32_t) readBuffer[1] << 8) | ((uint32_t) readBuffer[2]));

        int32_t var1, var2;
        int16_t calc_temp;
        result >>= 4;
        var1 = ((((result >> 3) - ((int32_t)t1 << 1))) * ((int32_t)t2)) >> 11;
        var2 = (((((result >> 4) - ((int32_t)t1)) * ((result >> 4) - ((int32_t)t1))) >> 12) * ((int32_t)t3)) >> 14;
        temp_comp_280 = var1 + var2;
        temp = (temp_comp_280 * 5 + 128) >> 8;
        temp /= 100;

      }
    }
  }
  if (type == BME680 ) {
    uint16_t t1;
    int16_t t2;
    int8_t t3;
    uint32_t xresult = 0;
    clean_buffers();
    Wire.beginTransmission(addr);
    Wire.write(BME680_COEFF1_ADDR);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)addr, (uint8_t)BME680_COEFF1_SIZE);
    timeout = millis() + (DEFAULT_TIMEOUT / 2);
    while ( millis() < timeout) {
      if (Wire.available() < (BME680_COEFF1_SIZE)) {
        delay(BME680_MEASUREMENT_DELAY / 2);
      } else {
        for (int i = 0; i < BME680_COEFF1_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        t2 = (int16_t)(((uint16_t)readBuffer[BME680_T2_MSB] << 8) | (uint16_t)readBuffer[BME680_T2_LSB]);
        t3 = readBuffer[BME680_T3];
      }
    }
    Wire.beginTransmission(addr);
    Wire.write(BME680_COEFF2_ADDR);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)addr, (uint8_t)BME680_COEFF2_SIZE);
    timeout = millis() + (DEFAULT_TIMEOUT / 2);
    while ( millis() < timeout) {
      if (Wire.available() < BME680_COEFF2_SIZE) {
        delay(BME680_MEASUREMENT_DELAY / 2);
      } else {
        for (int i = 0; i < BME680_COEFF2_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        t1 = (((uint16_t)readBuffer[BME680_T1_MSB] << 8) | (uint16_t)readBuffer[BME680_T1_LSB]);
      }
    }
    clean_buffers();
    Wire.beginTransmission(addr);
    writeBuffer[0] = BME680_CONTROL_M_REG;
    writeBuffer[1] = BME680_CONTROL_M_ON;
    for (int i = 0; i < BME680_CFG_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    Wire.beginTransmission(addr);
    Wire.write(BME680_READ_ALL);
    Wire.endTransmission();
    delay(BME680_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)BME680_READ_ALL_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < BME680_READ_ALL_SIZE) {
        delay(BME680_MEASUREMENT_DELAY);
      } else {
        for (int i = 0; i < BME680_READ_ALL_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        //result =  (uint32_t) (((uint32_t) readBuffer[0] * 4096) | ((uint32_t) readBuffer[6] * 1));
        xresult =  (uint32_t) (((uint32_t) readBuffer[5] * 4096)      | ((uint32_t) readBuffer[6] * 16)    | ((uint32_t) readBuffer[7] / 16));
        int64_t var1, var2, var3 ;
        int16_t calc_temp;
        var1 = ((int32_t) xresult >> 3) - ((int32_t) t1 << 1);
        var2 = (var1 * (int32_t) t2) >> 11;
        var3 = ((var1 >> 1) * (var1 >> 1)) >> 12;
        var3 = ((var3) * ((int32_t) t3 << 4)) >> 14;
        temp_comp  = (int32_t) (var2 + var3);
        calc_temp = (int16_t) ((( temp_comp * 5) + 128) >> 8);
        temp = (float)calc_temp;
        temp /= 100;
      }
    }
  }
  if (type == DHT1X ) {
    Wire.beginTransmission(addr);
    Wire.write(DHT12_READ_T);
    Wire.endTransmission();
    delay(DHT12_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)DHT12_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < DHT12_DATA_SIZE) {
        delay(DHT12_MEASUREMENT_DELAY);
      } else {
        for (int i = 0; i < DHT12_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        uint8_t minor = readBuffer[1] & 0x7F;
        uint8_t sign  = readBuffer[1] & 0x80;
        temp = (readBuffer[0] + (float) minor / 10);
        if (sign)  // negative temperature
          temp = -temp;
      }
    }
  }
}

void readSensors ()
{
  hum = 0;
  temp = 0;
  csvline1 = "";
  csvline2 = "";

  //uptime in seconds
  seconds = millis() / 1000;
  LCD.setFont(SmallFont);
  LCD.setBackColor(64, 64, 64);
  LCD.setColor(0, 0, 0);
  LCD.printNumI (seconds, 2, 307);
  LCD.setBackColor(0, 0, 0);
  for (mux = 0; mux < sizeof(multiplexer); mux++ )
  {
    for (dev = 0; dev < 3; dev++)
    {
      for (bus = 0; bus < 8; bus++)
      {
        choose_i2c_bus();
        type = sensor[mux][bus][dev][get_type];
        addr = sensor[mux][bus][dev][get_address];
        colm = (sensor[mux][bus][dev][get_collumn] - 1);
        hum = 0;
        temp = 0;
        if (type != EMPTY) {
          get_temperature();
          get_humidity();
        }
        delay(100);
        if (type != EMPTY) {
          if (colm != NOCOLM) {
            x = 2 + (colm * 80); y = 44 + (28 * bus);
            LCD.setColor(0, 0, 255);
            LCD.printNumF (hum, 2, x, y, '.', 5);
            x = 38 + (colm * 80); y = 12 + 46 + (28 * bus);
            LCD.setColor(255, 255, 0);
            LCD.printNumF (temp, 2, x, y, '.', 5);
          }
          csvline1  += String(hum) + ",";
          csvline2  += String(temp) + ",";
        }
      }
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

void setup()
{
  Wire.begin();
  LCD.InitLCD();
  Serial.begin(115200);
  setupSD ();
  drawTable ();
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
