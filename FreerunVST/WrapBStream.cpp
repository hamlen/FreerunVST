#include "pluginterfaces/base/ibstream.h"

#include "Main.h"
#include "WrapBStream.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

void WrappedBStream::begin_exposed_stream()
{
	if (!substream || (substream->tell(&hidden_size) != kResultOk))
		hidden_size = 0;
	LOG("WrappedBStream::begin_exposed_stream set hidden_size = %ld\n", hidden_size);
}

void WrappedBStream::reset()
{
	hidden_size = 0;
	substream->seek(0, kIBSeekSet);
}

tresult PLUGIN_API WrappedBStream::read(void* buffer, int32 numBytes, int32* numBytesRead)
{
	if (!substream || (numBytes < 0)) return kResultFalse;
	int64 pos;
	if (substream->tell(&pos) != kResultOk) return kResultFalse;
	if (pos < hidden_size)
	{
		if (substream->seek(hidden_size, kIBSeekSet) != kResultOk)
			return kResultFalse;
	}
	return substream->read(buffer, numBytes, numBytesRead);
}

tresult PLUGIN_API WrappedBStream::tell(int64* pos)
{
	LOG("WrappedBStream::tell called.\n");
	if (!substream) return kResultFalse;
	int64 subpos;
	const tresult tr = substream->tell(&subpos);
	if (tr == kResultOk)
	{
		if (subpos < hidden_size) return kResultFalse;
		*pos = subpos - hidden_size;
	}
	return tr;
}

tresult PLUGIN_API WrappedBStream::seek(int64 pos, int32 mode, int64* result)
{
	LOG("WrappedBStream::seek called.\n");
	if (!substream) return kResultFalse;
	if (mode == kIBSeekSet)
	{
		tresult tr = substream->seek(pos + hidden_size, kIBSeekSet, result);
		if (result) *result = pos;
		return tr;
	}
	int64 newpos;
	tresult tr = substream->seek(pos, mode, &newpos);
	if (tr == kResultOk)
	{
		if (newpos < hidden_size)
		{
			substream->seek(hidden_size, kIBSeekSet, result);
			if (result) *result = 0;
		}
		else if (result)
			*result -= hidden_size;
	}
	return tr;
}
