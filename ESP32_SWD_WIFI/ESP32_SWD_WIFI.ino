#define FLASH_SECTOR_SIZE 0x1000

#include <Arduino.h>
#include "defines.h"
#include "stm_swd.h"
#include "webpages.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager/tree/feature_asyncwebserver
#include <arm_debug.h>

AsyncWebServer *server;               // initialise webserver
String errorMessage = "";     // error message to display on webpage
static bool flashTask=false;
static uint8_t buffer[35000]; //large enough to hold the entire binary
static size_t bufferSize = 0;
static size_t bufferPos = 0;
unsigned swd_clock_pin = swd_clock_pin_A;
unsigned swd_data_pin = swd_data_pin_A;
static String index_html = index_html_template;
STM32Flash *flasher;
     

void setup()
{

  #ifndef DUAL_BOARD
    // hide the board selection row
    int replacePos = index_html.indexOf("id=\"boardselect\"");
    if (replacePos != -1) {
      index_html = index_html.substring(0, replacePos) + "class=\"hidden-row\"" + index_html.substring(replacePos + strlen("id=\boardselect\""));
    }
  #endif

  Serial.begin(115200);
  //swd_begin();
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  bool res;
  res = wm.autoConnect("ESP32_OTA_Flasher");
  if (!res)
  {
    Serial.println("Failed to connect");
    ESP.restart();
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname(DNS_NAME);
  ArduinoOTA.begin();
  server = new AsyncWebServer(80);
  configureWebServer();
  server->begin();

}

void loop()
{
  ArduinoOTA.handle();
  if(flashTask){
    flasher = new STM32Flash(swd_clock_pin, swd_data_pin, ARMDebug::LOG_NONE);
    flasher->flash(buffer, bufferSize);
    flashTask=false;
  }
}

unsigned long hstol(String recv)
{
  char c[recv.length() + 1];
  recv.toCharArray(c, recv.length() + 1);
  return strtoul(c, NULL, 16);
}


void configureWebServer() {
  // configure web server

  // if url isn't found
  server->onNotFound(notFound);

  // run handleUpload function when any file is uploaded
  server->onFileUpload(handleUpload);


  server->on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
      
      request->send_P(200, "text/html", index_html.c_str());
  });

  server->on("/", HTTP_POST, [](AsyncWebServerRequest * request) {
      /*Serial.println("Request Parameters:");
      for (size_t i = 0; i < request->params(); i++) {
        AsyncWebParameter* param = request->getParam(i);
        Serial.print("Name: [");
        Serial.print(param->name());
        Serial.println("]");
        Serial.print("Value: [");
        Serial.print(param->value());
        Serial.println("]");
        Serial.println("-----------");
      }*/
      AsyncWebParameter* boardParam = request->getParam("board", true);
      if (boardParam) {
          String boardValue = boardParam->value();
          if (boardValue == String("A")) {
            Serial.println("using A Pins");
            swd_clock_pin = int(swd_clock_pin_A);
            swd_data_pin = int(swd_data_pin_A);
          } else {
            Serial.println("using B Pins");
            swd_clock_pin = int(swd_clock_pin_B);
            swd_data_pin = int(swd_data_pin_B);
          }
           Serial.println("swd on pins: " + String(swd_clock_pin) + " " + String(swd_data_pin));
      }
      //connect and flash

      flashTask = true;
      

     
  },handleUpload);

  server->on("/readProgress", HTTP_GET, [](AsyncWebServerRequest * request) {
    handleProgress(request);
  });
}

void notFound(AsyncWebServerRequest *request) {
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
  Serial.println(logmessage);
  request->send(404, "text/plain", "Not found");
}


// handles upload and writes to flash
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
      Serial.println("starting upload");
      bufferPos=0;
      bufferSize=0;
    }

    if (len) {
      // Copy data to the buffer
      for (size_t i = 0; i < len; i++) {
        buffer[bufferPos++] = data[i];
      }

    }

    if (final) {
    bufferSize = index + len;
    Serial.println("upload complete, starting flash task"); 
    request->redirect("/");    
}
  
}


void handleProgress(AsyncWebServerRequest *request){
 
 String value = "0";
 if(bufferSize>0){
  value = String(int((float(flasher->getTotalWrittenBytes()) / float(bufferSize)) * 100));
 }
 //Serial.printf("BufferSize: %i, BufferPos: %i, TotalWrittenBytes: %i, Percent: %s\n", bufferSize, bufferPos, totalWrittenBytes, value);
 
 request->send(200, "text/plane", value); //Send flash progress value only to client ajax request
 
}
