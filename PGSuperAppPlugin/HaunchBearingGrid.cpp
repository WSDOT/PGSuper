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

// HaunchBearingGrid.cpp : implementation file
//

#include "stdafx.h"
#include "HaunchBearingGrid.h"
#include "EditHaunchDlg.h"

#include <System\Tokenizer.h>
#include "PGSuperUnits.h"
#include <Units\Measure.h>
#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\ClosureJointData.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const ROWCOL _STARTCOL = 1;

#define PIER 0
#define TS 1

GRID_IMPLEMENT_REGISTER(CHaunchBearingGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CHaunchBearingGrid

CHaunchBearingGrid::CHaunchBearingGrid()
{
//   RegisterClass();
}

CHaunchBearingGrid::~CHaunchBearingGrid()
{
}

BEGIN_MESSAGE_MAP(CHaunchBearingGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CHaunchBearingGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CHaunchBearingGrid message handlers


void CHaunchBearingGrid::CustomInit()
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
   const int num_cols=_STARTCOL+2;

	SetRowCount(num_rows);
	SetColCount(num_cols);

   // Turn off row, column, and whole table selection
   GetParam()->EnableSelection((WORD)(GX_SELFULL & ~GX_SELCOL & ~GX_SELROW & ~GX_SELTABLE));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

   // Header
   ROWCOL col=1;
	SetStyleRange(CGXRange(0,col,0,col+1), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_TOP)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
			.SetValue(_T("Location"))
		);

   CString strLabel;
   strLabel.Format(_T("Slab Offset (%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,col+2), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetVerticalAlignment(DT_TOP)
		.SetEnabled(FALSE)          // disables usage as current cell
		.SetValue(strLabel)
		);

   // Hide the row header column
   HideCols(0, 0);

	EnableIntelliMouse();
	SetFocus();

	GetParam( )->EnableUndo(TRUE);
}

void CHaunchBearingGrid::SetRowStyle(ROWCOL nRow)
{
   ROWCOL col = _STARTCOL;
	SetStyleRange(CGXRange(nRow,_STARTCOL,nRow,_STARTCOL+1), CGXStyle()
 			.SetEnabled(FALSE)          // disables usage as current cell
			.SetInterior(GXSYSCOLOR(COLOR_BTNFACE))
			.SetHorizontalAlignment(DT_CENTER)
			.SetVerticalAlignment(DT_VCENTER)
		);

	SetStyleRange(CGXRange(nRow,_STARTCOL+2), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
		);
}

void CHaunchBearingGrid::DoDataExchange(CDataExchange*pDX)
{
   if (pDX->m_bSaveAndValidate)
   {
      CEditHaunchDlg* pParent = (CEditHaunchDlg*)(GetParent()->GetParent());
      if (pParent->GetSlabOffsetType() == pgsTypes::sotBearingLine)
      {
         GetGridData(pDX);
      }
   }
   else
   {
      FillGrid();
   }
}

void CHaunchBearingGrid::FillGrid()
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   ROWCOL rows = GetRowCount();
   if (0 < rows)
   {
      RemoveRows(1, rows);
   }

   // we want to merge cells
   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   CEditHaunchDlg* pParent = (CEditHaunchDlg*)(GetParent()->GetParent());
   // get all the piers and temporary supports where slab offset is defined
   std::vector<std::pair<const CPierData2*, const CTemporarySupportData*>> vSupports;
   PierIndexType nPiers = pParent->m_BridgeDesc.GetPierCount();
   for (PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++)
   {
      const CPierData2* pPier = pParent->m_BridgeDesc.GetPier(pierIdx);
      if (pPier->HasSlabOffset())
      {
         vSupports.emplace_back(pPier, nullptr);
      }
   }
   SupportIndexType nTS = pParent->m_BridgeDesc.GetTemporarySupportCount();
   for (SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++)
   {
      const CTemporarySupportData* pTS = pParent->m_BridgeDesc.GetTemporarySupport(tsIdx);
      if (pTS->GetClosureJoint(0))
      {
         vSupports.emplace_back(nullptr, pTS);
      }
   }
   // sort them so they are in order
   std::sort(vSupports.begin(), vSupports.end(), [](auto& a, auto& b) 
   {
      Float64 staA = (a.first) ? a.first->GetStation() : a.second->GetStation();
      Float64 staB = (b.first) ? b.first->GetStation() : b.second->GetStation();
      return staA < staB;
   });

   ROWCOL row = 1;
   for (auto& support : vSupports)
   {
      if (support.first)
      {
         auto* pPier = support.first;
         auto pierIdx = pPier->GetIndex();
         std::array<Float64, 2> slabOffset;
         pPier->GetSlabOffset(&slabOffset[pgsTypes::Back], &slabOffset[pgsTypes::Ahead],pParent->m_BridgeDesc.GetSlabOffsetType() == pgsTypes::sotSegment ? true : false);
         CString strSupport;
         strSupport.Format(_T("%s"), LABEL_PIER_EX(pPier->IsAbutment(),pierIdx));
         if (pPier->IsAbutment())
         {
            pgsTypes::PierFaceType face = pPier->GetPrevSpan() == nullptr ? pgsTypes::Ahead : pgsTypes::Back;
            InsertRows(row, 1);
            SetRowStyle(row);
            SetStyleRange(CGXRange(row, _STARTCOL), CGXStyle().SetValue(strSupport).SetItemDataPtr((void*)PIER));
            SetStyleRange(CGXRange(row, _STARTCOL + 1), CGXStyle().SetValue(pPier->GetPrevSpan() == nullptr ? _T("Ahead") : _T("Back")).SetItemDataPtr((void*)pierIdx));
            SetStyleRange(CGXRange(row, _STARTCOL + 2), CGXStyle()
               .SetReadOnly(FALSE)
               .SetEnabled(TRUE)
               .SetValue(FormatDimension(slabOffset[face], *m_pUnit, false)).SetItemDataPtr((void*)face));
         }
         else
         {
            InsertRows(row, 2);
            SetRowStyle(row);

            SetStyleRange(CGXRange(row, _STARTCOL, row + 1, _STARTCOL), CGXStyle().SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE).SetValue(strSupport).SetItemDataPtr((void*)PIER));
            SetStyleRange(CGXRange(row, _STARTCOL + 1), CGXStyle().SetValue(_T("Back")).SetItemDataPtr((void*)pierIdx));
            SetStyleRange(CGXRange(row, _STARTCOL + 2), CGXStyle()
               .SetReadOnly(FALSE)
               .SetEnabled(TRUE)
               .SetValue(FormatDimension(slabOffset[pgsTypes::Back], *m_pUnit, false)).SetItemDataPtr((void*)pgsTypes::Back));

            row++;
            SetRowStyle(row);

            SetStyleRange(CGXRange(row, _STARTCOL + 1), CGXStyle().SetValue(_T("Ahead")).SetItemDataPtr((void*)pierIdx));
            SetStyleRange(CGXRange(row, _STARTCOL + 2), CGXStyle()
               .SetReadOnly(FALSE)
               .SetEnabled(TRUE)
               .SetValue(FormatDimension(slabOffset[pgsTypes::Ahead], *m_pUnit, false)).SetItemDataPtr((void*)pgsTypes::Ahead));
         }
      }
      else
      {
         auto* pTS = support.second;
         auto tsIdx = pTS->GetIndex();
         std::array<Float64, 2> slabOffset;
         pTS->GetSlabOffset(&slabOffset[pgsTypes::Back], &slabOffset[pgsTypes::Ahead], pParent->m_BridgeDesc.GetSlabOffsetType() == pgsTypes::sotSegment ? true : false);

         InsertRows(row, 2);
         SetRowStyle(row);
         CString strSupport;
         strSupport.Format(_T("TS %d (%s)"), LABEL_TEMPORARY_SUPPORT(tsIdx), pTS->GetSupportType() == pgsTypes::ErectionTower ? _T("ET") : _T("SB"));

         SetStyleRange(CGXRange(row, _STARTCOL, row + 1, _STARTCOL), CGXStyle().SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE).SetValue(strSupport).SetItemDataPtr((void*)TS));

         SetStyleRange(CGXRange(row, _STARTCOL + 1), CGXStyle().SetValue(_T("Back")).SetItemDataPtr((void*)tsIdx));
         SetStyleRange(CGXRange(row, _STARTCOL + 2), CGXStyle()
            .SetReadOnly(FALSE)
            .SetEnabled(TRUE)
            .SetValue(FormatDimension(slabOffset[pgsTypes::Back], *m_pUnit, false)).SetItemDataPtr((void*)pgsTypes::Back));

         row++;
         SetRowStyle(row);

         SetStyleRange(CGXRange(row, _STARTCOL + 1), CGXStyle().SetValue(_T("Ahead")).SetItemDataPtr((void*)tsIdx));
         SetStyleRange(CGXRange(row, _STARTCOL + 2), CGXStyle()
            .SetReadOnly(FALSE)
            .SetEnabled(TRUE)
            .SetValue(FormatDimension(slabOffset[pgsTypes::Ahead], *m_pUnit, false)).SetItemDataPtr((void*)pgsTypes::Ahead));
      }
      row++;
   } // next support
   
   ResizeRowHeightsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);

	ScrollCellInView(1, GetLeftCol());
}

