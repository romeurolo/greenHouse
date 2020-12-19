// Import required libraries
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "time.h"
#include "SPIFFS.h"
#include "Update.h"
#include <EEPROM.h>
#include <ESPmDNS.h>

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

struct scheduleDays {
    String SUNDAY;
    String MONDAY;
    String TUESDAY;
    String WEDNESDAY;
    String THURSDAY;
    String FRIDAY;
    String SATURDAY;
};
 scheduleDays sd;

String softwareVersion ="v 1.0";

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
const char* PASSHASH = "e7d3e769f3f593dadcb8634cc5b09fc90dd3a61c4a06a79cb0923662fe6fae6b";
const char* ROOTUSER = "Administrator";
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
updateScheduleStruct();

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
    WiFi.setHostname("MyArduino");
  WiFi.begin(ssid, password);
  int atempt = 0;
 
  while (WiFi.status() != WL_CONNECTED && atempt <20) { 
    delay(2000);
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
  if(!MDNS.begin("esp32")) {
     Serial.println("Error starting mDNS");
     return;
  }
    }

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());

if(WiFi.status() == WL_CONNECTED){
  wlConnection = true;
   configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}
//-------------- create task in second core to loop and update schedule---------------------
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
      
      if (request->hasParam("ssid") && request->hasParam("password")) {
      
      inputMessage = request->getParam("ssid")->value();
       writeFile("/ssid.txt",inputMessage);
      inputMessage2 = request->getParam("password")->value();
      writeFile("/pass.txt",inputMessage2);
      delay(100);
       request->send(200, "text/plain", "OK");
      ESP.restart();
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

    //-------------------------------Response to Favicon------------------------
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
  
//-------------------------------Response to JSON GET----------------------------------
server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("data"))
    {
        String param;
        param = request->getParam("data")->value();
        if (param == "wifilist")
        {
            listNetworks();
        }
        else if (param == "filelist")
        {
            listDirectorie(SPIFFS, "/", 0);
        }
        else if (param == "timers")
        {
            createTimersJson();
        }
        else if (param == "schedule")
        {
            createScheduleJson();
        }
        else
        {
            createJson();
        }
    }
    else
    {
        createJson();
    }

    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJson(doc, *response);
    request->send(response);
    doc.clear();
});

//---------------------------Response to JSON POST ------------------------------------

AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/rest", [](AsyncWebServerRequest *request, JsonVariant &json) {
  doc = json.as<JsonObject>();
  bool hasTimer = doc.containsKey("manualTimers");
  bool hasSchedule = doc.containsKey("scheduleValues");
  bool hasDigital = doc.containsKey("digital");
  
  if(hasTimer){
    JsonArray timerValues = doc["manualTimers"];
    for(int x=0; x<4; x++){
      relayTimers[x] = millis()+int(timerValues[x]);
      } 
    }
    
  if(hasSchedule){
    String s =doc["scheduleValues"];
    schedule =s;
    writeFile("/data.txt",s); 
    updateScheduleStruct();
    }
    
  if(hasDigital){
    JsonArray digitalValues = doc["digital"];
    for(int x=0; x<4; x++){
    if(RELAY_NO){
        relaySTATE[x] = int(digitalValues[x]);
      }
      else{
        relaySTATE[x] = !(int(digitalValues[x]));
      }
    } 
    outputUpdate("");   
  }
   
  doc.clear();
  request->send(200, "text/plain", "OK");
});

// listener to json Post
server.addHandler(handler);

  // Start server
  server.begin();
   
}
//-------------------------------Function to update outputs-----------------------------------
  void outputUpdate(String zones){
    if(zones =="" || zones.length() != 4){
      //------ ----alterar função para recever string com dados
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
    else
      {
      for(int x=0; x < NUM_RELAYS; x++){
        bool out = false;
        if(zones.substring(x,x+1) == "1"){
          out = true;
         }
        if(!RELAY_NO){
          out =!out;
         }  
        digitalWrite(relayGPIOs[x],out);
        }      
      } 
  }
  
//--------------------------File creating function--------------------------------------------------------------------- 
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


//--------------------------Function to create Json with digital array and software version--------
void createJson()
{
   JsonArray digitalValues = doc.createNestedArray("digital");
   JsonArray software = doc.createNestedArray("software");
   
    digitalValues.add(relaySTATE[0]);
    digitalValues.add(relaySTATE[1]);
    digitalValues.add(relaySTATE[2]);
    digitalValues.add(relaySTATE[3]);
    software.add(softwareVersion);
}
//--------------------------Function to create Json with manual timers------------------
void createTimersJson()
{
   JsonArray timerValues = doc.createNestedArray("manualTimers");
    long actMillis=millis();
    timerValues.add(relayTimers[0]-actMillis);
    timerValues.add(relayTimers[1]-actMillis);
    timerValues.add(relayTimers[2]-actMillis);
    timerValues.add(relayTimers[3]-actMillis);
}
//--------------------------Function to create Json with schedule values------------------
void createScheduleJson()
{
    JsonArray scheduleValues = doc.createNestedArray("scheduleValues");
    scheduleValues.add(String(schedule));
}

//--------------------------Function to create Json file list in SPIFFS------------------
void listDirectorie(fs::FS &fs, const char * dirname, uint8_t levels){
    JsonArray fileListName = doc.createNestedArray("fileName");
    JsonArray fileListSize = doc.createNestedArray("fileSize");
    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
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

//--------------------------Function to create  Json Networks list received in ESP32------------------
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

//--------------------------Function to write Files from SPIFFS------------------
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
  
//--------------------------Function to read Files from SPIFFS------------------  
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
                  }
                }
            file.close();
            return output;
            }
  }
  
