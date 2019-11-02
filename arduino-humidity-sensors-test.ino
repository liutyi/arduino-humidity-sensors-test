/*
   Multiple i2c humidity sensors tester.
   code for https://wiki.liutyi.info/display/ARDUINO/Test+i2c+humidity+sensors
   HW Modification: https://wiki.liutyi.info/display/ARDUINO/v7+Sensors+Board
*/
#include <SPI.h>
#include <SD.h>
#include <UTFT.h>
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

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
#define APPNAME "Humidity sensors tester v7.2"
const String HEADER[11] { "SHT2x", "SHT3x", "BMEx8", "Si702", "HTU21", "SHT85", "HDC10", "HDC20", "AHT1x", "AM232"};
#define FOOTER "https://wiki.liutyi.info/"

// Variables for file rotation 000-999
#define t_base_name "t_v7_"
#define rh_base_name "h_v7_"
const uint8_t t_base_name_size = sizeof(t_base_name) - 1;
const uint8_t rh_base_name_size = sizeof(rh_base_name) - 1;
char tFileName[] = t_base_name "000.csv";
char rhFileName[] = rh_base_name "000.csv";

// Define number and addresses of multiplexers
uint8_t multiplexer[6] = {112, 113, 114, 115, 116, 117};

// Type of sensor
#define EMPTY 0 /* slot is empty or sensor disabled */
#define SHT2X 1 /* include SHT20, SHT21, SHT25, HTU21d*/
#define SI70XX 2 /* includes Si7021 */
#define HDC1X 3 /* includes HTU21d */
#define SHT3X 4 /* include SHT30, SHT31, SHT35*/
#define BME280 5 /* includes BME280 */
#define BME680 6 /* includes BME680 */
#define DHT1X 7 /* includes DHT12 */
#define DHT2X 8 /* includes DHT22 */
#define AHT1X 9 /* includes AHT10 */
#define SHT8X 10 /* includes SHT85 */
#define HTU2X 11 /* includes HTU21d */
#define AM2320 12  /* includes AM2320 */
#define HDC2X 13  /* includes  HDC2080*/
#define DISABLED 14  /* includes  */

// indexes name in sensor arrays
#define get_type 0 /* indexes name in sensor arrays */
#define get_collumn 1 /* indexes name in sensor arrays */
#define get_address 2 /* indexes name in sensor arrays */
#define UNDEF 255 /* sensor have no position on display */
#define NOCOLM 254 /* do not display this sensor */

