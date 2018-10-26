///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

#if !defined(AFX_COPYGIRDERDLG_H__F07E0F0D_33D6_11D3_AD91_00105A9AF985__INCLUDED_)
#define AFX_COPYGIRDERDLG_H__F07E0F0D_33D6_11D3_AD91_00105A9AF985__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CopyGirderDlg.h : header file
//

#include "resource.h"
#include <vector>

/////////////////////////////////////////////////////////////////////////////
// CCopyGirderDlg dialog

class CCopyGirderDlg : public CDialog
{
   friend CPGSuperDoc;
// Construction
public:
	CCopyGirderDlg(IBroker* pBroker, CPGSuperDoc* pDoc, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCopyGirderDlg)
	enum { IDD = IDD_COPY_GIRDER_PROPERTIES };
	CButton	m_DoCopyGirder;
	CButton	m_DoCopyMaterial;
	CButton	m_DoCopyTransverse;
	CButton	m_DoCopyLongitudinalRebar;
	CButton	m_DoCopyPrestressing;
	CButton	m_DoCopyHandling;
	CButton	m_DoCopySlabOffset;
	//CListBox	m_ToList;
	//CListBox	m_FromList;
   
   CComboBox m_FromSpan;
   CComboBox m_FromGirder;
   CComboBox m_ToSpan;
   CComboBox m_ToGirder;

	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCopyGirderDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

   // return selected girder to copy properties from
   SpanGirderHashType GetFromSpanGirder();

   // return a list of spans and girders to be copied to
   std::vector<SpanGirderHashType> GetToSpanGirders();

   SpanGirderHashType m_FromSpanGirderHashValue;
   std::vector<SpanGirderHashType> m_ToSpanGirderHashValues;

   BOOL m_bCopyGirder;
   BOOL m_bCopyTransverse;
   BOOL m_bCopyPrestressing;
   BOOL m_bCopyHandling;
   BOOL m_bCopyMaterial;
   BOOL m_bCopyLongitudinalRebar;
   BOOL m_bCopySlabOffset;

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCopyGirderDlg)
	virtual BOOL OnInitDialog();
   afx_msg void OnFromSpanChanged();
   afx_msg void OnToSpanChanged();
   afx_msg void OnToGirderChanged();
	//afx_msg void OnSelchangeFromList();
	//afx_msg void OnSelchangeToList();
	afx_msg void OnCopyPrestressing();
	afx_msg void OnCopyHandling();
	afx_msg void OnCopySlabOffset();
	afx_msg void OnCopyTransverse();
   afx_msg void OnCopyLongitudinalRebar();
   afx_msg void OnHelp();
	afx_msg void OnCopyMaterial();
	afx_msg void OnCopyGirder();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

//   void FillListBox(CListBox& rbox, std::map<int,SpanGirderHashType>& indicies,SpanGirderHashType skip);

   void CopyToSelectionChanged();
   void UpdateApply();
   void UpdateButtons();
   CPGSuperDoc* m_pDoc;
   IBroker* m_pBroker;

   void FillComboBoxes(CComboBox& cbSpan,CComboBox& cbGirder, bool bIncludeAllSpanGirder);
   void FillGirderComboBox(CComboBox& cbGirder,SpanIndexType spanIdx,bool bIncludeAll);

   std::map<int,SpanGirderHashType> m_ToListIndicies;
   std::map<int,SpanGirderHashType> m_FromListIndicies;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COPYGIRDERDLG_H__F07E0F0D_33D6_11D3_AD91_00105A9AF985__INCLUDED_)
