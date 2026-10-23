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
#include "PierPointGrid.h"

#include <IFace/Tools.h>
#include <EAF\EAFDisplayUnits.h>

#include <PGSuperUIUtil.h>


GRID_IMPLEMENT_REGISTER(CPierPointGrid, CS_DBLCLKS, 0, 0, 0);

void CPierPointGrid::DDX_PierPointGrid(CDataExchange* pDX, CPierPointGrid& grid, CPierData2* pPier)
{
    if (pDX->m_bSaveAndValidate)
    {
        grid.GetPierPointData(*pPier);
    }
    else
    {
        grid.SetPierPointData(*pPier);
    }
}

void CPierPointGrid::DDV_PierPointGrid(CDataExchange* pDX, CPierPointGrid& grid)
{
    if (pDX->m_bSaveAndValidate)
    {
        //placeholder
    }
}

/////////////////////////////////////////////////////////////////////////////
// CPierPointGrid

CPierPointGrid::CPierPointGrid()
{
//   RegisterClass();
}

CPierPointGrid::~CPierPointGrid()
{
}

BEGIN_MESSAGE_MAP(CPierPointGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CPierPointGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPierPointGrid message handlers

void CPierPointGrid::CustomInit()
{
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );

	GetParam( )->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   SetMergeCellsMode(gxnMergeDelayEval);

   const int num_rows = 0;
   const int num_cols = 2;

	SetRowCount(num_rows);
	SetColCount(num_cols);

		// Turn off selecting whole pier points when clicking on a pier point header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE & ~GX_SELMULTIPLE & ~GX_SELSHIFT & ~GX_SELKEYBOARD));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);


   
   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

// set text along top row
   int pp = 0;

	SetStyleRange(CGXRange(0,pp++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Node"))
		);

   CString cv;

   cv.Format(_T("X pt."),pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,pp++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv.Format(_T("Y pt."),pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,pp++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);


   // make it so that text fits correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();
	SetFocus();

   GetParam()->SetLockReadOnly(TRUE);
	GetParam( )->EnableUndo(TRUE);
}

void CPierPointGrid::AddPierPoint()
{
    ROWCOL nRows = GetRowCount();

    CPierPointData pierPointData;

    if (0 < nRows)
    {
        GetPierPointData(nRows, &pierPointData); // get pier point data from the last row
    }

    GetParam()->EnableUndo(FALSE);
    GetParam()->SetLockReadOnly(FALSE);

    if (nRows == 0)
    {
        AddPierPoint(pierPointData);
    }
    else
    {
        // set the default spacing in the last row
        SetPierPointData(nRows, pierPointData);
        pierPointData.Set_X(pierPointData.Get_X() + WBFL::Units::ConvertToSysUnits(4.0, WBFL::Units::Measure::Feet));
        // add the new column (adds row to the grid)
        AddPierPoint(pierPointData);
    }

    GetParam()->SetLockReadOnly(TRUE);
    GetParam()->EnableUndo(TRUE);
}


void CPierPointGrid::RemoveSelectedPierPoints()
{
	GetParam( )->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CDWordArray selRows;
   ROWCOL nSelRows = GetSelectedRows(selRows);
   for ( int r = nSelRows-1; r >= 0; r-- )
   {
      ROWCOL selRow = selRows[r];
      RemoveRows(selRow,selRow);
   }

   GetParam()->SetLockReadOnly(TRUE);
	GetParam( )->EnableUndo(TRUE);
}

void CPierPointGrid::GetPierPointData(CPierData2& pier)
{
   ROWCOL nRows = GetRowCount();
   pier.SetPierPointCount((PierPointIndexType)nRows);
   for ( ROWCOL row = 1; row <= nRows; row++ )
   {
      PierPointIndexType ppIdx = (PierPointIndexType)(row-1);
      CPierPointData pierPoint;
      GetPierPointData(row,&pierPoint);
	  pier.SetPierPointData(ppIdx, pierPoint);
   }
}

void CPierPointGrid::SetPierPointData(const CPierData2& pier)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   ROWCOL nRows = GetRowCount();
   if ( 0 < nRows )
   {
      RemoveRows(1,nRows);
   }


   IndexType nPierPoints = pier.GetPierPointCount();
   for ( IndexType ppIdx = 0; ppIdx < nPierPoints; ppIdx++ )
   {
       const CPierPointData& pierPoint = pier.GetPierPointData(ppIdx);
       AddPierPoint(pierPoint);
   }

   ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));

   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(TRUE);
}

void CPierPointGrid::SetPierPointData(ROWCOL row,const CPierPointData& pierPoint)
{
   
   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   ROWCOL col = 1;

   Float64 x = WBFL::Units::ConvertFromSysUnits(pierPoint.Get_X(),pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   SetStyleRange(CGXRange(row,col++), CGXStyle()
      .SetEnabled(TRUE)
      .SetReadOnly(FALSE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(x)
      );

   Float64 y = WBFL::Units::ConvertFromSysUnits(pierPoint.Get_Y(),pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   SetStyleRange(CGXRange(row,col++), CGXStyle()
      .SetEnabled(TRUE)
      .SetReadOnly(FALSE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(y)
      );
}

void CPierPointGrid::AddPierPoint(const CPierPointData& pierPoint)
{
   InsertRows(GetRowCount()+1,1);
   ROWCOL row = GetRowCount();
   SetPierPointData(row, pierPoint);
}

void CPierPointGrid::GetPierPointData(ROWCOL row,CPierPointData* pPierPoint)
{ 
   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   ROWCOL col = 1;

   Float64 X = _tstof(GetCellValue(row,col++));
   X = WBFL::Units::ConvertToSysUnits(X, pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   pPierPoint->Set_X(X);

   Float64 Y = _tstof(GetCellValue(row,col));
   Y = WBFL::Units::ConvertToSysUnits(Y, pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   pPierPoint->Set_Y(Y);
}

CString CPierPointGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
{
    if (IsCurrentCell(nRow, nCol) && IsActiveCurrentCell())
    {
        CString s;
        CGXControl* pControl = GetControl(nRow, nCol);
        pControl->GetValue(s);
        return s;
    }
    else
    {
        return GetValueRowCol(nRow, nCol);
    }
}

void CPierPointGrid::OnClickedButtonRowCol(ROWCOL nRow,ROWCOL nCol)
{
   if ( nCol != 3 )
   {
      return;
   }
}

void CPierPointGrid::OnModifyCell(ROWCOL nRow,ROWCOL nCol)
{

   if ( nCol == 3 )
   {
      GetParam()->EnableUndo(FALSE);
      GetParam()->SetLockReadOnly(FALSE);

      SetStyleRange(CGXRange(nRow,nCol+2),CGXStyle()
        .SetEnabled(FALSE)
        .SetReadOnly(TRUE)
        .SetInterior(::GetSysColor(COLOR_BTNFACE))
        .SetTextColor(::GetSysColor(COLOR_BTNFACE))
        .SetHorizontalAlignment(DT_RIGHT)
        );

      GetParam()->SetLockReadOnly(TRUE);
      GetParam()->EnableUndo(TRUE);
   }
   else
   {
       __super::OnModifyCell(nRow, nCol);

       if (GetParent())
           GetParent()->PostMessage(WM_PIERPOINT_GRID_CELL_CHANGED, (WPARAM)nRow, (LPARAM)nCol);
   }

}
