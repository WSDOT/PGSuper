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

// CrownSlopeGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "CrownSlopeGrid.h"
#include "ProfilePage.h"
#include "PGSuperUnits.h"
#include "PGSuperDoc.h"

#include <EAF\EAFDisplayUnits.h>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CCrownSlopeGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CCrownSlopeGrid

CCrownSlopeGrid::CCrownSlopeGrid()
{
//   RegisterClass();
}

CCrownSlopeGrid::~CCrownSlopeGrid()
{
}

BEGIN_MESSAGE_MAP(CCrownSlopeGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CCrownSlopeGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
//	ON_COMMAND(ID_EDIT_INSERTROW, OnEditInsertRow)
//	ON_COMMAND(ID_EDIT_REMOVEROWS, OnEditRemoveRows)
//	ON_UPDATE_COMMAND_UI(ID_EDIT_REMOVEROWS, OnUpdateEditRemoveRows)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CCrownSlopeGrid::AppendRow()
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

void CCrownSlopeGrid::RemoveRows()
{
	CGXRangeList* pSelList = GetParam()->GetRangeList();
	if (pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1)
	{
		CGXRange range = pSelList->GetHead();
		range.ExpandRange(1, 0, GetRowCount(), 0);
      CGXGridWnd::RemoveRows(range.top, range.bottom);
	}
}

void CCrownSlopeGrid::InitRowData(ROWCOL row)
{
	GetParam()->EnableUndo(FALSE);

   SetValueRange(CGXRange(row,1),"0+00");
   SetValueRange(CGXRange(row,2),"-0.02");
   SetValueRange(CGXRange(row,3),"-0.02");
   SetValueRange(CGXRange(row,4),"0.00");

   GetParam()->EnableUndo(TRUE);
}

void CCrownSlopeGrid::CustomInit()
{
   CProfilePage* pParent = (CProfilePage*)GetParent();

   GET_IFACE2(pParent->GetBroker(),IEAFDisplayUnits,pDisplayUnits);
   const unitmgtLengthData& alignment_unit = pDisplayUnits->GetAlignmentLengthUnit();
   std::_tstring strUnitTag = alignment_unit.UnitOfMeasure.UnitTag();

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
			.SetValue(_T("Section\n#"))
		);

   CString cv = "Station";
	this->SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   CString strLeftSlope;
   strLeftSlope.Format(_T("Left\nSlope\n(%s/%s)"),strUnitTag.c_str(),strUnitTag.c_str());
	this->SetStyleRange(CGXRange(0,2), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strLeftSlope)
		);

   CString strRightSlope;
   strRightSlope.Format(_T("Right\nSlope\n(%s/%s)"),strUnitTag.c_str(),strUnitTag.c_str());
	this->SetStyleRange(CGXRange(0,3), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strRightSlope)
		);

   CString strCPO;
   strCPO.Format(_T("Crown Point Offset\nfrom\nProfile Grade\n(%s)"),strUnitTag.c_str());
	this->SetStyleRange(CGXRange(0,4), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strCPO)
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

void CCrownSlopeGrid::SetRowStyle(ROWCOL nRow)
{
	GetParam()->EnableUndo(FALSE);

   this->SetStyleRange(CGXRange(nRow,1,nRow,4), CGXStyle()
	      .SetHorizontalAlignment(DT_RIGHT)
		);

	GetParam()->EnableUndo(TRUE);
}

CString CCrownSlopeGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CCrownSlopeGrid::SetRowData(ROWCOL nRow,CrownData2& data)
{
	GetParam()->EnableUndo(FALSE);

   CProfilePage* pParent = (CProfilePage*)GetParent();

   GET_IFACE2(pParent->GetBroker(),IEAFDisplayUnits,pDisplayUnits);
   UnitModeType unit_mode = (UnitModeType)(pDisplayUnits->GetUnitMode());

   Float64 station = data.Station;
   station = ::ConvertFromSysUnits(station,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);

   CComPtr<IStation> objStation;
   objStation.CoCreateInstance(CLSID_Station);
   objStation->put_Value(station);
   CComBSTR bstrStation;
   objStation->AsString(unit_mode,&bstrStation);
   SetValueRange(CGXRange(nRow,1),CString(bstrStation));

   SetValueRange(CGXRange(nRow,2),data.Left);
   SetValueRange(CGXRange(nRow,3),data.Right);

   CString strCPO = GetCrownPointOffset(data.CrownPointOffset);
   SetValueRange(CGXRange(nRow,4), strCPO);

   GetParam()->EnableUndo(TRUE);
}

