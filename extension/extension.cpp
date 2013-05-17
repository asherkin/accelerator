/*
 * =============================================================================
 * Accelerator Extension
 * Copyright (C) 2011 Asher Baker (asherkin).  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "extension.h"

#include <IWebternet.h>
#include "MemoryDownloader.h"

#include "client/linux/handler/exception_handler.h"

#include <signal.h>
#include <dirent.h> 
#include <unistd.h>

Accelerator g_accelerator;
SMEXT_LINK(&g_accelerator);

IWebternet *webternet;
static IThreadHandle *uploadThread;

char buffer[255];
google_breakpad::ExceptionHandler *handler = NULL;

void (*SignalHandler)(int, siginfo_t *, void *);

const int kExceptionSignals[] = {
	SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS
};

const int kNumHandledSignals = sizeof(kExceptionSignals) / sizeof(kExceptionSignals[0]);

static bool dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded)
{
	printf("Wrote minidump to: %s\n", descriptor.path());
	return succeeded;
}

void OnGameFrame(bool simulating)
{
	bool weHaveBeenFuckedOver = false;
	struct sigaction oact;

	for (int i = 0; i < kNumHandledSignals; ++i) {
		sigaction(kExceptionSignals[i], NULL, &oact);

		if (oact.sa_sigaction != SignalHandler) {
			weHaveBeenFuckedOver = true;
			break;
		}
	}

	if (!weHaveBeenFuckedOver) {
		return;
	}

	struct sigaction act;
	memset(&act, 0, sizeof(act));
	sigemptyset(&act.sa_mask);

	for (int i = 0; i < kNumHandledSignals; ++i) {
		sigaddset(&act.sa_mask, kExceptionSignals[i]);
	}
	
	act.sa_sigaction = SignalHandler;
	act.sa_flags = SA_ONSTACK | SA_SIGINFO;

	for (int i = 0; i < kNumHandledSignals; ++i) {
		sigaction(kExceptionSignals[i], &act, NULL);
	}
}

void UploadCrashDump(const char *path)
{
	IWebForm *form = webternet->CreateForm();

	form->AddString("UserID", g_pSM->GetCoreConfigValue("MinidumpAccount"));

	form->AddFile("upload_file_minidump", path);

	MemoryDownloader data;
	IWebTransfer *xfer = webternet->CreateSession();
	xfer->SetFailOnHTTPError(true);

	if (!xfer->PostAndDownload("http://crash.limetech.org/submit", form, &data, NULL))
	{
		printf(">>> UPLOAD FAILED\n");
	} else {
		printf(">>> UPLOADED CRASH DUMP");
		printf("%s", data.GetBuffer());
	}
}

void Accelerator::OnCoreMapStart(edict_t *pEdictList, int edictCount, int clientMax)
{
	DIR *dumps = opendir(buffer);
	dirent *dump;

	char path[512];

	while ((dump = readdir(dumps)) != NULL)
	{
		if (dump->d_type == DT_DIR)
			continue;

		printf(">>> UPLOADING %s\n", dump->d_name);
		
		g_pSM->Format(path, sizeof(path), "%s/%s", buffer, dump->d_name);

		UploadCrashDump(path);
		unlink(path);
	}

	closedir(dumps);
}

bool Accelerator::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddDependency(myself, "webternet.ext", true, true);
	SM_GET_IFACE(WEBTERNET, webternet);

	g_pSM->BuildPath(Path_SM, buffer, 255, "data/dumps");

	google_breakpad::MinidumpDescriptor descriptor(buffer);
	handler = new google_breakpad::ExceptionHandler(descriptor, NULL, dumpCallback, NULL, true, -1);

	struct sigaction oact;
	sigaction(SIGSEGV, NULL, &oact);
	SignalHandler = oact.sa_sigaction;

	g_pSM->AddGameFrameHook(OnGameFrame);

	return true;
}

void Accelerator::SDK_OnUnload() 
{
	g_pSM->RemoveGameFrameHook(OnGameFrame);
}

