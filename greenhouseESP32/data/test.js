var arr = { City: 'Moscow', Age: 25 };
$.ajax({
    url: '192.168.1.254/rest',
    type: 'POST',
    data: JSON.stringify(arr),
    contentType: 'application/json; charset=utf-8',
    dataType: 'json',
    async: false,
    success: function (msg) {
        alert(msg);
    }
});