bool CCrownSlopeGrid::GetRowData(ROWCOL nRow,Float64* pStation,Float64* pLeft,Float64* pRight,Float64* pCPO)
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

   Float64 station_value;
   station->get_Value(&station_value);
   station_value = ::ConvertToSysUnits(station_value,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);
   *pStation = station_value;

   CString strLeft = GetCellValue(nRow,2);
   *pLeft = _tstof(strLeft);

   CString strRight = GetCellValue(nRow,3);
   *pRight = _tstof(strRight);

   CString strCPO = GetCellValue(nRow,4);
   *pCPO = GetCrownPointOffset(strCPO);

   return true;
}

void CCrownSlopeGrid::SetCrownSlopeData(std::vector<CrownData2>& curves)
{
   GetParam()->EnableUndo(FALSE);

   // empty the grid
   if ( 0 < GetRowCount() )
      CGXGridWnd::RemoveRows(1,GetRowCount());

   // now fill it up
   std::vector<CrownData2>::iterator iter;
   for ( iter = curves.begin(); iter != curves.end(); iter++ )
   {
      CrownData2& curve_data = *iter;
      AppendRow();
      SetRowData(GetRowCount(),curve_data);
   }

   GetParam()->EnableUndo(TRUE);
}

bool CCrownSlopeGrid::GetCrownSlopeData(std::vector<CrownData2>& curves)
{
   curves.clear();
   ROWCOL nRows = GetRowCount();
   for (ROWCOL row = 1; row <= nRows; row++ )
   {
      CrownData2 curve_data;
      if ( !GetRowData(row,&curve_data.Station,&curve_data.Left,&curve_data.Right,&curve_data.CrownPointOffset) )
         return false;

      curves.push_back(curve_data);
   }

   return true;
}

bool SortByStation(const CrownData2& c1,const CrownData2& c2)
{
   return c1.Station < c2.Station;
}

void CCrownSlopeGrid::SortCrossSections()
{
   std::vector<CrownData2> curves;
   
   if (GetCrownSlopeData(curves))
   {
      std::sort(curves.begin(),curves.end(),SortByStation);
      SetCrownSlopeData(curves);
   }
}

Float64 CCrownSlopeGrid::GetCrownPointOffset(const CString& strAlignmentOffset)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   Float64 sign = 1;
   TCHAR cDir = strAlignmentOffset.GetAt(strAlignmentOffset.GetLength()-1);
   if ( cDir == _T('L') )
      sign = -1;
   else if ( cDir == _T('R') )
      sign = 1;

   Float64 value = _tstof(strAlignmentOffset);
   value *= sign;

   value = ::ConvertToSysUnits(value,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);

   return value;
}

CString CCrownSlopeGrid::GetCrownPointOffset(Float64 alignmentOffset)
{
   int sign = Sign(alignmentOffset);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CString strAlignmentOffset;
   if ( sign == 0 )
      strAlignmentOffset.Format(_T("%s"),FormatDimension(alignmentOffset,pDisplayUnits->GetAlignmentLengthUnit(),false));
   else if ( sign < 0 )
      strAlignmentOffset.Format(_T("%s %c"),FormatDimension(fabs(alignmentOffset),pDisplayUnits->GetAlignmentLengthUnit(),false),_T('L'));
   else
      strAlignmentOffset.Format(_T("%s %c"),FormatDimension(fabs(alignmentOffset),pDisplayUnits->GetAlignmentLengthUnit(),false),_T('R'));

   return strAlignmentOffset;
}
