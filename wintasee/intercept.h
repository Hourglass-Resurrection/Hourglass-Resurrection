/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef INTERCEPT_H_INCL
#define INTERCEPT_H_INCL

// first some helper macros to use when defining trampoline and hook functions

#define TRAMPFUNC __declspec(noinline)
#define HOOKFUNC //__declspec(noinline)

#define TRAMPOLINE_DEF  INTERNAL_TRAMPOLINE_DEF
#define TRAMPOLINE_DEF_VOID  INTERNAL_TRAMPOLINE_DEF_VOID
#if _MSC_VER > 1310
	#define TRAMPOLINE_DEF_CUSTOM(...)          { __VA_ARGS__ ; }
#else
	#define TRAMPOLINE_DEF_CUSTOM(__VA_ARGS__)  { __VA_ARGS__ ; }
#endif

// this macro is used to fill trampolines with some default code as its original contents,
// that's big enough to get overwritten with a jump to the target.
// (more than big enough, and slightly different per function, to stay on the safe side.)
#define TRAMPOLINE_CONTENTS \
	static int x__ = 0; \
	x__++; \
	x__++; \
	if(x__==2){debugprintf("Function \""__FUNCTION__"\" got called before it was set up!"); _asm{int 3} MessageBox(NULL, "Failed to hook function \""__FUNCTION__"\". This should never happen.", "Error", MB_ICONERROR);} \
	x__++; \
	x__++;
#define INTERNAL_TRAMPOLINE_DEF { TRAMPOLINE_CONTENTS return 0; }
#define INTERNAL_TRAMPOLINE_DEF_VOID { TRAMPOLINE_CONTENTS }

// the trampoline's original contents should never get called.
// mistakes in the code that could cause it to get called are:
// 1: if you forget to use the TRAMPFUNC macro, and the compiler inlines the trampoline contents into the caller.
// 2: if you're adding code that gets called from DllMain before hooks are set up, and you forget to check notramps.
// 3: if you're calling the trampoline to a function that's in a DLL that isn't loaded by the game.
// if you expect 2 or 3 to be a problem somewhere, you could put a call to the real function
// inside the initial trampoline contents, but beware the linking requirements that can add.
// mistake 3 can be fixed by setting the hook mode to force load that DLL so the trampoline is valid,
// but I try to minimize this to keep the program as lightweight as I reasonably can.






// API for actually performing interception/hooking

// transforms all calls by the current process to the function at dwAddressToIntercept
// into calls to the provided function dwReplaced,
// and transforms all calls to the function at dwTrampoline
// into calls to the "old" function that was at dwAddressToIntercept before.
BOOL InterceptGlobalFunction(FARPROC dwAddressToIntercept, FARPROC dwReplaced, FARPROC dwTrampoline, bool trampolineOnly=false, BOOL rvOnSkip=TRUE);

// same as dwAddressToIntercept, except it automatically calculates dwAddressToIntercept
// based on the DLL name (c_szDllName) and function name (c_szApiName) that you provide.
BOOL InterceptAPI(const char* c_szDllName, const char* c_szApiName, FARPROC dwReplaced, FARPROC dwTrampoline, bool trampolineOnly=false, bool forceLoad=false, bool allowTrack=true, BOOL rvOnSkip=TRUE);

// these were for "mass-hooking" functions with the same name in multiple DLLs. not used anymore.
//void InterceptAllDllAPI(const char* c_szDllName, const char* c_szApiName, FARPROC dwReplaced, FARPROC dwTrampoline, bool trampolineOnly=false); // expects c_szDllName, dwReplaced, and dwTrampoline to actually be pointers to equally-sized null-terminated arrays of their respective types
//BOOL InterceptAPIDynamicTramp(const char* c_szDllName, const char* c_szApiName, FARPROC*& dwReplacedArrayPtr, FARPROC*& dwTrampolineArrayPtr, bool trampolineOnly, const char**& c_szDllNameArrayPtr);

// call this when a new DLL gets loaded dynamically,
// to catch functions that InterceptAPI originally failed on due to the DLL not being loaded yet.
// you don't have to specify which functions to hook,
// because it remembers that based on which previous calls to InterceptAPI failed.
void RetryInterceptAPIs(const char* c_szDllName);

