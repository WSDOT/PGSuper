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

// StrandRowGrid.cpp : implementation file
//

#include "stdafx.h"
#include "StrandRowGrid.h"

#include <System\Tokenizer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CStrandRowGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CStrandRowGrid

CStrandRowGrid::CStrandRowGrid():
m_pHandler(NULL)
{
//   RegisterClass();
}

CStrandRowGrid::~CStrandRowGrid()
{
}

BEGIN_MESSAGE_MAP(CStrandRowGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CStrandRowGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CStrandRowGrid message handlers

int CStrandRowGrid::GetColWidth(ROWCOL nCol)
{
   if ( IsColHidden(nCol) )
      return CGXGridWnd::GetColWidth(nCol);

	CRect rect = GetGridRect( );

   switch (nCol)
   {
   case 0:
      return (int)(rect.Width( )*(Float64)4/20);
   case 1:
      return (int)(rect.Width( )*(Float64)8/20);
   case 2:
      return (int)(rect.Width( )*(Float64)8/20);
   default:
      ASSERT(0);
      return (int)(rect.Width( )/5);
   }
}

void CStrandRowGrid::CustomInit(StrandRowGridEventHandler* pHandler)
{
   m_pHandler = pHandler;
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );

	GetParam( )->EnableUndo(FALSE);

   const int num_rows=0;
   const int num_cols=2;

	SetRowCount(num_rows);
	SetColCount(num_cols);

   // Turn off selecting 
	GetParam()->EnableSelection((WORD) (~GX_SELROW & ~GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   // disable left side
	SetStyleRange(CGXRange(0,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
		);

// set text along top row
	SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Row\n(in)"))
		);

	SetStyleRange(CGXRange(0,2), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("No. Strands"))
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

void CStrandRowGrid::SetRowStyle(ROWCOL nRow)
{
	GetParam()->EnableUndo(FALSE);

	SetStyleRange(CGXRange(nRow,1), CGXStyle()
         .SetControl(GX_IDS_CTRL_STATIC)
         .SetHorizontalAlignment(DT_RIGHT)
		);

   SetStyleRange(CGXRange(nRow,2), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)  //
         .SetHorizontalAlignment(DT_RIGHT)
         );

	GetParam()->EnableUndo(TRUE);
}

CString CStrandRowGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CStrandRowGrid::FillGrid(const CTxDOTOptionalDesignGirderData::AvailableStrandsInRowContainer& availStrands,
                              const CTxDOTOptionalDesignGirderData::StrandRowContainer& filledStrandRows)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);
 
   // remove all but top row
   ROWCOL rows = GetRowCount();
   if (1 <= rows)
	   RemoveRows(1, rows);

   // One row for each available strand row
   ROWCOL numRows = (ROWCOL)availStrands.size();
   ROWCOL row = 1;
   InsertRows(row, numRows);

   CTxDOTOptionalDesignGirderData::AvailableStrandsInRowConstIterator iter = availStrands.begin();
   for (ROWCOL row = 1; row<=numRows; row++  )
   {
      const CTxDOTOptionalDesignGirderData::AvailableStrandsInRow& avail_row = *iter;

      SetRowStyle(row);

      // row index
      CString crow;
      crow.Format(_T("%d"),row);
      SetStyleRange(CGXRange(row,0), CGXStyle()
         .SetValue(crow));

      // Elevation
      Float64 elev = ::ConvertFromSysUnits(avail_row.RowElev ,unitMeasure::Inch);
      CString strelev;
      strelev.Format(_T("%.3f"),elev);

      SetStyleRange(CGXRange(row,1), CGXStyle()
         .SetReadOnly(TRUE)
         .SetUserAttribute(1,avail_row.RowElev) // use user attribute to store entire Float64 - this way we don't have to worry about string conversion
         .SetValue(strelev));

      // Combo box containing available strands
      CString strStrands = GetStrandRowList(avail_row.AvailableStrandIncrements);

      // See if this row contains data
      CString strCurrent;
      CTxDOTOptionalDesignGirderData::StrandRow temp_row(avail_row.RowElev, 0);

      CTxDOTOptionalDesignGirderData::StrandRowConstIterator srit = filledStrandRows.find(temp_row);
      if (srit != filledStrandRows.end())
      {
         const CTxDOTOptionalDesignGirderData::StrandRow& rsrow = *srit;
         strCurrent.Format(_T("%d"), rsrow.StrandsInRow);
      }
      else
      {
         // No strands at the elev - Get the first entry from the strand list as the default selection (should be "0")
         int idx = strStrands.Find(_T("\n"),0);
         strCurrent = strStrands.Left(idx);
      }

      SetStyleRange(CGXRange(row,2), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(strStrands)
			.SetValue(strCurrent)
         );

      iter++;
   }


   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);

	ScrollCellInView(1, GetLeftCol());
}

CTxDOTOptionalDesignGirderData::StrandRowContainer CStrandRowGrid::GetData()
{
   CTxDOTOptionalDesignGirderData::StrandRowContainer strandrows;

   ROWCOL nRows = GetRowCount();

   for ( ROWCOL row = 1; row <= nRows; row++ )
   {
      // Get rows that have strands
      CString strStrands = GetCellValue(row,2);
      if (strStrands!=_T("0"))
      {
         CTxDOTOptionalDesignGirderData::StrandRow strrow;

         long strandcnt;
         sysTokenizer::ParseLong(strStrands, &strandcnt);  // save num strands as integral value as well

         strrow.StrandsInRow = (StrandIndexType)strandcnt;

         // retreive Float64 from cell
         CGXStyle style;
         GetStyleRowCol(row, 1, style);
         const CGXAbstractUserAttribute& rat = style.GetUserAttribute(1);

         strrow.RowElev = rat.GetDoubleValue();

         strandrows.insert(strrow);
      }
   }

   return strandrows;
}

void CStrandRowGrid::ComputeStrands(StrandIndexType* pNum, Float64* pCg)
{
   StrandIndexType noStrands=0;
   Float64 cg=0.0;

   ROWCOL nRows = GetRowCount();

   for ( ROWCOL row = 1; row <= nRows; row++ )
   {
      // Get rows that have strands
      CString strStrands = GetCellValue(row,2);
      if (strStrands!=_T("0"))
      {
         long strandcnt;
         sysTokenizer::ParseLong(strStrands, &strandcnt);  // save num strands as integral value as well

         CString strRow = GetCellValue(row,1);
         Float64 rowht;
         sysTokenizer::ParseDouble(strRow, &rowht);  // save num strands as integral value as well

         noStrands += (StrandIndexType)strandcnt;

         cg += strandcnt * rowht;
      }
   }

   *pNum = noStrands;
   if (noStrands>0)
   {
      *pCg = cg/noStrands;
   }
   else
   {
      *pCg = 0.0;
   }
}

void CStrandRowGrid::OnModifyCell (ROWCOL nRow, ROWCOL nCol)
{
   m_pHandler->OnGridDataChanged();
}

CString CStrandRowGrid::GetStrandRowList(const std::vector<CTxDOTOptionalDesignGirderData::StrandIncrement>& rowData)
{
   // Build our string
   CString strands_list;
   for (std::vector<CTxDOTOptionalDesignGirderData::StrandIncrement>::const_iterator it=rowData.begin(); it!=rowData.end(); it++)
   {
      const CTxDOTOptionalDesignGirderData::StrandIncrement& strd = *it;
      CString tmp;
      tmp.Format(_T("%d\n"),strd.TotalStrands);

      strands_list += tmp;
   }

   return strands_list;
}
