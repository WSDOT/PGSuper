///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// DebondGrid.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperDoc.h"
#include "resource.h"
#include "StrandGrid.h"
#include "GirderSegmentStrandsPage.h"
#include "PGSuperUnits.h"

#include <system\tokenizer.h>

#include <GenericBridge\Helpers.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ERROR_SUCCESS                                 0
#define ERROR_Y_MUST_BE_POSITIVE                      1
#define ERROR_DEBOND_LENGTH_MORE_THAN_SEGMENT         2
#define ERROR_LEFT_DEBOND_LENGTH_MUST_BE_POSITIVE     3
#define ERROR_RIGHT_DEBOND_LENGTH_MUST_BE_POSITIVE    4

#define ERROR_STRAND_COUNT                            5
#define ERROR_S1_MUST_BE_ZERO                         6
#define ERROR_S1_MUST_BE_POSITIVE                     7
#define ERROR_S2_MUST_BE_POSITIVE                     8

/////////////////////////////////////////////////////////////////////////////
// CStrandGrid

CStrandGrid::CStrandGrid()
{
//   RegisterClass();
}

CStrandGrid::~CStrandGrid()
{
}

BEGIN_MESSAGE_MAP(CStrandGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CStrandGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CStrandGrid message handlers
//
int CStrandGrid::GetColWidth(ROWCOL nCol)
{
   if ( nCol == GetLeftDebondCheckCol() || nCol == GetRightDebondCheckCol() )
   {
      return 15; // debond left/right check boxes
   }

   return CGXGridWnd::GetColWidth(nCol);
}

void CStrandGrid::CustomInit(const CPrecastSegmentData* pSegment)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   // we need the segment length for validating debond lengths (can't debond more
   // length than the segment)
   GET_IFACE2(pBroker, IBridge,pBridge);
   m_SegmentLength = pBridge->GetSegmentLength(pSegment->GetSegmentKey());

   // Initialize the grid. For CWnd based grids this call is essential. 
	Initialize( );

   // we want to merge cells
   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   const int num_rows = 1;
   const int num_cols = GetSpecializedColCount() + 15;

   SetRowCount(num_rows);
	SetColCount(num_cols);

	GetParam( )->EnableUndo(FALSE);

	// Turn off selecting whole columns when clicking on a column header (also turns on whole row selection)
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   SetFrozenRows(1/*# frozen rows*/,1/*# extra header rows*/);

   ROWCOL col = 0;

   // set text along top row
	SetStyleRange(CGXRange(0,col,1,col++), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
			.SetValue(GetRowLabelHeading())
		);

	SetStyleRange(CGXRange(0,col,1,col++), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
         .SetWrapText(TRUE)
         .SetValue(_T("Type"))
		);

   col = InitSpecializedColumns(col,pDisplayUnits);

   SetStyleRange(CGXRange(0,col,0,col+1), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(_T("Left End"))
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      );

   CString strLabel;
   strLabel.Format(_T("Y (%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(1,col++), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

	SetStyleRange(CGXRange(1,col++), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
   	.SetValue(_T("Face"))
		);

   SetStyleRange(CGXRange(0,col,0,col+1), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(_T("Left Harp Pt"))
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      );

   strLabel.Format(_T("Y (%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(1,col++), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

	SetStyleRange(CGXRange(1,col++), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
   	.SetValue(_T("Face"))
		);

   SetStyleRange(CGXRange(0,col,0,col+1), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(_T("Right Harp Pt"))
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      );

   strLabel.Format(_T("Y (%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(1,col++), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

	SetStyleRange(CGXRange(1,col++), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
   	.SetValue(_T("Face"))
		);

   SetStyleRange(CGXRange(0,col,0,col+1), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(_T("Right End"))
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      );

	SetStyleRange(CGXRange(1,col++), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

	SetStyleRange(CGXRange(1,col++), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(_T("Face"))
		);

   SetStyleRange(CGXRange(0,col,0,col+1), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(_T("Ext. Strands"))
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      );

   SetStyleRange(CGXRange(1,col++), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Left"))
		);

	SetStyleRange(CGXRange(1,col++), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Right"))
		);


   SetStyleRange(CGXRange(0,col,0,col+3), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(_T("Debond"))
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      );

   strLabel.Format(_T("Left (%s)"),pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(1,col,1,col+1), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(strLabel)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		);
   col += 2;

   strLabel.Format(_T("Right (%s)"),pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(1,col,1,col+1), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(strLabel)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		);
   col += 2;

   // make text fit correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();
	SetFocus();

	GetParam( )->EnableUndo(TRUE);
}

void CStrandGrid::SetRowStyle(ROWCOL nRow)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   ROWCOL col = 0;

   // Row number
   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(nRow-1)
      );

   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(_T("Straight\nHarped\nTemporary"))
      .SetValue(0L)
      );

   col = SetSpecializedColumnStyles(nRow, col);

   // Left End
   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetControl(GX_IDS_CTRL_EDIT)
      .SetHorizontalAlignment(DT_RIGHT));

   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(_T("Top\nBottom"))
      .SetValue(0L)
      );

   // Left Harp Point
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetHorizontalAlignment(DT_RIGHT));

   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(_T("Top\nBottom"))
      .SetValue(0L)
      );

   // Right Harp Point
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetHorizontalAlignment(DT_RIGHT));
   
   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(_T("Top\nBottom"))
      .SetValue(0L)
      );

   // Right End
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetHorizontalAlignment(DT_RIGHT));

   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(_T("Top\nBottom"))
      .SetValue(0L)
      );

   // Extended Strands
   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetControl(GX_IDS_CTRL_CHECKBOX3D)
      );

   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetControl(GX_IDS_CTRL_CHECKBOX3D)
      );

   // Debond
   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetControl(GX_IDS_CTRL_CHECKBOX3D)
      );

   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetHorizontalAlignment(DT_RIGHT));

   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetControl(GX_IDS_CTRL_CHECKBOX3D)
      );

   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetHorizontalAlignment(DT_RIGHT));
}

