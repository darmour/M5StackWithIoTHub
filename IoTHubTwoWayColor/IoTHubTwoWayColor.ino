// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. 
#include <M5Stack.h>

#include <WiFi.h>
#include "AzureIotHub.h"
#include "Esp32MQTTClient.h"

#include "DHT12.h"
#include <Wire.h>      //The DHT12 uses I2C comunication.

DHT12 dht12;          //Preset scale CELSIUS and ID 0x5c.

#define INTERVAL 2000

#define MESSAGE_MAX_LEN 256
#define NOTE_DH2 661

// Please input the SSID and password of WiFi for each of the wifi networks the devices might connect to.  
int numSSID = 2;                        //The number of wifi networks in the array.  
const char* ssid[]     = {"",""};       //Place your wifi SSIDs in the quotes
const char* password[] = {"",""};       //Place the passwords for the SSIDs in the quotes


//////////////////////////////
//Define Colors
/////////////////////////////
#define C0 TFT_BLACK //Black
#define C1 TFT_RED //Red
#define C2 TFT_YELLOW  //Yellow
#define C3 TFT_BLUE  //Blue
#define C4 TFT_GREEN //Green
#define C5 TFT_MAGENTA  //Magenta
#define C6 TFT_WHITE  //White

//////////////////////////
//Define Fonts
/////////////////////////
#define FSS9 &FreeSans9pt7b
#define FSS12 &FreeSans12pt7b
#define FSS18 &FreeSans18pt7b
#define FSS24 &FreeSans24pt7b



static unsigned int ScreenColor565[] = {C0, C1, C2, C3, C4, C5, C6};
static const char *ScreenColorText[] = {"Black", "Red", "Yellow", "Blue", "Green" ,"Magenta", "White"};
int ScreenColorIndex = 0;
int ScreenColorCount = 7;
/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */

//M5Stack1
#define DEVICE_ID "M5Stack1"                //The device ID should match the value you used when creating the device's twin in Azure IoT Hub
static const char* connectionString = "";   //Place the connection string of the device inside the quotes.  Obtain this value from from the device's twin that you created in Azure IoT Hub 


//
//M5Stack2
//#define DEVICE_ID "M5Stack2"
//static const char* connectionString = "";   //Place the connection string of the device inside the quotes.  Obtain this value from from the device twin you created in Azure IoT Hub 


const char *messageData = "{\"deviceId\":\"%s\", \"messageId\":%d, \"messageType\":\"%s\", \"Temperature\":%f, \"Humidity\":%f}";
const char *messageColorData = "{\"deviceId\":\"%s\", \"messageId\":%d, \"messageType\":\"%s\",\"Color\":\"%s\",  \"ColorRGB\":%d}";

int messageCount = 1;
static bool hasWifi = false;
static bool messageSending = true;
static uint64_t send_interval_ms;

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Utilities
static void InitWifi()
{

  int connectRetries = 0;
  for (int i=0;((i>numSSID) || (hasWifi == false));i++)
  {
    

      //Serial.printf("Connecting to %s...\n", ssid);
      M5.Lcd.printf("Connecting to %s...\n", ssid[i]);
      WiFi.begin(ssid[i], password[i]);
      while ((WiFi.status() != WL_CONNECTED) && (connectRetries<40)) {
        delay(500);
        Serial.print(".");
        M5.Lcd.print(".");
        connectRetries++;
      }
      if (WiFi.status() == WL_CONNECTED)
      {
        hasWifi = true;
        Serial.println("WiFi connected");
        M5.Lcd.println("\nWiFi Connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        M5.Lcd.print(WiFi.localIP());
        M5.Lcd.print("\n");
      }
      else 
      {
        connectRetries = 0;
        M5.Lcd.print("\n");
      }
      
  } 
}






//////////////////////////////////////////////////////////////////////////////////////////////
//Clear the screen, draw the message and the menu
void ClearScreen(unsigned int color565, String screenMessage)
{
    M5.Lcd.fillScreen(color565);
    M5.Lcd.setCursor(0, 20);
    if ((color565==TFT_YELLOW)||(color565==TFT_WHITE)||(color565==TFT_GREEN))
    {
      M5.Lcd.setTextColor(TFT_BLACK);
    }
    else
    {
      M5.Lcd.setTextColor(TFT_WHITE);
    }
    
    M5.Lcd.println(screenMessage);
    
    //Print menu for buttons
    M5.Lcd.setCursor(5, 218);
    M5.Lcd.print("Send Temp  |");
    M5.Lcd.setCursor(117, 218);
    M5.Lcd.print("Select Color |");
    M5.Lcd.setCursor(230, 218);
    M5.Lcd.print("Send Color");
  
}

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
  {
    Serial.println("Send Confirmation Callback finished.");
    //M5.Lcd.print("Send Confirmation Callback finished.");
    //M5.Lcd.print("\n");
  }
}



