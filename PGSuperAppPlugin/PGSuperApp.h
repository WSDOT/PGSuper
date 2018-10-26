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

   afx_msg void OnHelp();

   // Registery helper functions
   UINT GetLocalMachineInt(LPCTSTR lpszSection, LPCTSTR lpszEntry,int nDefault);
   CString GetLocalMachineString(LPCTSTR lpszSection, LPCTSTR lpszEntry,LPCTSTR lpszDefault);


	DECLARE_MESSAGE_MAP()

protected:
   HKEY GetAppLocalMachineRegistryKey();
   HKEY GetUninstallRegistryKey();
   HKEY GetLocalMachineSectionKey(LPCTSTR lpszSection);
   HKEY GetLocalMachineSectionKey(HKEY hAppKey,LPCTSTR lpszSection);
   UINT GetLocalMachineInt(HKEY hAppKey,LPCTSTR lpszSection, LPCTSTR lpszEntry,int nDefault);
   CString GetLocalMachineString(HKEY hAppKey,LPCTSTR lpszSection, LPCTSTR lpszEntry,LPCTSTR lpszDefault);
};

extern class CPGSuperAppPluginApp theApp;
