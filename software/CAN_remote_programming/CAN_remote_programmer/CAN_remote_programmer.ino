
/*
 * Copyright (C) 2013 Kevin Bruget
 * Copyright (C) 2008 Fabian Greif, Roboterclub Aachen e.V.
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your 
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, see <http://www.gnu.org/licenses/>.
 */

/* 
This is the Arduino sketch to reprogram nodes through the CAN bus. You will need
a CAN shield or an Arduino compatible board with integrated can controller, see 
for example:
   http://wiki.splashelec.com/CANinterfacer 

The board which runs this sketch will act as a CAN-USB interface for a host 
computer : it receives USB data from the python script and sends CAN message to 
a distant node to reprogram it. 

The work is heavily based on the Fabian Greif's CAN Debugger software, which has 
much more features and an integrated shell to observe CAN trafic from an USB 
connected PC.

Fabian Greifs hardware too is more powerfull as it has a more high speed USB 
connection an provisions for electrical isolation, see:
http://www.kreatives-chaos.com/artikel/can-debugger

*/




#include <Can.h>

const prog_uint8_t can_filter[] = 
{
	// Group 0
	MCP2515_FILTER(0),				// Filter 0
	MCP2515_FILTER(0),				// Filter 1
	
	// Group 1
	MCP2515_FILTER_EXTENDED(0),		// Filter 2
	MCP2515_FILTER_EXTENDED(0),		// Filter 3
	MCP2515_FILTER_EXTENDED(0),		// Filter 4
	MCP2515_FILTER_EXTENDED(0),		// Filter 5
	
	MCP2515_FILTER(0),				// Mask 0 (for group 0)
	MCP2515_FILTER_EXTENDED(0),		// Mask 1 (for group 1)
};

void StreamPrint_progmem(Print &out,PGM_P format,...)
{
  // program memory version of printf - copy of format string and result share a buffer
  // so as to avoid too much memory use
  char formatString[128], *ptr;
  strncpy_P( formatString, format, sizeof(formatString) ); // copy in from program mem
  // null terminate - leave last char since we might need it in worst case for result's \0
  formatString[ sizeof(formatString)-2 ]='\0'; 
  ptr=&formatString[ strlen(formatString)+1 ]; // our result buffer...
  va_list args;
  va_start (args,format);
  vsnprintf(ptr, sizeof(formatString)-1-strlen(formatString), formatString, args );
  va_end (args);
  formatString[ sizeof(formatString)-1 ]='\0'; 
  out.print(ptr);
}
 
#define Serialprint(format, ...) StreamPrint_progmem(Serial,PSTR(format),##__VA_ARGS__)
#define Streamprint(stream,format, ...) StreamPrint_progmem(stream,PSTR(format),##__VA_ARGS__)


void term_put_hex(const uint8_t val)
{
  uint8_t tmp = val >> 4;
  
  if (tmp > 9)
    tmp += 'A' - 10;
  else 
    tmp += '0';
  Serial.write(tmp);
  
  tmp = val & 0x0f;
  
  if (tmp > 9) 
    tmp += 'A' - 10;
  else 
    tmp += '0';
  Serial.write(tmp);
}

void setup(){
      Serial.begin(115200);
      while (!Serial) {
        ; // wait for serial port to connect. Needed for Leonardo only
      }

      // Initialize MCP2515
      CAN.mcp2515_init(BITRATE_250_KBPS);
      CAN.activateInterrupt();
      // Load filters and masks
      //can.mcp2515_static_filter(can_filter);
      CAN.resetFiltersAndMasks();
}

void loop()
{      
    static char buffer[40];
    static uint8_t pos; 
    can_t message;
    
    //Get the bootloader response if any
    if (CAN.mcp2515_get_message(&message))
    {
      uint8_t length = message.length;
      
      //Case of a remote frame			
      if (message.flags.rtr)
      {
        // print identifier if extended ID
        if (message.flags.extended) 
        {
          Serialprint("R%081x",message.id);
        } 
        else  // print identifier
        {
          uint16_t id = message.id;
          Serialprint("r%03x",id);
        }
        Serial.write(length + '0');
      }
      else     //Case of a data frame			
      {
        // print identifier if extended ID
        if (message.flags.extended) 
        {
          Serialprint("T%081x",message.id);
        } 
        else // print identifier
        {
          uint16_t id = message.id;
          Serialprint("t%03x",id);
        }
        Serial.write(length + '0');
      				
        // print data
        for (uint8_t i = 0; i < length; i++)
          term_put_hex(message.data[i]);
        }
          
      Serial.write('\r');
    }
      
    while(Serial.available()) /// receive USB Data from python script
    {  
    
      char chr = Serial.read(); // read the incoming data
      if (chr != '\r'){ // wait the end of the message
                   
        buffer[pos] = chr;
        pos++;
      			
        if (pos >= sizeof(buffer)) {
        // format-error: command to long!
        pos = 0;
        }
      }
      else {
        buffer[pos] = '\0';
        CAN.usbcan_decode_command(buffer, pos); // try to send by CAN
        pos = 0;
       }
    }
}


