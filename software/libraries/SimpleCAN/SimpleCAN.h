#ifndef SIMPLECAN_H
#define SIMPLECAN_H

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
#include "config.h"

#if defined (__cplusplus)
}
#endif

class SimpleCANClass
{
  public:
	int toto;
	bool leader;
	//Initialisation function with node ID
	void init(uint8_t bitrate);
	//Election process
	void election(uint16_t id);
	//Set synchronisation period
	void synchronisation();
	//Define filters according to sensor data IDs (currently work with standard ID)
	void setFilterOnID(uint16_t *ptr, size_t n_elem);
	//Define filters according to a range of IDs (currently work with standard ID)
	//See documentation for the description of each range table
	void setFilterOnRangeID(int rangeRXF0=0, int rangeRXF1=0, int rangeRXF2=0, int rangeRXF3=0, int rangeRXF4=0, int rangeRXF5=0);
	void setFilterRXF0(uint16_t id);
	void setFilterRXF1(uint16_t id);
	void setFilterRXF2(uint16_t id);
	void setFilterRXF3(uint16_t id);
	void setFilterRXF4(uint16_t id);
	void setFilterRXF5(uint16_t id);
	/**Debugging*/
	//Automatically send the message into CAN frames. Split is made if necessary
	void printString2Can(char []);
  private:
};

extern SimpleCANClass SimpleCAN;

#endif // SIMPLECAN_H




























