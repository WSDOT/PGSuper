///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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
#include "ColumnLayoutGrid.h"
#include <EAF\EAFDisplayUnits.h>
#include <PGSuperUIUtil.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CColumnLayoutGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CColumnLayoutGrid

CColumnLayoutGrid::CColumnLayoutGrid()
{
//   RegisterClass();
}

CColumnLayoutGrid::~CColumnLayoutGrid()
{
}

BEGIN_MESSAGE_MAP(CColumnLayoutGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CColumnLayoutGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColumnLayoutGrid message handlers

void CColumnLayoutGrid::CustomInit()
{
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );

	GetParam( )->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   SetMergeCellsMode(gxnMergeDelayEval);

   const int num_rows = 0;
   const int num_cols = 6;

	SetRowCount(num_rows);
	SetColCount(num_cols);

		// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE & ~GX_SELMULTIPLE & ~GX_SELSHIFT & ~GX_SELKEYBOARD));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);


   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

// set text along top row
   int col = 0;

	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Col"))
		);

   CString cv;
   cv.Format(_T("Height\n(%s)"),pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetValue(cv)
		);

	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Transverse\nFixity"))
		);

	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Shape"))
		);

   cv.Format(_T("Diameter\nWidth\n(%s)"),pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv.Format(_T(" \nDepth\n(%s)"),pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv.Format(_T("S\nSpacing\n(%s)"),pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,col++), CGXStyle()
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

void CColumnLayoutGrid::SetHeightMeasurementType(CColumnData::ColumnHeightMeasurementType measure)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CString cv;
   cv.Format(_T("%s\n(%s)"),(measure == CColumnData::chtHeight ? _T("Height") : _T("Bottom\nElev")),pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetValue(cv)
		);
}

void CColumnLayoutGrid::AddColumn()
{
   ROWCOL nRows = GetRowCount();

   CColumnData columnData;
   Float64 S;

   if ( 0 < nRows )
   {
      GetColumnData(nRows,&columnData,&S); // get column data from the last row
   }

   S = ::ConvertToSysUnits(10.0,unitMeasure::Feet); // default spacing
   if ( 1 < nRows )
   {
      // get the spacing between the last two columns
      CColumnData columnData1;
      Float64 S1;
      GetColumnData(nRows-1,&columnData1,&S1);
      S = S1;
   }

	GetParam( )->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   if ( nRows == 0 )
   {
      AddColumn(columnData,0);
   }
   else
   {
      // set the default spacing in the last row
      SetColumnData(nRows,columnData,S);
      // add the new column (adds row to the grid)
      AddColumn(columnData,0);
   }

   GetParam()->SetLockReadOnly(TRUE);
   GetParam( )->EnableUndo(TRUE);
}

void CColumnLayoutGrid::RemoveSelectedColumns()
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

   ROWCOL nRows = GetRowCount();
   if ( 0 < nRows )
   {
      SetStyleRange(CGXRange(nRows,6), CGXStyle()
         .SetEnabled(FALSE)
         .SetReadOnly(TRUE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_BTNFACE))
         .SetHorizontalAlignment(DT_RIGHT)
            );
   }

   GetParam()->SetLockReadOnly(TRUE);
	GetParam( )->EnableUndo(TRUE);
}

void CColumnLayoutGrid::GetColumnData(CPierData2& pier)
{
   ROWCOL nRows = GetRowCount();
   pier.SetColumnCount((ColumnIndexType)nRows);
   for ( ROWCOL row = 1; row <= nRows; row++ )
   {
      ColumnIndexType colIdx = (ColumnIndexType)(row-1);
      CColumnData column;
      Float64 S;
      GetColumnData(row,&column,&S);
      pier.SetColumnData(colIdx,column);
      if ( row < nRows )
      {
         pier.SetColumnSpacing(colIdx,S);
      }
   }
}

void CColumnLayoutGrid::SetColumnData(const CPierData2& pier)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   ROWCOL nRows = GetRowCount();
   if ( 0 < nRows )
   {
      RemoveRows(1,nRows);
   }

   if ( pier.GetColumnCount() == 0 )
   {
      // Always have one column
      CColumnData column;
      AddColumn(column,0);
   }
   else
   {
      IndexType nColumns = pier.GetColumnCount();
      for ( IndexType colIdx = 0; colIdx < nColumns; colIdx++ )
      {
         const CColumnData& column = pier.GetColumnData(colIdx);
         Float64 S = (colIdx != nColumns-1 ? pier.GetColumnSpacing(colIdx) : 0);
         AddColumn(column,S);
      }
   }

   ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));

   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(TRUE);
}

