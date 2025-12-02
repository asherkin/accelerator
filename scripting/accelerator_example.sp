#include <sourcemod>
#include <accelerator>

#pragma newdecls required
#pragma semicolon 1

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

	// Call the forward manually. (For late loading plugins)
	if (Accelerator_IsDoneUploadingCrashes())
	{
		Accelerator_OnDoneUploadingCrashes();
	}
}

// This is called when the extension is done uploading crashes
public void Accelerator_OnDoneUploadingCrashes()
{
	LogMessage("Accelerator is done uploading crashes!");

	int max = Accelerator_GetUploadedCrashCount();

	if (max == 0)
	{
		LogMessage("No crashes were uploaded!");
		return;
	}

	char buffer[512];

	for (int i = 0; i < max; i++)
	{
		Accelerator_GetCrashHTTPResponse(i, buffer, sizeof(buffer));
		LogMessage("Crash #%i: HTTP reponse: \"%s\".", i, buffer);
	}
}

// Admin command to list crashes
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
