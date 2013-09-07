#ifndef INPUT_H_INCL
#define INPUT_H_INCL

#include "../external/dinput.h"
#include "../external/Xinput.h"

#define MOUSE_PRESSED_FLAG 0x80 // Flag saying if a DIMOUSESTATE button is pressed. FIXME: Put the correct value.

#define KEY_FLAG   0x00001
#define MOUSE_FLAG 0x00010
#define JOY_FLAG   0x00100 // There are 8 joysticks supported, so the 7 next flags are also reserved for the other joysticks
#define XJOY_FLAG  0x10000 // There are 4 xinput joysticks supported, so the 3 next flags are also reserved for the other xinput joysticks

struct CurrentInput {
	unsigned char keys[256]; // Contains 0/1, indexed by DIK_XXX. SIZE: 256 bytes
	DIMOUSESTATE mouse; // SIZE: 16 bytes
	DIJOYSTATE joypad[8]; // SIZE: ((4 * 8) + (4 * 4) + (32 * 1) * 8) = 304 bytes
	// We can use extended variants of MOUSESTATE and JOYSTATE which contain more buttons, do we want to? If the game supports it, so can we...
	// Extra size: MOUSESTATE2 +4 bytes        JOYSTATE2 +96 bytes (PER JOYPAD! So: Extra 768 bytes!)
	XINPUT_GAMEPAD gamepad[4]; // SIZE: 12 * 4 = 48 bytes

	// clear the whole structure
	void clear(){
		for (int i=0; i<256; i++)
			keys[i] = 0;
		mouse.lX = mouse.lY = mouse.lZ = 0;
		for (int i=0; i<4; i++)
			mouse.rgbButtons[i] = 0;
		// TODO: the rest, I'm lazy...
	}

	// Pack the full input state into an array of bytes, and returns the size of the array.
	int serialize(unsigned char* input_pack){

		// The full mask containing which input devices are not empty.
		// We are using a 4-bytes structure, allowing 32 devices.
		int mask = 0;

		/* Pack the keyboard */
		unsigned char key_list[256];
		unsigned char key_count = 0;
		for (int i=1; i<256; i++){ // We are starting at 1 because 0 is not a valid key code.
			if (keys[i] != 0){
				key_list[key_count++] = i;
			}
		}

		if (key_count > 0){ // We do have keys pressed.
			mask |= KEY_FLAG;
		}

		/* Pack the mouse */
		bool isMouseUsed = (mouse.lX != 0) || (mouse.lY != 0) || (mouse.lZ != 0);
		for (int i=0; i<4; i++)
			isMouseUsed |= ((mouse.rgbButtons[i] & MOUSE_PRESSED_FLAG) != 0);

		unsigned char mouse_packed[13]; // 4 bytes for each direction (X, Y, Z) and 1 byte for the buttons.
		if (isMouseUsed){
			memmove(mouse_packed, &mouse.lX, 4);
			memmove(mouse_packed+4, &mouse.lY, 4);
			memmove(mouse_packed+8, &mouse.lZ, 4);

			// We can pack the four button states into a single byte.
			unsigned char buttons = 0;
			for (int i=0; i<4; i++){
				if(mouse.rgbButtons[i] & MOUSE_PRESSED_FLAG)
					buttons |= (1 << i);
			}
			mouse_packed[12] = buttons;

			mask |= MOUSE_FLAG;
		}

		/* Write the whole input */
		
		// Write the mask.
		memmove(input_pack, &mask, 4);
		int current_pos = 4; // The current position into the buffer.

		// Write the keyboard.
		if (key_count > 0){
			// We first write the number of keys pressed, then the list of keys pressed.
			input_pack[current_pos++] = key_count;
			memmove(input_pack + current_pos, key_list, key_count);
			current_pos += key_count;
		}

		// Write the mouse.
		if (isMouseUsed){
			memmove(input_pack + current_pos, mouse_packed, 13);
			current_pos += 13;
		}

		// Write the end of buffer (do we need that ?).
		input_pack[current_pos] = '\0';

		return current_pos;
	}

	// Decode the byte array, and returns the number of bytes read.
	int unserialize(unsigned char* packed_input){

		clear();

		int mask;
		memmove(&mask, packed_input, 4);
		int current_pos = 4;

		if (mask & KEY_FLAG){ // A keyboard is present.
			unsigned char key_count = packed_input[current_pos++];
			for (int i=0; i<key_count; i++){
				keys[packed_input[current_pos++]] = 1;
			}
		}

		if (mask & MOUSE_FLAG){ // A mouse is present.
			memmove(&mouse.lX, packed_input+current_pos, 4);
			memmove(&mouse.lY, packed_input+current_pos+4, 4);
			memmove(&mouse.lZ, packed_input+current_pos+8, 4);
			current_pos += 12;

			unsigned char buttons = packed_input[current_pos++];
			for (int i=0; i<4; i++){
				if (buttons & (1 << i))
					mouse.rgbButtons[i] = MOUSE_PRESSED_FLAG;
			}
		}

		return current_pos;
	}

}; // <-- TOTAL SIZE: 624 bytes
   // Extending DirectInput to use MOUSESTATE2 and JOYSTATE2 instead will boost this to 1396 bytes. (2048 with very safe rounding)
   // This will most likely have a speed impact, specially on games that use the smaller size input types (like VirtualKeys or XInput) since we will copy basically 1.5 kilobytes of junk.
   // Movie frames will of course only contain the important parts, input type, and the data of the devices that has input.
   // The rest of the data will be padded (with 0s) as we feed the input to the game.

#endif