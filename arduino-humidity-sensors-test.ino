/*
   Multiple i2c humidity sensors tester.
   code for https://wiki.liutyi.info/display/ARDUINO/Test+i2c+humidity+sensors
   HW Modification: https://wiki.liutyi.info/display/ARDUINO/v10-AHT+Sensors+Board
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
#define APPNAME "Humidity sensors tester v10.1-AHT"
const String HEADER[5] { "REF", "AHT10", "AHT20", "AHT21", "AHT25"};
#define FOOTER "https://wiki.liutyi.info/"

// Variables for file rotation 000-999
#define t_base_name "t_10_"
#define rh_base_name "h_10_"
const uint8_t t_base_name_size = sizeof(t_base_name) - 1;
const uint8_t rh_base_name_size = sizeof(rh_base_name) - 1;
char tFileName[] = t_base_name "000.csv";
char rhFileName[] = rh_base_name "000.csv";

// Type of sensor - not in use in this version
#define EMPTY 0 /* slot is empty */
#define AHT 9 /* includes AHT10 */
#define DISABLED 17  /* disabled  */

// indexes name in sensor arrays
#define get_type 0 /* indexes name in sensor arrays */
#define get_collumn 1 /* indexes name in sensor arrays */
#define get_address 2 /* indexes name in sensor arrays */
#define get_color 3 /* indexes name in sensor arrays */
#define UNDEF 255 /* sensor have no position on display */
#define NOCOLM 254 /* do not display this sensor */

#define MUXES 4 /* multiplexer active on the board */
#define DEVS 1 /* max number of sensor on same i2c lane */
// Define number and addresses of multiplexers
uint8_t multiplexer[MUXES] = {112, 113, 114, 115};

// Sensor properties by [multiplexor][i2c_bus][number][get_type/get_collumn/get_address/get_color]
const uint8_t sensor[MUXES][8][DEVS][4] =
{
  {
    {  {AHT, 2, 56, 240} },
    {  {AHT, 2, 56, 240} },
    {  {AHT, 2, 56, 240} },
    {  {AHT, 2, 56, 240} },
    {  {AHT, 2, 56, 240} },
    {  {AHT, 2, 56, 240} },
    {  {AHT, 2, 56, 240} },
    {  {AHT, 2, 56, 240} }
  },
  {
    {  {AHT, 3, 56, 240} },
    {  {AHT, 3, 56, 240} },
    {  {AHT, 3, 56, 240} },
    {  {AHT, 3, 56, 240} },
    {  {AHT, 3, 56, 240} },
    {  {AHT, 3, 56, 240} },
    {  {AHT, 3, 56, 240} },
    {  {AHT, 3, 56, 240} }
  },
    {
    {  {AHT, 4, 56, 240} },
    {  {AHT, 4, 56, 240} },
    {  {AHT, 4, 56, 240} },
    {  {AHT, 4, 56, 240} },
    {  {AHT, 4, 56, 240} },
    {  {AHT, 4, 56, 240} },
    {  {AHT, 4, 56, 240} },
    {  {AHT, 4, 56, 240} }
  },
    {
    {  {AHT, 5, 56, 240} },
    {  {AHT, 5, 56, 240} },
    {  {AHT, 5, 56, 240} },
    {  {AHT, 5, 56, 240} },
    {  {AHT, 5, 56, 240} },
    {  {AHT, 5, 56, 240} },
    {  {AHT, 5, 56, 240} },
    {  {AHT, 5, 56, 240} }
  }
};
// Set header (sensor names) for csv file. leave empty if intend to substitute sensors time-to-time without firmware update
#define csvheader "Time,HIH7120,SHT85,AHT10,AHT10,AHT10,AHT10,AHT10,AHT10,AHT10,AHT10,AHT20,AHT20,AHT20,AHT20,AHT20,AHT20,AHT20,AHT20,AHT21,AHT21,AHT21,AHT21,AHT21,AHT21,AHT21,AHT21,AHT25,AHT25,AHT25,AHT25,AHT25,AHT25,AHT25,AHT25"
// Sensor communication variables
#define DEFAULT_TIMEOUT 300
#define AHT_CMD_SIZE 3
#define AHT_DATA_SIZE 6
#define AHT_MEASUREMENT_DELAY 60
#define AHT_RESET_DURATION 20
#define AHT_READ 0xAC /* t and RH Calibrated Measurement 1010 1100 */
#define AHT_READ_DATA0 0x33
#define AHT_READ_DATA1 0x00
#define AHT_INIT 0xE1 /* Init: 1110 0001 */
#define AHT_INIT_DATA0 0x08
#define AHT_INIT_DATA1 0x00
#define AHT_RESET 0xBA /* Reset: 1011 1010 */

