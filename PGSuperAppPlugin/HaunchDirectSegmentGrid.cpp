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

// HaunchDirectSegmentGrid.cpp : implementation file
//

#include "stdafx.h"
#include "HaunchDirectSegmentGrid.h"
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

GRID_IMPLEMENT_REGISTER(CHaunchDirectSegmentGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CHaunchDirectSegmentGrid

CHaunchDirectSegmentGrid::CHaunchDirectSegmentGrid():
   m_pParent(nullptr)
{
//   RegisterClass();
}

CHaunchDirectSegmentGrid::~CHaunchDirectSegmentGrid()
{
}

BEGIN_MESSAGE_MAP(CHaunchDirectSegmentGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CHaunchDirectSegmentGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
   ON_WM_DESTROY()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CHaunchDirectSegmentGrid message handlers
CBridgeDescription2* CHaunchDirectSegmentGrid::GetBridgeDesc()
{
   return m_pParent->GetBridgeDesc();
}

void CHaunchDirectSegmentGrid::CustomInit(GroupIndexType grpIdx,CEditHaunchByHaunchDlg* pParent)
{
   m_GroupIdx = grpIdx;
   m_pParent = pParent;

   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
   Initialize();

   // Don't allow pasting to increase grid size
   m_nClipboardFlags |= GX_DNDNOAPPENDCOLS | GX_DNDNOAPPENDROWS;

   EnableIntelliMouse();
}

void CHaunchDirectSegmentGrid::InvalidateGrid()
{
   ClearCells(CGXRange(0,0,GetRowCount(),GetColCount()));
   BuildGridAndHeader();
   FillGrid();
}

int CHaunchDirectSegmentGrid::GetColWidth(ROWCOL nCol)
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

void CHaunchDirectSegmentGrid::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   if (pDX->m_bSaveAndValidate)
   {
      GetGridData(pDX);
   }
}

void CHaunchDirectSegmentGrid::BuildGridAndHeader()
{
   CEAFDocument* pDoc = EAFGetDocument();
   BOOL bIsPGSuper = pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc));

   GetParam()->EnableUndo(FALSE);

   // we want to merge cells
   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   CBridgeDescription2* pBridge = GetBridgeDesc();

   SegmentIndexType nSegs = pBridge->GetGirderGroup(m_GroupIdx)->GetGirder(0)->GetSegmentCount();
   GirderIndexType nGdrs = pBridge->GetGirderGroup(m_GroupIdx)->GetGirderCount();

   pgsTypes::HaunchInputDistributionType disttype = GetHaunchInputDistributionType();

   ROWCOL nRows = (ROWCOL)2;
   ROWCOL nCols = (ROWCOL)(nSegs * disttype);

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

         col++;
      }
   }
   else
   {
      m_nExtraHeaderRows = 1;
      SetFrozenRows(nRows/*# frozen rows*/,m_nExtraHeaderRows/*# extra header rows*/);

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
      } // next segment
   }

   // top left corner
   SetStyleRange(CGXRange(0,0,1,0),CGXStyle().SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue(_T("Girder")));

   // make text fit correctly
   ResizeRowHeightsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));

   // don't allow users to resize grids
   GetParam()->EnableTrackColWidth(0);
   GetParam()->EnableTrackRowHeight(0);

   SetFocus();

   GetParam()->EnableUndo(TRUE);
}

