///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

// SegmentTendonGrid.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperDoc.h"
#include "resource.h"
#include "SegmentTendonGrid.h"
#include "GirderSegmentTendonsPage.h"
#include "PGSuperUnits.h"

#include <system\tokenizer.h>

#include <GenericBridge\Helpers.h>

#include <IFace\Bridge.h>
#include <IFace\Allowables.h>
#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\SplicedGirderData.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ERROR_SUCCESS                                 0
#define ERROR_Y_MUST_BE_POSITIVE                      1
#define ERROR_STRAND_COUNT                            2

/////////////////////////////////////////////////////////////////////////////
// CSegmentTendonGrid

CSegmentTendonGrid::CSegmentTendonGrid()
{
//   RegisterClass();
}

CSegmentTendonGrid::~CSegmentTendonGrid()
{
}

BEGIN_MESSAGE_MAP(CSegmentTendonGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CSegmentTendonGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSegmentTendonGrid message handlers
//
int CSegmentTendonGrid::GetColWidth(ROWCOL nCol)
{
   if (nCol == nPjackUserCol)
   {
      return 0;
   }

   if (nCol == nPjackCheckCol)
   {
      return 20; // want it to just fit the check box
   }
   return CGXGridWnd::GetColWidth(nCol);
}

void CSegmentTendonGrid::CustomInit(const CPrecastSegmentData* pSegment)
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
   const int num_cols = 13;

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
			.SetValue(_T("Duct"))
		);

   nDuctTypeCol = col++;
	SetStyleRange(CGXRange(0, nDuctTypeCol,1, nDuctTypeCol), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
         .SetWrapText(TRUE)
         .SetValue(_T("Type"))
		);
	
   nDuctShapeCol = col++;
	SetStyleRange(CGXRange(0, nDuctShapeCol, 1, nDuctShapeCol), CGXStyle()
		.SetHorizontalAlignment(DT_CENTER)
		.SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
		.SetWrapText(TRUE)
		.SetValue(_T("Shape"))
	);
	
   nStrandsCol = col++;
	SetStyleRange(CGXRange(0, nStrandsCol, 1, nStrandsCol), CGXStyle()
		.SetHorizontalAlignment(DT_CENTER)
		.SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
		.SetWrapText(TRUE)
		.SetValue(_T("#\nStrands"))
	);

	// Pjack covers 2 rows and 2 columns
   nPjackCheckCol = col++;
   nPjackCol = col++;
   CString strLabel;
	strLabel.Format(_T("Pjack\n(%s)"), pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0, nPjackCheckCol, 1, nPjackCol), CGXStyle()
		.SetHorizontalAlignment(DT_CENTER)
		.SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)
		.SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		.SetWrapText(TRUE)
		.SetValue(strLabel)
	);

   // this will be a hidden cell that holds the last user input Pjack
   nPjackUserCol = col++;
   SetStyleRange(CGXRange(0, nPjackUserCol, 1, nPjackUserCol), CGXStyle()
      .SetWrapText(TRUE)
      .SetEnabled(FALSE)          // disables usage as current cell
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_VCENTER)
      .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
      .SetValue(_T("last user pjack"))
   );
   HideCols(nPjackUserCol, nPjackUserCol);
 
   nJackEndCol = col++;
   SetStyleRange(CGXRange(0, nJackEndCol, 1, nJackEndCol), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)          // disables usage as current cell
      .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
      .SetWrapText(TRUE)
      .SetValue(_T("Jacking\nEnd"))
   );

   nLeftEndYCol = col++;
   nLeftEndDatumCol = col++;
   SetStyleRange(CGXRange(0,nLeftEndYCol,0,nLeftEndDatumCol), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(_T("Left End"))
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      );

   strLabel.Format(_T("Y (%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(1,nLeftEndYCol), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

	SetStyleRange(CGXRange(1,nLeftEndDatumCol), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
   	.SetValue(_T("Face"))
		);

   nMiddleYCol = col++;
   nMiddleDatumCol = col++;
   SetStyleRange(CGXRange(0,nMiddleYCol,0,nMiddleDatumCol), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(_T("Middle"))
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      );

   strLabel.Format(_T("Y (%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(1,nMiddleYCol), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

	SetStyleRange(CGXRange(1,nMiddleDatumCol), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
   	.SetValue(_T("Face"))
		);

   nRightEndYCol = col++;
   nRightEndDatumCol = col++;
   SetStyleRange(CGXRange(0,nRightEndYCol,0,nRightEndDatumCol), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(_T("Right End"))
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      );

	SetStyleRange(CGXRange(1,nRightEndYCol), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

	SetStyleRange(CGXRange(1,nRightEndDatumCol), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(_T("Face"))
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

void CSegmentTendonGrid::SetRowStyle(ROWCOL nRow)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   // Row number
   SetStyleRange(CGXRange(nRow,0),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(nRow-1)
      );

   GET_IFACE2(pBroker, ILibraryNames, pLibNames);

   std::vector<std::_tstring> vNames;
   pLibNames->EnumDuctNames(&vNames);

   std::_tstring strDuctNames;
   std::vector<std::_tstring>::iterator begin(vNames.begin());
   std::vector<std::_tstring>::iterator iter(begin);
   std::vector<std::_tstring>::iterator end(vNames.end());
   for (; iter != end; iter++)
   {
	   if (iter != begin)
	   {
		   strDuctNames += _T("\n");
	   }
	   strDuctNames += (*iter);
   }

   SetStyleRange(CGXRange(nRow,nDuctTypeCol),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(strDuctNames.c_str())
      .SetValue(0L)
      );


   SetStyleRange(CGXRange(nRow, nDuctShapeCol), CGXStyle()
	   .SetHorizontalAlignment(DT_RIGHT)
	   .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
	   .SetChoiceList(_T("Straight\nParabolic"))
	   .SetValue(0L)
   );

   // # Strands
   SetStyleRange(CGXRange(nRow, nStrandsCol), CGXStyle()
	   .SetControl(GX_IDS_CTRL_SPINEDIT)
	   .SetHorizontalAlignment(DT_RIGHT)
   );

   // Pjack check box
   SetStyleRange(CGXRange(nRow, nPjackCheckCol), CGXStyle()
	   .SetHorizontalAlignment(DT_CENTER)
	   .SetControl(GX_IDS_CTRL_CHECKBOX3D)
   );

   // Pjack
   SetStyleRange(CGXRange(nRow, nPjackCol), CGXStyle()
      .SetReadOnly(TRUE)
      .SetControl(GX_IDS_CTRL_STATIC)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
      .SetEnabled(FALSE)
      .SetHorizontalAlignment(DT_RIGHT)
   );


   // jacking end
   SetStyleRange(CGXRange(nRow, nJackEndCol), CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(_T("Left\nRight\nBoth"))
      .SetValue(0L)
   );

   // Left End
   SetStyleRange(CGXRange(nRow,nLeftEndYCol),CGXStyle().SetControl(GX_IDS_CTRL_EDIT).SetHorizontalAlignment(DT_RIGHT));
   SetStyleRange(CGXRange(nRow,nLeftEndDatumCol),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(_T("Top\nBottom"))
      .SetValue(0L)
      );

   // Center Point
   SetStyleRange(CGXRange(nRow,nMiddleYCol),CGXStyle().SetControl(GX_IDS_CTRL_EDIT).SetHorizontalAlignment(DT_RIGHT));
   SetStyleRange(CGXRange(nRow,nMiddleDatumCol),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(_T("Top\nBottom"))
      .SetValue(0L)
      );

   // Right End
   SetStyleRange(CGXRange(nRow,nRightEndYCol),CGXStyle().SetControl(GX_IDS_CTRL_EDIT).SetHorizontalAlignment(DT_RIGHT));
   SetStyleRange(CGXRange(nRow,nRightEndDatumCol),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(_T("Top\nBottom"))
      .SetValue(0L)
      );
}

CString CSegmentTendonGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CSegmentTendonGrid::OnClickedButtonRowCol(ROWCOL nRow, ROWCOL nCol)
{
   if (nCol == 4)
   {
      // Pjack check box
      OnCalcPjack(nRow);
   }

   __super::OnClickedButtonRowCol(nRow, nCol);
}

void CSegmentTendonGrid::OnCalcPjack(ROWCOL nRow)
{
   // Calc Pj check box was clicked

   CString strCheck = GetCellValue(nRow, nPjackCheckCol);
   if (strCheck == _T("1"))
   {
      // Box was unchecked... Replace the current value with the last user input Pjack

      // First make the cell writable
      GetParam()->SetLockReadOnly(FALSE);
      this->SetStyleRange(CGXRange(nRow, nPjackCol),
         CGXStyle()
         .SetReadOnly(FALSE)
         .SetControl(GX_IDS_CTRL_EDIT)
         .SetInterior(::GetSysColor(COLOR_WINDOW))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetEnabled(TRUE)
         .SetHorizontalAlignment(DT_RIGHT)
      );
      GetParam()->SetLockReadOnly(TRUE);

      Float64 LastPjack = _tstof(GetCellValue(nRow, nPjackUserCol));
      SetValueRange(CGXRange(nRow, nPjackCol), LastPjack);
   }
   else
   {
      // Box was checked... Save the last user input for Pjack
      Float64 LastPjack = _tstof(GetCellValue(nRow, nPjackCol));
      SetValueRange(CGXRange(nRow, nPjackUserCol), LastPjack);

      // Make the cell read only
      GetParam()->SetLockReadOnly(FALSE);
      this->SetStyleRange(CGXRange(nRow, nPjackCol),
         CGXStyle()
         .SetReadOnly(TRUE)
         .SetControl(GX_IDS_CTRL_STATIC)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
         .SetEnabled(FALSE)
         .SetHorizontalAlignment(DT_RIGHT)
      );
      GetParam()->SetLockReadOnly(TRUE);

      // Set Pjack to the max value
      UpdateMaxPjack(nRow);
   }

   ResizeColWidthsToFit(CGXRange(0, 0, GetRowCount(), GetColCount()));
}

CSegmentDuctData CSegmentTendonGrid::GetDuctRow(ROWCOL nRow)
{
   CSegmentDuctData duct;

   duct.Name = GetDuctName(nRow);
   duct.nStrands = GetStrandCount(nRow);
   GetPjack(nRow,&duct);
   duct.JackingEnd = GetJackingEnd(nRow);
   duct.DuctGeometryType = GetTendonShape(nRow);
   GetDuctPoints(nRow, &duct);

   return duct;
}

void CSegmentTendonGrid::AppendRow(const CSegmentDuctData& duct)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   // Appends a row to the grid and initializes it with data from duct
   // DOES NOT APPEND A DUCT ROW TO THE DUCT DATA OBJECT
   ROWCOL nRow = AppendRow();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker, ILibraryNames, pLibNames);
   std::vector<std::_tstring> vNames;
   pLibNames->EnumDuctNames(&vNames);
   std::vector<std::_tstring>::iterator iter(vNames.begin());
   std::vector<std::_tstring>::iterator end(vNames.end());
   short idx = 0;
   for (; iter != end; iter++, idx++)
   {
      if (duct.Name == *iter)
      {
         break;
      }
   }

   SetValueRange(CGXRange(nRow, nDuctTypeCol), idx);
   SetValueRange(CGXRange(nRow, nDuctShapeCol), (LONG)duct.DuctGeometryType);
   SetValueRange(CGXRange(nRow, nStrandsCol), (LONG)duct.nStrands);
   SetValueRange(CGXRange(nRow, nPjackCheckCol), (LONG)!duct.bPjCalc);

   CString strValue;
   strValue.Format(_T("%s"), ::FormatDimension(duct.Pj, pDisplayUnits->GetGeneralForceUnit(), false));
   SetValueRange(CGXRange(nRow, nPjackCol), strValue);

   strValue.Format(_T("%s"), ::FormatDimension(duct.LastUserPj, pDisplayUnits->GetGeneralForceUnit(), false));
   SetValueRange(CGXRange(nRow, nPjackUserCol), strValue);

   SetValueRange(CGXRange(nRow, nJackEndCol), (LONG)duct.JackingEnd);

   strValue.Format(_T("%s"), ::FormatDimension(duct.DuctPoint[CSegmentDuctData::Left].first, pDisplayUnits->GetComponentDimUnit(), false));
   SetValueRange(CGXRange(nRow, nLeftEndYCol), strValue);
   SetValueRange(CGXRange(nRow, nLeftEndDatumCol), (LONG)duct.DuctPoint[CSegmentDuctData::Left].second);

   strValue.Format(_T("%s"), ::FormatDimension(duct.DuctPoint[CSegmentDuctData::Middle].first, pDisplayUnits->GetComponentDimUnit(), false));
   SetValueRange(CGXRange(nRow, nMiddleYCol), strValue);
   SetValueRange(CGXRange(nRow, nMiddleDatumCol), (LONG)duct.DuctPoint[CSegmentDuctData::Middle].second);

   strValue.Format(_T("%s"), ::FormatDimension(duct.DuctPoint[CSegmentDuctData::Right].first, pDisplayUnits->GetComponentDimUnit(), false));
   SetValueRange(CGXRange(nRow, nRightEndYCol), strValue);
   SetValueRange(CGXRange(nRow, nRightEndDatumCol), (LONG)duct.DuctPoint[CSegmentDuctData::Right].second);

   UpdateDuctPoints(nRow);
   UpdateNumStrandsList(nRow);
   OnCalcPjack(nRow);

   GetParam()->EnableUndo(TRUE);
   GetParam()->SetLockReadOnly(TRUE);
}

ROWCOL CSegmentTendonGrid::AppendRow()
{
	ROWCOL nRow = GetRowCount()+1;
	InsertRows(nRow, 1);
   SetRowStyle(nRow);
   return nRow;
}

void CSegmentTendonGrid::OnAddRow()
{
   CSegmentDuctData duct;
   AppendRow(duct);
   RefreshRowHeading(2, GetRowCount());

   OnChangedSelection(nullptr,FALSE,FALSE);

   CGirderSegmentTendonsPage* pParent = (CGirderSegmentTendonsPage*)GetParent();
   ASSERT(pParent->IsKindOf(RUNTIME_CLASS(CGirderSegmentTendonsPage)));
   pParent->OnChange();
}

void CSegmentTendonGrid::OnRemoveSelectedRows()
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

   CGirderSegmentTendonsPage* pParent = (CGirderSegmentTendonsPage*)GetParent();
   ASSERT(pParent->IsKindOf(RUNTIME_CLASS(CGirderSegmentTendonsPage)));
   pParent->OnChange();
}

void CSegmentTendonGrid::OnChangedSelection(const CGXRange* pChangedRect,BOOL bIsDragging, BOOL bKey)
{
	CGirderSegmentTendonsPage* pParent = (CGirderSegmentTendonsPage*)GetParent();
   ASSERT(pParent->IsKindOf(RUNTIME_CLASS(CGirderSegmentTendonsPage)));

   CRowColArray sel_rows;
   ROWCOL nSelRows = GetSelectedRows(sel_rows);
   pParent->EnableRemoveButton( 0 < nSelRows ? TRUE : FALSE );
}

void CSegmentTendonGrid::UpdateData(CDataExchange* pDX, CSegmentPTData* pTendons)
{
   if ( pDX == nullptr || pDX->m_bSaveAndValidate )
   {
       //if pDX is nullptr or if m_bSaveAndValidate is true
       //we are getting data out of the grid
      std::vector<CSegmentDuctData> ducts;
      ROWCOL nRows = GetRowCount();
      for ( ROWCOL row = 2; row <= nRows; row++ )
      {
         CSegmentDuctData duct = GetDuctRow(row);
         UINT result = Validate(row,duct);
         if ( result != ERROR_SUCCESS )
         {
            // row data isn't valid
            if ( pDX )
            {
               ShowValidationError(row,result);
               pDX->Fail();
            }
         }
         ducts.push_back(duct);
      }

      pTendons->SetDucts(ducts);
   }
   else
   {
      FillGrid(pTendons);
   }
}

void CSegmentTendonGrid::FillGrid(const CSegmentPTData* pTendons)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   ROWCOL nRows = GetRowCount();
   if ( 2 <= nRows )
   {
      CGXGridWnd::RemoveRows(2,nRows);
   }

   const auto& ducts = pTendons->GetDucts();
   for(const auto& duct : ducts)
   {
      AppendRow(duct);
   }
   ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));

   nRows = GetRowCount();

   ScrollCellInView(0 < nRows ? nRows-1 : 0, GetLeftCol());

   RefreshRowHeading(2, nRows);

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CSegmentTendonGrid::OnModifyCell(ROWCOL nRow, ROWCOL nCol)
{
   if (nCol == nDuctTypeCol)
   {
      // duct size changed... update the number of strands choice
      UpdateNumStrandsList(nRow);
   }
   else if (nCol == nDuctShapeCol)
   {
      UpdateDuctPoints(nRow);
   }
   else if (nCol == nStrandsCol)
   {
      if (ComputePjackMax(nRow))
      {
         UpdateMaxPjack(nRow);
      }
   }
   else
   {
      __super::OnModifyCell(nRow, nCol);
   }

   CGirderSegmentTendonsPage* pParent = (CGirderSegmentTendonsPage*)GetParent();
   ASSERT(pParent->IsKindOf(RUNTIME_CLASS(CGirderSegmentTendonsPage)));

   pParent->OnChange();
}

