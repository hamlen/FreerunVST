#pragma once

#include <Windows.h>

#pragma warning(push)
#pragma warning(disable : 4996)
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "base/source/fstring.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstautomationstate.h"
#include "pluginterfaces/vst/ivstchannelcontextinfo.h"
#include "pluginterfaces/vst/ivstinterappaudio.h"
#include "pluginterfaces/vst/ivstmidilearn.h"
#include "pluginterfaces/vst/ivstphysicalui.h"
#include "pluginterfaces/vst/ivstparameterfunctionname.h"
#include "pluginterfaces/vst/ivstprefetchablesupport.h"
#include "pluginterfaces/vst/ivstrepresentation.h"
#include "pluginterfaces/vst/ivstremapparamid.h"
#include "pluginterfaces/vst/ivstmidimapping2.h"
#pragma warning(pop)

#include "Main.h"
#include "HideParamChanges.h"

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace ChannelContext;

/* Q: Why must we cram all the following interfaces into one giant class?
 *
 * A: When a VST host calls createInstance to create a new plugin object, there is unfortunately no
 *    reliable way to know which interface(s) it needs on the returned object.  The createInstance
 *    call from the host specifies a class id and an interface id.  But the interface id could be a
 *    generic parent interface (e.g., IPluginBase) (which the host will later specialize by calling
 *    queryInterface), and the class id could specify a class that implements any combination of
 *    interfaces.  We therefore only discover at runtime which interfaces must be implemented by
 *    each object requested by the host.  To statically create a universal plugin wrapper class that
 *    reliably substitutes for any possible requested object, it must therefore implement all the
 *    plugin interfaces in the VST3 standard.
 * 
 *    To ensure that hosts' queryInterface calls aren't confused by this overabundance of interfaces,
 *    our universal objects implement a custom queryInterface that restricts them at runtime to only
 *    the interfaces supported by each wrapped plugin object.  Thus, at runtime our universal objects
 *    emulate only the interfaces that the underlying object loaded from the wrapped plugin supports.
 */

