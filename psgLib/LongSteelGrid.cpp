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

// DiaphramGrid.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "LongSteelGrid.h"
#include "LongSteelPage.h"
#include <system\tokenizer.h>

#include <EAF\EAFApp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CLongSteelGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CLongSteelGrid

CLongSteelGrid::CLongSteelGrid()
{
//   RegisterClass();
}

CLongSteelGrid::~CLongSteelGrid()
{
}

BEGIN_MESSAGE_MAP(CLongSteelGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CLongSteelGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_COMMAND(ID_EDIT_INSERTROW, OnEditInsertrow)
	ON_COMMAND(ID_EDIT_REMOVEROWS, OnEditRemoverows)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REMOVEROWS, OnUpdateEditRemoverows)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CLongSteelGrid message handlers

int CLongSteelGrid::GetColWidth(ROWCOL nCol)
{
	CRect rect = GetGridRect( );

   switch (nCol)
   {
   case 0:
      return rect.Width( )/12;
   default:
      return rect.Width( )*2/11;
   }
}

BOOL CLongSteelGrid::OnRButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
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

BOOL CLongSteelGrid::OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   CLongSteelPage* pdlg = (CLongSteelPage*)GetParent();
   ASSERT (pdlg);

   if (nCol==0 && nRow!=0)
      pdlg->OnEnableDelete(true);
   else
      pdlg->OnEnableDelete(false);

   return TRUE;
}

void CLongSteelGrid::OnEditInsertrow()
{
   Insertrow();
}

void CLongSteelGrid::Insertrow()
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
   SetCurrentCell(nRow, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
}

