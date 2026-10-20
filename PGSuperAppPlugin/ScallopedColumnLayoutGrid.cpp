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
    header.Format(_T("Radius\n(%s)"), units.GetString());
    SetStyleRange(CGXRange(0, 7), CGXStyle()
        .SetWrapText(TRUE).SetEnabled(FALSE)
        .SetHorizontalAlignment(DT_CENTER).SetVerticalAlignment(DT_VCENTER)
        .SetValue(header));
    header.Format(_T("Scallop Depth\n(%s)"), units.GetString());
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

void CScallopedColumnLayoutGrid::SetColumnData(const CPierData2& pier)
{
    CColumnLayoutGrid::SetColumnData(pier);
    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
    const auto unit = pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure;


    ///// Implement


    UpdateLastRowStyles();
}

void CScallopedColumnLayoutGrid::GetColumnData(CPierData2& pier)
{
    ROWCOL nRows = GetRowCount();
    pier.SetColumnCount((ColumnIndexType)nRows);
    for (ROWCOL row = 1; row <= nRows; row++)
    {
        ColumnIndexType colIdx = (ColumnIndexType)(row - 1);
        CColumnData column;
        Float64 S;
        GetColumnData(row, &column, &S);
        pier.SetColumnData(colIdx, column);
        if (row < nRows)
        {
            pier.SetColumnSpacing(colIdx, S);
        }
    }
    const auto& nCols = pier.GetColumnCount();
    ColumnIndexType refColumnIdx;
    Float64 offset;
    pgsTypes::OffsetMeasurementType offsetType;
    pier.GetTransverseOffset(&refColumnIdx, &offset, &offsetType);
    if (refColumnIdx >= nCols)
    {
        pier.SetTransverseOffset(nCols - 1, offset, offsetType);
    }
}

void CScallopedColumnLayoutGrid::GetColumnData(ROWCOL row, CColumnData* pColumn, Float64* pS)
{

//// Implement

}
