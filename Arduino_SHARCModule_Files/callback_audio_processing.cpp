/*
 * Copyright (c) 2018-2019 Analog Devices, Inc.  All rights reserved.
 *
 * These are the hooks for the audio processing functions.
 *
 */

#include <audio_processing/audio_effects_selector.h>
#include <math.h>
#include "midi_setup.h"

// Define your audio system parameters in this file
#include "common/audio_system_config.h"

// Support for simple multi-core data sharing
#include "common/multicore_shared_memory.h"

// Variables related to the audio framework that is currently selected (e.g. input and output buffers)
#include "audio_framework_selector.h"

// Includes all header files for effects and calls for effect selector
//#include "audio_processing/audio_effects_selector.h"

// Prototypes for this file
#include "callback_audio_processing.h"

/*
 *
 * Available Processing Power
 * --------------------------
 *
 * The two SHARC cores provide a hefty amount of audio processing power.  However, it is
 * important to ensure that any audio processing code can run and complete within one frame of audio.
 *
 * The total number of cycles available in the audio callback can be calculated as follows:
 *
 * total cycles = ( processor-clock-speed * audio-block-size ) / audio-sample-rate//
 *
 * For example, if the processor is running at 450MHz, the audio sampling rate is 48KHz and the
 * audio block size is set to 32 words, the total number of processor cycles available in each
 * callback is 300,000 cycles or 300,000/32 or 9,375 per sample of audio.
 *
 * Available Audio Buffers
 * -----------------------
 *
 * There are several sets of audio input and output buffers that correspond to the
 * various peripherals (e.g. audio codec, USB, S/PDIF, A2B).
 *
 * To send audio from USB out the DAC on the ADAU1761 one simply needs to copy data
 * from the USB buffers and copy them to the ADAU1761 buffer.
 *
 * for (i=0;i<AUDIO_BLOCK_SIZE;i++) {
 *   audiochannel_adau1761_0_left_out[i] = audiochannel_USB_0_left_in[i];
 *   audiochannel_adau1761_0_right_out[i] = audiochannel_USB_0_right_in[i];
 * }
 *
 * The framework ensures that audio is sample rate converted as needed (e.g S/PDIF)
 * and arrives where it needs to be on time using DMA.  It also manages the conversion
 * between fixed and floating point.
 *
 * Below is a list of the various input buffers and output buffers that are available.
 * Be sure that the corresponding peripheral has been enabled in audio_system_config.h
 *
 * Input Buffers
 * *************
 *
 *  Audio from the ADAU1761 ADCs
 *     audiochannel_adau1761_0_left_in[]
 *     audiochannel_adau1761_0_left_in[]
 *
 *  Audio from the S/PDIF receiver
 *     audiochannel_spdif_0_left_in[]
 *     audiochannel_spdif_0_right_in[]
 *
 *  Audio from USB (be sure to enable USB in audio_system_config.h)
 *     audiochannel_USB_0_left_in[]
 *     audiochannel_USB_0_right_in[]
 *
 *  Audio from A2B Bus
 *     audiochannel_a2b_0_left_in[]
 *     audiochannel_a2b_0_right_in[]
 *     audiochannel_a2b_1_left_in[]
 *     audiochannel_a2b_1_right_in[]
 *     audiochannel_a2b_2_left_in[]
 *     audiochannel_a2b_2_right_in[]
 *     audiochannel_a2b_3_left_in[]
 *     audiochannel_a2b_3_right_in[]
 *
 *
 *  Audio from Faust (be sure to enable Faust in audio_system_config.h and include the libraries)
 *
 *     audioChannel_faust_0_left_in[]
 *     audioChannel_faust_0_right_in[]
 *     audioChannel_faust_1_left_in[]
 *     audioChannel_faust_1_right_in[]
 *     audioChannel_faust_2_left_in[]
 *     audioChannel_faust_2_right_in[]
 *     audioChannel_faust_3_left_in[]
 *     audioChannel_faust_3_right_in[]
 *
 * Output Buffers
 * **************
 *  Audio to the ADAU1761 DACs
 *     audiochannel_adau1761_0_left_out[]
 *     audiochannel_adau1761_0_left_out[]
 *
 *  Audio to the S/PDIF transmitter
 *     audiochannel_spdif_0_left_out[]
 *     audiochannel_spdif_0_right_out[]
 *
 *  Audio to USB (be sure to enable USB in audio_system_config.h)
 *     audiochannel_USB_0_left_out[]
 *     audiochannel_USB_0_right_out[]
 *
 *  Audio to A2B Bus (be sure to enable A2B in audio_system_config.h)
 *     audiochannel_a2b_0_left_out[]
 *     audiochannel_a2b_0_right_out[]
 *     audiochannel_a2b_1_left_out[]
 *     audiochannel_a2b_1_right_out[]
 *     audiochannel_a2b_2_left_out[]
 *     audiochannel_a2b_2_right_out[]
 *     audiochannel_a2b_3_left_out[]
 *     audiochannel_a2b_3_right_out[]
 *
 *  Audio from Faust (be sure to enable Faust in audio_system_config.h)
 *
 *     audioChannel_faust_0_left_out[]
 *     audioChannel_faust_0_right_out[]
 *     audioChannel_faust_1_left_out[]
 *     audioChannel_faust_1_right_out[]
 *     audioChannel_faust_2_left_out[]
 *     audioChannel_faust_2_right_out[]
 *     audioChannel_faust_3_left_out[]
 *     audioChannel_faust_3_right_out[]
 *
 *  Note: Faust processing occurs before the audio callback so any data
 *  copied into Faust's input buffers will be available the next time
 *  the callback is called.  Similarly, Faust's output buffers contain
 *  audio that was processed before the callback.
 *
 *
 * There is also a set of buffers for sending audio to / from SHARC Core 2
 *
 *  Output to SHARC Core 2
 *     audiochannel_to_sharc_core2_0_left[]
 *     audiochannel_to_sharc_core2_0_right[]
 *     audiochannel_to_sharc_core2_1_left[]
 *     audiochannel_to_sharc_core2_1_right[]
 *     audiochannel_to_sharc_core2_2_left[]
 *     audiochannel_to_sharc_core2_2_right[]
 *     audiochannel_to_sharc_core2_3_left[]
 *     audiochannel_to_sharc_core2_3_right[]
 *
 *  Input from SHARC Core 2 (processed audio from SHARC Core 2)
 *     audiochannel_from_sharc_core2_0_left[]
 *     audiochannel_from_sharc_core2_0_right[]
 *     audiochannel_from_sharc_core2_1_left[]
 *     audiochannel_from_sharc_core2_1_right[]
 *     audiochannel_from_sharc_core2_2_left[]
 *     audiochannel_from_sharc_core2_2_right[]
 *     audiochannel_from_sharc_core2_3_left[]
 *     audiochannel_from_sharc_core2_3_right[]
 *
 * Finally, there is a set of aliased buffers that sends audio to the
 * right place.  On SHARC 1, the In[] buffers are received from the ADC
 * and the Out[] buffers are sent to either SHARC 2 (when in dual core more)
 * or to the DACs (when in single core mode).  The In[] buffers on SHARC core
 * 2 are received from SHARC core 1 and the Out[] buffers are sent to the DACs
 * (via SHARC core 1).
 *
 *     audiochannel_0_left_in[]
 *     audiochannel_0_right_in[]
 *
 *     audiochannel_1_left_out[]
 *     audiochannel_1_right_out[]
 *     audiochannel_2_left_out[]
 *     audiochannel_2_right_out[]
 *     audiochannel_3_left_out[]
 *     audiochannel_3_right_out[]
 *
 *     When the automotive board is being used, there are 16 channels of aliased
 *     buffers, not 8.  So they go up to audiochannel_7_left_in / audiochannel_7_right_in
 *     and audiochannel_7_left_out / audiochannel_7_right_out
 *
 * See the .c/.h file for the corresponding audio framework in the Audio_Frameworks
 * directory to see the buffers that are available for other frameworks (like the
 * 16 channel automotive framework).
 *
 */

