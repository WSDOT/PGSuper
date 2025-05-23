///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

#if !defined(AFX_EDITMOMENTLOADDLG_H__82043FFC_1EBA_44DE_9D6A_10BB289BAA99__INCLUDED_)
#define AFX_EDITMOMENTLOADDLG_H__82043FFC_1EBA_44DE_9D6A_10BB289BAA99__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EditMomentLoadDlg.h : header file
//
#include "resource.h"
#include <PgsExt\MomentLoadData.h>
#include <PgsExt\TimelineManager.h>

/////////////////////////////////////////////////////////////////////////////
// CEditMomentLoadDlg dialog

class CEditMomentLoadDlg : public CDialog
{
// Construction
public:
	CEditMomentLoadDlg(const CMomentLoadData& load,const CTimelineManager* pTimelineMgr,CWnd* pParent = nullptr);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEditMomentLoadDlg)
	enum { IDD = IDD_EDIT_MOMENTLOAD };
	CStatic	m_SpanLengthCtrl;
	CComboBox	m_GirderCB;
	CComboBox	m_SpanCB;
	//}}AFX_DATA
   int m_LocationIdx;

   CMomentLoadData m_Load;
   EventIDType m_EventID;
   CComPtr<IBroker> m_pBroker;

   bool                 m_WasLiveLoad;

   CTimelineManager m_TimelineMgr;
   bool m_bWasNewEventCreated;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditMomentLoadDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditMomentLoadDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnEditchangeLoadcase();
	afx_msg void OnEditchangeSpans();
	afx_msg void OnEditchangeGirders();
	afx_msg void OnHelp();
   afx_msg void OnEventChanging();
   afx_msg void OnEventChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   void UpdateEventLoadCase(bool isInitial=false);
   void UpdateSpanLength();
   void UpdateGirderList();

   void FillEventList();
   EventIndexType CreateEvent();

   int m_PrevEventIdx;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITMOMENTLOADDLG_H__82043FFC_1EBA_44DE_9D6A_10BB289BAA99__INCLUDED_)
