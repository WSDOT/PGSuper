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

#include "LinearDuctGrid.h"
#include "SplicedGirderGeneralPage.h"

// CLinearDuctDlg dialog

class CLinearDuctDlg : public CDialog, public CLinearDuctGridCallback
{
	DECLARE_DYNAMIC(CLinearDuctDlg)

public:
	CLinearDuctDlg(CSplicedGirderGeneralPage* pGdrDlg, CPTData* pPTData,DuctIndexType ductIdx,CWnd* pParent = nullptr);   // standard constructor
	virtual ~CLinearDuctDlg();

   void EnableDeleteBtn(BOOL bEnable);

   const CGirderKey& GetGirderKey() const;

   const CLinearDuctGeometry& GetDuctGeometry() const;

   WBFL::Graphing::PointMapper::MapMode GetTendonControlMapMode() const;

// Dialog Data
	enum { IDD = IDD_LINEAR_DUCT };

   // returns the current value of the measurement type. only use when the dialog is open
   CLinearDuctGeometry::MeasurementType GetMeasurementType();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

   CPTData m_PTData;
   DuctIndexType m_DuctIdx;
   CLinearDuctGrid m_Grid;
   CSplicedGirderGeneralPage* m_pGirderlineDlg;
   CDrawTendonsControl m_DrawTendons;

   int m_PrevMeasurmentTypeIdx;

	DECLARE_MESSAGE_MAP()
public:
   virtual BOOL OnInitDialog();
   afx_msg void OnAddPoint();
   afx_msg void OnDeletePoint();
   afx_msg void OnMeasurementTypeChanging();
   afx_msg void OnMeasurementTypeChanged();
   afx_msg void OnHelp();
   afx_msg void OnSchematicButton();

   virtual void OnDuctChanged();
};
