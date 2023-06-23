var getJSON = function (url, callback) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url, true);
    xhr.responseType = 'json';
    xhr.onload = function () {
        var status = xhr.status;
        if (status == 200) {
            callback(null, xhr.response);
        } else {
            callback(status);
        }
    };
    xhr.send();
};

var doAsync = function(theUrl, type, json, callback) {
    var xmlHttp = new XMLHttpRequest();
    xmlHttp.responseType = 'json';
    xmlHttp.onreadystatechange = function() {
        if (xmlHttp.readyState == 4 && xmlHttp.status == 200)
            callback(xmlHttp.response);
    }

    xmlHttp.open(type, theUrl, true); // true for asynchronous 
    xmlHttp.send(json);
}


var sendData = function() {
    var config = new Object();


    config.deviceName = document.getElementById('deviceName').value;

    config.deviceName = document.getElementById('deviceName').value;
    config.stSSID = document.getElementById('stSSID').value;
    config.stPASS = document.getElementById('stPASS').value;
    config.ip = document.getElementById('ip').value;
    config.gateway = document.getElementById('gateway').value;
    config.subnet = document.getElementById('subnet').value;
    config.dns = document.getElementById('dns').value;

    config.www_username = "admin";
    config.www_password = "admin";
    config.apSSID = "MyIoT";
    config.apPASS = "a1234567";

    console.log(JSON.stringify(config));

    const formData = new FormData();
    const blob = new Blob([JSON.stringify(config)], {type: "application/json"});
    formData.append('file', blob, 'config.json');
    doAsync('upload', "POST", formData , function(data) {
        onLoad();
    });
    
}

function onLoad() {

    getJSON('config.json', function (err, data) {
        if (err == null) {
            document.getElementById('deviceName').value = data.deviceName;
            document.getElementById('stSSID').value = data.stSSID;
            document.getElementById('stPASS').value = data.stPASS;
            document.getElementById('ip').value = data.ip;
            document.getElementById('gateway').value = data.gateway;
            document.getElementById('subnet').value = data.subnet;
            document.getElementById('dns').value = data.dns;
        }
    });


    getJSON('networks', function (err, data) {
        if (err == null) {
            var options = '';
            for (i = 0; i < data.length; i++) {
                options += '<option value="' + data[i].ssid + '" />';
            }
            document.getElementById('ssids').innerHTML = options;
        }
    });

}

