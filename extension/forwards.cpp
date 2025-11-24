#include <array>
#include <string>
#include "extension.h"
#include "forwards.h"


static SourceMod::IForward* s_ondoneuploadingforward = nullptr;

static void OnDoneUploadingCallback(void* data)
{
	if (s_ondoneuploadingforward) {
		s_ondoneuploadingforward->Execute();
	}
}


void extforwards::Init()
{
	s_ondoneuploadingforward = forwards->CreateForward("Accelerator_OnDoneUploadingCrashes", SourceMod::ExecType::ET_Ignore, 0, nullptr);
}

void extforwards::Shutdown()
{
	if (s_ondoneuploadingforward) {
		forwards->ReleaseForward(s_ondoneuploadingforward);
		s_ondoneuploadingforward = nullptr;
	}
}

void extforwards::CallOnDoneUploadingForward()
{
	smutils->AddFrameAction(OnDoneUploadingCallback, nullptr);
}

