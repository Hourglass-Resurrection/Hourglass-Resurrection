/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#include "../wintasee.h"
//#include "../tls.h"

#if defined(_DEBUG) || 0//0
    #define _DINPUTDEBUG
#endif

#if defined(_DINPUTDEBUG)
    #define dinputdebugprintf debugprintf
    #define DINPUT_ENTER ENTER
#else
    #define dinputdebugprintf(...) ((void)0)
    #define DINPUT_ENTER(...) ((void)0)
#endif

DEFINE_LOCAL_GUID(IID_IDirectInputDeviceA, 0x5944E680,0xC92E,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(IID_IDirectInputDeviceW, 0x5944E681,0xC92E,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(IID_IDirectInputDevice2A,0x5944E682,0xC92E,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(IID_IDirectInputDevice2W,0x5944E683,0xC92E,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(IID_IDirectInputDevice7A,0x57D7C6BC,0x2356,0x11D3,0x8E,0x9D,0x00,0xC0,0x4F,0x68,0x44,0xAE);
DEFINE_LOCAL_GUID(IID_IDirectInputDevice7W,0x57D7C6BD,0x2356,0x11D3,0x8E,0x9D,0x00,0xC0,0x4F,0x68,0x44,0xAE);
DEFINE_LOCAL_GUID(IID_IDirectInputDevice8A,0x54D41080,0xDC15,0x4833,0xA4,0x1B,0x74,0x8F,0x73,0xA3,0x81,0x79);
DEFINE_LOCAL_GUID(IID_IDirectInputDevice8W,0x54D41081,0xDC15,0x4833,0xA4,0x1B,0x74,0x8F,0x73,0xA3,0x81,0x79);

// Device GUIDs, re-defined as local GUIDs to avoid linker errors.
DEFINE_LOCAL_GUID(GUID_SysMouse,   0x6F1D2B60,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(GUID_SysKeyboard,0x6F1D2B61,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

// DeviceObject GUIDs, re-defined as local GUIDs to avoid linker errors.
DEFINE_LOCAL_GUID(GUID_XAxis,   0xA36D02E0,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(GUID_YAxis,   0xA36D02E1,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(GUID_ZAxis,   0xA36D02E2,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(GUID_RxAxis,  0xA36D02F4,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(GUID_RyAxis,  0xA36D02F5,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(GUID_RzAxis,  0xA36D02E3,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(GUID_Slider,  0xA36D02E4,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(GUID_Button,  0xA36D02F0,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(GUID_Key,     0x55728220,0xD33C,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(GUID_POV,     0xA36D02F2,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(GUID_Unknown, 0xA36D02F3,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

template<typename IDirectInputDeviceN> struct IDirectInputDeviceTraits {};
template<> struct IDirectInputDeviceTraits<IDirectInputDeviceA>   { typedef LPDIENUMDEVICEOBJECTSCALLBACKA LPDIENUMDEVICEOBJECTSCALLBACKN; typedef LPDIDEVICEINSTANCEA LPDIDEVICEINSTANCEN; typedef LPDIDEVICEOBJECTINSTANCEA LPDIDEVICEOBJECTINSTANCEN; typedef LPDIENUMEFFECTSCALLBACKA LPDIENUMEFFECTSCALLBACKN; typedef LPDIEFFECTINFOA LPDIEFFECTINFON; typedef LPDIACTIONFORMATA LPDIACTIONFORMATN; typedef LPDIDEVICEIMAGEINFOHEADERA LPDIDEVICEIMAGEINFOHEADERN; typedef LPCSTR  LPCNSTR; typedef CHAR  NCHAR; enum {defaultDIDEVICEOBJECTDATAsize = 16}; };
template<> struct IDirectInputDeviceTraits<IDirectInputDeviceW>   { typedef LPDIENUMDEVICEOBJECTSCALLBACKW LPDIENUMDEVICEOBJECTSCALLBACKN; typedef LPDIDEVICEINSTANCEW LPDIDEVICEINSTANCEN; typedef LPDIDEVICEOBJECTINSTANCEW LPDIDEVICEOBJECTINSTANCEN; typedef LPDIENUMEFFECTSCALLBACKW LPDIENUMEFFECTSCALLBACKN; typedef LPDIEFFECTINFOW LPDIEFFECTINFON; typedef LPDIACTIONFORMATW LPDIACTIONFORMATN; typedef LPDIDEVICEIMAGEINFOHEADERW LPDIDEVICEIMAGEINFOHEADERN; typedef LPCWSTR LPCNSTR; typedef WCHAR NCHAR; enum {defaultDIDEVICEOBJECTDATAsize = 16}; };
template<> struct IDirectInputDeviceTraits<IDirectInputDevice2A>  { typedef LPDIENUMDEVICEOBJECTSCALLBACKA LPDIENUMDEVICEOBJECTSCALLBACKN; typedef LPDIDEVICEINSTANCEA LPDIDEVICEINSTANCEN; typedef LPDIDEVICEOBJECTINSTANCEA LPDIDEVICEOBJECTINSTANCEN; typedef LPDIENUMEFFECTSCALLBACKA LPDIENUMEFFECTSCALLBACKN; typedef LPDIEFFECTINFOA LPDIEFFECTINFON; typedef LPDIACTIONFORMATA LPDIACTIONFORMATN; typedef LPDIDEVICEIMAGEINFOHEADERA LPDIDEVICEIMAGEINFOHEADERN; typedef LPCSTR  LPCNSTR; typedef CHAR  NCHAR; enum {defaultDIDEVICEOBJECTDATAsize = 16}; };
template<> struct IDirectInputDeviceTraits<IDirectInputDevice2W>  { typedef LPDIENUMDEVICEOBJECTSCALLBACKW LPDIENUMDEVICEOBJECTSCALLBACKN; typedef LPDIDEVICEINSTANCEW LPDIDEVICEINSTANCEN; typedef LPDIDEVICEOBJECTINSTANCEW LPDIDEVICEOBJECTINSTANCEN; typedef LPDIENUMEFFECTSCALLBACKW LPDIENUMEFFECTSCALLBACKN; typedef LPDIEFFECTINFOW LPDIEFFECTINFON; typedef LPDIACTIONFORMATW LPDIACTIONFORMATN; typedef LPDIDEVICEIMAGEINFOHEADERW LPDIDEVICEIMAGEINFOHEADERN; typedef LPCWSTR LPCNSTR; typedef WCHAR NCHAR; enum {defaultDIDEVICEOBJECTDATAsize = 16}; };
template<> struct IDirectInputDeviceTraits<IDirectInputDevice7A>  { typedef LPDIENUMDEVICEOBJECTSCALLBACKA LPDIENUMDEVICEOBJECTSCALLBACKN; typedef LPDIDEVICEINSTANCEA LPDIDEVICEINSTANCEN; typedef LPDIDEVICEOBJECTINSTANCEA LPDIDEVICEOBJECTINSTANCEN; typedef LPDIENUMEFFECTSCALLBACKA LPDIENUMEFFECTSCALLBACKN; typedef LPDIEFFECTINFOA LPDIEFFECTINFON; typedef LPDIACTIONFORMATA LPDIACTIONFORMATN; typedef LPDIDEVICEIMAGEINFOHEADERA LPDIDEVICEIMAGEINFOHEADERN; typedef LPCSTR  LPCNSTR; typedef CHAR  NCHAR; enum {defaultDIDEVICEOBJECTDATAsize = 16}; };
template<> struct IDirectInputDeviceTraits<IDirectInputDevice7W>  { typedef LPDIENUMDEVICEOBJECTSCALLBACKW LPDIENUMDEVICEOBJECTSCALLBACKN; typedef LPDIDEVICEINSTANCEW LPDIDEVICEINSTANCEN; typedef LPDIDEVICEOBJECTINSTANCEW LPDIDEVICEOBJECTINSTANCEN; typedef LPDIENUMEFFECTSCALLBACKW LPDIENUMEFFECTSCALLBACKN; typedef LPDIEFFECTINFOW LPDIEFFECTINFON; typedef LPDIACTIONFORMATW LPDIACTIONFORMATN; typedef LPDIDEVICEIMAGEINFOHEADERW LPDIDEVICEIMAGEINFOHEADERN; typedef LPCWSTR LPCNSTR; typedef WCHAR NCHAR; enum {defaultDIDEVICEOBJECTDATAsize = 16}; };
template<> struct IDirectInputDeviceTraits<IDirectInputDevice8A>  { typedef LPDIENUMDEVICEOBJECTSCALLBACKA LPDIENUMDEVICEOBJECTSCALLBACKN; typedef LPDIDEVICEINSTANCEA LPDIDEVICEINSTANCEN; typedef LPDIDEVICEOBJECTINSTANCEA LPDIDEVICEOBJECTINSTANCEN; typedef LPDIENUMEFFECTSCALLBACKA LPDIENUMEFFECTSCALLBACKN; typedef LPDIEFFECTINFOA LPDIEFFECTINFON; typedef LPDIACTIONFORMATA LPDIACTIONFORMATN; typedef LPDIDEVICEIMAGEINFOHEADERA LPDIDEVICEIMAGEINFOHEADERN; typedef LPCSTR  LPCNSTR; typedef CHAR  NCHAR; enum {defaultDIDEVICEOBJECTDATAsize = 20}; };
template<> struct IDirectInputDeviceTraits<IDirectInputDevice8W>  { typedef LPDIENUMDEVICEOBJECTSCALLBACKW LPDIENUMDEVICEOBJECTSCALLBACKN; typedef LPDIDEVICEINSTANCEW LPDIDEVICEINSTANCEN; typedef LPDIDEVICEOBJECTINSTANCEW LPDIDEVICEOBJECTINSTANCEN; typedef LPDIENUMEFFECTSCALLBACKW LPDIENUMEFFECTSCALLBACKN; typedef LPDIEFFECTINFOW LPDIEFFECTINFON; typedef LPDIACTIONFORMATW LPDIACTIONFORMATN; typedef LPDIDEVICEIMAGEINFOHEADERW LPDIDEVICEIMAGEINFOHEADERN; typedef LPCWSTR LPCNSTR; typedef WCHAR NCHAR; enum {defaultDIDEVICEOBJECTDATAsize = 20}; };

// The DIDEVICEOBJECTINSTANCE struct has to be locally re-implemented thanks to it being defined in dinput.h as 2 different structs,
// this creates a conflict for our catch-all model since only the *A-implementation gets used if we use the normal struct due to our
// compiler being set to MultiByte which undefines UNICODE.
// This templated implementation lets the struct become anamorphic and it can take the shape of either version of the struct.
// NCHAR will take the form of a CHAR or WCHAR depending on which version of the IDirectInputDevice that is created by the game.
// Since the struct will become an exact replica of either the ANSI or Unicode version, the game should experience no problems in
// accessing the data from this struct compared to a "real" one.
// -- Warepire
template<typename NCHAR>
struct MyDIDEVICEOBJECTINSTANCE {
	DWORD	dwSize;
	GUID	guidType;
	DWORD	dwOfs;
	DWORD	dwType;
	DWORD	dwFlags;
	NCHAR	tszName[MAX_PATH];
	DWORD	dwFFMaxForce;
	DWORD	dwFFForceResolution;
	WORD	wCollectionNumber;
	WORD	wDesignatorIndex;
	WORD	wUsagePage;
	WORD	wUsage;
	DWORD	dwDimension;
	WORD	wExponent;
	WORD	wReportId;
};

// Due to NCHAR being able to take the shape of both a CHAR and WCHAR we cannot assign it data in string format.
// This is caused by the difference in length of the 2 datatypes. We are however lucky enough that the values we
// want to assign are the same, so we will just initiate tszName as an array using hexadecimal values.
// To make the code somewhat easier to read I added these defines.
// -- Warepire
#define XAXIS	{ 0x58, 0x2D, 0x61, 0x78, 0x69, 0x73, 0x00 }			// "X-axis\0"
#define YAXIS	{ 0x59, 0x2D, 0x61, 0x78, 0x69, 0x73, 0x00 }			// "Y-axis\0"
#define WHEEL	{ 0x57, 0x68, 0x65, 0x65, 0x6C, 0x00 }					// "Wheel\0"
#define BUTTON0 { 0x42, 0x75, 0x74, 0x74, 0x6F, 0x6E, 0x20, 0x30, 0x00 }// "Button 0\0"
#define BUTTON1 { 0x42, 0x75, 0x74, 0x74, 0x6F, 0x6E, 0x20, 0x31, 0x00 }// "Button 1\0"
#define BUTTON2 { 0x42, 0x75, 0x74, 0x74, 0x6F, 0x6E, 0x20, 0x32, 0x00 }// "Button 2\0"

#include <vector>
#include <algorithm>
typedef std::vector<struct BufferedInput*> BufferedInputList;
static BufferedInputList s_bufferedKeySlots;

HOOKFUNC HKL WINAPI MyGetKeyboardLayout(DWORD idThread);

struct BufferedInput
{
	DIDEVICEOBJECTDATA* data;
	DWORD size;
	DWORD used;
	DWORD startOffset; // circular buffer
	BOOL overflowed; // for DI_BUFFEROVERFLOW
	HANDLE event;
	BufferedInputList& bufferList;

	BufferedInput(BufferedInputList& buflist) : data(NULL), size(0), used(0), startOffset(0), overflowed(FALSE), event(NULL), bufferList(buflist)
	{
		dinputdebugprintf(__FUNCTION__ "(0x%X) adding self to list.\n", this);
		bufferList.push_back(this);
	}
	~BufferedInput()
	{
		Resize(0);

		dinputdebugprintf(__FUNCTION__ "(0x%X) removing self from list.\n", this);
		bufferList.erase(std::remove(bufferList.begin(), bufferList.end(), this), bufferList.end());
	}
	void Resize(DWORD newSize)
	{
        DINPUT_ENTER(newSize);
		DWORD oldSize = size;
		size = newSize;

		if(oldSize != newSize)
		{
			dinputdebugprintf(__FUNCTION__ " allocating %u -> %u.\n", oldSize, newSize);
			data = (DIDEVICEOBJECTDATA*)realloc(data, newSize * sizeof(DIDEVICEOBJECTDATA));

			if(used > newSize)
			{
				used = newSize;
				overflowed = TRUE;
			}
		}
		dinputdebugprintf(__FUNCTION__ " done.\n");
	}
	HRESULT GetData(DWORD elemSize, LPDIDEVICEOBJECTDATA dataOut, LPDWORD numElements, DWORD flags)
	{
		DINPUT_ENTER(elemSize, dataOut, numElements, flags);
		if(!numElements)
			return DIERR_INVALIDPARAM;

		dinputdebugprintf(__FUNCTION__ " size=%d, used=%d, *numElements=%d\n", size, used, *numElements);

		DWORD retrieved = 0;
		DWORD requested = *numElements;
		DWORD newUsed = used;

		if(elemSize == sizeof(DIDEVICEOBJECTDATA))
		{
			while(requested && newUsed)
			{
				dinputdebugprintf(__FUNCTION__ " assigning %d bytes to 0x%X from 0x%X.\n", elemSize, &dataOut[retrieved], &data[(startOffset + retrieved) % size]);
				if(dataOut)
					dataOut[retrieved] = data[(startOffset + retrieved) % size];
				retrieved++;
				requested--;
				newUsed--;
			}
		}
		else
		{
			while(requested && newUsed)
			{
				dinputdebugprintf(__FUNCTION__ " copying %d bytes to 0x%X from 0x%X.\n", elemSize, &dataOut[retrieved], &data[(startOffset + retrieved) % size]);
				if(dataOut)
					memcpy(((char*)dataOut) + elemSize * retrieved, &data[(startOffset + retrieved) % size], elemSize);

				// if memcpy doesn't work
				//if(dataOut)
				//{
				//	DIDEVICEOBJECTDATA& to = *(DIDEVICEOBJECTDATA*)(((char*)dataOut) + elemSize * retrieved);
				//	DIDEVICEOBJECTDATA& from = data[(startOffset + retrieved) % size];
				//	if(elemSize >= 4)
				//		to.dwOfs = from.dwOfs;
				//	if(elemSize >= 8)
				//		to.dwData = from.dwData;
				//	if(elemSize >= 12)
				//		to.dwSequence = from.dwSequence;
				//	if(elemSize >= 16)
				//		to.dwTimeStamp = from.dwTimeStamp;
				//}

				DIDEVICEOBJECTDATA& to = *(DIDEVICEOBJECTDATA*)(((char*)dataOut) + elemSize * retrieved);
				dinputdebugprintf("BufferedInput::GotEvent(VK=0x??, DIK=0x%X, data=0x%X, id=0x%X) (used=%d)", to.dwOfs, to.dwData, to.dwSequence, newUsed);

				retrieved++;
				requested--;
				newUsed--;

				dinputdebugprintf("BufferedInput::GotEvent(VK=0x??, DIK=0x%X, data=0x%X, id=0x%X) (used=%d)", to.dwOfs, to.dwData, to.dwSequence, newUsed);
			}
		}

		HRESULT rv = overflowed ? DI_BUFFEROVERFLOW : DI_OK;
		*numElements = retrieved;

		if(!(flags & DIGDD_PEEK))
		{
			used = newUsed;
			startOffset += retrieved;
			if(retrieved)
			{
				overflowed = FALSE;
				if(used && event)
					SetEvent(event); // tells app that we still have more input, otherwise it will lag behind weirdly
			}
		}

		return rv;
	}
	void AddEvent(DIDEVICEOBJECTDATA inputEvent)
	{
		DINPUT_ENTER();
		if(used >= size)
			overflowed = TRUE;
		else
		{
			// FIXME: this should only happen for the keyboard device
			// convert event from VK to DIK
			HKL keyboardLayout = MyGetKeyboardLayout(0);
			int VK = inputEvent.dwOfs;
			int DIK = MapVirtualKeyEx(VK, /*MAPVK_VK_TO_VSC*/0, keyboardLayout) & 0xFF;
			inputEvent.dwOfs = DIK;

			dinputdebugprintf(__FUNCTION__ "(VK=0x%X, DIK=0x%X, data=0x%X, id=0x%X) (used=%d)\n", VK, DIK, inputEvent.dwData, inputEvent.dwSequence, used);

			data[(startOffset + used) % size] = inputEvent;
			used++;

			dinputdebugprintf(__FUNCTION__ "(VK=0x%X, DIK=0x%X, data=0x%X, id=0x%X) (used=%d)\n", VK, DIK, inputEvent.dwData, inputEvent.dwSequence, used);
		}
		if(event)
			SetEvent(event);
	}
	static void AddEventToAllDevices(DIDEVICEOBJECTDATA inputEvent, BufferedInputList& bufferList)
	{
		DINPUT_ENTER();
		for(int i = (int)bufferList.size()-1; i >= 0; i--)
			bufferList[i]->AddEvent(inputEvent);
	}
	void AddMouseEvent(DIDEVICEOBJECTDATA inputEvent)
	{
		DINPUT_ENTER();
		if(used >= size)
			overflowed = TRUE;
		else
		{
			dinputdebugprintf(__FUNCTION__ "(dwOfs=0x%X, data=0x%X, id=0x%X) (used=%d)\n", inputEvent.dwOfs, inputEvent.dwData, inputEvent.dwSequence, used);

			data[(startOffset + used) % size] = inputEvent;
			used++;
		}
		if(event)
			SetEvent(event);
	}
	static void AddMouseEventToAllDevices(DIDEVICEOBJECTDATA inputEvent, BufferedInputList& bufferList)
	{
		DINPUT_ENTER();
		for(int i = (int)bufferList.size()-1; i >= 0; i--)
			bufferList[i]->AddMouseEvent(inputEvent);
	}
};

// HACK: Something to init GUID to if it's not passed as a param to the class
// TODO: Implement the hooking in such a way that GUID is ALWAYS needed for this hook.
static const GUID emptyGUID = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

// TODO: more than keyboard
template<typename IDirectInputDeviceN>
class MyDirectInputDevice : public IDirectInputDeviceN
{
public:

	typedef typename IDirectInputDeviceTraits<IDirectInputDeviceN>::LPCNSTR LPCNSTR;
	typedef typename IDirectInputDeviceTraits<IDirectInputDeviceN>::LPDIENUMDEVICEOBJECTSCALLBACKN LPDIENUMDEVICEOBJECTSCALLBACKN;
	typedef typename IDirectInputDeviceTraits<IDirectInputDeviceN>::LPDIDEVICEOBJECTINSTANCEN LPDIDEVICEOBJECTINSTANCEN;
	typedef typename IDirectInputDeviceTraits<IDirectInputDeviceN>::LPDIDEVICEINSTANCEN LPDIDEVICEINSTANCEN;
	typedef typename IDirectInputDeviceTraits<IDirectInputDeviceN>::LPDIENUMEFFECTSCALLBACKN LPDIENUMEFFECTSCALLBACKN;
	typedef typename IDirectInputDeviceTraits<IDirectInputDeviceN>::LPDIEFFECTINFON LPDIEFFECTINFON;
	typedef typename IDirectInputDeviceTraits<IDirectInputDeviceN>::LPDIACTIONFORMATN LPDIACTIONFORMATN;
	typedef typename IDirectInputDeviceTraits<IDirectInputDeviceN>::LPDIDEVICEIMAGEINFOHEADERN LPDIDEVICEIMAGEINFOHEADERN;
	typedef typename IDirectInputDeviceTraits<IDirectInputDeviceN>::NCHAR NCHAR;

	MyDirectInputDevice(IDirectInputDeviceN* device) : m_device(device), m_type(emptyGUID), m_acquired(FALSE), m_bufferedInput(s_bufferedKeySlots)
	{
		debugprintf("MyDirectInputDevice created without GUID.\n");
	}

	MyDirectInputDevice(IDirectInputDeviceN* device, REFGUID guid) : m_device(device), m_type(guid), m_acquired(FALSE), m_bufferedInput(s_bufferedKeySlots)
	{
		debugprintf("MyDirectInputDevice created, received GUID: %Xl, %Xh, %Xh, %s.\n", guid.Data1, guid.Data2, guid.Data3, guid.Data4);
	}

	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj)
	{
		DINPUT_ENTER();
		HRESULT rv = m_device->QueryInterface(riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	STDMETHOD_(ULONG,AddRef)()
	{
		ULONG count = m_device->AddRef();
		dinputdebugprintf(__FUNCTION__ " called (returned %d).\n", count);
		return count;
	}

	STDMETHOD_(ULONG,Release)()
	{
		ULONG count = m_device->Release();
		dinputdebugprintf(__FUNCTION__ " called (returned %d).\n", count);
		if(0 == count)
			delete this;
		return count;
	}

	/*** IDirectInputDevice methods ***/
	STDMETHOD(GetCapabilities)(LPDIDEVCAPS lpDIDevCaps)
	{
		DINPUT_ENTER();
		if(m_type == GUID_SysMouse)
		{
			// This function requires that lpDIDevCaps exists and that it's dwSize member is initialized to either
			// sizeof(DIDEVCAPS_DX3) which is 24 bytes or sizeof(DIDEVCAPS) which is 44 bytes.
			if(lpDIDevCaps == NULL) return E_POINTER;
			if(lpDIDevCaps->dwSize != 24 && lpDIDevCaps->dwSize != 44) return DIERR_INVALIDPARAM;
			lpDIDevCaps->dwFlags = (DIDC_ATTACHED | DIDC_EMULATED);
			lpDIDevCaps->dwDevType = 0x112;
			lpDIDevCaps->dwAxes = 3;
			lpDIDevCaps->dwButtons = 3;
			lpDIDevCaps->dwPOVs = 0;
			if(lpDIDevCaps->dwSize == 44 /*sizeof(DIDEVCAPS)*/) // These are only defined in structs for DX-versions 5+
			{
				lpDIDevCaps->dwFFSamplePeriod = 0;
				lpDIDevCaps->dwFFMinTimeResolution = 0;
				lpDIDevCaps->dwFirmwareRevision = 0;
				lpDIDevCaps->dwHardwareRevision = 0;
				lpDIDevCaps->dwFFDriverVersion = 0;
			}
			return DI_OK;
		}
		//return rvfilter(m_device->GetCapabilities(devCaps));
		return DIERR_INVALIDPARAM; // NYI! for keyboard or gamepads
	}

	STDMETHOD(EnumObjects)(LPDIENUMDEVICEOBJECTSCALLBACKN lpCallback, LPVOID pvRef, DWORD dwFlags)	
	{
        DINPUT_ENTER(dwFlags);

		if(m_type == GUID_SysMouse)
		{
			// A SysMouse device follows a defined standard, this data is the same for any pointer device loaded in DI as a SysMouse.
			// Mice with more buttons/wheels may look differently after these objects, however, the array below represents a regular
			// 3 button mouse with a scroll wheel, and that is what we choose to emulate as mice without scroll wheels are very rare
			// in todays world, as far as I know anyway. This should be compatible with any mouse as long as it is loaded as SysMouse.
			// -- Warepire
			DWORD size = (sizeof(NCHAR) > sizeof(CHAR)) ? 576 : 316;
			struct MyDIDEVICEOBJECTINSTANCE<NCHAR> EmulatedSysMouse[6] = {
				{ size, GUID_XAxis,  0x0, 0x001, DIDOI_ASPECTPOSITION, XAXIS,   0, 0, 0, 0, 0, 0, 0, 0, 0 },
				{ size, GUID_YAxis,  0x4, 0x101, DIDOI_ASPECTPOSITION, YAXIS,   0, 0, 0, 0, 0, 0, 0, 0, 0 },
				{ size, GUID_ZAxis,  0x8, 0x201, DIDOI_ASPECTPOSITION, WHEEL,   0, 0, 0, 0, 0, 0, 0, 0, 0 },
				{ size, GUID_Button, 0xC, 0x301, 0,                    BUTTON0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
				{ size, GUID_Button, 0xD, 0x401, 0,                    BUTTON1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
				{ size, GUID_Button, 0xE, 0x501, 0,                    BUTTON2, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };

			// Requests that will return all the objects of our emulated mouse.
			// The if statement checks for these flags: DIDFT_ALL || DIDFT_ENUMCOLLECTION(0) || DIDFT_NOCOLLECTION
			// ALL is quite self-describing, the other 2 will both return all objects in HID collection 0 for this device.
			// Which means all objects since they all belong to HID collection 0.
			if((dwFlags == DIDFT_ALL) || (((dwFlags >> 8) & 0xFFFF) == 0x0000) || ((dwFlags & DIDFT_NOCOLLECTION) == DIDFT_NOCOLLECTION))
			{
				for(unsigned int i = 0; i < 6; i++)
				{
					if(lpCallback((LPDIDEVICEOBJECTINSTANCEN)(&(EmulatedSysMouse[i])), pvRef) == DIENUM_STOP) break;
				}
			}
			else // Requests that will return subsets of the objects.
			{
				if((dwFlags & DIDFT_RELAXIS) == DIDFT_RELAXIS)
				{
					for(unsigned int i = 0; i < 3; i++)
					{
						if(lpCallback((LPDIDEVICEOBJECTINSTANCEN)(&(EmulatedSysMouse[i])), pvRef) == DIENUM_STOP) break;
					}
				}
				if((dwFlags & DIDFT_PSHBUTTON) == DIDFT_PSHBUTTON)
				{
					for(unsigned int i = 3; i < 6; i++)
					{
						if(lpCallback((LPDIDEVICEOBJECTINSTANCEN)(&(EmulatedSysMouse[i])), pvRef) == DIENUM_STOP) break;
					}
				}

				// The flags DIDFT_AXIS and DIDFT_BUTTON can be ignored since they are pre-defined combinations of
				// DIDFT_ABSAXIS & DIDFT_RELAXIS and DIDFT_PSHBUTTON & DIDFT_TGLBUTTON respectively.

				// Objects our emulated mouse do not have, for these we will do nothing:
				// DIDFT_ABSAXIS
				// DIDFT_ALIAS
				// DIDFT_COLLECTION
				// DIDFT_FFACTUATOR
				// DIDFT_FFEFFECTTRIGGER
				// DIDFT_NODATA
				// DIDFT_OUTPUT
				// DIDFT_TGLBUTTON
				// DIDFT_VENDORDEFINED
			}
			return DI_OK;
		}
		if(m_type == GUID_SysKeyboard)
		{
			return DIERR_INVALIDPARAM; // TODO: NYI ... EnumObjects is relatively useless for keyboards since you cannot use it to check for available keyboard keys or LEDs.
		}
		return DIERR_INVALIDPARAM; // TODO: NYI ... Gamepads, wheels, joysticks etc, how do we handle this? There aren't any real standards for these...

		//return m_device->EnumObjects(callback, ref, flags);
		//return DIERR_INVALIDPARAM; // NYI!
	}

	STDMETHOD(GetProperty)(REFGUID rguid, LPDIPROPHEADER ph)
	{
		DINPUT_ENTER();
		//return rvfilter(m_device->GetProperty(rguid, ph));
		if(&rguid == &DIPROP_BUFFERSIZE)
		{
			DWORD& size = *(DWORD*)(ph + 1);
			size = m_bufferedInput.size;
			return DI_OK;
		}
		else
		{
			return DIERR_UNSUPPORTED;
		}
	}

	STDMETHOD(SetProperty)(REFGUID rguid, LPCDIPROPHEADER ph)
	{
		DINPUT_ENTER();
		//return m_device->SetProperty(rguid, ph);
		if(&rguid == &DIPROP_BUFFERSIZE)
		{
			DWORD size = *(DWORD*)(ph + 1);
			if(size > 1024)
				size = 1024;
			m_bufferedInput.Resize(size);
			return DI_OK;
		}
		else
		{
			return DIERR_UNSUPPORTED;
		}
	}

	STDMETHOD(Acquire)()
	{
		DINPUT_ENTER();
		//return m_device->Acquire();
		if(m_acquired)
			return DI_NOEFFECT;
		m_acquired = TRUE;
		return DI_OK;
	}

	STDMETHOD(Unacquire)()
	{
		DINPUT_ENTER();
		//return m_device->Unacquire();
		if(!m_acquired)
			return DI_NOEFFECT;
		m_acquired = FALSE;
		return DI_OK;
	}

	STDMETHOD(GetDeviceState)(DWORD size, LPVOID data)
	{
		DINPUT_ENTER();
		//return rvfilter(m_device->GetDeviceState(size, data));

		if(!m_acquired)
			return DIERR_NOTACQUIRED;

		// not-so-direct input
		// since the movie already has to store the VK constants,
		// try to convert those to DIK key state
		if(m_type == GUID_SysKeyboard)
		{
			HKL keyboardLayout = MyGetKeyboardLayout(0);

			if(size > 256)
				size = 256;

			BYTE* keys = (BYTE*)data;

			for(unsigned int i = 0; i < size; i++)
			{
				int DIK = i;
				int VK = MapVirtualKeyEx(DIK, /*MAPVK_VSC_TO_VK_EX*/3, keyboardLayout) & 0xFF;

				// unfortunately MapVirtualKeyEx is slightly broken, so patch up the results ourselves...
				// (note that some of the left/right modifier keys get lost too despite MAPVK_VSC_TO_VK_EX)
				switch(DIK)
				{
				case DIK_LEFT:    VK = VK_LEFT; break;
				case DIK_RIGHT:   VK = VK_RIGHT; break;
				case DIK_UP:      VK = VK_UP; break;
				case DIK_DOWN:    VK = VK_DOWN; break;
				case DIK_PRIOR:   VK = VK_PRIOR; break;
				case DIK_NEXT:    VK = VK_NEXT; break;
				case DIK_HOME:    VK = VK_HOME; break;
				case DIK_END:     VK = VK_END; break;
				case DIK_INSERT:  VK = VK_INSERT; break;
				case DIK_DELETE:  VK = VK_DELETE; break;
				case DIK_DIVIDE:  VK = VK_DIVIDE; break;
				case DIK_NUMLOCK: VK = VK_NUMLOCK; break;
				case DIK_LWIN:    VK = VK_LWIN; break;
				case DIK_RWIN:    VK = VK_RWIN; break;
				case DIK_RMENU:   VK = VK_RMENU; break;
				case DIK_RCONTROL:VK = VK_RCONTROL; break;
				// these work for me, but are here in case other layouts need them
				case DIK_RSHIFT:  VK = VK_RSHIFT; break;
				case DIK_LMENU:   VK = VK_LMENU; break;
				case DIK_LCONTROL:VK = VK_LCONTROL; break;
				case DIK_LSHIFT:  VK = VK_LSHIFT; break;
				}


				HOOKFUNC SHORT WINAPI MyGetKeyState(int vKey);

				keys[DIK] = (BYTE)(MyGetKeyState(VK) & 0xFF);

				if(keys[DIK] & 0x80)
					verbosedebugprintf("PRESSED: DIK 0x%X -> VK 0x%X", DIK, VK);
			}
			return DI_OK;
		}
		if(m_type == GUID_SysMouse)
		{
			// In the case of the game using DIMOUSESTATE2 we need to make sure the extra buttons are set to "idle" to avoid weird problems.
			if(size == sizeof(DIMOUSESTATE2))
			{
				((LPDIMOUSESTATE2)data)->rgbButtons[4] = 0;
				((LPDIMOUSESTATE2)data)->rgbButtons[5] = 0;
				((LPDIMOUSESTATE2)data)->rgbButtons[6] = 0;
				((LPDIMOUSESTATE2)data)->rgbButtons[7] = 0;
			}
			memcpy(data, &curinput.mouse.di, sizeof(DIMOUSESTATE));
			return DI_OK;
		}
		return E_PENDING;
	}

	STDMETHOD(GetDeviceData)(DWORD size, LPDIDEVICEOBJECTDATA data, LPDWORD numElements, DWORD flags)
	{
		DINPUT_ENTER();
		//return m_device->GetDeviceData(size, data, numElements, flags);
		if(!m_acquired)
			return DIERR_NOTACQUIRED;
		if(m_bufferedInput.size == 0)
			return DIERR_NOTBUFFERED;
		if(!size)
			size = IDirectInputDeviceTraits<IDirectInputDeviceN>::defaultDIDEVICEOBJECTDATAsize;
		return m_bufferedInput.GetData(size, data, numElements, flags);
	}

	STDMETHOD(SetDataFormat)(LPCDIDATAFORMAT lpdf)
	{
		DINPUT_ENTER();
		//return rvfilter(m_device->SetDataFormat(df));
		
		//debugprintf("df = 0x%X\n", df); // can't get at c_dfDIKeyboard... so do it at a lower level
		//debugprintf("dfnumobjs = %d\n", df->dwNumObjs);
		//for(unsigned int i = 0; i < df->dwNumObjs; i++)
		//{
		//	debugprintf("i = %d\n", i);
		//	debugprintf("ofs = 0x%X\n", df->rgodf[i].dwOfs);
		//	debugprintf("type = 0x%X\n", DIDFT_GETTYPE(df->rgodf[i].dwType));
		//	debugprintf("inst = 0x%X\n", DIDFT_GETINSTANCE(df->rgodf[i].dwType));
		//	debugprintf("guid = 0x%X\n", df->rgodf[i].pguid->Data1); // GUID_Key
		//	debugprintf("flags = 0x%X\n", df->rgodf[i].dwFlags);
		//}

		if(m_acquired)
			return DIERR_ACQUIRED;

		// NYI... assume 256 byte keyboard for now
		return DI_OK;
	}

	STDMETHOD(SetEventNotification)(HANDLE event)
	{
		DINPUT_ENTER();
		//return rvfilter(m_device->SetEventNotification(event));
		if(m_acquired)
			return DIERR_ACQUIRED;
		m_bufferedInput.event = event;
		return DI_OK;
	}

	STDMETHOD(SetCooperativeLevel)(HWND window, DWORD level)
	{
		DINPUT_ENTER();
		if(IsWindow(window))
			gamehwnd = window;
		if(m_type == GUID_SysMouse){
			cmdprintf("MOUSEREG: %d", window);
		}
		//return rvfilter(m_device->SetCooperativeLevel(window, level));
		return DI_OK;
	}

	STDMETHOD(GetObjectInfo)(LPDIDEVICEOBJECTINSTANCEN pdidoi, DWORD dwObj, DWORD dwHow)
	{
        DINPUT_ENTER(dwObj, dwHow);
		if(m_type == GUID_SysMouse)
		{
			// This function requires that pdidoi is created by the game, and has it's dwSize member inited to the size of the struct,
			// if the game passes a NULL pointer or a struct without the size member inited we cannot continue.
			if(pdidoi == NULL) return E_POINTER;
			DWORD zero = 0;
			if(memcmp(pdidoi, &zero, 4) == 0) return DIERR_INVALIDPARAM;
			switch(dwHow)
			{
				// Due to games being able to pass wrong values (either through bad code or bad coders) we cannot merge
				// DIPH_BYOFFSET & DIPH_BYID and handle them at the same time. So this massive block of code is required.
				case DIPH_BYOFFSET:
				{
					DWORD size;
					// Since our typedef system doesn't let us access the members of pdidoi, nor let us cast it to our struct, we have no choice but
					// to memcpy the data out of and into the struct, grabbing the size like this is mandatory because we need to leave it "untouched"
					// when we fill the rest of the struct.
					memcpy(&size, pdidoi, 4);
					switch(dwObj)
					{
						// Sadly the compiler does not support creating the object outside of the switch statement,
						// and then assigning all the values depending on case...
						case 0x0:
						{
							MyDIDEVICEOBJECTINSTANCE<NCHAR> xaxis = { size, GUID_XAxis, 0x0, 0x001, DIDOI_ASPECTPOSITION, XAXIS, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
							memcpy(pdidoi, &xaxis, xaxis.dwSize);
							break;
						}
						case 0x4:
						{
							MyDIDEVICEOBJECTINSTANCE<NCHAR> yaxis = { size, GUID_YAxis, 0x4, 0x101, DIDOI_ASPECTPOSITION, YAXIS, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
							memcpy(pdidoi, &yaxis, yaxis.dwSize);
							break;
						}
						case 0x8:
						{
							MyDIDEVICEOBJECTINSTANCE<NCHAR> wheel = { size, GUID_ZAxis, 0x8, 0x201, DIDOI_ASPECTPOSITION, WHEEL, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
							memcpy(pdidoi, &wheel, wheel.dwSize);
							break;
						}
						case 0xC:
						{
							MyDIDEVICEOBJECTINSTANCE<NCHAR> button0 = { size, GUID_Button, 0xC, 0x304, 0, BUTTON0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
							memcpy(pdidoi, &button0, button0.dwSize);
							break;
						}
						case 0xD:
						{
							MyDIDEVICEOBJECTINSTANCE<NCHAR> button1 = { size, GUID_Button, 0xD, 0x404, 0, BUTTON1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
							memcpy(pdidoi, &button1, button1.dwSize);
							break;
						}
						case 0xE:
						{
							MyDIDEVICEOBJECTINSTANCE<NCHAR> button2 = { size, GUID_Button, 0xE, 0x504, 0, BUTTON2, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
							memcpy(pdidoi, &button2, button2.dwSize);
							break;
						}
						default: return DIERR_OBJECTNOTFOUND;
					}
					break;
				}
				case DIPH_BYID:
				{
					DWORD size;
					memcpy(&size, pdidoi, 4);
					switch(dwObj)
					{
						case 0x001:
						{
							MyDIDEVICEOBJECTINSTANCE<NCHAR> xaxis = { size, GUID_XAxis, 0x0, 0x001, DIDOI_ASPECTPOSITION, XAXIS, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
							memcpy(pdidoi, &xaxis, xaxis.dwSize);
							break;
						}
						case 0x101:
						{
							MyDIDEVICEOBJECTINSTANCE<NCHAR> yaxis = { size, GUID_YAxis, 0x4, 0x101, DIDOI_ASPECTPOSITION, YAXIS, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
							memcpy(pdidoi, &yaxis, yaxis.dwSize);
							break;
						}
						case 0x201:
						{
							MyDIDEVICEOBJECTINSTANCE<NCHAR> wheel = { size, GUID_ZAxis, 0x8, 0x201, DIDOI_ASPECTPOSITION, WHEEL, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
							memcpy(pdidoi, &wheel, wheel.dwSize);
							break;
						}
						case 0x304:
						{
							MyDIDEVICEOBJECTINSTANCE<NCHAR> button0 = { size, GUID_Button, 0xC, 0x304, 0, BUTTON0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
							memcpy(pdidoi, &button0, button0.dwSize);
							break;
						}
						case 0x404:
						{
							MyDIDEVICEOBJECTINSTANCE<NCHAR> button1 = { size, GUID_Button, 0xD, 0x404, 0, BUTTON1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
							memcpy(pdidoi, &button1, button1.dwSize);
							break;
						}
						case 0x504:
						{
							MyDIDEVICEOBJECTINSTANCE<NCHAR> button2 = { size, GUID_Button, 0xE, 0x504, 0, BUTTON2, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
							memcpy(pdidoi, &button2, button2.dwSize);
							break;
						}
						default: return DIERR_OBJECTNOTFOUND;
					}
					break;
				}
				case DIPH_BYUSAGE:
				{
					// Despite MSDN giving a very detailed explanation on how to use this dwHow method it seems that for SysMouse it is not even
					// closely supported, and this is what the function returns for this case when I call it on every PC I own.
					// -- Warepire
					return E_NOTIMPL;
				}
			}
			return DI_OK;
		}
		if(m_type == GUID_SysKeyboard)
		{
			return DIERR_INVALIDPARAM; // NYI!
		}
		//return rvfilter(m_device->GetObjectInfo(object, objId, objHow));
		return DIERR_INVALIDPARAM; // NYI!
	}

	STDMETHOD(GetDeviceInfo)(LPDIDEVICEINSTANCEN di)
	{
		DINPUT_ENTER();
		//return rvfilter(m_device->GetDeviceInfo(di));
		return DIERR_INVALIDPARAM; // NYI!
	}

	STDMETHOD(RunControlPanel)(HWND owner, DWORD flags)
	{
		DINPUT_ENTER();
		//return rvfilter(m_device->RunControlPanel(owner, flags));
		return DI_OK;
	}

	STDMETHOD(Initialize)(HINSTANCE instance, DWORD version, REFGUID rguid)
	{
		DINPUT_ENTER();
		//return rvfilter(m_device->Initialize(instance, version, rguid));
		return DI_OK;
	}

	// DirectInputDevice2 methods
    STDMETHOD(CreateEffect)(REFGUID a, LPCDIEFFECT b, LPDIRECTINPUTEFFECT * c, LPUNKNOWN d)
	{
		DINPUT_ENTER();
		//return m_device->CreateEffect(a,b,c,d);
		return DIERR_DEVICEFULL;
	}
    STDMETHOD(EnumEffects)(LPDIENUMEFFECTSCALLBACKN a, LPVOID b, DWORD c)
	{
		DINPUT_ENTER();
		//return m_device->EnumEffects(a,b,c);
		return DI_OK;
	}
    STDMETHOD(GetEffectInfo)(LPDIEFFECTINFON a, REFGUID b)
	{
		DINPUT_ENTER();
		//return m_device->GetEffectInfo(a,b);
		return E_POINTER;
	}
    STDMETHOD(GetForceFeedbackState)(LPDWORD a)
	{
		DINPUT_ENTER();
		//return m_device->GetForceFeedbackState(a);
		return DIERR_UNSUPPORTED;
	}
    STDMETHOD(SendForceFeedbackCommand)(DWORD a)
	{
		DINPUT_ENTER();
		//return m_device->SendForceFeedbackCommand(a);
		return DIERR_UNSUPPORTED;
	}
    STDMETHOD(EnumCreatedEffectObjects)(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK a, LPVOID b, DWORD c)
	{
		DINPUT_ENTER();
		//return m_device->EnumCreatedEffectObjects(a,b,c);
		return DI_OK;
	}
    STDMETHOD(Escape)(LPDIEFFESCAPE a)
	{
		DINPUT_ENTER();
		//return m_device->Escape(a);
		return DI_OK;
	}
    STDMETHOD(Poll)()
	{
		DINPUT_ENTER();
		//return rvfilter(m_device->Poll());
		if(!m_acquired)
			return DIERR_NOTACQUIRED;
		return DI_NOEFFECT;
	}

	STDMETHOD(SendDeviceData)(DWORD a, LPCDIDEVICEOBJECTDATA b, LPDWORD c, DWORD d)
	{
		DINPUT_ENTER();
		//return rvfilter(m_device->SendDeviceData(a,b,c,d));
		return DI_OK; // according to the documentation, this function never does anything anyway and should not be called
	}
	// IDirectInputDevice7 methods
    STDMETHOD(EnumEffectsInFile)(LPCNSTR a, LPDIENUMEFFECTSINFILECALLBACK b, LPVOID c, DWORD d)
	{
		DINPUT_ENTER();
		//return m_device->EnumEffectsInFile(a,b,c,d);
		return DI_OK;
	}
    STDMETHOD(WriteEffectToFile)(LPCNSTR a, DWORD b, LPDIFILEEFFECT c, DWORD d)
	{
		DINPUT_ENTER();
		//return m_device->WriteEffectToFile(a,b,c,d);
		return DIERR_INVALIDPARAM; // more like DIERR_NYI
	}
	// IDirectInputDevice8 methods
    STDMETHOD(BuildActionMap)(LPDIACTIONFORMATN a, LPCNSTR b, DWORD c)
	{
		DINPUT_ENTER();
		//return m_device->BuildActionMap(a,b,c);
		return DIERR_MAPFILEFAIL; // more like DIERR_NYI
	}
    STDMETHOD(SetActionMap)(LPDIACTIONFORMATN a, LPCNSTR b, DWORD c)
	{
		DINPUT_ENTER();
		//return m_device->SetActionMap(a,b,c);
		return DIERR_INVALIDPARAM; // more like DIERR_NYI
	}
    STDMETHOD(GetImageInfo)(LPDIDEVICEIMAGEINFOHEADERN a)
	{
		DINPUT_ENTER();
		//return m_device->GetImageInfo(a);
		return DIERR_MAPFILEFAIL; // more like DIERR_NYI
	}

	//HRESULT rvfilter(HRESULT rv)
	//{
	////	if(rv == DIERR_INPUTLOST)
	////		return DI_OK;
	//	return rv;
	//}

private:
	IDirectInputDeviceN* m_device;
	REFGUID m_type;
	BufferedInput m_bufferedInput;
	BOOL m_acquired;
};

// methods DirectInputDevice doesn't implement
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceA>::CreateEffect(REFGUID a, LPCDIEFFECT b, LPDIRECTINPUTEFFECT * c, LPUNKNOWN d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceA>::EnumEffects(LPDIENUMEFFECTSCALLBACKN a, LPVOID b, DWORD c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceA>::GetEffectInfo(LPDIEFFECTINFON a, REFGUID b) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceA>::GetForceFeedbackState(LPDWORD a) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceA>::SendForceFeedbackCommand(DWORD a) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceA>::EnumCreatedEffectObjects(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK a, LPVOID b, DWORD c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceA>::Escape(LPDIEFFESCAPE a) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceA>::Poll() IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceA>::SendDeviceData(DWORD a, LPCDIDEVICEOBJECTDATA b, LPDWORD c, DWORD d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceW>::CreateEffect(REFGUID a, LPCDIEFFECT b, LPDIRECTINPUTEFFECT * c, LPUNKNOWN d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceW>::EnumEffects(LPDIENUMEFFECTSCALLBACKN a, LPVOID b, DWORD c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceW>::GetEffectInfo(LPDIEFFECTINFON a, REFGUID b) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceW>::GetForceFeedbackState(LPDWORD a) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceW>::SendForceFeedbackCommand(DWORD a) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceW>::EnumCreatedEffectObjects(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK a, LPVOID b, DWORD c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceW>::Escape(LPDIEFFESCAPE a) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceW>::Poll() IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceW>::SendDeviceData(DWORD a, LPCDIDEVICEOBJECTDATA b, LPDWORD c, DWORD d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceA>::EnumEffectsInFile(LPCNSTR a, LPDIENUMEFFECTSINFILECALLBACK b, LPVOID c, DWORD d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceA>::WriteEffectToFile(LPCNSTR a, DWORD b, LPDIFILEEFFECT c, DWORD d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceA>::BuildActionMap(LPDIACTIONFORMATN a, LPCNSTR b, DWORD c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceA>::SetActionMap(LPDIACTIONFORMATN a, LPCNSTR b, DWORD c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceA>::GetImageInfo(LPDIDEVICEIMAGEINFOHEADERN a) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceW>::EnumEffectsInFile(LPCNSTR a, LPDIENUMEFFECTSINFILECALLBACK b, LPVOID c, DWORD d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceW>::WriteEffectToFile(LPCNSTR a, DWORD b, LPDIFILEEFFECT c, DWORD d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceW>::BuildActionMap(LPDIACTIONFORMATN a, LPCNSTR b, DWORD c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceW>::SetActionMap(LPDIACTIONFORMATN a, LPCNSTR b, DWORD c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDeviceW>::GetImageInfo(LPDIDEVICEIMAGEINFOHEADERN a) IMPOSSIBLE_IMPL
// methods DirectInputDevice2 doesn't implement
template<> HRESULT MyDirectInputDevice<IDirectInputDevice2A>::EnumEffectsInFile(LPCNSTR a, LPDIENUMEFFECTSINFILECALLBACK b, LPVOID c, DWORD d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDevice2A>::WriteEffectToFile(LPCNSTR a, DWORD b, LPDIFILEEFFECT c, DWORD d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDevice2A>::BuildActionMap(LPDIACTIONFORMATN a, LPCNSTR b, DWORD c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDevice2A>::SetActionMap(LPDIACTIONFORMATN a, LPCNSTR b, DWORD c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDevice2A>::GetImageInfo(LPDIDEVICEIMAGEINFOHEADERN a) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDevice2W>::EnumEffectsInFile(LPCNSTR a, LPDIENUMEFFECTSINFILECALLBACK b, LPVOID c, DWORD d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDevice2W>::WriteEffectToFile(LPCNSTR a, DWORD b, LPDIFILEEFFECT c, DWORD d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDevice2W>::BuildActionMap(LPDIACTIONFORMATN a, LPCNSTR b, DWORD c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDevice2W>::SetActionMap(LPDIACTIONFORMATN a, LPCNSTR b, DWORD c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDevice2W>::GetImageInfo(LPDIDEVICEIMAGEINFOHEADERN a) IMPOSSIBLE_IMPL
// methods DirectInputDevice7 doesn't implement
template<> HRESULT MyDirectInputDevice<IDirectInputDevice7W>::BuildActionMap(LPDIACTIONFORMATN a, LPCNSTR b, DWORD c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDevice7W>::SetActionMap(LPDIACTIONFORMATN a, LPCNSTR b, DWORD c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDevice7W>::GetImageInfo(LPDIDEVICEIMAGEINFOHEADERN a) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDevice7A>::BuildActionMap(LPDIACTIONFORMATN a, LPCNSTR b, DWORD c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDevice7A>::SetActionMap(LPDIACTIONFORMATN a, LPCNSTR b, DWORD c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInputDevice<IDirectInputDevice7A>::GetImageInfo(LPDIDEVICEIMAGEINFOHEADERN a) IMPOSSIBLE_IMPL



DEFINE_LOCAL_GUID(IID_IDirectInputA,       0x89521360,0xAA8A,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(IID_IDirectInputW,       0x89521361,0xAA8A,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(IID_IDirectInput2A,      0x5944E662,0xAA8A,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(IID_IDirectInput2W,      0x5944E663,0xAA8A,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(IID_IDirectInput7A,      0x9A4CB684,0x236D,0x11D3,0x8E,0x9D,0x00,0xC0,0x4F,0x68,0x44,0xAE);
DEFINE_LOCAL_GUID(IID_IDirectInput7W,      0x9A4CB685,0x236D,0x11D3,0x8E,0x9D,0x00,0xC0,0x4F,0x68,0x44,0xAE);
DEFINE_LOCAL_GUID(IID_IDirectInput8A,      0xBF798030,0x483A,0x4DA2,0xAA,0x99,0x5D,0x64,0xED,0x36,0x97,0x00);
DEFINE_LOCAL_GUID(IID_IDirectInput8W,      0xBF798031,0x483A,0x4DA2,0xAA,0x99,0x5D,0x64,0xED,0x36,0x97,0x00);

template<typename IDirectInputN> struct IDirectInputTraits {};
template<> struct IDirectInputTraits<IDirectInputA>   { typedef IDirectInputDeviceA  IDirectInputDeviceN; typedef LPCDIDEVICEINSTANCEA LPCDIDEVICEINSTANCEN; typedef LPDIACTIONFORMATA LPDIACTIONFORMATN; typedef LPDIENUMDEVICESBYSEMANTICSCBA LPDIENUMDEVICESBYSEMANTICSCBN; typedef LPDICONFIGUREDEVICESPARAMSA LPDICONFIGUREDEVICESPARAMSN; typedef LPCSTR  LPCNSTR; };
template<> struct IDirectInputTraits<IDirectInputW>   { typedef IDirectInputDeviceW  IDirectInputDeviceN; typedef LPCDIDEVICEINSTANCEW LPCDIDEVICEINSTANCEN; typedef LPDIACTIONFORMATW LPDIACTIONFORMATN; typedef LPDIENUMDEVICESBYSEMANTICSCBW LPDIENUMDEVICESBYSEMANTICSCBN; typedef LPDICONFIGUREDEVICESPARAMSW LPDICONFIGUREDEVICESPARAMSN; typedef LPCWSTR LPCNSTR; };
template<> struct IDirectInputTraits<IDirectInput2A>  { typedef IDirectInputDeviceA  IDirectInputDeviceN; typedef LPCDIDEVICEINSTANCEA LPCDIDEVICEINSTANCEN; typedef LPDIACTIONFORMATA LPDIACTIONFORMATN; typedef LPDIENUMDEVICESBYSEMANTICSCBA LPDIENUMDEVICESBYSEMANTICSCBN; typedef LPDICONFIGUREDEVICESPARAMSA LPDICONFIGUREDEVICESPARAMSN; typedef LPCSTR  LPCNSTR; };
template<> struct IDirectInputTraits<IDirectInput2W>  { typedef IDirectInputDeviceW  IDirectInputDeviceN; typedef LPCDIDEVICEINSTANCEW LPCDIDEVICEINSTANCEN; typedef LPDIACTIONFORMATW LPDIACTIONFORMATN; typedef LPDIENUMDEVICESBYSEMANTICSCBW LPDIENUMDEVICESBYSEMANTICSCBN; typedef LPDICONFIGUREDEVICESPARAMSW LPDICONFIGUREDEVICESPARAMSN; typedef LPCWSTR LPCNSTR; };
template<> struct IDirectInputTraits<IDirectInput7A>  { typedef IDirectInputDeviceA  IDirectInputDeviceN; typedef LPCDIDEVICEINSTANCEA LPCDIDEVICEINSTANCEN; typedef LPDIACTIONFORMATA LPDIACTIONFORMATN; typedef LPDIENUMDEVICESBYSEMANTICSCBA LPDIENUMDEVICESBYSEMANTICSCBN; typedef LPDICONFIGUREDEVICESPARAMSA LPDICONFIGUREDEVICESPARAMSN; typedef LPCSTR  LPCNSTR; };
template<> struct IDirectInputTraits<IDirectInput7W>  { typedef IDirectInputDeviceW  IDirectInputDeviceN; typedef LPCDIDEVICEINSTANCEW LPCDIDEVICEINSTANCEN; typedef LPDIACTIONFORMATW LPDIACTIONFORMATN; typedef LPDIENUMDEVICESBYSEMANTICSCBW LPDIENUMDEVICESBYSEMANTICSCBN; typedef LPDICONFIGUREDEVICESPARAMSW LPDICONFIGUREDEVICESPARAMSN; typedef LPCWSTR LPCNSTR; };
template<> struct IDirectInputTraits<IDirectInput8A>  { typedef IDirectInputDevice8A IDirectInputDeviceN; typedef LPCDIDEVICEINSTANCEA LPCDIDEVICEINSTANCEN; typedef LPDIACTIONFORMATA LPDIACTIONFORMATN; typedef LPDIENUMDEVICESBYSEMANTICSCBA LPDIENUMDEVICESBYSEMANTICSCBN; typedef LPDICONFIGUREDEVICESPARAMSA LPDICONFIGUREDEVICESPARAMSN; typedef LPCSTR  LPCNSTR; };
template<> struct IDirectInputTraits<IDirectInput8W>  { typedef IDirectInputDevice8W IDirectInputDeviceN; typedef LPCDIDEVICEINSTANCEW LPCDIDEVICEINSTANCEN; typedef LPDIACTIONFORMATW LPDIACTIONFORMATN; typedef LPDIENUMDEVICESBYSEMANTICSCBW LPDIENUMDEVICESBYSEMANTICSCBN; typedef LPDICONFIGUREDEVICESPARAMSW LPDICONFIGUREDEVICESPARAMSN; typedef LPCWSTR LPCNSTR; };


template<typename IDirectInputN>
class MyDirectInput : public IDirectInputN
{
public:

	typedef typename IDirectInputTraits<IDirectInputN>::IDirectInputDeviceN IDirectInputDeviceN;
	typedef typename IDirectInputTraits<IDirectInputN>::LPCNSTR LPCNSTR;
	typedef typename IDirectInputTraits<IDirectInputN>::LPCDIDEVICEINSTANCEN LPCDIDEVICEINSTANCEN;
	typedef typename IDirectInputTraits<IDirectInputN>::LPDIACTIONFORMATN LPDIACTIONFORMATN;
	typedef typename IDirectInputTraits<IDirectInputN>::LPDIENUMDEVICESBYSEMANTICSCBN LPDIENUMDEVICESBYSEMANTICSCBN;
	typedef typename IDirectInputTraits<IDirectInputN>::LPDICONFIGUREDEVICESPARAMSN LPDICONFIGUREDEVICESPARAMSN;
	typedef BOOL (FAR PASCAL * LPDIENUMDEVICESCALLBACKN)(LPCDIDEVICEINSTANCEN, LPVOID);
	static const GUID IID_IDirectInputDeviceN;

	MyDirectInput(IDirectInputN* di) : m_di(di)
	{
		DINPUT_ENTER();
	}
	~MyDirectInput()
	{
		DINPUT_ENTER();
	}

	/*** IUnknown methods ***/
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj)
	{
		DINPUT_ENTER();
		HRESULT rv = m_di->QueryInterface(riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

    ULONG STDMETHODCALLTYPE AddRef()
	{
		DINPUT_ENTER();
		return m_di->AddRef();
	}

    ULONG STDMETHODCALLTYPE Release()
	{
		DINPUT_ENTER();
		ULONG count = m_di->Release();
		if(0 == count)
			delete this;

		return count;
	}

    /*** IDirectInputN methods ***/
    STDMETHOD(CreateDevice)(REFGUID rguid, IDirectInputDeviceN** device, LPUNKNOWN unknown)
	{
		DINPUT_ENTER();
		ThreadLocalStuff& curtls = tls;
		curtls.callerisuntrusted++;
		HRESULT hr = m_di->CreateDevice(rguid, device, unknown);
		if(SUCCEEDED(hr))
		{
			debugprintf("Hooking input device with GUID: %Xl, %Xh, %Xh, %s.\n", rguid.Data1, rguid.Data2, rguid.Data3, rguid.Data4);
			// Return our own keyboard device that checks for injected keypresses
			// (at least if rguid == GUID_SysKeyboard that's what it'll do)
			HookCOMInterfaceEx(IID_IDirectInputDeviceN, (LPVOID*)device, rguid);
		}
		curtls.callerisuntrusted--;

		return hr;
	}

    STDMETHOD(EnumDevices)(DWORD devType,LPDIENUMDEVICESCALLBACKN callback, LPVOID ref, DWORD flags)
	{
		DINPUT_ENTER();
		// FIXME: NYI.
		// this is leaking data to the game!
		// for now, let's at least untrust it.
		ThreadLocalStuff& curtls = tls;
		curtls.callerisuntrusted++;
		HRESULT rv = m_di->EnumDevices(devType, callback, ref, flags);
		curtls.callerisuntrusted--;
		return rv;
	}

    STDMETHOD(GetDeviceStatus)(REFGUID rguid)
	{
		DINPUT_ENTER();
		return m_di->GetDeviceStatus(rguid);
	}

    STDMETHOD(RunControlPanel)(HWND owner, DWORD flags)
	{
		DINPUT_ENTER();
		return m_di->RunControlPanel(owner, flags);
	}

    STDMETHOD(Initialize)(HINSTANCE instance, DWORD version)
	{
		DINPUT_ENTER();
		return m_di->Initialize(instance, version);
	}

	STDMETHOD(CreateDeviceEx)(REFGUID a, REFIID b, LPVOID* c, LPUNKNOWN d)
	{
        DINPUT_ENTER(a.Data1, b.Data1);
		ThreadLocalStuff& curtls = tls;
		curtls.callerisuntrusted++;
		HRESULT hr = m_di->CreateDeviceEx(a,b,c,d);
		if(SUCCEEDED(hr))
			HookCOMInterface(b, c);
		curtls.callerisuntrusted--;
		return hr;
	}

    STDMETHOD(FindDevice)(REFGUID a, LPCNSTR b, LPGUID c)
	{
		DINPUT_ENTER();
		return m_di->FindDevice(a,b,c);
	}
    STDMETHOD(EnumDevicesBySemantics)(LPCNSTR a, LPDIACTIONFORMATN b, LPDIENUMDEVICESBYSEMANTICSCBN c, LPVOID d, DWORD e)
	{
		DINPUT_ENTER();
		return m_di->EnumDevicesBySemantics(a,b,c,d,e);
	}
    STDMETHOD(ConfigureDevices)(LPDICONFIGUREDEVICESCALLBACK a, LPDICONFIGUREDEVICESPARAMSN b, DWORD c, LPVOID d)
	{
		DINPUT_ENTER();
		return m_di->ConfigureDevices(a,b,c,d);
	}

private:
	IDirectInputN* m_di;
};

// unimplemented methods for old versions of DirectInput that didn't have them
template<> HRESULT MyDirectInput<IDirectInputA>::FindDevice(REFGUID a, LPCNSTR b, LPGUID c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInputW>::FindDevice(REFGUID a, LPCNSTR b, LPGUID c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInputA>::EnumDevicesBySemantics(LPCNSTR a, LPDIACTIONFORMATN b, LPDIENUMDEVICESBYSEMANTICSCBN c, LPVOID d, DWORD e) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInputW>::EnumDevicesBySemantics(LPCNSTR a, LPDIACTIONFORMATN b, LPDIENUMDEVICESBYSEMANTICSCBN c, LPVOID d, DWORD e) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInputA>::ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK a, LPDICONFIGUREDEVICESPARAMSN b, DWORD c, LPVOID d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInputW>::ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK a, LPDICONFIGUREDEVICESPARAMSN b, DWORD c, LPVOID d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInputA>::CreateDeviceEx(REFGUID a, REFIID b, LPVOID* c, LPUNKNOWN d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInputW>::CreateDeviceEx(REFGUID a, REFIID b, LPVOID* c, LPUNKNOWN d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInput2A>::EnumDevicesBySemantics(LPCNSTR a, LPDIACTIONFORMATN b, LPDIENUMDEVICESBYSEMANTICSCBN c, LPVOID d, DWORD e) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInput2W>::EnumDevicesBySemantics(LPCNSTR a, LPDIACTIONFORMATN b, LPDIENUMDEVICESBYSEMANTICSCBN c, LPVOID d, DWORD e) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInput2A>::ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK a, LPDICONFIGUREDEVICESPARAMSN b, DWORD c, LPVOID d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInput2W>::ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK a, LPDICONFIGUREDEVICESPARAMSN b, DWORD c, LPVOID d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInput2A>::CreateDeviceEx(REFGUID a, REFIID b, LPVOID* c, LPUNKNOWN d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInput2W>::CreateDeviceEx(REFGUID a, REFIID b, LPVOID* c, LPUNKNOWN d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInput7A>::EnumDevicesBySemantics(LPCNSTR a, LPDIACTIONFORMATN b, LPDIENUMDEVICESBYSEMANTICSCBN c, LPVOID d, DWORD e) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInput7W>::EnumDevicesBySemantics(LPCNSTR a, LPDIACTIONFORMATN b, LPDIENUMDEVICESBYSEMANTICSCBN c, LPVOID d, DWORD e) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInput7A>::ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK a, LPDICONFIGUREDEVICESPARAMSN b, DWORD c, LPVOID d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInput7W>::ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK a, LPDICONFIGUREDEVICESPARAMSN b, DWORD c, LPVOID d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInput8A>::CreateDeviceEx(REFGUID a, REFIID b, LPVOID* c, LPUNKNOWN d) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectInput<IDirectInput8W>::CreateDeviceEx(REFGUID a, REFIID b, LPVOID* c, LPUNKNOWN d) IMPOSSIBLE_IMPL

template<> const GUID MyDirectInput<IDirectInputA>::IID_IDirectInputDeviceN = IID_IDirectInputDeviceA;
template<> const GUID MyDirectInput<IDirectInputW>::IID_IDirectInputDeviceN = IID_IDirectInputDeviceW;
template<> const GUID MyDirectInput<IDirectInput2A>::IID_IDirectInputDeviceN = IID_IDirectInputDeviceA;
template<> const GUID MyDirectInput<IDirectInput2W>::IID_IDirectInputDeviceN = IID_IDirectInputDeviceW;
template<> const GUID MyDirectInput<IDirectInput7A>::IID_IDirectInputDeviceN = IID_IDirectInputDeviceA;
template<> const GUID MyDirectInput<IDirectInput7W>::IID_IDirectInputDeviceN = IID_IDirectInputDeviceW;
template<> const GUID MyDirectInput<IDirectInput8A>::IID_IDirectInputDeviceN = IID_IDirectInputDevice8A;
template<> const GUID MyDirectInput<IDirectInput8W>::IID_IDirectInputDeviceN = IID_IDirectInputDevice8W;




int g_midFrameAsyncKeyRequests = 0;

HOOK_FUNCTION(SHORT, WINAPI, GetAsyncKeyState, int vKey)
HOOKFUNC SHORT WINAPI MyGetAsyncKeyState(int vKey)
{
//	return GetAsyncKeyState(vKey);
	debuglog(LCF_KEYBOARD|LCF_FREQUENT, __FUNCTION__ "(0x%X) called.\n", vKey);

	if(vKey < 0 || vKey > 255)
		return 0;

	g_midFrameAsyncKeyRequests++;
	if(g_midFrameAsyncKeyRequests >= 4096 && g_midFrameAsyncKeyRequests < 4096+512)
		return 0; // prevent freezing in games that pause until a key is released (e.g. IWBTG)

	if(vKey == VK_CAPITAL || vKey == VK_NUMLOCK || vKey == VK_SCROLL)
	{
		// special case for these because curinput stores the toggle status of these keys
		// whereas GetAsyncKeyState treats them like other keys
		unsigned char curbit = asynckeybit[vKey];
		if(curbit)
		{
			//if(s_frameThreadId == GetCurrentThreadId())
			//if(tls.isFrameThread)
			if(tls_IsPrimaryThread())
			{
				if(curbit < 16)
					asynckeybit[vKey]++;
				else
					asynckeybit[vKey] = 0;
			}
			return (curbit == 1) ? 0x8001 : 0x8000;
		}
		return 0;
	}

	if(asynckeybit[vKey]) // if the key has been pressed since the last call to this function
	{
		//if(s_frameThreadId == GetCurrentThreadId())
		//if(tls.isFrameThread)
		if(tls_IsPrimaryThread())
			asynckeybit[vKey] = 0;
		if(curinput.keys[vKey])
			return (SHORT)0x8001; // key is just now pressed
		return 1; // key is not held, but was pressed earlier
	}

	if(curinput.keys[vKey])
		return (SHORT)0x8000; // key is held
	return 0; // key is not held
}

static bool disableGetKeyStateLogging = false;

HOOK_FUNCTION(SHORT, WINAPI, GetKeyState, int vKey)
HOOKFUNC SHORT WINAPI MyGetKeyState(int vKey)
{
	// WARNING: PeekMessage SOMETIMES internally calls this function (both directly and indirectly),
	// so we must not change the state of anything in here.

//	return GetKeyState(vKey);

	SHORT rv;
	if(vKey == VK_CAPITAL || vKey == VK_NUMLOCK || vKey == VK_SCROLL)
	{
		// special case for these because curinput stores the toggle status of these keys
		// whereas GetKeyState (perhaps surprisingly) treats them like other keys
		if(curinput.keys[vKey] != previnput.keys[vKey])
			rv = curinput.keys[vKey] ? (SHORT)0xFF81 : (SHORT)0xFF80;
		else
			rv = curinput.keys[vKey] ? 1 : 0;
	}
	else
	{
		if(curinput.keys[vKey])
			rv = synckeybit[vKey] ? (SHORT)0xFF81 : (SHORT)0xFF80;
		else
			rv = synckeybit[vKey] ? 1 : 0;
	}

	if(!disableGetKeyStateLogging)
		debuglog(LCF_KEYBOARD|LCF_FREQUENT, __FUNCTION__ "(%d) called, returned 0x%X.\n", vKey, rv);

	return rv;
}
HOOK_FUNCTION(BOOL, WINAPI, GetKeyboardState, PBYTE lpKeyState)
HOOKFUNC BOOL WINAPI MyGetKeyboardState(PBYTE lpKeyState)
{
	// WARNING: PeekMessage SOMETIMES internally calls this function,
	// so we must not change the state of anything in here.
	// (MyPeekMessageA -> _PeekMessageA@20 -> _NtUserPeekMessage@20 -> _KiUserExceptionDispatcher@8 -> ___ClientImmProcessKey@4 -> _ImmProcessKey@20 -> MyGetKeyboardState)

//	return GetKeyboardState(lpKeyState);
	debuglog(LCF_KEYBOARD|LCF_FREQUENT, __FUNCTION__ " called.\n");
	if(!lpKeyState)
		return FALSE;
	disableGetKeyStateLogging = true;
	for(int i = 0; i < 256; i++)
		lpKeyState[i] = (BYTE)(MyGetKeyState(i) & 0xFF);
	disableGetKeyStateLogging = false;
	return TRUE;
}

static LASTINPUTINFO s_lii = {sizeof(LASTINPUTINFO)};

HOOK_FUNCTION(BOOL, WINAPI, GetLastInputInfo, PLASTINPUTINFO plii)
HOOKFUNC BOOL WINAPI MyGetLastInputInfo(PLASTINPUTINFO plii)
{
	debuglog(LCF_KEYBOARD|LCF_TIMEFUNC|LCF_TIMEGET|LCF_UNTESTED|LCF_FREQUENT, __FUNCTION__ " called.\n");
	//return GetLastInputInfo(plii);
	if(plii)
	{
		plii->dwTime = s_lii.dwTime;
		return TRUE;
	}
	return FALSE;
}

void ProcessFrameInput()
{
	static DWORD inputEventSequenceID = 0;

	// do some processing per key that changed states.
	// this is so MyGetAsyncKeyState can properly mimic GetAsyncKeyState's
	// return value pattern of 0x0000 -> 0x8001 -> 0x8000 when a key is pressed,
	// and also so directinput buffered keyboard input can work.
	for(DWORD i = 1; i < 256; i++)
	{
		if(curinput.keys[i] != previnput.keys[i])
		{
			debuglog(LCF_KEYBOARD, "key 0x%X, %d->%d\n", i, previnput.keys[i], curinput.keys[i]);

			if(i == VK_CAPITAL || i == VK_NUMLOCK || i == VK_SCROLL)
			{
				asynckeybit[i] = 1;
				synckeybit[i] = 1;
			}
			else
			{
				asynckeybit[i] = curinput.keys[i];
				synckeybit[i] = !synckeybit[i];
			}

			DWORD timeStamp = detTimer.GetTicks();
			s_lii.dwTime = timeStamp;

			__declspec(noinline) SHORT WINAPI MyGetKeyState(int vKey);
			DIDEVICEOBJECTDATA keyEvent = {i, static_cast<DWORD>(MyGetKeyState(i) & 0xFF), timeStamp, inputEventSequenceID++};
			BufferedInput::AddEventToAllDevices(keyEvent, s_bufferedKeySlots);
		}
	}

	// Send mouse events.
	if (curinput.mouse.di.lX != 0){
		DWORD timeStamp = detTimer.GetTicks();
		s_lii.dwTime = timeStamp;
		DIDEVICEOBJECTDATA mouseEvent = {DIMOFS_X, static_cast<DWORD>(curinput.mouse.di.lX), timeStamp, inputEventSequenceID++};
		BufferedInput::AddMouseEventToAllDevices(mouseEvent, s_bufferedKeySlots);
	}
	if (curinput.mouse.di.lY != 0){
		DWORD timeStamp = detTimer.GetTicks();
		s_lii.dwTime = timeStamp;
		DIDEVICEOBJECTDATA mouseEvent = {DIMOFS_Y, static_cast<DWORD>(curinput.mouse.di.lY), timeStamp, inputEventSequenceID++};
		BufferedInput::AddMouseEventToAllDevices(mouseEvent, s_bufferedKeySlots);
	}
	if (curinput.mouse.di.lZ != 0){
		DWORD timeStamp = detTimer.GetTicks();
		s_lii.dwTime = timeStamp;
		DIDEVICEOBJECTDATA mouseEvent = {DIMOFS_Z, static_cast<DWORD>(curinput.mouse.di.lZ), timeStamp, inputEventSequenceID++};
		BufferedInput::AddMouseEventToAllDevices(mouseEvent, s_bufferedKeySlots);
	}
	if (curinput.mouse.di.rgbButtons[0] != previnput.mouse.di.rgbButtons[0]){
		DWORD timeStamp = detTimer.GetTicks();
		s_lii.dwTime = timeStamp;
		DIDEVICEOBJECTDATA mouseEvent = {DIMOFS_BUTTON0, static_cast<DWORD>(curinput.mouse.di.rgbButtons[0]?0x80:0x00), timeStamp, inputEventSequenceID++};
		BufferedInput::AddMouseEventToAllDevices(mouseEvent, s_bufferedKeySlots);
	}
	if (curinput.mouse.di.rgbButtons[1] != previnput.mouse.di.rgbButtons[1]){
		DWORD timeStamp = detTimer.GetTicks();
		s_lii.dwTime = timeStamp;
		DIDEVICEOBJECTDATA mouseEvent = {DIMOFS_BUTTON1, static_cast<DWORD>(curinput.mouse.di.rgbButtons[1]?0x80:0x00), timeStamp, inputEventSequenceID++};
		BufferedInput::AddMouseEventToAllDevices(mouseEvent, s_bufferedKeySlots);
	}
	if (curinput.mouse.di.rgbButtons[2] != previnput.mouse.di.rgbButtons[2]){
		DWORD timeStamp = detTimer.GetTicks();
		s_lii.dwTime = timeStamp;
		DIDEVICEOBJECTDATA mouseEvent = {DIMOFS_BUTTON2, static_cast<DWORD>(curinput.mouse.di.rgbButtons[2]?0x80:0x00), timeStamp, inputEventSequenceID++};
		BufferedInput::AddMouseEventToAllDevices(mouseEvent, s_bufferedKeySlots);
	}
	/*if (curinput.mouse.rgbButtons[3] && !previnput.mouse.rgbButtons[3]){
		DWORD timeStamp = detTimer.GetTicks();
		s_lii.dwTime = timeStamp;
		DIDEVICEOBJECTDATA mouseEvent = {DIMOFS_BUTTON3, static_cast<DWORD>(0x80), timeStamp, inputEventSequenceID++};
		BufferedInput::AddMouseEventToAllDevices(mouseEvent, s_bufferedKeySlots);
	}*/

	/* Pass mouse cursor absolute coords.
	 * If no mouse event were recorded during the current frame,
	 * we have to pass the previous absolute coords into the current frame.
	 */

	bool isMouseUsed = (curinput.mouse.di.lX != 0) || (curinput.mouse.di.lY != 0) || (curinput.mouse.di.lZ != 0);
	for (int i=0; i<4; i++)
		isMouseUsed |= ((curinput.mouse.di.rgbButtons[i] & MOUSE_PRESSED_FLAG) != 0);

	if (!isMouseUsed)
		curinput.mouse.coords = previnput.mouse.coords;

}

HOOK_FUNCTION(MMRESULT, WINAPI, joyReleaseCapture, UINT uJoyID)
HOOKFUNC MMRESULT WINAPI MyjoyReleaseCapture(UINT uJoyID)
{
	debuglog(LCF_JOYPAD|LCF_UNTESTED, __FUNCTION__ " called (uJoyID=%d).\n", uJoyID);
return MMSYSERR_NODRIVER; // NYI
	//cmdprintf("WAITING: %u", 1);
	MMRESULT rv = joyReleaseCapture(uJoyID);
	//cmdprintf("WAITED: %u", 1);
	return rv;
}
HOOK_FUNCTION(MMRESULT, WINAPI, joySetCapture, HWND hwnd, UINT uJoyID, UINT uPeriod, BOOL fChanged)
HOOKFUNC MMRESULT WINAPI MyjoySetCapture(HWND hwnd, UINT uJoyID, UINT uPeriod, BOOL fChanged)
{
	debuglog(LCF_JOYPAD|LCF_UNTESTED, __FUNCTION__ " called (hwnd=0x%X, uJoyID=%d, uPeriod=%d, fChanged=%d).\n", hwnd, uJoyID, uPeriod, fChanged);
return MMSYSERR_NODRIVER; // NYI
	MMRESULT rv = joySetCapture(hwnd, uJoyID, uPeriod, fChanged);
	return rv;
}

HOOK_FUNCTION(MMRESULT, WINAPI, joyGetPosEx, UINT uJoyID, LPJOYINFOEX pji)
HOOKFUNC MMRESULT WINAPI MyjoyGetPosEx(UINT uJoyID, LPJOYINFOEX pji)
{
	debuglog(LCF_JOYPAD/*|LCF_UNTESTED*/, __FUNCTION__ " called (uJoyID=%d).\n", uJoyID);
return MMSYSERR_NODRIVER; // NYI

	char threadTypeName[64];
	sprintf(threadTypeName, "JoypadThread(%d)", uJoyID);
	tls.curThreadCreateName = threadTypeName;
debuglog(LCF_JOYPAD|LCF_UNTESTED, "in " __FUNCTION__ ", tls.curThreadCreateName = %s\n", tls.curThreadCreateName);

	MMRESULT rv = joyGetPosEx(uJoyID, pji);

	tls.curThreadCreateName = NULL;

	return rv;
//	return MMSYSERR_NODRIVER ;
}
HOOK_FUNCTION(MMRESULT, WINAPI, joyGetPos, UINT uJoyID, LPJOYINFO pji)
HOOKFUNC MMRESULT WINAPI MyjoyGetPos(UINT uJoyID, LPJOYINFO pji)
{
	debuglog(LCF_JOYPAD/*|LCF_UNTESTED*/, __FUNCTION__ " called (uJoyID=%d).\n", uJoyID);
return MMSYSERR_NODRIVER; // NYI
	char threadTypeName[64];
	sprintf(threadTypeName, "JoypadThread(%d)", uJoyID);
	tls.curThreadCreateName = threadTypeName;

	MMRESULT rv = joyGetPos(uJoyID, pji);

	tls.curThreadCreateName = NULL;

	return rv;
}

HOOK_FUNCTION(UINT, WINAPI, joyGetNumDevs)
HOOKFUNC UINT WINAPI MyjoyGetNumDevs()
{
	debuglog(LCF_JOYPAD|LCF_UNTESTED, __FUNCTION__ " called.\n");
return 0; // NYI
	UINT rv = joyGetNumDevs();
	return rv;
}
HOOK_FUNCTION(MMRESULT, WINAPI, joyGetDevCapsA, UINT_PTR uJoyID, LPJOYCAPSA pjc, UINT cbjc)
HOOKFUNC MMRESULT WINAPI MyjoyGetDevCapsA(UINT_PTR uJoyID, LPJOYCAPSA pjc, UINT cbjc)
{
	debuglog(LCF_JOYPAD|LCF_UNTESTED, __FUNCTION__ " called.\n");
return MMSYSERR_NODRIVER; // NYI
	MMRESULT rv = joyGetDevCapsA(uJoyID, pjc, cbjc);
	return rv;
}
HOOK_FUNCTION(MMRESULT, WINAPI, joyGetDevCapsW, UINT_PTR uJoyID, LPJOYCAPSW pjc, UINT cbjc)
HOOKFUNC MMRESULT WINAPI MyjoyGetDevCapsW(UINT_PTR uJoyID, LPJOYCAPSW pjc, UINT cbjc)
{
	debuglog(LCF_JOYPAD|LCF_UNTESTED, __FUNCTION__ " called.\n");
return MMSYSERR_NODRIVER; // NYI
	MMRESULT rv = joyGetDevCapsW(uJoyID, pjc, cbjc);
	return rv;
}


HOOK_FUNCTION(BOOL, WINAPI, GetCursorPos, LPPOINT lpPoint)
HOOKFUNC BOOL WINAPI MyGetCursorPos(LPPOINT lpPoint)
{
	if(!lpPoint) { return FALSE; }
	lpPoint->x = curinput.mouse.coords.x;
	lpPoint->y = curinput.mouse.coords.y;
	ClientToScreen(gamehwnd, lpPoint);
	//return GetCursorPos(lpPoint);
	return TRUE;
}

HOOK_FUNCTION(BOOL, WINAPI, GetCursorInfo, PCURSORINFO pci)
HOOKFUNC BOOL WINAPI MyGetCursorInfo(PCURSORINFO pci)
{
	if(!GetCursorInfo(pci)) { return FALSE; }
	return MyGetCursorPos(&pci->ptScreenPos);
}


HOOK_FUNCTION(HRESULT, WINAPI, DirectInputCreateA, HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTA *ppDI, LPUNKNOWN punkOuter)
HOOKFUNC HRESULT WINAPI MyDirectInputCreateA(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTA *ppDI, LPUNKNOWN punkOuter)
{
	debuglog(LCF_DINPUT, __FUNCTION__ " called.\n");
	ThreadLocalStuff& curtls = tls;
	const char* oldName = curtls.curThreadCreateName;
	curtls.curThreadCreateName = "DirectInput";
	curtls.callerisuntrusted++;
	HRESULT rv = DirectInputCreateA(hinst, dwVersion, ppDI, punkOuter);

	if(SUCCEEDED(rv))
	{
		if(dwVersion < 0x500)
			HookCOMInterface(IID_IDirectInputA, (LPVOID*)ppDI);
		else if(dwVersion < 0x700)
			HookCOMInterface(IID_IDirectInput2A, (LPVOID*)ppDI);
		else if(dwVersion < 0x800)
			HookCOMInterface(IID_IDirectInput7A, (LPVOID*)ppDI);
		else if(dwVersion < 0x900)
			HookCOMInterface(IID_IDirectInput8A, (LPVOID*)ppDI);
	}
	else
	{
		debuglog(LCF_DINPUT|LCF_ERROR, "DirectInputCreateA FAILED, all on its own.\n");
	}
	curtls.curThreadCreateName = oldName;
	curtls.callerisuntrusted--;

	return rv;
}
HOOK_FUNCTION(HRESULT, WINAPI, DirectInputCreateW, HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTW *ppDI, LPUNKNOWN punkOuter)
HOOKFUNC HRESULT WINAPI MyDirectInputCreateW(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUTW *ppDI, LPUNKNOWN punkOuter)
{
	debuglog(LCF_DINPUT, __FUNCTION__ " called.\n");
	ThreadLocalStuff& curtls = tls;
	const char* oldName = curtls.curThreadCreateName;
	curtls.curThreadCreateName = "DirectInput";
	curtls.callerisuntrusted++;
	HRESULT rv = DirectInputCreateW(hinst, dwVersion, ppDI, punkOuter);

	if(SUCCEEDED(rv))
	{
		if(dwVersion < 0x500)
			HookCOMInterface(IID_IDirectInputW, (LPVOID*)ppDI);
		else if(dwVersion < 0x700)
			HookCOMInterface(IID_IDirectInput2W, (LPVOID*)ppDI);
		else if(dwVersion < 0x800)
			HookCOMInterface(IID_IDirectInput7W, (LPVOID*)ppDI);
		else if(dwVersion < 0x900)
			HookCOMInterface(IID_IDirectInput8W, (LPVOID*)ppDI);
	}
	else
	{
		debuglog(LCF_DINPUT|LCF_ERROR, "DirectInputCreateW FAILED, all on its own.\n");
	}
	curtls.curThreadCreateName = oldName;
	curtls.callerisuntrusted--;

	return rv;
}

HOOK_FUNCTION(HRESULT, WINAPI, DirectInputCreateEx, HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter)
HOOKFUNC HRESULT WINAPI MyDirectInputCreateEx(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter)
{
	debuglog(LCF_DINPUT, __FUNCTION__ " called.\n");
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;
	const char* oldName = curtls.curThreadCreateName;
	curtls.curThreadCreateName = "directinputex";
	HRESULT rv = DirectInputCreateEx(hinst, dwVersion, riidltf, ppvOut, punkOuter);

	if(SUCCEEDED(rv))
	{
		HookCOMInterface(riidltf, ppvOut);
	}
	else
	{
		debuglog(LCF_DINPUT|LCF_ERROR, "DirectInputCreateEx FAILED, all on its own.\n");
	}
	curtls.curThreadCreateName = oldName;
	curtls.callerisuntrusted--;
	return rv;
}


HOOK_FUNCTION(HRESULT, WINAPI, DirectInput8Create, HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter)
HOOKFUNC HRESULT WINAPI MyDirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter)
{
	debuglog(LCF_DINPUT, __FUNCTION__ " called.\n");
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;
	const char* oldName = curtls.curThreadCreateName;
	curtls.curThreadCreateName = "directinput8";
	HRESULT rv = DirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);

	if(SUCCEEDED(rv))
	{
		HookCOMInterface(riidltf, ppvOut);
	}
	else
	{
		debuglog(LCF_DINPUT|LCF_ERROR, "DirectInput8Create FAILED, all on its own.\n");
	}
	curtls.curThreadCreateName = oldName;
	curtls.callerisuntrusted--;
	return rv;
}

bool HookCOMInterfaceInput(REFIID riid, LPVOID* ppvOut, bool uncheckedFastNew)
{
	switch(riid.Data1)
	{
		HOOKRIID(DirectInput,A);
		HOOKRIID(DirectInput,W);
		HOOKRIID(DirectInput,2A);
		HOOKRIID(DirectInput,2W);
		HOOKRIID(DirectInput,7A);
		HOOKRIID(DirectInput,7W);
		HOOKRIID(DirectInput,8A);
		HOOKRIID(DirectInput,8W);

		HOOKRIID2(DirectInputDeviceA, MyDirectInputDevice);
		HOOKRIID2(DirectInputDeviceW, MyDirectInputDevice);
		HOOKRIID2(DirectInputDevice2A, MyDirectInputDevice);
		HOOKRIID2(DirectInputDevice2W, MyDirectInputDevice);
		HOOKRIID2(DirectInputDevice7A, MyDirectInputDevice);
		HOOKRIID2(DirectInputDevice7W, MyDirectInputDevice);
		HOOKRIID2(DirectInputDevice8A, MyDirectInputDevice);
		HOOKRIID2(DirectInputDevice8W, MyDirectInputDevice);

		default: return false;
	}
	return true;
}

bool HookCOMInterfaceInputEx(REFIID riid, LPVOID* ppvOut, REFGUID parameter, bool uncheckedFastNew)
{
	switch(riid.Data1)
	{
		HOOKRIID(DirectInput,A);
		HOOKRIID(DirectInput,W);
		HOOKRIID(DirectInput,2A);
		HOOKRIID(DirectInput,2W);
		HOOKRIID(DirectInput,7A);
		HOOKRIID(DirectInput,7W);
		HOOKRIID(DirectInput,8A);
		HOOKRIID(DirectInput,8W);

		HOOKRIID2EX(DirectInputDeviceA, MyDirectInputDevice, parameter);
		HOOKRIID2EX(DirectInputDeviceW, MyDirectInputDevice, parameter);
		HOOKRIID2EX(DirectInputDevice2A, MyDirectInputDevice, parameter);
		HOOKRIID2EX(DirectInputDevice2W, MyDirectInputDevice, parameter);
		HOOKRIID2EX(DirectInputDevice7A, MyDirectInputDevice, parameter);
		HOOKRIID2EX(DirectInputDevice7W, MyDirectInputDevice, parameter);
		HOOKRIID2EX(DirectInputDevice8A, MyDirectInputDevice, parameter);
		HOOKRIID2EX(DirectInputDevice8W, MyDirectInputDevice, parameter);

		default: return false;
	}
	return true;
}

void ApplyInputIntercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		MAKE_INTERCEPT(1, DINPUT, DirectInputCreateA),
		MAKE_INTERCEPT(1, DINPUT, DirectInputCreateW),
		MAKE_INTERCEPT(1, DINPUT, DirectInputCreateEx),
		MAKE_INTERCEPT(1, DINPUT8, DirectInput8Create),
		MAKE_INTERCEPT(1, USER32, GetAsyncKeyState),
		MAKE_INTERCEPT(1, USER32, GetKeyState),
		MAKE_INTERCEPT(1, USER32, GetKeyboardState),
		MAKE_INTERCEPT(1, USER32, GetLastInputInfo),
		MAKE_INTERCEPT(1, WINMM, joyReleaseCapture),
		MAKE_INTERCEPT(1, WINMM, joySetCapture),
		MAKE_INTERCEPT(1, WINMM, joyGetPosEx),
		MAKE_INTERCEPT(1, WINMM, joyGetPos),
		MAKE_INTERCEPT(1, WINMM, joyGetNumDevs),
		MAKE_INTERCEPT(1, WINMM, joyGetDevCapsA),
		MAKE_INTERCEPT(1, WINMM, joyGetDevCapsW),
		MAKE_INTERCEPT(1, USER32, GetCursorPos),
		MAKE_INTERCEPT(1, USER32, GetCursorInfo),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}
