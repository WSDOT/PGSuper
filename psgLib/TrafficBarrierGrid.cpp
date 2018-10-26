///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// TrafficBarrierGrid.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "TrafficBarrierGrid.h"
#include "TrafficBarrierDlg.h"
#include <system\tokenizer.h>
#include <EAF\EAFApp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CTrafficBarrierGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CTrafficBarrierGrid

CTrafficBarrierGrid::CTrafficBarrierGrid()
{
//   RegisterClass();
}

CTrafficBarrierGrid::~CTrafficBarrierGrid()
{
}

BEGIN_MESSAGE_MAP(CTrafficBarrierGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CTrafficBarrierGrid)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTrafficBarrierGrid message handlers

int CTrafficBarrierGrid::GetColWidth(ROWCOL nCol)
{
	CRect rect = GetGridRect( );

   return rect.Width( )/3;
}

BOOL CTrafficBarrierGrid::OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   SelectRow(nRow);

   CTrafficBarrierDlg* pdlg = (CTrafficBarrierDlg*)GetParent();
   ASSERT (pdlg);

	if (GetParam() == NULL)
	{
      pdlg->EnableMoveUp(FALSE);
      pdlg->EnableMoveDown(FALSE);
		return TRUE;
	}

	CGXRangeList* pSelList = GetParam()->GetRangeList();

   // don't allow row 1 to be deleted unless it's the only row
   int cnt = GetRowCount();
   if (cnt>1)
   {
	   bool res = !pSelList->IsAnyCellFromRow(1) && pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1;
      pdlg->EnableMoveUp(res);
      pdlg->EnableMoveDown(res);
   }
   else if (cnt==1)
   {
	   bool res = pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1;
      pdlg->EnableMoveUp(res);
      pdlg->EnableMoveDown(res);
   }

   return TRUE;
}


void CTrafficBarrierGrid::AppendRow()
{
	ROWCOL nRow = 0;
   nRow = GetRowCount()+1;

	InsertRows(nRow, 1);
   SetRowStyle(nRow);
   SetCurrentCell(nRow, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	Invalidate();
   SelectRow(nRow);
}

void CTrafficBarrierGrid::RemoveRows()
{
	CGXRangeList* pSelList = GetParam()->GetRangeList();
	if (pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1)
	{
		CGXRange range = pSelList->GetHead();
		range.ExpandRange(1, 0, GetRowCount(), 0);
      CGXGridWnd::RemoveRows(range.top, range.bottom);
	}

   CTrafficBarrierDlg* pdlg = (CTrafficBarrierDlg*)GetParent();
   ASSERT (pdlg);
   pdlg->EnableMoveUp(FALSE);
   pdlg->EnableMoveDown(FALSE);
}

void CTrafficBarrierGrid::MoveUp()
{
   CRowColArray selRows;
   ROWCOL nSelRows  = GetSelectedRows(selRows);

   for ( ROWCOL row = 0; row < nSelRows; row++ )
   {
      ROWCOL selRow = selRows[row];
      if ( selRow != 1 )
      {
         SwapRows(selRow,selRow-1);
         SelectRow(selRow-1);
      }
   }
}

void CTrafficBarrierGrid::MoveDown()
{
   CRowColArray selRows;
   ROWCOL nSelRows  = GetSelectedRows(selRows);

   for ( ROWCOL row = 0; row < nSelRows; row++ )
   {
      ROWCOL selRow = selRows[row];

      if ( selRow != GetRowCount() )
      {
         SwapRows(selRow,selRow+1);
         SelectRow(selRow+1);
      }
   }
}

void CTrafficBarrierGrid::CustomInit()
{
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );
	EnableIntelliMouse();

   ResetGrid();
}

void CTrafficBarrierGrid::SetRowStyle(ROWCOL nRow)
{
	GetParam()->EnableUndo(FALSE);

	SetStyleRange(CGXRange(nRow,0), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER));

	SetStyleRange(CGXRange(nRow,1), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT));

	SetStyleRange(CGXRange(nRow,2), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT));

	GetParam()->EnableUndo(TRUE);
}

CString CTrafficBarrierGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
{
    if (IsCurrentCell(nRow, nCol) && IsActiveCurrentCell())
    {
        CString s;
        CGXControl* pControl = GetControl(nRow, nCol);
        pControl->GetValue(s);
        return s;
    }
    else
        return GetValueRowCol(nRow, nCol);
}


// validate input
BOOL CTrafficBarrierGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
{
	return CGXGridWnd::OnValidateCell(nRow, nCol);
}

