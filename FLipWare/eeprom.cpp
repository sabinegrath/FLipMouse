#include "fabi.h"
#include <Wire.h>
#include <EEPROM.h>

#define SLOT_VALID 123
#define deviceaddress 0x50

int nextSlotAddress=0;


void writeEEPROM(unsigned int eeaddress, byte data ) 
{
  #ifdef TEENSY_LC
    Wire.beginTransmission(deviceaddress);
    Wire.send((int)(eeaddress >> 8));   // MSB
    Wire.send((int)(eeaddress & 0xFF)); // LSB
    Wire.send(data);
    Wire.endTransmission();
    delay(5);
  #else
    EEPROM.write(eeaddress, data);
  #endif
}
 
byte readEEPROM(unsigned int eeaddress ) 
{

  #ifdef TEENSY_LC
    byte rdata = 0xFF;
    Wire.beginTransmission(deviceaddress);
    Wire.send((int)(eeaddress >> 8));   // MSB
    Wire.send((int)(eeaddress & 0xFF)); // LSB
    Wire.endTransmission();
 
    Wire.requestFrom(deviceaddress,1);
 
    if (Wire.available()) rdata = Wire.receive();
 
    return rdata;
  #else
    return(EEPROM.read(eeaddress));
  #endif
}



void printCurrentSlot()
{
        Serial.print("Slot :"); Serial.println(settings.slotname);
        Serial.print("AT AX "); Serial.println(settings.ax); 
        Serial.print("AT AY "); Serial.println(settings.ay);
        Serial.print("AT DX "); Serial.println(settings.dx);
        Serial.print("AT DY "); Serial.println(settings.dy);
        Serial.print("AT TS "); Serial.println(settings.ts);
        Serial.print("AT TP "); Serial.println(settings.tp);
        Serial.print("AT WS "); Serial.println(settings.ws);
        Serial.print("AT TT "); Serial.println(settings.tt);
        Serial.print("AT MM "); Serial.println(settings.mouseOn);
        Serial.print("AT GU "); Serial.println(settings.gu);
        Serial.print("AT GD "); Serial.println(settings.gd);
        Serial.print("AT GL "); Serial.println(settings.gl);
        Serial.print("AT GR "); Serial.println(settings.gr);
        for (int i=0;i<NUMBER_OF_BUTTONS;i++) 
        {
           Serial.print("AT BM "); 
           if (i<9) Serial.print("0");
           Serial.println(i+1); 
           Serial.print("AT "); 
           int actCmd = buttons[i].mode;
           char cmdStr[4];
           strcpy_PF(cmdStr,(uint_farptr_t)atCommands[actCmd].atCmd);
           Serial.print(cmdStr);
            switch (pgm_read_byte_near(&(atCommands[actCmd].partype))) 
            {
               case PARTYPE_UINT: 
               case PARTYPE_INT:  Serial.print(" ");Serial.print(buttons[i].value); break;
               case PARTYPE_STRING: Serial.print(" ");Serial.print(buttons[i].keystring); break;
            }
            Serial.println("");
        }
}

