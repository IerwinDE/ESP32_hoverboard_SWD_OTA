#include "Arduino.h"
#include "defines.h"
#include "stm_swd.h"
#include <arm_debug.h>



STM32Flash::STM32Flash(unsigned clockPin, unsigned dataPin, LogLevel logLevel)
    : ARMDebug(clockPin, dataPin, logLevel)
{}

bool STM32Flash::connect(){
  //open SWD Connection
  bool begin = ARMDebug::begin();
  if(!begin){
    lastError="SWD connection failed";
    Serial.println(lastError);
    return false;
  } else {
    Serial.println("SWD connection established");
    return true;
  }
  
}

bool STM32Flash::flash(uint8_t* buffer, size_t bufferSize)
{
  totalWrittenBytes = 0;
  if(!connect()){
    return false;
  };
  if(!open_flash()){
    return false;
  };
  if(!erase_flash()){
    return false;
  };
  int maxretry = MAX_RETRIES;
  // write in haldf word steps
  for (size_t i = 0; i < bufferSize; i += 2) {
    maxretry=MAX_RETRIES;
    waitbusy();
    writePG1();
    uint16_t data = *((uint16_t*)(buffer + i)); 
    memStoreHalf(FLASH_OFFSET + i, data); 
    waitbusy();
    uint16_t temp;
    memLoadHalf(FLASH_OFFSET + i, temp);
    if (temp != data) {
        maxretry--;
        handleFault();
        Serial.println("verify failed at 0x" + String(FLASH_OFFSET + i) + ". expected: 0x" + String(data, HEX) + " read: 0x" + String(temp, HEX) + " retrying");
        i -= 2;
        uint32_t status = checkStatusAndConfirm();
        if (status != 0x20 && status != 0x24) { //if its not just an EOP or PGERR error, reconnect
            connect();
            open_flash();
        }
        if(maxretry--<=0){
          lastError="to many retries. aborting";
          Serial.println(lastError);
          return false;
        }
    } else {
        totalWrittenBytes += 2;
    }
  }
  Serial.println("flahing complete");
  return true;
}


uint32_t STM32Flash::checkStatusAndConfirm(){
  uint32_t temp;
  memLoad(FLASH_SR, temp);
  Serial.println("flash status checkandconfirm: 0x" + String(temp, HEX));
  if(temp & 0x20 != 0){
    Serial.println("EOP bit set. confirming");
    memStore(FLASH_SR, 0x20); //clear EOP bit
  }
  if(temp & 0x04 != 0){
    Serial.println("PGERR bit set. confirming");
    memStore(FLASH_SR, 0x04); //clear PGERR bit
  }
  return temp;
}

bool STM32Flash::waitbusy(unsigned long waitTime){ //wait for busy flag to reset. Will return false if a timeout occured
  long timeout = millis();
  uint32_t temp;
  while(temp & 0x01 != 0){
    if(!memLoad(FLASH_SR, temp)){
      temp=0xFF; //invalidate if read failed
    }
    if(millis()-timeout>waitTime){
      Serial.println("waitbusy timeout");
      return false;
    }
  }
  return true;
}


bool STM32Flash::open_flash()
{
  //open flash for writing
  uint32_t temp;
  Serial.println("opening flash");
  temp = checkStatusAndConfirm();
  memStore(0x40022004, 0x45670123); //KEY 1
  memStore(0x40022004, 0xCDEF89AB); //KEY 1 
  temp = checkStatusAndConfirm();
  if( temp == 0 ) {
    Serial.println("flash has been opened");
  }else{
    lastError="flash could not be opened";
    Serial.println(lastError);
    return false;
  }
  return true;
}


bool STM32Flash::writePG1()
{
  return memStore(FLASH_CR, 0x01); //set PG bit
}


  
bool STM32Flash::erase_flash() //perform mass erase
{
  //erase the flash
    uint32_t temp = 0;
    //Serial.println("erasig flash");
    memStore(FLASH_CR, temp | 0x04); //Set the MER bit in the FLASH_CR register
    memStore(FLASH_CR, temp | 0x44); //Set the STRT bit in the FLASH_CR register
    waitbusy(10000); //wait up to 10 Seconds
  
    //Read and verify
    memLoad(FLASH_OFFSET, temp);
    if(temp!=0xffffffff){
      Serial.println("flash erase failed");
      return false;
    }
    Serial.println("flash has been cleared");
    return true;
}




