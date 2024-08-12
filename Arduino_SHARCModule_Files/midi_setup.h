/*
 * midi_setup.h
 *
 *  Created on: Sep 30, 2021
 *      Author: zzeng12
 */

#ifndef MIDI_SETUP_H_
#define MIDI_SETUP_H_
#include <math.h>

class Keyboard{

	public:
		int midiNote;
		bool playing;

		void reset(){
			midiNote = 0;
			playing = false;
			return;
		}
};


#endif /* MIDI_SETUP_H_ */
