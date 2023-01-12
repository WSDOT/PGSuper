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

// BridgeDescFramingGrid2.cpp : implementation file
//

// NOTE
// In this grid, when we talk about segment lengths, we don't mean the length of segments of the girder.
// Segment lengths in this context is the distance between segment supports measured along the alignment
// which is based on stationing.

#include "stdafx.h"
#include "BridgeDescFramingGrid.h"
#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include "BridgeDescDlg.h"
#include "BridgeDescFramingPage.h"

#include "SpanDetailsDlg.h"
#include "PierDetailsDlg.h"
#include "InsertSpanDlg.h"
#include "TemporarySupportDlg.h"

#include <Units\Measure.h>
#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\ClosureJointData.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class CRowType : public CGXAbstractUserAttribute
{
public:
   enum RowType { Span, Pier, TempSupport };
   CRowType(RowType type,CollectionIndexType idx)
   {
      m_Type = type;
      m_Index = idx;
   }

   virtual CGXAbstractUserAttribute* Clone() const
   {
      return new CRowType(m_Type,m_Index);
   }

   RowType m_Type;
   CollectionIndexType m_Index;
};

GRID_IMPLEMENT_REGISTER(CBridgeDescFramingGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescFramingGrid

CBridgeDescFramingGrid::CBridgeDescFramingGrid():
   m_bDoValidate(true)
{
//   RegisterClass();
   HRESULT hr = m_objStation.CoCreateInstance(CLSID_Station);
   hr = m_objAngle.CoCreateInstance(CLSID_Angle);
   hr = m_objDirection.CoCreateInstance(CLSID_Direction);
}

CBridgeDescFramingGrid::~CBridgeDescFramingGrid()
{
}

BEGIN_MESSAGE_MAP(CBridgeDescFramingGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CBridgeDescFramingGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
   ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

std::vector<Float64> CBridgeDescFramingGrid::GetSpanLengths()
{
   std::vector<Float64> spanLengths;

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   SpanIndexType nSpans = pDlg->m_BridgeDesc.GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      spanLengths.push_back( pDlg->m_BridgeDesc.GetSpan(spanIdx)->GetSpanLength() );
   }

   return spanLengths;
}

std::vector<Float64> CBridgeDescFramingGrid::GetSegmentLengths()
{
   std::vector<Float64> segmentLengths;

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   SpanIndexType nSpans = pDlg->m_BridgeDesc.GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      CSpanData2* pSpan = pDlg->m_BridgeDesc.GetSpan(spanIdx);
      Float64 startSpanStation = pSpan->GetPrevPier()->GetStation();
      std::vector<CTemporarySupportData*> vTS = pSpan->GetTemporarySupports();
      for (const auto& pTS : vTS)
      {
         Float64 station = pTS->GetStation();
         Float64 segmentLength = station - startSpanStation;
         segmentLengths.push_back(segmentLength);
      }
   }

   return segmentLengths;
}

void CBridgeDescFramingGrid::SetPierOrientation(LPCTSTR strOrientation)
{
   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );


   PierIndexType nPiers = pDlg->m_BridgeDesc.GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      CPierData2* pPier = pDlg->m_BridgeDesc.GetPier(pierIdx);
      pPier->SetOrientation(strOrientation);
   }

   SupportIndexType nTS = pDlg->m_BridgeDesc.GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      CTemporarySupportData ts = *pDlg->m_BridgeDesc.GetTemporarySupport(tsIdx);
      ts.SetOrientation(strOrientation);
      pDlg->m_BridgeDesc.SetTemporarySupportByIndex(tsIdx,ts);
   }

   FillGrid(pDlg->m_BridgeDesc);
}

void CBridgeDescFramingGrid::SetSpanLengths(const std::vector<Float64>& spanLengths,PierIndexType fixedPierIdx)
{
   ROWCOL row = GetPierRow(fixedPierIdx);
   CPierData2* pFixedPierData = GetPierRowData(row);
   Float64 station = pFixedPierData->GetStation();

   // work backwards to the first pier
   PierIndexType idx = fixedPierIdx-1;
   while (0 <= idx && idx != ALL_PIERS)
   {
      station -= spanLengths[idx];
      idx--;
   }

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   // holding first pier constant
   CPierData2* pPrevPier = pDlg->m_BridgeDesc.GetPier(0);
   Float64 first_pier_station = pPrevPier->GetStation();
   Float64 pier_offset = station - first_pier_station; // this is the amount the bridge is moving... need to move temporary supports this amount too
   pPrevPier->SetStation( station );

   ASSERT(spanLengths.size() == pDlg->m_BridgeDesc.GetSpanCount());

   for (const auto& L : spanLengths)
   {
      CSpanData2* pNextSpan = pPrevPier->GetNextSpan();
      ASSERT(pNextSpan);

      std::vector<CTemporarySupportData*> vTS = pNextSpan->GetTemporarySupports();
      for (const auto& pTS : vTS)
      {
         Float64 tsStation = pTS->GetStation();
         tsStation += pier_offset;
         pTS->SetStation(tsStation);
      }

      CPierData2* pNextPier = pNextSpan->GetNextPier();
      pNextPier->SetStation( pPrevPier->GetStation() + L );

      pPrevPier = pNextPier;
   }

   FillGrid(pDlg->m_BridgeDesc);
}

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescFramingGrid message handlers

BOOL CBridgeDescFramingGrid::OnRButtonClickedRowCol(ROWCOL nRow, ROWCOL nCol, UINT nFlags, CPoint pt)
{
	 // unreferenced parameters
	 nRow, nCol, nFlags;

	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_FRAMING_GRID2_CONTEXT));

	CMenu* pPopup = menu.GetSubMenu( 0 );
	ASSERT( pPopup != nullptr );

   // deal with disabling delete since update stuff doesn't seem to work right
   UINT bCanDelete = EnableRemovePierBtn() ? MF_ENABLED|MF_BYCOMMAND : MF_GRAYED|MF_BYCOMMAND;
   pPopup->EnableMenuItem(IDC_REMOVE_PIER, bCanDelete);

   bCanDelete = EnableRemoveTemporarySupportBtn() ? MF_ENABLED|MF_BYCOMMAND : MF_GRAYED|MF_BYCOMMAND;
   pPopup->EnableMenuItem(IDC_REMOVE_TEMP_SUPPORT, bCanDelete);


	// display the menu
	ClientToScreen(&pt);
   CWnd* pParent = GetParent();
	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, pParent);

	// we processed the message
	return TRUE;
}