CString CStrandGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

CStrandRow CStrandGrid::GetStrandRow(ROWCOL nRow)
{
   CStrandRow strandRow;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   ROWCOL col = 1;

   strandRow.m_StrandType = (pgsTypes::StrandType)(_tstoi(GetCellValue(nRow,col++)));

   col = GetSpecializedColumnValues(nRow, col, strandRow, pDisplayUnits);

   // Left End
   Float64 value = _tstof(GetCellValue(nRow,col++));
   strandRow.m_Y[ZoneBreakType::Start] = WBFL::Units::ConvertToSysUnits(value,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   strandRow.m_Face[ZoneBreakType::Start] = (pgsTypes::FaceType)(_tstoi(GetCellValue(nRow,col++)));

   // Left Harp Point
   value = _tstof(GetCellValue(nRow,col++));
   strandRow.m_Y[ZoneBreakType::LeftBreak] = WBFL::Units::ConvertToSysUnits(value,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   strandRow.m_Face[ZoneBreakType::LeftBreak] = (pgsTypes::FaceType)(_tstoi(GetCellValue(nRow,col++)));

   // Right Harp Point
   value = _tstof(GetCellValue(nRow,col++));
   strandRow.m_Y[ZoneBreakType::RightBreak] = WBFL::Units::ConvertToSysUnits(value,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   strandRow.m_Face[ZoneBreakType::RightBreak] = (pgsTypes::FaceType)(_tstoi(GetCellValue(nRow,col++)));

   // Right End
   value = _tstof(GetCellValue(nRow,col++));
   strandRow.m_Y[ZoneBreakType::End] = WBFL::Units::ConvertToSysUnits(value,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   strandRow.m_Face[ZoneBreakType::End] = (pgsTypes::FaceType)(_tstoi(GetCellValue(nRow,col++)));

   strandRow.m_bIsExtendedStrand[pgsTypes::metStart] = GetCellValue(nRow,col++) == _T("1") ? true : false;
   strandRow.m_bIsExtendedStrand[pgsTypes::metEnd]   = GetCellValue(nRow,col++) == _T("1") ? true : false;

   strandRow.m_bIsDebonded[pgsTypes::metStart] = GetCellValue(nRow,col++) == _T("1") ? true : false;
   value = _tstof(GetCellValue(nRow,col++));
   strandRow.m_DebondLength[pgsTypes::metStart] = WBFL::Units::ConvertToSysUnits(value,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);

   strandRow.m_bIsDebonded[pgsTypes::metEnd]   = GetCellValue(nRow,col++) == _T("1") ? true : false;
   value = _tstof(GetCellValue(nRow,col++));
   strandRow.m_DebondLength[pgsTypes::metEnd] = WBFL::Units::ConvertToSysUnits(value,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);

   return strandRow;
}

void CStrandGrid::AppendRow(const CStrandRow& strandRow)
{
   // Appends a row to the grid and initializes it with data from strandRow
   // DOES NOT APPEND A STRAND ROW TO THE STRAND DATA OBJECT
   ROWCOL nRow = AppendRow();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   ROWCOL col = 1;

   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue((LONG)strandRow.m_StrandType));

   col = AppendSpecializedColumnValues(nRow, col, strandRow, pDisplayUnits);

   // Left End
   CString strValue;
   strValue.Format(_T("%s"),::FormatDimension(strandRow.m_Y[ZoneBreakType::Start],pDisplayUnits->GetComponentDimUnit(),false));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strValue));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue((LONG)(strandRow.m_Face[ZoneBreakType::Start])));

   // Left Harp Point
   strValue.Format(_T("%s"),::FormatDimension(strandRow.m_Y[ZoneBreakType::LeftBreak],pDisplayUnits->GetComponentDimUnit(),false));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strValue));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue((LONG)(strandRow.m_Face[ZoneBreakType::LeftBreak])));

   // Right Harp Point
   strValue.Format(_T("%s"),::FormatDimension(strandRow.m_Y[ZoneBreakType::RightBreak],pDisplayUnits->GetComponentDimUnit(),false));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strValue));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue((LONG)(strandRow.m_Face[ZoneBreakType::RightBreak])));

   // Right End
   strValue.Format(_T("%s"),::FormatDimension(strandRow.m_Y[ZoneBreakType::End],pDisplayUnits->GetComponentDimUnit(),false));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strValue));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue((LONG)(strandRow.m_Face[ZoneBreakType::End])));

   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strandRow.m_bIsExtendedStrand[pgsTypes::metStart] ? 1L : 0L));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strandRow.m_bIsExtendedStrand[pgsTypes::metEnd] ? 1L : 0L));

   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strandRow.m_bIsDebonded[pgsTypes::metStart] ? 1L : 0L));
   Float64 debondLength = (strandRow.m_bIsDebonded[pgsTypes::metStart] ? strandRow.m_DebondLength[pgsTypes::metStart] : 0);
   strValue.Format(_T("%s"),::FormatDimension(debondLength,pDisplayUnits->GetSpanLengthUnit(),false));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strValue));

   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strandRow.m_bIsDebonded[pgsTypes::metEnd] ? 1L : 0L));
   debondLength = (strandRow.m_bIsDebonded[pgsTypes::metEnd] ? strandRow.m_DebondLength[pgsTypes::metEnd] : 0);
   strValue.Format(_T("%s"),::FormatDimension(debondLength,pDisplayUnits->GetSpanLengthUnit(),false));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strValue));

   ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));

   UpdateExtendedStrandProperties(nRow);
}