///////////////////////////////////////////////////////
// This is the send message to device capability
//////////////////////////////////////////////////////
static void MessageCallback(const char* payLoad, int size)
{

  bool found = false;
  int foundAt = 0;
  Serial.println("Message callback:");
  Serial.println(payLoad);
  //M5.Lcd.print("Message callback:");
  //M5.Lcd.print("\n");

  for(int i=0; i < ScreenColorCount;i++)
  {
      if (strcmp(payLoad,ScreenColorText[i])==0)
      {
         M5.Lcd.println(ScreenColorText[i]);
         found = true;
         foundAt=i;
      }
     
  }
  if (found)
  {
      //const char *screenMsg = "Received request to change screen color\0";
      ClearScreen(ScreenColor565[foundAt],"Received request\n to change screen color");
        
   }
   else
   {
      char messagePayload[MESSAGE_MAX_LEN];
      snprintf(messagePayload,MESSAGE_MAX_LEN,"Message received: %s. Not a recognized color.", payLoad);
      ClearScreen(TFT_BLACK,messagePayload);
      
   }
 
}


//////////////////////////////////////////////////////////////////////////////////
//This will the Device Twin Message.  You could change the "Firmware Version" here.
//////////////////////////////////////////////////////////////////////////////////
static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size)
{
  char *temp = (char *)malloc(size + 1);
  if (temp == NULL)
  {
    return;
  }
  memcpy(temp, payLoad, size);
  temp[size] = '\0';
  // Display Twin message.
   
  free(temp);
}


/////////////////////////////////////////////////////////////////
///This gets called when you send a direct method to the device
/////////////////////////////////////////////////////////////////
static int  DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
{
  LogInfo("Try to invoke method %s", methodName);
  M5.Lcd.printf("Try to invoke method %s", methodName);
  //M5.Lcd.print(methodName);

  const char *responseMessage = "\"Successfully invoke device method\"";
  int result = 200;

  if (strcmp(methodName, "start") == 0)
  {
    LogInfo("Start sending temperature and humidity data");
    messageSending = true;
    M5.Lcd.print("\n\n starting...\n\n");
  }
  else if (strcmp(methodName, "stop") == 0)
  {
    LogInfo("Stop sending temperature and humidity data");
    messageSending = false;
    M5.Lcd.print("\n\n stopping...\n\n");
  }
  

  
  else if (strcmp(methodName, "clear") == 0)
  {
    LogInfo("clear");
   // M5.Speaker.setVolume(2);
    //M5.Speaker.setBeep(500, 10); 
    //M5.Speaker.beep();
    M5.Speaker.tone(NOTE_DH2, 200); //frequency 3000, with a duration of 200ms
    ClearScreen(TFT_BLACK, "Clear Screen Command Received");

  

  }
  else
  {
    LogInfo("No method %s found", methodName);
    char messagePayload[MESSAGE_MAX_LEN];
    snprintf(messagePayload,MESSAGE_MAX_LEN,"No method %s found", methodName);
    ClearScreen(TFT_BLACK,messagePayload);
    responseMessage = "\"No method found\"";
    result = 404;
    
  }

  *response_size = strlen(responseMessage) + 1;
  *response = (unsigned char *)strdup(responseMessage);
  M5.update();
  return result;
}

