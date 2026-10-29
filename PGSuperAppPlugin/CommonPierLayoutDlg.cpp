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

// CommonPierLayoutDlg.cpp : implementation file
//

///////////////////////////////////////////////////////////////////////////
// NOTE: Duplicate code warning
//
// This dialog along with all its property pages are basically repeated in
// the PGSuperLibrary project. I could not get a single implementation to
// work because of issues with the module resources.
//
// If changes are made here, the same changes are likely needed in
// the other location.

#include "stdafx.h"
#include "PierLayoutPage.h"
#include "CommonPierLayoutDlg.h"
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <IFace/Tools.h>

CCommonPierLayoutDlg::CCommonPierLayoutDlg(CWnd* pParent)
    :CDialog(IDD_PIER_LAYOUT_COMMON, pParent)
{

    // only using the fixed option (no pinned at base of column,
    // it leads to unstable models before continuity is achieved)
    m_cbColumnFixity.SetFixityTypes(COLUMN_FIXITY_FIXED);

}

// Add to message map
BEGIN_MESSAGE_MAP(CCommonPierLayoutDlg, CDialog)
    ON_MESSAGE(WM_COLUMN_GRID_CELL_CHANGED, OnColumnGridCellChanged)

    ON_CBN_SELCHANGE(IDC_HEIGHT_MEASURE, OnHeightMeasureChanged)
    ON_BN_CLICKED(IDC_ADD_COLUMN, &CCommonPierLayoutDlg::OnAddColumn)
    ON_BN_CLICKED(IDC_REMOVE_COLUMN, &CCommonPierLayoutDlg::OnRemoveColumns)

    ON_EN_CHANGE(IDC_W, &CCommonPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_H1L, &CCommonPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_H2L, &CCommonPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_H1R, &CCommonPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_H2R, &CCommonPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_X2L, &CCommonPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_X1L, &CCommonPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_X2R, &CCommonPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_X1R, &CCommonPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_OHL, &CCommonPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_OHR, &CCommonPierLayoutDlg::OnPierLayoutChanged)

    ON_CBN_SELCHANGE(IDC_REFCOLUMN, &CCommonPierLayoutDlg::OnRefColumnChanged)
    ON_EN_CHANGE(IDC_REFCOLUMN_OFFSET, &CCommonPierLayoutDlg::OnRefColumnChanged)
    ON_CBN_SELCHANGE(IDC_REFCOLUMN_MEASUREMENT, &CCommonPierLayoutDlg::OnRefColumnChanged)

    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDBLCLK()

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCommonPierLayoutDlg message handlers

BOOL CCommonPierLayoutDlg::OnInitDialog()
{

    m_ColumnLayoutGrid.SubclassDlgItem(IDC_COLUMN_GRID, this);
    m_ColumnLayoutGrid.CustomInit();

    m_Pier.GetTransverseOffset(&m_RefColumnIdx, &m_TransverseOffset, &m_TransverseOffsetMeasurement);
    m_XBeamWidth = m_Pier.GetXBeamWidth();

    for (int i = 0; i < 2; i++)
    {
        pgsTypes::SideType side = (pgsTypes::SideType)i;
        m_Pier.GetXBeamDimensions(side, &m_XBeamHeight[side], &m_XBeamTaperHeight[side], &m_XBeamTaperLength[side], &m_XBeamEndSlopeOffset[side]);
        m_XBeamOverhang[side] = m_Pier.GetXBeamOverhang(side);
    }

    m_ColumnFixity = m_Pier.GetColumnFixity();

    FillTransverseLocationComboBox();
    FillRefColumnComboBox(m_Pier.GetColumnCount());
    FillHeightMeasureComboBox();

    CDialog::OnInitDialog();

    OnHeightMeasureChanged();


    return TRUE;
}

void CCommonPierLayoutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_FIXITY, m_cbColumnFixity);

    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

    DDX_MetaFileStatic(pDX, IDC_PIER_LAYOUT_GUIDE, m_LayoutPicture,_T("COMMONPIERLAYOUT"), _T("Metafile") );

    DDX_UnitValueAndTag(pDX, IDC_H1L, IDC_H1L_UNIT, m_XBeamHeight[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(pDX, IDC_H2L, IDC_H2L_UNIT, m_XBeamTaperHeight[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(pDX, IDC_X2L, IDC_X2L_UNIT, m_XBeamTaperLength[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(pDX, IDC_X1L, IDC_X1L_UNIT, m_XBeamEndSlopeOffset[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(pDX, IDC_H1R, IDC_H1R_UNIT, m_XBeamHeight[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(pDX, IDC_H2R, IDC_H2R_UNIT, m_XBeamTaperHeight[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(pDX, IDC_X2R, IDC_X2R_UNIT, m_XBeamTaperLength[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(pDX, IDC_X1R, IDC_X1R_UNIT, m_XBeamEndSlopeOffset[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(pDX, IDC_W, IDC_W_UNIT, m_XBeamWidth, pDisplayUnits->GetSpanLengthUnit());

    DDX_CBIndex(pDX, IDC_REFCOLUMN, m_RefColumnIdx);
    DDX_OffsetAndTag(pDX, IDC_REFCOLUMN_OFFSET, IDC_REFCOLUMN_OFFSET_UNIT, m_TransverseOffset, pDisplayUnits->GetSpanLengthUnit());
    DDX_CBItemData(pDX, IDC_REFCOLUMN_MEASUREMENT, m_TransverseOffsetMeasurement);

    DDX_UnitValueAndTag(pDX, IDC_OHL, IDC_OHL_UNIT, m_XBeamOverhang[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(pDX, IDC_OHR, IDC_OHR_UNIT, m_XBeamOverhang[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

    DDX_CBItemData(pDX, IDC_FIXITY, m_ColumnFixity);

    CColumnLayoutGrid::DDV_ColumnGrid(pDX, m_ColumnLayoutGrid);
    CColumnLayoutGrid::DDX_ColumnGrid(pDX, m_ColumnLayoutGrid, &m_Pier);

    m_ColumnHeightMeasurementType = m_Pier.GetColumnData(0).GetColumnHeightMeasurementType();
    DDX_CBItemData(pDX, IDC_HEIGHT_MEASURE, m_ColumnHeightMeasurementType);

    if (pDX->m_bSaveAndValidate)
    {
        // XBeam width, W, must be greater than zero
        DDV_UnitValueGreaterThanZero(pDX, IDC_W, m_XBeamWidth, pDisplayUnits->GetSpanLengthUnit());

        // H1 and H3 must be > 0
        DDV_UnitValueGreaterThanZero(pDX, IDC_OHL, m_XBeamHeight[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueGreaterThanZero(pDX, IDC_OHR, m_XBeamHeight[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

        // X1..X4 must be >= 0
        DDV_UnitValueZeroOrMore(pDX, IDC_X2L, m_XBeamTaperLength[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueZeroOrMore(pDX, IDC_X1L, m_XBeamEndSlopeOffset[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueZeroOrMore(pDX, IDC_X2R, m_XBeamTaperLength[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueZeroOrMore(pDX, IDC_X1R, m_XBeamEndSlopeOffset[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

        // Left end
        if (0 < m_XBeamTaperLength[pgsTypes::stLeft])
        {
            if (IsZero(m_XBeamTaperHeight[pgsTypes::stLeft]))
            {
                pDX->PrepareCtrl(IDC_H2L);
                AfxMessageBox(_T("H2L must be greater than zero when X2L is greater than zero."));
                pDX->Fail();
            }
            else if (m_XBeamTaperLength[pgsTypes::stLeft] < m_XBeamEndSlopeOffset[pgsTypes::stLeft])
            {
                pDX->PrepareCtrl(IDC_X2L);
                AfxMessageBox(_T("X2L must be greater than X1L when X2L is greater than zero."));
                pDX->Fail();
            }
        }
        else if (!IsZero(m_XBeamTaperHeight[pgsTypes::stLeft]))
        {
            pDX->PrepareCtrl(IDC_H2L);
            AfxMessageBox(_T("H2L must be zero when X2L is zero."));
            pDX->Fail();
        }

        // Right end
        if (0 < m_XBeamTaperLength[pgsTypes::stRight])
        {
            if (IsZero(m_XBeamTaperHeight[pgsTypes::stRight]))
            {
                pDX->PrepareCtrl(IDC_H2R);
                AfxMessageBox(_T("H2R must be greater than zero when X2R is greater than zero."));
                pDX->Fail();
            }
            else if (m_XBeamTaperLength[pgsTypes::stRight] < m_XBeamEndSlopeOffset[pgsTypes::stRight])
            {
                pDX->PrepareCtrl(IDC_X2R);
                AfxMessageBox(_T("X2R must be greater than X1R when X2R is greater than zero."));
                pDX->Fail();
            }
        }
        else if (!IsZero(m_XBeamTaperHeight[pgsTypes::stRight]))
        {
            pDX->PrepareCtrl(IDC_H2R);
            AfxMessageBox(_T("H4 must be zero when X2R is zero."));
            pDX->Fail();
        }

        // Overhangs must satisfy the first/last column radius limits.
        Float64 D1 = 0.0, D2 = 0.0;
        ATLASSERT(1 <= m_Pier.GetColumnCount());

        m_Pier.GetColumnData(0).GetColumnDimensions(&D1, &D2);
        DDV_UnitValueLimitOrMore(pDX, IDC_X5, m_XBeamOverhang[pgsTypes::stLeft], D1 / 2, pDisplayUnits->GetSpanLengthUnit());

        m_Pier.GetColumnData(m_Pier.GetColumnCount() - 1).GetColumnDimensions(&D1, &D2);
        DDV_UnitValueLimitOrMore(pDX, IDC_X6, m_XBeamOverhang[pgsTypes::stRight], D1 / 2, pDisplayUnits->GetSpanLengthUnit());

        Float64 spacingSum = 0.0;
        for (SpacingIndexType spaIdx = 0; spaIdx < m_Pier.GetColumnCount() - 1; spaIdx++)
        {
            spacingSum += m_Pier.GetColumnSpacing(spaIdx);
        }

        const Float64 pierWidth = m_XBeamOverhang[pgsTypes::stLeft] + m_XBeamOverhang[pgsTypes::stRight] + spacingSum;
        const Float64 taperLengthSum = m_XBeamTaperLength[pgsTypes::stLeft] + m_XBeamTaperLength[pgsTypes::stRight];
        if (pierWidth < taperLengthSum)
        {
            pDX->PrepareCtrl(IDC_OHL);
            AfxMessageBox(_T("X2L + X2R cannot exceed the overall pier width (OHL + OHR + summation of S)."));
            pDX->Fail();
        }
    }
}

void CCommonPierLayoutDlg::RefreshDisplay()
{
    if (GetParent())
        GetParent()->SendMessage(WM_PIER_LAYOUT_CHANGED);
}


void CCommonPierLayoutDlg::FillTransverseLocationComboBox()
{
    CComboBox* pcbMeasure = (CComboBox*)GetDlgItem(IDC_REFCOLUMN_MEASUREMENT);
    pcbMeasure->ResetContent();
    int idx = pcbMeasure->AddString(_T("from the Alignment"));
    pcbMeasure->SetItemData(idx, (DWORD_PTR)pgsTypes::omtAlignment);
    idx = pcbMeasure->AddString(_T("from the Bridgeline"));
    pcbMeasure->SetItemData(idx, (DWORD_PTR)pgsTypes::omtBridge);
}

void CCommonPierLayoutDlg::FillRefColumnComboBox(ColumnIndexType nColumns)
{
    CComboBox* pcbRefColumn = (CComboBox*)GetDlgItem(IDC_REFCOLUMN);
    int curSel = pcbRefColumn->GetCurSel();
    pcbRefColumn->ResetContent();
    if (nColumns == INVALID_INDEX)
    {
        nColumns = (ColumnIndexType)m_ColumnLayoutGrid.GetRowCount();
    }

    for (ColumnIndexType colIdx = 0; colIdx < nColumns; colIdx++)
    {
        CString strLabel;
        strLabel.Format(_T("Column %d"), LABEL_COLUMN(colIdx));
        pcbRefColumn->AddString(strLabel);
    }

    if (pcbRefColumn->SetCurSel(curSel) == CB_ERR)
    {
        if (m_RefColumnIdx < nColumns)
        {
            pcbRefColumn->SetCurSel(m_RefColumnIdx);
        }
        else
        {
            pcbRefColumn->SetCurSel(0);
        }
    }
}

void CCommonPierLayoutDlg::FillHeightMeasureComboBox()
{
    CComboBox* pcbHeightMeasure = (CComboBox*)GetDlgItem(IDC_HEIGHT_MEASURE);
    pcbHeightMeasure->ResetContent();
    int idx = pcbHeightMeasure->AddString(_T("Column Height (H)"));
    pcbHeightMeasure->SetItemData(idx, (DWORD_PTR)CColumnData::chtHeight);
    idx = pcbHeightMeasure->AddString(_T("Bottom Elevation"));
    pcbHeightMeasure->SetItemData(idx, (DWORD_PTR)CColumnData::chtBottomElevation);
}

void CCommonPierLayoutDlg::OnHeightMeasureChanged()
{
    CComboBox* pcbHeightMeasure = (CComboBox*)GetDlgItem(IDC_HEIGHT_MEASURE);
    int curSel = pcbHeightMeasure->GetCurSel();
    CColumnData::ColumnHeightMeasurementType measure = (CColumnData::ColumnHeightMeasurementType)(pcbHeightMeasure->GetItemData(curSel));
    m_ColumnLayoutGrid.SetHeightMeasurementType(measure);
}

LRESULT CCommonPierLayoutDlg::OnColumnGridCellChanged(WPARAM wParam, LPARAM lParam)
{

    FillRefColumnComboBox();

    // Update pier data with current column data
    m_ColumnLayoutGrid.GetColumnData(m_Pier);

    OnPierLayoutChanged();

    return 0;
}

void CCommonPierLayoutDlg::OnAddColumn()
{
    m_ColumnLayoutGrid.AddColumn();
    FillRefColumnComboBox();

    // Update pier data with current column data
    m_ColumnLayoutGrid.GetColumnData(m_Pier);

    OnPierLayoutChanged();

}

void CCommonPierLayoutDlg::OnRemoveColumns()
{
	if (m_ColumnLayoutGrid.GetRowCount() > 1)
    {
        m_ColumnLayoutGrid.RemoveSelectedColumns();


        FillRefColumnComboBox();

        // Update pier data with current column data
        const auto nCols = m_Pier.GetColumnCount();
        if (nCols > 1)
        {
            m_ColumnLayoutGrid.GetColumnData(m_Pier);
        }

    }
    else
    {
        AfxMessageBox(_T("The pier must have at least one column"), MB_OK | MB_ICONEXCLAMATION);
    }
        
    OnPierLayoutChanged();
}

void CCommonPierLayoutDlg::SetPierModelType(const pgsTypes::PierModelType& pierModelType)
{
    m_PierModelType = pierModelType;
}

void CCommonPierLayoutDlg::SetPierData(const CPierData2& pierData)
{
    m_Pier = pierData;
    m_Pier.SetPierLayoutType(pgsTypes::pltCommon);
}

const CPierData2* CCommonPierLayoutDlg::GetPierData() const
{
    return &m_Pier;
}

void CCommonPierLayoutDlg::OnPierLayoutChanged()
{
	ChangePierLayout();
}

void CCommonPierLayoutDlg::ChangePierLayout()
{
    // Get the current values from the edit controls into member variables
    CDataExchange dx(this, TRUE);

    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

    CPierLayoutPage* pPage =
        DYNAMIC_DOWNCAST(CPierLayoutPage, GetParent());

    // Exchange XBeam dimensions
    DDX_UnitValueAndTag(&dx, IDC_H1L, IDC_H1L_UNIT, m_XBeamHeight[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(&dx, IDC_H2L, IDC_H2L_UNIT, m_XBeamTaperHeight[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(&dx, IDC_X2L, IDC_X2L_UNIT, m_XBeamTaperLength[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(&dx, IDC_X1L, IDC_X1L_UNIT, m_XBeamEndSlopeOffset[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(&dx, IDC_H1R, IDC_H1R_UNIT, m_XBeamHeight[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(&dx, IDC_H2R, IDC_H2R_UNIT, m_XBeamTaperHeight[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(&dx, IDC_X2R, IDC_X2R_UNIT, m_XBeamTaperLength[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(&dx, IDC_X1R, IDC_X1R_UNIT, m_XBeamEndSlopeOffset[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(&dx, IDC_W, IDC_W_UNIT, m_XBeamWidth, pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(&dx, IDC_OHL, IDC_OHL_UNIT, m_XBeamOverhang[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(&dx, IDC_OHR, IDC_OHR_UNIT, m_XBeamOverhang[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

    // Update the pier data with the current values
    m_Pier.SetXBeamDimensions(pgsTypes::stLeft, m_XBeamHeight[pgsTypes::stLeft], m_XBeamTaperHeight[pgsTypes::stLeft],
        m_XBeamTaperLength[pgsTypes::stLeft], m_XBeamEndSlopeOffset[pgsTypes::stLeft]);

    m_Pier.SetXBeamDimensions(pgsTypes::stRight, m_XBeamHeight[pgsTypes::stRight], m_XBeamTaperHeight[pgsTypes::stRight],
        m_XBeamTaperLength[pgsTypes::stRight], m_XBeamEndSlopeOffset[pgsTypes::stRight]);

    m_Pier.SetXBeamWidth(m_XBeamWidth);

    m_Pier.SetXBeamOverhang(pgsTypes::stLeft, m_XBeamOverhang[pgsTypes::stLeft]);
    m_Pier.SetXBeamOverhang(pgsTypes::stRight, m_XBeamOverhang[pgsTypes::stRight]);

    if (pPage->m_pPierLayoutPopout != nullptr)
    {
        // XBeam width, W, must be greater than zero
        DDV_UnitValueGreaterThanZero(&dx, IDC_W, m_XBeamWidth, pDisplayUnits->GetSpanLengthUnit());

        // H1 and H3 must be > 0
        DDV_UnitValueGreaterThanZero(&dx, IDC_H1L, m_XBeamHeight[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueGreaterThanZero(&dx, IDC_H1R, m_XBeamHeight[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

        // X1..X4 must be >= 0
        DDV_UnitValueZeroOrMore(&dx, IDC_X2L, m_XBeamTaperLength[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueZeroOrMore(&dx, IDC_X1L, m_XBeamEndSlopeOffset[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueZeroOrMore(&dx, IDC_X2R, m_XBeamTaperLength[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueZeroOrMore(&dx, IDC_X1R, m_XBeamEndSlopeOffset[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

        // Left end
        if (0 < m_XBeamTaperLength[pgsTypes::stLeft])
        {
            if (IsZero(m_XBeamTaperHeight[pgsTypes::stLeft]))
            {
                dx.PrepareCtrl(IDC_H2L);
                AfxMessageBox(_T("H2L must be greater than zero when X2L is greater than zero."));
                dx.Fail();
            }
            else if (m_XBeamTaperLength[pgsTypes::stLeft] < m_XBeamEndSlopeOffset[pgsTypes::stLeft])
            {
                dx.PrepareCtrl(IDC_X2L);
                AfxMessageBox(_T("X2L must be greater than X1L when X2L is greater than zero."));
                dx.Fail();
            }
        }
        else if (!IsZero(m_XBeamTaperHeight[pgsTypes::stLeft]))
        {
            dx.PrepareCtrl(IDC_H2L);
            AfxMessageBox(_T("H2L must be zero when X2L is zero."));
            dx.Fail();
        }

        // Right end
        if (0 < m_XBeamTaperLength[pgsTypes::stRight])
        {
            if (IsZero(m_XBeamTaperHeight[pgsTypes::stRight]))
            {
                dx.PrepareCtrl(IDC_H2R);
                AfxMessageBox(_T("H2R must be greater than zero when X2R is greater than zero."));
                dx.Fail();
            }
            else if (m_XBeamTaperLength[pgsTypes::stRight] < m_XBeamEndSlopeOffset[pgsTypes::stRight])
            {
                dx.PrepareCtrl(IDC_X2R);
                AfxMessageBox(_T("X2R must be greater than X1R when X2R is greater than zero."));
                dx.Fail();
            }
        }
        else if (!IsZero(m_XBeamTaperHeight[pgsTypes::stRight]))
        {
            dx.PrepareCtrl(IDC_H2R);
            AfxMessageBox(_T("H2R must be zero when X2R is zero."));
            dx.Fail();
        }

        // Overhangs must satisfy the first/last column radius limits.
        Float64 D1 = 0.0, D2 = 0.0;
        ATLASSERT(1 <= m_Pier.GetColumnCount());

        m_Pier.GetColumnData(0).GetColumnDimensions(&D1, &D2);
        DDV_UnitValueLimitOrMore(&dx, IDC_OHL, m_XBeamOverhang[pgsTypes::stLeft], D1 / 2, pDisplayUnits->GetSpanLengthUnit());

        m_Pier.GetColumnData(m_Pier.GetColumnCount() - 1).GetColumnDimensions(&D1, &D2);
        DDV_UnitValueLimitOrMore(&dx, IDC_OHR, m_XBeamOverhang[pgsTypes::stRight], D1 / 2, pDisplayUnits->GetSpanLengthUnit());

        Float64 spacingSum = 0.0;
        for (SpacingIndexType spaIdx = 0; spaIdx < m_Pier.GetColumnCount() - 1; spaIdx++)
        {
            spacingSum += m_Pier.GetColumnSpacing(spaIdx);
        }

        const Float64 pierWidth = m_XBeamOverhang[pgsTypes::stLeft] + m_XBeamOverhang[pgsTypes::stRight] + spacingSum;
        const Float64 taperLengthSum = m_XBeamTaperLength[pgsTypes::stLeft] + m_XBeamTaperLength[pgsTypes::stRight];
        if (pierWidth < taperLengthSum)
        {
            dx.PrepareCtrl(IDC_OHL);
            AfxMessageBox(_T("X2L + X2R cannot exceed the overall pier width (OHL + OHR + summation of S)."));
            dx.Fail();
        }
    }

    RefreshDisplay();
}

void CCommonPierLayoutDlg::OnRefColumnChanged()
{
    // Get the current values from the edit controls into member variables
    CDataExchange dx(this, TRUE);

    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

    DDX_CBIndex(&dx, IDC_REFCOLUMN, m_RefColumnIdx);
    DDX_OffsetAndTag(&dx, IDC_REFCOLUMN_OFFSET, IDC_REFCOLUMN_OFFSET_UNIT, m_TransverseOffset, pDisplayUnits->GetSpanLengthUnit());
    DDX_CBItemData(&dx, IDC_REFCOLUMN_MEASUREMENT, m_TransverseOffsetMeasurement);

	m_Pier.SetTransverseOffset(m_RefColumnIdx, m_TransverseOffset, m_TransverseOffsetMeasurement);

    OnPierLayoutChanged();
}
