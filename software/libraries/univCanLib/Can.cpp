#include "Can.h"

CAN::CAN(){

}


/****************************BEG spi**************************/
#ifdef	SPI_PRESCALER
	#if (SPI_PRESCALER == 2) || (SPI_PRESCALER == 8) || (SPI_PRESCALER == 32) || (SPI_PRESCALER == 64)
		#define	R_SPSR	(1<<SPI2X)
		#define SPI_PRESCALER_ 	(SPI_PRESCALER * 2)
	#else
		#define	R_SPSR	0
		#define	SPI_PRESCALER_	SPI_PRESCALER
	#endif

	#define	SPI_CLOCK_RATE_BIT0		(1<<SPR0)
	#define	SPI_CLOCK_RATE_BIT1		(1<<SPR1)

	#if (SPI_PRESCALER_ == 4)
		#define	R_SPCR	0
	#elif (SPI_PRESCALER_ == 16)
		#define	R_SPCR	SPI_CLOCK_RATE_BIT0
	#elif (SPI_PRESCALER_ == 64)
		#define	R_SPCR	SPI_CLOCK_RATE_BIT1
	#elif (SPI_PRESCALER_ == 128)
		#define	R_SPCR	SPI_CLOCK_RATE_BIT1 | SPI_CLOCK_RATE_BIT0
	#else
		#error	 SPI_PRESCALER must be one of the values of 2^n with n = 1..7!
	#endif
#else
	#error	SPI_PRESCALER not defined!
#endif

// ----------------------------------------------------------------------------
#ifdef USE_SOFTWARE_SPI
	static uint8_t usi_interface_spi_temp;

	void CAN::spi_start(uint8_t data) {
		usi_interface_spi_temp = spi_putc(data);
	}

	uint8_t CAN::spi_wait(void) {
		return usi_interface_spi_temp;
	}
#else
	void CAN::spi_start(uint8_t data) {
		SPDR = data;
	}

	uint8_t CAN::spi_wait(void) {
		// Wait until the previous values are ​​written
		while(!(SPSR & (1<<SPIF)))
			;

		return SPDR;
	}
#endif
// ----------------------------------------------------------------------------
void CAN::mcp2515_spi_init(void)
{
	#ifndef USE_SOFTWARE_SPI
		//Activate SPI Master Interfaces
		SPCR = (1<<SPE)|(1<<MSTR) | R_SPCR;
		SPSR = R_SPSR;
	#endif
}

// ----------------------------------------------------------------------------
uint8_t CAN::spi_putc(uint8_t data)
{
	
	// put byte in send-buffer
	SPDR = data;

	// wait until byte was send
	while( !( SPSR & (1<<SPIF) ) )
		;

	return SPDR;

}
/****************************END spi**************************/


/*******************************BEG mcp2515_write_id******************************/
// ----------------------------------------------------------------------------
#if SUPPORT_EXTENDED_CANID
	void CAN::mcp2515_write_id(const uint32_t *id, uint8_t extended)
	{
		uint8_t tmp;

		if (extended) {
			spi_start(*((uint16_t *) id + 1) >> 5);
	
			// Calculate next value
			tmp  = (*((uint8_t *) id + 2) << 3) & 0xe0;
			tmp |= (1 << IDE);
			tmp |= (*((uint8_t *) id + 2)) & 0x03;
	
			// Wait until the previous values are ​​written
			spi_wait();
	
			// Write remaining values
			spi_putc(tmp);
			spi_putc(*((uint8_t *) id + 1));
			spi_putc(*((uint8_t *) id));
		}
		else {
			spi_start(*((uint16_t *) id) >> 3);
	
			// Calculate next values
			tmp = *((uint8_t *) id) << 5;
			spi_wait();
	
			spi_putc(tmp);
			spi_putc(0);
			spi_putc(0);
		}
	}
#else
	void CAN::mcp2515_write_id(const uint16_t *id)
	{
		uint8_t tmp;

		spi_start(*id >> 3);
		tmp = *((uint8_t *) id) << 5;
		spi_wait();

		spi_putc(tmp);
		spi_putc(0);
		spi_putc(0);
	}
#endif	// USE_EXTENDED_CANID
/*******************************END mcp2515_write_id******************************/