void CBridgeDescFramingGrid::OnChangedSelection(const CGXRange* pChangedRect,BOOL bIsDragging, BOOL bKey)
{
   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );
   pParent->EnableRemovePierBtn(EnableRemovePierBtn());
   pParent->EnableRemoveTemporarySupportBtn(EnableRemoveTemporarySupportBtn());

   CGXGridWnd::OnChangedSelection(pChangedRect,bIsDragging,bKey);
}

void CBridgeDescFramingGrid::SetDoNotValidateCells()
{
   m_bDoValidate = false;
}

void CBridgeDescFramingGrid::InsertRow()
{
	ROWCOL nRow = 0;
   nRow = GetRowCount()+1;
	nRow = Max((ROWCOL)1, nRow);

	InsertRows(nRow, 1);

   ScrollCellInView(nRow+1, GetLeftCol());
}

void CBridgeDescFramingGrid::OnAddTemporarySupport()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   CRowColArray sel_rows;
   ROWCOL nSelRows = GetSelectedRows(sel_rows);
   if ( nSelRows == 0 )
   {
      nSelRows = 1;
      sel_rows.Add(GetRowCount());
   }

   CTemporarySupportDlg dlg(&pDlg->m_BridgeDesc,INVALID_INDEX,pDlg->GetExtensionPages(),this);
   if ( dlg.DoModal() == IDOK )
   {
      pDlg->m_BridgeDesc = *dlg.GetBridgeDescription();
      FillGrid(pDlg->m_BridgeDesc);
   }
}

void CBridgeDescFramingGrid::OnAddPier()
{
   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   // add at end of bridge
   CInsertSpanDlg dlg(&pDlg->m_BridgeDesc);;
   if ( dlg.DoModal() == IDOK )
   {
      pDlg->m_BridgeDesc.InsertSpan(dlg.m_RefPierIdx,dlg.m_PierFace,dlg.m_SpanLength,nullptr,nullptr,dlg.m_bCreateNewGroup,dlg.m_EventIndex);
      FillGrid(pDlg->m_BridgeDesc);
   }
}

void CBridgeDescFramingGrid::OnRemovePier()
{
   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   PierIndexType nPiers = GetPierCount();

   CRowColArray sel_rows;
   ROWCOL nSelRows = GetSelectedRows(sel_rows);
   ATLASSERT(nSelRows == 1);
   for ( int r = nSelRows-1; r >= 0; r-- )
   {
      ROWCOL selRow = sel_rows[r];
      PierIndexType pierIdx = GetPierIndex(selRow);
      SpanIndexType spanIdx = GetSpanIndex(selRow);
      pgsTypes::RemovePierType removePier = pgsTypes::NextPier; // generally, we always remove the span and the right (next) pier
      if (pierIdx != INVALID_INDEX)
      {
         ATLASSERT(spanIdx == INVALID_INDEX);
         spanIdx = (SpanIndexType)pierIdx; // remove the next span
         if (spanIdx == 0)
         {
            // pier 0 is selected so we want to remove span 0 and pier 0 which is span 0 and it's left (prev) pier
            removePier = pgsTypes::PrevPier;
         }

         if (pierIdx == nPiers-1)
         {
            // the last pier is selected,
            // spanIdx is one too much... back up by one
            spanIdx--;

            // still want to remove the last pier, so that is the pier to the right (next) of the last span
            removePier = pgsTypes::NextPier;
         }
      }

      pDlg->m_BridgeDesc.RemoveSpan(spanIdx, removePier);
   }

   FillGrid(pDlg->m_BridgeDesc);
}

void CBridgeDescFramingGrid::OnRemoveTemporarySupport()
{
   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   CRowColArray sel_rows;
   ROWCOL nSelRows = GetSelectedRows(sel_rows);
   ATLASSERT(nSelRows == 1);
   for ( int r = nSelRows-1; r >= 0; r-- )
   {
      ROWCOL selRow = sel_rows[r];
      SupportIndexType tsIdx = GetTemporarySupportIndex(selRow);
      if ( 0 <= tsIdx)
      {
         pDlg->m_BridgeDesc.RemoveTemporarySupportByIndex(tsIdx);
      }
   }

   FillGrid(pDlg->m_BridgeDesc);
}

bool CBridgeDescFramingGrid::EnableRemovePierBtn()
{
	if (GetParam() == nullptr)
   {
		return false;
   }

   if ( GetPierCount() == 2 )
   {
      return false; // can't delete the last span
   }

   CRowColArray selRows;
   ROWCOL nSelRows  = GetSelectedRows(selRows);
   if ( nSelRows != 1 )
   {
      return false; // only one selection at a time
   }

   ROWCOL selRow = selRows.GetAt(0);
   SupportIndexType tsIdx = GetTemporarySupportIndex(selRow);
   if (tsIdx != INVALID_INDEX)
   {
      // a temporary support row is selected
      return false;
   }

   SpanIndexType spanIdx = GetSpanIndex(selRow);
   PierIndexType pierIdx = GetPierIndex(selRow);

   if ( spanIdx == INVALID_INDEX && pierIdx == INVALID_INDEX )
   {
      return false;
   }

   return true;
}

bool CBridgeDescFramingGrid::EnableRemoveTemporarySupportBtn()
{
	if (GetParam() == nullptr)
   {
		return false;
   }

   CRowColArray selRows;
   ROWCOL nSelRows  = GetSelectedRows(selRows);
   if ( nSelRows != 1 )
   {
      return false; // only one selection at a time
   }

   ROWCOL selRow = selRows.GetAt(0);
   SupportIndexType tsIdx = GetTemporarySupportIndex(selRow);

   if ( tsIdx == INVALID_INDEX )
   {
      return false;
   }

   return true;
}

