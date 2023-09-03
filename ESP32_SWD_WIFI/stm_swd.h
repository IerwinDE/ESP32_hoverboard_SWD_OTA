#pragma once
#include "defines.h"
#include "arm_debug.h"

class STM32Flash : public ARMDebug
{
public:
   STM32Flash(unsigned clockPin, unsigned dataPin, LogLevel logLevel = LOG_NORMAL);
   bool flash(uint8_t* buffer, size_t bufferSize);
   size_t getTotalWrittenBytes() const { return totalWrittenBytes; }
   String getLastError() const { return lastError; }

protected:
   bool waitbusy(unsigned long waitTime = 100);
   bool open_flash();
   bool erase_flash(); //perform mass erase
   bool writePG1();
   bool connect();
   uint32_t checkStatusAndConfirm();
   String lastError;
   size_t totalWrittenBytes = 0;
};
