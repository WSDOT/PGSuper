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

#include <GraphicsLib\GraphicsLib.h>
#include "DrawStrandControl.h"
#include "StrandGrid.h"

struct IStrandGeometry;

// CGirderSegmentStrandsPage dialog

class CGirderSegmentStrandsPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CGirderSegmentStrandsPage)

public:
	CGirderSegmentStrandsPage();
	virtual ~CGirderSegmentStrandsPage();


// Dialog Data
	enum { IDD = IDD_SEGMENT_STRANDS };

   Float64 GetMaxPjack(StrandIndexType nStrands); // allowable by spec
   Float64 GetUltPjack(StrandIndexType nStrands); // breaking strength

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   CDrawStrandControl m_DrawStrands;
   CMetaFileStatic m_Picture;

	//{{AFX_MSG(CGirderSegmentStrandsPage)
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	afx_msg void OnUpdateTemporaryStrandPjEdit();
	afx_msg void OnUpdateHarpedStrandPjEdit();
	afx_msg void OnUpdateStraightStrandPjEdit();
	afx_msg void OnHelp();
   afx_msg void OnStrandTypeChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   void OnUpdateStrandPjEdit(UINT nCheck,UINT nForceEdit,UINT nUnit,pgsTypes::StrandType strandType);

   void UpdateStrandControls();
   void UpdateStrandList(UINT nIDC);

   void InitPjackEdits();
   void InitPjackEdits(UINT nCalcPjack,UINT nPjackEdit,UINT nPjackUnit,pgsTypes::StrandType strandType);

   CStrandGrid m_Grid;
	int	m_StrandSizeIdx;
public:
   afx_msg void OnBnClickedAdd();
   afx_msg void OnBnClickedRemove();

   void EnableRemoveButton(BOOL bEnable);

   void OnChange();
};