// Sensor properties by [multiplexor][i2c_bus][number][get_type/get_collumn/get_address]
const uint8_t sensor[6][8][3][3] =
{
  {
    {  {SHT2X, 1, 64}, {SHT3X, 2, 68}, {BME280, 3, 118} },
    {  {SHT2X, 1, 64}, {SHT3X, 2, 68}, {BME280, 3, 118} },
    {  {SHT2X, 1, 64}, {SHT3X, 2, 68}, {BME280, 3, 118} },
    {  {SHT2X, 1, 64}, {SHT3X, 2, 68}, {BME280, 3, 118} },
    {  {SHT2X, 1, 64}, {SHT3X, 2, 68}, {BME280, 3, 118} },
    {  {SHT2X, 1, 64}, {SHT3X, 2, 68}, {BME280, 3, 119} },
    {  {SHT2X, 1, 64}, {SHT3X, 2, 68}, {BME680, 3, 119} },
    {  {SHT2X, 1, 64}, {SHT3X, 2, 68}, {BME680, 3, 119} }
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
    {  {HTU2X, 5, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {HTU2X, 5, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {HTU2X, 5, 64}, {SHT8X, 6, 68}, {EMPTY, UNDEF, 0} },
    {  {HTU2X, 5, 64}, {SHT8X, 6, 68}, {EMPTY, UNDEF, 0} },
    {  {HTU2X, 5, 64}, {SHT8X, 6, 68}, {EMPTY, UNDEF, 0} },
    {  {HTU2X, 5, 64}, {SHT3X, 6, 68}, {EMPTY, UNDEF, 0} },
    {  {HTU2X, 5, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {HTU2X, 5, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} }
  },
  {
    {  {AHT1X, 9, 56}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {AHT1X, 9, 56}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {AHT1X, 9, 56}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {AHT1X, 9, 56}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {AHT1X, 9, 56}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {AHT1X, 9, 56}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {AHT1X, 9, 56}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {AHT1X, 9, 56}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} }
  },
  {
    {  {HDC1X, 7, 64}, {AM2320, 10, 92}, {EMPTY, UNDEF, 0} },
    {  {HDC1X, 7, 64}, {AM2320, 10, 92}, {EMPTY, UNDEF, 0} },
    {  {HDC1X, 7, 64}, {AM2320, 10, 92}, {EMPTY, UNDEF, 0} },
    {  {HDC1X, 7, 64}, {AM2320, 10, 92}, {EMPTY, UNDEF, 0} },
    {  {HDC1X, 7, 64}, {AM2320, 10, 92}, {EMPTY, UNDEF, 0} },
    {  {HDC1X, 7, 64}, {AM2320, 10, 92}, {EMPTY, UNDEF, 0} },
    {  {HDC1X, 7, 64}, {AM2320, 10, 92}, {EMPTY, UNDEF, 0} },
    {  {HDC1X, 7, 64}, {AM2320, 10, 92}, {EMPTY, UNDEF, 0} }
  },
  {
    {  {HDC2X, 8, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {HDC2X, 8, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {HDC2X, 8, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {HDC2X, 8, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {HDC2X, 8, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {HDC2X, 8, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {HDC2X, 8, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} },
    {  {HDC2X, 8, 64}, {EMPTY, UNDEF, 0}, {EMPTY, UNDEF, 0} }
  }

};
// Set header (sensor names) for csv file. leave empty if intend to substitute sensors time-to-time without firmware update
#define csvheader "Time,SHT20,SHT20,SHT20,SHT21-C,SHT21-C,SHT21-G,SHT21-G,SHT25,SHT30,SHT30,SHT31,SHT31,SHT35,SHT35,SHT31-2,SHT31-2,BME280,BME280,BME280,BME280-G,BME280-G,BME280-A,BME680-C,BME680-C,Si7021,Si7021,Si7021-Y,Si7021-Y,Si7021-Y,Si7021-Y,Si7021-G,Si7021-A,HTU21d-Y,HTU21d-Y,HTU21d-Y,HTU21d-Y,HTU21d-Y,HTU21d-G,HTU21d-G,HTU21d-A,SHT85,SHT85,SHT85,SHT31-A,AHT15,AHT15,AHT15,AHT15,AHT10,AHT10,AHT10,AHT10,HDC1080-G,HDC1080-G,HDC1080-G,HDC1080-C,HDC1080-C,HDC1080-C,HDC1080-C,HDC1080-C,AM2320,AM2320,AM2320,AM2320,AM2320,AM2320,AM2320,AM2320,HDC2080-C,HDC2080-C,HDC2080-C,HDC2080-C,HDC2080-C,HDC2080-C,HDC2080-C,HDC2080-C"
// Sensor communication variables
#define DEFAULT_TIMEOUT 300
#define SHT2X_CMD_SIZE 1
#define SHT2X_DATA_SIZE 2
#define SHT2X_T_MEASUREMENT_DELAY 85 /* 85 - 14bit,  43 - 13 bit, 22 - 12 bit, 11 - 11 bit */
#define SHT2X_RH_MEASUREMENT_DELAY 30 /* 29 - 12bit,  15 - 11 bit, 9 - 10 bit, 4 - 8 bit */
#define SHT_RESET_DURATION 20 /* should be 15 */
#define SHT2X_READ_T 0xE3 /* HOLD the master */
//#define SHT2X_READ_T 0xF3
#define SHT2X_READ_RH 0xE5 /* HOLD the master */
//#define SHT2X_READ_RH 0xF5
#define SHT2X_RESET 0xFE


#define SHT3X_CMD_SIZE 2
#define SHT3X_DATA_SIZE 6
#define SHT3X_MEASUREMENT_DELAY 16  /* HIGH = 15 MID=6 LOW=4 */
#define SHT3X_CLOCK_STRETCH 0x2C
//#define SHT3X_CLOCK_STRETCH 0x24
#define SHT3X_HRES_READ 0x06
//#define SHT3X_HRES_READ 0x00
#define SHT3X_CONTROL 0x30
#define SHT3X_RESET 0xA2
#define SHT3X_HEATER_OFF 0x66
#define SHT3X_CLEAR_STATUS 0x41

#define HDC1X_CONFIG_CMD_SIZE 3
#define HDC1X_DATA_SIZE 2
#define HDC1X_READ_T 0x00
#define HDC1X_READ_RH 0x01
#define HDC1X_CONFIG 0x02
#define HDC1X_HRES 0x00
#define HDC1X_RESET 0x80
#define HDC1X_MEASUREMENT_DELAY 15 /* t 6.35 -14 bit 3.65 - 11bit; RH 14bit - 6.50, 11 bit - 3.85, 8bit - 2.50 */
#define HDC1X_RESET_DURATION 15 /*After power-up, the sensor needs at most 15 ms, to be ready*/

#define HDC2X_CONFIG_CMD_SIZE 2
#define HDC2X_DATA_SIZE 2
#define HDC2X_READ_T 0x00
#define HDC2X_READ_RH 0x02
#define HDC2X_CONFIG_REG 0x0F
#define HDC2X_CONFIG_DATA 0x01 /* 00: 14b bit resolution t/RH, 00 - both t and RH, 0 - no action, 1 - measurement */
#define HDC2X_RESET_REG 0x0E /* 0E - Reset D0 - Reset and 1 Hz sampling */
#define HDC2X_RESET_DATA 0x80 /* disable AMM, Heater off, High Z, Active Low, Level sensitive */
#define HDC2X_MEASUREMENT_DELAY 10 /* t 6.1 - 14 bit 3.5 - 11bit; RH 14bit - 6.6, 11 bit - 4.0, 8bit - 2.75 */
#define HDC2X_RESET_DURATION 3 /*After power-up, the sensor needs at most 3 ms, to be ready*/

#define DHT12_CONFIG_CMD_SIZE 3
#define DHT12_DATA_SIZE 2
#define DHT12_READ_T 0x02
#define DHT12_READ_RH 0x00
#define DHT12_MEASUREMENT_DELAY 50

#define BME280_CMD_SIZE 1
#define BME280_CFG_SIZE 2
#define BME280_DATA_SIZE 2
#define BME280_T_DATA_SIZE 3
#define BME280_MEASUREMENT_DELAY 20  /* HIGH = 15 MID=6 LOW=4 */
#define BME280_RESET_DURATION 300 /* should be 15 */
#define BME280_READ_T 0xFA
#define BME280_READ_RH 0xFD
#define BME280_RESET_REGISTER 0xE0
#define BME280_RESET 0xB6
#define BME280_CONFIG_REG 0xF5
#define BME280_CONFIG 0x00 /* 0.5ms standby, filter off, no 3-wire SPI */
#define BME280_CONTROL_RH_REG 0xF2 /* need to update RH and M register both to apply RH */
//#define BME280_CONTROL_RH 0x01 /* 001 - RH oversampling x1 */
#define BME280_CONTROL_RH 0x05 /* 101 - RH oversampling x16 */

#define BME280_CONTROL_M_REG 0xF4
//#define BME280_CONTROL_M 0x27 /* 001 - t oversampling x1, 001 - p oversampling x1 11 - normal (not sleep) mode */
#define BME280_CONTROL_M 0xA3 /* 101 - t oversampling x16, 000 - p oversampling x0 11 - normal (not sleep) mode */

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
#define BME680_MEASUREMENT_DELAY 20  /* HIGH = 15 MID=6 LOW=4 */
#define BME680_RESET_DURATION 150 /* should be 15 */
#define BME680_READ_ALL 0x1D
#define BME680_READ_ALL_SIZE 15
#define BME680_READ_T 0x22
#define BME680_READ_RH 0x25
#define BME680_RESET_REGISTER 0xE0
#define BME680_RESET 0xB6
#define BME680_CONFIG_REG 0x75
#define BME680_CONFIG 0x00 /* 0.5ms standby, filter off, no 3-wire SPI */
#define BME680_CONTROL_RH_REG 0x72 /* need to update RH and M register both to apply RH */
//#define BME680_CONTROL_RH 0x01 /* 001 - RH oversampling x1 */
#define BME680_CONTROL_RH 0x05 /* 101 - RH oversampling x16 */
#define BME680_CONTROL_M_REG 0x74
//#define BME680_CONTROL_M 0x20 /* 001 - t oversampling x1, 000 - p disabled (oversampling x0) 00 - sleep mode */
#define BME680_CONTROL_M 0xA0 /* 101 - t oversampling x16, 000 - p disabled (oversampling x0) 00 - sleep mode */
#define BME680_CONTROL_M_ON 0x21
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

#define AHT1X_CMD_SIZE 3
#define AHT1X_DATA_SIZE 6
#define AHT1X_T_MEASUREMENT_DELAY 60
#define AHT1X_RH_MEASUREMENT_DELAY 20
#define AHT1X_RESET_DURATION 20
#define AHT1X_READ 0xAC /* t and RH Calibrated Measurement 1010 1100 */
#define AHT1X_READ_DATA0 0x33
#define AHT1X_READ_DATA1 0x00
#define AHT1X_INIT 0xE1 /* Init: 1110 0001 */
#define AHT1X_INIT_DATA0 0x08
#define AHT1X_INIT_DATA1 0x00
#define AHT1X_RESET 0xBA /* Reset: 1011 1010 */

#define AM2320_READ    0x03 /// function code
#define AM2320_READ_T  0x02 /// read t
#define AM2320_READ_H  0x00 /// read rh
#define AM2320_DATA_SIZE 4
#define AM2320_CMD_SIZE 3
#define AM2320_T_MEASUREMENT_DELAY 20
#define AM2320_RH_MEASUREMENT_DELAY 10


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
int32_t temp_comp_680;
int32_t temp_comp_280;
uint8_t cycle;

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
      if (tFileName[t_base_name_size + 2] != '9') {
        tFileName[t_base_name_size + 2]++;
      } else if (tFileName[t_base_name_size + 1] != '9') {
        tFileName[t_base_name_size + 2] = '0';
        tFileName[t_base_name_size + 1]++;
      } else if (tFileName[t_base_name_size ] != '9') {
        tFileName[t_base_name_size + 2] = '0';
        tFileName[t_base_name_size + 1] = '0';
        tFileName[t_base_name_size]++;
      } else {
        LCD.print("Can't generate temperature file name", LEFT, 36);
        return;
      }
      while (SD.exists(rhFileName)) {
        if (rhFileName[rh_base_name_size + 2] != '9') {
          rhFileName[rh_base_name_size + 2]++;
        } else if (rhFileName[rh_base_name_size + 1] != '9') {
          rhFileName[rh_base_name_size + 2] = '0';
          rhFileName[rh_base_name_size + 1]++;
        } else if (rhFileName[rh_base_name_size] != '9') {
          rhFileName[rh_base_name_size + 1] = '0';
          rhFileName[rh_base_name_size + 2] = '0';
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
  LCD.print(APPNAME, CENTER, 1);
  // Footer Text (Yellow)
  LCD.setBackColor(64, 64, 64);
  LCD.setColor(255, 255, 0);
  LCD.print(FOOTER, CENTER, 307);
  // Table title
  LCD.setBackColor(0, 0, 0);
  LCD.setColor(150, 150, 150);
  LCD.setFont(SmallFont);
  // Gray Frame
  //LCD.setColor(60, 60, 60);
  LCD.drawRect(0, 14, 479, 305);
  //Draw Grid and header text
  uint8_t i = 0;
  for (int y = 14; y < 270; y += 28)
    LCD.drawLine(1, y, 479, y);
  for (int x = 48; x < 490; x += 48) {
    LCD.print(HEADER[i], ( x - 42 ), 24);
    LCD.drawLine(x, 14, x, 266);
    i++;
  }
}

void initSensors ()
{
  LCD.setBackColor(0, 0, 0);
  LCD.setColor(100, 100, 0);
  LCD.setFont(SmallFont);
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
          x = 5 + (colm * 48);
          y = 46 + (28 * bus);
          LCD.printNumI (addr, x, y);
        }
      }
    }
  }
}

void choose_i2c_bus() {
  for (uint8_t i = 0; i < sizeof(multiplexer); i++) {
    uint8_t addr = multiplexer[i];
    Wire.beginTransmission(addr);
    if ( i == mux ) {
      Wire.write(1 << bus);
    } else {
      Wire.write(0);
    }
    Wire.endTransmission();
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

void init_sensor (uint8_t itype, uint8_t iaddr)
{
  clean_buffers();
  if ((type == SHT2X) | (type == SI70XX) | (type == HTU2X)) {
    Wire.beginTransmission(iaddr);
    Wire.write(SHT2X_RESET);
    Wire.endTransmission();
    //delay(SHT_RESET_DURATION); /* since there is no other configuration, skip delay */
  }
  if ((type == SHT3X) | (type == SHT8X)) {
    Wire.beginTransmission(iaddr);
    writeBuffer[0] = SHT3X_CONTROL;
    writeBuffer[1] = SHT3X_RESET;
    for (int i = 0; i < SHT3X_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    //delay(SHT_RESET_DURATION); /* since there is no other configuration, skip delay */
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
  if (type == HDC2X) {
    Wire.beginTransmission(addr);
    writeBuffer[0] = HDC2X_RESET_REG;
    writeBuffer[1] = HDC2X_RESET_DATA;
    for (int i = 0; i < HDC2X_CONFIG_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    //delay(HDC2X_RESET_DURATION);
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
  if (type == AHT1X) {
    Wire.beginTransmission(addr);
    Wire.write(AHT1X_RESET);
    Wire.endTransmission();
    delay(AHT1X_RESET_DURATION);
    Wire.beginTransmission(addr);
    writeBuffer[0] = AHT1X_INIT;
    writeBuffer[1] = AHT1X_INIT_DATA0;
    writeBuffer[2] = AHT1X_INIT_DATA1;
    for (int i = 0; i < AHT1X_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
  }
}


void get_humidity ()
{
  uint16_t result = 0;
  clean_buffers();
  if ((type == SHT2X) | (type == SI70XX) | (type == HTU2X)) {
    Wire.beginTransmission(addr);
    Wire.write(SHT2X_READ_RH);
    Wire.endTransmission();
    delay(SHT2X_RH_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)SHT2X_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < SHT2X_DATA_SIZE) {
        delay(SHT2X_RH_MEASUREMENT_DELAY / 4);
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
        break;
      }
    }
  }
  if ((type == SHT3X) | (type == SHT8X)) {
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
        break;
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
        break;
      }
    }
  }
  if (type == HDC2X) {
    Wire.beginTransmission(addr);
    writeBuffer[0] = HDC2X_CONFIG_REG;
    writeBuffer[1] = HDC2X_CONFIG_DATA;
    for (int i = 0; i < HDC2X_CONFIG_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    delay(HDC2X_RESET_DURATION);
    Wire.beginTransmission(addr);
    Wire.write(HDC2X_READ_RH);
    Wire.endTransmission();
    delay(HDC2X_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)HDC2X_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < HDC2X_DATA_SIZE) {
        delay(HDC2X_MEASUREMENT_DELAY);
      } else {
        for (int i = 0; i < HDC2X_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        result =  (readBuffer[1] << 8) + readBuffer[0];
        result &= ~0x03;
        hum = (float)result;
        hum *= 100;
        hum /= 65536;
        break;
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
        delay(BME280_MEASUREMENT_DELAY / 4);
      } else {
        for (int i = 0; i < BME280_COEFF3_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        h1 = (uint8_t) readBuffer[BME280_H1];
        break;
      }
    }
    Wire.beginTransmission(addr);
    Wire.write(BME280_COEFF2_ADDR);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)addr, (uint8_t)BME280_COEFF2_SIZE);
    timeout = millis() + (DEFAULT_TIMEOUT / 2);
    while ( millis() < timeout) {
      if (Wire.available() < BME280_COEFF2_SIZE) {
        delay(BME280_MEASUREMENT_DELAY / 4);
      } else {
        for (int i = 0; i < BME280_COEFF2_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        h2 = (uint16_t) (((uint16_t) readBuffer[BME280_H2_MSB] << 8) | (uint16_t)(readBuffer[BME280_H2_LSB]));
        h3 = (uint8_t) readBuffer[BME280_H3];
        h4 = (int16_t) (((uint16_t) readBuffer[BME280_H4_MSB] << 4) | ((uint16_t)(readBuffer[BME280_H4_LSB]) & 0x0F));
        h5 = (int16_t) (((uint16_t) readBuffer[BME280_H5_MSB] << 4) | ((uint16_t)(readBuffer[BME280_H5_LSB]) >> 4));
        h6 = (int8_t) readBuffer[BME280_H6];
        break;
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
        delay(BME280_MEASUREMENT_DELAY / 4);
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
        break;
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
        delay(BME680_MEASUREMENT_DELAY / 4);
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
        break;
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
        temp_scaled =  (((int32_t) temp_comp_680 * 5) + 128) >> 8;
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
        hum = (float)calc_hum;
        hum /= 1000;
        break;
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
        delay(DHT12_MEASUREMENT_DELAY / 4);
      } else {
        for (int i = 0; i < DHT12_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        hum = (readBuffer[0] + (float) readBuffer[1] / 10);
        break;
      }
    }
  }
  if (type == AHT1X ) {
    uint32_t xresult;
    Wire.beginTransmission(addr);
    writeBuffer[0] = AHT1X_READ;
    writeBuffer[1] = AHT1X_READ_DATA0;
    writeBuffer[2] = AHT1X_READ_DATA1;
    for (int i = 0; i < AHT1X_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    delay(AHT1X_RH_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)AHT1X_DATA_SIZE);

    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < AHT1X_DATA_SIZE) {
        delay(AHT1X_RH_MEASUREMENT_DELAY / 4);
      } else {
        for (int i = 0; i < AHT1X_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        xresult = (((uint32_t)readBuffer[1] << 16) | ((uint32_t)readBuffer[2] << 8) | (uint32_t)readBuffer[3]) >> 4;
        hum = (float)xresult;
        hum *= 100;
        hum /= 1048576;
      }
    }
  }
  if (type == AM2320 ) {
    uint16_t xresult;
    //Wake up
    Wire.beginTransmission(addr);
    Wire.write(0x00);
    Wire.endTransmission();
    delay(2);
    Wire.beginTransmission(addr);
    writeBuffer[0] = AM2320_READ;
    writeBuffer[1] = AM2320_READ_H;
    writeBuffer[2] = AM2320_DATA_SIZE;
    for (int i = 0; i < AM2320_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    delay(AM2320_RH_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)AM2320_DATA_SIZE);

    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < AM2320_DATA_SIZE) {
        delay(AHT1X_RH_MEASUREMENT_DELAY / 4);
      } else {
        for (int i = 0; i < AM2320_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        xresult = ((uint16_t)readBuffer[2] << 8) + readBuffer[3] ;
        hum = (float)xresult;
        hum /= 10;
      }
    }
  }
}

void get_temperature () {
  uint16_t result = 0;
  clean_buffers();
  if ((type == SHT2X) | (type == SI70XX) | (type == HTU2X)) {
    Wire.beginTransmission(addr);
    Wire.write(SHT2X_READ_T);
    Wire.endTransmission();
    delay(SHT2X_T_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)SHT2X_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < SHT2X_DATA_SIZE) {
        delay(SHT2X_T_MEASUREMENT_DELAY / 4);
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
        break;
      }
    }
  }
  if ((type == SHT3X) | (type == SHT8X)) {

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
        delay(SHT3X_MEASUREMENT_DELAY / 4);
      } else {
        for (int i = 0; i < SHT3X_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        result = (readBuffer[0] << 8) + readBuffer[1];
        temp = (float)result;
        temp *= 175;
        temp /= 65535;
        temp -= 45;
        break;
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
        delay(HDC1X_MEASUREMENT_DELAY / 4);
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
        break;
      }
    }
  }
  if (type == HDC2X) {
    Wire.beginTransmission(addr);
    writeBuffer[0] = HDC2X_CONFIG_REG;
    writeBuffer[1] = HDC2X_CONFIG_DATA;
    for (int i = 0; i < HDC2X_CONFIG_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    delay(HDC2X_RESET_DURATION);
    Wire.beginTransmission(addr);
    Wire.write(HDC2X_READ_T);
    Wire.endTransmission();
    delay(HDC2X_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)HDC2X_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < HDC2X_DATA_SIZE) {
        delay(HDC2X_MEASUREMENT_DELAY / 4);
      } else {
        for (int i = 0; i < HDC2X_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        result =  (readBuffer[1] << 8) + readBuffer[0];
        result &= ~0x03;
        temp = (float)result;
        temp *= 165;
        temp /= 65536;
        temp -= 40;
        break;
      }
    }
  }

  if (type == BME280 ) {
    uint16_t t1;
    int16_t t2, t3;
    uint32_t xresult = 0;
    clean_buffers();
    Wire.beginTransmission(addr);
    Wire.write(BME280_COEFF1_ADDR);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)addr, (uint8_t)BME280_COEFF1_SIZE);
    timeout = millis() + (DEFAULT_TIMEOUT / 2);
    while ( millis() < timeout) {
      if (Wire.available() < (BME280_COEFF1_SIZE)) {
        delay(BME280_MEASUREMENT_DELAY / 4);
      } else {
        for (int i = 0; i < BME280_COEFF1_SIZE; i++) {
          readBuffer[i] = Wire.read();

        }
        t1 = (uint16_t) (((uint16_t)readBuffer[BME280_T1_MSB] << 8) | (uint16_t)readBuffer[BME280_T1_LSB]);
        t2 = (int16_t)(((uint16_t)readBuffer[BME280_T2_MSB] << 8) | (uint16_t)readBuffer[BME280_T2_LSB]);
        t3 = (int16_t)(((uint16_t)readBuffer[BME280_T3_MSB] << 8) | (uint16_t)readBuffer[BME280_T3_LSB]);
        break;
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
        int32_t var1, var2;
        int16_t calc_temp;
        xresult = (uint32_t) (((uint32_t) readBuffer[0] << 12) | ((uint32_t) readBuffer[1] << 4) | ((uint32_t) readBuffer[2] >> 4 ));
        var1 = (int32_t)((xresult / 8) - ((int32_t)t1 * 2));
        var1 = (var1 * ((int32_t)t2)) / 2048;
        var2 = (int32_t)((xresult / 16) - ((int32_t)t1));
        var2 = (((var2 * var2) / 4096) * ((int32_t)t3)) / 16384;
        temp_comp_280 = (int32_t)(var1 + var2);
        temp = (temp_comp_280 * 5 + 128) / 256;
        //temp = (var1 + var2) / 5120.0;
        temp /= 100;
        break;
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
        delay(BME680_MEASUREMENT_DELAY / 4);
      } else {
        for (int i = 0; i < BME680_COEFF1_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        t2 = (int16_t)(((uint16_t)readBuffer[BME680_T2_MSB] << 8) | (uint16_t)readBuffer[BME680_T2_LSB]);
        t3 = readBuffer[BME680_T3];
        break;
      }
    }
    Wire.beginTransmission(addr);
    Wire.write(BME680_COEFF2_ADDR);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)addr, (uint8_t)BME680_COEFF2_SIZE);
    timeout = millis() + (DEFAULT_TIMEOUT / 2);
    while ( millis() < timeout) {
      if (Wire.available() < BME680_COEFF2_SIZE) {
        delay(BME680_MEASUREMENT_DELAY / 4);
      } else {
        for (int i = 0; i < BME680_COEFF2_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        t1 = (((uint16_t)readBuffer[BME680_T1_MSB] << 8) | (uint16_t)readBuffer[BME680_T1_LSB]);
        break;
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
        xresult =  (uint32_t) (((uint32_t) readBuffer[5] << 12 )      | ((uint32_t) readBuffer[6] << 4 )    | ((uint32_t) readBuffer[7] >> 4 ));
        int64_t var1, var2, var3 ;
        int16_t calc_temp;
        var1 = ((int32_t) xresult >> 3) - ((int32_t) t1 << 1);
        var2 = (var1 * (int32_t) t2) >> 11;
        var3 = ((var1 >> 1) * (var1 >> 1)) >> 12;
        var3 = ((var3) * ((int32_t) t3 << 4)) >> 14;
        temp_comp_680  = (int32_t) (var2 + var3);
        calc_temp = (int16_t) ((( temp_comp_680 * 5) + 128) >> 8);
        temp = (float)calc_temp;
        temp /= 100;
        break;
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
        break;
      }
    }
  }
  if (type == AHT1X ) {
    uint32_t xresult;
    Wire.beginTransmission(addr);
    writeBuffer[0] = AHT1X_READ;
    writeBuffer[1] = AHT1X_READ_DATA0;
    writeBuffer[2] = AHT1X_READ_DATA1;
    for (int i = 0; i < AHT1X_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    delay(AHT1X_T_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)AHT1X_DATA_SIZE);

    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < AHT1X_DATA_SIZE) {
        delay(AHT1X_T_MEASUREMENT_DELAY / 4);
      } else {
        for (int i = 0; i < AHT1X_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        xresult = (((uint32_t)readBuffer[3] & 0x0F) << 16) | ((uint32_t)readBuffer[4] << 8) | (uint32_t)readBuffer[5];
        temp = (float)xresult;
        temp *= 200;
        temp /= 1048576;
        temp -= 50;
      }
    }
  }

  if (type == AM2320 ) {
    uint16_t xresult;
    //Wake up
    Wire.beginTransmission(addr);
    Wire.write(0x00);
    Wire.endTransmission();
    delay(2);
    Wire.beginTransmission(addr);
    writeBuffer[0] = AM2320_READ;
    writeBuffer[1] = AM2320_READ_T;
    writeBuffer[2] = AM2320_DATA_SIZE;
    for (int i = 0; i < AM2320_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    delay(AM2320_T_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)AM2320_DATA_SIZE);

    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < AM2320_DATA_SIZE) {
        delay(AHT1X_T_MEASUREMENT_DELAY / 4);
      } else {
        for (int i = 0; i < AM2320_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        xresult = ((uint16_t)(readBuffer[2] & 0x7F) << 8) + readBuffer[3];
        temp = (float)xresult;
        temp /= 10.0;
        temp = ((readBuffer[2] & 0x80) >> 7) == 1 ? temp * (-1) : temp;

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
  LCD.setFont(SmallFont);
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
        if ((type != EMPTY) & (type != DISABLED)) {
          get_temperature();
          get_humidity();
        }
        if ((type != EMPTY) & (type != DISABLED)) {
          if (colm != NOCOLM) {
            x = 2 + (colm * 48);
            y = 44 + (28 * bus);
            if ( type == SHT8X) {
              LCD.setColor(50, 255, 50);
            } else {
              LCD.setColor(50, 50, 255);
            }
            LCD.printNumF (hum, 2, x, y, '.', 5);
            Serial.print(hum);
            Serial.print(",");
            x = 2 + (colm * 48);
            y = 12 + 46 + (28 * bus);
            if ( type == SHT8X) {
              LCD.setColor(255, 255, 100);
            } else {
              LCD.setColor(255, 255, 0);
            }
            LCD.printNumF (temp, 2, x, y, '.', 5);
          }
          csvline1  += String(hum) + ",";
          csvline2  += String(temp) + ",";
          LCD.setFont(SmallFont);
          LCD.setBackColor(64, 64, 64);
          LCD.setColor(240, 240, 240);
          DateTime now = rtc.now();
          char datestr[32];
          snprintf(datestr, sizeof(datestr), "%4d-%02d-%02d", now.year(), now.month(), now.day());
          LCD.print (datestr, 910, 307);
          char timestr[9];
          snprintf(timestr, sizeof(timestr), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
          LCD.print (timestr, 2, 307);
          LCD.setBackColor(0, 0, 0);

        }
      }
    }
  }
  Serial.println();
  //Serial.println(csvline1);
  //Serial.println(csvline2);
  // Write to humidity log file
  DateTime now = rtc.now();
  char datestr[32];
  snprintf(datestr, sizeof(datestr), "%4d-%02d-%02d", now.year(), now.month(), now.day());
  char timestr[9];
  snprintf(timestr, sizeof(timestr), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  logfile1 = SD.open(rhFileName, FILE_WRITE );
  if (logfile1) {
    csvline1 = String(timestr) + "," + csvline1;
    logfile1.println(csvline1);
    //LCD.print(csvline1, LEFT, 280);
    logfile1.close();
  } else {
    LCD.print("Cannot write RH to file", LEFT, 280);
  }
  // Write to temperatures log file
  logfile2 = SD.open(tFileName, FILE_WRITE );
  if (logfile2) {
    csvline2 = String(timestr) + "," + csvline2;
    logfile2.println(csvline2);
    //LCD.print(csvline2, LEFT, 290);
    logfile2.close();
  } else {
    LCD.print("Cannot write t to file", LEFT, 290);
  }
}

void SDdateTime(uint16_t* sddate, uint16_t* sdtime) {
  DateTime now = rtc.now();
  *sddate = FAT_DATE(now.year(), now.month(), now.day());
  *sdtime = FAT_TIME(now.hour(), now.minute(), now.second());
}
void setup()
{
  Wire.begin();
  LCD.InitLCD();
  Serial.begin(115200);
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power!");
  }
  setupSD ();
  SdFile::dateTimeCallback(SDdateTime);
  drawTable ();
  initSensors ();
  delay(50);
  drawTable ();
}


void loop()
{
  readSensors ();
  delay(50);
  cycle++;
  if ( (cycle % 5) == 0 ) {
    drawTable ();
  }
}
