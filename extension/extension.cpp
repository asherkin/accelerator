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
#include "common/linux/linux_libc_support.h"
#include "third_party/lss/linux_syscall_support.h"

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
IGameConfig *gameconfig;

typedef void (*GetSpew_t)(char *buffer, unsigned int length);
GetSpew_t GetSpew;

char spewBuffer[65536]; // Hi.

char dumpStoragePath[512];
char logPath[512];

google_breakpad::ExceptionHandler *handler = NULL;

# if 0
struct PluginInfo {
	unsigned int serial;
	PluginStatus status;
	char filename[256];
	char name[256];
	char author[256];
	char description[256];
	char version[256];
	char url[256];
};

unsigned int plugin_count;
PluginInfo plugins[256];
#endif

#if defined _LINUX
void (*SignalHandler)(int, siginfo_t *, void *);

const int kExceptionSignals[] = {
	SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS
};

const int kNumHandledSignals = sizeof(kExceptionSignals) / sizeof(kExceptionSignals[0]);

static bool dumpCallback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded)
{
	//printf("Wrote minidump to: %s\n", descriptor.path());

	if (succeeded) {
		sys_write(STDOUT_FILENO, "Wrote minidump to: ", 19);
	} else {
		sys_write(STDOUT_FILENO, "Failed to write minidump to: ", 29);
	}

	sys_write(STDOUT_FILENO, descriptor.path(), my_strlen(descriptor.path()));
	sys_write(STDOUT_FILENO, "\n", 1);

	if (!succeeded) {
		return succeeded;
	}

	my_strlcpy(dumpStoragePath, descriptor.path(), sizeof(dumpStoragePath));
	my_strlcat(dumpStoragePath, ".txt", sizeof(dumpStoragePath));

	int extra = sys_open(dumpStoragePath, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (extra == -1) {
		sys_write(STDOUT_FILENO, "Failed to open metadata file!\n", 30);
		return succeeded;
	}

	if (GetSpew) {
		GetSpew(spewBuffer, sizeof(spewBuffer));
                sys_write(extra, "-------- CONSOLE HISTORY BEGIN --------\n", 40);
		sys_write(extra, spewBuffer, my_strlen(spewBuffer));
		sys_write(extra, "-------- CONSOLE HISTORY END --------\n", 38);
	}

#if 0
	char pis[64];
	char pds[32];
	for (unsigned i = 0; i < plugin_count; ++i) {
		PluginInfo *p = &plugins[i];
		if (p->serial == 0) continue;
		my_uitos(pds, i, my_uint_len(i));
		pds[my_uint_len(i)] = '\0';
		my_strlcpy(pis, "plugin[", sizeof(pis));
		my_strlcat(pis, pds, sizeof(pis));
		my_strlcat(pis, "].", sizeof(pis));
		sys_write(extra, pis, my_strlen(pis));
		sys_write(extra, "filename=", 9);
		sys_write(extra, p->filename, my_strlen(p->filename));
		sys_write(extra, "\n", 1);
		sys_write(extra, pis, my_strlen(pis));
		sys_write(extra, "name=", 5);
		sys_write(extra, p->name, my_strlen(p->name));
		sys_write(extra, "\n", 1);
		sys_write(extra, pis, my_strlen(pis));
		sys_write(extra, "author=", 7);
		sys_write(extra, p->author, my_strlen(p->author));
		sys_write(extra, "\n", 1);
		sys_write(extra, pis, my_strlen(pis));
		sys_write(extra, "description=", 12);
		sys_write(extra, p->description, my_strlen(p->description));
		sys_write(extra, "\n", 1);
		sys_write(extra, pis, my_strlen(pis));
		sys_write(extra, "version=", 8);
		sys_write(extra, p->version, my_strlen(p->version));
		sys_write(extra, "\n", 1);
		sys_write(extra, pis, my_strlen(pis));
		sys_write(extra, "url=", 4);
		sys_write(extra, p->url, my_strlen(p->url));
		sys_write(extra, "\n", 1);
	}
#endif

	sys_close(extra);

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
		case EXCEPTION_INVALID_HANDLE:
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		case EXCEPTION_DATATYPE_MISALIGNMENT:
		case EXCEPTION_ILLEGAL_INSTRUCTION:
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
		case EXCEPTION_STACK_OVERFLOW:
		case 0xC0000409: // STATUS_STACK_BUFFER_OVERRUN
		case 0xC0000374: // STATUS_HEAP_CORRUPTION
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
	if (!succeeded) {
		printf("Failed to write minidump to: %ls\\%ls.dmp\n", dump_path, minidump_id);
		return succeeded;
	}

	printf("Wrote minidump to: %ls\\%ls.dmp\n", dump_path, minidump_id);

	sprintf(dumpStoragePath, "%ls\\%ls.dmp.txt", dump_path, minidump_id);

	FILE *extra = fopen(dumpStoragePath, "wb");
	if (!extra) {
		printf("Failed to open metadata file!\n");
		return succeeded;
	}

	if (GetSpew) {
		GetSpew(spewBuffer, sizeof(spewBuffer));
		fprintf(extra, "-------- CONSOLE HISTORY BEGIN --------\n%s-------- CONSOLE HISTORY END --------\n", spewBuffer);
	}

        fclose(extra);

	return succeeded;
}
#else
#error Bad platform.
#endif

bool UploadAndDeleteCrashDump(const char *path, char *response, int maxlen)
{
	IWebForm *form = webternet->CreateForm();

	const char *minidumpAccount = g_pSM->GetCoreConfigValue("MinidumpAccount");
	if (minidumpAccount) form->AddString("UserID", minidumpAccount);

	form->AddString("GameDirectory", g_pSM->GetGameFolderName());
	form->AddString("ExtensionVersion", SMEXT_CONF_VERSION);

	form->AddFile("upload_file_minidump", path);

	char metapath[512];
	g_pSM->Format(metapath, sizeof(metapath), "%s.txt", path);
	if (libsys->PathExists(metapath)) {
		form->AddFile("upload_file_metadata", metapath);
	}

	MemoryDownloader data;
	IWebTransfer *xfer = webternet->CreateSession();
	xfer->SetFailOnHTTPError(true);

	bool uploaded = xfer->PostAndDownload("http://crash.limetech.org/submit", form, &data, NULL);	

	if (response) {
		if (uploaded) {
			g_pSM->Format(response, maxlen, "%s", data.GetBuffer());
		} else {
			g_pSM->Format(response, maxlen, "%s (%d)", xfer->LastErrorMessage(), xfer->LastErrorCode());
		}
	}

	if (libsys->PathExists(metapath)) {
		unlink(metapath);
	}

	unlink(path);

	return uploaded;
}

class UploadThread: public IThread
{
	void RunThread(IThreadHandle *pHandle) {
		rootconsole->ConsolePrint("Accelerator upload thread started.");

		FILE *log = fopen(logPath, "a");
		if (!log) {
			g_pSM->LogError(myself, "Failed to open Accelerator log file: %s", logPath);
		}

		IDirectory *dumps = libsys->OpenDirectory(dumpStoragePath);

		int count = 0;
		int failed = 0;
		char path[512];
		char response[512];

		while (dumps->MoreFiles()) {
			if (!dumps->IsEntryFile()) {
				dumps->NextEntry();
				continue;
			}

			const char *name = dumps->GetEntryName();

			int namelen = strlen(name);
			if (namelen < 4 || strcmp(&name[namelen-4], ".dmp") != 0) {
				dumps->NextEntry();
				continue;
			}

			g_pSM->Format(path, sizeof(path), "%s/%s", dumpStoragePath, name);
			bool uploaded = UploadAndDeleteCrashDump(path, response, sizeof(response));

			if (uploaded) {
				count++;
				g_pSM->LogError(myself, "Accelerator uploaded crash dump: %s", response);
				if (log) fprintf(log, "Uploaded crash dump: %s\n", response);
			} else {
				failed++;
				g_pSM->LogError(myself, "Accelerator failed to upload crash dump: %s", response);
                                if (log) fprintf(log, "Failed to upload crash dump: %s\n", response);
			}

			dumps->NextEntry();
		}

		libsys->CloseDirectory(dumps);

		if (log) {
			fclose(log);
		}

		rootconsole->ConsolePrint("Accelerator upload thread finished. (%d uploaded, %d failed)", count, failed);
	}

	void OnTerminate(IThreadHandle *pHandle, bool cancel) {
		rootconsole->ConsolePrint("Accelerator upload thread terminated. (canceled = %s)", (cancel ? "true" : "false"));
	}

} uploadThread;

bool Accelerator::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddDependency(myself, "webternet.ext", true, true);
	SM_GET_IFACE(WEBTERNET, webternet);

	g_pSM->BuildPath(Path_SM, dumpStoragePath, sizeof(dumpStoragePath), "data/dumps");

	if (!libsys->IsPathDirectory(dumpStoragePath))
	{
		if (!libsys->CreateFolder(dumpStoragePath))
		{
			if (error)
				g_pSM->Format(error, maxlength, "%s didn't exist and we couldn't create it :(", dumpStoragePath);
			return false;
		}
	}

	g_pSM->BuildPath(Path_SM, logPath, sizeof(logPath), "logs/accelerator.log");

	threader->MakeThread(&uploadThread);

	if (!gameconfs->LoadGameConfigFile("accelerator.games", &gameconfig, error, maxlength)) {
		return false;
	}

	if (!gameconfig->GetMemSig("GetSpew", (void **)&GetSpew)) {
		smutils->LogError(myself, "WARNING: GetSpew not found in gamedata, console output will not be included in crash reports.");
	} else if (!GetSpew) {
		smutils->LogError(myself, "WARNING: Sigscan for GetSpew failed, console output will not be included in crash reports.");
	}

#if defined _LINUX
	google_breakpad::MinidumpDescriptor descriptor(dumpStoragePath);
	handler = new google_breakpad::ExceptionHandler(descriptor, NULL, dumpCallback, NULL, true, -1);

	struct sigaction oact;
	sigaction(SIGSEGV, NULL, &oact);
	SignalHandler = oact.sa_sigaction;
	
	g_pSM->AddGameFrameHook(OnGameFrame);
#elif defined _WINDOWS
	wchar_t *buf = new wchar_t[sizeof(dumpStoragePath)];
	size_t num_chars = mbstowcs(buf, dumpStoragePath, sizeof(dumpStoragePath));

	handler = new google_breakpad::ExceptionHandler(std::wstring(buf, num_chars), NULL, dumpCallback, NULL, google_breakpad::ExceptionHandler::HANDLER_ALL);

	vectoredHandler = AddVectoredExceptionHandler(0, BreakpadVectoredHandler);
	
	delete buf;
#else
#error Bad platform.
#endif

#if 0
	IPluginIterator *i = plsys->GetPluginIterator();
	while (i->MorePlugins()) {
		IPlugin *p = i->GetPlugin();
		const sm_plugininfo_t *pmi = p->GetPublicInfo();
		PluginInfo *pi = &plugins[plugin_count++];

		pi->serial = p->GetSerial();
		pi->status = p->GetStatus();

		strncpy(pi->filename, p->GetFilename(), sizeof(pi->filename) - 1);

		strncpy(pi->name, pmi->name, sizeof(pi->name) - 1);
		strncpy(pi->author, pmi->author, sizeof(pi->author) - 1);
		strncpy(pi->description, pmi->description, sizeof(pi->description) - 1);
		strncpy(pi->version, pmi->version, sizeof(pi->version) - 1);
		strncpy(pi->url, pmi->url, sizeof(pi->url) - 1);

		i->NextPlugin();
	}
	delete i;
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

