///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
   if ( nCol == 23 || nCol == 25 )
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
   const int num_cols = 26;
   
   // determine if there are cantilevers being modeled
   // NOTE: Tricky code... the pSegment that is passed in here is the one
   // that is being manipulated. It isn't attached to an actual bridge model
   // so we can't get real pier objects. Since this segment is not begin
   // modeled in the context of bridge editing, we can go back to the original
   // bridge to get the piers for this segment.
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pRealSegment = pIBridgeDesc->GetPrecastSegmentData(pSegment->GetSegmentKey());

   bool bLeftCantilever = false;
   bool bRightCantilever = false;
   std::vector<const CPierData2*> vPiers = pRealSegment->GetPiers();
   if ( 2 == vPiers.size() )
   {
      if ( vPiers.front()->HasCantilever() )
      {
         bLeftCantilever = true;
      }

      if ( vPiers.back()->HasCantilever() )
      {
         bRightCantilever = true;
      }
   }

	SetRowCount(num_rows);
	SetColCount(num_cols);

	GetParam( )->EnableUndo(FALSE);

	// Turn off selecting whole columns when clicking on a column header (also turns on whole row selection)
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   SetFrozenRows(1/*# frozen rows*/,1/*# extra header rows*/);

   ROWCOL col = 0;

   // set text along top row
	SetStyleRange(CGXRange(0,col,1,col++), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
			.SetValue(_T("Row"))
		);

	SetStyleRange(CGXRange(0,col,0,col+1), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
			.SetValue(_T("Spacing"))
		);

   CString strLabel;
   strLabel.Format(_T("S1 (%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(1,col++), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(strLabel)
      );

   strLabel.Format(_T("S2 (%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(1,col++), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(strLabel)
      );

	SetStyleRange(CGXRange(0,col,1,col++), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
         .SetWrapText(TRUE)
         .SetValue(_T("Type"))
		);

	SetStyleRange(CGXRange(0,col,1,col++), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
         .SetWrapText(TRUE)
         .SetValue(_T("#\nStrands"))
		);

   SetStyleRange(CGXRange(0,col,0,col+1), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(_T("Left End"))
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

   SetStyleRange(CGXRange(0,col,0,col+5), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(_T("Left Harp Pt"))
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      );

   SetStyleRange(CGXRange(1,col++,1,col++),CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      .SetValue(_T("Dist"))
      );

   SetStyleRange(CGXRange(1,col++,1,col++),CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      .SetValue(_T("X"))
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

   SetStyleRange(CGXRange(0,col,0,col+5), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetValue(_T("Right Harp Pt"))
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      );

   SetStyleRange(CGXRange(1,col++,1,col++),CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      .SetValue(_T("X"))
      );

   SetStyleRange(CGXRange(1,col++,1,col++),CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      .SetValue(_T("Dist"))
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

   // if we don't have cantilevers, hide the columns that relate
   // to cantilevered strand dimensions
   if ( !bLeftCantilever )
   {
      HideCols(7,8);
   }

   if ( !bRightCantilever )
   {
      HideCols(15,16);
   }

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
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   ROWCOL col = 0;

   // Row number
   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(nRow-1)
      );

   // Spacing
   SetStyleRange(CGXRange(nRow,col++,nRow,col++),CGXStyle().SetHorizontalAlignment(DT_RIGHT));

   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(_T("Straight\nHarped\nTemporary"))
      .SetValue(0L)
      );

   // # Strands
   SetStyleRange(CGXRange(nRow,col,nRow,col++),CGXStyle()
      .SetControl(GX_IDS_CTRL_SPINEDIT)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetUserAttribute(GX_IDS_UA_VALID_MIN,0L)
      .SetUserAttribute(GX_IDS_UA_SPINBOUND_MIN,0L)
      );

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
   CString strChoices;
   strChoices.Format(_T("%s\n%s"),pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str(),_T("%"));

   // Dist
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetHorizontalAlignment(DT_RIGHT));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(strChoices)
      .SetValue(0L)
      );

   // X
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetHorizontalAlignment(DT_RIGHT));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(strChoices)
      .SetValue(0L)
      );

   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetHorizontalAlignment(DT_RIGHT));

   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(_T("Top\nBottom"))
      .SetValue(0L)
      );

   // Right Harp Point

   // X
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetHorizontalAlignment(DT_RIGHT));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(strChoices)
      .SetValue(0L)
      );

   // Dist
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetHorizontalAlignment(DT_RIGHT));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetChoiceList(strChoices)
      .SetValue(0L)
      );

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

   Float64 value = _tstof(GetCellValue(nRow,col++));
   strandRow.m_InnerSpacing = ::ConvertToSysUnits(value,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

   value = _tstof(GetCellValue(nRow,col++));
   strandRow.m_Spacing = ::ConvertToSysUnits(value,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

   strandRow.m_StrandType = (pgsTypes::StrandType)(_tstoi(GetCellValue(nRow,col++)));

   strandRow.m_nStrands = _tstoi(GetCellValue(nRow,col++));

   // Left End
   value = _tstof(GetCellValue(nRow,col++));
   strandRow.m_Y[LOCATION_START] = ::ConvertToSysUnits(value,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   strandRow.m_Face[LOCATION_START] = (pgsTypes::FaceType)(_tstoi(GetCellValue(nRow,col++)));

   // Left Harp Point
   value = _tstof(GetCellValue(nRow,col++));
   if ( GetCellValue(nRow,col++) == _T("1") )
   {
      strandRow.m_X[LOCATION_START] = -0.01*value;
   }
   else
   {
      strandRow.m_X[LOCATION_START] = ::ConvertToSysUnits(value,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   }
   value = _tstof(GetCellValue(nRow,col++));
   if ( GetCellValue(nRow,col++) == _T("1") )
   {
      strandRow.m_X[LOCATION_LEFT_HP] = -0.01*value;
   }
   else
   {
      strandRow.m_X[LOCATION_LEFT_HP] = ::ConvertToSysUnits(value,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   }
   value = _tstof(GetCellValue(nRow,col++));
   strandRow.m_Y[LOCATION_LEFT_HP] = ::ConvertToSysUnits(value,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   strandRow.m_Face[LOCATION_LEFT_HP] = (pgsTypes::FaceType)(_tstoi(GetCellValue(nRow,col++)));

   // Right Harp Point
   value = _tstof(GetCellValue(nRow,col++));
   if ( GetCellValue(nRow,col++) == _T("1") )
   {
      strandRow.m_X[LOCATION_RIGHT_HP] = -0.01*value;
   }
   else
   {
      strandRow.m_X[LOCATION_RIGHT_HP] = ::ConvertToSysUnits(value,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   }

   value = _tstof(GetCellValue(nRow,col++));
   if ( GetCellValue(nRow,col++) == _T("1") )
   {
      strandRow.m_X[LOCATION_END] = -0.01*value;
   }
   else
   {
      strandRow.m_X[LOCATION_END] = ::ConvertToSysUnits(value,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   }

   value = _tstof(GetCellValue(nRow,col++));
   strandRow.m_Y[LOCATION_RIGHT_HP] = ::ConvertToSysUnits(value,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   strandRow.m_Face[LOCATION_RIGHT_HP] = (pgsTypes::FaceType)(_tstoi(GetCellValue(nRow,col++)));

   // Right End
   value = _tstof(GetCellValue(nRow,col++));
   strandRow.m_Y[LOCATION_END] = ::ConvertToSysUnits(value,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   strandRow.m_Face[LOCATION_END] = (pgsTypes::FaceType)(_tstoi(GetCellValue(nRow,col++)));

   strandRow.m_bIsExtendedStrand[pgsTypes::metStart] = GetCellValue(nRow,col++) == _T("1") ? true : false;
   strandRow.m_bIsExtendedStrand[pgsTypes::metEnd]   = GetCellValue(nRow,col++) == _T("1") ? true : false;

   strandRow.m_bIsDebonded[pgsTypes::metStart] = GetCellValue(nRow,col++) == _T("1") ? true : false;
   value = _tstof(GetCellValue(nRow,col++));
   strandRow.m_DebondLength[pgsTypes::metStart] = ::ConvertToSysUnits(value,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);

   strandRow.m_bIsDebonded[pgsTypes::metEnd]   = GetCellValue(nRow,col++) == _T("1") ? true : false;
   value = _tstof(GetCellValue(nRow,col++));
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

   ROWCOL col = 1;

   CString strValue;
   strValue.Format(_T("%s"),::FormatDimension(strandRow.m_InnerSpacing,pDisplayUnits->GetComponentDimUnit(),false));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strValue));

   strValue.Format(_T("%s"),::FormatDimension(strandRow.m_Spacing,pDisplayUnits->GetComponentDimUnit(),false));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strValue));

   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue((LONG)strandRow.m_StrandType));

   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue((LONG)strandRow.m_nStrands));

   // Left End
   strValue.Format(_T("%s"),::FormatDimension(strandRow.m_Y[LOCATION_START],pDisplayUnits->GetComponentDimUnit(),false));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strValue));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue((LONG)(strandRow.m_Face[LOCATION_START])));

   // Left Harp Point
   if ( strandRow.m_X[LOCATION_START] < 0 )
   {
      // fractional measure
      SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(100*fabs(strandRow.m_X[LOCATION_START])));
      SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(1L)); // %
   }
   else
   {
      strValue.Format(_T("%s"),::FormatDimension(strandRow.m_X[LOCATION_START],pDisplayUnits->GetSpanLengthUnit(),false));
      SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strValue));
      SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(0L)); // unit
   }
   if ( strandRow.m_X[LOCATION_LEFT_HP] < 0 )
   {
      // fractional measure
      SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(100*fabs(strandRow.m_X[LOCATION_LEFT_HP])));
      SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(1L)); // %
   }
   else
   {
      strValue.Format(_T("%s"),::FormatDimension(strandRow.m_X[LOCATION_LEFT_HP],pDisplayUnits->GetSpanLengthUnit(),false));
      SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strValue));
      SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(0L)); // unit
   }
   strValue.Format(_T("%s"),::FormatDimension(strandRow.m_Y[LOCATION_LEFT_HP],pDisplayUnits->GetComponentDimUnit(),false));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strValue));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue((LONG)(strandRow.m_Face[LOCATION_LEFT_HP])));

   // Right Harp Point
   if ( strandRow.m_X[LOCATION_RIGHT_HP] < 0 )
   {
      // fractional measure
      SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(100*fabs(strandRow.m_X[LOCATION_RIGHT_HP])));
      SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(1L)); // %
   }
   else
   {
      strValue.Format(_T("%s"),::FormatDimension(strandRow.m_X[LOCATION_RIGHT_HP],pDisplayUnits->GetSpanLengthUnit(),false));
      SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strValue));
      SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(0L)); // unit
   }
   if ( strandRow.m_X[LOCATION_END] < 0 )
   {
      // fractional measure
      SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(100*fabs(strandRow.m_X[LOCATION_END])));
      SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(1L)); // %
   }
   else
   {
      strValue.Format(_T("%s"),::FormatDimension(strandRow.m_X[LOCATION_END],pDisplayUnits->GetSpanLengthUnit(),false));
      SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strValue));
      SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(0L)); // unit
   }
   strValue.Format(_T("%s"),::FormatDimension(strandRow.m_Y[LOCATION_RIGHT_HP],pDisplayUnits->GetComponentDimUnit(),false));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strValue));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue((LONG)(strandRow.m_Face[LOCATION_RIGHT_HP])));

   // Right End
   strValue.Format(_T("%s"),::FormatDimension(strandRow.m_Y[LOCATION_END],pDisplayUnits->GetComponentDimUnit(),false));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue(strValue));
   SetStyleRange(CGXRange(nRow,col++),CGXStyle().SetValue((LONG)(strandRow.m_Face[LOCATION_END])));

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

   OnChangedSelection(NULL,FALSE,FALSE);

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

   OnChangedSelection(NULL,FALSE,FALSE);

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

      pStrands->SetStrandDefinitionType(CStrandData::sdtDirectInput);
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

   ATLASSERT(pStrands->GetStrandDefinitionType() == CStrandData::sdtDirectInput);

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
   UpdateExtendedStrandProperties(nRow);

   CGirderSegmentStrandsPage* pParent = (CGirderSegmentStrandsPage*)GetParent();
   ASSERT(pParent);
   pParent->OnChange();
}