BOOL CSegmentTendonGrid::OnEndEditing(ROWCOL nRow, ROWCOL nCol)
{
   ResizeColWidthsToFit(CGXRange(0, 0, GetRowCount(), GetColCount()));

   CGirderSegmentTendonsPage* pParent = (CGirderSegmentTendonsPage*)GetParent();
   ASSERT(pParent->IsKindOf(RUNTIME_CLASS(CGirderSegmentTendonsPage)));

   pParent->OnChange();

   return CGXGridWnd::OnEndEditing(nRow, nCol);
}

UINT CSegmentTendonGrid::Validate(ROWCOL nRow, CSegmentDuctData& duct)
{
   if ( IsLE(duct.DuctPoint[CSegmentDuctData::Left].first,0.0,0.0) || IsLE(duct.DuctPoint[CSegmentDuctData::Right].first,0.0,0.0) )
   {
      return ERROR_Y_MUST_BE_POSITIVE;
   }

   if (duct.DuctGeometryType == CSegmentDuctData::Parabolic && IsLE(duct.DuctPoint[CSegmentDuctData::Middle].first, 0.0, 0.0))
   {
      return ERROR_Y_MUST_BE_POSITIVE;
   }

   if (duct.nStrands == 0)
   {
      return ERROR_STRAND_COUNT;
   }

   return ERROR_SUCCESS;
}

