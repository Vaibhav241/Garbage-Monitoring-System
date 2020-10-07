#include <avr/sleep.h>//this AVR library contains the methods that controls the sleep modes
#define interruptPin 2 //Pin we are going to use to wake up the Arduino
#include <DS3232RTC.h>  //RTC Library https://github.com/JChristensen/DS3232RTC
#include <avr/power.h>

////Variables needed for the ULTRASONIC  Sensor
#define Battery_Pin     A0        //battery read pin
#define TIP_122_Pin     4         // trigger pin for battery
#define ULN2803A_Pin    12         // trigger pin for battery
#define trigPin         10          //U_S
#define echoPin         9          //U_S
//INITIALISATION
#define INIT_DATA       1          //U_S
static int  init_value       =     1;

//    GSM SETTINGS
#define MOBILE_NUMBER     "8377920943"
#define APN             "www"
#define CLIENT_API_URL "website name"

const String CID        =      "Enter CID";
//const String Device_id  =      "DelhiGD-001";        //  6 min DONE
//const String Device_id  =      "DelhiGD-002";         // 6 hours DONE
//const String Device_id  =      "DelhiGD-003";         //6 hours DONE
//const String Device_id  =      "DelhiGD-004";         //6 hours DONE
const String Device_id  =      "DelhiGD-005";       //6 hours DONE
String Date_time ;


String HTTP_POST_PARAMS;        // AS per client GD is sensor data , DC is battery data and DT is date time, DI is device ID,  CID is fixed and is given by client
// Delay time
int delay_val           =     3000;


//  Private Function define
void gsm_http_post ();
void Ultrasonic_data (void);
void Battery_Read (void);
void Update_data (float, float, String);

// defining variables
static int min_time     =     0;
static int hour_time    =     0;

// battery paraneter
int analog_value;
float Batt_Voltage;
float Divider_Voltage;
const float R1          =     7500;
const float R2          =     6200;

//ultrasonic parameter
long duration;
float distance_cm;
float distance_mt;
String strn;

// FOr Payload time set in hours
const int time_interval =     6; // Sets the wakeup intervall in Hours


// Initialization function
void setup() {

  //Start Serial Comunication for Serial1 and Serial2
 // Serial.begin(9600);
  Serial2.begin(9600);

  // Pin configuration
  pinMode(ULN2803A_Pin, OUTPUT); // for GSM trigger
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(TIP_122_Pin, OUTPUT); // for battery read and trigger
  pinMode(interruptPin, INPUT_PULLUP); //Set pin d2 to input using the buildin pullup resistor

  // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
  RTC.setAlarm(ALM1_MATCH_HOURS , 0, 0 , 0, 1);
  RTC.setAlarm(ALM1_MATCH_HOURS , 0, 0, 0, 1);

  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_1, false);
  RTC.alarmInterrupt(ALARM_2, false);
  RTC.squareWave(SQWAVE_NONE);
  /*
     Uncomment the block block to set the time on your RTC. Remember to comment it again
     otherwise you will set the time at everytime you upload the sketch
  */
  //  Begin block
      /*  RTC SET HERE    */
//        tmElements_t tm;
//              tm.Hour = 16;               // set the RTC to an arbitrary time
//              tm.Minute = 58;
//              tm.Second = 50;
//              tm.Day = 24;
//              tm.Month = 4;
//              tm.Year = 2019 - 1970;      // tmElements_t.Year is the offset from 1970
//              RTC.write(tm);              // set the RTC from the tm structure
        /*RTC SET TILL HERE*/
  //   Block end

  time_t t; //create a temporary time variable so we can set the time and read the time from the RTC
  t = RTC.get(); //Gets the current time of the RTC

  // for hour trigger
  hour_time = hour(t);
  hour_time +=  time_interval;
  RTC.setAlarm(ALM1_MATCH_HOURS , 0, 0, hour_time, 0); // Setting alarm 1 to go off 5 minutes from now


  // clear the alarm flag
  RTC.alarm(ALARM_1);
  // configure the INT/SQW pin for "interrupt" operation (disable square wave output)
  RTC.squareWave(SQWAVE_NONE);
  // enable interrupt output for Alarm 1
  RTC.alarmInterrupt(ALARM_1, true);
}

