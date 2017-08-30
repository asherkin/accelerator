#include "client/linux/handler/exception_handler.h"
#include "common/linux/linux_libc_support.h"
#include "third_party/lss/linux_syscall_support.h"

#include <google_breakpad/processor/minidump.h>
#include <google_breakpad/processor/minidump_processor.h>
#include <google_breakpad/processor/process_state.h>
#include <google_breakpad/processor/call_stack.h>
#include <google_breakpad/processor/stack_frame.h>
#include <processor/pathname_stripper.h>

#include <signal.h>
#include <dirent.h>
#include <unistd.h>
#include <setjmp.h>

#include <streambuf>

extern "C" {
#include "sha256.h"
}

using google_breakpad::MinidumpDescriptor;
using google_breakpad::ExceptionHandler;

using google_breakpad::Minidump;
using google_breakpad::MinidumpProcessor;
using google_breakpad::ProcessState;
using google_breakpad::ProcessResult;
using google_breakpad::CallStack;
using google_breakpad::StackFrame;
using google_breakpad::PathnameStripper;

sigjmp_buf envbuf;
char path[1024];

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
	siglongjmp(envbuf, 1);

	return succeeded;
}

int main(int argc, char *argv[])
{
	bool generateCrash = false;
	if (argc <= 1) {
		generateCrash = true;

		if (sigsetjmp(envbuf, 0) == 0) {
			MinidumpDescriptor descriptor(".");
			ExceptionHandler *handler = new ExceptionHandler(descriptor, NULL, dumpCallback, NULL, true, -1);

			// Test shit here.
			__builtin_trap();

			delete handler;
			return 0;
		}

		printf("Returned from signal handler, path: %s\n", path);

		argc = 2;
	}

	for (int i = 1; i < argc; ++i) {
		if (!generateCrash) {
			my_strlcpy(path, argv[i], sizeof(path));
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

		SHA256_CTX hashctx;
		sha256_init(&hashctx);

		sha256_update(&hashctx, (const unsigned char *)processState.crash_reason().data(), processState.crash_reason().length());

		//printf("1|%d|%s|%x|%d", processState.crashed(), processState.crash_reason().c_str(), (intptr_t)processState.crash_address(), requestingThread);
		for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
			const StackFrame *frame = stack->frames()->at(frameIndex);
			if (frame->module) {
				auto codeFile = PathnameStripper::File(frame->module->code_file());
				auto debugId = frame->module->debug_identifier();
				auto moduleOffset = frame->ReturnAddress() - frame->module->base_address();

				sha256_update(&hashctx, (const unsigned char *)codeFile.data(), codeFile.length());
				sha256_update(&hashctx, (const unsigned char *)debugId.data(), debugId.length());
				sha256_update(&hashctx, (const unsigned char *)&moduleOffset, sizeof(moduleOffset));

				//printf("|%d|%s|%s|%x", frame->trust, codeFile.c_str(), debugId.c_str(), (intptr_t)moduleOffset);
			} else {
				//printf("|%d|||%x", frame->trust, (intptr_t)frame->ReturnAddress());
			}
		}
		//printf("\n");

		unsigned char hash[SHA256_BLOCK_SIZE];
		sha256_final(&hashctx, hash);

		for (int c = 0; c < SHA256_BLOCK_SIZE; ++c) {
			printf("%02x", hash[c]);
		}
		printf("\n");
	}

	if (generateCrash) {
		unlink(path);
	}

	return 0;
}
