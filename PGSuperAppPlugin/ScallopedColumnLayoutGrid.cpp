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

#include "stdafx.h"
#include "ScallopedColumnLayoutGrid.h"
#include <cmath>

#include <IFace/Tools.h>
#include <EAF\EAFDisplayUnits.h>

#include <PGSuperUIUtil.h>


GRID_IMPLEMENT_REGISTER(CScallopedColumnLayoutGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CColumnLayoutGrid

CScallopedColumnLayoutGrid::CScallopedColumnLayoutGrid()
{
//   RegisterClass();
}

CScallopedColumnLayoutGrid::~CScallopedColumnLayoutGrid()
{
}

void CScallopedColumnLayoutGrid::CustomInit()
{
    CColumnLayoutGrid::CustomInit();
    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
    CString units = pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str();
    CString header;
    header.Format(_T("R\nBay Radius\n(%s)"), units.GetString());
    SetStyleRange(CGXRange(0, 7), CGXStyle()
        .SetWrapText(TRUE).SetEnabled(FALSE)
        .SetHorizontalAlignment(DT_CENTER).SetVerticalAlignment(DT_VCENTER)
        .SetValue(header));
    header.Format(_T("D\nScallop Depth\n(%s)"), units.GetString());
    SetStyleRange(CGXRange(0, 8), CGXStyle()
        .SetWrapText(TRUE).SetEnabled(FALSE)
        .SetHorizontalAlignment(DT_CENTER).SetVerticalAlignment(DT_VCENTER)
        .SetValue(header));
    ResizeRowHeightsToFit(CGXRange(0, 0, 0, GetColCount()));
}

void CScallopedColumnLayoutGrid::UpdateLastRowStyles()
{
    CColumnLayoutGrid::UpdateLastRowStyles();
    const ROWCOL nRows = GetRowCount();
    if (nRows > 0)
    {
        // Only radius is interval-based. Depth remains editable in every row.
        SetStyleRange(CGXRange(nRows, 7), CGXStyle()
            .SetEnabled(FALSE).SetReadOnly(TRUE)
            .SetInterior(::GetSysColor(COLOR_BTNFACE))
            .SetTextColor(::GetSysColor(COLOR_BTNFACE))
            .SetHorizontalAlignment(DT_RIGHT));
        SetStyleRange(CGXRange(nRows, 8), CGXStyle()
            .SetEnabled(TRUE).SetReadOnly(FALSE)
            .SetInterior(::GetSysColor(COLOR_WINDOW))
            .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
            .SetHorizontalAlignment(DT_RIGHT));
    }
}

void CScallopedColumnLayoutGrid::SetGeometryCell(ROWCOL row, ROWCOL col, Float64 value)
{
    SetStyleRange(CGXRange(row, col), CGXStyle()
        .SetEnabled(TRUE).SetReadOnly(FALSE)
        .SetInterior(::GetSysColor(COLOR_WINDOW))
        .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
        .SetHorizontalAlignment(DT_RIGHT).SetValue(value));
}

// Requires the scallop geometry accessors declared in PierData2.h.
// Scallop depth is indexed by column; radius is indexed by interval.
void CScallopedColumnLayoutGrid::SetColumnData(const CPierData2& pier)
{
    CColumnLayoutGrid::SetColumnData(pier);
    GetParam()->EnableUndo(FALSE);
    GetParam()->SetLockReadOnly(FALSE);

    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
    const auto unit = pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure;
    const ROWCOL nRows = GetRowCount();

    for (ROWCOL row = 1; row <= nRows; ++row)
    {
        const ColumnIndexType idx = static_cast<ColumnIndexType>(row - 1);
        const Float64 depth = WBFL::Units::ConvertFromSysUnits(
            pier.GetColumnScallopDepth(idx), unit);
        SetGeometryCell(row, 8, depth);

        if (row < nRows)
        {
            const Float64 radius = WBFL::Units::ConvertFromSysUnits(
                pier.GetColumnScallopRadius(idx), unit);
            SetGeometryCell(row, 7, radius);
        }
    }

    UpdateLastRowStyles();
    GetParam()->SetLockReadOnly(TRUE);
    GetParam()->EnableUndo(TRUE);
}

void CScallopedColumnLayoutGrid::GetColumnData(CPierData2& pier)
{
    // Preserve the base class handling of column properties, spacing,
    // and the transverse reference-column adjustment.
    CColumnLayoutGrid::GetColumnData(pier);

    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
    const auto unit = pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure;
    const ROWCOL nRows = GetRowCount();

    for (ROWCOL row = 1; row <= nRows; ++row)
    {
        const ColumnIndexType idx = static_cast<ColumnIndexType>(row - 1);
        const Float64 depth = WBFL::Units::ConvertToSysUnits(
            _tstof(GetCellValue(row, 8)), unit);
        pier.SetColumnScallopDepth(idx, depth);

        if (row < nRows)
        {
            const Float64 radius = WBFL::Units::ConvertToSysUnits(
                _tstof(GetCellValue(row, 7)), unit);
            pier.SetColumnScallopRadius(idx, radius);
        }
    }
}

void CScallopedColumnLayoutGrid::AddColumn()
{
    const ROWCOL oldRows = GetRowCount();
    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
    const auto unit = pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure;

    // Capture displayed values before the base grid inserts the new row.
    // The last row has no bay radius; use the last existing bay instead.
    const Float64 depth = oldRows > 0 ? _tstof(GetCellValue(oldRows, 8))
        : WBFL::Units::ConvertFromSysUnits(
            WBFL::Units::ConvertToSysUnits(5.0, WBFL::Units::Measure::Feet), unit);
    const Float64 radius = oldRows > 1 ? _tstof(GetCellValue(oldRows - 1, 7))
        : WBFL::Units::ConvertFromSysUnits(
            WBFL::Units::ConvertToSysUnits(10.0, WBFL::Units::Measure::Feet), unit);

    CColumnLayoutGrid::AddColumn();

    GetParam()->EnableUndo(FALSE);
    GetParam()->SetLockReadOnly(FALSE);

    if (oldRows > 0)
        SetGeometryCell(oldRows, 7, radius);
    SetGeometryCell(oldRows + 1, 8, depth);

    // Disable radius on the new last row.
    UpdateLastRowStyles();

    GetParam()->SetLockReadOnly(TRUE);
    GetParam()->EnableUndo(TRUE);
}


void CScallopedColumnLayoutGrid::DDV_ColumnGrid(CDataExchange* pDX, CScallopedColumnLayoutGrid& grid)
{
    CColumnLayoutGrid::DDV_ColumnGrid(pDX, grid);
    if (!pDX->m_bSaveAndValidate)
        return;
    for (ROWCOL row = 1; row <= grid.GetRowCount(); ++row)
    {
        for (ROWCOL col : {ROWCOL(6), ROWCOL(7), ROWCOL(8)})
        {
            if (row == grid.GetRowCount() && col != 8)
                continue; // No spacing or bay radius on the last row.
            CString text = grid.GetCellValue(row, col);
            text.Trim();
            TCHAR* end = nullptr;
            const Float64 value = _tcstod(text.GetString(), &end);
            if (text.IsEmpty() || end == text.GetString() || *end != 0 ||
                !std::isfinite(value) || value <= 0.0)
            {
                pDX->PrepareCtrl(grid.GetDlgCtrlID());
                grid.SetCurrentCell(row, col);
                CString message;
                message.Format(_T("Column %u: %s must be a positive, finite number."), row,
                    col == 6 ? _T("spacing") : col == 7 ? _T("bay radius") : _T("scallop depth"));
                AfxMessageBox(message);
                pDX->Fail();
            }
        }
    }
}

BOOL CScallopedColumnLayoutGrid::OnEndEditing(ROWCOL nRow, ROWCOL nCol)
{
    CGXControl* control = GetControl(nRow, nCol);
    const BOOL modified = control && control->GetModify();
    const BOOL accepted = CColumnLayoutGrid::OnEndEditing(nRow, nCol);
    if (accepted && modified && GetParent())
    {
        // The grid has not finished storing/resetting the edit control yet.
        // Defer validation and preview updates until this callback unwinds.
        GetParent()->PostMessage(WM_COLUMN_GRID_CELL_CHANGED, (WPARAM)nRow, (LPARAM)nCol);
    }
    return accepted;
}
