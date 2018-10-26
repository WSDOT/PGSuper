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

// BridgeDescDeckPointGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "BridgeDescDeckPointGrid.h"
#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include "BridgeDescDlg.h"
#include "BridgeDescDeckDetailsPage.h"
#include <Units\Measure.h>

#include <EAF\EAFDisplayUnits.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDeckPointGrid

CBridgeDescDeckPointGrid::CBridgeDescDeckPointGrid()
{
   HRESULT hr = m_objStation.CoCreateInstance(CLSID_Station);
   m_bEnabled = TRUE;
}

CBridgeDescDeckPointGrid::~CBridgeDescDeckPointGrid()
{
}

BEGIN_MESSAGE_MAP(CBridgeDescDeckPointGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(BridgeDescDeckPointGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDeckPointGrid message handlers

int CBridgeDescDeckPointGrid::GetColWidth(ROWCOL nCol)
{
   if ( nCol == 0 )
   {
      return 0;
   }

   return CGXGridWnd::GetColWidth(nCol);
}

void CBridgeDescDeckPointGrid::OnModifyCell(ROWCOL nRow,ROWCOL nCol)
{
   if ( (nCol == 3 || nCol == 4) && nRow%2 == 0)
   {
      // left transition type changed
      CDeckPoint deck_point;
      GetTransitionRowData(nRow,&deck_point);

      // enable offset cells in next row only if not parallel
      BOOL bEnableLeft  = (deck_point.LeftTransitionType  == pgsTypes::dptParallel ? FALSE : TRUE);
      BOOL bEnableRight = (deck_point.RightTransitionType == pgsTypes::dptParallel ? FALSE : TRUE);

	   GetParam()->EnableUndo(FALSE);
      GetParam()->SetLockReadOnly(FALSE);

      // set up styles
      CGXStyle enable_style;
      CGXStyle disable_style;
      enable_style.SetEnabled(TRUE)
           .SetReadOnly(FALSE)
           .SetInterior(::GetSysColor(COLOR_WINDOW))
           .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

      disable_style.SetEnabled(FALSE)
           .SetReadOnly(TRUE)
           .SetInterior(::GetSysColor(COLOR_BTNFACE))
           .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

      // set up ranges
      CGXRange left_range(nRow+1,3);
      CGXRange right_range(nRow+1,4);


      // if disabled, make values same 
      if ( !bEnableLeft )
      {
         SetStyleRange(left_range,CGXStyle().SetValue( GetCellValue(nRow-1,3) ) );
      }

      if ( !bEnableRight )
      {
         SetStyleRange(right_range,CGXStyle().SetValue( GetCellValue(nRow-1,4) ) );
      }

      
      // set the styles
      SetStyleRange(left_range,  (bEnableLeft  ? enable_style : disable_style) );
      SetStyleRange(right_range, (bEnableRight ? enable_style : disable_style) );

      GetParam()->SetLockReadOnly(TRUE);
      GetParam()->EnableUndo(FALSE);

   }

   CGXGridWnd::OnEndEditing(nRow,nCol);
}

BOOL CBridgeDescDeckPointGrid::OnValidateCell(ROWCOL nRow,ROWCOL nCol)
{
   CGXControl* pControl = GetControl(nRow,nCol);
   CWnd* pWnd = (CWnd*)pControl;

   CString strText;
   if ( pControl->IsInit() )
   {
      pControl->GetCurrentText(strText);
   }
   else
   {
      strText = GetValueRowCol(nRow,nCol);
   }

   if ( nCol == 1 )
   {
      // Validation station... 
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      UnitModeType unitMode = (UnitModeType)(pDisplayUnits->GetUnitMode());
      m_objStation->FromString(CComBSTR(strText),unitMode);

      Float64 station;
      m_objStation->get_Value(&station);
      station = ::ConvertToSysUnits(station,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);

      CDeckPoint prevPoint, nextPoint;
      prevPoint.Station = station - 100;
      nextPoint.Station = station + 100;

      if ( nRow != 1 )
      {
         GetPointRowData(nRow-2,&prevPoint);
      }

      ROWCOL nRows = GetRowCount();
      if ( nRow != nRows )
      {
         GetPointRowData(nRow+2,&nextPoint);
      }

      if ( station <= prevPoint.Station || nextPoint.Station <= station )
      {
         SetWarningText(_T("Invalid station"));
         return FALSE;
      }
   }

	return CGXGridWnd::OnValidateCell(nRow, nCol);
}

ROWCOL CBridgeDescDeckPointGrid::AddRow()
{
	ROWCOL nRow = 0;
   nRow = GetRowCount()+1;
	nRow = Max((ROWCOL)1, nRow);

	InsertRows(nRow, 1);
   return nRow;
}

void CBridgeDescDeckPointGrid::CustomInit()
{
   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
	Initialize( );

   SetMergeCellsMode(gxnMergeDelayEval);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

	GetParam( )->EnableUndo(FALSE);

   const int num_rows = 0;
   const int num_cols = 4;

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
	SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Station"))
		);

	SetStyleRange(CGXRange(0,2), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Measured\nFrom"))
		);

   CString cv;
   cv.Format(_T("Left Offset (%s)"), pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,3), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv.Format(_T("Right Offset (%s)"), pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,4), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   // make it so that text fits correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));
	ResizeColWidthsToFit(CGXRange(0,0,num_rows,num_cols));

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();
	SetFocus();

	GetParam( )->EnableUndo(TRUE);
}

CString CBridgeDescDeckPointGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CBridgeDescDeckPointGrid::SetPointRowData(ROWCOL row,const CDeckPoint& point)
{
   SetStyleRange(CGXRange(row,0), CGXStyle()
      .SetValue(_T(""))
      );

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   CString strStation = FormatStation(pDisplayUnits->GetStationFormat(),point.Station);

   SetStyleRange(CGXRange(row,1), CGXStyle()
      .SetControl(GX_IDS_CTRL_EDIT)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetEnabled(TRUE)
      .SetValue(strStation)
      );

   CString strDatumChoices(_T("Bridge Line\nAlignment"));
   SetStyleRange(CGXRange(row,2),CGXStyle()
      .SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
      .SetHorizontalAlignment(DT_LEFT)
      .SetChoiceList(strDatumChoices)
      .SetValue(point.MeasurementType == pgsTypes::omtBridge ? _T("Bridge Line") : _T("Alignment"))
      );

   Float64 offset = ::ConvertFromSysUnits(point.LeftEdge,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);
   SetStyleRange(CGXRange(row,3),CGXStyle()
      .SetControl(GX_IDS_CTRL_EDIT)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(offset)
      );

   offset = ::ConvertFromSysUnits(point.RightEdge,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);
   SetStyleRange(CGXRange(row,4),CGXStyle()
      .SetControl(GX_IDS_CTRL_EDIT)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(offset)
      );

   ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));
}

