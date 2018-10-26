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

#define DEBOND_CHECK_COL 1
#define FIRST_DEBOND_COL 2
#define LAST_DEBOND_COL  3
#define FIRST_EXTEND_COL 4
#define LAST_EXTEND_COL  5

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
   if ( nCol == 1 && !IsColHidden(nCol))
      return 15;

   return CGXGridWnd::GetColWidth(nCol);
}

void CGirderDescDebondGrid::InsertRow()
{
	ROWCOL nRow = GetRowCount()+1;

	InsertRows(nRow, 1);
   SetRowStyle(nRow);
}

void CGirderDescDebondGrid::CustomInit(bool bSymmetricDebond)
{
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   CGirderDescDlg* pParent = (CGirderDescDlg*)(GetParent()->GetParent());

	Initialize( );

	GetParam( )->EnableUndo(FALSE);

   const int num_rows=0;
   const int num_cols=5;

	SetRowCount(num_rows);
	SetColCount(num_cols);

   int col = 0;

		// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELROW|GX_SELSHIFT|GX_SELKEYBOARD));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,col), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
         .SetHorizontalAlignment(DT_CENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

// set text along top row
	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Strand"))
		);

   CGXFont font;
   font.SetOrientation(900); // vertical text 

	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetFont(font)
			.SetValue(_T("Debond"))
		);

   CString cv = CString(_T("Debond\nLength\n")) + 
                CString(pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str()) + 
                CString(_T("\nLeft End"));
	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv = CString(_T("Debond\nLength\n")) + 
        CString(pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure.UnitTag().c_str()) + 
        CString(_T("\nRight End"));
	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Extend\nLeft\nEnd"))
		);

	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Extend\nRight\nEnd"))
		);

   // make it so that text fits correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();
	SetFocus();


   // if their can't be any debonded strands, hide the debond columns
   CanDebond( pStrandGeom->CanDebondStrands(pParent->m_strGirderName.c_str(),pgsTypes::Straight) );

   GET_IFACE2(pBroker,ISpecification,pSpec);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   if ( !pSpecEntry->AllowStraightStrandExtensions() )
   {
      VERIFY(HideCols(FIRST_EXTEND_COL,LAST_EXTEND_COL,TRUE));
   }

	GetParam( )->EnableUndo(TRUE);

   SymmetricDebond(bSymmetricDebond);
}

