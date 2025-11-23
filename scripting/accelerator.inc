/**
 * Accelerator's SourcePawn include file
 */
 
#if defined _accelerator_included
 #endinput
#endif
#define _accelerator_included


/**
 * Called a crash dump is uploaded.
 * 
 * @param index         Index of the crash.
 * @param response      HTTP response in plain text.
 * @param responsesize  HTTP response char buffer size.
 */
forward void Accelerator_OnCrashUploaded(int index, const char[] response, int responsesize);

/**
 * Do not edit below this line!
 */
public Extension __ext_accelerator =
{
	name = "accelerator",
	file = "accelerator.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};