#pragma once

#include <Windows.h>

#pragma warning(push)
#pragma warning(disable : 4996)
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/main/pluginfactory.h"
#pragma warning(pop)
using namespace Steinberg;

// ---- BEGIN COMPILE-TIME OPTIONS ----

#define FILENAME_PREFIX L"Freerun"
#define DISPLAYED_NAME_SUFFIX " (FR)"

// When FreerunVST wraps a plugin P, it can either reuse P's class identifiers or choose new
// unique identifiers.  Here are are the trade-offs:
// (A) Reusing P's ids works fine as long as your host doesn't care that two different plugins
//     (both P and FreerunVST-wrapped-P) are using the same ids (which are supposed to be
//     unique).  If this doesn't confuse your host, reusing P's ids is a fine choice.
// (B) Letting FreerunVST choose new unique ids is more standards-compliant for your host,
//     unless your host and plugin have some back-channel communication that exposes P's ids
//     to the host.  For example, if your host enables special optimizations or workarounds
//     for P when it sees P's identifiers, then you might need to preserve P's identifiers.
#define UNIQUE_CLASS_IDS

// Enable this to print logging info to a .log file in the Documents folder (slows performance).
#undef LOGGING

constexpr uint32 default_freerunning_param_id = 333333333;

// ---- END COMPILE-TIME OPTIONS ----




constexpr uint32 num_freerun_params = 1;

#ifdef UNIQUE_CLASS_IDS
static const FUID FreerunUID(0x8F3A1C92, 0x4D7B4E11, 0x9C3F82A7, 0xB1D4F0C3);
#endif

#ifdef LOGGING
#define LOGFILE L"FreerunVST.log"
void log(const char* format, ...);
#	define LOG(format, ...) log((format), __VA_ARGS__)
#else
#	define LOG(format, ...) 0
#endif

#define _WRAPINTERFACE(i,me) \
	if (i && FUnknownPrivate::iidEqual(iid, i->iid)) { \
		addRef(); *obj = static_cast<decltype(i)>(me); return kResultOk; \
	}

// Add code to queryInterface that tests whether the requested interface id matches that of
// object i, and returns the wrapper object's corresponding interface if so.
#define WRAPINTERFACE(i) _WRAPINTERFACE(i,this)

// Special case: The wrapper object has multiple implementations of IPluginBase since it
// implements multiple interfaces that inherit it, so we must choose which IPluginBase to
// return (and be consistent) when the host requests that interface.  Therefore, when the
// host requests interface i2 (e.g., IPluginBase), this code returns the i2 implementation
// inherited by the wrapper object's i1 interface (e.g., IComponent).
#define WRAPINTERFACE2(i1,i2) _WRAPINTERFACE(i2,static_cast<decltype(i1)>(this))

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)
#define _PROJ1(a,b) a
#define _PROJ2(a,b) b
#define UNPACK(macro,args) macro args
#define PROJ1(x) UNPACK(_PROJ1,(x))
#define PROJ2(x) UNPACK(_PROJ2,(x))

// Implement a wrapper method that calls the wrapped plugin's method.
#define WRAP_METHOD(mname,params,args,suffix,retval) \
  PROJ1(WRAPPED_OBJECT)::mname params suffix { \
    LOG(STRINGIFY(PROJ1(WRAPPED_OBJECT)) "::" #mname " called.\n"); \
    auto r = (PROJ2(WRAPPED_OBJECT)) ? ((PROJ2(WRAPPED_OBJECT))->mname args) : (retval); \
	LOG(STRINGIFY(PROJ1(WRAPPED_OBJECT)) "::" #mname " returning %d.\n", r); \
	return r; \
  }

#define WRAP_METHOD_TR(mname,params,args) PLUGIN_API WRAP_METHOD(mname,params,args,,kResultFalse)
#define WRAP_METHOD_INT(mname,params,args) PLUGIN_API WRAP_METHOD(mname,params,args,,0)
#define WRAP_METHOD_PV(mname,params,args) PLUGIN_API WRAP_METHOD(mname,params,args,,0.)
#define WRAP_METHOD_PTR(mname,params,args) PLUGIN_API WRAP_METHOD(mname,params,args,,nullptr)

#define WRAP_METHOD_VOID(mname,params,args) \
  PLUGIN_API PROJ1(WRAPPED_OBJECT)::mname params { if (PROJ2(WRAPPED_OBJECT)) { (PROJ2(WRAPPED_OBJECT))->mname args; } }

#define BEGIN_WRAPPING private: PROJ1(WRAPPED_OBJECT)* PROJ2(WRAPPED_OBJECT) = nullptr; public:

extern HMODULE hmodule;
extern HMODULE load_sublibrary(void);
