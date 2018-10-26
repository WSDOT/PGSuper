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

// LLDFGrid.cpp : implementation file
//

#include "stdafx.h"
#include "LiveLoadDistFactorsDlg.h"
#include "LLDFGrid.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLLDFGrid

CLLDFGrid::CLLDFGrid()
{
   m_bEnabled = TRUE;
}

CLLDFGrid::~CLLDFGrid()
{
}

BEGIN_MESSAGE_MAP(CLLDFGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(LLDFGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CLLDFGrid message handlers

int CLLDFGrid::GetColWidth(ROWCOL nCol)
{
   int default_width = CGXGridWnd::GetColWidth(0);
	CRect rect = GetGridRect( );

   switch (nCol)
   {
   case 0:
      return default_width;

   case 1:
   case 2:
   case 3:
   case 4:
   case 5:
   case 6:
   case 7:
   case 8:
      return (rect.Width()-default_width)/8;

   default:
      return CGXGridWnd::GetColWidth(nCol);
   }
}

void CLLDFGrid::CustomInit()
{
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );

	GetParam( )->EnableUndo(FALSE);

   const int num_rows = 1;
   const int num_cols = 8;

	SetRowCount(num_rows);
	SetColCount(num_cols);

	// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELROW & ~GX_SELTABLE));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   // use 1 extra row as headers
   SetFrozenRows(1,1);

   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   SetStyleRange(CGXRange(0,1),CGXStyle().SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue("Exterior Girders"));
   SetStyleRange(CGXRange(0,2),CGXStyle().SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue("Exterior Girders"));
   SetStyleRange(CGXRange(0,3),CGXStyle().SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue("Exterior Girders"));
   SetStyleRange(CGXRange(0,4),CGXStyle().SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue("Exterior Girders"));
   
   SetStyleRange(CGXRange(0,5),CGXStyle().SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue("Interior Girders"));
   SetStyleRange(CGXRange(0,6),CGXStyle().SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue("Interior Girders"));
   SetStyleRange(CGXRange(0,7),CGXStyle().SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue("Interior Girders"));
   SetStyleRange(CGXRange(0,8),CGXStyle().SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue("Interior Girders"));

   SetStyleRange(CGXRange(0,0),CGXStyle().SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue(" "));
   SetStyleRange(CGXRange(1,0),CGXStyle().SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue(" "));

   // set text along top row
	SetStyleRange(CGXRange(1,1), CGXStyle()
         .SetControl(GX_IDS_CTRL_HEADER)
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
			.SetValue("+M")
		);

	SetStyleRange(CGXRange(1,2), CGXStyle()
         .SetControl(GX_IDS_CTRL_HEADER)
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
			.SetValue("-M")
		);

	SetStyleRange(CGXRange(1,3), CGXStyle()
         .SetControl(GX_IDS_CTRL_HEADER)
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
			.SetValue("V")
		);

	SetStyleRange(CGXRange(1,4), CGXStyle()
         .SetControl(GX_IDS_CTRL_HEADER)
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
			.SetValue("R")
		);

	SetStyleRange(CGXRange(1,5), CGXStyle()
         .SetControl(GX_IDS_CTRL_HEADER)
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
			.SetValue("+M")
		);

	SetStyleRange(CGXRange(1,6), CGXStyle()
         .SetControl(GX_IDS_CTRL_HEADER)
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
			.SetValue("-M")
		);

	SetStyleRange(CGXRange(1,7), CGXStyle()
         .SetControl(GX_IDS_CTRL_HEADER)
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
			.SetValue("V")
		);

	SetStyleRange(CGXRange(1,8), CGXStyle()
         .SetControl(GX_IDS_CTRL_HEADER)
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
			.SetValue("R")
		);

   // make it so that text fits correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,num_rows,num_cols));

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();
	SetFocus();

	GetParam( )->EnableUndo(TRUE);
}

void CLLDFGrid::AddPierRow(pgsTypes::LimitState ls,const CPierData* pPier)
{
   GetParam()->EnableUndo(FALSE);

   CLiveLoadDistFactorsDlg* pParent = (CLiveLoadDistFactorsDlg*)GetParent();

   bool bContinuous = pPier->IsContinuous();

   bool bIntegralLeft, bIntegralRight;
   pPier->IsIntegral(&bIntegralLeft,&bIntegralRight);
   
   ROWCOL nRows = GetRowCount();
   nRows++;
   InsertRows(nRows,1);

   CString strLabel;
   PierIndexType pierIdx = pPier->GetPierIndex();
   long nPiers = pPier->GetBridgeDescription()->GetPierCount();
   strLabel.Format("%s %d",(pierIdx == 0 || pierIdx == nPiers-1 ? "Abut" : "Pier"),
                            pierIdx+1);

   SetStyleRange(CGXRange(nRows,0), CGXStyle().SetHorizontalAlignment(DT_LEFT).SetValue(strLabel));

   SetStyleRange(CGXRange(nRows,1), CGXStyle().SetEnabled(FALSE).SetInterior(::GetSysColor(COLOR_BTNFACE))); // +M

   if ( bContinuous || bIntegralLeft || bIntegralRight )
   {
      SetStyleRange(CGXRange(nRows,2), CGXStyle()
         .SetEnabled(TRUE)
         .SetHorizontalAlignment(DT_RIGHT)
         .SetValue(pPier->GetLLDFNegMoment(ls,pgsTypes::Exterior))); // -M
   }
   else
   {
      SetStyleRange(CGXRange(nRows,2), CGXStyle().SetEnabled(FALSE).SetInterior(::GetSysColor(COLOR_BTNFACE))); // -M
   }

   SetStyleRange(CGXRange(nRows,3), CGXStyle().SetEnabled(FALSE).SetInterior(::GetSysColor(COLOR_BTNFACE))); // V
   SetStyleRange(CGXRange(nRows,4), CGXStyle()
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(pPier->GetLLDFReaction(ls,pgsTypes::Exterior))); // R

   SetStyleRange(CGXRange(nRows,5), CGXStyle().SetEnabled(FALSE).SetInterior(::GetSysColor(COLOR_BTNFACE))); // +M

   if ( bContinuous || bIntegralLeft || bIntegralRight )
   {
      SetStyleRange(CGXRange(nRows,6), CGXStyle()
         .SetEnabled(TRUE)
         .SetHorizontalAlignment(DT_RIGHT)
         .SetValue(pPier->GetLLDFNegMoment(ls,pgsTypes::Interior))); // -M
   }
   else
   {
      SetStyleRange(CGXRange(nRows,6), CGXStyle().SetEnabled(FALSE).SetInterior(::GetSysColor(COLOR_BTNFACE))); // -M
   }

   SetStyleRange(CGXRange(nRows,7), CGXStyle().SetEnabled(FALSE).SetInterior(::GetSysColor(COLOR_BTNFACE))); // V
   SetStyleRange(CGXRange(nRows,8), CGXStyle()
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(pPier->GetLLDFReaction(ls,pgsTypes::Interior))); // R

   GetParam()->EnableUndo(TRUE);
}

void CLLDFGrid::AddSpanRow(pgsTypes::LimitState ls,const CSpanData* pSpan)
{
   GetParam()->EnableUndo(FALSE);

   bool bContinuousLeft  = pSpan->GetPrevPier()->IsContinuous();
   bool bContinuousRight = pSpan->GetNextPier()->IsContinuous();

   bool bIntegral, bIntegralLeft, bIntegralRight;
   pSpan->GetPrevPier()->IsIntegral(&bIntegral,&bIntegralRight);
   pSpan->GetNextPier()->IsIntegral(&bIntegralLeft,&bIntegral);

   BOOL bEnableNegMoment = (bContinuousLeft || bContinuousRight || bIntegralLeft || bIntegralRight) ? TRUE : FALSE;
   
   ROWCOL nRows = GetRowCount();
   nRows++;
   InsertRows(nRows,1);

   CString strLabel;
   strLabel.Format("Span %d",LABEL_SPAN(pSpan->GetSpanIndex()));
   SetStyleRange(CGXRange(nRows,0), CGXStyle().SetHorizontalAlignment(DT_RIGHT).SetValue(strLabel));

   SetStyleRange(CGXRange(nRows,1), CGXStyle()
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(pSpan->GetLLDFPosMoment(ls,pgsTypes::Exterior))); // +M

   if ( bEnableNegMoment )
      SetStyleRange(CGXRange(nRows,2), CGXStyle()
         .SetEnabled(TRUE)
         .SetHorizontalAlignment(DT_RIGHT)
         .SetValue(pSpan->GetLLDFNegMoment(ls,pgsTypes::Exterior))); // -M  
   else
      SetStyleRange(CGXRange(nRows,2), CGXStyle().SetEnabled(FALSE).SetInterior(::GetSysColor(COLOR_BTNFACE))); // -M  


   SetStyleRange(CGXRange(nRows,3), CGXStyle()
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(pSpan->GetLLDFShear(ls,pgsTypes::Exterior))); // V

   SetStyleRange(CGXRange(nRows,4), CGXStyle().SetEnabled(FALSE).SetInterior(::GetSysColor(COLOR_BTNFACE))); // R

   SetStyleRange(CGXRange(nRows,5), CGXStyle()
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(pSpan->GetLLDFPosMoment(ls,pgsTypes::Interior))); // +M

   if ( bEnableNegMoment )
      SetStyleRange(CGXRange(nRows,6), CGXStyle()
         .SetEnabled(TRUE)
         .SetHorizontalAlignment(DT_RIGHT)
         .SetValue(pSpan->GetLLDFNegMoment(ls,pgsTypes::Interior))); // -M
   else
      SetStyleRange(CGXRange(nRows,6), CGXStyle().SetEnabled(FALSE).SetInterior(::GetSysColor(COLOR_BTNFACE))); // -M


   SetStyleRange(CGXRange(nRows,7), CGXStyle()
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(pSpan->GetLLDFShear(ls,pgsTypes::Interior))); // V

   SetStyleRange(CGXRange(nRows,8), CGXStyle().SetEnabled(FALSE).SetInterior(::GetSysColor(COLOR_BTNFACE))); // R

   GetParam()->EnableUndo(TRUE);
}

void CLLDFGrid::GetPierRow(pgsTypes::LimitState ls,CPierData* pPier)
{
   ROWCOL row = pPier->GetPierIndex()*2 + 2;

   bool bCont = pPier->IsContinuous();

   bool bIntLeft, bIntRight;
   pPier->IsIntegral(&bIntLeft,&bIntRight);

   if ( bCont || bIntLeft || bIntRight )
   {
      pPier->SetLLDFNegMoment(ls,pgsTypes::Exterior,atof(GetCellValue(row,2)));
      pPier->SetLLDFNegMoment(ls,pgsTypes::Interior,atof(GetCellValue(row,6)));
   }
   else
   {
      // if the pier is not integral or continuous, set the LLDF to 1.0
      // atof in the above "if" block returns 0 in this case. If the user
      // later changes the boundary conditions the LLDF will be 0. This is 
      // not desirable. To elimiate this problem, set the unused LLDF to 1.0
      pPier->SetLLDFNegMoment(ls,pgsTypes::Exterior,1.0);
      pPier->SetLLDFNegMoment(ls,pgsTypes::Interior,1.0);
   }

   pPier->SetLLDFReaction(ls,pgsTypes::Exterior,atof(GetCellValue(row,4)));
   pPier->SetLLDFReaction(ls,pgsTypes::Interior,atof(GetCellValue(row,8)));
}

void CLLDFGrid::GetSpanRow(pgsTypes::LimitState ls,CSpanData* pSpan)
{
   ROWCOL row = pSpan->GetSpanIndex()*2 + 3;

   pSpan->SetLLDFPosMoment(ls,pgsTypes::Exterior,atof(GetCellValue(row,1)));
   pSpan->SetLLDFShear(ls,pgsTypes::Exterior,atof(GetCellValue(row,3)));

   pSpan->SetLLDFPosMoment(ls,pgsTypes::Interior,atof(GetCellValue(row,5)));
   pSpan->SetLLDFShear(ls,pgsTypes::Interior,atof(GetCellValue(row,7)));


   // see note in GetPierRow. We don't atof to return 0 for LLDF.
   bool bContRight = pSpan->GetPrevPier()->IsContinuous();
   bool bContLeft  = pSpan->GetNextPier()->IsContinuous();

   bool bIntLeft, bIntRight, bIntDummy;
   pSpan->GetPrevPier()->IsIntegral(&bIntDummy,&bIntRight);
   pSpan->GetNextPier()->IsIntegral(&bIntLeft,&bIntDummy);

   if ( bContLeft || bContRight || bIntLeft || bIntRight )
   {
      pSpan->SetLLDFNegMoment(ls,pgsTypes::Exterior,atof(GetCellValue(row,2)));
      pSpan->SetLLDFNegMoment(ls,pgsTypes::Interior,atof(GetCellValue(row,6)));
   }
   else
   {
      pSpan->SetLLDFNegMoment(ls,pgsTypes::Exterior,1.0);
      pSpan->SetLLDFNegMoment(ls,pgsTypes::Interior,1.0);
   }
}

CString CLLDFGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
{
    if (IsCurrentCell(nRow, nCol) && IsActiveCurrentCell())
    {
        CString s;
        CGXControl* pControl = GetControl(nRow, nCol);
        pControl->GetValue(s);
        return s;
  }
    else
        return GetValueRowCol(nRow, nCol);
}

void CLLDFGrid::FillGrid(pgsTypes::LimitState ls,const CBridgeDescription* pBridgeDesc)
{
   const CPierData* pPier = pBridgeDesc->GetPier(0);
   const CSpanData* pSpan = NULL;
   do
   {
      AddPierRow(ls,pPier);

      pSpan = pPier->GetNextSpan();
      if ( pSpan )
      {
         AddSpanRow(ls,pSpan);
         pPier = pSpan->GetNextPier();
      }
   } while ( pSpan );

   Enable(m_bEnabled);
}

void CLLDFGrid::GetData(pgsTypes::LimitState ls,CBridgeDescription* pBridgeDesc)
{
   CPierData* pPier = pBridgeDesc->GetPier(0);
   CSpanData* pSpan = NULL;

   do
   {
      GetPierRow(ls,pPier);
      pSpan = pPier->GetNextSpan();
      if ( pSpan )
      {
         GetSpanRow(ls,pSpan);
         pPier = pSpan->GetNextPier();
      }
   } while ( pSpan );
}

void CLLDFGrid::Enable(BOOL bEnable)
{
   m_bEnabled = bEnable;

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CGXStyle style;
   CGXRange range;
   if ( bEnable )
   {
      style.SetInterior(::GetSysColor(COLOR_BTNFACE))
           .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

      // column headers (2 rows)
      range = CGXRange(0,0,1,GetColCount());
      SetStyleRange(range,style);

      // left column
      range = CGXRange(0,0,GetRowCount(),0);
      SetStyleRange(range,style);

      // main field
      style.SetInterior(::GetSysColor(COLOR_WINDOW));
      ROWCOL rowIdx;
      for ( rowIdx = 2; rowIdx <= GetRowCount(); rowIdx++ )
      {
         ROWCOL colIdx;
         for ( colIdx = 1; colIdx <= GetColCount(); colIdx++ )
         {
            CGXStyle cellStyle;
            GetStyleRowCol(rowIdx,colIdx,cellStyle);

            if ( cellStyle.GetEnabled() )
            {
               range = CGXRange(rowIdx,colIdx);
               SetStyleRange(range,style);
            }
         }
      }
   }
   else
   {
      style.SetInterior(::GetSysColor(COLOR_BTNFACE))
           .SetTextColor(::GetSysColor(COLOR_GRAYTEXT));

      range = CGXRange(0,0,GetRowCount(),GetColCount());
      SetStyleRange(range,style);
   }

   EnableWindow(bEnable);

   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(FALSE);
}