void CTrafficBarrierGrid::ResetGrid()
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();


   CTrafficBarrierDlg* pdlg = (CTrafficBarrierDlg*)GetParent();
   ASSERT(pdlg);

	GetParam( )->EnableUndo(FALSE);

   if ( GetRowCount() > 0 )
      CGXGridWnd::RemoveRows(1,GetRowCount());

   const int num_rows = 0;
   const int num_cols = 2;

	SetRowCount(num_rows);
	SetColCount(num_cols);

	// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   // set text along top row
	SetStyleRange(CGXRange(0,0), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Point"))
		);

   CString cv;
   cv.Format(_T("X (%s)"),pDisplayUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv.Format(_T("Y (%s)"),pDisplayUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,2), CGXStyle()
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

	SetFocus();

	GetParam( )->EnableUndo(TRUE);
}

void CTrafficBarrierGrid::UploadData(CDataExchange* pDX, IPoint2dCollection* points)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   CComPtr<IEnumPoint2d> enum_points;
   points->get__Enum(&enum_points);

   ROWCOL nRow = 1;
   CComPtr<IPoint2d> point;
   while ( enum_points->Next(1,&point,NULL) != S_FALSE )
   {
      double x,y;
      point->get_X(&x);
      point->get_Y(&y);

      x = ::ConvertFromSysUnits(x,pDisplayUnits->ComponentDim.UnitOfMeasure);
      y = ::ConvertFromSysUnits(y,pDisplayUnits->ComponentDim.UnitOfMeasure);

      AppendRow();
      SetValueRange(CGXRange(nRow,1),x);
      SetValueRange(CGXRange(nRow,2),y);

      nRow++;
      point.Release();
   }

   SelectRow(1);
}

void CTrafficBarrierGrid::DownloadData(CDataExchange* pDX, IPoint2dCollection* points)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   points->Clear();

   ROWCOL nRows = GetRowCount();

   ROWCOL nRow=1;
   for (ROWCOL i = 0; i < nRows; i++)
   {
      double x,y;
      ParseRow(nRow,pDX,&x,&y);

      x = ::ConvertToSysUnits(x,pDisplayUnits->ComponentDim.UnitOfMeasure);
      y = ::ConvertToSysUnits(y,pDisplayUnits->ComponentDim.UnitOfMeasure);

      CComPtr<IPoint2d> p;
      p.CoCreateInstance(CLSID_Point2d);
      p->Move(x,y);
      points->Add(p);

      nRow++;
   }
}

void CTrafficBarrierGrid::ParseRow(ROWCOL nRow, CDataExchange* pDX, double* pX,double* pY)
{
	CString s = GetCellValue(nRow, 1);
   sysTokenizer::ParseDouble(s, pX);

	s = GetCellValue(nRow, 2);
   sysTokenizer::ParseDouble(s, pY);
}

void CTrafficBarrierGrid::OnChangedSelection(const CGXRange* pChangedRect,BOOL bIsDragging, BOOL bKey)
{
   CTrafficBarrierDlg* pParent = (CTrafficBarrierDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CTrafficBarrierDlg) ) );

   pParent->EnableMoveUp(CanEnableMoveUp());
   pParent->EnableMoveDown(CanEnableMoveDown());
}

BOOL CTrafficBarrierGrid::CanEnableMoveUp()
{
   CRowColArray selRows;
   ROWCOL nSelRows  = GetSelectedRows(selRows);
   return (0 < nSelRows && selRows[0] != 1 ? TRUE : FALSE);
}

BOOL CTrafficBarrierGrid::CanEnableMoveDown()
{
   CRowColArray selRows;
   ROWCOL nSelRows  = GetSelectedRows(selRows);
   return (0 < nSelRows && selRows[0] != GetRowCount() ? TRUE : FALSE);
}

void CTrafficBarrierGrid::SwapRows(ROWCOL row1,ROWCOL row2)
{
   CString strX1 = GetCellValue(row1,1);
   CString strY1 = GetCellValue(row1,2);

   CString strX2 = GetCellValue(row2,1);
   CString strY2 = GetCellValue(row2,2);

	SetStyleRange(CGXRange(row1,1), CGXStyle().SetValue(strX2));
	SetStyleRange(CGXRange(row1,2), CGXStyle().SetValue(strY2));

	SetStyleRange(CGXRange(row2,1), CGXStyle().SetValue(strX1));
	SetStyleRange(CGXRange(row2,2), CGXStyle().SetValue(strY1));
}

void CTrafficBarrierGrid::SelectRow(ROWCOL nRow)
{
   SelectRange(CGXRange(0,0,GetRowCount(),3),FALSE); // unselect everything
   SelectRange(CGXRange(nRow,0,nRow,3),TRUE); // select this row
}
