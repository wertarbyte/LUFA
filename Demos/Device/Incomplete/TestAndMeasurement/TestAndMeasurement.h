/*
             LUFA Library
     Copyright (C) Dean Camera, 2010.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com
*/

/*
  Copyright 2010  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this 
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in 
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting 
  documentation, and that the name of the author not be used in 
  advertising or publicity pertaining to distribution of the 
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#ifndef _TESTANDMEASUREMENT_H_
#define _TESTANDMEASUREMENT_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/wdt.h>
		#include <avr/power.h>
		#include <avr/interrupt.h>

		#include "Descriptors.h"

		#include <LUFA/Version.h>
		#include <LUFA/Drivers/USB/USB.h>
		#include <LUFA/Drivers/Board/LEDs.h>

	/* Macros: */
		/** LED mask for the library LED driver, to indicate that the USB interface is not ready. */
		#define LEDMASK_USB_NOTREADY                   LEDS_LED1

		/** LED mask for the library LED driver, to indicate that the USB interface is enumerating. */
		#define LEDMASK_USB_ENUMERATING               (LEDS_LED2 | LEDS_LED3)

		/** LED mask for the library LED driver, to indicate that the USB interface is ready. */
		#define LEDMASK_USB_READY                     (LEDS_LED2 | LEDS_LED4)

		/** LED mask for the library LED driver, to indicate that an error has occurred in the USB interface. */
		#define LEDMASK_USB_ERROR                     (LEDS_LED1 | LEDS_LED3)
		
		#define Req_InitiateAbortBulkOut              0x01
		#define Req_CheckAbortBulkOutStatus           0x02
		#define Req_InitiateAbortBulkIn               0x03
		#define Req_CheckAbortBulkInStatus            0x04
		#define Req_InitiateClear                     0x05
		#define Req_CheckClearStatus                  0x06
		#define Req_GetCapabilities                   0x07
		#define Req_IndicatorPulse                    0x40
		
		#define TMC_REQUEST_STATUS_SUCCESS            0x01
		#define TMC_REQUEST_STATUS_PENDING            0x02
		#define TMC_REQUEST_STATUS_FAILED             0x80
		#define TMC_REQUEST_STATUS_NOTRANSFER         0x81
		#define TMC_REQUEST_STATUS_NOCHECKINITIATED   0x82
		#define TMC_REQUEST_STATUS_CHECKINPROGRESS    0x83

	/* Type Defines */
		typedef struct
		{
			uint8_t  Status;
			uint8_t  _RESERVED1;

			uint16_t TMCVersion;
			
			struct
			{
				unsigned char ListenOnly             : 1;
				unsigned char TalkOnly               : 1;
				unsigned char PulseIndicateSupported : 1;
				unsigned char _RESERVED              : 5;
			} Interface;
			
			struct
			{
				unsigned char SupportsAbortINOnMatch : 1;
				unsigned char _RESERVED              : 7;
			} Device;
			
			uint8_t _RESERVED2[6];
			uint8_t _RESERVED3[12];			
		} TMC_Capabilities_t;
		
	/* Function Prototypes: */
		void SetupHardware(void);
		void TMC_Task(void);

		void EVENT_USB_Device_Connect(void);
		void EVENT_USB_Device_Disconnect(void);
		void EVENT_USB_Device_ConfigurationChanged(void);
		void EVENT_USB_Device_UnhandledControlRequest(void);

#endif