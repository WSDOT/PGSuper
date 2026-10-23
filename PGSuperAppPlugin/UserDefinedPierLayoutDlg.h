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
#include "resource.h"
#include <PsgLib\PierData2.h>
#include "ColumnLayoutGrid.h"
#include "PierPointGrid.h"
#include <PgsExt\ColumnFixityComboBox.h>
#include "DrawPierLayoutControl.h"


// UserDefinedPierLayoutDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// UserDefinedPierLayoutDlg dialog

class CPierLayoutPage;

class CUserDefinedPierLayoutDlg : public CDialog, public IPierLayoutDataSource
{

	friend class CPierLayoutPage;
	friend class CPierConnectionsPage;

// Construction
public:
	CUserDefinedPierLayoutDlg(CWnd* pParent = nullptr);

	void SetPierModelType(const pgsTypes::PierModelType& pierModelType);
	void SetPierData(const CPierData2& pierData) override;

	// IPierLayoutDataSource implementation
	const CPierData2* GetPierData() const override;

   // Implementation
protected:
	void DoDataExchange(CDataExchange* pDX);


	virtual BOOL OnInitDialog() override;

	afx_msg LRESULT OnColumnGridCellChanged(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPierPointGridCellChanged(WPARAM wParam, LPARAM lParam);
	afx_msg void OnHeightMeasureChanged();
	afx_msg void OnAddColumn();
	afx_msg void OnRemoveColumns();
	afx_msg void OnAddPierPoint();
	afx_msg void OnRemovePierPoints();
	afx_msg void OnPierLayoutChanged();
	afx_msg void OnRefColumnChanged();

	void ChangePierLayout() override;

	DECLARE_MESSAGE_MAP()

	pgsTypes::PierModelType m_PierModelType;
	CPierData2 m_Pier;

	CMetaFileStatic m_LayoutPicture;

	CColumnLayoutGrid m_ColumnLayoutGrid;
	CPierPointGrid m_PierPointGrid;
	CColumnFixityComboBox m_cbColumnFixity;
	CColumnData::ColumnHeightMeasurementType m_ColumnHeightMeasurementType;

	void FillRefColumnComboBox(ColumnIndexType nColumns=INVALID_INDEX);
	void FillHeightMeasureComboBox();
	void FillTransverseLocationComboBox();
	void RefreshDisplay();

	ColumnIndexType m_RefColumnIdx;
	Float64 m_TransverseOffset;
	pgsTypes::OffsetMeasurementType m_TransverseOffsetMeasurement;
	Float64 m_XBeamWidth;
	Float64 m_XBeamHeight[2];
	Float64 m_XBeamTaperHeight[2];
	Float64 m_XBeamTaperLength[2];
	Float64 m_XBeamEndSlopeOffset[2];
	Float64 m_XBeamOverhang[2];
	pgsTypes::ColumnLongitudinalBaseFixityType m_ColumnFixity;
};


