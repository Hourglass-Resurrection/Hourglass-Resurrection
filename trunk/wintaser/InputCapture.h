#define DIRECTINPUT_VERSION 0x0500  // for joystick support
#define DI_KEY_PRESSED_FLAG 0x80 // To test if a DI key was pressed.
#define DI_KEY_NUMBER 256 // How many keys are there.

#include "../external/dinput.h"
#include "../external/Xinput.h"
#include <map>
using namespace std;

typedef enum
{
	CURRENT_INPUT_NONE,
	CURRENT_INPUT_DI,
	CURRENT_INPUT_XINPUT
} CURRENT_INPUT_DEVICE;

struct CurrentInput { // Can be extended with another 254 types since type can contain 256 different values
	CURRENT_INPUT_DEVICE type; // Will specify if the contents are VirtualKey data, Direct Input Data or XInput Data.
	union {
		struct { char keys[256]; DIMOUSESTATE mouse; DIJOYSTATE joypad[8]; } directInput; // <-- SIZE: (256 * 1) + (4 * 3) + (4 * 1) + ((4 * 8) + (4 * 4) + (32 * 1) * 8) = 576 bytes
		// ^^^^ We need to bundle JoyState with the rest to support multiplayer games where someone can use a joypad, another a keyboard etc.
		// We can use extended variants of MOUSESTATE and JOYSTATE which contain more buttons, do we want to? If the game supports it, so can we...
		// Extra size: MOUSESTATE2 +4 bytes        JOYSTATE2 +96 bytes (PER JOYPAD! So: Extra 768 bytes!)
		struct { XINPUT_GAMEPAD gamepad[4]; } xinput; // <-- SIZE: ((2 * 1) + (1 * 2) + (2 * 4) * 4) = 48 bytes
		// ^^^^ Only to make it conform to the format. Otherwise the struct is unnecessary.
	};

	// clear the whole structure
	void clear(){
		type = CURRENT_INPUT_NONE;
		for (int i=0; i<256; i++)
			directInput.keys[i] = 0;
		// TODO: the rest, I'm lazy...
	}

	// Right now, we are using a union, so we don't allow multiple devices.
	// We need to check if we can add a single input, based on what we already have inserted.
	bool isSourceCompatible(SINGLE_INPUT_DEVICE sid){
		switch(type){
		case CURRENT_INPUT_NONE: // We didn't insert anything yet.
			return true;
		case CURRENT_INPUT_DI:
			return ((sid == SINGLE_INPUT_DI_KEYBOARD) || (sid == SINGLE_INPUT_DI_MOUSE) || (sid == SINGLE_INPUT_DI_JOYSTICK));
		case CURRENT_INPUT_XINPUT:
			return (sid == SINGLE_INPUT_XINPUT_JOYSTICK);
		default:
			return false;
		}
	}

	// Usually called 
	void setTypeBasedOnSingleInput(SINGLE_INPUT_DEVICE sid){
		if(type != CURRENT_INPUT_NONE) // The type is already set.
			return;
		switch(sid){
		case SINGLE_INPUT_DI_KEYBOARD:
		case SINGLE_INPUT_DI_MOUSE:
		case SINGLE_INPUT_DI_JOYSTICK:
			type = CURRENT_INPUT_DI;
		case SINGLE_INPUT_XINPUT_JOYSTICK:
			type = CURRENT_INPUT_XINPUT;
		}
	}

}; // <-- TOTAL SIZE: 576 + 1 = 577 bytes (640 bytes with safe rounding)
   // Extending DirectInput to use MOUSESTATE2 and JOYSTATE2 instead will boost this to 1412 bytes. (2048 with very safe rounding)
   // This will most likely have a speed impact, specially on games that use the smaller size input types (like VirtualKeys or XInput) since we will copy basically 1.5 kilobytes of junk.
   // Movie frames will of course only contain the important parts, input type, and the data of the devices that has input.
   // The rest of the data will be padded (with 0s) as we feed the input to the game.


// Type of device
typedef enum
{
	SINGLE_INPUT_DI_KEYBOARD,
	SINGLE_INPUT_DI_MOUSE,
	SINGLE_INPUT_DI_JOYSTICK,
	SINGLE_INPUT_XINPUT_JOYSTICK
} SINGLE_INPUT_DEVICE;

// Structure that contains a single input from any type of device.
struct SingleInput
{
	SINGLE_INPUT_DEVICE device; // type of the device.
	short key; // which key/button was pressed. The structure depends on the device type.
	           // single key+modifiers for keyboard, single button for joystick.

	// We need to implement the < operator to use this struct in a map.
	bool operator<( const SingleInput &si ) const {
		return ((this->device < si.device) || ((this->device < si.device) && (this->key < si.key)));
	}
};

// Structure to hold the information about a modifier key.
struct ModifierKey
{
	char DIK; // DIK value of the modifier key
	char flag; // Corresponding flag
};





/* This class is responsible for capturing input from all input devices, and sharing them to the game.
 * 
 */
class InputCapture
{

private:
	LPDIRECTINPUT lpDI; // Direct Input controller.
	LPDIRECTINPUTDEVICE lpDIDKeyboard; // Direct Input keyboard.
	LPDIRECTINPUTDEVICE lpDIDMouse; // Direct Input mouse.
	static std::map<SingleInput,SingleInput> inputMapping; // mapping from a single input to a single input.
	static std::map<SingleInput,WORD> eventMapping; // mapping from a single input to an event.

	static struct ModifierKey modifierKeys[]; // List of modifier keys.
	static int modifierCount; // Number of modifier keys.

	// Return if the key is a modifier.
	bool isModifier(char key);

	// From a keyboard state, builds the char that contains the flags from all pressed modifier keys.
	char buildModifier(char* keys);

public:

	// Init all input devices it can.
	bool initInputs(HINSTANCE hInst, HWND hWnd);

	// Init a DI keyboard if possible
	HRESULT initDIKeyboard(HWND hWnd);

	// Init a DI mouse if possible
	HRESULT initDIMouse(HWND hWnd);

	// Release all inputs that have been acquired.
	void releaseInputs();

	// Add a map from a key+modifiers to a key.
	void AddKeyToKeyMapping(short modifiedKey, char destinationKey);

	// Add a map from a key+modifiers to an event.
	void AddKeyToEventMapping(short modifiedKey, WORD eventId);

	// Clear all input mappings
	void EmptyAllInputMappings();

	// Clear all event mappings
	void EmptyAllEventMappings();

	/* Big function here, should be called by wintaser (should be splitted up ?)
	 * - gather the key states from lpDIDKeyboard
	 * - compare to previousKeyStates to see which key were pressed (modifiers don't need this)
	 * - Build the list of modifiers+pressed_key for the following mapping
	 * - Gather the list of corresponding inputs/events after remapping
	 * - Execute all events
	 * - Build the InputState struct and return it

	 * Well, another possibility is to also return the list of events...
	 */
	void ProcessInputs(CurrentInput* currentI, HWND hWnd);

	// Returns the next single input pressed (+modifiers), is used inside the GUI mapping config.
	//unsigned short NextSingleInput();


	/* Mapping */

	// Build (or restore to) a default mapping.
	//void BuildDefaultMapping();

	// Save current mapping into a config file
	//void SaveMapping();

	// Load mapping from a config file
	//void LoadMapping();


}