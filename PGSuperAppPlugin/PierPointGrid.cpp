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
#include <cmath>


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
        for (ROWCOL row = 1; row <= grid.GetRowCount(); ++row)
        {
           for (ROWCOL col = 1; col <= 4; ++col)
           {
              if (col == 3 || (col == 4 &&
                  grid.GetCellValue(row,3) == _T("Straight")))
                 continue;
              Float64 value;
              if (!grid.ReadNumber(row, col, value) || (col == 4 && value <= 0))
              {
                 AfxMessageBox(_T("Enter a valid coordinate or a positive arc radius."));
                 pDX->Fail();
              }
           }
        }
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
   const int num_cols = 4;

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
			.SetValue(_T("Point"))
		);

   CString cv;

   cv.Format(_T("X (%s)"),pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,pp++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv.Format(_T("Y (%s)"),pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,pp++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);


   SetStyleRange(CGXRange(0,pp++), CGXStyle()
      .SetWrapText(TRUE).SetEnabled(FALSE)
      .SetHorizontalAlignment(DT_CENTER).SetVerticalAlignment(DT_VCENTER)
      .SetValue(_T("Transition from\nprevious point")));
   cv.Format(_T("Radius (%s)"), pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(0,pp++), CGXStyle()
      .SetWrapText(TRUE).SetEnabled(FALSE)
      .SetHorizontalAlignment(DT_CENTER).SetVerticalAlignment(DT_VCENTER).SetValue(cv));
   ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));

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
    // The last row is the fixed beam endpoint. Insert new points before it.
    const ROWCOL endRow = GetRowCount();
    if (endRow == 0) return;
    CPierPointData pierPointData;
    if (endRow > 1)
       GetPierPointData(endRow - 1, &pierPointData);
    else
       pierPointData.Set_Y(m_StartDepth);
    Float64 endX = 0;
    ReadNumber(endRow, 1, endX);
    auto pBroker = EAFGetBroker();
    GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
    endX = WBFL::Units::ConvertToSysUnits(endX, pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
    // Start halfway along the remaining segment, with a straight transition.
    pierPointData.Set_X((pierPointData.Get_X() + endX) / 2);
    pierPointData.SetTransition(CPierPointData::ttStraight, 0);
    pierPointData.SetEndPoint(false);
    GetParam()->EnableUndo(FALSE);
    GetParam()->SetLockReadOnly(FALSE);
    InsertRows(endRow, 1);
    SetPierPointData(endRow, pierPointData);

    UpdateTransitionCells();
    ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));
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
      if (selRow > 0 && selRow < GetRowCount())
         RemoveRows(selRow,selRow);
   }
   UpdateTransitionCells();

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


   Float64 h1l, h1r, h2, x2, x1l, x1r;
   pier.GetXBeamDimensions(pgsTypes::stLeft, &h1l, &h2, &x2, &x1l);
   pier.GetXBeamDimensions(pgsTypes::stRight, &h1r, &h2, &x2, &x1r);
   Float64 length = pier.GetXBeamOverhang(pgsTypes::stLeft) + pier.GetXBeamOverhang(pgsTypes::stRight) - x1l - x1r;
   std::vector<CPierPointData> points = pier.GetPierPointData();
   for (ColumnIndexType idx = 1; idx < pier.GetColumnCount(); ++idx)
      length += pier.GetColumnSpacing(idx - 1);
   CPierPointData::UpdateEndPoint(points, length, h1r);
   m_StartDepth = h1l;
   for (const auto& point : points)
      AddPierPoint(point);

   UpdateTransitionCells();
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

   CString transition = _T("Straight");
   if (pierPoint.GetTransitionType() == CPierPointData::ttArcUpward) transition = _T("Arc upward");
   else if (pierPoint.GetTransitionType() == CPierPointData::ttArcDownward) transition = _T("Arc downward");
   SetStyleRange(CGXRange(row,3), CGXStyle()
      .SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
      .SetChoiceList(_T("Straight\nArc upward\nArc downward"))
      .SetEnabled(TRUE).SetReadOnly(FALSE).SetValue(transition));
   const Float64 radius = WBFL::Units::ConvertFromSysUnits(pierPoint.GetRadius(), pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   SetStyleRange(CGXRange(row,4), CGXStyle().SetHorizontalAlignment(DT_RIGHT).SetValue(radius));
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
   pPierPoint->SetEndPoint(row == GetRowCount());
   CPierPointData::TransitionType type = CPierPointData::ttStraight;
   {
      const CString transition = GetCellValue(row,3);
      if (transition == _T("Arc upward")) type = CPierPointData::ttArcUpward;
      else if (transition == _T("Arc downward")) type = CPierPointData::ttArcDownward;
   }
   Float64 radius = type == CPierPointData::ttStraight ? 0 : _tstof(GetCellValue(row,4));
   radius = WBFL::Units::ConvertToSysUnits(radius, pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   pPierPoint->SetTransition(type, radius);
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

bool CPierPointGrid::ReadNumber(ROWCOL row, ROWCOL col, Float64& value)
{
   CString text = GetCellValue(row,col);
   text.Trim();
   if (text.IsEmpty()) return false;
   TCHAR* end = nullptr;
   value = _tcstod(text, &end);
   return end != text.GetString() && *end == 0 && std::isfinite(value);
}

void CPierPointGrid::UpdateRadiusCell(ROWCOL row)
{
   const CString transition = GetCellValue(row,3);
   const BOOL enabled =
      (transition == _T("Arc upward") || transition == _T("Arc downward"));
   SetStyleRange(CGXRange(row,4), CGXStyle()
      .SetEnabled(enabled).SetReadOnly(!enabled)
      .SetInterior(::GetSysColor(enabled ? COLOR_WINDOW : COLOR_BTNFACE))
      .SetTextColor(::GetSysColor(enabled ? COLOR_WINDOWTEXT : COLOR_GRAYTEXT)));
   if (!enabled)
      SetStyleRange(CGXRange(row,4), CGXStyle().SetValue(_T("-")));
   else
   {
      Float64 radius;
      if (!ReadNumber(row,4,radius) || radius <= 0)
      {
         auto pBroker = EAFGetBroker();
         GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
         const Float64 previousX = row > 1 ? _tstof(GetCellValue(row-1,1)) : 0;
         const Float64 previousY = row > 1 ? _tstof(GetCellValue(row-1,2)) :
            WBFL::Units::ConvertFromSysUnits(m_StartDepth, pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
         const Float64 dx = _tstof(GetCellValue(row,1)) - previousX;
         const Float64 dy = _tstof(GetCellValue(row,2)) - previousY;
         radius = 10 * std::hypot(dx,dy);
         SetStyleRange(CGXRange(row,4), CGXStyle().SetValue(radius));
      }
   }
}

void CPierPointGrid::UpdateEndPoint(Float64 length, Float64 depth, Float64 startDepth)
{
   m_StartDepth = startDepth;
   const ROWCOL row = GetRowCount();
   if (row == 0) return;
   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   GetParam()->SetLockReadOnly(FALSE);
   SetStyleRange(CGXRange(row,1), CGXStyle().SetValue(
      WBFL::Units::ConvertFromSysUnits(length, pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure)));
   SetStyleRange(CGXRange(row,2), CGXStyle().SetValue(
      WBFL::Units::ConvertFromSysUnits(depth, pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure)));
   GetParam()->SetLockReadOnly(TRUE);
}

void CPierPointGrid::UpdateTransitionCells()
{
   for (ROWCOL row = 1; row <= GetRowCount(); ++row)
   {
      const BOOL endPoint = row == GetRowCount();
      CString label;
      if (endPoint) label = _T("End");
      else label.Format(_T("%d"), row);
      SetStyleRange(CGXRange(row,0), CGXStyle().SetValue(label));
      SetStyleRange(CGXRange(row,1,row,2), CGXStyle()
         .SetEnabled(!endPoint).SetReadOnly(endPoint)
         .SetInterior(::GetSysColor(endPoint ? COLOR_BTNFACE : COLOR_WINDOW)));
      SetStyleRange(CGXRange(row,3), CGXStyle().SetEnabled(TRUE).SetReadOnly(FALSE));
      UpdateRadiusCell(row);
   }
}

void CPierPointGrid::OnModifyCell(ROWCOL nRow,ROWCOL nCol)
{
   __super::OnModifyCell(nRow,nCol);
   if (nRow > 0 && nCol == 3)
   {
      GetParam()->EnableUndo(FALSE);
      GetParam()->SetLockReadOnly(FALSE);
      UpdateRadiusCell(nRow);
      GetParam()->SetLockReadOnly(TRUE);
      GetParam()->EnableUndo(TRUE);
   }
}

BOOL CPierPointGrid::OnEndEditing(ROWCOL nRow, ROWCOL nCol)
{
   CGXControl* control = GetControl(nRow,nCol);
   const BOOL modified = control && control->GetModify();
   if (modified && (nCol == 1 || nCol == 2 || nCol == 4))
   {
      Float64 value;
      if (!ReadNumber(nRow,nCol,value) || (nCol == 4 && value <= 0))
      {
         AfxMessageBox(_T("Enter a valid coordinate or a positive arc radius."));
         return FALSE;
      }
   }
   const BOOL accepted = CGXGridWnd::OnEndEditing(nRow,nCol);
   if (accepted && modified && GetParent())
      GetParent()->PostMessage(WM_PIERPOINT_GRID_CELL_CHANGED, (WPARAM)nRow, (LPARAM)nCol);
   return accepted;
}
