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

// ShearBarsLegsGrid.cpp : implementation file
//
#include "stdafx.h"

#include "ShearBarsLegsGrid.h"
#include "ShearDesignPage.h"
#include <Units\Measure.h>
#include <EAF\EAFApp.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFUtilities.h>
#include <Lrfd\RebarPool.h>
#include <IFace\Tools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

 GRID_IMPLEMENT_REGISTER(CShearBarsLegsGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CShearBarsLegsGrid

CShearBarsLegsGrid::CShearBarsLegsGrid()
{
   RegisterClass();
}

CShearBarsLegsGrid::~CShearBarsLegsGrid()
{
}

BEGIN_MESSAGE_MAP(CShearBarsLegsGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CShearBarsLegsGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.

	ON_COMMAND(ID_EDIT_INSERTROW, OnEditInsertRow)
	ON_COMMAND(ID_EDIT_REMOVEROWS, OnEditRemoveRows)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REMOVEROWS, OnUpdateEditRemoveRows)

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CShearBarsLegsGrid message handlers

int CShearBarsLegsGrid::GetColWidth(ROWCOL nCol)
{
	CRect rect = GetGridRect( );
   ROWCOL numCol = GetColCount();
   int parts = 2*numCol + 1;

   switch (nCol)
   {
   case 0:
      return rect.Width( )/parts;
   default:
      return rect.Width( )*2/parts;
   }
}

BOOL CShearBarsLegsGrid::OnRButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	 // unreferenced parameters
	 nRow, nCol, nFlags;

	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_ADD_DELETE_POPUP));

	CMenu* pPopup = menu.GetSubMenu( 0 );
	ASSERT( pPopup != nullptr );

   // deal with disabling delete since update stuff doesn't seem to work right
   UINT dodel = EnableItemDelete() ? MF_ENABLED|MF_BYCOMMAND : MF_GRAYED|MF_BYCOMMAND;
   pPopup->EnableMenuItem(ID_EDIT_REMOVEROWS, dodel);

	// display the menu
	ClientToScreen(&pt);
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, 
	pt.x, pt.y, this);

	// we processed the message
	return TRUE;
}

BOOL CShearBarsLegsGrid::OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   CShearDesignPage* pdlg = (CShearDesignPage*)GetParent();
   ASSERT (pdlg);

   ROWCOL nrows = GetRowCount();

   if (nCol==0 && nRow!=0 )
      pdlg->OnEnableDelete(true);
   else
      pdlg->OnEnableDelete(false);

   return TRUE;
}

void CShearBarsLegsGrid::OnEditInsertRow()
{
   // call back to parent for this so things get set up correctly
   CShearDesignPage* pdlg = (CShearDesignPage*)GetParent();
   ASSERT (pdlg);

   pdlg->DoInsertRow();
}

void CShearBarsLegsGrid::InsertRow(bool bAppend)
{
	ROWCOL nRow = 0;

   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

	// if there are no cells selected,
	// copy the current cell's coordinates
	CGXRangeList selList;
	if (CopyRangeList(selList, TRUE))
		nRow = selList.GetHead()->top;
	else
      nRow = bAppend ? GetRowCount()+1 : 0;

	nRow = Max((ROWCOL)1, nRow);

	InsertRows(nRow, 1);
   SetRowStyle(nRow);
   
	ScrollCellInView(nRow+1, GetLeftCol());
}

void CShearBarsLegsGrid::OnEditRemoveRows()
{
   // call back to parent for this so things get set up correctly
   CShearDesignPage* pdlg = (CShearDesignPage*)GetParent();
   ASSERT (pdlg);

   pdlg->DoRemoveRows();
}

void CShearBarsLegsGrid::DoRemoveRows()
{
	CGXRangeList* pSelList = GetParam()->GetRangeList();
	if (pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1)
	{
		CGXRange range = pSelList->GetHead();
		range.ExpandRange(1, 0, GetRowCount(), 0);
		RemoveRows(range.top, range.bottom);
      SetCurrentCell(range.top,1); // if this is not here, the next insert will go below grid bottom
	}
}

void CShearBarsLegsGrid::OnUpdateEditRemoveRows(CCmdUI* pCmdUI)
{
   BOOL flag = EnableItemDelete() ? TRUE:FALSE;
   pCmdUI->Enable(flag);
}

bool CShearBarsLegsGrid::EnableItemDelete()
{
	if (GetParam() == nullptr)
		return false;

   ROWCOL nrows = GetRowCount();

	CGXRangeList* pSelList = GetParam()->GetRangeList();
	return (pSelList->IsAnyCellFromCol(0) && 
           pSelList->GetCount() == 1);
}

