#pragma warning(push)
#pragma warning(disable : 4996)
#include "pluginterfaces/base/ibstream.h"
#pragma warning(pop)

#include "Main.h"
#include "WrappedStream.h"
#include <bit>

using namespace Steinberg;
using namespace Vst;

IMPLEMENT_FUNKNOWN_METHODS(WrappedStream, IBStream, IBStream::iid)

WrappedStream::WrappedStream(IBStream* src) : substream(src)
{
    LOG("WrappedStream from stream constructor called\n");
    FUNKNOWN_CTOR
    if (src) src->tell(&start_pos);
}

tresult PLUGIN_API WrappedStream::seek(int64 offset, int32 mode, int64* newPos)
{
    LOG("WrappedStream::seek called (offset=%lu, mode=%d, newPos=%p)\n", offset, mode, newPos);
    tresult tr = kResultFalse;
    if (!substream) return kResultFalse;
    if (offset >= 0)
    {
        // non-leftward seek
        if (mode == substream->kIBSeekSet)
            offset += start_pos;
        tr = substream->seek(offset, mode, newPos);
        if (newPos) *newPos -= start_pos;
    }
    else
    {
        // leftward seek
        if (mode == substream->kIBSeekSet) return kInvalidArgument;
        int64 pos1, pos2;
        if (substream->tell(&pos1) != kResultOk) return kResultFalse;
        tr = substream->seek(offset, mode, &pos2);
        if (tr != kResultOk)
        {
            // Host should not move position on failure, but double-check.
            if (substream->tell(&pos2) != kResultOk)
                pos2 = -1; // If even tell() fails, try a restore (below) as a last resort.
        }
        if (pos2 < start_pos)
        {
            // Position moved into hidden area. Restore it and signal failure.
            substream->seek(pos1, substream->kIBSeekSet, &pos2);
            if (tr == kResultOk) tr = kInvalidArgument;
        }
        if (newPos) *newPos = pos2 - start_pos;
    }
    LOG("WrappedStream::seek returned (subpos=%lu)\n", newSubpos - start_pos);
    return tr;
}

tresult PLUGIN_API WrappedStream::tell(int64* posOut)
{
    LOG("WrappedStream::tell called.\n");
    if (!substream) return kResultFalse;
    int64 subpos = (posOut ? *posOut : 0) + start_pos;
    const tresult tr = substream->tell(posOut ? &subpos : nullptr);
    if (posOut) *posOut = subpos - start_pos;
    return tr;
}

void FreerunSaveState_v1::byteswap() {
    data.tempo = std::bit_cast<double>(_byteswap_uint64(std::bit_cast<uint64_t>(data.tempo)));
    data.state = _byteswap_ulong(data.state);
    data.sigNum = _byteswap_ulong(data.sigNum);
    data.sigDen = _byteswap_ulong(data.sigDen);
};

void FreerunSaveState_v1::toContext(bool& fr, ProcessContext pc) const
{
    fr = data.freerunning;
    if (data.freerunning)
    {
        pc.state &= ~(pc.kPlaying | pc.kRecording);
        pc.state |= data.state & (pc.kPlaying | pc.kRecording);
        if (data.tempo > 0.0)
        {
            pc.tempo = data.tempo;
            pc.state |= pc.kTempoValid;
        }
        if ((data.sigNum > 0) && (data.sigDen > 0))
        {
            pc.timeSigNumerator = data.sigNum;
            pc.timeSigDenominator = data.sigDen;
            pc.state |= pc.kTimeSigValid;
        }
    }
}

void FreerunSaveState_v1::fromContext(bool fr, const ProcessContext pc)
{
    data.freerunning = fr;
    data.state = pc.state;
    data.tempo = (pc.state & pc.kTempoValid) ? pc.tempo : 0.0;
    data.sigNum = (pc.state & pc.kTimeSigValid) ? pc.timeSigNumerator : 0;
    data.sigDen = (pc.state & pc.kTimeSigValid) ? pc.timeSigDenominator : 0;
}
