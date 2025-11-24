/**
 * =============================================================================
 * Accelerator Extension
 * Copyright (C) 2009-2010 Asher Baker (asherkin).  All rights reserved.
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
 *
 */

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

/**
 * @file extension.hpp
 * @brief Accelerator extension code header.
 */

#include <atomic>
#include <vector>
#include <mutex>
#include "smsdk_ext.h"

/**
 * @brief Represents a crash that has been successfully uploaded to Accelerator's backend
 */
class UploadedCrash
{
public:
	UploadedCrash(const char* response) :
		m_httpresponse(response)
	{
	}

	const std::string& GetHTTPResponse() const { return m_httpresponse; }

private:
	std::string m_httpresponse; // HTTP response from the crash upload in free form text.
};

/**
 * @brief Sample implementation of the SDK Extension.
 */
class Accelerator : public SDKExtension, IPluginsListener
{
public: // SDKExtension
	Accelerator();

	/**
	 * @brief This is called after the initial loading sequence has been processed.
	 *
	 * @param error		Error message buffer.
	 * @param maxlength	Size of error message buffer.
	 * @param late		Whether or not the module was loaded after map load.
	 * @return			True to succeed loading, false to fail.
	 */
	virtual bool SDK_OnLoad(char *error, size_t maxlen, bool late);
	
	/**
	 * @brief This is called right before the extension is unloaded.
	 */
	virtual void SDK_OnUnload();

	/**
	 * @brief This is called once all known extensions have been loaded.
	 */
	virtual void SDK_OnAllLoaded();

	/**
	 * @brief Called on server activation before plugins receive the OnServerLoad forward.
	 * 
	 * @param pEdictList		Edicts list.
	 * @param edictCount		Number of edicts in the list.
	 * @param clientMax			Maximum number of clients allowed in the server.
	 */
	virtual void OnCoreMapStart(edict_t *pEdictList, int edictCount, int clientMax);

public: // IPluginsListener
	/**
	 * @brief Called when a plugin is fully loaded successfully.
	 */
	virtual void OnPluginLoaded(IPlugin *plugin);

	/**
	 * @brief Called when a plugin is unloaded (only if fully loaded).
	 */
	virtual void OnPluginUnloaded(IPlugin *plugin);

	/**
	 * @brief Stores an uploaded crash into the vector.
	 * @param crash Uploaded crash instance to store.
	 */
	void StoreUploadedCrash(UploadedCrash& crash);
	/**
	 * @brief Retrieves an uploaded crash from the given element.
	 * @param element Vector element (index) to read.
	 * @return Upload crash if found or NULL if out of bounds.
	 */
	const UploadedCrash* GetUploadedCrash(int element) const;
	/**
	 * @brief Returns the number of uploaded crashes (for SourcePawn plugins).
	 * @return Number of uploaded crashes.
	 */
	cell_t GetUploadedCrashCount() const;
	/**
	 * @brief Signals the extension that uploading is done.
	 */
	void MarkAsDoneUploading() { m_doneuploading.store(true); }
	/**
	 * @brief Is Accelerator done uploading crashes.
	 * @return Returns true if yes, false otherwise.
	 */
	bool IsDoneUploading() const { return m_doneuploading.load(); }
	/**
	 * @brief Has the 'OnMapStart' function called at least once.
	 * @return True if yes, false otherwise.
	 */
	bool IsMapStarted() const { return m_maphasstarted.load(); }

private:
	std::vector<UploadedCrash> m_uploadedcrashes; // Vector of uploaded crashes
	std::vector<sp_nativeinfo_t> m_natives; // Vector of SourcePawn natives
	mutable std::mutex m_uploadedcrashes_mutex; // mutex for accessing the m_uploadedcrashes vector
	std::atomic_bool m_doneuploading; // Signals that Accelerator is done uploading crashes.
	std::atomic_bool m_maphasstarted; // Signals that OnMapStart has been called at least once.
};

// Expose the extension singleton.
extern Accelerator g_accelerator;

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
