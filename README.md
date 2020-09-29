# greenHouse
<h1>Greenhouse irrigation controller with web control</h1>
<p></p>
The goal of this project is to create a irrigation controler using a ESP32 microcontroler.
the ESP32 had wifi conection so the code running in the ESP32 is a simple Assync webserver, so is capable of multiple requests and also have a REST API to send and receive jsons.
The JSON file that ESP32 serve is simple only contain information about the state of the relays and the timers associated whit every output.
In this moment the is no code to receive Jsons, just for now.
<p></p>
In terms of security, when you try to connect to the esp32 for the first time a LOGIN page apears and the login operation relies on SHA256 crypto. the hass pass is stored in the ESP32 memory. After the success login the ESP respond the HTML page that have all control possibilities.

<h2>Login page</h2>
![login menu](img/login%20menu.png)
<p></p>
<h2>Schedule irrigation</h2>
![login menu](img/login%20menu.png)
<p></p>
<h2></h2>


