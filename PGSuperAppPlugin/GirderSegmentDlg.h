///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#include "GirderSegmentGeneralPage.h"
#include "GirderSegmentStrandsPage.h"
#include "GirderSegmentLongitudinalRebarPage.h"
#include "GirderSegmentStirrupsPage.h"
#include "..\BridgeDescLiftingPage.h"

#include <IFace\ExtendUI.h>
#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\BridgeDescription2.h>

// CGirderSegmentDlg

class CGirderSegmentDlg : public CPropertySheet, public IEditSegmentData
{
	DECLARE_DYNAMIC(CGirderSegmentDlg)

public:
	CGirderSegmentDlg(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CGirderSegmentDlg(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey,const std::set<EditSplicedGirderExtension>& editSplicedGirderExtensions,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

   // IEditSegmentData
   virtual const CSegmentKey& GetSegmentKey() { return m_SegmentKey; }

   CTimelineManager m_TimelineMgr; // copy of the timeine manager we are editing
   CSplicedGirderData m_Girder; // copy of the girder we are editing (contains the segment we are editing)
   CSegmentKey m_SegmentKey; // key to the segment we are editing
   SegmentIDType m_SegmentID; // ID of the segment we are editing

   CGirderSegmentStirrupsPage m_StirrupsPage;

   bool m_bCopyToAll; // if true, the data from this dialog is applied to all segments at this position in this group

   bool WasEventCreated() { return m_GeneralPage.m_bWasEventCreated; }

public:
	virtual ~CGirderSegmentDlg();
	virtual INT_PTR DoModal();

   // Returns a macro transaction object that contains editing transactions
   // for all the extension pages. The caller is responsble for deleting this object
   txnTransaction* GetExtensionPageTransaction();

protected:
	DECLARE_MESSAGE_MAP()

   void CommonInit(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey);
   void Init(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey);
   void Init(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey,const std::set<EditSplicedGirderExtension>& editSplicedGirderExtensions);
   void CreateExtensionPages();
   void CreateExtensionPages(const std::set<EditSplicedGirderExtension>& editSplicedGirderExtensions);
   void DestroyExtensionPages();

   bool m_bEditingInGirder;

   txnMacroTxn m_Macro;
   std::vector<std::pair<IEditSegmentCallback*,CPropertyPage*>> m_ExtensionPages;
   std::set<EditSplicedGirderExtension> m_SplicedGirderExtensionPages;
   void NotifyExtensionPages();
   void NotifySplicedGirderExtensionPages();

   ConfigStrandFillVector ComputeStrandFillVector(pgsTypes::StrandType type);

   CGirderSegmentGeneralPage m_GeneralPage;
   CGirderSegmentStrandsPage m_StrandsPage;
   CGirderSegmentLongitudinalRebarPage m_RebarPage;
   CGirderDescLiftingPage m_LiftingPage;

   afx_msg BOOL OnOK();
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);

   CButton m_CheckBox;

   friend CGirderSegmentStrandsPage;

public:
   virtual BOOL OnInitDialog();
protected:
   virtual void DoDataExchange(CDataExchange* pDX);
};


