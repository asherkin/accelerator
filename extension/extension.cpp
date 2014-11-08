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

#if defined _LINUX
#include "client/linux/handler/exception_handler.h"

#include <signal.h>
#include <dirent.h> 
#include <unistd.h>
#elif defined _WINDOWS
#define _STDINT // ~.~
#include "client/windows/handler/exception_handler.h"
#else
#error Bad platform.
#endif

Accelerator g_accelerator;
SMEXT_LINK(&g_accelerator);

IWebternet *webternet;
static IThreadHandle *uploadThread;

char buffer[255];
google_breakpad::ExceptionHandler *handler = NULL;

#if defined _LINUX
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
#elif defined _WINDOWS
void *vectoredHandler = NULL;

LONG CALLBACK BreakpadVectoredHandler(_In_ PEXCEPTION_POINTERS ExceptionInfo)
{
	switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
	{
		case EXCEPTION_ACCESS_VIOLATION:
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		case EXCEPTION_DATATYPE_MISALIGNMENT:
		case EXCEPTION_ILLEGAL_INSTRUCTION:
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
		case EXCEPTION_STACK_OVERFLOW:
			break;
		case 0: // Valve use this for Sys_Error.
			if ((ExceptionInfo->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE) == 0)
				return EXCEPTION_CONTINUE_SEARCH;
			break;
		default:
			return EXCEPTION_CONTINUE_SEARCH;
	}
	
	if (handler->WriteMinidumpForException(ExceptionInfo))
	{
		// Stop the handler thread from deadlocking us.
		delete handler;
		
		// Stop Valve's handler being called.
		ExceptionInfo->ExceptionRecord->ExceptionCode = EXCEPTION_BREAKPOINT;
		
		return EXCEPTION_EXECUTE_HANDLER;
	} else {
		return EXCEPTION_CONTINUE_SEARCH;
	}
}

static bool dumpCallback(const wchar_t* dump_path,
                         const wchar_t* minidump_id,
                         void* context,
                         EXCEPTION_POINTERS* exinfo,
                         MDRawAssertionInfo* assertion,
                         bool succeeded)
{
	printf("Wrote minidump to: %ls\\%ls.dmp\n", dump_path, minidump_id);
	return succeeded;
}
#else
#error Bad platform.
#endif

void UploadCrashDump(const char *path)
{
	IWebForm *form = webternet->CreateForm();

	form->AddString("UserID", g_pSM->GetCoreConfigValue("MinidumpAccount"));
	form->AddString("GameDir", g_pSM->GetGameFolderName());
	form->AddString("ExtensionVersion", SMEXT_CONF_VERSION);

	form->AddFile("upload_file_minidump", path);

	MemoryDownloader data;
	IWebTransfer *xfer = webternet->CreateSession();
	xfer->SetFailOnHTTPError(true);

	printf(">>> UPLOADING %s\n", path);
	
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
	IDirectory *dumps = libsys->OpenDirectory(buffer);

	char path[512];
	int count = 0;

	while (dumps->MoreFiles())
	{
		if (!dumps->IsEntryFile())
		{
			dumps->NextEntry();
			continue;
		}
		
		g_pSM->Format(path, sizeof(path), "%s/%s", buffer, dumps->GetEntryName());
		UploadCrashDump(path);
		
		int err = 0;
#if defined _LINUX
		err = unlink(path);
#elif defined _WINDOWS
		err = _unlink(path);
#else
#error Bad platform.
#endif
		if (err != 0) {
			printf(">>> FAILED TO DELETE CRASH DUMP!!!\n");
		}

		count++;
		dumps->NextEntry();
	}

	libsys->CloseDirectory(dumps);
	
	if (count > 0) {
		printf(">>> UPLOADED %d CRASH DUMPS\n", count);
	}
}

bool Accelerator::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddDependency(myself, "webternet.ext", true, true);
	SM_GET_IFACE(WEBTERNET, webternet);

	g_pSM->BuildPath(Path_SM, buffer, sizeof(buffer), "data/dumps");

	if (!libsys->IsPathDirectory(buffer))
	{
		if (!libsys->CreateFolder(buffer))
		{
			if (error)
				g_pSM->Format(error, maxlength, "%s didn't exist and we couldn't create it :(", buffer);
			return false;
		}
	}

#if defined _LINUX
	google_breakpad::MinidumpDescriptor descriptor(buffer);
	handler = new google_breakpad::ExceptionHandler(descriptor, NULL, dumpCallback, NULL, true, -1);

	struct sigaction oact;
	sigaction(SIGSEGV, NULL, &oact);
	SignalHandler = oact.sa_sigaction;
	
	g_pSM->AddGameFrameHook(OnGameFrame);
#elif defined _WINDOWS
	wchar_t *buf = new wchar_t[sizeof(buffer)];
	size_t num_chars = mbstowcs(buf, buffer, sizeof(buffer));

	handler = new google_breakpad::ExceptionHandler(std::wstring(buf, num_chars), NULL, dumpCallback, NULL, google_breakpad::ExceptionHandler::HANDLER_ALL);

	vectoredHandler = AddVectoredExceptionHandler(0, BreakpadVectoredHandler);
	
	delete buf;
#else
#error Bad platform.
#endif

	return true;
}

void Accelerator::SDK_OnUnload() 
{
#if defined _LINUX
	g_pSM->RemoveGameFrameHook(OnGameFrame);
#elif defined _WINDOWS
	if (vectoredHandler) {
		RemoveVectoredExceptionHandler(vectoredHandler);
	}
#else
#error Bad platform.
#endif

	delete handler;
}

