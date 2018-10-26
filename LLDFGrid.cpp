///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "LiveLoadDistFactorsDlg.h"
#include "LLDFGrid.h"

#include <system\tokenizer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CLLDFGrid grid control

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
   ON_COMMAND(ID_EDIT_COPY, &CLLDFGrid::OnEditCopy)
   ON_COMMAND(ID_EDIT_PASTE, &CLLDFGrid::OnEditPaste)
   ON_MESSAGE(WM_GX_NEEDCHANGETAB, ChangeTabName) 
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CLLDFGrid message handlers

int CLLDFGrid::GetColWidth(ROWCOL nCol)
{
   int default_width = CGXGridWnd::GetColWidth(0);

   CGXTabWnd* pw = (CGXTabWnd*)this->GetParent();

	CRect rect;
   pw->GetInsideRect(rect);

   switch (nCol)
   {
   case 0:
//      return default_width;

   case 1:
   case 2:
   case 3:
   case 4:
   case 5:
   case 6:
      return (rect.Width())/7;

   default:
      return CGXGridWnd::GetColWidth(nCol);
   }
}

void CLLDFGrid::CustomInit(SpanIndexType ispan, bool bContinuous)
{
   m_SpanIdx = ispan;
   m_bContinuous = bContinuous;

// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );

	GetParam( )->EnableUndo(FALSE);

   const int num_rows = 1;
   const int num_cols = 6;

	SetRowCount(num_rows);
	SetColCount(num_cols);

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

   SetStyleRange(CGXRange(0,1),CGXStyle().SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue("Strength/Service"));
   SetStyleRange(CGXRange(0,2),CGXStyle().SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue("Strength/Service"));
   SetStyleRange(CGXRange(0,3),CGXStyle().SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue("Strength/Service"));
   
   SetStyleRange(CGXRange(0,4),CGXStyle().SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue("Fatigue"));
   SetStyleRange(CGXRange(0,5),CGXStyle().SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue("Fatigue"));
   SetStyleRange(CGXRange(0,6),CGXStyle().SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue("Fatigue"));

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
			.SetValue("+M")
		);

	SetStyleRange(CGXRange(1,5), CGXStyle()
         .SetControl(GX_IDS_CTRL_HEADER)
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
			.SetValue("-M")
		);

	SetStyleRange(CGXRange(1,6), CGXStyle()
         .SetControl(GX_IDS_CTRL_HEADER)
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
			.SetValue("V")
		);

   // make it so that text fits correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,num_rows,num_cols));

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();

   // set copy and paste options
   ImplementCutPaste();
   this->m_nClipboardFlags ^= GX_DNDSTYLES; // don't paste styles
   this->m_nClipboardFlags |= GX_DNDNOAPPENDCOLS | GX_DNDNOAPPENDROWS | GX_DNDDIFFRANGEDLG ^ GX_DNDMULTI;

	SetFocus();

	GetParam( )->EnableUndo(TRUE);
}

void CLLDFGrid::AddGirderRow(GirderIndexType gdr, const CSpanData2* pSpan)
{
   GetParam()->EnableUndo(FALSE);

   ROWCOL nRows = GetRowCount();
   nRows++;
   InsertRows(nRows,1);

   // Style for dead cells
   CGXStyle deadStyle;
   deadStyle.SetControl(GX_IDS_CTRL_STATIC).SetReadOnly(TRUE).SetEnabled(FALSE).SetInterior(::GetSysColor(COLOR_BTNFACE));

   CString strLabel;
   strLabel.Format(_T("Girder %s"),LABEL_GIRDER(gdr));
   SetStyleRange(CGXRange(nRows,0), CGXStyle().SetHorizontalAlignment(DT_RIGHT).SetValue(strLabel));

   // Strength/service
   SetStyleRange(CGXRange(nRows,1), CGXStyle()
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(pSpan->GetLLDFPosMoment(gdr,pgsTypes::ServiceI))); // +M

   if ( m_bContinuous )
      SetStyleRange(CGXRange(nRows,2), CGXStyle()
         .SetEnabled(TRUE)
         .SetHorizontalAlignment(DT_RIGHT)
         .SetValue(pSpan->GetLLDFNegMoment(gdr,pgsTypes::ServiceI))); // -M  
   else
      SetStyleRange(CGXRange(nRows,2), deadStyle);
         


   SetStyleRange(CGXRange(nRows,3), CGXStyle()
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(pSpan->GetLLDFShear(gdr,pgsTypes::ServiceI))); // V


   // Fatigue
   SetStyleRange(CGXRange(nRows,4), CGXStyle()
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(pSpan->GetLLDFPosMoment(gdr,pgsTypes::FatigueI))); // +M

   if ( m_bContinuous )
      SetStyleRange(CGXRange(nRows,5), CGXStyle()
         .SetEnabled(TRUE)
         .SetHorizontalAlignment(DT_RIGHT)
         .SetValue(pSpan->GetLLDFNegMoment(gdr,pgsTypes::FatigueI))); // -M
   else
      SetStyleRange(CGXRange(nRows,5),  deadStyle); 


   SetStyleRange(CGXRange(nRows,6), CGXStyle()
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(pSpan->GetLLDFShear(gdr,pgsTypes::FatigueI))); // V

   GetParam()->EnableUndo(TRUE);
}

