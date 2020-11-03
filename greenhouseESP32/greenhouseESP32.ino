
// Import required libraries
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "time.h"
#include "SPIFFS.h"
#include "Update.h"
#include <EEPROM.h>



// Set to true to define Relay as Normally Open (NO)
#define RELAY_NO    true

// Set number of relays
#define NUM_RELAYS  5

void writeString(char add,String data);
String read_String(char add);


// Assign each GPIO to a relay
int relayGPIOs[NUM_RELAYS] = {2, 26, 27, 25, 33};
int relaySTATE[NUM_RELAYS] = {0,0,0,0,0};
long relayTimers[NUM_RELAYS] ={0,0,0,0,0};
String schedule="";


// Replace with your network credentials
char ssid[32] ="";
char password[32] ="";
const char* host = "esp32";
const char *ssidAP = "ESP32AP";
const char *passwordAP = "";

const char* PARAM_INPUT_1 = "relay";
const char* PARAM_INPUT_2 = "state";
const char* PARAM_INPUT_3 = "username";
const char* PARAM_INPUT_4 = "password";
const char* PARAM_INPUT_5 = "timming";
const char* PARAM_INPUT_6 = "timer";
const char* PARAM_INPUT_7 = "date";


const char* PASSHASH = "4a7f6f76bbe95876c2fde2d8693ed8c3aca8d5a85a0df6e7cb0fd7b2751be915";
const char* ROOTUSER = "romeurolo";
bool logedIn = false;
long timermillis = 0;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;
bool shouldReboot = false;
bool wlConnection = false;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// create sever to connect if no network
//WiFiServer serverAP(80);

TaskHandle_t Task1;

// Create Json document
DynamicJsonDocument doc(4096);

// -----------------------separation html side ----------------------

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  EEPROM.begin(512);
//-------------------------Read ssid and wifi password from EEPROM----------------
String copy;
copy = read_String(10);
copy.toCharArray(ssid, 42);
//copy = read_String(100);
//copy.toCharArray(password, 110);
 
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
  int atempt = 0;
 
  while (WiFi.status() != WL_CONNECTED && atempt <10) { 
    delay(1000);
    atempt++;
    Serial.println("Connecting to WiFi..");
  }  
    
  if(WiFi.status() != WL_CONNECTED){
    WiFi.disconnect();
    Serial.println("Fail to connect to WiFi, Start AP mode");
   WiFi.softAP(ssidAP, passwordAP);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  Serial.println("Server started");
    }


  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

if(WiFi.status() == WL_CONNECTED){
  wlConnection = true;
   configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 0 */                  
  delay(500); 



  

//-------------------------------Response to main page------------------------
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    
    if(logedIn && wlConnection){
       request->send(SPIFFS, "/control.html","text/html");
      }
      else if(!logedIn && wlConnection){
         request->send(SPIFFS, "/indexlogin.html","text/html");
        }       
        else{
          request->send(SPIFFS, "/indexAP.html","text/html");
          }
  });
  
  //-------------------------------Response to javascript------------------------
  server.on("/min.js", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/min.js","text/javascript");
});

//-------------------------------Response to OTA Update------------------------
  
  server.on("/OTA", HTTP_POST, [](AsyncWebServerRequest *request){
    shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot?"OK":"FAIL");
    response->addHeader("Connection", "close");
    request->send(response);
  },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index){
      Serial.printf("Update Start: %s\n", filename.c_str());
      //Update.runAsync(true);
      if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
        Update.printError(Serial);
      }
    }
    if(!Update.hasError()){
      if(Update.write(data, len) != len){
        Update.printError(Serial);
      }
    }
    if(final){
      if(Update.end(true)){
        Serial.printf("Update Success: %uB\n", index+len);
        Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
      } else {
        Update.printError(Serial);
      }
    }
  });
  

//-------------------------------Response to JSON ----------------------------------

  server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request){
if (request->hasParam("wifilist") && request->getParam("wifilist")->value() =="true") {
        listNetworks();
      }
      else{
        createJson();
      }
      
   String jsonOutput;
   int docSize=doc.memoryUsage();
   char char_array[docSize];
   serializeJson(doc,char_array, docSize);
   request->send(200, "application/json", char_array);
   doc.clear();
      
  });

//-------------------------------Response to LOGIN ----------------------------------
server.on("/login", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputMessage2;
    if (request->hasParam(PARAM_INPUT_3) && request->hasParam(PARAM_INPUT_4)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_4)->value();
      if(inputMessage2==PASSHASH && inputMessage==ROOTUSER){ 
      logedIn=true;
        }
      }
       request->send(200, "text/plain", "OK");
  });

