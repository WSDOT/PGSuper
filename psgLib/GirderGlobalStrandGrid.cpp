///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// DiaphramGrid.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "GirderGlobalStrandGrid.h"
#include "StrandGridLocation.h"
#include "GirderHarpedStrandPage.h"
#include <system\tokenizer.h>

#include <EAF\EAFApp.h>
#include <PgsExt\GirderLabel.h>

#include "StrandGenerationDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static COLORREF DISABLED_COLOR = RGB(200,200,200);
static COLORREF SYS_COLOR = GetSysColor(COLOR_WINDOW);

static COLORREF GetEntryColor(CollectionIndexType entryIndex)
{
   return entryIndex%2 ? SYS_COLOR : DISABLED_COLOR;
}

// get number of grid rows needed for an entry
static ROWCOL GetEntryLoad(const CGirderGlobalStrandGrid::GlobalStrandGridEntry& entry, bool useHarpedGrid)
{
   if (0.0 < entry.m_X)
   {
      return 2;
   }
   else if (useHarpedGrid && entry.m_Type==GirderLibraryEntry::stAdjustable && 0.0 < entry.m_Hend_X)
   {
      return 2;
   }
   else
   {
      return 1;
   }
}

/////////////////////////////////////////////////////////////////////////////
// CGirderGlobalStrandGrid

CGirderGlobalStrandGrid::CGirderGlobalStrandGrid(CGirderGlobalStrandGridClient* pClient):
m_pClient(pClient)
{
   ATLASSERT(m_pClient!=0);
}

CGirderGlobalStrandGrid::~CGirderGlobalStrandGrid()
{
}

BEGIN_MESSAGE_MAP(CGirderGlobalStrandGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CGirderGlobalStrandGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_COMMAND(ID_EDIT_ROW, OnEditrow)
	ON_COMMAND(ID_EDIT_INSERTROW, OnEditInsertrow)
	ON_COMMAND(ID_EDIT_REMOVEROWS, OnEditRemoverows)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGirderGlobalStrandGrid message handlers

int CGirderGlobalStrandGrid::GetColWidth(ROWCOL nCol)
{
   if ( IsColHidden(nCol) )
   {
      return CGXGridWnd::GetColWidth(nCol);
   }
   else
   {
	   int width = GetGridRect( ).Width( );

      switch (nCol)
      {
      case 0:
         return width/7;

      default:
         return width/6;
      }
   }
}
BOOL CGirderGlobalStrandGrid::OnGridKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{

   ROWCOL row = GetSelectedRow();
   if (row < 1)
   {
      // allow keyboarders to get into first row if there is one
      if (m_Entries.empty())
      {
         return FALSE;
      }
      else
      {
         row=1;
         SelectRow(row);
      }
   }

   if (nChar==VK_DOWN)
   {
      CollectionIndexType orig_idx = GetRowEntry(row);
      if ( (m_Entries.size()-1) <= orig_idx )
      {
         ::MessageBeep(MB_ICONASTERISK);
      }
      else
      {
         CollectionIndexType idx=orig_idx;
         while(idx==orig_idx)
         {
            idx = GetRowEntry(++row);
         }
      }

      SetCurrentCell(row, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	   ScrollCellInView(row+1, GetLeftCol());
      SelectRow(row);
   }
   else if (nChar==VK_UP)
   {
      CollectionIndexType orig_idx =GetRowEntry(row);
      if (orig_idx==0)
      {
         ::MessageBeep(MB_ICONASTERISK);
      }
      else
      {
         CollectionIndexType idx=orig_idx;
         while(idx==orig_idx)
         {
            idx = GetRowEntry(--row);
         }
      }

      SetCurrentCell(row, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	   ScrollCellInView(row-1, GetLeftCol());
      SelectRow(row);
   }

   return TRUE;
}

BOOL CGirderGlobalStrandGrid::OnRButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   this->SelectRow(nRow);

   if (0 < nRow)
   {
	    // unreferenced parameters
	    nRow, nCol, nFlags;

	   CMenu menu;
	   VERIFY(menu.LoadMenu(IDR_ADD_DELETE_POPUP));

	   CMenu* pPopup = menu.GetSubMenu( 0 );
	   ASSERT( pPopup != NULL );

	   // display the menu
	   ClientToScreen(&pt);
	   pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
	   pt.x, pt.y, this);
   }

	// we processed the message
	return TRUE;
}

BOOL CGirderGlobalStrandGrid::OnLButtonDblClkRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   if (0 < nRow)
   {
      EditSelectedRow();
   }

	return TRUE;
}