void CLLDFGrid::GetGirderRow(GirderIndexType gdr, CSpanData2* pSpan)
{
   ROWCOL row = ROWCOL(gdr + 2);

   pSpan->SetLLDFPosMoment(gdr, pgsTypes::StrengthI, _tstof(GetCellValue(row,1)));
   pSpan->SetLLDFShear(gdr, pgsTypes::StrengthI, _tstof(GetCellValue(row,3)));

   pSpan->SetLLDFPosMoment(gdr, pgsTypes::FatigueI, _tstof(GetCellValue(row,4)));
   pSpan->SetLLDFShear(gdr, pgsTypes::FatigueI, _tstof(GetCellValue(row,6)));

   bool bContRight = pSpan->GetPrevPier()->IsContinuous();
   bool bContLeft  = pSpan->GetNextPier()->IsContinuous();

   bool bIntLeft, bIntRight, bIntDummy;
   pSpan->GetPrevPier()->IsIntegral(&bIntDummy,&bIntRight);
   pSpan->GetNextPier()->IsIntegral(&bIntLeft,&bIntDummy);

   if ( bContLeft || bContRight || bIntLeft || bIntRight )
   {
      pSpan->SetLLDFNegMoment(gdr, pgsTypes::StrengthI, _tstof(GetCellValue(row,2)));
      pSpan->SetLLDFNegMoment(gdr, pgsTypes::FatigueI, _tstof(GetCellValue(row,5)));
   }
   else
   {
      pSpan->SetLLDFNegMoment(gdr, pgsTypes::StrengthI, 1.0);
      pSpan->SetLLDFNegMoment(gdr, pgsTypes::FatigueI,  1.0);
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

void CLLDFGrid::FillGrid(const CBridgeDescription2* pBridgeDesc)
{
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(m_SpanIdx);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   GirderIndexType nGirders = pGroup->GetGirderCount();

   for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
   {
      AddGirderRow(gdrIdx, pSpan);
   }

   SetCurrentCell(2,1);
   Enable(m_bEnabled);
}

void CLLDFGrid::GetData(CBridgeDescription2* pBridgeDesc)
{
   CSpanData2* pSpan = pBridgeDesc->GetSpan(m_SpanIdx);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   GirderIndexType nGirders = pGroup->GetGirderCount();

   for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
   {
      GetGirderRow(gdrIdx, pSpan);
   }
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

BOOL CLLDFGrid::OnRButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
	 // unreferenced parameters
	 nRow, nCol, nFlags;

	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_COPY_PASTE));

	CMenu* pPopup = menu.GetSubMenu( 0 );
	ASSERT( pPopup != nullptr );

   if (!CanPaste())
   {
      menu.EnableMenuItem(ID_EDIT_PASTE,MF_GRAYED);
   }

   if (!CanCopy())
   {
      menu.EnableMenuItem(ID_EDIT_COPY,MF_GRAYED);
   }

	// display the menu
	ClientToScreen(&pt);
	BOOL st = pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
	pt.x, pt.y, this);

	// we processed the message
	return TRUE;
}

void CLLDFGrid::OnEditCopy()
{
   this->Copy();
}

void CLLDFGrid::OnEditPaste()
{
   if (this->Paste())
   {

      // See if other tabs are selected and paste to them if so
      CGXTabWnd* pw = (CGXTabWnd*)this->GetParent();
      int n = pw->GetBeam().GetCount();
      for (int i=0; i<n; i++)
      {
         if (i != this->m_SpanIdx)
         {
            CGXTabInfo& rti = pw->GetBeam().GetTab(i);
            BOOL sel = rti.bSel;
            if (sel!=FALSE)
            {
               // Tab is selected, paste to other grid
               CLLDFGrid* pGrid = (CLLDFGrid*)rti.pExtra;

               // Get range selected on current grid
               CRowColArray awCols, awRows;
               this->GetSelectedCols(awCols);
               this->GetSelectedRows(awRows);
               CGXRange range(awRows[0],awCols[0],awRows[awRows.GetCount()-1],awCols[awCols.GetCount()-1]);

               // Unselect all on other grid
               pGrid->SelectRange(CGXRange().SetTable( ), FALSE);

               // select same range on other grid
               pGrid->SelectRange(range, TRUE);

               BOOL st = pGrid->Paste();
               ATLASSERT(st);
            }
         }
      }
   }
}

// validate input
BOOL CLLDFGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
{
   if (nCol!=0 && nRow!=0 && nRow!=1)
   {
	   CString s;
	   CGXControl* pControl = GetControl(nRow, nCol);
	   pControl->GetCurrentText(s);

      Float64 d;
      if (!sysTokenizer::ParseDouble(s, &d))
	   {
		   SetWarningText (_T("Distribution factor value must be a non-negative number"));
		   return FALSE;
	   }

      if (d<0.0)
	   {
		   SetWarningText (_T("Distribution factor values may not be negative"));
		   return FALSE;
	   }

	   return TRUE;
   }

	return CGXGridWnd::OnValidateCell(nRow, nCol);
}