//-------------------------------Response to data Update GET------------------------

  // Send a GET request to <ESP_IP>/update?relay=<inputMessage>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    String inputMessage2;
    String inputParam2;
   
    // GET input1 value on <ESP_IP>/update?relay=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      inputParam2 = PARAM_INPUT_2;
      if(RELAY_NO){
        relaySTATE[inputMessage.toInt()-1] = inputMessage2.toInt();
      }
      else{
        relaySTATE[inputMessage.toInt()-1] = !inputMessage2.toInt();
      }
      outputUpdate();    
    }
    
   else if (request->hasParam(PARAM_INPUT_5)) {
      inputMessage = request->getParam(PARAM_INPUT_5)->value();
      inputParam = PARAM_INPUT_1;
      schedule=inputMessage;
      inputMessage2 = inputMessage.length(); 
     
    
      }
      
      else if (request->hasParam(PARAM_INPUT_6) && request->hasParam(PARAM_INPUT_7)) {
      inputMessage = request->getParam(PARAM_INPUT_6)->value();
      inputParam = PARAM_INPUT_1;
      inputMessage2 = request->getParam(PARAM_INPUT_7)->value();
      inputParam2 = PARAM_INPUT_2;
      relayTimers[inputMessage.toInt()-1] = millis()+inputMessage2.toInt();
      }

      else if (request->hasParam("ssid") && request->hasParam("password")) {
      inputMessage = request->getParam("ssid")->value();
       writeString(10, inputMessage);
      inputMessage2 = request->getParam("password")->value();
      writeString(110, inputMessage2);
      delay(100);
      ESP.restart();
      }
    
    else {
      inputMessage = "No message sent";
      inputParam = "none";
      }
    
    Serial.println("relay/param1:"+inputMessage + " state/param2:"+inputMessage2);
    request->send(200, "text/plain", "OK");
   
  });
 
  // Start server
  server.begin();
   
}
//-------------------------------Internal Functions-----------------------------------
  void outputUpdate(){
    for(int in=0; in < NUM_RELAYS; in++){
    if(RELAY_NO){
      digitalWrite(relayGPIOs[in],relaySTATE[in]);
      }
      else
      {
      digitalWrite(relayGPIOs[in],!relaySTATE[in]);
      }
    } 
  }

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.println(timeinfo.tm_hour);
}

void createJson()
{
   JsonArray digitalValues = doc.createNestedArray("digital");
   JsonArray timerValues = doc.createNestedArray("manualTimers");
   JsonArray scheduleValues = doc.createNestedArray("scheduleValues");
  
    // Add the value at the end of the array
    digitalValues.add(relaySTATE[0]);
    digitalValues.add(relaySTATE[1]);
    digitalValues.add(relaySTATE[2]);
    digitalValues.add(relaySTATE[3]);
    long actMillis=millis();
    timerValues.add(relayTimers[0]-actMillis);
    timerValues.add(relayTimers[1]-actMillis);
    timerValues.add(relayTimers[2]-actMillis);
    timerValues.add(relayTimers[3]-actMillis);
    scheduleValues.add(schedule);
}

void writeString(char add,String data)
{
  int _size = data.length();
  int i;
  for(i=0;i<_size;i++)
  {
    EEPROM.write(add+i,data[i]);
  }
  EEPROM.write(add+_size,'\0');   //Add termination null character for String Data
  EEPROM.commit();
}
 
 
String read_String(char add)
{
  int i;
  char data[100]; //Max 100 Bytes
  int len=0;
  unsigned char k;
  k=EEPROM.read(add);
  while(k != '\0' && len<512)   //Read until null character
  {    
    k=EEPROM.read(add+len);
    data[len]=k;
    len++;
  }
  data[len]='\0';
  return String(data);
}

void listNetworks() {
  JsonArray wifiList = doc.createNestedArray("wifiList");
  int numSsid = WiFi.scanNetworks();
  if (numSsid == -1)
  { 
   wifiList.add("Couldn't get a wifi connection list");
  }
  else{   
    
  for (int thisNet = 0; thisNet<numSsid; thisNet++) {
    String wifiName = WiFi.SSID(thisNet);
    wifiList.add(wifiName); 
    }
  }
}

void Task1code( void * pvParameters ){
  for(;;){
    
  if(schedule !=""){
 
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  long hour = timeinfo.tm_hour;
  long minutes = timeinfo.tm_min;
/*
  Serial.println("time");
  Serial.println(hour);
  Serial.println(minutes);
*/
  for(int s =0; s < schedule.length()/9; s++){
    long segment;
    segment = (schedule.substring((s*9)+2,(s*9)+3)).toInt();
    relaySTATE[0]=0;
    relaySTATE[1]=0;
    relaySTATE[2]=0;
    relaySTATE[3]=0;
     segment = (schedule.substring((s*9)+4,(s*9)+6)).toInt();
   if(segment==hour){
    segment = (schedule.substring((s*9)+7,(s*9)+9)).toInt();
    if(segment <= minutes && (segment + 15) > minutes){ 
      segment = (schedule.substring((s*9)+2,(s*9)+3)).toInt();
      relaySTATE[segment]=1;
        }
      }
    }
    
  }
  else{
    relaySTATE[0]=0;
    relaySTATE[1]=0;
    relaySTATE[2]=0;
    relaySTATE[3]=0;
    outputUpdate();
    }
  } 
}

  
void loop() {
int actualMillis = millis();
for(int t=0; t < NUM_RELAYS; t++){
  if(actualMillis > relayTimers[t]){
    relayTimers[t]=0;
    relaySTATE[t]=0;
   }
   else{
    relaySTATE[t]=1;
    }
  }
outputUpdate();





  /*if(logedIn){
    if(millis()-timermillis >= 10000){
      //logedIn=false;
      }
  //Serial.println(millis());
  }
  */
   
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
