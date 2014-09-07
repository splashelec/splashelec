#ifndef CONFIG_H
#define CONFIG_H

//Number of entries
#define ENTRIES 7
//Clock period of 100 ms
#define TCLOCK 100
//Clock of the board
#define F_CPU 16000000
//Number of network variables
#define N 8

#include <Arduino.h>

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

//XXX
//Faire les correspondance idNV <-> Nom de variable. Ex: #define AngleBarre 54

#define headSailWinchSpeed 50
#define mainSailWinchSpeed 51
#define rudderAngle 54

//Network variables
typedef struct {
	uint16_t id;
	char* name;
	uint8_t data[8];
	float scaling;
	uint8_t length;
	uint16_t timestamp;
} networkVariables_t;

/*Database of all information available trought nodes
* Warning: ID 0x00-0x48 are reserved for the SimpleCAN protocol
* 		   ID 0xFF and board IDs are reserved for the bootloader reprogramming
* Do not use for data definition	
*/


// XXX to remove ?
const networkVariables_t networkVariables[ENTRIES] PROGMEM = {
//Accelero
	{	51, 
		"Accelero", 
		{0,0,0,0,0,0,0,0},
		1.84,
		6,
		0
	},
//Speedo
	{	52, 
		"Speedo",
		{0,0,0,0,0,0,0,0},
		10.5,
		6,
		0
	},
//Compass
	{	53, 
		"Compass",
		{0,0,0,0,0,0,0,0},
		0.5,
		5,
		0
	},
//Joystick
	{	54, 
		"Joystick",
		{0,0,0,0,0,0,0,0},
		0.5,
		7,
		0
	},
//Wind
	{	55, 
		"Wind",
		{0,0,0,0,0,0,0,0},
		0.5,
		7,
		0
	},
//GPS Longitude
	{	56, 
		"Longitude", 
		{0,0,0,0,0,0,0,0},
		0.125,
		6,
		0
	},
//GPS Lattitude
	{	57, 
		"Lattitude", 
		{0,0,0,0,0,0,0,0},
		0.125,
		6,
		0
	}
};

//Mapping functions
void mapSend(uint8_t*, uint64_t, uint32_t);
void mapReceive(uint64_t*, uint8_t*, uint8_t);

//XXX
//Automatic pilot

#endif // CONFIG_H
