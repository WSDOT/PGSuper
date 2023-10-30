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

// HaunchSegmentGrid.cpp : implementation file
//

#include "stdafx.h"
#include "HaunchSegmentGrid.h"
#include "EditHaunchACamberDlg.h"

#include <System\Tokenizer.h>
#include "PGSuperUnits.h"
#include <Units\Measure.h>
#include <EAF\EAFDisplayUnits.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\HaunchDepthInputConversionTool.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CHaunchSegmentGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CHaunchSegmentGrid

CHaunchSegmentGrid::CHaunchSegmentGrid()
{
//   RegisterClass();
}

CHaunchSegmentGrid::~CHaunchSegmentGrid()
{
}

BEGIN_MESSAGE_MAP(CHaunchSegmentGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CHaunchSegmentGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
   ON_WM_DESTROY()
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CHaunchSegmentGrid message handlers

int CHaunchSegmentGrid::GetColWidth(ROWCOL nCol)
{
   if ( IsColHidden(nCol) )
      return CGXGridWnd::GetColWidth(nCol);

	CRect rect = GetGridRect( );

   return (int)(rect.Width( )*(Float64)1/7);
}

CBridgeDescription2* CHaunchSegmentGrid::GetBridgeDesc()
{
   CEditHaunchACamberDlg* pParent;
   if (m_GroupIdx == ALL_GROUPS)
   {
      pParent = (CEditHaunchACamberDlg*)(GetParent()->GetParent());
   }
   else
   {
      pParent = (CEditHaunchACamberDlg*)(GetParent()->GetParent()->GetParent());
   }
   return pParent->GetBridgeDesc();
}

void CHaunchSegmentGrid::CustomInit(GroupIndexType grpIdx)
{
   m_GroupIdx = grpIdx;

   // initialize units
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   m_pUnit = &(pDisplayUnits->GetComponentDimUnit());

// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );

   // Don't allow pasting to increase grid size
   m_nClipboardFlags |= GX_DNDNOAPPENDCOLS | GX_DNDNOAPPENDROWS;

	GetParam( )->EnableUndo(FALSE);

   // we want to merge cells
   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   CBridgeDescription2* pBridge = GetBridgeDesc();

   ROWCOL nRows;
   ROWCOL nCols;
   if (m_GroupIdx == ALL_GROUPS)
   {
      nRows = 2;
      nCols = (ROWCOL)(pBridge->GetPierCount() * 2 - 2);
   }
   else
   {
      nRows = 3;
      nCols = (ROWCOL)(pBridge->GetGirderGroup(m_GroupIdx)->GetGirder(0)->GetSegmentCount() * 2);
   }

   m_nExtraHeaderRows = nRows - 1;
   SetRowCount(nRows);
   SetColCount(nCols);

   // Turn off row, column, and whole table selection
   GetParam()->EnableSelection((WORD)(GX_SELFULL & ~GX_SELCOL & ~GX_SELROW & ~GX_SELTABLE));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   SetFrozenRows(nRows-1/*# frozen rows*/, m_nExtraHeaderRows/*# extra header rows*/);

   ROWCOL col = 1;
   if (m_GroupIdx == ALL_GROUPS)
   {
      PierIndexType nPiers = pBridge->GetPierCount();
      for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
      {
         if (pierIdx == 0 || pierIdx == nPiers - 1)
         {
            CString strLabel;
            strLabel.Format(_T("%s"), LABEL_PIER_EX(true,pierIdx));
            SetStyleRange(CGXRange(0, col), CGXStyle()
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_TOP)
            .SetEnabled(FALSE)
            .SetValue(strLabel));

            strLabel.Format(_T("%s (%s)"), pierIdx == 0 ? _T("Ahead") : _T("Back"), m_pUnit->UnitOfMeasure.UnitTag().c_str());
            SetStyleRange(CGXRange(1, col), CGXStyle()
               .SetHorizontalAlignment(DT_CENTER)
               .SetVerticalAlignment(DT_TOP)
               .SetEnabled(FALSE)
               .SetValue(strLabel)
            );
         }
         else
         {
            CString strLabel;
            strLabel.Format(_T("%s"), LABEL_PIER_EX(false, pierIdx));
            SetStyleRange(CGXRange(0, col, 0, col+1), CGXStyle()
               .SetHorizontalAlignment(DT_CENTER)
               .SetVerticalAlignment(DT_TOP)
               .SetEnabled(FALSE)
               .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
               .SetValue(strLabel)
            );

            strLabel.Format(_T("%s (%s)"), _T("Back"), m_pUnit->UnitOfMeasure.UnitTag().c_str());
            SetStyleRange(CGXRange(1, col), CGXStyle()
               .SetHorizontalAlignment(DT_CENTER)
               .SetVerticalAlignment(DT_TOP)
               .SetEnabled(FALSE)
               .SetValue(strLabel)
            );

            col++;

            strLabel.Format(_T("%s (%s)"), _T("Ahead"), m_pUnit->UnitOfMeasure.UnitTag().c_str());
            SetStyleRange(CGXRange(1, col), CGXStyle()
               .SetHorizontalAlignment(DT_CENTER)
               .SetVerticalAlignment(DT_TOP)
               .SetEnabled(FALSE)
               .SetValue(strLabel)
            );
         }

         col++;
      } // next pier
   }
   else
   {
      auto* pGroup = pBridge->GetGirderGroup(m_GroupIdx);
      auto* pGirder = pGroup->GetGirder(0); // all girders in the group have the same number of segments
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         auto* pSegment = pGirder->GetSegment(segIdx);

         CString strLabel;
         strLabel.Format(_T("Segment %d"), LABEL_SEGMENT(segIdx));
         SetStyleRange(CGXRange(0, col, 0, col + 1), CGXStyle()
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_TOP)
            .SetEnabled(FALSE)
            .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
            .SetValue(strLabel)
         );

         for(int i = 0; i < 2; i++)
         {
            pgsTypes::MemberEndType end = (pgsTypes::MemberEndType)i;
            CPierData2* pPier;
            CTemporarySupportData* pTS;
            pSegment->GetSupport(end, &pPier, &pTS);
            if (pPier)
            {
               CString strLabel;
               strLabel.Format(_T("%s"), LABEL_PIER_EX(pPier->IsAbutment(),pPier->GetIndex()));

               SetStyleRange(CGXRange(1, col), CGXStyle()
                  .SetHorizontalAlignment(DT_CENTER)
                  .SetVerticalAlignment(DT_TOP)
                  .SetEnabled(FALSE)
                  .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
                  .SetValue(strLabel)
               );

               strLabel.Format(_T("%s (%s)"), end == pgsTypes::metStart ? _T("Ahead") : _T("Back"), m_pUnit->UnitOfMeasure.UnitTag().c_str());
               SetStyleRange(CGXRange(2, col), CGXStyle()
                  .SetHorizontalAlignment(DT_CENTER)
                  .SetVerticalAlignment(DT_TOP)
                  .SetEnabled(FALSE)
                  .SetValue(strLabel)
               );
            }
            else
            {
               CString strLabel;
               strLabel.Format(_T("TS %d (%s)"), LABEL_TEMPORARY_SUPPORT(pTS->GetIndex()), pTS->GetSupportType() == pgsTypes::ErectionTower ? _T("ET") : _T("SB"));
               SetStyleRange(CGXRange(1, col), CGXStyle()
                  .SetHorizontalAlignment(DT_CENTER)
                  .SetVerticalAlignment(DT_TOP)
                  .SetEnabled(FALSE)
                  .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
                  .SetValue(strLabel)
               );

               strLabel.Format(_T("%s (%s)"), end == pgsTypes::metStart ? _T("Ahead") : _T("Back"), m_pUnit->UnitOfMeasure.UnitTag().c_str());
               SetStyleRange(CGXRange(2, col), CGXStyle()
                  .SetHorizontalAlignment(DT_CENTER)
                  .SetVerticalAlignment(DT_TOP)
                  .SetEnabled(FALSE)
                  .SetValue(strLabel)
               );
            }
            col++;
         } // next end
      } // next segment
   }

   // top left corner
   SetStyleRange(CGXRange(0,0,m_nExtraHeaderRows,0),CGXStyle().SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE).SetControl(GX_IDS_CTRL_HEADER).SetValue(_T(" ")));

   // make text fit correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();
	SetFocus();

	GetParam( )->EnableUndo(TRUE);
}

