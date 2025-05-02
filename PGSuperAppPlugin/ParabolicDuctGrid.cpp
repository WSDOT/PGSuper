///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// ParabolicDuctGrid.cpp : implementation file
//

#include "stdafx.h"
#include "ParabolicDuctGrid.h"
#include "ParabolicDuctDlg.h"

#include <IFace/Tools.h>
#include <EAF\EAFDisplayUnits.h>
#include "PGSuperUnits.h"


GRID_IMPLEMENT_REGISTER(CParabolicDuctGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CParabolicDuctGrid

CParabolicDuctGrid::CParabolicDuctGrid()
{
//   RegisterClass();
   m_pCallback = nullptr;
}

CParabolicDuctGrid::~CParabolicDuctGrid()
{
}

BEGIN_MESSAGE_MAP(CParabolicDuctGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CLinearDuctGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


int CParabolicDuctGrid::GetColWidth(ROWCOL nCol)
{
	CRect rect = GetGridRect( );
   if ( 0 <= nCol && nCol <= 3 )
   {
      return rect.Width()/8;
   }
   else
   {
      return rect.Width()/4;
   }
   //return CGXGridWnd::GetColWidth(nCol);
}

void CParabolicDuctGrid::CustomInit(CParabolicDuctGridCallback* pCallback)
{
   m_pCallback = pCallback;

   
   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
	this->Initialize( );

	this->GetParam( )->EnableUndo(FALSE);

   const int num_rows=0;
   const int num_cols=5;

	this->SetRowCount(num_rows);
	this->SetColCount(num_cols);

   // Turn off selecting whole columns when clicking on a column header
	this->GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELROW & ~GX_SELTABLE));

   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   // no row moving
	this->GetParam()->EnableMoveRows(FALSE);

   ROWCOL col = 0;

   // disable left side
	this->SetStyleRange(CGXRange(0,col,num_rows,col++), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetValue(_T("Point"))
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		);

	this->SetStyleRange(CGXRange(0,col,num_rows,col++), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetValue(_T("Point"))
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		);

   // set text along top row
	this->SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetValue(_T("Location"))
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		);

	this->SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Location"))
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		);

   CString strOffset;
   strOffset.Format(_T("Offset\n(%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strOffset)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
 	);

   this->SetStyleRange(CGXRange(0,col++), CGXStyle()
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

void CParabolicDuctGrid::SetTendonRange(PierIndexType startPierIdx, PierIndexType endPierIdx)
{
   m_DuctGeometry.SetRange(startPierIdx, endPierIdx);
   SetData(m_DuctGeometry);
}

void CParabolicDuctGrid::GetTendonRange(PierIndexType* pStartPierIdx, PierIndexType* pEndPierIdx) const
{
   m_DuctGeometry.GetRange(pStartPierIdx, pEndPierIdx);
}

CParabolicDuctGeometry CParabolicDuctGrid::GetData()
{
   PierIndexType startPierIdx, endPierIdx;
   m_DuctGeometry.GetRange(&startPierIdx, &endPierIdx);

   SpanIndexType startSpanIdx = startPierIdx;
   SpanIndexType endSpanIdx = endPierIdx-1;

   ROWCOL row = 1;
   Float64 distance,offset;
   CDuctGeometry::OffsetType offsetType;
   GetRowValues(row++,&distance,&offset,&offsetType);
   m_DuctGeometry.SetStartPoint(startPierIdx,distance,offset,offsetType);

   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      GetRowValues(row++,&distance,&offset,&offsetType);
      m_DuctGeometry.SetLowPoint(spanIdx,distance,offset,offsetType);

      if ( spanIdx < endSpanIdx )
      {
         PierIndexType pierIdx = spanIdx+1;
         Float64 distLeftIP;
         Float64 dummyOffset;
         CDuctGeometry::OffsetType dummyOffsetType;
         GetRowValues(row++,&distLeftIP,&dummyOffset,&dummyOffsetType);

         Float64 x;
         Float64 highOffset;
         CDuctGeometry::OffsetType highOffsetType;
         GetRowValues(row++,&x,&highOffset,&highOffsetType);
         
         Float64 distRightIP;
         GetRowValues(row++,&distRightIP,&dummyOffset,&dummyOffsetType);

         m_DuctGeometry.SetHighPoint(pierIdx,distLeftIP,highOffset,highOffsetType,distRightIP);
      }
   }

   GetRowValues(row++,&distance,&offset,&offsetType);
   m_DuctGeometry.SetEndPoint(endPierIdx,distance,offset,offsetType);

   return m_DuctGeometry;
}

void CParabolicDuctGrid::SetData(const CParabolicDuctGeometry& ductGeometry)
{
   ROWCOL nRows = GetRowCount();
   if (0 < nRows)
   {
      // remove old data if there is any
      RemoveRows(1, nRows);
   }

   m_DuctGeometry = ductGeometry;

   PierIndexType startPierIdx, endPierIdx;
   ductGeometry.GetRange(&startPierIdx, &endPierIdx);

   SpanIndexType startSpanIdx = startPierIdx;
   SpanIndexType endSpanIdx   = endPierIdx-1;

   // Start point
   PierIndexType pierIdx;
   Float64 distance,offset;
   CDuctGeometry::OffsetType offsetType;
   ductGeometry.GetStartPoint(&pierIdx,&distance,&offset,&offsetType);
   InsertFirstPoint(startSpanIdx,distance,offset,offsetType);

   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      ductGeometry.GetLowPoint(spanIdx,&distance,&offset,&offsetType);
      InsertLowPoint(spanIdx,distance,offset,offsetType);
      
      if ( spanIdx < endSpanIdx )
      {
         PierIndexType pierIdx = spanIdx+1;
         Float64 distLeftIP;
         
         Float64 highOffset;
         CDuctGeometry::OffsetType highOffsetType;
         
         Float64 distRightIP;

         ductGeometry.GetHighPoint(pierIdx,&distLeftIP,&highOffset,&highOffsetType,&distRightIP);

         InsertInflectionPoint(spanIdx,distLeftIP);
         InsertHighPoint(pierIdx,highOffset,highOffsetType);
         InsertInflectionPoint(spanIdx+1,distRightIP);
      }
   }

   ductGeometry.GetEndPoint(&pierIdx,&distance,&offset,&offsetType);
   InsertLastPoint(endSpanIdx,distance,offset,offsetType);
}

