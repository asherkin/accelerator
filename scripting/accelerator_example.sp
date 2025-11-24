#include <sourcemod>
#include <accelerator>

#pragma newdecls required
#pragma semicolon 1

bool g_bLoggedCrashes = false;
Handle g_Timer = null;

public Plugin myinfo =
{
	name = "Accelerator Example",
	author = "caxanga334",
	description = "Example Accelerator natives plugin.",
	version = "1.0.0",
	url = "https://github.com/asherkin/accelerator"
};


public void OnPluginStart()
{
	RegAdminCmd("sm_listcrashes", Command_ListCrashes, ADMFLAG_RCON, "List all uploaded crash dumps.");
}

public void OnMapStart()
{
	if (!g_bLoggedCrashes)
	{
		if (g_Timer == null)
		{
			g_Timer = CreateTimer(0.1, Timer_LogCrashes, .flags = TIMER_REPEAT);
		}
	}
}

// This is called when the extension is done uploading crashes
public void Accelerator_OnDoneUploadingCrashes()
{
	LogMessage("Accelerator is done uploading crashes!");
}

Action Command_ListCrashes(int client, int args)
{
	if (!Accelerator_IsDoneUploadingCrashes())
	{
		ReplyToCommand(client, "Accelerator is still uploading crashes, please wait!");
		return Plugin_Handled;
	}

	int max = Accelerator_GetUploadedCrashCount();

	if (max == 0)
	{
		ReplyToCommand(client, "No crashes were uploaded!");
		return Plugin_Handled;
	}

	char buffer[512];

	for (int i = 0; i < max; i++)
	{
		Accelerator_GetCrashHTTPResponse(i, buffer, sizeof(buffer));
		ReplyToCommand(client, "Crash #%i: HTTP reponse: \"%s\".", i, buffer);
	}

	return Plugin_Handled;
}

Action Timer_LogCrashes(Handle timer)
{
	if (!Accelerator_IsDoneUploadingCrashes())
	{
		return Plugin_Continue;
	}

	int max = Accelerator_GetUploadedCrashCount();

	if (max == 0)
	{
		LogMessage("No crashes were uploaded!");
		g_Timer = null;
		g_bLoggedCrashes = true;
		return Plugin_Stop;
	}

	char buffer[512];

	for (int i = 0; i < max; i++)
	{
		Accelerator_GetCrashHTTPResponse(i, buffer, sizeof(buffer));
		LogMessage("Crash #%i: HTTP reponse: \"%s\".", i, buffer);
	}

	g_Timer = null;
	g_bLoggedCrashes = true;
	return Plugin_Stop;
}