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

// DiaphramGrid.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "GirderStrandGrid.h"
#include <system\tokenizer.h>

#include <EAF\EAFApp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderStrandGrid

CGirderStrandGrid::CGirderStrandGrid(CGirderStrandGridClient* pClient):
m_pClient(pClient)
{
   ATLASSERT(m_pClient!=0);
}

CGirderStrandGrid::~CGirderStrandGrid()
{
}

BEGIN_MESSAGE_MAP(CGirderStrandGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CGirderStrandGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_COMMAND(ID_STR_INSERT, OnEditInsertrow)
	ON_COMMAND(ID_STR_DELETE, OnEditRemoverows)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGirderStrandGrid message handlers

int CGirderStrandGrid::GetColWidth(ROWCOL nCol)
{
	CRect rect = GetGridRect( );

   switch (nCol)
   {
   case 0:
      return rect.Width( )/5-1;
   default:
      return rect.Width( )*2/5;
   }
}

BOOL CGirderStrandGrid::OnRButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
	 // unreferenced parameters
	 nCol, nFlags;

   if (0 < nRow)
   {
	   CMenu menu;
	   VERIFY(menu.LoadMenu(IDR_STRAIGHT_POPUP));

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

void CGirderStrandGrid::OnEditInsertrow()
{
   Insertrow();
}

void CGirderStrandGrid::Insertrow()
{
	ROWCOL nRow = 0;

	// if there are no cells selected,
	// copy the current cell's coordinates
	CGXRangeList selList;
	if (CopyRangeList(selList, TRUE))
   {
		nRow = selList.GetHead()->top;
   }
	else
   {
		nRow = GetRowCount()+1;
   }

	nRow = Max((ROWCOL)1, nRow);

	InsertRows(nRow, 1);
   SetRowStyle(nRow);
   SetCurrentCell(nRow, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	ScrollCellInView(nRow, GetLeftCol());

   if ( 0 < GetRowCount() )
   {
      // if there is at least 1 row, enable the delete button
      m_pClient->OnEnableDelete(this,true); 
   }
}

void CGirderStrandGrid::OnEditAppendrow()
{
   Appendrow();
}

void CGirderStrandGrid::Appendrow()
{
	ROWCOL nRow = 0;
   nRow = GetRowCount()+1;

	InsertRows(nRow, 1);
   SetRowStyle(nRow);
   SetCurrentCell(nRow, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	ScrollCellInView(nRow, GetLeftCol());
	Invalidate();

   if ( 0 < GetRowCount() )
   {
      // if there is at least 1 row, enable the delete button
      m_pClient->OnEnableDelete(this,true); 
   }
}

void CGirderStrandGrid::OnEditRemoverows()
{
   Removerows();
}

void CGirderStrandGrid::Removerows()
{
	CGXRangeList selList;
	if (CopyRangeList(selList, TRUE))
   {
		ROWCOL nStartRow = selList.GetHead()->top;
      ROWCOL nEndRow   = selList.GetTail()->bottom;

      if (0 < nStartRow)
      {
		   RemoveRows(nStartRow, nEndRow);

         if ( 0 == GetRowCount() )
         {
            // if there are no more rows,
            // disable the delete button
            m_pClient->OnEnableDelete(this,false); 
         }
      }
	}
}

void CGirderStrandGrid::CustomInit()
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   // Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	this->Initialize( );

	this->GetParam( )->EnableUndo(FALSE);

   const int num_rows=0;
   const int num_cols=2;

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
			.SetValue(_T("Fill #"))
		);

   CString cv;
   cv.Format(_T("Xt (%s)"),pDisplayUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv.Format(_T("Yt (%s)"),pDisplayUnits->ComponentDim.UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(0,2), CGXStyle()
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

void CGirderStrandGrid::SetRowStyle(ROWCOL nRow)
{
   // value range for x and y
	this->SetStyleRange(CGXRange(nRow,1,nRow,2), CGXStyle()
			.SetUserAttribute(GX_IDS_UA_VALID_MIN, _T("0"))
			.SetUserAttribute(GX_IDS_UA_VALID_MAX, _T("1.0e99"))
			.SetUserAttribute(GX_IDS_UA_VALID_MSG, _T("Please enter a positive value"))
         .SetHorizontalAlignment(DT_RIGHT)
		);
}

CString CGirderStrandGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

bool CGirderStrandGrid::GetRowData(ROWCOL nRow, Float64* pX, Float64* pY)
{
   Float64 x,y;

   CString s = GetCellValue(nRow, 1);
   Float64 d = _tstof(s);
   if (s.IsEmpty())
   {
      return false;
   }
   else if (d==0.0 && s[0]!=_T('0'))
   {
      return false;
   }
   
   x = d;

   s = GetCellValue(nRow, 2);
   d = _tstof(s);
   if (s.IsEmpty())
   {
      return false;
   }
   else if (d==0.0 && s[0]!=_T('0'))
   {
      return false;
   }
   
   y = d;

   *pX = x;
   *pY = y;

   return true;
}

void CGirderStrandGrid::FillGrid(IPoint2dCollection* points)
{
   GetParam()->SetLockReadOnly(FALSE);
   CollectionIndexType count;
   points->get_Count(&count);
   if (0 < count)
   {
      // size grid
      for (CollectionIndexType i = 0; i < count; i++)
      {
	      Insertrow();
      }

      // fill grid
      ROWCOL nRow=1;
      CComPtr<IPoint2d> point;
      CComPtr<IEnumPoint2d> enumpoints;
      points->get__Enum(&enumpoints);
      while ( enumpoints->Next(1,&point,NULL) != S_FALSE )
      {
         Float64 x,y;
         point->get_X(&x);
         point->get_Y(&y);

         SetValueRange(CGXRange(nRow, 1), x);
         SetValueRange(CGXRange(nRow, 2), y);
         nRow++;

         point.Release();
      }
   }

   GetParam()->SetLockReadOnly(TRUE);
}

// validate input
BOOL CGirderStrandGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
{
	CString s;
	CGXControl* pControl = GetControl(nRow, nCol);
	pControl->GetCurrentText(s);

	if ((nCol==1 || nCol==2) && !s.IsEmpty( ))
	{
      Float64 d;
      if (!sysTokenizer::ParseDouble(s, &d))
		{
			SetWarningText (_T("Value must be a number"));
			return FALSE;
		}
		return TRUE;
	}

	return CGXGridWnd::OnValidateCell(nRow, nCol);
}
