///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// LinearDuctGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "LinearDuctGrid.h"
#include "LinearDuctDlg.h"
#include <EAF\EAFDisplayUnits.h>
#include "PGSuperUnits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CLinearDuctGrid, CS_DBLCLKS, 0, 0, 0);

ROWCOL nDistanceCol   = 0;
ROWCOL nOffsetCol     = 0;
ROWCOL nOffsetTypeCol = 0;

/////////////////////////////////////////////////////////////////////////////
// CLinearDuctGrid

CLinearDuctGrid::CLinearDuctGrid()
{
//   RegisterClass();
   m_pCallback = NULL;
}

CLinearDuctGrid::~CLinearDuctGrid()
{
}

BEGIN_MESSAGE_MAP(CLinearDuctGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CLinearDuctGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CLinearDuctGrid::CustomInit(CLinearDuctGridCallback* pCallback)
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
   const int num_cols=3;

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
         .SetValue("Point")
		);
   col++;

   // set text along top row
   nDistanceCol = col++;
   CString strDistance;
   strDistance.Format(_T("Distance\n(%s)"),pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(0,nDistanceCol), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetValue(strDistance)
		);

   nOffsetCol = col++;
   CString strOffset;
   strOffset.Format(_T("Offset\n(%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(0,nOffsetCol), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strOffset)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		);

   nOffsetTypeCol = col++;
	this->SetStyleRange(CGXRange(0,nOffsetTypeCol), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strOffset)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
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

CLinearDuctGeometry CLinearDuctGrid::GetData()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CLinearDuctGeometry ductGeometry;
   ROWCOL nRows = GetRowCount();
   for ( ROWCOL row = 1; row <= nRows; row++ )
   {
      Float64 distFromPrev = _tstof(GetCellValue(row,nDistanceCol));
      Float64 offset       = _tstof(GetCellValue(row,nOffsetCol));
      CLinearDuctGeometry::OffsetType offsetType = CLinearDuctGeometry::OffsetType(_tstoi(GetCellValue(row,nOffsetTypeCol)));

      distFromPrev = ::ConvertToSysUnits(distFromPrev,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
      offset       = ::ConvertToSysUnits(offset,      pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

      ductGeometry.AddPoint(distFromPrev,offset,offsetType);
   }

   return ductGeometry;
}

void CLinearDuctGrid::SetData(const CLinearDuctGeometry& ductGeometry)
{
   CollectionIndexType nPoints = ductGeometry.GetPointCount();
   for (CollectionIndexType idx = 0; idx < nPoints; idx++ )
   {
      ROWCOL row = ROWCOL(idx+1);
      InsertRows(row,1);
      SetRowStyle(row);
      FillRow(row);
   }
}

void CLinearDuctGrid::AddPoint()
{
   ROWCOL nRow = GetRowCount()+1;
   GetParam()->EnableUndo(FALSE);

   InsertRows(nRow,1);
   SetRowStyle(nRow);

   Float64 distFromPrev = ::ConvertToSysUnits(nRow == 1 ? 0.0 : 1.0,unitMeasure::Feet);
   Float64 offset = ::ConvertToSysUnits(2.0,unitMeasure::Inch);

   CLinearDuctDlg* pParent = (CLinearDuctDlg*)GetParent();
   pParent->m_DuctGeometry.AddPoint(distFromPrev,offset,CLinearDuctGeometry::BottomGirder);

   FillRow(nRow);

	Invalidate();
   GetParam()->EnableUndo(TRUE);

   if ( m_pCallback )
      m_pCallback->OnDuctChanged();
}

void CLinearDuctGrid::FillRow(ROWCOL row)
{
   // I dont' like this method as it call back to the parent dialog... SetData and GetData don't do that

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   Float64 distFromPrev;
   Float64 offset;
   CLinearDuctGeometry::OffsetType offsetType;

   CLinearDuctDlg* pParent = (CLinearDuctDlg*)GetParent();
   pParent->m_DuctGeometry.GetPoint(CollectionIndexType(row-1),&distFromPrev,&offset,&offsetType);

   CString strDist;
   strDist.Format(_T("%s"),FormatDimension(distFromPrev,pDisplayUnits->GetSpanLengthUnit(),false));
   SetValueRange(CGXRange(row,nDistanceCol),strDist);

   CString strOffset;
   strOffset.Format(_T("%s"),FormatDimension(offset,pDisplayUnits->GetComponentDimUnit(),false));
   SetValueRange(CGXRange(row,nOffsetCol),strOffset);

   SetValueRange(CGXRange(row,nOffsetTypeCol),(long)offsetType);
}

void CLinearDuctGrid::SetRowStyle(ROWCOL nRow)
{
   SetStyleRange(CGXRange(nRow,nOffsetTypeCol), CGXStyle()
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
   	.SetChoiceList(_T("Bottom Girder\nTop Girder\nTop Slab"))
		.SetValue(0L)
      );
}

CString CLinearDuctGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CLinearDuctGrid::DeletePoint()
{
	CGXRangeList* pSelList = GetParam()->GetRangeList();
	if (pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1)
	{
		CGXRange range = pSelList->GetHead();
		range.ExpandRange(1, 0, GetRowCount(), 0);
		RemoveRows(range.top, range.bottom);
	}

   CLinearDuctDlg* pParent = (CLinearDuctDlg*)GetParent();
   pParent->EnableDeleteBtn(FALSE);
}

BOOL CLinearDuctGrid::OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   CLinearDuctDlg* pParent = (CLinearDuctDlg*)GetParent();

	if (GetParam() == NULL)
	{
      pParent->EnableDeleteBtn(FALSE);
		return TRUE;
	}

   if ( nCol == 0 )
   {
      pParent->EnableDeleteBtn(TRUE);
   }
   else
   {
      pParent->EnableDeleteBtn(FALSE);
   }

   return TRUE;
}