/*
 * Place any initialization code here for the audio processing
 */


struct Drums {
	float tau;
	float tauf;
	float A;
	float r; //end time
	float TimePeak;
	float t_r;
	float A_t;
	float I_t;
	float fc;
	float fm;
	float I_0;
	float DrumSynth;
	float counter;
	float dither;
	float freqShift;
}Kickdrum,SubKick,Snaredrum,Midtom,SubTom,Hightom,Hihat;

//Global var
Keyboard keys[5];	//an arr of 6, can synthesize up to 6 notes
//var from midi messages
float tempAudio;	//sum synthesized sound
float tempAudio2;	//sum synthesized sound
float tempAudio3;	//sum synthesized sound
float tempAudio4;	//sum synthesized sound
float tempAudio5;	//sum synthesized sound


// button default
int type = 0;
int type2 = 0;
int type3 = 0;




void processaudio_setup(void) {

	// Initialize the audio effects in the audio_processing/ folder
	//audio_effects_setup_core1();

	// *******************************************************************************
	// Add any custom setup code here
	// *******************************************************************************

	//initialize the synth
	int i = 0;
	for(i=0; i<=5; i++){
		keys[i].reset();
	}

	//counter for keeping tract of time t
	Kickdrum.counter = 0;
	SubKick.counter = 0;
	Snaredrum.counter = 0;
	Midtom.counter = 0;
	SubTom.counter = 0;
	Hightom.counter = 0;

}

