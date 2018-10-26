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

#include <PgsExt\Keys.h>
#include <PgsExt\ClosureJointData.h>
#include "ClosureJointGeneralPage.h"
#include "ClosureJointLongitudinalReinforcementPage.h"
#include "ClosureJointStirrupsPage.h"

#include <IFace\ExtendUI.h>

// CClosureJointDlg

class CClosureJointDlg : public CPropertySheet, public IEditClosureJointData
{
	DECLARE_DYNAMIC(CClosureJointDlg)

public:
	CClosureJointDlg(const CSegmentKey& closureKey,const CClosureJointData* pClosureJoint, EventIndexType eventIdx,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CClosureJointDlg(const CSegmentKey& closureKey,const CClosureJointData* pClosureJoint, EventIndexType eventIdx,const std::set<EditSplicedGirderExtension>& editSplicedGirderExtensions,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

   // IEditClosureJointData
   virtual const CClosureKey& GetClosureKey() { return m_ClosureKey; }

   EventIndexType m_EventIndex; // cast closure joint stage
   CClosureJointData m_ClosureJoint; // does not have association with temporary support, segments, or spliced girder
   CSegmentKey m_ClosureKey; // segmentIndex is the closure joint index

   bool m_bCopyToAllClosureJoints; // if true, the data from this dialog is applied to all closure joints at this support

public:
	virtual ~CClosureJointDlg();
	virtual INT_PTR DoModal();

   // Returns a macro transaction object that contains editing transactions
   // for all the extension pages. The caller is responsble for deleting this object
   txnTransaction* GetExtensionPageTransaction();

protected:
	DECLARE_MESSAGE_MAP()

   void CommonInit();
   void Init();
   void Init(const std::set<EditSplicedGirderExtension>& editSplicedGirderExtensions);
   void CreateExtensionPages();
   void CreateExtensionPages(const std::set<EditSplicedGirderExtension>& editSplicedGirderExtensions);
   void DestroyExtensionPages();


   txnMacroTxn m_Macro;
   std::vector<std::pair<IEditClosureJointCallback*,CPropertyPage*>> m_ExtensionPages;
   std::set<EditSplicedGirderExtension> m_SplicedGirderExtensionPages;
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


