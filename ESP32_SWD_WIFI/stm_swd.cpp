#include "Arduino.h"
#include "defines.h"
#include "stm_swd.h"
#include <arm_debug.h>
#include <mutex>


std::mutex mtx;

STM32Flash::STM32Flash(unsigned clockPin, unsigned dataPin, LogLevel logLevel)
    : ARMDebug(clockPin, dataPin, logLevel)
{}

bool STM32Flash::connect(){
  std::lock_guard<std::mutex> lock(mtx);
  lastError="";
  //open SWD Connection
  bool begin = ARMDebug::begin();
  if(!begin){
    lastError="SWD connection failed";
    Serial.println(lastError);
    connected=false;
    return false;
  } else {
    Serial.println("SWD connection established");
    readDetails();
    connected=true;
    return true;
  }
}

String getNameForId(uint32_t id){
  switch (id)
  {
  case 0x414:
    return "STM32F10xx";
    break;
  
  default:
    return "unknown";
    break;
  }
}

void STM32Flash::readDetails(){
  uint32_t temp;
  memLoad(0xE0042000, temp);
  deviceId="0x" + String(temp  & 0x00000FFF, HEX);
  deviceName=getNameForId(temp  & 0x00000FFF);
  memLoad(0x1FFFF7E0, temp);
  flashSize=temp & 0x00000FFF;
  memLoad(FLASH_OBR, temp);  
  locked = ((temp & 0x2) != 0);
  Serial.println("device id: " + deviceId + " name: " + deviceName + " flash size: " + String(flashSize) + " locked: " + String(locked));
}

bool STM32Flash::flash(uint8_t* buffer, size_t bufferSize)
{
  Serial.println("flashing " + String(bufferSize) + " bytes");
  std::lock_guard<std::mutex> lock(mtx);
  long time = millis();
  totalWrittenBytes = 0;
  if(!open_flash()){
    return false;
  };
  if(!erase_flash()){
    return false;
  };
  int maxretry = MAX_RETRIES;
  // write in haldf word steps
  for (size_t i = 0; i < bufferSize; i += 2) {
    waitbusy();
    writePG1();
    uint16_t data = *((uint16_t*)(buffer + i)); 
    memStoreHalf(FLASH_OFFSET + i, data); 
    waitbusy();
    uint16_t temp;
    int readRetry= 2;
    while( !memLoadHalf(FLASH_OFFSET + i, temp) && (readRetry--)>0){
       Serial.println("retrying verification");
    }
    if (temp != data) {
        handleFault();
        Serial.println("verify failed at 0x" + String(FLASH_OFFSET + i) + ". expected: 0x" + String(data, HEX) + " read: 0x" + String(temp, HEX) + " retrying ");
        
        
        i -= 2;
        uint32_t status = checkStatusAndConfirm();
        if (status != 0x20 && status != 0x24) { //if its not just an EOP or PGERR error, reconnect
            Serial.println("FLASH_SR: 0x" +String(status, HEX)+ " reconnecting");
            connect();
            open_flash();
        }
        if((maxretry--)<=0){
          lastError="to many retries. aborting";
          Serial.println(lastError);
          return false;
        }
    } else {
        maxretry=MAX_RETRIES;
        totalWrittenBytes += 2;
    }
  }
  Serial.println("flahing complete. took " + String((millis() - time)/1000) + "s.");
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
  uint32_t temp = 0x01;
  while((temp & 0x01) != 0){
    if(!memLoad(FLASH_SR, temp)){
      temp=0xFF; //invalidate if read failed
    }
    if(millis()-timeout>waitTime){
      Serial.println("waitbusy timeout");
      return false;
    }
  }
  delayMicroseconds(2);
  return true;
}


bool STM32Flash::open_flash()
{
  //open flash for writing
  uint32_t temp;
  Serial.println("opening flash");
  memStore(FLASH_KEYR, FLASH_KEY1); //KEY 1
  memStore(FLASH_KEYR, FLASH_KEY2); //KEY 1 
  memLoad(FLASH_CR, temp);
  if( (temp & 0x80) == 0 ) {
    Serial.println("flash has been opened");
  }else{
    lastError="flash could not be opened. FLASH_CR: 0x" + String(temp, HEX) + " expected: 0x0";
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



bool STM32Flash::reset(){
    //erase the flash
    uint32_t temp = 0;
    Serial.println("resetting the STM");
    memStore(0xe000ed0c, 0x5fa0004);
    Serial.println("reset command sent");
    return true;
}


bool STM32Flash::isConnected() //perform mass erase
{
  return connected;
}

bool STM32Flash::isLocked() //perform mass erase
{
  return locked;
}

String STM32Flash::getDeviceId(){
  return deviceId;
}

String STM32Flash::getDeviceName(){
  return deviceName;
}

size_t STM32Flash::getFlashSize(){
  return flashSize;
}

size_t STM32Flash::getTotalWrittenBytes(){
  return totalWrittenBytes;
}

bool STM32Flash::removeReadProtection(){
  std::lock_guard<std::mutex> lock(mtx);
  open_flash();
  uint32_t temp;

  //open option bytes
  memStore(FLASH_OPTKEYR, FLASH_KEY1); //KEY 1
  memStore(FLASH_OPTKEYR, FLASH_KEY2); //KEY 1 

  //erase 
  memLoad(FLASH_CR, temp);
  memStore(FLASH_CR, temp | FLASH_CR_OPTER); //set OPTER bit
  memStore(FLASH_CR, temp | FLASH_CR_OPTER | FLASH_CR_STRT); //set STRT bit
  waitbusy();

  memLoad(FLASH_SR, temp);
  if((temp & 0x20) != 0){
    Serial.println("Erase Opeation on Optionbytes successful");
    memStore(FLASH_SR, temp | 0x20); //clear EOP bit
  } else {
    Serial.println("Erase Opeation on Optionbytes failed");
    return false;
  }
  waitbusy(10000); //wait up to 10 Seconds for flash to erase
  
  //now we need to disable the write protection because clearing option bytes always restores the locked state
  //disable opter bit/* Erase option bytes instruction */
	memStore(FLASH_CR, FLASH_CR_OPTPG | FLASH_CR_OPTWRE);
  memStoreHalf(FLASH_OBP_RDP, 0x00a5U);
  waitbusy();
  
  memLoad(FLASH_SR, temp);
  Serial.println("FLASH_SR: " + String(temp, HEX));
  if((temp & 0x20) != 0){
    Serial.println("Read protection removed");
    memStore(FLASH_SR, temp | 0x20); //clear EOP bit
  } else {
    Serial.println("failed to remove read protection");
    return false;
  }
  reset();
  return true;
}