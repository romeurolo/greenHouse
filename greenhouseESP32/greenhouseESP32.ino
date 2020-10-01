
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
int relaySTATE[NUM_RELAYS] = {false, false, false, false, false,};

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

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
    <!-- Required meta tags -->
    <meta charset="utf-8">
    <meta name="description" content="" />
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css"
        integrity="sha384-JcKb8q3iqJ61gNV9KGb8thSsNjpSL0n8PARn9HuZOnIxN0hoP+VmmDGMN5t9UJ0Z" crossorigin="anonymous">

    <script src="https://code.jquery.com/jquery-3.3.1.min.js"></script>
    <script src="https://192.168.1.65/js/min.js"></script>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        .switch {
            position: relative;
            display: inline-block;
            width: 120px;
            height: 68px
        }

        .switch input {
            display: none
        }

        .slider {
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #ccc;
            border-radius: 34px
        }

        .slider:before {
            position: absolute;
            content: "";
            height: 52px;
            width: 52px;
            left: 8px;
            bottom: 8px;
            background-color: #fff;
            -webkit-transition: .4s;
            transition: .4s;
            border-radius: 68px
        }

        input:checked+.slider {
            background-color: #2196F3
        }

        input:checked+.slider:before {
            -webkit-transform: translateX(52px);
            -ms-transform: translateX(52px);
            transform: translateX(52px)
        }

        .dotRed {
            height: 25px;
            width: 25px;
            background-color: #f8041d;
            ;
            border-radius: 50%;
            display: inline-block;
        }

        .dotGreen {
            height: 25px;
            width: 25px;
            background-color: #23bd47;
            ;
            border-radius: 50%;
            display: inline-block;
        }

        tbody {
            display: block;
            height: 500px;
            overflow-y: scroll;
        }

        tr {
            display: block;
        }

        th,
        td {
            width: 400px;
        }
    </style>
</head>

<body>
    <div class="container-fluid">
        <div class="row">
            <div class="col-sm-6 col-lg-6">
                <h2>ESTUFA</h2>
            </div>
            <div class="col-sm-6 col-lg-6" id="connectionStatus">
            </div>
        </div>
        <p></p>
        <div class="row">
            <div class="col-xs-6 col-lg-3 text-center">
                <button type="button" onclick="menuChange(this)" class="btn btn-lg btn-primary"
                    id="menumanualButtons">Modo
                    Manual</button>
            </div>
            <div class="col-xs-6 col-lg-3 text-center">
                <button type="button" onclick="menuChange(this)" class="btn btn-lg btn-primary" id="menucard1">Manual
                    temporizado</button>
            </div>
            <div class="col-xs-6 col-lg-3 text-center">
                <button type="button" onclick="menuChange(this)" class="btn btn-lg btn-primary"
                    id="menutimerTable">Programar
                    Relógio</button>
            </div>
            <div class="col-xs-6 col-lg-3 text-center">
                <button type="button" onclick="menuChange(this)" class="btn btn-lg btn-primary"
                    id="menucurrentState">Estado
                    Actual</button>
            </div>
        </div>

        <p></p>
        <p></p>
        <hr style="height:1px;background-color:black">
        <div class="row" id="main">
        </div>
    </div>
  
  
</body>
</html>
)rawliteral";


