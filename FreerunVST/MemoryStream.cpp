#pragma warning(push)
#pragma warning(disable : 4996)
#include "pluginterfaces/base/ibstream.h"
#pragma warning(pop)

#include "Main.h"
#include "MemoryStream.h"
#include <vector>

using namespace Steinberg;
using ubig_t = std::conditional_t<(sizeof(size_t) > sizeof(int64_t)), size_t, uint64>;

IMPLEMENT_FUNKNOWN_METHODS(MemoryStream, IBStream, IBStream::iid)

MemoryStream::MemoryStream(IBStream* src, int32 size) : pos(0)
{
    LOG("MemoryStream from stream constructor called (src=%p, size=%d)\n", src, size);

    FUNKNOWN_CTOR

    if (src && (size > 0))
    {
        buffer.resize(size);
        int32 numBytesRead = 0;
        if (src->read(buffer.data(), size, &numBytesRead) == kResultOk)
        {
            buffer.resize(numBytesRead);
            LOG("MemoryStream from stream constructor read %d bytes from source stream.\n", numBytesRead);
        }
        else
            buffer.resize(0);
    }
}

tresult PLUGIN_API MemoryStream::write(void* data, int32 numBytes, int32* numBytesWritten)
{
    LOG("MemoryStream::write called.\n");
    if (numBytes < 0) return kInvalidArgument;
    const size_t toWrite = static_cast<size_t>(numBytes);

    if (toWrite > buffer.size() - pos)
        buffer.resize(pos + toWrite);

    const uint8_t* src = static_cast<const uint8_t*>(data);
    if (toWrite > 0)
        memcpy(buffer.data() + pos, src, toWrite);
    pos += toWrite;

    if (numBytesWritten)
        *numBytesWritten = numBytes;

    return kResultOk;
}

tresult PLUGIN_API MemoryStream::read(void* data, int32 numBytes, int32* numBytesRead)
{
    LOG("MemoryStream::read called (data=%p, numBytes=%d, numBytesRead=%p, pos=%lu, size=%lu)\n", data, numBytes, numBytesRead, pos, buffer.size());
    if (numBytes < 0) return kInvalidArgument;

    const size_t available = buffer.size() - pos;
    size_t toRead = (available < static_cast<size_t>(numBytes)) ? available : static_cast<size_t>(numBytes);
    if (!data) toRead = 0;

    if (toRead > 0)
        memcpy(data, buffer.data() + pos, toRead);
    pos += toRead;
    if (numBytesRead)
        *numBytesRead = static_cast<int32>(toRead);

    LOG("MemoryStream::read returned %lu bytes (pos=%lu)\n", toRead, pos);
    return kResultOk;
}

tresult PLUGIN_API MemoryStream::seek(int64 offset, int32 mode, int64* newPos)
{
    LOG("MemoryStream::seek called (offset=%lu, mode=%d, newPos=%p)\n", offset, mode, newPos);
    if ((mode != kIBSeekSet) && (mode != kIBSeekCur) && (mode != kIBSeekEnd))
        return kInvalidArgument;

    const ubig_t start = static_cast<ubig_t>((mode == kIBSeekSet) ? 0 : (mode == kIBSeekCur) ? pos : buffer.size());
    ubig_t target = start + static_cast<ubig_t>(offset); // modularly congruent to correct answer

    if ((offset < 0) && (target >= start))
        target = 0;
    else if ((offset > 0) && ((target <= start) || (target >= SIZE_MAX)))
        target = static_cast<ubig_t>(buffer.size());

    pos = static_cast<size_t>(target);

    if (newPos)
        *newPos = static_cast<int64>(pos);

    LOG("MemoryStream::seek returned (pos=%lu)\n", pos);
    return kResultOk;
}

tresult PLUGIN_API MemoryStream::tell(int64* posOut)
{
    LOG("MemoryStream::tell called.\n");
    if (!posOut) return kInvalidArgument;

    if (static_cast<ubig_t>(pos) > static_cast<ubig_t>(kMaxInt64))
        return kResultFalse;

    *posOut = static_cast<int64>(pos);

    return kResultOk;
}