#define HIH7X_CMD_SIZE 1
#define HIH7X_DATA_SIZE 4
#define HIH7X_MEASUREMENT_DELAY 45
#define HIH7X_NORMAL_MODE 0x80
#define HIH7X_COMMAND_MODE 0xA0
#define HIH7X_DUMMY_BYTE 0X00
#define HIH7X_DATA_FETCH 0xDF

#define SHT8X_CMD_SIZE 2
#define SHT8X_DATA_SIZE 6
#define SHT8X_MEASUREMENT_DELAY 16  /* HIGH = 15 MID=6 LOW=4 */
#define SHT8X_CLOCK_STRETCH 0x2C
#define SHT8X_HRES_READ 0x06
#define SHT8X_CONTROL 0x30
#define SHT8X_RESET 0xA2
#define SHT8X_HEATER_OFF 0x66
#define SHT8X_CLEAR_STATUS 0x41


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
uint8_t colg;
String csvline1 = "";
String csvline2 = "";
long seconds;
float hum = 0;
float temp = 0;
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
  for (int x = 96; x < 490; x += 96) {
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
  root_i2c_bus();
  init_sensor_sht85();
  for (mux = 0; mux < sizeof(multiplexer); mux++)
  {
    for (bus = 0; bus < 8; bus++)
    {
      choose_i2c_bus();
      for (dev = 0; dev < DEVS; dev++)
      {
        type = sensor[mux][bus][dev][get_type];
        addr = sensor[mux][bus][dev][get_address];
        colm = (sensor[mux][bus][dev][get_collumn] - 1);
        if (type != EMPTY) {
          init_sensor(type, addr);
        }
        if (colm != NOCOLM) {
          x = 5 + (colm * 96);
          y = 46 + (28 * bus);
          LCD.printNumI (addr, x, y);
        }
      }
    }
  }
}

