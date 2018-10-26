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
#if !defined(AFX_EDITDISTRIBUTEDLOADDLG_H__F8A5B269_E1BB_4889_A346_AE48EB03520D__INCLUDED_)
#define AFX_EDITDISTRIBUTEDLOADDLG_H__F8A5B269_E1BB_4889_A346_AE48EB03520D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EditDistributedLoadDlg.h : header file
//
#include "PGSuperAppPlugin\resource.h"
#include <pgsExt\DistributedLoadData.h>

/////////////////////////////////////////////////////////////////////////////
// CEditDistributedLoadDlg dialog

class CEditDistributedLoadDlg : public CDialog
{
// Construction
public:
	CEditDistributedLoadDlg(CDistributedLoadData load, IBroker* pBroker,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEditDistributedLoadDlg)
	enum { IDD = IDD_EDIT_DISTRIBUTEDLOAD };
	CStatic	m_SpanLengthCtrl;
	CStatic	m_LocationUnitCtrl;
	CEdit	m_RightLocationCtrl;
	CEdit	m_LeftLocationCtrl;
	CComboBox	m_LoadTypeCB;
	CButton	m_FractionalCtrl;
	CComboBox	m_GirderCB;
	CComboBox	m_SpanCB;
	CComboBox	m_StageCB;
	CComboBox	m_LoadCaseCB;
	//}}AFX_DATA

   CDistributedLoadData m_Load;
   IBroker*             m_pBroker;

   bool                 m_bUnitsSI;
   const unitLength*    m_pLengthUnit;

   bool                 m_WasLiveLoad;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditDistributedLoadDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditDistributedLoadDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnFractional();
	afx_msg void OnEditchangeLoadcase();
	afx_msg void OnEditchangeLoadtype();
	afx_msg void OnEditchangeSpans();
	afx_msg void OnEditchangeGirders();
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   void UpdateLocationUnit();
   void UpdateStageLoadCase(bool isInitial=false);
   void UpdateLoadType();
   void UpdateSpanLength();

   void UpdateGirderList();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITDISTRIBUTEDLOADDLG_H__F8A5B269_E1BB_4889_A346_AE48EB03520D__INCLUDED_)
