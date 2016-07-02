/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

// first some helper macros to use when defining trampoline and hook functions

#define TRAMPFUNC __declspec(noinline)
#define HOOKFUNC __declspec(noinline)

/*
 * This macro is used to declare a hook function and a trampoline, and fill their bodies
 * with pre-defined code. For Trampolines this his code is long enough to safely get
 * overwritten with a jump to the target. The hook function is designed to jump to the
 * My[x] implementation.
 *
 * The trampolines original contents should never get called. If it happens, it means that the
 * DLL the trampoline points at hasn't been loaded yet. For "non-default" DLLs (such as
 * kernel32.dll), this may indicate that the application doesn't use it.
 *
 * Intended usage example:
 * HOOK_FUNCTION(DWORD, WINAPI, TargetFunction, params...)
 * HOOKFUNC DWORD WINAPI MyTargetFunction(params...) { code; }
 *
 * The usage of the C-cast for the return allows for void-functions to be declared.
 * The C-cast to void will cause the compiler to throw away the value that's being cast.
 *
 * The dummy typedef is to force ending the macro with a ;
 */
#define HOOK_FUNCTION(return_type, call_convention, target, ...) \
    TRAMPFUNC return_type call_convention Tramp##target(##__VA_ARGS__) \
    { \
        static int x__ = 0; \
        x__++; \
        x__++; \
        if (x__==2) \
        { \
            debugprintf("Function %s got called before it was set up!", __FUNCTION__); \
            _asm { int 3 }; \
        } \
        x__++; \
        x__++; \
        return (return_type)0; \
    } \
    typedef int dummy_typedef_##target

#define HOOK_DECLARE(return_type, call_convention, target, ...) \
    TRAMPFUNC return_type call_convention Tramp##target(##__VA_ARGS__); \
    HOOKFUNC return_type call_convention My##target(##__VA_ARGS__); \
    static auto& target = Tramp##target

// API for actually performing interception/hooking

// transforms all calls by the current process to the function at dwAddressToIntercept
// into calls to the provided function dwReplaced,
// and transforms all calls to the function at dwTrampoline
// into calls to the "old" function that was at dwAddressToIntercept before.
BOOL InterceptGlobalFunction(FARPROC dwAddressToIntercept, FARPROC dwReplaced, FARPROC dwTrampoline, bool trampolineOnly=false, BOOL rvOnSkip=TRUE);

// same as dwAddressToIntercept, except it automatically calculates dwAddressToIntercept
// based on the DLL name (c_szDllName) and function name (c_szApiName) that you provide.
BOOL InterceptAPI(const char* c_szDllName, const char* c_szApiName, FARPROC dwReplaced, FARPROC dwTrampoline, bool trampolineOnly=false, bool forceLoad=false, bool allowTrack=true, BOOL rvOnSkip=TRUE, bool ordinal = false);

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
void HookCOMInterfaceEx(REFIID riid, LPVOID *ppvOut, REFGUID parameter, bool uncheckedFastNew=false);

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
#define HOOKRIID2EX(x,my,param) case IID_I##x##_Data1: if(IID_I##x == riid && (uncheckedFastNew || type_needs_hooking<I##x*,my<I##x>*>(reinterpret_cast<I##x*>(*ppvOut)))) (uncheckedFastNew&&!s_isDebug || debugprintf("HOOKED COM INTERFACE: I" #x " (0x%X)\n", *ppvOut)), *ppvOut = new my<I##x>(reinterpret_cast<I##x*>(*ppvOut),param); break
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

    /*
     * The meaning of "enabled" (the first parameter to MAKE_INTERCEPT) is:
     *  0 means to point our trampoline at it if it exists, and nothing else
     *  1 means to point it at our replacement function and point our trampoline at it if it exists
     *  2 means the same as 1 except force dll to load if it does not exist so our trampoline always works
     * -1 means the same as 0 except force dll to load if it does not exist so our trampoline always works
     */
    int enabled;

    bool ordinal;
    //const char** dllNameArray;
};

#define MAKE_INTERCEPT(enabled, dll, name) {#dll".dll", #name, (FARPROC)My##name, (FARPROC)Tramp##name, enabled, false}
#define MAKE_INTERCEPT2(enabled, dll, name, myname) {#dll".dll", #name, (FARPROC)My##myname, (FARPROC)Tramp##myname, enabled, false}
#define MAKE_INTERCEPT3(enabled, dllWithExt, name, suffix) {#dllWithExt, #name, (FARPROC)My##name##_##suffix, (FARPROC)Tramp##name##_##suffix, enabled, false}
#define MAKE_INTERCEPT_ORD(enabled, dll, name, ordinal) {#dll".dll", reinterpret_cast<const char*>(ordinal), (FARPROC)My##name, (FARPROC)Tramp##name, enabled, true}
//#define MAKE_INTERCEPT_DYNAMICTRAMP(enabled, dllWithExt, name) {#dllWithExt, #name, (FARPROC)(void*)ArrayMy##name, (FARPROC)(void*)ArrayTramp##name, (enabled>0)?3:-3, ArrayName##name}
//#define MAKE_INTERCEPT_ALLDLLS(enabled, name) {NULL, #name, (FARPROC)(void*)ArrayMy##name, (FARPROC)(void*)ArrayTramp##name, (enabled>0)?4:-4, (const char*)(void*)ArrayName##name}

void ApplyInterceptTable(const InterceptDescriptor* intercepts, int count);
