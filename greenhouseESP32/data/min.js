function toggleCheckbox(element) {
    var xhr = new XMLHttpRequest();
    if (element.checked) { xhr.open("GET", "/update?relay=" + element.id + "&state=1", true); }
    else { xhr.open("GET", "/update?relay=" + element.id + "&state=0", true); }
    xhr.send();
}
function jsonRequest(cb1, cb2) {
    $.ajax({
        url: "/json",
        timeout: 2000,
        type: 'GET',
        dataType: 'json',
        success: function (results) {
            cb1(null, results, cb2);
            $("#connectionStatus").empty();
            $("#connectionStatus").append("<a class='nav-link disabled'>Conected <span class='dotGreen'></span></a>");
        },
        error: function (request, statusText, httpError) {
            cb1(httpError || statusText, cb2), null;
            $("#connectionStatus").empty();
            $("#connectionStatus").append("<a class='nav-link disabled'>Disconected <span class='dotRed'></span></a>");
        }
    });

}

function processResults(error, data, cb) {
    if (error) {
        if ($("#" + e).length == 1) {
            $("#" + e).attr("checked", false);
        }
    }


    if (data) {
        for (var e = 1; e <= 4; e++) {
            if ($("#" + e).length == 1) {
                $("#" + e).attr("checked", Boolean(data.digital[e - 1]));
            }
        }
    }
}

function processCardsTimer(error, data, cb) {
    if (error) {
    }

    if (data) {
        for (var x = 0; x < 5; x++) {
            var milis = (data.manualTimers[x]) / 60000;
            countDownDate[x] = addMinutes(new Date(), milis);
        }
    }
}

function menuChange(element) {
    $("#main").empty();
    if (String(element.id).slice(4) == "timerTable") {
        $("#main").append(timerTable());
        jsonRequest(updateShedule);
    }
    if (String(element.id).slice(4) == "card1") {
        $("#main").append(cards());
        jsonRequest(processCardsTimer);
    }

    if (String(element.id).slice(4) == "manualButtons") {
        $("#main").append(manualButtons());
    }
    if (String(element.id).slice(4) == "currentState") {
        //$("#main").append(manualButtons());
    }
    if (String(element.id).slice(4) == "Update") {
        $("#main").append(createUpdate());
    }
}



function stopTimer(element) {
    var cardId = element.id.slice(9);
    countDownDate[cardId - 1] = new Date().getTime();
    $("#timerStart" + cardId).attr("disabled", false);
    $("#timerValue" + cardId).attr("disabled", false);
    $("#card" + cardId + "Body").attr("style", "background-color:Red;");
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/update?timer=" + cardId + "&date=0", true);
    xhr.send();

}
function startTimer(element) {
    var cardId = element.id.slice(10);
    var tmin = $("#timerValue" + cardId).val();
    countDownDate[cardId - 1] = addMinutes(new Date(), tmin);

    miliSecs[cardId - 1] = tmin * 60 * 1000;


    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/update?timer=" + cardId + "&date=" + miliSecs[cardId - 1], true);
    xhr.send();

}
function addMinutes(date, minutes) {
    return new Date(date.getTime() + minutes * 60000);

}