/****************************mcp2515_static_filter function to set a filter****************************/
// ----------------------------------------------------------------------------
// Filter setting
void CAN::mcp2515_static_filter(const prog_uint8_t *filter)
{
	// Switch into configuration mode because filters can be change only in this mode 
	mcp2515_bit_modify(CANCTRL, 0xe0, (1<<REQOP2));
	while ((mcp2515_read_register(CANSTAT) & 0xe0) != (1<<REQOP2))
		;

	mcp2515_write_register(RXB0CTRL, (1<<BUKT));
	mcp2515_write_register(RXB1CTRL, 0);
	
	uint8_t i, j;
	for (i = 0; i < 0x30; i += 0x10)
	{
		RESET(MCP2515_CS);
		spi_putc(SPI_WRITE);
		spi_putc(i);
	
		for (j = 0; j < 12; j++) 
		{
			if (i == 0x20 && j >= 0x08)
				break;
		
			spi_putc(pgm_read_byte(filter++));
		}
		SET(MCP2515_CS);
	}

	mcp2515_bit_modify(CANCTRL, 0xe0, 0);
}

/****************************END mcp2515_static_filter****************************/

/********************************mcp2515_sleep and wakeup function********************************/
// ----------------------------------------------------------------------------
void CAN::mcp2515_sleep(void)
{
	// put also the 2551 in standby mode
	// for this, connect RX1BF to the RS pin of the 2551
	mcp2515_bit_modify(BFPCTRL, (1<<B1BFS), (1<<B1BFS));

	// put the 2515 in sleep mode
	mcp2515_set_mode(SLEEP_MODE);

	// enable generating an interrupt for wakeup when activity on bus
	mcp2515_bit_modify(CANINTE, (1<<WAKIE), (1<<WAKIE));
}

// ----------------------------------------------------------------------------
void CAN::mcp2515_wakeup(void)
{
	// reset int enable and cancel the interrupt flag
	mcp2515_bit_modify(CANINTE, (1<<WAKIE), 0);
	mcp2515_bit_modify(CANINTF, (1<<WAKIF), 0);

	// re-enable the 2551
	mcp2515_bit_modify(BFPCTRL, (1<<B1BFS), 0);

	// when we get up of sleep, we are in listen mode, return into normal mode
	mcp2515_set_mode(NORMAL_MODE);
}

/********************************END mcp2515_sleep and wakeup function********************************/


/*******************************mcp2515_set_mode function to set CAN mode******************************/
// ----------------------------------------------------------------------------
void CAN::mcp2515_set_mode(can_mode_t mode)
{
	uint8_t reg = 0;

	if (mode == LISTEN_ONLY_MODE) {
		reg = (1<<REQOP1)|(1<<REQOP0);
	}
	else if (mode == LOOPBACK_MODE) {
		reg = (1<<REQOP1);
	}
	else if (mode == SLEEP_MODE) {
		reg = (1<<REQOP0);
	}
	
	// set the new mode
	mcp2515_bit_modify(CANCTRL, (1<<REQOP2)|(1<<REQOP1)|(1<<REQOP0), reg);
	while ((mcp2515_read_register(CANSTAT) & 0xe0) != reg) {
		// wait for the new mode to become active
	}
}

/*******************************END mcp2515_set_mode******************************/


/****************************mcp2515_set_dyn_filter function to set a dynamic filter***************************/
// ----------------------------------------------------------------------------
// set a filter
bool CAN::mcp2515_set_filter(uint8_t number, const can_filter_t *filter)
{
	uint8_t mask_address = 0;
	uint8_t mode = mcp2515_read_register(CANSTAT);

	if (number > 5)
		return false;

	// change to configuration mode
	mcp2515_change_operation_mode( (1<<REQOP2) );

	// set filter mask
	if (number == 0)
	{
		mask_address = RXM0SIDH;
	
		#if SUPPORT_EXTENDED_CANID
			if (filter->flags.extended == 0x3) {
				// only extended identifier
				mcp2515_write_register(RXB0CTRL, (1<<RXM1));
			}
			else if (filter->flags.extended == 0x2) {
				// only standard identifier
				mcp2515_write_register(RXB0CTRL, (1<<RXM0));
			}
			else {
				// receive all messages
				mcp2515_write_register(RXB0CTRL, 0);
			}
		#else
			// Buffer 0: Receive all messages with standard identifier
			mcp2515_write_register(RXB0CTRL, (1<<RXM0));
		#endif
	}
	else if (number == 2)
	{
		mask_address = RXM1SIDH;
	
		#if SUPPORT_EXTENDED_CANID
			if (filter->flags.extended == 0x3) {
				// only extended identifier
				mcp2515_write_register(RXB1CTRL, (1<<RXM1));
			}
			else if (filter->flags.extended == 0x2) {
				// only standard identifier
				mcp2515_write_register(RXB1CTRL, (1<<RXM0));
			}
			else {
				mcp2515_write_register(RXB1CTRL, 0);
			}
		#else
			// Buffer 1: Receive all messages with standard identifier
			mcp2515_write_register(RXB1CTRL, (1<<RXM0));
		#endif
	}

	if (mask_address)
	{
		RESET(MCP2515_CS);
		spi_putc(SPI_WRITE);
		spi_putc(mask_address);
		#if SUPPORT_EXTENDED_CANID
			mcp2515_write_id(&filter->mask, (filter->flags.extended == 0x2) ? 0 : 1);
		#else
			mcp2515_write_id(&filter->mask);
		#endif
		SET(MCP2515_CS);
	
		_delay_us(1);
	}

	// write filter
	uint8_t filter_address;
	if (number >= 3) {
		number -= 3;
		filter_address = RXF3SIDH;
	}
	else {
		filter_address = RXF0SIDH;
	}

	RESET(MCP2515_CS);
	spi_putc(SPI_WRITE);
	spi_putc(filter_address | (number * 4));
	#if SUPPORT_EXTENDED_CANID
		mcp2515_write_id(&filter->id, (filter->flags.extended == 0x2) ? 0 : 1);
	#else
		mcp2515_write_id(&filter->id);
	#endif
	SET(MCP2515_CS);

	_delay_us(1);

	// restore previous mode
	mcp2515_change_operation_mode( mode );

	return true;
}

