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
        + "<th scope='col' style='text-align:center;'>Hora</th>"
        + "<th scope='col' style='text-align:center;'>Estufa 1 </th>"
        + "<th scope='col' style='text-align:center;'>Estufa 2</th>"
        + "<th scope='col' style='text-align:center;'>Estufa 3</th>"
        + "<th scope='col' style='text-align:center;'>Estufa 4</th>"
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
                    + "<th class='table-dark' scope='row' style='text-align:center;'>" + h + "H " + m + "m</th>"
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
            + "id='" + i + "'>"
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
        + "<div class='card-body' style='background-color:Red;' id='card" + number + "Body'> "
        + "<h2 class='card-title text-center'>ESTUFA " + number + "</h2>"
        + "<p class='card-text text-center' id='countdown" + number + "'>REGA PARADA</p>"
        + "</div>"
        + "<ul class='list-group list-group-flush' style='background-color:DodgerBlue;'>"
        + "<li class='list-group-item' style='background-color:rgb(0,186,255);'>"
        + "<div class='form-group' style='background-color:rgb(0,186,255);'>"
        + "<label for='exampleFormControlSelect" + number + "'>Tempo de rega 'MINUTOS'</label>"
        + "<select class='form-control' id='timerValue" + number + "' name='valueTimerValue'>"
        + "<option>5</option><option>10</option><option>15</option>"
        + "<option>20</option><option>25</option><option>30</option>"
        + "</select>"
        + "</div>"
        + "</li>"
        + "</ul>"
        + "<div class='card-body' style='background-color:rgb(0, 186, 255);'>"
        + "<button type='button' onclick='startTimer(this)' class='btn btn-lg btn-success'"
        + " id='timerStart" + number + "'>Começar Rega</button > "
        + "<button type='button' onclick='stopTimer(this)' class='btn btn-lg btn-danger' "
        + "id='timerStop" + number + "'>Parar Rega</button>"
        + "</div>"
        + "</div>"
        + "</div>";

    return card;
}

setInterval(checkSwitch, 750);
setInterval(timer, 1000);

var countDownDate=[0, 0, 0, 0];
var distance=[0, 0, 0, 0];
var minutes=[0, 0, 0, 0];
var seconds=[0, 0, 0, 0];
var estufa1=[];
var estufa2=[];
var estufa3=[];
var estufa4=[];
var greenHouseTimming = [estufa1, estufa2, estufa3, estufa4];