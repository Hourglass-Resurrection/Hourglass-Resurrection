#ifndef INPUT_H_INCL
#define INPUT_H_INCL

#include "../external/dinput.h"
#include "../external/Xinput.h"

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
		// TODO: the rest, I'm lazy...
	}
}; // <-- TOTAL SIZE: 624 bytes
   // Extending DirectInput to use MOUSESTATE2 and JOYSTATE2 instead will boost this to 1396 bytes. (2048 with very safe rounding)
   // This will most likely have a speed impact, specially on games that use the smaller size input types (like VirtualKeys or XInput) since we will copy basically 1.5 kilobytes of junk.
   // Movie frames will of course only contain the important parts, input type, and the data of the devices that has input.
   // The rest of the data will be padded (with 0s) as we feed the input to the game.

#endif