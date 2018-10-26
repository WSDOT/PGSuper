///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperDoc.h"
#include "BridgeDescShearGrid.h"
#include "BridgeDescShearPage.h"
#include <Units\Measure.h>
#include <EAF\EAFDisplayUnits.h>
#include <Lrfd\RebarPool.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CGirderDescShearGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CGirderDescShearGrid

CGirderDescShearGrid::CGirderDescShearGrid()
{
//   RegisterClass();
}

CGirderDescShearGrid::~CGirderDescShearGrid()
{
}

BEGIN_MESSAGE_MAP(CGirderDescShearGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CGirderDescShearGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_COMMAND(ID_EDIT_INSERTROW, OnEditInsertRow)
	ON_COMMAND(ID_EDIT_REMOVEROWS, OnEditRemoveRows)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REMOVEROWS, OnUpdateEditRemoveRows)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGirderDescShearGrid message handlers

int CGirderDescShearGrid::GetColWidth(ROWCOL nCol)
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

BOOL CGirderDescShearGrid::OnRButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
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

BOOL CGirderDescShearGrid::OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   CGirderDescShearPage* pdlg = (CGirderDescShearPage*)GetParent();
   ASSERT (pdlg);

   ROWCOL nrows = GetRowCount();

   if (nCol==0 && (nRow!=0 && nRow!=nrows))
      pdlg->OnEnableDelete(true);
   else
      pdlg->OnEnableDelete(false);

   return TRUE;
}

void CGirderDescShearGrid::OnEditInsertRow()
{
   // call back to parent for this so things get set up correctly
   CGirderDescShearPage* pdlg = (CGirderDescShearPage*)GetParent();
   ASSERT (pdlg);

   pdlg->DoInsertRow();
}

