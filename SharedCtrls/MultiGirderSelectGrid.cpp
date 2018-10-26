///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#include <afxwin.h>
#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdisp.h>
#endif // _AFX_NO_OLE_SUPPORT

#include <grid\gxall.h>

#include "SharedCTrls\MultiGirderSelectGrid.h" 
#include <PgsExt\GirderLabel.h>

#include <EAF\EAFUtilities.h>
#include <IFace\Tools.h>
#include <IFace\DocumentType.h>

#if defined _DEBUG
#include <IFace\Bridge.h>
#endif



GRID_IMPLEMENT_REGISTER(CMultiGirderSelectGrid, CS_DBLCLKS, 0, 0, 0);

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
// CMultiGirderSelectGrid

CMultiGirderSelectGrid::CMultiGirderSelectGrid()
{
//   RegisterClass();
}

CMultiGirderSelectGrid::~CMultiGirderSelectGrid()
{
}

BEGIN_MESSAGE_MAP(CMultiGirderSelectGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CMultiGirderSelectGrid)
   ON_WM_GETDLGCODE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMultiGirderSelectGrid message handlers
/*
int CMultiGirderSelectGrid::GetColWidth(ROWCOL nCol)
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

void CMultiGirderSelectGrid::CustomInit(const GroupGirderOnCollection& groupGirderCollection, std::_tstring(*pGetGirderLabel)(GirderIndexType))
{
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate. 
   AFX_MANAGE_STATE(AfxGetAppModuleState());

	Initialize( );

	GetParam( )->EnableUndo(FALSE);

   GroupIndexType nGroups = groupGirderCollection.size();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

#if defined _DEBUG
   GET_IFACE2(pBroker,IBridge,pBridge);
   ATLASSERT(pBridge->GetGirderGroupCount() == nGroups);
#endif

   // Determine max number of girders
   GirderIndexType max_gdrs = 0;
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      max_gdrs = Max( max_gdrs, (GirderIndexType)groupGirderCollection[grpIdx].size());
   }

   const ROWCOL num_rows = (ROWCOL)max_gdrs;
   const ROWCOL num_cols = (ROWCOL)nGroups;

	SetRowCount(num_rows);
	SetColCount(num_cols);

   // mimic excel selection behavior
   GetParam()->SetExcelLikeSelectionFrame(TRUE);

   // no row or column moving
	GetParam()->EnableMoveRows(FALSE);
	GetParam()->EnableMoveCols(FALSE);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
         .SetHorizontalAlignment(DT_CENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   // disable top row
	SetStyleRange(CGXRange(0,0,0,num_cols), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   // top row labels
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   bool bIsPGSuper = pDocType->IsPGSuperDocument();
   CString strGroupLabel(bIsPGSuper ? _T("Span") : _T("Group"));

   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      CString lbl;
      lbl.Format(_T("%s %d"), strGroupLabel, LABEL_GROUP(grpIdx));

	   SetStyleRange(CGXRange(0,ROWCOL(grpIdx+1)), CGXStyle()
            .SetWrapText(TRUE)
			   .SetEnabled(FALSE)          // disables usage as current cell
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
			   .SetValue(lbl)
		   );
   }

   // left column labels
   for (GirderIndexType ig=0; ig<max_gdrs; ig++)
   {
      CString lbl;
      lbl.Format(_T("Girder %s"), LABEL_GIRDER(ig));

	   SetStyleRange(CGXRange(ROWCOL(ig+1),0), CGXStyle()
            .SetWrapText(TRUE)
			   .SetEnabled(FALSE)          // disables usage as current cell
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
			   .SetValue(lbl)
		   );
   }

   // fill controls for girders
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      GirderIndexType nGirders = groupGirderCollection[grpIdx].size();

      for (GirderIndexType gdrIdx = 0; gdrIdx < max_gdrs; gdrIdx++)
      {
         ROWCOL nCol = ROWCOL(grpIdx+1);
         ROWCOL nRow = ROWCOL(gdrIdx+1);

         if (gdrIdx < nGirders)
         {

            UINT enabled = groupGirderCollection[grpIdx][gdrIdx] ? 1 : 0;
            // unchecked check box for girder
            SetStyleRange(CGXRange(nRow,nCol), CGXStyle()
			         .SetControl(GX_IDS_CTRL_CHECKBOX3D)  //
			         .SetValue(enabled)
                  .SetHorizontalAlignment(DT_CENTER)
                  );
         }
         else
         {
            // no girder here - disable cell
            SetStyleRange(CGXRange(nRow,nCol), CGXStyle()
			      .SetEnabled(FALSE)
               );
         }
      }
   }

   // Make it so that text fits correctly in header rows/cols
   ResizeColWidthsToFit(CGXRange(0,0,num_rows,num_cols));

   // don't allow users to resize grids
//   GetParam( )->EnableTrackColWidth(0); 
//   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();

   // Get grid started in dialog navigation:
	SetFocus();
   SetCurrentCell(1,1);

	GetParam( )->EnableUndo(TRUE);
}

bool CMultiGirderSelectGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
{
    if (IsCurrentCell(nRow, nCol) && IsActiveCurrentCell())
    {
        CGXControl* pControl = GetControl(nRow, nCol);
        if (pControl != NULL)
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

std::vector<CGirderKey> CMultiGirderSelectGrid::GetData()
{
   std::vector<CGirderKey> data;

   ROWCOL nRows = GetRowCount();
   ROWCOL nCols = GetColCount();

   for ( ROWCOL col = 0; col < nCols; col++ )
   {
      for ( ROWCOL row = 0; row < nRows; row++ )
      {
         bool bDat = GetCellValue(row+1,col+1);

         if (bDat)
         {
            CGirderKey girderKey(col, row);
            data.push_back(girderKey);
         }
      }
   }

   return data;
}

BOOL CMultiGirderSelectGrid::OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
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

         for (POSITION pos = m_SelectedRange.GetHeadPosition(); pos != NULL;)
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
               if (pCheck != NULL) // only set check boxes
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

BOOL CMultiGirderSelectGrid::ProcessKeys(CWnd* pSender, UINT nMessage,
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
          this->SetSelection(NULL);
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

void CMultiGirderSelectGrid::OnChangedSelection(const CGXRange* pRange, BOOL bIsDragging, BOOL bKey)
{
   if (!bIsDragging && !bKey)
      m_SelectedRange = *(this->GetParam()->GetRangeList());
}

UINT CMultiGirderSelectGrid::OnGetDlgCode()
{
   // Make it so Tab key navigates to next control on containing dialog
 	return CWnd::OnGetDlgCode() | DLGC_WANTARROWS | DLGC_WANTCHARS;
}

void CMultiGirderSelectGrid::OnDrawItem(CDC *pDC, ROWCOL nRow, ROWCOL nCol, const CRect& rectItem, const CGXStyle& style)
{
   AFX_MANAGE_STATE(AfxGetAppModuleState());  
   CGXGridCore::OnDrawItem(pDC, nRow, nCol, rectItem, style);
}

void CMultiGirderSelectGrid::SetAllValues(bool val)
{

   ROWCOL nRows = GetRowCount();
   ROWCOL nCols = GetColCount();

   for (ROWCOL row=1; row<=nRows; row++)
   {
      for (ROWCOL col=1; col<=nCols; col++)
      {
         CGXControl* pControl = GetControl(row, col);
         CGXCheckBox* pCheck = dynamic_cast<CGXCheckBox*>(pControl);
         if (pCheck != NULL) // only set check boxes
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

