///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// LLDFPierGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "LiveLoadDistFactorsDlg.h"
#include "LLDFPierGrid.h"

#include <system\tokenizer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CLLDFPierGrid grid control

/////////////////////////////////////////////////////////////////////////////
// CLLDFPierGrid

CLLDFPierGrid::CLLDFPierGrid()
{
   m_bEnabled = TRUE;
}

CLLDFPierGrid::~CLLDFPierGrid()
{
}

BEGIN_MESSAGE_MAP(CLLDFPierGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(LLDFPierGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
   ON_COMMAND(ID_EDIT_COPY, &CLLDFPierGrid::OnEditCopy)
   ON_COMMAND(ID_EDIT_PASTE, &CLLDFPierGrid::OnEditPaste)
   ON_MESSAGE(WM_GX_NEEDCHANGETAB, ChangeTabName)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CLLDFPierGrid message handlers

int CLLDFPierGrid::GetColWidth(ROWCOL nCol)
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
      return (rect.Width())/5;

   default:
      return CGXGridWnd::GetColWidth(nCol);
   }
}

void CLLDFPierGrid::CustomInit(PierIndexType iPier)
{
   m_PierIdx = iPier;

// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );

	GetParam( )->EnableUndo(FALSE);

   const int num_rows = 1;
   const int num_cols = 4;

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
   
   SetStyleRange(CGXRange(0,3),CGXStyle().SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue("Fatigue"));
   SetStyleRange(CGXRange(0,4),CGXStyle().SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue("Fatigue"));

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
			.SetValue("-M")
		);

	SetStyleRange(CGXRange(1,2), CGXStyle()
         .SetControl(GX_IDS_CTRL_HEADER)
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
			.SetValue("Reaction")
		);

	SetStyleRange(CGXRange(1,3), CGXStyle()
         .SetControl(GX_IDS_CTRL_HEADER)
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
			.SetValue("-M")
		);

	SetStyleRange(CGXRange(1,4), CGXStyle()
         .SetControl(GX_IDS_CTRL_HEADER)
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
			.SetValue("Reaction")
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

void CLLDFPierGrid::AddGirderRow(GirderIndexType gdr, const CPierData* pPier)
{
   GetParam()->EnableUndo(FALSE);

   bool bContinuous  = pPier->IsContinuous();

   bool bIntegralLeft,bIntegralRight;
   pPier->IsIntegral(&bIntegralLeft,&bIntegralRight);

   BOOL bEnableNegMoment = (bContinuous || bIntegralLeft || bIntegralRight) ? TRUE : FALSE;
   
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
   if ( bEnableNegMoment )
      SetStyleRange(CGXRange(nRows,1), CGXStyle()
         .SetEnabled(TRUE)
         .SetHorizontalAlignment(DT_RIGHT)
         .SetValue(pPier->GetLLDFNegMoment(gdr,pgsTypes::ServiceI))); // -M
   else
      SetStyleRange(CGXRange(nRows,1), deadStyle);

   SetStyleRange(CGXRange(nRows,2), CGXStyle()
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(pPier->GetLLDFReaction(gdr,pgsTypes::ServiceI))); // reaction

   // Fatigue
   if ( bEnableNegMoment )
      SetStyleRange(CGXRange(nRows,3), CGXStyle()
         .SetEnabled(TRUE)
         .SetHorizontalAlignment(DT_RIGHT)
         .SetValue(pPier->GetLLDFNegMoment(gdr,pgsTypes::FatigueI))); // -M
   else
      SetStyleRange(CGXRange(nRows,3), deadStyle);

   SetStyleRange(CGXRange(nRows,4), CGXStyle()
      .SetEnabled(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(pPier->GetLLDFReaction(gdr,pgsTypes::FatigueI))); // reaction

   GetParam()->EnableUndo(TRUE);
}

void CLLDFPierGrid::GetGirderRow(GirderIndexType gdr, CPierData* pPier)
{
   bool bContinuous  = pPier->IsContinuous();

   bool bIntegralLeft,bIntegralRight;
   pPier->IsIntegral(&bIntegralLeft,&bIntegralRight);

   BOOL bEnableNegMoment = (bContinuous || bIntegralLeft || bIntegralRight) ? TRUE : FALSE;

   ROWCOL row = ROWCOL(gdr + 2);

   if ( bEnableNegMoment )
   {
      pPier->SetLLDFNegMoment(gdr, pgsTypes::StrengthI, _tstof(GetCellValue(row,1)));
      pPier->SetLLDFNegMoment(gdr, pgsTypes::FatigueI, _tstof(GetCellValue(row,3)));
   }

   pPier->SetLLDFReaction(gdr, pgsTypes::StrengthI, _tstof(GetCellValue(row,2)));
   pPier->SetLLDFReaction(gdr, pgsTypes::FatigueI, _tstof(GetCellValue(row,4)));
}

CString CLLDFPierGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CLLDFPierGrid::FillGrid(const CBridgeDescription* pBridgeDesc)
{
   const CPierData* pPier = pBridgeDesc->GetPier(m_PierIdx);

   GirderIndexType ngdrs = GetPierGirderCount(pPier);

   for (GirderIndexType igdr=0; igdr<ngdrs; igdr++)
   {
      AddGirderRow(igdr, pPier);
   }

   SetCurrentCell(2,1);
   Enable(m_bEnabled);
}

void CLLDFPierGrid::GetData(CBridgeDescription* pBridgeDesc)
{
   CPierData* pPier = pBridgeDesc->GetPier(m_PierIdx);

   GirderIndexType ngdrs = GetPierGirderCount(pPier);

   for (GirderIndexType igdr=0; igdr<ngdrs; igdr++)
   {
      GetGirderRow(igdr, pPier);
   }
}

void CLLDFPierGrid::Enable(BOOL bEnable)
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

BOOL CLLDFPierGrid::OnRButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
	 // unreferenced parameters
	 nRow, nCol, nFlags;

	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_COPY_PASTE));

	CMenu* pPopup = menu.GetSubMenu( 0 );
	ASSERT( pPopup != NULL );

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

