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

// AssumedExcessCamberSpanGrid.cpp : implementation file
//

#include "stdafx.h"
#include "AssumedExcessCamberSpanGrid.h"
#include "EditHaunchACamberDlg.h"

#include <System\Tokenizer.h>
#include "PGSuperUnits.h"
#include <Units\Measure.h>
#include <EAF\EAFDisplayUnits.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CAssumedExcessCamberSpanGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CAssumedExcessCamberSpanGrid

CAssumedExcessCamberSpanGrid::CAssumedExcessCamberSpanGrid()
{
//   RegisterClass();
}

CAssumedExcessCamberSpanGrid::~CAssumedExcessCamberSpanGrid()
{
}

BEGIN_MESSAGE_MAP(CAssumedExcessCamberSpanGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CAssumedExcessCamberSpanGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAssumedExcessCamberSpanGrid message handlers

int CAssumedExcessCamberSpanGrid::GetColWidth(ROWCOL nCol)
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

void CAssumedExcessCamberSpanGrid::CustomInit()
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
   strval.Format(_T("Assumed Excess Camber\n(%s)"), m_pUnit->UnitOfMeasure.UnitTag().c_str());

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

void CAssumedExcessCamberSpanGrid::SetRowStyle(ROWCOL nRow)
{

	GetParam()->EnableUndo(FALSE);

	SetStyleRange(CGXRange(nRow,1), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
		);

	GetParam()->EnableUndo(TRUE);
}

CString CAssumedExcessCamberSpanGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CAssumedExcessCamberSpanGrid::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   if (pDX->m_bSaveAndValidate)
   {
      CEditHaunchACamberDlg* pParent = (CEditHaunchACamberDlg*)(GetParent()->GetParent());
      if (pParent->GetAssumedExcessCamberType() == pgsTypes::aecSpan)
      {
         GetGridData(pDX);
      }
   }
   else
   {
      FillGrid();
   }
}

void CAssumedExcessCamberSpanGrid::FillGrid()
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);
 
   // remove all but top row
   ROWCOL rows = GetRowCount();
   if (0 < rows)
   {
      RemoveRows(1, rows);
   }

   ROWCOL row = 1;
   CEditHaunchACamberDlg* pParent = (CEditHaunchACamberDlg*)(GetParent()->GetParent());
   CBridgeDescription2* pBridge = pParent->GetBridgeDesc();

   SpanIndexType nSpans = pBridge->GetSpanCount();
   for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++, row++)
   {
      auto* pSpan = pBridge->GetSpan(spanIdx);
      auto assumedExcessCamber = pSpan->GetAssumedExcessCamber(0);

      InsertRows(row,1);
      SetRowStyle(row);

      CString strLabel;
      strLabel.Format(_T("Span %s"), LABEL_SPAN(spanIdx));
      SetStyleRange(CGXRange(row, 0), CGXStyle().SetValue(strLabel).SetItemDataPtr((void*)spanIdx));
      SetStyleRange(CGXRange(row, 1), CGXStyle()
         .SetReadOnly(FALSE)
         .SetEnabled(TRUE)
         .SetValue(FormatDimension(assumedExcessCamber,*m_pUnit, false))
         );
   }

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);

	ScrollCellInView(1, GetLeftCol());
}

void CAssumedExcessCamberSpanGrid::GetGridData(CDataExchange* pDX)
{
   CEditHaunchACamberDlg* pParent = (CEditHaunchACamberDlg*)(GetParent()->GetParent());
   CBridgeDescription2* pBridge = pParent->GetBridgeDesc();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2_NOCHECK(pBroker, IEAFDisplayUnits, pDisplayUnits);

   ROWCOL nRows = GetRowCount();
   for (ROWCOL row = 1; row <= nRows; row++)
   {
      CString strValue = GetCellValue(row, 1);
      Float64 value;
      if (strValue.IsEmpty() || !WBFL::System::Tokenizer::ParseDouble(strValue, &value))
      {
         AfxMessageBox( _T("Assumed Excess Camber value is not a number - must be a number zero or greater"), MB_ICONERROR | MB_OK);
         SetCurrentCell(row,1,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
         pDX->Fail();
      }
      else
      {
         Float64 assumedExcessCamber = WBFL::Units::ConvertToSysUnits(value, m_pUnit->UnitOfMeasure);

         CGXStyle style;
         GetStyleRowCol(row, 0, style);
         SpanIndexType spanIdx = (SpanIndexType)(style.GetItemDataPtr());

         auto* pSpan = pBridge->GetSpan(spanIdx);
         pSpan->SetAssumedExcessCamber(assumedExcessCamber);
      }
   }
}
