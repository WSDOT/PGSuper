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

// ScallopedPierLayoutDlg.cpp : implementation file
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
#include "ScallopedPierLayoutDlg.h"
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <IFace/Tools.h>
#include <IFace/Bridge.h>
#include "ScallopGeometry.h"
#include <Math\PiecewiseFunction.h>

CScallopedPierLayoutDlg::CScallopedPierLayoutDlg(CWnd* pParent)
    :CDialog(IDD_PIER_LAYOUT_SCALLOPED, pParent)
{

    // only using the fixed option (no pinned at base of column,
    // it leads to unstable models before continuity is achieved)
    m_cbColumnFixity.SetFixityTypes(COLUMN_FIXITY_FIXED);

}

// Add to message map
BEGIN_MESSAGE_MAP(CScallopedPierLayoutDlg, CDialog)
    ON_MESSAGE(WM_COLUMN_GRID_CELL_CHANGED, OnColumnGridCellChanged)

    ON_CBN_SELCHANGE(IDC_HEIGHT_MEASURE, OnHeightMeasureChanged)
    ON_BN_CLICKED(IDC_ADD_COLUMN, &CScallopedPierLayoutDlg::OnAddColumn)
    ON_BN_CLICKED(IDC_REMOVE_COLUMN, &CScallopedPierLayoutDlg::OnRemoveColumns)

    ON_EN_KILLFOCUS(IDC_W, &CScallopedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_KILLFOCUS(IDC_H1L, &CScallopedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_KILLFOCUS(IDC_H1R, &CScallopedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_KILLFOCUS(IDC_RL, &CScallopedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_KILLFOCUS(IDC_RR, &CScallopedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_KILLFOCUS(IDC_X1L, &CScallopedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_KILLFOCUS(IDC_X1R, &CScallopedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_KILLFOCUS(IDC_OHL, &CScallopedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_KILLFOCUS(IDC_OHR, &CScallopedPierLayoutDlg::OnPierLayoutChanged)

    ON_CBN_SELCHANGE(IDC_REFCOLUMN, &CScallopedPierLayoutDlg::OnRefColumnChanged)
    ON_EN_KILLFOCUS(IDC_REFCOLUMN_OFFSET, &CScallopedPierLayoutDlg::OnRefColumnChanged)
    ON_CBN_SELCHANGE(IDC_REFCOLUMN_MEASUREMENT, &CScallopedPierLayoutDlg::OnRefColumnChanged)

    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDBLCLK()

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScallopedPierLayoutDlg message handlers

BOOL CScallopedPierLayoutDlg::OnInitDialog()
{
    m_ColumnLayoutGrid.SubclassDlgItem(IDC_COLUMN_GRID, this);
    m_ColumnLayoutGrid.CustomInit();

    m_Pier.GetTransverseOffset(&m_RefColumnIdx, &m_TransverseOffset, &m_TransverseOffsetMeasurement);
    m_XBeamWidth = m_Pier.GetXBeamWidth();

    for (int i = 0; i < 2; i++)
    {
        pgsTypes::SideType side = (pgsTypes::SideType)i;
		Float64 xBeamHeight, xBeamTaperHeight, xBeamTaperLength;
        m_Pier.GetXBeamDimensions(side, &m_XBeamHeight[side], &xBeamTaperHeight, &xBeamTaperLength, &m_XBeamEndSlopeOffset[side]);
        m_XBeamOverhang[side] = m_Pier.GetXBeamOverhang(side);
        m_XBeamOverhangRadius[side] = m_Pier.GetXBeamOverhangRadius(side);
    }

    m_ColumnFixity = m_Pier.GetColumnFixity();

    FillTransverseLocationComboBox();
    FillRefColumnComboBox(m_Pier.GetColumnCount());
    FillHeightMeasureComboBox();

    CDialog::OnInitDialog();

    // Set grid presentation while notifications are still suppressed.
    // PierLayoutPage constructs every layout dialog, including inactive ones.
    OnHeightMeasureChanged();
    m_Initialized = true;

    return TRUE;
}

void CScallopedPierLayoutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_FIXITY, m_cbColumnFixity);

    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

    DDX_MetaFileStatic(pDX, IDC_PIER_LAYOUT_GUIDE, m_LayoutPicture,_T("SCALLOPEDPIERLAYOUT"), _T("Metafile") );

    DDX_UnitValueAndTag(pDX, IDC_H1L, IDC_H1L_UNIT, m_XBeamHeight[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(pDX, IDC_H1R, IDC_H1R_UNIT, m_XBeamHeight[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(pDX, IDC_X1L, IDC_X1L_UNIT, m_XBeamEndSlopeOffset[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(pDX, IDC_X1R, IDC_X1R_UNIT, m_XBeamEndSlopeOffset[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(pDX, IDC_OHL, IDC_OHL_UNIT, m_XBeamOverhang[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(pDX, IDC_OHR, IDC_OHR_UNIT, m_XBeamOverhang[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(pDX, IDC_RL, IDC_RL_UNIT, m_XBeamOverhangRadius[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(pDX, IDC_RR, IDC_RR_UNIT, m_XBeamOverhangRadius[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(pDX, IDC_W, IDC_W_UNIT, m_XBeamWidth, pDisplayUnits->GetSpanLengthUnit());

    DDX_CBIndex(pDX, IDC_REFCOLUMN, m_RefColumnIdx);
    DDX_OffsetAndTag(pDX, IDC_REFCOLUMN_OFFSET, IDC_REFCOLUMN_OFFSET_UNIT, m_TransverseOffset, pDisplayUnits->GetSpanLengthUnit());
    DDX_CBItemData(pDX, IDC_REFCOLUMN_MEASUREMENT, m_TransverseOffsetMeasurement);

    DDX_CBItemData(pDX, IDC_FIXITY, m_ColumnFixity);

    CScallopedColumnLayoutGrid::DDV_ColumnGrid(pDX, m_ColumnLayoutGrid);
    if (!pDX->m_bSaveAndValidate)
        m_ColumnLayoutGrid.SetColumnData(m_Pier);

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

        DDV_UnitValueGreaterThanZero(pDX, IDC_RL, m_XBeamOverhangRadius[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueGreaterThanZero(pDX, IDC_RR, m_XBeamOverhangRadius[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());
        ValidateScallopGeometry(pDX);
    }
}

void CScallopedPierLayoutDlg::RefreshDisplay()
{
    if (GetParent())
        GetParent()->SendMessage(WM_PIER_LAYOUT_CHANGED);
}


void CScallopedPierLayoutDlg::FillTransverseLocationComboBox()
{
    CComboBox* pcbMeasure = (CComboBox*)GetDlgItem(IDC_REFCOLUMN_MEASUREMENT);
    pcbMeasure->ResetContent();
    int idx = pcbMeasure->AddString(_T("from the Alignment"));
    pcbMeasure->SetItemData(idx, (DWORD_PTR)pgsTypes::omtAlignment);
    idx = pcbMeasure->AddString(_T("from the Bridgeline"));
    pcbMeasure->SetItemData(idx, (DWORD_PTR)pgsTypes::omtBridge);
}

void CScallopedPierLayoutDlg::FillRefColumnComboBox(ColumnIndexType nColumns)
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

void CScallopedPierLayoutDlg::FillHeightMeasureComboBox()
{
    CComboBox* pcbHeightMeasure = (CComboBox*)GetDlgItem(IDC_HEIGHT_MEASURE);
    pcbHeightMeasure->ResetContent();
    int idx = pcbHeightMeasure->AddString(_T("Column Height (H)"));
    pcbHeightMeasure->SetItemData(idx, (DWORD_PTR)CColumnData::chtHeight);
    idx = pcbHeightMeasure->AddString(_T("Bottom Elevation"));
    pcbHeightMeasure->SetItemData(idx, (DWORD_PTR)CColumnData::chtBottomElevation);
}

void CScallopedPierLayoutDlg::OnHeightMeasureChanged()
{
    CComboBox* pcbHeightMeasure = (CComboBox*)GetDlgItem(IDC_HEIGHT_MEASURE);
    int curSel = pcbHeightMeasure->GetCurSel();
    CColumnData::ColumnHeightMeasurementType measure = (CColumnData::ColumnHeightMeasurementType)(pcbHeightMeasure->GetItemData(curSel));
    m_ColumnLayoutGrid.SetHeightMeasurementType(measure);

    OnPierLayoutChanged();
}

LRESULT CScallopedPierLayoutDlg::OnColumnGridCellChanged(WPARAM wParam, LPARAM lParam)
{

    FillRefColumnComboBox();


    OnPierLayoutChanged();

    return 0;
}

void CScallopedPierLayoutDlg::OnAddColumn()
{
    m_ColumnLayoutGrid.AddColumn();
    FillRefColumnComboBox();


    OnPierLayoutChanged();

}

void CScallopedPierLayoutDlg::OnRemoveColumns()
{
    if (m_ColumnLayoutGrid.GetRowCount() > 1)
    {
        m_ColumnLayoutGrid.RemoveSelectedColumns();

        FillRefColumnComboBox();

        OnPierLayoutChanged();
    }
    else
    {
        AfxMessageBox(_T("The pier must have at least one column"), MB_OK | MB_ICONEXCLAMATION);
    }
}

void CScallopedPierLayoutDlg::SetPierModelType(const pgsTypes::PierModelType& pierModelType)
{
    m_PierModelType = pierModelType;
}

void CScallopedPierLayoutDlg::SetPierData(const CPierData2& pierData)
{
    m_Pier = pierData;
    m_Pier.SetPierLayoutType(pgsTypes::pltScalloped);
}

const CPierData2* CScallopedPierLayoutDlg::GetPierData() const
{
    return &m_Pier;
}

void CScallopedPierLayoutDlg::OnPierLayoutChanged()
{
    ChangePierLayout();
}

void CScallopedPierLayoutDlg::ChangePierLayout()
{
    if (!m_Initialized || m_UpdatingLayout)
        return;
    m_UpdatingLayout = true;
    // UpdateData catches validation failures. The validator only commits a
    // candidate pier after all checks, preserving the last valid preview.
    if (UpdateData(TRUE))
        RefreshDisplay();
    m_UpdatingLayout = false;
}

void CScallopedPierLayoutDlg::OnRefColumnChanged()
{
    ChangePierLayout();
}

void CScallopedPierLayoutDlg::ValidateScallopGeometry(CDataExchange* pDX)
{
    using xbrScallop::Point;
    for (Float64 value : {m_XBeamWidth, m_XBeamHeight[0], m_XBeamHeight[1],
        m_XBeamEndSlopeOffset[0], m_XBeamEndSlopeOffset[1], m_XBeamOverhang[0],
        m_XBeamOverhang[1], m_XBeamOverhangRadius[0], m_XBeamOverhangRadius[1], m_TransverseOffset})
    {
        if (!std::isfinite(value))
        {
            pDX->PrepareCtrl(IDC_W);
            AfxMessageBox(_T("Cross beam dimensions and reference offset must be finite numbers."));
            pDX->Fail();
        }
    }
    // Copy construction intentionally creates a new pier with an invalid ID/index.
    // Assignment preserves the existing pier identity needed by bridge queries.
    CPierData2 candidate;
    candidate = m_Pier;
    m_ColumnLayoutGrid.GetColumnData(candidate);
    const ColumnIndexType count = candidate.GetColumnCount();
    if (m_RefColumnIdx >= count)
    {
        pDX->PrepareCtrl(IDC_REFCOLUMN);
        AfxMessageBox(_T("Select a reference column."));
        pDX->Fail();
    }
    candidate.SetTransverseOffset(m_RefColumnIdx, m_TransverseOffset, m_TransverseOffsetMeasurement);
    candidate.SetXBeamWidth(m_XBeamWidth);
    for (int side = pgsTypes::stLeft; side <= pgsTypes::stRight; ++side)
    {
        const auto st = static_cast<pgsTypes::SideType>(side);
        candidate.SetXBeamDimensions(st, m_XBeamHeight[side], 0.0, 0.0, m_XBeamEndSlopeOffset[side]);
        candidate.SetXBeamOverhang(st, m_XBeamOverhang[side]);
        candidate.SetXBeamOverhangRadius(st, m_XBeamOverhangRadius[side]);
        if (m_XBeamEndSlopeOffset[side] > m_XBeamOverhang[side])
        {
            pDX->PrepareCtrl(side == pgsTypes::stLeft ? IDC_X1L : IDC_X1R);
            AfxMessageBox(_T("The end slope offset cannot exceed its overhang length."));
            pDX->Fail();
        }
    }
    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IBridge, pBridge);
    // The top profile is safe to compute before constructing any bottom arcs.
    CComPtr<IPoint2dCollection> profile;
    pBridge->GetLowerXBeamProfile(candidate, &profile);
    IndexType nPoints;
    profile->get_Count(&nPoints);
    std::vector<Point> top;
    WBFL::Math::PiecewiseFunction topY;
    for (IndexType i = 0; i < nPoints; ++i)
    {
        CComPtr<IPoint2d> point;
        profile->get_Item(i, &point);
        Float64 x, y;
        point->Location(&x, &y);
        top.push_back({x, y});
        topY.AddPoint(x, y);
    }
    if (top.size() < 2)
    {
        pDX->PrepareCtrl(IDC_COLUMN_GRID);
        AfxMessageBox(_T("The lower cross beam must have a nonzero length."));
        pDX->Fail();
    }
    std::vector<Point> ends = {{top.front().x + m_XBeamEndSlopeOffset[pgsTypes::stLeft],
        top.front().y - m_XBeamHeight[pgsTypes::stLeft]}};
    for (ColumnIndexType i = 0; i < count; ++i)
    {
        const Float64 x = pBridge->ConvertCrossBeamToPierCoordinate(candidate, pBridge->GetColumnLocation(candidate, i));
        ends.push_back({x, topY.Evaluate(x) - candidate.GetColumnScallopDepth(i)});
    }
    ends.push_back({top.back().x - m_XBeamEndSlopeOffset[pgsTypes::stRight],
        top.back().y - m_XBeamHeight[pgsTypes::stRight]});
    for (ColumnIndexType arc = 0; arc <= count; ++arc)
    {
        const Float64 radius = arc == 0 ? m_XBeamOverhangRadius[pgsTypes::stLeft]
            : arc == count ? m_XBeamOverhangRadius[pgsTypes::stRight]
            : candidate.GetColumnScallopRadius(arc - 1);
        const auto error = xbrScallop::ValidateArc(ends[arc], ends[arc + 1], radius, top);
        if (error == xbrScallop::ArcError::None)
            continue;
        if (error == xbrScallop::ArcError::AboveTop)
        {
            const ColumnIndexType column = arc == 0 ? 0 : arc == count ? count - 1
                : candidate.GetColumnScallopDepth(arc - 1) <= candidate.GetColumnScallopDepth(arc) ? arc - 1 : arc;
            pDX->PrepareCtrl(IDC_COLUMN_GRID);
            m_ColumnLayoutGrid.SetCurrentCell(column + 1, 8);
        }
        else
        {
            pDX->PrepareCtrl(arc == 0 ? IDC_RL : arc == count ? IDC_RR : IDC_COLUMN_GRID);
            if (arc > 0 && arc < count)
                m_ColumnLayoutGrid.SetCurrentCell(arc, 7);
        }
        CString location, message;
        if (arc == 0) location = _T("Left overhang");
        else if (arc == count) location = _T("Right overhang");
        else location.Format(_T("Bay between columns %u and %u"), arc, arc + 1);
        message.Format(_T("%s: %s"), location.GetString(),
            error == xbrScallop::ArcError::InvalidChord
                ? _T("Radius must be at least half the distance between arc endpoints, including their elevation difference.")
                : error == xbrScallop::ArcError::AboveTop
                    ? _T("Arc extends above the lower cross beam top. Increase the column depth or radius.")
                    : _T("Arc turns back past an endpoint. Increase the radius or adjust the endpoint depths."));
        AfxMessageBox(message);
        pDX->Fail();
    }
    m_Pier = candidate;
}
