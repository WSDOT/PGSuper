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

// LinearDuctGrid.cpp : implementation file
//

#include "stdafx.h"
#include "LinearDuctGrid.h"
#include "LinearDuctDlg.h"
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include "PGSuperUnits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CLinearDuctGrid, CS_DBLCLKS, 0, 0, 0);

ROWCOL nLocationCol   = 0;
ROWCOL nUnitCol       = 0;
ROWCOL nOffsetCol     = 0;
ROWCOL nOffsetTypeCol = 0;

/////////////////////////////////////////////////////////////////////////////
// CLinearDuctGrid

CLinearDuctGrid::CLinearDuctGrid()
{
//   RegisterClass();
   m_pCallback = nullptr;
}

CLinearDuctGrid::~CLinearDuctGrid()
{
}

BEGIN_MESSAGE_MAP(CLinearDuctGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CLinearDuctGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CLinearDuctGrid::GetColWidth(ROWCOL nCol)
{
	CRect rect = GetGridRect( );
   if ( 0 <= nCol && nCol <= 2 )
   {
      return rect.Width()/6;
   }
   else
   {
      return rect.Width()/4;
   }
   //return CGXGridWnd::GetColWidth(nCol);
}

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
   const int num_cols=4;

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
   nLocationCol = col++;
   nUnitCol     = col++;
	this->SetStyleRange(CGXRange(0,nLocationCol,0,nUnitCol), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetValue(_T("Location"))
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		);

   nOffsetCol     = col++;
   nOffsetTypeCol = col++;
   CString strOffset;
   strOffset.Format(_T("Offset\n(%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(0,nOffsetCol,0,nOffsetTypeCol), CGXStyle()
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
	this->GetParam( )->EnableUndo(TRUE);
}

void CLinearDuctGrid::GetData(CLinearDuctGeometry& ductGeometry)
{
   ductGeometry.Clear(); // make sure there aren't any points in the geometry object

   ROWCOL nRows = GetRowCount();
   for ( ROWCOL row = 1; row <= nRows; row++ )
   {
      Float64 location,offset;
      CLinearDuctGeometry::OffsetType offsetType;
      GetPoint(row,&location,&offset,&offsetType);
      ductGeometry.AddPoint(location,offset,offsetType);
   }
}

void CLinearDuctGrid::SetData(const CLinearDuctGeometry& ductGeometry)
{
   // remove existing rows (clears the grid)
   ROWCOL nRows = GetRowCount();
   if ( 0 < nRows )
   {
	   RemoveRows(1,nRows);
   }

   CollectionIndexType nPoints = ductGeometry.GetPointCount();
   InsertRows(1,(ROWCOL)nPoints);
   for (CollectionIndexType idx = 0; idx < nPoints; idx++ )
   {
      ROWCOL row = ROWCOL(idx+1);
      SetRowStyle(row);

      Float64 location;
      Float64 offset;
      CLinearDuctGeometry::OffsetType offsetType;
      ductGeometry.GetPoint(idx,&location,&offset,&offsetType);
      FillRow(row,location,offset,offsetType);
   }
}

void CLinearDuctGrid::AddPoint()
{
   ROWCOL nRow = GetRowCount();
   GetParam()->EnableUndo(FALSE);

   InsertRows(nRow,1);
   
   SetRowStyle(nRow);

   Float64 location = ::ConvertToSysUnits(1.0,unitMeasure::Feet);
   Float64 offset   = ::ConvertToSysUnits(2.0,unitMeasure::Inch);

   FillRow(nRow,location,offset,CLinearDuctGeometry::BottomGirder);

   CLinearDuctDlg* pParent = (CLinearDuctDlg*)GetParent();
   if ( pParent->GetMeasurementType() == CLinearDuctGeometry::FromPrevious )
   {
      // need to adjust last point so it isn't past the end of the girder
      CLinearDuctGeometry::OffsetType offsetType;
      Float64 distance;
      GetPoint(GetRowCount(),&distance,&offset,&offsetType);
      distance -= location;
      FillRow(GetRowCount(),distance,offset,offsetType);
   }


	Invalidate();
   GetParam()->EnableUndo(TRUE);

   if ( m_pCallback )
   {
      m_pCallback->OnDuctChanged();
   }
}

void CLinearDuctGrid::DeletePoint()
{
#pragma Reminder("UPDATE: can't delete last row")

	CGXRangeList* pSelList = GetParam()->GetRangeList();
	if (pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1)
	{
		CGXRange range = pSelList->GetHead();
		range.ExpandRange(1, 0, GetRowCount(), 0);
		RemoveRows(range.top, range.bottom);
	}

   SetDeleteButtonState();

   if ( m_pCallback )
   {
      m_pCallback->OnDuctChanged();
   }
}

void CLinearDuctGrid::SetMeasurementType(CLinearDuctGeometry::MeasurementType mt)
{
   CLinearDuctGeometry ductGeometry;
   ductGeometry.SetMeasurementType(mt); // this is important.. want the current state
   // to be opposite what we will be setting it to below.

   // set the duct geometry type to the opposite value than it is getting set to
   // this was the value before the combo box selection changed
   if ( mt == CLinearDuctGeometry::AlongGirder )
   {
      ductGeometry.SetMeasurementType(CLinearDuctGeometry::FromPrevious);
   }
   else
   {
      ductGeometry.SetMeasurementType(CLinearDuctGeometry::AlongGirder);
   }
   GetData(ductGeometry);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);

   CLinearDuctDlg* pParent = (CLinearDuctDlg*)GetParent();
   const CGirderKey& girderKey = pParent->GetGirderKey();

   // convert the duct geometry to the new measurement type
   Float64 Lg = pBridge->GetGirderLength(girderKey);
   ductGeometry.ConvertMeasurementType(mt,Lg);

   // put the data back in the grid
   SetData(ductGeometry);

   if ( m_pCallback )
   {
      m_pCallback->OnDuctChanged();
   }
}

void CLinearDuctGrid::FillRow(ROWCOL row,Float64 location,Float64 offset,CLinearDuctGeometry::OffsetType offsetType)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CString strLocation;
   if ( location < 0 )
   {
      // fractional measure
      SetValueRange(CGXRange(row,nLocationCol),-100.*location);
      SetValueRange(CGXRange(row,nUnitCol),1L);
   }
   else
   {
      strLocation.Format(_T("%s"),FormatDimension(location,pDisplayUnits->GetSpanLengthUnit(),false));
      SetValueRange(CGXRange(row,nLocationCol),strLocation);
      SetValueRange(CGXRange(row,nUnitCol),0L);
   }

   CString strOffset;
   strOffset.Format(_T("%s"),FormatDimension(offset,pDisplayUnits->GetComponentDimUnit(),false));
   SetValueRange(CGXRange(row,nOffsetCol),strOffset);

   SetValueRange(CGXRange(row,nOffsetTypeCol),(long)offsetType);
}

