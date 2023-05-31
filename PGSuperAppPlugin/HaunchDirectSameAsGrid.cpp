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

// HaunchDirectSameAsGrid.cpp : implementation file
//

#include "stdafx.h"
#include "HaunchDirectSameAsGrid.h"
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

GRID_IMPLEMENT_REGISTER(CHaunchDirectSameAsGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CHaunchDirectSameAsGrid

CHaunchDirectSameAsGrid::CHaunchDirectSameAsGrid()
{
//   RegisterClass();
}

CHaunchDirectSameAsGrid::~CHaunchDirectSameAsGrid()
{
}

BEGIN_MESSAGE_MAP(CHaunchDirectSameAsGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CHaunchDirectSameAsGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
   ON_WM_DESTROY()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CHaunchDirectSameAsGrid message handlers
CBridgeDescription2* CHaunchDirectSameAsGrid::GetBridgeDesc()
{
   return m_pParent->GetBridgeDesc();
}

void CHaunchDirectSameAsGrid::CustomInit(pgsTypes::HaunchLayoutType layout, GroupIndexType group, CEditHaunchByHaunchDlg* pParent)
{
   m_LayoutType = layout;
   m_Group = group;
   m_pParent = pParent;

   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
   Initialize();

   // Don't allow pasting to increase grid size
   m_nClipboardFlags |= GX_DNDNOAPPENDCOLS | GX_DNDNOAPPENDROWS;

   EnableIntelliMouse();
}

void CHaunchDirectSameAsGrid::InvalidateGrid()
{
   ClearCells(CGXRange(0,0,GetRowCount(),GetColCount()));
   BuildGridAndHeader();
}

int CHaunchDirectSameAsGrid::GetColWidth(ROWCOL nCol)
{
   if (IsColHidden(nCol))
      return CGXGridWnd::GetColWidth(nCol);

   CRect rect = GetGridRect();

   pgsTypes::HaunchInputDistributionType disttype = GetHaunchInputDistributionType();
   if (disttype != pgsTypes::hidUniform)
   {
      return rect.Width() / 15;
   }
   else
   {
      return rect.Width() / 10;
   }
}

void CHaunchDirectSameAsGrid::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   if (pDX->m_bSaveAndValidate)
   {
      GetGridData(pDX);
   }
}

void CHaunchDirectSameAsGrid::BuildGridAndHeader()
{
   CEAFDocument* pDoc = EAFGetDocument();
   BOOL bIsPGSuper = pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc));

   m_DoSpans = bIsPGSuper || m_LayoutType == pgsTypes::hltAlongSpans;

   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   // we want to merge cells
   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   CBridgeDescription2* pBridge = GetBridgeDesc();
   SegmentIndexType nSegs = pBridge->GetGirderGroup(m_Group)->GetGirder(0)->GetSegmentCount();
   SegmentIndexType nSpans = pBridge->GetSpanCount();
   GirderIndexType nGdrs = pBridge->GetGirderGroup(m_Group)->GetGirderCount();

   pgsTypes::HaunchInputDistributionType disttype = GetHaunchInputDistributionType();

   m_nExtraHeaderRows = (disttype == pgsTypes::hidUniform) ? 0 : 1;
   ROWCOL nRows = m_nExtraHeaderRows;
   ROWCOL nCols = (m_LayoutType == pgsTypes::hltAlongSegments) ? (ROWCOL)(nSegs * disttype) : (ROWCOL)(nSpans * disttype);

   SetRowCount(nRows);
   SetColCount(nCols);

   // Turn off row, column, and whole table selection
   GetParam()->EnableSelection((WORD)(GX_SELFULL & ~GX_SELCOL & ~GX_SELROW & ~GX_SELTABLE));

   // no row moving
   GetParam()->EnableMoveRows(FALSE);
   GetParam()->EnableMoveCols(FALSE);

   SetFrozenRows(nRows/*# frozen rows*/,m_nExtraHeaderRows/*# extra header rows*/);

   if (!m_DoSpans)
   {
      // Segments
      ROWCOL col = 1;
      for (SegmentIndexType segIdx = 0; segIdx < nSegs; segIdx++)
      {
         CString strLabel;
         strLabel.Format(_T("Segment %d"),LABEL_SEGMENT(segIdx));
         SetStyleRange(CGXRange(0,col,0,col + disttype - 1),CGXStyle()
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_TOP)
            .SetEnabled(FALSE)
            .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
            .SetValue(strLabel)
         );

         if (disttype != pgsTypes::hidUniform)
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
                  .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
                  .SetValue(strLabel)
               );

               col++;
            }
         }
         else
         {
            col++;
         }
      } // next segment
   }
   else
   {
      // Spans
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

         if (disttype != pgsTypes::hidUniform)
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
         else
         {
            col++;
         }
      } // next span
   }

   // top left corner
   SetStyleRange(CGXRange(0,0,GetRowCount(),0),CGXStyle().SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue(_T("Girder")));

   FillGrid();

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

