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

// DebondGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperDoc.h"
#include "PGSuperAppPlugin\resource.h"
#include "StrandGrid.h"
#include "GirderSegmentStrandsPage.h"
#include "PGSuperUnits.h"

#include <system\tokenizer.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
   if ( nCol == 11 || nCol == 13 )
      return 15; // debond left/right check boxes

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
   const int num_cols = 14;

	SetRowCount(num_rows);
	SetColCount(num_cols);

	GetParam( )->EnableUndo(FALSE);

	// Turn off selecting whole columns when clicking on a column header (also turns on whole row selection)
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   SetFrozenRows(1/*# frozen rows*/,1/*# extra header rows*/);

   // set text along top row
	SetStyleRange(CGXRange(0,0,1,0), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
			.SetValue(_T("Row"))
		);

	SetStyleRange(CGXRange(0,1,0,2), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
			.SetValue(_T("Spacing"))
		);

   CString strLabel;
   strLabel.Format(_T("S1 (%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(1,1), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(strLabel)
      );

   strLabel.Format(_T("S2 (%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(1,2), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(strLabel)
      );

	SetStyleRange(CGXRange(0,3,1,3), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
         .SetWrapText(TRUE)
         .SetValue(_T("#\nStrands"))
		);

   SetStyleRange(CGXRange(0,4,0,5), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(_T("Left End"))
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      );

   strLabel.Format(_T("Y (%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(1,4), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

	SetStyleRange(CGXRange(1,5), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
   	.SetValue(_T("Face"))
		);

   SetStyleRange(CGXRange(0,6,0,7), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(_T("Right End"))
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      );

	SetStyleRange(CGXRange(1,6), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

	SetStyleRange(CGXRange(1,7), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(_T("Face"))
		);

	SetStyleRange(CGXRange(0,8,1,8), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
      .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
         .SetWrapText(TRUE)
			.SetValue(_T("Temp.\nStrands"))
		);


   SetStyleRange(CGXRange(0,9,0,10), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(_T("Ext. Strands"))
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      );

   SetStyleRange(CGXRange(1,9), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Left"))
		);

	SetStyleRange(CGXRange(1,10), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Right"))
		);


   SetStyleRange(CGXRange(0,11,0,14), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(_T("Debond"))
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      );

   strLabel.Format(_T("Left (%s)"),pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(1,11,1,12), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(strLabel)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		);

   strLabel.Format(_T("Right (%s)"),pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(1,13,1,14), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(strLabel)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		);

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
   // Row number
   SetStyleRange(CGXRange(nRow,0),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(nRow-1)
      );

   // Spacing and # Strands
   SetStyleRange(CGXRange(nRow,1,nRow,3),CGXStyle().SetHorizontalAlignment(DT_RIGHT));

   // Location
   SetStyleRange(CGXRange(nRow,4),CGXStyle().SetHorizontalAlignment(DT_RIGHT));

   SetStyleRange(CGXRange(nRow,5),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(_T("Top\nBottom"))
      .SetValue(0L)
      );

   SetStyleRange(CGXRange(nRow,6),CGXStyle().SetHorizontalAlignment(DT_RIGHT));

   SetStyleRange(CGXRange(nRow,7),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(_T("Top\nBottom"))
      .SetValue(0L)
      );

   // Temporary Strands
   SetStyleRange(CGXRange(nRow,8),CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetControl(GX_IDS_CTRL_CHECKBOX3D)
      );

   // Extended Strands
   SetStyleRange(CGXRange(nRow,9),CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetControl(GX_IDS_CTRL_CHECKBOX3D)
      );

   SetStyleRange(CGXRange(nRow,10),CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetControl(GX_IDS_CTRL_CHECKBOX3D)
      );

   // Debond
   SetStyleRange(CGXRange(nRow,11),CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetControl(GX_IDS_CTRL_CHECKBOX3D)
      );

   SetStyleRange(CGXRange(nRow,12),CGXStyle().SetHorizontalAlignment(DT_RIGHT));

   SetStyleRange(CGXRange(nRow,13),CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetControl(GX_IDS_CTRL_CHECKBOX3D)
      );

   SetStyleRange(CGXRange(nRow,14),CGXStyle().SetHorizontalAlignment(DT_RIGHT));
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

   Float64 value = _tstof(GetCellValue(nRow,1));
   strandRow.m_InnerSpacing = ::ConvertToSysUnits(value,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

   value = _tstof(GetCellValue(nRow,2));
   strandRow.m_Spacing = ::ConvertToSysUnits(value,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

   strandRow.m_nStrands = _tstoi(GetCellValue(nRow,3));

   value = _tstof(GetCellValue(nRow,4));
   strandRow.m_Y[pgsTypes::metStart] = ::ConvertToSysUnits(value,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   strandRow.m_Face[pgsTypes::metStart] = (pgsTypes::FaceType)(_tstoi(GetCellValue(nRow,5)));

   value = _tstof(GetCellValue(nRow,6));
   strandRow.m_Y[pgsTypes::metEnd] = ::ConvertToSysUnits(value,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   strandRow.m_Face[pgsTypes::metEnd] = (pgsTypes::FaceType)(_tstoi(GetCellValue(nRow,7)));

   strandRow.m_bIsTemporaryStrand = GetCellValue(nRow,8) == _T("1") ? true : false;
   strandRow.m_bIsExtendedStrand[pgsTypes::metStart] = GetCellValue(nRow,9) == _T("1") ? true : false;
   strandRow.m_bIsExtendedStrand[pgsTypes::metEnd]   = GetCellValue(nRow,10) == _T("1") ? true : false;

   strandRow.m_bIsDebonded[pgsTypes::metStart] = GetCellValue(nRow,11) == _T("1") ? true : false;
   value = _tstof(GetCellValue(nRow,12));
   strandRow.m_DebondLength[pgsTypes::metStart] = ::ConvertToSysUnits(value,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);

   strandRow.m_bIsDebonded[pgsTypes::metEnd]   = GetCellValue(nRow,13) == _T("1") ? true : false;
   value = _tstof(GetCellValue(nRow,14));
   strandRow.m_DebondLength[pgsTypes::metEnd] = ::ConvertToSysUnits(value,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);

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

   CString strValue;
   strValue.Format(_T("%s"),::FormatDimension(strandRow.m_InnerSpacing,pDisplayUnits->GetComponentDimUnit(),false));
   SetStyleRange(CGXRange(nRow,1),CGXStyle().SetValue(strValue));

   strValue.Format(_T("%s"),::FormatDimension(strandRow.m_Spacing,pDisplayUnits->GetComponentDimUnit(),false));
   SetStyleRange(CGXRange(nRow,2),CGXStyle().SetValue(strValue));

   SetStyleRange(CGXRange(nRow,3),CGXStyle().SetValue((LONG)strandRow.m_nStrands));

   strValue.Format(_T("%s"),::FormatDimension(strandRow.m_Y[pgsTypes::metStart],pDisplayUnits->GetComponentDimUnit(),false));
   SetStyleRange(CGXRange(nRow,4),CGXStyle().SetValue(strValue));
   SetStyleRange(CGXRange(nRow,5),CGXStyle().SetValue((LONG)(strandRow.m_Face[pgsTypes::metStart])));

   strValue.Format(_T("%s"),::FormatDimension(strandRow.m_Y[pgsTypes::metEnd],pDisplayUnits->GetComponentDimUnit(),false));
   SetStyleRange(CGXRange(nRow,6),CGXStyle().SetValue(strValue));
   SetStyleRange(CGXRange(nRow,7),CGXStyle().SetValue((LONG)(strandRow.m_Face[pgsTypes::metEnd])));

   SetStyleRange(CGXRange(nRow,8),CGXStyle().SetValue(strandRow.m_bIsTemporaryStrand ? 1L : 0L));

   SetStyleRange(CGXRange(nRow, 9),CGXStyle().SetValue(strandRow.m_bIsExtendedStrand[pgsTypes::metStart] ? 1L : 0L));
   SetStyleRange(CGXRange(nRow,10),CGXStyle().SetValue(strandRow.m_bIsExtendedStrand[pgsTypes::metEnd] ? 1L : 0L));

   SetStyleRange(CGXRange(nRow,11),CGXStyle().SetValue(strandRow.m_bIsDebonded[pgsTypes::metStart] ? 1L : 0L));
   if ( strandRow.m_bIsDebonded[pgsTypes::metStart] )
   {
      strValue.Format(_T("%s"),::FormatDimension(strandRow.m_DebondLength[pgsTypes::metStart],pDisplayUnits->GetSpanLengthUnit(),false));
      SetStyleRange(CGXRange(nRow,12),CGXStyle().SetValue(strValue));
   }

   SetStyleRange(CGXRange(nRow,13),CGXStyle().SetValue(strandRow.m_bIsDebonded[pgsTypes::metEnd] ? 1L : 0L));
   if ( strandRow.m_bIsDebonded[pgsTypes::metEnd] )
   {
      strValue.Format(_T("%s"),::FormatDimension(strandRow.m_DebondLength[pgsTypes::metEnd],pDisplayUnits->GetSpanLengthUnit(),false));
      SetStyleRange(CGXRange(nRow,14),CGXStyle().SetValue(strValue));
   }

   ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));
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

   OnChangedSelection(NULL,FALSE,FALSE);
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

   OnChangedSelection(NULL,FALSE,FALSE);
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
   if ( pDX == NULL || pDX->m_bSaveAndValidate )
   {
      // if pDX is NULL or if m_bSaveAndValidate is true
      // we are getting data out of the grid
      CStrandRowCollection strandRows;
      ROWCOL nRows = GetRowCount();
      for ( ROWCOL row = 2; row <= nRows; row++ )
      {
         CStrandRow strandRow = GetStrandRow(row);

         // if pDX is not NULL we are validating data
         if ( pDX != NULL && !Validate(row,strandRow) )
         {
            // row data isn't valid
            pDX->Fail();
         }
         strandRows.push_back(strandRow);
      }

      pStrands->NumPermStrandsType = CStrandData::npsUser;
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
   if ( 2 < nRows )
   {
      CGXGridWnd::RemoveRows(2,GetRowCount());
   }

   ATLASSERT(pStrands->NumPermStrandsType == CStrandData::npsUser);

   const CStrandRowCollection& strandRows = pStrands->GetStrandRows();
   CStrandRowCollection::const_iterator iter(strandRows.begin());
   CStrandRowCollection::const_iterator iterEnd(strandRows.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const CStrandRow& strandRow(*iter);
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
   CGirderSegmentStrandsPage* pParent = (CGirderSegmentStrandsPage*)GetParent();
   ASSERT(pParent);
   pParent->OnChange();
}

void CStrandGrid::OnClickedButtonRowCol(ROWCOL nRow, ROWCOL nCol)
{
   if ( 8 <= nCol && nCol <= 10 || nCol == 11 || nCol == 13 ) 
   {
      // temporary strand, left extension, right extension, left debond, or right debond check box was clicked
      UpdateExtendedStrandProperties(nRow,nCol);

      CGirderSegmentStrandsPage* pParent = (CGirderSegmentStrandsPage*)GetParent();
      ASSERT(pParent);
      pParent->OnChange();
   }
}

BOOL CStrandGrid::OnEndEditing(ROWCOL nRow,ROWCOL nCol)
{
   CGirderSegmentStrandsPage* pParent = (CGirderSegmentStrandsPage*)GetParent();
   ASSERT(pParent);
   pParent->OnChange();
   return TRUE;
}

void CStrandGrid::UpdateExtendedStrandProperties(ROWCOL nRow, ROWCOL nCol)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CGXStyle style;
   GetStyleRowCol(nRow, nCol, style);
   CString strValue = style.GetValue();
   bool bIsChecked = (strValue == _T("1") ? true : false);

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

   if ( nCol == 8 )
   {
      // The temporary strand check box we clicked
      if ( bIsChecked )
      {
         // Temporary strands can't be extended
         SetStyleRange(CGXRange(nRow,9,nRow,10),CGXStyle(disabled_style).SetValue(0L));

         // Temporary strands can't be debonded (except for the assumed debonding)
         SetStyleRange(CGXRange(nRow,11),CGXStyle(disabled_style).SetValue(0L)); // debond check box
         SetStyleRange(CGXRange(nRow,12),disabled_style); // debond length
         SetStyleRange(CGXRange(nRow,13),CGXStyle(disabled_style).SetValue(0L)); // debond check box
         SetStyleRange(CGXRange(nRow,14),disabled_style); // debond length
      }
      else
      {
         SetStyleRange(CGXRange(nRow,9,nRow,10),enabled_style);

         SetStyleRange(CGXRange(nRow,11),enabled_style);
         SetStyleRange(CGXRange(nRow,12),enabled_style);
         SetStyleRange(CGXRange(nRow,13),enabled_style);
         SetStyleRange(CGXRange(nRow,14),enabled_style);
      }
   }

   if ( nCol == 9 || nCol == 10 )
   {
      // The extend strands button was clicked
      if ( bIsChecked )
      {
         // Temporary strands cannot be extended... disable and uncheck the box
         SetStyleRange(CGXRange(nRow,8),CGXStyle(disabled_style).SetValue(0L));

         // Extended strands cannot be deonded either
         SetStyleRange(CGXRange(nRow,(nCol == 9 ? nCol+2 : nCol+3)),CGXStyle(disabled_style).SetValue(0L)); // debond check box
         SetStyleRange(CGXRange(nRow,(nCol == 9 ? nCol+3 : nCol+4)),disabled_style); // debond length
      }
      else
      {
         CGXStyle otherStyle;
         GetStyleRowCol(nRow, (nCol == 9 ? 10 : 9), otherStyle);

         if ( otherStyle.GetValue() == _T("0") )
         {
            // enable the temporary strands check box only if the other strand extension is not checked
            SetStyleRange(CGXRange(nRow,8),enabled_style);
         }

         SetStyleRange(CGXRange(nRow,(nCol == 9 ? nCol+2 : nCol+3)),enabled_style); // debond check box
         SetStyleRange(CGXRange(nRow,(nCol == 9 ? nCol+3 : nCol+4)),enabled_style); // debond length
      }
   }

   // If debonded left/right, disable and clear check for temp and extended strands
   if ( nCol == 11 || nCol == 13 )
   {
      // The debond button was checked
      if ( bIsChecked )
      {
         // Debonded strands can't be temporary
         SetStyleRange(CGXRange(nRow,8),CGXStyle(disabled_style).SetValue(0L));

         // Debonded strands can't be extended
         SetStyleRange(CGXRange(nRow,nCol == 11 ? nCol-2 : nCol-3),CGXStyle(disabled_style).SetValue(0L));
      }
      else
      {
         CGXStyle otherStyle;
         GetStyleRowCol(nRow, (nCol == 11 ? 13 : 11), otherStyle);

         if ( otherStyle.GetValue() == _T("0") )
         {
            // enable the temporary strands check box only if the other debond is not checked
            SetStyleRange(CGXRange(nRow,8),enabled_style);
         }

         SetStyleRange(CGXRange(nRow,nCol == 11 ? nCol-2 : nCol-3),enabled_style);
      }
   }

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

bool CStrandGrid::Validate(ROWCOL nRow,CStrandRow& strandRow)
{
   if ( strandRow.m_nStrands == 0 )
   {
      CString strMsg;
      strMsg.Format(_T("Row %d: Number of strands must be greater than zero"),nRow-1);
      AfxMessageBox(strMsg);
      return false;
   }

   // Check inner spacing requirements
   if ( IsOdd(strandRow.m_nStrands) && !IsZero(strandRow.m_InnerSpacing) )
   {
      CString strMsg;
      strMsg.Format(_T("Row %d: S1 must be zero when the number of strands is odd"),nRow-1);
      AfxMessageBox(strMsg);
      return false;
   }
   else if ( IsEven(strandRow.m_nStrands) && IsLE(strandRow.m_InnerSpacing,0.0,0.0) )
   {
      CString strMsg;
      strMsg.Format(_T("Row %d: S1 must be greater than zero when the number of strands is even"),nRow-1);
      AfxMessageBox(strMsg);
      return false;
   }

   // Check main spacing requirement
   if ( 3 < strandRow.m_nStrands && IsLE(strandRow.m_Spacing,0.0,0.0) )
   {
      CString strMsg;
      strMsg.Format(_T("Row %d: S2 must be greater than zero"),nRow-1);
      AfxMessageBox(strMsg);
      return false;
   }

   // Check position
   if ( IsLE(strandRow.m_Y[pgsTypes::metStart],0.0,0.0) || IsLE(strandRow.m_Y[pgsTypes::metStart],0.0,0.0) )
   {
      CString strMsg;
      strMsg.Format(_T("Row %d: Location of strand, Y, must greater than zero"),nRow-1);
      AfxMessageBox(strMsg);
      return false;
   }

   // Check debond length
   if ( strandRow.m_bIsDebonded[pgsTypes::metStart] || strandRow.m_bIsDebonded[pgsTypes::metEnd] )
   {
      Float64 l1 = (strandRow.m_bIsDebonded[pgsTypes::metStart] ? strandRow.m_DebondLength[pgsTypes::metStart] : 0);
      Float64 l2 = (strandRow.m_bIsDebonded[pgsTypes::metEnd]   ? strandRow.m_DebondLength[pgsTypes::metEnd]   : 0);
      if ( m_SegmentLength < (l1+l2) )
      {
         CString strMsg;
         strMsg.Format(_T("Row %d: Debond lengths exceed the length of the segment"),nRow-1);
         AfxMessageBox(strMsg);
         return false;
      }
   }

   return true;
}