// Update the count in the cards
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
    var scheduleList = "";

    //Add the id to the list if is selected, if the value has been added, the set dont repeat the entry' 
    $("td").each(function (index) {
        if ($(this).text() == "REGAR") {
            //console.log(String($(this).attr("id")).split("E")[1]);
            scheduleList = scheduleList + String($(this).attr("id"));
        }
    });
    var xhr = new XMLHttpRequest();
    xhr.open("GET", "/update?timming=" + scheduleList, true);
    xhr.send();


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

        for (var h = 00; h < 24; h++) {
            for (var m = 00; m < 50; m = m + 15) {
                rowHTML = rowHTML + ("<tr>"
                    + "<th class='table-dark' scope='row' style='text-align:center;'>" + (('0' + h)).slice(-2) + "H " + (('0' + m)).slice(-2) + "m</th>"
                    + "<td style='text-align:center;' onclick='toggleSchedule(this)' id='E:1:" + (('0' + h)).slice(-2) + ":" + (('0' + m)).slice(-2) + "'></td>"
                    + "<td style='text-align:center;' onclick='toggleSchedule(this)' id='E:2:" + (('0' + h)).slice(-2) + ":" + (('0' + m)).slice(-2) + "'></td>"
                    + "<td style='text-align:center;' onclick='toggleSchedule(this)' id='E:3:" + (('0' + h)).slice(-2) + ":" + (('0' + m)).slice(-2) + "'></td>"
                    + "<td style='text-align:center;' onclick='toggleSchedule(this)' id='E:4:" + (('0' + h)).slice(-2) + ":" + (('0' + m)).slice(-2) + "'></td>"
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

function updateShedule(error, data, cb) {
    if (error) {
    }

    if (data && data.scheduleValues[0]) {
        var schedule = data.scheduleValues[0].split("E");
        for (var e = 1; e < schedule.length; e++) {
            var element = document.getElementById("E" + schedule[e]);
            element.classList.toggle("bg-success");
            element.innerHTML = "REGAR";
        }
    }
}

function manualButtons() {
    var buttons = "<div class='col-sm-12 col-lg-12 text-center' id='manualButtons'>";
    for (var i = 1; i <= 4; i++) {
        buttons += "<h4>ESTUFA " + i + "</h4>"
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

function createUpdate() {
    var updateCard = "<div class='row justify-content-center' id='updateCard'>" +
        "<div class='col-sm-12 col-lg-12 text-center'>" +
        "<div class='card m-3' style='width: 25rem;'>" +
        "<div class='card-body'>" +
        "<h5 class='card-title text-center'>Update software</h5>" +
        "<p class='card-text' id = 'prg' > progress: 0 %</p > " +
        "<div class='progress' > " +
        "<div class='progress-bar progress-bar-striped bg-success' role = 'progressbar'" +
        "style = 'width: 0%' aria - valuenow='25' aria - valuemin='0' aria - valuemax='100' id = 'prgBar' ></div > " +
        "</div > " +
        "</div > " +
        "<form method = 'POST' action = '#' enctype = 'multipart/form-data' id = 'upload_form' > " +
        "<ul class='list-group list-group-flush' > " +
        "<li class='list-group-item' > " +
        "<input class='form-control-file ' type = 'file' name = 'update' > " +
        "</li > " +
        "<li class='list-group-item' > " +
        "<button type='button' onclick='OTA()' class='btn btn-primary'>Update</button>" +
        "</li > " +
        "</ul > " +
        "</form > " +
        "</div > " +
        "</div > " +
        "</div >";

    return updateCard;
}

function OTA() {
    var form = $('#upload_form')[0];
    var data = new FormData(form);
    $.ajax({
        url: '/OTA',
        type: 'POST',
        data: data,
        contentType: false,
        processData: false,
        xhr: function () {
            var xhr = new window.XMLHttpRequest();
            xhr.upload.addEventListener('progress', function (evt) {
                if (evt.lengthComputable) {
                    var per = evt.loaded / evt.total;
                    $('#prg').html('progress: ' + Math.round(per * 100) + '%');
                    $('#prgBar').attr('style', "width: " + Math.round(per * 100) + "%");
                }
            }, false);
            return xhr;
        },
        success: function (d, s) {
            // console.log('success!');

            setInterval(function () {
                $('#prg').html('Rebooting: ' + rebootTime + '%');
                $('#prgBar').attr('style', "width: " + rebootTime + "%");
                rebootTime++;
                if (rebootTime > 100) {
                    clearTimeout(this);
                    location.replace("/")
                }
            }, 100)
        },
        error: function (a, b, c) {
            //console.log('Error!');
        }
    });
}

setInterval(function () { if ($("#1").length == 1) { jsonRequest(processResults); } }, 550);
setInterval(timer, 1000);
document.getElementById("connectionStatus").addEventListener("load", jsonRequest(processResults));


var countDownDate = [0, 0, 0, 0];
var distance = [0, 0, 0, 0];
var minutes = [0, 0, 0, 0];
var seconds = [0, 0, 0, 0];
var miliSecs = [0, 0, 0, 0];
var estufa1 = [];
var estufa2 = [];
var estufa3 = [];
var estufa4 = [];
var greenHouseTimming = [estufa1, estufa2, estufa3, estufa4];
var rebootTime = 0;
