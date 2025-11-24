#include <algorithm>
#include <amtl/am-string.h>
#include "extension.h"
#include "natives.h"

static cell_t Native_GetUploadedCrashCount(IPluginContext* context, const cell_t* params)
{
	return g_accelerator.GetUploadedCrashCount();
}

static cell_t Native_IsDoneUploadingCrashes(IPluginContext* context, const cell_t* params)
{
	if (g_accelerator.IsDoneUploading()) {
		return 1;
	}

	return 0;
}

static cell_t Native_GetCrashHTTPResponse(IPluginContext* context, const cell_t* params)
{
	if (!g_accelerator.IsDoneUploading()) {
		context->ReportError("Wait until accelerator is done uploading crashes before accessing crash information!");
		return 0;
	}

	int element = static_cast<int>(params[1]);
	const UploadedCrash* crash = g_accelerator.GetUploadedCrash(element);

	if (!crash) {
		context->ReportError("Crash index %i is invalid!", element);
		return 0;
	}

	char* buffer;
	context->LocalToString(params[2], &buffer);

	size_t maxsize = static_cast<size_t>(params[3]);

	ke::SafeStrcpy(buffer, maxsize, crash->GetHTTPResponse().c_str());

	return 0;
}

void natives::SetupNatives(std::vector<sp_nativeinfo_t>& vec)
{
	sp_nativeinfo_t list[] = {
		{"Accelerator_GetUploadedCrashCount", Native_GetUploadedCrashCount},
		{"Accelerator_IsDoneUploadingCrashes", Native_IsDoneUploadingCrashes},
		{"Accelerator_GetCrashHTTPResponse", Native_GetCrashHTTPResponse},
	};

	vec.insert(vec.end(), std::begin(list), std::end(list));
}
