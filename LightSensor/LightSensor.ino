// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include <Wire.h>
#include <avr/power.h>
#include "RTClib.h"
#include <SPI.h>
#include <SD.h>
#include "LowPower.h"
#include "AS726X.h"

#define VBATPIN A9
#define LOWPOWER; // Disable serial print

// reat time clock
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Light sensor
AS726X sensor;

// SD card
const int chipSelect = 4;
int i = 0;
unsigned long last_file_size = 0;
String fileName;
File dataFile;

void error() {
  while (1) {
    pinMode (LED_BUILTIN, OUTPUT);
    digitalWrite (LED_BUILTIN, HIGH);
    delay (100);
    digitalWrite (LED_BUILTIN, LOW);
    delay (100);
  }
}

DateTime get_now() {
  power_twi_enable();
  DateTime now = rtc.now();
  power_twi_disable();
  return now;
}

void print_time() {
  DateTime now = get_now();
  Serial.begin(9600);
  while (!Serial) { }  // wait for Serial to initialize
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.print(") ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  Serial.flush();
  Serial.end();
}

void flash() {
  pinMode (LED_BUILTIN, OUTPUT);
  digitalWrite (LED_BUILTIN, HIGH);
  delay (50);
  digitalWrite (LED_BUILTIN, LOW);
  delay (50);
  digitalWrite (LED_BUILTIN, HIGH);
  delay (50);
  digitalWrite (LED_BUILTIN, LOW);
  pinMode (LED_BUILTIN, INPUT);
}

String gatherMeasurements() {
  power_twi_enable();
  if (!sensor.isalive()) error();
  String measurements = "";
  sensor.takeMeasurements();
  byte tempC = sensor.getTemperature();
  measurements += String(tempC);
  measurements += ",";
  measurements += String(sensor.getCalibratedViolet());
  measurements += ",";
  measurements += String(sensor.getCalibratedBlue());
  measurements += ",";
  measurements += String(sensor.getCalibratedGreen());
  measurements += ",";
  measurements += String(sensor.getCalibratedYellow());
  measurements += ",";
  measurements += String(sensor.getCalibratedOrange());
  measurements += ",";
  measurements += String(sensor.getCalibratedRed());
  power_twi_disable();
  return measurements;
}

void prepare_SD_file() {
  Serial.println("Open File...");
  DateTime now = get_now();
  fileName = "";
  fileName += String(now.year());
  char buff[4];
  sprintf(buff, "%02d", now.month());
  fileName += String(buff);
  sprintf(buff, "%02d", now.day());
  fileName += String(buff);
  fileName += ".txt";
  File dataFile = SD.open(fileName, FILE_WRITE);

  if (!dataFile) {
    Serial.print("error opening ");
    Serial.println(fileName);
    error();
  }
  last_file_size =  dataFile.size();

  Serial.print(fileName);
  Serial.println(" Prepared");

  dataFile.println("UnixTime, VBat, T°C, V, B, G, Y, O, R");
  dataFile.close();
  dataFile = SD.open(fileName, FILE_WRITE);
}

void write_to_SD() {
  String dataString = String(get_now().unixtime());
  dataString += ",";

  // Bat voltage
  power_adc_enable();
  ADCSRA |= 1 << 7;
  float measuredvbat = analogRead(VBATPIN);
  ADCSRA = 0;
  power_adc_disable();
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  dataString += String(measuredvbat);
  dataString += ",";

  // Light sensor
  dataString += gatherMeasurements();

  // SD card write
  power_spi_enable();
  dataFile.println(dataString);
#ifndef LOWPOWER
  Serial.println(dataString);
#endif
  if (i >= 8) {
    dataFile.flush();
    dataFile.close();
    dataFile = SD.open(fileName, FILE_WRITE);
    if (!dataFile) error();
    unsigned long file_size =  dataFile.size();
    if (file_size <= last_file_size) error();
    last_file_size = file_size;
    i = 0;
  }
  i++;
  power_spi_disable();
}

void power_all() {
  power_adc_enable(); // ADC converter
  power_spi_enable(); // SPI
  power_usart0_enable(); // Serial (USART)
  power_timer0_enable(); // Timer 0
  power_timer1_enable(); // Timer 1
  power_timer2_enable(); // Timer 2
  power_twi_enable(); // TWI (I2C)
}

void power_down() {
  // Disable USB clock
  USBCON |= _BV(FRZCLK);
  // Disable USB PLL
  PLLCSR &= ~_BV(PLLE);
  // Disable USB
  USBCON &= ~_BV(USBE);

  // disable ADC
  ADCSRA = 0;

  power_adc_disable(); // ADC converter
  power_twi_disable(); // TWI (I2C)
  power_spi_disable(); // SPI
  //power_timer0_disable();// Timer 0
  power_timer1_disable();// Timer 1
  power_timer2_disable();// Timer 2

#ifdef LOWPOWER
  power_usart0_disable();// Serial (USART)
  Serial.end();
#endif
}

void printDirectory(File dir) {
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    if (!entry.isDirectory()) {
      Serial.print(entry.name());
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void upload_files() {
  File root = SD.open("/");
  printDirectory(root);
  root.close();
  
  while (true) {
    Serial.println("Waiting for choice");
    delay(500);
    if (Serial.available() > 0) {
      String filetoopen = Serial.readString();
      // Length (with one extra character for the null terminator)
      int str_len = filetoopen.length() + 1; 
      // Prepare the character array (the buffer) 
      char char_array[str_len];
      
      // Copy it over 
      filetoopen.toCharArray(char_array, str_len);
      File dataFile = SD.open(char_array);
      Serial.println("Received file to open");
      Serial.println(filetoopen);

      // if the file is available, write to it:
      if (dataFile) {
        Serial.println("Prepare Sending File");
        while (dataFile.available()) {
          Serial.write(dataFile.read());
        }
        Serial.println("Done");
        dataFile.close();
      }
      // if the file isn't open, pop up an error:
      else {
        Serial.println("error opening " + filetoopen );
      }
    }
  }

}

void setup() {
  power_all();
  USBDevice.attach();
  delay(1000);

  Serial.begin(9600);
  delay(500);
  Serial.println("Starting");

  Serial.println("Initializing SD card...");
  delay(100);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    error();
  }
  Serial.println("card initialized.");

  // Check for pc connection to upload files
  bool pc_connected = 0;
  for (uint8_t i = 0; i <= 10; i++) {
    Serial.println("Waiting for PC...");
    if (Serial.available() > 0) {
      Serial.println("PC Connected");
      Serial.read();
      pc_connected = 1;
      break;
    }
    delay(1000);
  }
  if (pc_connected) upload_files();

  Serial.println("No PC Connected");

  prepare_SD_file();

  Serial.println("Initializing spectral sensor...");
  delay(100);
  power_twi_enable();
  if (!sensor.isalive()) error();
  if (sensor.begin(Wire, 3, 3) == 0) error();

  Serial.println("Initializing RTC...");
  delay(100);
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    error();
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power!");
    error();
  }
      
  Serial.println("Going into low power mode");
  delay(100);
  // Power Saving
  power_down();
}

void loop() {
  flash();
  write_to_SD();
#ifdef LOWPOWER
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
#else
  delay(1000);
#endif
}
