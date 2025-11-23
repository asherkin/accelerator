#include <array>
#include <string>
#include "extension.h"
#include "forwards.h"

static SourceMod::IForward* s_postuploadfoward = nullptr;

struct CrashPawnData
{
	int index;
	std::string response;
};

static void OnCrashUploadedCallback(void* data)
{
	CrashPawnData* crashdata = reinterpret_cast<CrashPawnData*>(data);
	
	if (s_postuploadfoward)
	{
		s_postuploadfoward->PushCell(crashdata->index);
		s_postuploadfoward->PushString(crashdata->response.c_str());
		s_postuploadfoward->PushCell(static_cast<cell_t>(crashdata->response.size()));
		s_postuploadfoward->Execute(nullptr);
	}
	
	delete crashdata;
}

void extforwards::Init()
{
	std::array types = {
		SourceMod::ParamType::Param_Cell,
		SourceMod::ParamType::Param_String,
		SourceMod::ParamType::Param_Cell,
	};

	s_postuploadfoward = forwards->CreateForward("Accelerator_OnCrashUploaded", SourceMod::ExecType::ET_Ignore, 3, types.data());
}

void extforwards::Shutdown()
{
	if (s_postuploadfoward)
	{
		forwards->ReleaseForward(s_postuploadfoward);
		s_postuploadfoward = nullptr;
	}
}

void extforwards::CallForward(int crashIndex, const char* response)
{
	CrashPawnData* data = new CrashPawnData;

	data->index = crashIndex;
	data->response.assign(response);

	// Accelerator uploads from a secondary thread, calling a forward directly will crash
	// this will call the forward from the main thread.
	smutils->AddFrameAction(OnCrashUploadedCallback, reinterpret_cast<void*>(data));
}