ROWCOL CStrandGrid::AppendRow()
{
	ROWCOL nRow = GetRowCount()+1;
	InsertRows(nRow, 1);
   SetRowStyle(nRow);
   return nRow;
}

void CStrandGrid::OnAddRow()
{
   CStrandRow strandRow;
   AppendRow(strandRow);

   OnChangedSelection(nullptr,FALSE,FALSE);

   CGirderSegmentStrandsPage* pParent = (CGirderSegmentStrandsPage*)GetParent();
   ASSERT(pParent);
   pParent->OnChange();
}

void CStrandGrid::OnRemoveSelectedRows()
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

   // for some reason the row headers wont redraw
   // I think it has to do with cell merging
   // reset the values manually
   ROWCOL nRows = GetRowCount();
   for (ROWCOL row = 2; row <= nRows; row++)
   {
      SetValueRange(CGXRange(row, 0), row - 1);
   }

   OnChangedSelection(nullptr,FALSE,FALSE);

   CGirderSegmentStrandsPage* pParent = (CGirderSegmentStrandsPage*)GetParent();
   ASSERT(pParent);
   pParent->OnChange();
}

void CStrandGrid::OnChangedSelection(const CGXRange* pChangedRect,BOOL bIsDragging, BOOL bKey)
{
   CGirderSegmentStrandsPage* pParent = (CGirderSegmentStrandsPage*)GetParent();
   ASSERT(pParent);

   CRowColArray sel_rows;
   ROWCOL nSelRows = GetSelectedRows(sel_rows);
   pParent->EnableRemoveButton( 0 < nSelRows ? TRUE : FALSE );
}

