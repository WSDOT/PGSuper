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

// HaunchDirectEntireBridgeGrid.cpp : implementation file
//

#include "stdafx.h"
#include "HaunchDirectEntireBridgeGrid.h"
#include "EditHaunchByHaunchDlg.h"

#include <System\Tokenizer.h>
#include "PGSuperUnits.h"
#include "PGSuperDoc.h"
#include <Units\Measure.h>
#include <EAF\EAFDisplayUnits.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\HaunchDepthInputConversionTool.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CHaunchDirectEntireBridgeGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CHaunchDirectEntireBridgeGrid

CHaunchDirectEntireBridgeGrid::CHaunchDirectEntireBridgeGrid()
{
//   RegisterClass();
}

CHaunchDirectEntireBridgeGrid::~CHaunchDirectEntireBridgeGrid()
{
}

BEGIN_MESSAGE_MAP(CHaunchDirectEntireBridgeGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CHaunchDirectEntireBridgeGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
   ON_WM_DESTROY()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CHaunchDirectEntireBridgeGrid message handlers
CBridgeDescription2* CHaunchDirectEntireBridgeGrid::GetBridgeDesc()
{
   CWnd* pPapa;
   pPapa = GetParent();

   CEditHaunchByHaunchDlg* pParent = dynamic_cast<CEditHaunchByHaunchDlg*>(pPapa);
   return pParent->GetBridgeDesc();
}

void CHaunchDirectEntireBridgeGrid::CustomInit()
{
   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
   Initialize();

   // Don't allow pasting to increase grid size
   m_nClipboardFlags |= GX_DNDNOAPPENDCOLS | GX_DNDNOAPPENDROWS;

   EnableIntelliMouse();
}

void CHaunchDirectEntireBridgeGrid::InvalidateGrid()
{
   ClearCells(CGXRange(0,0,GetRowCount(),GetColCount()));
   BuildGridAndHeader();
   FillGrid();
}

int CHaunchDirectEntireBridgeGrid::GetColWidth(ROWCOL nCol)
{
   if (IsColHidden(nCol))
      return CGXGridWnd::GetColWidth(nCol);

   CRect rect = GetGridRect();

   pgsTypes::HaunchInputDistributionType disttype = GetHaunchInputDistributionType();
   if (disttype == pgsTypes::hidUniform || nCol == 0)
   {
      return rect.Width() / 10;
   }
   else
   {
      return rect.Width() / 15;
   }
}

void CHaunchDirectEntireBridgeGrid::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   if (pDX->m_bSaveAndValidate)
   {
      GetGridData(pDX);
   }
}

void CHaunchDirectEntireBridgeGrid::BuildGridAndHeader()
{
   CEAFDocument* pDoc = EAFGetDocument();
   BOOL bIsPGSuper = pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc));

   GetParam()->EnableUndo(FALSE);

   // we want to merge cells
   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   CBridgeDescription2* pBridge = GetBridgeDesc();
   SegmentIndexType nSpans = pBridge->GetSpanCount();

   pgsTypes::HaunchLayoutType layoutType = GetHaunchLayoutType();
   pgsTypes::HaunchInputDistributionType disttype = GetHaunchInputDistributionType();

   ROWCOL nRows = 1;
   ROWCOL nCols = (ROWCOL)(disttype);

   SetRowCount(nRows);
   SetColCount(nCols);

   // Turn off row, column, and whole table selection
   GetParam()->EnableSelection((WORD)(GX_SELFULL & ~GX_SELCOL & ~GX_SELROW & ~GX_SELTABLE));

   // no row moving
   GetParam()->EnableMoveRows(FALSE);

   m_nExtraHeaderRows = (disttype == pgsTypes::hidUniform) ? 0 : 1;
   SetFrozenRows(nRows/*# frozen rows*/,m_nExtraHeaderRows/*# extra header rows*/);

   ROWCOL col = 1;
   CString strLabel = (layoutType == pgsTypes::hltAlongSpans) ? _T("All Spans") : _T("All Segments");
   SetStyleRange(CGXRange(0,col,0,col + disttype - 1),CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
      .SetEnabled(FALSE)
      .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
      .SetValue(strLabel)
   );

   if (pgsTypes::hidUniform != disttype)
   {
      for (int fracno = 0; fracno < disttype; fracno++)
      {
         CString strLabel;
         strLabel.Format(_T("%.2f"),(float)fracno / (disttype - 1));
         if (strLabel.GetAt(3) == '0') // trim excess zeroes
         {
            strLabel.Truncate(3);
         }

         SetStyleRange(CGXRange(1,col),CGXStyle()
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_TOP)
            .SetEnabled(FALSE)
            .SetInterior(::GetSysColor(COLOR_BTNFACE))
            .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
            .SetValue(strLabel)
         );

         col++;
      }
   }

   // make text fit correctly
   ResizeRowHeightsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));

   // don't allow users to resize grids
   GetParam()->EnableTrackColWidth(0);
   GetParam()->EnableTrackRowHeight(0);

   SetFocus();

   GetParam()->EnableUndo(TRUE);
}

