#ifndef _INCLUDE_FORWARDS_H_
#define _INCLUDE_FORWARDS_H_

namespace extforwards
{
	// Initialize the sourcepawn forwards.
	void Init();
	// Shutdown the sourcepawn forwards.
	void Shutdown();
	// Calls the on done uploadind forward. (thread safe)
	void CallOnDoneUploadingForward();
}



#endif // !_INCLUDE_FORWARDS_H_
