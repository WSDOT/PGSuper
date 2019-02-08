///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#if !defined(AFX_CONFIGUREPGSUPERDLG_H__A9BD22F0_087F_47A7_93E2_ED85BAF336D6__INCLUDED_)
#define AFX_CONFIGUREPGSUPERDLG_H__A9BD22F0_087F_47A7_93E2_ED85BAF336D6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConfigurePGSuperDlg.h : header file
//

#include "resource.h"
#include <PgsExt\CatalogServers.h>
#include <afx.h>
#include <afxlinkctrl.h>

/////////////////////////////////////////////////////////////////////////////
// CConfigurePGSuperDlg dialog

class CConfigurePGSuperDlg : public CPropertyPage
{
// Construction
public:
	CConfigurePGSuperDlg(LPCTSTR lpszAppName,LPCTSTR lpszTemplateExt,BOOL bFirstRun,CWnd* pParent = nullptr);   // standard constructor
   virtual ~CConfigurePGSuperDlg();

// Dialog Data
	//{{AFX_DATA(CConfigurePGSuperDlg)
	enum { IDD = IDD_CONFIGUREPGSUPER };
	CListBox	m_PublisherList;
   CString m_CurrentServer; // name of current catalog server
	CString	m_Publisher;
	SharedResourceType m_SharedResourceType;
	//}}AFX_DATA
   CMFCLinkCtrl   m_PublisherHyperLink;

   CacheUpdateFrequency m_CacheUpdateFrequency;

   CCatalogServers m_Servers;
   CString m_TemplateFileExt;

// Save data going into dialog so we can compare on the way out
// in order to determine if we need an update
   SharedResourceType m_OriginalSharedResourceType;
   CString m_OriginalServer;
	CString	m_OriginalPublisher;
   const CCatalogServer* m_OriginalServerPtr;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigurePGSuperDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConfigurePGSuperDlg)
	afx_msg void OnHelp();
	virtual BOOL OnInitDialog() override;
   afx_msg void OnAddCatalogServer();
	afx_msg void OnServerChanged();
	afx_msg void OnGeneric();
	afx_msg void OnDownload();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void UpdateFrequencyList();
   void ServerList();
   void PublisherList();
   void ConfigureWebLink();

   void OnMethod();

   CString m_PageTitle;
   CString m_AppName;
   bool m_bNetworkError;
  int	m_Method;
public:
   virtual LRESULT OnWizardNext() override;
   virtual LRESULT OnWizardBack() override;
   afx_msg void OnLbnSelchangePublishers();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGUREPGSUPERDLG_H__A9BD22F0_087F_47A7_93E2_ED85BAF336D6__INCLUDED_)
