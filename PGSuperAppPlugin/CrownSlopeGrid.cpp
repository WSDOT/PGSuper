///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#include "stdafx.h"
#include "CrownSlopeGrid.h"
#include "CrownSlopePage.h"
#include "PGSuperUnits.h"
#include "PGSuperDoc.h"

#include <EAF\EAFDisplayUnits.h>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// simple, exception-safe class for blocking events
class SimpleMutex
{
public:
   SimpleMutex(bool& flag):
   m_Flag(flag)
   {
      m_Flag = true;
   }

   ~SimpleMutex()
   {
      m_Flag = false;
   }
private:
   bool& m_Flag;
};

static bool stbBlockEvent = false;


GRID_IMPLEMENT_REGISTER(CCrownSlopeGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CCrownSlopeGrid
CCrownSlopeGrid::CCrownSlopeGrid():
   m_pRoadwaySectionData(nullptr)
{
}

CCrownSlopeGrid::CCrownSlopeGrid(RoadwaySectionData* pRoadwaySectionData):
   m_pRoadwaySectionData(pRoadwaySectionData)
{
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

bool CCrownSlopeGrid::AppendRow()
{
	ROWCOL nRow = GetRowCount()+1;

   if (!InsertRows(nRow, 1))
   {
      return false;
   }

   SetRowStyle(nRow);
   InitRowData(nRow);
   SetCurrentCell(nRow, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	ScrollCellInView(nRow, GetLeftCol());
	Invalidate();

   return true;
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

   // renumber rows or blast segments data if no more segments
   ROWCOL nrows = GetRowCount();
   if (nrows > 1)
   {
      for (ROWCOL row = 2; row <= nrows; row++)
      {
         SetValueRange(CGXRange(row, 0), row - 1); // row num
      }
   }
   else
   {
      m_pRoadwaySectionData->NumberOfSegmentsPerSection = 2;
      m_pRoadwaySectionData->ControllingRidgePointIdx = 1;
      m_pRoadwaySectionData->RoadwaySectionTemplates.clear();
      UpdateGridSizeAndHeaders(*m_pRoadwaySectionData);
   }
}

bool CCrownSlopeGrid::IsRowSelected()
{
	CGXRangeList* pSelList = GetParam()->GetRangeList();
   return (pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1);
}

bool CCrownSlopeGrid::IsGridEmpty()
{
   ROWCOL nrows = GetRowCount();
   return nrows == 1;
}

bool CCrownSlopeGrid::IsGridDataValid(ROWCOL * pBadRow)
{
   if (!IsGridEmpty())
   {
      ROWCOL nrows = GetRowCount();
      for (ROWCOL irow = 2; irow <= nrows; irow++)
      {
         RoadwaySectionTemplate data;
         if (!GetRowData(irow, data))
         {
            *pBadRow = irow;
            return false;
         }
      }
   }

   return true;
}

void CCrownSlopeGrid::InitRowData(ROWCOL row)
{
	GetParam()->EnableUndo(FALSE);

   ROWCOL nrows = GetRowCount();
   if (nrows > 2)
   {
      SetValueRange(CGXRange(row, 0), row - 1); // row num
      // copy data from row above
      ROWCOL cprow = max(2, row-1);
      ROWCOL ncols = GetColCount();
      for (ROWCOL icol = 1; icol <= ncols; icol++)
      {
         CString s = GetCellValue(cprow, icol);
         SetValueRange(CGXRange(row, icol), s);
      }
   }
   else
   {
      // No row to copy. just make some reasonable assumptions
      SetValueRange(CGXRange(row, 0), row - 1); // row num
      SetValueRange(CGXRange(row, 1), "0+00");
      SetValueRange(CGXRange(row, 2), "-0.02");

      ROWCOL col = 3;
      for (IndexType ir = 0; ir < m_pRoadwaySectionData->NumberOfSegmentsPerSection - 2; ir++)
      {
         SetValueRange(CGXRange(row, col++), "10.00");
         SetValueRange(CGXRange(row, col++), "-0.02");
      }

      SetValueRange(CGXRange(row, col), "-0.02");
   }

   GetParam()->EnableUndo(TRUE);
}

void CCrownSlopeGrid::CustomInit()
{
   CCrownSlopePage* pParent = (CCrownSlopePage*)GetParent();

   GET_IFACE2(pParent->GetBroker(), IEAFDisplayUnits, pDisplayUnits);
   const unitmgtLengthData& alignment_unit = pDisplayUnits->GetAlignmentLengthUnit();
   std::_tstring strUnitTag = alignment_unit.UnitOfMeasure.UnitTag();

   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
   this->Initialize();

	this->EnableIntelliMouse();
}

void CCrownSlopeGrid::UpdateGridSizeAndHeaders(const RoadwaySectionData& data)
{
   CCrownSlopePage* pParent = (CCrownSlopePage*)GetParent();
   GET_IFACE2(pParent->GetBroker(), IEAFDisplayUnits,pDisplayUnits);

	this->GetParam( )->EnableUndo(FALSE);

   const ROWCOL num_rows=1;
   const ROWCOL num_cols = 2 + (ROWCOL)data.NumberOfSegmentsPerSection*2 - 2 -1;

	this->SetRowCount(num_rows);
	this->SetColCount(num_cols);

		// Turn off selecting whole columns when clicking on a column header
	this->GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   // no row moving
	this->GetParam()->EnableMoveRows(FALSE);

   // we want to merge cells
   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   SetFrozenRows(1/*# frozen rows*/,1/*# extra header rows*/);

   ROWCOL col = 0;

   // disable left side
	this->SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
         .SetHorizontalAlignment(DT_CENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   // set text along top row
	SetStyleRange(CGXRange(0,col,1,col), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
      .SetValue(_T("Temp-\nlate"))
   );
   col++;

	SetStyleRange(CGXRange(0,col,1,col), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
			.SetValue(_T("Station"))
      .SetWrapText(FALSE)
   );
   col++;

	this->SetStyleRange(CGXRange(0,col), CGXStyle()
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetValue(_T("Segment 1"))
      .SetWrapText(FALSE)
   );

   CString strUnitTag = pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure.UnitTag().c_str();

   CString strSlope;
   strSlope.Format(_T("Slope\n(%s/%s)"),strUnitTag,strUnitTag);

   CString strLength;
   strLength.Format(_T("Length\n(%s)"),strUnitTag);

	this->SetStyleRange(CGXRange(1,col++), CGXStyle()
			.SetEnabled(FALSE)          // disables usage as current cell
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetValue(strSlope)
      .SetWrapText(TRUE)
   );
   
   IndexType ns = 2;
   for (; ns < data.NumberOfSegmentsPerSection; ns++)
   {
      CString strSegment;
      strSegment.Format(_T("Segment %d"), ns);

      SetStyleRange(CGXRange(0, col, 0, col + 1), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
         .SetEnabled(FALSE)
         .SetValue(strSegment)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
         .SetWrapText(TRUE)
      );

	   this->SetStyleRange(CGXRange(1,col++), CGXStyle()
			   .SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
         .SetValue(strLength)
         .SetWrapText(TRUE)
      );

	   this->SetStyleRange(CGXRange(1,col++), CGXStyle()
			   .SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
         .SetValue(strSlope)
         .SetWrapText(TRUE)
      );
   }

   CString strSegment;
   strSegment.Format(_T("Segment %d"), ns);

	this->SetStyleRange(CGXRange(0,col), CGXStyle()
			.SetEnabled(FALSE)          // disables usage as current cell
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetValue(strSegment)
      .SetWrapText(FALSE)
		);

	this->SetStyleRange(CGXRange(1,col), CGXStyle()
			.SetEnabled(FALSE)          // disables usage as current cell
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetValue(strSlope)
      .SetWrapText(TRUE)
   );

   // make it so that text fits correctly in header row
	this->ResizeRowHeightsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));
	this->ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));

   // don't allow users to resize grids
   this->GetParam( )->EnableTrackRowHeight(0); 

	this->SetFocus();

	this->GetParam( )->EnableUndo(TRUE);
}

