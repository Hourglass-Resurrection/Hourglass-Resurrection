#ifndef INPUT_CAPTURE_H
#define INPUT_CAPTURE_H

#define DIRECTINPUT_VERSION 0x0500  // for joystick support
#define DI_KEY_PRESSED_FLAG 0x80 // To test if a DI key was pressed.
#define DI_KEY_PRESSED(key) (key & DI_KEY_PRESSED_FLAG)
#define DI_KEY_NUMBER 256 // How many keys are there.

#include "../shared/input.h" // Contains the CurrentInput struct
#include <map>
using namespace std;


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

	char description[48]; // Text to display on a GUI.

	// We need to implement the < operator to use this struct in a map.
	bool operator<( const SingleInput &si ) const {
		return ((device < si.device) || ((device == si.device) && (key < si.key)));
	}
};



// Structure to hold the information about a modifier key.
struct ModifierKey
{
	unsigned char DIK; // DIK value of the modifier key
	unsigned char flag; // Corresponding flag
};


// Structure to hold the information about an event.
struct Event
{
	SingleInput defaultInput; // Default input mapped to this event.
	WORD id; // Id used to identify the event when sending a message.
	const char* description; // Text to display on a GUI.

	// We need to implement the < operator to use this struct in a map.
	bool operator<( const Event &ev ) const {
		return (id < ev.id);
	}
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
	static const int modifierCount = 8; // Number of modifier keys.

	static struct SingleInput SIList[]; // List of all inputs that can be mapped.
	static struct Event eventList[]; // List of all events that can be mapped.

	HWND hotkeysbox;
	HWND gameinputbox;

	// Return if the key is a modifier.
	bool IsModifier(char key);

	// From a keyboard state, builds the char that contains the flags from all pressed modifier keys.
	char BuildModifier(char* keys);

	// Convert a SingleInput struct to a string.
	void InputToDescription(SingleInput &si);

	// Get the current keyboard state.
	void GetKeyboardState(char* keys);

	// Get the next input pressed.
	void NextInput(SingleInput* si);

	// We need to be able to remove a map element based on the value. It doesn't need to be fast.
	void RemoveValueFromInputMap(const SingleInput* si);

	// Same for events
	void RemoveValueFromEventMap(const WORD eventId);

	// Used by the callback to populate the listboxes.
	void PopulateListbox(HWND listbox);

public:

	InputCapture::InputCapture();
	InputCapture::InputCapture(char* filename);

	static const int SICount = 144;
	static const int eventCount = 67;

	// Init all input devices it can.
	bool InitInputs(HINSTANCE hInst, HWND hWnd);

	// Init a DI keyboard if possible
	HRESULT InitDIKeyboard(HWND hWnd);

	// Init a DI mouse if possible
	HRESULT InitDIMouse(HWND hWnd);

	// Release all inputs that have been acquired.
	void ReleaseInputs();

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

	// Retreive the next single input pressed, and map it to the SingleInput whose index on the SIList is SIListIndex.
	void ReassignInput(int SIListIndex);

	// Set the single input to its default mapping.
	void DefaultInput(int SIListIndex);

	// Disable the single input.
	void DisableInput(int SIListIndex);

	// Retreive the next single input pressed, and map it to the event.
	void ReassignEvent(int eventListIndex);

	// Set the event to its default mapping.
	void DefaultEvent(int eventListIndex);

	// Disable the event.
	void DisableEvent(int eventListIndex);

	// Build (or restore to) a default input mapping.
	void BuildDefaultInputMapping();

	// Build (or restore to) a default event mapping.
	void BuildDefaultEventMapping();

	// Build the string with the input and its mapping.
	void FormatInputMapping(int index, char* from, char* to);

	// Build the string with the event and its mapping.
	void FormatEventMapping(int index, char* from, char* to);

	// Save current mapping into a config file
	void SaveMapping(char* filename);

	// Load mapping from a config file
	void LoadMapping(char* filename);

	// Callback to change config of all the input.
	// This SHOULD work according to the C++ FAQ Lite section 33.2
	static LRESULT CALLBACK ConfigureInput(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif
