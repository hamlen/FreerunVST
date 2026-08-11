#pragma warning(push)
#pragma warning(disable : 4996)
#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/fstrdefs.h"
#pragma warning(pop)

#include "FreerunVST.h"
#include "WrappedStream.h"
#include <set>

static ParamID freshTag(std::set<ParamID> usedTags, ParamID tag)
{
	for (ParamID offset = 0; tag + offset != tag; ++offset)
	{
		if ((tag + offset >= 0) && (usedTags.count(tag + offset) == 0))
		{
			usedTags.insert(tag + offset);
			return tag + offset;
		}
		else if ((tag - offset >= 0) && (usedTags.count(tag - offset) == 0))
		{
			usedTags.insert(tag - offset);
			return tag - offset;
		}
	}
	return tag;
}

#define INITSUB(m) subunknown->queryInterface(std::remove_pointer_t<decltype(m)>::iid, (void**)&m)
#define DEINITSUB(m) if (m) { m->release(); m = nullptr; }

FreerunVST::FreerunVST(FUnknown* subobj)
{
	LOG("FreerunVST constructor called.\n");

	// Increase the reference count for the sub-library
	(void)load_sublibrary();

	subunknown = subobj;
	INITSUB(subbase);

	INITSUB(subfactory);
	INITSUB(subfactory2);
	INITSUB(subfactory3);

	INITSUB(subcomponent);
	INITSUB(subprefetch);

	INITSUB(subvst);
	INITSUB(subproc);
	INITSUB(subunitdata);
	INITSUB(subproglist);
	INITSUB(sublatency);

	INITSUB(subcontroller);
	INITSUB(subcontroller2);
	INITSUB(submidimap);
	INITSUB(submidimap2);
	INITSUB(submidilearn);
	INITSUB(submidilearn2);
	INITSUB(subunitinfo);
	INITSUB(subedit);
	INITSUB(subautomation);
	INITSUB(subinter);
	INITSUB(subpreset);
	INITSUB(subkeyswitch);
	INITSUB(subexpression);
	INITSUB(subphysical);
	INITSUB(subfuncname);
	INITSUB(subxmlrep);

	INITSUB(subconnection);
	INITSUB(subinfo);

	std::set<ParamID> usedTags;
	if (subcontroller)
	{
		const int32 count = subcontroller->getParameterCount();
		for (int32 i = 0; i < count; ++i)
		{
			ParameterInfo info;
			if (subcontroller->getParameterInfo(i, info) == kResultOk)
				usedTags.insert(info.id);
		}
	}

	freerun_parameters = new ParameterContainer();
	paramTags[0] = freshTag(usedTags, default_freerunning_param_id);
	RangeParameter* rp = new RangeParameter(STR16("Freerun"), paramTags[0], STR16(""), 0.0, 1.0, 0.0);
	if (rp)
	{
		rp->setPrecision(0);
		freerun_parameters->addParameter(rp);
	}
	fr_param_changes = new HideParamChanges(paramTags);
}

FreerunVST::~FreerunVST(void)
{
	LOG("FreerunVST destructor called.\n");

	if (fr_param_changes) delete fr_param_changes;
	if (freerun_parameters) delete freerun_parameters;

	DEINITSUB(subinfo);
	DEINITSUB(subconnection);

	DEINITSUB(subxmlrep);
	DEINITSUB(subfuncname);
	DEINITSUB(subphysical);
	DEINITSUB(subexpression);
	DEINITSUB(subkeyswitch);
	DEINITSUB(subpreset);
	DEINITSUB(subinter);
	DEINITSUB(subautomation);
	DEINITSUB(subedit);
	DEINITSUB(subunitinfo);
	DEINITSUB(submidilearn2);
	DEINITSUB(submidilearn);
	DEINITSUB(submidimap2);
	DEINITSUB(submidimap);
	DEINITSUB(subcontroller2);
	DEINITSUB(subcontroller);

	DEINITSUB(sublatency);
	DEINITSUB(subproglist);
	DEINITSUB(subunitdata);
	DEINITSUB(subproc);
	DEINITSUB(subvst);

	DEINITSUB(subprefetch);
	DEINITSUB(subcomponent);

	DEINITSUB(subfactory3);
	DEINITSUB(subfactory2);
	DEINITSUB(subfactory);

	DEINITSUB(subbase);
	DEINITSUB(subunknown);

	// Decrease the reference count for the sub-library (possibly unloading it).
	// We should really call the sub-library's ExitDll function if it's about to unload,
	// but there's no thread-safe way to do it.  Many VST hosts ignore ExitDll anyway.
	FreeLibrary(hmodule);
}

#ifdef UNIQUE_CLASS_IDS
#ifdef LOGGING
static void printTUID(const char* tuid, char* out)
{
	for (int i = 0; i < sizeof(TUID); ++i)
	{
		unsigned char c = static_cast<unsigned char>(tuid[i]);
		out[i * 2] =     "0123456789ABCDEF"[c >> 4];
		out[i * 2 + 1] = "0123456789ABCDEF"[c & 0x0F];
	}
	out[2 * sizeof(TUID)] = 0;
}
#endif

static void translateCID(char* cid)
{
	if (cid)
	{
#ifdef LOGGING
		char oldid[64], myid[64], newid[64];
		printTUID(cid, oldid);
#endif
		const char* freerun_id = FreerunUID.toTUID();
		for (size_t i = 0; i < sizeof(TUID); ++i)
			cid[i] ^= freerun_id[i];
#ifdef LOGGING
		printTUID(freerun_id, myid);
		printTUID(cid, newid);
		LOG("translateCID: %s ^ %s -> %s\n", oldid, myid, newid);
#endif
	}
}

tresult PLUGIN_API FreerunVST::getControllerClassId(TUID classId)
{
	if (!subcomponent) return kResultFalse;
	const tresult tr = subcomponent->getControllerClassId(classId);
	if (tr == kResultOk)
		translateCID(classId);
	return tr;
}
#else
static void translateCID(TUID cid) {}
#endif

tresult PLUGIN_API FreerunVST::createInstance(FIDString cid, FIDString _iid, void** obj)
{
	LOG("FreerunVST::createInstance called.\n");
	if (subfactory)
	{
		FUnknown* subobj;
#ifdef UNIQUE_CLASS_IDS
		TUID new_tuid;
		memcpy(new_tuid, cid, 16);
		translateCID(new_tuid);
#else
		FIDString const new_tuid = cid;
#endif
		tresult tr = subfactory->createInstance(new_tuid, _iid, (void**)&subobj);
		if (tr == kResultOk)
		{
			FUnknown* ap;
			if ((subobj->queryInterface(IAudioProcessor::iid, (void**)&ap) == kResultOk) ||
				(subobj->queryInterface(IEditController::iid, (void**)&ap) == kResultOk) ||
				(subobj->queryInterface(IPluginFactory::iid, (void**)&ap) == kResultOk))
			{
				LOG("FreerunVST::createInstance: Detected known interface. Proxying it.\n");
				ap->release();
				FreerunVST* t = new FreerunVST(subobj); // t->refCount = 1
				tr = t->queryInterface(_iid, obj);      // t->refCount = 2
				t->release();                           // t->refCount = 1
				return tr;
			}
			LOG("FreerunVST::createInstance: returning unknown interface without proxying.\n");
			*obj = subobj;
			return kResultOk;
		}
		LOG("FreerunVST::createInstance: subfactory->createInstance returned error %d.\n", tr);
		return tr;
	}
	LOG("FreerunVST::createInstance has no subfactory.\n");
	return kResultFalse;
}

static size_t strnlen16_s(const char16* s, size_t max_len) {
	size_t i = 0;
	while (i < max_len && s[i])
		++i;
	return i;
}

tresult PLUGIN_API FreerunVST::getClassInfo(int32 index, PClassInfo* info)
{
	LOG("FreerunVST::getClassInfo called.\n");
	if (!subfactory) return kResultFalse;
	tresult tr = subfactory->getClassInfo(index, info);
	if (tr == kResultOk)
	{
		constexpr size_t bufsize = sizeof(info->name);
		constexpr size_t sufsize = sizeof(DISPLAYED_NAME_SUFFIX);
		size_t namelen = strnlen_s(info->name, bufsize);
		memcpy(info->name + ((namelen + sufsize <= bufsize) ? namelen : (bufsize - sufsize)), DISPLAYED_NAME_SUFFIX, sufsize);
		translateCID(info->cid);
	}
	LOG("FreerunVST::getClassInfo: returning class name as: %s\n", info->name);
	return tr;
}

tresult PLUGIN_API FreerunVST::getClassInfo2(int32 index, PClassInfo2* info)
{
	LOG("FreerunVST::getClassInfo2 called.\n");
	if (!subfactory2) return kResultFalse;
	tresult tr = subfactory2->getClassInfo2(index, info);
	if (tr == kResultOk)
	{
		constexpr size_t bufsize = sizeof(info->name);
		constexpr size_t sufsize = sizeof(DISPLAYED_NAME_SUFFIX);
		size_t namelen = strnlen_s(info->name, bufsize);
		memcpy(info->name + ((namelen + sufsize <= bufsize) ? namelen : (bufsize - sufsize)), DISPLAYED_NAME_SUFFIX, sufsize);
		translateCID(info->cid);
	}
	LOG("FreerunVST::getClassInfo2: returning class name as: %s\n", info->name);
	return tr;
}

tresult PLUGIN_API FreerunVST::getClassInfoUnicode(int32 index, PClassInfoW* info)
{
	LOG("FreerunVST::getClassInfoUnicode called.\n");
	if (!subfactory3) return kResultFalse;
	tresult tr = subfactory3->getClassInfoUnicode(index, info);
	if (tr == kResultOk)
	{
		constexpr size_t bufsize = sizeof(info->name);
		constexpr size_t sufsize = sizeof(DISPLAYED_NAME_SUFFIX);
		size_t namelen = strnlen16_s(info->name, bufsize);
		memcpy(info->name + ((namelen + sufsize <= bufsize) ? namelen : (bufsize - sufsize)), STR16(DISPLAYED_NAME_SUFFIX), sufsize * sizeof(*info->name));
		translateCID(info->cid);
	}
	LOG("FreerunVST::getClassInfoUnicode: returning class name as: %ls\n", info->name);
	return tr;
}

tresult FreerunVST::writeInternalState(IBStream* dst) const
{
	LOG("FreerunVST::writeInternalState called.\n");
	if (!dst) return kResultFalse;
	const FreerunSaveStateHeader hdr = { freerun_magic, savedata_version };
	FreerunSaveState_v1 ss;
	ss.fromContext(freerunning, switchContext);
	tresult tr = dst->write((void*)&hdr, sizeof(hdr));
	if (tr != kResultOk) return tr;
	tr = dst->write(ss.pData, ss.size);
	return tr;
}

tresult FreerunVST::readInternalState(IBStream* src)
{
	LOG("FreerunVST::readInternalState called.\n");
	if (!src) return kResultFalse;
	FreerunSaveStateHeader hdr;
	int64 pos;
	if (src->tell(&pos) != kResultOk) return kResultFalse;
	int32 numBytesRead = 0;
	tresult tr = src->read(&hdr, sizeof(hdr), &numBytesRead);
	if ((tr != kResultOk) || (numBytesRead != sizeof(hdr)))
	{
		LOG("FreerunVST::readInternalState couldn't read the stream.\n");
		return kResultFalse;
	}
	bool swap_endian = false;
	if (hdr.magic != freerun_magic)
	{
		hdr.byteswap();
		if (hdr.magic == freerun_magic)
			swap_endian = true;
		else
		{
			LOG("FreerunVST::readInternalState did not find magic number.\n");
			src->seek(pos, IBStream::IStreamSeekMode::kIBSeekSet);
			return kResultFalse;
		}
	}
	LOG("FreerunVST::readInternalState found magic number.\n");

	FreerunSaveState *ss = nullptr;
	if (hdr.savedata_version == 1)
	{
		FreerunSaveState_v1 ss1;
		ss = &ss1;
	}
	if (!ss)
	{
		LOG("FreerunVST::readInternalState found unrecognized save version %u.\n", hdr.savedata_version);
		return kResultFalse;
	}
	tr = src->read(ss->pData, ss->size, &numBytesRead);
	if ((tr == kResultOk) && (numBytesRead != ss->size)) tr = kResultFalse;
	if (tr == kResultOk)
	{
		if (swap_endian) ss->byteswap();
		ss->toContext(freerunning, switchContext);
	}
	else
	{
		LOG("FreerunVST::readInternalState couldn't read internal state block.\n");
		src->seek(pos, IBStream::IStreamSeekMode::kIBSeekSet);
		return kResultFalse;
	}

	refresh_freerun_parameters = true;

	LOG("FreerunVST::readInternalState returned.\n");
	return kResultTrue;
}

// Write the processor's state to the stream.
tresult FreerunVST::component_getState(IBStream* stream)
{
	LOG("FreerunVST::component_getState called.\n");
	if (!subcomponent) return kResultFalse;
	const tresult tr = writeInternalState(stream);
	if (tr != kResultOk) return tr;
	WrappedStream ws(stream);
	return subcomponent->getState(&ws);
}

// Write the controller's state to the stream.
tresult FreerunVST::controller_getState(IBStream* stream)
{
	LOG("FreerunVST::controller_getState called.\n");
	if (!subcontroller) return kResultFalse;
	const tresult tr = writeInternalState(stream);
	if (tr != kResultOk) return tr;
	WrappedStream ws(stream);
	return subcontroller->getState(&ws);
}

// Read the processor's state from the stream.
tresult FreerunVST::component_setState(IBStream* stream)
{
	LOG("FreerunVST::component_setState called.\n");
	if (!subcomponent) return kResultFalse;
	const tresult tr = readInternalState(stream);
	if (tr != kResultOk) return subcomponent->setState(stream);
	WrappedStream ws(stream);
	return subcomponent->setState(&ws);
}

// Read the controller's state from the stream.
tresult FreerunVST::controller_setState(IBStream* stream)
{
	LOG("FreerunVST::controller_setState called.\n");
	if (!subcontroller) return kResultFalse;
	const tresult tr = readInternalState(stream);
	if (tr != kResultOk) return subcontroller->setState(stream);
	WrappedStream ws(stream);
	return subcontroller->setState(&ws);
}

// Controller reads processor's state from the stream.
tresult FreerunVST::setComponentState(IBStream* stream)
{
	LOG("FreerunVST::setComponentState called.\n");
	if (!subcontroller) return kResultFalse;
	const tresult tr = readInternalState(stream);
	if (tr != kResultOk) return subcontroller->setComponentState(stream);
	WrappedStream ws(stream);
	return subcontroller->setComponentState(&ws);
}

int32 PLUGIN_API FreerunVST::getParameterCount()
{
	return (subcontroller ? subcontroller->getParameterCount() : 0) + num_freerun_params;
}

tresult PLUGIN_API FreerunVST::getParameterInfo(int32 paramIndex, ParameterInfo& info)
{
	LOG("FreerunVST::getParameterInfo called.\n");
	if (!subcontroller) return kResultFalse;
	const int32 subcount = subcontroller->getParameterCount();
	if (paramIndex < subcount)
		return subcontroller->getParameterInfo(paramIndex, info);
	Parameter* param = freerun_parameters->getParameterByIndex(paramIndex - subcount);
	if (param)
	{
		info = param->getInfo();
		return kResultTrue;
	}
	return kResultFalse;
}

tresult PLUGIN_API FreerunVST::getParamStringByValue(ParamID tag, ParamValue valueNormalized, String128 string)
{
	LOG("FreerunVST::getParamStringByValue called.\n");
	if (!subcontroller) return kResultFalse;
	Parameter* param = freerun_parameters->getParameter(tag);
	if (param)
	{
		param->toString(valueNormalized, string);
		return kResultTrue;
	}
	return subcontroller->getParamStringByValue(tag, valueNormalized, string);
}

tresult PLUGIN_API FreerunVST::getParamValueByString(ParamID tag, TChar* string, ParamValue& valueNormalized)
{
	LOG("FreerunVST::getParamValueByString called.\n");
	if (!subcontroller) return kResultFalse;
	Parameter* param = freerun_parameters->getParameter(tag);
	if (param)
	{
		if (param->fromString(string, valueNormalized))
			return kResultTrue;
	}
	return subcontroller->getParamValueByString(tag, string, valueNormalized);
}

ParamValue PLUGIN_API FreerunVST::normalizedParamToPlain(ParamID tag, ParamValue valueNormalized)
{
	LOG("FreerunVST::normalizedParamToPlain called.\n");
	if (!subcontroller) return kResultFalse;
	Parameter* param = freerun_parameters->getParameter(tag);
	return param ? param->toPlain(valueNormalized) : subcontroller->normalizedParamToPlain(tag, valueNormalized);
}

ParamValue PLUGIN_API FreerunVST::plainParamToNormalized(ParamID tag, ParamValue plainValue)
{
	LOG("FreerunVST::plainParamToNormalized called.\n");
	if (!subcontroller) return kResultFalse;
	Parameter* param = freerun_parameters->getParameter(tag);
	return param ? param->toNormalized(plainValue) : subcontroller->plainParamToNormalized(tag, plainValue);
}

ParamValue PLUGIN_API FreerunVST::getParamNormalized(ParamID tag)
{
	LOG("FreerunVST::getParamNormalized called.\n");
	if (!subcontroller) return kResultFalse;
	Parameter* param = freerun_parameters->getParameter(tag);
	return param ? param->getNormalized() : subcontroller->getParamNormalized(tag);
}

tresult PLUGIN_API FreerunVST::setParamNormalized(ParamID tag, ParamValue value)
{
	LOG("FreerunVST::setParamNormalized called.\n");
	if (!subcontroller) return kResultFalse;
	Parameter* param = freerun_parameters->getParameter(tag);
	if (param)
	{
		param->setNormalized(value);
		return kResultTrue;
	}
	return subcontroller->setParamNormalized(tag, value);
}

uint32 PLUGIN_API FreerunVST::getProcessContextRequirements(void)
{
	uint32 req = subproc ? subproc->getProcessContextRequirements() : 0;
	req |= kNeedTempo;
	if (req & kNeedBarPositionMusic)
		req |= kNeedTimeSignature | kNeedProjectTimeMusic;
	return req;
}

static int64 safe_round(double d, const int64 imax, const double dmax)
{
	if (std::isnan(d)) return 0;
	d = std::round(d);
	if (d <= 0.0) return 0;
	if (d > dmax) return imax;
	return static_cast<int64>(d);
}
#define safeRound(d,max) safe_round((d),(max),static_cast<double>((max)))