void CSegmentTendonGrid::ShowValidationError(ROWCOL nRow,UINT iError)
{
   CString strMsg;
   switch(iError)
   {
   case ERROR_Y_MUST_BE_POSITIVE:
      strMsg.Format(_T("Row %d: Location of duct, Y, must greater than zero"),nRow-1);
      break;

   case ERROR_STRAND_COUNT:
      strMsg.Format(_T("Row %d: Number of strands must be greater than zero"), nRow - 1);
      break;

   default:
      ATLASSERT(false); // should never get here
      strMsg.Format(_T("Row %d: Unknown error"),nRow-1);
   }

   AfxMessageBox(strMsg);
}

void CSegmentTendonGrid::UpdateMaxPjack(ROWCOL nRow)
{
   StrandIndexType nStrands = (StrandIndexType)_tstoi(GetCellValue(nRow, nStrandsCol));
   CGirderSegmentTendonsPage* pParent = (CGirderSegmentTendonsPage*)GetParent();
   ASSERT(pParent->IsKindOf(RUNTIME_CLASS(CGirderSegmentTendonsPage)));
   const matPsStrand* pStrand = pParent->GetStrand();

   Float64 Pjack = lrfdPsStrand::GetPjackPT(*pStrand, nStrands);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   GetParam()->SetLockReadOnly(FALSE);
   SetValueRange(CGXRange(nRow, nPjackCol), FormatDimension(Pjack, pDisplayUnits->GetGeneralForceUnit(), false));
   GetParam()->SetLockReadOnly(TRUE);
}

