///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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


// CTimelineEventDlg dialog
//#include <afxmenubutton.h>
#include <PgsExt\TimelineEvent.h>
#include <PgsExt\TimelineManager.h>
#include "ActivityGrid.h"

class CTimelineEventDlg : public CDialog
{
	DECLARE_DYNAMIC(CTimelineEventDlg)

public:
	CTimelineEventDlg(const CTimelineManager& timelineMgr,
                     EventIndexType eventIdx, // index of event, INVALID_INDEX if creating a new event
                     BOOL bEditEvent, // true to display the event details editing, otherwise so the list of prev. defined events for creating new events on the fly
                     EventIndexType minEventIdx = INVALID_INDEX, // if an event index is specified, the newly created event cannot be before that event index
                     BOOL bReadOnly=FALSE,
                     CWnd* pParent = nullptr);   // standard constructor
	virtual ~CTimelineEventDlg();

   bool UpdateTimelineManager(const CTimelineManager& timelineMgr);

   CTimelineManager m_TimelineManager; // our local copy of the timeline manager which were are operating upon
   CTimelineEvent* m_pTimelineEvent; // the event in m_TimelineManager that we are operating upon
   EventIndexType m_EventIndex;
   EventIndexType m_MinEventIdx;

// Dialog Data
	enum { IDD = IDD_TIMELINE_EVENT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
   BOOL m_bReadOnly;
   BOOL m_bEdit;

   CCoolButton m_btnAdd;
   CActivityGrid m_Grid;
   CListCtrl m_TimelineEventList;

   afx_msg void OnConstructSegments();
   afx_msg void OnErectPiers();
   afx_msg void OnErectSegments();
   afx_msg void OnRemoveTempSupports();
   afx_msg void OnRemoveActivities();
   afx_msg void OnCastClosureJoints();
   afx_msg void OnCastDeck();
   afx_msg void OnCastLongitudinalJoints();
   afx_msg void OnApplyLoads();
   afx_msg void OnGeometryControl();
   afx_msg void OnStressTendons();

   void UpdateAddButton();

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog() override;
   afx_msg void OnHelp();
};