/****************************END mcp2515_set_dyn_filter***************************/


/****************************mcp2515_send_message function to send a CAN msg***************************/
uint8_t CAN::mcp2515_send_message(const can_t *msg)
{
	// Read status of the MCP2515
	uint8_t status = mcp2515_read_status(SPI_READ_STATUS);

	/* Statusbyte:
	 *
	 * Bit	Function
	 *  2	TXB0CNTRL.TXREQ
	 *  4	TXB1CNTRL.TXREQ
	 *  6	TXB2CNTRL.TXREQ
	 */
	uint8_t address;
	if (_bit_is_clear(status, 2)) {
		address = 0x00;
	}
	else if (_bit_is_clear(status, 4)) {
		address = 0x02;
	} 
	else if (_bit_is_clear(status, 6)) {
		address = 0x04;
	}
	else {
		// All buffers are occupied,
		// Message can not be sent
		return 0;
	}

	RESET(MCP2515_CS);
	spi_putc(SPI_WRITE_TX | address);
	#if SUPPORT_EXTENDED_CANID
		mcp2515_write_id(&msg->id, msg->flags.extended);
	#else
		mcp2515_write_id(&msg->id);
	#endif
	uint8_t length = msg->length & 0x0f;

	// If the message contains a "Remote Transmit Request"?
	if (msg->flags.rtr)
	{
		// An RTR msg indeed has a length,
		// But contains no data
	
		// Setting the message length + RTR
		spi_putc((1<<RTR) | length);
	}
	else
	{
		// Setting the message length
		spi_putc(length);
	
		// Data
		for (uint8_t i=0;i<length;i++) {
			spi_putc(msg->data[i]);
		}
	}
	SET(MCP2515_CS);

	_delay_us(1);

	//Send CAN message
	//the last three bits in the RTS command specify which
	//Buffer to be sent.
	RESET(MCP2515_CS);
	address = (address == 0) ? 1 : address;
	spi_putc(SPI_RTS | address);
	SET(MCP2515_CS);

	CAN_INDICATE_TX_TRAFFIC_FUNCTION;

	return address;
}

/****************************END mcp2515_send_message***************************/


/****************************BEG mcp2515_regdump***************************/
void CAN::mcp2515_regdump(void)
{
	uint8_t mode = mcp2515_read_register( CANSTAT );

	// change to configuration mode
	mcp2515_change_operation_mode( (1<<REQOP2) );

	printf_P("MCP2515 Regdump:\n");
	uint8_t i;
	for (i=0; i < 16; i++) {
		printf_P("%3i: %02x   ", i, mcp2515_read_register(i));
		printf_P("%3i: %02x   ", i+16*1, mcp2515_read_register(i+16*1));
		printf_P("%3i: %02x   ", i+16*2, mcp2515_read_register(i+16*2));
		printf_P("%3i: %02x   ", i+16*3, mcp2515_read_register(i+16*3));
		printf_P("%3i: %02x   ", i+16*4, mcp2515_read_register(i+16*4));
		printf_P("%3i: %02x   ", i+16*5, mcp2515_read_register(i+16*5));
		printf_P("%3i: %02x   ", i+16*6, mcp2515_read_register(i+16*6));
		printf_P("%3i: %02x\n", i+16*7, mcp2515_read_register(i+16*7));
	}

	mcp2515_change_operation_mode( mode );
}

/****************************END mcp2515_regdump***************************/