void CColumnLayoutGrid::SetColumnData(ROWCOL row,const CColumnData& column,Float64 S)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   ROWCOL col = 1;

   // Height/Bottom Elevation
   Float64 value = ::ConvertFromSysUnits(column.GetColumnHeight(),pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   SetStyleRange(CGXRange(row,col++), CGXStyle()
      .SetEnabled(TRUE)
      .SetReadOnly(FALSE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(value)
      );

   // Fixity
   CString strChoiceList = CString(CFCB_TBFIX_STR) + CString(_T("\n")) + CString(CFCB_TFIX_BPIN_STR) + CString(_T("\n")) + CString(CFCB_TPIN_BFIX_STR);

   SetStyleRange(CGXRange(row,col++), CGXStyle()
      .SetEnabled(TRUE)
      .SetReadOnly(FALSE)
		.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
		.SetChoiceList(strChoiceList)
      .SetValue( GetTransverseFixityString(column.GetTransverseFixity()) )
      .SetHorizontalAlignment(DT_RIGHT)
      );

   // Section
	SetStyleRange(CGXRange(row,col++), CGXStyle()
		.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
		.SetChoiceList(_T("Circle\nRectangle"))
      .SetValue(column.GetColumnShape() == CColumnData::cstCircle ? _T("Circle") : _T("Rectangle"))
      .SetHorizontalAlignment(DT_RIGHT)
		);

   Float64 D1, D2;
   column.GetColumnDimensions(&D1,&D2);
   value = ::ConvertFromSysUnits(D1,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   SetStyleRange(CGXRange(row,col++),CGXStyle()
      .SetEnabled(TRUE)
      .SetReadOnly(FALSE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(value)
      );

   value = ::ConvertFromSysUnits(D2,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   SetStyleRange(CGXRange(row,col++),CGXStyle()
      .SetEnabled(TRUE)
      .SetReadOnly(FALSE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(value)
      );
   if ( column.GetColumnShape() == CColumnData::cstCircle )
   {
      SetStyleRange(CGXRange(row,col-1),CGXStyle()
         .SetEnabled(FALSE)
         .SetReadOnly(TRUE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_BTNFACE))
         .SetHorizontalAlignment(DT_RIGHT)
         );
   }


   // Spacing
   // Set the value for the spacing to the next column and disable this cell
   // This assumes this column is in the last row and spacing to next isn't applicable
   value = ::ConvertFromSysUnits(S,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   SetStyleRange(CGXRange(row,col), CGXStyle()
      .SetEnabled(FALSE)
      .SetReadOnly(TRUE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetTextColor(::GetSysColor(COLOR_BTNFACE))
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(value)
         );

   if ( 1 < row )
   {
      // if this isn't the first row, enable the spacing in the previous row
      // because it obviously isn't the last row
      SetStyleRange(CGXRange(row-1,col),CGXStyle()
         .SetEnabled(TRUE)
         .SetReadOnly(FALSE)
         .SetInterior(::GetSysColor(COLOR_WINDOW))
        .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         );
   }

   col++;
}

void CColumnLayoutGrid::AddColumn(const CColumnData& column,Float64 S)
{
   InsertRows(GetRowCount()+1,1);
   ROWCOL row = GetRowCount();
   SetColumnData(row,column,S);
}

void CColumnLayoutGrid::GetColumnData(ROWCOL row,CColumnData* pColumn,Float64* pS)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   ROWCOL col = 1;

   Float64 H = _tstof(GetCellValue(row,col++));
   H = ::ConvertToSysUnits(H,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   pColumn->SetColumnHeight(H);

   // Fixity
   CString strFixity = GetCellValue(row,col++);
   pgsTypes::ColumnTransverseFixityType fixity = GetTransverseFixityTypeFromString(strFixity);
   pColumn->SetTransverseFixity(fixity);

   // Shape Details
   CString strShape = GetCellValue(row,col++);
   if ( strShape == _T("Circle") )
   {
      pColumn->SetColumnShape(CColumnData::cstCircle);
   }
   else
   {
      pColumn->SetColumnShape(CColumnData::cstRectangle);
   }

   Float64 D1 = _tstof(GetCellValue(row,col++));
   D1 = ::ConvertToSysUnits(D1,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   Float64 D2 = _tstof(GetCellValue(row,col++));
   D2 = ::ConvertToSysUnits(D2,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   pColumn->SetColumnDimensions(D1,D2);
   
   // Spacing
   Float64 S = _tstof(GetCellValue(row,col++));
   *pS = ::ConvertToSysUnits(S,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
}

CString CColumnLayoutGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CColumnLayoutGrid::OnClickedButtonRowCol(ROWCOL nRow,ROWCOL nCol)
{
   if ( nCol != 3 )
   {
      return;
   }
}

void CColumnLayoutGrid::OnModifyCell(ROWCOL nRow,ROWCOL nCol)
{
   if ( nCol == 3 )
   {
      GetParam()->EnableUndo(FALSE);
      GetParam()->SetLockReadOnly(FALSE);

      CString str = GetCellValue(nRow,nCol);
      if ( str == _T("Rectangle") )
      {
         SetStyleRange(CGXRange(nRow,nCol+2),CGXStyle()
            .SetEnabled(TRUE)
            .SetReadOnly(FALSE)
            .SetInterior(::GetSysColor(COLOR_WINDOW))
            .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
            .SetHorizontalAlignment(DT_RIGHT)
            );
      }
      else
      {
         SetStyleRange(CGXRange(nRow,nCol+2),CGXStyle()
            .SetEnabled(FALSE)
            .SetReadOnly(TRUE)
            .SetInterior(::GetSysColor(COLOR_BTNFACE))
            .SetTextColor(::GetSysColor(COLOR_BTNFACE))
            .SetHorizontalAlignment(DT_RIGHT)
            );
      }

      GetParam()->SetLockReadOnly(TRUE);
      GetParam()->EnableUndo(TRUE);
   }
   else
   {
      __super::OnModifyCell(nRow, nCol);
   }
}
