///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
#include "PGSuperCatalogServers.h"
#include "PGSuperCommandLineInfo.h"

#include <System\Date.h>
#include <PsgLib\LibraryManager.h>

//   DECLARE_LOGFILE;
#if defined ENABLE_LOGGING
static dbgLogDumpContext m_Log;
#endif

class CXShutDown;
class CPGSuperCommandLineInfo;

enum CacheUpdateFrequency
{
   Never,
   Always,
   Daily,
   Weekly,
   Monthly
};


extern class CComModule _Module;

/////////////////////////////////////////////////////////////////////////////
// CPGSuperApp:
// See PGSuper.cpp for the implementation of this class
//

class CPGSuperApp : public CEAFApp
{
public:
	CPGSuperApp();

// CEAFApp overrides
protected:
   virtual CEAFSplashScreenInfo GetSplashScreenInfo();
   virtual LPCTSTR GetRegistryKey();
   virtual BOOL CreateApplicationPlugins();
   virtual CMDIFrameWnd* CreateMainFrame();
   virtual CDocManager* CreateDocumentManager();
   virtual CEAFCommandLineInfo& GetCommandLineInfo();
   virtual CATID GetComponentInfoCategoryID();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPGSuperApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual LRESULT ProcessWndProcException(CException* e, const MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CPGSuperApp)
	afx_msg void OnAppAbout();
	afx_msg void OnHelpJoinArpList();
	afx_msg void OnHelpInetWsdot();
	afx_msg void OnHelpInetPgsuper();
   afx_msg void OnHelpInetARP();
	afx_msg void OnProgramSettings();
   afx_msg void OnScreenSize();
	//}}AFX_MSG
	afx_msg void OnProgramSettings(BOOL bFirstRun);
	DECLARE_MESSAGE_MAP()

public:
   CString GetEngineerName();
   CString GetEngineerCompany();

   CString GetMasterLibraryFile();
   CString GetCachedMasterLibraryFile();
   void GetTemplateFolders(CString& strWorkgroupFolder);

   CString GetVersion(bool bIncludeBuildNumber) const;
   CString GetVersionString(bool bIncludeBuildNumber) const;

   // URL's
   CString GetWsdotUrl();
   CString GetWsdotBridgeUrl();
   CString GetPGSuperUrl();

    void SetCacheUpdateFrequency(CacheUpdateFrequency frequence);
    CacheUpdateFrequency GetCacheUpdateFrequency();

    void SetSharedResourceType(SharedResourceType resType);
    SharedResourceType GetSharedResourceType();

    CString GetMasterLibraryPublisher() const;

private:

   CPGSuperCommandLineInfo m_CommandLineInfo;

   CPGSuperCatalogServers m_CatalogServers;

   virtual void RegistryInit(); // All registry initialization goes here
   virtual void RegistryExit(); // All registry cleanup goes here
   void RegistryConvert(); // Convert any old registry settings for current program (move into app plugin class)

   void Process1250Testing(const CPGSuperCommandLineInfo& cmdInfo);
   void ProcessTxDotCad(const CPGSuperCommandLineInfo& cmdInfo);

   SharedResourceType   m_SharedResourceType;     // method for using shared resources (Master lib and Workgroup templates)
   CacheUpdateFrequency m_CacheUpdateFrequency;

   CString m_CurrentCatalogServer; // name of current catalog server
   CString m_Publisher;     // Name of publisher in m_CurrentServer
   CString m_MasterLibraryFileURL; // URL of a published Master library file

   // Cache file/folder for Internet or Local Network resources
   CString m_MasterLibraryFileCache; 
   CString m_WorkgroupTemplateFolderCache;

   CString m_UserTemplateFolder;

   CString m_EngineerName;
   CString m_CompanyName;

   bool IsTimeToUpdateCache();
   bool AreUpdatesPending();

   void UpdateCache(); // only updates if needed
   bool DoCacheUpdate(); // always does the update
   CTime GetLastCacheUpdateTime();
   void SetLastCacheUpdateTime(const CTime& time);
   bool UpdateCatalogCache(IProgressMonitor* pProgress);
   void RestoreLibraryAndTemplatesToDefault();
   void DeleteCache(LPCSTR pstrCache);
   void RecursiveDelete(LPCSTR pstr);
   
   CString GetDefaultMasterLibraryFile();
   CString GetDefaultWorkgroupTemplateFolder();
   CString GetCacheFolder();
   CString GetSaveCacheFolder();


   sysDate GetInstallDate();
   sysDate GetLastRunDate();

   sysDate m_LastRunDate;

   BOOL IsFirstRun();

};

extern CPGSuperApp theApp;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PGSUPER_H__59D503E9_265C_11D2_8EB0_006097DF3C68__INCLUDED_)