void CBridgeDescFramingGrid::GetTransactions(CEAFMacroTxn& macro)
{
   std::for_each(std::begin(m_PierTransactions), std::end(m_PierTransactions),
      [&macro](auto& txns)
      {
         std::for_each(std::begin(txns.second), std::end(txns.second), [&macro](auto& txn) {macro.AddTransaction(std::move(txn)); });
      }
   );

   m_PierTransactions.clear();

   std::for_each(std::begin(m_SpanTransactions), std::end(m_SpanTransactions), 
      [&macro](auto& txns)
      {
         std::for_each(std::begin(txns.second), std::end(txns.second), [&macro](auto& txn) {macro.AddTransaction(std::move(txn)); });
      }
   );
 
   m_SpanTransactions.clear();

   std::for_each(std::begin(m_TempSupportTransactions), std::end(m_TempSupportTransactions), 
      [&macro](auto& txns)
      {
         std::for_each(std::begin(txns.second), std::end(txns.second), [&macro](auto& txn) {macro.AddTransaction(std::move(txn)); });
      }
   );

   m_TempSupportTransactions.clear();
}

void CBridgeDescFramingGrid::CustomInit()
{
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );

	GetParam( )->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   SetMergeCellsMode(gxnMergeDelayEval);

   const int num_rows = 0;
   const int num_cols = 6;

	SetRowCount(num_rows);
	SetColCount(num_cols);

		// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE & ~GX_SELMULTIPLE & ~GX_SELSHIFT & ~GX_SELKEYBOARD));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

// set text along top row
   int col = 0;
	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   CString cv = _T("Station");
	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv = _T("Orientation");
	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Segments"))
		);

	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
			.SetValue(_T("Piers\\Spans"))
      );

	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
			.SetValue(_T("Piers\\Spans"))
		);

	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Temp Support"))
		);

   // make it so that text fits correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();
	SetFocus();

   GetParam()->SetLockReadOnly(TRUE);
	GetParam( )->EnableUndo(TRUE);
}

CString CBridgeDescFramingGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

CPierData2* CBridgeDescFramingGrid::GetPierRowData(ROWCOL nRow)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   PierIndexType pierIdx = GetPierIndex(nRow);
   ASSERT( pierIdx != ALL_PIERS );

   CPierData2* pPier = pDlg->m_BridgeDesc.GetPier(pierIdx);

   // update pier using the data in the grid

   // Station
   CString strStation = GetCellValue(nRow,1);
   UnitModeType unitMode = (UnitModeType)(pDisplayUnits->GetUnitMode());
   m_objStation->FromString(CComBSTR(strStation),unitMode);
   Float64 station;
   m_objStation->get_Value(&station);
   station = WBFL::Units::ConvertToSysUnits(station,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);
   pPier->SetStation(station);

   // Orientation
   CString strOrientation = GetCellValue(nRow,2);
   strOrientation.MakeUpper();
   strOrientation.TrimLeft();
   strOrientation.TrimRight();
   if ( strOrientation.GetLength() == 1 && strOrientation.GetAt(0) == 'N')
   {
      // if user input N for normal, fill out the whole word
      strOrientation = _T("NORMAL");
   }
   pPier->SetOrientation( strOrientation );

   return pPier;
}

CTemporarySupportData CBridgeDescFramingGrid::GetTemporarySupportRowData(ROWCOL nRow)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   SupportIndexType tsIdx = GetTemporarySupportIndex(nRow);
   ASSERT( tsIdx != INVALID_INDEX );

   CTemporarySupportData tsData = *pDlg->m_BridgeDesc.GetTemporarySupport(tsIdx);

   // update temporary support using the data in the grid

   // Station
   CString strStation = GetCellValue(nRow,1);
   UnitModeType unitMode = (UnitModeType)(pDisplayUnits->GetUnitMode());
   m_objStation->FromString(CComBSTR(strStation),unitMode);
   Float64 station;
   m_objStation->get_Value(&station);
   station = WBFL::Units::ConvertToSysUnits(station,pDisplayUnits->GetAlignmentLengthUnit().UnitOfMeasure);
   tsData.SetStation(station);

   // Orientation
   CString strOrientation = GetCellValue(nRow,2);
   strOrientation.MakeUpper();
   strOrientation.TrimLeft();
   strOrientation.TrimRight();
   if ( strOrientation.GetLength() == 1 && strOrientation.GetAt(0) == 'N')
   {
      // if user input N for normal, fill out the whole word
      strOrientation = _T("NORMAL");
   }
   tsData.SetOrientation( strOrientation );

   return tsData;
}

