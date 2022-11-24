#define BLYNK_TEMPLATE_ID           "TMPLpJeyYrSj"
#define BLYNK_DEVICE_NAME           "Megatoniac Home"
#define BLYNK_AUTH_TOKEN            "RipTcM-dIU1B7VXZYl1VXsnMyJlnXwsY"
//#define LINE_TOKEN "xIlivTYGOQsvsbMbDU4Wa4vx9turg9SFECvsDvmLh4S"
#define LINE_TOKEN  "T5qTUoUaVUMkF2pMLbFH6zdurN9ALX6Q9l6Ew6oePTq" //group

#define BLYNK_PRINT Serial

#include <M5Stack.h>                // Import library M5 stack
#include <WiFi.h>                   // Import library WiFi
#include <WiFiClient.h>             // Import library 
#include <BlynkSimpleEsp32.h>       // Import library Blynk
#include <TridentTD_LineNotify.h>   // Import library Line notify
#include <Wire.h>                   // Import library 
#include <DHT12.h>                  // Import library DHT12 sensor
#include <Keypad.h>                 // Import library Keypad

//use xTaskCreate to multi thread
//use Timer  of Blynk library

char auth[] = BLYNK_AUTH_TOKEN;

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "RYUNB";              // WiFi name that is connected
char pass[] = "turtlebot";          // WiFi password that is connected
DHT12 dht12;                        // Rename variable
BlynkTimer timer;

//KeyBoard setup
const byte rows = 4;                  // Define number of row for keypad
const byte cols = 3;                  // Define number of column for keypad
byte rowPins[rows] = {17, 16, 2, 5};  // Define row pin for keypad
byte colPins[cols] = {12, 13, 15};    // Define column pin for keypad
//mapping keypad
char keys[rows][cols] =
{
  {'1','2','3'}, 
  {'4','5','6'}, 
  {'7','8','9'},
  {'-','0','-'}
};
// define variable for keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols);

//Unlock key 
String passd = String(1234) ; //Define default password

// This function is called every time the Virtual Pin 0 state changes
BLYNK_WRITE(V0)
{
  // Set incoming value from pin V0 to a variable
  int value = param.asInt();

  // Update state
  Blynk.virtualWrite(V1, value);
}
bool FanStatus = false;
bool FanStatusB = false;
BLYNK_WRITE(V6)//fan
{
  // Set incoming value from pin V0 to a variable
  int fanvalue = param.asInt();
  if(fanvalue==1){
    FanStatusB = true;
  }
  else{FanStatusB = false;}
  // Update state
}
bool LightStatus = false;   
BLYNK_WRITE(V7)//light
{
  // Set incoming value from pin V0 to a variable
  int lightvalue = param.asInt();
  if(lightvalue==1){
    LightStatus = true;
  }
  else{LightStatus = false;}
  // Update state  
}

BLYNK_CONNECTED()
{
  Blynk.syncVirtual(V6);
  Blynk.syncVirtual(V7);
}


void myTimerEvent()//check uptime
{
   Blynk.virtualWrite(V2, millis() / 1000);
}
bool templine = false;      // Define logic about temperature to change state
bool LightLine = false;     // Define logic about light to change state
const int Apin = 34;        // Define light sensor pin
const int Decis = 2600;     // Define light value to change the state

void sendSensor(){ 
  float t =dht12.readTemperature();   // Read and collect temperature value 
  if (t >=30){
    if (templine==false){
    LINE.notify("Temp is too high!");             // Notify to LINE
    LINE.notify("Ventilation fan is turn on");    // Notify to LINE
    FanStatus = true;                             //
    templine = true;                              //
    }   
  }
  else if (t<=28){templine = false;FanStatus = false;}
    float hum = dht12.readHumidity();   // Read and collect humudity value
  
  uint16_t LightVal = analogRead(Apin); // Read and collect humudity value
  
  if (LightVal > Decis) {
      Serial.println("Light value is too low."); // Show light status in serial monitor 
      if(LightLine == false){
        LINE.notify("Recommend to turn the light on."); // Notify to LINE
        LightLine = true;       
      }
  }
  else if(LightVal < Decis-300) {
    LightLine = false;
    //LightStatus = false;
    Serial.println("Light value is " + String(LightVal)); // Show light value in serial monitor 
  } 
  Serial.println(LightLine);  
  Blynk.virtualWrite(V4,t);
  Blynk.virtualWrite(V5,hum);
  float Per = (4096-LightVal)*100/4096;   // Map light value from sensor to percentage
  Serial.println(Per);                    // Show light value in percentage in serial monitor 
  Blynk.virtualWrite(V8,Per);
  Serial.println(t);                      // Show temperature in degree Celcius in serial monitor 
  Serial.println(hum);                    // Show humidit in percentage in serial monitor 
  
}

