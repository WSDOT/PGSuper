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

// SpanLengthGrid.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include "SpanLengthGrid.h"
#include <Units\Measure.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CSpanLengthGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CSpanLengthGrid

CSpanLengthGrid::CSpanLengthGrid()
{
//   RegisterClass();
}

CSpanLengthGrid::~CSpanLengthGrid()
{
}

BEGIN_MESSAGE_MAP(CSpanLengthGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(BridgeDescPierGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSpanLengthGrid message handlers

int CSpanLengthGrid::GetColWidth(ROWCOL nCol)
{
	CRect rect = GetGridRect( );

   switch (nCol)
   {
   case 0:
      return rect.Width()/4;

   case 1:
      return 3*rect.Width()/4;

   default:
      return CGXGridWnd::GetColWidth(nCol);
   }
}

void CSpanLengthGrid::InsertRow()
{
	ROWCOL nRow = 0;
   nRow = GetRowCount()+1;
	nRow = Max((ROWCOL)1, nRow);

	InsertRows(nRow, 1);
   SetRowStyle(nRow);


	ScrollCellInView(nRow+1, GetLeftCol());
}

void CSpanLengthGrid::CustomInit()
{
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );

	GetParam( )->EnableUndo(FALSE);

   const int num_rows = 0;
   const int num_cols = 1;

	SetRowCount(num_rows);
	SetColCount(num_cols);

		// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

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
			.SetValue(_T("Span"))
		);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   const unitLength& um = pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure;
   CString cv;
   cv.Format(_T("Length (%s)"), um.UnitTag().c_str());
	SetStyleRange(CGXRange(0,1), CGXStyle()
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

	GetParam( )->EnableUndo(TRUE);
}

void CSpanLengthGrid::SetRowStyle(ROWCOL nRow)
{
	GetParam()->EnableUndo(FALSE);

   SetStyleRange(CGXRange(nRow,0), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
   );

   SetStyleRange(CGXRange(nRow,1), CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
   );

	GetParam()->EnableUndo(TRUE);
}

CString CSpanLengthGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CSpanLengthGrid::FillGrid(const std::vector<Float64>& vSpanLengths)
{
	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   // remove all but top row
   ROWCOL rows = GetRowCount();
   if (rows>=1)
	   RemoveRows(1, rows);

   IndexType size = vSpanLengths.size();
   if (0 < size)
   {
      // size grid
      for (IndexType i = 0; i < size; i++)
	      InsertRow();

      // fill grid
      ROWCOL nRow=1;
      std::vector<Float64>::const_iterator iter;
      for ( iter = vSpanLengths.begin(); iter != vSpanLengths.end(); iter++ )
      {
         CString strPierLabel = pgsPierLabel::GetPierLabel(nRow-1).c_str();

         // left column header
         SetValueRange(CGXRange(nRow, 0), strPierLabel);

         Float64 L = *iter;
         CString strLength;
         strLength.Format(_T("%s"),FormatDimension(L,pDisplayUnits->GetSpanLengthUnit(),false));
         SetValueRange(CGXRange(nRow,1),strLength);
         nRow++;
      }
   }
   else
   {
	   InsertRow();
   }

   SelectRange(CGXRange(1,1));

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CSpanLengthGrid::GetSpanLengths(std::vector<Float64>& vSpanLengths)
{
   vSpanLengths.clear();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   const unitLength& um = pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure;

   ROWCOL rows = GetRowCount();
   for ( ROWCOL row = 1; row <= rows; row++ )
   {
      Float64 L = _tstof(GetCellValue(row,1));
      L = ::ConvertToSysUnits(L,um);
      vSpanLengths.push_back(L);
   }
}
