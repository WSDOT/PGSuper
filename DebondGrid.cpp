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

// DebondGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperDoc.h"
#include "DebondGrid.h"
#include "DebondDlg.h"
#include "GirderDescDlg.h"
#include <Units\Measure.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CGirderDescDebondGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CGirderDescDebondGrid

CGirderDescDebondGrid::CGirderDescDebondGrid()
{
//   RegisterClass();
   m_bSymmetricDebond = true;
}

CGirderDescDebondGrid::~CGirderDescDebondGrid()
{
}

BEGIN_MESSAGE_MAP(CGirderDescDebondGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CGirderDescDebondGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGirderDescDebondGrid message handlers

int CGirderDescDebondGrid::GetColWidth(ROWCOL nCol)
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

void CGirderDescDebondGrid::InsertRow()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CGirderDescDebondPage* pParent = (CGirderDescDebondPage*)GetParent();
   CGirderDescDlg* pDlg = (CGirderDescDlg*)(pParent->GetParent());

	ROWCOL nRow = GetRowCount()+1;

	InsertRows(nRow, 1);
   SetRowStyle(nRow);

   // Put in some reasonable default values
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   Float64 debond_length = pStrandGeom->GetDefaultDebondLength(pDlg->m_CurrentSpanIdx,pDlg->m_CurrentGirderIdx);
   debond_length = ::ConvertFromSysUnits(debond_length,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   
   CString strDebondLength;
   strDebondLength.Format(_T("%g"),debond_length);
   SetStyleRange(CGXRange(nRow,2), CGXStyle().SetValue(strDebondLength));
   SetStyleRange(CGXRange(nRow,3), CGXStyle().SetValue(strDebondLength));


	ScrollCellInView(nRow+1, GetLeftCol());

   // The modify event doesn't seem to be firing so call the handler directly
   OnModifyCell(nRow,1);
}

void CGirderDescDebondGrid::DoRemoveRows()
{
	CGXRangeList* pSelList = GetParam()->GetRangeList();
	if (pSelList->IsAnyCellFromCol(0) || pSelList->IsAnyCellFromCol(1) || pSelList->IsAnyCellFromCol(2) || pSelList->IsAnyCellFromCol(3)
      && pSelList->GetCount() == 1)
	{
		CGXRange range = pSelList->GetHead();
		range.ExpandRange(1, 0, GetRowCount(), 0);
		RemoveRows(range.top, range.bottom);

      if ( 0 < GetRowCount() )
         SetCurrentCell(1,1); // if this is not here, the next insert will go below grid bottom
	}
   UpdateStrandLists();
}

void CGirderDescDebondGrid::CustomInit(bool bSymmetricDebond)
{
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

	Initialize( );

	GetParam( )->EnableUndo(FALSE);

   const int num_rows=0;
   const int num_cols=3;

	SetRowCount(num_rows);
	SetColCount(num_cols);

		// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELROW|GX_SELSHIFT|GX_SELKEYBOARD));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
         .SetHorizontalAlignment(DT_CENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

// set text along top row
	SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Strand"))
		);

   CString cv = CString(_T("Debond\nLength\n")) + 
                CString(pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str()) + 
                CString(_T("\nLeft End"));
	SetStyleRange(CGXRange(0,2), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv = CString(_T("Debond\nLength\n")) + 
        CString(pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str()) + 
        CString(_T("\nRight End"));
	SetStyleRange(CGXRange(0,3), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   // make it so that text fits correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();
	SetFocus();

	GetParam( )->EnableUndo(TRUE);

   SymmetricDebond(bSymmetricDebond);
}

void CGirderDescDebondGrid::SetRowStyle(ROWCOL nRow)
{
	GetParam()->EnableUndo(FALSE);

   CString strStrands = GetStrandList(nRow);

   // Get the first entry from the strand list... this will be the default selection
   int idx = strStrands.Find(_T("\n"),0);
   CString strFirst = strStrands.Left(idx);

   SetStyleRange(CGXRange(nRow,0), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         );

   SetStyleRange(CGXRange(nRow,1), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)  //
			.SetChoiceList(strStrands)
			.SetValue(strFirst)
         .SetHorizontalAlignment(DT_RIGHT)
         );

	SetStyleRange(CGXRange(nRow,2), CGXStyle()
			.SetUserAttribute(GX_IDS_UA_VALID_MIN, _T("0.0e01"))
			.SetUserAttribute(GX_IDS_UA_VALID_MAX, _T("1.0e99"))
			.SetUserAttribute(GX_IDS_UA_VALID_MSG, _T("Please enter a positive value"))
         .SetHorizontalAlignment(DT_RIGHT)
		);

	SetStyleRange(CGXRange(nRow,3), CGXStyle()
			.SetUserAttribute(GX_IDS_UA_VALID_MIN, _T("0.0e01"))
			.SetUserAttribute(GX_IDS_UA_VALID_MAX, _T("1.0e99"))
			.SetUserAttribute(GX_IDS_UA_VALID_MSG, _T("Please enter a positive value"))
         .SetHorizontalAlignment(DT_RIGHT)
		);


	GetParam()->EnableUndo(TRUE);
}

CString CGirderDescDebondGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CGirderDescDebondGrid::FillGrid(const std::vector<CDebondInfo>& debondInfo)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);
 
   // remove all but top row
   ROWCOL rows = GetRowCount();
   if (1 <= rows)
	   RemoveRows(1, rows);

   ROWCOL row = 1;
   std::vector<CDebondInfo>::const_iterator iter;
   for ( iter = debondInfo.begin(); iter != debondInfo.end(); iter++ )
   {
      InsertRow();

      const CDebondInfo& debond_info = *iter;

      /////////////////////// Strand Index ///////////////////////
      CString strStrand;
      if ( debond_info.idxStrand2 != -1 )
         strStrand.Format(_T("%d & %d"),debond_info.idxStrand1+1,debond_info.idxStrand2+1);
      else
         strStrand.Format(_T("%d"),debond_info.idxStrand1+1);

      SetStyleRange(CGXRange(row,1), CGXStyle().SetValue(strStrand));


      //////////////////////// Debond Length - Left /////////////////
      Float64 debond_length = debond_info.Length1;
      debond_length = ::ConvertFromSysUnits(debond_length,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   
      CString strDebondLength;
      strDebondLength.Format(_T("%g"),debond_length);
      SetStyleRange(CGXRange(row,2), CGXStyle().SetValue(strDebondLength));

      //////////////////////// Debond Length - Right /////////////////
      debond_length = debond_info.Length2;
      debond_length = ::ConvertFromSysUnits(debond_length,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   
      strDebondLength.Format(_T("%g"),debond_length);
      SetStyleRange(CGXRange(row,3), CGXStyle().SetValue(strDebondLength));

      row++;
   }

   UpdateStrandLists();

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CGirderDescDebondGrid::GetData(std::vector<CDebondInfo>& debondInfo)
{
   debondInfo.clear();
   ROWCOL nRows = GetRowCount();

   for ( ROWCOL row = 0; row < nRows; row++ )
   {
      CDebondInfo debond_info;


      ////////////// Strand Index ////////////////////
      CString strStrands = GetCellValue(row+1,1);

      int idx = strStrands.Find(_T("&"),0);
      if ( idx < 0 )
      {
         // '&' not found... therefore only a single value
         debond_info.idxStrand1 = _tstoi(strStrands) - 1;
         debond_info.idxStrand2 = INVALID_INDEX;
      }
      else
      {
         // there are two strand indicies
         debond_info.idxStrand1 = _tstoi(strStrands.Mid(0,idx)) - 1;
         debond_info.idxStrand2 = _tstoi(strStrands.Mid(idx+1)) - 1;
      }

      ///////// Debond Length - Left ///////////////
      Float64 length = GetLeftDebondLength(row+1);
      debond_info.Length1 = length;

      ///////// Debond Length - Right ///////////////
      if ( m_bSymmetricDebond )
      {
         debond_info.Length2 = debond_info.Length1;
      }
      else
      {
         length = GetRightDebondLength(row+1);
         debond_info.Length2 = length;
      }

      debondInfo.push_back(debond_info);
   }
}

Float64 CGirderDescDebondGrid::GetLeftDebondLength(ROWCOL row)
{
   return GetDebondLength(row,2);
}


Float64 CGirderDescDebondGrid::GetRightDebondLength(ROWCOL row)
{
   return GetDebondLength(row,3);
}

Float64 CGirderDescDebondGrid::GetDebondLength(ROWCOL row,ROWCOL col)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   Float64 length;
   CString strDebondLength = GetCellValue(row,col);
   length = _tstof(strDebondLength);

   // this is in display units... convert to system units
   length = ::ConvertToSysUnits(length,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);

   return length;
}

void CGirderDescDebondGrid::OnModifyCell(ROWCOL nRow,ROWCOL nCol)
{
   CGirderDescDebondPage* pdlg = (CGirderDescDebondPage*)GetParent();

   if ( nCol == 1 )
   {
      UpdateStrandLists();

      // cause the dialog to be repainted so that the picture
      // of the strands is up to date
      pdlg->Invalidate();
      pdlg->UpdateWindow();
   }

   pdlg->OnChange();
}

void CGirderDescDebondGrid::UpdateStrandLists()
{
   ROWCOL nRows = GetRowCount();
   for ( ROWCOL row = 0; row < nRows; row++ )
   {
      CString strStrands = GetStrandList(row);

      CString strCurrent;
      strCurrent = GetCellValue(row+1,1);

      SetStyleRange(CGXRange(row+1,1), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(strStrands)
			.SetValue(strCurrent)
         );
   }
}

CString CGirderDescDebondGrid::GetStrandList(ROWCOL nRow)
{
   CGirderDescDebondPage* pdlg = (CGirderDescDebondPage*)GetParent();
   std::vector<CString> strList = pdlg->GetStrandList();

   // Take strand numbers used in other rows out of the list
   ROWCOL nRows = GetRowCount();
   for ( ROWCOL row = 0; row < nRows; row++ )
   {
      if ( row == nRow )
         continue;

      CString strUsed = GetCellValue(row+1,1);
      if ( strUsed.GetLength() == 0 )
         continue;

      // If strands are used in another row, remove from this list
      bool found = false;
      for (std::vector<CString>::iterator it=strList.begin(); it!=strList.end(); it++)
      {
         if (*it == strUsed)
         {
            strList.erase(it);
            found = true;
            break;
         }
      }
      ATLASSERT(found); // should always find a match
   }

   // now build our string
   CString this_rows_list;
   for (std::vector<CString>::iterator it=strList.begin(); it!=strList.end(); it++)
   {
      this_rows_list += *it;
      this_rows_list += _T("\n");
   }

   return this_rows_list;
}

StrandIndexType CGirderDescDebondGrid::GetNumDebondedStrands()
{
   std::vector<CDebondInfo> debond_info;
   GetData(debond_info);
   StrandIndexType count = 0;
   std::vector<CDebondInfo>::iterator iter;
   for ( iter = debond_info.begin(); iter != debond_info.end(); iter++ )
   {
      CDebondInfo& di = *iter;
      if ( di.idxStrand1 != Uint32_Max )
         count++;

      if ( di.idxStrand2 != Uint32_Max )
         count++;
   }

   return count;
}

void CGirderDescDebondGrid::SymmetricDebond(bool bSymmetricDebond)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);


   m_bSymmetricDebond = bSymmetricDebond;

   CString strColHeading = CString(_T("Debond\nLength\n")) + 
                           CString(pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str());

   if ( m_bSymmetricDebond )
   {
      VERIFY(HideCols(3,3));
      strColHeading += _T("\nBoth End");
   }
   else
   {
      VERIFY(HideCols(3,3,FALSE));
      strColHeading += _T("\nLeft End");
   }

	SetStyleRange(CGXRange(0,2), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strColHeading)
		);
}


BOOL CGirderDescDebondGrid::OnLButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
   CGirderDescDebondPage* pdlg = (CGirderDescDebondPage*)GetParent();
   ASSERT (pdlg);

   ROWCOL nrows = GetRowCount();

   if (nCol==0 &&nRow>0)
      pdlg->OnEnableDelete(true);
   else
      pdlg->OnEnableDelete(false);

   return TRUE;
}
