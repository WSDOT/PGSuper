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

#include "GirderSegmentGeneralPage.h"
#include "GirderSegmentStrandsPage.h"
#include "GirderSegmentLongitudinalRebarPage.h"
#include "GirderSegmentStirrupsPage.h"
#include "..\BridgeDescLiftingPage.h"

#include <PgsExt\SplicedGirderData.h>

// CGirderSegmentDlg

class CGirderSegmentDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(CGirderSegmentDlg)

public:
	CGirderSegmentDlg(bool bEditingInGirder,CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CGirderSegmentDlg();

   CSplicedGirderData m_Girder; // copy of the girder we are editing (contains the segment we are editing)
   CSegmentKey m_SegmentKey; // key to the segment we are editing
   SegmentIDType m_SegmentID; // ID of the segment we are editing

   EventIndexType m_ConstructionEventIdx;
   EventIndexType m_ErectionEventIdx;

   //void RestoreTransverseReinfocementToLibraryDefaults();
   //void RestoreLongitudinalReinfocementToLibraryDefaults();

   CGirderSegmentStirrupsPage m_Stirrups;

   bool m_bCopyToAll; // if true, the data from this dialog is applied to all segments at this position in this group

protected:
	DECLARE_MESSAGE_MAP()

   bool m_bEditingInGirder; // true if editing from within the girder (Check box is for copying to all segments in this girder)
                            // false if editing as an individual element (Check box is for copy to all segments at this position in the group)

   void Init();
   ConfigStrandFillVector ComputeStrandFillVector(pgsTypes::StrandType type);

   CGirderSegmentGeneralPage m_General;
   CGirderSegmentStrandsPage m_Strands;
   CGirderSegmentLongitudinalRebarPage m_Rebar;
   CGirderDescLiftingPage m_Lifting;

   afx_msg BOOL OnOK();
   CButton m_CheckBox;

   friend CGirderSegmentStrandsPage;

public:
   virtual BOOL OnInitDialog();
protected:
   virtual void DoDataExchange(CDataExchange* pDX);
};