class FreerunVST : public FObject,
	public IPluginFactory3,

	public IComponent, public IPrefetchableSupport,
	public IAudioProcessor,
	public IProcessContextRequirements, public IUnitData, public IProgramListData, public IAudioPresentationLatency,

	public IEditController, public IEditController2,
	public IUnitInfo, public IMidiMapping, public IMidiLearn, public IEditControllerHostEditing, public IAutomationState,
	public IInterAppAudioConnectionNotification, public IInterAppAudioPresetManager, public IKeyswitchController,
	public INoteExpressionController, public INoteExpressionPhysicalUIMapping, public IParameterFunctionName,
	public IXmlRepresentationController, public IRemapParamID, public IMidiLearn2, public IMidiMapping2,

	public IConnectionPoint, public IInfoListener
{
public:
	FreerunVST(void) {};
	FreerunVST(FUnknown* subobj);
	~FreerunVST(void);

	// Create a custom queryInterface implementation that exports only and exactly the interfaces of the wrapped object.
	DEFINE_INTERFACES
		WRAPINTERFACE2(subcomponent, subbase)

		WRAPINTERFACE(subfactory)
		WRAPINTERFACE(subfactory2)
		WRAPINTERFACE(subfactory3)

		WRAPINTERFACE(subcomponent)
		WRAPINTERFACE(subprefetch)

		WRAPINTERFACE(subvst)
		WRAPINTERFACE(subproc)
		WRAPINTERFACE(subunitdata)
		WRAPINTERFACE(subproglist)
		WRAPINTERFACE(sublatency)

		WRAPINTERFACE(subcontroller)
		WRAPINTERFACE(subcontroller2)
		WRAPINTERFACE(submidimap)
		WRAPINTERFACE(submidilearn)
		WRAPINTERFACE(subunitinfo)
		WRAPINTERFACE(subedit)
		WRAPINTERFACE(subautomation)
		WRAPINTERFACE(subinter)
		WRAPINTERFACE(subpreset)
		WRAPINTERFACE(subkeyswitch)
		WRAPINTERFACE(subexpression)
		WRAPINTERFACE(subphysical)
		WRAPINTERFACE(subfuncname)
		WRAPINTERFACE(subxmlrep)
		WRAPINTERFACE(subremapparam)

		WRAPINTERFACE(subconnection)
	END_DEFINE_INTERFACES(FObject)

	DELEGATE_REFCOUNT(FObject)

	// IPluginBase members:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IPluginBase,subbase
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(initialize, (FUnknown* context), (context));
	tresult WRAP_METHOD_TR(terminate, (void), ());

	// IPluginFactory:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IPluginFactory,subfactory
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(getFactoryInfo, (PFactoryInfo* info), (info));
	int32 WRAP_METHOD_INT(countClasses, (void), ());
	tresult PLUGIN_API getClassInfo(int32 index, PClassInfo* info) override;
	tresult PLUGIN_API createInstance(FIDString cid, FIDString _iid, void** obj) override;

	// IPluginFactory2:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IPluginFactory2,subfactory2
	BEGIN_WRAPPING
	tresult PLUGIN_API getClassInfo2(int32 index, PClassInfo2* info) override;

	// IPluginFactory3:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IPluginFactory3,subfactory3
	BEGIN_WRAPPING
	tresult PLUGIN_API getClassInfoUnicode(int32 index, PClassInfoW* info) override;
	tresult WRAP_METHOD_TR(setHostContext, (FUnknown* context), (context));

	// IComponent members:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IComponent,subcomponent
	BEGIN_WRAPPING
#ifdef UNIQUE_CLASS_IDS
	tresult PLUGIN_API getControllerClassId(TUID classId) override;
#else
	tresult WRAP_METHOD_TR(getControllerClassId, (TUID classId), (classId));
#endif
	tresult WRAP_METHOD_TR(setIoMode, (IoMode mode), (mode));
	int32 WRAP_METHOD_INT(getBusCount, (MediaType type, BusDirection dir), (type,dir));
	tresult WRAP_METHOD_TR(getBusInfo, (MediaType type, BusDirection dir, int32 index, BusInfo& bus), (type,dir,index,bus));
	tresult WRAP_METHOD_TR(getRoutingInfo, (RoutingInfo& inInfo, RoutingInfo& outInfo), (inInfo,outInfo));
	tresult WRAP_METHOD_TR(activateBus, (MediaType type, BusDirection dir, int32 index, TBool state), (type,dir,index,state));
	tresult WRAP_METHOD_TR(setActive, (TBool state), (state));
private:
	tresult component_setState(IBStream* state);
	tresult component_getState(IBStream* state);
public:
	tresult PLUGIN_API IComponent::setState(IBStream* state) { return component_setState(state); }
	tresult PLUGIN_API IComponent::getState(IBStream* state) { return component_getState(state); }

	// IPrefetchableSupport member:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IPrefetchableSupport,subprefetch
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(getPrefetchableSupport, (PrefetchableSupport& prefetchable), (prefetchable));

	// IAudioProcessor members:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IAudioProcessor,subvst
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(setBusArrangements, (SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts), (inputs,numIns,outputs,numOuts));
	tresult WRAP_METHOD_TR(getBusArrangement, (BusDirection dir, int32 index, SpeakerArrangement& arr), (dir,index,arr));
	tresult WRAP_METHOD_TR(canProcessSampleSize, (int32 symbolicSampleSize), (symbolicSampleSize));
	uint32 WRAP_METHOD_INT(getLatencySamples, (void), ());
	tresult WRAP_METHOD_TR(setupProcessing, (ProcessSetup& newSetup), (newSetup));
	tresult WRAP_METHOD_TR(setProcessing, (TBool state), (state));
	tresult PLUGIN_API process(ProcessData& data) override;
	uint32 WRAP_METHOD_TR(getTailSamples, (void), ());
	
	// IProcessConnectionRequirements member:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IProcessContextRequirements,subproc
	BEGIN_WRAPPING
	uint32 PLUGIN_API getProcessContextRequirements(void) override;

	// IUnitData members:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IUnitData,subunitdata
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(unitDataSupported, (UnitID unitID), (unitID));
	tresult WRAP_METHOD_TR(getUnitData, (UnitID unitId, IBStream* data), (unitId,data));
	tresult WRAP_METHOD_TR(setUnitData, (UnitID unitId, IBStream* data), (unitId,data));

	// IProgramListData members:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IProgramListData,subproglist
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(programDataSupported, (ProgramListID listId), (listId));
	tresult WRAP_METHOD_TR(getProgramData, (ProgramListID listId, int32 programIndex, IBStream* data), (listId,programIndex,data));
	tresult WRAP_METHOD_TR(setProgramData, (ProgramListID listId, int32 programIndex, IBStream* data), (listId,programIndex,data));

	// IAudioPresentationLatency member
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IAudioPresentationLatency,sublatency
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(setAudioPresentationLatencySamples, (BusDirection dir, int32 busIndex, uint32 latencyInSamples), (dir,busIndex,latencyInSamples));

	// IEditController members:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IEditController,subcontroller
	BEGIN_WRAPPING
	tresult PLUGIN_API setComponentState(IBStream* state) override;
private:
	tresult controller_setState(IBStream* state);
	tresult controller_getState(IBStream* state);
public:
	tresult PLUGIN_API IEditController::setState(IBStream* state) { return controller_setState(state); }
	tresult PLUGIN_API IEditController::getState(IBStream* state) { return controller_getState(state); }
	int32 PLUGIN_API getParameterCount(void) override;
	tresult PLUGIN_API getParameterInfo(int32 paramIndex, ParameterInfo& info) override;
	tresult PLUGIN_API getParamStringByValue(ParamID tag, ParamValue valueNormalized, String128 string) override;
	tresult PLUGIN_API getParamValueByString(ParamID tag, TChar* string, ParamValue& valueNormalized) override;
	ParamValue PLUGIN_API normalizedParamToPlain(ParamID tag, ParamValue valueNormalized) override;
	ParamValue PLUGIN_API plainParamToNormalized(ParamID tag, ParamValue plainValue) override;
	ParamValue PLUGIN_API getParamNormalized(ParamID tag) override;
	tresult PLUGIN_API setParamNormalized(ParamID tag, ParamValue value) override;
	tresult WRAP_METHOD_TR(setComponentHandler, (IComponentHandler* handler), (handler));
	IPlugView* WRAP_METHOD_PTR(createView, (FIDString name), (name));

	// IEditController2 members:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IEditController2,subcontroller2
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(setKnobMode, (KnobMode mode), (mode));
	tresult WRAP_METHOD_TR(openHelp, (TBool onlyCheck), (onlyCheck));
	tresult WRAP_METHOD_TR(openAboutBox, (TBool onlyCheck), (onlyCheck));

	// IUnitData members:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IUnitInfo,subunitinfo
	BEGIN_WRAPPING
	int32 WRAP_METHOD_INT(getUnitCount, (void), ());
	tresult WRAP_METHOD_TR(getUnitInfo, (int32 unitIndex, UnitInfo& info), (unitIndex, info));
	int32 WRAP_METHOD_INT(getProgramListCount, (void), ());
	tresult WRAP_METHOD_TR(getProgramListInfo, (int32 listIndex, ProgramListInfo& info), (listIndex, info));
	tresult WRAP_METHOD_TR(getProgramName, (ProgramListID listId, int32 programIndex, String128 name), (listId, programIndex, name));
	tresult WRAP_METHOD_TR(getProgramInfo, (ProgramListID listId, int32 programIndex, Vst::CString attributeId, String128 attributeValue), (listId, programIndex, attributeId, attributeValue));
	tresult WRAP_METHOD_TR(hasProgramPitchNames, (ProgramListID listId, int32 programIndex), (listId, programIndex));
	tresult WRAP_METHOD_TR(getProgramPitchName, (ProgramListID listId, int32 programIndex, int16 midiPitch, String128 name), (listId, programIndex, midiPitch, name));
	UnitID WRAP_METHOD_INT(getSelectedUnit, (void), ());
	tresult WRAP_METHOD_TR(selectUnit, (UnitID unitId), (unitId));
	tresult WRAP_METHOD_TR(getUnitByBus, (MediaType type, BusDirection dir, int32 busIndex, int32 channel, UnitID& unitId), (type, dir, busIndex, channel, unitId));
	tresult WRAP_METHOD_TR(setUnitProgramData, (int32 listOrUnitId, int32 programIndex, IBStream* data), (listOrUnitId, programIndex, data));

	// IMidiMapping member:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IMidiMapping,submidimap
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(getMidiControllerAssignment, (int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& id), (busIndex, channel, midiControllerNumber, id));

	// IMidiLearn member:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IMidiLearn,submidilearn
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(onLiveMIDIControllerInput, (int32 busIndex, int16 channel, CtrlNumber midiCC), (busIndex,channel,midiCC));

	// IEditControllerHostEditing members:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IEditControllerHostEditing,subedit
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(beginEditFromHost, (ParamID paramID), (paramID));
	tresult WRAP_METHOD_TR(endEditFromHost, (ParamID paramID), (paramID));

	// IAutomationState member:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IAutomationState,subautomation
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(setAutomationState, (int32 state), (state));

	// IInterAppAudioConnectionNotification member:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IInterAppAudioConnectionNotification,subinter
	BEGIN_WRAPPING
	void WRAP_METHOD_VOID(onInterAppAudioConnectionStateChange, (TBool newState), (newState));

	// IInterAppAudioPresetManager members:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IInterAppAudioPresetManager,subpreset
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(runLoadPresetBrowser, (void), ());
	tresult WRAP_METHOD_TR(runSavePresetBrowser, (void), ());
	tresult WRAP_METHOD_TR(loadNextPreset, (void), ());
	tresult WRAP_METHOD_TR(loadPreviousPreset, (void), ());

	// IKeyswitchController members:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IKeyswitchController,subkeyswitch
	BEGIN_WRAPPING
	int32 WRAP_METHOD_INT(getKeyswitchCount, (int32 busIndex, int16 channel), (busIndex,channel));
	tresult WRAP_METHOD_TR(getKeyswitchInfo, (int32 busIndex, int16 channel, int32 keySwitchIndex, KeyswitchInfo& info), (busIndex,channel,keySwitchIndex,info));

	// INoteExpressionController members:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT INoteExpressionController,subexpression
	BEGIN_WRAPPING
	int32 WRAP_METHOD_INT(getNoteExpressionCount, (int32 busIndex, int16 channel), (busIndex,channel));
	tresult WRAP_METHOD_TR(getNoteExpressionInfo, (int32 busIndex, int16 channel, int32 noteExpressionIndex, NoteExpressionTypeInfo& info), (busIndex,channel,noteExpressionIndex,info));
	tresult WRAP_METHOD_TR(getNoteExpressionStringByValue, (int32 busIndex, int16 channel, NoteExpressionTypeID id, NoteExpressionValue valueNormalized, String128 string), (busIndex,channel,id,valueNormalized,string));
	tresult WRAP_METHOD_TR(getNoteExpressionValueByString, (int32 busIndex, int16 channel, NoteExpressionTypeID id, const TChar* string, NoteExpressionValue& valueNormalized), (busIndex,channel,id,string,valueNormalized));

	// INoteExpressionPhysicalUIMapping member:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT INoteExpressionPhysicalUIMapping,subphysical
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(getPhysicalUIMapping, (int32 busIndex, int16 channel, PhysicalUIMapList& list), (busIndex,channel,list));

	// IParameterFunctionName member:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IParameterFunctionName,subfuncname
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(getParameterIDFromFunctionName, (UnitID unitID, FIDString functionName, ParamID& paramID), (unitID,functionName,paramID));

	// IXmlRepresentationController member:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IXmlRepresentationController,subxmlrep
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(getXmlRepresentationStream, (RepresentationInfo& info, IBStream* stream), (info,stream));

	// IRemapParamID member:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IRemapParamID,subremapparam
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(getCompatibleParamID, (const TUID pluginToReplaceUID, ParamID oldParamID, ParamID& newParamID), (pluginToReplaceUID,oldParamID,newParamID));

	// IConnectionPoint members:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IConnectionPoint,subconnection
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(connect, (IConnectionPoint* other), (other));
	tresult WRAP_METHOD_TR(disconnect, (IConnectionPoint* other), (other));
	tresult WRAP_METHOD_TR(notify, (IMessage* message), (message));

	// IInfoListener member:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IInfoListener,subinfo
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(setChannelContextInfos, (IAttributeList* list), (list));

	// IMidiLearn2 members:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IMidiLearn2,submidilearn2
	BEGIN_WRAPPING
	tresult WRAP_METHOD_TR(onLiveMidi2ControllerInput, (BusIndex index, MidiChannel channel, Midi2Controller midiCC), (index,channel,midiCC));
	tresult WRAP_METHOD_TR(onLiveMidi1ControllerInput, (BusIndex index, MidiChannel channel, CtrlNumber midiCC), (index,channel,midiCC));

	// IMidiMapping2 members:
#	undef WRAPPED_OBJECT
#	define WRAPPED_OBJECT IMidiMapping2,submidimap2
	BEGIN_WRAPPING
	uint32 WRAP_METHOD_INT(getNumMidi2ControllerAssignments, (BusDirections direction), (direction));
	tresult WRAP_METHOD_TR(getMidi2ControllerAssignments, (BusDirections direction, const Midi2ControllerParamIDAssignmentList& list), (direction,list));
	uint32 WRAP_METHOD_INT(getNumMidi1ControllerAssignments, (BusDirections direction), (direction));
	tresult WRAP_METHOD_TR(getMidi1ControllerAssignments, (BusDirections direction, const Midi1ControllerParamIDAssignmentList& list), (direction,list));

protected:
	FUnknown* subunknown = nullptr;
	tresult readInternalState(IBStream* src);
	tresult writeInternalState(IBStream* dst) const;
	ParamID paramTags[num_freerun_params] = {};
	ParameterContainer* freerun_parameters = nullptr;
	HideParamChanges* fr_param_changes = nullptr;
	ProcessContext switchContext = {};
	int64 switchTime = 0;
	bool freerunning = false;
	bool refresh_freerun_parameters = true;
};
