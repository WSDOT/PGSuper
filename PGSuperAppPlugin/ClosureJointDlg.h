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

#include <PgsExt\Keys.h>
#include <PgsExt\ClosureJointData.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\TimelineManager.h>
#include "ClosureJointGeneralPage.h"
#include "ClosureJointLongitudinalReinforcementPage.h"
#include "ClosureJointStirrupsPage.h"

#include <IFace\ExtendUI.h>
#include <EAF\EAFMacroTxn.h>

// CClosureJointDlg

class CClosureJointDlg : public CPropertySheet, public IEditClosureJointData
{
	DECLARE_DYNAMIC(CClosureJointDlg)

public:
	CClosureJointDlg(const CBridgeDescription2* pBridgeDesc,const CClosureKey& closureKey, CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);
	CClosureJointDlg(const CBridgeDescription2* pBridgeDesc,const CClosureKey& closureKey, const std::vector<EditSplicedGirderExtension>& editSplicedGirderExtensions,CWnd* pParentWnd = nullptr, UINT iSelectPage = 0);

   // IEditClosureJointData
   virtual const CClosureKey& GetClosureKey() { return m_ClosureKey; }

   bool m_bEditingInGirder;

   const CBridgeDescription2* m_pBridgeDesc;
   CTimelineManager m_TimelineMgr;
   CClosureJointData m_ClosureJoint; // does not have association with temporary support, segments, or spliced girder
   CClosureKey m_ClosureKey; // segmentIndex is the closure joint index
   ClosureIDType m_ClosureID;
   PierIDType m_PierID;
   SupportIDType m_TempSupportID;

   bool m_bCopyToAllClosureJoints; // if true, the data from this dialog is applied to all closure joints at this support

public:
	virtual ~CClosureJointDlg();
	virtual INT_PTR DoModal();

   // Returns a macro transaction object that contains editing transactions
   // for all the extension pages. The caller is responsble for deleting this object
   std::unique_ptr<CEAFTransaction> GetExtensionPageTransaction();

   bool WasEventCreated() { return m_General.m_bWasEventCreated; }

protected:
	DECLARE_MESSAGE_MAP()

   void CommonInit(const CBridgeDescription2* pBridgeDesc);
   void Init(const CBridgeDescription2* pBridgeDesc);
   void Init(const CBridgeDescription2* pBridgeDesc,const std::vector<EditSplicedGirderExtension>& editSplicedGirderExtensions);
   void CreateExtensionPages();
   void CreateExtensionPages(const std::vector<EditSplicedGirderExtension>& editSplicedGirderExtensions);
   void DestroyExtensionPages();


   CEAFMacroTxn m_Macro;
   std::vector<std::pair<IEditClosureJointCallback*,CPropertyPage*>> m_ExtensionPages;
   std::vector<EditSplicedGirderExtension> m_SplicedGirderExtensionPages;
   void NotifyExtensionPages();
   void NotifySplicedGirderExtensionPages();

   CClosureJointGeneralPage m_General;
   CClosureJointLongitudinalReinforcementPage m_Longitudinal;
   CClosureJointStirrupsPage m_Stirrups;

   afx_msg BOOL OnOK();
	afx_msg LRESULT OnKickIdle(WPARAM, LPARAM);

   CButton m_CheckBox;

public:
   virtual BOOL OnInitDialog();
protected:
   virtual void DoDataExchange(CDataExchange* pDX);
};


