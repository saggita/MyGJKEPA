#ifndef CL_ERROR_H
#define CL_ERROR_H

//#include <Adl/AdlError.h>


#ifdef DX11RENDER
	#include <windows.h>
#endif

#ifdef _DEBUG
	#include <assert.h>
	#define ADLASSERT(x) if(!(x)){__debugbreak(); }
#else
	#define ADLASSERT(x) if(x){}
#endif

#ifdef _DEBUG
	#define COMPILE_TIME_ASSERT(x) {int compileTimeAssertFailed[x]; compileTimeAssertFailed[0];}
#else
	#define COMPILE_TIME_ASSERT(x)
#endif


#define WARN(msg) debugPrintf("WARNING: %s\n", msg);

#endif

