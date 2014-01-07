// coding: utf-8
// ----------------------------------------------------------------------------
/* Copyright (c) 2010, Roboterclub Aachen e.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of the Roboterclub Aachen e.V. nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ROBOTERCLUB AACHEN E.V. ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ROBOTERCLUB AACHEN E.V. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: utils.h 434 2010-11-21 19:56:49Z dergraaf $
 */
// ----------------------------------------------------------------------------

#ifndef	UTILS_H
#define	UTILS_H

// ----------------------------------------------------------------------------
/**
 * \ingroup	utils_h
 * \name	Nützliches
 */
//@{

#define	LOW_BYTE(x)		((uint8_t) (x & 0xff))
#define	HIGH_BYTE(x)	((uint8_t) (x >> 8))
#define LOW_WORD(x)		((uint16_t) (x & 0xffff))
#define HIGH_WORD(x)    ((uint16_t) (x >> 16))

//@}

// ----------------------------------------------------------------------------
/**
 * \ingroup	utils_h
 * \name	Port-Makros
  * The macros RESET(), SET() SET_OUTPUT(), SET_INPUT() and IS_SET()
  * always refer to a particular bit of a port and thus help
  * to make the code very portable.
  *
  * Example:
  * 
  * #define LED D 5  // PORT D, pin 5
  *
  * SET_OUTPUT(LED); // turn pin an output (for example DDRD |= (1 << 5);)
  *
  * SET(LED);        // Turn LED on.
  * 
  *
  * Or:
  *
  * 
  * #define SWITCH B,1 // PORT B, pin 1
  *
  * SET_INPUT_WITH_PULLUP (SWITCH);
  *
  * if (IS_SET (SWITCH)) {
  * ...
  * }
  * 
  *
  * Thus, only one "define" must be changed when another pin shall be used. 
 */
//@{
#if defined(__DOXYGEN__)

#define RESET(x)		//!< reset one bit of a port
#define SET(x)			//!< set bit
#define	TOGGLE(x)		//!< invert bit

#define	SET_OUTPUT(x)	//!< activate on bit of port as output
#define	SET_INPUT(x)	//!< as input
#define	SET_PULLUP(x)	//!< activate pull-up resistor (only with inputs)

#define	SET_INPUT_WITH_PULLUP(x)	//!< declare as input with activated pullup

#define	IS_SET(x)		//!< query state of input

#else /* !DOXYGEN */

/* Why we need here sometimes so strange constructions was explainend for example 
 * here: http://www.mikrocontroller.net/forum/read-1-324854.html#324980 .
 */
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

#endif /* DOXYGEN */
//@}

#endif	// UTILS_H
