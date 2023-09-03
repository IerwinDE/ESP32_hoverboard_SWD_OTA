const char index_html_template[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <style>
  .hidden-row {
    display: none;
  }
  </style>
</head>
<body>
  <h1>Hoverboard OTA Flasher</h1>
  <form id="upload_form" enctype="multipart/form-data" method="post">
  <p>
    <table>
      <tr id="boardselect">
        <td>Select Board:</td>
        <td><input type="radio" name="mainboard" id="mainboardA" value="A" checked> Mainboard A</td>
        <td><input type="radio" name="mainboard" id="mainboardB" value="B"> Mainboard B</td>
      </tr>
      <tr>
        <td>Select Firmware:</td>
        <td colspan="2"><input type="file" name="file1" id="file1"></td>
      </tr>
      
      <tr>
        <td colspan="3"><br><input type="button" name="flash" id="flash" onclick="confirmAndFlash()" value="Upload & Flash"></td>
      </tr>

    </table>
  </p>
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
    var updateStatus = false; // Status der Aktualisierung
    var updateInterval; // Speichert das Intervall für die Aktualisierung

    function startUpdating() {
      updateStatus = true;
      updateInterval = setInterval(function() {
        getData();
      }, 500);
    }

    function stopUpdating() {
      updateStatus = false;
      clearInterval(updateInterval);
    }

    function getData() {
      if (updateStatus) { // Führt die Aktualisierung nur aus, wenn updateStatus true ist
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
          if (this.readyState == 4 && this.status == 200) {
            _("progressBar2").value = this.responseText;
            _("perc2").innerHTML = this.responseText + "&#37;";

            if (parseInt(this.responseText) >= 100) {
              stopUpdating(); // Stoppt die Aktualisierung, wenn das Ergebnis 100 erreicht hat
            }
          }
        };
        xhttp.open("GET", "readProgress", true);
        xhttp.send();
      }
    }


    function _(el) {
      return document.getElementById(el);
    }

  
    function confirmAndFlash() {
      if (confirm("Are you want to flash the selected file?")) {
        var file = _("file1").files[0];
        var formdata = new FormData();
        formdata.append("file1", file);
        formdata.append("board", document.querySelector('input[name="mainboard"]:checked').value);
        var ajax = new XMLHttpRequest();
        ajax.upload.addEventListener("progress", progressHandler, false);
        ajax.addEventListener("load", completeHandler, false);
        ajax.addEventListener("error", errorHandler, false);
        ajax.addEventListener("abort", abortHandler, false);
        ajax.open("POST", "/");
        ajax.send(formdata);
        startUpdating();
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