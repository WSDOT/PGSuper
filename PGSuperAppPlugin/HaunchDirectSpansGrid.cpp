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

// HaunchDirectSpansGrid.cpp : implementation file
//

#include "stdafx.h"
#include "HaunchDirectSpansGrid.h"
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

GRID_IMPLEMENT_REGISTER(CHaunchDirectSpansGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CHaunchDirectSpansGrid

CHaunchDirectSpansGrid::CHaunchDirectSpansGrid()
{
//   RegisterClass();
}

CHaunchDirectSpansGrid::~CHaunchDirectSpansGrid()
{
}

BEGIN_MESSAGE_MAP(CHaunchDirectSpansGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CHaunchDirectSpansGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
   ON_WM_DESTROY()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CHaunchDirectSpansGrid message handlers
CBridgeDescription2* CHaunchDirectSpansGrid::GetBridgeDesc()
{
   CWnd* pPapa;
   pPapa = GetParent();
   CEditHaunchByHaunchDlg* pParent = dynamic_cast<CEditHaunchByHaunchDlg*>(pPapa);
   return pParent->GetBridgeDesc();
}

void CHaunchDirectSpansGrid::CustomInit()
{
   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
   Initialize();

   // Don't allow pasting to increase grid size
   m_nClipboardFlags |= GX_DNDNOAPPENDCOLS | GX_DNDNOAPPENDROWS;

   EnableIntelliMouse();
}

void CHaunchDirectSpansGrid::InvalidateGrid()
{
   ClearCells(CGXRange(0,0,GetRowCount(),GetColCount()));
   BuildGridAndHeader();
   FillGrid();
}

int CHaunchDirectSpansGrid::GetColWidth(ROWCOL nCol)
{
   if (IsColHidden(nCol))
      return CGXGridWnd::GetColWidth(nCol);

   CRect rect = GetGridRect();

   pgsTypes::HaunchInputDistributionType disttype = GetHaunchInputDistributionType();
   if (disttype != pgsTypes::hidUniform)
   {
      return rect.Width() / 20;
   }
   else
   {
      return rect.Width() / 15;
   }
}

void CHaunchDirectSpansGrid::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   if (pDX->m_bSaveAndValidate)
   {
      GetGridData(pDX);
   }
}

void CHaunchDirectSpansGrid::BuildGridAndHeader()
{
   CEAFDocument* pDoc = EAFGetDocument();
   BOOL bIsPGSuper = pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc));

   GetParam()->EnableUndo(FALSE);

   // we want to merge cells
   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   CBridgeDescription2* pBridge = GetBridgeDesc();

   SpanIndexType nSpans = pBridge->GetSpanCount();
   GirderIndexType maxGdrs = 0;
   for (SpanIndexType ispan = 0; ispan < nSpans; ispan++)
   {
      maxGdrs = max(maxGdrs,pBridge->GetSpan(ispan)->GetGirderCount());
   }

   pgsTypes::HaunchInputDistributionType disttype = GetHaunchInputDistributionType();

   ROWCOL nRows = (disttype == pgsTypes::hidUniform) ? (ROWCOL)maxGdrs : (ROWCOL)maxGdrs + 1;
   ROWCOL nCols = (disttype == pgsTypes::hidUniform) ? (ROWCOL)(nSpans) : (ROWCOL)(nSpans * disttype);

   SetRowCount(nRows);
   SetColCount(nCols);

   // Turn off row, column, and whole table selection
   GetParam()->EnableSelection((WORD)(GX_SELFULL & ~GX_SELCOL & ~GX_SELROW & ~GX_SELTABLE));
   // no row moving
   GetParam()->EnableMoveRows(FALSE);

   if (disttype == pgsTypes::hidUniform)
   {
      m_nExtraHeaderRows = 0;
      SetFrozenRows(nRows/*# frozen rows*/,m_nExtraHeaderRows/*# extra header rows*/);

      ROWCOL col = 1;
      for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++)
      {
         CString strLabel;
         strLabel.Format(_T("Span %s"),LABEL_SPAN(spanIdx));
         SetStyleRange(CGXRange(0,col,0,col),CGXStyle()
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_TOP)
            .SetEnabled(FALSE)
            .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
            .SetValue(strLabel)
         );

         col++;
      }
   }
   else
   {
      m_nExtraHeaderRows = 1;
      SetFrozenRows(nRows/*# frozen rows*/,m_nExtraHeaderRows/*# extra header rows*/);

      ROWCOL col = 1;
      for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++)
      {
         CString strLabel;
         strLabel.Format(_T("Span %s"),LABEL_SPAN(spanIdx));
         SetStyleRange(CGXRange(0,col,0,col + disttype - 1),CGXStyle()
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_TOP)
            .SetEnabled(FALSE)
            .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
            .SetValue(strLabel)
         );

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
      } // next span
   }

   // top left corner
   SetStyleRange(CGXRange(0,0,1,0),CGXStyle()
      .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
      .SetControl(GX_IDS_CTRL_HEADER)
      .SetValue(_T("Girder")));

   // make text fit correctly
   ResizeRowHeightsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));

   // don't allow users to resize grids
   GetParam()->EnableTrackColWidth(0);
   GetParam()->EnableTrackRowHeight(0);

   SetFocus();

   GetParam()->EnableUndo(TRUE);
}

