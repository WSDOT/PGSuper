///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

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
