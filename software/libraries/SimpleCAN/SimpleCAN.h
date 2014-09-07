#ifndef SIMPLECAN_H
#define SIMPLECAN_H

#include "Arduino.h"
#include "../CAN/Can.h"
#include "sailingSpecifics.h"

#if defined (__cplusplus)
	extern "C" {
#endif

#include <stdint.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdbool.h>
#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>



#if defined (__cplusplus)
}
#endif

typedef struct
{
	uint32_t id;
	uint64_t data;
	uint8_t length;
} NetworkVariable;

class SimpleCANClass
{
	public:
		//XXX tenter de mettre ces variables en private (au moins VarArray)
		bool leader;
		NetworkVariable VarArray[N];
		//ISR
		void matchAndRetrieve();
		//Initialisation function with node ID
		void init(uint8_t bitrate);
		//Election process
		void election(uint32_t id);
		//Set synchronisation period
		void synchronisation();
		//New subscription
		void keepVariableUpdated(uint32_t id);
		//Update the variable
		void setVariable(uint32_t id,uint64_t val);
		//Get the variable
		uint64_t getVariable(uint32_t id);		

		/**Debugging*/
		//Automatically send the message into CAN frames. Split is made if necessary
		void printString2Can(char []);

	private:
	
		
};

void isrSimpleCAN();

extern SimpleCANClass SimpleCAN;

#endif // SIMPLECAN_H
