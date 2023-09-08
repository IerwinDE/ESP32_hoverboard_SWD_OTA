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
static int flashTask, lastTask=0;
static uint8_t buffer[35000]; //large enough to hold the entire binary
static size_t bufferSize = 0;
static size_t bufferPos = 0;
static String index_html = index_html_template;
STM32Flash *flasherA;
STM32Flash *flasherB;
     

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


  flasherA = new STM32Flash(swd_clock_pin_A, swd_data_pin_A, ARMDebug::LOG_NONE);
  flasherB = new STM32Flash(swd_clock_pin_B, swd_data_pin_B, ARMDebug::LOG_NONE);
}

void loop()
{
  ArduinoOTA.handle();
  switch(flashTask){
    case 1: //flash board A
      lastTask=1;
      flasherA->debugHalt();
      if(flasherA->isLocked()){
        flasherA->removeReadProtection();
      }
      flasherA->flash(buffer, bufferSize);
      flasherA->reset();
      flashTask=0;
    break;
    case 2: //flash board B
      flasherA->debugHalt();
      lastTask=2;
      if(flasherB->isLocked()){
        flasherB->removeReadProtection();
      }
      flasherB->flash(buffer, bufferSize);
      flasherB->reset();
      flashTask=0;
    break;
    case 0: //dont flash, refresh status and sleep
      flasherA->connect();
      flasherB->connect();
      delay(1000);
    break;
    
    
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
                 flashTask = 1;
          } else {
                 flashTask = 2;
          }
        
      }
      //connect and flash    
  },handleUpload);


  server->on("/getDataFromServer", HTTP_GET, [](AsyncWebServerRequest *request){
  // Schrittweise JSON-Erstellung für beide Mainboards
  String jsonData = "{";
  
  // Daten für Mainboard A
  jsonData += "\"mainboardA\": {";
  jsonData += "\"deviceName\": \"" + flasherA->getDeviceName() + "\", ";
  jsonData += "\"deviceID\": \"" + flasherA->getDeviceId() + "\", ";
  jsonData += "\"flashSize\": \"" + String(flasherA->getFlashSize()) + "KBytes\", ";
  if(flasherA->isConnected()){
    if(flasherA->isLocked()){
      jsonData += "\"flashSize\": \"unknown\", ";
      jsonData += "\"status\": \"locked\"";
    } else {
      jsonData += "\"flashSize\": \"" + String(flasherA->getFlashSize()) + "KBytes\", ";
      jsonData += "\"status\": \"unlocked\"";
    }
  } else {
    jsonData += "\"status\": \"\"";
  }

  jsonData += "}, ";
  
  // Daten für Mainboard B
  jsonData += "\"mainboardB\": {";
  jsonData += "\"deviceName\": \"" + flasherB->getDeviceName() + "\", ";
  jsonData += "\"deviceID\": \"" + flasherB->getDeviceId() + "\", ";
  if(flasherB->isConnected()){
    if(flasherB->isLocked()){
      jsonData += "\"flashSize\": \"unknown\", ";
      jsonData += "\"status\": \"locked\"";
    } else {
      jsonData += "\"flashSize\": \"" + String(flasherB->getFlashSize()) + "KBytes\", ";
      jsonData += "\"status\": \"unlocked\"";
    }
  } else {
    jsonData += "\"status\": \"\"";
  }
    jsonData += "}, ";

    //Serial.println("handleProgress "+String(bufferSize)+" "+String(bufferPos)+" "+String(flasherA->getTotalWrittenBytes())+" "+String(flasherB->getTotalWrittenBytes())+" "+String(lastTask)+"\n");
 
    String value = "0";
    if(bufferSize>0){
      switch (lastTask)
      {
      case 1:
    
        value = String(int((float(flasherA->getTotalWrittenBytes()) / float(bufferSize)) * 100));
        break;
      
      case 2:
        value = String(int((float(flasherB->getTotalWrittenBytes()) / float(bufferSize)) * 100));
        break;
      }
    }


    jsonData += "\"progress\": \"" + value + "\"";

    if(!flasherA->isConnected() && !flasherB->isConnected()){
      jsonData += ", \"error\": \"No mainboard connected\"";
    } else {
      jsonData += ", \"error\": \"" + errorMessage + "\"";
    }

    

    jsonData += "}";

    request->send(200, "application/json", jsonData);
    });
}

void notFound(AsyncWebServerRequest *request) {
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
  Serial.println(logmessage);
  request->send(404, "text/plain", "Not found");
}


// handles upload and writes to flash
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if(flashTask!=0){
        errorMessage = "Flash task already running";
        Serial.println(errorMessage);
        return;
    }else {
      
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
  
}