/****************************mcp2515_read_id function to read an ID***************************/
#if	SUPPORT_EXTENDED_CANID
uint8_t CAN::mcp2515_read_id(uint32_t *id)
{
	uint8_t first;
	uint8_t tmp;

	first = spi_putc(0xff);
	tmp   = spi_putc(0xff);

	if (tmp & (1 << IDE)) {
		spi_start(0xff);
	
		*((uint16_t *) id + 1)  = (uint16_t) first << 5;
		*((uint8_t *)  id + 1)  = spi_wait();
		spi_start(0xff);
	
		*((uint8_t *)  id + 2) |= (tmp >> 3) & 0x1C;
		*((uint8_t *)  id + 2) |=  tmp & 0x03;
	
		*((uint8_t *)  id)      = spi_wait();
	
		return TRUE;
	}
	else {
		spi_start(0xff);
	
		*((uint8_t *)  id + 3) = 0;
		*((uint8_t *)  id + 2) = 0;
	
		*((uint16_t *) id) = (uint16_t) first << 3;
	
		spi_wait();
		spi_start(0xff);
	
		*((uint8_t *) id) |= tmp >> 5;
	
		spi_wait();
	
		return FALSE;
	}
}

#else

uint8_t CAN::mcp2515_read_id(uint16_t *id)
{
	uint8_t first;
	uint8_t tmp;

	first = spi_putc(0xff);
	tmp   = spi_putc(0xff);

	if (tmp & (1 << IDE)) {
		spi_putc(0xff);
		spi_putc(0xff);
	
		return 1;			// extended-frame
	}
	else {
		spi_start(0xff);
	
		*id = (uint16_t) first << 3;
	
		spi_wait();
		spi_start(0xff);
	
		*((uint8_t *) id) |= tmp >> 5;
	
		spi_wait();
	
		if (tmp & (1 << SRR))
			return 2;		// RTR-frame
		else
			return 0;		// normal-frame
	}
}
#endif	// SUPPORT_EXTENDED_CANID

/****************************END mcp2515_read_id***************************/


/****************************mcp2515_get_message function to get an incoming msg***************************/
uint8_t CAN::mcp2515_get_message(can_t *msg)
{
	uint8_t addr;

	#ifdef	RXnBF_FUNKTION
		if (!IS_SET(MCP2515_RX0BF))
			addr = SPI_READ_RX;
		else if (!IS_SET(MCP2515_RX1BF))
			addr = SPI_READ_RX | 0x04;
		else
			return 0;
	#else
		// read status
		uint8_t status = mcp2515_read_status(SPI_RX_STATUS);
	
		if (_bit_is_set(status,6)) {
			// message in buffer 0
			addr = SPI_READ_RX;
		}
		else if (_bit_is_set(status,7)) {
			// message in buffer 1
			addr = SPI_READ_RX | 0x04;
		}
		else {
			// Error: no message available
			return 0;
		}
	#endif

	RESET(MCP2515_CS);
	spi_putc(addr);

	// CAN ID reading and checking
	uint8_t tmp = mcp2515_read_id(&msg->id);
	#if SUPPORT_EXTENDED_CANID
		msg->flags.extended = tmp & 0x01;
	#else
		if (tmp & 0x01) {
			// Discard messages with extended ID
			SET(MCP2515_CS);
			#ifdef	RXnBF_FUNKTION
			if (!IS_SET(MCP2515_RX0BF))
			#else
			if (_bit_is_set(status, 6))
			#endif
				mcp2515_bit_modify(CANINTF, (1<<RX0IF), 0);
			else
				mcp2515_bit_modify(CANINTF, (1<<RX1IF), 0);
		
			return 0;
		}
	#endif

	// read DLC
	uint8_t length = spi_putc(0xff);
	#ifdef RXnBF_FUNKTION
		if (!(tmp & 0x01))
			msg->flags.rtr = (tmp & 0x02) ? 1 : 0;
		else
			msg->flags.rtr = (length & (1<<RTR)) ? 1 : 0;
	#else
		msg->flags.rtr = (_bit_is_set(status, 3)) ? 1 : 0;
	#endif

	length &= 0x0f;
	msg->length = length;
	// read data
	for (uint8_t i=0;i<length;i++) {
		msg->data[i] = spi_putc(0xff);
	}
	SET(MCP2515_CS);

	// clear interrupt flag
	#ifdef RXnBF_FUNKTION
	if (!IS_SET(MCP2515_RX0BF))
	#else
	if (_bit_is_set(status, 6))
	#endif
		mcp2515_bit_modify(CANINTF, (1<<RX0IF), 0);
	else
		mcp2515_bit_modify(CANINTF, (1<<RX1IF), 0);

	CAN_INDICATE_RX_TRAFFIC_FUNCTION;

	#ifdef RXnBF_FUNKTION
		return 1;
	#else
		return (status & 0x07) + 1;
	#endif
}

/****************************END mcp2515_get_message***************************/


