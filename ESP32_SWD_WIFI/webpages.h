const char index_html_template[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <style>
    body {
      font-family: Arial, Helvetica, sans-serif;
    }
    .hidden-row {
      display: none;
    }
    .board-container {
      display: inline-block;
      border: 1px solid #ccc;
      padding: 10px;
      text-align: left;
    }
    .board-title {
      float: left;
      font-weight: bold;
    }
    .param-label {
      float:left;
      width: 150px;
      padding-left: 0px;
    }
    .error {
      font-weight: bold;
      float:left;
      padding-top: 5px;
      padding-left: 0px;
    }
  </style>
</head>
<body>
  <h1>Hoverboard OTA Flasher</h1>
  

<div class="board-container" id="mainboardAContainer">
  <span class="board-title"><input type="radio" name="board" id="mainboardA" value="A" checked> Mainboard A</span><br>
  <span class="param-label">Device Name:</span> <span id="deviceNameA"></span><br>
  <span class="param-label">Device ID:</span> <span id="deviceIDA"></span><br>
  <span class="param-label">Flash Size:</span> <span id="flashSizeA"></span><br>
  <span class="param-label">Status:</span> <span id="statusA"></span><br>
</div>

<div class="board-container" id="mainboardBContainer">
  <span class="board-title"><input type="radio" name="board" id="mainboardB" value="B"> Mainboard B</span></span><br>
  <span class="param-label">Device Name:</span> <span id="deviceNameB"></span><br>
  <span class="param-label">Device ID:</span> <span id="deviceIDB"></span><br>
  <span class="param-label">Flash Size:</span> <span id="flashSizeB"></span><br>
  <span class="param-label">Status:</span> <span id="statusB"></span><br>
</div>
<br>
<span class="error" id="error"></span><br>
<br>
<form id="upload_form" enctype="multipart/form-data" method="post">
  <p>
    <label for="file1">Select Firmware:</label>
    <input type="file" name="file1" id="file1">
  </p>
  <input type="button" name="flash" id="flash" onclick="confirmAndFlash()" value="Upload & Flash">
</form>

  <table>
    <tr>
      <td>Upload:</td>
      <td><progress id="progressBar1" value="0" max="100"></progress></td>
      <td><span id="perc1">0&#37;</span></td>
    </tr>
    <tr>
      <td>Flash:</td>
      <td><progress id="progressBar2" value="0" max="100"></progress></td>
      <td><span id="perc2">0&#37;</span></td>
    </tr>
  </table>

  
  
  <script>
    var updateInterval;

    function startUpdating() {
      clearInterval(updateInterval);
      updateInterval = setInterval(function() {
        getDataFromServer();
      }, 1000);
    }

    function stopUpdating() {
      clearInterval(updateInterval);
    }

    function getDataFromServer() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var data = JSON.parse(this.responseText);

      // Mainboard A
      document.getElementById("deviceNameA").textContent = data.mainboardA.deviceName;
      document.getElementById("deviceIDA").textContent = data.mainboardA.deviceID;
      document.getElementById("flashSizeA").textContent = data.mainboardA.flashSize;
      document.getElementById("statusA").textContent = data.mainboardA.status;

      // Mainboard B
      document.getElementById("deviceNameB").textContent = data.mainboardB.deviceName;
      document.getElementById("deviceIDB").textContent = data.mainboardB.deviceID;
      document.getElementById("flashSizeB").textContent = data.mainboardB.flashSize;
      document.getElementById("statusB").textContent = data.mainboardB.status;

      if (data.mainboardA && data.mainboardA.status === "") {
        document.getElementById("mainboardAContainer").style.display = "none"; 
        document.getElementById("mainboardB").checked = true;
        document.getElementById("mainboardB").style.display = "none"; 
      } else {
        document.getElementById("mainboardAContainer").style.display = "inline-block"; 
        document.getElementById("mainboardB").style.display = "inline-block"; 
      }

      if (data.mainboardA && data.mainboardB.status === "") {
        document.getElementById("mainboardBContainer").style.display = "none"; 
        document.getElementById("mainboardA").checked = true;
        document.getElementById("mainboardA").style.display = "none"; 
      } else {
        document.getElementById("mainboardBContainer").style.display = "inline-block"; 
        document.getElementById("mainboardA").style.display = "inline-block"; 
      }

      _("progressBar2").value = data.progress;
      _("perc2").innerHTML = data.progress + "&#37;";

      _("error").innerHTML = data.error;
    }
  };
  xhttp.open("GET", "/getDataFromServer", true);
  xhttp.send();
}

    // Starten Sie die Aktualisierung, wenn die Seite geladen wird
    window.onload = function() {
      startUpdating();
    };

    

    function _(el) {
      return document.getElementById(el);
    }

  
    function confirmAndFlash() {
      var securityQuestion = "Are you want to flash the selected file?"
      if ((document.querySelector('input[name="board"]:checked').value == "A" && _("statusA").innerHTML == "locked") || (document.querySelector('input[name="board"]:checked').value == "B" && _("statusB").innerHTML == "locked"))
      {
        securityQuestion = "\n\nYou are about to flash a read protected device, the original firmware will be overwritten and lost. Are you sure?";
      } 
      if (confirm(securityQuestion)) {
        var file = _("file1").files[0];
        var formdata = new FormData();
        formdata.append("file1", file);
        formdata.append("board", document.querySelector('input[name="board"]:checked').value);
        var ajax = new XMLHttpRequest();
        ajax.upload.addEventListener("progress", progressHandler, false);
        ajax.addEventListener("load", completeHandler, false);
        ajax.addEventListener("error", errorHandler, false);
        ajax.addEventListener("abort", abortHandler, false);
        ajax.open("POST", "/");
        ajax.send(formdata);
      }
    }

    function progressHandler(event) {
      var percent = (event.loaded / event.total) * 100;
      _("progressBar1").value = Math.round(percent);
      _("perc1").innerHTML = Math.round(percent) + "&#37;";
    }

    function completeHandler(event) {
      var responseText = event.target.responseText;
      _("progressBar1").value = 100;
      _("perc1").innerHTML = "100%";
    }

    function errorHandler(event) {
      _("status").innerHTML = "Upload Failed";
    }

    function abortHandler(event) {
      _("status").innerHTML = "Upload Aborted";
    }

  
  </script>
</body>
</html>

)rawliteral";