void CLLDFPierGrid::OnEditCopy()
{
   this->Copy();
}

void CLLDFPierGrid::OnEditPaste()
{
   if (this->Paste())
   {

      // See if other tabs are selected and paste to them if so
      CGXTabWnd* pw = (CGXTabWnd*)this->GetParent();
      int n = pw->GetBeam().GetCount();
      for (int i=0; i<n; i++)
      {
         if (i != this->m_PierIdx)
         {
            CGXTabInfo& rti = pw->GetBeam().GetTab(i);
            BOOL sel = rti.bSel;
            if (sel!=FALSE)
            {
               // Tab is selected, paste to other grid
               CLLDFPierGrid* pGrid = (CLLDFPierGrid*)rti.pExtra;

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
BOOL CLLDFPierGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
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

LRESULT CLLDFPierGrid::ChangeTabName( WPARAM wParam, LPARAM lParam ) 
{
   // don't allow userss to change tab names
   return FALSE;
}

BOOL CLLDFPierGrid::OnEndEditing(ROWCOL nRow, ROWCOL nCol)
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
         if (i != this->m_PierIdx)
         {
            CGXTabInfo& rti = pw->GetBeam().GetTab(i);
            BOOL sel = rti.bSel;
            if (sel!=FALSE)
            {
               CLLDFPierGrid* pGrid = (CLLDFPierGrid*)rti.pExtra;

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

BOOL CLLDFPierGrid::PasteTextRowCol(ROWCOL nRow, ROWCOL nCol, const CString& str, UINT nFlags, const CGXStyle* pOldStyle)
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

void CLLDFPierGrid::SetGirderLLDF(GirderIndexType gdr, Float64 value )
{
   ROWCOL row = ROWCOL(gdr+2); // first two rows are header
   ROWCOL nrows = this->GetRowCount();
   if (row<=nrows)
   {
      for (ROWCOL col=1; col<=4; col++)
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

void CLLDFPierGrid::SetGirderLLDF(GirderIndexType gdr, const PierLLDF& rlldf )
{
   ROWCOL row = ROWCOL(gdr+2); // first two rows are header
   ATLASSERT(this->GetColCount()==4);
   const Float64* pdbl = &(rlldf.pgNMService); // pointer to first member in struct
   for (ROWCOL col=1; col<=4; col++)
   {
      CGXStyle cellStyle;
      GetStyleRowCol(row, col, cellStyle);

      // don't paste to read-only cells
      if ( !cellStyle.GetIncludeReadOnly() || !cellStyle.GetReadOnly())
      {
         Float64 rounded = RoundOff(*pdbl,0.0001);
         cellStyle.SetValue(rounded);
         SetStyleRange(CGXRange(row,col), cellStyle);
      }

      pdbl++; // columns in grid are in same order as struct;
   }
}

// Make Tab key go to next control
BOOL CLLDFPierGrid::ProcessKeys(CWnd* pSender, UINT nMessage, UINT nChar, UINT nRepCnt, UINT flags)
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

            if (pWndNext != NULL)
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