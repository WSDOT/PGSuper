///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include "resource.h"
#include <PgsExt\GeometryControlActivity.h>
#include <PgsExt\TimelineManager.h>

// CGeometryControlDlg dialog

class CGeometryControlDlg : public CDialog
{
	DECLARE_DYNAMIC(CGeometryControlDlg)

public:
	CGeometryControlDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CGeometryControlDlg();

// Dialog Data
	enum { IDD = IDD_GEOMETRY_CONTROL };

   CTimelineManager m_TimelineMgr;
   EventIndexType m_EventIndex;

protected:
   BOOL m_bReadOnly;
   pgsTypes::GeometryControlActivityType m_GeometryControlActivityType;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnHelp();
   afx_msg void OnBnClickedRadio1();
   afx_msg void OnBnClickedGceCheck();
   afx_msg void OnBnClickedRadio2();
};