static int64 currentSystemTime()
{
	static LARGE_INTEGER freq = {};
	if (!freq.QuadPart)
		QueryPerformanceFrequency(&freq);

	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return
		(counter.QuadPart / freq.QuadPart) * 1'000'000'000LL +
		((counter.QuadPart % freq.QuadPart) * 1'000'000'000LL / freq.QuadPart);
}

// Update a ProcessContext pc to a freerunning state equal to startContext plus delta_seconds:
// 1. Copy tempo, time signature, and smpte frame rate from startContext into pc.
// 2. Copy transport state (playing/recording), and transport position validity from startContext into pc,
//    and clear pc's next-midi-clock and cycling-state (which are never valid during freerunning).
// 3. Advance pc's projectTimeSamples, continousTimeSamples (sic), projectTimeMusic, barPositionMusic, and smpteOffsetSubframes
//    delta_seconds beyond their respective values in startContext.
// Precondition: pc must have accurate sampleRate (preserved).
static void update_times(ProcessContext& pc, const ProcessContext& startContext, const double delta_seconds)
{
	const TSamples oldPTS = startContext.projectTimeSamples;
	const TSamples oldCTS = startContext.continousTimeSamples;
	const int32 oldSOSF = startContext.smpteOffsetSubframes;

	// Copy tempo, time signature, frame rate, and transport positions (if pc and startContext don't refer to the same structure).
	if (&pc != &startContext)
	{
		pc.tempo = startContext.tempo;
		pc.timeSigNumerator = startContext.timeSigNumerator;
		pc.timeSigDenominator = startContext.timeSigDenominator;
		pc.frameRate = startContext.frameRate;

		pc.projectTimeSamples = startContext.projectTimeSamples;
		pc.continousTimeSamples = startContext.continousTimeSamples;
		pc.projectTimeMusic = startContext.projectTimeMusic;
		pc.barPositionMusic = startContext.barPositionMusic;
		pc.smpteOffsetSubframes = startContext.smpteOffsetSubframes;
	}

	// Update the state bitfield
	uint32 newstate = pc.state & ~(
		pc.kPlaying | pc.kRecording
		| pc.kBarPositionValid | pc.kClockValid
		| pc.kContTimeValid | pc.kCycleActive | pc.kCycleValid | pc.kProjectTimeMusicValid
		| pc.kSmpteValid | pc.kTempoValid | pc.kTimeSigValid
		);
	newstate |= startContext.state & (
		pc.kPlaying | pc.kRecording
		| pc.kTempoValid | pc.kTimeSigValid
		| pc.kContTimeValid | pc.kSmpteValid
		);
	if (newstate & pc.kTempoValid)
	{
		newstate |= startContext.state & pc.kProjectTimeMusicValid;
		if ((newstate & pc.kProjectTimeMusicValid) && (newstate & pc.kTimeSigValid) && (pc.timeSigNumerator > 0) && (pc.timeSigDenominator > 0))
			newstate |= startContext.state & pc.kBarPositionValid;
	}
	pc.state = newstate;

	// Compute shift amount in units of quarter notes, samples, and smpte subframes.
	const TQuarterNotes delta_qn = (pc.tempo / 60.0) * delta_seconds;
	const TSamples delta_samples = safeRound(pc.sampleRate * delta_seconds, kMaxInt64);
	const int32 delta_subframes = static_cast<int32>(safeRound(80.0 * static_cast<double>(pc.frameRate.framesPerSecond) * delta_seconds, kMaxInt32));

	// Advance projectTimeSamples
	pc.projectTimeSamples += delta_samples;
	if (pc.projectTimeSamples < oldPTS)
	{
		// Integer overflow: Start computing from zero to get back into low-integer territory.
		pc.projectTimeSamples = delta_samples;
		pc.continousTimeSamples = 0;
		pc.barPositionMusic -= pc.projectTimeMusic; // makes bar position negative (allowed by VST standard)
		pc.projectTimeMusic = 0.0;
		pc.smpteOffsetSubframes = 0;
	}

	// Advance continousTimeSamples
	pc.continousTimeSamples += delta_samples;
	if (pc.continousTimeSamples < oldCTS) pc.state &= ~pc.kContTimeValid;

	// Advance projectTimeMusic
	pc.projectTimeMusic += delta_qn;

	// Advance barPositionMusic
	if ((pc.timeSigNumerator > 0) && (pc.timeSigDenominator > 0))
	{
		const TQuarterNotes qn_per_bar = static_cast<TQuarterNotes>(4 * pc.timeSigNumerator) / static_cast<TQuarterNotes>(pc.timeSigDenominator);
		pc.barPositionMusic += std::floor((pc.projectTimeMusic - pc.barPositionMusic + delta_qn) / qn_per_bar) * qn_per_bar;
	}

	// Advance smpteOffsetSubframes
	pc.smpteOffsetSubframes += delta_subframes;
	if (pc.smpteOffsetSubframes < oldSOSF) pc.state &= ~pc.kSmpteValid;
}

static BOOL WINAPI return_zero(ULONGLONG* hns)
{
	hns = 0;
	return TRUE;
}

// If our very first process() call is already freerunning, we must infer a reasonable switchContext
// from (1) the live context passed to process(), (2) the context loaded from the saved plugin state,
// and (3) the current system time.  For example, tempo should come from the saved context, but
// sample rate must be preserved from the live context (since otherwise the buffers we're about to
// pass to the hosted plugin would have incorrect timing); and the transport position needs to be
// something all plugin instances can agree upon without explicit communication, and that doesn't
// depend on the exact timing at which instances were saved, loaded, initialized, etc., so that
// all instances that start life in freerunning mode remain in sync with each other.
static int64 init_preswitched_context(const ProcessContext* liveContext, ProcessContext& newContext)
{
	// Preserved from saved state: tempo, timeSigNumerator, timeSigDenominator, frameRate
	newContext.state &= newContext.kTempoValid | newContext.kTimeSigValid | newContext.kSmpteValid;

	// Chord, sample rate, and system time are drawn from the live context.
	newContext.state |= liveContext->state & (liveContext->kChordValid | liveContext->kSystemTimeValid);
	newContext.chord = liveContext->chord;
	newContext.sampleRate = liveContext->sampleRate;
	newContext.systemTime = liveContext->systemTime;

	// Transport position data is zeroed for now, since it will be computed from the clock.
	newContext.projectTimeSamples = 0;
	newContext.continousTimeSamples = 0;
	newContext.projectTimeMusic = 0.0;
	newContext.barPositionMusic = 0.0;
	newContext.smpteOffsetSubframes = 0;
	newContext.cycleStartMusic = 0.0;
	newContext.cycleEndMusic = 0.0;
	newContext.samplesToNextClock = 0;

	// Our basis for selecting a transport position for plugin instances that begin life in freerunning mode
	// will be the system-up time.  The time at which the system went up is precise (to the hectonanosecond),
	// is consistently agreed among all plugin instances on the same machine, and is hopefully a feasible
	// song duration that avoids passing absurdly huge values to hosted plugins.
	typedef BOOL(WINAPI* QUIT)(ULONGLONG*);
	static QUIT pQUIT = NULL;
	if (!pQUIT)
	{
		if (HMODULE hKernel32 = GetModuleHandleA("kernel32.dll"))
		{
			pQUIT = reinterpret_cast<QUIT>(GetProcAddress(hKernel32, "QueryUnbiasedInterruptTimePrecise")); // Windows 10+
			if (!pQUIT) pQUIT = reinterpret_cast<QUIT>(GetProcAddress(hKernel32, "QueryUnbiasedInterruptTime")); // Windows 8+ fallback
		}
		if (!pQUIT) pQUIT = return_zero; // final fallback (just start the timeline now)
	}
	ULONGLONG hns;
	_ReadWriteBarrier(); // Instruct the compiler to please keep the two clock queries together.
	pQUIT(&hns);
	const int64 systime = currentSystemTime();
	_ReadWriteBarrier();

	// Position the initial transport to the number of seconds since system-up.
	// Also copy transport state (playing/recording) and validity of transport elements
	// (continousTimeSamples (sic), projectTimeMusic, barPositionMusic, and smpteOffsetSubframes)
	// from the saved state, since that allows the user to lock the plugin into a desired
	// transport state and save it in freerunning mode.
	update_times(newContext, newContext, static_cast<double>(hns) / 1e7);

	// Return the switch time as the system-up time.
	const int64 start_time = systime - static_cast<int64>(hns * 100);
	return (start_time <= systime) ? start_time : 0LL;
}

tresult PLUGIN_API FreerunVST::process(ProcessData& data)
{
	if (!data.processContext)
	{
		LOG("FreerunVST::process: no processContext.\n");
		return subvst ? subvst->process(data) : kResultFalse;
	}

	// On first call to process(), initialize the switchContext using best available data.
	// If we're not already in freerunning mode, this initial data will be ignored
	// and later replaced at the first switch to freerunning.
	if (switchContext.sampleRate <= 0.0)
		switchTime = init_preswitched_context(data.processContext, switchContext);

	// Detect any change to/from freerunning mode.
	const int32 numParamChanges = data.inputParameterChanges ? data.inputParameterChanges->getParameterCount() : 0;
	for (int32 i = 0; i < numParamChanges; ++i)
	{
		IParamValueQueue* q = data.inputParameterChanges->getParameterData(i);
		if (q)
		{
			const ParamID tag = q->getParameterId();
			if (tag == paramTags[0])
			{
				const int32 n = q->getPointCount();
				if (n > 0)
				{
					int32 offset;
					ParamValue value;
					if (q->getPoint(n - 1, offset, value) == kResultOk)
					{
						if (!freerunning && (value >= 0.5))
						{
							switchContext = *data.processContext;
							switchTime = currentSystemTime();
						}
						freerunning = (value < 0.5) ? false : true;
					}
				}
			}
		}
	}

	// If in freerunning mode, replace the host's transport with the freerunning transport.
	tresult tr = kResultFalse;
	if (subvst)
	{
		if (freerunning && (switchContext.sampleRate > 0.0))
		{
			ProcessData pd = data;
			ProcessContext pc = *data.processContext;
			fr_param_changes->setSubparams(pd.inputParameterChanges);
			pd.inputParameterChanges = fr_param_changes;
			pd.processContext = &pc;
			update_times(pc, switchContext, static_cast<double>(currentSystemTime() - switchTime) / 1e9);
			LOG("FreerunVST::process: freerunning at sample position %ld.\n", pc.projectTimeSamples);
			tr = subvst->process(pd);
		}
		else
		{
			tr = subvst->process(data);
		}
	}
	else
	{
		LOG("FreerunVST::process: no subvst!\n");
	}

	// If any internal parameters changed, inform the host.
	if (refresh_freerun_parameters && data.outputParameterChanges)
	{
		static_assert(num_freerun_params == 1, "param refresh code needs updating");
		int32 dummy;
		IParamValueQueue* q = data.outputParameterChanges->addParameterData(paramTags[0], dummy);
		if (q)
		{
			if (q->addPoint(0, freerunning ? 1.0 : 0.0, dummy) == kResultOk)
				refresh_freerun_parameters = false;
		}
	}

	return tr;
}
