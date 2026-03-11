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

#include "ParabolicDuctGrid.h"
#include "SplicedGirderGeneralPage.h"
#include "DrawTendonsControl.h"

// CParabolicDuctDlg dialog

class CParabolicDuctDlg : public CDialog, public CParabolicDuctGridCallback
{
	DECLARE_DYNAMIC(CParabolicDuctDlg)

public:
	CParabolicDuctDlg(CSplicedGirderGeneralPage* pGdrDlg,CPTData* pPTData,DuctIndexType ductIdx,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CParabolicDuctDlg();


// Dialog Data
	enum { IDD = IDD_PARABOLIC_DUCT };

   const CParabolicDuctGeometry& GetDuctGeometry() const;

   WBFL::Graphing::PointMapper::MapMode GetTendonControlMapMode() const;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   void FillPierLists();

	DECLARE_MESSAGE_MAP()

   CSplicedGirderGeneralPage* m_pGirderlineDlg;
   CPTData m_PTData;
   DuctIndexType m_DuctIdx;
   CDrawTendonsControl m_DrawTendons;
   CParabolicDuctGrid m_Grid;

public:
   virtual BOOL OnInitDialog();
   afx_msg void OnHelp();
   afx_msg void OnRangeChanged();
   afx_msg void OnSchematicButton();

   virtual void OnDuctChanged();
};