void CGirderDescShearGrid::InsertRow(bool bAppend)
{
	ROWCOL nRow = 0;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

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
   Float64 zonlen = ::ConvertToSysUnits(3.0,unitMeasure::Feet);
   zonlen = ::ConvertFromSysUnits(zonlen,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   CString cval;
   cval.Format(_T("%g"),zonlen);
   SetStyleRange(CGXRange(nRow,1), CGXStyle().SetValue(cval));

   // zone length in the last row is infinite.
   ROWCOL nrows = GetRowCount();
   if (nrows==1)
   {
	   SetStyleRange(CGXRange(1,1), CGXStyle()
		.SetControl(GX_IDS_CTRL_STATIC)
		.SetValue(_T("to mid-span"))
      .SetReadOnly(TRUE)
      .SetHorizontalAlignment(DT_RIGHT)
      );

      SelectRange(CGXRange(1,1));

   }


	ScrollCellInView(nRow+1, GetLeftCol());
}

void CGirderDescShearGrid::OnEditRemoveRows()
{
   // call back to parent for this so things get set up correctly
   CGirderDescShearPage* pdlg = (CGirderDescShearPage*)GetParent();
   ASSERT (pdlg);

   pdlg->DoRemoveRows();
}

void CGirderDescShearGrid::DoRemoveRows()
{
	CGXRangeList* pSelList = GetParam()->GetRangeList();
	if (pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1)
	{
		CGXRange range = pSelList->GetHead();
		range.ExpandRange(1, 0, GetRowCount(), 0);
		RemoveRows(range.top, range.bottom);
      SetCurrentCell(1,1); // if this is not here, the next insert will go below grid bottom
	}
}

void CGirderDescShearGrid::OnUpdateEditRemoveRows(CCmdUI* pCmdUI)
{
   BOOL flag = EnableItemDelete() ? TRUE:FALSE;
   pCmdUI->Enable(flag);
}

bool CGirderDescShearGrid::EnableItemDelete()
{
	if (GetParam() == NULL)
		return false;

   ROWCOL nrows = GetRowCount();

	CGXRangeList* pSelList = GetParam()->GetRangeList();
	return (pSelList->IsAnyCellFromCol(0) && 
           !pSelList->IsAnyCellFromRow(nrows) &&
           pSelList->GetCount() == 1);
}


void CGirderDescShearGrid::CustomInit()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
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

   CString cv = CString(_T("Zone Length\n")) + CString(pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv = CString(_T("Spacing\n")) + CString(pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,2), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

	SetStyleRange(CGXRange(0,3), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Vertical Bars\nSize"))
		);

	SetStyleRange(CGXRange(0,4), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Vertical Bars\n#"))
		);

	SetStyleRange(CGXRange(0,5), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Horizontal Bars\nSize"))
		);

	SetStyleRange(CGXRange(0,6), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Horizontal Bars\n#"))
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

void CGirderDescShearGrid::SetRowStyle(ROWCOL nRow)
{
	GetParam()->EnableUndo(FALSE);

	SetStyleRange(CGXRange(nRow,1,nRow,2), CGXStyle()
			.SetUserAttribute(GX_IDS_UA_VALID_MIN, _T("0.0e01"))
			.SetUserAttribute(GX_IDS_UA_VALID_MAX, _T("1.0e99"))
			.SetUserAttribute(GX_IDS_UA_VALID_MSG, _T("Please enter a positive value"))
         .SetHorizontalAlignment(DT_RIGHT)
		);

	SetStyleRange(CGXRange(nRow,3), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(_T("None\n#3\n#4\n#5\n#6\n"))
			.SetValue(_T("None"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

	SetStyleRange(CGXRange(nRow,4), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(_T("1\n2\n4\n6\n"))
			.SetValue(_T("2"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

	SetStyleRange(CGXRange(nRow,5), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(_T("None\n#3\n#4\n#5\n#6\n"))
			.SetValue(_T("None"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

	SetStyleRange(CGXRange(nRow,6), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(_T("1\n2\n4\n6\n"))
			.SetValue(_T("2"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

   GetParam()->EnableUndo(TRUE);
}

CString CGirderDescShearGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

matRebar::Size CGirderDescShearGrid::GetBarSize(ROWCOL row,ROWCOL col)
{
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

bool CGirderDescShearGrid::GetRowData(ROWCOL nRow, GirderLibraryEntry::ShearZoneInfo* pszi)
{
   CString s = GetCellValue(nRow, 1);
   Float64 d = _tstof(s);
   if (s.IsEmpty() || (d==0.0 && s[0]!=_T('0')))
      pszi->ZoneLength = 0;
   else
      pszi->ZoneLength = d;

   s = GetCellValue(nRow, 2);
   d = _tstof(s);
   if (s.IsEmpty() || (d==0.0 && s[0]!=_T('0')))
      pszi->StirrupSpacing = 0;
   else
      pszi->StirrupSpacing = d;

   pszi->VertBarSize = GetBarSize(nRow,3);

   s = GetCellValue(nRow,4);
   pszi->nVertBars = _tstoi(s);


   pszi->HorzBarSize = GetBarSize(nRow,5);

   s = GetCellValue(nRow,6);
   pszi->nHorzBars = _tstoi(s);

   return true;
}

void CGirderDescShearGrid::FillGrid(const GirderLibraryEntry::ShearZoneInfoVec& rvec)
{
	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);


   // remove all but top row
   ROWCOL rows = GetRowCount();
   if (rows>=1)
	   RemoveRows(1, rows);

   GirderLibraryEntry::ShearZoneInfoVec::size_type size = rvec.size();
   if (0 < size)
   {
      // size grid
      for (GirderLibraryEntry::ShearZoneInfoVec::size_type i=0; i<size; i++)
      {
	      InsertRow(true);
      }

      // fill grid
      ROWCOL nRow=1;
      for (GirderLibraryEntry::ShearZoneInfoVec::const_iterator it = rvec.begin(); it!=rvec.end(); it++)
      {
         if (nRow<size)
         {
            SetValueRange(CGXRange(nRow, 1), (*it).ZoneLength);
         }

         SetValueRange(CGXRange(nRow, 2), (*it).StirrupSpacing);

         CString tmp;
         tmp.Format(_T("%s"),lrfdRebarPool::GetBarSize((*it).VertBarSize).c_str());
         VERIFY(SetValueRange(CGXRange(nRow, 3), tmp));

         tmp.Format(_T("%d"),(*it).nVertBars);
         VERIFY(SetValueRange(CGXRange(nRow, 4), tmp));


         tmp.Format(_T("%s"),lrfdRebarPool::GetBarSize((*it).HorzBarSize).c_str());
         VERIFY(SetValueRange(CGXRange(nRow, 5), tmp));

         tmp.Format(_T("%d"),(*it).nHorzBars);
         VERIFY(SetValueRange(CGXRange(nRow, 6), tmp));

         nRow++;
      }
   }
   else
   {
	   InsertRow(true);
   }

   SelectRange(CGXRange(1,1));

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}