/****************************mcp2515_get_dyn_filter function to get a dynamic filter***************************/
uint8_t CAN::mcp2515_get_filter(uint8_t number, can_filter_t *filter)
{
	uint8_t mask_address;
	uint8_t filter_address;
	uint8_t temp;
	uint8_t mode = mcp2515_read_register(CANSTAT);

	if (number > 5)
		return 0;

	// change to configuration mode
	mcp2515_change_operation_mode( (1<<REQOP2) );

	if (number <= 1)
	{
		mask_address = RXM0SIDH;
		temp = mcp2515_read_register(RXB0CTRL);
	}
	else
	{
		mask_address = RXM1SIDH;
		temp = mcp2515_read_register(RXB1CTRL);
	}

	temp &= (1<<RXM1)|(1<<RXM0);

	if (temp == 0) {
		// filter and masks are disabled
		#if SUPPORT_EXTENDED_CANID
		filter->flags.extended = 0;
		#endif
		filter->flags.rtr = 0;
		filter->mask = 0;
		filter->id = 0;
	
		return 1;
	}

	#if SUPPORT_EXTENDED_CANID
	// transform bits so that they match the format from can.h
	temp >>= 5;
	temp = ~temp;
	if (temp & 1) temp = 0x3;

	filter->flags.extended = temp;
	#endif

	// read mask
	RESET(MCP2515_CS);
	spi_putc(SPI_READ);
	spi_putc(mask_address);
	mcp2515_read_id(&filter->mask);
	SET(MCP2515_CS);

	if (number <= 2)
	{
		filter_address = RXF0SIDH + number * 4;
	}
	else
	{
		filter_address = RXF3SIDH + (number - 3) * 4;
	}

	// read filter
	RESET(MCP2515_CS);
	spi_putc(SPI_READ);
	spi_putc(filter_address);
	mcp2515_read_id(&filter->id);
	SET(MCP2515_CS);

	// restore previous mode
	mcp2515_change_operation_mode( mode );

	return 1;
}


/****************************END mcp2515_get_dyn_filter***************************/

/****************************mcp2515_error_register**************************/
can_error_register_t CAN::mcp2515_read_error_register(void)
{
	can_error_register_t error;

	error.tx = mcp2515_read_register(TEC);
	error.rx = mcp2515_read_register(REC);

	return error;
}


/****************************END mcp2515_error_register**************************/

/****************************mcp2515_buffer**************************/
// ----------------------------------------------------------------------------
// check if there are any new messages waiting
bool CAN::mcp2515_check_message(void)
{
	#if defined(MCP2515_INT)
		return ((!IS_SET(MCP2515_INT)) ? true : false);
	#else
		#ifdef RXnBF_FUNKTION
			if (!IS_SET(MCP2515_RX0BF) || !IS_SET(MCP2515_RX1BF))
				return true;
			else
				return false;
		#else
			return ((mcp2515_read_status(SPI_RX_STATUS) & 0xC0) ? true : false);
		#endif
	#endif
}
// ----------------------------------------------------------------------------
// check if there is a free buffer to send messages
bool CAN::mcp2515_check_free_buffer(void)
{
	uint8_t status = mcp2515_read_status(SPI_READ_STATUS);

	if ((status & 0x54) == 0x54)
		return false;		// all buffers used
	else
		return true;
}

/****************************END mcp2515_buffer**************************/


/****************************BEG mcp2515**************************/

#ifndef	MCP2515_CLKOUT_PRESCALER
	#error	MCP2515_CLKOUT_PRESCALER not defined!
#elif MCP2515_CLKOUT_PRESCALER == 0
	#define	CLKOUT_PRESCALER_	0x0
#elif MCP2515_CLKOUT_PRESCALER == 1
	#define	CLKOUT_PRESCALER_	0x4
#elif MCP2515_CLKOUT_PRESCALER == 2
	#define	CLKOUT_PRESCALER_	0x5
#elif MCP2515_CLKOUT_PRESCALER == 4
	#define	CLKOUT_PRESCALER_	0x6
#elif MCP2515_CLKOUT_PRESCALER == 8
	#define	CLKOUT_PRESCALER_	0x7
#else
	#error	invaild value of MCP2515_CLKOUT_PRESCALER
#endif

// -------------------------------------------------------------------------
void CAN::mcp2515_write_register( uint8_t adress, uint8_t data )
{
	RESET(MCP2515_CS);

	spi_putc(SPI_WRITE);
	spi_putc(adress);
	spi_putc(data);

	SET(MCP2515_CS);
}

// -------------------------------------------------------------------------
uint8_t CAN::mcp2515_read_register(uint8_t adress)
{
	uint8_t data;

	RESET(MCP2515_CS);

	spi_putc(SPI_READ);
	spi_putc(adress);

	data = spi_putc(0xff);	

	SET(MCP2515_CS);

	return data;
}