void CStrandGrid::UpdateStrandData(CDataExchange* pDX,CStrandData* pStrands)
{
   if ( pDX == nullptr || pDX->m_bSaveAndValidate )
   {
      // if pDX is nullptr or if m_bSaveAndValidate is true
      // we are getting data out of the grid
      CStrandRowCollection strandRows;
      ROWCOL nRows = GetRowCount();
      for ( ROWCOL row = 2; row <= nRows; row++ )
      {
         CStrandRow strandRow = GetStrandRow(row);
         UINT result = Validate(row,strandRow);
         if ( result != ERROR_SUCCESS )
         {
            // row data isn't valid
            if ( pDX )
            {
               ShowValidationError(row,result);
               pDX->Fail();
            }
         }
         strandRows.push_back(strandRow);
      }

      pStrands->SetStrandDefinitionType(GetStrandDefinitionType());
      pStrands->SetStrandRows(strandRows);
   }
   else
   {
      FillGrid(pStrands);
   }
}

void CStrandGrid::FillGrid(CStrandData* pStrands)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   ROWCOL nRows = GetRowCount();
   if ( 2 <= nRows )
   {
      CGXGridWnd::RemoveRows(2,nRows);
   }

   const CStrandRowCollection& strandRows = pStrands->GetStrandRows();
   for( const auto& strandRow : strandRows)
   {
      AppendRow(strandRow);
   }
   ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));

   nRows = GetRowCount();

   ScrollCellInView(0 < nRows ? nRows-1 : 0, GetLeftCol());

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CStrandGrid::OnModifyCell(ROWCOL nRow,ROWCOL nCol)
{
   if (nCol != GetLeftDebondLengthCol() && nCol != GetRightDebondLengthCol())
   {
      UpdateExtendedStrandProperties(nRow);

      CGirderSegmentStrandsPage* pParent = (CGirderSegmentStrandsPage*)GetParent();
      ASSERT(pParent);
      pParent->OnChange();
   }
   else
   {
      __super::OnModifyCell(nRow, nCol);
   }
}

void CStrandGrid::OnClickedButtonRowCol(ROWCOL nRow, ROWCOL nCol)
{
   if ( nCol == GetLeftExtendCheckCol() || nCol == GetRightExtendCheckCol() || nCol == GetLeftDebondCheckCol() || nCol == GetRightDebondCheckCol())
   {
      // left extension, right extension, left debond, or right debond check box was clicked
      UpdateExtendedStrandProperties(nRow);

      CGirderSegmentStrandsPage* pParent = (CGirderSegmentStrandsPage*)GetParent();
      ASSERT(pParent);
      pParent->OnChange();
   }

   __super::OnClickedButtonRowCol(nRow, nCol);
}

BOOL CStrandGrid::OnEndEditing(ROWCOL nRow,ROWCOL nCol)
{
   CGirderSegmentStrandsPage* pParent = (CGirderSegmentStrandsPage*)GetParent();
   ASSERT(pParent);

   pParent->OnChange();
   
   return __super::OnEndEditing(nRow, nCol);
}

