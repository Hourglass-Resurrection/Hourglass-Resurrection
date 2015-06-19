#pragma once

#include "../external/dinput.h"
#include "../external/Xinput.h"
#include "../wintaser/logging.h"
#define MOUSE_PRESSED_FLAG 0x80 // Flag saying if a DIMOUSESTATE button is pressed.

#define KEY_FLAG   0x00001
#define MOUSE_FLAG 0x00010
#define JOY_FLAG   0x00100 // There are 8 joysticks supported, so the 7 next flags are also reserved for the other joysticks
#define XJOY_FLAG  0x10000 // There are 4 xinput joysticks supported, so the 3 next flags are also reserved for the other xinput joysticks

struct CurrentInput {
	unsigned char keys[256]; // Contains 0/1, indexed by DIK_XXX. SIZE: 256 bytes
	struct { DIMOUSESTATE di; POINT coords; } mouse; // SIZE: 24 bytes
	DIJOYSTATE joypad[8]; // SIZE: ((4 * 8) + (4 * 4) + (32 * 1) * 8) = 304 bytes
	// We can use extended variants of MOUSESTATE and JOYSTATE which contain more buttons, do we want to? If the game supports it, so can we...
	// Extra size: MOUSESTATE2 +4 bytes        JOYSTATE2 +96 bytes (PER JOYPAD! So: Extra 768 bytes!)
	XINPUT_GAMEPAD gamepad[4]; // SIZE: 12 * 4 = 48 bytes

	// clear the whole structure
	void clear(){
		for (int i=0; i<256; i++)
			keys[i] = 0;
		mouse.di.lX = mouse.di.lY = mouse.di.lZ = mouse.coords.x = mouse.coords.y = 0;
		for (int i=0; i<4; i++)
			mouse.di.rgbButtons[i] = 0;
		memset(joypad, 0, 8*sizeof(DIJOYSTATE));
		memset(gamepad, 0, 4*sizeof(XINPUT_GAMEPAD));
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
		bool isMouseUsed = (mouse.di.lX != 0) || (mouse.di.lY != 0) || (mouse.di.lZ != 0);
		for (int i=0; i<4; i++)
			isMouseUsed |= ((mouse.di.rgbButtons[i] & MOUSE_PRESSED_FLAG) != 0);

		unsigned char mouse_packed[21]; // 4 bytes for each relative direction (X, Y, Z), 1 byte for the buttons and 8 bytes for the aboslute coords.
		if (isMouseUsed){
			memmove(mouse_packed, &mouse.di.lX, 4);
			memmove(mouse_packed+4, &mouse.di.lY, 4);
			memmove(mouse_packed+8, &mouse.di.lZ, 4);

			// We can pack the four button states into a single byte.
			unsigned char buttons = 0;
			for (int i=0; i<4; i++){
				if(mouse.di.rgbButtons[i] & MOUSE_PRESSED_FLAG)
					buttons |= (1 << i);
			}
			mouse_packed[12] = buttons;
			memmove(mouse_packed+13, &mouse.coords.x, 4);
			memmove(mouse_packed+17, &mouse.coords.y, 4);

			mask |= MOUSE_FLAG;
		}

		/* Pack the Xbox controllers */
		bool isXControllerUsed[4];
		for (int i=0; i<4; i++){
			isXControllerUsed[i] = (gamepad[i].wButtons != 0) || (gamepad[i].bLeftTrigger != 0) || (gamepad[i].bRightTrigger != 0) || (gamepad[i].sThumbLX != 0) || (gamepad[i].sThumbLY != 0) || (gamepad[i].sThumbRX != 0) || (gamepad[i].sThumbRY != 0);
			if (isXControllerUsed[i])
				mask |= (XJOY_FLAG << i);
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
			memmove(input_pack + current_pos, mouse_packed, 21);
			current_pos += 21;
		}

		// Write the XControllers
		for (int i=0; i<4; i++){
			if (isXControllerUsed[i]){
				memmove(input_pack + current_pos, &gamepad[i], sizeof(XINPUT_GAMEPAD));
				current_pos += sizeof(XINPUT_GAMEPAD);
			}
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
			memmove(&mouse.di.lX, packed_input+current_pos, 4);
			memmove(&mouse.di.lY, packed_input+current_pos+4, 4);
			memmove(&mouse.di.lZ, packed_input+current_pos+8, 4);
			current_pos += 12;

			unsigned char buttons = packed_input[current_pos++];
			for (int i=0; i<4; i++){
				if (buttons & (1 << i))
					mouse.di.rgbButtons[i] = MOUSE_PRESSED_FLAG;
			}
			memmove(&mouse.coords.x, packed_input+current_pos, 4);
			memmove(&mouse.coords.y, packed_input+current_pos+4, 4);
			current_pos += 8;
		}

		for (int i=0; i<4; i++){
			if (mask & (XJOY_FLAG << i)){ // The i-th XController is present.
				memmove(&gamepad[i],packed_input+current_pos, sizeof(XINPUT_GAMEPAD));
				current_pos += sizeof(XINPUT_GAMEPAD);
			}
		}

		return current_pos;
	}

}; // <-- TOTAL SIZE: 624 bytes
   // Extending DirectInput to use MOUSESTATE2 and JOYSTATE2 instead will boost this to 1396 bytes. (2048 with very safe rounding)
   // This will most likely have a speed impact, specially on games that use the smaller size input types (like VirtualKeys or XInput) since we will copy basically 1.5 kilobytes of junk.
   // Movie frames will of course only contain the important parts, input type, and the data of the devices that has input.
   // The rest of the data will be padded (with 0s) as we feed the input to the game.
