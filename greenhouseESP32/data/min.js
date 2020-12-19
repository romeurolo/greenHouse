function toggleCheckbox(element) {
    var data = { digital: [0, 0, 0, 0] }
    for (var e = 1; e <= 4; e++) {
        if ($("#" + e).length == 1) {
            if ($("#" + e + ":checked").length) {
                data.digital[e - 1] = 1;
            }
        }
    }
    jsonSend(data, "/rest");
}

function jsonRequest(cb1, jsonURL) {
    $.ajax({
        url: jsonURL,
        timeout: 2000,
        type: 'GET',
        dataType: 'json',
        success: function (results) {
            cb1(null, results);
            $("#connectionStatus").empty();
            $("#connectionStatus").append("<a class='nav-link disabled'>Conected <span class='dotGreen'></span></a>");

        },
        error: function (request, statusText, httpError) {
            cb1((httpError || statusText), null);
            $("#connectionStatus").empty();
            $("#connectionStatus").append("<a class='nav-link disabled'>Disconected <span class='dotRed'></span></a>");
        }
    });
}

function jsonSend(data, jsonURL) {
    $.ajax({
        url: jsonURL,
        type: 'POST',
        data: JSON.stringify(data),
        contentType: 'application/json; charset=utf-8',
        dataType: 'json',
        async: true,
        success: function (msg) {
            //alert(msg);
        },
        error: function (httpError) {
            //alert(httpError);
        }
    });
}

function processResults(error, data) {
    if (error) {
        for (var e = 1; e <= 4; e++) {
            if ($("#" + e).length == 1) {
                $("#" + e).attr("checked", false);
            }
        }
    }

    if (data) {
        software = data.software[0];
        for (var e = 1; e <= 4; e++) {
            if ($("#" + e).length == 1) {
                $("#" + e).attr("checked", Boolean(data.digital[e - 1]));
            }
        }
    }
}

function processFileList(error, data) {
    var fileListData = "0";
    if (error) {
        fileListData = "<tr>"
            + "<th  scope='row' style='text-align:center;'>No files</th>"
            + "<td style='text-align:center; id='file1'> 0</td>"
            + "<td style='text-align:center; id='file1'>-</td> "
            + "</tr>";
    }
    if (data) {
        fileListData = "";
        for (var x = 0; x < data.fileName.length; x++) {
            fileListData = fileListData + "<tr>"
                + "<th  scope='row' style='text-align:center;'>"
                + "<a href='/file"
                + data.fileName[x]
                + "'class='badge badge-success'>"
                + data.fileName[x]
                + "</a>"
                + "</th>"
                + "<td style='text-align:center; id='file1'>"
                + data.fileSize[x]
                + "</td>"
                + "<td style='text-align:center; id='file1'>"
                + "<button type='button' onclick='deleteFile(this)'"
                + "class='btn btn-lg btn-danger mx-auto' id='delete" + data.fileName[x] + "'>"
                + " Delete</button>"
                + "</td> "
                + "</tr>";
        }
    }
    $("#main").append(createSPIFFS(fileListData));
}

function processCardsTimer(error, data) {
    if (error) {
    }

    if (data) {
        for (var x = 0; x < 5; x++) {
            var milis = (data.manualTimers[x]) / 60000;
            countDownDate[x] = addMinutes(new Date(), milis);
        }
    }
}
function deleteFile(element) {
    var path;
    path = String(element.id).slice(6);
    var xhr = new XMLHttpRequest();
    xhr.open("DELETE", ("/file" + path), true);
    xhr.send();

    xhr.onreadystatechange = function () {
        if (xhr.readyState == 4 && xhr.status == 204) {
            window.alert(path + " deleted");
            $("#main").empty();
            jsonRequest(processFileList, "/json?data=filelist");
        }
    }
}

function menuChange(element) {
    $("#main").empty();
    if (String(element.id).slice(4) == "timerTable") {
        jsonRequest(updateSchedule, "/json?data=schedule");
    }
    if (String(element.id).slice(4) == "card1") {
        $("#main").append(cards());
        jsonRequest(processCardsTimer, "/json?data=timers");
    }
    if (String(element.id).slice(4) == "manualButtons") {
        $("#main").append(manualButtons());
    }
    if (String(element.id).slice(4) == "currentState") {
        $("#main").append(createCurrentState());
        $("#currentState").ready(jsonRequest(processState, "/json"));
    }
    if (String(element.id).slice(4) == "Update") {
        $("#main").append(createUpdate());
    }
    if (String(element.id).slice(4) == "SPIFFS") {
        jsonRequest(processFileList, "/json?data=filelist");
    }
}

