
// This Arduino version library was developed based on Fabian Greif's  
// "universelle-can-bibliothek",
// see: http://www.kreatives-chaos.com/artikel/universelle-can-bibliothek
// and: https://github.com/dergraaf/avr-can-lib
// This work is published under a BSD license.

// The adaptation to Arduino was done by Kevin Bruget.


// This version of the library works for the moment only with the 
// MCP2515 CAN controler.
// The original library works also with the SJA1000 controller as well
// as with the AT90CAN microcontroller integrated CAN hardware.

// ToDo: 
// - recovering comments and documentation from the original library
// - adding Arduino style example sketches 
// - using universal methods where possible instead of the MCP2515 versions
// - adding a keywords.txt file
// - re-adding SJA1000, AT90CAN code
// - add Arduino Due support  

 
#ifndef CAN_H
#define CAN_H
#include "Arduino.h"

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
#include <string.h>

#if defined (__cplusplus)
}
#endif
/****************************config.h****************************/
// -----------------------------------------------------------------------------
/* Global settings for building the can-lib and application program.
 *
 * The following two #defines must be set identically for the can-lib and
 * your application program. They control the underlying CAN struct. If the
 * settings disagree, the underlying CAN struct will be broken, with
 * unpredictable results.
 * If can.h detects that any of the #defines is not defined, it will set them
 * to the default values shown here, so it is in your own interest to have a
 * consistent setting. Ommiting the #defines in both can-lib and application
 * program will apply the defaults in a consistent way too.
 *
 * Select if you want to use 29 bit identifiers.
 */

#define	SUPPORT_EXTENDED_CANID	1

/* Select if you want to use timestamps.
 * Timestamps are sourced from a register internal to the AT90CAN.
 * Selecting them on any other controller will have no effect, they will
 * be 0 all the time.
 */
#define	SUPPORT_TIMESTAMPS		0


// -----------------------------------------------------------------------------
/* Global settings for building the can-lib.
 *
 * Select ONE CAN controller for which you are building the can-lib. 
 */
#define	SUPPORT_MCP2515			1

/****************************spi.h****************************/
#ifndef SPI_PRESCALER
	#define	SPI_PRESCALER			8
#endif

// -----------------------------------------------------------------------------
/* Setting for MCP2515
 *
 * Declare which pins you are using for communication.
 * Remember NOT to use them in your application!
 * It is a good idea to use bits from the port that carries MOSI, MISO, SCK.
 */
/****************************mcp2515_defs.h****************************/
// MCP2515 register address
#define SPI_RESET		0xC0
#define	SPI_READ		0x03
#define	SPI_READ_RX		0x90
#define	SPI_WRITE		0x02
#define	SPI_WRITE_TX	0x40
#define	SPI_RTS			0x80
#define SPI_READ_STATUS	0xA0
#define	SPI_RX_STATUS	0xB0
#define	SPI_BIT_MODIFY	0x05

#define RXF0SIDH	0x00
#define RXF0SIDL	0x01
#define RXF0EID8	0x02
#define RXF0EID0	0x03
#define RXF1SIDH	0x04
#define RXF1SIDL	0x05
#define RXF1EID8	0x06
#define RXF1EID0	0x07
#define RXF2SIDH	0x08
#define RXF2SIDL	0x09
#define RXF2EID8	0x0A
#define RXF2EID0	0x0B
#define BFPCTRL		0x0C
#define TXRTSCTRL	0x0D
#define CANSTAT		0x0E
#define CANCTRL		0x0F

#define RXF3SIDH	0x10
#define RXF3SIDL	0x11
#define RXF3EID8	0x12
#define RXF3EID0	0x13
#define RXF4SIDH	0x14
#define RXF4SIDL	0x15
#define RXF4EID8	0x16
#define RXF4EID0	0x17
#define RXF5SIDH	0x18
#define RXF5SIDL	0x19
#define RXF5EID8	0x1A
#define RXF5EID0	0x1B
#define TEC			0x1C
#define REC         0x1D

#define RXM0SIDH	0x20
#define RXM0SIDL	0x21
#define RXM0EID8	0x22
#define RXM0EID0	0x23
#define RXM1SIDH	0x24
#define RXM1SIDL	0x25
#define RXM1EID8	0x26
#define RXM1EID0	0x27
#define CNF3		0x28
#define CNF2		0x29
#define CNF1		0x2A
#define CANINTE		0x2B
#define CANINTF		0x2C
#define EFLG		0x2D