void CGirderGlobalStrandGrid::EditSelectedRow()
{
   ROWCOL row = GetSelectedRow();
   CollectionIndexType idx =GetRowEntry(row);
   GlobalStrandGridEntry& entry = m_Entries[idx];

   // existing number of rows
   pgsTypes::AdjustableStrandType adj_type = m_pClient->GetAdjustableStrandType();
   bool use_harped = m_pClient->DoUseHarpedGrid();

   ROWCOL old_nrows = GetEntryLoad(entry, use_harped);

   if (EditEntry(row, entry, false))
   {
      // entry was changed - we must delete or append a row if needed and refill grid
      ROWCOL new_nrows = GetEntryLoad(entry, use_harped);
      if (old_nrows < new_nrows)
      {
      	InsertRows(row, 1);
      }
      else if (new_nrows < old_nrows)
      {
		   RemoveRows(row+1, row+1);
      }

      COLORREF curr_color  =  GetEntryColor(idx);

      FillRowsWithEntry(row, entry, use_harped, adj_type, curr_color);
      SelectRow(row);

      OnChangeStrandData();
   }
}


BOOL CGirderGlobalStrandGrid::OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   SelectRow(nRow);

   return TRUE;
}

void CGirderGlobalStrandGrid::InsertSelectedRow()
{
   ROWCOL sel_row = GetSelectedRow();
   CollectionIndexType idx =GetRowEntry(sel_row);
   EntryIteratorType entry_iter = m_Entries.begin()+idx;
   ATLASSERT(entry_iter != m_Entries.end());

   GlobalStrandGridEntry entry;
   if (EditEntry(sel_row, entry, true))
   {
      pgsTypes::AdjustableStrandType adj_type = m_pClient->GetAdjustableStrandType();
      bool use_harped = m_pClient->DoUseHarpedGrid();

      // entry selected - we must add some new rows
      ROWCOL new_nrows = GetEntryLoad(entry, use_harped);
	   InsertRows(sel_row, new_nrows);

      // add entry to data store
      m_Entries.insert(entry_iter, entry);

      // draw new entry and all below it
      CollectionIndexType size = m_Entries.size();
      ROWCOL row = sel_row;
      for (CollectionIndexType cnt=idx; cnt<size; cnt++)
      {
         COLORREF curr_color  =  GetEntryColor(cnt);
         row += FillRowsWithEntry(row, m_Entries[cnt], use_harped, adj_type, curr_color);
      }

      SetCurrentCell(sel_row, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	   ScrollCellInView(sel_row, GetLeftCol());
      SelectRow(sel_row);
      Invalidate();

      OnChangeStrandData();
   }
}


void CGirderGlobalStrandGrid::OnEditInsertrow()
{
   InsertSelectedRow();
}

void CGirderGlobalStrandGrid::OnEditrow()
{
   EditSelectedRow();
}

void CGirderGlobalStrandGrid::OnEditAppendrow()
{
   AppendSelectedRow();
}

void CGirderGlobalStrandGrid::AppendSelectedRow()
{
	ROWCOL nrow = 0;
   nrow = GetRowCount()+1;

   GlobalStrandGridEntry entry;
   if (EditEntry(nrow, entry, true))
   {
      pgsTypes::AdjustableStrandType adj_type = m_pClient->GetAdjustableStrandType();
      bool use_harped = m_pClient->DoUseHarpedGrid();

      // entry was changed - we must add some new rows
      Appendrow();

      ROWCOL new_nrows = GetEntryLoad(entry, use_harped);
      if (1 < new_nrows)
      {
         Appendrow();
      }

      // add entry to data store
      m_Entries.push_back(entry);

      CollectionIndexType idx = m_Entries.size()-1;
      COLORREF curr_color  =  GetEntryColor(idx);


      FillRowsWithEntry(nrow, entry, use_harped, adj_type, curr_color);
      SelectRow(nrow);
   }

   nrow = GetRowCount();
   SetCurrentCell(nrow, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	ScrollCellInView(nrow, GetLeftCol());
	Invalidate();

   OnChangeStrandData();
}

ROWCOL CGirderGlobalStrandGrid::Appendrow()
{
	ROWCOL nRow = GetRowCount()+1;

	InsertRows(nRow, 1);

   return nRow;
}

void CGirderGlobalStrandGrid::AppendEntry(CGirderGlobalStrandGrid::GlobalStrandGridEntry& entry)
{
	ROWCOL nrow = 0;
   nrow = GetRowCount()+1;

   pgsTypes::AdjustableStrandType adj_type = m_pClient->GetAdjustableStrandType();
   bool use_harped = m_pClient->DoUseHarpedGrid();

   // entry was changed - we must add some new rows
   Appendrow();

   ROWCOL new_nrows = GetEntryLoad(entry, use_harped);
   if (1 < new_nrows)
   {
      Appendrow();
   }

   // add entry to data store
   m_Entries.push_back(entry);

   CollectionIndexType idx = m_Entries.size()-1;
   COLORREF curr_color  =  GetEntryColor(idx);


   FillRowsWithEntry(nrow, entry, use_harped, adj_type, curr_color);
   SelectRow(nrow);

   nrow = GetRowCount();
   SetCurrentCell(nrow, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	ScrollCellInView(nrow, GetLeftCol());
	Invalidate();

   OnChangeStrandData();
}

void CGirderGlobalStrandGrid::OnEditRemoverows()
{
   RemoveSelectedRow();
}

void CGirderGlobalStrandGrid::RemoveSelectedRow()
{
   ROWCOL sel_row = GetSelectedRow();
   CollectionIndexType idx =GetRowEntry(sel_row);
   EntryIteratorType entry_iter = m_Entries.begin()+idx;
   ATLASSERT(entry_iter != m_Entries.end());

   // get number of rows used by entry and delete 'em
   pgsTypes::AdjustableStrandType adj_type = m_pClient->GetAdjustableStrandType();
   bool use_harped = m_pClient->DoUseHarpedGrid();
   ROWCOL nrows = GetEntryLoad(*entry_iter, use_harped);
   RemoveRows(sel_row, sel_row+nrows-1);

   // delete entry from data store
   m_Entries.erase(entry_iter);

   // now refill trailing entries so color matches
   CollectionIndexType size = m_Entries.size();

   ROWCOL row = sel_row;
   for (CollectionIndexType cnt=idx; cnt<size; cnt++)
   {
      COLORREF curr_color  =  GetEntryColor(cnt);
      row += FillRowsWithEntry(row, m_Entries[cnt], use_harped, adj_type, curr_color);
   }

   if (0 < row)
   {
      sel_row = Min(sel_row, GetRowCount());

      SetCurrentCell(sel_row, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	   ScrollCellInView(sel_row, GetLeftCol());
      SelectRow(sel_row);
   }

   Invalidate();

   OnChangeStrandData();
}

void CGirderGlobalStrandGrid::MoveUpSelectedRow()
{
   ROWCOL sel_row = GetSelectedRow();
   if (sel_row<2)
   {
      ::MessageBeep(MB_ICONASTERISK);
   }
   else
   {
      CollectionIndexType idx =GetRowEntry(sel_row);
      EntryIteratorType entry_iter = m_Entries.begin()+idx;
      ATLASSERT(entry_iter!=m_Entries.end());

      // find top row using number of rows we have to move up
      pgsTypes::AdjustableStrandType adj_type = m_pClient->GetAdjustableStrandType();
      bool use_harped = m_pClient->DoUseHarpedGrid();
      EntryIteratorType above_iter = entry_iter-1;
      ROWCOL load = GetEntryLoad(*above_iter, use_harped);
      ROWCOL top_row = sel_row-load;

      // deal with data move
      GlobalStrandGridEntry entry = *entry_iter;
      EntryIteratorType pivot_iter = m_Entries.erase(entry_iter);
      pivot_iter--;
      m_Entries.insert(pivot_iter, entry);
      idx--;

      // draw moved entry and all below it
      CollectionIndexType size = m_Entries.size();
      ROWCOL row = top_row;
      for (CollectionIndexType cnt=idx; cnt<size; cnt++)
      {
         COLORREF curr_color  =  GetEntryColor(cnt);
         row += FillRowsWithEntry(row, m_Entries[cnt], use_harped, adj_type, curr_color);
      }

      SetCurrentCell(top_row, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	   ScrollCellInView(top_row, GetLeftCol());
      SelectRow(top_row);
      Invalidate();

      OnChangeStrandData();
   }
}

void CGirderGlobalStrandGrid::MoveDownSelectedRow()
{
   ROWCOL sel_row = GetSelectedRow();
   CollectionIndexType idx =GetRowEntry(sel_row);
   CollectionIndexType num_entries = m_Entries.size();
   if (num_entries-1 <= idx)
   {
      ::MessageBeep(MB_ICONASTERISK);
   }
   else
   {
      pgsTypes::AdjustableStrandType adj_type = m_pClient->GetAdjustableStrandType();
      bool use_harped = m_pClient->DoUseHarpedGrid();

      // deal with data move
      EntryIteratorType entry_iter = m_Entries.begin()+idx;
      GlobalStrandGridEntry entry = *entry_iter;

      EntryIteratorType pivot_iter = m_Entries.erase(entry_iter);

      // get number of rows taken by entry below. must do before pivot gets hosed by insert
      ROWCOL top_row = sel_row+GetEntryLoad(*pivot_iter, use_harped);

      pivot_iter++;
      m_Entries.insert(pivot_iter, entry);

      // draw pivot entry and all below it
      CollectionIndexType size = m_Entries.size();
      ROWCOL row = sel_row;
      for (CollectionIndexType cnt=idx; cnt<size; cnt++)
      {
         COLORREF curr_color  =  GetEntryColor(cnt);
         row += FillRowsWithEntry(row, m_Entries[cnt], use_harped, adj_type, curr_color);
      }

      SetCurrentCell(top_row, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	   ScrollCellInView(top_row+1, GetLeftCol());
      SelectRow(top_row);
      Invalidate();

      OnChangeStrandData();
   }
}

void CGirderGlobalStrandGrid::OnChangeUseHarpedGrid()
{
   bool use_harped = m_pClient->DoUseHarpedGrid();
   BOOL st;
   if (use_harped)
   {
      st=HideCols(4,5,FALSE);

      ROWCOL row = GetSelectedRow();

      // this could possibily change the total size of the grid
      ROWCOL num_e_rows = GetRowsForEntries();

      if (0 < num_e_rows)
      {
    	   ROWCOL nrows = GetRowCount();
    	   ROWCOL rows_needed = num_e_rows - nrows;
         // size grid
         if (0 < rows_needed)
         {
         	InsertRows(1, rows_needed);
         }

         RedrawGrid();

         row = Min(row, num_e_rows);

         SetCurrentCell(row, 1, GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	      ScrollCellInView(row,1);
         SelectRow(row);
      }
   }
   else
   {
      st=HideCols(4,5);

      ROWCOL row = GetSelectedRow();

      // this could possibily change the total size of the grid
      ROWCOL num_e_rows = GetRowsForEntries();

      if (0 < num_e_rows)
      {
    	   ROWCOL nrows = GetRowCount();
    	   ROWCOL rows_2delete = nrows - num_e_rows;
         // size grid
         if (0 < rows_2delete)
         {
         	RemoveRows(1, rows_2delete);
         }

         RedrawGrid();

         row = Min(row, nrows-rows_2delete);

         SetCurrentCell(row, 1, GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	      ScrollCellInView(row,1);
         SelectRow(row);
      }
   }

   OnChangeStrandData();
}

void CGirderGlobalStrandGrid::OnChangeWebStrandType()
{
   OnChangeUseHarpedGrid();
   this->RedrawGrid();

   OnChangeStrandData();
}

void CGirderGlobalStrandGrid::CustomInit()
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   // Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	this->Initialize( );

	this->GetParam( )->EnableUndo(FALSE);

   bool use_harped = m_pClient->DoUseHarpedGrid();

   const int num_rows = 0;
   const int num_cols = 5;

	this->SetRowCount(num_rows);
	this->SetColCount(num_cols);

   // disable left side
	this->SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

// set text along top row
	this->SetStyleRange(CGXRange(0,0), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Fill #"))
		);

   CString cv;
   cv.Format(_T("Xb (%s)"),pDisplayUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv.Format(_T("Yb (%s)"),pDisplayUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(0,2), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv = _T("Type");
	this->SetStyleRange(CGXRange(0,3), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv.Format(_T("Xt (%s)"),pDisplayUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(0,4), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv.Format(_T("Yt (%s)"),pDisplayUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(0,5), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   // make it so that text fits correctly in header row
	this->ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));

   // no dragging
   this->GetParam()->EnableMoveRows(FALSE);
   this->EnableOleDropTarget(GX_DNDDISABLED);

   // Turn off default selection behavior
   this->GetParam()->EnableSelection(FALSE);

   // don't allow users to resize grids
   this->GetParam( )->EnableTrackColWidth(0); 
   this->GetParam( )->EnableTrackRowHeight(0); 

	this->EnableIntelliMouse();
	this->SetFocus();

   if (!use_harped)
   {
      VERIFY(HideCols(4,5));
   }

	this->GetParam( )->EnableUndo(TRUE);
}


void CGirderGlobalStrandGrid::FillGrid(EntryCollectionType& entries)
{
   GetParam()->SetLockReadOnly(FALSE);

   if ( 1 <= GetRowCount() )
   {
      RemoveRows(1,GetRowCount());
   }

   m_Entries = entries;
   long num_rows = GetRowsForEntries();

   if (0 < num_rows)
   {
      // size grid
   	InsertRows(1, num_rows);

      RedrawGrid();

      SetCurrentCell(1, 1, GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	   ScrollCellInView(1,1);
   }

   GetParam()->SetLockReadOnly(TRUE);

   OnChangeStrandData();
}

void CGirderGlobalStrandGrid::RedrawGrid()
{
   pgsTypes::AdjustableStrandType adj_type = m_pClient->GetAdjustableStrandType();
   bool use_harped = m_pClient->DoUseHarpedGrid();

   // fill grid
   ROWCOL nRow=1;
   long idx=0;
   for (EntryIteratorType it=m_Entries.begin(); it!=m_Entries.end(); it++)
   {
      COLORREF curr_color =  GetEntryColor(idx++);

      nRow += FillRowsWithEntry(nRow, *it, use_harped, adj_type, curr_color);
   }
}

ROWCOL CGirderGlobalStrandGrid::FillRowsWithEntry(ROWCOL nRow, GlobalStrandGridEntry& entry, bool useHarpedEnd, pgsTypes::AdjustableStrandType asType, COLORREF currColor)
{
   GetParam()->SetLockReadOnly(FALSE);

   ROWCOL nrow_orig = nRow;
   Float64 x = entry.m_X;
   Float64 y = entry.m_Y;

   CString stype;
   if (entry.m_Type == GirderLibraryEntry::stStraight)
   {
      if (entry.m_CanDebond)
      {
         stype = _T("Straight-DB");
      }
      else
      {
         stype = _T("Straight");
      }
   }
   else if (entry.m_Type == GirderLibraryEntry::stAdjustable)
   {
      stype = LOCAL_LABEL_HARP_TYPE(asType);
   }
   else
   {
      ATLASSERT(false);
   }

   this->SetStyleRange(CGXRange(nRow,0), CGXStyle()
         .SetEnabled(FALSE));

   this->SetStyleRange(CGXRange(nRow,1,nRow,5), CGXStyle()
         .SetEnabled(FALSE)
         .SetHorizontalAlignment(DT_RIGHT)
         .SetInterior(currColor));

   SetValueRange(CGXRange(nRow, 1), x);
   SetValueRange(CGXRange(nRow, 2), y);
   SetValueRange(CGXRange(nRow, 3), stype);

   // fill harped if need be
   if (useHarpedEnd && entry.m_Type == GirderLibraryEntry::stAdjustable )
   {
      SetValueRange(CGXRange(nRow, 4), entry.m_Hend_X);
      SetValueRange(CGXRange(nRow, 5), entry.m_Hend_Y);
   }
   else
   {
      ClearCells(CGXRange(nRow,4,nRow,5));

      SetStyleRange(CGXRange(nRow,4,nRow,5), CGXStyle()
            .SetWrapText(TRUE)
			   .SetEnabled(FALSE)
            .SetInterior(DISABLED_COLOR));
   }
   
   ROWCOL rows_used = GetEntryLoad(entry, useHarpedEnd);
   if (rows_used == 2)
   {
      nRow++;

      // add second row of negative x values
      SetValueRange(CGXRange(nRow, 1), -x);
      SetValueRange(CGXRange(nRow, 2), y);
      SetValueRange(CGXRange(nRow, 3), stype);

      SetStyleRange(CGXRange(nRow,1,nRow,5), CGXStyle()
            .SetEnabled(FALSE)
            .SetHorizontalAlignment(DT_RIGHT)
            .SetInterior(currColor));

      if (useHarpedEnd && entry.m_Type == GirderLibraryEntry::stAdjustable )
      {
         SetValueRange(CGXRange(nRow, 4), -entry.m_Hend_X);
         SetValueRange(CGXRange(nRow, 5), entry.m_Hend_Y);
      }
      else
      {
         ClearCells(CGXRange(nRow,4,nRow,5));

         SetStyleRange(CGXRange(nRow,4,nRow,5), CGXStyle()
               .SetWrapText(TRUE)
			      .SetEnabled(FALSE)
               .SetInterior(DISABLED_COLOR));
      }
   }

   GetParam()->SetLockReadOnly(TRUE);

   return rows_used;
}

ROWCOL CGirderGlobalStrandGrid::GetSelectedRow()
{
   ROWCOL nRow;
	// if there are no cells selected, return 0
	CGXRangeList selList;
	if (CopyRangeList(selList, TRUE))
   {
		nRow = selList.GetHead()->top;
   }
	else
   {
		nRow = 0;
   }
   
   return nRow;
}

void CGirderGlobalStrandGrid::SelectRow(ROWCOL nRow)
{
   ROWCOL ncols = 5;

   // unselect currently selected rows
   CRowColArray awRows;
   GetSelectedRows(awRows);
   INT_PTR cnt = awRows.GetSize();
   for(INT_PTR idx=0; idx<cnt; idx++)
   {
      ROWCOL row = awRows[idx];
      SelectRange(CGXRange(row,0,row,ncols), FALSE);
   }

   if (0 < nRow)
   {
      // select all rows of same color adjacent to this one
      ROWCOL toprow = nRow;
      ROWCOL botrow = nRow;

      CGXStyle mstyle;
      GetStyleRowCol(nRow, 1, mstyle);
      CGXBrush mbrush = mstyle.GetInterior();

      // row above
      if (1 < nRow)
      {
         CGXStyle tstyle;
         GetStyleRowCol(nRow-1, 1, tstyle);
         if (tstyle.GetInterior() == mbrush)
         {
            toprow = nRow-1;
         }
      }

      // row below
      CGXStyle bstyle;
      if (GetStyleRowCol(nRow+1, 1, bstyle))
      {
         if (bstyle.GetInterior() == mbrush)
         {
            botrow = nRow+1;
         }
      }

      SelectRange(CGXRange(toprow,0,botrow,ncols), TRUE);
   }

   m_pClient->OnEnableDelete(nRow!=0);
}

CollectionIndexType CGirderGlobalStrandGrid::GetRowEntry(ROWCOL nRow)
{
   ATLASSERT(0 < nRow && nRow <= GetRowCount());

   bool use_harped = m_pClient->DoUseHarpedGrid();

   // brute search from start
   CollectionIndexType global_index=0;
   ROWCOL curr_row=0;
   for(CGirderGlobalStrandGrid::EntryIteratorType it2=m_Entries.begin(); it2!=m_Entries.end(); it2++)
   {
      CGirderGlobalStrandGrid::GlobalStrandGridEntry& grid_entry = *it2;
      
      ROWCOL nrows = GetEntryLoad(grid_entry, use_harped);
      curr_row+= nrows;
      if ( nRow <= curr_row )
      {
         break;
      }
      else
      {
         global_index++;
      }
   }

   return global_index;
}

ROWCOL CGirderGlobalStrandGrid::GetRowsForEntries()
{
   bool use_harped = m_pClient->DoUseHarpedGrid();

   ROWCOL cnt = 0;
   for(CGirderGlobalStrandGrid::EntryIteratorType it2=m_Entries.begin(); it2!=m_Entries.end(); it2++)
   {
      CGirderGlobalStrandGrid::GlobalStrandGridEntry& grid_entry = *it2;

      ROWCOL nrows = GetEntryLoad(grid_entry, use_harped);

      cnt += nrows;
   }

   return cnt;
}

bool CGirderGlobalStrandGrid::EditEntry(ROWCOL row, GlobalStrandGridEntry& entry, bool isNewEntry)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   pgsTypes::AdjustableStrandType adj_type = m_pClient->GetAdjustableStrandType();
   bool use_harped = m_pClient->DoUseHarpedGrid();

   CStrandGridLocation dlg;

   dlg.m_UnitString = pDisplayUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str();
   dlg.m_UnitString.TrimLeft(_T("("));
   dlg.m_UnitString.TrimRight(_T(")"));

   dlg.m_Row = row;
   dlg.SetEntry(entry, use_harped, adj_type);

   INT_PTR st = dlg.DoModal();
   if (st==IDOK)
   {
      GlobalStrandGridEntry new_entry = dlg.GetEntry();
      if (isNewEntry || entry != new_entry)
      {
         entry = new_entry;
         return true;
      }
   }

   return false;
}


void CGirderGlobalStrandGrid::OnChangeStrandData()
{
   bool use_harped = m_pClient->DoUseHarpedGrid();

   // count number of strands
   Uint16 ns(0), ndb(0), nh(0);
   for(EntryIteratorType iter=m_Entries.begin(); iter!=m_Entries.end(); iter++)
   {
      const GlobalStrandGridEntry& entry = *iter;
      if (entry.m_Type==GirderLibraryEntry::stStraight)
      {
         ns++;

         if (entry.m_CanDebond)
         {
            ndb++;
         }

         if (0.0 < entry.m_X)
         {
            ns++;

            if (entry.m_CanDebond)
            {
               ndb++;
            }
         }
      }
      else if (entry.m_Type == GirderLibraryEntry::stAdjustable)
      {
         nh++;

         if (0.0 < entry.m_X)
         {
            nh++;
         }
         else if (use_harped)
         {
            if (0.0 < entry.m_Hend_X)
            {
               nh++;
            }
         }
      }
   }

   m_pClient->UpdateStrandStatus(ns, ndb, nh);
}

void CGirderGlobalStrandGrid::ReverseHarpedStrandOrder()
{
   // collect all the harped strand entries in a temporary vector
   EntryCollectionType harped_strands;
   EntryIteratorType iter;
   for ( iter = m_Entries.begin(); iter != m_Entries.end(); iter++ )
   {
      CGirderGlobalStrandGrid::GlobalStrandGridEntry& entry = *iter;
      if ( entry.m_Type == GirderLibraryEntry::stAdjustable )
      {
         harped_strands.push_back(entry);
      }
   }

   // reverse the order of the entries
   std::reverse(harped_strands.begin(),harped_strands.end());

   // do a 1 for 1 replacement of the harped strands in the main entry vector
   CollectionIndexType idx = 0;
   for ( iter = m_Entries.begin(); iter != m_Entries.end(); iter++ )
   {
      CGirderGlobalStrandGrid::GlobalStrandGridEntry& entry = *iter;
      if ( entry.m_Type == GirderLibraryEntry::stAdjustable )
      {
         entry = harped_strands[idx++];
      }
   }

   RedrawGrid();
}

void CGirderGlobalStrandGrid::GenerateStrandPositions()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   CStrandGenerationDlg dlg; // this dialog works in system units
   dlg.m_DoUseHarpedGrid = m_pClient->DoUseHarpedGrid();
   dlg.m_AdjustableStrandType = m_pClient->GetAdjustableStrandType();

   dlg.m_Xstart = ::ConvertToSysUnits(1.0,unitMeasure::Inch);
   dlg.m_Ystart = ::ConvertToSysUnits(2.0,unitMeasure::Inch);
   dlg.m_nStrandsX = 8;
   dlg.m_nStrandsY = 1;
   
   dlg.m_LayoutType = CStrandGenerationDlg::ltSpacing;

   dlg.m_StrandGenerationType = CStrandGenerationDlg::sgSequential;

   Float64 spacing = ::ConvertToSysUnits(2.0,unitMeasure::Inch);
   dlg.m_Xend = spacing;
   dlg.m_Yend = spacing;

   dlg.m_StrandType = 0; // straight;
   dlg.m_Xstart2 = dlg.m_Xstart;
   dlg.m_Ystart2 = dlg.m_Ystart;
   dlg.m_Xend2 = dlg.m_Xend;
   dlg.m_Yend2 = dlg.m_Yend;

   if ( dlg.DoModal() == IDOK )
   {
      if ( dlg.m_StrandType == 0 )
      {
         GenerateStraightStrands(dlg);
      }
      else
      {
         GenerateHarpedStrands(dlg);
      }

   }
}

void CGirderGlobalStrandGrid::GenerateStraightStrands(CStrandGenerationDlg& dlg)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();


   if ( dlg.m_bDelete )
   {
      DeleteAllStraightStrands();
   }

   // strand grid must be generated in display units
   Float64 x_start = ::ConvertFromSysUnits(dlg.m_Xstart,pDisplayUnits->ComponentDim.UnitOfMeasure);
   Float64 y_start = ::ConvertFromSysUnits(dlg.m_Ystart,pDisplayUnits->ComponentDim.UnitOfMeasure);
   Float64 x_end,y_end, Xspacing,Yspacing;
   if ( dlg.m_LayoutType == CStrandGenerationDlg::ltSpacing )
   {
      Xspacing = ::ConvertFromSysUnits(dlg.m_Xend,  pDisplayUnits->ComponentDim.UnitOfMeasure);;
      Yspacing = ::ConvertFromSysUnits(dlg.m_Yend,  pDisplayUnits->ComponentDim.UnitOfMeasure);

      x_end = x_start + Xspacing*(dlg.m_nStrandsX-1);
      y_end = y_start + Yspacing*(dlg.m_nStrandsY-1);
   }
   else
   {
      x_end = ::ConvertFromSysUnits(dlg.m_Xend,  pDisplayUnits->ComponentDim.UnitOfMeasure);
      y_end = ::ConvertFromSysUnits(dlg.m_Yend,  pDisplayUnits->ComponentDim.UnitOfMeasure);

      Xspacing = (x_end-x_start)/(dlg.m_nStrandsX-1);
      Yspacing = (y_end-y_start)/(dlg.m_nStrandsY-1);
   }

   Int16 Nx1;
   Int16 strand_step;
   if (dlg.m_StrandGenerationType == CStrandGenerationDlg::sgSequential)
   {
      Nx1 = dlg.m_nStrandsX;
      strand_step = 1;
   }
   else
   {
      Nx1 = dlg.m_nStrandsX/2;
      strand_step = 2;
   }


   Int16 Nx2 = dlg.m_nStrandsX - Nx1;


   Float64 y = y_start;
   for ( Int16 i = 0; i < dlg.m_nStrandsY; i++ )
   {
      Float64 x = x_start;
      for ( Int16 j = 0; j < Nx1; j++ )
      {
         CGirderGlobalStrandGrid::GlobalStrandGridEntry entry;
         entry.m_Type = GirderLibraryEntry::stStraight;
         entry.m_CanDebond = false;
         entry.m_X = x;
         entry.m_Y = y;

         x += Xspacing*strand_step;

         AppendEntry(entry);
      }

      x = x_start + Xspacing;
      for ( Int16 j = 0; j < Nx2; j++ )
      {
         CGirderGlobalStrandGrid::GlobalStrandGridEntry entry;
         entry.m_Type = GirderLibraryEntry::stStraight;
         entry.m_CanDebond = false;
         entry.m_X = x;
         entry.m_Y = y;

         x += Xspacing*strand_step;

         AppendEntry(entry);
      }

      y += Yspacing;
   }
}

void CGirderGlobalStrandGrid::GenerateHarpedStrands(CStrandGenerationDlg& dlg)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   if ( dlg.m_bDelete )
   {
      DeleteAllHarpedStrands();
   }

   // strand grid must be generated in display units
   Float64 x_start_1 = ::ConvertFromSysUnits(dlg.m_Xstart, pDisplayUnits->ComponentDim.UnitOfMeasure);
   Float64 y_start_1 = ::ConvertFromSysUnits(dlg.m_Ystart, pDisplayUnits->ComponentDim.UnitOfMeasure);
   Float64 x_start_2 = ::ConvertFromSysUnits(dlg.m_Xstart2,pDisplayUnits->ComponentDim.UnitOfMeasure);
   Float64 y_start_2 = ::ConvertFromSysUnits(dlg.m_Ystart2,pDisplayUnits->ComponentDim.UnitOfMeasure);
   Float64 x_end_1,y_end_1, Xspacing_1,Yspacing_1;
   Float64 x_end_2,y_end_2, Xspacing_2,Yspacing_2;
   if ( dlg.m_LayoutType == CStrandGenerationDlg::ltSpacing )
   {
      Xspacing_1 = ::ConvertFromSysUnits(dlg.m_Xend,  pDisplayUnits->ComponentDim.UnitOfMeasure);;
      Yspacing_1 = ::ConvertFromSysUnits(dlg.m_Yend,  pDisplayUnits->ComponentDim.UnitOfMeasure);

      x_end_1 = x_start_1 + Xspacing_1*(dlg.m_nStrandsX-1);
      y_end_1 = y_start_1 + Yspacing_1*(dlg.m_nStrandsY-1);

      Xspacing_2 = ::ConvertFromSysUnits(dlg.m_Xend2,  pDisplayUnits->ComponentDim.UnitOfMeasure);;
      Yspacing_2 = ::ConvertFromSysUnits(dlg.m_Yend2,  pDisplayUnits->ComponentDim.UnitOfMeasure);

      x_end_2 = x_start_2 + Xspacing_1*(dlg.m_nStrandsX-1);
      y_end_2 = y_start_2 + Yspacing_1*(dlg.m_nStrandsY-1);
   }
   else
   {
      x_end_1 = ::ConvertFromSysUnits(dlg.m_Xend,  pDisplayUnits->ComponentDim.UnitOfMeasure);
      y_end_1 = ::ConvertFromSysUnits(dlg.m_Yend,  pDisplayUnits->ComponentDim.UnitOfMeasure);

      Xspacing_1 = (x_end_1-x_start_1)/(dlg.m_nStrandsX-1);
      Yspacing_1 = (y_end_1-y_start_1)/(dlg.m_nStrandsY-1);

      x_end_2 = ::ConvertFromSysUnits(dlg.m_Xend2,  pDisplayUnits->ComponentDim.UnitOfMeasure);
      y_end_2 = ::ConvertFromSysUnits(dlg.m_Yend2,  pDisplayUnits->ComponentDim.UnitOfMeasure);

      Xspacing_2 = (x_end_2-x_start_2)/(dlg.m_nStrandsX-1);
      Yspacing_2 = (y_end_2-y_start_2)/(dlg.m_nStrandsY-1);
   }

   Int16 Nx1;
   Int16 strand_step;
   if (dlg.m_StrandGenerationType == CStrandGenerationDlg::sgSequential)
   {
      Nx1 = dlg.m_nStrandsX;
      strand_step = 1;
   }
   else
   {
      Nx1 = dlg.m_nStrandsX/2;
      strand_step = 2;
   }


   Int16 Nx2 = dlg.m_nStrandsX - Nx1;


   Float64 y1 = y_start_1;
   Float64 y2 = y_start_2;
   for ( Int16 i = 0; i < dlg.m_nStrandsY; i++ )
   {
      Float64 x1 = x_start_1;
      Float64 x2 = x_start_2;

      for ( Int16 j = 0; j < Nx1; j++ )
      {
         CGirderGlobalStrandGrid::GlobalStrandGridEntry entry;
         entry.m_Type = GirderLibraryEntry::stAdjustable;
         entry.m_CanDebond = false;

         entry.m_X = x1;
         entry.m_Y = y1;

         entry.m_Hend_X = x2;
         entry.m_Hend_Y = y2;

         x1 += Xspacing_1*strand_step;
         x2 += Xspacing_2*strand_step;

         AppendEntry(entry);
      }

      x1 = x_start_1 + Xspacing_1;
      x2 = x_start_2 + Xspacing_2;
      for ( Int16 j = 0; j < Nx2; j++ )
      {
         CGirderGlobalStrandGrid::GlobalStrandGridEntry entry;
         entry.m_Type = GirderLibraryEntry::stAdjustable;
         entry.m_CanDebond = false;

         entry.m_X = x1;
         entry.m_Y = y1;

         entry.m_Hend_X = x2;
         entry.m_Hend_Y = y2;

         x1 += Xspacing_1*strand_step;
         x2 += Xspacing_2*strand_step;

         AppendEntry(entry);
      }

      y1 += Yspacing_1;
      y2 += Yspacing_2;
   }
}

bool straight_strand_pred(CGirderGlobalStrandGrid::GlobalStrandGridEntry& entry)
{ return entry.m_Type == GirderLibraryEntry::stStraight; }

bool harped_strand_pred(CGirderGlobalStrandGrid::GlobalStrandGridEntry& entry)
{ return entry.m_Type == GirderLibraryEntry::stAdjustable; }

void CGirderGlobalStrandGrid::DeleteAllStraightStrands()
{
   EntryCollectionType::iterator new_begin = std::remove_if(m_Entries.begin(),m_Entries.end(),straight_strand_pred); // doesn't actually remove
   m_Entries.erase(new_begin,m_Entries.end());
   FillGrid(m_Entries);
}

void CGirderGlobalStrandGrid::DeleteAllHarpedStrands()
{
   EntryCollectionType::iterator new_begin = std::remove_if(m_Entries.begin(),m_Entries.end(),harped_strand_pred); // doesn't actually remove
   m_Entries.erase(new_begin,m_Entries.end());
   FillGrid(m_Entries);
}