void Light(){                             //control light
  if(LightStatus == true){

      digitalWrite(25,HIGH);
      Serial.println("Light on");       // Show status in serial monitor 
  }
  if(LightStatus == false){ 

      digitalWrite(25,LOW);
      Serial.println("Light off");      // Show status in serial monitor 
  }
}

void Fan(){                             //control fan
  if(FanStatus == true ||FanStatusB ==true ){
      //digitalWrite(25,LOW);
      digitalWrite(26,HIGH);
      Serial.println("Mot on");
  }
  else{ 
      //digitalWrite(25,LOW);
      digitalWrite(26,LOW);
      Serial.println("Mot off");
  }
}
void AccessScreen() {            // Define function for LCD interface which use for unlock the door
  M5.Lcd.setCursor(110, 90);
  M5.Lcd.println("1   2   3");
  M5.Lcd.setCursor(110, 120);
  M5.Lcd.println("4   5   6");
  M5.Lcd.setCursor(110, 150);
  M5.Lcd.println("7   8   9");
  M5.Lcd.setCursor(110, 180);
  M5.Lcd.println("-   0   -");
}
  enum State {Lock, Unlock, PasswordLock, ChangePass}; // Define name state
  enum State myState;
  
int i = 0 ;
int currentTime = 0 ;         // Define variable which will use for timer
int settingTime = 30000 ;     // Set lock time if password incorrect for 3 times
int Time1 = 0 ;               // Define variable which will use for timer
int Time2 = 0 ;               // Define variable which will use for timer
String typing = "" ;          // Define variable which input password from user 
String passdNew = "" ;        // Define variable for new password
void Door(void* pvParameters) { //thread 1 task
  while (1) {
    M5.update(); 
    Serial.print(myState);
    switch (myState)
    {
      case Lock:                              // Lock state
        M5.Lcd.clear(BLACK);
        if(i<3){                              // Condition to lock the door if password is incorrect 3 times 
          M5.Lcd.setTextSize(2);
          M5.Lcd.setCursor(80, 20);
          M5.Lcd.println("Access Passcode");
          AccessScreen();                     // Show LCD interface
          M5.Lcd.setCursor(140, 55);
          typing = "" ; //enter password      // Make variable blank 
          while(typing.length() != 4 )        // Waiting for password which contains 4 digit
          {
            char key = keypad.getKey();       // Read value from keypad 
            M5.Lcd.print(key); 
            typing += String(key);            // Collect variable from keypad to password
          } 
          delay(500);
          M5.Lcd.clear(BLACK);
          if(typing == passd) {               // Password is correct
              myState = Unlock ;              // Chang state 
              Time1 = millis() ;              // Collect time from timer as variable
              i = 0 ;                         // Reset times of incorrect
          }
          else {                              // Password is incorrect
            i++ ;                             // Add times of incorrect
            for (byte i=0 ; i<3 ; i++){       // show that "Pass is incorrect" in LCD
              M5.Lcd.setCursor(65, 50);
              M5.Lcd.print("Pass is incorrect");
              //Serial.print("Pass is incorrect");
              delay(200);
              M5.Lcd.clear(BLACK);
              delay(200);
            }
          }
          typing = "" ;                       // Reset typing password variable
        }
        else{                                 // Password is incorrect for 3 times
          LINE.notify("Someone try to get in your home!"); // Notify to LINE
          myState = PasswordLock ;            // Change state
          Time1 = millis() ;                  // Collect time from timer as variable
          M5.Lcd.setCursor(115, 50);
          M5.Lcd.print("Lock: ");
        } 
        break;

      case PasswordLock :                     // State when password is incorrect for 3 times
        Time2 = millis() ;                    // Collect time from timer as variable
        currentTime = Time2 - Time1 ;         // Collect current time
        if (currentTime >= settingTime){      // time out
          myState = Lock ;                    // Change state
          i = 0 ;                             // Reset times of incorrect
        }
        else{
          M5.Lcd.setCursor(185, 50);
          M5.Lcd.print((settingTime - currentTime)/1000); // Mapping time to second
          delay(500);
          //Lcd.clear(BLACK);
          M5.Lcd.setTextColor(BLACK);
          M5.Lcd.setCursor(185, 50);
          M5.Lcd.print((settingTime - currentTime)/1000); // Show lock time in LCD 
          M5.Lcd.setTextColor(WHITE);
        }
        break;
        
      case Unlock :                           // Unlock state
        for (byte i=0 ; i<3 ; i++){           // show that "Unlock" in LCD
          M5.Lcd.setTextSize(6);
          M5.Lcd.setCursor(25, 80);
          M5.Lcd.print("Unlock");
          Serial.print("Unlock");
          delay(200);
          M5.Lcd.clear(BLACK);
          delay(200);
        }
        M5.Lcd.setTextSize(6);
        M5.Lcd.setCursor(25, 80);
        M5.Lcd.print("Open");               // Represent that the door can open by show "Unlock" in LCD
        M5.Lcd.setTextSize(2);
        Time2 = millis() ;                  // Collect time from timer as variable
        currentTime = Time2 - Time1 ;       // Collect current time
        while (currentTime <= 5000){        // time condition to change password
          M5.update();
          Time2 = millis() ;
          currentTime = Time2 - Time1 ;
          if(M5.BtnC.wasReleased())         // Press and release Button C to change password 
          {
            myState = ChangePass ;          // Change state
            M5.Lcd.clear(BLACK);
            break ;
          }    
        }
        if (myState == Unlock){            // Button C not press and release in 5 second       
          M5.Lcd.clear(BLACK);
          myState = Lock ;                 // Change state
        }
        break ;

      case ChangePass :                   // State when need to change password 
        passdNew = "" ;                   // Blank new password
        typing = "" ;                     // Blank typing password
        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(90, 20);
        M5.Lcd.println("New Passcode");
        AccessScreen();
        M5.Lcd.setCursor(140, 55);
        while (passdNew.length() != 4)  // Waiting for new password which contains 4 digit
          {
            M5.update();
            if (M5.BtnB.wasReleased()){myState = Lock; passdNew = "pppp"; break;}   // Press and release button B for cancel password change
            char key = keypad.getKey(); // Read value from keypad 
            M5.Lcd.print(key); 
            passdNew += String(key); // Collect variable from keypad to password
          }
        while (myState == ChangePass){
          M5.update();
          if (M5.BtnC.wasReleased()){break;}                                    // Press and release button C to enter new password
          if (M5.BtnB.wasReleased()){myState = Lock; passdNew = "pppp"; break;} // Press and release button B for cancel password change
        }
        M5.Lcd.clear(BLACK);
        while (myState == ChangePass){                // Confirm your password
          M5.Lcd.setTextSize(2);
          M5.Lcd.setCursor(80, 20);
          M5.Lcd.println("Confirm Passcode");
          AccessScreen();
          M5.Lcd.setCursor(140, 55);
          while(typing.length() != 4)  
            {
              M5.update();
              if (M5.BtnB.wasReleased()){myState = Lock; typing = "pppp"; break;} // Press and release button B for cancel password change
              char key = keypad.getKey();
              M5.Lcd.print(key); 
              typing += String(key);
            }
          while (myState == ChangePass){
            M5.update();
            if (M5.BtnB.wasReleased()){myState = Lock; break;} // Press and release button B for cancel password change
            if (M5.BtnC.wasReleased()){                        // Complete your password change
              if(typing == passdNew){                          // Confirm correct new password 
                myState = Lock;                                // Change state
                passd = passdNew;                              // set new password as current password to use
                M5.Lcd.clear(BLACK);
                break;
              }
              else {                                          // Confirm incorrect password, you must enter password again until correct or cancel password change
                M5.Lcd.setTextColor(BLACK);
                M5.Lcd.setCursor(140, 55);
                M5.Lcd.print(typing);
                M5.Lcd.setTextColor(WHITE);
                M5.Lcd.setCursor(140, 55);
                typing = "";
                while(typing.length() != 4)  
                 {
                 M5.update();
                 if (M5.BtnB.wasReleased()){myState = Lock; typing = "pppp"; } // Press and release button B for cancel password change
                 char key = keypad.getKey();
                 M5.Lcd.print(key); 
                 typing += String(key);
                 }
              }
            }
          }
        }
        break;
      
      default:
        break;
    }
  }
}
void BlynkTask(void* pvParameters){
  while (1){
    Blynk.run();        //run blynk service
    timer.run();        //run timer service
  }
}
void setup()
{ 
  M5.begin(); //
  Wire.begin();   
  Blynk.begin(auth, ssid, pass);            //star blynk and connect to wifi
  timer.setInterval(1000L, myTimerEvent);   //set timer to run function every 1 sec
  timer.setInterval(1000L, sendSensor);     //set timer to run function every 1 sec
  timer.setInterval(1000L, Light);          //set timer to run function every 1 sec
  timer.setInterval(1000L, Fan);            //set timer to run function every 1 sec
  LINE.setToken(LINE_TOKEN);                //set line token to connect to line api
  //LINE.notify(String("M5 start "));
  pinMode(25,OUTPUT); // Define pin 25 as output data
  pinMode(26,OUTPUT); // Define pin 26 as output data
  pinMode(23,OUTPUT); // Define pin 23 as output data
  
  myState = Lock;
  
  xTaskCreate(BlynkTask, "Blynk", 16384, NULL, 1, NULL); // Define task 1 about Blynk
  xTaskCreate(Door, "Door", 16384, NULL, 2, NULL); // Define task 2 about door system
  
  
}

void loop()
{
  
}