ROWCOL CStrandGrid::GetStrandTypeCol()
{
   return 1;
}

ROWCOL CStrandGrid::GetHarpStrandStartCol()
{
   return GetSpecializedColCount() + 4;
}

ROWCOL CStrandGrid::GetHarpStrandEndCol()
{
   return GetHarpStrandStartCol() + 3;
}

ROWCOL CStrandGrid::GetLeftExtendCheckCol()
{
   return GetHarpStrandEndCol() + 3;
}

ROWCOL CStrandGrid::GetRightExtendCheckCol()
{
   return GetLeftExtendCheckCol() + 1;
}

ROWCOL CStrandGrid::GetLeftDebondCheckCol()
{
   return GetRightExtendCheckCol() + 1;
}

ROWCOL CStrandGrid::GetLeftDebondLengthCol()
{
   return GetLeftDebondCheckCol() + 1;
}

ROWCOL CStrandGrid::GetRightDebondCheckCol()
{
   return GetLeftDebondLengthCol() + 1;
}

ROWCOL CStrandGrid::GetRightDebondLengthCol()
{
   return GetRightDebondCheckCol() + 1;
}

void CStrandGrid::UpdateExtendedStrandProperties(ROWCOL nRow)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   bool bHarpStrands = GetCellValue(nRow, GetStrandTypeCol()) == _T("1") ? true : false;
   bool bTempStrands = GetCellValue(nRow, GetStrandTypeCol()) == _T("2") ? true : false;
   bool bIsExtended[2] = {GetCellValue(nRow, GetLeftExtendCheckCol())  == _T("1") ? true : false,
                          GetCellValue(nRow, GetRightExtendCheckCol()) == _T("1") ? true : false};
   bool bIsDebonded[2] = {GetCellValue(nRow, GetLeftDebondCheckCol())  == _T("1") ? true : false,
                          GetCellValue(nRow, GetRightDebondCheckCol()) == _T("1") ? true : false};

   CGXStyle enabled_style;
   CGXStyle disabled_style;
   enabled_style.SetEnabled(TRUE)
        .SetReadOnly(FALSE)
        .SetInterior(::GetSysColor(COLOR_WINDOW))
        .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

   disabled_style.SetEnabled(FALSE)
        .SetReadOnly(TRUE)
        .SetInterior(::GetSysColor(COLOR_BTNFACE))
        .SetTextColor(::GetSysColor(COLOR_BTNFACE)); // using the same color for text as the interior of the cell effectively hides the text

   SetStyleRange(CGXRange(nRow, GetHarpStrandStartCol(),nRow, GetHarpStrandEndCol()),enabled_style); // harped strands

   SetStyleRange(CGXRange(nRow, GetLeftExtendCheckCol(),nRow, GetRightExtendCheckCol()),enabled_style); // extended strands

   // debonded strands
   SetStyleRange(CGXRange(nRow, GetLeftDebondCheckCol()),enabled_style);
   if ( bIsDebonded[pgsTypes::metStart] )
   {
      SetStyleRange(CGXRange(nRow, GetLeftDebondLengthCol()),enabled_style);
   }
   else
   {
      SetStyleRange(CGXRange(nRow, GetLeftDebondLengthCol()),disabled_style);
   }
   SetStyleRange(CGXRange(nRow, GetRightDebondCheckCol()),enabled_style);
   if ( bIsDebonded[pgsTypes::metEnd] )
   {
      SetStyleRange(CGXRange(nRow, GetRightDebondLengthCol()),enabled_style);
   }
   else
   {
      SetStyleRange(CGXRange(nRow, GetRightDebondLengthCol()),disabled_style);
   }

   if ( bHarpStrands )
   {
      // can't be extended strands
      SetStyleRange(CGXRange(nRow, GetLeftExtendCheckCol(),nRow, GetRightExtendCheckCol()),CGXStyle(disabled_style).SetValue(0L));

      // can't be debonded
      SetStyleRange(CGXRange(nRow, GetLeftDebondCheckCol()),CGXStyle(disabled_style).SetValue(0L));
      SetStyleRange(CGXRange(nRow, GetLeftDebondLengthCol()),CGXStyle(disabled_style));
      SetStyleRange(CGXRange(nRow, GetRightDebondCheckCol()),CGXStyle(disabled_style).SetValue(0L));
      SetStyleRange(CGXRange(nRow, GetRightDebondLengthCol()),CGXStyle(disabled_style));
   }
   else
   {
      // Harp point int parameters
      SetStyleRange(CGXRange(nRow, GetHarpStrandStartCol(),nRow, GetHarpStrandEndCol()),disabled_style);
   }

   if ( bTempStrands )
   {
      // can't be extended strands
      SetStyleRange(CGXRange(nRow, GetLeftExtendCheckCol(),nRow, GetRightExtendCheckCol()),CGXStyle(disabled_style).SetValue(0L));

      // can't be debonded
      SetStyleRange(CGXRange(nRow, GetLeftDebondCheckCol()),CGXStyle(disabled_style).SetValue(0L));
      SetStyleRange(CGXRange(nRow, GetLeftDebondLengthCol()),CGXStyle(disabled_style));
      SetStyleRange(CGXRange(nRow, GetRightDebondCheckCol()),CGXStyle(disabled_style).SetValue(0L));
      SetStyleRange(CGXRange(nRow, GetRightDebondLengthCol()),CGXStyle(disabled_style));
   }
   
   if ( bIsExtended[pgsTypes::metStart] )
   {
      // can't be debonded
      SetStyleRange(CGXRange(nRow, GetLeftDebondCheckCol()),CGXStyle(disabled_style).SetValue(0L));
      SetStyleRange(CGXRange(nRow, GetLeftDebondLengthCol()),CGXStyle(disabled_style));
   }
   
   if ( bIsExtended[pgsTypes::metEnd] )
   {
      // can't be debonded
      SetStyleRange(CGXRange(nRow, GetRightDebondCheckCol()),CGXStyle(disabled_style).SetValue(0L));
      SetStyleRange(CGXRange(nRow, GetRightDebondLengthCol()),CGXStyle(disabled_style));
   }

   if ( bIsDebonded[pgsTypes::metStart] )
   {
      // can't be extended
      SetStyleRange(CGXRange(nRow, GetLeftExtendCheckCol()),CGXStyle(disabled_style).SetValue(0L));
   }

   if ( bIsDebonded[pgsTypes::metEnd] )
   {
      // can't be extended
      SetStyleRange(CGXRange(nRow, GetRightExtendCheckCol()),CGXStyle(disabled_style).SetValue(0L));
   }

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