void CHaunchDirectSpansGrid::FillGrid()
{
   CComPtr<IBroker> pBroker;
   ::EAFGetBroker(&pBroker);
   CEAFDocument* pDoc = EAFGetDocument();
   BOOL bIsPGSuper = pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc));

   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CBridgeDescription2* pBridgeOrig = GetBridgeDesc();
   SpanIndexType nSpans = pBridgeOrig->GetSpanCount();
   GirderIndexType maxGdrs = 0;
   for (SpanIndexType ispan = 0; ispan < nSpans; ispan++)
   {
      maxGdrs = max(maxGdrs,pBridgeOrig->GetSpan(ispan)->GetGirderCount());
   }

   pgsTypes::HaunchInputDistributionType disttype = GetHaunchInputDistributionType();

   for (auto gdrIdx = 0; gdrIdx < maxGdrs; gdrIdx++)
   {
      ROWCOL row = m_nExtraHeaderRows + gdrIdx + 1;

      SetInitialRowStyle(row);
      CString strLabel;
      strLabel.Format(_T("%s"),LABEL_GIRDER(gdrIdx));
      SetStyleRange(CGXRange(row,0),CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetFont(CGXFont().SetBold(TRUE))
         .SetValue(strLabel));
   }

   CEditHaunchByHaunchDlg* pParent = (CEditHaunchByHaunchDlg*)GetParent();

   // Convert current data if needed
   HaunchDepthInputConversionTool conversionTool(pBridgeOrig,pBroker,false);
   auto convPair = conversionTool.ConvertToDirectHaunchInput(pgsTypes::hilPerEach,pgsTypes::hltAlongSpans,disttype);
   const CBridgeDescription2* pBridge = &convPair.second;

   // Fill haunch values
   if (disttype == pgsTypes::hidUniform)
   {

      ROWCOL col = 1;
      for (SpanIndexType ispan = 0; ispan < nSpans; ispan++)
      {
         const CSpanData2* pSpan = pBridge->GetSpan(ispan);

         for (auto gdrIdx = 0; gdrIdx < maxGdrs; gdrIdx++)
         {
            ROWCOL row = m_nExtraHeaderRows + gdrIdx + 1;
            std::vector<Float64> haunches = pSpan->GetDirectHaunchDepths(gdrIdx);

            // Function below adds deck thickness if needed and converts units
            CString cellStr = pParent->ConvertValueToGridString(haunches.front());

            SetStyleRange(CGXRange(row,col),CGXStyle()
               .SetReadOnly(FALSE)
               .SetEnabled(TRUE)
               .SetHorizontalAlignment(DT_RIGHT)
               .SetInterior(::GetSysColor(COLOR_WINDOW))
               .SetValue(cellStr));
         }

         col++;
      } // next span
   }
   else
   {
      ROWCOL spanCol = 1;
      for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++)
      {
         auto* pSpan = pBridge->GetSpan(spanIdx);
         GirderIndexType nGdrs = pSpan->GetGirderCount();

         for (auto gdrIdx = 0; gdrIdx < nGdrs; gdrIdx++)
         {
            ROWCOL row = m_nExtraHeaderRows + gdrIdx + 1;
            ROWCOL col = spanCol;

            std::vector<Float64> haunches = pSpan->GetDirectHaunchDepths(gdrIdx);

            for (int idis = 0; idis < disttype; idis++)
            {
               Float64 haunch = haunches[idis];
               CString cellStr = pParent->ConvertValueToGridString(haunch);

               SetStyleRange(CGXRange(row,col),CGXStyle()
                  .SetReadOnly(FALSE)
                  .SetEnabled(TRUE)
                  .SetHorizontalAlignment(DT_RIGHT)
                  .SetInterior(::GetSysColor(COLOR_WINDOW))
                  .SetValue(cellStr));

               col++;
            }
         } // next girder

         spanCol += disttype;
      } // span
   }

   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(TRUE);
   ScrollCellInView(1,GetLeftCol());
}