void loop() {
  if (init_value == INIT_DATA)
  {
    init_value = 0;
    digitalWrite(ULN2803A_Pin, HIGH);
    delay(10000);
    Ultrasonic_data();
    digitalWrite(ULN2803A_Pin, LOW);
  }
  else
  {
    Going_To_Sleep();
  }
}


/* The program will continue from here after the timer timeout*/


void Going_To_Sleep() {
  digitalWrite(ULN2803A_Pin, LOW);
  sleep_enable();//Enabling sleep mode
  attachInterrupt(0, wakeUp, LOW);//attaching a interrupt to pin d2
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);//Setting the sleep mode, in our case full sleep
 sleep_cpu();//activating sleep mode
// Serial.println("just woke up!");//next line of code executed after the interrupt

  Ultrasonic_data();//function that reads Ultrasonic data in cm with pin 9 and 10
  time_t t;
  t = RTC.get();
  //  for Set  hour_time New Alarm setting
  hour_time +=  time_interval;
  if (hour_time > 23)
  {
    hour_time = 0;
  }
 RTC.setAlarm(ALM1_MATCH_HOURS , 0,  0 , hour_time, 0);

  // clear the alarm flag
  RTC.alarm(ALARM_1);
}

void wakeUp() {
 sleep_disable();//Disable sleep mode
  detachInterrupt(0); //Removes the interrupt from pin 2;

}

//This function reads the temperature and humidity from the DHT sensor
void Ultrasonic_data() {
  float avrg = 0;
  float sum = 0;
  for (int i = 0; i < 20; i++)
  {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    //duration = pulseIn(echoPin, HIGH);
    avrg = pulseIn(echoPin, HIGH);
    sum += avrg;
    delay(1);
  }

  duration = sum / 20;
  distance_cm = duration * 0.034 / 2;
  delay(1000);
  Battery_Read ();// For Battery read pin A0
}

void Battery_Read ()
{
  digitalWrite(TIP_122_Pin, HIGH);
  analog_value = analogRead(Battery_Pin);

  Divider_Voltage = ((analog_value * 5.0) / 1023);

  Batt_Voltage =  ((R1 + R2) * Divider_Voltage) / R2;
  if (Batt_Voltage > 8.5)
  {
    digitalWrite(ULN2803A_Pin, HIGH);
    Update_data(distance_cm, Batt_Voltage, Date_time); //function for update all the data with time
    delay(5000);
    digitalWrite(ULN2803A_Pin, LOW);
  }
  else if (Batt_Voltage <= 8.5 && init_value == 0)
  {
    init_value == 2;
    digitalWrite(ULN2803A_Pin, HIGH);
    sms();
    Update_data(distance_cm, Batt_Voltage, Date_time);
    delay(5000);
    digitalWrite(ULN2803A_Pin, LOW);
  }
  else
  {
    //DO nothing
  }
 delay(10);
  digitalWrite(TIP_122_Pin, LOW);



}

void Update_data(float distance_value, float Batt_Value, String Date_data)
{
  time_t t;
  t = RTC.get();
  String sec_value = String(second(t));
  String min_value = String(minute(t));
  String hour_value = String(hour(t));
  String day_value = String(day(t));
  String month_value = String(month(t));
  String year_value = String(year(t));

  if (second(t) < 10)
  {
    sec_value = "0" + String(second(t));
  }
  if (minute(t) < 10)
  {
    min_value = "0" + String(minute(t));
  }
  if (hour(t) < 10)
  {
    hour_value = "0" + String(hour(t));
  }
  if (day(t) < 10)
  {
    day_value = "0" + String(day(t));
  }
  if (month(t) < 10)
  {
    month_value = "0" + String(month(t));
  }
  //   if(year(t)<10)
  //  {
  //    year_value= "0" + String(year(t));
  //  }

  Date_data = String(year(t)) + "-" + month_value + "-" + day_value + "_" + hour_value + ":" + min_value + ":" + sec_value;
  HTTP_POST_PARAMS = "CID=" + CID + "&" + "DI=" + Device_id + "&" + "GD=" + String(distance_value) + "&" + "DC=" + String(Batt_Value) + "&" + "DT=" + Date_data;

  gsm_http_post();// for posting data
  //sms();

}