void CBridgeDescFramingGrid::FillGrid(const CBridgeDescription2& bridgeDesc)
{
	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   // remove all rows
   ROWCOL rows = GetRowCount();
   if ( 1 < rows )
   {
      RemoveRows(1, rows, GX_INVALIDATE);
   }

   const CPierData2* pPier = bridgeDesc.GetPier(0);

   ROWCOL row = 1;

   // first pier
   InsertRow();
   FillPierRow(row++,pPier);

   Float64 last_station = pPier->GetStation();

   PierIndexType nPiers = bridgeDesc.GetPierCount();
   SupportIndexType nTS = bridgeDesc.GetTemporarySupportCount();

   PierIndexType pierIdx = 1;
   SupportIndexType tsIdx = 0;

   while ( pierIdx < nPiers )
   {
      pPier = bridgeDesc.GetPier(pierIdx);
      Float64 next_pier_station = pPier->GetStation();

      while ( tsIdx < nTS && bridgeDesc.GetTemporarySupport(tsIdx)->GetStation() < next_pier_station )
      {
         // temporary support is before the next pier... add it
         const CTemporarySupportData* pTS = bridgeDesc.GetTemporarySupport(tsIdx);
         InsertRow();
         Float64 ts_station = pTS->GetStation();
         FillSegmentRow(row++);
         last_station = ts_station;

         InsertRow();
         FillTemporarySupportRow(row++,pTS);

         tsIdx++;
      }

      InsertRow();
      FillSegmentRow(row++);
      last_station = pPier->GetStation();

      InsertRow();
      FillPierRow(row++,pPier);

      pierIdx++;
   }

   FillSegmentColumn();
   FillSpanColumn();

   HideCols(6,6,nTS == 0 ? TRUE : FALSE);

   ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),4));

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CBridgeDescFramingGrid::FillPierRow(ROWCOL row,const CPierData2* pPierData)
{
	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   CString strStation = FormatStation(pDisplayUnits->GetStationFormat(),pPierData->GetStation());

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();

   CString strPierLabel = pParent->GetPierLabel(pPierData->IsAbutment(), pPierData->GetIndex());

   ROWCOL col = 0;

   // left column header
   SetStyleRange(CGXRange(row,col++), CGXStyle()
      .SetHorizontalAlignment(DT_LEFT)
      .SetValue(strPierLabel)
      .SetUserAttribute(0,CRowType(CRowType::Pier,pPierData->GetIndex()))
   );

   // station
   SetStyleRange(CGXRange(row,col++), CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetReadOnly(FALSE)
      .SetEnabled(TRUE)
      .SetValue(strStation)
   );

   // orientation
   SetStyleRange(CGXRange(row,col++), CGXStyle()
        .SetHorizontalAlignment(DT_RIGHT)
        .SetInterior(::GetSysColor(COLOR_WINDOW))
        .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
        .SetReadOnly(FALSE)
        .SetEnabled(TRUE)
        .SetValue(pPierData->GetOrientation())
      );

   col++; // segment column
   col++; // spans column
   col++; // spans column

   // temp support
   SetStyleRange(CGXRange(row,col++), CGXStyle()
            .SetEnabled(FALSE)
            .SetReadOnly(TRUE)
            .SetInterior( ::GetSysColor(COLOR_BTNFACE) )
            .SetTextColor( ::GetSysColor(COLOR_WINDOWTEXT) )
         );

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CBridgeDescFramingGrid::FillTemporarySupportRow(ROWCOL row,const CTemporarySupportData* pTSData)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   ROWCOL col = 0;

   // label
   CString strLabel;
   strLabel.Format(_T("TS %d"),LABEL_TEMPORARY_SUPPORT(pTSData->GetIndex()));
   SetStyleRange(CGXRange(row,col++),CGXStyle()
      .SetValue(strLabel)
      .SetUserAttribute( 0, CRowType(CRowType::TempSupport,pTSData->GetIndex()) )
      );

   // station
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CString strStation = FormatStation(pDisplayUnits->GetStationFormat(),pTSData->GetStation());
   SetStyleRange(CGXRange(row,col++), CGXStyle()
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(strStation)
   );

   // orientation
   CGXStyle style;
   style.SetHorizontalAlignment(DT_RIGHT);
   style.SetInterior(::GetSysColor(COLOR_WINDOW))
        .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
        .SetReadOnly(FALSE)
        .SetEnabled(TRUE)
        .SetValue(pTSData->GetOrientation());
   
   SetStyleRange(CGXRange(row,col++), style );

   col++; // segment column
   col++; // spans column
   col++; // spans column

   // edit button
   SetStyleRange(CGXRange(row,col++), CGXStyle()
			.SetControl(GX_IDS_CTRL_PUSHBTN)
			.SetChoiceList(_T("Edit"))
         .SetHorizontalAlignment(DT_LEFT)
         .SetEnabled(TRUE)
         );

   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(TRUE);
}

void CBridgeDescFramingGrid::FillSegmentRow(ROWCOL row)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   SetStyleRange(CGXRange(row,0,row,3), CGXStyle()
      .SetHorizontalAlignment(DT_CENTER)
      .SetEnabled(FALSE)
      .SetReadOnly(TRUE)
      .SetInterior( ::GetSysColor(COLOR_BTNFACE) )
      .SetTextColor( ::GetSysColor(COLOR_WINDOWTEXT) )
      .SetValue(_T(""))
   );

   // temp support
   SetStyleRange(CGXRange(row,6), CGXStyle()
            .SetEnabled(FALSE)
            .SetReadOnly(TRUE)
            .SetInterior( ::GetSysColor(COLOR_BTNFACE) )
            .SetTextColor( ::GetSysColor(COLOR_WINDOWTEXT) )
         );

   GetParam()->SetLockReadOnly(TRUE);
   GetParam()->EnableUndo(TRUE);
}