void CHaunchDirectSpansGrid::GetGridData(CDataExchange* pDX)
{
   CEditHaunchByHaunchDlg* pParent = (CEditHaunchByHaunchDlg*)GetParent();

   pgsTypes::HaunchInputDistributionType disttype = GetHaunchInputDistributionType();

   CBridgeDescription2* pBridge = GetBridgeDesc();
   SpanIndexType nSpans = pBridge->GetSpanCount();

   ROWCOL spanCol = 1;
   std::vector<Float64> haunches((std::size_t)disttype,0.0); // fixed size vector
   for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++)
   {
      auto* pSpan = pBridge->GetSpan(spanIdx);
      GirderIndexType nGdrs = pSpan->GetGirderCount();

      for (auto gdrIdx = 0; gdrIdx < nGdrs; gdrIdx++)
      {
         ROWCOL row = m_nExtraHeaderRows + gdrIdx + 1;
         ROWCOL col = spanCol;

         for (int idis = 0; idis < disttype; idis++)
         {
            CString strValue = GetCellValue(row,col);

            // Convert units and subtracts deck thickness from input value if needed
            Float64 value = pParent->GetValueFromGrid(strValue,pDX,row,col,this);

            haunches[idis] = value;
            col++;
         }

         pSpan->SetDirectHaunchDepths(gdrIdx,haunches);
         row++;
      } // girder

      spanCol += disttype;
   } // span

   ATLASSERT(pBridge->GetHaunchInputDepthType() == pgsTypes::hidHaunchDirectly || pBridge->GetHaunchInputDepthType() == pgsTypes::hidHaunchPlusSlabDirectly);
   pBridge->SetHaunchInputLocationType(pgsTypes::hilPerEach);
   pBridge->SetHaunchInputDistributionType(disttype);
   pBridge->SetHaunchLayoutType(pgsTypes::hltAlongSpans);
}

CString CHaunchDirectSpansGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CHaunchDirectSpansGrid::SetInitialRowStyle(ROWCOL row)
{
   ROWCOL nCols = GetColCount();
   SetStyleRange(CGXRange(row, 1, row, nCols), CGXStyle()
      .SetReadOnly(TRUE)
      .SetEnabled(FALSE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
   );
}

void CHaunchDirectSpansGrid::OnDestroy()
{
   CGXGridWnd::OnDestroy();
   delete this;
}

pgsTypes::HaunchInputDistributionType CHaunchDirectSpansGrid::GetHaunchInputDistributionType()
{
   CEditHaunchByHaunchDlg* pGrandPa = (CEditHaunchByHaunchDlg*)GetParent();
   pgsTypes::HaunchInputDistributionType disttype = pGrandPa->GetHaunchInputDistributionType();
   return disttype;
}