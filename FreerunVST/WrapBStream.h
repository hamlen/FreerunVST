#pragma once

#pragma warning(push)
#pragma warning(disable : 4996)
#include "pluginterfaces/base/ibstream.h"
#pragma warning(pop)

#include "Main.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

class WrappedBStream : public FObject, public IBStream
{
public:
	WrappedBStream(void) {};
	WrappedBStream(IBStream* s) { substream = s; };
	~WrappedBStream(void) {};

	DEFINE_INTERFACES
	END_DEFINE_INTERFACES(FObject)

	DELEGATE_REFCOUNT(FObject)

#	undef WRAPPED_INTERFACE
#	undef WRAPPED_OBJECT
#	define WRAPPED_INTERFACE IBStream
#	define WRAPPED_OBJECT substream
	tresult PLUGIN_API read(void* buffer, int32 numBytes, int32* numBytesRead = nullptr);
	tresult WRAP_METHOD_TR(write, (void* buffer, int32 numBytes, int32* numBytesWritten = nullptr), (buffer,numBytes,numBytesWritten));
	tresult PLUGIN_API seek(int64 pos, int32 mode, int64* result = nullptr);
	tresult PLUGIN_API tell(int64* pos);

	void begin_exposed_stream();
	void reset();
	IBStream* exposedStream() { return (hidden_size > 0) ? this : substream; }

private:
	int64 hidden_size = 0;
	IBStream* substream = nullptr;
};
