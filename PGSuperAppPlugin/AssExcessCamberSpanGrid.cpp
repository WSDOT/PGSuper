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

// AssExcessCamberSpanGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "AssExcessCamberSpanGrid.h"

#include <System\Tokenizer.h>
#include "PGSuperUnits.h"
#include <Units\Measure.h>
#include <EAF\EAFDisplayUnits.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CAssExcessCamberSpanGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CAssExcessCamberSpanGrid

CAssExcessCamberSpanGrid::CAssExcessCamberSpanGrid()
{
//   RegisterClass();
}

CAssExcessCamberSpanGrid::~CAssExcessCamberSpanGrid()
{
}

BEGIN_MESSAGE_MAP(CAssExcessCamberSpanGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CAssExcessCamberSpanGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAssExcessCamberSpanGrid message handlers

int CAssExcessCamberSpanGrid::GetColWidth(ROWCOL nCol)
{
   if ( IsColHidden(nCol) )
      return CGXGridWnd::GetColWidth(nCol);

	CRect rect = GetGridRect( );

   switch (nCol)
   {
   case 0:
      return (int)(rect.Width( )*(Float64)4/10);
   case 1:
      return (int)(rect.Width( )*(Float64)6/10);
   default:
      ASSERT(0);
      return (int)(rect.Width( )/2);
   }
}

void CAssExcessCamberSpanGrid::CustomInit()
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
   const int num_cols=1;

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

   CString strval;
   strval.Format(_T("Assumed Excess Camber\n(%s)"), M_pCompUnit->UnitOfMeasure.UnitTag().c_str());

// set text along top row
	SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strval)
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

void CAssExcessCamberSpanGrid::SetRowStyle(ROWCOL nRow)
{

	GetParam()->EnableUndo(FALSE);

	SetStyleRange(CGXRange(nRow,1), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
		);

	GetParam()->EnableUndo(TRUE);
}

CString CAssExcessCamberSpanGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CAssExcessCamberSpanGrid::FillGrid(const HaunchInputData& haunchData)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);
 
   // remove all but top row
   ROWCOL rows = GetRowCount();
   if (1 <= rows)
	   RemoveRows(1, rows);

   // One row for each available strand row
   ROWCOL numRows = (ROWCOL)haunchData.m_AssExcessCamberSpans.size();
   ROWCOL row = 1;
   InsertRows(row, numRows);

   AssExcessCamberSpanDataConstIter iter = haunchData.m_AssExcessCamberSpans.begin();
   for (ROWCOL row = 1; row<=numRows; row++  )
   {
      const AssExcessCamberSpanData& hp = *iter;

      SetRowStyle(row);

      // row index
      CString crow;
      crow.Format(_T("Span %d"),row);
      SetStyleRange(CGXRange(row,0), CGXStyle()
         .SetValue(crow));

      // AssExcessCamber
      SetStyleRange(CGXRange(row,1), CGXStyle()
         .SetReadOnly(FALSE)
         .SetEnabled(TRUE)
         .SetValue(FormatDimension(hp.m_AssExcessCambersForGirders[0],*M_pCompUnit, false))
         );

      iter++;
   }

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);

	ScrollCellInView(1, GetLeftCol());
}

void CAssExcessCamberSpanGrid::GetData(HaunchInputData* pData, CDataExchange* pDX)
{
   pData->m_AssExcessCamberType = pgsTypes::aecSpan;

   ROWCOL nRows = GetRowCount();

   for ( ROWCOL row = 1; row <= nRows; row++ )
   {
      AssExcessCamberSpanData* php = &(pData->m_AssExcessCamberSpans[row-1]);

      CString str = GetCellValue(row,1);
      if (!str.IsEmpty())
      {
         Float64 val;
         if(sysTokenizer::ParseDouble(str, &val))
         {
            val = ::ConvertToSysUnits(val, M_pCompUnit->UnitOfMeasure);

            php->m_AssExcessCambersForGirders[0] = val; // use first slot in girder vector for all spans
         }
         else
         {
            AfxMessageBox( _T("Assumed Excess Camber value is not a number - must be a number zero or greater"), MB_ICONEXCLAMATION);
            this->SetCurrentCell(row,1,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
            pDX->Fail();
         }
      }
   }
}
