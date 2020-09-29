
// Import required libraries
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "AsyncJson.h"
#include "ArduinoJson.h"
#include "time.h"



// Set to true to define Relay as Normally Open (NO)
#define RELAY_NO    true

// Set number of relays
#define NUM_RELAYS  5

// Assign each GPIO to a relay
int relayGPIOs[NUM_RELAYS] = {2, 26, 27, 25, 33};
int relaySTATE[NUM_RELAYS] = {false,false,false,false,false,};

// Replace with your network credentials
const char* ssid = "beirario";
const char* password = "beirario";

const char* PARAM_INPUT_1 = "relay";  
const char* PARAM_INPUT_2 = "state";
const char* PARAM_INPUT_3 = "username";
const char* PARAM_INPUT_4 = "password";
const char* PASSHASH = "4a7f6f76bbe95876c2fde2d8693ed8c3aca8d5a85a0df6e7cb0fd7b2751be915";
const char* ROOTUSER ="romeurolo";
bool logedIn = false;
 long timermillis = 0;
 const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create Json document
StaticJsonDocument<500> doc;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
    <!-- Required meta tags -->
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no">

    <!-- Bootstrap CSS -->
    <link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css"
        integrity="sha384-JcKb8q3iqJ61gNV9KGb8thSsNjpSL0n8PARn9HuZOnIxN0hoP+VmmDGMN5t9UJ0Z" crossorigin="anonymous">

    <script src="https://code.jquery.com/jquery-3.3.1.min.js"></script>

  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  %BUTTONPLACEHOLDER%
<script>

        function toggleCheckbox(element) {
            var xhr = new XMLHttpRequest();
            if (element.checked) { xhr.open("GET", "/update?relay=" + element.id + "&state=1", true); }
            else { xhr.open("GET", "/update?relay=" + element.id + "&state=0", true); }
            xhr.send();
        }

        function checkSwitch() {


            $.ajax({
                url: "http://192.168.1.99/json",
                type: 'GET',
                dataType: 'json',
                success: function (results) { processResults(null, results) },
                error: function (request, statusText, httpError) { processResults(httpError || statusText), null }
            });
            function processResults(error, data) {
                if (error) {
                    console.log(error)
                }
                {
                    if (data) {
                        if(data.digital.length == 5){
                          document.getElementById("1").checked = data.digital[0];
                          document.getElementById("2").checked = data.digital[1];
                          document.getElementById("3").checked = data.digital[2];
                          document.getElementById("4").checked = data.digital[3];
                          document.getElementById("5").checked = data.digital[4];
                        }
                    }
                }
            }
        }
        setInterval(checkSwitch, 250)
    </script>
</body>
</html>
)rawliteral";


const char loginIndex[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
    <!-- Required meta tags -->
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no">

    <!-- Bootstrap CSS -->
    <link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css"
        integrity="sha384-JcKb8q3iqJ61gNV9KGb8thSsNjpSL0n8PARn9HuZOnIxN0hoP+VmmDGMN5t9UJ0Z" crossorigin="anonymous">

    <script src="https://cdnjs.cloudflare.com/ajax/libs/crypto-js/3.1.9-1/crypto-js.min.js"></script>

    <title>ESTUFAS</title>
</head>

<body>

    <div class="card mx-auto" style="width: 18rem;">
        <div class="card-body">
            <h5 class="card-title">ESTUFAS JORGE ROLO</h5>
            <form name="loginForm">
                <div class="form-group">
                    <label for="exampleInputEmail1">Username</label>
                    <input type="text" class="form-control" id="Username" name="userid">
                </div>
                <div class="form-group">
                    <label for="exampleInputPassword1">Password</label>
                    <input type="password" class="form-control" id="password" name="pwd">
                </div>
                <button type="button" onclick="check(this.form)" class="btn btn-success btn-lg">Login</button>
            </form>

        </div>
    </div>

    </div>
    <script>
        function check(form) {
            var hash = CryptoJS.SHA256(form.pwd.value);
            var xhr = new XMLHttpRequest();
            xhr.open("GET", "/update?username=" + form.userid.value + "&password=" + hash, true);
            xhr.send();
            setTimeout(() => { window.location.reload(); }, 500);
        }
    </script>
</body>

</html>
)rawliteral";




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

// Replaces placeholder with button section in your web page
  String processor(const String& var){
    //Serial.println(var);
    if(var == "BUTTONPLACEHOLDER"){
      String buttons ="";
      for(int i=1; i<=NUM_RELAYS; i++){
        String relayStateValue = relayState(i);
        buttons+= "<h4>Relay #" + String(i) + " - GPIO " + relayGPIOs[i-1] + "</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"" + String(i) + "\" "+ relayStateValue +"><span class=\"slider\"></span></label>";
      }
      return buttons;
    }
    return String();
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
       request->send_P(200, "text/html", index_html, processor);
      }
      else{
         request->send_P(200, "text/html", loginIndex);
        }       
  });

  server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request){

   JsonArray digitalValues = doc.createNestedArray("digital");
   String jsonOutput;
  for (int pin = 0; pin < NUM_RELAYS; pin++) { 
    // Read the digital input
    int value = relaySTATE[pin];

    // Add the value at the end of the array
    digitalValues.add(value);
    }
    /*Serial.println("json GET");
    Serial.print(relaySTATE[0]);
    Serial.print(relaySTATE[1]);
    Serial.print(relaySTATE[2]);
    Serial.print(relaySTATE[3]);
    Serial.println(relaySTATE[4]);
    */
    
  serializeJsonPretty(doc, jsonOutput);
  
// Length (with one extra character for the null terminator)
 int str_len = jsonOutput.length() + 1; 
    //Serial.println("string lenght:"+str_len);
 
// Prepare the character array (the buffer) 
 char char_array[str_len];
  jsonOutput.toCharArray(char_array, str_len);
   request->send(200, "application/json", char_array);
   doc.clear();
   configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
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
    Serial.println("ligar motor");
    break;
    }
    else{
    Serial.println("desligarmotor");
    }
  }

}