void CBridgeDescFramingGrid::FillSegmentColumn()
{
   ROWCOL col = 3;

   // This column not seen for PGSuper
   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      HideCols(col,col);
   }

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   GroupIndexType nGroups = pDlg->m_BridgeDesc.GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CSplicedGirderData* pGirder = pDlg->m_BridgeDesc.GetGirderGroup(grpIdx)->GetGirder(0);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);

         ROWCOL startRow;
         CString strStartSupportLabel;
         Float64 startStation;
         if ( pSegment->GetClosureJoint(pgsTypes::metStart) )
         {
            if ( pSegment->GetClosureJoint(pgsTypes::metStart)->GetTemporarySupport() )
            {
               startRow = GetTemporarySupportRow(pSegment->GetClosureJoint(pgsTypes::metStart)->GetTemporarySupport()->GetIndex());
               if (pSegment->GetClosureJoint(pgsTypes::metStart)->GetTemporarySupport()->GetSupportType() == pgsTypes::ErectionTower)
               {
                  strStartSupportLabel = _T("Erection Tower");
               }
               else
               {
                  strStartSupportLabel = _T("Strongback");
               }
               startStation = pSegment->GetClosureJoint(pgsTypes::metStart)->GetTemporarySupport()->GetStation();
            }
            else
            {
               startRow = GetPierRow(pSegment->GetClosureJoint(pgsTypes::metStart)->GetPier()->GetIndex());
               // parent builds label
               strStartSupportLabel = pParent->GetPierLabel(pSegment->GetClosureJoint(pgsTypes::metStart)->GetPier()->IsAbutment(),pSegment->GetClosureJoint(pgsTypes::metStart)->GetPier()->GetIndex());
               startStation = pSegment->GetClosureJoint(pgsTypes::metStart)->GetPier()->GetStation();
            }
         }
         else
         {
            startRow = GetPierRow(pGirder->GetPier(pgsTypes::metStart)->GetIndex());
            strStartSupportLabel = pParent->GetPierLabel(pGirder->GetPier(pgsTypes::metStart)->IsAbutment(), pGirder->GetPier(pgsTypes::metStart)->GetIndex());
            startStation = pGirder->GetPier(pgsTypes::metStart)->GetStation();
         }

         ROWCOL endRow;
         CString strEndSupportLabel;
         Float64 endStation;
         if ( pSegment->GetClosureJoint(pgsTypes::metEnd) )
         {
            if ( pSegment->GetClosureJoint(pgsTypes::metEnd)->GetTemporarySupport() )
            {
               endRow = GetTemporarySupportRow(pSegment->GetClosureJoint(pgsTypes::metEnd)->GetTemporarySupport()->GetIndex());
               if (pSegment->GetClosureJoint(pgsTypes::metEnd)->GetTemporarySupport()->GetSupportType() == pgsTypes::ErectionTower)
               {
                  strEndSupportLabel = _T("Erection Tower");
               }
               else
               {
                  strEndSupportLabel = _T("Strongback");
               }
               endStation = pSegment->GetClosureJoint(pgsTypes::metEnd)->GetTemporarySupport()->GetStation();
            }
            else
            {
               endRow = GetPierRow(pSegment->GetClosureJoint(pgsTypes::metEnd)->GetPier()->GetIndex());
               strEndSupportLabel = pParent->GetPierLabel(pSegment->GetClosureJoint(pgsTypes::metEnd)->GetPier()->IsAbutment(),pSegment->GetClosureJoint(pgsTypes::metEnd)->GetPier()->GetIndex());
               endStation = pSegment->GetClosureJoint(pgsTypes::metEnd)->GetPier()->GetStation();
            }
         }
         else
         {
            endRow = GetPierRow(pGirder->GetPier(pgsTypes::metEnd)->GetIndex());
            strEndSupportLabel = pParent->GetPierLabel(pGirder->GetPier(pgsTypes::metEnd)->IsAbutment(), pGirder->GetPier(pgsTypes::metEnd)->GetIndex());
            endStation = pGirder->GetPier(pgsTypes::metEnd)->GetStation();
         }

         SetStyleRange(CGXRange(startRow,col), CGXStyle()
            .SetEnabled(FALSE)
            .SetReadOnly(TRUE)
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
            .SetInterior( ::GetSysColor(COLOR_BTNFACE) )
            .SetTextColor( ::GetSysColor(COLOR_WINDOWTEXT) )
            .SetValue(strStartSupportLabel)
         );

         SetStyleRange(CGXRange(endRow,col), CGXStyle()
            .SetEnabled(FALSE)
            .SetReadOnly(TRUE)
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
            .SetInterior( ::GetSysColor(COLOR_BTNFACE) )
            .SetTextColor( ::GetSysColor(COLOR_WINDOWTEXT) )
            .SetValue(strEndSupportLabel)
         );

         Float64 segmentLength = endStation - startStation;
         CString strSegmentLength;
         strSegmentLength.Format(_T("%s"),FormatDimension(segmentLength,pDisplayUnits->GetSpanLengthUnit()));
         for ( ROWCOL row = startRow+1; row < endRow; row++ )
         {
            SetStyleRange(CGXRange(row,col), CGXStyle()
               .SetEnabled(FALSE)
               .SetReadOnly(TRUE)
               .SetHorizontalAlignment(DT_CENTER)
               .SetVerticalAlignment(DT_VCENTER)
               .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
               .SetInterior( ::GetSysColor(COLOR_BTNFACE) )
               .SetTextColor( segmentLength <= 0 ? RED : ::GetSysColor(COLOR_WINDOWTEXT) )
               .SetValue(strSegmentLength)
            );
         }
      }
   }

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CBridgeDescFramingGrid::FillSpanColumn()
{
   ROWCOL col = 4;

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   pParent->EnableInsertSpanBtn(TRUE);
   pParent->EnableInsertTempSupportBtn(TRUE);

   SpanIndexType nSpans = pDlg->m_BridgeDesc.GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      const CSpanData2* pSpan = pDlg->m_BridgeDesc.GetSpan(spanIdx);
      const CPierData2* pPrevPier = pSpan->GetPrevPier();
      const CPierData2* pNextPier = pSpan->GetNextPier();
      
      // Get row values for prev and next pier
      ROWCOL prevPierRow = GetPierRow(pPrevPier->GetIndex());
      ROWCOL nextPierRow = GetPierRow(pNextPier->GetIndex());

      // create prev and next pier labels
      CString strPrevPierLabel = pParent->GetPierLabel(pPrevPier->IsAbutment(),pPrevPier->GetIndex());
      CString strNextPierLabel = pParent->GetPierLabel(pNextPier->IsAbutment(),pNextPier->GetIndex());

      // label prev pier
      SetStyleRange(CGXRange(prevPierRow,col), CGXStyle()
         .SetEnabled(FALSE)
         .SetReadOnly(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetInterior( ::GetSysColor(COLOR_BTNFACE) )
         .SetTextColor( ::GetSysColor(COLOR_WINDOWTEXT) )
         .SetValue(strPrevPierLabel)
      );

      // prev pier edit button
      SetStyleRange(CGXRange(prevPierRow,col+1), CGXStyle()
			.SetControl(GX_IDS_CTRL_PUSHBTN)
			.SetChoiceList(_T("Edit"))
         .SetHorizontalAlignment(DT_LEFT)
         .SetEnabled(TRUE)
      );
      
      // label next pier
      SetStyleRange(CGXRange(nextPierRow,col), CGXStyle()
         .SetEnabled(FALSE)
         .SetReadOnly(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
         .SetInterior( ::GetSysColor(COLOR_BTNFACE) )
         .SetTextColor( ::GetSysColor(COLOR_WINDOWTEXT) )
         .SetValue(strNextPierLabel)
      );
      
      // next pier edit button
      SetStyleRange(CGXRange(nextPierRow,col+1), CGXStyle()
			.SetControl(GX_IDS_CTRL_PUSHBTN)
			.SetChoiceList(_T("Edit"))
         .SetHorizontalAlignment(DT_LEFT)
         .SetEnabled(TRUE)
      );

      // fill in all rows between prev and next pier with span length
      Float64 spanLength = pNextPier->GetStation() - pPrevPier->GetStation();
      CString strSpanLength;
      strSpanLength.Format(_T("%s"),FormatDimension(spanLength,pDisplayUnits->GetSpanLengthUnit()));
      for (ROWCOL row = prevPierRow + 1; row < nextPierRow; row++)
      {
         SetStyleRange(CGXRange(row, col), CGXStyle()
            .SetEnabled(FALSE)
            .SetReadOnly(TRUE)
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
            .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
            .SetInterior(::GetSysColor(COLOR_BTNFACE))
            .SetTextColor(spanLength <= 0 ? RED : ::GetSysColor(COLOR_WINDOWTEXT))
            .SetValue(strSpanLength)
         );
      }

      // enable edit button for pier
      SetStyleRange(CGXRange(prevPierRow, col + 1), CGXStyle()
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetEnabled(TRUE)
      );

      SetStyleRange(CGXRange(prevPierRow+1,col+1,nextPierRow-1,col+1), CGXStyle()
         .SetMergeCell(GX_MERGE_VERTICAL | GX_MERGE_COMPVALUE)
         .SetControl(GX_IDS_CTRL_PUSHBTN)
			.SetChoiceList(_T("Edit"))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetHorizontalAlignment(DT_LEFT)
         .SetEnabled(TRUE)
         .SetValue(0L)
         .SetUserAttribute(0, CRowType(CRowType::Span, pPrevPier->GetNextSpan()->GetIndex()))
      );

      // enable edit button for pier
      SetStyleRange(CGXRange(nextPierRow, col + 1), CGXStyle()
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetEnabled(TRUE)
      );
   }

   SupportIndexType nTS = pDlg->m_BridgeDesc.GetTemporarySupportCount();
   for (SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++)
   {
      ROWCOL tsRow = GetTemporarySupportRow(tsIdx);
      SetStyleRange(CGXRange(tsRow, col + 2), CGXStyle()
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetEnabled(TRUE)
      );
   }

   if (!pDlg->m_BridgeDesc.IsValidLayout())
   {
      // disable edit buttons because bridge is not valid
      pParent->EnableInsertSpanBtn(FALSE);
      pParent->EnableInsertTempSupportBtn(FALSE);

      SupportIndexType nTS = pDlg->m_BridgeDesc.GetTemporarySupportCount();
      for (SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++)
      {
         ROWCOL tsRow = GetTemporarySupportRow(tsIdx);
         SetStyleRange(CGXRange(tsRow, col + 2), CGXStyle()
            .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
            .SetEnabled(FALSE)
         );
      }

      for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++)
      {
         const CSpanData2* pSpan = pDlg->m_BridgeDesc.GetSpan(spanIdx);
         const CPierData2* pPrevPier = pSpan->GetPrevPier();
         const CPierData2* pNextPier = pSpan->GetNextPier();

         // Get row values for prev and next pier
         ROWCOL prevPierRow = GetPierRow(pPrevPier->GetIndex());
         ROWCOL nextPierRow = GetPierRow(pNextPier->GetIndex());

         for (ROWCOL r = prevPierRow; r <= nextPierRow; r++)
         {
            SetStyleRange(CGXRange(r, col + 1), CGXStyle()
               .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
               .SetEnabled(FALSE)
            );
         }
      }
   }

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CBridgeDescFramingGrid::OnClickedButtonRowCol(ROWCOL nRow,ROWCOL nCol)
{
   if ( nCol != 5 && nCol != 6 )
   {
      return;
   }

   CGXStyle style1, style2;
   GetStyleRowCol(nRow, 0, style1); // piers and temp support info is in column 0
   GetStyleRowCol(nRow, 5, style2); // span info is in column 5 (if we put span info  in col 0, it would override temp support info)

   BOOL bPierOrTS = style1.GetIncludeUserAttribute(0);
   BOOL bSpan = style2.GetIncludeUserAttribute(0);
   if (bSpan && nCol == 5)
   {
      const CRowType& rowType = dynamic_cast<const CRowType&>(style2.GetUserAttribute(0));
      ATLASSERT(rowType.m_Type == CRowType::Span);
      EditSpan(rowType.m_Index);
   }
   else if (bPierOrTS)
   {
      const CRowType& rowType = dynamic_cast<const CRowType&>(style1.GetUserAttribute(0));
      if (nCol == 5)
      {
         ATLASSERT(rowType.m_Type == CRowType::Pier);
         EditPier(rowType.m_Index);
      }
      else
      {
         ATLASSERT(nCol == 6);
         ATLASSERT(rowType.m_Type == CRowType::TempSupport);
         EditTemporarySupport(rowType.m_Index);
      }
   }
   else
   {
      ATLASSERT(false);
   }
}

// validate input
BOOL CBridgeDescFramingGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
{
   if (!m_bDoValidate)
   {
      // User hit Cancel button - just let any input errors go
      return TRUE;
   }

	CString s;
	CGXControl* pControl = GetControl(nRow, nCol);
	pControl->GetCurrentText(s);

	if (nCol==1)
	{
      // station
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      HRESULT hr = m_objStation->FromString( CComBSTR(s), (UnitModeType)(pDisplayUnits->GetUnitMode()));
      if ( FAILED(hr) )
      {
			SetWarningText (_T("Invalid Station Value"));
         DisplayWarningText();
         // Don't let focus move outside of grid. See Mantis 1090
         SetFocus();

         return FALSE;
      }
      else
      {
         return TRUE;
      }
   }
	else if (nCol==2)
	{
         // orientation
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IValidate,pValidate);
      UINT result = pValidate->Orientation(s);

      if (result == VALIDATE_INVALID)
      {
			SetWarningText (_T("Orientation is invalid"));
         DisplayWarningText();
         // Don't let focus move outside of grid. See Mantis 1090
         SetFocus();

         return FALSE;
      }
      else if ( result == VALIDATE_SKEW_ANGLE )
      {
		   SetWarningText (_T("Skew angle must be less than 88 deg"));
         DisplayWarningText();
         // Don't let focus move outside of grid. See Mantis 1090
         SetFocus();

         return FALSE;
      }
   }

	return CGXGridWnd::OnValidateCell(nRow, nCol);
}

