///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// 
// Do not include stdafx.h here - this control is to be shared among multiple dll projects
#include <WBFLVersion.h>
#include "stdafx.h"
#include <afxwin.h>
#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdisp.h>

#endif // _AFX_NO_OLE_SUPPORT

#include "SharedCTrls\MultiSelectGrid.h" 
#include <PsgLib\Keys.h>

#include <EAF\EAFUtilities.h>
#include <IFace\Tools.h>
#include <IFace\DocumentType.h>

#if defined _DEBUG
#include <IFace\Bridge.h>
#endif


GRID_IMPLEMENT_REGISTER(CMultiSelectGrid, CS_DBLCLKS, 0, 0, 0);

// Utility for building list of cells from a range. CGXRange::GetFirstCell/GetNextCell do not work as advertised
typedef std::pair<ROWCOL,ROWCOL> RowAndCol;
typedef std::vector<RowAndCol> RowAndColCollection;
typedef RowAndColCollection::iterator RowAndColIterator;

inline RowAndColCollection GetCellList(CGXRange& range, ROWCOL numRows, ROWCOL numCols)
{
   RowAndColCollection theList;

   if (range.IsCells())
   {
      for(ROWCOL row = range.top; row<=range.bottom; row++)
      {
         for (ROWCOL col=range.left; col<=range.right; col++)
         {
            RowAndCol rac(row, col);
            theList.push_back(rac);
         }
      }
   }
   else if (range.IsRows())
   {
      for(ROWCOL row = range.top; row<=range.bottom; row++)
      {
         for (ROWCOL col=1; col<=numCols; col++)
         {
            RowAndCol rac(row, col);
            theList.push_back(rac);
         }
      }
   }
   else if (range.IsCols())
   {
      for(ROWCOL col = range.left; col<=range.right; col++)
      {
         for (ROWCOL row=1; row<=numRows; row++)
         {
            RowAndCol rac(row, col);
            theList.push_back(rac);
         }
      }
   }
   else if (range.IsTable())
   {
      for(ROWCOL col = 1; col<=numCols; col++)
      {
         for (ROWCOL row=1; row<=numRows; row++)
         {
            RowAndCol rac(row, col);
            theList.push_back(rac);
         }
      }
   }
   else
      ATLASSERT(false); // hmmmm...


   return theList;
}


/////////////////////////////////////////////////////////////////////////////
// CMultiSelectGrid

CMultiSelectGrid::CMultiSelectGrid()
{
//   RegisterClass();
}

CMultiSelectGrid::~CMultiSelectGrid()
{
}

BEGIN_MESSAGE_MAP(CMultiSelectGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CMultiSelectGrid)
   ON_WM_GETDLGCODE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMultiSelectGrid message handlers
/*
int CMultiSelectGrid::GetColWidth(ROWCOL nCol)
{
   if ( IsColHidden(nCol) )
      return CGXGridWnd::GetColWidth(nCol);

	CRect rect = GetGridRect( );

   switch (nCol)
   {
   case 0:
      return rect.Width( )/7;
   default:
      return rect.Width( )*2/7;
   }
}
*/

void CMultiSelectGrid::Enable(bool bEnable)
{
	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CGXStyle style;
   CGXRange range;
   if ( bEnable )
   {
      style.SetEnabled(TRUE)
         .SetReadOnly(FALSE)
         .SetInterior(::GetSysColor(COLOR_WINDOW));
//           .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

      range = CGXRange(1,1,GetRowCount(),GetColCount());
      SetStyleRange(range,style);

	   GetParam()->EnableSelection((WORD) (GX_SELFULL));
   }
   else
   {
      style.SetEnabled(FALSE)
           .SetReadOnly(TRUE)
           .SetInterior(::GetSysColor(COLOR_BTNFACE));
//           .SetTextColor(::GetSysColor(COLOR_GRAYTEXT));

      range = CGXRange(0,0,GetRowCount(),GetColCount());
      SetStyleRange(range,style);

	   GetParam()->EnableSelection((WORD) (GX_SELNONE));
   }

   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(FALSE);
}

bool CMultiSelectGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
{
    if (IsCurrentCell(nRow, nCol) && IsActiveCurrentCell())
    {
        CGXControl* pControl = GetControl(nRow, nCol);
        if (pControl != nullptr)
        {
           CString huh;
           pControl->GetValue(huh);
           return huh == _T("1");
        }
        else
        {
           return false;
        }
    }
    else
    {
        CString huh = GetValueRowCol(nRow, nCol);
        return huh == _T("1");
    }
}

