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

// AssExcessCamberGirderGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "AssExcessCamberGirderGrid.h"

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

GRID_IMPLEMENT_REGISTER(CAssExcessCamberGirderGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CAssExcessCamberGirderGrid

CAssExcessCamberGirderGrid::CAssExcessCamberGirderGrid()
{
//   RegisterClass();
}

CAssExcessCamberGirderGrid::~CAssExcessCamberGirderGrid()
{
}

BEGIN_MESSAGE_MAP(CAssExcessCamberGirderGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CAssExcessCamberGirderGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAssExcessCamberGirderGrid message handlers

int CAssExcessCamberGirderGrid::GetColWidth(ROWCOL nCol)
{
   if ( IsColHidden(nCol) )
      return CGXGridWnd::GetColWidth(nCol);

	CRect rect = GetGridRect( );

   return (int)(rect.Width( )*(Float64)1/6);
}

void CAssExcessCamberGirderGrid::CustomInit(SpanIndexType nSpans, GirderIndexType maxGirdersPerSpan)
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

   // Create entire grid with read-only place holders for inputs. Fill grid will change types to writeable later
   const int num_rows = (int)maxGirdersPerSpan;
   const int num_cols = (int)nSpans;

	SetRowCount(num_rows);
	SetColCount(num_cols);

   // Turn off selecting 
	GetParam()->EnableSelection((WORD) (~GX_SELROW & ~GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   ROWCOL col = 1;

   for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++)
   {
      CString strLabel;
      strLabel.Format(_T("Span %d\n(%s)"), LABEL_SPAN(spanIdx),  m_pCompUnit->UnitOfMeasure.UnitTag().c_str());

	   SetStyleRange(CGXRange(0,col++), CGXStyle()
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_TOP)
			   .SetEnabled(FALSE)          // disables usage as current cell
			   .SetValue(strLabel)
		   );
   }

// Set up rows as read only
   ROWCOL row = 1;
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

void CAssExcessCamberGirderGrid::FillGrid(const HaunchInputData& haunchData)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);
 
   // Grid was already sized in CustomInit. Just fill and enable data locations
   ROWCOL rowStart = 1;
   ROWCOL col = 1;

   AssExcessCamberSpanDataConstIter spanIt = haunchData.m_AssExcessCamberSpans.begin();
   for (; spanIt!=haunchData.m_AssExcessCamberSpans.end(); spanIt++)
   {
      ROWCOL row = rowStart;
      const AssExcessCamberSpanData& hpv = *spanIt;
      for (std::vector<Float64>::const_iterator gdrIt = hpv.m_AssExcessCambersForGirders.begin(); gdrIt!=hpv.m_AssExcessCambersForGirders.end(); gdrIt++)
      {
         Float64 AssExcessCamber = *gdrIt;

         SetStyleRange(CGXRange(row,col), CGXStyle()
            .SetReadOnly(FALSE)
            .SetEnabled(TRUE)
            .SetInterior(::GetSysColor(COLOR_WINDOW))
            .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
            .SetValue(FormatDimension(AssExcessCamber,*m_pCompUnit, false))
            );

         row++;
      }

      col ++;
   }

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);

	ScrollCellInView(rowStart, GetLeftCol());
}

void CAssExcessCamberGirderGrid::GetData(HaunchInputData* pData, CDataExchange* pDX)
{
   pData->m_AssExcessCamberType = pgsTypes::aecGirder;

   // Number of spans and girders cannot change so use original data structure to iterate into grid
   SpanIndexType span = 0;
   AssExcessCamberSpanDataIter spanIt = pData->m_AssExcessCamberSpans.begin();
   for (; spanIt!=pData->m_AssExcessCamberSpans.end(); spanIt++)
   {
      GirderIndexType gdr = 0;
      AssExcessCamberSpanData& rhpv = *spanIt;
      for (std::vector<Float64>::iterator gdrIt = rhpv.m_AssExcessCambersForGirders.begin(); gdrIt!=rhpv.m_AssExcessCambersForGirders.end(); gdrIt++)
      {
         Float64* pVal = &(*gdrIt);

         // Extract converted value from grid
         *pVal = GetAssExcessCamberAtCells( span, gdr, pDX);

         gdr++;
      }

      span++;
   }
}

Float64 CAssExcessCamberGirderGrid::GetAssExcessCamberAtCells(SpanIndexType span, GirderIndexType gdr, CDataExchange* pDX)
{
   Float64 val;

   ROWCOL col = (ROWCOL)span + 1;
   ROWCOL row = (ROWCOL)gdr+1;

   CString str = GetCellValue(row,col);
   if (!str.IsEmpty())
   {
      Float64 val2;
      if(sysTokenizer::ParseDouble(str, &val2))
      {
         val = ::ConvertToSysUnits(val2, m_pCompUnit->UnitOfMeasure);
      }
      else
      {
         CString str;
         str.Format(_T("Assumed Excess Camber value at Span %s, Girder %s is not a number - must be a positive number"), LABEL_SPAN(span), LABEL_GIRDER(gdr));
         AfxMessageBox( str , MB_ICONEXCLAMATION);
         this->SetCurrentCell(row,col,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
         pDX->Fail();
      }
   }

   return val;
}

CString CAssExcessCamberGirderGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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