void CLongSteelGrid::Appendrow()
{
	ROWCOL nRow = 0;
   nRow = GetRowCount()+1;

	InsertRows(nRow, 1);
   SetRowStyle(nRow);
   SetCurrentCell(nRow, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	Invalidate();
}

void CLongSteelGrid::OnEditRemoverows()
{
   Removerows();
}

void CLongSteelGrid::Removerows()
{
	CGXRangeList* pSelList = GetParam()->GetRangeList();
	if (pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1)
	{
		CGXRange range = pSelList->GetHead();
		range.ExpandRange(1, 0, GetRowCount(), 0);
		RemoveRows(range.top, range.bottom);
	}
}

void CLongSteelGrid::OnUpdateEditRemoverows(CCmdUI* pCmdUI)
{
	if (GetParam() == NULL)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	CGXRangeList* pSelList = GetParam()->GetRangeList();
	pCmdUI->Enable(pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1);
}

void CLongSteelGrid::CustomInit()
{
   CEAFApp* pApp;
   {
      AFX_MANAGE_STATE(AfxGetAppModuleState());
      pApp = (CEAFApp*)AfxGetApp();
   }
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();


// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	this->Initialize( );

	this->GetParam( )->EnableUndo(FALSE);

   const int num_rows=0;
   const int num_cols=5;

	this->SetRowCount(num_rows);
	this->SetColCount(num_cols);

		// Turn off selecting whole columns when clicking on a column header
	this->GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

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
			.SetValue(_T("Row\n#"))
		);

	this->SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue("Girder\nFace")
		);

	this->SetStyleRange(CGXRange(0,2), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue("Bar\nSize")
		);

	this->SetStyleRange(CGXRange(0,3), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue("# of Bars")
		);

   CString cv;
   cv.Format("Cover\n(%s)",pDisplayUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(0,4), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv.Format("Spacing\n(%s)",pDisplayUnits->SpanLength.UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(0,5), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
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

void CLongSteelGrid::SetRowStyle(ROWCOL nRow)
{
	GetParam()->EnableUndo(FALSE);

	this->SetStyleRange(CGXRange(nRow,1), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(_T("Top\nBottom"))
			.SetValue(_T("Top"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

	this->SetStyleRange(CGXRange(nRow,2), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(_T("#3\n#4\n#5\n#6\n#8\n#9\n#10\n#11\n#14\n#18"))
			.SetValue(_T("#4"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

	this->SetStyleRange(CGXRange(nRow,3), CGXStyle()
			.SetUserAttribute(GX_IDS_UA_VALID_MIN, _T("1"))
			.SetUserAttribute(GX_IDS_UA_VALID_MAX, _T("1.0e99"))
			.SetUserAttribute(GX_IDS_UA_VALID_MSG, _T("You must have at least one bar in a row"))
         .SetHorizontalAlignment(DT_RIGHT)
		);

	this->SetStyleRange(CGXRange(nRow,4,nRow,5), CGXStyle()
			.SetUserAttribute(GX_IDS_UA_VALID_MIN, _T("0"))
			.SetUserAttribute(GX_IDS_UA_VALID_MAX, _T("1.0e99"))
			.SetUserAttribute(GX_IDS_UA_VALID_MSG, _T("Please enter a positive value"))
         .SetHorizontalAlignment(DT_RIGHT)
		);

	GetParam()->EnableUndo(TRUE);
}

CString CLongSteelGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

bool CLongSteelGrid::GetRowData(ROWCOL nRow, GirderLibraryEntry::LongSteelInfo* plsi)
{
   double d;
   int i;

   CString s = GetCellValue(nRow, 1);
   if (s=="Top")
      plsi->Face = GirderLibraryEntry::GirderTop;
   else
      plsi->Face = GirderLibraryEntry::GirderBottom;

   s = GetCellValue(nRow, 2);
   s.TrimLeft();
   int l = s.GetLength();
   CString s2 = s.Right(l-1);
   i = atoi(s2);
   if (s.IsEmpty() || (i==0))
      return false;
   plsi->BarSize = i;

   s = GetCellValue(nRow, 3);
   i = atoi(s);
   if (s.IsEmpty())
      i=0;
   else if (i==0 && s[0]!='0')
      return false;
   plsi->NumberOfBars = i;

   s = GetCellValue(nRow, 4);
   d = atof(s);
   if (s.IsEmpty())
      d=0;
   else if (d==0.0 && s[0]!='0')
      return false;
   plsi->Cover = d;

   s = GetCellValue(nRow, 5);
   d = atof(s);
   if (s.IsEmpty())
      d=0;
   else if (d==0.0 && s[0]!='0')
      return false;
   plsi->BarSpacing = d;

   return true;
}

void CLongSteelGrid::FillGrid(const GirderLibraryEntry::LongSteelInfoVec& rvec)
{

   GetParam()->SetLockReadOnly(FALSE);
   int size = rvec.size();
   if (size>0)
   {
      // size grid
      for (int i=0; i<size; i++)
	      Insertrow();

      // fill grid
      ROWCOL nRow=1;
      for (GirderLibraryEntry::LongSteelInfoVec::const_iterator it = rvec.begin(); it!=rvec.end(); it++)
      {
         CString tmp;
         GirderLibraryEntry::GirderFace face = (*it).Face;
         if (face==GirderLibraryEntry::GirderBottom)
            tmp = "Bottom";
         else
            tmp = "Top";
            
         VERIFY(SetValueRange(CGXRange(nRow, 1), tmp));

         Int32 size = (*it).BarSize;
         tmp.Format("#%d", size);
         VERIFY(SetValueRange(CGXRange(nRow, 2), tmp));

         VERIFY(SetValueRange(CGXRange(nRow, 3), (*it).NumberOfBars));
         VERIFY(SetValueRange(CGXRange(nRow, 4), (*it).Cover));
         VERIFY(SetValueRange(CGXRange(nRow, 5), (*it).BarSpacing));
         nRow++;
      }
   }

   GetParam()->SetLockReadOnly(TRUE);
}

// validate input
BOOL CLongSteelGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
{
	CString s;
	CGXControl* pControl = GetControl(nRow, nCol);
	pControl->GetCurrentText(s);

	if (nCol==3  && !s.IsEmpty( ))
	{
      long l;
      if (!sysTokenizer::ParseLong(s, &l))
		{
			SetWarningText (_T("Value must be a number"));
			return FALSE;
		}
		return TRUE;
	}
	else if ((nCol==4 || nCol==5)  && !s.IsEmpty( ))
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