bool CSegmentTendonGrid::ComputePjackMax(ROWCOL row)
{
   return (0 < _tstoi(GetCellValue(row, nPjackCheckCol)) ? false : true);
}

CSegmentDuctData::GeometryType CSegmentTendonGrid::GetTendonShape(ROWCOL nRow)
{
   return (CSegmentDuctData::GeometryType)(_tstoi(GetCellValue(nRow, nDuctShapeCol)));
}

void CSegmentTendonGrid::UpdateDuctPoints(ROWCOL nRow)
{
   CSegmentDuctData::GeometryType shape = GetTendonShape(nRow);
   CGXStyle style;
   if (shape == CSegmentDuctData::Linear)
   {
      style.SetEnabled(FALSE)
         .SetReadOnly(TRUE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
   }
   else
   {
      style.SetEnabled(TRUE)
         .SetReadOnly(FALSE)
         .SetInterior(::GetSysColor(COLOR_WINDOW))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
   }

   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);
   
   SetStyleRange(CGXRange(nRow, nMiddleYCol), style);
   SetStyleRange(CGXRange(nRow, nMiddleDatumCol), style);
   
   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(TRUE);
}

void CSegmentTendonGrid::OnStrandChanged()
{
   ROWCOL nRows = GetRowCount();
   for (ROWCOL row = 1; row < nRows; row++)
   {
      if (ComputePjackMax(row + 1))
      {
         UpdateMaxPjack(row + 1);
      }
   }
}

