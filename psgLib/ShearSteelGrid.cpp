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

// ShearSteelGrid.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "ShearSteelGrid.h"
#include "ShearSteelPage.h"
#include <system\tokenizer.h>
#include <EAF\EAFApp.h>

#include <LRFD\RebarPool.h>

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
//   RegisterClass();
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
   CShearSteelPage* ppage = (CShearSteelPage*)GetParent();
   ASSERT( ppage->IsKindOf(RUNTIME_CLASS(CShearSteelPage)) );
   
   ppage->DoInsertRow();
}

void CShearSteelGrid::DoInsertRow()
{
	ROWCOL nRow = 0;

	// if there are no cells selected,
	// copy the current cell's coordinates
	CGXRangeList selList;
	if (CopyRangeList(selList, TRUE))
		nRow = selList.GetHead()->top;
	else
		nRow = GetRowCount()+1;

	nRow = max(1, nRow);

	InsertRows(nRow, 1);
   SetRowStyle(nRow);

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

void CShearSteelGrid::OnEditRemoveRows()
{
   CShearSteelPage* ppage = (CShearSteelPage*)GetParent();
   ASSERT( ppage->IsKindOf(RUNTIME_CLASS(CShearSteelPage)) );
   ppage->DoRemoveRows();
}

void CShearSteelGrid::DoRemoveRows()
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

void CShearSteelGrid::OnUpdateEditRemoveRows(CCmdUI* pCmdUI)
{
	if (GetParam() == NULL)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

   ROWCOL nrows = GetRowCount();

	CGXRangeList* pSelList = GetParam()->GetRangeList();
	pCmdUI->Enable(pSelList->IsAnyCellFromCol(0) && 
                  !pSelList->IsAnyCellFromRow(nrows) &&
                  pSelList->GetCount() == 1);
}

void CShearSteelGrid::CustomInit()
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();


// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	this->Initialize( );

	this->GetParam( )->EnableUndo(FALSE);

   const int num_rows = 0;
   const int num_cols = 6;

	this->SetRowCount(num_rows);
	this->SetColCount(num_cols);

		// Turn off selecting whole columns when clicking on a column header
	this->GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   // no row moving
	this->GetParam()->EnableMoveRows(FALSE);

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
			.SetValue(_T("Zone\n#"))
		);

   CString cv;
   cv.Format(_T("Zone Length\n(%s)"),pDisplayUnits->SpanLength.UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv.Format(_T("Spacing\n(%s)"),pDisplayUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(0,2), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);


	this->SetStyleRange(CGXRange(0,3), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Vertical Bars\nSize"))
		);

	this->SetStyleRange(CGXRange(0,4), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Vertical Bars\n#"))
		);

	this->SetStyleRange(CGXRange(0,5), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Horizontal Bars\nSize"))
		);

	this->SetStyleRange(CGXRange(0,6), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Horizontal Bars\n#"))
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

void CShearSteelGrid::SetRowStyle(ROWCOL nRow)
{
	GetParam()->EnableUndo(FALSE);

	this->SetStyleRange(CGXRange(nRow,1,nRow,2), CGXStyle()
			.SetUserAttribute(GX_IDS_UA_VALID_MIN, _T("0"))
			.SetUserAttribute(GX_IDS_UA_VALID_MAX, _T("1.0e99"))
			.SetUserAttribute(GX_IDS_UA_VALID_MSG, _T("Please enter a positive value"))
         .SetHorizontalAlignment(DT_RIGHT)
		);

	this->SetStyleRange(CGXRange(nRow,3), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(_T("None\n#3\n#4\n#5\n#6\n"))
			.SetValue(_T("None"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

	this->SetStyleRange(CGXRange(nRow,4), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(_T("1\n2\n4\n6\n"))
			.SetValue(_T("2"))
         .SetHorizontalAlignment(DT_RIGHT)
		);

	this->SetStyleRange(CGXRange(nRow,5), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(_T("None\n#3\n#4\n#5\n#6\n"))
			.SetValue(_T("None"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

	this->SetStyleRange(CGXRange(nRow,6), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(_T("1\n2\n4\n6\n"))
			.SetValue(_T("2"))
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
        return GetValueRowCol(nRow, nCol);
}

bool CShearSteelGrid::GetRowData(ROWCOL nRow, GirderLibraryEntry::ShearZoneInfo* pszi)
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

void CShearSteelGrid::FillGrid(const GirderLibraryEntry::ShearZoneInfoVec& rvec)
{
	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CollectionIndexType size = rvec.size();
   if (size>0)
   {
      // size grid
      for (CollectionIndexType i=0; i<size; i++)
	      DoInsertRow();

      // fill grid
      ROWCOL nRow=1;
      for (GirderLibraryEntry::ShearZoneInfoVec::const_iterator it = rvec.begin(); it!=rvec.end(); it++)
      {
         
         if (nRow<size)
            SetValueRange(CGXRange(nRow, 1), (*it).ZoneLength);

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
	   DoInsertRow();
   }

   SelectRange(CGXRange(1,1));

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

// validate input
BOOL CShearSteelGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
{
	CString s;
	CGXControl* pControl = GetControl(nRow, nCol);
	pControl->GetCurrentText(s);

if ((nCol==1 || nCol==2)  && !s.IsEmpty( ))
	{
      double d;
      if (!sysTokenizer::ParseDouble(s, &d))
		{
			SetWarningText (_T("Value must be a number"));
			return FALSE;
		}
		return TRUE;
	}

	return CGXGridWnd::OnValidateCell(nRow, nCol);
}

matRebar::Size CShearSteelGrid::GetBarSize(ROWCOL row,ROWCOL col)
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
