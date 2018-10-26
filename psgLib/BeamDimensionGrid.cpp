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
#include "BeamDimensionGrid.h"
#include "GirderDimensionsPage.h"
#include "GirderMainSheet.h"
#include <system\tokenizer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CBeamDimensionGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CBeamDimensionGrid

CBeamDimensionGrid::CBeamDimensionGrid()
{
//   RegisterClass();
}

CBeamDimensionGrid::~CBeamDimensionGrid()
{
}

BEGIN_MESSAGE_MAP(CBeamDimensionGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CBeamDimensionGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_COMMAND(ID_EDIT_INSERTROW, OnEditInsertrow)
	ON_COMMAND(ID_EDIT_REMOVEROWS, OnEditRemoverows)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REMOVEROWS, OnUpdateEditRemoverows)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBeamDimensionGrid message handlers
void CBeamDimensionGrid::OnEditInsertrow()
{
   Insertrow();
}

void CBeamDimensionGrid::Insertrow()
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

void CBeamDimensionGrid::Appendrow()
{
	ROWCOL nRow = 0;
   nRow = GetRowCount()+1;

	InsertRows(nRow, 1);
   SetRowStyle(nRow);
   SetCurrentCell(nRow, GetLeftCol(), GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
	Invalidate();
}

void CBeamDimensionGrid::OnEditRemoverows()
{
   Removerows();
}

void CBeamDimensionGrid::Removerows()
{
	CGXRangeList* pSelList = GetParam()->GetRangeList();
	if (pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1)
	{
		CGXRange range = pSelList->GetHead();
		range.ExpandRange(1, 0, GetRowCount(), 0);
		RemoveRows(range.top, range.bottom);
	}
}

void CBeamDimensionGrid::OnUpdateEditRemoverows(CCmdUI* pCmdUI)
{
	if (GetParam() == NULL)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	CGXRangeList* pSelList = GetParam()->GetRangeList();
	pCmdUI->Enable(pSelList->IsAnyCellFromCol(0) && pSelList->GetCount() == 1);
}

void CBeamDimensionGrid::CustomInit()
{
   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
	Initialize( );
	EnableIntelliMouse();

   ResetGrid();

}

void CBeamDimensionGrid::SetRowStyle(ROWCOL nRow)
{
	GetParam()->EnableUndo(FALSE);

	this->SetStyleRange(CGXRange(nRow,1), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
         .SetHorizontalAlignment(DT_RIGHT)
         );


	GetParam()->EnableUndo(TRUE);
}

CString CBeamDimensionGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

// validate input
BOOL CBeamDimensionGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
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

void CBeamDimensionGrid::ResetGrid()
{
   CGirderDimensionsPage* pdlg = (CGirderDimensionsPage*)GetParent();
   ASSERT(pdlg);
   CGirderMainSheet* pDad = (CGirderMainSheet*)pdlg->GetParent();

	GetParam( )->EnableUndo(FALSE);

   if ( GetRowCount() > 0 )
      RemoveRows(1,GetRowCount());

   CComPtr<IBeamFactory> pFactory;
   pDad->m_Entry.GetBeamFactory(&pFactory);
   std::vector<std::_tstring> names = pFactory->GetDimensionNames();

   const ROWCOL num_rows = (ROWCOL)names.size();
   const ROWCOL num_cols = 1;

	SetRowCount(num_rows);
	SetColCount(num_cols);

		// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

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
			.SetValue(_T(""))
		);

	SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Value"))
		);

   // make it so that text fits correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	SetFocus();

	GetParam( )->EnableUndo(TRUE);
}