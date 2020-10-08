
// Import required libraries
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "time.h"
#include "SPIFFS.h"
#include "Update.h"

// Set to true to define Relay as Normally Open (NO)
#define RELAY_NO    true

// Set number of relays
#define NUM_RELAYS  5

// Assign each GPIO to a relay
int relayGPIOs[NUM_RELAYS] = {2, 26, 27, 25, 33};
int relaySTATE[NUM_RELAYS] = {false, false, false, false, false,};
long relayTimers[NUM_RELAYS] ={0,0,0,0,0};
String schedule="";


// Replace with your network credentials
const char* ssid = "Cervejaria_Imperial";
const char* password = "";

const char* PARAM_INPUT_1 = "relay";
const char* PARAM_INPUT_2 = "state";
const char* PARAM_INPUT_3 = "username";
const char* PARAM_INPUT_4 = "password";
const char* PASSHASH = "4a7f6f76bbe95876c2fde2d8693ed8c3aca8d5a85a0df6e7cb0fd7b2751be915";
const char* ROOTUSER = "romeurolo";
bool logedIn = false;
long timermillis = 0;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create Json document
StaticJsonDocument<500> doc;

// -----------------------separation html side ----------------------

String relayState(int numRelay){
  if(RELAY_NO){
    if(digitalRead(relayGPIOs[numRelay-1])){
      return "";
    }
    else {
      return "checked";
    }
  }
  else {
    if(digitalRead(relayGPIOs[numRelay-1])){
      return "checked";
    }
    else {
      return "";
    }
  }
  return "";
}

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

    // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Set all relays to off when the program starts - if set to Normally Open (NO), the relay is off when you set the relay to HIGH
  for(int i=1; i<=NUM_RELAYS; i++){
    pinMode(relayGPIOs[i-1], OUTPUT);
    if(RELAY_NO){
      digitalWrite(relayGPIOs[i-1], LOW);
      relaySTATE[i-1] = LOW;
    }
    else{
      digitalWrite(relayGPIOs[i-1], HIGH);
      relaySTATE[i-1] = HIGH;
    }
  }
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
   
    if(logedIn){
       request->send(SPIFFS, "/control.html","text/html");
      }
      else{
         request->send(SPIFFS, "/indexlogin.html","text/html");
        }       
  });
  
  server.on("/min.js", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/min.js","text/javascript");
});

 server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/favicon.ico","image/x-icon");
});

 server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
  request->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    //ESP.restart();
    //Check if parameter exists (Compatibility)
int args = request->args();
for(int i=0;i<args;i++){
  Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i));
}

if(request->hasArg("download")){
  String arg = request->arg("download");
Serial.println(arg);
 }
  });
  



  server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request){
long duration = millis();
   JsonArray digitalValues = doc.createNestedArray("digital");
   JsonArray timerValues = doc.createNestedArray("manualTimers");
   JsonArray scheduleValues = doc.createNestedArray("scheduleValues");
   String jsonOutput;
  for (int pin = 0; pin < NUM_RELAYS; pin++) { 
    // Read the digital input
    int value = relaySTATE[pin];
    int timerValue = relayTimers[pin];
    // Add the value at the end of the array
    digitalValues.add(value);
    timerValues.add(timerValues);
    }
    scheduleValues.add(schedule);
  serializeJson(doc, jsonOutput);
  
// Length (with one extra character for the null terminator)
 int str_len = jsonOutput.length() + 1; 
// Prepare the character array (the buffer) 
 char char_array[str_len];
  jsonOutput.toCharArray(char_array, str_len);
   request->send(200, "application/json", char_array);
   doc.clear();
   //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
   Serial.println("json send time"+millis()-duration);
  //printLocalTime();
  });


  // Send a GET request to <ESP_IP>/update?relay=<inputMessage>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    String inputMessage2;
    String inputParam2;
   
    // GET input1 value on <ESP_IP>/update?relay=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1) & request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      inputParam2 = PARAM_INPUT_2;
      if(RELAY_NO){
        Serial.print("NO ");
        digitalWrite(relayGPIOs[inputMessage.toInt()-1], inputMessage2.toInt());
        relaySTATE[inputMessage.toInt()-1] = inputMessage2.toInt();
      }
      else{
        Serial.print("NC ");
        digitalWrite(relayGPIOs[inputMessage.toInt()-1], !inputMessage2.toInt());
        relaySTATE[inputMessage.toInt()-1] = !inputMessage2.toInt();
      }
    }
    else if (request->hasParam(PARAM_INPUT_3) & request->hasParam(PARAM_INPUT_4)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputParam = PARAM_INPUT_1;
      inputMessage2 = request->getParam(PARAM_INPUT_4)->value();
      inputParam2 = PARAM_INPUT_2;
      if(inputMessage2==PASSHASH){ 
      logedIn=true;
      timermillis = millis();
      }
      
      }
    
    else {
      inputMessage = "No message sent";
      inputParam = "none";
      }
    
    
    Serial.println("relay:"+inputMessage + " state:"+inputMessage2);
    request->send(200, "text/plain", "OK");
   
  });
  // Start server
  server.begin();
   
}
  
void loop() {
  if(logedIn){
    if(millis()-timermillis >= 10000){
      //logedIn=false;
      }
  //Serial.println(millis());
  }
for(int e=0; e < NUM_RELAYS; e++){
  if(relaySTATE[e]==true){
    //Serial.println("ligar motor");
    break;
    }
    else{
    //Serial.println("desligarmotor");
    }
  }

}