#define TXB0CTRL	0x30
#define TXB0SIDH	0x31
#define TXB0SIDL	0x32
#define TXB0EID8	0x33
#define TXB0EID0	0x34
#define TXB0DLC		0x35
#define TXB0D0		0x36
#define TXB0D1		0x37
#define TXB0D2		0x38
#define TXB0D3		0x39
#define TXB0D4		0x3A
#define TXB0D5		0x3B
#define TXB0D6		0x3C
#define TXB0D7		0x3D

#define TXB1CTRL	0x40
#define TXB1SIDH	0x41
#define TXB1SIDL	0x42
#define TXB1EID8	0x43
#define TXB1EID0	0x44
#define TXB1DLC		0x45
#define TXB1D0		0x46
#define TXB1D1		0x47
#define TXB1D2		0x48
#define TXB1D3		0x49
#define TXB1D4		0x4A
#define TXB1D5		0x4B
#define TXB1D6		0x4C
#define TXB1D7		0x4D

#define TXB2CTRL	0x50
#define TXB2SIDH	0x51
#define TXB2SIDL	0x52
#define TXB2EID8	0x53
#define TXB2EID0	0x54
#define TXB2DLC		0x55
#define TXB2D0		0x56
#define TXB2D1		0x57
#define TXB2D2		0x58
#define TXB2D3		0x59
#define TXB2D4		0x5A
#define TXB2D5		0x5B
#define TXB2D6		0x5C
#define TXB2D7		0x5D

#define RXB0CTRL	0x60
#define RXB0SIDH	0x61
#define RXB0SIDL	0x62
#define RXB0EID8	0x63
#define RXB0EID0	0x64
#define RXB0DLC		0x65
#define RXB0D0		0x66
#define RXB0D1		0x67
#define RXB0D2		0x68
#define RXB0D3		0x69
#define RXB0D4		0x6A
#define RXB0D5		0x6B
#define RXB0D6		0x6C
#define RXB0D7		0x6D

#define RXB1CTRL	0x70
#define RXB1SIDH	0x71
#define RXB1SIDL	0x72
#define RXB1EID8	0x73
#define RXB1EID0	0x74
#define RXB1DLC		0x75
#define RXB1D0		0x76
#define RXB1D1		0x77
#define RXB1D2		0x78
#define RXB1D3		0x79
#define RXB1D4		0x7A
#define RXB1D5		0x7B
#define RXB1D6		0x7C
#define RXB1D7		0x7D

/**     Bitdefinition of BFPCTRL */
#define B1BFS		5
#define B0BFS		4
#define B1BFE		3
#define B0BFE		2
#define B1BFM		1
#define B0BFM		0

/** 	Bitdefinition of TXRTSCTRL */
#define B2RTS		5
#define B1RTS		4
#define B0RTS		3
#define B2RTSM		2
#define B1RTSM		1
#define B0RTSM		0

/** 	Bitdefinition of CANSTAT */
#define OPMOD2		7
#define OPMOD1		6
#define OPMOD0		5
#define ICOD2		3
#define ICOD1		2
#define ICOD0		1

/** 	Bitdefinition of CANCTRL */
#define REQOP2		7
#define REQOP1		6
#define REQOP0		5
#define ABAT		4
#define CLKEN		2
#define CLKPRE1		1
#define CLKPRE0		0

/** 	Bitdefinition of CNF3 */
#define WAKFIL		6
#define PHSEG22		2
#define PHSEG21		1
#define PHSEG20		0

/** 	Bitdefinition of CNF2 */
#define BTLMODE		7
#define SAM			6
#define PHSEG12		5
#define PHSEG11		4
#define PHSEG10		3
#define PHSEG2		2
#define PHSEG1		1
#define PHSEG0		0

/** 	Bitdefinition of CNF1 */
#define SJW1		7
#define SJW0		6
#define BRP5		5
#define BRP4		4
#define BRP3		3
#define BRP2		2
#define BRP1		1
#define BRP0		0

/** 	Bitdefinition of CANINTE */
#define MERRE		7
#define WAKIE		6
#define ERRIE		5
#define TX2IE		4
#define TX1IE		3
#define TX0IE		2
#define RX1IE		1
#define RX0IE		0

/** 	Bitdefinition of CANINTF */
#define MERRF		7
#define WAKIF		6
#define ERRIF		5
#define TX2IF		4
#define TX1IF		3
#define TX0IF		2
#define RX1IF		1
#define RX0IF		0

