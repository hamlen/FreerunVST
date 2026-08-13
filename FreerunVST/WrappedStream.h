#pragma once

#pragma warning(push)
#pragma warning(disable : 4996)
#include "pluginterfaces/base/ibstream.h"
#include <pluginterfaces/vst/ivstprocesscontext.h>
#pragma warning(pop)

using namespace Steinberg;
using namespace Vst;

class WrappedStream : public IBStream
{
public:
    WrappedStream() { FUNKNOWN_CTOR }
    WrappedStream(IBStream* src);
    virtual ~WrappedStream() { FUNKNOWN_DTOR };

#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IBStream,substream
    BEGIN_WRAPPING
    tresult WRAP_METHOD_TR(write, (void* data, int32 numBytes, int32* numBytesWritten = nullptr), (data,numBytes,numBytesWritten));
    tresult WRAP_METHOD_TR(read, (void* data, int32 numBytes, int32* numBytesRead = nullptr), (data, numBytes,numBytesRead));
    tresult PLUGIN_API seek(int64 offset, int32 mode, int64* newPos = nullptr) SMTG_OVERRIDE;
    tresult PLUGIN_API tell(int64* posOut) SMTG_OVERRIDE;

    DECLARE_FUNKNOWN_METHODS

private:
    int64 start_pos = 0;
};

constexpr int64 freerun_magic = 0x0F33F333;
constexpr uint32 savedata_version = 1;

#pragma pack(push, 1)
class FreerunSaveStateHeader
{
public:
	int64 magic;
	uint32 savedata_version;

	void byteswap() {
		magic = _byteswap_uint64(magic);
		savedata_version = _byteswap_ulong(savedata_version);
	}
};
#pragma pack(pop)

class FreerunSaveState
{
public:
	FreerunSaveState() {};
	FreerunSaveState(void* _data, const int32 _size) : pData(_data), size(_size) {};

	void* const pData = nullptr;
	const int32 size = 0;

	virtual void byteswap() = 0;
	virtual void fromContext(bool fr, const ProcessContext pc) = 0;
	virtual void toContext(bool& freerunning, ProcessContext pc) const = 0;
};

#pragma pack(push, 1)
struct FreerunSaveData_v1
{
	double tempo = 0.0;
	int32 state = 0;
	int32 sigNum = 0, sigDen = 0;
	bool freerunning = false;
};
#pragma pack(pop)

class FreerunSaveState_v1 : public FreerunSaveState
{
public:
	FreerunSaveState_v1() : FreerunSaveState(&data, sizeof(data)) {};
	void byteswap() override;
	void fromContext(bool fr, const ProcessContext pc) override;
	void toContext(bool& freerunning, ProcessContext pc) const override;
private:
	FreerunSaveData_v1 data;
};
