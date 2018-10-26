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

#include <PgsExt\SegmentKey.h>
#include <PgsExt\ClosureJointData.h>
#include "ClosureJointGeneralPage.h"
#include "ClosureJointLongitudinalReinforcementPage.h"
#include "ClosureJointStirrupsPage.h"

// CClosureJointDlg

class CClosureJointDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(CClosureJointDlg)

public:
	CClosureJointDlg(UINT nIDCaption, const CSegmentKey& closureKey,const CClosureJointData* pClosureJoint, EventIndexType eventIdx,bool bEditingInGirder,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CClosureJointDlg(LPCTSTR pszCaption, const CSegmentKey& closureKey,const CClosureJointData* pClosureJoint, EventIndexType eventIdx,bool bEditingInGirder,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CClosureJointDlg();

   EventIndexType m_EventIndex; // cast closure joint stage
   CClosureJointData m_ClosureJoint; // does not have association with temporary support, segments, or spliced girder
   CSegmentKey m_ClosureKey; // segmentIndex is the closure joint index

   bool m_bCopyToAllClosureJoints; // if true, the data from this dialog is applied to all closure joints at this support

protected:
	DECLARE_MESSAGE_MAP()

   bool m_bEditingInGirder; // true if editing from within the girder (Check box is for copying to all closures in this girder)
                            // false if editing as an individual element (Check box is for copy to all closures at this support)

   void Init();
   CClosureJointGeneralPage m_General;
   CClosureJointLongitudinalReinforcementPage m_Longitudinal;
   CClosureJointStirrupsPage m_Stirrups;

   afx_msg BOOL OnOK();
   CButton m_CheckBox;

public:
   virtual BOOL OnInitDialog();
protected:
   virtual void DoDataExchange(CDataExchange* pDX);
};