void CHaunchSegmentGrid::DoDataExchange(CDataExchange* pDX)
{
   __super::DoDataExchange(pDX);
   if (pDX->m_bSaveAndValidate)
   {
      const CBridgeDescription2* pBridge = GetBridgeDesc();
      if (pBridge->GetSlabOffsetType() == pgsTypes::sotSegment)
      {
         GetGridData(pDX);
      }
   }
   else
   {
      FillGrid();
   }
}

void CHaunchSegmentGrid::FillGrid()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   const CBridgeDescription2* pBridgeOrig = GetBridgeDesc();

   // Convert current haunch data if needed
   HaunchDepthInputConversionTool conversionTool(pBridgeOrig,pBroker,false);
   auto convPair = conversionTool.ConvertToSlabOffsetInput(pgsTypes::sotSegment);
   const CBridgeDescription2* pBridge = &convPair.second;

   GroupIndexType startGroupIdx = (m_GroupIdx == ALL_GROUPS ? 0 : m_GroupIdx);
   GroupIndexType endGroupIdx = (m_GroupIdx == ALL_GROUPS ? pBridge->GetGirderGroupCount() - 1 : startGroupIdx);

   ROWCOL nRows = GetRowCount();
   if (m_nExtraHeaderRows+1 < nRows)
   {
      RemoveRows(m_nExtraHeaderRows+1, nRows);
   }
  
   long nMaxGirders = -1;
   for(GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++)
   {
      ROWCOL row = m_nExtraHeaderRows+1;

      auto* pGroup = pBridge->GetGirderGroup(grpIdx);
      auto nGirders = pGroup->GetGirderCount();

      for (auto gdrIdx = 0; gdrIdx < nGirders; gdrIdx++, row++)
      {
         ROWCOL col = (ROWCOL)(grpIdx - startGroupIdx) * 2 + 1;

         auto* pGirder = pGroup->GetGirder(gdrIdx);

         if (grpIdx == startGroupIdx || nMaxGirders <= gdrIdx)
         {
            InsertRows(row, 1);
            SetInitialRowStyle(row);

            CString strLabel;
            strLabel.Format(_T("Girder %s"), LABEL_GIRDER(gdrIdx));
            SetStyleRange(CGXRange(row, 0), CGXStyle().SetValue(strLabel));
         }

         auto nSegments = pGirder->GetSegmentCount();
         for (auto segIdx = 0; segIdx < nSegments; segIdx++)
         {
            auto* pSegment = pGirder->GetSegment(segIdx);

            std::array<Float64, 2> slabOffset;
            pSegment->GetSlabOffset(&slabOffset[pgsTypes::metStart], &slabOffset[pgsTypes::metEnd], true);

            SetStyleRange(CGXRange(row, col), CGXStyle()
               .SetReadOnly(FALSE)
               .SetEnabled(TRUE)
               .SetHorizontalAlignment(DT_RIGHT)
               .SetInterior(::GetSysColor(COLOR_WINDOW))
               .SetValue(FormatDimension(slabOffset[pgsTypes::metStart], *m_pUnit, false)));

            col++;

            SetStyleRange(CGXRange(row, col), CGXStyle()
               .SetReadOnly(FALSE)
               .SetEnabled(TRUE)
               .SetHorizontalAlignment(DT_RIGHT)
               .SetInterior(::GetSysColor(COLOR_WINDOW))
               .SetValue(FormatDimension(slabOffset[pgsTypes::metEnd], *m_pUnit, false)));

            col++;
         } // next segment
      } // next girder
      nMaxGirders = Max(nMaxGirders, (long)nGirders);
   } // next group

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);

	ScrollCellInView(1, GetLeftCol());
}

