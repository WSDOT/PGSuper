///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include "TimelineGrid.h"
#include <PgsExt\TimelineManager.h>

// CEditTimelineDlg dialog

class CEditTimelineDlg : public CDialog
{
	DECLARE_DYNAMIC(CEditTimelineDlg)

public:
	CEditTimelineDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CEditTimelineDlg();

// Dialog Data
	enum { IDD = IDD_TIMELINE_MANAGER };
   CTimelineManager m_TimelineManager;

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnAddEvent();
   afx_msg void OnRemoveEvent();
   afx_msg void OnHelp();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   CTimelineGrid m_Grid;

	DECLARE_MESSAGE_MAP()

   friend CTimelineGrid;
};