// call this when a DLL gets unloaded dynamically,
// otherwise if it gets loaded again later,
// RetryInterceptAPIs will think it's already hooked and won't know it needs to re-hook it.
void UnInterceptUnloadingAPIs(const char* c_szDllName);

// this hooks a C++ object's methods using a different mechanism than InterceptGlobalFunction:
// it simply writes the new function pointer (replace) into the object's vtable,
// and gives you back the old function pointer that was there (in oldfuncPointer).
// it's less safe in some ways,
// but it's often less intrusive than another alternate mechanism for hooking COM objects
// which is to wrap the entire interface with your own interface.
BOOL HookVTable(void* iface, int entry, FARPROC replace, FARPROC& oldfuncPointer, const char* debugname);

// offsetof_virtual, used to figure out what "entry" number to pass into HookVTable.
// use for COM objects like this:
// int offset = offsetof_virtual(&IDirect3DDevice::EndScene);
// this implementation is probably quite compiler-specific and scornworthy,
// but so far it has been serving its intended purpose just fine.
template <typename F>
int offsetof_virtual(F f)
{
	unsigned char* c = *(unsigned char**)&f;
	while(*c++ != 0xFF) {} // search for jmp ff/4? (why don't x86 docs say what each combination of bytes does? I can't decode their gibberish tables.)
	switch(*c++)
	{
		case 0x60:
			return *((unsigned char*)c) / 4;
		//case 0x??: // I've never seen this (2-byte address) case used so far, and I can't find the damn thing in any x86 docs, so it's disabled for now
		//	return *((short*)c) / 4;
		case 0xA0:
			return *((int*)c) / 4;
		default: /*0x20:*/
			return 0;
	}
}
// if something goes wrong with offsetof_virtual and you just can't figure it out,
// keep in mind that this is only for convenience since the result is always a constant number
// which is guaranteed to be the same for any given function in the case of COM objects.
// (for example, the offset of IDirect3DDevice8::DrawPrimitive is always 70,
//  and you can figure that out by counting the 70 functions above it in the interface)
// so you could replace the call to offsetof_virtual with a number in any places that bother you.
// I'm only using this because I don't like counting manually and I don't like putting magic numbers everywhere.


// given a COM interface
// before calling this function, ppvOut must point to a COM interface
// and riid must be the that interface must be of the same 
void HookCOMInterface(REFIID riid, LPVOID *ppvOut, bool uncheckedFastNew=false);

