///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// AssumedExcessCamberGirderGrid.cpp : implementation file
//

#include "stdafx.h"
#include "AssumedExcessCamberGirderGrid.h"
#include "EditHaunchACamberDlg.h"

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

GRID_IMPLEMENT_REGISTER(CAssumedExcessCamberGirderGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CAssumedExcessCamberGirderGrid

CAssumedExcessCamberGirderGrid::CAssumedExcessCamberGirderGrid()
{
//   RegisterClass();
}

CAssumedExcessCamberGirderGrid::~CAssumedExcessCamberGirderGrid()
{
}

BEGIN_MESSAGE_MAP(CAssumedExcessCamberGirderGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CAssumedExcessCamberGirderGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAssumedExcessCamberGirderGrid message handlers

int CAssumedExcessCamberGirderGrid::GetColWidth(ROWCOL nCol)
{
   if ( IsColHidden(nCol) )
      return CGXGridWnd::GetColWidth(nCol);

	CRect rect = GetGridRect( );

   return (int)(rect.Width( )*(Float64)1/6);
}

void CAssumedExcessCamberGirderGrid::CustomInit()
{
   // initialize units
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   m_pUnit = &(pDisplayUnits->GetComponentDimUnit());

// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );

	GetParam( )->EnableUndo(FALSE);

   // Create entire grid with read-only place holders for inputs. Fill grid will change types to writeable later
   CEditHaunchACamberDlg* pParent = (CEditHaunchACamberDlg*)GetParent()->GetParent();
   SpanIndexType nSpans = pParent->GetBridgeDesc()->GetSpanCount();

   const int num_rows = 0;
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
      strLabel.Format(_T("Span %s\n(%s)"), LABEL_SPAN(spanIdx),  m_pUnit->UnitOfMeasure.UnitTag().c_str());

	   SetStyleRange(CGXRange(0,col++), CGXStyle()
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_TOP)
			   .SetEnabled(FALSE)          // disables usage as current cell
			   .SetValue(strLabel)
		   );
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

void CAssumedExcessCamberGirderGrid::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   if (pDX->m_bSaveAndValidate)
   {
      CEditHaunchACamberDlg* pParent = (CEditHaunchACamberDlg*)GetParent()->GetParent();
      if (pParent->GetAssumedExcessCamberType() == pgsTypes::aecGirder)
      {
         GetGridData(pDX);
      }
   }
   else
   {
      FillGrid();
   }
}

void CAssumedExcessCamberGirderGrid::FillGrid()
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   ROWCOL nRows = GetRowCount();
   if (0 < nRows)
   {
      RemoveRows(1, nRows);
   }

   CEditHaunchACamberDlg* pParent = (CEditHaunchACamberDlg*)GetParent()->GetParent();
   CBridgeDescription2* pBridge = pParent->GetBridgeDesc();
   long nMaxGirders = -1;
   SpanIndexType nSpans = pBridge->GetSpanCount();
   for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++)
   {
      ROWCOL col = (ROWCOL)(spanIdx + 1);

      auto* pSpan = pBridge->GetSpan(spanIdx);
      auto nGirders = pSpan->GetGirderCount();
      for (auto gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         ROWCOL row = (ROWCOL)(gdrIdx+1);
         if (spanIdx == 0 || nMaxGirders <= gdrIdx)
         {
            InsertRows(row, 1);
            SetInitialRowStyle(row);

            CString strLabel;
            strLabel.Format(_T("Girder %s"), LABEL_GIRDER(gdrIdx));
            SetStyleRange(CGXRange(row, 0), CGXStyle().SetValue(strLabel));
         }

         Float64 assumedExcessCamber = pSpan->GetAssumedExcessCamber(gdrIdx);
         SetStyleRange(CGXRange(row, col), CGXStyle()
            .SetReadOnly(FALSE)
            .SetEnabled(TRUE)
            .SetHorizontalAlignment(DT_RIGHT)
            .SetInterior(::GetSysColor(COLOR_WINDOW))
            .SetValue(FormatDimension(assumedExcessCamber, *m_pUnit, false)));
      } // next girder

      nMaxGirders = Max(nMaxGirders, (long)nGirders);
   } // next span

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);

	ScrollCellInView(1, GetLeftCol());
}

void CAssumedExcessCamberGirderGrid::GetGridData(CDataExchange* pDX)
{
   CEditHaunchACamberDlg* pParent = (CEditHaunchACamberDlg*)GetParent()->GetParent();
   CBridgeDescription2* pBridge = pParent->GetBridgeDesc();

   SpanIndexType nSpans = pBridge->GetSpanCount();
   for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++)
   {
      ROWCOL col = (ROWCOL)(spanIdx + 1);

      auto* pSpan = pBridge->GetSpan(spanIdx);
      auto nGirders = pSpan->GetGirderCount();
      for (auto gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         ROWCOL row = (ROWCOL)(gdrIdx + 1);

         CString strValue = GetCellValue(row, col);
         Float64 value;
         if (strValue.IsEmpty() || !WBFL::System::Tokenizer::ParseDouble(strValue, &value))
         {
            CString strError;
            strError.Format(_T("Assumed Excess Camber value at Span %s, Girder %s is not a number - must be a positive number"), LABEL_SPAN(spanIdx), LABEL_GIRDER(gdrIdx));
            AfxMessageBox(strError, MB_ICONERROR | MB_OK);
            SetCurrentCell(row,col,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
            pDX->Fail();
         }
         else
         {
            Float64 assumedExcessCamber = WBFL::Units::ConvertToSysUnits(value, m_pUnit->UnitOfMeasure);
            pSpan->SetAssumedExcessCamber(gdrIdx,assumedExcessCamber);
         }
      } // next girder
   } // next span
}

CString CAssumedExcessCamberGirderGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CAssumedExcessCamberGirderGrid::SetInitialRowStyle(ROWCOL row)
{
   ROWCOL nCols = GetColCount();
   SetStyleRange(CGXRange(row, 1, row, nCols), CGXStyle()
      .SetReadOnly(TRUE)
      .SetEnabled(FALSE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
   );
}
