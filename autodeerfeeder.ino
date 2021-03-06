/*
 * Auto Deer Feeder
 * version 1.0
 * (c) 2022 Nathan "nwb99" Barnett
 * GPLv3 (see LICENSE)
 */

#include <Wire.h>
#include <DS3231.h>

#define ALARM1_HR 23
#define ALARM1_M 39
#define ALARM1_S 30
#define ALARM1_INTPIN 2
#define RELAY_PIN 5

#define BUTTON 8

/*******************************/
// global variables for date time
byte hr;
byte minute;
byte sec;
byte dow; // day of week, 1-7
byte date;  // 1-31
byte mo;
byte yr;
byte Alarm1Bits = 0x08; // 1000 in binary. Alarm when hour, min, sec match
bool A1Dy = true;
bool A1h12 = false;
bool A1PM = false;

volatile bool alarmTriggered = false;

char instr[20]; // allocate 20 bytes for date string coming over serial. terminate with '\0'
const char notdt[26] = "Invalid date and/or time!";
const byte not_thone[4] = { 4, 6, 9, 11 }; // months not with 31 days
bool retFail;
/*******************************/

/*******************************/
// function prototypes
void retDateTime();
bool validDateTime();
void setDateTime();
void printDateTime();
void alarmTrigger();
/*******************************/

// initialize clock object
DS3231 dtclock;
DateTime dt;


void setup() {
  // initialize serial and i2c interfaces
  Serial.begin(9600);
  Wire.begin();

  pinMode(ALARM1_INTPIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ALARM1_INTPIN), alarmTrigger, FALLING);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  pinMode(BUTTON, INPUT_PULLUP);

  alarm();
}

void loop() {

  if (digitalRead(BUTTON) == LOW) {
    feeder();
  }
  
  if (Serial.available() > 0) {
    char in;
    in = Serial.read();
    
    switch (in) {
      case 'p':
        printDateTime();
        break;
      case 'w':
        retDateTime();
        if (!validDateTime()) {
          Serial.println(notdt);
        }
        else {
        setDateTime();
        printDateTime();
        }
        break;
      //case 'a':
       // alarm();
        //break;
      case 'y':
        dtclock.turnOnAlarm(1);
        if (dtclock.checkAlarmEnabled(1)) {
          Serial.println("Alarm 1 is enabled.");
        }
        else {
          Serial.println("Alarm 1 did not enable.");
        }
        break;
      case 'n':
        dtclock.turnOffAlarm(1);
        dtclock.checkIfAlarm(1);
        if (!dtclock.checkAlarmEnabled(1)) {
          Serial.println("Alarm 1 is disabled.");
        }
        else {
          Serial.println("Alarm 1 did not disable.");
        }
        break;
      case 'z':
        if (dtclock.checkAlarmEnabled(1)) {
          Serial.println("Alarm 1 is enabled.");
        }
        else {
          Serial.println("Alarm 1 is disabled.");
        }
        break;
    }
  }

  if (alarmTriggered == true) {
    feeder();
  }
}

/*******************************/
// begin retDateTime function
// Reads from serial in the order YYMMDDwHHMMSS$, where '$' (dollar sign) is the termination character.
// assumes 24-hour time format
void retDateTime() {

  bool eol = false;
  char inchar;
  byte i = 0;

  Serial.println("Enter date and time (YYMMDDwHHMMSS$). '$' indicates termination character.");
  Serial.println("'w' indicates day of week, 1-7.");
  while (!eol) {
    if (Serial.available()) {
      inchar = Serial.read(); // reads one byte at a time from serial buffer.
      if (i > 19) {
        Serial.println("Input buffer overflow! Input must be <19 characters.");
        Serial.println("Date/time not changed.");
        retFail = true;
        return;
      }
      else {
        instr[i] = inchar;
        ++i;
      }
      if (inchar == '$') {
        instr[i] = '\0';
        eol = true;
      }
    }
  }
}
// end retDateTime function
/*******************************/

