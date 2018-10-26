///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// ProfileGrid.cpp : implementation file
//

#include "stdafx.h"
#include "ProfileGrid.h"
#include "ProfilePage.h"

#include <EAF\EAFDisplayUnits.h>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CProfileGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CProfileGrid

CProfileGrid::CProfileGrid()
{
//   RegisterClass();
}

CProfileGrid::~CProfileGrid()
{
}

BEGIN_MESSAGE_MAP(CProfileGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CProfileGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
//	ON_COMMAND(ID_EDIT_INSERTROW, OnEditInsertRow)
//	ON_COMMAND(ID_EDIT_REMOVEROWS, OnEditRemoveRows)
//	ON_UPDATE_COMMAND_UI(ID_EDIT_REMOVEROWS, OnUpdateEditRemoveRows)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CProfileGrid::AppendRow()
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

void CProfileGrid::RemoveRows()
{
	CGXRangeList* pSelList = GetParam()->GetRangeList();
	if (pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1)
	{
		CGXRange range = pSelList->GetHead();
		range.ExpandRange(1, 0, GetRowCount(), 0);
      CGXGridWnd::RemoveRows(range.top, range.bottom);
	}
}

void CProfileGrid::InitRowData(ROWCOL row)
{
	GetParam()->EnableUndo(FALSE);

   SetValueRange(CGXRange(row,1),"0+00");
   SetValueRange(CGXRange(row,2),"0.00");
   SetValueRange(CGXRange(row,3),"0.00");
   SetValueRange(CGXRange(row,4),"0.00");

   GetParam()->EnableUndo(TRUE);
}

void CProfileGrid::CustomInit()
{
   CProfilePage* pParent = (CProfilePage*)GetParent();

   GET_IFACE2(pParent->GetBroker(),IEAFDisplayUnits,pDisplayUnits);
   const unitmgtLengthData& alignment_unit = pDisplayUnits->GetAlignmentLengthUnit();
   std::string strUnitTag = alignment_unit.UnitOfMeasure.UnitTag();

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

   // no row moving
	this->GetParam()->EnableMoveRows(FALSE);

   // disable left side
	this->SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
         .SetHorizontalAlignment(DT_CENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   // set text along top row
	this->SetStyleRange(CGXRange(0,0), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Curve\n#"))
		);

   CString cv = "PVI\nStation";
	this->SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

	this->SetStyleRange(CGXRange(0,2), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue("Exit\nGrade\n(%)")
		);

   CString strLength1;
   strLength1.Format("L1\n(%s)",strUnitTag.c_str());
	this->SetStyleRange(CGXRange(0,3), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strLength1)
      );

   CString strLength2;
   strLength2.Format("L2\n(%s)",strUnitTag.c_str());
	this->SetStyleRange(CGXRange(0,4), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strLength2)
       );

   // make it so that text fits correctly in header row
	this->ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));

   // don't allow users to resize grids
   this->GetParam( )->EnableTrackColWidth(0); 
   this->GetParam( )->EnableTrackRowHeight(0); 

	this->EnableIntelliMouse();
	this->SetFocus();

	this->GetParam( )->EnableUndo(TRUE);
}

void CProfileGrid::SetRowStyle(ROWCOL nRow)
{
	GetParam()->EnableUndo(FALSE);

   this->SetStyleRange(CGXRange(nRow,1,nRow,4), CGXStyle()
	      .SetHorizontalAlignment(DT_RIGHT)
		);

	GetParam()->EnableUndo(TRUE);
}

CString CProfileGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CProfileGrid::SetRowData(ROWCOL nRow,VertCurveData& data)
{
	GetParam()->EnableUndo(FALSE);

   CProfilePage* pParent = (CProfilePage*)GetParent();

   GET_IFACE2(pParent->GetBroker(),IEAFDisplayUnits,pDisplayUnits);
   UnitModeType unit_mode = (UnitModeType)(pDisplayUnits->GetUnitMode());

   double station = data.PVIStation;
   station = ::ConvertFromSysUnits(station,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);

   CComPtr<IStation> objStation;
   objStation.CoCreateInstance(CLSID_Station);
   objStation->put_Value(station);
   CComBSTR bstrStation;
   objStation->AsString(unit_mode,&bstrStation);
   SetValueRange(CGXRange(nRow,1),CString(bstrStation));

   double grade = data.ExitGrade*100;
   SetValueRange(CGXRange(nRow,2), grade);

   double L1 = ::ConvertFromSysUnits(data.L1,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);
   SetValueRange(CGXRange(nRow,3),L1);

   double L2 = ::ConvertFromSysUnits(data.L2,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);
   SetValueRange(CGXRange(nRow,4),L2);

   GetParam()->EnableUndo(TRUE);
}

bool CProfileGrid::GetRowData(ROWCOL nRow,double* pStation,double* pGrade,double* pL1,double* pL2)
{
   CProfilePage* pParent = (CProfilePage*)GetParent();

   GET_IFACE2(pParent->GetBroker(),IEAFDisplayUnits,pDisplayUnits);
   UnitModeType unit_mode = (UnitModeType)(pDisplayUnits->GetUnitMode());

   CString strStation = GetCellValue(nRow,1);
   CComPtr<IStation> station;
   station.CoCreateInstance(CLSID_Station);
   HRESULT hr = station->FromString(CComBSTR(strStation),unit_mode);
   if ( FAILED(hr) )
      return false;

   double station_value;
   station->get_Value(&station_value);
   station_value = ::ConvertToSysUnits(station_value,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);
   *pStation = station_value;

   CString strGrade = GetCellValue(nRow,2);
   *pGrade = atof(strGrade)/100;

   CString strL1 = GetCellValue(nRow,3);
   *pL1 = atof(strL1);
   *pL1 = ::ConvertToSysUnits(*pL1,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);

   CString strL2 = GetCellValue(nRow,4);
   *pL2 = atof(strL2);
   *pL2 = ::ConvertToSysUnits(*pL2,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);

   return true;
}

void CProfileGrid::SetCurveData(std::vector<VertCurveData>& curves)
{
   GetParam()->EnableUndo(FALSE);

   // empty the grid
   if ( 0 < GetRowCount() )
      CGXGridWnd::RemoveRows(1,GetRowCount());

   // now fill it up
   std::vector<VertCurveData>::iterator iter;
   for ( iter = curves.begin(); iter != curves.end(); iter++ )
   {
      VertCurveData& curve_data = *iter;
      AppendRow();
      SetRowData(GetRowCount(),curve_data);
   }

   GetParam()->EnableUndo(TRUE);
}

bool CProfileGrid::GetCurveData(std::vector<VertCurveData>& curves)
{
   curves.clear();
   ROWCOL nRows = GetRowCount();
   for (ROWCOL row = 1; row <= nRows; row++ )
   {
      VertCurveData curve_data;
      if ( !GetRowData(row,&curve_data.PVIStation,&curve_data.ExitGrade,&curve_data.L1,&curve_data.L2) )
         return false;

      curves.push_back(curve_data);
   }

   return true;
}

bool SortByStation(const VertCurveData& c1,const VertCurveData& c2)
{
   return c1.PVIStation < c2.PVIStation;
}

void CProfileGrid::SortCurves()
{
   std::vector<VertCurveData> curves;
   
   if ( GetCurveData(curves) )
   {
      std::sort(curves.begin(),curves.end(),SortByStation);
      SetCurveData(curves);
   }
}