void CSegmentTendonGrid::OnInstallationTypeChanged()
{
   ROWCOL nRows = GetRowCount();
   for (ROWCOL row = 1; row < nRows; row++)
   {
      UpdateNumStrandsList(row + 1);
   }
}

CString CSegmentTendonGrid::GetDuctName(ROWCOL nRow)
{
   CString ductName = GetCellValue(nRow, nDuctTypeCol);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, ILibrary, pLib);
   const DuctLibraryEntry* pDuctEntry = pLib->GetDuctEntry(ductName);
   if (pDuctEntry == nullptr)
   {
      // sometimes the duct name comes back as the index into the choice list
      // get the duct entry another way
      GET_IFACE2(pBroker, ILibraryNames, pLibNames);
      std::vector<std::_tstring> vNames;
      pLibNames->EnumDuctNames(&vNames);
      IndexType ductNameIdx = (IndexType)_tstol(ductName);
      pDuctEntry = pLib->GetDuctEntry(vNames[ductNameIdx].c_str());
      if (pDuctEntry == nullptr)
      {
         ATLASSERT(false);
         return _T("UNKNOWN");
      }
      else
      {
         ductName = pDuctEntry->GetName().c_str();
      }
   }
   return ductName;
}

StrandIndexType CSegmentTendonGrid::GetStrandCount(ROWCOL nRow)
{
   return (StrandIndexType)_tstol(GetCellValue(nRow, nStrandsCol));
}

