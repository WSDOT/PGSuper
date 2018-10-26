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

#include "PGSuperAppPlugin\resource.h"
#include "GirderGrid.h"
#include "DuctGrid.h"
#include "SlabOffsetGrid.h"
#include <PgsExt\BridgeDescription2.h>
#include <Material\PsStrand.h>

#include "DrawTendonsControl.h"

#include "SlabOffsetHyperLink.h"

// CSplicedGirderGeneralPage dialog

class CSplicedGirderGeneralPage : public CPropertyPage
{
	DECLARE_DYNAMIC(CSplicedGirderGeneralPage)

public:
	CSplicedGirderGeneralPage();
	virtual ~CSplicedGirderGeneralPage();

// Dialog Data
	enum { IDD = IDD_SPLICEDGIRDER };

   std::vector<EventIndexType> m_TendonStressingEvent; // index is duct index, value is event when tendon is stressed

   int GetDuctCount();

   const matPsStrand* GetStrand();
   pgsTypes::StrandInstallationType GetInstallationType();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   void FillGirderComboBox();
   void FillStrandList(UINT nIDC);
   void FillStrandList(CComboBox* pList,matPsStrand::Grade grade,matPsStrand::Type type);
   void SetStrand();

   void FillDuctType();
   void FillInstallationType();

   CSlabOffsetHyperLink m_ctrlSlabOffsetHyperLink;
   void UpdateSlabOffsetHyperLink();
   void UpdateSlabOffsetControls();

   CGirderGrid m_GirderGrid;
   CDuctGrid   m_DuctGrid;
   CSlabOffsetGrid m_SlabOffsetGrid;
   CDrawTendonsControl m_DrawTendons;

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnAddDuct();
   afx_msg void OnDeleteDuct();
   afx_msg void OnStrandChanged();
   afx_msg void OnInstallationTypeChanged();
   afx_msg void OnConditionFactorTypeChanged();
   afx_msg LRESULT OnChangeSlabOffsetType(WPARAM wParam,LPARAM lParam);

   void OnDuctChanged();
   afx_msg void OnHelp();
};