BOOL CBridgeDescFramingGrid::OnEndEditing(ROWCOL nRow,ROWCOL nCol)
{
   if ( nCol == 1 || nCol == 2 )
   {
      // Pier or temporary support station or orientation changed... 

      CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
      ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

      CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
      ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

      PierIndexType pierIdx = GetPierIndex(nRow);
      if ( pierIdx != INVALID_INDEX )
      {
         // it was the pier that changed
         CPierData2* pPierData = GetPierRowData(nRow);
         FillPierRow(nRow,pPierData); // updates station and orientation formatting
      }
      else
      {
         // it was the temporary support that changed

         if (pDlg->m_BridgeDesc.IsValidBridge())
         {
            // temporary support station could have changed... update all temporary support rows
            // from where the ts used to be to where it is now

            SupportIndexType currTsIdx = GetTemporarySupportIndex(nRow);
            CTemporarySupportData tsData = GetTemporarySupportRowData(nRow);

            if (tsData.GetStation() <= pDlg->m_BridgeDesc.GetPier(0)->GetStation() || pDlg->m_BridgeDesc.GetPier(pDlg->m_BridgeDesc.GetPierCount() - 1)->GetStation() <= tsData.GetStation())
            {
               // new station is not on the bridge
               CComPtr<IBroker> pBroker;
               EAFGetBroker(&pBroker);
               GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);
               CString strStation = FormatStation(pDisplayUnits->GetStationFormat(), tsData.GetStation());
               m_sWarningText.Format(_T("Station %s is not on the bridge"), strStation);
               return FALSE;
            }

            // update the bridge model with the new tempoary support data. the bridge model will give us back
            // the new index of the temporary support (the TS could have moved so it might have a new index)
            // Update all the temporary support rows in the grid based on the updated bridge model
            SupportIndexType newTsIdx = pDlg->m_BridgeDesc.SetTemporarySupportByIndex(currTsIdx, tsData);
            for (SupportIndexType tsIdx = Min(newTsIdx, currTsIdx); tsIdx <= Max(newTsIdx, currTsIdx); tsIdx++)
            {
               const CTemporarySupportData* pTS = pDlg->m_BridgeDesc.GetTemporarySupport(tsIdx);
               ROWCOL tsRow = GetTemporarySupportRow(tsIdx);
               FillTemporarySupportRow(tsRow, pTS); // updates station and orientation formatting
            }
         }
      }

      FillSpanColumn();   // updates span lengths
      FillSegmentColumn(); // update segment lengths

      ResizeColWidthsToFit(CGXRange(0, 0, GetRowCount(), 4));
   }
   return CGXGridWnd::OnEndEditing(nRow,nCol);
}