void CBridgeDescDeckPointGrid::GetPointRowData(ROWCOL row,CDeckPoint* pPoint)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   // Station
   CString strStation = GetCellValue(row,1);
   UnitModeType unitMode = (UnitModeType)(pDisplayUnits->GetUnitMode());
   m_objStation->FromString(CComBSTR(strStation),unitMode);

   Float64 station;
   m_objStation->get_Value(&station);
   pPoint->Station = ::ConvertToSysUnits(station,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);

   // Datum
   CString strDatum = GetCellValue(row,2);
   pPoint->MeasurementType = (strDatum == _T("Alignment") ? pgsTypes::omtAlignment : pgsTypes::omtBridge);

   // Left Edge
   Float64 offset = _tstof(GetCellValue(row,3));
   pPoint->LeftEdge = ::ConvertToSysUnits(offset,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);

   // Right Edge
   offset = _tstof(GetCellValue(row,4));
   pPoint->RightEdge = ::ConvertToSysUnits(offset,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);
}

void CBridgeDescDeckPointGrid::SetTransitionRowData(ROWCOL row,const CDeckPoint& point)
{
   SetStyleRange(CGXRange(row,0), CGXStyle()
      .SetValue(_T(""))
      );

   SetStyleRange(CGXRange(row,1,row,2), CGXStyle()
      .SetMergeCell(GX_MERGE_HORIZONTAL)
      .SetHorizontalAlignment(DT_CENTER)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      //.SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
      .SetReadOnly(TRUE)
      .SetEnabled(FALSE)
      .SetValue(_T("Transition"))
      );

   CString strChoices(_T("Parallel\nLinear\nSpline"));
   CString strChoice;
   if ( point.LeftTransitionType == pgsTypes::dptParallel )
   {
      strChoice = _T("Parallel");
   }
   else if ( point.LeftTransitionType == pgsTypes::dptLinear )
   {
      strChoice = _T("Linear");
   }
   else if ( point.LeftTransitionType == pgsTypes::dptSpline )
   {
      strChoice = _T("Spline");
   }

   SetStyleRange(CGXRange(row,3), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
      .SetChoiceList(strChoices)
      .SetValue(strChoice)
      );

   if ( point.RightTransitionType == pgsTypes::dptParallel )
   {
      strChoice = _T("Parallel");
   }
   else if ( point.RightTransitionType == pgsTypes::dptLinear )
   {
      strChoice = _T("Linear");
   }
   else if ( point.RightTransitionType == pgsTypes::dptSpline )
   {
      strChoice = _T("Spline");
   }

   SetStyleRange(CGXRange(row,4), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
      .SetChoiceList(strChoices)
      .SetValue(strChoice)
      );
}

