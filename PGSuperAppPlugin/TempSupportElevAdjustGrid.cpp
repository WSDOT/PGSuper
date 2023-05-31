///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

// TempSupportElevAdjustGrid.cpp : implementation file
//

#include "stdafx.h"
#include "TempSupportElevAdjustGrid.h"
#include "EditHaunchDlg.h"

#include "PGSuperUnits.h"
#include <Units\Measure.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CTempSupportElevAdjustGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CTempSupportElevAdjustGrid

CTempSupportElevAdjustGrid::CTempSupportElevAdjustGrid()
{
//   RegisterClass();
}

CTempSupportElevAdjustGrid::~CTempSupportElevAdjustGrid()
{
}

BEGIN_MESSAGE_MAP(CTempSupportElevAdjustGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CTempSupportElevAdjustGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
   ON_WM_DESTROY()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTempSupportElevAdjustGrid message handlers
CBridgeDescription2* CTempSupportElevAdjustGrid::GetBridgeDesc()
{
   return &(m_pParent->m_BridgeDesc);
}

void CTempSupportElevAdjustGrid::CustomInit(GroupIndexType group, CEditHaunchDlg* pParent)
{
   m_Group = group;
   m_pParent = pParent;

   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
   Initialize();

   // Don't allow pasting to increase grid size
   m_nClipboardFlags |= GX_DNDNOAPPENDCOLS | GX_DNDNOAPPENDROWS;

   BuildGridAndHeader();

   EnableIntelliMouse();
}

int CTempSupportElevAdjustGrid::GetColWidth(ROWCOL nCol)
{
   if (IsColHidden(nCol))
      return CGXGridWnd::GetColWidth(nCol);

   CRect rect = GetGridRect();

   if (nCol == 0)
   {
      return rect.Width() * 2/ 6;
   }
   else
   {
      return rect.Width() / 6;
   }

}

void CTempSupportElevAdjustGrid::BuildGridAndHeader()
{
   CComPtr<IBroker> pBroker;
   ::EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   // build list of temporary supports that can be adjusted
   m_ActiveTempSupports.clear();
   const CBridgeDescription2* pBridge = GetBridgeDesc();
   PierIndexType nTS = pBridge->GetTemporarySupportCount();
   for (PierIndexType tsIdx = 0; tsIdx < nTS; tsIdx++)
   {
      const CTemporarySupportData* pTs = pBridge->GetTemporarySupport(tsIdx);
      if (pTs->HasElevationAdjustment())
      {
         m_ActiveTempSupports.push_back(tsIdx);
      }
   }

   SetRowCount(1);
   SetColCount((ROWCOL)m_ActiveTempSupports.size());

   // Turn off row, column, and whole table selection
   GetParam()->EnableSelection((WORD)(GX_SELFULL & ~GX_SELCOL & ~GX_SELROW & ~GX_SELTABLE));

   // no row moving
   GetParam()->EnableMoveRows(FALSE);
   GetParam()->EnableMoveCols(FALSE);

   CString strLabel(_T("Temp Support"));
   SetStyleRange(CGXRange(0,0),CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetValue(strLabel)
   );

   const WBFL::Units::LengthData& lengthUnit = pDisplayUnits->GetComponentDimUnit();

   strLabel.Format(_T("Adjustment (%s)"), lengthUnit.UnitOfMeasure.UnitTag().c_str());
   SetStyleRange(CGXRange(1,0),CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
      .SetValue(strLabel)
   );

   ROWCOL col = 1;
   for (auto index : m_ActiveTempSupports)
   {
      const CTemporarySupportData* pTs = pBridge->GetTemporarySupport(index);
      if (pTs->GetSupportType() == pgsTypes::ErectionTower)
      {
         strLabel.Format(_T("ET %d"), index+1);
      }
      else
      {
         strLabel.Format(_T("SB %d"),index + 1);
      }

      SetStyleRange(CGXRange(0,col),CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
         .SetEnabled(FALSE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetValue(strLabel)
      );

      Float64 offset = pTs->GetElevationAdjustment();

      SetStyleRange(CGXRange(1,col),CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         .SetVerticalAlignment(DT_TOP)
         .SetValue(FormatDimension(offset,lengthUnit,false))
      );

      col++;
   }

   // make text fit correctly
   ResizeRowHeightsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));

   // don't allow users to resize grids
   GetParam()->EnableTrackColWidth(0);
   GetParam()->EnableTrackRowHeight(0);

   ScrollCellInView(1,GetLeftCol());
   SetFocus();

   GetParam()->EnableUndo(TRUE);
   GetParam()->SetLockReadOnly(TRUE);
}

void CTempSupportElevAdjustGrid::GetGridData(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   ::EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CBridgeDescription2* pBridge = GetBridgeDesc();
   ROWCOL col = 1;
   for (auto index : m_ActiveTempSupports)
   {
      Float64 value = _tstof(GetCellValue(1,col++));

      value = WBFL::Units::ConvertToSysUnits(value,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

      CTemporarySupportData* pTs = pBridge->GetTemporarySupport(index);
      pTs->SetElevationAdjustment(value);
   }
}

CString CTempSupportElevAdjustGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CTempSupportElevAdjustGrid::SetInitialRowStyle(ROWCOL row)
{
   ROWCOL nCols = GetColCount();
   SetStyleRange(CGXRange(row, 1, row, nCols), CGXStyle()
      .SetReadOnly(TRUE)
      .SetEnabled(FALSE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
   );
}

void CTempSupportElevAdjustGrid::OnDestroy()
{
   CGXGridWnd::OnDestroy();
   delete this;
}