/** 	Bitdefinition of EFLG */
#define RX1OVR		7
#define RX0OVR		6
#define TXB0		5
#define TXEP		4
#define RXEP		3
#define TXWAR		2
#define RXWAR		1
#define EWARN		0

/** 	Bitdefinition of TXBnCTRL (n = 0, 1, 2) */
#define ABTF		6
#define MLOA		5
#define TXERR		4
#define TXREQ		3
#define TXP1		1
#define TXP0		0

/** 	Bitdefinition of RXB0CTRL */
#define RXM1		6
#define RXM0		5
#define RXRTR		3
#define BUKT		2
#define BUKT1		1
#define FILHIT0		0

/** 	Bitdefinition of TXBnSIDL (n = 0, 1) */
#define	EXIDE		3

/**
 * 	Bitdefinition of RXB1CTRL
 * 	RXM1, RXM0, RXRTR and FILHIT0 already defined
 */
#define FILHIT2		2
#define FILHIT1		1

/** 	Bitdefinition of RXBnSIDL (n = 0, 1) */
#define	SRR			4
#define	IDE			3

/**
 * 	Bitdefinition of RXBnDLC (n = 0, 1)
 */
#define	RTR			6
#define	DLC3		3
#define	DLC2		2
#define	DLC1		1
#define DLC0		0

/****************************can.h****************************/
#define	ONLY_NON_RTR		2
#define	ONLY_RTR			3

#define	CAN_ALL_FILTER		0xff

#ifndef	SUPPORT_EXTENDED_CANID
	#define	SUPPORT_EXTENDED_CANID	1
#endif

#ifndef	SUPPORT_TIMESTAMPS
	#define	SUPPORT_TIMESTAMPS		0
#endif

#if SUPPORT_EXTENDED_CANID	
	#define MCP2515_FILTER_EXTENDED(id)	\
		(uint8_t)  ((uint32_t) (id) >> 21), \
		(uint8_t)((((uint32_t) (id) >> 13) & 0xe0) | (1<<3) | \
		(((uint32_t) (id) >> 16) & 0x3)), \
		(uint8_t)  ((uint32_t) (id) >> 8), \
		(uint8_t)  ((uint32_t) (id))
#endif
	
#define	MCP2515_FILTER(id) \
	(uint8_t)((uint32_t) id >> 3), \
	(uint8_t)((uint32_t) id << 5), \
	0, \
	0

#ifndef	CAN_FORCE_TX_ORDER
	#define	CAN_FORCE_TX_ORDER		0
#endif
	
// settings for buffered operation (only possible for the AT90CAN...)
#if !defined(CAN_TX_BUFFER_SIZE) || CAN_TX_BUFFER_SIZE == 0
	#define	CAN_TX_BUFFER_SIZE		0
	
	// forced order is only possible with buffered transmission
	#undef	CAN_FORCE_TX_ORDER
	#define	CAN_FORCE_TX_ORDER		0
#endif

#ifndef	CAN_RX_BUFFER_SIZE
	#define	CAN_RX_BUFFER_SIZE		0
#endif

#ifndef	CAN_INDICATE_TX_TRAFFIC_FUNCTION
	#define	CAN_INDICATE_TX_TRAFFIC_FUNCTION
#endif

#ifndef	CAN_INDICATE_RX_TRAFFIC_FUNCTION
	#define	CAN_INDICATE_RX_TRAFFIC_FUNCTION
#endif

#ifdef	CAN_DEBUG_LEVEL
	#define	DEBUG_INFO(format, ...)		printf_P(PSTR(format), ##__VA_ARGS__)
#else
	#define	DEBUG_INFO(format, ...)
#endif


/****************************mcp2515_private.h****************************/
#if defined(__AVR_ATmega16__) || defined(__AVR_ATmega32__) || defined(__AVR_ATmega644__)
	#define	P_MOSI	B,5
	#define	P_MISO	B,6
	#define	P_SCK	B,7 //TODO CS and INT pin
#elif defined(__AVR_ATmega8__)  || defined(__AVR_ATmega48__) || \
	  defined(__AVR_ATmega88__) || defined(__AVR_ATmega168__)
	#define	P_MOSI	B,3
	#define	P_MISO	B,4
	#define	P_SCK	B,5 //TODO CS and INT pin