void saveToEEPROM(char * slotname)
{
   char act_slotname[MAX_SLOTNAME_LEN];
   int address = 0;
   int tmpStartAddress=0;
   uint8_t found=0;
   uint8_t * p;

   if (!slotname) address=EmptySlotAddress;
   else
   {
     if (strlen(slotname) >= MAX_SLOTNAME_LEN) slotname[MAX_SLOTNAME_LEN-1]=0;
     
     while ((readEEPROM(address)==SLOT_VALID) && (!found) && ((address+1) < EmptySlotAddress))  // indicates valid eeprom content !
     {
       tmpStartAddress=address;
       address++;

       uint8_t i=0;
       while ((act_slotname[i++]=readEEPROM(address++)) != 0) ;      

       // Serial.print("peeking slot "); Serial.print(act_slotname); Serial.print(" @ "); Serial.println(tmpStartAddress);
       address=tmpStartAddress;
       if (!strcmp(act_slotname, slotname)) found=1; 
       else  address+= (sizeof(settingsType)+NUMBER_OF_BUTTONS*sizeof(buttonType)+1);      
     }
   }

   if (DebugOutput==DEBUG_FULLOUTPUT)  {
     Serial.print("Writing slot "); if (slotname) Serial.print(slotname);
     Serial.print(" starting from EEPROM address "); Serial.println(address);
   }
   
   // start with new slot 
   writeEEPROM(address++,SLOT_VALID);  

   // update slotname
   if (!slotname) settings.slotname[0]=0;
   else strcpy(settings.slotname,slotname);
         
   // write general settings 
   p = (uint8_t*) &settings;
   for (unsigned int t=0;t<sizeof(settingsType);t++)
      writeEEPROM(address++,*p++);

   // write all buttons
   p = (uint8_t*) buttons;
   for (unsigned int i=0;i<NUMBER_OF_BUTTONS*sizeof(buttonType);i++)
        writeEEPROM(address++,*p++);

   if (EmptySlotAddress<=address) {
     writeEEPROM(address,0);
     EmptySlotAddress=address;
  
  }
}

void readFromEEPROM(char * slotname)
{
   char act_slotname[MAX_SLOTNAME_LEN];
   int address=0;
   int tmpSlotAddress=0;
   int tmpStartAddress=0;
   //uint8_t done;
   uint8_t numSlots=0;
   uint8_t* p;
   
   
   while (readEEPROM(address)==SLOT_VALID)  // indicates valid eeprom content !
   {
      uint8_t found=0;
     
      if (reportSlotParameters) found=1;
      if ((!slotname) && (address==nextSlotAddress)) found=1;
      address++;

      tmpStartAddress=address;
      uint8_t i=0;
      while ((act_slotname[i++]=readEEPROM(address++)) != 0) ; 
 
      if (DebugOutput==DEBUG_FULLOUTPUT)  
      {  Serial.print("found slotname "); Serial.println(act_slotname); }
     
      if (slotname)  {
        if (!strcmp(act_slotname, slotname)) found=1;  
      }
      
      address=tmpStartAddress;
      if (found)  {
           
        p = (uint8_t*) &settings;
        for (unsigned int t=0;t<sizeof(settingsType);t++)
            *p++=readEEPROM(address++);
        
        
        p = (uint8_t*) buttons;
        for (unsigned int i=0;i<NUMBER_OF_BUTTONS*sizeof(buttonType);i++) 
           *p++=readEEPROM(address++);
        
        if (reportSlotParameters)   
           printCurrentSlot();

        actSlot=numSlots+1; 
        tmpSlotAddress=address;
      } 
      else address += (sizeof(settingsType) + NUMBER_OF_BUTTONS * sizeof(buttonType));   // skip this slot  
      numSlots++;
   }
   
   makeTone (TONE_CHANGESLOT,actSlot);

   EmptySlotAddress=address;
   if (tmpSlotAddress) nextSlotAddress=tmpSlotAddress;
   if (nextSlotAddress==EmptySlotAddress) nextSlotAddress=0;
   
   if (DebugOutput==DEBUG_FULLOUTPUT)  
   {
       Serial.print(numSlots); Serial.print(" slots were found in EEPROM, occupying ");
       Serial.print(address); Serial.println(" bytes.");
   }
   if (reportSlotParameters) 
       Serial.println("END");

}

void deleteSlots()
{
   EmptySlotAddress=0;
   nextSlotAddress=0;
   writeEEPROM(0,0);
}

void listSlots()
{
   int address=0,tmpStartAddress=0;
   uint8_t numSlots=0;
   uint8_t b;
   
   while (readEEPROM(address)==SLOT_VALID)  // indicates valid eeprom content !
   {
     numSlots++;
     address++;
     tmpStartAddress=address;
     Serial.print("Slot"); Serial.print(numSlots); Serial.print(":"); 
     while ((b=readEEPROM(address++)) != 0)   // print slot name
         Serial.write(b);
     Serial.println();
     
     address=tmpStartAddress+sizeof(settingsType)+NUMBER_OF_BUTTONS*sizeof(buttonType);         
   }
}




