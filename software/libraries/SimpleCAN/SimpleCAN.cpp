#include "SimpleCAN.h"
#include "../CAN/Can.h"

//Instance of SimpleCAN
SimpleCANClass SimpleCAN;


void SimpleCANClass::init(uint8_t bitrate){
	//Initialisation of the MCP2515
	CAN.mcp2515_init(bitrate);
  	// Reset filters and masks
 	CAN.resetFiltersAndMasks();
}

void SimpleCANClass::election(uint16_t id){
	/* Node start sending its ID on the bus just after the init process
	* Then other nodes know each other IDs and could start an election
	* The minimum ID will win the election and will be responsible of sync process
	*/	
	leader = false;
	can_t msg;
	msg.id=id;
	msg.data[0]='S';
	msg.length=1;
	msg.flags.rtr=0;
	msg.flags.extended=0;
	CAN.mcp2515_send_message(&msg);
}

void SimpleCANClass::synchronisation(){
	/*Init interruption
	* Timer: 1
	* Prescaler: 64
	* Target frequency: 10hz
	* Int Mode: CTC
	*/
	cli();//stop interrupts
	//set timer1 interrupt at 10Hz
	TCCR1A = 0;// set entire TCCR1A register to 0
	TCCR1B = 0;// same for TCCR1B
	TCNT1  = 0;//initialize counter value to 0
	// set compare match register for 10hz increments
	OCR1A = F_CPU*10/(TCLOCK*64); //24999 = (16*10^6) / (10*64) - 1 (must be <65536)
	// turn on CTC mode
	TCCR1B |= (1 << WGM12);
	// Set CS10 and CS11 bit for 64 prescaler
	TCCR1B |= (1 << CS11); 
	TCCR1B |= (1 << CS10);  
	// enable timer compare interrupt
	TIMSK1 |= (1 << OCIE1A);
	sei();//allow interrupts
}

//***************Tools for debugging****************//
//This function allows the transmission of text message into a CAN message. The string is automatically cut to fit into a frame.
//For better results, use short messages (<=8 characters) to use only one frame.
void SimpleCANClass::printString2Can(char data[]){
  can_t msg;
  int compt=0;
  msg.id =0xFF;
  msg.flags.rtr = 0;
  msg.flags.extended = 0;
  for(int i=0;i<strlen(data)+1;i++){
    if(data[i]!='\0') msg.data[i%8]=data[i];
    else {
      msg.length = (i-1)-8*compt;
      CAN.mcp2515_send_message(&msg);
      break;
    }
    if(i%8==7){
      msg.length = 8;
      CAN.mcp2515_send_message(&msg);
      compt++;
    }
  }
}


/* How filters and mask work :
* Each filters have 4 registers								Each masks have 4 registers
* RXFnSIDH bits 7-0 apply on bits <10:3> of ID				RXFnSIDH bits 7-0 apply on bits <10:3> of ID
* RXFnSIDL bits 7-5 apply on bits <2:0> of ID				RXFnSIDL 7-5 apply on bits <2:0> of ID
* RXFnSID8 used for extended ID bits <15:8>					RXFnSID used for extended ID bits <15:8>	
* RXFnSID0 used for extended ID bits <7:0>					RXFnSID used for extended ID bits <7:0>
* 
* Mask Bit	|	Filter Bit	|	Message Identifier bit	|	State
*	0		|		X		|			X				| Accepted		
*	1		|		0		|			0				| Accepted		
*	1		|		0		|			1				| Rejected		
*	1		|		1		|			0				| Rejected	
*	1		|		1		|			1				| Accepted	
*
* RX0 buffer has 1 mask and 2 filters
* RX1 buffer has 1 mask and 4 filters
*/	
void SimpleCANClass::setFilterOnID(uint16_t *ptr, size_t n_elem){
	if(n_elem>5){
		//Too many id for subscription, accept all messages
		CAN.resetFiltersAndMasks();
		return;
	}
	//Set mask for buffer 0
	CAN.setMaskOrFilter(0x20,   0b11111111, 0b11100000, 0b00000000, 0b00000000);
	//Set mask for buffer 1
	CAN.setMaskOrFilter(0x24,   0b11111111, 0b11100000, 0b00000000, 0b00000000);
	//Filter RXF0 reserved for synchro message
  	setFilterRXF0(48);
	switch(n_elem){
		case 0: //No subscription, accept only sync msg
			break;
		case 1: //In case of one subscription, we set filter on buffer 1 to let buffer 0 free for sync msg
			setFilterRXF2(ptr[0]);
			break;
		case 2:
			setFilterRXF1(ptr[0]);
			setFilterRXF2(ptr[1]);
			break;
		case 3:
			setFilterRXF1(ptr[0]);
			setFilterRXF2(ptr[1]);
			setFilterRXF3(ptr[2]);
			break;
		case 4:
			setFilterRXF1(ptr[0]);
			setFilterRXF2(ptr[1]);
			setFilterRXF3(ptr[2]);
			setFilterRXF4(ptr[3]);
			break;
		case 5:
			setFilterRXF1(ptr[0]);
			setFilterRXF2(ptr[1]);
			setFilterRXF3(ptr[2]);
			setFilterRXF4(ptr[3]);
			setFilterRXF5(ptr[4]);
			break;
	}
}

