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

// HaunchGirderGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "HaunchGirderGrid.h"

#include <System\Tokenizer.h>
#include "PGSuperUnits.h"
#include <Units\Measure.h>
#include <EAF\EAFDisplayUnits.h>
#include <PgsExt\GirderLabel.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CHaunchGirderGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CHaunchGirderGrid

CHaunchGirderGrid::CHaunchGirderGrid()
{
//   RegisterClass();
}

CHaunchGirderGrid::~CHaunchGirderGrid()
{
}

BEGIN_MESSAGE_MAP(CHaunchGirderGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CHaunchGirderGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CHaunchGirderGrid message handlers

int CHaunchGirderGrid::GetColWidth(ROWCOL nCol)
{
   if ( IsColHidden(nCol) )
      return CGXGridWnd::GetColWidth(nCol);

	CRect rect = GetGridRect( );

   return (int)(rect.Width( )*(Float64)1/7);
}

void CHaunchGirderGrid::CustomInit(SpanIndexType nSpans, GirderIndexType maxGirdersPerSpan)
{
   // initialize units
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   m_pCompUnit = &(pDisplayUnits->GetComponentDimUnit());

// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );

	GetParam( )->EnableUndo(FALSE);

   // we want to merge cells
   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   // Create entire grid with read-only place holders for inputs. Fill grid will change types to writeable later
   const int num_rows = (int)maxGirdersPerSpan;
   const int num_cols = (int)nSpans*2;

	SetRowCount(num_rows+1);
	SetColCount(num_cols);

   // Turn off selecting 
	GetParam()->EnableSelection((WORD) (~GX_SELROW & ~GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   // disable left side
	SetStyleRange(CGXRange(0,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
		);

   SetFrozenRows(1/*# frozen rows*/,1/*# extra header rows*/);

   ROWCOL col = 0;

   // set text along top rows
	SetStyleRange(CGXRange(0,col,1,col++), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
			.SetValue(_T(" "))
		);

   for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++)
   {
      CString strLabel;
      strLabel.Format(_T("Span %d"), LABEL_SPAN(spanIdx));

	   SetStyleRange(CGXRange(0,col,0,col+1), CGXStyle()
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_TOP)
			   .SetEnabled(FALSE)          // disables usage as current cell
            .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
			   .SetValue(strLabel)
		   );

      strLabel.Format(_T("Start (%s)"), m_pCompUnit->UnitOfMeasure.UnitTag().c_str());

      SetStyleRange(CGXRange(1,col++), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetEnabled(FALSE)
         .SetValue(strLabel)
         );

      strLabel.Format(_T("End (%s)"), m_pCompUnit->UnitOfMeasure.UnitTag().c_str());

      SetStyleRange(CGXRange(1,col++), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetEnabled(FALSE)
         .SetValue(strLabel)
         );
   }

// Set up rows as read only
   ROWCOL row = 2;
   for (int igdr=0; igdr<(int)maxGirdersPerSpan; igdr++)
   {
      CString strLabel;
      strLabel.Format(_T("Girder %s"), LABEL_GIRDER(igdr));

      SetStyleRange(CGXRange(row,0), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetEnabled(FALSE)
         .SetValue(strLabel)
         );

      SetStyleRange(CGXRange(row,1,row,num_cols), CGXStyle()
         .SetReadOnly(TRUE)
         .SetEnabled(FALSE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
			.SetUserAttribute(GX_IDS_UA_VALID_MIN, _T("0.0e01"))
			.SetUserAttribute(GX_IDS_UA_VALID_MAX, _T("1.0e99"))
			.SetUserAttribute(GX_IDS_UA_VALID_MSG, _T("Please enter a positive number"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

      row++;
   }

   // make text fit correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();
	SetFocus();

	GetParam( )->EnableUndo(TRUE);

}


void CHaunchGirderGrid::FillGrid(const HaunchInputData& haunchData)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);
 
   // Grid was already sized in CustomInit. Just fill and enable data locations
   ROWCOL rowStart = 2;
   ROWCOL col = 1;
   std::vector<HaunchPairVec>::const_iterator spanIt = haunchData.m_SpanGirdersHaunch.begin();
   for (; spanIt!=haunchData.m_SpanGirdersHaunch.end(); spanIt++)
   {
      ROWCOL row = rowStart;
      const HaunchPairVec& hpv = *spanIt;
      for (HaunchPairVecConstIter gdrIt = hpv.begin(); gdrIt!=hpv.end(); gdrIt++)
      {
         const HaunchPair& hp = *gdrIt;

         // Start

         SetStyleRange(CGXRange(row,col), CGXStyle()
            .SetReadOnly(FALSE)
            .SetEnabled(TRUE)
            .SetInterior(::GetSysColor(COLOR_WINDOW))
            .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
            .SetValue(FormatDimension(hp.first,*m_pCompUnit, false))
            );

         // end
         SetStyleRange(CGXRange(row,col+1), CGXStyle()
            .SetReadOnly(FALSE)
            .SetEnabled(TRUE)
            .SetInterior(::GetSysColor(COLOR_WINDOW))
            .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
            .SetValue(FormatDimension(hp.second,*m_pCompUnit, false))
            );

         row++;
      }

      col +=2;
   }

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);

	ScrollCellInView(rowStart, GetLeftCol());
}

HaunchInputData CHaunchGirderGrid::GetData(Float64 minA, CString& minValError, const HaunchInputData& origData, CDataExchange* pDX)
{
   HaunchInputData data;
   data.m_SlabOffsetType = pgsTypes::sotGirder;

   // Number of spans and girders cannot change so use data structure to iterate into grid
   SpanIndexType span = 0;
   std::vector<HaunchPairVec>::const_iterator spanIt = origData.m_SpanGirdersHaunch.begin();
   for (; spanIt!=origData.m_SpanGirdersHaunch.end(); spanIt++)
   {
      GirderIndexType gdr = 0;
      HaunchPairVec newGdrVec;
      HaunchPair newPair(0,0);
      const HaunchPairVec& rhpv = *spanIt;
      for (HaunchPairVecConstIter gdrIt = rhpv.begin(); gdrIt!=rhpv.end(); gdrIt++)
      {

         // Extract converted doubles from grid
         newPair = GetHpAtCells( span, gdr, pDX);

         if (newPair.first < minA)
         {
            AfxMessageBox( minValError, MB_ICONEXCLAMATION);
            this->SetCurrentCell((ROWCOL)gdr+1,1,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
            pDX->Fail();
         }

         if (newPair.second < minA)
         {
            AfxMessageBox( minValError, MB_ICONEXCLAMATION);
            this->SetCurrentCell((ROWCOL)gdr+1,1,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
            pDX->Fail();
         }

         newGdrVec.push_back(newPair);
         gdr++;
      }

      data.m_SpanGirdersHaunch.push_back(newGdrVec);
      span++;
   }

   return data;
}

HaunchPair CHaunchGirderGrid::GetHpAtCells(SpanIndexType span, GirderIndexType gdr, CDataExchange* pDX)
{
   HaunchPair hp(0.0,0.0);

   ROWCOL col = (ROWCOL)span*2 + 1;
   ROWCOL row = (ROWCOL)gdr+2;

   CString str = GetCellValue(row,col);
   if (!str.IsEmpty())
   {
      Float64 val;
      if(sysTokenizer::ParseDouble(str, &val))
      {
         hp.first = ::ConvertToSysUnits(val, m_pCompUnit->UnitOfMeasure);
      }
      else
      {
         AfxMessageBox( _T("Start value is not a number - must be a positive number"), MB_ICONEXCLAMATION);
         this->SetCurrentCell(row,col,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
         pDX->Fail();
      }
   }

   str = GetCellValue(row,col+1);
   if (!str.IsEmpty())
   {
      Float64 val;
      if(sysTokenizer::ParseDouble(str, &val))
      {
         hp.second = ::ConvertToSysUnits(val, m_pCompUnit->UnitOfMeasure);
      }
      else
      {
         AfxMessageBox( _T("End value is not a number - must be a positive number"), MB_ICONEXCLAMATION);
         this->SetCurrentCell(row,col,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
         pDX->Fail();
      }
   }

   return hp;
}

CString CHaunchGirderGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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