BOOL CMultiSelectGrid::OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   if (nRow!=0 && nCol!=0 && !(nFlags&&MK_CONTROL) && !(nFlags&&MK_SHIFT))
   {
   	GetParam( )->EnableUndo(FALSE);

      BOOL st = TRUE;

      // If we click on a selected cell, set all selected cells to same value
      if( m_SelectedRange.IsCellInList(nRow, nCol) )
      {
         ROWCOL nRows = GetRowCount();
         ROWCOL nCols = GetColCount();

         bool bDat = GetCellValue(nRow, nCol); 
         CString cellval = bDat ? _T("0") : _T("1"); // flip value of cell that was clicked on

         for (POSITION pos = m_SelectedRange.GetHeadPosition(); pos != nullptr;)
         {
            CGXRange* pRange = m_SelectedRange.GetNext(pos);

            // Get detailed list of selected cells 
            RowAndColCollection rac = GetCellList(*pRange, nRows, nCols);

            for (RowAndColIterator rcit = rac.begin(); rcit != rac.end(); rcit++)
            {
               ROWCOL nRow2 = rcit->first;
               ROWCOL nCol2 = rcit->second;

               CGXControl* pControl = GetControl(nRow2, nCol2);
               CGXCheckBox* pCheck = dynamic_cast<CGXCheckBox*>(pControl);
               if (pCheck != nullptr) // only set check boxes
               {
                  SetValueRange(CGXRange(nRow2, nCol2), cellval);
                  TRACE(_T("SetValue Range (%d, %d)"),nRow2, nCol2);
               }
            }
         }
      }
      else
      {
         // no cells selected, do default behavior
         st = CGXGridWnd::OnLButtonClickedRowCol(nRow, nCol, nFlags, pt);
      }

      // clear selection
      m_SelectedRange.DeleteAll();

   	GetParam( )->EnableUndo(TRUE);

      return st;
   }
   else
   {
      BOOL st = CGXGridWnd::OnLButtonClickedRowCol(nRow, nCol, nFlags, pt);

      // Store the last selected range
      m_SelectedRange = *(this->GetParam()->GetRangeList());
      return st;
   }
}

BOOL CMultiSelectGrid::ProcessKeys(CWnd* pSender, UINT nMessage,
                   UINT nChar, UINT nRepCnt, UINT flags)
{
   // Treat spacebar like left mouse
    if (nMessage == WM_KEYDOWN)
    {
      switch (nChar)
      {
      case VK_SPACE: 
        {
          ROWCOL nRow = 1, nCol = 1;
          GetCurrentCell(nRow, nCol);

          BOOL st = OnLButtonClickedRowCol(nRow, nCol, 0, CPoint());

          // clear selection
          this->SetSelection(nullptr);
          m_SelectedRange.DeleteAll();

          return st;

        }
      }
    }

    if (nMessage == WM_KEYUP)
    {
      switch (nChar)
      {
      case VK_LEFT: 
      case VK_UP:
      case VK_RIGHT:
      case VK_DOWN:
         {
         // The keyboard can also select cells - store any changes
         m_SelectedRange = *(this->GetParam()->GetRangeList());
         }
      }
    }

    BOOL st = CGXGridWnd::ProcessKeys(pSender, nMessage, nChar, nRepCnt, flags);


   return st;
}

void CMultiSelectGrid::OnChangedSelection(const CGXRange* pRange, BOOL bIsDragging, BOOL bKey)
{
   if (!bIsDragging && !bKey)
      m_SelectedRange = *(this->GetParam()->GetRangeList());
}

UINT CMultiSelectGrid::OnGetDlgCode()
{
   // Make it so Tab key navigates to next control on containing dialog
 	return CWnd::OnGetDlgCode() | DLGC_WANTARROWS | DLGC_WANTCHARS;
}

void CMultiSelectGrid::OnDrawItem(CDC *pDC, ROWCOL nRow, ROWCOL nCol, const CRect& rectItem, const CGXStyle& style)
{
   CGXGridCore::OnDrawItem(pDC, nRow, nCol, rectItem, style);
}

void CMultiSelectGrid::SetAllValues(bool val)
{

   ROWCOL nRows = GetRowCount();
   ROWCOL nCols = GetColCount();

   for (ROWCOL row=1; row<=nRows; row++)
   {
      for (ROWCOL col=1; col<=nCols; col++)
      {
         CGXControl* pControl = GetControl(row, col);
         CGXCheckBox* pCheck = dynamic_cast<CGXCheckBox*>(pControl);
         if (pCheck != nullptr) // only set check boxes
         {
            SetValueRange(CGXRange(row, col), val ? _T("1") : _T("0"));
            TRACE(_T("SetValue Range (%d, %d)"),row, col);
         }
      }
   }
}

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