//--------------------------Function to delete Files from SPIFFS------------------  
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

//--------------------------Function to update schedule Struct from Schedule String------------------  
void updateScheduleStruct(){
    sd.SUNDAY="";
    sd.MONDAY="";
    sd.TUESDAY="";
    sd.WEDNESDAY="";
    sd.THURSDAY="";
    sd.FRIDAY="";
    sd.SATURDAY="";

  for(int x =0; x<schedule.length()/26; x++){
    if(schedule.substring((x*26)+19,(x*26)+20) != "0"){
       sd.MONDAY+=schedule.substring((x*26),(x*26)+18);
      }
    if(schedule.substring((x*26)+20,(x*26)+21) != "0"){  
   sd.TUESDAY+=schedule.substring((x*26),(x*26)+18);
      }
    if(schedule.substring((x*26)+21,(x*26)+22) != "0"){  
   sd.WEDNESDAY+=schedule.substring((x*26),(x*26)+18);
      }
    if(schedule.substring((x*26)+22,(x*26)+23) != "0"){  
   sd.THURSDAY+=schedule.substring((x*26),(x*26)+18);
      }
    if(schedule.substring((x*26)+23,(x*26)+24) != "0"){  
   sd.FRIDAY+=schedule.substring((x*26),(x*26)+18);
      }
    if(schedule.substring((x*26)+24,(x*26)+25) != "0"){
   sd.SATURDAY+=schedule.substring((x*26),(x*26)+18);
      }
    if(schedule.substring((x*26)+25,(x*26)+26) != "0"){
   sd.SUNDAY+=schedule.substring((x*26),(x*26)+18);
      }
    }
  }
  
//--------------------------Function that will run in second Core always to check the schedue and manual timers, and update the outputs--------------
void Task1code(void *pvParameters)
{
    long lastMillis = 0;
    for (;;)
    {
        int scheduleStates[NUM_RELAYS] = {0, 0, 0, 0};
        long actualMillis = millis();
        String out;
        if (lastMillis < actualMillis)
        {
            lastMillis = actualMillis + 1000;
            String todaySchedule = "";
            struct tm timeinfo;
            getLocalTime(&timeinfo);
            long hour = timeinfo.tm_hour;
            long minutes = timeinfo.tm_min;
            int day = timeinfo.tm_wday;

            //check the day and copy values from schedule struct
            switch (day)
            {
            case 0:
                todaySchedule = sd.SUNDAY;
                break;
            case 1:
                todaySchedule = sd.MONDAY;
                break;
            case 2:
                todaySchedule = sd.TUESDAY;
                break;
            case 3:
                todaySchedule = sd.WEDNESDAY;
                break;
            case 4:
                todaySchedule = sd.THURSDAY;
                break;
            case 5:
                todaySchedule = sd.FRIDAY;
                break;
            case 6:
                todaySchedule = sd.SATURDAY;
                break;
            default:
                break;
            }

            //loop to check the manual timers
            for (int t = 0; t < NUM_RELAYS; t++)
            {
                if (actualMillis > relayTimers[t])
                {
                    relayTimers[t] = 0;
                }
                else
                {
                    scheduleStates[t] = 1;
                }
            }

            if (todaySchedule != "")
            {
                //----------------------------------------------------------------------------------------------------------------------01234567890123456789012345
                //  alterar este loop para aceitar novo schedule value tamanho de cada segmento 18 com E, zonbas e data inicial e final E:1234:00:00:00:15:1234500
                for (int s = 0; s < todaySchedule.length() / 18; s++)
                {
                    int ih = (todaySchedule.substring((s * 18) + 7, (s * 18) + 9)).toInt();
                    int im = (todaySchedule.substring((s * 18) + 10, (s * 18) + 12)).toInt();
                    int fh = (todaySchedule.substring((s * 18) + 13, (s * 18) + 15)).toInt();
                    int fm = (todaySchedule.substring((s * 18) + 16, (s * 18) + 18)).toInt();
                    String zones = (todaySchedule.substring((s * 18) + 2, (s * 19) + 6));
                    if (ih >= hour && fh >= hour)
                    {
                        if (im <= minutes && (fm > minutes || fh > ih))
                        {
                            for (int z = 0; z < 4; z++)
                            {
                                if (zones[z] != 0)
                                {
                                    scheduleStates[z] = 1;
                                }
                            }
                        }
                    }
                }
            }
            for (int y = 0; y < 4; y++)
            {
                out += scheduleStates[y];
            }
            outputUpdate(out);
        }
    }
}

//------------------loop function---------------
void loop() {
}
