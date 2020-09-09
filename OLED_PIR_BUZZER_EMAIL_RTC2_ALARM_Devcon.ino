/////////////////////////////////////////////////////////////////////////////////////////////
//The schedules are set for every day at the same time period for the sake of this example //
/////////////////////////////////////////////////////////////////////////////////////////////


#include <NTPClient.h> //Library to update time from an internet time server
#include <WiFi.h> // Wifi Library
#include <WiFiUdp.h> // Wifi UDP Library
#include <Wire.h> // I2C Library communicates with the OLED and the RTC Module
#include <RtcDS3231.h> // Library for the DS3231 RTC module 
#include <Adafruit_SSD1306.h> // OLED with SSD1306 driver Library 
#include <Arduino.h>
#include <Time.h>
#include "ESP32_MailClient.h"


#define countof(a) (sizeof(a) / sizeof(a[0]))
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
//A global string to store the RTC Date Time
String GlobalRTCDateTime;
// GPIO Defined here as GPIO 16 as INPUT GPIO 17 as buzzer output 
const int PIRPIN = 16;
const int BUZZERPIN = 17;

//get the current time from time server for GMT +4 
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "africa.pool.ntp.org", 4*3600);//

//The Email Sending data object contains config and data to send
SMTPData smtpData;


// WIFI Credentials 

const char *ssid     = "Maker_Space";
const char *password = "ApolloRocks";
RtcDS3231<TwoWire> Rtc(Wire);

///////////////////////////////////
// Watch Daemon parameters       //
///////////////////////////////////

int StartHours = 9;
int EndHours = 23;
int SecInactivityAlert = 20; //number of iniactive second as Warning
int SecInactivityCritical = 40 ; // number of seconds as Critical
int CurrHr, CurrMin, CurrSec;
boolean AlertSent = false;
boolean CriticalSent = false;
boolean MonitoringStarted =false;
unsigned long AlertMillis;
unsigned long CriticalMillis;
String Recipient;

void setup() {
  Serial.begin(115200);
  delay(500);
  Rtc.Begin(); 
  
  
  
  
  Recipient="yovanfowdar@gmail.com";  
  pinMode(BUZZERPIN,OUTPUT);
  pinMode(PIRPIN,INPUT);
  WiFi.begin(ssid, password);
 
   
  //delay(20000); // Give some time for wifi to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Warning WiFi not connected using RTC Date Time");
  }
  UpdateRTC();
  Serial.println("RTC UPDATE PROCESS");
  delay(500);
  GetRTCTime();
  SendEmail(Recipient, "Elderly Monitoring System Status", "SYSTEM POWERED ON @ " +GlobalRTCDateTime ,1 ); 
  

  
  
  Serial.println("Starting Up");
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.clearDisplay();
  display.display();
  delay(1000);

}

void loop() {
delay(1000);
watchdaemon();

  
  int PIRState = digitalRead(PIRPIN);
  if ( PIRState == HIGH)
  {
    MovementDetected();
    digitalWrite(BUZZERPIN,HIGH);
    
     
  }
  else 
  {
    Monitoring();
    digitalWrite(BUZZERPIN,LOW); 
  }
 
}

//Callback function to get the Email sending status
void sendCallback(SendStatus info);


void MovementDetected()
{
   
   
   display.clearDisplay();
   display.setCursor(0,0);
   display.setTextSize(2); 
   display.setTextColor(SSD1306_WHITE);
   display.setCursor(16,25); 
   display.println("MOVEMENT");
   display.setCursor(16,40); 
   display.println("DETECTED");
   display.display();
   // If I was in a previous alert stage either where it was an Alert or Critical Type I will send a message to the recipient informing that movement has been detedted
   if ((AlertSent)||(CriticalSent))
   {
     SendEmail(Recipient, "Movement has been detected", "Please note that a movement has been detected by the system after an Alert, A Critical or both messages have been sent", 1) ;
 
   }
    InitialiseInactivityValues();
}


void Monitoring()
{
   display.clearDisplay();
   display.setTextSize(2); 
   display.setTextColor(SSD1306_WHITE);
   display.setCursor(5,30); 
   display.println(F("MONITORING"));   
   display.setCursor(20,30);
   display.display();
}

void UpdateRTC()
{
  // Do udp NTP lookup, epoch time is unix time - subtract the 30 extra yrs (946684800UL) library expects 2000 epoch time starts on 01/01/1970
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime()-946684800UL;
  Rtc.SetDateTime(epochTime);
}


