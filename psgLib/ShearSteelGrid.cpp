///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

// BridgeDescShearGrid.cpp : implementation file
//
#include "stdafx.h"

#include "ShearSteelGrid.h"
#include <psgLib\ShearSteelPage.h>
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

 GRID_IMPLEMENT_REGISTER(CShearSteelGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CShearSteelGrid

CShearSteelGrid::CShearSteelGrid()
{
   RegisterClass();
}

CShearSteelGrid::~CShearSteelGrid()
{
}

BEGIN_MESSAGE_MAP(CShearSteelGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CShearSteelGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.

	ON_COMMAND(ID_EDIT_INSERTROW, OnEditInsertRow)
	ON_COMMAND(ID_EDIT_REMOVEROWS, OnEditRemoveRows)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REMOVEROWS, OnUpdateEditRemoveRows)

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CShearSteelGrid message handlers

int CShearSteelGrid::GetColWidth(ROWCOL nCol)
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

BOOL CShearSteelGrid::OnRButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	 // unreferenced parameters
	 nRow, nCol, nFlags;

	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_ADD_DELETE_POPUP));

	CMenu* pPopup = menu.GetSubMenu( 0 );
	ASSERT( pPopup != NULL );

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

BOOL CShearSteelGrid::OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   CShearSteelPage* pdlg = (CShearSteelPage*)GetParent();
   ASSERT (pdlg);

   ROWCOL nrows = GetRowCount();

   if (nCol==0 && (nRow!=0 && nRow!=nrows))
      pdlg->OnEnableDelete(true);
   else
      pdlg->OnEnableDelete(false);

   return TRUE;
}

void CShearSteelGrid::OnEditInsertRow()
{
   // call back to parent for this so things get set up correctly
   CShearSteelPage* pdlg = (CShearSteelPage*)GetParent();
   ASSERT (pdlg);

   pdlg->DoInsertRow();
}