// -------------------------------------------------------------------------
void CAN::mcp2515_bit_modify(uint8_t adress, uint8_t mask, uint8_t data)
{
	RESET(MCP2515_CS);

	spi_putc(SPI_BIT_MODIFY);
	spi_putc(adress);
	spi_putc(mask);
	spi_putc(data);

	SET(MCP2515_CS);
}

// ----------------------------------------------------------------------------
uint8_t CAN::mcp2515_read_status(uint8_t type)
{
	uint8_t data;

	RESET(MCP2515_CS);

	spi_putc(type);
	data = spi_putc(0xff);
	spi_putc(0xff);
	SET(MCP2515_CS);

	return data;
}

// -------------------------------------------------------------------------
// <!> const PROGMEM added for newer avr-gcc versions
const prog_uint8_t PROGMEM _mcp2515_cnf[8][3] = {
	// 10 kbps
	{	0x05, //CNF3
		0xB8, //CNF2
		0x31  //CNF1
	},
	// 20 kbps
	{	0x05,
		0xB8,
		0x18
	},
	// 50 kbps
	{	0x05,
		0xB8,
		0x09
	},
	// 100 kbps
	{	0x05,
		0xB8,
		0x04
	},
	// 125 kbps
	{	0x05,					
		0xB8,
		0x03
	},
	// 250 kbps
	{	0x05,
		0xB8,
		0x01
	},
	// 500 kbps
	{	0x05,
		0xB8,
		0x00
	},
	// 1 Mbps
	{	
		0x02,
		0x90,
		0x80
	}
};

// -------------------------------------------------------------------------
bool CAN::mcp2515_init(uint8_t bitrate)
{
	if (bitrate >= 8)
		return false;

	SET(MCP2515_CS);
	SET_OUTPUT(MCP2515_CS);

	// Enable pins for the SPI interface
	RESET(P_SCK);
	RESET(P_MOSI);
	RESET(P_MISO);

	SET_OUTPUT(P_SCK);
	SET_OUTPUT(P_MOSI);
	SET_INPUT(P_MISO);

	// Set SPI setting
	mcp2515_spi_init();

	// MCP2515 reset by software reset 
	// then he is automatically in the configuration mode
	RESET(MCP2515_CS);
	spi_putc(SPI_RESET);

	_delay_ms(1);

	SET(MCP2515_CS);

	// Wait a bit until the MCP2515 has restarted
	_delay_ms(10);

	// CNF1 .. 3 register load (bit timing)
	RESET(MCP2515_CS);
	spi_putc(SPI_WRITE);
	spi_putc(CNF3);
	for (uint8_t i=0; i<3 ;i++ ) {
		spi_putc(pgm_read_byte(&_mcp2515_cnf[bitrate][i]));
	}
	// Enable / Disable the interrupts
	spi_putc(MCP2515_INTERRUPTS);
	SET(MCP2515_CS);

	// TXnRTS bits switch as inputs
	mcp2515_write_register(TXRTSCTRL, 0);

	#if defined(MCP2515_INT)
		SET_INPUT(MCP2515_INT);
		SET(MCP2515_INT);
	#endif

	#ifdef RXnBF_FUNKTION
		SET_INPUT(MCP2515_RX0BF);
		SET_INPUT(MCP2515_RX1BF);
	
		SET(MCP2515_RX0BF);
		SET(MCP2515_RX1BF);
	
		// Enable pin functions for RX0BF and RX1BF
		mcp2515_write_register(BFPCTRL, (1<<B0BFE)|(1<<B1BFE)|(1<<B0BFM)|(1<<B1BFM));
	#else
		#ifdef MCP2515_TRANSCEIVER_SLEEP
			// activate the pin RX1BF as GPIO which is connected 
			// to RS of MCP2551 and set it to 0
			mcp2515_write_register(BFPCTRL, (1<<B1BFE));
		#else
			// Disabling RXnBF Pins (High Impedance State)
			mcp2515_write_register(BFPCTRL, 0);
		#endif
	#endif

	//Testing whether that can be accessed on the registers described
	bool error = false;

	if (mcp2515_read_register(CNF2) != pgm_read_byte(&_mcp2515_cnf[bitrate][1])) {
		error = true;
	}

	// Device back into normal mode enable 
	// and enable / disable the Clkout pins
	mcp2515_write_register(CANCTRL, CLKOUT_PRESCALER_);

	if (error) {
		return false;
	}
	else
	{
		while ((mcp2515_read_register(CANSTAT) & 0xe0) != 0) {
			// Wait until mode switch to Normal
		}
	
		return true;
	}
}
/****************************END mcp2515**************************/