//////////////////////////////////////////////////////////////////////////////////
//Get the temp and send it to IoT Hub
//////////////////////////////////////////////////////////////////////////////////
static void SendTempToAzure()
{
      //make sure too many messages aren't sent w/i the send interval
       if (messageSending && (int)(millis() - send_interval_ms) >= INTERVAL)
        {
          char messagePayload[MESSAGE_MAX_LEN];
          char messagePayload2[MESSAGE_MAX_LEN];
          //Default temp and humidity to random values.  Then overwrite them with the actual values if the sensor is connnected
          //Record temp in Fahrenheit
          float temperature = (float)random(20,30) * 1.8+32;  

          //Record temp in Celsius
          //float temperature = (float)random(20,30);  
          
          float humidity = (float)random(0, 1000)/10;
          
          
         //Read data from the temp sensor.  If the sensor is not present, use the initialized random data.  Change FAHRENHEIT to CELSIUS if required.
         if (dht12.readTemperature(FAHRENHEIT)>1)
          {
            temperature = dht12.readTemperature(FAHRENHEIT);
            humidity = dht12.readHumidity();
          }

           // Send teperature data
          snprintf(messagePayload,MESSAGE_MAX_LEN, messageData, DEVICE_ID, messageCount++, "TempHumidity", temperature,humidity);
          Serial.println(messagePayload);
          EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(messagePayload, MESSAGE);
          Esp32MQTTClient_Event_AddProp(message, "temperatureAlert", "true");
          Esp32MQTTClient_SendEventInstance(message);
        
          send_interval_ms = millis();
          ClearScreen(ScreenColor565[ScreenColorIndex], messagePayload);
        }
}

//////////////////////////////////////////////////////////
//ManageButtonPresses
void CheckButtonPress()
{

    /////////////////////
    //Button A Pressed
    if(M5.BtnA.wasPressed()) 
    {
        SendTempToAzure();
        //ClearScreen(ScreenColor565[ScreenColorIndex], "temp sent....");
    }
    else
    {
      Esp32MQTTClient_Check();
    }

    //////////////////////
    //Button B Pressed
    if(M5.BtnB.wasPressed())
    {
    
      if(ScreenColorIndex>5)
      {
        ScreenColorIndex=0;
      }
      else
      {
        ScreenColorIndex++;
      }
      ClearScreen(ScreenColor565[ScreenColorIndex], ScreenColorText[ScreenColorIndex]);

    }
  
    ///////////////////////////
    //Button C Pressed
    if(M5.BtnC.wasPressed())
    {
      //M5.Lcd.printf("wasPressed B \r\n");
      char messagePayload[MESSAGE_MAX_LEN];
      snprintf(messagePayload,MESSAGE_MAX_LEN, messageColorData, DEVICE_ID, messageCount++, "Color", ScreenColorText[ScreenColorIndex],ScreenColor565[ScreenColorIndex]);

      EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(messagePayload, MESSAGE);
      //Esp32MQTTClient_Event_AddProp(message, "Color", ScreenColorText[ScreenColorIndex]);
      Esp32MQTTClient_SendEventInstance(message);
      ClearScreen(ScreenColor565[ScreenColorIndex], "color sent....");
   
    }
 
    delay(100);

}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Arduino sketch
void setup()
{
  M5.begin();
  Wire.begin();
 
  M5.Lcd.setFreeFont(FSS9);
  M5.Lcd.setTextWrap(true);
  M5.Lcd.setCursor(0, 20);
  Serial.begin(115200);
  Serial.println("ESP32 Device");
  Serial.println("Initializing...");
  M5.Lcd.print("Initializing...");
  // Initialize the WiFi module
  Serial.println(" > WiFi");
  hasWifi = false;
  InitWifi();
  if (!hasWifi)
  {
    return;
  }
  randomSeed(analogRead(0));

  Serial.println(" > IoT Hub");
  M5.Lcd.println(" > IoT Hub Connected!!");
  delay(3000);
  
  Esp32MQTTClient_SetOption(OPTION_MINI_SOLUTION_NAME, "GetStarted");
  Esp32MQTTClient_Init((const uint8_t*)connectionString, true);

  //////////////////////////////////////////
  //IoT Functions
  Esp32MQTTClient_SetSendConfirmationCallback(SendConfirmationCallback);
  Esp32MQTTClient_SetMessageCallback(MessageCallback);
  Esp32MQTTClient_SetDeviceTwinCallback(DeviceTwinCallback);
  Esp32MQTTClient_SetDeviceMethodCallback(DeviceMethodCallback);

  ///////////////////////////////////////////////////////////
  //Print menu for buttons
  ClearScreen(TFT_BLACK, "Select an action button from the menu");
  
  send_interval_ms = millis();
}



void loop()
{
  CheckButtonPress();
  M5.update();

}
