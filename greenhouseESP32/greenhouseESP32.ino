
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
#define NUM_RELAYS  4

void writeString(char add,String data);
String read_String(char add);

// Assign each GPIO to a relay
int relayGPIOs[NUM_RELAYS] = {2, 26, 27, 25};
int relaySTATE[NUM_RELAYS] = {0,0,0,0};
long relayTimers[NUM_RELAYS] ={0,0,0,0};
String schedule="";


// Replace with your network credentials
char ssid[32] ="";
char password[32] ="";
const char* host = "esp32";
const char *ssidAP = "ESP32AP";
const char *passwordAP = "";

const char* PARAM_INPUT_1 = "relay";
const char* PARAM_INPUT_2 = "state";
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

TaskHandle_t Task1;

// Create Json document
DynamicJsonDocument doc(4096);

// -----------------------setup function ----------------------

void setup(){
  
  // Serial port for debugging purposes
  Serial.begin(115200);
  
 // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
//read ssid and pass from spiffs
String copy;
copy=readFile("/ssid.txt");
copy.toCharArray(ssid, 32);
copy=readFile("/pass.txt");
copy.toCharArray(password, 32);
//Read schedule from data.txt
schedule=readFile("/data.txt");

  // Set all relays to off when the program starts - if set to Normally Open (NO), the relay is off when you set the relay to HIGH
  for(int i=0; i<NUM_RELAYS; i++){
    pinMode(relayGPIOs[i], OUTPUT);
    if(RELAY_NO){
      digitalWrite(relayGPIOs[i], LOW);
      relaySTATE[i] = LOW;
    }
    else{
      digitalWrite(relayGPIOs[i], HIGH);
      relaySTATE[i] = HIGH;
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

//-------------------------------Response to main page------------------------
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){

    String inputMessage;
    String inputMessage2;
    
    if (logedIn == false && request->hasParam("username") && request->hasParam("password")) {
      inputMessage = request->getParam("username")->value();
      inputMessage2 = request->getParam("password")->value();
      if(inputMessage2==PASSHASH && inputMessage==ROOTUSER){ 
      logedIn=true;
        }
      }
    
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
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/favicon.ico","image/x-icon");
});

//---------------------------------Response to files read in spiffs----------------------
// Serve files in directory "/" when request url starts with "/file"
server.serveStatic("/file", SPIFFS, "/");

//---------------------------------Response to files delete in spiffs----------------------
 server.on("/file", HTTP_DELETE, [](AsyncWebServerRequest *request){
  String url =request->url();
  url=url.substring(5);
    //Serial.println(url);
    char path[url.length()+1];
    url.toCharArray(path, url.length()+1);
    int code = deleteFile(SPIFFS, path);
    if(code == 204){
      request->send(204, "text/plain", "DELETED");}
    else{
   request->send(410, "text/plain", "File not found / Forbidden");
    }
 });

 //---------------------------------Response to files create in spiffs----------------------

 server.on("/file", HTTP_POST, [](AsyncWebServerRequest *request){
        request->send(200);
      }, handleUpload);
           
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
      else if(request->hasParam("filelist") && request->getParam("filelist")->value() =="true"){
        listDirectorie(SPIFFS, "/", 0);
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
      writeFile("/data.txt",inputMessage);      
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
       writeFile("/ssid.txt",inputMessage);
      inputMessage2 = request->getParam("password")->value();
      writeFile("/pass.txt",inputMessage2);
      delay(100);
       request->send(200, "text/plain", "OK");
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
 
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    // open the file on first call and store the file handle in the request object
    request->_tempFile = SPIFFS.open("/" + filename, "w");
  }

  if (len) {
    // stream the incoming chunk to the opened file
    request->_tempFile.write(data, len);
  }

  if (final) {
    // close the file handle as the upload is now done
    request->_tempFile.close();
    request->redirect("/");
  }
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


void writeFile(String fileDir, String input){
  File file = SPIFFS.open(fileDir, "w");
        if(!file){
          // File not found
          Serial.println("Failed to open test file");
          return;
            } else {
            file.print(input);
            file.close();
            }
  }
  
String readFile(String fileDir){
  String output;
  File file = SPIFFS.open(fileDir, "r");
        if(!file){
          // File not found
          Serial.println("Failed to open data file");          
          return "";
            } else {
                byte fbyte;
                char fchar;
              while(file.available()){
                fbyte= file.read();
                if(fbyte != -1){
                  fchar = char(fbyte);
                output += fchar;
                Serial.print(fchar);
                  }
                }
            file.close();
            int l = output.length();
            //output.remove(l-1);
            return output;
            }
  }

void listDirectorie(fs::FS &fs, const char * dirname, uint8_t levels){
    JsonArray fileListName = doc.createNestedArray("fileName");
    JsonArray fileListSize = doc.createNestedArray("fileSize");
    File root = fs.open(dirname);
    if(!root){
        //Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        //Serial.println(" - not a directory");
        return;
    }
    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            if(levels){
                listDirectorie(fs, file.name(), levels -1);
            }
        } else {
            fileListName.add(String(file.name()));
            fileListSize.add(String(file.size()));  
        }
        file = root.openNextFile();
    }
}

int deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
        return 204;
    } else {
        Serial.println("- delete failed");
        return 410;
    }
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
   long lastMillis=0;
  for(;;){
  long actualMillis = millis();
  
    
  if(schedule !="" && lastMillis< actualMillis){
  lastMillis=actualMillis+1000;
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
    outputUpdate();
    
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