LRESULT CLLDFGrid::ChangeTabName( WPARAM wParam, LPARAM lParam ) 
{
   // don't allow userss to change tab names
   return FALSE;
}


BOOL CLLDFGrid::OnEndEditing(ROWCOL nRow, ROWCOL nCol)
{
   CString s;
   CGXControl* pControl = GetControl(nRow, nCol);
   BOOL bModified = pControl->GetModify();
   if (bModified)
   {
      pControl->GetCurrentText(s);

      // See if other tabs are selected and set values for those if so
      CGXTabWnd* pw = (CGXTabWnd*)this->GetParent();
      int n = pw->GetBeam().GetCount();
      for (int i=0; i<n; i++)
      {
         if (i != this->m_SpanIdx)
         {
            CGXTabInfo& rti = pw->GetBeam().GetTab(i);
            BOOL sel = rti.bSel;
            if (sel!=FALSE)
            {
               CLLDFGrid* pGrid = (CLLDFGrid*)rti.pExtra;

               ROWCOL nrows = pGrid->GetRowCount();
               if (nRow<=nrows)  // don't allow overflow
               {
                  CGXStyle cellStyle;
                  pGrid->GetStyleRowCol(nRow,nCol,cellStyle);

                  if ( !cellStyle.GetIncludeReadOnly() || !cellStyle.GetReadOnly())
                  {
                     pGrid->GetParam()->EnableUndo(TRUE);

                     cellStyle.SetValue(s);
                     pGrid->SetStyleRange(CGXRange(nRow,nCol), cellStyle);

                     pGrid->GetParam()->EnableUndo(FALSE);
                  }
               }
            }
         }
      }
   }

	return CGXGridWnd::OnEndEditing(nRow, nCol);
}

BOOL CLLDFGrid::PasteTextRowCol(ROWCOL nRow, ROWCOL nCol, const CString& str, UINT nFlags, const CGXStyle* pOldStyle)
{
   CGXStyle cellStyle;
   GetStyleRowCol(nRow,nCol,cellStyle);

   // don't paste to read-only cells
   if ( !cellStyle.GetIncludeReadOnly() || !cellStyle.GetReadOnly())
   {
      return CGXGridWnd::PasteTextRowCol(nRow, nCol, str, nFlags, pOldStyle);
   }
   
   return TRUE;
}

void CLLDFGrid::SetGirderLLDF(GirderIndexType gdr, Float64 value )
{
   ROWCOL row = ROWCOL(gdr+2); // first two rows are header
   ROWCOL nrows = this->GetRowCount();
   if (row<=nrows)
   {
      for (ROWCOL col=1; col<=6; col++)
      {
         CGXStyle cellStyle;
         GetStyleRowCol(row, col, cellStyle);

         // don't paste to read-only cells
         if ( !cellStyle.GetIncludeReadOnly() || !cellStyle.GetReadOnly())
         {
            cellStyle.SetValue(value);
            SetStyleRange(CGXRange(row,col), cellStyle);
         }
      }
   }
}

void CLLDFGrid::SetGirderLLDF(GirderIndexType gdr, const SpanLLDF& rlldf )
{
   ROWCOL row = ROWCOL(gdr+2); // first two rows are header
   ATLASSERT(this->GetColCount()==6);
   const Float64* pdbl = &(rlldf.sgPMService); // pointer to first member in struct
   for (ROWCOL col=1; col<=6; col++)
   {
      CGXStyle cellStyle;
      GetStyleRowCol(row, col, cellStyle);

      // don't paste to read-only cells
      if ( !cellStyle.GetIncludeReadOnly() || !cellStyle.GetReadOnly())
      {
         // Doubles coming out have crazy sig figs - let's round them to something reasonable
         Float64 rounded = RoundOff(*pdbl,0.0001);
         cellStyle.SetValue(rounded);
         SetStyleRange(CGXRange(row,col), cellStyle);
      }

      pdbl++; // columns in grid are in same order as struct;
   }
}

BOOL CLLDFGrid::ProcessKeys(CWnd* pSender, UINT nMessage, UINT nChar, UINT nRepCnt, UINT flags)
{
   if (pSender == this)
   {
      if (nMessage == WM_KEYDOWN && nChar == VK_TAB)
      {
         CWnd* pDlg = GetParent();

         while (pDlg && !pDlg->IsKindOf(RUNTIME_CLASS(CDialog)))
                   pDlg = pDlg->GetParent();

         if (pDlg)
         {
            CWnd* pWndNext = pDlg->GetNextDlgTabItem(m_pGridWnd);

            if (pWndNext != nullptr)
            {
               TRACE("SetFocus ");
               pWndNext->SetFocus();
               return TRUE;
            }
         }
      }
    }

   return CGXGridWnd::ProcessKeys(pSender, nMessage, nChar, nRepCnt, flags);
}