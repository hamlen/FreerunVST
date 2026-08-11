#pragma once

#pragma warning(push)
#pragma warning(disable : 4996)
#include "pluginterfaces/base/ibstream.h"
#pragma warning(pop)

#include <vector>

using namespace Steinberg;

class MemoryStream : public IBStream
{
public:
    MemoryStream() : pos(0) { FUNKNOWN_CTOR }
    MemoryStream(IBStream* src, int32 size);
    virtual ~MemoryStream() { FUNKNOWN_DTOR };

    tresult PLUGIN_API write(void* data, int32 numBytes, int32* numBytesWritten = nullptr) SMTG_OVERRIDE;
    tresult PLUGIN_API read(void* data, int32 numBytes, int32* numBytesRead = nullptr) SMTG_OVERRIDE;
    tresult PLUGIN_API seek(int64 offset, int32 mode, int64* newPos = nullptr) SMTG_OVERRIDE;
    tresult PLUGIN_API tell(int64* posOut) SMTG_OVERRIDE;

    const uint8_t* data() const { return buffer.data(); }
    uint8_t* data() { return buffer.data(); }
    size_t size() const { return buffer.size(); }

    DECLARE_FUNKNOWN_METHODS

private:
    std::vector<uint8_t> buffer;
    size_t pos = 0;
};