// like HookCOMInterface, except it works even if you're not sure what type the object is
// (as long as it inherits from IUnknown), so in that sense it's safer.
// but it currently doesn't provide an output pointer,
// because it doesn't know what interface type to cast the pointer back to,
// thus this only works for interfaces that we use VT hooking on (e.g. VTHOOKRIID).
#define HookCOMInterfaceUnknownVT(ifacename, ppu) \
do{ \
	void* pvOut = 0; \
	if((ppu) && SUCCEEDED((ppu)->QueryInterface(IID_##ifacename, &pvOut))) \
	{ \
		HookCOMInterface(IID_##ifacename, &pvOut); \
		((ifacename*)pvOut)->Release(); \
	} \
}while(0)




// note: if you get a LNK2005 (duplicate reference error) trying to use one of these,
// it's probably because some code accidentally referenced a UUID directly from a header (such as IID_IUnknown) without using this macro first,
// causing uuid.lib to get automatically linked in and break random other GUIDs.
#define DEFINE_LOCAL_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
		static const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }; \
		static const DWORD name##_Data1 = l







#include "print.h"

// return false if the given object is NULL or of the destination hook type TM, true otherwise
template<typename TI, typename TM>
bool type_needs_hooking(TI o)
{
	if(!o) return false;
#ifndef _DEBUG
	cmdprintf("PAUSEEXCEPTIONPRINTING: ");
#endif
	bool rv;
	__try {
		rv = dynamic_cast<TM>(o) == NULL;
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		rv = true;
	}
#ifndef _DEBUG
	cmdprintf("RESUMEEXCEPTIONPRINTING: ");
#endif
	verbosedebugprintf("NEEDS HOOKING: %d\n", rv);
	//_asm{int 3}
	return rv;
}

template<typename TI, typename TM>
bool is_of_type(TI o)
{
	if(!o) return false;
	bool rv;
	__try {
		rv = dynamic_cast<TM>(o) != NULL;
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		rv = false;
	}
	return rv;
}

#ifdef _DEBUG
enum { s_isDebug = true };
#else
enum { s_isDebug = false };
#endif

#define HOOKRIID(x,n) case IID_I##x##n##_Data1: if(IID_I##x##n == riid && (uncheckedFastNew || type_needs_hooking<I##x##n*,My##x<I##x##n>*>(reinterpret_cast<I##x##n*>(*ppvOut)))) (uncheckedFastNew&&!s_isDebug || debugprintf("HOOKED COM INTERFACE: I" #x #n " (0x%X)\n", *ppvOut)), *ppvOut = new My##x<I##x##n>(reinterpret_cast<I##x##n*>(*ppvOut)); break
#define HOOKRIID2(x,my) case IID_I##x##_Data1: if(IID_I##x == riid && (uncheckedFastNew || type_needs_hooking<I##x*,my<I##x>*>(reinterpret_cast<I##x*>(*ppvOut)))) (uncheckedFastNew&&!s_isDebug || debugprintf("HOOKED COM INTERFACE: I" #x " (0x%X)\n", *ppvOut)), *ppvOut = new my<I##x>(reinterpret_cast<I##x*>(*ppvOut)); break
#define HOOKRIID3(x,my) case IID_##x##_Data1: if(IID_##x == riid && (uncheckedFastNew || type_needs_hooking<x*,my*>(reinterpret_cast<x*>(*ppvOut)))) (uncheckedFastNew&&!s_isDebug || debugprintf("HOOKED COM INTERFACE: " #x " (0x%X)\n", *ppvOut)), *ppvOut = new my(reinterpret_cast<x*>(*ppvOut)); break
//#define HOOKRIID4(x,my) case IID_##x##_Data1: if(IID_##x == riid && (uncheckedFastNew || type_needs_hooking<x*,my*>(reinterpret_cast<x*>(*ppvOut)))) (uncheckedFastNew&&!s_isDebug || debugprintf("HOOKED COM INTERFACE: " #x " (0x%X)\n", *ppvOut)), *ppvOut = new my(reinterpret_cast<x*>(*ppvOut)); break
#define VTHOOKRIID(x,n) case IID_I##x##n##_Data1: if(IID_I##x##n == riid) if(My##x<I##x##n>::Hook(reinterpret_cast<I##x##n*>(*ppvOut))) (uncheckedFastNew&&!s_isDebug || debugprintf("HOOKED COM INTERFACE: I" #x #n " (0x%X)\n", *ppvOut)); break
#define VTHOOKRIID2(x,n,n2) case IID_I##x##n##_Data1: if(IID_I##x##n == riid) if(My##x<I##x##n2>::Hook(reinterpret_cast<I##x##n2*>(*ppvOut))) (uncheckedFastNew&&!s_isDebug || debugprintf("HOOKED COM INTERFACE: I" #x #n " (0x%X)\n", *ppvOut)); break
#define VTHOOKRIID3(i,my) case IID_##i##_Data1: if(IID_##i == riid) if(my::Hook(reinterpret_cast<i*>(*ppvOut))) (uncheckedFastNew&&!s_isDebug || debugprintf("HOOKED COM INTERFACE: " #i " (0x%X)\n", *ppvOut)); break
#define VTHOOKRIID3MULTI3(i,my) case IID_##i##_Data1: if(IID_##i == riid) { BOOL v = 0; i* pv = reinterpret_cast<i*>(*ppvOut); \
	if(!v) v = my<0>::Hook(pv); if(!v) v = my<1>::Hook(pv); if(!v) v = my<2>::Hook(pv); \
	if(!(v<=0)) (uncheckedFastNew&&!s_isDebug || debugprintf("HOOKED COM INTERFACE: " #i " (0x%X)\n", *ppvOut)); }	break
//#define VTHOOKRIID3MULTI8(i,my) case IID_##i##_Data1: if(IID_##i == riid) { BOOL v = 0; i* pv = reinterpret_cast<i*>(*ppvOut); \
//	if(!v) v = my<0>::Hook(pv); if(!v) v = my<1>::Hook(pv); \
//	if(!v) v = my<2>::Hook(pv); if(!v) v = my<3>::Hook(pv); \
//	if(!v) v = my<4>::Hook(pv); if(!v) v = my<5>::Hook(pv); \
//	if(!v) v = my<6>::Hook(pv); if(!v) v = my<7>::Hook(pv); \
//	if(!(v<=0)) (uncheckedFastNew&&!s_isDebug || debugprintf("HOOKED COM INTERFACE: " #i " (0x%X)\n", *ppvOut)); }	break
//#define VTHOOKRIID4(iid,iface,my) case iid##_Data1: if(iid == riid) if(my::Hook(reinterpret_cast<iface*>(*ppvOut))) (uncheckedFastNew&&!s_isDebug || debugprintf("HOOKED COM INTERFACE: " #iid " (0x%X)\n", *ppvOut)); break
#define IGNOREHOOKRIID(x,n) case IID_I##x##n##_Data1: if(IID_I##x##n == riid) debugprintf("IGNORED COM INTERFACE: I" #x #n " (0x%X)\n", *ppvOut); break
#define VTHOOKFUNC(i,x) HookVTable(obj, offsetof_virtual(&i::x), (FARPROC)My##x, (FARPROC&)x, __FUNCTION__": " #x)
#define VTHOOKFUNC2(i,x,x2) HookVTable(obj, offsetof_virtual(&i::x2), (FARPROC)My##x, (FARPROC&)x, __FUNCTION__": " #x)
//#define VTHOOKFUNC3(i,x) HookVTable(obj, offsetof_virtual(&i::x), (FARPROC)My##x, (FARPROC&)Orig##x, __FUNCTION__": " #x)
//#define VTHOOKFUNC4(i,x,my,orig) HookVTable(obj, offsetof_virtual(&i::x), (FARPROC)my, (FARPROC&)orig, __FUNCTION__": " #x)

// use for COM functions that shouldn't get called
#define IMPOSSIBLE_IMPL { debugprintf("IMPOSSIBLE: " __FUNCTION__ " called.\n"); return 0; }




struct InterceptDescriptor
{
	const char* dllName;
	const char* functionName;
	FARPROC replacementFunction;
	FARPROC trampolineFunction;
	int enabled;
	// the meaning of "enabled" (the first parameter to MAKE_INTERCEPT) is:
	//  0 means to point our trampoline at it if it exists, and nothing else
	//  1 means to point it at our replacement function and point our trampoline at it if it exists
	//  2 means the same as 1 except force dll to load if it does not exist so our trampoline always works
	// -1 means the same as 0 except force dll to load if it does not exist so our trampoline always works

	//const char** dllNameArray;
};

#define MAKE_INTERCEPT(enabled, dll, name) {#dll".dll", #name, (FARPROC)My##name, (FARPROC)Tramp##name, enabled}
#define MAKE_INTERCEPT2(enabled, dll, name, myname) {#dll".dll", #name, (FARPROC)My##myname, (FARPROC)Tramp##myname, enabled}
#define MAKE_INTERCEPT3(enabled, dllWithExt, name, suffix) {#dllWithExt, #name, (FARPROC)My##name##_##suffix, (FARPROC)Tramp##name##_##suffix, enabled}
//#define MAKE_INTERCEPT_DYNAMICTRAMP(enabled, dllWithExt, name) {#dllWithExt, #name, (FARPROC)(void*)ArrayMy##name, (FARPROC)(void*)ArrayTramp##name, (enabled>0)?3:-3, ArrayName##name}
//#define MAKE_INTERCEPT_ALLDLLS(enabled, name) {NULL, #name, (FARPROC)(void*)ArrayMy##name, (FARPROC)(void*)ArrayTramp##name, (enabled>0)?4:-4, (const char*)(void*)ArrayName##name}


void ApplyInterceptTable(const InterceptDescriptor* intercepts, int count);



#endif
