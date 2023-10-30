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

// HorizontalAlignmentGrid.cpp : implementation file
//

#include "stdafx.h"
#include "HorizontalAlignmentGrid.h"
#include "HorizontalAlignmentPage.h"

#include <EAF\EAFDisplayUnits.h>
#include <CoordGeom/Station.h>
#include <CoordGeom/Angle.h>
#include <CoordGeom/Direction.h>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CHorizontalAlignmentGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CHorizontalAlignmentGrid

CHorizontalAlignmentGrid::CHorizontalAlignmentGrid()
{
//   RegisterClass();
}

CHorizontalAlignmentGrid::~CHorizontalAlignmentGrid()
{
}

BEGIN_MESSAGE_MAP(CHorizontalAlignmentGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CHorizontalAlignmentGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
//	ON_COMMAND(ID_EDIT_INSERTROW, OnEditInsertRow)
//	ON_COMMAND(ID_EDIT_REMOVEROWS, OnEditRemoveRows)
//	ON_UPDATE_COMMAND_UI(ID_EDIT_REMOVEROWS, OnUpdateEditRemoveRows)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CHorizontalAlignmentGrid message handlers
//
int CHorizontalAlignmentGrid::GetColWidth(ROWCOL nCol)
{
   CRect rect = GetGridRect();
   int gridWidth = rect.Width();
   switch(nCol)
   {
   case 0:
      return gridWidth/10;

   case 1:
   case 2:
      return gridWidth/4;

   case 3:
   case 4:
   case 5:
      return 7*gridWidth/60;
   }

   return CGXGridWnd::GetColWidth(nCol);
}

void CHorizontalAlignmentGrid::AppendRow()
{
	ROWCOL nRow = 0;
   nRow = GetRowCount()+1;

	InsertRows(nRow, 1);
   SetRowStyle(nRow);
   InitRowData(nRow);
   SetCurrentCell(nRow, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	ScrollCellInView(nRow, GetLeftCol());
	Invalidate();
}

void CHorizontalAlignmentGrid::RemoveRows()
{
   CRowColArray awRows;
   ROWCOL nRowsSelected = GetSelectedRows(awRows);
   if ( 0 < nRowsSelected )
   {
      for ( int i = nRowsSelected - 1; i >= 0; i-- )
      {
         ROWCOL row = awRows[i];
         CGXGridWnd::RemoveRows(row,row);
      }
   }
}

void CHorizontalAlignmentGrid::InitRowData(ROWCOL row)
{
	GetParam()->EnableUndo(FALSE);

   CHorizontalAlignmentPage* pParent = (CHorizontalAlignmentPage*)GetParent();
   GET_IFACE2(pParent->GetBroker(),IEAFDisplayUnits,pDisplayUnits);
   UnitModeType unit_mode = (UnitModeType)(pDisplayUnits->GetUnitMode());

   SetValueRange(CGXRange(row,1),unit_mode == umUS ? _T("0+00") : _T("0+000"));
   SetValueRange(CGXRange(row,2),_T("45 L"));
   SetValueRange(CGXRange(row,3),_T("1000"));
   SetValueRange(CGXRange(row,4),_T("0"));
   SetValueRange(CGXRange(row,5),_T("0"));

   GetParam()->EnableUndo(TRUE);
}

void CHorizontalAlignmentGrid::CustomInit()
{
   CHorizontalAlignmentPage* pParent = (CHorizontalAlignmentPage*)GetParent();

   GET_IFACE2(pParent->GetBroker(),IEAFDisplayUnits,pDisplayUnits);
   const WBFL::Units::LengthData& alignment_unit = pDisplayUnits->GetAlignmentLengthUnit();
   std::_tstring strUnitTag = alignment_unit.UnitOfMeasure.UnitTag();

   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
	Initialize( );

	GetParam( )->EnableUndo(FALSE);

   const int num_rows=0;
   const int num_cols=5;

	SetRowCount(num_rows);
	SetColCount(num_cols);

		// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
         .SetHorizontalAlignment(DT_CENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   // set text along top row
	SetStyleRange(CGXRange(0,0), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Curve\n#"))
		);

   CString cv = _T("PI\nStation");
	SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

	SetStyleRange(CGXRange(0,2), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("* Forward\nTangent\nor\nDelta"))
		);

   CString strRadius;
   strRadius.Format(_T("Radius\n(%s)"),strUnitTag.c_str());
	SetStyleRange(CGXRange(0,3), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strRadius)
		);

   CString strEntrySpiral;
   strEntrySpiral.Format(_T("Entry\nSpiral\n(%s)"),strUnitTag.c_str());
	SetStyleRange(CGXRange(0,4), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strEntrySpiral)
		);

   CString strExitSpiral;
   strExitSpiral.Format(_T("Exit\nSpiral\n(%s)"),strUnitTag.c_str());
	SetStyleRange(CGXRange(0,5), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strExitSpiral)
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

void CHorizontalAlignmentGrid::SetRowStyle(ROWCOL nRow)
{
	GetParam()->EnableUndo(FALSE);

   SetStyleRange(CGXRange(nRow,1,nRow,2), CGXStyle()
	      .SetHorizontalAlignment(DT_RIGHT)
		);

	SetStyleRange(CGXRange(nRow,3,nRow,5), CGXStyle()
			.SetUserAttribute(GX_IDS_UA_VALID_MIN, _T("0.0e01"))
			.SetUserAttribute(GX_IDS_UA_VALID_MAX, _T("1.0e99"))
			.SetUserAttribute(GX_IDS_UA_VALID_MSG, _T("Please enter a positive value"))
         .SetHorizontalAlignment(DT_RIGHT)
		);

	GetParam()->EnableUndo(TRUE);
}

CString CHorizontalAlignmentGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CHorizontalAlignmentGrid::SetRowData(ROWCOL nRow,CompoundCurveData& data)
{
	GetParam()->EnableUndo(FALSE);

   CHorizontalAlignmentPage* pParent = (CHorizontalAlignmentPage*)GetParent();

   GET_IFACE2(pParent->GetBroker(),IEAFDisplayUnits,pDisplayUnits);
   UnitModeType unit_mode = (UnitModeType)(pDisplayUnits->GetUnitMode());

   Float64 station = data.PIStation;

   CComPtr<IStation> objStation;
   objStation.CoCreateInstance(CLSID_Station);
   objStation->put_Value(station);
   CComBSTR bstrStation;
   objStation->AsString(unit_mode,VARIANT_FALSE,&bstrStation);
   SetValueRange(CGXRange(nRow,1),CString(bstrStation));

   if ( data.bFwdTangent )
   {
      // forward tangent is a bearing
      CComBSTR bstrDirection;
      pParent->m_DirFormatter->Format(data.FwdTangent,nullptr,&bstrDirection);
      SetValueRange(CGXRange(nRow,2),CString(bstrDirection));
   }
   else
   {
      // forward tangent is a delta angle
      CComBSTR bstrAngle;
      pParent->m_AngleFormatter->Format(data.FwdTangent,nullptr,&bstrAngle);
      SetValueRange(CGXRange(nRow,2),CString(bstrAngle));
   }

   Float64 radius = WBFL::Units::ConvertFromSysUnits(data.Radius,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);
   SetValueRange(CGXRange(nRow,3),radius );

   Float64 entry_spiral = WBFL::Units::ConvertFromSysUnits(data.EntrySpiral,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);
   SetValueRange(CGXRange(nRow,4),entry_spiral );

   Float64 exit_spiral = WBFL::Units::ConvertFromSysUnits(data.ExitSpiral,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);
   SetValueRange(CGXRange(nRow,5),exit_spiral );

   GetParam()->EnableUndo(TRUE);
}

bool CHorizontalAlignmentGrid::GetRowData(ROWCOL nRow,Float64* pStation,Float64* pFwdTangent,bool* pbFwdTangent,Float64* pRadius,Float64* pEntrySpiral,Float64* pExitSpiral)
{
   CHorizontalAlignmentPage* pParent = (CHorizontalAlignmentPage*)GetParent();

   GET_IFACE2(pParent->GetBroker(),IEAFDisplayUnits,pDisplayUnits);

   try
   {
      std::_tstring strStation(GetCellValue(nRow, 1));
      WBFL::COGO::Station station(strStation, pDisplayUnits->GetStationFormat());
      *pStation = station.GetValue();
   }
   catch (...)
   {
      // station string isn't in a valid format
      return false;
   }

   // assume input is an angle
   std::_tstring strAngle(GetCellValue(nRow, 2));
   try
   {
      WBFL::COGO::Angle angle(strAngle);
      *pFwdTangent = angle.GetValue();
      *pbFwdTangent = false;

      if (::IsLE(*pFwdTangent, -M_PI) || ::IsGE(M_PI, *pFwdTangent) || ::IsZero(*pFwdTangent))
      {
         // a curve delta has been input... delta must be between -PI and PI (but not exactly +\-PI)
         return false;
      }
   }
   catch (...)
   {
      // it isn't an angle so assume it is a direction/bearing
      try
      {
         WBFL::COGO::Direction direction(strAngle);
         *pFwdTangent = direction.GetValue();
         *pbFwdTangent = true;
      }
      catch (...)
      {
         return false;
      }  
   }



   CString strRadius = GetCellValue(nRow,3);
   *pRadius = _tstof(strRadius);
   *pRadius = WBFL::Units::ConvertToSysUnits(*pRadius,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);

   CString strEntrySpiral = GetCellValue(nRow,4);
   *pEntrySpiral = _tstof(strEntrySpiral);
   *pEntrySpiral = WBFL::Units::ConvertToSysUnits(*pEntrySpiral,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);

   CString strExitSpiral = GetCellValue(nRow,5);
   *pExitSpiral = _tstof(strExitSpiral);
   *pExitSpiral = WBFL::Units::ConvertToSysUnits(*pExitSpiral,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);

   if ( ::IsLT(*pRadius,0.0) || ::IsLT(*pEntrySpiral,0.0) || ::IsLT(*pExitSpiral,0.0) )
   {
      // Radius must be greater than zero
      // Spirals must be a positive value
      return false;
   }

   return true;
}

void CHorizontalAlignmentGrid::SetCurveData(std::vector<CompoundCurveData>& curves)
{
   GetParam()->EnableUndo(FALSE);

   // empty the grid
   if ( 0 < GetRowCount() )
      CGXGridWnd::RemoveRows(1,GetRowCount());

   // now fill it up
   std::vector<CompoundCurveData>::iterator iter;
   for ( iter = curves.begin(); iter != curves.end(); iter++ )
   {
      CompoundCurveData& curve_data = *iter;
      AppendRow();
      SetRowData(GetRowCount(),curve_data);
   }

   GetParam()->EnableUndo(TRUE);
}

bool CHorizontalAlignmentGrid::GetCurveData(std::vector<CompoundCurveData>& curves)
{
   curves.clear();
   ROWCOL nRows = GetRowCount();
   for (ROWCOL row = 1; row <= nRows; row++ )
   {
      CompoundCurveData curve_data;
      if ( !GetRowData(row,&curve_data.PIStation,&curve_data.FwdTangent,&curve_data.bFwdTangent,&curve_data.Radius,&curve_data.EntrySpiral,&curve_data.ExitSpiral) )
      {
         return false;
      }

      curves.push_back(curve_data);
   }

   return true;
}

bool SortByStation(const CompoundCurveData& c1,const CompoundCurveData& c2)
{
   return c1.PIStation < c2.PIStation;
}

void CHorizontalAlignmentGrid::SortCurves()
{
   std::vector<CompoundCurveData> curves;
   if ( GetCurveData(curves) )
   {
      std::sort(curves.begin(),curves.end(),SortByStation);
      SetCurveData(curves);
   }
}