const char javaScript[] PROGMEM = R"rawliteral(
function toggleCheckbox(element) {
            var xhr = new XMLHttpRequest();
            if (element.checked) { xhr.open("GET", "/update?relay=" + element.id + "&state=1", true); }
            else { xhr.open("GET", "/update?relay=" + element.id + "&state=0", true); }
            xhr.send();
        }
        function checkSwitch() {
            $.ajax({
                url: "http://192.168.1.81/json",
                type: 'GET',
                timeout: 1000,
                dataType: 'json',
                success: function (results) { processResults(null, results) },
                error: function (request, statusText, httpError) { processResults(httpError || statusText), null }
            });
            function processResults(error, data) {
                if (error) {
                    $("#connectionStatus").empty();
                    $("#connectionStatus").append("<h2>ESP - Disconected  <span class='dotRed'></span> </h2>");
                    for (var e = 1; e <= 4; e++) {
                        if ($("#" + e).length == 1) {
                            $("#" + e).attr("checked", false);
                        }
                    }
                }

                if (data) {
                    $("#connectionStatus").empty();
                    $("#connectionStatus").append("<h2>ESP - Connected  <span class='dotGreen'></span> </h2>");


                    for (var e = 1; e <= 4; e++) {
                        if ($("#" + e).length == 1) {
                            $("#" + e).attr("checked", Boolean(data.digital[e - 1]));

                        }
                    }
                }
            }
        }
        function menuChange(element) {
            $("#main").empty();
            if (String(element.id).slice(4) == "timerTable") {
                $("#main").append(timerTable());
            }
            if (String(element.id).slice(4) == "card1") {
                $("#main").append(cards());
            }
            if (String(element.id).slice(4) == "manualButtons") {
                $("#main").append(manualButtons());
            }
            if (String(element.id).slice(4) == "currentState") {
                //$("#main").append(manualButtons());
            }
        }



        function stopTimer(element) {
            var cardId = element.id.slice(9);
            countDownDate[cardId - 1] = new Date().getTime();
            $("#timerStart" + cardId).attr("disabled", false);
            $("#timerValue" + cardId).attr("disabled", false);
            $("#card" + cardId + "Body").attr("style", "background-color:Red;");

        }
        function startTimer(element) {
            var cardId = element.id.slice(10);
            countDownDate[cardId - 1] = addMinutes(new Date(), $("#timerValue" + cardId).val());
        }
        function addMinutes(date, minutes) {
            return new Date(date.getTime() + minutes * 60000);
        }
        setInterval(checkSwitch, 750)
        setInterval(timer, 1000)
        // Update the count down every 1 second
        function timer() {
            if ($("#cards").length) {
                // Get today's date and time
                var now = new Date().getTime();
                for (var t = 1; t <= 4; t++) {
                    // Find the distance between now and the count down date
                    distance[t - 1] = countDownDate[t - 1] - now;
                    // Time calculations minutes and seconds
                    minutes[t - 1] = Math.floor((distance[t - 1] % (1000 * 60 * 60)) / (1000 * 60));
                    seconds[t - 1] = Math.floor((distance[t - 1] % (1000 * 60)) / 1000);
                    // Output the result in an element with id="countdown"
                    document.getElementById("countdown" + t).innerHTML = minutes[t - 1] + "m " + seconds[t - 1] + "s ";
                    // If the count down is over, write some text 
                    $("#card" + t + "Body").attr("style", "background-color:MediumSeaGreen;");
                    if (distance[t - 1] < 0) {
                        $("#timerStart" + t).attr("disabled", false);
                        $("#timerValue" + t).attr("disabled", false);
                        $("#card" + t + "Body").attr("style", "background-color:Red;");
                        document.getElementById("countdown" + t).innerHTML = "REGA PARADA";
                    }
                    else {
                        $("#card" + t + "Body").attr("style", "background-color:MediumSeaGreen;");
                        $("#timerStart" + t).attr("disabled", true);
                        $("#timerValue" + t).attr("disabled", true);
                    }
                }
            }
        }

        function saveSchedule() {

        }

        function timerTable() {
            var tableHTML =
                "<div class='col-sm-12 col-lg-12 mx-my-5' id='timerTable'>"
                + "<table class='table table-bordered'>"
                + "<thead class='thead-dark'>"
                + "<tr>"
                + "<th scope='col' style='text-align:center'>Hora</th>"
                + "<th scope='col' style='text-align:center'>Estufa 1 </th>"
                + "<th scope='col' style='text-align:center'>Estufa 2</th>"
                + "<th scope='col' style='text-align:center'>Estufa 3</th>"
                + "<th scope='col' style='text-align:center'>Estufa 4</th>"
                + "</tr>"
                + "</thead>"
                + "<tbody>"
                + createRows()
                + "</tbody >"
                + "</table >"
                + "</div > "
                + "<button type='button' onclick='saveSchedule()'"
                + "class='btn btn-lg btn-primary mx-auto' id='buttonSaveSchedule'>"
                + " Salvar temporização</button>";

            function createRows() {
                var rowHTML = "";

                for (var h = 0; h < 24; h++) {
                    for (var m = 0; m < 50; m = m + 15) {
                        rowHTML = rowHTML + ("<tr>"
                            + "<th class='table-dark' scope='row' style='text-align:center'>" + h + "H " + m + "m</th>"
                            + "<td style='text-align:center;' onclick='toggleSchedule(this)' id='estufa:1:" + h + ":" + m + "'></td>"
                            + "<td style='text-align:center;' onclick='toggleSchedule(this)' id='estufa:2:" + h + ":" + m + "'></td>"
                            + "<td style='text-align:center;' onclick='toggleSchedule(this)' id='estufa:3:" + h + ":" + m + "'></td>"
                            + "<td style='text-align:center;' onclick='toggleSchedule(this)' id='estufa:4:" + h + ":" + m + "'></td>"
                            + "</tr>");
                    }
                }
                return rowHTML;
            }
            return tableHTML;
        }

        function toggleSchedule(form) {
            form.classList.toggle("bg-success");
            if (form.innerHTML === "REGAR") {
                form.innerHTML = "";
            } else {
                form.innerHTML = "REGAR";
            }
        }

        function manualButtons() {
            var buttons = "<div class='col-sm-12 col-lg-12 text-center' id='manualButtons'>";
            for (var i = 1; i <= 4; i++) {
                buttons += "<h4>Relay #" + i + " - GPIO " + "</h4>"
                    + "<label class='switch'><input type='checkbox' onchange='toggleCheckbox(this)'"
                    + "id='" + i + "''>"
                    + "<span class='slider'></span></label>";
            }
            buttons += "</div>";
            return buttons;
        }

        function cards() {
            var cards = "<div class='row' id='cards'>";
            for (var xcard = 1; xcard <= 4; xcard++) {
                cards += createCard(xcard);
            }
            cards += "</div>";
            return cards;
        }

        function createCard(number) {

            var card = "<div class='col-sm-12 col-lg-6'>"
                + "<div class='card m-3' id='card" + number + "'> "
                + "<div class='card-body' style='background-color: Red;' id='card" + number + "Body'> "
                + "<h2 class='card-title text-center'>ESTUFA " + number + "</h2>"
                + "<p class='card-text text-center' id='countdown" + number + "'>REGA PARADA</p>"
                + "</div>"
                + "<ul class='list-group list-group - flush' style='background-color: DodgerBlue; '>"
                + "<li class='list-group-item' style='background-color: rgb(0, 186, 255); '>"
                + "<div class='form-group' style='background-color: rgb(0, 186, 255); '>"
                + "<label for='exampleFormControlSelect" + number + "'>Tempo de rega 'MINUTOS'</label>"
                + "<select class='form-control' id='timerValue" + number + "' name='valueTimerValue'>"
                + "<option>5</option><option>10</option><option>15</option>"
                + "<option>20</option><option>25</option><option>30</option>"
                + "</select>"
                + "</div>"
                + "</li>"
                + "</ul>"
                + "<div class='card-body' style='background-color: rgb(0, 186, 255); '>"
                + "<button type='button' onclick='startTimer(this)' class='btn btn-lg btn-success'"
                + " id='timerStart" + number + "'>Começar Rega</button > "
                + "<button type='button' onclick='stopTimer(this)' class='btn btn-lg btn-danger' "
                + "id='timerStop" + number + "'>Parar Rega</button>"
                + "</div>"
                + "</div>"
                + "</div>";

            return card;
        }

        var countDownDate=[0, 0, 0, 0];
        var distance=[0, 0, 0, 0];
        var minutes=[0, 0, 0, 0];
        var seconds=[0, 0, 0, 0];
        var estufa1=[];
        var estufa2=[];
        var estufa3=[];
        var estufa4=[];
        var greenHouseTimming = [estufa1, estufa2, estufa3, estufa4];
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
      String buttons =javaScript;

      
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
      request->send_P(200, "text/html", index_html);
      }
      else{
         request->send_P(200, "text/html", loginIndex);
        }       
  });
  server.on("/js", HTTP_GET, [](AsyncWebServerRequest *request){
   
      request->send_P(200, "text/javascript", javaScript);
      
  });

  server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request){
long duration = millis();
   JsonArray digitalValues = doc.createNestedArray("digital");
   String jsonOutput;
  for (int pin = 0; pin < NUM_RELAYS; pin++) { 
    // Read the digital input
    int value = relaySTATE[pin];
    // Add the value at the end of the array
    digitalValues.add(value);
    }
    
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
