#include "SimpleCAN.h"

//Instance of SimpleCANClass
SimpleCANClass SimpleCAN;

void isrSimpleCAN() {
    SimpleCAN.matchAndRetrieve();
}

//ISR
void SimpleCANClass::matchAndRetrieve(){
	
	uint32_t bufID = 0;
	uint8_t statu = 0;
	uint8_t frameType = 0;
	

	CAN.mcp2515_read_id_complete(&bufID,&statu,&frameType);

	//Tests if the array contains a node matching the ID in the buffer
	
	int i = 0;
	NetworkVariable currentNV;
	uint32_t currentID = 0;
	do{
		currentNV = SimpleCAN.VarArray[i];
		currentID = currentNV.id;
		i++;
	}
	while(i != N && currentID != bufID);

	if(i != N+1)
	{
		//Continue to read the message in the buffer and convert it
		can_t msg;
		CAN.mcp2515_get_data(&msg,frameType,statu);
		mapReceive(&SimpleCAN.VarArray[i-1].data,msg.data,msg.length);
	}
}

void SimpleCANClass::init(uint8_t bitrate){

	//Initialisation of the MCP2515
	CAN.mcp2515_init(bitrate);
  	// Reset filters and masks
 	CAN.resetFiltersAndMasks();
	// Initialization of network variable array
	for (int i=0; i<N; i++){
		this->VarArray[i].id = 0;
		this->VarArray[i].data = 0;
		this->VarArray[i].length = 0;
	}
	// Configuration of interrupts
	attachInterrupt(0,isrSimpleCAN,FALLING);
	//Run the ISR to check if an interruption has already occured
	isrSimpleCAN();
}

void SimpleCANClass::election(uint32_t id){
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
		if(data[i]!='\0'){
			msg.data[i%8]=data[i];
		}
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

void SimpleCANClass::setVariable(uint32_t id, uint64_t data){
 	can_t msg;
	msg.flags.rtr = 0;
	msg.id=id;

	//Put data into data[8]
        mapSend(msg.data,data,id);

	//Precise how many bytes are used
	msg.length=8;

	//send the CAN message
	noInterrupts(); // needed to avoid conflicts while using SPI
	CAN.mcp2515_send_message(&msg);
	interrupts();
}



//New subscription to a network variable
void SimpleCANClass::keepVariableUpdated(uint32_t id){

	//Add it to the list
	int i=0;
	while(this->VarArray[i].id != 0){
		i++;
	}
	this->VarArray[i].id=id;
}



uint64_t SimpleCANClass::getVariable(uint32_t id){

	NetworkVariable currentNV;
	uint32_t currentID;

	int i = 0;
	do{
		currentNV = this->VarArray[i];
		currentID = currentNV.id;
		i++;
	}
	while(i != N+1 && currentID != id);

	if(i != N+1)
	{
		return currentNV.data;
		
	}
	else
	{
		return -1;
		
	}
	
}
