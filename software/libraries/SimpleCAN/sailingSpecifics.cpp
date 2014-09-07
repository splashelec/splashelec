#include "sailingSpecifics.h"


//Mapping functions
void mapSend(uint8_t data[8], uint64_t value, uint32_t id)
{
	
	//Conversion
	switch(id)
	{
		case 50: //headSailWinchSpeed
		{

		}
		case 51: //mainSailWinchSpeed
		{

		}
		case 52: //
		{

		}
		case 53: //Seat
		{
			
		}
		case 54: //rudderAngle
		{
			
		}
		default:
		{

		}
	}

	//Partitioning
	for(int i = 0 ; i<8 ; i++)
	{
		data[i]=(uint8_t)(value>>(8*i));
	}
}

void mapReceive(uint64_t* data, uint8_t tmp[8], uint8_t length)
{
	*data=0;
	//Recombination
	for(int i = 0 ; i<length ; i++)
	{
		*data=*data | ((uint64_t)(tmp[i])<<(8*i));	
	}
	
}
