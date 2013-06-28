#include "InputCapture.h"

struct ModifierKey InputCapture::modifierKeys[] = 
{
	{DIK_LCONTROL, 0x1},
	{DIK_RCONTROL, 0x2},
	{DIK_LSHIFT,   0x4},
	{DIK_RSHIFT,   0x8},
	{DIK_LMENU,    0x10},
	{DIK_RMENU,    0x20},
	{DIK_LWIN,     0x40},
	{DIK_RWIN,     0x80}
};

int InputCapture::modifierCount = 8;

bool InputCapture::isModifier(char key){
	for (int i=0; i<DI_KEY_NUMBER; i++){
		ModifierKey mkey = InputCapture::modifierKeys[i]; // Damn, no foreach structure in c++...
	}
}

char InputCapture::buildModifier(char* keys){
	char modifiers = 0;
	for (int i=0; i<DI_KEY_NUMBER; i++){
		ModifierKey mkey = InputCapture::modifierKeys[i]; // Damn, no foreach structure in c++...
		if (keys[mkey.DIK] & DI_KEY_PRESSED_FLAG){
			modifiers |= mkey.flag;
		}
	}
	return modifiers;
}

bool InputCapture::initInputs(HINSTANCE hInst, HWND hWnd){

	// Init the main DI interface.
	HRESULT rval = DirectInputCreate(hInst, DIRECTINPUT_VERSION, &lpDI, NULL);
	if(rval != DI_OK)
	{
		MessageBox(hWnd, "DirectInput failed... You must have at least DirectX 5", "Error", MB_OK);
		return false;
	}

	// Try to init a keyboard.
	rval = InputCapture::InitDIKeyboard(hWnd);

	// Is a keyboard mandatory ?
	if(rval != DI_OK)
	{
		MessageBox(hWnd, "I couldn't find any keyboard here", "Error", MB_OK);
		return false;
	}

	// Try to init a mouse.
	//rval = InitDIMouse(hWnd);

	return true;
}

HRESULT InputCapture::initDIKeyboard(HWND hWnd){
	HRESULT rval = lpDI->CreateDevice(GUID_SysKeyboard, &lpDIDKeyboard, NULL);
	if(rval != DI_OK) return rval;

	rval = lpDIDKeyboard->SetCooperativeLevel(hWnd, DISCL_NONEXCLUSIVE | (/*BackgroundInput*/true?DISCL_BACKGROUND:DISCL_FOREGROUND));
	if(rval != DI_OK) return rval;

	rval = lpDIDKeyboard->SetDataFormat(&c_dfDIKeyboard);
	if(rval != DI_OK) return rval;

	// Copy from original code. Why is there a loop here ???
	for(int i = 0; i < 10; i++)
	{
		rval = lpDIDKeyboard->Acquire();
		if (rval == DI_OK) break;
		Sleep(10);
	}

	return rval;
}

// Init a DI mouse if possible
HRESULT InputCapture::initDIMouse(HWND hWnd){
	HRESULT rval = lpDI->CreateDevice(GUID_SysMouse, &lpDIDMouse, NULL);
	if(rval != DI_OK) return rval;

	rval = lpDIDMouse->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
	if(rval != DI_OK) return rval;

	rval = lpDIDMouse->SetDataFormat(&c_dfDIMouse);
	if(rval != DI_OK) return rval;

	rval = lpDIDMouse->Acquire();
	return rval;
}

void InputCapture::releaseInputs(){

	if(lpDI){
		if(lpDIDMouse){
			inputState.lpDIDMouse->Release();
			inputState.lpDIDMouse = NULL;
		}

		if(inputState.lpDIDKeyboard){
			inputState.lpDIDKeyboard->Release();
			inputState.lpDIDKeyboard = NULL;
		}

		lpDI->Release();
		lpDI = NULL;
	}
}

void InputCapture::AddKeyToKeyMapping(short modifiedKey, char destinationKey){
	SingleInput mapFrom = {IC_INPUT_DI_KEYBOARD, modifiedKey};
	SingleInput mapTo = {IC_INPUT_DI_KEYBOARD, (short)destinationKey};
	inputMapping[mapFrom] = mapTo; // Cool syntax!
}

// Add a map from a key+modifiers to an event.
void InputCapture::AddKeyToEventMapping(short modifiedKey, WORD eventId){
	SingleInput mapFrom = {IC_INPUT_DI_KEYBOARD, modifiedKey};
	eventMapping[mapFrom] = eventId;
}

