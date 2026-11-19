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

// UserDefinedPierLayoutDlg.cpp : implementation file
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
#include "UserDefinedPierLayoutDlg.h"
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Alignment.h>
#include <IFace/Tools.h>

CUserDefinedPierLayoutDlg::CUserDefinedPierLayoutDlg(CWnd* pParent)
    :CDialog(IDD_PIER_LAYOUT_USERDEFINED, pParent)
{

    // only using the fixed option (no pinned at base of column,
    // it leads to unstable models before continuity is achieved)
    m_cbColumnFixity.SetFixityTypes(COLUMN_FIXITY_FIXED);

}

// Add to message map
BEGIN_MESSAGE_MAP(CUserDefinedPierLayoutDlg, CDialog)
    ON_MESSAGE(WM_COLUMN_GRID_CELL_CHANGED, OnColumnGridCellChanged)
    ON_MESSAGE(WM_PIERPOINT_GRID_CELL_CHANGED, OnPierPointGridCellChanged)

    ON_CBN_SELCHANGE(IDC_HEIGHT_MEASURE, OnHeightMeasureChanged)
    ON_BN_CLICKED(IDC_ADD_COLUMN, &CUserDefinedPierLayoutDlg::OnAddColumn)
    ON_BN_CLICKED(IDC_REMOVE_COLUMN, &CUserDefinedPierLayoutDlg::OnRemoveColumns)

    ON_BN_CLICKED(IDC_ADD_PIERPOINT, &CUserDefinedPierLayoutDlg::OnAddPierPoint)
    ON_BN_CLICKED(IDC_REMOVE_PIERPOINT, &CUserDefinedPierLayoutDlg::OnRemovePierPoints)

    ON_EN_KILLFOCUS(IDC_W, &CUserDefinedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_KILLFOCUS(IDC_H1L, &CUserDefinedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_KILLFOCUS(IDC_H1R, &CUserDefinedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_KILLFOCUS(IDC_X1L, &CUserDefinedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_KILLFOCUS(IDC_X1R, &CUserDefinedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_KILLFOCUS(IDC_OHL, &CUserDefinedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_KILLFOCUS(IDC_OHR, &CUserDefinedPierLayoutDlg::OnPierLayoutChanged)

    ON_CBN_SELCHANGE(IDC_REFCOLUMN, &CUserDefinedPierLayoutDlg::OnRefColumnChanged)
    ON_EN_KILLFOCUS(IDC_REFCOLUMN_OFFSET, &CUserDefinedPierLayoutDlg::OnRefColumnChanged)
    ON_CBN_SELCHANGE(IDC_REFCOLUMN_MEASUREMENT, &CUserDefinedPierLayoutDlg::OnRefColumnChanged)

    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDBLCLK()

END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CUserDefinedPierLayoutDlg message handlers

BOOL CUserDefinedPierLayoutDlg::OnInitDialog()
{

    m_ColumnLayoutGrid.SubclassDlgItem(IDC_COLUMN_GRID, this);
    m_ColumnLayoutGrid.CustomInit();

    m_PierPointGrid.SubclassDlgItem(IDC_PIER_POINT_GRID, this);
    m_PierPointGrid.CustomInit();

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
    m_Initialized = true;

    return TRUE;
}

void CUserDefinedPierLayoutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_FIXITY, m_cbColumnFixity);

    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

    DDX_MetaFileStatic(pDX, IDC_PIER_LAYOUT_GUIDE, m_LayoutPicture,_T("USERDEFINEDPIERLAYOUT"), _T("Metafile") );

    DDX_UnitValueAndTag(pDX, IDC_H1L, IDC_H1L_UNIT, m_XBeamHeight[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(pDX, IDC_X1L, IDC_X1L_UNIT, m_XBeamEndSlopeOffset[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(pDX, IDC_H1R, IDC_H1R_UNIT, m_XBeamHeight[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(pDX, IDC_X1R, IDC_X1R_UNIT, m_XBeamEndSlopeOffset[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(pDX, IDC_OHL, IDC_OHL_UNIT, m_XBeamOverhang[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(pDX, IDC_OHR, IDC_OHR_UNIT, m_XBeamOverhang[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(pDX, IDC_W, IDC_W_UNIT, m_XBeamWidth, pDisplayUnits->GetSpanLengthUnit());

    DDX_CBIndex(pDX, IDC_REFCOLUMN, m_RefColumnIdx);
    DDX_OffsetAndTag(pDX, IDC_REFCOLUMN_OFFSET, IDC_REFCOLUMN_OFFSET_UNIT, m_TransverseOffset, pDisplayUnits->GetSpanLengthUnit());
    DDX_CBItemData(pDX, IDC_REFCOLUMN_MEASUREMENT, m_TransverseOffsetMeasurement);

    DDX_CBItemData(pDX, IDC_FIXITY, m_ColumnFixity);

    CColumnLayoutGrid::DDV_ColumnGrid(pDX, m_ColumnLayoutGrid);
    CColumnLayoutGrid::DDX_ColumnGrid(pDX, m_ColumnLayoutGrid, &m_Pier);

    if (pDX->m_bSaveAndValidate)
    {
        Float64 length = m_XBeamOverhang[0] + m_XBeamOverhang[1] -
            m_XBeamEndSlopeOffset[0] - m_XBeamEndSlopeOffset[1];
        for (ColumnIndexType idx = 1; idx < m_Pier.GetColumnCount(); ++idx)
            length += m_Pier.GetColumnSpacing(idx - 1);
        m_PierPointGrid.UpdateEndPoint(length, m_XBeamHeight[1], m_XBeamHeight[0]);
    }

    CPierPointGrid::DDV_PierPointGrid(pDX, m_PierPointGrid);
    CPierPointGrid::DDX_PierPointGrid(pDX, m_PierPointGrid, &m_Pier);

    m_ColumnHeightMeasurementType = m_Pier.GetColumnData(0).GetColumnHeightMeasurementType();
    DDX_CBItemData(pDX, IDC_HEIGHT_MEASURE, m_ColumnHeightMeasurementType);

    if (pDX->m_bSaveAndValidate)
    {
        // XBeam width, W, must be greater than zero
        DDV_UnitValueGreaterThanZero(pDX, IDC_W, m_XBeamWidth, pDisplayUnits->GetSpanLengthUnit());

        // H1 and H3 must be > 0
        DDV_UnitValueGreaterThanZero(pDX, IDC_H1L, m_XBeamHeight[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueGreaterThanZero(pDX, IDC_H1R, m_XBeamHeight[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

        // X1..X4 must be >= 0
        DDV_UnitValueZeroOrMore(pDX, IDC_X1L, m_XBeamEndSlopeOffset[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueZeroOrMore(pDX, IDC_X1R, m_XBeamEndSlopeOffset[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

        DDV_UnitValueZeroOrMore(pDX, IDC_OHL, m_XBeamOverhang[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueZeroOrMore(pDX, IDC_OHR, m_XBeamOverhang[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());


        const IndexType nCols = m_Pier.GetColumnCount();

        Float64 xxbRightColumn = m_XBeamOverhang[pgsTypes::stLeft] - m_XBeamEndSlopeOffset[pgsTypes::stLeft];

        IndexType nSpaces = (nCols - 1);
        for (IndexType idx = 0; idx < nSpaces; idx++)
        {
            Float64 spacing = m_Pier.GetColumnSpacing(idx);
            xxbRightColumn += spacing;
        }

        Float64 xxbRightLimit = xxbRightColumn + m_XBeamOverhang[pgsTypes::stRight] - m_XBeamEndSlopeOffset[pgsTypes::stRight];

        Float64 prevPoint = -DBL_MAX;

        for (const auto& ppData : m_Pier.GetPierPointData())
        {

            const auto& xxb = ppData.Get_X();
            const auto& y = ppData.Get_Y();

            if (xxb < 0.0 || xxb > xxbRightLimit || y < 0.0)
            {
                CString msg = _T("Pier point is outside the top and sides of the lower crossbeam.");
                AfxMessageBox(msg);
                pDX->Fail();
            }

            if (xxb < prevPoint)
            {
                CString msg = _T("Pier point must increase left to right along the crossbeam.");
                AfxMessageBox(msg);
                pDX->Fail();
            }

            prevPoint = xxb;

        }

        CPierData2 candidate;
        candidate = m_Pier; // Preserve pier index and ID; the copy constructor clears them.
        for (int side = 0; side < 2; ++side)
        {
            candidate.SetXBeamDimensions(static_cast<pgsTypes::SideType>(side),m_XBeamHeight[side],
                m_XBeamTaperHeight[side],m_XBeamTaperLength[side],m_XBeamEndSlopeOffset[side]);
            candidate.SetXBeamOverhang(static_cast<pgsTypes::SideType>(side),m_XBeamOverhang[side]);
        }
        candidate.SetTransverseOffset(m_RefColumnIdx,m_TransverseOffset,m_TransverseOffsetMeasurement);
        try
        {
            GET_IFACE2(pBroker, IBridge, pBridge);
            CComPtr<IPoint2dCollection> profile;
            pBridge->GetBottomXBeamProfile(candidate,&profile);
        }
        catch (HRESULT)
        {
            AfxMessageBox(_T("Invalid profile arc. Increase the radius, adjust the endpoints, or change the arc direction. "
                "Arcs must run left to right and remain below the top of the crossbeam."));
            pDX->Fail();
        }
    }

}

void CUserDefinedPierLayoutDlg::RefreshDisplay()
{
    if (GetParent())
        GetParent()->SendMessage(WM_PIER_LAYOUT_CHANGED);
}

void CUserDefinedPierLayoutDlg::FillTransverseLocationComboBox()
{
    CComboBox* pcbMeasure = (CComboBox*)GetDlgItem(IDC_REFCOLUMN_MEASUREMENT);
    pcbMeasure->ResetContent();
    int idx = pcbMeasure->AddString(_T("from the Alignment"));
    pcbMeasure->SetItemData(idx, (DWORD_PTR)pgsTypes::omtAlignment);
    idx = pcbMeasure->AddString(_T("from the Bridgeline"));
    pcbMeasure->SetItemData(idx, (DWORD_PTR)pgsTypes::omtBridge);
}

void CUserDefinedPierLayoutDlg::FillRefColumnComboBox(ColumnIndexType nColumns)
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

void CUserDefinedPierLayoutDlg::FillHeightMeasureComboBox()
{
    CComboBox* pcbHeightMeasure = (CComboBox*)GetDlgItem(IDC_HEIGHT_MEASURE);
    pcbHeightMeasure->ResetContent();
    int idx = pcbHeightMeasure->AddString(_T("Column Height (H)"));
    pcbHeightMeasure->SetItemData(idx, (DWORD_PTR)CColumnData::chtHeight);
    idx = pcbHeightMeasure->AddString(_T("Bottom Elevation"));
    pcbHeightMeasure->SetItemData(idx, (DWORD_PTR)CColumnData::chtBottomElevation);
}

void CUserDefinedPierLayoutDlg::OnHeightMeasureChanged()
{
    CComboBox* pcbHeightMeasure = (CComboBox*)GetDlgItem(IDC_HEIGHT_MEASURE);
    int curSel = pcbHeightMeasure->GetCurSel();
    CColumnData::ColumnHeightMeasurementType measure = (CColumnData::ColumnHeightMeasurementType)(pcbHeightMeasure->GetItemData(curSel));
    m_ColumnLayoutGrid.SetHeightMeasurementType(measure);
}

LRESULT CUserDefinedPierLayoutDlg::OnColumnGridCellChanged(WPARAM wParam, LPARAM lParam)
{

    FillRefColumnComboBox();

    // Update pier data with current column data
    m_ColumnLayoutGrid.GetColumnData(m_Pier);

    OnPierLayoutChanged();

    return 0;
}

LRESULT CUserDefinedPierLayoutDlg::OnPierPointGridCellChanged(WPARAM wParam, LPARAM lParam)
{
    // Update pier data with current pier point data
    OnPierLayoutChanged();

    return 0;
}

void CUserDefinedPierLayoutDlg::OnAddColumn()
{
    m_ColumnLayoutGrid.AddColumn();
    FillRefColumnComboBox();

    // Update pier data with current column data
    m_ColumnLayoutGrid.GetColumnData(m_Pier);

    OnPierLayoutChanged();

}

void CUserDefinedPierLayoutDlg::OnRemoveColumns()
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

        OnPierLayoutChanged();
    }
    else
    {
        AfxMessageBox(_T("The pier must have at least one column"), MB_OK | MB_ICONEXCLAMATION);
    }
}

void CUserDefinedPierLayoutDlg::OnAddPierPoint()
{
    m_PierPointGrid.AddPierPoint();

    // Update pier data with current pier point data
    m_PierPointGrid.GetPierPointData(m_Pier);

    OnPierLayoutChanged();

}

void CUserDefinedPierLayoutDlg::OnRemovePierPoints()
{
    m_PierPointGrid.RemoveSelectedPierPoints();

    // Update pier data with current column data
    m_PierPointGrid.GetPierPointData(m_Pier);
    
    OnPierLayoutChanged();
}

void CUserDefinedPierLayoutDlg::SetPierModelType(const pgsTypes::PierModelType& pierModelType)
{
    m_PierModelType = pierModelType;
}

void CUserDefinedPierLayoutDlg::SetPierData(const CPierData2& pierData)
{
    m_Pier = pierData;
    m_Pier.SetPierLayoutType(pgsTypes::pltUserDefined);
}

const CPierData2* CUserDefinedPierLayoutDlg::GetPierData() const
{
    return &m_Pier;
}

void CUserDefinedPierLayoutDlg::OnPierLayoutChanged()
{
    if (!m_Initialized || m_UpdatingLayout) return;
    m_UpdatingLayout = true;
    CPierData2 previous;
    previous = m_Pier; // Preserve identity if validation fails and we restore the snapshot.
    if (UpdateData(TRUE)) ChangePierLayout();
    else m_Pier = previous;
    m_UpdatingLayout = false;
}

void CUserDefinedPierLayoutDlg::ChangePierLayout()
{
    // Get the current values from the edit controls into member variables
    CDataExchange dx(this, TRUE);

    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

    CPierLayoutPage* pPage =
        DYNAMIC_DOWNCAST(CPierLayoutPage, GetParent());

    // Exchange XBeam dimensions
    DDX_UnitValueAndTag(&dx, IDC_H1L, IDC_H1L_UNIT, m_XBeamHeight[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(&dx, IDC_X1L, IDC_X1L_UNIT, m_XBeamEndSlopeOffset[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(&dx, IDC_H1R, IDC_H1R_UNIT, m_XBeamHeight[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(&dx, IDC_X1R, IDC_X1R_UNIT, m_XBeamEndSlopeOffset[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(&dx, IDC_OHL, IDC_OHL_UNIT, m_XBeamOverhang[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(&dx, IDC_OHR, IDC_OHR_UNIT, m_XBeamOverhang[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(&dx, IDC_W, IDC_W_UNIT, m_XBeamWidth, pDisplayUnits->GetSpanLengthUnit());

    // Update the pier data with the current values
    m_Pier.SetXBeamDimensions(pgsTypes::stLeft, m_XBeamHeight[pgsTypes::stLeft], m_XBeamTaperHeight[pgsTypes::stLeft],
        m_XBeamTaperLength[pgsTypes::stLeft], m_XBeamEndSlopeOffset[pgsTypes::stLeft]);

    m_Pier.SetXBeamDimensions(pgsTypes::stRight, m_XBeamHeight[pgsTypes::stRight], m_XBeamTaperHeight[pgsTypes::stRight],
        m_XBeamTaperLength[pgsTypes::stRight], m_XBeamEndSlopeOffset[pgsTypes::stRight]);

    m_Pier.SetXBeamWidth(m_XBeamWidth);

    m_Pier.SetXBeamOverhang(pgsTypes::stLeft, m_XBeamOverhang[pgsTypes::stLeft]);
    m_Pier.SetXBeamOverhang(pgsTypes::stRight, m_XBeamOverhang[pgsTypes::stRight]);

    const Float64 length = m_XBeamOverhang[0] + m_XBeamOverhang[1] + m_Pier.GetColumnSpacingWidth() -
        m_XBeamEndSlopeOffset[0] - m_XBeamEndSlopeOffset[1];
    m_PierPointGrid.UpdateEndPoint(length, m_XBeamHeight[1], m_XBeamHeight[0]);
    m_PierPointGrid.GetPierPointData(m_Pier);

    if (pPage->m_pPierLayoutPopout != nullptr)
    {
        // XBeam width, W, must be greater than zero
        DDV_UnitValueGreaterThanZero(&dx, IDC_W, m_XBeamWidth, pDisplayUnits->GetSpanLengthUnit());

        // H1 and H3 must be > 0
        DDV_UnitValueGreaterThanZero(&dx, IDC_H1L, m_XBeamHeight[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueGreaterThanZero(&dx, IDC_H1R, m_XBeamHeight[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

        // X1..X4 must be >= 0
        DDV_UnitValueZeroOrMore(&dx, IDC_X1L, m_XBeamEndSlopeOffset[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueZeroOrMore(&dx, IDC_X1R, m_XBeamEndSlopeOffset[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());


        const IndexType nCols = m_Pier.GetColumnCount();

        Float64 xxbRightColumn = m_XBeamOverhang[pgsTypes::stLeft] - m_XBeamEndSlopeOffset[pgsTypes::stLeft];

        IndexType nSpaces = (nCols - 1);
        for (IndexType idx = 0; idx < nSpaces; idx++)
        {
            Float64 spacing = m_Pier.GetColumnSpacing(idx);
            xxbRightColumn += spacing;
        }

        Float64 xxbRightLimit = xxbRightColumn + m_XBeamOverhang[pgsTypes::stRight] - m_XBeamEndSlopeOffset[pgsTypes::stRight];

        Float64 prevPoint = -DBL_MAX;

        for (const auto& ppData : m_Pier.GetPierPointData())
        {

            const auto& xxb = ppData.Get_X();
            const auto& y = ppData.Get_Y();

            if (xxb < 0.0 || xxb > xxbRightLimit || y < 0.0)
            {
                CString msg = _T("Pier point is outside the top and sides of the lower crossbeam.");
                AfxMessageBox(msg);
                dx.Fail();
            }

            if (xxb < prevPoint)
            {
                CString msg = _T("Pier point must increase left to right along the crossbeam.");
                AfxMessageBox(msg);
                dx.Fail();
            }

            prevPoint = xxb;

        }
    }

    RefreshDisplay();
}

void CUserDefinedPierLayoutDlg::OnRefColumnChanged()
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