/*
 * This callback is called every time we have a new audio buffer that is ready
 * for processing.  It's currently configured for in-place processing so if no
 * processing is done to the audio, it is passed through unaffected.
 *
 * See the header file for the framework you have selected in the Audio_Frameworks
 * directory for a list of the input and output buffers that are available based on
 * the framework and hardware.
 *
 * The two SHARC cores provide a hefty amount of audio processing power. However, it is important
 * to ensure that any audio processing code can run and complete within one frame of audio.
 *
 * The total number of cycles available in the audio callback can be calculated as follows:
 * total cycles = ( processor-clock-speed * audio-block-size ) / audio-sample-rate
 *
 * For example, if the processor is running at 450MHz, the audio sampling rate is 48KHz and the audio
 * block size is set to 32 words, the total number of processor cycles available in each callback
 * is 300,000 cycles or 300,000/32 or 9,375 per sample of audio
 */

// When debugging audio algorithms, helpful to comment out this pragma for more linear single stepping.
#pragma optimize_for_speed
void processaudio_callback(void) {

	/*if (true) {

		// Copy incoming audio buffers to the effects input buffers
		copy_buffer(audiochannel_0_left_in, audio_effects_left_in,
				AUDIO_BLOCK_SIZE);
		copy_buffer(audiochannel_0_right_in, audio_effects_right_in,
				AUDIO_BLOCK_SIZE);

		// Process audio effects
		audio_effects_process_audio_core1();

		// Copy processed audio back to input buffers
		copy_buffer(audio_effects_left_out, audiochannel_0_left_in,
				AUDIO_BLOCK_SIZE);
		copy_buffer(audio_effects_right_out, audiochannel_0_right_in,
				AUDIO_BLOCK_SIZE);

	}*/

	//knob to control fundamental frequencies for the drums
	Kickdrum.freqShift = multicore_data->audioproj_fin_pot_hadc0+1;
	Snaredrum.freqShift = multicore_data->audioproj_fin_pot_hadc1+1;
	Midtom.freqShift = multicore_data->audioproj_fin_pot_hadc2+1;
	Hightom.freqShift = multicore_data->audioproj_fin_pot_hadc2+1;



	// Otherwise, perform our C-based block processing here!
	for (int i = 0; i < AUDIO_BLOCK_SIZE; i++) {

		// *******************************************************************************
		// Replace the pass-through code below with your custom audio processing code here
		// *******************************************************************************

		// Default: Pass audio just from 1/8" (or 1/4" on Audio Project Fin) inputs to outputs
		tempAudio = 0;	//reset loop
		tempAudio2 = 0;
		tempAudio3 = 0;
		tempAudio4 = 0;
		tempAudio5 = 0;

		for(int j=0; j<=5; j++){

			//Kickdrum
			if(keys[j].midiNote == 60){

				//Kick Fundamental
				Kickdrum.A = 0.999;
				Kickdrum.r = 0.3;
				Kickdrum.t_r = Kickdrum.counter/AUDIO_SAMPLE_RATE;
				Kickdrum.TimePeak = 0.005;
				Kickdrum.tau = 0.065;

				//Get time envelope A_t
				//Attack
				if (Kickdrum.t_r <= Kickdrum.TimePeak){
					Kickdrum.A_t = 199.826*Kickdrum.t_r;
				}

				//Decay
				if ((Kickdrum.t_r > Kickdrum.TimePeak) && (Kickdrum.t_r <= Kickdrum.r)){
					Kickdrum.A_t = Kickdrum.A*exp(-(Kickdrum.t_r-Kickdrum.TimePeak)/Kickdrum.tau);
				}

				//End of decay
				if (Kickdrum.t_r > Kickdrum.r){
					Kickdrum.A_t = 0;
				}

				//Get frequency envelope I_t
				Kickdrum.I_t = -(1/70)*Kickdrum.t_r + 1;
				Kickdrum.fc = 70*Kickdrum.freqShift;
				Kickdrum.fm = 30*Kickdrum.freqShift;
				Kickdrum.I_0 = 1.15+type3*2;

				//FM synthesis sound
				Kickdrum.DrumSynth = Kickdrum.A_t*sin(2*PI*Kickdrum.fc*Kickdrum.t_r + Kickdrum.I_0*Kickdrum.I_t*sin(2*PI*Kickdrum.fm*Kickdrum.t_r));

				//Kick Percussion
				SubKick.r = 0.03;
				SubKick.t_r = SubKick.counter/AUDIO_SAMPLE_RATE;
				SubKick.A_t = -(1/SubKick.r)*SubKick.t_r + 1;
				SubKick.I_t = SubKick.A_t;
				SubKick.fc = 200;
				SubKick.fm = 350;
				SubKick.I_0 = 5;

				//FM synthesize the percussive sound; the percussive is shorter than the kick drum fundamental sound itself
				if(Kickdrum.t_r <= SubKick.r){
					SubKick.DrumSynth = SubKick.A_t*sin(2*PI*SubKick.fc*SubKick.t_r + SubKick.I_0*SubKick.I_t*sin(2*PI*SubKick.fm*SubKick.t_r));
				}

				else{
					SubKick.DrumSynth = 0;
				}

				//keep looping over the counter as long as t < time length of the drum sound
				if((Kickdrum.counter)<=14400){
					tempAudio = 0.001*SubKick.DrumSynth+Kickdrum.DrumSynth;
					Kickdrum.counter++; //increment time
					SubKick.counter++;
				}

				//if hits the end of the time decay of the drum sound, check if user is still pressing on the key
				//mute and reset if the key is released
				if(Kickdrum.counter == 14401 && keys[j].playing == false){
					tempAudio = 0;
					keys[j].reset();
				}

				//keep looping if the user keep pressing on the key
				if(Kickdrum.counter == 14401 && keys[j].playing == true){
					Kickdrum.counter = 0;
					Kickdrum.t_r = 0;
					SubKick.counter = 0;
					SubKick.t_r = 0;
				}

			}

			//Snaredrum
			if(keys[j].midiNote == 61){

				//Snare Fundamental
				Snaredrum.A = 0.999;
				Snaredrum.r = 0.25;
				Snaredrum.t_r = Snaredrum.counter/AUDIO_SAMPLE_RATE;
				Snaredrum.TimePeak = 0.00152;
				Snaredrum.tau = 0.04;

				//Get time envelope A_t
				if (Snaredrum.t_r <= Snaredrum.TimePeak){
					Snaredrum.A_t = 657.237*Snaredrum.t_r;
				}

				if ((Snaredrum.t_r > Snaredrum.TimePeak) && (Snaredrum.t_r <= Snaredrum.r)){
					Snaredrum.A_t = Snaredrum.A*exp(-(Snaredrum.t_r-Snaredrum.TimePeak)/Snaredrum.tau);
				}

				if (Snaredrum.t_r > Snaredrum.r){
					Snaredrum.A_t = 0;
				}

				//Get frequency envelope I_t
				Snaredrum.tauf = 0.03;
				Snaredrum.I_t = exp(-Snaredrum.t_r/Snaredrum.tauf);
				Snaredrum.fc = 80*Snaredrum.freqShift;
				Snaredrum.fm = 85*Snaredrum.freqShift;
				Snaredrum.I_0 = 1+type2*2;

				//FM synthesis sound
				Snaredrum.DrumSynth = Snaredrum.A_t*sin(2*PI*Snaredrum.fc*Snaredrum.t_r + Snaredrum.I_0*Snaredrum.I_t*sin(2*PI*Snaredrum.fm*Snaredrum.t_r));

				//Snare sound made out of white noise
				Snaredrum.dither = Snaredrum.A_t*(rand()%2-1)*0.035;

				if((Snaredrum.counter)<=12000){
					tempAudio2 = Snaredrum.DrumSynth+Snaredrum.dither;
					Snaredrum.counter++;	//increment time
				}

				//time check
				if(Snaredrum.counter == 12001 && keys[j].playing == false){
					tempAudio2 = 0;
					keys[j].reset();
				}

				if(Snaredrum.counter == 12001 && keys[j].playing == true){
					Snaredrum.counter = 0;
					Snaredrum.t_r = 0;
				}

			}

			//Midtom
			if(keys[j].midiNote == 62){

				//Tom Fundamental
				Midtom.A = 0.999;
				Midtom.r = 0.4;
				Midtom.t_r = Midtom.counter/AUDIO_SAMPLE_RATE;
				Midtom.TimePeak = 0.00642;
				Midtom.tau = 0.1;

				//Get time envelope A_t
				if (Midtom.t_r <= Midtom.TimePeak){
					Midtom.A_t = 155.607*Midtom.t_r;
				}

				if ((Midtom.t_r > Midtom.TimePeak) && (Midtom.t_r <= Midtom.r)){
					Midtom.A_t = Midtom.A*exp(-(Midtom.t_r-Midtom.TimePeak)/Midtom.tau);
				}

				if (Midtom.t_r > Midtom.r){
					Midtom.A_t = 0;
				}

				//Get frequency envelope I_t
				Midtom.tauf = 70;
				Midtom.I_t = 18500*pow((Midtom.t_r+0.01),2)*exp(-Midtom.tauf*(Midtom.t_r+0.01));
				Midtom.fc = 110*Midtom.freqShift;
				Midtom.fm = 113*Midtom.freqShift;
				Midtom.I_0 = 1.5+type*2;

				//FM synthesis sound
				Midtom.DrumSynth = Midtom.A_t*sin(2*PI*Midtom.fc*Midtom.t_r + Midtom.I_0*Midtom.I_t*sin(2*PI*Midtom.fm*Midtom.t_r));

				//Tom Percussion
				SubTom.r = 0.03;
				SubTom.t_r = SubTom.counter/AUDIO_SAMPLE_RATE;
				SubTom.A_t = -(1/SubTom.r)*SubTom.t_r + 1;
				SubTom.I_t = SubTom.A_t;
				SubTom.fc = 200;
				SubTom.fm = 350;
				SubTom.I_0 = 5;

				if(Midtom.t_r <= SubTom.r){
					SubTom.DrumSynth = SubTom.A_t*sin(2*PI*SubTom.fc*SubTom.t_r + SubTom.I_0*SubTom.I_t*sin(2*PI*SubTom.fm*SubTom.t_r));
				}

				else{
					SubTom.DrumSynth = 0;
				}

				if((Midtom.counter)<=19200){
					tempAudio3 = 0.005*SubTom.DrumSynth+Midtom.DrumSynth;
					Midtom.counter++; //increment time
					SubTom.counter++;
				}


				if(Midtom.counter == 19201 && keys[j].playing == false){
					tempAudio3 = 0;
					keys[j].reset();
				}

				if(Midtom.counter == 19201 && keys[j].playing == true){
					Midtom.counter = 0;
					Midtom.t_r = 0;
					SubTom.counter = 0;
					SubTom.t_r = 0;
				}
			}

			//Hightom
			if(keys[j].midiNote == 63){

				//Hightom Fundamental
				Hightom.A = 0.999;
				Hightom.r = 0.4;
				Hightom.t_r = Hightom.counter/AUDIO_SAMPLE_RATE;
				Hightom.TimePeak = 0.0144;
				Hightom.tau = 0.1;

				//Get time envelope A_t
				if (Hightom.t_r <= Hightom.TimePeak){
					Hightom.A_t = 69.375*Hightom.t_r;
				}

				if ((Hightom.t_r > Hightom.TimePeak) && (Hightom.t_r <= Hightom.r)){
					Hightom.A_t = Hightom.A*exp(-(Hightom.t_r-Hightom.TimePeak)/Hightom.tau);
				}

				if (Hightom.t_r > Hightom.r){
					Hightom.A_t = 0;
				}

				//Get frequency envelope I_t
				Hightom.tauf = 100;
				Hightom.I_t = 18500*pow((Hightom.t_r+0.01),2)*exp(-Hightom.tauf*(Hightom.t_r+0.01));
				Hightom.fc = 200*Hightom.freqShift;
				Hightom.fm = 400*Hightom.freqShift;
				Hightom.I_0 = 1.5+type*2;

				//FM synthesis sound
				Hightom.DrumSynth = Hightom.A_t*sin(2*PI*Hightom.fc*Hightom.t_r + Hightom.I_0*Hightom.I_t*sin(2*PI*Hightom.fm*Hightom.t_r));

				//Tom Percussion
				SubTom.r = 0.03;
				SubTom.t_r = SubTom.counter/AUDIO_SAMPLE_RATE;
				SubTom.A_t = -(1/SubTom.r)*SubTom.t_r + 1;
				SubTom.I_t = SubTom.A_t;
				SubTom.fc = 200;
				SubTom.fm = 350;
				SubTom.I_0 = 5;

				if(Hightom.t_r <= SubTom.r){
					SubTom.DrumSynth = SubTom.A_t*sin(2*PI*SubTom.fc*SubTom.t_r + SubTom.I_0*SubTom.I_t*sin(2*PI*SubTom.fm*SubTom.t_r));
				}

				else{
					SubTom.DrumSynth = 0;
				}

				//time check
				if((Hightom.counter)<=19200){
					tempAudio4 = 0.005*SubTom.DrumSynth+Hightom.DrumSynth;
					Hightom.counter++; //increment time
					SubTom.counter++;
				}


				if(Hightom.counter == 19201 && keys[j].playing == false){
					tempAudio4 = 0;
					keys[j].reset();
				}

				if(Hightom.counter == 19201 && keys[j].playing == true){
					Hightom.counter = 0;
					Hightom.t_r = 0;
					SubTom.counter = 0;
					SubTom.t_r = 0;
				}
			}

			//Hihat
			if(keys[j].midiNote == 64){

				//Hihat Fundamental Tone
				Hihat.A = 1;
				Hihat.r = 0.3;
				Hihat.t_r = Hihat.counter/AUDIO_SAMPLE_RATE;
				Hihat.TimePeak = 0.00122;
				Hihat.tau = 0.045;

				//Get time envelope A_t
				if (Hihat.t_r <= Hihat.TimePeak){
					Hihat.A_t = 819.672*Hihat.t_r;
				}

				if ((Hihat.t_r > Hihat.TimePeak) && (Hihat.t_r <= Hihat.r)){
					Hihat.A_t = Hihat.A*exp(-(Hihat.t_r-Hihat.TimePeak)/Hihat.tau);
				}

				if (Hihat.t_r > Hihat.r){
					Hihat.A_t = 0;
				}

				//Get frequency envelope I_t
				Hihat.I_t = exp(-Hihat.t_r/0.2);
				Hihat.fc = 350;
				Hihat.fm = 2*Hihat.fc;
				Hihat.I_0 = 20;

				//FM synthesis sound
				Hihat.DrumSynth = Hihat.A_t*sin(2*PI*Hihat.fc*Hihat.t_r + Hihat.I_0*Hihat.I_t*sin(2*PI*Hihat.fm*Hihat.t_r));

				//Hihat sound from white noise
				Hihat.dither = Hihat.A_t*(rand()%2-1)*0.2;

				if((Hihat.counter)<=14400){
					tempAudio5 = 0.15*Hihat.DrumSynth+Hihat.dither;
					Hihat.counter++;	//increment time
				}


				if(Hihat.counter == 14401 && keys[j].playing == false){
					tempAudio5 = 0;
					keys[j].reset();
				}

				if(Hihat.counter == 14401 && keys[j].playing == true){
					Hihat.counter = 0;
					Hihat.t_r = 0;
				}

			}

			//add up all sounds
			audiochannel_0_left_out[i] = 2*tempAudio + 2*tempAudio2 + tempAudio3 + tempAudio4 + tempAudio5;
			audiochannel_0_right_out[i] = 2*tempAudio + 2*tempAudio2 + tempAudio3 + tempAudio4 + tempAudio5;



		}



		/* Below are some additional examples of how to receive audio from the various input buffers

		 // Example: Pass audio just from 1/8" (or 1/4" on Audio Project Fin) inputs to outputs
		 audioChannel_0_left_out[i] = audioChannel_0_left_in[i];
		 audioChannel_0_right_out[i] = AudioChannel_0_right_in[i];

		 // Example: mix audio in from 1/8" jacks and A2B input
		 audiochannel_0_left_out[i] = audiochannel_0_left_in[i] + audiochannel_a2b_0_left_in[i];
		 audiochannel_0_right_out[i] = audiochannel_0_right_in[i] + audiochannel_a2b_0_right_in[i];

		 // Example: receive audio from S/PDIF inputs and analog inputs
		 audiochannel_0_left_out[i] = audiochannel_0_left_in[i] + audiochannel_spdif_0_left_in[i];
		 audiochannel_0_right_out[i] = audiochannel_0_right_in[i] + audiochannel_spdif_0_right_in[i];

		 */

		/* You can also write directly to the various output buffers to explicitly route
		 * audio to different peripherals (ADAU1761, S/PDIF, A2B, etc.).  If you're using both
		 * cores to process audio (configured in common/audio_system_config.h), write your
		 * processed audio data to the audiochannel_N_left_out/audiochannel_N_right_out buffers
		 * and direct the output to the second core.  The function below, processAudio_OutputRouting(),
		 * is then used to route audio returning from the second core to various peripherals.
		 *
		 * However, if you're only using a single core in the audio processing path, you can redirect audio to
		 * specific peripherals by writing to the corresponding output buffers as shown in the
		 * examples below.  When using just one core for processing, audio written to the
		 * audiochannel_0_left_out/audiochannel_0_right_out buffers will get sent to the ADAU1761.

		 // Example: Send audio in from ADAU1761 to the A2B bus (be sure to enable A2B in audio_system_config.h)
		 audiochannel_a2b_0_left_out[i] = audiochannel_0_left_in[i];
		 audiochannel_a2b_0_right_out[i] = audiochannel_0_right_in[i];

		 // Example: Send audio from ADAU1761 to the SPDIF transmitter
		 audiochannel_spdif_0_left_out[i]  = audiochannel_adau1761_0_left_in[i];
		 audiochannel_spdif_0_right_out[i] = audiochannel_adau1761_0_right_in[i];

		 // Example: Send first stereo pair from A2B bus to ADAU1761 audio out
		 audiochannel_0_left_out[i] = audiochannel_a2b_0_left_in[i];
		 audiochannel_0_right_out[i] = audiochannel_a2b_0_right_in[i];
		 */
		// If we're using just one core and A2B is enabled, copy the output buffer to A2B bus as well
#if (!USE_BOTH_CORES_TO_PROCESS_AUDIO) && (ENABLE_A2B)
			audiochannel_a2b_0_left_out[i] = audiochannel_0_left_out[i];
			audiochannel_a2b_0_right_out[i] = audiochannel_0_right_out[i];
#endif

		// If we're using Faust, copy audio into the flow
#if (USE_FAUST_ALGORITHM_CORE1)

		// Copy 8 channel audio from Faust to output buffers
		audiochannel_0_left_out[i] = audioChannel_faust_0_left_out[i];
		audiochannel_0_right_out[i] = audioChannel_faust_0_right_out[i];
		audiochannel_1_left_out[i] = audioChannel_faust_1_left_out[i];
		audiochannel_1_right_out[i] = audioChannel_faust_1_right_out[i];
		audiochannel_2_left_out[i] = audioChannel_faust_2_left_out[i];
		audiochannel_2_right_out[i] = audioChannel_faust_2_right_out[i];
		audiochannel_3_left_out[i] = audioChannel_faust_3_left_out[i];
		audiochannel_3_right_out[i] = audioChannel_faust_3_right_out[i];

		// Route audio to Faust for next block
		audioChannel_faust_0_left_in[i] = audiochannel_0_left_in[i] + audiochannel_spdif_0_left_in[i];
		audioChannel_faust_0_right_in[i] = audiochannel_0_right_in[i] + audiochannel_spdif_0_right_in[i];

#endif
	}

}