UINT CStrandGrid::Validate(ROWCOL nRow,CStrandRow& strandRow)
{

   // Check position
   if ( IsLE(strandRow.m_Y[pgsTypes::metStart],0.0,0.0) || IsLE(strandRow.m_Y[pgsTypes::metStart],0.0,0.0) )
   {
      return ERROR_Y_MUST_BE_POSITIVE;
   }

   // Check debond length
   if ( strandRow.m_bIsDebonded[pgsTypes::metStart] || strandRow.m_bIsDebonded[pgsTypes::metEnd] )
   {
      Float64 l1 = (strandRow.m_bIsDebonded[pgsTypes::metStart] ? strandRow.m_DebondLength[pgsTypes::metStart] : 0);
      Float64 l2 = (strandRow.m_bIsDebonded[pgsTypes::metEnd]   ? strandRow.m_DebondLength[pgsTypes::metEnd]   : 0);
      if ( m_SegmentLength < (l1+l2) )
      {
         return ERROR_DEBOND_LENGTH_MORE_THAN_SEGMENT;
      }

      if ( strandRow.m_bIsDebonded[pgsTypes::metStart] && ::IsLE(strandRow.m_DebondLength[pgsTypes::metStart],0.0) )
      {
         return ERROR_LEFT_DEBOND_LENGTH_MUST_BE_POSITIVE;
      }

      if ( strandRow.m_bIsDebonded[pgsTypes::metEnd] && ::IsLE(strandRow.m_DebondLength[pgsTypes::metEnd],0.0) )
      {
         return ERROR_RIGHT_DEBOND_LENGTH_MUST_BE_POSITIVE;
      }
   }

   return ERROR_SUCCESS;
}