void CHaunchSegmentGrid::GetGridData(CDataExchange* pDX)
{
   CBridgeDescription2* pBridge = GetBridgeDesc();

   Float64 minSlabOffset = pBridge->GetMinSlabOffset();
   CString strMinValError;
   strMinValError.Format(_T("Slab Offset must be greater or equal to slab depth + fillet (%s)"), FormatDimension(minSlabOffset, *m_pUnit));

   GroupIndexType startGroupIdx = (m_GroupIdx == ALL_GROUPS ? 0 : m_GroupIdx);
   GroupIndexType endGroupIdx = (m_GroupIdx == ALL_GROUPS ? pBridge->GetGirderGroupCount() - 1 : startGroupIdx);
   for (GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++)
   {
      ROWCOL row = m_nExtraHeaderRows + 1;

      auto* pGroup = pBridge->GetGirderGroup(grpIdx);
      auto nGirders = pGroup->GetGirderCount();
      for (auto gdrIdx = 0; gdrIdx < nGirders; gdrIdx++, row++)
      {
         ROWCOL col = (ROWCOL)(grpIdx - startGroupIdx) * 2 + 1;
         auto* pGirder = pGroup->GetGirder(gdrIdx);
         auto nSegments = pGirder->GetSegmentCount();
         for (auto segIdx = 0; segIdx < nSegments; segIdx++)
         {
            auto* pSegment = pGirder->GetSegment(segIdx);

            std::array<Float64, 2> slabOffset;
            for (int i = 0; i < 2; i++, col++)
            {
               pgsTypes::MemberEndType end = (pgsTypes::MemberEndType)i;
               CString strValue = GetCellValue(row, col);
               Float64 value;
               if (strValue.IsEmpty() || !WBFL::System::Tokenizer::ParseDouble(strValue, &value) || value < 0)
               {
                  AfxMessageBox(_T("Value is not a number - must be a positive number"), MB_ICONEXCLAMATION);
                  SetCurrentCell(row,col, GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
                  pDX->Fail();
               }
               else
               {
                  slabOffset[end] = WBFL::Units::ConvertToSysUnits(value, m_pUnit->UnitOfMeasure);
                  if (IsLT(slabOffset[end], minSlabOffset))
                  {
                     AfxMessageBox(strMinValError, MB_ICONERROR | MB_OK);
                     this->SetCurrentCell(row, col, GX_SCROLLINVIEW | GX_DISPLAYEDITWND);
                     pDX->Fail();
                  }
               }
            } // next segment end

            pSegment->SetSlabOffset(slabOffset[pgsTypes::metStart], slabOffset[pgsTypes::metEnd]);
         } // next segment
      } // next girder
   } // next group
}

CString CHaunchSegmentGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CHaunchSegmentGrid::SetInitialRowStyle(ROWCOL row)
{
   ROWCOL nCols = GetColCount();
   SetStyleRange(CGXRange(row, 1, row, nCols), CGXStyle()
      .SetReadOnly(TRUE)
      .SetEnabled(FALSE)
      .SetInterior(::GetSysColor(COLOR_BTNFACE))
   );
}

void CHaunchSegmentGrid::OnDestroy()
{
   CGXGridWnd::OnDestroy();
   delete this;
}