void CStrandGrid::OnClickedButtonRowCol(ROWCOL nRow, ROWCOL nCol)
{
   if ( nCol == 21 || nCol == 22 || nCol == 23 || nCol == 25 ) 
   {
      // left extension, right extension, left debond, or right debond check box was clicked
      UpdateExtendedStrandProperties(nRow);

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

void CStrandGrid::UpdateExtendedStrandProperties(ROWCOL nRow)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   bool bHarpStrands = GetCellValue(nRow,3) == _T("1") ? true : false;
   bool bTempStrands = GetCellValue(nRow,3) == _T("2") ? true : false;
   bool bIsExtended[2] = {GetCellValue(nRow,21) == _T("1") ? true : false,
                          GetCellValue(nRow,22) == _T("1") ? true : false};
   bool bIsDebonded[2] = {GetCellValue(nRow,23) == _T("1") ? true : false,
                          GetCellValue(nRow,25) == _T("1") ? true : false};

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

   SetStyleRange(CGXRange(nRow,7,nRow,18),enabled_style);

   SetStyleRange(CGXRange(nRow,21,nRow,22),enabled_style);
   SetStyleRange(CGXRange(nRow,23),enabled_style);
   if ( bIsDebonded[pgsTypes::metStart] )
   {
      SetStyleRange(CGXRange(nRow,24),enabled_style);
   }
   else
   {
      SetStyleRange(CGXRange(nRow,24),disabled_style);
   }
   SetStyleRange(CGXRange(nRow,25),enabled_style);
   if ( bIsDebonded[pgsTypes::metEnd] )
   {
      SetStyleRange(CGXRange(nRow,26),enabled_style);
   }
   else
   {
      SetStyleRange(CGXRange(nRow,26),disabled_style);
   }

   if ( !bHarpStrands )
   {
      SetStyleRange(CGXRange(nRow,7,nRow,18),disabled_style);
   }

   if ( bTempStrands )
   {
      // can't be extended strands
      SetStyleRange(CGXRange(nRow,21,nRow,22),CGXStyle(disabled_style).SetValue(0L));

      // can't be debonded
      SetStyleRange(CGXRange(nRow,23),CGXStyle(disabled_style).SetValue(0L));
      SetStyleRange(CGXRange(nRow,24),CGXStyle(disabled_style));
      SetStyleRange(CGXRange(nRow,25),CGXStyle(disabled_style).SetValue(0L));
      SetStyleRange(CGXRange(nRow,26),CGXStyle(disabled_style));
   }
   
   if ( bIsExtended[pgsTypes::metStart] )
   {
      // can't be debonded
      SetStyleRange(CGXRange(nRow,23),CGXStyle(disabled_style).SetValue(0L));
      SetStyleRange(CGXRange(nRow,24),CGXStyle(disabled_style));
   }
   
   if ( bIsExtended[pgsTypes::metEnd] )
   {
      // can't be debonded
      SetStyleRange(CGXRange(nRow,25),CGXStyle(disabled_style).SetValue(0L));
      SetStyleRange(CGXRange(nRow,26),CGXStyle(disabled_style));
   }

   if ( bIsDebonded[pgsTypes::metStart] )
   {
      // can't be extended
      SetStyleRange(CGXRange(nRow,21),CGXStyle(disabled_style).SetValue(0L));
   }

   if ( bIsDebonded[pgsTypes::metEnd] )
   {
      // can't be extended
      SetStyleRange(CGXRange(nRow,22),CGXStyle(disabled_style).SetValue(0L));
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

      if ( strandRow.m_bIsDebonded[pgsTypes::metStart] && ::IsLE(strandRow.m_DebondLength[pgsTypes::metStart],0.0) )
      {
         CString strMsg;
         strMsg.Format(_T("Row %d: Left end debond length must be greater than 0.0"),nRow-1);
         AfxMessageBox(strMsg);
         return false;
      }

      if ( strandRow.m_bIsDebonded[pgsTypes::metEnd] && ::IsLE(strandRow.m_DebondLength[pgsTypes::metEnd],0.0) )
      {
         CString strMsg;
         strMsg.Format(_T("Row %d: Right end debond length must be greater than 0.0"),nRow-1);
         AfxMessageBox(strMsg);
         return false;
      }
   }

   return true;
}
