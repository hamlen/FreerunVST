#pragma once

#pragma warning(push)
#pragma warning(disable : 4996)
#include "pluginterfaces/vst/ivstparameterchanges.h"
#pragma warning(pop)

#include "Main.h"

#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;

class HideParamChanges : public FObject, public IParameterChanges
{
public:
	HideParamChanges(void) {};
	HideParamChanges(const ParamID* tags) : paramTags(tags) {}
	~HideParamChanges(void) {};

	DEFINE_INTERFACES
		DEF_INTERFACE(IParameterChanges)
	END_DEFINE_INTERFACES(FObject)

	DELEGATE_REFCOUNT(FObject)

	int32 PLUGIN_API getParameterCount();
	IParamValueQueue* PLUGIN_API getParameterData(int32 index);
	IParamValueQueue* PLUGIN_API addParameterData(const ParamID& id, int32& index);

	void setSubparams(IParameterChanges* paramChanges);
private:
	int32 hidden_param(ParamID id) const;
	bool is_hidden_index(int32 index) const;
	int32 num_hidden_indexes_below(int32 index) const;
	IParameterChanges* subparams = nullptr;
	int32 hiddenIndexes[num_freerun_params];
	const ParamID* paramTags = nullptr;
};
