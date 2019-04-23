# Accelerator
Accelerator replaces the default SRCDS crash handler with one that is a lot more reliable and uploads the crash reports to a community-accessible processing backend.
This is useful since Valve's processing system discards crash reports from modded servers - and they still receive a representative sample of crash reports for actual game issues.

The server's crash report is sent to the processing backend service ([Throttle](https://crash.limetech.org/)) for analysis.
Throttle will compare the crash to other existing crash reports and provide a link to an overview page of the matching crash signature and could include information on resolving the cause of the crash.

## Installation
- [Download the latest build for your OS](https://builds.limetech.org/?p=accelerator)
- Extract the `.zip` file and merge the `addons/sourcemod` directory with the one on your server.

## Configuration

Edit `addons/sourcemod/configs/core.cfg` in order to link your crash reports to your steam account
```
"MinidumpAccount"  "<insert your steam64 id here>"
```

## More information
If you'd like to know more information then please read [this thread](https://forums.alliedmods.net/showthread.php?t=277703).