void CStrandGrid::ShowValidationError(ROWCOL nRow,UINT iError)
{
   CString strMsg;
   switch(iError)
   {
   case ERROR_Y_MUST_BE_POSITIVE:
      strMsg.Format(_T("Row %d: Location of strand, Y, must greater than zero"),nRow-1);
      break;

   case ERROR_DEBOND_LENGTH_MORE_THAN_SEGMENT:
      strMsg.Format(_T("Row %d: Debond lengths exceed the length of the segment"),nRow-1);
      break;

   case ERROR_LEFT_DEBOND_LENGTH_MUST_BE_POSITIVE:
      strMsg.Format(_T("Row %d: Left end debond length must be greater than 0.0"),nRow-1);
      break;

   case ERROR_RIGHT_DEBOND_LENGTH_MUST_BE_POSITIVE:
      strMsg.Format(_T("Row %d: Right end debond length must be greater than 0.0"),nRow-1);
      break;
   
   default:
      ATLASSERT(false); // should never get here
      strMsg.Format(_T("Row %d: Unknown error"),nRow-1);
   }

   AfxMessageBox(strMsg);
}

/////////////////////////////////////////////////////////////////////////////
// CRowStrandGrid
ROWCOL CRowStrandGrid::InitSpecializedColumns(ROWCOL col,IEAFDisplayUnits* pDisplayUnits)
{
   SetStyleRange(CGXRange(0, col, 0, col + 1), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)          // disables usage as current cell
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      .SetValue(_T("Spacing"))
   );

   CString strLabel;
   strLabel.Format(_T("S1 (%s)"), pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(1, col++), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(strLabel)
   );

   strLabel.Format(_T("S2 (%s)"), pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(1, col++), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(strLabel)
   );

   SetStyleRange(CGXRange(0, col, 1, col++), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)          // disables usage as current cell
      .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
      .SetWrapText(TRUE)
      .SetValue(_T("#\nStrands"))
   );

   return col;
}

ROWCOL CRowStrandGrid::SetSpecializedColumnStyles(ROWCOL nRow, ROWCOL col)
{
   // Spacing
   SetStyleRange(CGXRange(nRow, col, nRow, col + 1), CGXStyle().SetHorizontalAlignment(DT_RIGHT));
   col += 2;

   // # Strands
   SetStyleRange(CGXRange(nRow, col, nRow, col++), CGXStyle()
      .SetControl(GX_IDS_CTRL_SPINEDIT)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetUserAttribute(GX_IDS_UA_VALID_MIN, 0L)
      .SetUserAttribute(GX_IDS_UA_SPINBOUND_MIN, 0L)
   );
   return col;
}