void CBridgeDescDeckPointGrid::GetTransitionRowData(ROWCOL row,CDeckPoint *pPoint)
{
   CString strValue = GetCellValue(row,3);
   if ( strValue == _T("Parallel") )
   {
      pPoint->LeftTransitionType = pgsTypes::dptParallel;
   }
   else if ( strValue == _T("Linear") )
   {
      pPoint->LeftTransitionType = pgsTypes::dptLinear;
   }
   else
   {
      pPoint->LeftTransitionType = pgsTypes::dptSpline;
   }

   strValue = GetCellValue(row,4);
   if ( strValue == _T("Parallel") )
   {
      pPoint->RightTransitionType = pgsTypes::dptParallel;
   }
   else if ( strValue == _T("Linear") )
   {
      pPoint->RightTransitionType = pgsTypes::dptLinear;
   }
   else
   {
      pPoint->RightTransitionType = pgsTypes::dptSpline;
   }
}

void CBridgeDescDeckPointGrid::FillGrid(const CDeckDescription2* pDeck)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   if ( 1 <= GetRowCount() )
   {
      RemoveRows(1,GetRowCount()); // clear the grid, except the header row
   }

   std::vector<CDeckPoint>::const_iterator iter;
   CollectionIndexType cPoint = 0;
   CollectionIndexType nPoints = pDeck->DeckEdgePoints.size();
   for ( iter = pDeck->DeckEdgePoints.begin(); iter != pDeck->DeckEdgePoints.end(); iter++, cPoint++ )
   {
      const CDeckPoint& point = *iter;
      if ( cPoint != nPoints-1 )
      {
         // add 2 rows
         ROWCOL row1 = AddRow();
         SetPointRowData(row1,point);

         ROWCOL row2 = AddRow();
         SetTransitionRowData(row2,point);
      }
      else
      {
         // this is the last point, don't add the row for transition data
         ROWCOL row = AddRow();
         SetPointRowData(row,point);
      }
   }

   // make sure the girder spacing following
   // a transition row is enabled/disabled properly
   ROWCOL nRows = GetRowCount();
   for ( ROWCOL r = 2; r < nRows; r+=2 )
   {
      OnModifyCell(r,3);
      OnModifyCell(r,4);
   }

   Enable(m_bEnabled);

   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(TRUE);
}

std::vector<CDeckPoint> CBridgeDescDeckPointGrid::GetEdgePoints()
{
   std::vector<CDeckPoint> deckPoints;
   ROWCOL nRows = GetRowCount();

   for ( ROWCOL row = 1; row <= nRows; row++ )
   {
      CDeckPoint point;

      if ( row % 2 != 0 )
      {
         GetPointRowData(row,&point);
         deckPoints.push_back(point);
      }
      else
      {
         CDeckPoint& p = deckPoints.back();
         GetTransitionRowData(row,&p);
      }
   }

   return deckPoints;
}

void CBridgeDescDeckPointGrid::SelectRow(ROWCOL nRow)
{
   if ( GetRowCount() == 1 )
   {
      return;
   }

   SelectRange(CGXRange(0,0,GetRowCount(),GetColCount()),FALSE); // unselect everything

   ROWCOL firstRow = (nRow%2 == 0 ? nRow-1 : nRow);

   if ( nRow == GetRowCount() )
   {
      firstRow--;
   }

   SelectRange(CGXRange(firstRow,0,firstRow+1,GetColCount()),TRUE); // select this row

   CBridgeDescDeckDetailsPage* pParent = (CBridgeDescDeckDetailsPage*)GetParent();
   pParent->EnableRemove(TRUE);
}

BOOL CBridgeDescDeckPointGrid::OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   SelectRow(nRow);

   return TRUE;
}

void CBridgeDescDeckPointGrid::RemoveSelectedRows()
{
	CGXRangeList* pSelList = GetParam()->GetRangeList();
	if (pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1)
	{
		CGXRange range = pSelList->GetHead();
		range.ExpandRange(1, 0, GetRowCount(), 0);
      CGXGridWnd::RemoveRows(range.top, range.bottom);
	}

   CBridgeDescDeckDetailsPage* pParent = (CBridgeDescDeckDetailsPage*)GetParent();
   pParent->EnableRemove(FALSE);
}

void CBridgeDescDeckPointGrid::Enable(BOOL bEnable)
{
   m_bEnabled = bEnable;

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CGXStyle style;
   CGXRange range;
   if ( bEnable )
   {
      style.SetEnabled(TRUE)
           .SetReadOnly(FALSE)
           .SetInterior(::GetSysColor(COLOR_WINDOW))
           .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

      if ( 0 < GetColCount() && 0 < GetRowCount() )
      {
         range = CGXRange(1,1,1,GetColCount());
         SetStyleRange(range,style);
      }

      style.SetInterior(::GetSysColor(COLOR_BTNFACE));

      range = CGXRange(0,0,0,GetColCount());
      SetStyleRange(range,style);

      range = CGXRange(0,0,Min(GetRowCount(),(ROWCOL)1),0);
      SetStyleRange(range,style);
   }
   else
   {
      style.SetEnabled(FALSE)
           .SetReadOnly(TRUE)
           .SetInterior(::GetSysColor(COLOR_BTNFACE))
           .SetTextColor(::GetSysColor(COLOR_GRAYTEXT));

      range = CGXRange(0,0,GetRowCount(),GetColCount());
      SetStyleRange(range,style);
   }


   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(FALSE);
}
