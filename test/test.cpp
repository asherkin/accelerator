#if defined _LINUX
#include "client/linux/handler/exception_handler.h"
#include "common/linux/linux_libc_support.h"
#include "third_party/lss/linux_syscall_support.h"

#include <signal.h>
#include <dirent.h>
#include <unistd.h>

#define my_jmp_buf sigjmp_buf
#define my_setjmp(x) sigsetjmp(x, 0)
#define my_longjmp siglongjmp

using google_breakpad::MinidumpDescriptor;

#elif defined _WINDOWS
#define _STDINT // ~.~
#include "client/windows/handler/exception_handler.h"

#define my_jmp_buf jmp_buf
#define my_setjmp(x) setjmp(x)
#define my_longjmp longjmp

#else
#error Bad platform.
#endif

#include <google_breakpad/processor/minidump.h>
#include <google_breakpad/processor/minidump_processor.h>
#include <google_breakpad/processor/process_state.h>
#include <google_breakpad/processor/call_stack.h>
#include <google_breakpad/processor/stack_frame.h>
#include <processor/pathname_stripper.h>

#include <streambuf>

#include <setjmp.h>

using google_breakpad::ExceptionHandler;

using google_breakpad::Minidump;
using google_breakpad::MinidumpProcessor;
using google_breakpad::ProcessState;
using google_breakpad::ProcessResult;
using google_breakpad::CallStack;
using google_breakpad::StackFrame;
using google_breakpad::PathnameStripper;
using google_breakpad::CodeModule;

my_jmp_buf envbuf;
char path[1024];

#if defined _LINUX
static bool dumpCallback(const MinidumpDescriptor &descriptor, void *context, bool succeeded)
{
	if (succeeded) {
		sys_write(STDOUT_FILENO, "Wrote minidump to: ", 19);
	} else {
		sys_write(STDOUT_FILENO, "Failed to write minidump to: ", 29);
	}

	sys_write(STDOUT_FILENO, descriptor.path(), my_strlen(descriptor.path()));
	sys_write(STDOUT_FILENO, "\n", 1);

	my_strlcpy(path, descriptor.path(), sizeof(path));
	my_longjmp(envbuf, 1);

	return succeeded;
}
#elif defined _WINDOWS
static bool dumpCallback(const wchar_t *dump_path, const wchar_t *minidump_id, void *context, EXCEPTION_POINTERS *exinfo, MDRawAssertionInfo *assertion, bool succeeded)
{
	if (succeeded) {
		printf("Wrote minidump to: %ls\\%ls.dmp\n", dump_path, minidump_id);
	} else {
		printf("Failed to write minidump to: %ls\\%ls.dmp\n", dump_path, minidump_id);
	}

	// TODO: setjmp/longjmp doesn't play nicely with SEH on Windows, so we never get back.
	// But the exception handler is called and writes the dump, so the user can just invoke us again.

	// snprintf(path, sizeof(path), "%ls\\%ls.dmp", dump_path, minidump_id);
	// my_longjmp(envbuf, 1);

	return succeeded;
}
#endif

int main(int argc, char *argv[])
{
	bool generateCrash = false;
	if (argc <= 1) {
		generateCrash = true;

		if (my_setjmp(envbuf) == 0) {
#if defined _LINUX
			MinidumpDescriptor descriptor(".");
			ExceptionHandler *handler = new ExceptionHandler(descriptor, NULL, dumpCallback, NULL, true, -1);
#elif defined _WINDOWS
			ExceptionHandler *handler = new ExceptionHandler(L".", NULL, dumpCallback, NULL, ExceptionHandler::HANDLER_ALL);
#endif

			volatile int *ptr = (volatile int *)(0xdeadbeef);
			*ptr = 0;

			delete handler;
			return 0;
		}

		printf("Returned from signal handler, path: %s\n", path);

		argc = 2;
	}

	for (int i = 1; i < argc; ++i) {
		if (!generateCrash) {
			strncpy(path, argv[i], sizeof(path));
		}

		MinidumpProcessor minidumpProcessor(nullptr, nullptr);

		std::streambuf *old = std::clog.rdbuf();
		std::clog.rdbuf(nullptr);

		ProcessState processState;
		ProcessResult processResult = minidumpProcessor.Process(path, &processState);

		std::clog.rdbuf(old);

		if (processResult != google_breakpad::PROCESS_OK) {
			continue;
		}

		int requestingThread = processState.requesting_thread();
		if (requestingThread == -1) {
			requestingThread = 0;
		}

		const CallStack *stack = processState.threads()->at(requestingThread);
		if (!stack) {
			fprintf(stderr, "Minidump missing stack!\n");
			continue;
		}

		int frameCount = stack->frames()->size();
		if (frameCount > 10) {
			frameCount = 10;
		}

		printf("1|%d|%s|%x|%d", processState.crashed(), processState.crash_reason().c_str(), (intptr_t)processState.crash_address(), requestingThread);

		std::map<const CodeModule *, unsigned int> moduleMap;

		unsigned int moduleCount = processState.modules()->module_count();
		for (unsigned int moduleIndex = 0; moduleIndex < moduleCount; ++moduleIndex) {
			auto module = processState.modules()->GetModuleAtIndex(moduleIndex);
			moduleMap[module] = moduleIndex;

			auto debugFile = PathnameStripper::File(module->debug_file());
			auto debugIdentifier = module->debug_identifier();
			printf("|M|%s|%s", debugFile.c_str(), debugIdentifier.c_str());
		}

		for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
			const StackFrame *frame = stack->frames()->at(frameIndex);
			if (frame->module) {
				auto moduleIndex = moduleMap[frame->module];
				auto moduleOffset = frame->ReturnAddress() - frame->module->base_address();

				printf("|F|%d|%x", moduleIndex, (intptr_t)moduleOffset);
			} else {
				printf("|F|%d|%x", -1, (intptr_t)frame->ReturnAddress());
			}
		}

		printf("\n");
	}

	if (generateCrash) {
		unlink(path);
	}

	return 0;
}