/****************************BEG can_buffer**************************/
#if CAN_RX_BUFFER_SIZE > 0 || CAN_TX_BUFFER_SIZE > 0

	// -----------------------------------------------------------------------------
	void CAN::can_buffer_init(can_buffer_t *buf, uint8_t size, can_t *list)
	{
		ENTER_CRITICAL_SECTION;
		buf->size = size;
		buf->buf = list;
	
		buf->head = 0;
		buf->tail = 0;
		buf->used = 0;
		LEAVE_CRITICAL_SECTION;
	}

	// -----------------------------------------------------------------------------
	bool CAN::can_buffer_empty(can_buffer_t *buf)
	{
		uint8_t used;
	
		ENTER_CRITICAL_SECTION;
		used = buf->used;
		LEAVE_CRITICAL_SECTION;
	
		if (used == 0)
			return true;
		else
			return false;
	}

	// -----------------------------------------------------------------------------
	bool CAN::can_buffer_full(can_buffer_t *buf)
	{
		uint8_t used;
		uint8_t size;
	
		ENTER_CRITICAL_SECTION;
		used = buf->used;
		size = buf->size;
		LEAVE_CRITICAL_SECTION;
	
		if (used >= size)
			return true;
		else
			return false;
	}

	// -----------------------------------------------------------------------------
	can_t CAN::*can_buffer_get_enqueue_ptr(can_buffer_t *buf)
	{
		if (can_buffer_full( buf ))
			return NULL;
	
		return &buf->buf[buf->head];
	}

	// -----------------------------------------------------------------------------
	void CAN::can_buffer_enqueue(can_buffer_t *buf)
	{
		ENTER_CRITICAL_SECTION;
		buf->used ++;
		if (++buf->head >= buf->size)
			buf->head = 0;
		LEAVE_CRITICAL_SECTION;
	}

	// -----------------------------------------------------------------------------
	can_t CAN::*can_buffer_get_dequeue_ptr(can_buffer_t *buf)
	{
		if (can_buffer_empty( buf ))
			return NULL;
	
		return &buf->buf[buf->tail];
	}

	// -----------------------------------------------------------------------------
	void CAN::can_buffer_dequeue(can_buffer_t *buf)
	{
		ENTER_CRITICAL_SECTION;
		buf->used --;
		if (++buf->tail >= buf->size)
			buf->tail = 0;
		LEAVE_CRITICAL_SECTION;
	}

#endif
/****************************END can_buffer**************************/

/****************************inline header functions**************************/
uint8_t CAN::read_and_replace_atomar(volatile uint8_t *data, uint8_t new_data)
{
	uint8_t old_data;
	
	ENTER_CRITICAL_SECTION
	
	// Exchange data
	old_data = *data;
	*data = new_data;
	
	LEAVE_CRITICAL_SECTION
	
	return old_data;
}

uint8_t CAN::swap (uint8_t x)
{
	if (__builtin_constant_p(x))
		x = (x << 4) | (x >> 4);
	else
		asm volatile ("swap %0" : "=r" (x) : "0" (x));
	
	return x;
}

uint8_t CAN::bit_count8(uint8_t n)
{
	n = ((n >> 1) & 0x55) + (n & 0x55);
	n = ((n >> 2) & 0x33) + (n & 0x33);
	n = ((n >> 4) + n) & 0xf;
	
	return n;
}


uint8_t CAN::bit_count32(uint32_t n)
{
	n = (n & MASK_01010101) + ((n >> 1) & MASK_01010101);
	n = (n & MASK_00110011) + ((n >> 2) & MASK_00110011);
	n = (n & MASK_00001111) + ((n >> 4) & MASK_00001111);
	
	return n % 255 ;
}

//Switch CAN mode [LISTEN_ONLY_MODE, LOOPBACK_MODE, NORMAL_MODE, SLEEP_MODE]
void CAN::mcp2515_change_operation_mode(uint8_t mode)
{
	mcp2515_bit_modify(CANCTRL, 0xe0, mode);
	while ((mcp2515_read_register(CANSTAT) & 0xe0) != (mode & 0xe0))
		;
}

void CAN::resetFiltersAndMasks(void) {
	//disable first buffer
	setMaskOrFilter(0x20,   0b00000000, 0b00000000, 0b00000000, 0b00000000);
	setMaskOrFilter(0x00, 0b00000000, 0b00000000, 0b00000000, 0b00000000);
	setMaskOrFilter(0x04, 0b00000000, 0b00000000, 0b00000000, 0b00000000);

	//disable the second buffer
	setMaskOrFilter(0x24,   0b00000000, 0b00000000, 0b00000000, 0b00000000); 
	setMaskOrFilter(0x08, 0b00000000, 0b00000000, 0b00000000, 0b00000000);
	setMaskOrFilter(0x10, 0b00000000, 0b00000000, 0b00000000, 0b00000000); 
	setMaskOrFilter(0x14, 0b00000000, 0b00000000, 0b00000000, 0b00000000);
	setMaskOrFilter(0x18, 0b00000000, 0b00000000, 0b00000000, 0b00000000); 
}

