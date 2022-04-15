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

#include "DrawStrandControl.h"
#include "SegmentTendonGrid.h"

struct IStrandGeometry;
class CPrecastSegmentData;

// CGirderSegmentTendonsPage dialog

class CGirderSegmentTendonsPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CGirderSegmentTendonsPage)

public:
	CGirderSegmentTendonsPage();
	virtual ~CGirderSegmentTendonsPage();

   void Init(CPrecastSegmentData* pSegment);
   CPrecastSegmentData* GetSegment();

   pgsTypes::StrandInstallationType GetInstallationType();

// Dialog Data
	enum { IDD = IDD_SEGMENT_TENDONS };
   const matPsStrand* GetStrand();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   CDrawStrandControl m_DrawStrands;

	//{{AFX_MSG(CGirderSegmentTendonsPage)
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	afx_msg void OnHelp();
	afx_msg void OnStrandTypeChanged();
   afx_msg void OnInstallationTypeChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void UpdateStrandList(UINT nIDC);
   void UpdateSectionDepth();
   void UpdateDuctMaterialList();
   void UpdateInstallationMethodList();
   void UpdateInstallationTimeList();

   std::unique_ptr<CSegmentTendonGrid> m_pGrid;

   CPrecastSegmentData* m_pSegment; // holds the strand data for the calling dialog
   CStrandData m_Strands; // holds strand data while editing is occuring (this is used to update the display)
   CSegmentPTData m_Tendons; // holds strand data while editing is occuring (this is used to update the display)

public:
   afx_msg void OnBnClickedAdd();
   afx_msg void OnBnClickedRemove();

   void EnableRemoveButton(BOOL bEnable);

   void OnChange();
};
