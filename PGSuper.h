///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#include "resource.h"       // main symbols 
#include "CountedMultiDocTemplate.h"
#include "ReportViewDocTemplate.h"
#include "BridgePlanView.h"
#include "GirderModelElevationView.h"
#include "LibraryEditor.h"
#include "ReportView.h"
#include "Hints.h"
#include "MainFrm.h"
#include "PGSuperCatalog.h"
#include "PGSuperCatalogServers.h"
#include "PGSuperPluginMgr.h"

#include <System\Date.h>
#include <PsgLib\LibraryManager.h>

typedef void(*DocCallback)(CDocument* pDoc,void* pStuff); 

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

enum SharedResourceType
{
   Default,   // use the defaults provided by PGSuper
   Internet,  // use library file and workgroup templates published on the internet
   Local      // use library file and workgroup templates published on a local network
};

HRESULT AfxGetBroker(IBroker** ppBroker);

/////////////////////////////////////////////////////////////////////////////
// CPGSuperApp:
// See PGSuper.cpp for the implementation of this class
//

class CPGSuperApp : public CWinApp
{
public:
	CPGSuperApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPGSuperApp)
	public:
	virtual BOOL InitInstance();
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);
	virtual int ExitInstance();
	virtual LRESULT ProcessWndProcException(CException* e, const MSG* pMsg);
	virtual int Run();
	//}}AFX_VIRTUAL

