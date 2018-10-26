#pragma once

#include "PGSuperCommandLineInfo.h"

#include <System\Date.h>
#include <PsgLib\LibraryManager.h>

class CPGSuperAppPluginApp : public CWinApp
{
public:

// Overrides
	virtual BOOL InitInstance();
	virtual int ExitInstance();

   CString GetVersion(bool bIncludeBuildNumber) const;


	DECLARE_MESSAGE_MAP()
};

extern class CPGSuperAppPluginApp theApp;
