
This bootloader was developped by Fabian Greif.
Detailed documentation can be found at his page :
http://www.kreatives-chaos.com/artikel/can-bootloader
You can find on this web site another version of this bootloader for the AT90CAN.

Kevin Bruget adapted the bootloader to our CANinterfacer board :
http://wiki.splashelec.com/index.php/CANinterfacer
This board uses a ATMEGA32U4 and is compatible to an 
Arduino Leonardo with an CAN shield.

Instructions to use properly the CAN bootloader for Arduino

*config.h*
This file has to be modified in order to fit your configuration
(board wiring and jumper setting)
1. #define	BOOT_LED			PORT,NUMBER
2. #define	MCP2515_CS			PORT,NUMBER
3. #define	MCP2515_INT			PORT,NUMBER

*Makefile*
The makefile contains three areas that need to be modify in order to fit your project
1. MCU = your board
2. F_CPU = your frequency clock
3. BOOTLOADER_BOARD_ID = the identifier you want for this board (MUST be unique in the CAN-bus)

If you want to user other configurations, you have to compile a new bootloader
(see http://www.kreatives-chaos.com/artikel/can-bootloader#compilieren)

*Uploading*
1. Open a terminal and go to the directory of the bootloader.
2. Type "make clean" and "make all" to generate the .hex file of the bootloader
3. Type avrdude -p BOARD_TYPE (e.g m32u4 for Leonardo) -c PROGRAMMER (e.g usbtiny) -U flash:w:bootloader.hex:i 

Have a nice day with the bootloader ;-)