void root_i2c_bus()
{
  for (uint8_t i = 0; i < sizeof(multiplexer); i++) {
    uint8_t addr = multiplexer[i];
    Wire.beginTransmission(addr);
    Wire.write(0);
    Wire.endTransmission();
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

void init_sensor_sht85()
{
   uint8_t iaddr=68;
   Wire.beginTransmission(iaddr);
   writeBuffer[0] = SHT8X_CONTROL;
   writeBuffer[1] = SHT8X_RESET;
   for (int i = 0; i < SHT8X_CMD_SIZE; i++) {
     Wire.write(writeBuffer[i]);
   }
   Wire.endTransmission();
}  
void init_sensor (uint8_t itype, uint8_t iaddr)
{
  clean_buffers();
    Wire.beginTransmission(iaddr);
    Wire.write(AHT_RESET);
    Wire.endTransmission();
    delay(AHT_RESET_DURATION);
    Wire.beginTransmission(addr);
    writeBuffer[0] = AHT_INIT;
    writeBuffer[1] = AHT_INIT_DATA0;
    writeBuffer[2] = AHT_INIT_DATA1;
    for (int i = 0; i < AHT_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
}

void get_t_and_rh_sht85 ()
{
    uint16_t result = 0;
    uint32_t xresult;
    uint8_t addr=68;
    clean_buffers();
    writeBuffer[0] = SHT8X_CLOCK_STRETCH;
    writeBuffer[1] = SHT8X_HRES_READ;
    Wire.beginTransmission(addr);
    for (int i = 0; i < SHT8X_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    delay(SHT8X_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)SHT8X_DATA_SIZE);
    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if  (Wire.available() < SHT8X_DATA_SIZE) {
        delay(SHT8X_MEASUREMENT_DELAY);
      } else {
        for (int i = 0; i < SHT8X_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        result = (readBuffer[3] << 8) + readBuffer[4];
        hum = (float)result;
        hum *= 100;
        hum /= 65535;
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
void get_t_and_rh_hih7120 ()
{
    uint32_t xresult;
    clean_buffers();
    uint8_t addr=39;
    Wire.beginTransmission(addr);
    writeBuffer[0] = HIH7X_DATA_FETCH;
    for (int i = 0; i < HIH7X_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    delay(HIH7X_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)HIH7X_DATA_SIZE);

    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < HIH7X_DATA_SIZE) {
        delay(HIH7X_MEASUREMENT_DELAY / 4);
      } else {
        for (int i = 0; i < HIH7X_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        xresult = (((uint32_t)(readBuffer[0] & 0x3F) << 8) | ((uint32_t)readBuffer[1]));
        hum = (float)xresult;
        hum *= 100;
        hum /= 16384;
        xresult = (((uint32_t)readBuffer[2]) << 6) | ((uint32_t)(readBuffer[3] & ~0x03) / 4);
        temp = (float)xresult;
        temp *= 165;
        temp /= 16384;
        temp -= 40;  
      }
    }        
}
void get_t_and_rh ()
{
  uint16_t result = 0;
  clean_buffers();
    uint32_t xresult;
    Wire.beginTransmission(addr);
    writeBuffer[0] = AHT_READ;
    writeBuffer[1] = AHT_READ_DATA0;
    writeBuffer[2] = AHT_READ_DATA1;
    for (int i = 0; i < AHT_CMD_SIZE; i++) {
      Wire.write(writeBuffer[i]);
    }
    Wire.endTransmission();
    delay(AHT_MEASUREMENT_DELAY);
    Wire.requestFrom((uint8_t)addr, (uint8_t)AHT_DATA_SIZE);

    timeout = millis() + DEFAULT_TIMEOUT;
    while ( millis() < timeout) {
      if (Wire.available() < AHT_DATA_SIZE) {
        delay(AHT_MEASUREMENT_DELAY / 4);
      } else {
        for (int i = 0; i < AHT_DATA_SIZE; i++) {
          readBuffer[i] = Wire.read();
        }
        xresult = (((uint32_t)readBuffer[1] << 16) | ((uint32_t)readBuffer[2] << 8) | (uint32_t)readBuffer[3]) >> 4;
        hum = (float)xresult;
        hum *= 100;
        hum /= 1048576;
        xresult = (((uint32_t)readBuffer[3] & 0x0F) << 16) | ((uint32_t)readBuffer[4] << 8) | (uint32_t)readBuffer[5];
        temp = (float)xresult;
        temp *= 200;
        temp /= 1048576;
        temp -= 50;
      }
    }
}


void print_data_lcd ()
{
            x = 4 + 2 + (colm * 96);
            y = 44 + (28 * bus);
            LCD.setColor(50, 50, 255);
            LCD.printNumF (hum, 2, x, y, '.', 5);
            Serial.print(hum);
            Serial.print(",");
            x = 48 + 2 + (colm * 96);
            y = 12 + 46 + (28 * bus);
            LCD.setColor(255, colg, 0);
            LCD.printNumF (temp, 2, x, y, '.', 5);
}

void print_time_lcd ()
{
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

void readSensors ()
{
  hum = 0;
  temp = 0;
  csvline1 = "";
  csvline2 = "";
  LCD.setFont(SmallFont);
  LCD.setBackColor(0, 0, 0);
  root_i2c_bus();
  get_t_and_rh_hih7120 ();
  colm=0; colg=255; bus=4; print_data_lcd ();
          csvline1  += String(hum) + ",";
          csvline2  += String(temp) + ",";
  get_t_and_rh_sht85 ();
  colm=0; colg=255; bus=0; print_data_lcd ();
          csvline1  += String(hum) + ",";
          csvline2  += String(temp) + ",";
  for (mux = 0; mux < sizeof(multiplexer); mux++ )
  {
    for (dev = 0; dev < DEVS; dev++)
    {
      for (bus = 0; bus < 8; bus++)
      {
        choose_i2c_bus();
        type = sensor[mux][bus][dev][get_type];
        addr = sensor[mux][bus][dev][get_address];
        colm = (sensor[mux][bus][dev][get_collumn] - 1);
        colg = sensor[mux][bus][dev][get_color];
        hum = 0;
        temp = 0;
        if ((type != EMPTY) & (type != DISABLED)) {
          get_t_and_rh();
        }
        if ((type != EMPTY) & (type != DISABLED)) {
          if (colm != NOCOLM) {
           print_data_lcd ();
          }
          csvline1  += String(hum) + ",";
          csvline2  += String(temp) + ",";
          print_time_lcd ();
        }
      }
    }
  }
  Serial.println();
  Serial.println(csvline1);
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
  Wire.setClock(100000L);
  LCD.InitLCD();
  Serial.begin(115200);
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