void gsm_http_post ()
{
  delay(10000);
  Serial2.println("AT");
  // send_to_serial();//Print GSM Status an the Serial Output;
  delay(1000);

  Serial2.println("AT+CGATT?");
  // send_to_serial();//Print GSM Status an the Serial Output;
  delay(1000);

  Serial2.println("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  //send_to_serial();//Print GSM Status an the Serial Output;
  delay(delay_val);

  Serial2.println("AT+SAPBR=3,1,\"APN\",\"" + String(APN) + "\"");
  // send_to_serial();//Print GSM Status an the Serial Output;
  delay(delay_val);

  Serial2.println("AT+SAPBR=1,1");
  // send_to_serial();//Print GSM Status an the Serial Output;
  delay(delay_val);

  Serial2.println("AT+SAPBR=2,1");
  //  send_to_serial();//Print GSM Status an the Serial Output;
  delay(delay_val);

  Serial2.println("AT+HTTPINIT");
  // send_to_serial();//Print GSM Status an the Serial Output;
  delay(delay_val);

  Serial2.println("AT+HTTPPARA=\"CID\",1");
  // send_to_serial();//Print GSM Status an the Serial Output;
  delay(delay_val);

  Serial2.println("AT+HTTPPARA=\"URL\",\"" + String(CLIENT_API_URL) + "\"");
  // send_to_serial();//Print GSM Status an the Serial Output;
  delay(delay_val);

  Serial2.println("AT+HTTPPARA=\"CONTENT\",\"application/x-www-form-urlencoded\"");
  // send_to_serial();//Print GSM Status an the Serial Output;
  delay(delay_val);

  Serial2.println("AT+HTTPDATA=" + String(HTTP_POST_PARAMS.length()) +  ",10000");
  //  send_to_serial();//Print GSM Status an the Serial Output;
  delay(delay_val);
  delay(delay_val);

  Serial2.println(HTTP_POST_PARAMS);
  //  send_to_serial();//Print GSM Status an the Serial Output;
  delay(5000);

  Serial2.println("AT+HTTPACTION=1");
  //  send_to_serial();//Print GSM Status an the Serial Output;
  delay(5000);

  //Terminate HTTP & GPRS and then switch off
  Serial2.println("AT+HTTPTERM");
  //  send_to_serial();//Print GSM Status an the Serial Output;
  delay(delay_val);

  Serial2.println("AT+SAPBR=0,1");
  //  send_to_serial();//Print GSM Status an the Serial Output;
  delay(delay_val);

  Serial2.println("");
  delay(1000);
}

//void send_to_serial() {
//  while (Serial2.available() != 0)
//  {
//    Serial.write(Serial2.read());
//  }
//
//}

void sms()
{
  delay(10000);
  String sms_data;
  sms_data = Device_id + " DISTANCE = " + String(distance_cm) + "cm"   + " & "  + "BATTERY= " + String(Batt_Voltage) + "V" ;
  Serial2.println("AT\r");//
  delay(1000);
  Serial2.println("AT+CMGF=1");// select message mode
  delay(1000);
  //  Serial2.println("AT+CMGS=\"8800674575\"\r");//REMOVE 0322000000 AND ENTER YOUR RECEIVER PHONE NO HERE
  Serial2.println("AT+CMGS=\"" + String(MOBILE_NUMBER) + "\"\r"); //REMOVE 0322000000 AND ENTER YOUR RECEIVER PHONE NO HERE
  delay(1000);
  Serial2.println(sms_data);
  Serial2.write((char)26);// CTRL+Z SENDING CODE CTRL+Z=0x26
  delay(10000);
  //   Serial.println("DONE SMS");//

}