ROWCOL CRowStrandGrid::GetSpecializedColumnValues(ROWCOL nRow, ROWCOL col, CStrandRow& strandRow, IEAFDisplayUnits* pDisplayUnits)
{
   Float64 value = _tstof(GetCellValue(nRow, col++));
   strandRow.m_Z = WBFL::Units::ConvertToSysUnits(value, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

   value = _tstof(GetCellValue(nRow, col++));
   strandRow.m_Spacing = WBFL::Units::ConvertToSysUnits(value, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

   strandRow.m_nStrands = _tstoi(GetCellValue(nRow, col++));

   return col;
}

ROWCOL CRowStrandGrid::AppendSpecializedColumnValues(ROWCOL nRow, ROWCOL col, const CStrandRow& strandRow, IEAFDisplayUnits* pDisplayUnits)
{
   CString strValue;
   strValue.Format(_T("%s"), ::FormatDimension(strandRow.m_Z, pDisplayUnits->GetComponentDimUnit(), false));
   SetStyleRange(CGXRange(nRow, col++), CGXStyle().SetValue(strValue));

   strValue.Format(_T("%s"), ::FormatDimension(strandRow.m_Spacing, pDisplayUnits->GetComponentDimUnit(), false));
   SetStyleRange(CGXRange(nRow, col++), CGXStyle().SetValue(strValue));

   SetStyleRange(CGXRange(nRow, col++), CGXStyle().SetValue((LONG)strandRow.m_nStrands));

   return col;
}

UINT CRowStrandGrid::Validate(ROWCOL nRow, CStrandRow& strandRow)
{
   if (strandRow.m_nStrands == 0)
   {
      return ERROR_STRAND_COUNT;
   }

   // Check inner spacing requirements
   if (IsOdd(strandRow.m_nStrands) && !IsZero(strandRow.m_Z))
   {
      return ERROR_S1_MUST_BE_ZERO;
   }
   else if (IsEven(strandRow.m_nStrands) && strandRow.m_Z <= 0)
   {
      return ERROR_S1_MUST_BE_POSITIVE;
   }

   // Check main spacing requirement
   if (3 <= strandRow.m_nStrands && strandRow.m_Spacing <= 0)
   {
      return ERROR_S2_MUST_BE_POSITIVE;
   }

   return CStrandGrid::Validate(nRow, strandRow);
}

void CRowStrandGrid::ShowValidationError(ROWCOL nRow, UINT iError)
{
   CString strMsg;
   switch (iError)
   {
   case ERROR_STRAND_COUNT:
      strMsg.Format(_T("Row %d: Number of strands must be greater than zero"), nRow - 1);
      break;

   case ERROR_S1_MUST_BE_ZERO:
      strMsg.Format(_T("Row %d: S1 must be zero when the number of strands is odd"), nRow - 1);
      break;

   case ERROR_S1_MUST_BE_POSITIVE:
      strMsg.Format(_T("Row %d: S1 must be greater than zero when the number of strands is even"), nRow - 1);
      break;

   case ERROR_S2_MUST_BE_POSITIVE:
      strMsg.Format(_T("Row %d: S2 must be greater than zero"), nRow - 1);
      break;
   }

   if (strMsg.IsEmpty())
   {
      // not our error... try the base class
      CStrandGrid::ShowValidationError(nRow, iError);
   }
   else
   {
      AfxMessageBox(strMsg);
   }
}

/////////////////////////////////////////////////////////////////////////////
// CPointStrandGrid
ROWCOL CPointStrandGrid::InitSpecializedColumns(ROWCOL col, IEAFDisplayUnits* pDisplayUnits)
{
   CString strLabel;
   strLabel.Format(_T("Z (%s)"), pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(0, col, 1, col), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)          // disables usage as current cell
      .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
      .SetValue(strLabel)
   );

   col++;

   return col;
}

ROWCOL CPointStrandGrid::SetSpecializedColumnStyles(ROWCOL nRow, ROWCOL col)
{
   SetStyleRange(CGXRange(nRow, col++), CGXStyle().SetHorizontalAlignment(DT_RIGHT));
   return col;
}

ROWCOL CPointStrandGrid::GetSpecializedColumnValues(ROWCOL nRow, ROWCOL col, CStrandRow& strandRow, IEAFDisplayUnits* pDisplayUnits)
{
   Float64 value = _tstof(GetCellValue(nRow, col++));
   strandRow.m_Z = WBFL::Units::ConvertToSysUnits(value, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

   strandRow.m_nStrands = 1;

   return col;
}

ROWCOL CPointStrandGrid::AppendSpecializedColumnValues(ROWCOL nRow, ROWCOL col, const CStrandRow& strandRow, IEAFDisplayUnits* pDisplayUnits)
{
   CString strValue;
   strValue.Format(_T("%s"), ::FormatDimension(strandRow.m_Z, pDisplayUnits->GetComponentDimUnit(), false));
   SetStyleRange(CGXRange(nRow, col++), CGXStyle().SetValue(strValue));

   return col;
}

UINT CPointStrandGrid::Validate(ROWCOL nRow, CStrandRow& strandRow)
{
   return CStrandGrid::Validate(nRow, strandRow);
}

void CPointStrandGrid::ShowValidationError(ROWCOL nRow, UINT iError)
{
   CStrandGrid::ShowValidationError(nRow, iError);
}