void CHaunchDirectSameAsGrid::FillGrid()
{
   CComPtr<IBroker> pBroker;
   ::EAFGetBroker(&pBroker);

   const CBridgeDescription2* pBridgeOrig = GetBridgeDesc();

   SegmentIndexType nSegs = pBridgeOrig->GetGirderGroup(m_Group)->GetGirder(0)->GetSegmentCount();
   SegmentIndexType nSpans = pBridgeOrig->GetSpanCount();

   pgsTypes::HaunchInputDistributionType disttype = GetHaunchInputDistributionType();

   GirderIndexType maxGdrs = 0;
   if (m_DoSpans)
   {
      for (SpanIndexType ispan = 0; ispan < nSpans; ispan++)
      {
         maxGdrs = max(maxGdrs,pBridgeOrig->GetSpan(ispan)->GetGirderCount());
      }
   }
   else
   {
      maxGdrs = pBridgeOrig->GetGirderGroup(m_Group)->GetGirderCount();
   }

   ROWCOL nRows = GetRowCount();
   if (m_nExtraHeaderRows + 1 < nRows)
   {
      RemoveRows(m_nExtraHeaderRows + 1,nRows);
   }

   ROWCOL row = m_nExtraHeaderRows + 1;
   ROWCOL col = 1;

   InsertRows(row,1);
   SetInitialRowStyle(row);

   CString strLabel;
   strLabel.Format(_T("%s to %s"),LABEL_GIRDER(0),LABEL_GIRDER(maxGdrs-1));
   SetStyleRange(CGXRange(row,0),CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetFont(CGXFont().SetBold(TRUE))
      .SetValue(strLabel));

   if (m_DoSpans)
   {
      // Convert current haunch data if needed
      HaunchDepthInputConversionTool conversionTool(pBridgeOrig,pBroker,false);
      auto convPair = conversionTool.ConvertToDirectHaunchInput(pgsTypes::hilSame4AllGirders, pgsTypes::hltAlongSpans, disttype);
      const CBridgeDescription2* pBridge = &convPair.second;

      for (SpanIndexType ispan = 0; ispan < nSpans; ispan++)
      {
         const CSpanData2* pSpan = pBridge->GetSpan(ispan);

         std::vector<Float64> haunches = pSpan->GetDirectHaunchDepths(0); // depth is same for all girders

         for (auto haunch : haunches)
         {
            // Function below adds deck thickness if needed and converts units
            CString cellStr = m_pParent->ConvertValueToGridString(haunch);

            SetStyleRange(CGXRange(row,col),CGXStyle()
               .SetReadOnly(FALSE)
               .SetEnabled(TRUE)
               .SetHorizontalAlignment(DT_RIGHT)
               .SetInterior(::GetSysColor(COLOR_WINDOW))
               .SetValue(cellStr));

            col++;
         }
      }
   }
   else
   {
      // Convert current haunch data if needed
      HaunchDepthInputConversionTool conversionTool(pBridgeOrig,pBroker,false);
      auto convPair = conversionTool.ConvertToDirectHaunchInput(pgsTypes::hilSame4AllGirders, pgsTypes::hltAlongSegments, disttype);
      const CBridgeDescription2* pBridge = &convPair.second;

      auto* pGroup = pBridge->GetGirderGroup(m_Group);
      auto* pGirder = pGroup->GetGirder(0); // SameAs values are in girder zero
      auto nSegments = pGirder->GetSegmentCount();
      for (SegmentIndexType iseg = 0; iseg < nSegs; iseg++)
      {
         std::vector<Float64> haunches = pGirder->GetDirectHaunchDepths(iseg);
         for (auto haunch : haunches)
         {
            CString cellStr = m_pParent->ConvertValueToGridString(haunch);

            SetStyleRange(CGXRange(row,col),CGXStyle()
               .SetReadOnly(FALSE)
               .SetEnabled(TRUE)
               .SetHorizontalAlignment(DT_RIGHT)
               .SetInterior(::GetSysColor(COLOR_WINDOW))
               .SetValue(cellStr));

            col++;
         }
      }
   }
}

