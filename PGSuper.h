///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// PGSuper.h : main header file for the PGSUPER application
//

#if !defined(AFX_PGSUPER_H__59D503E9_265C_11D2_8EB0_006097DF3C68__INCLUDED_)
#define AFX_PGSUPER_H__59D503E9_265C_11D2_8EB0_006097DF3C68__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include <EAF\EAFApp.h>

#include "MainFrm.h"

//   DECLARE_LOGFILE;
#if defined ENABLE_LOGGING
static dbgLogDumpContext m_Log;
#endif

class CXShutDown;
class CPGSuperCommandLineInfo;


extern class CComModule _Module;

/////////////////////////////////////////////////////////////////////////////
// CPGSuperApp:
// See PGSuper.cpp for the implementation of this class
//

class CPGSuperApp : public CEAFPluginApp
{
public:
	CPGSuperApp();

// CEAFApp overrides
public:
   virtual OLECHAR* GetAppPluginCategoryName();
   virtual CATID GetAppPluginCategoryID();

protected:
   virtual CEAFSplashScreenInfo GetSplashScreenInfo();
   virtual LPCTSTR GetRegistryKey();
   virtual CMDIFrameWnd* CreateMainFrame();
   virtual CDocManager* CreateDocumentManager();
   virtual CATID GetComponentInfoCategoryID();
   virtual CString GetProductCode();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPGSuperApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CPGSuperApp)
	afx_msg void OnAppAbout();
	afx_msg void OnHelpJoinArpList();
	afx_msg void OnHelpInetWsdot();
	afx_msg void OnHelpInetPgsuper();
   afx_msg void OnHelpInetARP();
   afx_msg void OnScreenSize();
	//}}AFX_MSG
   virtual BOOL OnCmdMsg(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo);
	DECLARE_MESSAGE_MAP()

public:
   CString GetVersion(bool bIncludeBuildNumber) const;
   CString GetVersionString(bool bIncludeBuildNumber) const;

   // URL's
   CString GetWsdotUrl();
   CString GetWsdotBridgeUrl();
   CString GetPGSuperUrl();

private:
   virtual void RegistryInit(); // All registry initialization goes here
   virtual void RegistryExit(); // All registry cleanup goes here
};

extern CPGSuperApp theApp;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PGSUPER_H__59D503E9_265C_11D2_8EB0_006097DF3C68__INCLUDED_)