void CShearBarsLegsGrid::CustomInit()
{
   CEAFApp* pApp = EAFGetApp();

// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	Initialize( );

	GetParam( )->EnableUndo(FALSE);

   const int num_rows=0;
   const int num_cols=2;

	SetRowCount(num_rows);
	SetColCount(num_cols);

		// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

// set text along top row
	SetStyleRange(CGXRange(0,0), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Order\n#"))
		);

	SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Bar\nSize"))
		);

	SetStyleRange(CGXRange(0,2), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetValue(_T("# Of\nBars (Legs)\n"))
		);

   // make it so that text fits correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();
	SetFocus();

	GetParam( )->EnableUndo(TRUE);
}

void CShearBarsLegsGrid::SetRowStyle(ROWCOL nRow)
{
	GetParam()->EnableUndo(FALSE);

   CString strBarSizeChoiceList;
   lrfdRebarIter rebarIter(matRebar::A615,matRebar::Grade60,true);
   for ( rebarIter.Begin(); rebarIter; rebarIter.Next() )
   {
      const matRebar* pRebar = rebarIter.GetCurrentRebar();
      strBarSizeChoiceList += pRebar->GetName().c_str();
      strBarSizeChoiceList += _T("\n");
   }

   SetStyleRange(CGXRange(nRow,1), CGXStyle()
      .SetEnabled(TRUE)
      .SetReadOnly(FALSE)
      .SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
      .SetChoiceList(strBarSizeChoiceList)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(lrfdRebarPool::GetBarSize(matRebar::bs3).c_str())
      );

	SetStyleRange(CGXRange(nRow,2), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWN)
			.SetChoiceList(_T("1\n2\n4\n6\n"))
			.SetValue(_T("2"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

   GetParam()->EnableUndo(TRUE);
}

CString CShearBarsLegsGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

matRebar::Size CShearBarsLegsGrid::GetBarSize(ROWCOL row,ROWCOL col)
{
   assert(col==1);
   CString s = GetCellValue(row, col);
   s.TrimLeft();
   int l = s.GetLength();
   CString s2 = s.Right(l-1);
   int i = _tstoi(s2);
   if (s.IsEmpty() || (i==0))
      return matRebar::bsNone;

   switch(i)
   {
   case 3:  return matRebar::bs3;
   case 4:  return matRebar::bs4;
   case 5:  return matRebar::bs5;
   case 6:  return matRebar::bs6;
   case 7:  return matRebar::bs7;
   case 8:  return matRebar::bs8;
   case 9:  return matRebar::bs9;
   case 10: return matRebar::bs10;
   case 11: return matRebar::bs11;
   case 14: return matRebar::bs14;
   case 18: return matRebar::bs18;
   default: ATLASSERT(false);
   }

   return matRebar::bsNone;
}

bool CShearBarsLegsGrid::GetRowData(ROWCOL nRow, matRebar::Size* pSize, Float64* pnLegs)
{

   *pSize = GetBarSize(nRow,1);

   CString s = GetCellValue(nRow, 2);
   Float64 d = _tstof(s);
   if (s.IsEmpty() || (d==0.0 && s[0]!=_T('0')))
      *pnLegs = 0;
   else
      *pnLegs = d;

   return true;
}

CShearDesignPage::StirrupSizeBarComboColl CShearBarsLegsGrid::GetGridData() 
{
   CShearDesignPage::StirrupSizeBarComboColl coll;

   CShearDesignPage::StirrupSizeBarCombo cbo;

   ROWCOL nrows = GetRowCount();
   for(ROWCOL ir=1; ir<=nrows; ir++)
   {
      GetRowData(ir, &cbo.Size , &cbo.NLegs);

      coll.push_back(cbo);
   }

   return coll;
}

void CShearBarsLegsGrid::FillGrid(const CShearDesignPage::StirrupSizeBarComboColl& rvec)
{
	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   // remove all but top row
   ROWCOL rows = GetRowCount();
   if (rows>=1)
	   RemoveRows(1, rows);

   IndexType size = rvec.size();
   if (0 < size)
   {
      // size grid
      for (IndexType i=0; i<size; i++)
      {
	      InsertRow(true);
      }

      // fill grid
      ROWCOL nRow=1;
      for (CShearDesignPage::StirrupSizeBarComboConstIter it = rvec.begin(); it!=rvec.end(); it++)
      {

         CString tmp;
         tmp.Format(_T("%s"),lrfdRebarPool::GetBarSize((*it).Size).c_str());
         VERIFY(SetValueRange(CGXRange(nRow, 1), tmp));

         VERIFY(SetValueRange(CGXRange(nRow, 2), (*it).NLegs));

         nRow++;
      }
   }
   else
   {
	   InsertRow(true);
   }

	ScrollCellInView(1, GetLeftCol());

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}