SpanIndexType CBridgeDescFramingGrid::GetSelectedSpan()
{
   CRowColArray selRows;
   ROWCOL nSelRows = GetSelectedRows(selRows);
   if (nSelRows != 1)
   {
      return INVALID_INDEX; // only one selection at a time
   }

   ROWCOL selRow = selRows.GetAt(0);
   SpanIndexType spanIdx = GetSpanIndex(selRow);
   return spanIdx;
}

PierIndexType CBridgeDescFramingGrid::GetSelectedPier()
{
   CRowColArray selRows;
   ROWCOL nSelRows  = GetSelectedRows(selRows);
   if ( nSelRows != 1 )
   {
      return INVALID_INDEX; // only one selection at a time
   }

   ROWCOL selRow = selRows.GetAt(0);
   PierIndexType pierIdx = GetPierIndex(selRow);
   return pierIdx;
}

PierIndexType CBridgeDescFramingGrid::GetPierCount()
{
   ROWCOL nRows = GetRowCount();
   PierIndexType nPiers = 0;
   for ( ROWCOL row = 1; row <= nRows; row++ )
   {
      CGXStyle style;
      GetStyleRowCol(row,0,style);
      if ( style.GetIncludeUserAttribute(0) )
      {
         const CRowType& rowType = dynamic_cast<const CRowType&>(style.GetUserAttribute(0));

         if ( rowType.m_Type == CRowType::Pier )
         {
            nPiers++;
         }
      }
   }

   return nPiers;
}

BOOL CBridgeDescFramingGrid::CanActivateGrid(BOOL bActivate)
{
   if ( !bActivate )
   {
      // make sure all the grid data is active
      std::vector<Float64> spanLengths = GetSpanLengths();
      for (const auto& L : spanLengths)
      {
         if ( L <= 0 )
         {
   			AfxMessageBox(_T("Invalid Span Length"));
            return FALSE;
         }
      }

      std::vector<Float64> segmentLengths = GetSegmentLengths();
      for (const auto& L : segmentLengths)
      {
         if ( L <= 0 )
         {
   			AfxMessageBox(_T("Invalid Temporary Support Location"));
            return FALSE;
         }
      }

   }

   return CGXGridWnd::CanActivateGrid(bActivate);
}

void CBridgeDescFramingGrid::EditPier(PierIndexType pierIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   // This dialog makes a copy of the bridge model because it changes it.
   // If the user presses the Cancel button, we don't have to figure out
   // what got changed.
   CPierDetailsDlg dlg(&pDlg->m_BridgeDesc,pierIdx,pDlg->GetExtensionPages());
   if ( dlg.DoModal() == IDOK )
   {
      pDlg->m_BridgeDesc = *dlg.GetBridgeDescription();
      auto pPierTransaction = dlg.GetExtensionPageTransaction();
      SavePierTransaction(pierIdx,std::move(pPierTransaction));
      FillGrid(pDlg->m_BridgeDesc);
   }
}

