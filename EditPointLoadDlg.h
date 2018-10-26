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

// EditPointLoadDlg.h : header file
//
#include "PGSuperAppPlugin\resource.h"
#include <PgsExt\PointLoadData.h>
#include <PgsExt\TimelineManager.h>

/////////////////////////////////////////////////////////////////////////////
// CEditPointLoadDlg dialog

class CEditPointLoadDlg : public CDialog
{
// Construction
public:
	CEditPointLoadDlg(const CPointLoadData& load,const CTimelineManager* pTimelineMgr,CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEditPointLoadDlg)
	enum { IDD = IDD_EDIT_POINTLOAD };
	CStatic	m_SpanLengthCtrl;
	CEdit	m_LocationCtrl;
	CStatic	m_LocationUnitCtrl;
	CButton	m_FractionalCtrl;
	CComboBox	m_SpanCB;
	CComboBox	m_GirderCB;
	//}}AFX_DATA

   CPointLoadData m_Load;
   CComPtr<IBroker> m_pBroker;

   bool                 m_WasLiveLoad;

   CTimelineManager m_TimelineMgr;
   bool m_bWasNewEventCreated;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditPointLoadDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditPointLoadDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnFractional();
	afx_msg void OnEditchangeLoadcase();
	afx_msg void OnEditchangeSpans();
	afx_msg void OnEditchangeGirders();
	afx_msg void OnHelp();
   afx_msg void OnEventChanged();
   afx_msg void OnEventChanging();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
   void UpdateLocationUnit();
   void UpdateEventLoadCase(bool isInitial=false);
   void UpdateSpanLength();
   void UpdateSpanList();
   void UpdateGirderList();

   void FillEventList();
   EventIndexType CreateEvent();

   struct SpanType
   {
      SpanIndexType spanIdx;
      bool bStartCantilever;
      bool bEndCantilever;
   };

   std::vector<SpanType> m_Spans;

   int m_PrevEventIdx;
};