void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];
    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Day(),
            dt.Month(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    //Serial.println(datestring);
    GlobalRTCDateTime = datestring;
    //Serial.println("");
    //Serial.print(" Current GlobalRTCDateTime -->");
    //Serial.println(GlobalRTCDateTime);
    CurrHr = (GlobalRTCDateTime.substring(11,13)).toInt();
    CurrMin = (GlobalRTCDateTime.substring(14,16)).toInt(); 
    CurrSec = (GlobalRTCDateTime.substring(17,19)).toInt(); 

}


boolean SendEmail(String recipientemail, String subject, String message, int messagetype)
{
  String wrapperstart, wrapperend;
  smtpData.setLogin("mail.genplusltd.com", 465, "devcon@genplusltd.com", "n#},zvr+MYF[");
  smtpData.setSender("Elderly Monitor Agent", "devcon@genplusltd.com");
  smtpData.setPriority("High");
  smtpData.setSubject(subject);
  if (messagetype == 1) 
  {
    //Information 
    wrapperstart =  "<h1><strong><span style='background-color: #339966;'>";
    wrapperend = "</span></strong></h1><p>&nbsp;</p>";
   }
  
  if (messagetype == 2) 
  {
    //Alert
    wrapperstart =  "<h1><strong><span style='background-color: #ff6600;'>";
    wrapperend = "</span></strong></h1><p>&nbsp;</p>";
   }

   if (messagetype == 3 ) 
  {
    //Critical 
    wrapperstart =  "<h1><strong><span style='background-color: #ff0000;'>";
    wrapperend = "</span></strong></h1><p>&nbsp;</p>";
   }

   
  String emailmessage = wrapperstart + message + wrapperend;
  smtpData.setMessage(emailmessage, true);
  smtpData.addRecipient(recipientemail);
  smtpData.setSTARTTLS(false);// important 
  //smtpData.setDebug(true);
  
  smtpData.setSendCallback(sendCallback);
    
  if (!MailClient.sendMail(smtpData))
    Serial.println("Error sending Email, " + MailClient.smtpErrorReason());

  //Clear all data from Email object to free memory
  smtpData.empty();



}


void sendCallback(SendStatus msg)
{
  //Print the current status
  Serial.println(msg.info());

  //Do something when complete
  if (msg.success())
  {
   // Serial.println("----------------");
  }
}




void watchdaemon()
{
  //This is the main logic of the device where alerts are sent
  
  GetRTCTime();

  if ((CurrHr >= StartHours) && (CurrHr <= (EndHours-1)))
  {
    if (! MonitoringStarted)
    {
      InitialiseInactivityValues();
      //Serial.print( "Current Millis -->");
      //Serial.println(millis());
      
      MonitoringStarted = true;
      //Serial.print( "Alert Millis -->");
      //Serial.println(AlertMillis);
      //Serial.print( "Critical Millis -->");
      //Serial.println(CriticalMillis);
           
    }
    
    if (MonitoringStarted)
    {
     //Alert Threshold
     String message;
      if(( millis() > AlertMillis) && (!AlertSent))
      {
//        Serial.print( " Current millis -->");
//        Serial.println(millis());
//        Serial.println( " Threshold of AlertMillis exceeded");
        if (!AlertSent)
        {
          message = "The system has not monitored any movement for more than " + String(SecInactivityAlert) + " Seconds ";
          SendEmail(Recipient, "Alert Threshold Esceeded", message, 2);
          AlertSent = true;
        }
      }
      
      //Critical Threshold
      if( (millis() > CriticalMillis) && (!CriticalSent)) 
      {
//        Serial.print( " Current millis -->");
//        Serial.println(millis());
//        Serial.println( " Threshold of CriticalMillis exceeded");
        if(!CriticalSent)
        {    
         message = "The system has not monitored any movement for more than " + String(SecInactivityCritical) + " Seconds ";
         SendEmail(Recipient, "Critical Threshold Esceeded", message, 3);
         CriticalSent = true;
        }
      }

    }
         
  } 
}
void InitialiseInactivityValues()
{

   AlertMillis = millis() + (SecInactivityAlert * 1000);
   CriticalMillis = millis() + (SecInactivityCritical * 1000);
   AlertSent = false;
   CriticalSent = false;
   Serial.println("Values Reinitialized");
   
   
}



void GetRTCTime(){
  RtcDateTime now = Rtc.GetDateTime();
  
  printDateTime(now); // this will update the global time variable GlobalRTCDateTime;
}