void CLinearDuctGrid::GetPoint(ROWCOL row,Float64* pLocation,Float64* pOffset,CLinearDuctGeometry::OffsetType* pOffsetType)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   *pLocation = _tstof(GetCellValue(row,nLocationCol));
   if (_tstoi(GetCellValue(row,nUnitCol)) == 1L )
   {
      *pLocation /= -100.0;
   }
   else
   {
      *pLocation = ::ConvertToSysUnits(*pLocation,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   }

   *pOffset = _tstof(GetCellValue(row,nOffsetCol));
   *pOffset = ::ConvertToSysUnits(*pOffset, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   *pOffsetType = CLinearDuctGeometry::OffsetType(_tstoi(GetCellValue(row,nOffsetTypeCol)));
}

void CLinearDuctGrid::SetRowStyle(ROWCOL nRow)
{  
   CLinearDuctDlg* pParent = (CLinearDuctDlg*)GetParent();
   CLinearDuctGeometry::MeasurementType measurementType = pParent->GetMeasurementType();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   CString strUnits;
   strUnits.Format(_T("%s\n%s"),pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str(),_T("%"));
   SetStyleRange(CGXRange(nRow,nUnitCol), CGXStyle()
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
	   .SetChoiceList(strUnits)
	   .SetValue(0L)
      );

   if ( measurementType == CLinearDuctGeometry::FromPrevious )
   {
      SetStyleRange(CGXRange(nRow,nUnitCol), CGXStyle()
         .SetEnabled(FALSE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         );
   }
   else
   {
      SetStyleRange(CGXRange(nRow,nUnitCol), CGXStyle()
         .SetEnabled(TRUE)
         .SetInterior(::GetSysColor(COLOR_WINDOW))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         );
   }

   SetStyleRange(CGXRange(nRow,nOffsetTypeCol), CGXStyle()
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
   	.SetChoiceList(_T("Bottom Girder\nTop Girder"))
		.SetValue(0L)
      );

   if ( nRow == 1 || nRow == GetRowCount() )
   {
      // first row or last row... location is not editable
      SetStyleRange(CGXRange(nRow,nLocationCol,nRow,nUnitCol), CGXStyle()
         .SetEnabled(FALSE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         );
   }
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
    {
        return GetValueRowCol(nRow, nCol);
    }
}

void CLinearDuctGrid::OnChangedSelection(const CGXRange* pRange,BOOL bIsDragging,BOOL bKey)
{
   SetDeleteButtonState();
}

BOOL CLinearDuctGrid::OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   SetDeleteButtonState();
   return TRUE;
}

BOOL CLinearDuctGrid::OnEndEditing(ROWCOL nRow,ROWCOL nCol)
{
   if ( m_pCallback )
   {
      m_pCallback->OnDuctChanged();
   }

   return CGXGridWnd::OnEndEditing(nRow,nCol);
}

void CLinearDuctGrid::SetDeleteButtonState()
{
   CLinearDuctDlg* pParent = (CLinearDuctDlg*)GetParent();

	if (GetParam() == nullptr)
	{
      // grid isn't ready yet
      pParent->EnableDeleteBtn(FALSE);
	}
   else
   {
      // which rows are selected
      CRowColArray awRows;
      ROWCOL nSelRows = GetSelectedRows(awRows);
      if ( 0 < nSelRows && (awRows[0] != 1 && awRows[nSelRows-1] != GetRowCount()) )
      {
         // rows can be deleted if the first and last rows are not selected
         // (first and last row can never be deleted)
         pParent->EnableDeleteBtn(TRUE);
      }
      else
      {
         pParent->EnableDeleteBtn(FALSE);
      }
   }
}