void CGirderDescDebondGrid::SetRowStyle(ROWCOL nRow)
{
	GetParam()->EnableUndo(FALSE);

   int col = 0;

   SetStyleRange(CGXRange(nRow,col++), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         );

   SetStyleRange(CGXRange(nRow,col++), CGXStyle()
			.SetControl(GX_IDS_CTRL_CHECKBOX3D)  //
         .SetHorizontalAlignment(DT_CENTER)
         );

	SetStyleRange(CGXRange(nRow,col++), CGXStyle()
			.SetUserAttribute(GX_IDS_UA_VALID_MIN, _T("0.0e01"))
			.SetUserAttribute(GX_IDS_UA_VALID_MAX, _T("1.0e99"))
			.SetUserAttribute(GX_IDS_UA_VALID_MSG, _T("Please enter a positive value"))
         .SetHorizontalAlignment(DT_RIGHT)
		);

	SetStyleRange(CGXRange(nRow,col++), CGXStyle()
			.SetUserAttribute(GX_IDS_UA_VALID_MIN, _T("0.0e01"))
			.SetUserAttribute(GX_IDS_UA_VALID_MAX, _T("1.0e99"))
			.SetUserAttribute(GX_IDS_UA_VALID_MSG, _T("Please enter a positive value"))
         .SetHorizontalAlignment(DT_RIGHT)
		);


   SetStyleRange(CGXRange(nRow,col++), CGXStyle()
			.SetControl(GX_IDS_CTRL_CHECKBOX3D)  //
         .SetHorizontalAlignment(DT_CENTER)
         );

   SetStyleRange(CGXRange(nRow,col++), CGXStyle()
			.SetControl(GX_IDS_CTRL_CHECKBOX3D)  //
         .SetHorizontalAlignment(DT_CENTER)
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

void CGirderDescDebondGrid::FillGrid(const CGirderData& girderData)
{
   CGirderDescDlg* pParent = (CGirderDescDlg*)(GetParent()->GetParent());

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);
 
   // remove all but top row
   ROWCOL rows = GetRowCount();
   if (1 <= rows)
      RemoveRows(1, rows);

   ROWCOL row = 1;

   CComPtr<ILongArray> debondables;
   pStrandGeom->ListDebondableStrands(pParent->m_strGirderName.c_str(), pgsTypes::Straight, &debondables);

   StrandIndexType nStrands = girderData.Nstrands[pgsTypes::Straight];
   StrandIndexType currnum = 0;
   while( currnum < nStrands )
   {
      InsertRow();

      // put the strand number in the first column
      StrandIndexType nextnum = pStrandGeom->GetNextNumStrands(pParent->m_strGirderName.c_str(), pgsTypes::Straight, currnum);

      Int32 hash;
      CString str;
      if (nextnum-currnum == 1 )
      {
         hash = make_Int32(-1,(Int16)(nextnum-1));
         str.Format(_T("%d"),nextnum);
      }
      else
      {
         hash = make_Int32((Int16)(nextnum-2),(Uint16)(nextnum-1));
         str.Format(_T("%d & %d"),nextnum-1,nextnum);
      }

      SetStyleRange(CGXRange(row,0),CGXStyle().SetValue(str).SetIncludeItemDataPtr(TRUE).SetItemDataPtr((void*)hash));

      // disable debond cells for rows where the strands cannot be debonded
      IDType is_debondable;
      debondables->get_Item(nextnum-1,&is_debondable);
      if ( !is_debondable )
      {
         for (ROWCOL c = DEBOND_CHECK_COL; c <= LAST_DEBOND_COL; c++ )
         {
            SetStyleRange(CGXRange(row,c),CGXStyle()
               .SetReadOnly(TRUE)
               .SetEnabled(FALSE)
               .SetInterior(::GetSysColor(COLOR_BTNFACE))
               .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
               );
         }
      }
      else
      {
         for (ROWCOL c = FIRST_DEBOND_COL; c <= LAST_DEBOND_COL; c++ )
         {
            SetStyleRange(CGXRange(row,c),CGXStyle()
               .SetReadOnly(TRUE)
               .SetEnabled(FALSE)
               .SetInterior(::GetSysColor(COLOR_BTNFACE))
               .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
               );
         }
      }

      currnum = nextnum;

      row++;
   }

   std::vector<CDebondInfo>::const_iterator debond_iter;
   for ( debond_iter = girderData.Debond[pgsTypes::Straight].begin(); debond_iter != girderData.Debond[pgsTypes::Straight].end(); debond_iter++ )
   {
      const CDebondInfo& debond_info = *debond_iter;
      ROWCOL row = GetRow(debond_info.idxStrand1);

      // set debond check mark
      SetStyleRange(CGXRange(row,DEBOND_CHECK_COL),CGXStyle().SetValue(_T("1")));

      //////////////////////// Debond Length - Left /////////////////
      Float64 debond_length = debond_info.Length1;
      debond_length = ::ConvertFromSysUnits(debond_length,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   
      CString strDebondLength;
      strDebondLength.Format(_T("%g"),debond_length);
      SetStyleRange(CGXRange(row,FIRST_DEBOND_COL), CGXStyle()
         .SetValue(strDebondLength)
         .SetReadOnly(FALSE)
         .SetEnabled(TRUE)
         .SetInterior(::GetSysColor(COLOR_WINDOW))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         );

      //////////////////////// Debond Length - Right /////////////////
      debond_length = debond_info.Length2;
      debond_length = ::ConvertFromSysUnits(debond_length,pDisplayUnits->GetXSectionDimUnit().UnitOfMeasure);
   
      strDebondLength.Format(_T("%g"),debond_length);
      SetStyleRange(CGXRange(row,LAST_DEBOND_COL), CGXStyle()
         .SetValue(strDebondLength)
         .SetReadOnly(FALSE)
         .SetEnabled(TRUE)
         .SetInterior(::GetSysColor(COLOR_WINDOW))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         );

      // Disable extend strands check boxes
      SetStyleRange(CGXRange(row,FIRST_EXTEND_COL),CGXStyle()
         .SetValue(_T(""))
         .SetReadOnly(TRUE)
         .SetEnabled(FALSE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
         );

      SetStyleRange(CGXRange(row,LAST_EXTEND_COL),CGXStyle()
         .SetValue(_T(""))
         .SetReadOnly(TRUE)
         .SetEnabled(FALSE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
         );
   }

   std::vector<StrandIndexType>::const_iterator extend_iter(girderData.NextendedStrands[pgsTypes::Straight][pgsTypes::metStart].begin());
   std::vector<StrandIndexType>::const_iterator extend_iter_end(girderData.NextendedStrands[pgsTypes::Straight][pgsTypes::metStart].end());
   for ( ; extend_iter != extend_iter_end; extend_iter++ )
   {
      ROWCOL row = GetRow(*extend_iter);
      if ( row != -1 )
         SetStyleRange(CGXRange(row,FIRST_EXTEND_COL),CGXStyle().SetValue(_T("1")));
   }

   extend_iter = girderData.NextendedStrands[pgsTypes::Straight][pgsTypes::metEnd].begin();
   extend_iter_end = girderData.NextendedStrands[pgsTypes::Straight][pgsTypes::metEnd].end();
   for ( ; extend_iter != extend_iter_end; extend_iter++ )
   {
      ROWCOL row = GetRow(*extend_iter);
      if ( row != -1 )
         SetStyleRange(CGXRange(row,LAST_EXTEND_COL),CGXStyle().SetValue(_T("1")));
   }

   ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CGirderDescDebondGrid::GetData(CGirderData& girderData)
{
   girderData.Debond[pgsTypes::Straight].clear();
   girderData.NextendedStrands[pgsTypes::Straight][pgsTypes::metStart].clear();
   girderData.NextendedStrands[pgsTypes::Straight][pgsTypes::metEnd].clear();
   
   ROWCOL nRows = GetRowCount();

   for ( ROWCOL row = 0; row < nRows; row++ )
   {
      CGXStyle style;
      GetStyleRowCol(row+1,0,style);
      Int32 hash = (Int32)style.GetItemDataPtr();
      StrandIndexType strandIdx1 = (StrandIndexType)low_Int16(hash);
      StrandIndexType strandIdx2 = (StrandIndexType)high_Int16(hash);

      ////////////// Strand Index ////////////////////
      CString strDebondCheck = GetCellValue(row+1,1);
      if ( strDebondCheck == _T("1") )
      {
         CDebondInfo debond_info;

         debond_info.idxStrand1 = strandIdx1;
         debond_info.idxStrand2 = strandIdx2;

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

         girderData.Debond[pgsTypes::Straight].push_back(debond_info);
      }
      else
      {
         for ( ROWCOL c = FIRST_EXTEND_COL; c <= LAST_EXTEND_COL; c++ )
         {
            CString strCheck = GetCellValue(row+1,c);
            if ( strCheck == _T("1") )
            {
               if ( strandIdx1 != INVALID_INDEX )
                  girderData.NextendedStrands[pgsTypes::Straight][c == FIRST_EXTEND_COL ? pgsTypes::metStart : pgsTypes::metEnd].push_back(strandIdx1);

               if ( strandIdx2 != INVALID_INDEX )
                  girderData.NextendedStrands[pgsTypes::Straight][c == FIRST_EXTEND_COL ? pgsTypes::metStart : pgsTypes::metEnd].push_back(strandIdx2);
            }
         }
      }
   }
}

Float64 CGirderDescDebondGrid::GetLeftDebondLength(ROWCOL row)
{
   return GetDebondLength(row,FIRST_DEBOND_COL);
}

Float64 CGirderDescDebondGrid::GetRightDebondLength(ROWCOL row)
{
   return GetDebondLength(row,LAST_DEBOND_COL);
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

ROWCOL CGirderDescDebondGrid::GetRow(StrandIndexType strandIdx)
{
   ROWCOL nRows = GetRowCount();
   for ( ROWCOL row = 0; row < nRows; row++ )
   {
      CGXStyle style;
      GetStyleRowCol(row+1,0,style);
      Int32 hash = (Int32)style.GetItemDataPtr();
      StrandIndexType strandIdx1 = (StrandIndexType)high_Int16(hash);
      StrandIndexType strandIdx2 = (StrandIndexType)low_Int16(hash);

      if ( strandIdx == strandIdx1 || strandIdx == strandIdx2 )
         return row+1;
   }

   return -1;
}

void CGirderDescDebondGrid::OnClickedButtonRowCol(ROWCOL nHitRow,ROWCOL nHitCol)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CGirderDescDebondPage* pdlg = (CGirderDescDebondPage*)GetParent();

   if ( nHitCol == DEBOND_CHECK_COL )
   {
      // Debond check box was clicked
      // Determine if we are turning debonding on or off...
      // If the box is checked, uncheck and disable the Extend Strands check boxes, 
      // otherwise enable them
      CString strCheck = GetCellValue(nHitRow,nHitCol);
      if ( strCheck == _T("1") )
      {
         // box was unchecked and now it is checked
         for (ROWCOL c = FIRST_DEBOND_COL; c <= LAST_DEBOND_COL; c++ )
         {
            // Enable the debond length input cells
            SetStyleRange(CGXRange(nHitRow,c),CGXStyle()
               .SetEnabled(TRUE)
               .SetReadOnly(FALSE)
               .SetInterior(::GetSysColor(COLOR_WINDOW))
               .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
               );
         }

         // disable the extended strand cells
         for (ROWCOL c = FIRST_EXTEND_COL; c <= LAST_EXTEND_COL; c++ )
         {
            // Uncheck the extend strands box and disable it
            SetStyleRange(CGXRange(nHitRow,c),CGXStyle().SetValue(_T("0"))
               .SetEnabled(FALSE)
               .SetReadOnly(TRUE)
               .SetInterior(::GetSysColor(COLOR_BTNFACE))
               .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
               );

         }
      }
      else
      {
         // box was checked and now it is unchecked
         for (ROWCOL c = FIRST_DEBOND_COL; c <= LAST_DEBOND_COL; c++ )
         {
            // disable the debond length cells
            SetStyleRange(CGXRange(nHitRow,c),CGXStyle()
               .SetValue(_T("")) // erase the current value
               .SetEnabled(FALSE)
               .SetReadOnly(TRUE)
               .SetInterior(::GetSysColor(COLOR_BTNFACE))
               .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
               );
         }

         for (ROWCOL c = FIRST_EXTEND_COL; c <= LAST_EXTEND_COL; c++ )
         {
            // Set the value of the extended strands check box to "unchecked"
            SetStyleRange(CGXRange(nHitRow,c),CGXStyle()
               .SetEnabled(TRUE)
               .SetReadOnly(FALSE)
               .SetInterior(::GetSysColor(COLOR_WINDOW))
               .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
               );
         }
      }
   }

   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(TRUE);

   pdlg->OnChange();
}

StrandIndexType CGirderDescDebondGrid::GetNumDebondedStrands()
{
   ROWCOL nRows = GetRowCount();

   StrandIndexType nStrands = 0;
   for ( ROWCOL row = 0; row < nRows; row++ )
   {
      CGXStyle style;
      GetStyleRowCol(row+1,0,style);
      Int32 hash = (Int32)style.GetItemDataPtr();
      StrandIndexType strandIdx1 = (StrandIndexType)low_Int16(hash);
      StrandIndexType strandIdx2 = (StrandIndexType)high_Int16(hash);

      CString strDebondCheck = GetCellValue(row+1, DEBOND_CHECK_COL);
      if ( strandIdx1 != INVALID_INDEX && strDebondCheck == _T("1") )
         nStrands++;

      if ( strandIdx2 != INVALID_INDEX && strDebondCheck == _T("1") )
         nStrands++;
   }

   return nStrands;
}

StrandIndexType CGirderDescDebondGrid::GetNumExtendedStrands()
{
   ROWCOL nRows = GetRowCount();

   StrandIndexType nStrands = 0;
   for ( ROWCOL row = 0; row < nRows; row++ )
   {
      CGXStyle style;
      GetStyleRowCol(row+1,0,style);
      Int32 hash = (Int32)style.GetItemDataPtr();
      StrandIndexType strandIdx1 = (StrandIndexType)low_Int16(hash);
      StrandIndexType strandIdx2 = (StrandIndexType)high_Int16(hash);

      CString strExtendLeft  = GetCellValue(row+1, FIRST_EXTEND_COL);
      CString strExtendRight = GetCellValue(row+1, LAST_EXTEND_COL);
      if ( strandIdx1 != INVALID_INDEX && (strExtendLeft == _T("1") || strExtendRight == _T("1")) )
         nStrands++;

      if ( strandIdx2 != INVALID_INDEX && (strExtendLeft == _T("1") || strExtendRight == _T("1")) )
         nStrands++;
   }

   return nStrands;
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
      VERIFY(HideCols(LAST_DEBOND_COL,LAST_DEBOND_COL));
      strColHeading += _T("\nBoth");
   }
   else
   {
      VERIFY(HideCols(LAST_DEBOND_COL,LAST_DEBOND_COL,FALSE));
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

void CGirderDescDebondGrid::CanDebond(bool bCanDebond)
{
   VERIFY(HideCols(DEBOND_CHECK_COL,LAST_DEBOND_COL,bCanDebond ? FALSE : TRUE));
   if ( bCanDebond && m_bSymmetricDebond )
      HideCols(LAST_DEBOND_COL,LAST_DEBOND_COL);
}
