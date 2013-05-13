/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(INPUTHOOKS_INCL) && !defined(UNITY_BUILD)
#define INPUTHOOKS_INCL

#define DIRECTINPUT_VERSION 0x0800
#include "../../external/dinput.h"
#include "../global.h"
#include "../wintasee.h"
#include "../tls.h"

DEFINE_LOCAL_GUID(IID_IDirectInputDeviceA, 0x5944E680,0xC92E,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(IID_IDirectInputDeviceW, 0x5944E681,0xC92E,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(IID_IDirectInputDevice2A,0x5944E682,0xC92E,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(IID_IDirectInputDevice2W,0x5944E683,0xC92E,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_LOCAL_GUID(IID_IDirectInputDevice7A,0x57D7C6BC,0x2356,0x11D3,0x8E,0x9D,0x00,0xC0,0x4F,0x68,0x44,0xAE);
DEFINE_LOCAL_GUID(IID_IDirectInputDevice7W,0x57D7C6BD,0x2356,0x11D3,0x8E,0x9D,0x00,0xC0,0x4F,0x68,0x44,0xAE);
DEFINE_LOCAL_GUID(IID_IDirectInputDevice8A,0x54D41080,0xDC15,0x4833,0xA4,0x1B,0x74,0x8F,0x73,0xA3,0x81,0x79);
DEFINE_LOCAL_GUID(IID_IDirectInputDevice8W,0x54D41081,0xDC15,0x4833,0xA4,0x1B,0x74,0x8F,0x73,0xA3,0x81,0x79);

template<typename IDirectInputDeviceN> struct IDirectInputDeviceTraits {};
template<> struct IDirectInputDeviceTraits<IDirectInputDeviceA>   { typedef LPDIENUMDEVICEOBJECTSCALLBACKA LPDIENUMDEVICEOBJECTSCALLBACKN; typedef LPDIDEVICEINSTANCEA LPDIDEVICEINSTANCEN; typedef LPDIDEVICEOBJECTINSTANCEA LPDIDEVICEOBJECTINSTANCEN; typedef LPDIENUMEFFECTSCALLBACKA LPDIENUMEFFECTSCALLBACKN; typedef LPDIEFFECTINFOA LPDIEFFECTINFON; typedef LPDIACTIONFORMATA LPDIACTIONFORMATN; typedef LPDIDEVICEIMAGEINFOHEADERA LPDIDEVICEIMAGEINFOHEADERN; typedef LPCSTR  LPCNSTR; enum {defaultDIDEVICEOBJECTDATAsize = 16}; };
template<> struct IDirectInputDeviceTraits<IDirectInputDeviceW>   { typedef LPDIENUMDEVICEOBJECTSCALLBACKW LPDIENUMDEVICEOBJECTSCALLBACKN; typedef LPDIDEVICEINSTANCEW LPDIDEVICEINSTANCEN; typedef LPDIDEVICEOBJECTINSTANCEW LPDIDEVICEOBJECTINSTANCEN; typedef LPDIENUMEFFECTSCALLBACKW LPDIENUMEFFECTSCALLBACKN; typedef LPDIEFFECTINFOW LPDIEFFECTINFON; typedef LPDIACTIONFORMATW LPDIACTIONFORMATN; typedef LPDIDEVICEIMAGEINFOHEADERW LPDIDEVICEIMAGEINFOHEADERN; typedef LPCWSTR LPCNSTR; enum {defaultDIDEVICEOBJECTDATAsize = 16}; };
template<> struct IDirectInputDeviceTraits<IDirectInputDevice2A>  { typedef LPDIENUMDEVICEOBJECTSCALLBACKA LPDIENUMDEVICEOBJECTSCALLBACKN; typedef LPDIDEVICEINSTANCEA LPDIDEVICEINSTANCEN; typedef LPDIDEVICEOBJECTINSTANCEA LPDIDEVICEOBJECTINSTANCEN; typedef LPDIENUMEFFECTSCALLBACKA LPDIENUMEFFECTSCALLBACKN; typedef LPDIEFFECTINFOA LPDIEFFECTINFON; typedef LPDIACTIONFORMATA LPDIACTIONFORMATN; typedef LPDIDEVICEIMAGEINFOHEADERA LPDIDEVICEIMAGEINFOHEADERN; typedef LPCSTR  LPCNSTR; enum {defaultDIDEVICEOBJECTDATAsize = 16}; };
template<> struct IDirectInputDeviceTraits<IDirectInputDevice2W>  { typedef LPDIENUMDEVICEOBJECTSCALLBACKW LPDIENUMDEVICEOBJECTSCALLBACKN; typedef LPDIDEVICEINSTANCEW LPDIDEVICEINSTANCEN; typedef LPDIDEVICEOBJECTINSTANCEW LPDIDEVICEOBJECTINSTANCEN; typedef LPDIENUMEFFECTSCALLBACKW LPDIENUMEFFECTSCALLBACKN; typedef LPDIEFFECTINFOW LPDIEFFECTINFON; typedef LPDIACTIONFORMATW LPDIACTIONFORMATN; typedef LPDIDEVICEIMAGEINFOHEADERW LPDIDEVICEIMAGEINFOHEADERN; typedef LPCWSTR LPCNSTR; enum {defaultDIDEVICEOBJECTDATAsize = 16}; };
template<> struct IDirectInputDeviceTraits<IDirectInputDevice7A>  { typedef LPDIENUMDEVICEOBJECTSCALLBACKA LPDIENUMDEVICEOBJECTSCALLBACKN; typedef LPDIDEVICEINSTANCEA LPDIDEVICEINSTANCEN; typedef LPDIDEVICEOBJECTINSTANCEA LPDIDEVICEOBJECTINSTANCEN; typedef LPDIENUMEFFECTSCALLBACKA LPDIENUMEFFECTSCALLBACKN; typedef LPDIEFFECTINFOA LPDIEFFECTINFON; typedef LPDIACTIONFORMATA LPDIACTIONFORMATN; typedef LPDIDEVICEIMAGEINFOHEADERA LPDIDEVICEIMAGEINFOHEADERN; typedef LPCSTR  LPCNSTR; enum {defaultDIDEVICEOBJECTDATAsize = 16}; };
template<> struct IDirectInputDeviceTraits<IDirectInputDevice7W>  { typedef LPDIENUMDEVICEOBJECTSCALLBACKW LPDIENUMDEVICEOBJECTSCALLBACKN; typedef LPDIDEVICEINSTANCEW LPDIDEVICEINSTANCEN; typedef LPDIDEVICEOBJECTINSTANCEW LPDIDEVICEOBJECTINSTANCEN; typedef LPDIENUMEFFECTSCALLBACKW LPDIENUMEFFECTSCALLBACKN; typedef LPDIEFFECTINFOW LPDIEFFECTINFON; typedef LPDIACTIONFORMATW LPDIACTIONFORMATN; typedef LPDIDEVICEIMAGEINFOHEADERW LPDIDEVICEIMAGEINFOHEADERN; typedef LPCWSTR LPCNSTR; enum {defaultDIDEVICEOBJECTDATAsize = 16}; };
template<> struct IDirectInputDeviceTraits<IDirectInputDevice8A>  { typedef LPDIENUMDEVICEOBJECTSCALLBACKA LPDIENUMDEVICEOBJECTSCALLBACKN; typedef LPDIDEVICEINSTANCEA LPDIDEVICEINSTANCEN; typedef LPDIDEVICEOBJECTINSTANCEA LPDIDEVICEOBJECTINSTANCEN; typedef LPDIENUMEFFECTSCALLBACKA LPDIENUMEFFECTSCALLBACKN; typedef LPDIEFFECTINFOA LPDIEFFECTINFON; typedef LPDIACTIONFORMATA LPDIACTIONFORMATN; typedef LPDIDEVICEIMAGEINFOHEADERA LPDIDEVICEIMAGEINFOHEADERN; typedef LPCSTR  LPCNSTR; enum {defaultDIDEVICEOBJECTDATAsize = 20}; };
template<> struct IDirectInputDeviceTraits<IDirectInputDevice8W>  { typedef LPDIENUMDEVICEOBJECTSCALLBACKW LPDIENUMDEVICEOBJECTSCALLBACKN; typedef LPDIDEVICEINSTANCEW LPDIDEVICEINSTANCEN; typedef LPDIDEVICEOBJECTINSTANCEW LPDIDEVICEOBJECTINSTANCEN; typedef LPDIENUMEFFECTSCALLBACKW LPDIENUMEFFECTSCALLBACKN; typedef LPDIEFFECTINFOW LPDIEFFECTINFON; typedef LPDIACTIONFORMATW LPDIACTIONFORMATN; typedef LPDIDEVICEIMAGEINFOHEADERW LPDIDEVICEIMAGEINFOHEADERN; typedef LPCWSTR LPCNSTR; enum {defaultDIDEVICEOBJECTDATAsize = 20}; };

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
		dinputdebugprintf(__FUNCTION__ "(%d) called.\n", newSize);
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
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//dinputdebugprintf(__FUNCTION__ " numElements=0x%X, dataOut=0x%X\n", numElements, dataOut);
		if(!numElements)
			return DIERR_INVALIDPARAM;

		dinputdebugprintf(__FUNCTION__ " size=%d, used=%d, elemSize=%d, *numElements=%d, flags=%d\n", size,used,elemSize,*numElements,flags);

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
		dinputdebugprintf(__FUNCTION__ " called.\n");
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
		dinputdebugprintf(__FUNCTION__ " called.\n");
		for(int i = (int)bufferList.size()-1; i >= 0; i--)
			bufferList[i]->AddEvent(inputEvent);
	}
};

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

	MyDirectInputDevice(IDirectInputDeviceN* device) : m_device(device), m_acquired(FALSE), m_bufferedInput(s_bufferedKeySlots)
	{
		debugprintf("MyDirectInputDevice created.\n");
	}

	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)(REFIID riid, void** ppvObj)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
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
	STDMETHOD(GetCapabilities)(LPDIDEVCAPS devCaps)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return rvfilter(m_device->GetCapabilities(devCaps));
		return DIERR_INVALIDPARAM; // NYI!
	}

	STDMETHOD(EnumObjects)(LPDIENUMDEVICEOBJECTSCALLBACKN callback, LPVOID ref, DWORD flags)	
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return m_device->EnumObjects(callback, ref, flags);
		return DIERR_INVALIDPARAM; // NYI!
	}

	STDMETHOD(GetProperty)(REFGUID rguid, LPDIPROPHEADER ph)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
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
		dinputdebugprintf(__FUNCTION__ " called.\n");
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
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return m_device->Acquire();
		if(m_acquired)
			return DI_NOEFFECT;
		m_acquired = TRUE;
		return DI_OK;
	}

	STDMETHOD(Unacquire)()
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return m_device->Unacquire();
		if(!m_acquired)
			return DI_NOEFFECT;
		m_acquired = FALSE;
		return DI_OK;
	}

	STDMETHOD(GetDeviceState)(DWORD size, LPVOID data)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return rvfilter(m_device->GetDeviceState(size, data));

		if(!m_acquired)
			return DIERR_NOTACQUIRED;

		// not-so-direct input
		// since the movie already has to store the VK constants,
		// try to convert those to DIK key state

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

	STDMETHOD(GetDeviceData)(DWORD size, LPDIDEVICEOBJECTDATA data, LPDWORD numElements, DWORD flags)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return m_device->GetDeviceData(size, data, numElements, flags);
		if(!m_acquired)
			return DIERR_NOTACQUIRED;
		if(m_bufferedInput.size == 0)
			return DIERR_NOTBUFFERED;
		if(!size)
			size = IDirectInputDeviceTraits<IDirectInputDeviceN>::defaultDIDEVICEOBJECTDATAsize;
		return m_bufferedInput.GetData(size, data, numElements, flags);
	}

	STDMETHOD(SetDataFormat)(LPCDIDATAFORMAT df)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
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
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return rvfilter(m_device->SetEventNotification(event));
		if(m_acquired)
			return DIERR_ACQUIRED;
		m_bufferedInput.event = event;
		return DI_OK;
	}

	STDMETHOD(SetCooperativeLevel)(HWND window, DWORD level)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		if(IsWindow(window))
			gamehwnd = window;
		//return rvfilter(m_device->SetCooperativeLevel(window, level));
		return DI_OK;
	}

	STDMETHOD(GetObjectInfo)(LPDIDEVICEOBJECTINSTANCEN object, DWORD objId, DWORD objHow)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return rvfilter(m_device->GetObjectInfo(object, objId, objHow));
		return DIERR_INVALIDPARAM; // NYI!
	}

	STDMETHOD(GetDeviceInfo)(LPDIDEVICEINSTANCEN di)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return rvfilter(m_device->GetDeviceInfo(di));
		return DIERR_INVALIDPARAM; // NYI!
	}

	STDMETHOD(RunControlPanel)(HWND owner, DWORD flags)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return rvfilter(m_device->RunControlPanel(owner, flags));
		return DI_OK;
	}

	STDMETHOD(Initialize)(HINSTANCE instance, DWORD version, REFGUID rguid)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return rvfilter(m_device->Initialize(instance, version, rguid));
		return DI_OK;
	}

	// DirectInputDevice2 methods
    STDMETHOD(CreateEffect)(REFGUID a, LPCDIEFFECT b, LPDIRECTINPUTEFFECT * c, LPUNKNOWN d)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return m_device->CreateEffect(a,b,c,d);
		return DIERR_DEVICEFULL;
	}
    STDMETHOD(EnumEffects)(LPDIENUMEFFECTSCALLBACKN a, LPVOID b, DWORD c)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return m_device->EnumEffects(a,b,c);
		return DI_OK;
	}
    STDMETHOD(GetEffectInfo)(LPDIEFFECTINFON a, REFGUID b)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return m_device->GetEffectInfo(a,b);
		return E_POINTER;
	}
    STDMETHOD(GetForceFeedbackState)(LPDWORD a)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return m_device->GetForceFeedbackState(a);
		return DIERR_UNSUPPORTED;
	}
    STDMETHOD(SendForceFeedbackCommand)(DWORD a)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return m_device->SendForceFeedbackCommand(a);
		return DIERR_UNSUPPORTED;
	}
    STDMETHOD(EnumCreatedEffectObjects)(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK a, LPVOID b, DWORD c)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return m_device->EnumCreatedEffectObjects(a,b,c);
		return DI_OK;
	}
    STDMETHOD(Escape)(LPDIEFFESCAPE a)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return m_device->Escape(a);
		return DI_OK;
	}
    STDMETHOD(Poll)()
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return rvfilter(m_device->Poll());
		if(!m_acquired)
			return DIERR_NOTACQUIRED;
		return DI_NOEFFECT;
	}

	STDMETHOD(SendDeviceData)(DWORD a, LPCDIDEVICEOBJECTDATA b, LPDWORD c, DWORD d)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return rvfilter(m_device->SendDeviceData(a,b,c,d));
		return DI_OK; // according to the documentation, this function never does anything anyway and should not be called
	}
	// IDirectInputDevice7 methods
    STDMETHOD(EnumEffectsInFile)(LPCNSTR a, LPDIENUMEFFECTSINFILECALLBACK b, LPVOID c, DWORD d)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return m_device->EnumEffectsInFile(a,b,c,d);
		return DI_OK;
	}
    STDMETHOD(WriteEffectToFile)(LPCNSTR a, DWORD b, LPDIFILEEFFECT c, DWORD d)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return m_device->WriteEffectToFile(a,b,c,d);
		return DIERR_INVALIDPARAM; // more like DIERR_NYI
	}
	// IDirectInputDevice8 methods
    STDMETHOD(BuildActionMap)(LPDIACTIONFORMATN a, LPCNSTR b, DWORD c)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return m_device->BuildActionMap(a,b,c);
		return DIERR_MAPFILEFAIL; // more like DIERR_NYI
	}
    STDMETHOD(SetActionMap)(LPDIACTIONFORMATN a, LPCNSTR b, DWORD c)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		//return m_device->SetActionMap(a,b,c);
		return DIERR_INVALIDPARAM; // more like DIERR_NYI
	}
    STDMETHOD(GetImageInfo)(LPDIDEVICEIMAGEINFOHEADERN a)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
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
		dinputdebugprintf(__FUNCTION__ " called.\n");
	}
	~MyDirectInput()
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
	}

	/*** IUnknown methods ***/
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = m_di->QueryInterface(riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

    ULONG STDMETHODCALLTYPE AddRef()
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		return m_di->AddRef();
	}

    ULONG STDMETHODCALLTYPE Release()
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		ULONG count = m_di->Release();
		if(0 == count)
			delete this;

		return count;
	}

    /*** IDirectInputN methods ***/
    STDMETHOD(CreateDevice)(REFGUID rguid, IDirectInputDeviceN** device, LPUNKNOWN unknown)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		ThreadLocalStuff& curtls = tls;
		curtls.callerisuntrusted++;
		HRESULT hr = m_di->CreateDevice(rguid, device, unknown);
		if(SUCCEEDED(hr))
		{
			// Return our own keyboard device that checks for injected keypresses
			// (at least if rguid == GUID_SysKeyboard that's what it'll do)
			HookCOMInterface(IID_IDirectInputDeviceN, (LPVOID*)device);
		}
		curtls.callerisuntrusted--;

		return hr;
	}

    STDMETHOD(EnumDevices)(DWORD devType,LPDIENUMDEVICESCALLBACKN callback, LPVOID ref, DWORD flags)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
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
		dinputdebugprintf(__FUNCTION__ " called.\n");
		return m_di->GetDeviceStatus(rguid);
	}

    STDMETHOD(RunControlPanel)(HWND owner, DWORD flags)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		return m_di->RunControlPanel(owner, flags);
	}

    STDMETHOD(Initialize)(HINSTANCE instance, DWORD version)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		return m_di->Initialize(instance, version);
	}

	STDMETHOD(CreateDeviceEx)(REFGUID a, REFIID b, LPVOID* c, LPUNKNOWN d)
	{
		dinputdebugprintf(__FUNCTION__ "(0x%X, 0x%X) called.\n", a.Data1, b.Data1);
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
		dinputdebugprintf(__FUNCTION__ " called.\n");
		return m_di->FindDevice(a,b,c);
	}
    STDMETHOD(EnumDevicesBySemantics)(LPCNSTR a, LPDIACTIONFORMATN b, LPDIENUMDEVICESBYSEMANTICSCBN c, LPVOID d, DWORD e)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
		return m_di->EnumDevicesBySemantics(a,b,c,d,e);
	}
    STDMETHOD(ConfigureDevices)(LPDICONFIGUREDEVICESCALLBACK a, LPDICONFIGUREDEVICESPARAMSN b, DWORD c, LPVOID d)
	{
		dinputdebugprintf(__FUNCTION__ " called.\n");
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
	static int inputEventSequenceID = 0;

	// do some processing per key that changed states.
	// this is so MyGetAsyncKeyState can properly mimic GetAsyncKeyState's
	// return value pattern of 0x0000 -> 0x8001 -> 0x8000 when a key is pressed,
	// and also so directinput buffered keyboard input can work.
	for(int i = 1; i < 256; i++)
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
			DIDEVICEOBJECTDATA keyEvent = {i, MyGetKeyState(i) & 0xFF, timeStamp, inputEventSequenceID++};
			BufferedInput::AddEventToAllDevices(keyEvent, s_bufferedKeySlots);
		}
	}



	//if(curinput.keys['Z'])
	//	debugprintf("Z PRESSED ON %d (%c)\n", framecount, tasflags.playback?'p':'r');
}

