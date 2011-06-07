/*
             LUFA Library
     Copyright (C) Dean Camera, 2011.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2011  Dean Camera (dean [at] fourwalledcubicle [dot] com)

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

/** \file
 *
 *  Main source file for the AudioInput demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "AudioInput.h"

/** LUFA Audio Class driver interface configuration and state information. This structure is
 *  passed to all Audio Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_Audio_Device_t Microphone_Audio_Interface =
	{
		.Config =
			{
				.StreamingInterfaceNumber = 1,

				.DataINEndpointNumber     = AUDIO_STREAM_EPNUM,
				.DataINEndpointSize       = AUDIO_STREAM_EPSIZE,
			},
	};

/** Current audio sampling frequency of the streaming audio endpoint. */
static uint32_t CurrentAudioSampleFrequency = 48000;


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	sei();

	for (;;)
	{
		Audio_Device_USBTask(&Microphone_Audio_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	LEDs_Init();
	Buttons_Init();
	ADC_Init(ADC_FREE_RUNNING | ADC_PRESCALE_32);
	ADC_SetupChannel(MIC_IN_ADC_CHANNEL);
	USB_Init();

	/* Start the ADC conversion in free running mode */
	ADC_StartReading(ADC_REFERENCE_AVCC | ADC_RIGHT_ADJUSTED | MIC_IN_ADC_MUX_MASK);
}

/** ISR to handle the reloading of the data endpoint with the next sample. */
ISR(TIMER0_COMPA_vect, ISR_BLOCK)
{
	uint8_t PrevEndpoint = Endpoint_GetCurrentEndpoint();

	/* Check if the sample reload timer period has elapsed, and that the USB bus is ready for a new sample */
	if (Audio_Device_IsReadyForNextSample(&Microphone_Audio_Interface))
	{
		int16_t AudioSample;

		#if defined(USE_TEST_TONE)
			static uint8_t SquareWaveSampleCount;
			static int16_t CurrentWaveValue;
			
			/* In test tone mode, generate a square wave at 1/256 of the sample rate */
			if (SquareWaveSampleCount++ == 0xFF)
			  CurrentWaveValue ^= 0x8000;
			
			/* Only generate audio if the board button is being pressed */
			AudioSample = (Buttons_GetStatus() & BUTTONS_BUTTON1) ? CurrentWaveValue : 0;
		#else
			/* Audio sample is ADC value scaled to fit the entire range */
			AudioSample = ((SAMPLE_MAX_RANGE / ADC_MAX_RANGE) * ADC_GetResult());

			#if defined(MICROPHONE_BIASED_TO_HALF_RAIL)
			/* Microphone is biased to half rail voltage, subtract the bias from the sample value */
			AudioSample -= (SAMPLE_MAX_RANGE / 2);
			#endif
		#endif
		
		Audio_Device_WriteSample16(&Microphone_Audio_Interface, AudioSample);
	}

	Endpoint_SelectEndpoint(PrevEndpoint);
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);

	/* Sample reload timer initialization */
	TIMSK0  = (1 << OCIE0A);
	OCR0A   = ((F_CPU / 8 / CurrentAudioSampleFrequency) - 1);
	TCCR0A  = (1 << WGM01);  // CTC mode
	TCCR0B  = (1 << CS01);   // Fcpu/8 speed
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	/* Stop the sample reload timer */
	TCCR0B = 0;

	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= Audio_Device_ConfigureEndpoints(&Microphone_Audio_Interface);

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	Audio_Device_ProcessControlRequest(&Microphone_Audio_Interface);
}

/** Audio class driver callback for the setting and retrieval of streaming endpoint properties. This callback must be implemented
 *  in the user application to handle property manipulations on streaming audio endpoints.
 *
 *  When the DataLength parameter is NULL, this callback should only indicate whether the specified operation is valid for
 *  the given endpoint index, and should return as fast as possible. When non-NULL, this value may be altered for GET operations
 *  to indicate the size of the retreived data.
 *
 *  \note The length of the retrieved data stored into the Data buffer on GET operations should not exceed the initial value
 *        of the \c DataLength parameter.
 *
 *  \param[in,out] AudioInterfaceInfo  Pointer to a structure containing an Audio Class configuration and state.
 *  \param[in]     EndpointProperty    Property of the endpoint to get or set, a value from Audio_ClassRequests_t.
 *  \param[in]     EndpointIndex       Index of the streaming endpoint whose property is being referenced.
 *  \param[in]     EndpointControl     Parameter of the endpoint to get or set, a value from Audio_EndpointControls_t.
 *  \param[in,out] DataLength          For SET operations, the length of the parameter data to set. For GET operations, the maximum
 *                                     length of the retrieved data. When NULL, the function should return whether the given property
 *                                     and parameter is valid for the requested endpoint without reading or modifying the Data buffer.
 *  \param[in,out] Data                Pointer to a location where the parameter data is stored for SET operations, or where
 *                                     the retrieved data is to be stored for GET operations.
 *
 *  \return Boolean true if the property get/set was successful, false otherwise
 */
bool CALLBACK_Audio_GetSetEndpointProperty(USB_ClassInfo_Audio_Device_t* const AudioInterfaceInfo,
                                           const uint8_t EndpointProperty,
                                           const uint8_t EndpointAddress,
                                           const uint8_t EndpointControl,
                                           uint16_t* const DataLength,
                                           uint8_t* Data)
{
	/* Check the requested endpoint to see if a supported endpoint is being manipulated */
	if (EndpointAddress == (ENDPOINT_DESCRIPTOR_DIR_IN | Microphone_Audio_Interface.Config.DataINEndpointNumber))
	{
		/* Check the requested control to see if a supported control is being manipulated */
		if (EndpointControl == AUDIO_EPCONTROL_SamplingFreq)
		{
			/* Check the requested property to see if a supported property is being manipulated */
			if (EndpointProperty == AUDIO_REQ_SetCurrent)
			{
				/* Check if we are just testing for a valid property, or actually adjusting it */
				if (DataLength != NULL)
				{
					/* Set the new sampling frequency to the value given by the host */
					CurrentAudioSampleFrequency = (((uint32_t)Data[2] << 16) | ((uint32_t)Data[1] << 8) | (uint32_t)Data[0]);

					/* Adjust sample reload timer to the new frequency */
					OCR0A = ((F_CPU / 8 / CurrentAudioSampleFrequency) - 1);				
				}
				
				return true;
			}
			else if (EndpointProperty == AUDIO_REQ_GetCurrent)
			{
				/* Check if we are just testing for a valid property, or actually reading it */
				if (DataLength != NULL)
				{
					*DataLength = 3;

					Data[2] = (CurrentAudioSampleFrequency >> 16);
					Data[1] = (CurrentAudioSampleFrequency >> 8);
					Data[0] = (CurrentAudioSampleFrequency &  0xFF);					
				}
				
				return true;
			}
		}
	}
	
	return false;
}