void CCrownSlopeGrid::SetRowStyle(ROWCOL nRow)
{
	GetParam()->EnableUndo(FALSE);

   this->SetStyleRange(CGXRange(nRow, 0), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
   );

   ROWCOL ncols = GetColCount();
   this->SetStyleRange(CGXRange(nRow,1,nRow,ncols), CGXStyle()
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

void CCrownSlopeGrid::SetRowData(ROWCOL nRow, const RoadwaySectionTemplate& data)
{
   m_LengthCols.clear();
   m_SlopeCols.clear();

	GetParam()->EnableUndo(FALSE);

   CCrownSlopePage* pParent = (CCrownSlopePage*)GetParent();
   GET_IFACE2(pParent->GetBroker(),IEAFDisplayUnits,pDisplayUnits);
   UnitModeType unit_mode = (UnitModeType)(pDisplayUnits->GetUnitMode());

   Float64 station = data.Station;
   station = ::ConvertFromSysUnits(station,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);

   ROWCOL col = 0;
   SetValueRange(CGXRange(nRow,col++),nRow-1); // row num

   CComPtr<IStation> objStation;
   objStation.CoCreateInstance(CLSID_Station);
   objStation->put_Value(station);
   CComBSTR bstrStation;
   objStation->AsString(unit_mode,VARIANT_FALSE,&bstrStation);
   SetValueRange(CGXRange(nRow,col++),CString(bstrStation));

   m_SlopeCols.insert(col);
   SetValueRange(CGXRange(nRow,col++),data.LeftSlope);

   for (const auto& segment : data.SegmentDataVec)
   {
      Float64 length = ::ConvertFromSysUnits(segment.Length, pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);
      m_LengthCols.insert(col);
      SetValueRange(CGXRange(nRow,col++),length);
      m_SlopeCols.insert(col);
      SetValueRange(CGXRange(nRow,col++),segment.Slope);
   }

   m_SlopeCols.insert(col);
   SetValueRange(CGXRange(nRow,col),data.RightSlope);

   GetParam()->EnableUndo(TRUE);
}

bool CCrownSlopeGrid::GetRowData(ROWCOL nRow,RoadwaySectionTemplate& data)
{
   data.SegmentDataVec.clear();

   CCrownSlopePage* pParent = (CCrownSlopePage*)GetParent();

   GET_IFACE2(pParent->GetBroker(),IEAFDisplayUnits,pDisplayUnits);
   UnitModeType unit_mode = (UnitModeType)(pDisplayUnits->GetUnitMode());

   CString strStation = GetCellValue(nRow,1);
   CComPtr<IStation> station;
   station.CoCreateInstance(CLSID_Station);
   HRESULT hr = station->FromString(CComBSTR(strStation),unit_mode);
   if (FAILED(hr))
   {
      return false;
   }

   Float64 station_value;
   station->get_Value(&station_value);
   station_value = ::ConvertToSysUnits(station_value,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);
   data.Station = station_value;

   CString strVal = GetCellValue(nRow,2);
   data.LeftSlope = 0.0;
   if (!strVal.IsEmpty() && !sysTokenizer::ParseDouble(strVal, &data.LeftSlope))
	{
      return false;
	}

   ROWCOL ncols = GetColCount();
   ROWCOL col = 3;
   IndexType nsegments = (ncols - 3) / 2;
   for (IndexType ns = 0; ns < nsegments; ns++)
   {
      RoadwaySegmentData seg;
      strVal = GetCellValue(nRow,col);
      Float64 length = 0.0;
      if (!strVal.IsEmpty() && !sysTokenizer::ParseDouble(strVal, &length))
	   {
         return false;
	   }

      if (length < 0.0)
      {
         return false;
      }

      seg.Length = ::ConvertToSysUnits(length,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);

      col++;
      strVal = GetCellValue(nRow,col);
      seg.Slope = 0.0;
      if (!strVal.IsEmpty() && !sysTokenizer::ParseDouble(strVal, &seg.Slope))
	   {
         return false;
	   }

      data.SegmentDataVec.push_back(seg);

      col++;
   }

   strVal = GetCellValue(nRow,col);
   data.RightSlope = 0.0;
   if (!strVal.IsEmpty() && !sysTokenizer::ParseDouble(strVal, &data.RightSlope))
	{
      return false;
	}

   return true;
}

void CCrownSlopeGrid::InitRoadwaySectionData(bool updateHeader)
{
   // use mutex to keep grid from attempting to reselect current cell. program will crash if we don't do this.
   SimpleMutex mutex(stbBlockEvent);

   // set up our width and headers
   if (updateHeader)
   {
      UpdateGridSizeAndHeaders(*m_pRoadwaySectionData);
   }

   GetParam()->EnableUndo(FALSE);

   // empty the grid
   if ( 1 < GetRowCount() )
      CGXGridWnd::RemoveRows(2,GetRowCount());

   for(const auto& templ : m_pRoadwaySectionData->RoadwaySectionTemplates)
   {
      if (AppendRow())
      {
      SetRowData(GetRowCount(), templ);
   }
   }

   GetParam()->EnableUndo(TRUE);

	this->ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));
}

