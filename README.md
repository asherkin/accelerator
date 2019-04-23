# Accelerator
Accelerator replaces the default SRCDS crash handler with one that is a lot more reliable and uploads the crash reports to a community-accessible processing backend.
This is useful since Valve's processing system discards crash reports from modded servers - and they still receive a representative sample of crash reports for actual game issues.

The processing backend ([Throttle](https://crash.limetech.org/)) parses crash reports to extract useful information, and in the case of common issues affecting many servers where manual analysis has been done, tags them with helpful notices with information on resolving the cause of the crash.

## Installation
- [Download the latest build for your OS](https://builds.limetech.org/?p=accelerator)
- Extract the `.zip` file and merge the `addons/sourcemod` directory with the one on your server.

## More information
If you'd like to know more information then please read [this thread](https://forums.alliedmods.net/showthread.php?t=277703).