void CHaunchDirectSegmentGrid::FillGrid()
{
   CComPtr<IBroker> pBroker;
   ::EAFGetBroker(&pBroker);

   CEAFDocument* pDoc = EAFGetDocument();
   BOOL bIsPGSuper = pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc));

   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   ROWCOL nRows = GetRowCount();
   if (m_nExtraHeaderRows < nRows)
   {
      RemoveRows(m_nExtraHeaderRows + 1,nRows);
   }

   const CBridgeDescription2* pBridgeOrig = GetBridgeDesc();
   pgsTypes::HaunchInputDistributionType disttype = GetHaunchInputDistributionType();

   // Convert current haunch data if needed
   HaunchDepthInputConversionTool conversionTool(pBridgeOrig, pBroker,false);
   auto convPair = conversionTool.ConvertToDirectHaunchInput(pgsTypes::hilPerEach,pgsTypes::hltAlongSegments,disttype);

   const CBridgeDescription2* pBridge = &convPair.second;
   auto* pGroup = pBridge->GetGirderGroup(m_GroupIdx);
   auto nGirders = pGroup->GetGirderCount();

   ROWCOL row = m_nExtraHeaderRows + 1;
   for (auto gdrIdx = 0; gdrIdx < nGirders; gdrIdx++,row++)
   {
      ROWCOL col = 1;
      InsertRows(row,1);
      SetInitialRowStyle(row);

      CString strLabel;
      strLabel.Format(_T("%s"),LABEL_GIRDER(gdrIdx));
      SetStyleRange(CGXRange(row,0),CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetFont(CGXFont().SetBold(TRUE))
         .SetValue(strLabel));

      auto* pGirder = pGroup->GetGirder(gdrIdx);
      auto nSegments = pGirder->GetSegmentCount();
      for (auto segIdx = 0; segIdx < nSegments; segIdx++)
      {
         std::vector<Float64> haunchDepths = pGirder->GetDirectHaunchDepths(segIdx);
         for (auto haunch : haunchDepths)
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
      } // next segment
   } // next girder

   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(TRUE);
   ScrollCellInView(1,GetLeftCol());
}

void CHaunchDirectSegmentGrid::GetGridData(CDataExchange* pDX)
{
   ROWCOL row = m_nExtraHeaderRows + 1;

   pgsTypes::HaunchInputDistributionType disttype = GetHaunchInputDistributionType();
   CBridgeDescription2* pBridge = GetBridgeDesc();

   ATLASSERT(pBridge->GetHaunchInputDepthType()==pgsTypes::hidHaunchDirectly || pBridge->GetHaunchInputDepthType() == pgsTypes::hidHaunchPlusSlabDirectly);
   pBridge->SetHaunchInputLocationType(pgsTypes::hilPerEach);
   pBridge->SetHaunchInputDistributionType(disttype);
   pBridge->SetHaunchLayoutType(pgsTypes::hltAlongSegments);

   auto* pGroup = pBridge->GetGirderGroup(m_GroupIdx);
   auto nGirders = pGroup->GetGirderCount();
   for (auto gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
   {
      ROWCOL col = 1;
      auto* pGirder = pGroup->GetGirder(gdrIdx);
      auto nSegments = pGirder->GetSegmentCount();
      for (auto segIdx = 0; segIdx < nSegments; segIdx++)
      {
         std::vector<Float64> haunches((std::size_t)disttype,0.0); // fixed size vector
         for (auto idis = 0; idis < disttype; idis++)
         {
            CString strValue = GetCellValue(row,col);

            // Function below converts units and subtracts deck thickness from input value if needed
            Float64 value = m_pParent->GetValueFromGrid(strValue,pDX,row,col,this);

            haunches[idis] = value;
            col++;
         }

         pGirder->SetDirectHaunchDepths(segIdx,haunches);
      } // next segment

      row++;
   } // next girder
}

CString CHaunchDirectSegmentGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CHaunchDirectSegmentGrid::SetInitialRowStyle(ROWCOL row)
{
   ROWCOL nCols = GetColCount();
   SetStyleRange(CGXRange(row, 1, row, nCols), CGXStyle()
      .SetReadOnly(TRUE)
      .SetEnabled(FALSE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
   );
}

void CHaunchDirectSegmentGrid::OnDestroy()
{
   CGXGridWnd::OnDestroy();
   delete this;
}

pgsTypes::HaunchInputDistributionType CHaunchDirectSegmentGrid::GetHaunchInputDistributionType()
{
   pgsTypes::HaunchInputDistributionType disttype = m_pParent->GetHaunchInputDistributionType();
   return disttype;
}