void CHaunchDirectSameAsGrid::GetGridData(CDataExchange* pDX)
{
   CBridgeDescription2* pBridge = GetBridgeDesc();
   SegmentIndexType nSegs = pBridge->GetGirderGroup(m_Group)->GetGirder(0)->GetSegmentCount();
   SegmentIndexType nSpans = pBridge->GetSpanCount();
   pgsTypes::HaunchInputDistributionType disttype = GetHaunchInputDistributionType();

   ROWCOL row = m_nExtraHeaderRows + 1;
   ROWCOL col = 1;

   if (m_DoSpans)
   {
      ATLASSERT(pBridge->GetHaunchInputDepthType() == pgsTypes::hidHaunchDirectly || pBridge->GetHaunchInputDepthType() == pgsTypes::hidHaunchPlusSlabDirectly);
      pBridge->SetHaunchInputLocationType(pgsTypes::hilSame4AllGirders);
      pBridge->SetHaunchInputDistributionType(disttype);
      pBridge->SetHaunchLayoutType(pgsTypes::hltAlongSpans);

      for (SpanIndexType ispan = 0; ispan < nSpans; ispan++)
      {
         CSpanData2* pSpan = pBridge->GetSpan(ispan);

         std::vector<Float64> haunches((std::size_t)disttype,0.0); // fixed size vector
         for (auto idis = 0; idis < disttype; idis++)
         {
            CString strValue = GetCellValue(row,col);

            // Function below converts units and subtracts deck thickness from input value if needed
            Float64 value = m_pParent->GetValueFromGrid(strValue,pDX,row,col,this);

            haunches[idis] = value;
            col++;
         }

         pSpan->SetDirectHaunchDepths(haunches);
      }
   }
   else
   {
      ATLASSERT(pBridge->GetHaunchInputDepthType() == pgsTypes::hidHaunchDirectly || pBridge->GetHaunchInputDepthType() == pgsTypes::hidHaunchPlusSlabDirectly);
      pBridge->SetHaunchInputLocationType(pgsTypes::hilSame4AllGirders);
      pBridge->SetHaunchInputDistributionType(disttype);
      pBridge->SetHaunchLayoutType(pgsTypes::hltAlongSegments);

      // Segments
      std::vector<Float64> haunches((std::size_t)disttype,0.0); // fixed size vector

      auto* pGroup = pBridge->GetGirderGroup(m_Group);
      GirderIndexType nGdrs = pGroup->GetGirderCount();
      auto* pGirder0 = pGroup->GetGirder(0); // Assume that all girders in a group have the same number of segments
      auto nSegments = pGirder0->GetSegmentCount();
      for (SegmentIndexType iseg = 0; iseg < nSegs; iseg++)
      {
         for (auto idis = 0; idis < disttype; idis++)
         {
            CString strValue = GetCellValue(row,col);

            Float64 value = m_pParent->GetValueFromGrid(strValue,pDX,row,col,this);

            haunches[idis] = value;
            col++;
         }

         // Set for all girders in group
         for (GirderIndexType igdr = 0; igdr < nGdrs; igdr++)
         {
            auto* pGirder = pGroup->GetGirder(igdr);
            pGirder->SetDirectHaunchDepths(iseg,haunches);
      }
      } // segments
   }
}

CString CHaunchDirectSameAsGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CHaunchDirectSameAsGrid::SetInitialRowStyle(ROWCOL row)
{
   ROWCOL nCols = GetColCount();
   SetStyleRange(CGXRange(row, 1, row, nCols), CGXStyle()
      .SetReadOnly(TRUE)
      .SetEnabled(FALSE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
   );
}

void CHaunchDirectSameAsGrid::OnDestroy()
{
   CGXGridWnd::OnDestroy();
   delete this;
}

pgsTypes::HaunchInputDistributionType CHaunchDirectSameAsGrid::GetHaunchInputDistributionType()
{
   pgsTypes::HaunchInputDistributionType disttype = m_pParent->GetHaunchInputDistributionType();
   return disttype;
}