// Implementation
	COleTemplateServer m_server;
		// Server object for document creation

	//{{AFX_MSG(CPGSuperApp)
	afx_msg void OnAppAbout();
	afx_msg void OnFileNew();
   afx_msg void OnFileOpen();
	afx_msg void OnHelpJoinArpList();
	afx_msg void OnHelpInetWsdot();
	afx_msg void OnHelpInetPgsuper();
   afx_msg void OnHelpInetARP();
	afx_msg void OnAppLegal();
	afx_msg void OnProgramSettings();
   afx_msg void OnUpdateTemplate();
   afx_msg void OnScreenSize();
	//}}AFX_MSG
	afx_msg void OnProgramSettings(BOOL bFirstRun);
   afx_msg void OnImport(UINT nID);
   afx_msg void OnExport(UINT nID);
   afx_msg void OnImportMenu(CCmdUI* pCmdUI);
   afx_msg void OnExportMenu(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
   CCountedMultiDocTemplate* GetBridgeModelEditorTemplate();
   CCountedMultiDocTemplate* GetGirderModelEditorTemplate();
   CCountedMultiDocTemplate* GetLibraryEditorTemplate();
   CCountedMultiDocTemplate* GetAnalysisResultsViewTemplate();
   CCountedMultiDocTemplate* GetFactorOfSafetyViewTemplate();
   CCountedMultiDocTemplate* GetEditLoadsViewTemplate();

   // call this version when you want to get the doc template for view creation. the
   // values of the passed parameters are set
   CReportViewDocTemplate* GetReportViewTemplate(CollectionIndexType rptIdx,bool bPromptForSpec);

   // this method is called by the CReportView class during view initialization. It reads the
   // parameters it needs for view creation. DO NOT USE THIS VERSION TO CREATE A VIEW
   CReportViewDocTemplate* GetReportViewTemplate();

   CString GetAppLocation();

   CString GetEngineerName();
   CString GetEngineerCompany();
   CString GetTipFilePath() const;

   CString GetMasterLibraryFile();
   CString GetCachedMasterLibraryFile();
   void GetTemplateFolders(CString& strUserFolder,CString& strWorkgroupFolder);

   bool ShowProjectPropertiesOnNewProject();
   void ShowProjectPropertiesOnNewProject(bool bShow);

   CString GetVersion(bool bIncludeBuildNumber) const;
   CString GetVersionString(bool bIncludeBuildNumber) const;

	void ShowTipAtStartup(void);
   AcceptanceType ShowLegalNoticeAtStartup(void);

   bool IsAutoCalcEnabled() const;
   void EnableAutoCalc( bool bEnable );

   // flags for design dialog
   bool IsDesignFlexureEnabled() const;
   void EnableDesignFlexure( bool bEnable );

   bool IsDesignShearEnabled() const;
   void EnableDesignShear( bool bEnable );

   // set/get view settings for bridge model editor
   UINT GetBridgeEditorSettings() const;
   void SetBridgeEditorSettings(UINT settings);

   // set/get view settings for Girder model editor
   UINT GetGirderEditorSettings() const;
   void SetGirderEditorSettings(UINT settings);

   // set/get settings for UI hint dialogs
   UINT GetUIHintSettings() const;
   void SetUIHintSettings(UINT settings);

   // URL's
   CString GetWsdotUrl();
   CString GetWsdotBridgeUrl();
   CString GetPGSuperUrl();

    void SetCacheUpdateFrequency(CacheUpdateFrequency frequence);
    CacheUpdateFrequency GetCacheUpdateFrequency();

    void SetSharedResourceType(SharedResourceType resType);
    SharedResourceType GetSharedResourceType();

    CString GetMasterLibraryPublisher() const;

    CPGSuperCatalog& GetCatalog();

   // set/get view mode for list box in doc template dialog
   // 0 = SmallIconMode
   // 1 = LargeIconMode
   // 2 = ReportMode
   UINT GetDocTemplateViewMode() const;
   void SetDocTemplateViewMode(UINT mode);

   void ForEachDoc(DocCallback pfn,void* pStuff);
   bool IsDocLoaded();
   std::string GetLastError();

   bool UpdatingTemplate();

   UINT GetLocalMachineInt(LPCTSTR lpszSection, LPCTSTR lpszEntry,int nDefault);
   CString GetLocalMachineString(LPCTSTR lpszSection, LPCTSTR lpszEntry,LPCTSTR lpszDefault);

   void GetAppUnitSystem(IAppUnitSystem** ppAppUnitSystem);

   void GetImporter(UINT nID,IPGSuperImporter** importer);
   void GetExporter(UINT nID,IPGSuperExporter** exporter);

   BOOL ReadWindowPlacement(const CString& strKey,LPWINDOWPLACEMENT pwp);
   void WriteWindowPlacement(const CString& strKey,LPWINDOWPLACEMENT pwp);

private:
   bool m_bUpdatingTemplate;
	void ShowTipOfTheDay(void);
	AcceptanceType ShowLegalNotice(VARIANT_BOOL bGiveChoice = VARIANT_FALSE);

   HMENU m_hMenuShared;

   bool m_bAutoCalcEnabled;
   UINT m_BridgeModelEditorSettings;
   UINT m_GirderModelEditorSettings;

   UINT m_UIHintSettings;

   bool m_bDesignFlexureEnabled;
   bool m_bDesignShearEnabled;
   
   VARIANT_BOOL m_bShowLegalNotice;
   bool m_bShowProjectProperties;

   CString m_LastError;

   CPGSuperCatalogServers m_CatalogServers;
   CPGSuperCatalog m_Catalog;

   UINT m_DocTemplateViewMode;


   CComPtr<IAppUnitSystem> m_AppUnitSystem;

   CString m_strWindowPlacementFormat;


   typedef std::auto_ptr<CCountedMultiDocTemplate> DocTemplatePtr;
   CCountedMultiDocTemplate* m_pBridgeModelEditorTemplate;
   DocTemplatePtr m_pGirderModelEditorTemplate;
   DocTemplatePtr m_pLibraryEditorTemplate;
   DocTemplatePtr m_pAnalysisResultsViewTemplate;
   std::auto_ptr<CReportViewDocTemplate> m_pReportViewTemplate;
   DocTemplatePtr m_pFactorOfSafetyViewTemplate;
   DocTemplatePtr m_pEditLoadsViewTemplate;

   void RegistryInit(); // All registry initialization goes here
   void RegistryExit(); // All registry cleanup goes here

   void RegDocTemplates();
   void FinalDocumentInit(CPGSuperDoc* pDoc); // called when a document is opened to finalize the initialization process

   void SetTipFilePath(const CString& strTipPath);

   void Process1250Testing(const CPGSuperCommandLineInfo& cmdInfo);
   void ProcessTxDotCad(const CPGSuperCommandLineInfo& cmdInfo);

   SharedResourceType   m_SharedResourceType;     // method for using shared resources (Master lib and Workgroup templates)
   CacheUpdateFrequency m_CacheUpdateFrequency;

   CString m_Publisher; // Name of publisher
   CString m_MasterLibraryFileURL; // URL of a published Master library file
   CString m_WorkgroupTemplateFolderURL; // URL of a published workgroup template folder

   CString m_LocalMasterLibraryFile; // location of Master library file if on a local network
   CString m_LocalWorkgroupTemplateFolder; // locaion of workgroup template folder if on a local network

   // Cache file/folder for Internet or Local Network resources
   CString m_MasterLibraryFileCache; 
   CString m_WorkgroupTemplateFolderCache;

   CString m_UserTemplateFolder;

   CString m_EngineerName;
   CString m_CompanyName;

   CString m_TipFilePath; // path and filename

   bool IsTimeToUpdateCache();
   bool AreUpdatesPending();
   bool CheckInternetForUpdates();
   bool CheckLocalNetworkForUpdates();
   bool CheckForUpdates(const CString& strLocalMasterLibMD5,const CString& strLocalWorkgroupTemplateMD5);

   void UpdateCache(); // only updates if needed
   bool DoCacheUpdate(); // always does the update
   CTime GetLastCacheUpdateTime();
   void SetLastCacheUpdateTime(const CTime& time);
   bool UpdateMasterLibraryCache(IProgressMonitor* pProgress);
   bool UpdateWorkgroupTemplateCache(IProgressMonitor* pProgress);
   void DeleteCache(LPCSTR pstrCache);
   void RecursiveDelete(LPCSTR pstr);
   CString CleanFTPURL(const CString& strURL);

   CString GetDefaultMasterLibraryFile();
   CString GetDefaultWorkgroupTemplateFolder();
   CString GetCacheFolder();
   CString GetSaveCacheFolder();
   CString GetLocalMasterLibraryMD5Filename();
   CString GetLocalWorkgroupTemplateFolderMD5Filename();

   HKEY GetAppLocalMachineRegistryKey();
   HKEY GetUninstallRegistryKey();
   HKEY GetLocalMachineSectionKey(LPCTSTR lpszSection);
   HKEY GetLocalMachineSectionKey(HKEY hAppKey,LPCTSTR lpszSection);
   UINT GetLocalMachineInt(HKEY hAppKey,LPCTSTR lpszSection, LPCTSTR lpszEntry,int nDefault);
   CString GetLocalMachineString(HKEY hAppKey,LPCTSTR lpszSection, LPCTSTR lpszEntry,LPCTSTR lpszDefault);

   sysDate GetInstallDate();
   sysDate GetLastRunDate();

   sysDate m_LastRunDate;

   BOOL IsFirstRun();

   CPGSuperPluginMgr m_PluginMgr;
};

extern CPGSuperApp theApp;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PGSUPER_H__59D503E9_265C_11D2_8EB0_006097DF3C68__INCLUDED_)
