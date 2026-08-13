#pragma warning(push)
#pragma warning(disable : 4996)
#include "pluginterfaces/vst/ivstparameterchanges.h"
#pragma warning(pop)

#include "Main.h"
#include "HideParamChanges.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

int32 HideParamChanges::hidden_param_index(ParamID id) const
{
	if (!paramTags) return -1;
	for (int32 i = 0; i < num_freerun_params; ++i)
		if (id == paramTags[i]) return i;
	return -1;
}

bool HideParamChanges::is_hidden_index(int32 index) const
{
	if (index < 0) return false;
	for (int32 i : hiddenIndexes)
		if (i == index) return true;
	return false;
}

int32 HideParamChanges::num_hidden_indexes_below(int32 index) const
{
	int32 count = 0;
	for (int32 i : hiddenIndexes)
		if ((i >= 0) && (i < index))
			++count;
	return count;
}

void HideParamChanges::setSubparams(IParameterChanges* paramChanges)
{
	subparams = paramChanges;
	memset(hiddenIndexes, -1, sizeof(hiddenIndexes));
	const int32 count = subparams->getParameterCount();
	for (int32 i = 0; i < count; ++i)
	{
		if (IParamValueQueue* q = subparams->getParameterData(i))
		{
			const ParamID id = q->getParameterId();
			const int32 j = hidden_param_index(id);
			if ((j >= 0) && (j < num_freerun_params))
				hiddenIndexes[j] = i;
		}
	}
}

int32 PLUGIN_API HideParamChanges::getParameterCount()
{
	if (!subparams) return 0;
	const int32 n = subparams->getParameterCount();
	return n - num_hidden_indexes_below(n);
}

IParamValueQueue* PLUGIN_API HideParamChanges::getParameterData(int32 index)
{
	if (!subparams) return nullptr;
	return subparams->getParameterData(index + num_hidden_indexes_below(index));
}

IParamValueQueue* PLUGIN_API HideParamChanges::addParameterData(const ParamID& id, int32& index)
{
	if (!subparams) return nullptr;
	if (hidden_param_index(id) >= 0)
	{
		// The wrapped plugin is trying to change/add an undeclared param with one of our reserved ids.
		// This is a violation of the VST API protocol, since plugins may not dynamically add new params,
		// and our reserved ids don't conflict with any of the plugin's declared ids.
		return nullptr;
	}
	if (IParamValueQueue* q = subparams->addParameterData(id, index))
	{
		if (is_hidden_index(index))
			return nullptr; // The host tried to give away one of our reserved indexes (should never happen).
		index -= num_hidden_indexes_below(index);
		return q;
	}
	return nullptr;
}