void CAN::setMaskOrFilter(uint8_t mask, uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) {
	// change to configuration mode
	mcp2515_bit_modify(CANCTRL, 0xe0, (1<<REQOP2));
	while ((mcp2515_read_register(CANSTAT) & 0xe0) != (1<<REQOP2))
		;
	mcp2515_write_register(mask, b0);
	mcp2515_write_register(mask+1, b1);
	mcp2515_write_register(mask+2, b2);
	mcp2515_write_register(mask+3, b3);
	mcp2515_set_mode(NORMAL_MODE);
}

//Activate Interrupt when receive a message on RX0 or RX1
void CAN::activateInterrupt(void){
	mcp2515_write_register(CANINTE, 0x03);
}

void CAN::clearFlags(void){
	mcp2515_write_register(CANINTF, 0x00);
}


/*************************USB Functions for bootloader****************************/
uint8_t CAN::char_to_byte(char *s)
{
	uint8_t t = *s;
	
	if (t >= 'a')
		t = t - 'a' + 10;
	else if (t >= 'A')
		t = t - 'A' + 10;
	else
		t = t - '0';
	
	return t;
}

uint8_t CAN::hex_to_byte(char *s)
{
	return (char_to_byte(s) << 4) | char_to_byte(s + 1);
}

// ----------------------------------------------------------------------------
bool CAN::usbcan_decode_message(char *str, uint8_t length)
{
	can_t msg;
	uint8_t dlc_pos;
	bool extended;
	
	if (str[0] == 'R' || str[0] == 'T') {
		extended = true;
		dlc_pos = 9;
	}
	else {
		extended = false;
		dlc_pos = 4;
	}
	
	if (length < dlc_pos + 1)
		return false;
	
	// get the number of data-bytes for this message
	msg.length = str[dlc_pos] - '0';
	
	if (msg.length > 8)
		return false;		// too many data-bytes
	
	
	if (str[0] == 'r' || str[0] == 'R') {
		msg.flags.rtr = true;
		if (length != (dlc_pos + 1))
			return false;
	}
	else {
		msg.flags.rtr = false;
		if (length != (msg.length * 2 + dlc_pos + 1))
			return false;
	}
	
	// read the messge-identifier
	if (extended)
	{
		uint16_t id;
		uint16_t id2;
		
		id  = hex_to_byte(&str[1]) << 8;
		id |= hex_to_byte(&str[3]);
		
		id2  = hex_to_byte(&str[5]) << 8;
		id2 |= hex_to_byte(&str[7]);
		
		msg.id = (uint32_t) id << 16 | id2;
	}
	else {
		uint16_t id;
		
		id  = char_to_byte(&str[1]) << 8;
		id |= hex_to_byte(&str[2]);
		
		msg.id = id;
	}
	
	msg.flags.extended = extended;
	
	// read data if the message is no rtr-frame
	if (!msg.flags.rtr)
	{
		char *buf = str + dlc_pos + 1;
		uint8_t i;
		
		for (i=0; i < msg.length; i++)
		{
			msg.data[i] = hex_to_byte(buf);
			buf += 2;
		}
	}
	
	// finally try to send the message
	if (mcp2515_send_message( &msg ))
		return true;
	else
		return false;
}

// ----------------------------------------------------------------------------
char CAN::usbcan_decode_command(char *str, uint8_t length)
{
	uint8_t temp;
	uint8_t i;
	if (length == 0)
		return('0');
	
	switch (str[0]) {
		case 'R':	// send frames
		case 'T':
		case 'r':
		case 't':	
			if ( !usbcan_decode_message(str, length) ) {
				goto error;
			}
			break;	
	}

	return('\r');	// command could be executed
	
error:
	return('7');		// Error in command
}

//***************Tools for debugging****************//
//This function allows the transmission of text message into a CAN message. The string is automatically cut to fit into a frame.
//For better results, use short messages (<=8 characters) to use only one frame.
void CAN::printString2Can(char data[]){
  can_t msg;
  int compt=0;
  msg.id =0xFF;
  msg.flags.rtr = 0;
  msg.flags.extended = 0;
  for(int i=0;i<strlen(data)+1;i++){
    if(data[i]!='\0') msg.data[i%8]=data[i];
    else {
      msg.length = (i-1)-8*compt;
      mcp2515_send_message(&msg);
      break;
    }
    if(i%8==7){
      msg.length = 8;
      mcp2515_send_message(&msg);
      compt++;
    }
  }
}