HOOKFUNC MMRESULT WINAPI MyjoyReleaseCapture(UINT uJoyID)
{
	debuglog(LCF_JOYPAD|LCF_UNTESTED, __FUNCTION__ " called (uJoyID=%d).\n", uJoyID);
return MMSYSERR_NODRIVER; // NYI
	//cmdprintf("WAITING: %u", 1);
	MMRESULT rv = joyReleaseCapture(uJoyID);
	//cmdprintf("WAITED: %u", 1);
	return rv;
}
HOOKFUNC MMRESULT WINAPI MyjoySetCapture(HWND hwnd, UINT uJoyID, UINT uPeriod, BOOL fChanged)
{
	debuglog(LCF_JOYPAD|LCF_UNTESTED, __FUNCTION__ " called (hwnd=0x%X, uJoyID=%d, uPeriod=%d, fChanged=%d).\n", hwnd, uJoyID, uPeriod, fChanged);
return MMSYSERR_NODRIVER; // NYI
	MMRESULT rv = joySetCapture(hwnd, uJoyID, uPeriod, fChanged);
	return rv;
}

HOOKFUNC MMRESULT WINAPI MyjoyGetPosEx(UINT uJoyID, LPJOYINFOEX pji)
{
	debuglog(LCF_JOYPAD/*|LCF_UNTESTED*/, __FUNCTION__ " called (uJoyID=%d).\n", uJoyID);
return MMSYSERR_NODRIVER; // NYI

	char threadTypeName[64];
	sprintf(threadTypeName, "JoypadThread(%d)", uJoyID);
	tls.curThreadCreateName = threadTypeName;
debuglog(LCF_JOYPAD|LCF_UNTESTED, "in "__FUNCTION__", tls.curThreadCreateName = %s\n", tls.curThreadCreateName);

	MMRESULT rv = joyGetPosEx(uJoyID, pji);

	tls.curThreadCreateName = NULL;

	return rv;
//	return MMSYSERR_NODRIVER ;
}
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

