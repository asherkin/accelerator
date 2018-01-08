#if defined _accelerator_included
 #endinput
#endif
#define _accelerator_included

/**
 * Called once a crash is upload to htts//crash.limetech.org
 * 
 * @param num           Upload index file.
 * @param crashId       Id that Web assign to that crash.
 */
forward void OnCrashUpdated(int num, const char [] crashId );

/**
 * Do not edit below this line!
 */
public Extension:__ext_accelerator = 
{
	name = "Accelerator",
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