void CBridgeDescFramingGrid::EditSpan(SpanIndexType spanIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );

   // This dialog makes a copy of the bridge model because it changes it.
   // If the user presses the Cancel button, we don't have to figure out
   // what got changed.
   CSpanDetailsDlg dlg(&pDlg->m_BridgeDesc,spanIdx,pDlg->GetExtensionPages());

   if ( dlg.DoModal() == IDOK )
   {
      pDlg->m_BridgeDesc = *dlg.GetBridgeDescription();
      FillGrid(pDlg->m_BridgeDesc);
   }
}

void CBridgeDescFramingGrid::EditTemporarySupport(SupportIndexType tsIdx)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CBridgeDescFramingPage* pParent = (CBridgeDescFramingPage*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescFramingPage) ) );

   CBridgeDescDlg* pDlg = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pDlg->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg) ) );


   CTemporarySupportDlg dlg(&pDlg->m_BridgeDesc,tsIdx,pDlg->GetExtensionPages(),this);
   if ( dlg.DoModal() == IDOK )
   {
      pDlg->m_BridgeDesc = *dlg.GetBridgeDescription();
      FillGrid(pDlg->m_BridgeDesc);
   }
}

ROWCOL CBridgeDescFramingGrid::GetPierRow(PierIndexType pierIdx)
{
   ROWCOL nRows = GetRowCount();
   for (ROWCOL row = 1; row <= nRows; row++ )
   {
      if ( GetPierIndex(row) == pierIdx )
      {
         return row;
      }
   }

   return 1;
}

PierIndexType CBridgeDescFramingGrid::GetPierIndex(ROWCOL nRow)
{
   CGXStyle style;
   GetStyleRowCol(nRow,0,style);

   if ( style.GetIncludeUserAttribute(0) )
   {
      const CRowType& rowType = dynamic_cast<const CRowType&>(style.GetUserAttribute(0));
      if ( rowType.m_Type == CRowType::Pier )
      {
         return rowType.m_Index;
      }
      else
      {
         return INVALID_INDEX;
      }
   }
   else
   {
      return INVALID_INDEX;
   }
}

SpanIndexType CBridgeDescFramingGrid::GetSpanIndex(ROWCOL nRow)
{
   CGXStyle style;
   ROWCOL col = 5;
   GetStyleRowCol(nRow, col, style);

   if (style.GetIncludeUserAttribute(0))
   {
      const CRowType& rowType = dynamic_cast<const CRowType&>(style.GetUserAttribute(0));
      if (rowType.m_Type == CRowType::Span)
      {
         return rowType.m_Index;
      }
      else
      {
         return INVALID_INDEX;
      }
   }
   else
   {
      return INVALID_INDEX;
   }
}

ROWCOL CBridgeDescFramingGrid::GetTemporarySupportRow(SupportIndexType tsIdx)
{
   ROWCOL nRows = GetRowCount();
   for (ROWCOL row = 1; row <= nRows; row++ )
   {
      if ( GetTemporarySupportIndex(row) == tsIdx )
      {
         return row;
      }
   }

   return 1;
}

SupportIndexType CBridgeDescFramingGrid::GetTemporarySupportIndex(ROWCOL nRow)
{
   CGXStyle style;
   GetStyleRowCol(nRow,0,style);

   if ( style.GetIncludeUserAttribute(0) )
   {
      const CRowType& rowType = dynamic_cast<const CRowType&>(style.GetUserAttribute(0));
      if ( rowType.m_Type == CRowType::TempSupport )
      {
         return rowType.m_Index;
      }
   }

   return INVALID_INDEX;
}

void CBridgeDescFramingGrid::SavePierTransaction(PierIndexType pierIdx,std::unique_ptr<CEAFTransaction>&& pTxn)
{
   if ( pTxn == nullptr )
   {
      return;
   }

   auto found(m_PierTransactions.find(pierIdx));
   if ( found == m_PierTransactions.end())
   {
      m_PierTransactions.insert(std::make_pair(pierIdx,std::vector<std::unique_ptr<CEAFTransaction>>()));
      found = m_PierTransactions.find(pierIdx);
   }
   found->second.emplace_back(std::move(pTxn));
}

void CBridgeDescFramingGrid::SaveSpanTransaction(SpanIndexType spanIdx, std::unique_ptr<CEAFTransaction>&& pTxn)
{
   if ( pTxn == nullptr )
   {
      return;
   }

   auto found(m_SpanTransactions.find(spanIdx));
   if ( found == m_SpanTransactions.end())
   {
      m_SpanTransactions.insert(std::make_pair(spanIdx, std::vector<std::unique_ptr<CEAFTransaction>>()));
      found = m_SpanTransactions.find(spanIdx);
   }
   found->second.emplace_back(std::move(pTxn));
}

void CBridgeDescFramingGrid::SaveTemporarySupportTransaction(SupportIndexType tsIdx, std::unique_ptr<CEAFTransaction>&& pTxn)
{
   if ( pTxn == nullptr )
   {
      return;
   }

   auto found(m_TempSupportTransactions.find(tsIdx));
   if ( found == m_TempSupportTransactions.end())
   {
      m_TempSupportTransactions.insert(std::make_pair(tsIdx,std::vector<std::unique_ptr<CEAFTransaction>>()));
      found = m_TempSupportTransactions.find(tsIdx);
   }
   found->second.emplace_back(std::move(pTxn));
}

void CBridgeDescFramingGrid::OnKillFocus(CWnd* pNewWnd)
{
   CWnd* pParent = GetParent();
   CWnd* pGpParent = pParent->GetParent();
   CWnd* pcancel = pGpParent->GetDlgItem(IDCANCEL);
   if (pNewWnd == pcancel)
   {
      // do not validate cell values if user hits Cancel
      m_bDoValidate = false;
   }

   CGXGridWnd::OnKillFocus(pNewWnd);
}
