#ifndef _INCLUDE_NATIVES_H_
#define _INCLUDE_NATIVES_H_


namespace natives
{
	// Called during the load process to setup the extension natives
	void Setup(std::vector<sp_nativeinfo_t>& vec);
}

#endif // !_INCLUDE_NATIVES_H_