bool CCrownSlopeGrid::UpdateRoadwaySectionData()
{
   m_pRoadwaySectionData->RoadwaySectionTemplates.clear();

   ROWCOL nRows = GetRowCount();
   for (ROWCOL row = 2; row <= nRows; row++ )
   {
      RoadwaySectionTemplate curve_data;
      if ( !GetRowData(row, curve_data) )
         return false;

      m_pRoadwaySectionData->RoadwaySectionTemplates.push_back(curve_data);
   }

   return true;
}

bool SortByStation(const RoadwaySectionTemplate& c1,const RoadwaySectionTemplate& c2)
{
   return c1.Station < c2.Station;
}

bool CCrownSlopeGrid::SortCrossSections(bool doUpdateGrid)
{
   if (UpdateRoadwaySectionData())
   {
      std::sort(m_pRoadwaySectionData->RoadwaySectionTemplates.begin(),m_pRoadwaySectionData->RoadwaySectionTemplates.end(),SortByStation);

      if (doUpdateGrid)
      {
         InitRoadwaySectionData(false);
      }

      if (!m_pRoadwaySectionData->RoadwaySectionTemplates.empty())
      {
         std::vector<RoadwaySectionTemplate>::const_iterator iter = m_pRoadwaySectionData->RoadwaySectionTemplates.begin();
         Float64 sta = iter->Station;
         iter++;
         while (iter != m_pRoadwaySectionData->RoadwaySectionTemplates.end())
         {
            if (sta == iter->Station)
            {
               // can't have templates at the same station
               return false;
            }

            sta = iter->Station;
            iter++;
         }
      }

      return true;
   }

   return false;
}