/*******************************/
// begin validDateTime function
// checks that date time input is valid and write to appropriate variables
bool validDateTime() {
  byte temp1, temp2;

  // year
  temp1 = (byte)instr[0] - 48; // subtract 48 to convert from ASCII character
  temp2 = (byte)instr[1] - 48;
  yr = (temp1 * 10) + temp2;
  //Serial.print(yr);

  // month
  temp1 = (byte)instr[2] - 48;
  temp2 = (byte)instr[3] - 48;
  mo = (temp1 * 10) + temp2;
  //Serial.print(mo);
  
  // day
  temp1 = (byte)instr[4] - 48;
  temp2 = (byte)instr[5] - 48;
  date = (temp1 * 10) + temp2;
  //Serial.print(date);
  
  // day of week
  dow = (byte)instr[6] - 48;
  //Serial.print(dow);
  
  // hour
  temp1 = (byte)instr[7] - 48;
  temp2 = (byte)instr[8] - 48;
  hr = (temp1 * 10) + temp2;
  //Serial.print(hr);
  
  // minute
  temp1 = (byte)instr[9] - 48;
  temp2 = (byte)instr[10] - 48;
  minute = (temp1 * 10) + temp2;
  //Serial.print(minute);
  
  // second
  temp1 = (byte)instr[11] - 48;
  temp2 = (byte)instr[12] - 48;
  sec = (temp1 * 10) + temp2;
  //Serial.print(sec);
  //Serial.println();
  
  // check that input is within valid ranges
  if (yr > 99) {
    return false;
  }
  else if (mo < 1 || mo > 12) {
    return false;
  }
  else if (date < 1 || date > 31) {
    return false;
  }
  else if (dow < 1 || dow > 7) {
    return false;
  }
  else if (hr > 23) {
    return false;
  }
  else if (minute > 59) {
    return false;
  }
  else if (sec > 59) { // DOES NOT SUPPORT LEAP SECONDS!
    return false;
  }

  // check that date is valid for given month
  if (isleapYear((uint8_t)yr) && ((mo == 2) && (date < 29))) {
    return false;
  }
  else if ((mo == 2) && (date > 28)) {
    return false;
  }
  for (size_t i = 0; i < 4; ++i) {
    //Serial.println(not_thone[i]);
    if ((mo == not_thone[i]) && (date > 30)) {
      return false;
    }
  }
  return true;
}
// end validDateTime function
/*******************************/

/*******************************/
// begin setDateTime function
void setDateTime() {
  // write date and time to RTC
  dtclock.setClockMode(false); // 24-hour time format
  dtclock.setYear(yr);
  dtclock.setMonth(mo);
  dtclock.setDate(date);
  dtclock.setDoW(dow);
  dtclock.setHour(hr);
  dtclock.setMinute(minute);
  dtclock.setSecond(sec);
}
// end setDateTime function
/*******************************/

/*******************************/
// begin printDateTime function
void printDateTime() {
  dt = RTClib::now();
  // M/D/YY
  Serial.print("The current date and time: ");
  Serial.print(dt.month()); Serial.print("/");
  Serial.print(dt.day()); Serial.print("/");
  Serial.print(dt.year()); Serial.print("; ");
  if (dt.hour() < 10){
    Serial.print("0");
  }
  Serial.print(dt.hour()); Serial.print(":");
  if (dt.minute()< 10) {
    Serial.print("0");
  }
  Serial.print(dt.minute()); Serial.print(":");
  if (dt.second() < 10) {
    Serial.print("0");
  }
  Serial.println(dt.second());
}
// end printDateTime function
/*******************************/

/*******************************/
// begin alarm function
void alarm() {
  // is dow value arbitrary for 0x08?
  dtclock.setA1Time(7, ALARM1_HR, ALARM1_M, ALARM1_S, Alarm1Bits, A1Dy, A1h12, A1PM);
}
// end alarm function
/*******************************/

/*******************************/
// begin alarmTrigger function from interrupt
void alarmTrigger() {
  alarmTriggered = true;
}
// end alarmTrigger function
/*******************************/

/*******************************/
// begin alarmTrigger function
void feeder() {
  /*** reset alarm ***/
  alarmTriggered = false;
  dtclock.checkIfAlarm(1);
  /*******************/

  // trigger relay by pulling low
  digitalWrite(RELAY_PIN, LOW);
  delay(2000);
  digitalWrite(RELAY_PIN, HIGH);
}
// end alarmTrigger function
/*******************************/
