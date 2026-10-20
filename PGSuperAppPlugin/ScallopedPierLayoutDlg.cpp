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

    ON_EN_CHANGE(IDC_W, &CScallopedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_H1L, &CScallopedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_H1R, &CScallopedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_RL, &CScallopedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_RR, &CScallopedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_X1L, &CScallopedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_X1R, &CScallopedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_OHL, &CScallopedPierLayoutDlg::OnPierLayoutChanged)
    ON_EN_CHANGE(IDC_OHR, &CScallopedPierLayoutDlg::OnPierLayoutChanged)

    ON_CBN_SELCHANGE(IDC_REFCOLUMN, &CScallopedPierLayoutDlg::OnRefColumnChanged)
    ON_EN_CHANGE(IDC_REFCOLUMN_OFFSET, &CScallopedPierLayoutDlg::OnRefColumnChanged)
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

    OnHeightMeasureChanged();


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

    CColumnLayoutGrid::DDV_ColumnGrid(pDX, m_ColumnLayoutGrid);
    CColumnLayoutGrid::DDX_ColumnGrid(pDX, m_ColumnLayoutGrid, &m_Pier);

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

        DDV_UnitValueZeroOrMore(pDX, IDC_RL, m_XBeamOverhangRadius[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueZeroOrMore(pDX, IDC_RR, m_XBeamOverhangRadius[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

        for (SpacingIndexType spaIdx = 0;
            spaIdx < m_Pier.GetColumnCount() - 1;
            ++spaIdx)
        {
            const Float64 spacing = m_Pier.GetColumnSpacing(spaIdx);

            // Chord length must be less than the diameter.
            if (spacing >= 2.0 * m_XBeamOverhangRadius[pgsTypes::stLeft])
            {

                pDX->PrepareCtrl(IDC_RL);

                CString msg;
                msg.Format(
                    _T("R is too small for the spacing between columns %d and %d. ")
                    _T("R must be more than one-half of the column spacing."),
                    spaIdx + 1,
                    spaIdx + 2);

                AfxMessageBox(msg);
                pDX->Fail();
            }

            // Arc height (sagitta) must not exceed D.
            const Float64 halfChord = spacing / 2.0;
            const Float64 arcHeight =
                m_XBeamOverhangRadius[pgsTypes::stLeft] -
                sqrt(m_XBeamOverhangRadius[pgsTypes::stLeft] * m_XBeamOverhangRadius[pgsTypes::stLeft] -
                    halfChord * halfChord);

            if (arcHeight > 1.33333)
            {

                //pDX->PrepareCtrl(IDC_D);

                //CString msg;
                //msg.Format(
                //    _T("D is too small for the arc between columns %d and %d. ")
                //    _T("D must be at least the height of the arc."),
                //    spaIdx + 1,
                //    spaIdx + 2);

                //AfxMessageBox(msg);
                //pDX->Fail();
            }
        }

        // Validate left and right overhang arcs
        for (int side = pgsTypes::stLeft;
            side <= pgsTypes::stRight; ++side)
        {
            const Float64 run = m_XBeamOverhang[side];
            const Float64 dy =
                1.33333 - m_XBeamHeight[side];

            const Float64 R = m_XBeamOverhangRadius[side];
            const Float64 chord = sqrt(run * run + dy * dy);

            const CString sideName =
                side == pgsTypes::stLeft ? _T("left") : _T("right");

            // The chord cannot exceed the circle diameter.
            if (chord >= 2.0 * R)
            {
                pDX->PrepareCtrl(IDC_RL);

                CString msg;
                msg.Format(
                    _T("R is too small for the %s overhang arc. ")
                    _T("R must be more than one-half of the ")
                    _T("distance between the arc endpoints."),
                    sideName.GetString());

                AfxMessageBox(msg);
                pDX->Fail();
            }

            if (IsZero(chord))
                continue;

            const Float64 halfChord = chord / 2.0;

            const Float64 centerOffset =
                sqrt(R * R - halfChord * halfChord);

            // Circle center relative to the column endpoint.
            // The column endpoint is (0, 0).
            // The overhang endpoint is (run, dy).
            const Float64 centerX =
                0.5 * run + centerOffset * dy / chord;

            const Float64 centerY =
                0.5 * dy - centerOffset * run / chord;

            // Highest point of the complete circle.
            const Float64 peakX = centerX;
            const Float64 peakY = centerY + R;

            // Start with the higher endpoint.
            Float64 maxY = Max(0.0, dy);

            // Check whether the circle's highest point
            // lies on the actual upper minor arc.
            const Float64 startAngle =
                atan2(-centerY, -centerX);

            const Float64 endAngle =
                atan2(dy - centerY, run - centerX);

            // Normalize angles to [0, 2*pi).
            const Float64 twoPi = 2.0 * acos(-1.0);

            Float64 a0 = startAngle;
            Float64 a1 = endAngle;

            if (a0 < 0.0)
                a0 += twoPi;

            if (a1 < 0.0)
                a1 += twoPi;

            // Counterclockwise sweep from start to end.
            Float64 sweep = a1 - a0;

            if (sweep < 0.0)
                sweep += twoPi;

            // Select the minor arc.
            if (sweep > acos(-1.0))
            {
                std::swap(a0, a1);
                sweep = twoPi - sweep;
            }

            // The highest point is at pi/2.
            Float64 peakAngle = acos(-1.0) / 2.0;

            Float64 relativeAngle = peakAngle - a0;

            if (relativeAngle < 0.0)
                relativeAngle += twoPi;

            if (relativeAngle <= sweep)
                maxY = Max(maxY, peakY);

            // The top of the arc cannot exceed
            // the top of the lower crossbeam.
            if (maxY > 1.33333)
            {
                //pDX->PrepareCtrl(IDC_D);

                //CString msg;
                //msg.Format(
                //    _T("D is too small for the %s overhang arc. ")
                //    _T("The top of the arc cannot extend above ")
                //    _T("the top of the cross beam."),
                //    sideName.GetString());

                //AfxMessageBox(msg);
                //pDX->Fail();
            }
        }

        // Overhangs must satisfy the first/last column radius limits.
        Float64 D1 = 0.0, D2 = 0.0;
        ATLASSERT(1 <= m_Pier.GetColumnCount());


        Float64 spacingSum = 0.0;
        for (SpacingIndexType spaIdx = 0; spaIdx < m_Pier.GetColumnCount() - 1; spaIdx++)
        {
            spacingSum += m_Pier.GetColumnSpacing(spaIdx);
        }
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

    // Update pier data with current column data
    m_ColumnLayoutGrid.GetColumnData(m_Pier);

    OnPierLayoutChanged();

    return 0;
}

void CScallopedPierLayoutDlg::OnAddColumn()
{
    m_ColumnLayoutGrid.AddColumn();
    FillRefColumnComboBox();

    // Update pier data with current column data
    m_ColumnLayoutGrid.GetColumnData(m_Pier);

    OnPierLayoutChanged();

}

void CScallopedPierLayoutDlg::OnRemoveColumns()
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
    // Get the current values from the edit controls into member variables
    CDataExchange dx(this, TRUE);

    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

    CPierLayoutPage* pPage =
        DYNAMIC_DOWNCAST(CPierLayoutPage, GetParent());

    // Exchange XBeam dimensions
    DDX_UnitValueAndTag(&dx, IDC_H1L, IDC_H1L_UNIT, m_XBeamHeight[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(&dx, IDC_H1R, IDC_H1R_UNIT, m_XBeamHeight[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(&dx, IDC_X1L, IDC_X1L_UNIT, m_XBeamEndSlopeOffset[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(&dx, IDC_X1R, IDC_X1R_UNIT, m_XBeamEndSlopeOffset[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(&dx, IDC_OHL, IDC_OHL_UNIT, m_XBeamOverhang[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(&dx, IDC_OHR, IDC_OHR_UNIT, m_XBeamOverhang[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(&dx, IDC_RL, IDC_RL_UNIT, m_XBeamOverhangRadius[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
    DDX_UnitValueAndTag(&dx, IDC_RR, IDC_RR_UNIT, m_XBeamOverhangRadius[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

    DDX_UnitValueAndTag(&dx, IDC_W, IDC_W_UNIT, m_XBeamWidth, pDisplayUnits->GetSpanLengthUnit());

    // Update the pier data with the current values
    m_Pier.SetXBeamDimensions(pgsTypes::stLeft, m_XBeamHeight[pgsTypes::stLeft], 0.0, 0.0, m_XBeamEndSlopeOffset[pgsTypes::stLeft]);
    m_Pier.SetXBeamDimensions(pgsTypes::stRight, m_XBeamHeight[pgsTypes::stRight], 0.0, 0.0, m_XBeamEndSlopeOffset[pgsTypes::stRight]);

    m_Pier.SetXBeamWidth(m_XBeamWidth);

    m_Pier.SetXBeamOverhang(pgsTypes::stLeft, m_XBeamOverhang[pgsTypes::stLeft]);
    m_Pier.SetXBeamOverhang(pgsTypes::stRight, m_XBeamOverhang[pgsTypes::stRight]);

    m_Pier.SetXBeamOverhangRadius(pgsTypes::stLeft, m_XBeamOverhangRadius[pgsTypes::stLeft]);
    m_Pier.SetXBeamOverhangRadius(pgsTypes::stRight, m_XBeamOverhangRadius[pgsTypes::stRight]);

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
        DDV_UnitValueZeroOrMore(&dx, IDC_OHL, m_XBeamOverhang[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueZeroOrMore(&dx, IDC_OHR, m_XBeamOverhang[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueZeroOrMore(&dx, IDC_RL, m_XBeamOverhangRadius[pgsTypes::stLeft], pDisplayUnits->GetSpanLengthUnit());
        DDV_UnitValueZeroOrMore(&dx, IDC_RR, m_XBeamOverhangRadius[pgsTypes::stRight], pDisplayUnits->GetSpanLengthUnit());

        for (SpacingIndexType spaIdx = 0;
            spaIdx < m_Pier.GetColumnCount() - 1;
            ++spaIdx)
        {
            const Float64 spacing = m_Pier.GetColumnSpacing(spaIdx);

            // Chord length must be less than the diameter.
            if (spacing >= 2.0 * m_XBeamOverhangRadius[pgsTypes::stLeft])
            {

                dx.PrepareCtrl(IDC_RL);

                CString msg;
                msg.Format(
                    _T("R is too small for the spacing between columns %d and %d. ")
                    _T("R must be more than one-half of the column spacing."),
                    spaIdx + 1,
                    spaIdx + 2);

                AfxMessageBox(msg);
                dx.Fail();
            }

            // Arc height (sagitta) must not exceed D.
            const Float64 halfChord = spacing / 2.0;
            const Float64 arcHeight =
                m_XBeamOverhangRadius[pgsTypes::stLeft] -
                sqrt(m_XBeamOverhangRadius[pgsTypes::stLeft] * m_XBeamOverhangRadius[pgsTypes::stLeft] -
                    halfChord * halfChord);

            if (arcHeight > 1.33333)
            {

                //dx.PrepareCtrl(IDC_D);

                //CString msg;
                //msg.Format(
                //    _T("D is too small for the arc between columns %d and %d. ")
                //    _T("D must be at least the height of the arc."),
                //    spaIdx + 1,
                //    spaIdx + 2);

                //AfxMessageBox(msg);
                //dx.Fail();
            }
        }

        // Validate left and right overhang arcs
        for (int side = pgsTypes::stLeft;
            side <= pgsTypes::stRight; ++side)
        {
            const Float64 run = m_XBeamOverhang[side];
            const Float64 dy =
                1.3333 - m_XBeamHeight[side];

            const Float64 R = m_XBeamOverhangRadius[side];
            const Float64 chord = sqrt(run * run + dy * dy);

            const CString sideName =
                side == pgsTypes::stLeft ? _T("left") : _T("right");

            // The chord cannot exceed the circle diameter.
            if (chord >= 2.0 * R)
            {
                dx.PrepareCtrl(IDC_RL);

                CString msg;
                msg.Format(
                    _T("R is too small for the %s overhang arc. ")
                    _T("R must be more than one-half of the ")
                    _T("distance between the arc endpoints."),
                    sideName.GetString());

                AfxMessageBox(msg);
                dx.Fail();
            }

            if (IsZero(chord))
                continue;

            const Float64 halfChord = chord / 2.0;

            const Float64 centerOffset =
                sqrt(R * R - halfChord * halfChord);

            // Circle center relative to the column endpoint.
            // The column endpoint is (0, 0).
            // The overhang endpoint is (run, dy).
            const Float64 centerX =
                0.5 * run + centerOffset * dy / chord;

            const Float64 centerY =
                0.5 * dy - centerOffset * run / chord;

            // Highest point of the complete circle.
            const Float64 peakX = centerX;
            const Float64 peakY = centerY + R;

            // Start with the higher endpoint.
            Float64 maxY = Max(0.0, dy);

            // Check whether the circle's highest point
            // lies on the actual upper minor arc.
            const Float64 startAngle =
                atan2(-centerY, -centerX);

            const Float64 endAngle =
                atan2(dy - centerY, run - centerX);

            // Normalize angles to [0, 2*pi).
            const Float64 twoPi = 2.0 * acos(-1.0);

            Float64 a0 = startAngle;
            Float64 a1 = endAngle;

            if (a0 < 0.0)
                a0 += twoPi;

            if (a1 < 0.0)
                a1 += twoPi;

            // Counterclockwise sweep from start to end.
            Float64 sweep = a1 - a0;

            if (sweep < 0.0)
                sweep += twoPi;

            // Select the minor arc.
            if (sweep > acos(-1.0))
            {
                std::swap(a0, a1);
                sweep = twoPi - sweep;
            }

            // The highest point is at pi/2.
            Float64 peakAngle = acos(-1.0) / 2.0;

            Float64 relativeAngle = peakAngle - a0;

            if (relativeAngle < 0.0)
                relativeAngle += twoPi;

            if (relativeAngle <= sweep)
                maxY = Max(maxY, peakY);

            // The top of the arc cannot exceed
            // the top of the lower crossbeam.
            if (maxY > 1.33333)
            {
                //dx.PrepareCtrl(IDC_D);

                //CString msg;
                //msg.Format(
                //    _T("D is too small for the %s overhang arc. ")
                //    _T("The top of the arc cannot extend above ")
                //    _T("the top of the cross beam."),
                //    sideName.GetString());

                //AfxMessageBox(msg);
                //dx.Fail();
            }
        }

        // Overhangs must satisfy the first/last column radius limits.
        Float64 D1 = 0.0, D2 = 0.0;


        Float64 spacingSum = 0.0;
        for (SpacingIndexType spaIdx = 0; spaIdx < m_Pier.GetColumnCount() - 1; spaIdx++)
        {
            spacingSum += m_Pier.GetColumnSpacing(spaIdx);
        }
    }

    RefreshDisplay();
}

void CScallopedPierLayoutDlg::OnRefColumnChanged()
{
    // Get the current values from the edit controls into member variables
    CDataExchange dx(this, TRUE);

    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

    DDX_CBIndex(&dx, IDC_REFCOLUMN, m_RefColumnIdx);
    DDX_OffsetAndTag(&dx, IDC_REFCOLUMN_OFFSET, IDC_REFCOLUMN_OFFSET_UNIT, m_TransverseOffset, pDisplayUnits->GetSpanLengthUnit());
    DDX_CBItemData(&dx, IDC_REFCOLUMN_MEASUREMENT, m_TransverseOffsetMeasurement);

	m_Pier.SetTransverseOffset(m_RefColumnIdx, m_TransverseOffset, m_TransverseOffsetMeasurement);

    RefreshDisplay();
}
