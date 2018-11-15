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

// HaunchSpanGrid.cpp : implementation file
//

#include "stdafx.h"
#include "HaunchSpanGrid.h"

#include <System\Tokenizer.h>
#include "PGSuperUnits.h"
#include <Units\Measure.h>
#include <EAF\EAFDisplayUnits.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const ROWCOL _STARTCOL = 1;

GRID_IMPLEMENT_REGISTER(CHaunchSpanGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CHaunchSpanGrid

CHaunchSpanGrid::CHaunchSpanGrid()
{
//   RegisterClass();
}

CHaunchSpanGrid::~CHaunchSpanGrid()
{
}

BEGIN_MESSAGE_MAP(CHaunchSpanGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CHaunchSpanGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CHaunchSpanGrid message handlers

int CHaunchSpanGrid::GetColWidth(ROWCOL nCol)
{
   if ( IsColHidden(nCol) )
      return CGXGridWnd::GetColWidth(nCol);

	CRect rect = GetGridRect( );

   switch (nCol)
   {
   case 0:
   case 1:
      return (int)(rect.Width( )*(Float64)8/20);
   case 2:
   case 3:
      return (int)(rect.Width( )*(Float64)6/20);
   default:
      ASSERT(0);
      return (int)(rect.Width( )/5);
   }
}

void CHaunchSpanGrid::CustomInit()
{
   // initialize units
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   M_pCompUnit = &(pDisplayUnits->GetComponentDimUnit());

// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );

	GetParam( )->EnableUndo(FALSE);

   const int num_rows=0;
   const int num_cols=_STARTCOL+2;

	SetRowCount(num_rows);
	SetColCount(num_cols);

   // Turn off row, column, and whole table selection
   GetParam()->EnableSelection((WORD)(GX_SELFULL & ~GX_SELCOL & ~GX_SELROW & ~GX_SELTABLE));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

   // Header
   ROWCOL col=1;
	SetStyleRange(CGXRange(0,col,0,col+1), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
			.SetValue(_T("Location"))
		);

   CString strLabel;
   strLabel.Format(_T("Slab Offset (%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,col+2), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

   // Hide the row header column
   HideCols(0, 0);

	EnableIntelliMouse();
	SetFocus();

	GetParam( )->EnableUndo(TRUE);
}

void CHaunchSpanGrid::SetRowStyle(ROWCOL nRow)
{
   ROWCOL col = _STARTCOL;
	SetStyleRange(CGXRange(nRow,_STARTCOL,nRow,_STARTCOL+1), CGXStyle()
 			.SetEnabled(FALSE)          // disables usage as current cell
			.SetInterior(GXSYSCOLOR(COLOR_BTNFACE))
			.SetHorizontalAlignment(DT_CENTER)
			.SetVerticalAlignment(DT_VCENTER)
		);

	SetStyleRange(CGXRange(nRow,_STARTCOL+2), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
		);
}

void CHaunchSpanGrid::FillGrid(const HaunchInputData& haunchData)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);
 
   ROWCOL rows = GetRowCount();
   if (0 < rows)
	   RemoveRows(0, rows);

   // we want to merge cells
   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   // One row for each bearing line
   ROWCOL numRows = (ROWCOL)haunchData.m_BearingsSlabOffset.size();
   ROWCOL row = 1;
   InsertRows(row, numRows);

   m_GirderCounts.reserve(numRows);

   PierIndexType PierNo=1;
   SlabOffsetBearingDataConstIter iter = haunchData.m_BearingsSlabOffset.begin();
   for (; row<=numRows; row++  )
   {
      const SlabOffsetBearingData& hp = *iter;

      SetRowStyle(row);

      ROWCOL col = _STARTCOL;

      // row index
      CString crow;
      if (row==1 || row==numRows)
         crow.Format(_T("Abutment %d"),PierNo);
      else
         crow.Format(_T("Pier %d"),PierNo);

      SetStyleRange(CGXRange(row,col++), CGXStyle()
         .SetValue(crow));

      if (hp.m_PDType==SlabOffsetBearingData::pdAhead)
         crow = _AHEADSTR;
      else if (hp.m_PDType==SlabOffsetBearingData::pdBack)
         crow = _BACKSTR;
      else if (hp.m_PDType==SlabOffsetBearingData::pdCL)
         crow = _CLSTR;

      SetStyleRange(CGXRange(row,col++), CGXStyle()
         .SetValue(crow));

      SetStyleRange(CGXRange(row,col++), CGXStyle()
         .SetReadOnly(FALSE)
         .SetEnabled(TRUE)
         .SetValue(FormatDimension(hp.m_AsForGirders[0],*M_pCompUnit, false)) // use A at slot[0]
         );

      if (row!=1&& hp.m_PDType==SlabOffsetBearingData::pdAhead)
      {
	      SetStyleRange(CGXRange(row-1,_STARTCOL,row,_STARTCOL), CGXStyle()
               .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
	      );
      }

      if (hp.m_PDType!=SlabOffsetBearingData::pdBack)
         PierNo++;

      // save number of girders at bearing line
      m_GirderCounts.push_back(hp.m_AsForGirders.size());

      iter++;
   }

	ResizeRowHeightsToFit(CGXRange(0,0,numRows,2));

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);

	ScrollCellInView(1, GetLeftCol());
}

void CHaunchSpanGrid::GetData(Float64 minA, CString& minValError, HaunchInputData* pData, CDataExchange* pDX)
{
   pData->m_SlabOffsetType = pgsTypes::sotPier;

   SlabOffsetBearingDataIter brgIt = pData->m_BearingsSlabOffset.begin();

   ROWCOL nRows = GetRowCount();
   for ( ROWCOL row = 1; row <= nRows; row++ )
   {
      SlabOffsetBearingData& hp = *brgIt;

      CString str = GetCellValue(row,3);
      if (!str.IsEmpty())
      {
         Float64 val;
         if(sysTokenizer::ParseDouble(str, &val))
         {
            val = ::ConvertToSysUnits(val, M_pCompUnit->UnitOfMeasure);

            // Assign A to all girders in girder line
            hp.m_AsForGirders.assign(m_GirderCounts[row-1], val);

            if (val < minA)
            {
               AfxMessageBox( minValError, MB_ICONEXCLAMATION);
               this->SetCurrentCell(row,2,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
               pDX->Fail();
            }
         }
         else
         {
            AfxMessageBox( _T("Value is not a number - must be a positive number"), MB_ICONEXCLAMATION);
            this->SetCurrentCell(row,2,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
            pDX->Fail();
         }
      }

      brgIt++;
   }
}

CString CHaunchSpanGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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