void CShearSteelGrid::InsertRow(bool bAppend)
{
	ROWCOL nRow = 0;

   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

	// if there are no cells selected,
	// copy the current cell's coordinates
	CGXRangeList selList;
	if (CopyRangeList(selList, TRUE))
		nRow = selList.GetHead()->top;
	else
      nRow = bAppend ? GetRowCount()+1 : 0;

	nRow = max(1, nRow);

	InsertRows(nRow, 1);
   SetRowStyle(nRow);
   
   // put in some reasonable defaults
   Float64 zonlen = 0.0;
   CString cval;
   cval.Format(_T("%g"),zonlen);
   SetStyleRange(CGXRange(nRow,1), CGXStyle().SetValue(cval));

   // zone length in the last row is infinite.
   ROWCOL nrows = GetRowCount();
   if (nrows==1)
   {
      CString lastzlen = (m_IsSymmetrical) ? _T("to mid-span") : _T("to girder end");
	   SetStyleRange(CGXRange(1,1), CGXStyle()
		.SetControl(GX_IDS_CTRL_STATIC)
		.SetValue(lastzlen)
      .SetReadOnly(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      );

      SelectRange(CGXRange(1,1));
   }

	ScrollCellInView(nRow+1, GetLeftCol());
}

void CShearSteelGrid::OnEditRemoveRows()
{
   // call back to parent for this so things get set up correctly
   CShearSteelPage* pdlg = (CShearSteelPage*)GetParent();
   ASSERT (pdlg);

   pdlg->DoRemoveRows();
}

void CShearSteelGrid::DoRemoveRows()
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

void CShearSteelGrid::OnUpdateEditRemoveRows(CCmdUI* pCmdUI)
{
   BOOL flag = EnableItemDelete() ? TRUE:FALSE;
   pCmdUI->Enable(flag);
}

bool CShearSteelGrid::EnableItemDelete()
{
	if (GetParam() == NULL)
		return false;

   ROWCOL nrows = GetRowCount();

	CGXRangeList* pSelList = GetParam()->GetRangeList();
	return (pSelList->IsAnyCellFromCol(0) && 
           !pSelList->IsAnyCellFromRow(nrows) &&
           pSelList->GetCount() == 1);
}

void CShearSteelGrid::SetSymmetry(bool isSymmetrical)
{
   m_IsSymmetrical = isSymmetrical;

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   // Set text in last row
   CString lastzlen = (m_IsSymmetrical) ? _T("to mid-span") : _T("to girder end");

   ROWCOL nrows = GetRowCount();
   SetStyleRange(CGXRange(nrows,1), CGXStyle()
	.SetValue(lastzlen));

	GetParam( )->EnableUndo(TRUE);
   GetParam()->SetLockReadOnly(TRUE);
}

void CShearSteelGrid::CustomInit()
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	Initialize( );

	GetParam( )->EnableUndo(FALSE);

   const int num_rows=0;
   const int num_cols=6;

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
			.SetValue(_T("Zone\n#"))
		);

   CString cv = CString(_T("Zone Length\n")) + CString(pDisplayUnits->XSectionDim.UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

	SetStyleRange(CGXRange(0,2), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Bar\nSize"))
		);

   cv = CString(_T("Spacing\n")) + CString(pDisplayUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,3), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

	SetStyleRange(CGXRange(0,4), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetValue(_T("#\nOf\nVertical Legs\n"))
		);

	SetStyleRange(CGXRange(0,5), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetValue(_T("# Legs\nExtended\nInto\nDeck"))
		);

	SetStyleRange(CGXRange(0,6), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Confine-\nment\nBar\nSize"))
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

void CShearSteelGrid::SetRowStyle(ROWCOL nRow)
{
	GetParam()->EnableUndo(FALSE);

	SetStyleRange(CGXRange(nRow,1), CGXStyle()
			.SetUserAttribute(GX_IDS_UA_VALID_MIN, _T("0.0e01"))
			.SetUserAttribute(GX_IDS_UA_VALID_MAX, _T("1.0e99"))
			.SetUserAttribute(GX_IDS_UA_VALID_MSG, _T("Please enter a positive value"))
         .SetHorizontalAlignment(DT_RIGHT)
		);

	SetStyleRange(CGXRange(nRow,2), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(_T("None\n#3\n#4\n#5\n#6\n"))
			.SetValue(_T("None"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

	SetStyleRange(CGXRange(nRow,3), CGXStyle()
			.SetUserAttribute(GX_IDS_UA_VALID_MIN, _T("0.0e01"))
			.SetUserAttribute(GX_IDS_UA_VALID_MAX, _T("1.0e99"))
			.SetUserAttribute(GX_IDS_UA_VALID_MSG, _T("Please enter a positive value"))
         .SetHorizontalAlignment(DT_RIGHT)
		);

	SetStyleRange(CGXRange(nRow,4), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWN)
			.SetChoiceList(_T("0\n1\n2\n4\n6\n"))
			.SetValue(_T("0"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

	SetStyleRange(CGXRange(nRow,5), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWN)
			.SetChoiceList(_T("0\n1\n2\n4\n6\n"))
			.SetValue(_T("0"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

	SetStyleRange(CGXRange(nRow,6), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(_T("None\n#3\n#4\n#5\n#6\n"))
			.SetValue(_T("None"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

   GetParam()->EnableUndo(TRUE);
}

CString CShearSteelGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

matRebar::Size CShearSteelGrid::GetBarSize(ROWCOL row,ROWCOL col)
{
   assert(col==2 || col==6);
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

bool CShearSteelGrid::GetRowData(ROWCOL nRow, CShearZoneData* pszi)
{
   CString s = GetCellValue(nRow, 1);
   Float64 d = _tstof(s);
   if (s.IsEmpty() || (d==0.0 && s[0]!=_T('0')))
      pszi->ZoneLength = 0;
   else
      pszi->ZoneLength = d;

   pszi->VertBarSize = GetBarSize(nRow,2);

   s = GetCellValue(nRow, 3);
   d = _tstof(s);
   if (s.IsEmpty() || (d==0.0 && s[0]!=_T('0')))
      pszi->BarSpacing = 0;
   else
      pszi->BarSpacing = d;

   s = GetCellValue(nRow,4);
   pszi->nVertBars = _tstof(s);

   s = GetCellValue(nRow,5);
   pszi->nHorzInterfaceBars = _tstof(s);

   pszi->ConfinementBarSize = GetBarSize(nRow,6);

   return true;
}

void CShearSteelGrid::FillGrid(const CShearData::ShearZoneVec& rvec, bool isSymmetrical)
{
	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   m_IsSymmetrical = isSymmetrical;

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
      for (CShearData::ShearZoneConstIterator it = rvec.begin(); it!=rvec.end(); it++)
      {
         if (nRow<size)
         {
            SetValueRange(CGXRange(nRow, 1), (*it).ZoneLength);
         }

         CString tmp;
         tmp.Format(_T("%s"),lrfdRebarPool::GetBarSize((*it).VertBarSize).c_str());
         VERIFY(SetValueRange(CGXRange(nRow, 2), tmp));

         SetValueRange(CGXRange(nRow, 3), (*it).BarSpacing);

         VERIFY(SetValueRange(CGXRange(nRow, 4), (*it).nVertBars));
         VERIFY(SetValueRange(CGXRange(nRow, 5), (*it).nHorzInterfaceBars));

         tmp.Format(_T("%s"),lrfdRebarPool::GetBarSize((*it).ConfinementBarSize).c_str());
         VERIFY(SetValueRange(CGXRange(nRow, 6), tmp));

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
