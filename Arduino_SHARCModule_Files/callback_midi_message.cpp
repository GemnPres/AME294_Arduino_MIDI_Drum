/*
 * Copyright (c) 2018 Analog Devices, Inc.  All rights reserved.
 *
 * These are the hooks for the MIDI / Serial processing functions.
 *
 */
#include <stdint.h>
#include <math.h>
#include "midi_setup.h"

// Define your audio system parameters in this file
#include "common/audio_system_config.h"

/**
 * UART / MIDI messages can be processed either by the ARM core or by SHARC Core 1.
 * Select which option in the audio_system_config.h file.
 */
#if defined(MIDI_UART_MANAGED_BY_SHARC1_CORE) && (MIDI_UART_MANAGED_BY_SHARC1_CORE)

// Driver for UART / MIDI functionality for Audio Project Fin
#include "drivers/bm_uart_driver/bm_uart.h"

// Event logging / error handling / functionality
#include "drivers/bm_event_logging_driver/bm_event_logging.h"

#include "callback_midi_message.h"

// Create an instance of our MIDI UART driver
BM_UART midi_uart_sharc1;

/**
 * @brief Sets up MIDI on the SHARC Core 1
 *
 * @return true if successful
 */

extern Keyboard keys[6];	//global var

bool midi_setup_sharc1(void) {

	//tells SAM to look for MIDI sig
    if (uart_initialize(&midi_uart_sharc1, UART_BAUD_RATE_MIDI, UART_SERIAL_8N1, UART_AUDIOPROJ_DEVICE_MIDI)
        != UART_SUCCESS) {
        return false;
    }

    // Set our user call back for received MIDI bytes
    uart_set_rx_callback(&midi_uart_sharc1, midi_rx_callback_sharc1);

    return true;
}

/**
 * @brief Callback when new MIDI bytes arrive
 */
void midi_rx_callback_sharc1(void) {

    uint8_t val;	//input byte for the midi

    static uint32_t midi_state = 0;
    static bool midi_note_start = false;
    static bool midi_note_stop = false;
    static uint32_t midi_note = 0;
    static uint32_t midi_vol = 0;

    // Keep reading bytes from MIDI FIFO until we have processed all of them
    while (uart_available(&midi_uart_sharc1)) {

        // Replace the uart_read_byte() / uart_write_byte() functions below with any custom code
        // This code just passes the received MIDI byte back to MIDI out

        // Read the new byte
        uart_read_byte(&midi_uart_sharc1, &val);	//read func

        if(midi_state == 0){
        	if(val == 0x80){
        		midi_note_stop = true;
        	}

        	if(val == 0x90){
        		midi_note_start = true;
        	}

        	midi_state = 1;	//tells SAM to look for the next byte
        	return;	//return back to the start of While loop and test the 2nd byte; FIFO sequential for memory
        }

        if(midi_state == 1){
        	midi_note = val;
        	midi_state = 2;
        	return;
        }

        if(midi_state == 2){
        	midi_vol = val;
            midi_state = 0;	//have received all 3 bytes

            //generate or stop sig output
                  if(midi_note_start == true){
                  	//variable to check if a note is found or not
                  	bool found = false;
                  	//variable to count through voices of filter object
                  	int idx = 0;
                  	do{
                  		//if the voice is not currently playing
                  		if(!keys[idx].playing){
                  			keys[idx].playing = true;
                  			//assign note value to voice, convert to freq value
                  			keys[idx].midiNote = midi_note;
                  			found = true;	//We've found this note
                  		}

                  		idx++; //update to the next potential note

                  	}while(!found && idx<5);

                  		midi_note_start = false;

                  	return;
                  }

                  if(midi_note_stop){
                	  bool found = false;
                	  int idx = 0;

                	  do{
                		  //if the voice is not currently playing
                		  if(keys[idx].playing && keys[idx].midiNote == midi_note){

                			  keys[idx].playing = false;
                			  found = true;	//We've found this note
                		  }

                		  idx++; //update to the next potential note
                	  }while(!found && idx <5);

                	  midi_note_stop = false;

                	  return;
                  }



            return;	//back to the start of while loop; end of the testing for the 3rd byte
        }






        // Write that byte back to MIDI TX
        //uart_write_byte(&midi_uart_sharc1, val);	//write func; we only need to read for this lab
    }
}

#endif