BOOL CCrownSlopeGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
{
   CCrownSlopePage* pParent = (CCrownSlopePage*)GetParent();

   GET_IFACE2(pParent->GetBroker(),IEAFDisplayUnits,pDisplayUnits);
   UnitModeType unit_mode = (UnitModeType)(pDisplayUnits->GetUnitMode());

   Float64 bogus;

   if (nCol == 1)
   {
      CString strStation = GetCellValue(nRow, 1);
      CComPtr<IStation> station;
      station.CoCreateInstance(CLSID_Station);
      HRESULT hr = station->FromString(CComBSTR(strStation), unit_mode);
      if (FAILED(hr))
      {
         CString msg;
         msg.Format(_T("Invalid station data for template %d"), nRow - 1);
         SetWarningText(msg);
         return FALSE;
      }
   }
   else if (nCol == 2)
   {
      CString strVal = GetCellValue(nRow, 2);
      if (!strVal.IsEmpty() && !sysTokenizer::ParseDouble(strVal, &bogus))
      {
         CString msg;
         msg.Format(_T("Leftmost slope value not a number for template %d"), nRow - 1);
         SetWarningText(msg);
         return FALSE;
      }
   }
   else
   {
      ROWCOL numcols = GetColCount();

      if (nCol == numcols)
      {
         // rightmost column
         CString strVal = GetCellValue(nRow, nCol);
         if (!strVal.IsEmpty() && !sysTokenizer::ParseDouble(strVal, &bogus))
         {
            CString msg;
            msg.Format(_T("Rightmost slope value not a number for template %d"), nRow - 1);
            ::AfxMessageBox(msg, MB_ICONERROR | MB_OK);
            return FALSE;
         }
      }
      else
      {
         if (nCol % 2 != 0) // odd cols are length
         {
            CString strVal = GetCellValue(nRow, nCol);
            Float64 length;
            if (!strVal.IsEmpty() && !sysTokenizer::ParseDouble(strVal, &length))
            {
               CString msg;
               msg.Format(_T("Length value not a number for template %d"), nRow - 1);
               SetWarningText(msg);
               return FALSE;
            }

            if (length < 0.0)
         {
               CString msg;
               msg.Format(_T("A segment length is less than zero for template %d"), nRow - 1);
               SetWarningText(msg);
               return FALSE;
            }
         }
         else // even cols have slope
            {
            CString strVal = GetCellValue(nRow, nCol);
            if (!strVal.IsEmpty() && !sysTokenizer::ParseDouble(strVal, &bogus))
            {
               CString msg;
               msg.Format(_T("A slope value is not a number for template %d"), nRow - 1);
               SetWarningText(msg);
               return FALSE;
            }
         }
      }
   }

   return CGXGridWnd::OnValidateCell(nRow, nCol);
}

void CCrownSlopeGrid::OnModifyCell(ROWCOL nRow, ROWCOL nCol)
{
   // tell parent to draw template if we have valid data
   CCrownSlopePage* pParent = (CCrownSlopePage*)GetParent();
   pParent->OnChange(); 

   __super::OnModifyCell(nRow, nCol);
}

void CCrownSlopeGrid::OnMovedCurrentCell(ROWCOL nRow, ROWCOL nCol)
{
   if (!stbBlockEvent && nRow > 1)
   {
      // tell parent to update its graphic
      CCrownSlopePage* pParent = (CCrownSlopePage*)GetParent();
      pParent->OnGridTemplateClicked(nRow - 2);
   }
}