void CSegmentTendonGrid::GetPjack(ROWCOL nRow, CSegmentDuctData* pDuct)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   pDuct->bPjCalc = ComputePjackMax(nRow);

   pDuct->Pj = _tstof(GetCellValue(nRow, nPjackCol));
   pDuct->Pj = WBFL::Units::ConvertToSysUnits(pDuct->Pj, pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure);

   if (pDuct->bPjCalc)
   {
      pDuct->LastUserPj = _tstof(GetCellValue(nRow, nPjackUserCol));
      pDuct->LastUserPj = WBFL::Units::ConvertToSysUnits(pDuct->LastUserPj, pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure);
   }
   else
   {
      pDuct->LastUserPj = pDuct->Pj;
   }
}

pgsTypes::JackingEndType CSegmentTendonGrid::GetJackingEnd(ROWCOL nRow)
{
   return (pgsTypes::JackingEndType)_tstoi(GetCellValue(nRow, nJackEndCol));
}

void CSegmentTendonGrid::GetDuctPoints(ROWCOL nRow, CSegmentDuctData* pDuct)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
   Float64 value = _tstof(GetCellValue(nRow,nLeftEndYCol));
   pDuct->DuctPoint[CSegmentDuctData::Left].first = WBFL::Units::ConvertToSysUnits(value,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   pDuct->DuctPoint[CSegmentDuctData::Left].second = (pgsTypes::FaceType)(_tstoi(GetCellValue(nRow,nLeftEndDatumCol)));

   value = _tstof(GetCellValue(nRow, nMiddleYCol));
   pDuct->DuctPoint[CSegmentDuctData::Middle].first = WBFL::Units::ConvertToSysUnits(value, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   pDuct->DuctPoint[CSegmentDuctData::Middle].second = (pgsTypes::FaceType)(_tstoi(GetCellValue(nRow, nMiddleDatumCol)));

   value = _tstof(GetCellValue(nRow, nRightEndYCol));
   pDuct->DuctPoint[CSegmentDuctData::Right].first = WBFL::Units::ConvertToSysUnits(value, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   pDuct->DuctPoint[CSegmentDuctData::Right].second = (pgsTypes::FaceType)(_tstoi(GetCellValue(nRow, nRightEndDatumCol)));
}

void CSegmentTendonGrid::UpdateNumStrandsList(ROWCOL nRow)
{
   StrandIndexType nStrands = GetStrandCount(nRow);
   CString ductName = GetDuctName(nRow);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, ILibrary, pLib);
   const DuctLibraryEntry* pDuctEntry = pLib->GetDuctEntry(ductName);
   ATLASSERT(pDuctEntry);

   Float64 A = pDuctEntry->GetInsideArea();

   CGirderSegmentTendonsPage* pParent = (CGirderSegmentTendonsPage*)GetParent();
   ASSERT(pParent->IsKindOf(RUNTIME_CLASS(CGirderSegmentTendonsPage)));
   const matPsStrand* pStrand = pParent->GetStrand();
   Float64 aps = pStrand->GetNominalArea();

   // LRFD 5.4.6.2 Area of duct must be at least K times net area of prestressing steel
   GET_IFACE2(pBroker, IDuctLimits, pDuctLimits);
   Float64 K = pDuctLimits->GetTendonAreaLimit(pParent->GetInstallationType());

   StrandIndexType maxStrands = (StrandIndexType)fabs(A / (K*aps));

   SetStyleRange(CGXRange(nRow, nStrandsCol), CGXStyle()
      .SetUserAttribute(GX_IDS_UA_SPINBOUND_MIN, 0L)
      .SetUserAttribute(GX_IDS_UA_SPINBOUND_MAX, (LONG)maxStrands)
      .SetUserAttribute(GX_IDS_UA_SPINBOUND_WRAP, 1L)
   );

   nStrands = Min(nStrands, maxStrands);
   SetValueRange(CGXRange(nRow, nStrandsCol), (LONG)nStrands);

   // If the Calc Pjack box is unchecked, update the jacking force
   if (ComputePjackMax(nRow))
   {
      UpdateMaxPjack(nRow);
   }
}

void CSegmentTendonGrid::RefreshRowHeading(ROWCOL rFrom, ROWCOL rTo)
{
   CGirderSegmentTendonsPage* pParent = (CGirderSegmentTendonsPage*)GetParent();
   ASSERT(pParent->IsKindOf(RUNTIME_CLASS(CGirderSegmentTendonsPage)));
   std::_tstring strGirderName = pParent->GetSegment()->GetGirder()->GetGirderName();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, ILibrary, pLibrary);
   const GirderLibraryEntry* pGirderEntry = pLibrary->GetGirderEntry(strGirderName.c_str());
   CComPtr<IBeamFactory> factory;
   pGirderEntry->GetBeamFactory(&factory);
   WebIndexType nWebs = factory->GetWebCount(pGirderEntry->GetDimensions());

   for (ROWCOL row = rFrom; row <= rTo; row++)
   {
      DuctIndexType first_duct_in_row = nWebs*(row - 2);
      DuctIndexType last_duct_in_row = first_duct_in_row + nWebs - 1;
      CString strLabel;

      if (nWebs == 1)
      {
         strLabel.Format(_T("%d"), LABEL_DUCT(first_duct_in_row));
      }
      else
      {
         strLabel.Format(_T("%d - %d"), LABEL_DUCT(first_duct_in_row), LABEL_DUCT(last_duct_in_row));
      }

      SetValueRange(CGXRange(row, 0), strLabel);
   }
}
