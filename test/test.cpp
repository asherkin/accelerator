#if defined _LINUX
#include "client/linux/handler/exception_handler.h"
#include "common/linux/linux_libc_support.h"
#include "third_party/lss/linux_syscall_support.h"
#include "common/linux/dump_symbols.h"
#include "common/path_helper.h"

#include <signal.h>
#include <dirent.h>
#include <unistd.h>
#include <paths.h>

#define my_jmp_buf sigjmp_buf
#define my_setjmp(x) sigsetjmp(x, 0)
#define my_longjmp siglongjmp

using google_breakpad::MinidumpDescriptor;
using google_breakpad::WriteSymbolFile;

class StderrInhibitor
{
	FILE *saved_stderr = nullptr;

public:
	StderrInhibitor() {
		saved_stderr = fdopen(dup(fileno(stderr)), "w");
		if (freopen(_PATH_DEVNULL, "w", stderr)) {
			// If it fails, not a lot we can (or should) do.
			// Add this brace section to silence gcc warnings.
		}
	}

	~StderrInhibitor() {
		fflush(stderr);
		dup2(fileno(saved_stderr), fileno(stderr));
		fclose(saved_stderr);
	}
};

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

#include <sstream>
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

class ClogInhibitor
{
	std::streambuf *saved_clog = nullptr;

public:
	ClogInhibitor() {
		saved_clog = std::clog.rdbuf();
		std::clog.rdbuf(nullptr);
	}

	~ClogInhibitor() {
		std::clog.rdbuf(saved_clog);
	}
};

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
	bool shouldDumpSymbols = false;
	if (argc > 1 && strcmp(argv[1], "-d") == 0) {
		shouldDumpSymbols = true;
	}

	bool generateCrash = false;
	if (argc <= (shouldDumpSymbols ? 2 : 1)) {
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

		argc = (shouldDumpSymbols ? 3 : 2);
	}

	for (int i = (shouldDumpSymbols ? 2 : 1); i < argc; ++i) {
		if (!generateCrash) {
			strncpy(path, argv[i], sizeof(path));
		}

		ProcessState processState;
		ProcessResult processResult;
		MinidumpProcessor minidumpProcessor(nullptr, nullptr);

		{
			ClogInhibitor clogInhibitor;
			processResult = minidumpProcessor.Process(path, &processState);
		}

		if (processResult != google_breakpad::PROCESS_OK) {
			continue;
		}

		std::string os_short = "";
		std::string cpu_arch = "";
		if (processState.system_info()) {
			os_short = processState.system_info()->os_short;
			if (os_short.empty()) {
				os_short = processState.system_info()->os;
			}
			cpu_arch = processState.system_info()->cpu;
		}

		int requestingThread = processState.requesting_thread();
		if (requestingThread == -1) {
			requestingThread = 0;
		}

		const CallStack *stack = processState.threads()->at(requestingThread);
		if (!stack) {
			continue;
		}

		int frameCount = stack->frames()->size();
		if (frameCount > 1024) {
			frameCount = 1024;
		}

		std::ostringstream summaryStream;
		summaryStream << 2 << "|" << processState.time_date_stamp() << "|" << os_short << "|" << cpu_arch << "|" << processState.crashed() << "|" << processState.crash_reason() << "|" << std::hex << processState.crash_address() << std::dec << "|" << requestingThread;

		std::map<const CodeModule *, unsigned int> moduleMap;

		unsigned int moduleCount = processState.modules() ? processState.modules()->module_count() : 0;
		for (unsigned int moduleIndex = 0; moduleIndex < moduleCount; ++moduleIndex) {
			auto module = processState.modules()->GetModuleAtIndex(moduleIndex);
			moduleMap[module] = moduleIndex;

			auto debugFile = PathnameStripper::File(module->debug_file());
			auto debugIdentifier = module->debug_identifier();

			summaryStream << "|M|" << debugFile << "|" << debugIdentifier;
		}

		for (int frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
			const StackFrame *frame = stack->frames()->at(frameIndex);

			int moduleIndex = -1;
			auto moduleOffset = frame->ReturnAddress();
			if (frame->module) {
				moduleIndex = moduleMap[frame->module];
				moduleOffset -= frame->module->base_address();
			}

			summaryStream << "|F|" << moduleIndex << "|" << std::hex << moduleOffset << std::dec;
		}

		auto summaryLine = summaryStream.str();
		printf("%s\n", summaryLine.c_str());

#if defined _LINUX
		for (unsigned int moduleIndex = 0; shouldDumpSymbols && moduleIndex < moduleCount; ++moduleIndex) {
			auto module = processState.modules()->GetModuleAtIndex(moduleIndex);

			auto debugFile = module->debug_file();
			std::string vdsoOutputPath = "";

			if (debugFile == "linux-gate.so") {
				FILE *auxvFile = fopen("/proc/self/auxv", "rb");
				if (auxvFile) {
					auto workingDir = getcwd(nullptr, 0);
					vdsoOutputPath = workingDir + std::string("/linux-gate.so");
					free(workingDir);

					while (!feof(auxvFile)) {
						int auxvEntryId = 0;
						fread(&auxvEntryId, sizeof(auxvEntryId), 1, auxvFile);
						long auxvEntryValue = 0;
						fread(&auxvEntryValue, sizeof(auxvEntryValue), 1, auxvFile);

						if (auxvEntryId == 0) break;
						if (auxvEntryId != 33) continue; // AT_SYSINFO_EHDR

						Elf32_Ehdr *vdsoHdr = (Elf32_Ehdr *)auxvEntryValue;
						auto vdsoSize = vdsoHdr->e_shoff + (vdsoHdr->e_shentsize * vdsoHdr->e_shnum);
						void *vdsoBuffer = malloc(vdsoSize);
						memcpy(vdsoBuffer, vdsoHdr, vdsoSize);

						FILE *vdsoFile = fopen(vdsoOutputPath.c_str(), "wb");
						if (vdsoFile) {
							fwrite(vdsoBuffer, 1, vdsoSize, vdsoFile);
							fclose(vdsoFile);
							debugFile = vdsoOutputPath;
						}

						free(vdsoBuffer);
						break;
					}

					fclose(auxvFile);
				}
			}

			if (debugFile[0] != '/') {
				continue;
			}

			// printf("%s\n", debugFile.c_str());

			auto debugFileDir = google_breakpad::DirName(debugFile);
			std::vector<string> debug_dirs{
				debugFileDir,
				debugFileDir + "/.debug",
				"/usr/lib/debug" + debugFileDir,
			};

			std::ostringstream outputStream;
			google_breakpad::DumpOptions options(ALL_SYMBOL_DATA, true);

			{
				StderrInhibitor stdrrInhibitor;

				if (!WriteSymbolFile(debugFileDir, debugFile, "Linux", debug_dirs, options, outputStream)) {
					outputStream.str("");
					outputStream.clear();

					// Try again without debug dirs.
					if (!WriteSymbolFile(debugFileDir, debugFile, "Linux", {}, options, outputStream)) {
						// TODO: Something.
						continue;
					}
				}
			}

			// WriteSymbolFileHeaderOnly would do this for us, but this is just for testing.
			auto output = outputStream.str();
			output = output.substr(0, output.find("\n"));
			printf("%s\n", output.c_str());

			if (debugFile == vdsoOutputPath) {
				unlink(vdsoOutputPath.c_str());
			}
		}
#endif
	}

	if (generateCrash) {
		unlink(path);
	}

	return 0;
}