#elif defined(__AVR_ATmega2560__)
	#define	P_MOSI	B,2 /// Add support
	#define	P_MISO	B,3 /// for ATmega2560
	#define	P_SCK	B,1 /// Arduino Mega Adk
	#define	MCP2515_CS	B,0 ///
	#define	MCP2515_INT	D,2 ///
#elif defined(__AVR_ATmega328P__)
	#define	P_MOSI	B,3 /// Add support
	#define	P_MISO	B,4 /// for ATmega328p
	#define	P_SCK	B,5 /// Arduino Uno
	#define	MCP2515_CS	B,2 ///
	#define	MCP2515_INT	D,2 ///
#elif defined(__AVR_ATmega32U4__)
	#define	P_MOSI	B,2 /// Add support
	#define	P_MISO	B,3 /// for ATmega32u4
	#define	P_SCK	B,1 /// CAN Interfacer
	#define	MCP2515_CS	B,0 /// Jumper J103 /// Jumper J102 B,5
	#define	MCP2515_INT	D,0 /// Jumper J105 /// Jumper J104 D,3
#elif defined(__AVR_ATmega128__)
	#define	P_MOSI	B,2
	#define	P_MISO	B,3
	#define	P_SCK	B,1 //TODO CS and INT pin
#elif defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
	#define	P_MOSI	B,0
	#define	P_MISO	B,1
	#define	P_SCK	B,2 //TODO CS and INT pin
	
	#define	USE_SOFTWARE_SPI		1
#else
	#error	choosen AVR-type is not supported yet!
#endif


// ----------------------------------------------------------------------------
// load some default values

#ifndef	MCP2515_CLKOUT_PRESCALER
	#define	MCP2515_CLKOUT_PRESCALER	0
#endif

#ifndef	MCP2515_INTERRUPTS
	#define	MCP2515_INTERRUPTS			(1<<RX1IE)|(1<<RX0IE)
#endif

#ifndef	MCP2515_CS
	#error	MCP2515_CS ist nicht definiert!
#endif

#if defined(MCP2515_RX0BF) && !defined(MCP2515_RX1BF)
	#warning	only MCP2515_RX0BF but not MCP2515_RX1BF defined!
#elif !defined(MCP2515_RX0BF) && defined(MCP2515_RX1BF)
	#warning	only MCP2515_RX1BF but not MCP2515_RX0BF defined!
#elif defined(MCP2515_RX0BF) && defined(MCP2515_RX1BF)
	#define	RXnBF_FUNKTION
#endif



/****************************utils.h****************************/
//May be useless, redefinition of true, false and null
#ifndef	TRUE
	#define	TRUE	(1==1)
#elif !TRUE
	#error	fehlerhafte Definition fuer TRUE
#endif

#ifndef FALSE
	#define	FALSE	(1!=1)
#elif FALSE
	#error	fehlerhafte Definition fuer FALSE
#endif

#ifndef NULL
	#define NULL ((void*)0)		
#endif

#define	DEGREE_TO_RAD(x)	((x * M_PI) / 180)

#define	LOW_BYTE(x)		((uint8_t) (x & 0xff))
#define	HIGH_BYTE(x)	((uint8_t) (x >> 8))
#define LOW_WORD(x)		((uint16_t) (x & 0xffff))
#define HIGH_WORD(x)    ((uint16_t) (x >> 16))

#if __AVR_LIBC_VERSION__ >= 10600 && !defined (__cplusplus)

	#include <util/atomic.h>
	
	#define	ENTER_CRITICAL_SECTION		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
	#define	LEAVE_CRITICAL_SECTION		}
	
	#define	IRQ_LOCK					ATOMIC_BLOCK(ATOMIC_RESTORESTATE)

#else

	#define	ENTER_CRITICAL_SECTION		do { unsigned char sreg_ = SREG; cli();
	#define	LEAVE_CRITICAL_SECTION		SREG = sreg_; } while (0);

#endif

#define	vu8(x)	(*(volatile  uint8_t*)&(x))
#define	vs8(x)	(*(volatile   int8_t*)&(x))
#define	vu16(x)	(*(volatile uint16_t*)&(x))
#define	vs16(x)	(*(volatile  int16_t*)&(x))
#define	vu32(x)	(*(volatile uint32_t*)&(x))
#define	vs32(x)	(*(volatile  int32_t*)&(x))

#define	PORT(x)			_port2(x)
#define	DDR(x)			_ddr2(x)
#define	PIN(x)			_pin2(x)
#define	REG(x)			_reg(x)
#define	PIN_NUM(x)		_pin_num(x)