function createCurrentState() {
    var state = "";
    state = "<div class='jumbotron' id='currentState'>"
        + "<h1 class='display - 4' style='text-align:center;'>Estufa current State</h1>"
        + "<p class='lead' id='state1'></p>"
        + "<p class='lead' id='state2'></p>"
        + "<p class='lead' id='state3'></p>"
        + "<p class='lead' id='state4'></p>"
        + "</div>";

    return state;
}

function processState(error, data) {
    if (error) {
        for (var e = 1; e <= 4; e++) {
            if ($("#state" + e).length == 1) {
                $("#state" + e).empty();
                $("#state" + e).append("<span class='dotRed'></span>  No connection");
            }
        }
    }

    if (data) {
        var timerState = [0, 0, 0, 0];
        var actualDate = new Date();
        var now = actualDate.getHours() + ":" + actualDate.getMinutes();
        if (scheduleData) {
            var schedule = scheduleData.split("E");
            for (var e = 0; e < schedule.length; e++) {
                var timming = schedule[e].split(":");
                if (timming.length == 7) {
                    var zones = timming[1];
                    var initialHour = timming[2];
                    var initialMinute = timming[3];
                    var finalHour = timming[4];
                    var finalMinute = timming[5];
                    var day = actualDate.getDay() == 0 ? 6 : (actualDate.getDay() - 1);


                    if (timming[6][day] != "0") {
                        if (initialHour <= actualDate.getHours() && finalHour >= actualDate.getHours()) {
                            if (initialMinute <= actualDate.getMinutes() && (finalMinute > actualDate.getMinutes() || finalHour > initialHour)) {
                                for (var z = 0; z < 4; z++) {
                                    if (zones[z] != 0) {
                                        timerState[z] = 1;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        for (var e = 1; e <= 4; e++) {
            var teste = "E:1234:00:00:00:15:1234500E:0034:12:15:13:15:0000067E:0204:22:15:22:45:1030507";
            var zoneState = "";
            zoneState = "<span class='"
                + (Boolean(data.digital[e - 1] || distance[e - 1] > 0 || timerState[e - 1] == "1") ? "dotGreen" : "dotRed")
                + "'></span>  Estufa " + e + " - "
                + (Boolean(data.digital[e - 1] || distance[e - 1] > 0 || timerState[e - 1] == "1") ? "ON" : "OFF")
                + (Boolean(data.digital[e - 1]) ? " - Manual" : "")
                + (timerState[e - 1] == "1" ? " - Relógio" : "")
                + (distance[e - 1] > 0 ? " - Temporizado" : "");

            if ($("#state" + e).length == 1) {
                $("#state" + e).empty();
                $("#state" + e).append(zoneState);
            }
        }
    }
}


function stopTimer(element) {
    var cardId = element.id.slice(9);
    countDownDate[cardId - 1] = new Date().getTime();
    $("#timerStart" + cardId).attr("disabled", false);
    $("#timerValue" + cardId).attr("disabled", false);
    $("#card" + cardId + "Body").attr("style", "background-color:Red;");
    var data = { manualTimers: [0, 0, 0, 0] };
    for (var x = 0; x < 4; x++) {
        var mili = ((minutes[x] * 60) + seconds[x]) * 1000;
        if (mili < 0) {
            mili = 0;
        }
        data.manualTimers[x] = mili;
    }
    data.manualTimers[cardId - 1] = 0;
    jsonSend(data, "/rest");
}
function startTimer(element) {
    var cardId = element.id.slice(10);
    var tmin = $("#timerValue" + cardId).val();
    countDownDate[cardId - 1] = addMinutes(new Date(), tmin);
    miliSecs[cardId - 1] = tmin * 60 * 1000;


    var data = { manualTimers: [0, 0, 0, 0] };
    for (var x = 0; x < 4; x++) {
        var mili = ((minutes[x] * 60) + seconds[x]) * 1000;
        if (mili < 0) {
            mili = 0;
        }
        data.manualTimers[x] = mili;
    }
    data.manualTimers[cardId - 1] = miliSecs[cardId - 1];
    jsonSend(data, "/rest");
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

function saveSchedule(data, id) {
    var scheduleList = "";
    if (id == "0") {
        scheduleList = scheduleData;
        if (data) {
            scheduleList += data;
        }
    }
    else {
        var schedule = scheduleData.split("E");
        for (var e = 1; e < schedule.length; e++) {
            if (e == id) {
                if (data) {
                    scheduleList += data;
                }
            }
            else {
                scheduleList += ("E" + schedule[e]);
            }
        }
    }
    scheduleData = scheduleList;
    var data = { scheduleValues: scheduleList };
    jsonSend(data, "/rest");

    setTimeout(function () {
        $("#main").empty();
        $("#main").append(timerTable());
    }, 500);
}

function timerTable() {
    var modalButtonHTLM = "";
    var modalHTML = "<div class='modal fade' id='Modal0' data-backdrop='static' data-keyboard='false' tabindex='-1' aria-labelledby='Modal0Label' aria-hidden='true'>"
        + "<div class='modal-dialog'>"
        + "<div class='modal-content'>"
        + "<div class='modal-header'>"
        + "<h5 class='modal-title' id='Modal0Label'>"
        + createTimeForm(0, 0)
        + "</h5>"
        + "<button id='ModalButton0' type='button' class='close'  onclick='closeModal(this)' aria-label='Close'>"
        + "<span aria-hidden='true'>&times;</span>"
        + "</button>"
        + "</div>"
        + "<div class='modal-body' id='ModalBody0'>"
        + "Zonas <br>"
        + createZonesForm(0, "0000")
        + "<br><br>Dias da semana"
        + createDaysCheckbox(0, "0000000")
        + "<div class='modal-footer'>"
        + "<button id='EditModal0' type='button' onclick='editModal(this)'class='btn btn-primary'>Save</button>"
        + "</div>"
        + "</div>"
        + "</div>"
        + "</div>"
        + "</div>";;

    if (scheduleData) {
        var schedule = scheduleData.split("E");
        for (var e = 0; e < schedule.length; e++) {
            var timming = schedule[e].split(":");
            if (timming.length == 7) {
                var zones = timming[1];
                var initialHour = timming[2];
                var initialMinute = timming[3];
                var finalHour = timming[4];
                var finalMinute = timming[5];

                var days;
                switch (timming[6]) {
                    case "1234500":
                        days = "Days of the week";
                        break;
                    case "0000067":
                        days = "Weekend";
                        break;
                    case "1234567":
                        days = "All days";
                        break;
                    default:
                        days = "";
                        for (var z = 0; z < 7; z++) {
                            if (timming[6][z] != 0) {
                                days += " " + weekday[z] + " ";
                            }
                        }
                        break;
                }
                modalButtonHTLM += "<tr id='Modal" + e + "Box'><th><button id='ModalButton" + e + "' type='button' class='btn btn-primary' onclick='openModal(this)' data-target='#Modal" + e + "'>"
                    + initialHour + ":" + initialMinute + " - " + finalHour + ":" + finalMinute + "<p><h6>" + zones + "</h6></p>"
                    + "<p><h6>" + days + "</h6></p>"
                    + "</button></th></tr>";

                modalHTML += "<div class='modal fade' id='Modal" + e + "' data-backdrop='static' data-keyboard='false' tabindex='-1' aria-labelledby='Modal" + e + "Label' aria-hidden='true'>"
                    + "<div class='modal-dialog'>"
                    + "<div class='modal-content'>"
                    + "<div class='modal-header'>"
                    + "<h5 class='modal-title' id='Modal" + e + "Label'>"
                    + createTimeForm(e, timming)
                    + "</h5>"
                    + "<button id='ModalButton" + e + "' type='button' class='close'  onclick='closeModal(this)' aria-label='Close'>"
                    + "<span aria-hidden='true'>&times;</span>"
                    + "</button>"
                    + "</div>"
                    + "<div class='modal-body' id='ModalBody" + e + "'>"
                    + "Zonas <br>"
                    + createZonesForm(e, zones)
                    + "<br><br>Dias da semana"
                    + createDaysCheckbox(e, timming[6])
                    + "<div class='modal-footer'>"
                    + "<button id='EditModal" + e + "' type='button' onclick='editModal(this)'class='btn btn-primary'>Edit</button>"
                    + "<button id='DeleteModal" + e + "' type='button' onclick='deleteModal(this)'class='btn btn-danger'>Delete</button>"
                    + "</div>"
                    + "</div>"
                    + "</div>"
                    + "</div>"
                    + "</div>";
            }
        }
    }

    var tableHTML =
        "<div class='col-sm-12 col-lg-12 mx-my-5' id='timerTable'>"
        + "<table class='table table-bordered'>"
        + "<thead>"
        + "</thead>"
        + "<tbody>"
        + modalButtonHTLM
        + "</tbody >"
        + "</table >"
        + "</div > "
        + modalHTML
        + "<button type='Button' class='btn btn-primary' onclick='createModal()'>Add Timer</button>";


    function createDaysCheckbox(id, check) {
        var daysCheckbox = "";
        for (var y = 0; y < 7; y++) {
            daysCheckbox += "<p> <div class='form-check'>"
                + "<input class='form-check-input' type='checkbox' value='1' "
                + "id='Modal" + id + "Day" + (y + 1) + "' disabled " + (check[y] != 0 ? "checked" : "") + " > "
                + "<label class='form-check-label' for='Modal" + id + "Day" + (y + 1) + "'>"
                + weekday[y]
                + "</label>"
                + "</div></p>";
        }

        return daysCheckbox;
    }
    function createZonesForm(id, zones) {

        var zonesform = "";
        zonesform = "<div class='form-check form-check-inline'>"
            + "<input class='form-check-input' type='checkbox' id='Modal" + id + "zone1' value='1' disabled " + (zones.slice(0, 1) != "0" ? "checked" : "") + ">"
            + "<label class='form-check-label' for='inlineCheckbox1'>1</label>"
            + "<input class='form-check-input' type='checkbox' id='Modal" + id + "zone2' value='2' disabled " + (zones.slice(1, 2) != "0" ? "checked" : "") + ">"
            + "<label class='form-check-label' for='inlineCheckbox1'>2</label>"
            + "<input class='form-check-input' type='checkbox' id='Modal" + id + "zone3' value='3' disabled " + (zones.slice(2, 3) != "0" ? "checked" : "") + ">"
            + "<label class='form-check-label' for='inlineCheckbox1'>3</label>"
            + "<input class='form-check-input' type='checkbox' id='Modal" + id + "zone4' value='4' disabled " + (zones.slice(3) != "0" ? "checked" : "") + ">"
            + "<label class='form-check-label' for='inlineCheckbox1'>4</label>"
            + "</div>";

        return zonesform;
    }

    function createTimeForm(id, data) {
        if (data) {
        }
        else {
            data = new Array(7);
            data[2] = "f";
            data[3] = "f";
            data[4] = "f";
            data[5] = "f";
        }
        function createOptions(sh, sm) {
            var options = "";
            for (var h = 0; h < 24; h++) {
                for (var m = 0; m < 60; m += 5) {
                    options += "<option value='" + (('0' + h)).slice(-2) + ":" + (('0' + m)).slice(-2) + "'"
                        + (h == sh && m == sm ? "selected" : "") + ">" +
                        (('0' + h)).slice(-2) + ":" + (('0' + m)).slice(-2) + "</option > ";
                }
            }
            return options;
        }
        var timeFormHTML = "<div class='form-group' '>"
            + "<div class='form-row'>"
            + "<div class='form-group col-md-6'>"
            + "<label for='exampleFormControlSelect" + id + "'>Start</label>"
            + "<select class='form-control' id='Modal" + id + "Start' name='valueTimerValue' disabled onchange='modalValueUpdate(this)'>"
            + createOptions(data[2], data[3])
            + "</select>"
            + "</div>"
            + "<div class='form-group col-md-6'>"
            + "<label for='exampleFormControlSelect" + id + "'>End</label>"
            + "<select class='form-control' id='Modal" + id + "End' name='valueTimerValue' disabled>"
            + createOptions(data[4], data[5])
            + "</select>"
            + "</div>"
            + "</div>"
            + "</div>";

        return timeFormHTML;
    }

    return tableHTML;
}

function openModal(element) {
    var id = element.id.slice(11);
    $("#Modal" + id).modal();
}

function closeModal(element) {
    var idModal = element.id.slice(11);
    $("#Modal" + idModal).modal('hide');
    setTimeout(function () {
        $("#main").empty();
        $("#main").append(timerTable());
    }, 500);
}

function editModal(element) {
    //edit or save modal
    var idModal = String(element.id).slice(4);
    if (element.innerHTML != "Save") {
        element.innerHTML = "Save";
        $("#" + idModal + "Start").attr("disabled", false);
        $("#" + idModal + "End").attr("disabled", false);
        $("#" + idModal + "zone1").attr("disabled", false);
        $("#" + idModal + "zone2").attr("disabled", false);
        $("#" + idModal + "zone3").attr("disabled", false);
        $("#" + idModal + "zone4").attr("disabled", false);
        for (var x = 1; x <= 7; x++) {
            $("#" + idModal + "Day" + x).attr("disabled", false);
        }
    }
    else {
        var data = "E:";
        var validation = 1;
        var start = $("#" + idModal + "Start").val();
        var end = $("#" + idModal + "End").val();
        for (var z = 1; z <= 4; z++) {
            if ($("#" + idModal + "zone" + z + ":checked").length) {
                data += z;
                validation = 0;
            }
            else {
                data += "0";
            }
        }
        if (validation) {
            window.alert("Please select the Zones");
            return;
        }
        validation = 1;
        if (start >= end) {
            window.alert("The start time is after end time");
            return;
        }
        data += ":" + start;
        data += ":" + end + ":";
        for (var x = 1; x <= 7; x++) {
            if ($("#" + idModal + "Day" + x + ":checked").length) {
                data += x;
                validation = 0;
            }
            else {
                data += "0";
            }
        }
        if (validation) {
            window.alert("Please select the days");
        }
        if (validation == 0) {
            $("#" + idModal).modal('hide');
            saveSchedule(data, idModal.slice(5))
        }
    }
}

function deleteModal(element) {
    var idModal = String(element.id).slice(6);

    if (confirm("Do you want to delete this timer?")) {
        $("#" + idModal).modal('hide');
        $("#" + idModal + "Box").detach();
        $("#" + idModal).detach();
        var schedule = scheduleData.split("E");
        scheduleData = scheduleData.replace(("E" + schedule[idModal.slice(5)]), "");
        saveSchedule(null, 0);
    }
}

function modalValueUpdate(val) {
    $("#" + val.id.split("Start")[0] + "End").val(val.value);
}

function createModal() {
    $("#Modal0Start").attr("disabled", false);
    $("#Modal0End").attr("disabled", false);
    $("#Modal0zone1").attr("disabled", false);
    $("#Modal0zone2").attr("disabled", false);
    $("#Modal0zone3").attr("disabled", false);
    $("#Modal0zone4").attr("disabled", false);
    for (var x = 1; x <= 7; x++) {
        $("#Modal0Day" + x).attr("disabled", false);
    }
    $("#Modal0").modal();
}


function updateSchedule(error, data, cb) {
    if (error) {
        scheduleData = "null";
    }
    if (data) {
        scheduleData = data.scheduleValues[0];
        $("#main").append(timerTable());
    }
}

function manualButtons() {
    var buttons = "<div class='col-sm-12 col-lg-12 text-center' id='manualButtons'>";
    for (var i = 1; i <= 4; i++) {
        buttons += "<h4>ESTUFA " + i + "</h4>"
            + "<label class='switch'><input type='checkbox' onclick='toggleCheckbox(this)'"
            + "id='" + i + "'>"
            + "<span class='slider'></span></label>";
    }
    buttons += "</div>";
    return buttons;

}


function createSPIFFS(fileList) {
    var list = "<div class='col-sm-12 col-lg-12 mx-my-5' id='SPIFFS'>"
        + "<table class='table table-bordered'>"
        + "<thead class='thead-dark'>"
        + "<tr>"
        + "<th scope='col' style='text-align:center;'>File name <br> (Click in the file to download)</th>"
        + "<th scope='col' style='text-align:center;'>Size </th>"
        + "<th scope='col' style='text-align:center;'>Delete </th>"
        + "</tr>"
        + "</thead>"
        + "<tbody>"
        + fileList
        + "</tbody >"
        + "</table >"
        + "</div > "
        + "<form method='POST' action='/file' enctype='multipart/form-data'>"
        + "<input type='file' name='data'>"
        + "<button type='submit' class='btn btn-primary'>Update</button>";
    return list;
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
        "<p class='card-text'  ><h6 style='color:blue'> Current Firmware: " + software + "</h6></p > " +
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
        }
    });
}

setInterval(function () { if ($("#1").length == 1) { jsonRequest(processResults, "/json"); } }, 550);
setInterval(timer, 1000);
$("#connectionStatus").on("load", jsonRequest(processResults, "/json"));
$("#connectionStatus").on("load", $("#main").append(createCurrentState()));
$("#currentState").on("load", jsonRequest(processState, "/json"));
var countDownDate = [0, 0, 0, 0];
var distance = [0, 0, 0, 0];
var minutes = [0, 0, 0, 0];
var seconds = [0, 0, 0, 0];
var miliSecs = [0, 0, 0, 0];
var rebootTime = 0;
var scheduleData = "E:1234:00:00:00:15:1234500E:0034:12:15:13:15:0000067E:0204:22:15:22:45:1030507";
var weekday = new Array(7);
weekday[0] = "Monday";
weekday[1] = "Tuesday";
weekday[2] = "Wednesday";
weekday[3] = "Thursday";
weekday[4] = "Friday";
weekday[5] = "Saturday";
weekday[6] = "Sunday";
var software = "";


