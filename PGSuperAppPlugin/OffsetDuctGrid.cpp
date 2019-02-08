///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

// OffsetDuctGrid.cpp : implementation file
//

#include "stdafx.h"
#include "OffsetDuctGrid.h"
#include "OffsetDuctDlg.h"
#include <EAF\EAFDisplayUnits.h>
#include "PGSuperUnits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(COffsetDuctGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// COffsetDuctGrid

COffsetDuctGrid::COffsetDuctGrid()
{
//   RegisterClass();
   m_pCallback = nullptr;
}

COffsetDuctGrid::~COffsetDuctGrid()
{
}

BEGIN_MESSAGE_MAP(COffsetDuctGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(COffsetDuctGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void COffsetDuctGrid::CustomInit(COffsetDuctGridCallback* pCallback)
{
   m_pCallback = pCallback;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
	this->Initialize( );

	this->GetParam( )->EnableUndo(FALSE);

   const int num_rows=0;
   const int num_cols=2;

	this->SetRowCount(num_rows);
	this->SetColCount(num_cols);

   // Turn off selecting whole columns when clicking on a column header
	this->GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   // no row moving
	this->GetParam()->EnableMoveRows(FALSE);

   ROWCOL col = 0;

   // disable left side
	this->SetStyleRange(CGXRange(0,col,num_rows,col), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetValue("")
		);
   col++;

   // set text along top row
   CString strDistance;
   strDistance.Format(_T("Distance\n(%s)"),pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetValue(strDistance)
		);

   CString strOffset;
   strOffset.Format(_T("Offset\n(%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strOffset)
		);

   // don't allow users to resize grids
   this->GetParam( )->EnableTrackColWidth(0); 
   this->GetParam( )->EnableTrackRowHeight(0); 

	this->EnableIntelliMouse();
	this->SetFocus();

   // make it so that text fits correctly in header row
	this->ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));
   //this->ResizeColWidthsToFit(CGXRange(0,0,0,num_cols));
	this->GetParam( )->EnableUndo(TRUE);
}

COffsetDuctGeometry COffsetDuctGrid::GetData()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   COffsetDuctGeometry ductGeometry;
   ROWCOL nRows = GetRowCount();
   for ( ROWCOL row = 1; row <= nRows; row++ )
   {
      Float64 distFromPrev = _tstof(GetCellValue(row,1));
      Float64 offset       = _tstof(GetCellValue(row,2));

      distFromPrev = ::ConvertToSysUnits(distFromPrev,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
      offset       = ::ConvertToSysUnits(offset,      pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

      COffsetDuctGeometry::Point point;
      point.distance = distFromPrev;
      point.offset = offset;

      ductGeometry.Points.push_back(point);
   }

   return ductGeometry;
}

void COffsetDuctGrid::SetData(const COffsetDuctGeometry& ductGeometry)
{
   CollectionIndexType nPoints = ductGeometry.Points.size();
   for (CollectionIndexType idx = 0; idx < nPoints; idx++ )
   {
      COffsetDuctGeometry::Point point = ductGeometry.Points[idx];

      ROWCOL row = ROWCOL(idx+1);
      InsertRows(row,1);
      FillRow(row,point.distance,point.offset);
   }
}

void COffsetDuctGrid::AddPoint()
{
   ROWCOL nRow = GetRowCount()+1;
   GetParam()->EnableUndo(FALSE);

   InsertRows(nRow,1);

   COffsetDuctGeometry::Point point;
   point.distance = ::ConvertToSysUnits(nRow == 1 ? 0.0 : 1.0,unitMeasure::Feet);
   point.offset = ::ConvertToSysUnits(2.0,unitMeasure::Inch);

   COffsetDuctDlg* pParent = (COffsetDuctDlg*)GetParent();
   pParent->m_DuctGeometry.Points.push_back(point);

   FillRow(nRow,point.distance,point.offset);


	Invalidate();
   GetParam()->EnableUndo(TRUE);

   if ( m_pCallback )
      m_pCallback->OnDuctChanged();
}

void COffsetDuctGrid::FillRow(ROWCOL row,Float64 distance,Float64 offset)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CString strDist;
   strDist.Format(_T("%s"),FormatDimension(distance,pDisplayUnits->GetSpanLengthUnit(),false));
   SetValueRange(CGXRange(row,1),strDist);

   CString strOffset;
   strOffset.Format(_T("%s"),FormatDimension(offset,pDisplayUnits->GetComponentDimUnit(),false));
   SetValueRange(CGXRange(row,2),strOffset);
}

CString COffsetDuctGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void COffsetDuctGrid::DeletePoint()
{
	CGXRangeList* pSelList = GetParam()->GetRangeList();
	if (pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1)
	{
		CGXRange range = pSelList->GetHead();
		range.ExpandRange(1, 0, GetRowCount(), 0);
		RemoveRows(range.top, range.bottom);
	}

   COffsetDuctDlg* pParent = (COffsetDuctDlg*)GetParent();
   pParent->EnableDeleteBtn(FALSE);
}

BOOL COffsetDuctGrid::OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   COffsetDuctDlg* pParent = (COffsetDuctDlg*)GetParent();

	//if (GetParam() == nullptr)
	//{
 //     pParent->EnableDeleteBtn(FALSE);
	//	return TRUE;
	//}

 //  if ( nCol == 0 )
 //  {
 //     pParent->EnableDeleteBtn(TRUE);
 //  }
 //  else
 //  {
 //     pParent->EnableDeleteBtn(FALSE);
 //  }

   return TRUE;
}
