///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <PgsExt\ClosurePourData.h>
#include "ClosurePourGeneralPage.h"
#include "ClosurePourLongitudinalReinforcementPage.h"
#include "ClosurePourStirrupsPage.h"

// CClosurePourDlg

class CClosurePourDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(CClosurePourDlg)

public:
	CClosurePourDlg(UINT nIDCaption, const CSegmentKey& closureKey,const CClosurePourData* pClosurePour, EventIndexType eventIdx,bool bEditingInGirder,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CClosurePourDlg(LPCTSTR pszCaption, const CSegmentKey& closureKey,const CClosurePourData* pClosurePour, EventIndexType eventIdx,bool bEditingInGirder,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CClosurePourDlg();

   void FillMaterialComboBox(CComboBox* pCB);
   void GetStirrupMaterial(int idx,matRebar::Type& type,matRebar::Grade& grade);
   int GetStirrupMaterialIndex(matRebar::Type type,matRebar::Grade grade);

   EventIndexType m_EventIdx; // cast closure pour stage
   CClosurePourData m_ClosurePour; // does not have association with temporary support, segments, or spliced girder
   CSegmentKey m_ClosureKey; // segmentIndex is the closure pour index

   bool m_bCopyToAllClosurePours; // if true, the data from this dialog is applied to all closure pours at this support

protected:
	DECLARE_MESSAGE_MAP()

   bool m_bEditingInGirder; // true if editing from within the girder (Check box is for copying to all closures in this girder)
                            // false if editing as an individual element (Check box is for copy to all closures at this support)

   void Init();
   CClosurePourGeneralPage m_General;
   CClosurePourLongitudinalReinforcementPage m_Longitudinal;
   CClosurePourStirrupsPage m_Stirrups;

   afx_msg BOOL OnOK();
   CButton m_CheckBox;

public:
   virtual BOOL OnInitDialog();
protected:
   virtual void DoDataExchange(CDataExchange* pDX);
};