// Clear all input mappings
void InputCapture::EmptyAllInputMappings(){
	inputMapping.clear();
}

// Clear all event mappings
void InputCapture::EmptyAllEventMappings(){
	eventMapping.clear();
}

/* Big function here, should be called by wintaser (should be splitted up ?)
 * - gather the key states from lpDIDKeyboard
 * - iterate through all the entries in the maps, and search for matches
 * - insert matches of the input map into the CurrentInput structure
 * - execute the events of the event map matches
 */
void InputCapture::ProcessInputs(CurrentInput* currentI, HWND hWnd){

	// We first clear the CurrentInput object.
	currentI->clear();

	// Get the current keyboard state.
	char keys[256];
	HRESULT rval = InputCapture::lpDIDKeyboard->GetDeviceState(256, keys);

	if((rval == DIERR_INPUTLOST) || (rval == DIERR_NOTACQUIRED)){
		InputCapture::lpDIDKeyboard->Acquire();
		rval = InputCapture::lpDIDKeyboard->GetDeviceState(256, keys);
		if((rval == DIERR_INPUTLOST) || (rval == DIERR_NOTACQUIRED))
			// We couldn't get the state of the keyboard. Let's just say nothing was pressed.
			memset(keys, 0, sizeof(keys));
	}

	// Bulding the modifier from the array of pressed keys.
	char modifier = InputCapture::buildModifier(keys);

	// We want now to convert the initial inputs to their mapping.
	// There are two mappings: inputs and events.
	// We only need to consider the inputs that are actually mapped to something.
	// So it's better to iterate through the map than through the keys array (I guess).

	// Let's start with the input map first.
	for(std::map<SingleInput,SingleInput>::iterator iter = InputCapture::inputMapping.begin(); iter != InputCapture::inputMapping.end(); ++iter){
		SingleInput fromInput = iter->first;
		SingleInput toInput = iter->second;
		// We want to map fromInput -> toInput

		switch (fromInput.device){

		case SINGLE_INPUT_DI_KEYBOARD:{

			// Mapping source is a keyboard, we need to check if the modifiers are identical, and if the key is pressed.
			char fromModifier = (char)(fromInput.key >> 8);
			char fromKey = (char)(fromInput.key & 0xFF);
			if((fromModifier == modifier) && (keys[fromKey] & DI_KEY_PRESSED_FLAG)){
				// We have a match! Now we must insert the corresponding input into the CurrentInput object.

				if (! currentI->isSourceCompatible(toInput.device)){
					// This is a problem, we already have stored an input from another device.
					// Currently, we do not allow two inputs from two different devices to be passed.
					// To resolve this, I'm just gonna pass.
					break;
				}

				// If the device type is not yet set, set it.
				currentI->setTypeBasedOnSingleInput(toInput.device);

				// Store the input.
				currentI->directInput.keys[toInput.key] = 1;
			}
									  }
		case SINGLE_INPUT_DI_MOUSE:{
			// TODO
								   }

		case SINGLE_INPUT_DI_JOYSTICK:{
			// TODO
									  }

		case SINGLE_INPUT_XINPUT_JOYSTICK:{
			// TODO
										  }
		}
	}



	// Now doing the event map.
	// The code is almost the same, couldn't we factorise it? By using a single map?
	for(std::map<SingleInput,WORD>::iterator iter = InputCapture::eventMapping.begin(); iter != InputCapture::eventMapping.end(); ++iter){
		SingleInput fromInput = iter->first;
		WORD eventId = iter->second;
		// We want to map fromInput -> eventId

		switch (fromInput.device){

		case SINGLE_INPUT_DI_KEYBOARD:{

			// Mapping source is a keyboard, we need to check if the modifiers are identical, and if the key is pressed.
			char fromModifier = (char)(fromInput.key >> 8);
			char fromKey = (char)(fromInput.key & 0xFF);
			if((fromModifier == modifier) && (keys[fromKey] & DI_KEY_PRESSED_FLAG)){
				// We have a match! Now we just have to send the corresponding message.

				SendMessage(hWnd, WM_COMMAND, eventId, 777);
			}
									  }
		case SINGLE_INPUT_DI_MOUSE:{
			// TODO
								   }

		case SINGLE_INPUT_DI_JOYSTICK:{
			// TODO
									  }

		case SINGLE_INPUT_XINPUT_JOYSTICK:{
			// TODO
										  }
		}
	}


	// Returns 0 if the key is not a modifier, or the corresponding flag if it is.
	//char IsModifier(unsigned char key);

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