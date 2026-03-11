///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

#include <PsgLib\ErectPiersActivity.h>
#include <PsgLib\TimelineManager.h>
#include <EAF\EAFDisplayUnits.h>

// Pier and Temporary Support indicies are stored in the ItemData member of the
// list box controls. To differentiate between a Pier index and a Temporary Support index
// the Temporary Support indices are encoded/decoded with the following methods
#include <WBFLTypes.h>
#include <PsgLib\PierData2.h>
#include <PsgLib\TemporarySupportData.h>
#include <PgsExt\TimelineItemListBox.h>

bool IsTSIndex(SupportIndexType tsIdx);
SupportIndexType EncodeTSIndex(SupportIndexType tsIdx);
SupportIndexType DecodeTSIndex(SupportIndexType tsIdx);
CString GetLabel(const CPierData2* pPier,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);
CString GetLabel(const CTemporarySupportData* pTS,std::shared_ptr<IEAFDisplayUnits> pDisplayUnits);

// CCastClosureJointDlg dialog

class CCastClosureJointDlg : public CDialog
{
	DECLARE_DYNAMIC(CCastClosureJointDlg)

public:
	CCastClosureJointDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CCastClosureJointDlg();

// Dialog Data
	enum { IDD = IDD_CAST_CLOSURE_JOINT };

   CTimelineManager m_TimelineMgr;
   EventIndexType m_EventIndex;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   void FillLists();


   CTimelineItemListBox m_lbSource;
   CTimelineItemListBox m_lbTarget;

   BOOL m_bReadOnly;

   DECLARE_MESSAGE_MAP()

   const CBridgeDescription2* m_pBridgeDesc;
   std::shared_ptr<IEAFDisplayUnits> m_pDisplayUnits;

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnMoveToTargetList();
   afx_msg void OnMoveToSourceList();
   afx_msg void OnHelp();
};