void CHaunchBearingGrid::GetGridData(CDataExchange* pDX)
{
   CEditHaunchDlg* pParent = (CEditHaunchDlg*)(GetParent()->GetParent());

   Float64 minSlabOffset = pParent->m_BridgeDesc.GetMinSlabOffset();
   CString strMinValError;
   strMinValError.Format(_T("Slab Offset must be greater or equal to slab depth (%s)"), FormatDimension(minSlabOffset, *m_pUnit));

   ROWCOL nRows = GetRowCount();
   for (ROWCOL row = 1; row <= nRows; row++)
   {
      CString strValue = GetCellValue(row, _STARTCOL + 2);
      Float64 value;
      if (strValue.IsEmpty() || !sysTokenizer::ParseDouble(strValue, &value) || value < 0)
      {
         AfxMessageBox( _T("Value is not a number - must be a positive number"), MB_ICONEXCLAMATION);
         SetCurrentCell(row, _STARTCOL+2,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
         pDX->Fail();
      }
      else
      {
         Float64 slabOffset = ::ConvertToSysUnits(value, m_pUnit->UnitOfMeasure);

         if (::IsLT(slabOffset,minSlabOffset))
         {
            AfxMessageBox(strMinValError, MB_ICONERROR | MB_OK);
            this->SetCurrentCell(row, _STARTCOL+2,GX_SCROLLINVIEW|GX_DISPLAYEDITWND);
            pDX->Fail();
         }

         CGXStyle style;
         GetStyleRowCol(row, _STARTCOL, style);
         int type = (int)(UINT64)(style.GetItemDataPtr());

         GetStyleRowCol(row, _STARTCOL+1, style);
         IndexType idx = (IndexType)(style.GetItemDataPtr());

         GetStyleRowCol(row, _STARTCOL + 2, style);
         pgsTypes::PierFaceType face = (pgsTypes::PierFaceType)(UINT64)(style.GetItemDataPtr());

         if (type == PIER)
         {
            auto* pPier = pParent->m_BridgeDesc.GetPier(idx);
            pPier->SetSlabOffset(face, slabOffset);
         }
         else
         {
            auto* pTS = pParent->m_BridgeDesc.GetTemporarySupport(idx);
            pTS->SetSlabOffset(face, slabOffset);
         }
      }
   }
}

CString CHaunchBearingGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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