TRAMPFUNC UINT WINAPI MyjoyGetNumDevs()
{
	debuglog(LCF_JOYPAD|LCF_UNTESTED, __FUNCTION__ " called.\n");
return 0; // NYI
	UINT rv = joyGetNumDevs();
	return rv;
}
TRAMPFUNC MMRESULT WINAPI MyjoyGetDevCapsA(UINT_PTR uJoyID, LPJOYCAPSA pjc, UINT cbjc)
{
	debuglog(LCF_JOYPAD|LCF_UNTESTED, __FUNCTION__ " called.\n");
return MMSYSERR_NODRIVER; // NYI
	MMRESULT rv = joyGetDevCapsA(uJoyID, pjc, cbjc);
	return rv;
}
TRAMPFUNC MMRESULT WINAPI MyjoyGetDevCapsW(UINT_PTR uJoyID, LPJOYCAPSW pjc, UINT cbjc)
{
	debuglog(LCF_JOYPAD|LCF_UNTESTED, __FUNCTION__ " called.\n");
return MMSYSERR_NODRIVER; // NYI
	MMRESULT rv = joyGetDevCapsW(uJoyID, pjc, cbjc);
	return rv;
}


HOOKFUNC BOOL WINAPI MyGetCursorPos(LPPOINT lpPoint)
{
	if(!lpPoint) { return FALSE; }
	// NYI
	lpPoint->x = 0;
	lpPoint->y = 0;
	return TRUE;
}

HOOKFUNC BOOL WINAPI MyGetCursorInfo(PCURSORINFO pci)
{
	if(!GetCursorInfo(pci)) { return FALSE; }
	return MyGetCursorPos(&pci->ptScreenPos);
}


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

#else
#pragma message(__FILE__": (skipped compilation)")
#endif
