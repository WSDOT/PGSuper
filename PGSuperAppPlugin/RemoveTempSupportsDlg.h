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

#include <PsgLib\RemoveTemporarySupportsActivity.h>
#include <PsgLib\TimelineManager.h>
#include <PsgLib\TemporarySupportData.h>
#include <PgsExt\TimelineItemListBox.h>
#include <EAF\EAFDisplayUnits.h>

// CRemoveTempSupportsDlg dialog

class CRemoveTempSupportsDlg : public CDialog
{
	DECLARE_DYNAMIC(CRemoveTempSupportsDlg)

public:
	CRemoveTempSupportsDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CRemoveTempSupportsDlg();

// Dialog Data
	enum { IDD = IDD_REMOVE_TS };
   CTimelineManager m_TimelineMgr;
   EventIndexType m_EventIndex;

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
   void FillLists();

	DECLARE_MESSAGE_MAP()

   std::shared_ptr<IEAFDisplayUnits> m_pDisplayUnits;
   const CBridgeDescription2* m_pBridgeDesc;

   CTimelineItemListBox m_lbSource;
   CTimelineItemListBox m_lbTarget;

   BOOL m_bReadOnly;

public:
   virtual BOOL OnInitDialog() override;
   afx_msg void OnMoveToTargetList();
   afx_msg void OnMoveToSourceList();
   afx_msg void OnHelp();
};
