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

#if !defined(AFX_CONFIGUREPGSUPERDLG_H__A9BD22F0_087F_47A7_93E2_ED85BAF336D6__INCLUDED_)
#define AFX_CONFIGUREPGSUPERDLG_H__A9BD22F0_087F_47A7_93E2_ED85BAF336D6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConfigurePGSuperDlg.h : header file
//

#include "PGSuperCatalog.h"
#include "PGSuperCatalogServers.h"
#include <MfcTools\ddxFolder.h>
#include <MfcTools\ddxfile.h>

/////////////////////////////////////////////////////////////////////////////
// CConfigurePGSuperDlg dialog

class CConfigurePGSuperDlg : public CDialog
{
// Construction
public:
	CConfigurePGSuperDlg(const CPGSuperCatalog& catalog,BOOL bFirstRun,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConfigurePGSuperDlg)
	enum { IDD = IDD_CONFIGUREPGSUPER };
	CListBox	m_PublisherList;
	CString	m_Company;
	CString	m_Engineer;
	CString	m_Publisher;
	int		m_Method;
	//}}AFX_DATA

   CPGSuperCatalog m_Catalog;

   CString	m_LocalMasterLibraryFile;
   CString	m_LocalWorkgroupTemplateFolder;

   CString	m_UserFolder;
   CacheUpdateFrequency m_CacheUpdateFrequency;

   CPGSuperCatalogServers m_Servers;

   BOOL m_bUpdateCache;

   BOOL m_bFirstRun;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigurePGSuperDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfigurePGSuperDlg)
	afx_msg void OnHelp();
	virtual BOOL OnInitDialog();
	afx_msg void OnSetfocusPublishers();
	afx_msg void OnSetFocusNetwork();
   afx_msg void OnAddCatalogServer();
	afx_msg void OnServerChanged();
	afx_msg void OnUpdatenow();
	afx_msg void OnGeneric();
	afx_msg void OnLocal();
	afx_msg void OnDownload();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void HideOkAndCancelButtons();
   void UpdateFrequencyList();
   void ServerList();
   void PublisherList();

   void OnMethod();

   CGetFilenameControl m_ctrlLibraryFile;
   CGetFolderControl m_ctrlWorkgroupFolder;
   CGetFolderControl m_ctrlUserFolder;

   bool m_bNetworkError;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGUREPGSUPERDLG_H__A9BD22F0_087F_47A7_93E2_ED85BAF336D6__INCLUDED_)
