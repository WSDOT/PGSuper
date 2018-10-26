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

// DiaphramGrid.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "DiaphragmGrid.h"
#include "GirderDiaphragmPage.h"
#include <system\tokenizer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDiaphragmGrid

CDiaphragmGrid::CDiaphragmGrid()
{
}

CDiaphragmGrid::~CDiaphragmGrid()
{
}

BEGIN_MESSAGE_MAP(CDiaphragmGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CDiaphragmGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDiaphragmGrid message handlers

int CDiaphragmGrid::GetColWidth(ROWCOL nCol)
{
	CRect rect = GetGridRect( );
   if ( nCol == 0 )
      return rect.Width();
   else
      return CGXGridWnd::GetColWidth(nCol);
}

BOOL CDiaphragmGrid::OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   CGirderDiaphragmPage* pdlg = (CGirderDiaphragmPage*)GetParent();
   ASSERT (pdlg);

   if (nCol==0 && nRow!=0)
      pdlg->OnEnableDelete(true);
   else
      pdlg->OnEnableDelete(false);

   return TRUE;
}

BOOL CDiaphragmGrid::OnLButtonDblClkRowCol(ROWCOL nRow,ROWCOL nCol,UINT nFlags,CPoint pt)
{
   CGirderDiaphragmPage* pdlg = (CGirderDiaphragmPage*)GetParent();
   ASSERT (pdlg);

   if (nCol==0 && nRow!=0)
      pdlg->OnEdit();

   return TRUE;
}

void CDiaphragmGrid::RemoveRows()
{
	CGXRangeList* pSelList = GetParam()->GetRangeList();
	if (pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1)
	{
		CGXRange range = pSelList->GetHead();
		range.ExpandRange(1, 0, GetRowCount(), 0);
      CGXGridCore::RemoveRows(range.top, range.bottom);
      int remove_row = range.bottom - 1;
      m_Rules.erase(m_Rules.begin() + remove_row);
	}
}

void CDiaphragmGrid::CustomInit()
{
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	this->Initialize( );

	this->GetParam( )->EnableUndo(FALSE);

   const int num_rows=0;
   const int num_cols=0;

	this->SetRowCount(num_rows);
	this->SetColCount(num_cols);

	// Turn off selecting whole columns when clicking on a column header
	this->GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   // set text along top row
	this->SetStyleRange(CGXRange(0,0), CGXStyle()
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Intermediate Diaphragm Definition"))
		);

   
   // make it so that text fits correctly in header row
	this->ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));

   // don't allow users to resize grids
   this->GetParam( )->EnableTrackColWidth(0); 
   this->GetParam( )->EnableTrackRowHeight(0); 

	this->EnableIntelliMouse();
	this->SetFocus();

	this->GetParam( )->EnableUndo(TRUE);
}

void CDiaphragmGrid::SetRules(const GirderLibraryEntry::DiaphragmLayoutRules& rules)
{
   GetParam()->EnableUndo(FALSE);

   m_Rules = rules; //this will create a local copy for the grid and subsequent dialogs to operate on

   // empty the grid
   if ( 0 < GetRowCount() )
      CGXGridWnd::RemoveRows(1,GetRowCount());

   // now fill it up
   GirderLibraryEntry::DiaphragmLayoutRules::iterator iter;
   for ( iter = m_Rules.begin(); iter != m_Rules.end(); iter++ )
   {
      GirderLibraryEntry::DiaphragmLayoutRule& rule = *iter;

      AppendRow();
      SetRowData(GetRowCount(),rule);
   }

   GetParam()->EnableUndo(TRUE);
}

void CDiaphragmGrid::GetRules(GirderLibraryEntry::DiaphragmLayoutRules& rules)
{
   rules = m_Rules;
}

void CDiaphragmGrid::AppendRow()
{
	ROWCOL nRow = 0;
   nRow = GetRowCount()+1;

	InsertRows(nRow, 1);
	ScrollCellInView(nRow, GetLeftCol());
	Invalidate();
}

void CDiaphragmGrid::AddRow()
{
   AppendRow();
   GirderLibraryEntry::DiaphragmLayoutRule newRule;
   m_Rules.push_back(newRule);

   ROWCOL nRow = GetRowCount();

   SetRowData(nRow,m_Rules.back());
}

void CDiaphragmGrid::SetRowData(ROWCOL nRow,GirderLibraryEntry::DiaphragmLayoutRule& rule)
{
	GetParam()->EnableUndo(FALSE);

   SetStyleRange(CGXRange(nRow,0), CGXStyle()
      .SetControl(GX_IDS_CTRL_STATIC)
      .SetValue(rule.Description.c_str())
      .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
      .SetInterior(::GetSysColor(COLOR_WINDOW))
      );

   GetParam()->EnableUndo(TRUE);
}

GirderLibraryEntry::DiaphragmLayoutRule* CDiaphragmGrid::GetRule(ROWCOL row)
{
   int idx = row - 1;
   return &m_Rules[idx];
}

void CDiaphragmGrid::SetRule(ROWCOL row,GirderLibraryEntry::DiaphragmLayoutRule& rule)
{
   m_Rules[row-1] = rule;
   GirderLibraryEntry::DiaphragmLayoutRule& theRule = m_Rules[row-1];
   SetRowData(row,theRule);
}