/* Set filter on 3bit range ID
*  Actually only 6 ranges of IDs are created, you can add more with the same method
*  Range 1 : IDs 48-55
*  Range 2 : IDs 56-63
*  Range 3 : IDs 64-71
*  Range 4 : IDs 72-79
*  Range 5 : IDs 80-87
*  Range 6 : IDs 88-95
*/
void SimpleCANClass::setFilterOnRangeID(int rangeRXF0, int rangeRXF1, int rangeRXF2, int rangeRXF3, int rangeRXF4, int rangeRXF5){
	if(rangeRXF0 || rangeRXF1 || rangeRXF2 || rangeRXF3 || rangeRXF4 || rangeRXF5){
		//With this configuration, masks don't care about bits <3:0> of the ID
		//Set mask for buffer 0
		CAN.setMaskOrFilter(0x20,   0b11111111, 0b00000000, 0b00000000, 0b00000000);
		//Set mask for buffer 1
		CAN.setMaskOrFilter(0x24,   0b11111111, 0b00000000, 0b00000000, 0b00000000);
	}
		
	if(rangeRXF0){
		switch(rangeRXF0){
			case 1:
				setFilterRXF0(48);
				break;
			case 2:
				setFilterRXF0(56);
				break;
			case 3:
				setFilterRXF0(64);
				break;
			case 4:
				setFilterRXF0(72);
				break;
			case 5:
				setFilterRXF0(80);
				break;
			case 6:
				setFilterRXF0(88);
				break;
		}	
	}
	if(rangeRXF1){
		switch(rangeRXF1){
			case 1:
				setFilterRXF1(48);
				break;
			case 2:
				setFilterRXF1(56);
				break;
			case 3:
				setFilterRXF1(64);
				break;
			case 4:
				setFilterRXF1(72);
				break;
			case 5:
				setFilterRXF1(80);
				break;
			case 6:
				setFilterRXF1(88);
				break;
		}	
	}
	if(rangeRXF2){
		switch(rangeRXF2){
			case 1:
				setFilterRXF2(48);
				break;
			case 2:
				setFilterRXF2(56);
				break;
			case 3:
				setFilterRXF2(64);
				break;
			case 4:
				setFilterRXF2(72);
				break;
			case 5:
				setFilterRXF2(80);
				break;
			case 6:
				setFilterRXF2(88);
				break;
		}	
	}
	if(rangeRXF3){
		switch(rangeRXF3){
			case 1:
				setFilterRXF3(48);
				break;
			case 2:
				setFilterRXF3(56);
				break;
			case 3:
				setFilterRXF3(64);
				break;
			case 4:
				setFilterRXF3(72);
				break;
			case 5:
				setFilterRXF3(80);
				break;
			case 6:
				setFilterRXF3(88);
				break;
		}	
	}
	if(rangeRXF4){
		switch(rangeRXF4){
			case 1:
				setFilterRXF4(48);
				break;
			case 2:
				setFilterRXF4(56);
				break;
			case 3:
				setFilterRXF4(64);
				break;
			case 4:
				setFilterRXF4(72);
				break;
			case 5:
				setFilterRXF4(80);
				break;
			case 6:
				setFilterRXF4(88);
				break;
		}	
	}
	if(rangeRXF5){
		switch(rangeRXF5){
			case 1:
				setFilterRXF5(48);
				break;
			case 2:
				setFilterRXF5(56);
				break;
			case 3:
				setFilterRXF5(64);
				break;
			case 4:
				setFilterRXF5(72);
				break;
			case 5:
				setFilterRXF5(80);
				break;
			case 6:
				setFilterRXF5(88);
				break;
		}	
	}

}

void SimpleCANClass::setFilterRXF0(uint16_t id){
	uint8_t RX_SIDH = id>>3;
	uint8_t RX_SIDL = id<<5;
	uint8_t RX_SID8 = 0; //Not used for standard ID
	uint8_t RX_SID0 = 0; //Not used for standard ID
	CAN.setMaskOrFilter(0x00, RX_SIDH, RX_SIDL, RX_SID8, RX_SID0);
}

void SimpleCANClass::setFilterRXF1(uint16_t id){
	uint8_t RX_SIDH = id>>3;
	uint8_t RX_SIDL = id<<5;
	uint8_t RX_SID8 = 0; //Not used for standard ID
	uint8_t RX_SID0 = 0; //Not used for standard ID
	CAN.setMaskOrFilter(0x04, RX_SIDH, RX_SIDL, RX_SID8, RX_SID0);
}

void SimpleCANClass::setFilterRXF2(uint16_t id){
	uint8_t RX_SIDH = id>>3;
	uint8_t RX_SIDL = id<<5;
	uint8_t RX_SID8 = 0; //Not used for standard ID
	uint8_t RX_SID0 = 0; //Not used for standard ID
	CAN.setMaskOrFilter(0x08, RX_SIDH, RX_SIDL, RX_SID8, RX_SID0);
}

void SimpleCANClass::setFilterRXF3(uint16_t id){
	uint8_t RX_SIDH = id>>3;
	uint8_t RX_SIDL = id<<5;
	uint8_t RX_SID8 = 0; //Not used for standard ID
	uint8_t RX_SID0 = 0; //Not used for standard ID
	CAN.setMaskOrFilter(0x10, RX_SIDH, RX_SIDL, RX_SID8, RX_SID0);
}

void SimpleCANClass::setFilterRXF4(uint16_t id){
	uint8_t RX_SIDH = id>>3;
	uint8_t RX_SIDL = id<<5;
	uint8_t RX_SID8 = 0; //Not used for standard ID
	uint8_t RX_SID0 = 0; //Not used for standard ID
	CAN.setMaskOrFilter(0x14, RX_SIDH, RX_SIDL, RX_SID8, RX_SID0);
}

void SimpleCANClass::setFilterRXF5(uint16_t id){
	uint8_t RX_SIDH = id>>3;
	uint8_t RX_SIDL = id<<5;
	uint8_t RX_SID8 = 0; //Not used for standard ID
	uint8_t RX_SID0 = 0; //Not used for standard ID
	CAN.setMaskOrFilter(0x18, RX_SIDH, RX_SIDL, RX_SID8, RX_SID0);
}