#define	RESET(x)		RESET2(x)
#define	SET(x)			SET2(x)
#define	TOGGLE(x)		TOGGLE2(x)
#define	SET_OUTPUT(x)	SET_OUTPUT2(x)
#define	SET_INPUT(x)	SET_INPUT2(x)
#define	SET_PULLUP(x)	SET2(x)
#define	IS_SET(x)		IS_SET2(x)

#define	SET_INPUT_WITH_PULLUP(x)	SET_INPUT_WITH_PULLUP2(x)

#define	_port2(x)	PORT ## x
#define	_ddr2(x)	DDR ## x
#define	_pin2(x)	PIN ## x

#define	_reg(x,y)		x
#define	_pin_num(x,y)	y

#define	RESET2(x,y)		PORT(x) &= ~(1<<y)
#define	SET2(x,y)		PORT(x) |= (1<<y)
#define	TOGGLE2(x,y)	PORT(x) ^= (1<<y)

#define	SET_OUTPUT2(x,y)	DDR(x) |= (1<<y)
#define	SET_INPUT2(x,y)		DDR(x) &= ~(1<<y)
#define	SET_INPUT_WITH_PULLUP2(x,y)	SET_INPUT2(x,y);SET2(x,y)

#define	IS_SET2(x,y)	((PIN(x) & (1<<y)) != 0)

#define	_bit_is_set(pin, bit)	(pin & (1<<bit))
#define	_bit_is_clear(pin, bit)	(!(pin & (1<<bit)))

#define	STRING(x)	_STRING(x)
#define	_STRING(x)	#x


