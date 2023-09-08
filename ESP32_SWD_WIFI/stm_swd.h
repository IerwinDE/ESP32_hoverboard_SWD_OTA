#pragma once
#include "defines.h"
#include "arm_debug.h"

class STM32Flash : public ARMDebug
{
public:
   STM32Flash(unsigned clockPin, unsigned dataPin, LogLevel logLevel = LOG_NORMAL);
   bool flash(uint8_t* buffer, size_t bufferSize);
   size_t getTotalWrittenBytes();
   String getLastError() const { return lastError; }
   bool isConnected();
   bool isLocked();
   String getDeviceName();
   String getDeviceId();
   size_t getFlashSize();
   bool isReadProtected();
   bool removeReadProtection();
   bool reset();
   bool connect();
   
protected:
   void readDetails();
   bool waitbusy(unsigned long waitTime = 100);
   bool open_flash();
   bool erase_flash(); //perform mass erase
   bool writePG1();
   uint32_t checkStatusAndConfirm();

   

private:
   bool connected=false;
   bool locked=true;
   String lastError="";
   String deviceName="";
   String deviceId="";
   size_t flashSize=0;
   size_t totalWrittenBytes = 0;
};