void CParabolicDuctGrid::InsertFirstPoint(SpanIndexType spanIdx,Float64 distance,Float64 offset,CDuctGeometry::OffsetType offsetType)
{
   
   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   ROWCOL row = GetRowCount()+1;
   InsertRows(row,1);

   CString strSpanLabel;
   strSpanLabel.Format(_T("Span %s"),LABEL_SPAN(spanIdx));
   SetStyleRange(CGXRange(row,0),CGXStyle()
      .SetValue(strSpanLabel)
      .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
      .SetHorizontalAlignment(DT_LEFT)
      .SetVerticalAlignment(DT_VCENTER)
      .SetEnabled(FALSE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         );


   SetStyleRange(CGXRange(row,1), CGXStyle()
      .SetValue(_T("Start"))
      .SetEnabled(FALSE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
      );

   int choice = 0;
   if (distance < 0)
   {
      choice = 1;
      distance = -100 * distance;
   }
   else
   {
      choice = 0;
      distance = WBFL::Units::ConvertFromSysUnits(distance, pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   }

   SetStyleRange(CGXRange(row,2), CGXStyle()
      .SetValue(distance)
      );

   CString strMeasure;
   strMeasure.Format(_T("%s\n%s"), pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str(), _T("%"));
   SetStyleRange(CGXRange(row, 3), CGXStyle()
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(strMeasure)
      .SetValue((long)choice)
   );

   offset = WBFL::Units::ConvertFromSysUnits(offset,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   SetStyleRange(CGXRange(row,4), CGXStyle()
      .SetValue(offset)
      );

   SetStyleRange(CGXRange(row,5), CGXStyle()
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
   	.SetChoiceList(_T("Bottom Girder\nTop Girder"))
      .SetValue((long)offsetType)
      );
}

void CParabolicDuctGrid::InsertLowPoint(SpanIndexType spanIdx,Float64 distance,Float64 offset,CDuctGeometry::OffsetType offsetType)
{
   
   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   ROWCOL row = GetRowCount()+1;

   InsertRows(row,1);

   CString strSpanLabel;
   strSpanLabel.Format(_T("Span %s"),LABEL_SPAN(spanIdx));
   SetStyleRange(CGXRange(row,0),CGXStyle()
      .SetValue(strSpanLabel)
      .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
      .SetHorizontalAlignment(DT_LEFT)
      .SetVerticalAlignment(DT_VCENTER)
      .SetEnabled(FALSE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
      );


   SetStyleRange(CGXRange(row,1), CGXStyle()
      .SetValue(_T("Low"))
      .SetEnabled(FALSE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
      );

   int choice = 0;
   if ( distance < 0 )
   {
      choice = 1;
      distance = -100*distance;
   }
   else
   {
      choice = 0;
      distance = WBFL::Units::ConvertFromSysUnits(distance,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   }

   SetStyleRange(CGXRange(row,2), CGXStyle()
      .SetValue(distance)
      );

   CString strMeasure;
   strMeasure.Format(_T("%s\n%s"),pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str(),_T("%"));
   SetStyleRange(CGXRange(row,3), CGXStyle()
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
   	.SetChoiceList(strMeasure)
      .SetValue((long)choice)
      );


   offset = WBFL::Units::ConvertFromSysUnits(offset,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   SetStyleRange(CGXRange(row,4), CGXStyle()
      .SetValue(offset)
      );

   SetStyleRange(CGXRange(row,5), CGXStyle()
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
   	.SetChoiceList(_T("Bottom Girder\nTop Girder"))
      .SetValue((long)offsetType)
      );
}

void CParabolicDuctGrid::InsertInflectionPoint(SpanIndexType spanIdx,Float64 distance)
{
   
   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   ROWCOL row = GetRowCount()+1;

   InsertRows(row,1);

   CString strSpanLabel;
   strSpanLabel.Format(_T("Span %s"),LABEL_SPAN(spanIdx));
   SetStyleRange(CGXRange(row,0),CGXStyle()
      .SetValue(strSpanLabel)
      .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
      .SetHorizontalAlignment(DT_LEFT)
      .SetVerticalAlignment(DT_VCENTER)
      .SetEnabled(FALSE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
      );

   SetStyleRange(CGXRange(row,1), CGXStyle()
      .SetValue(_T("IP"))
      .SetEnabled(FALSE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
      );

   int choice = 0;
   if ( distance < 0 )
   {
      choice = 1;
      distance = -100*distance;
   }
   else
   {
      choice = 0;
      distance = WBFL::Units::ConvertFromSysUnits(distance,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   }

   SetStyleRange(CGXRange(row,2), CGXStyle()
      .SetValue(distance)
      );

   CString strMeasure;
   strMeasure.Format(_T("%s\n%s"),pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str(),_T("%"));
   SetStyleRange(CGXRange(row,3), CGXStyle()
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
   	.SetChoiceList(strMeasure)
      .SetValue((long)choice)
      );

   SetStyleRange(CGXRange(row,4), CGXStyle()
      .SetEnabled(FALSE)
      .SetReadOnly(TRUE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
      );

   SetStyleRange(CGXRange(row,5), CGXStyle()
      .SetEnabled(FALSE)
      .SetReadOnly(TRUE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
      );
}

void CParabolicDuctGrid::InsertHighPoint(PierIndexType pierIdx,Float64 offset,CDuctGeometry::OffsetType offsetType)
{
   
   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   ROWCOL row = GetRowCount()+1;

   InsertRows(row,1);


   CString strPierLabel;
   strPierLabel.Format(_T("Pier %s"),LABEL_PIER(pierIdx));
   SetStyleRange(CGXRange(row,0), CGXStyle()
      .SetValue(strPierLabel)
      .SetHorizontalAlignment(DT_LEFT)
      .SetVerticalAlignment(DT_VCENTER)
      .SetEnabled(FALSE)
      .SetReadOnly(TRUE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
      );

   SetStyleRange(CGXRange(row,1), CGXStyle()
      .SetValue(_T("High"))
      .SetEnabled(FALSE)
      .SetReadOnly(TRUE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
      );

   SetStyleRange(CGXRange(row,2), CGXStyle()
      .SetEnabled(FALSE)
      .SetReadOnly(TRUE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
   );

   SetStyleRange(CGXRange(row,3), CGXStyle()
      .SetEnabled(FALSE)
      .SetReadOnly(TRUE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
   );

   offset = WBFL::Units::ConvertFromSysUnits(offset,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   SetStyleRange(CGXRange(row,4), CGXStyle()
      .SetValue(offset)
      );

   SetStyleRange(CGXRange(row,5), CGXStyle()
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
   	.SetChoiceList(_T("Bottom Girder\nTop Girder"))
      .SetValue((long)offsetType)
      );
}

void CParabolicDuctGrid::InsertLastPoint(SpanIndexType spanIdx,Float64 distance,Float64 offset,CDuctGeometry::OffsetType offsetType)
{
   
   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   ROWCOL row = GetRowCount()+1;

   InsertRows(row,1);

   CString strSpanLabel;
   strSpanLabel.Format(_T("Span %s"),LABEL_SPAN(spanIdx));
   SetStyleRange(CGXRange(row,0),CGXStyle()
      .SetValue(strSpanLabel)
      .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
      .SetHorizontalAlignment(DT_LEFT)
      .SetVerticalAlignment(DT_VCENTER)
      .SetEnabled(FALSE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
      );

   SetStyleRange(CGXRange(row,1), CGXStyle()
      .SetValue(_T("End"))
      .SetEnabled(FALSE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
      );

   int choice = 0;
   if (distance < 0)
   {
      choice = 1;
      distance = -100 * distance;
   }
   else
   {
      choice = 0;
      distance = WBFL::Units::ConvertFromSysUnits(distance, pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   }

   SetStyleRange(CGXRange(row, 2), CGXStyle()
      .SetValue(distance)
   );

   CString strMeasure;
   strMeasure.Format(_T("%s\n%s"), pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str(), _T("%"));
   SetStyleRange(CGXRange(row, 3), CGXStyle()
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(strMeasure)
      .SetValue((long)choice)
   );

   offset = WBFL::Units::ConvertFromSysUnits(offset,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   SetStyleRange(CGXRange(row,4), CGXStyle()
      .SetValue(offset)
      );

   SetStyleRange(CGXRange(row,5), CGXStyle()
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
   	.SetChoiceList(_T("Bottom Girder\nTop Girder"))
      .SetValue((long)offsetType)
      );
}

CString CParabolicDuctGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

BOOL CParabolicDuctGrid::OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   return TRUE;
}

void CParabolicDuctGrid::GetRowValues(ROWCOL row,Float64* pDistance,Float64* pOffset,CDuctGeometry::OffsetType* pOffsetType)
{
   
   auto pBroker = EAFGetBroker();
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   Float64 distance = _tstof(GetCellValue(row,2));

   long locationType = _tstol(GetCellValue(row,3));
   if ( locationType == 0 )
   {
      distance = WBFL::Units::ConvertToSysUnits(distance,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   }
   else
   {
      distance /= -100; // fraction
   }

   Float64 offset = _tstof(GetCellValue(row,4));
   offset = WBFL::Units::ConvertToSysUnits(offset,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

   CDuctGeometry::OffsetType offsetType = CDuctGeometry::OffsetType(_tstoi(GetCellValue(row,5)));

   *pDistance   = distance;
   *pOffset     = offset;
   *pOffsetType = offsetType;
}

void CParabolicDuctGrid::OnModifyCell(ROWCOL nRow, ROWCOL nCol)
{
   if (m_pCallback)
   {
      m_pCallback->OnDuctChanged();
   }

   __super::OnModifyCell(nRow, nCol);
}