#if (USE_BOTH_CORES_TO_PROCESS_AUDIO)

/*
 * When using a dual core configuration, SHARC Core 1 is responsible for routing the
 * processed audio from SHARC Core 2 to the various output buffers for the
 * devices connected to the SC589.  For example, in a dual core framework, SHARC Core 1
 * may pass 8 channels of audio to Core 2, and then receive 8 channels of processed audio
 * back from Core 2.  It is this routine where we route these channels to the ADAU1761,
 * the A2B bus, SPDIF, etc.
 */
#pragma optimize_for_speed
void processaudio_output_routing(void) {

	static float t = 0;

	for (int i = 0; i < AUDIO_BLOCK_SIZE; i++) {

		// If automotive board is attached, send all 16 channels from core 2 to the DACs
#if defined(AUDIO_FRAMEWORK_16CH_SAM_AND_AUTOMOTIVE_FIN) && AUDIO_FRAMEWORK_16CH_SAM_AND_AUTOMOTIVE_FIN

		// Copy 16 channels from Core 2 to the DACs on the automotive board
		audiochannel_automotive_0_left_out[i] = audiochannel_from_sharc_core2_0_left[i];
		audiochannel_automotive_0_right_out[i] = audiochannel_from_sharc_core2_0_right[i];
		audiochannel_automotive_1_left_out[i] = audiochannel_from_sharc_core2_1_left[i];
		audiochannel_automotive_1_right_out[i] = audiochannel_from_sharc_core2_1_right[i];
		audiochannel_automotive_2_left_out[i] = audiochannel_from_sharc_core2_2_left[i];
		audiochannel_automotive_2_right_out[i] = audiochannel_from_sharc_core2_2_right[i];
		audiochannel_automotive_3_left_out[i] = audiochannel_from_sharc_core2_3_left[i];
		audiochannel_automotive_3_right_out[i] = audiochannel_from_sharc_core2_3_right[i];
		audiochannel_automotive_4_left_out[i] = audiochannel_from_sharc_core2_4_left[i];
		audiochannel_automotive_4_right_out[i] = audiochannel_from_sharc_core2_4_right[i];
		audiochannel_automotive_5_left_out[i] = audiochannel_from_sharc_core2_5_left[i];
		audiochannel_automotive_5_right_out[i] = audiochannel_from_sharc_core2_5_right[i];
		audiochannel_automotive_6_left_out[i] = audiochannel_from_sharc_core2_6_left[i];
		audiochannel_automotive_6_right_out[i] = audiochannel_from_sharc_core2_6_right[i];
		audiochannel_automotive_7_left_out[i] = audiochannel_from_sharc_core2_7_left[i];
		audiochannel_automotive_7_right_out[i] = audiochannel_from_sharc_core2_7_right[i];

#else

		// If A2B enabled, route audio down the A2B bus
#if (ENABLE_A2B)

		// Send all 8 channels from core 2 down the A2B bus
		audiochannel_a2b_0_left_out[i] = audiochannel_from_sharc_core2_0_left[i];
		audiochannel_a2b_0_right_out[i] = audiochannel_from_sharc_core2_0_right[i];
		audiochannel_a2b_1_left_out[i] = audiochannel_from_sharc_core2_1_left[i];
		audiochannel_a2b_1_right_out[i] = audiochannel_from_sharc_core2_1_right[i];
		audiochannel_a2b_2_left_out[i] = audiochannel_from_sharc_core2_2_left[i];
		audiochannel_a2b_2_right_out[i] = audiochannel_from_sharc_core2_2_right[i];
		audiochannel_a2b_3_left_out[i] = audiochannel_from_sharc_core2_3_left[i];
		audiochannel_a2b_3_right_out[i] = audiochannel_from_sharc_core2_3_right[i];

#endif

		// Send Audio from SHARC Core 2 out to the DACs (1/8" audio out connector)
		audiochannel_adau1761_0_left_out[i] =
				audiochannel_from_sharc_core2_0_left[i];
		audiochannel_adau1761_0_right_out[i] =
				audiochannel_from_sharc_core2_0_right[i];

		// Send audio from SHARC Core 2 to the SPDIF transmitter as well
		audiochannel_spdif_0_left_out[i] =
				audiochannel_from_sharc_core2_0_left[i];
		audiochannel_spdif_0_right_out[i] =
				audiochannel_from_sharc_core2_0_right[i];
#endif
	}
}
#endif