#if defined(DEBUG_LEVEL) && DEBUG_LEVEL
	#define	DEBUG_PRINT(s, ...)	do { static prog_char __s[] = (s); \
				printf_P(__s, ## __VA_ARGS__); } while (0)
#else
	#define	DEBUG_PRINT(s, ...)

#endif

#define MASK_01010101 (((uint32_t)(-1))/3)
#define MASK_00110011 (((uint32_t)(-1))/5)
#define MASK_00001111 (((uint32_t)(-1))/17)

#define	START_TIMED_BLOCK(time, gettime) \
	do { \
		static uint16_t last_time__; \
		uint16_t current_time__ = gettime; \
		if ((uint16_t) (current_time__ - last_time__) > time) { \
			last_time__ = current_time__;

#define END_TIMED_BLOCK \
		} \
	} while (0); 

// ----------------------------------------------------------------------------
#define	TO_DEG(x)		(x * 180.0 / M_PI)
#define	TO_RAD(x)		(x * M_PI / 180.0)

/**
 * Macro to supress "unused argument" warnings from the compiler
 */
#define	USE_IT(x)	(void) x


// ------------------------------TypeDef----------------------------------------
//Structure of CAN message
typedef struct
{
	#if SUPPORT_EXTENDED_CANID	
		uint32_t id;				//ID up to 29 bits
		struct {
			int rtr : 1;			//Remote-Transmit-Request-Frame flag
			int extended : 1;		//Extended ID flag
		} flags;
	#else
		uint16_t id;				//ID 11 bits
		struct {
			int rtr : 1;			//Remote-Transmit-Request-Frame flag
		} flags;
	#endif
	
	uint8_t length;				//Length
	uint8_t data[8];			//Data Field
	
	#if SUPPORT_TIMESTAMPS
		uint16_t timestamp;
	#endif
} can_t;

//Structure of filter
typedef struct
{
	#if	SUPPORT_EXTENDED_CANID
		uint32_t id;				//Extended ID 29 bits
		uint32_t mask;				//Mask
		struct {
			uint8_t rtr : 2;		//Remote Request Frame
			uint8_t extended : 2;	//Extended ID
		} flags;
	#else
		uint16_t id;				//Normal ID 11 bits
		uint16_t mask;				//Mask
			struct {
			uint8_t rtr : 2;		//Remote Request Frame
		} flags;
	#endif
} can_filter_t;

typedef struct {
	uint8_t rx;				
	uint8_t tx;				
} can_error_register_t;

typedef enum {
	LISTEN_ONLY_MODE,		//CAN Listen Mode
	LOOPBACK_MODE,			//CAN Loopback Mode
	NORMAL_MODE,			//CAN Normal Mode
	SLEEP_MODE				//CAN Sleep Mode
} can_mode_t;

typedef enum {
	BITRATE_10_KBPS	= 0,	
	BITRATE_20_KBPS	= 1,	
	BITRATE_50_KBPS	= 2,	
	BITRATE_100_KBPS = 3,	
	BITRATE_125_KBPS = 4,
	BITRATE_250_KBPS = 5,	
	BITRATE_500_KBPS = 6,	
	BITRATE_1_MBPS = 7,		
} can_bitrate_t;

typedef struct {
	can_t *buf;
	uint8_t size;
	
	uint8_t used;
	uint8_t head;
	uint8_t tail;
} can_buffer_t;

typedef struct {
	uint8_t b4;		// lsb
	uint8_t b3;
	uint8_t b2;
	uint8_t b1;		// msb
} long_to_byte_t;



// ----------------------------CAN Class---------------------------------------
// The comment lines contain the filenames from the original universal CAN library,
// see: http://www.kreatives-chaos.com/artikel/universelle-can-bibliothek

class CANclass
{
  public:
    CANclass();
	/**mcp2515_write_id**/
    static void spi_start(uint8_t); 
    static uint8_t spi_wait(void);
	void mcp2515_write_id(const uint32_t *, uint8_t);
	void mcp2515_write_id(const uint16_t *);
	/**mcp2515_static_filter**/
	void mcp2515_static_filter(const prog_uint8_t *);
	/**mcp2515_sleep**/
	void mcp2515_sleep(void);
	void mcp2515_wakeup(void);
	/**mcp2515_set_mode**/
	void mcp2515_set_mode(can_mode_t);
	/**mcp2515_set_dyn_filter**/
	bool mcp2515_set_filter(uint8_t, const can_filter_t *);
	/**mcp2515_send_message**/
	uint8_t mcp2515_send_message(const can_t *);
	/**mcp2515_regdump**/
	void mcp2515_regdump(void);
	/**mcp2515_read_id**/
	uint8_t mcp2515_read_id(uint32_t *);
	uint8_t mcp2515_read_id(uint16_t *);
	uint8_t mcp2515_read_id_complete(uint16_t *,uint8_t*,uint8_t*);
	uint8_t mcp2515_read_id_complete(uint32_t *,uint8_t*,uint8_t*);
	/**mcp2515_get_message**/
	uint8_t mcp2515_get_message(can_t *);
	/**mcg2512_get_data**/
	void mcp2515_get_data(can_t*, uint16_t,uint8_t);
	/**mcp2515_get_dyn_filter**/
	uint8_t mcp2515_get_filter(uint8_t, can_filter_t *);
	/**mcp2515_error_register**/
	can_error_register_t mcp2515_read_error_register(void);
	/**mcp2515_buffer**/
	bool mcp2515_check_message(void);
	bool mcp2515_check_free_buffer(void);
	/**mcp2515**/
	void mcp2515_write_register(uint8_t, uint8_t);
	uint8_t mcp2515_read_register(uint8_t);
	void mcp2515_bit_modify(uint8_t, uint8_t, uint8_t);
	uint8_t mcp2515_read_status(uint8_t);
	bool mcp2515_init(uint8_t);
	/**can_buffer**/
	void can_buffer_init(can_buffer_t *, uint8_t, can_t *);
	bool can_buffer_empty(can_buffer_t *);
	bool can_buffer_full(can_buffer_t *);
	can_t *can_buffer_get_enqueue_ptr(can_buffer_t *);
	void can_buffer_enqueue(can_buffer_t *);
	can_t *can_buffer_get_dequeue_ptr(can_buffer_t *);
	void can_buffer_dequeue(can_buffer_t *);
	/**spi**/
	void mcp2515_spi_init(void);
	uint8_t spi_putc(uint8_t );
	/**inline functions**/
	uint8_t read_and_replace_atomar(volatile uint8_t *, uint8_t);
	uint8_t swap (uint8_t);
	uint8_t bit_count8(uint8_t);
	uint8_t bit_count32(uint32_t);
	void mcp2515_change_operation_mode(uint8_t);
	/**Misc**/
	void resetFiltersAndMasks(void);
	void setMaskOrFilter(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
	void activateInterrupt(void);
	void clearFlags(void);
	/**USB**/
	bool usbcan_decode_message(char *str, uint8_t length);
	char usbcan_decode_command(char *str, uint8_t length);
	uint8_t char_to_byte(char *s);
	uint8_t hex_to_byte(char *s);
	/**Debugging*/
	void printString2Can(char []);
  private:
};


#endif // CAN_H


extern CANclass CAN;

