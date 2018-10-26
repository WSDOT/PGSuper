///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#include <PgsExt\ApplyLoadActivity.h>
#include <PgsExt\TimelineManager.h>
#include "afxcmn.h"

// CApplyLoadsDlg dialog

class CApplyLoadsDlg : public CDialog
{
	DECLARE_DYNAMIC(CApplyLoadsDlg)

public:
	CApplyLoadsDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent = NULL);   // standard constructor
	virtual ~CApplyLoadsDlg();

// Dialog Data
	enum { IDD = IDD_APPLYLOADS };
   
   CTimelineManager m_TimelineMgr;
   EventIndexType m_EventIndex;
   
protected:
   void InitalizeCheckBox(CDataExchange* pDX,EventIndexType eventIdx,UINT nIDC);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   CListCtrl m_ctrlUserLoads;

   BOOL m_bReadOnly;

   void InitUserLoads();
   void AddDistributedLoad(int rowIdx,LoadIDType loadID);
   void AddPointLoad(int rowIdx,LoadIDType loadID);
   void AddMomentLoad(int rowIdx,LoadIDType loadID);

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnHelp();
};