/*
 * This loop function is like a thread with a low priority.  This is good place to process
 * large FFTs in the background without interrupting the audio processing callback.
 */
void processaudio_background_loop(void) {

	// *******************************************************************************
	// Add any custom background processing here
	// *******************************************************************************

	//set button control
	if(multicore_data->audioproj_fin_sw_1_core1_pressed==true){
		multicore_data->audioproj_fin_sw_1_core1_pressed = false;
		type = (type+1)%4;	//cycle type button between 0 to 3
	}

	if(multicore_data->audioproj_fin_sw_2_core1_pressed==true){
		multicore_data->audioproj_fin_sw_2_core1_pressed = false;
		type2 = (type2+1)%4;	//cycle type button between 0 to 3
	}

	if(multicore_data->audioproj_fin_sw_3_core1_pressed==true){
		multicore_data->audioproj_fin_sw_3_core1_pressed = false;
		type3 = (type3+1)%4;	//cycle type button between 0 to 3
	}


	if(multicore_data->audioproj_fin_sw_4_core1_pressed==true){
		multicore_data->audioproj_fin_sw_4_core1_pressed = false;

		for(int i=0; i<=5; i++){
			keys[i].reset();
			tempAudio = 0;
			tempAudio2 = 0;
			tempAudio3 = 0;
			tempAudio4 = 0;
			tempAudio5 = 0;
		}
	}
}

/*
 * This function is called if the code in the audio processing callback takes too long
 * to complete (essentially exceeding the available computational resources of this core).
 */
void processaudio_mips_overflow(void) {
}