void CHaunchDirectEntireBridgeGrid::FillGrid()
{
   CComPtr<IBroker> pBroker;
   ::EAFGetBroker(&pBroker);

   CEAFDocument* pDoc = EAFGetDocument();
   BOOL bIsPGSuper = pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc));

   pgsTypes::HaunchLayoutType layoutType = GetHaunchLayoutType();
   pgsTypes::HaunchInputDistributionType disttype = GetHaunchInputDistributionType();

   const CBridgeDescription2* pBridgeDescOrig = GetBridgeDesc();

   // Convert current haunch data if needed
   HaunchDepthInputConversionTool conversionTool(pBridgeDescOrig, pBroker,false);
   auto convPair = conversionTool.ConvertToDirectHaunchInput(pgsTypes::hilSame4Bridge,layoutType,disttype);
   const CBridgeDescription2* pBridgeDesc = &convPair.second;

   std::vector<Float64> haunchDepths = pBridgeDesc->GetDirectHaunchDepths();

   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   ROWCOL nRows = GetRowCount();
   if (m_nExtraHeaderRows + 1 < nRows)
   {
      RemoveRows(m_nExtraHeaderRows + 1,nRows);
   }

   ROWCOL row = m_nExtraHeaderRows + 1;
   ROWCOL col = 1;

   if (pgsTypes::hidUniform != disttype)
   {
      InsertRows(row,1);
      SetInitialRowStyle(row);

      // top left corner
      SetStyleRange(CGXRange(0,0,2,0),CGXStyle().SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue(_T("Girder")));
   }

   CString strLabel(_T("All Girders"));
   SetStyleRange(CGXRange(row,0),CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetControl(GX_IDS_CTRL_HEADER)
      .SetFont(CGXFont().SetBold(TRUE))
      .SetValue(strLabel));

   CEditHaunchByHaunchDlg* pParent = (CEditHaunchByHaunchDlg*)GetParent();

   for (auto haunch : haunchDepths)
   {
      // Function below adds deck thickness if needed and converts units
      CString cellStr = pParent->ConvertValueToGridString(haunch);

      SetStyleRange(CGXRange(row,col),CGXStyle()
         .SetReadOnly(FALSE)
         .SetEnabled(TRUE)
         .SetHorizontalAlignment(DT_RIGHT)
         .SetInterior(::GetSysColor(COLOR_WINDOW))
         .SetValue(cellStr));

      col++;
   }

   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(TRUE);
   ScrollCellInView(1,GetLeftCol());
}

void CHaunchDirectEntireBridgeGrid::GetGridData(CDataExchange* pDX)
{
   CBridgeDescription2* pBridge = GetBridgeDesc();
   CEditHaunchByHaunchDlg* pParent = (CEditHaunchByHaunchDlg*)GetParent();

   pgsTypes::HaunchLayoutType layoutType = GetHaunchLayoutType();
   pgsTypes::HaunchInputDistributionType disttype = GetHaunchInputDistributionType();

   ROWCOL row = m_nExtraHeaderRows + 1;
   ROWCOL col = 1;
   std::vector<Float64> haunches;
   for (int i = 0; i < disttype; i++,col++)
   {
      CString strValue = GetCellValue(row,col);

      // Function below converts units and subtracts deck thickness from input value if needed
      Float64 value = pParent->GetValueFromGrid(strValue, pDX, row, col, this);

      haunches.push_back(value);
   }

   ATLASSERT(pBridge->GetHaunchInputDepthType() == pgsTypes::hidHaunchDirectly || pBridge->GetHaunchInputDepthType() == pgsTypes::hidHaunchPlusSlabDirectly);
   pBridge->SetHaunchInputLocationType(pgsTypes::hilSame4Bridge);
   pBridge->SetHaunchInputDistributionType(disttype);
   pBridge->SetHaunchLayoutType(layoutType);
   pBridge->SetDirectHaunchDepths(haunches);

   // Call below is necessary whenever changes are made and hilSame4Bridge is used
   pBridge->CopyDown(false,false,false,false,false,true,false);
}

CString CHaunchDirectEntireBridgeGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CHaunchDirectEntireBridgeGrid::SetInitialRowStyle(ROWCOL row)
{
   ROWCOL nCols = GetColCount();
   SetStyleRange(CGXRange(row, 1, row, nCols), CGXStyle()
      .SetReadOnly(TRUE)
      .SetEnabled(FALSE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
   );
}

void CHaunchDirectEntireBridgeGrid::OnDestroy()
{
   CGXGridWnd::OnDestroy();
   delete this;
}

pgsTypes::HaunchInputDistributionType CHaunchDirectEntireBridgeGrid::GetHaunchInputDistributionType()
{
   CEditHaunchByHaunchDlg* pParent = (CEditHaunchByHaunchDlg*)GetParent();
   pgsTypes::HaunchInputDistributionType disttype = pParent->GetHaunchInputDistributionType();
   return disttype;
}

pgsTypes::HaunchLayoutType CHaunchDirectEntireBridgeGrid::GetHaunchLayoutType()
{
   CEditHaunchByHaunchDlg* pGrandPa = (CEditHaunchByHaunchDlg*)GetParent();
   pgsTypes::HaunchLayoutType type = pGrandPa->GetHaunchLayoutType();
   return type;
}
