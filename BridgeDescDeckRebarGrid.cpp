///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

// BridgeDescDeckRebarGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "BridgeDescDeckRebarGrid.h"
#include "PGSuperDoc.h"
#include "BridgeDescDlg.h"
#include "BridgeDescDeckReinforcementPage.h"
#include <Units\Measure.h>

#include <EAF\EAFDisplayUnits.h>

#include <algorithm>

#include <LRFD\RebarPool.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDeckRebarGrid

CBridgeDescDeckRebarGrid::CBridgeDescDeckRebarGrid()
{
   m_bEnableTopMat    = TRUE;
   m_bEnableBottomMat = TRUE;
   m_nContinuousPiers = 0;
}

CBridgeDescDeckRebarGrid::~CBridgeDescDeckRebarGrid()
{
}

BEGIN_MESSAGE_MAP(CBridgeDescDeckRebarGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(BridgeDescDeckRebarGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDeckRebarGrid message handlers

int CBridgeDescDeckRebarGrid::GetColWidth(ROWCOL nCol)
{
	CRect rect = GetGridRect( );

   if ( nCol == 0 )
   {
      return rect.Width()/18;
   }
   else if ( nCol == 4 )
   {
      return 3*rect.Width()/18;
   }
   else
   {
      return rect.Width()/9;
   }
}

void CBridgeDescDeckRebarGrid::OnChangedSelection(const CGXRange* pChangedRect,BOOL bIsDragging, BOOL bKey)
{
   CBridgeDescDeckReinforcementPage* pParent = (CBridgeDescDeckReinforcementPage*)GetParent();
   ASSERT(pParent);

   CRowColArray sel_rows;
   ROWCOL nSelRows = GetSelectedRows(sel_rows);
   pParent->EnableRemoveBtn( 0 < nSelRows ? true : false );
}

void CBridgeDescDeckRebarGrid::AddRow()
{
	ROWCOL nRow = 0;
   nRow = GetRowCount()+1;
	nRow = Max((ROWCOL)1, nRow);

	InsertRows(nRow, 1);
   SetRowStyle(nRow);

   // Set some default data
   CBridgeDescDeckReinforcementPage* pParent = (CBridgeDescDeckReinforcementPage*)GetParent();
   CBridgeDescDlg* pGrandParent = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pGrandParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   CDeckRebarData::NegMomentRebarData rebarData;

   // find the first continuous pier
   PierIndexType nPiers = pGrandParent->m_BridgeDesc.GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      const CPierData2* pPier = pGrandParent->m_BridgeDesc.GetPier(pierIdx);
      if ( pPier->HasCantilever() || pPier->IsContinuousConnection() )
      {
         rebarData.PierIdx = pierIdx;
         break;
      }
   }

   rebarData.Mat         = CDeckRebarData::TopMat;
   rebarData.LumpSum     = 0;
   rebarData.RebarGrade  = pParent->m_RebarData.TopRebarGrade;
   rebarData.RebarType   = pParent->m_RebarData.TopRebarType;
   rebarData.RebarSize   = matRebar::bs4;
   rebarData.Spacing     = ::ConvertToSysUnits(18,unitMeasure::Inch);
   rebarData.LeftCutoff  = ::ConvertToSysUnits(10,unitMeasure::Feet);
   rebarData.RightCutoff = ::ConvertToSysUnits(10,unitMeasure::Feet);

   PutRowData(nRow,rebarData);

	ScrollCellInView(nRow+1, GetLeftCol());
}

void CBridgeDescDeckRebarGrid::RemoveSelectedRows()
{
   CRowColArray sel_rows;
   ROWCOL nSelRows = GetSelectedRows(sel_rows);
   for ( int r = nSelRows-1; r >= 0; r-- )
   {
      ROWCOL selRow = sel_rows[r];

      ROWCOL nRows = GetRowCount();

      RemoveRows(selRow,selRow);
   }

   OnChangedSelection(NULL,FALSE,FALSE);
}

void CBridgeDescDeckRebarGrid::CustomInit()
{
   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
	Initialize( );

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

	GetParam( )->EnableUndo(FALSE);

   const int num_rows = 0;
   const int num_cols = 8;

	SetRowCount(num_rows);
	SetColCount(num_cols);

	// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   // set text along top row
	SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Pier"))
		);

	SetStyleRange(CGXRange(0,2), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Mat"))
		);

	SetStyleRange(CGXRange(0,3), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Bar"))
		);

   CString cv;
   cv.Format(_T("Spacing\n%s"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,4), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

	SetStyleRange(CGXRange(0,5), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T(""))
		);

   cv.Format(_T("As\n%s"), pDisplayUnits->GetAvOverSUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,6), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv.Format(_T("Left Cutoff\n%s"),pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,7), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv.Format(_T("Right Cutoff\n%s"), pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,8), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   // make it so that text fits correctly in header row
	ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();
	SetFocus();

   CBridgeDescDeckReinforcementPage* pParent = (CBridgeDescDeckReinforcementPage*)GetParent();
   ASSERT(pParent);

   pParent->EnableRemoveBtn( false ); // start off disabled

   UpdatePierList();

	GetParam( )->EnableUndo(TRUE);
}

void CBridgeDescDeckRebarGrid::UpdatePierList()
{
   CBridgeDescDeckReinforcementPage* pParent = (CBridgeDescDeckReinforcementPage*)GetParent();
   ASSERT(pParent);

   CBridgeDescDlg* pGrandParent = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pGrandParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   PierIndexType nPiers = pGrandParent->m_BridgeDesc.GetPierCount();
   IndexType idx = 0;
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      const CPierData2* pPier = pGrandParent->m_BridgeDesc.GetPier(pierIdx);
      if ( pPier->HasCantilever() || pPier->IsContinuousConnection() )
      {
         if ( idx == 0 )
         {
            m_strPiers.Format(_T("%d"),LABEL_PIER(pierIdx));
            idx++;
         }
         else
         {
            CString str = m_strPiers;
            m_strPiers.Format(_T("%s\n%d"),str,LABEL_PIER(pierIdx));
            idx++;
         }
      }
   }

   m_nContinuousPiers = idx;

   pParent->EnableAddBtn( 0 < m_nContinuousPiers ? true : false );
}

void CBridgeDescDeckRebarGrid::SetRowStyle(ROWCOL nRow)
{
   GetParam()->EnableUndo(FALSE);

   this->SetStyleRange(CGXRange(nRow,1), CGXStyle()
		   .SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
		   .SetChoiceList(m_strPiers)
		   .SetValue(m_strPiers.Left(m_strPiers.Find(_T("\n"),0)))
         .SetHorizontalAlignment(DT_RIGHT)
         );

   CString strMats = (m_bEnableTopMat && m_bEnableBottomMat ? _T("Top\nBottom") : _T("Top"));
   this->SetStyleRange(CGXRange(nRow,2), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(strMats)
			.SetValue(_T("Top"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

   CBridgeDescDeckReinforcementPage* pParent = (CBridgeDescDeckReinforcementPage*)GetParent();
   matRebar::Type type;
   matRebar::Grade grade;
   pParent->GetRebarMaterial(&type,&grade);
   CString strBarSizeChoiceList(_T("None\n"));
   lrfdRebarIter rebarIter(type,grade);
   for ( rebarIter.Begin(); rebarIter; rebarIter.Next() )
   {
      const matRebar* pRebar = rebarIter.GetCurrentRebar();
      strBarSizeChoiceList += pRebar->GetName().c_str();
      strBarSizeChoiceList += _T("\n");
   }

   SetStyleRange(CGXRange(nRow,3), CGXStyle()
      .SetEnabled(TRUE)
      .SetReadOnly(FALSE)
      .SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
      .SetChoiceList(strBarSizeChoiceList)
      .SetHorizontalAlignment(DT_RIGHT)
      .SetValue(_T("None"))
      );


   this->SetStyleRange(CGXRange(nRow,4), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         );

   this->SetStyleRange(CGXRange(nRow,5), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetEnabled(FALSE)
         .SetReadOnly(TRUE)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetValue(_T("PLUS"))
         );

   this->SetStyleRange(CGXRange(nRow,6), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         );

   this->SetStyleRange(CGXRange(nRow,7), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         );

   this->SetStyleRange(CGXRange(nRow,8), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         );

	GetParam()->EnableUndo(TRUE);
}

CString CBridgeDescDeckRebarGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

bool CBridgeDescDeckRebarGrid::GetRowData(ROWCOL nRow, CDeckRebarData::NegMomentRebarData* pRebarData)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   // pier index
   CString strPier = GetCellValue(nRow,1);
   pRebarData->PierIdx = _tstoi(strPier) - 1;

   CBridgeDescDeckReinforcementPage* pParent = (CBridgeDescDeckReinforcementPage*)GetParent();
   ASSERT(pParent);

   // mat
   CString strMat = GetCellValue(nRow,2);
   pRebarData->Mat = (strMat == _T("Top") ? CDeckRebarData::TopMat : CDeckRebarData::BottomMat);

   // bar size
   pRebarData->RebarGrade = pParent->m_RebarData.TopRebarGrade;
   pRebarData->RebarType  = pParent->m_RebarData.TopRebarType;
   pRebarData->RebarSize  = GetBarSize(nRow);
   
   // spacing
   CString strSpacing = GetCellValue(nRow,4);
   Float64 spacing = _tstof(strSpacing);
   spacing = ::ConvertToSysUnits(spacing,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   pRebarData->Spacing = spacing;

   // lump sum area
   CString strAs = GetCellValue(nRow,6);
   Float64 As = _tstof(strAs);
   As = ::ConvertToSysUnits(As,pDisplayUnits->GetAvOverSUnit().UnitOfMeasure);
   pRebarData->LumpSum = As;

   // left offset
   CString strCutoff = GetCellValue(nRow,7);
   Float64 cutoff = _tstof(strCutoff);
   cutoff = ::ConvertToSysUnits(cutoff,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   pRebarData->LeftCutoff = cutoff;

   // right offset
   strCutoff = GetCellValue(nRow,8);
   cutoff = _tstof(strCutoff);
   cutoff = ::ConvertToSysUnits(cutoff,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   pRebarData->RightCutoff = cutoff;

   return true;
}

matRebar::Size CBridgeDescDeckRebarGrid::GetBarSize(ROWCOL row)
{
   CString s = GetCellValue(row, 3);
   s.TrimLeft();
   int l = s.GetLength();
   CString s2 = s.Right(l-1);
   int i = _tstoi(s2);
   if (s.IsEmpty() || (i==0))
   {
      return matRebar::bsNone;
   }

   switch(i)
   {
   case 3:  return matRebar::bs3;
   case 4:  return matRebar::bs4;
   case 5:  return matRebar::bs5;
   case 6:  return matRebar::bs6;
   case 7:  return matRebar::bs7;
   case 8:  return matRebar::bs8;
   case 9:  return matRebar::bs9;
   case 10: return matRebar::bs10;
   case 11: return matRebar::bs11;
   case 14: return matRebar::bs14;
   case 18: return matRebar::bs18;
   default: ATLASSERT(false);
   }

   return matRebar::bsNone;
}

void CBridgeDescDeckRebarGrid::OnModifyCell(ROWCOL nRow,ROWCOL nCol)
{
   if ( nCol == 1 )
   {
      // Pier Changed
      CString strPier = GetCellValue(nRow,1);
      PierIndexType pierIdx = _tstoi(strPier) - 1;

      CBridgeDescDeckReinforcementPage* pParent = (CBridgeDescDeckReinforcementPage*)GetParent();
      CBridgeDescDlg* pGrandParent = (CBridgeDescDlg*)(pParent->GetParent());
      ASSERT( pGrandParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );
      const CPierData2* pPier = pGrandParent->m_BridgeDesc.GetPier(pierIdx);
      UpdateCutoff(nRow,pPier);
   }
   else
   {
      CGXGridWnd::OnModifyCell(nRow,nCol);
   }
}

void CBridgeDescDeckRebarGrid::PutRowData(ROWCOL nRow, const CDeckRebarData::NegMomentRebarData& rebarData)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CBridgeDescDeckReinforcementPage* pParent = (CBridgeDescDeckReinforcementPage*)GetParent();
   ASSERT(pParent);

   CBridgeDescDlg* pGrandParent = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pGrandParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );
   const CPierData2* pPier = pGrandParent->m_BridgeDesc.GetPier(rebarData.PierIdx);
   if ( pPier->HasCantilever() || pPier->IsContinuousConnection() )
   {
      HideRows(nRow,nRow,FALSE);
   }
   else
   {
      HideRows(nRow,nRow,TRUE);
   }

   // pier index
   SetValueRange(CGXRange(nRow,1),(LONG)(rebarData.PierIdx+1L));

   // Mat
   SetValueRange(CGXRange(nRow,2),rebarData.Mat == CDeckRebarData::TopMat ? _T("Top") : _T("Bottom"));

   // bar size
   CString tmp;
   tmp.Format(_T("%s"),lrfdRebarPool::GetBarSize(rebarData.RebarSize).c_str());
   VERIFY(SetValueRange(CGXRange(nRow, 3), tmp));

   // spacing
   Float64 spacing = rebarData.Spacing;
   spacing = ::ConvertFromSysUnits(spacing,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   SetValueRange(CGXRange(nRow,4),spacing);

   // lump sum area
   Float64 As = rebarData.LumpSum;
   As = ::ConvertFromSysUnits(As,pDisplayUnits->GetAvOverSUnit().UnitOfMeasure);
   SetValueRange(CGXRange(nRow,6),As);

   // Cutoffs - don't input cut-offs for non-continuous side of pier

   // left cutoff
   Float64 cutoff = rebarData.LeftCutoff;
   cutoff = ::ConvertFromSysUnits(cutoff,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   SetValueRange(CGXRange(nRow,7),cutoff);

   // right cutoff
   cutoff = rebarData.RightCutoff;
   cutoff = ::ConvertFromSysUnits(cutoff,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   SetValueRange(CGXRange(nRow,8),cutoff);

   UpdateCutoff(nRow,pPier);

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CBridgeDescDeckRebarGrid::UpdateCutoff(ROWCOL nRow,const CPierData2* pPier)
{
   bool bHasLeftCutoff = true;
   bool bHasRightCutoff = true;

   if ( pPier->IsBoundaryPier() && !pPier->HasCantilever() )
   {
      pgsTypes::BoundaryConditionType boundaryConditionType = pPier->GetBoundaryConditionType();
      if ( boundaryConditionType == pgsTypes::bctHinge || boundaryConditionType == pgsTypes::bctRoller )
      {
         bHasLeftCutoff = false;
         bHasRightCutoff = false;
      }
      else
      {
         if ( boundaryConditionType == pgsTypes::bctIntegralAfterDeckHingeBack || 
              boundaryConditionType == pgsTypes::bctIntegralBeforeDeckHingeBack ||
              (boundaryConditionType == pgsTypes::bctIntegralAfterDeck && pPier->GetPrevSpan() == NULL) ||
              (boundaryConditionType == pgsTypes::bctIntegralBeforeDeck && pPier->GetPrevSpan() == NULL) 
            )
         {
            bHasLeftCutoff = false;
         }

         if ( boundaryConditionType == pgsTypes::bctIntegralAfterDeckHingeAhead || 
              boundaryConditionType == pgsTypes::bctIntegralBeforeDeckHingeAhead ||
             (boundaryConditionType == pgsTypes::bctIntegralAfterDeck && pPier->GetNextSpan() == NULL) ||
             (boundaryConditionType == pgsTypes::bctIntegralBeforeDeck && pPier->GetNextSpan() == NULL) 
            ) 
         {
            bHasRightCutoff = false;
         }
      }
   }

   if ( bHasLeftCutoff )
   {
      SetStyleRange(CGXRange(nRow,7), CGXStyle()
         .SetEnabled(TRUE)
         .SetFormat(GX_FMT_GEN)
         .SetReadOnly(FALSE)
         .SetInterior(::GetSysColor(COLOR_WINDOW))
         );
   }
   else
   {
      SetStyleRange(CGXRange(nRow,7), CGXStyle()
         .SetEnabled(FALSE)
         .SetFormat(GX_FMT_HIDDEN)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         );
   }

   if ( bHasRightCutoff )
   {
      SetStyleRange(CGXRange(nRow,8), CGXStyle()
         .SetEnabled(TRUE)
         .SetFormat(GX_FMT_GEN)
         .SetReadOnly(FALSE)
         .SetInterior(::GetSysColor(COLOR_WINDOW))
         );
   }
   else
   {
      SetStyleRange(CGXRange(nRow,8), CGXStyle()
         .SetEnabled(FALSE)
         .SetFormat(GX_FMT_HIDDEN)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         );
   }
}

void CBridgeDescDeckRebarGrid::FillGrid(const std::vector<CDeckRebarData::NegMomentRebarData>& vRebarData)
{
	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   // remove all but top row
   ROWCOL rows = GetRowCount();
   if (1 <= rows)
   {
	   RemoveRows(1, rows);
   }

   CollectionIndexType size = vRebarData.size();

   // size grid
   for (CollectionIndexType i = 0; i < size; i++)
   {
	   ROWCOL nRow = 0;
      nRow = GetRowCount()+1;
	   nRow = Max((ROWCOL)1, nRow);

	   InsertRows(nRow, 1);
      SetRowStyle(nRow);
   }

   // fill grid
   ROWCOL nRow=1;
   std::vector<CDeckRebarData::NegMomentRebarData>::const_iterator iter;
   for ( iter = vRebarData.begin(); iter != vRebarData.end(); iter++ )
   {
      const CDeckRebarData::NegMomentRebarData& rebarData = *iter;
      PutRowData(nRow,rebarData);
      nRow++;
   }

   SelectRange(CGXRange(1,1));

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

bool CBridgeDescDeckRebarGrid::GetRebarData(std::vector<CDeckRebarData::NegMomentRebarData>& vRebarData)
{
   vRebarData.clear();

   ROWCOL rows = GetRowCount();
   for ( ROWCOL row = 1; row <= rows; row++ )
   {
      CDeckRebarData::NegMomentRebarData rebarData;
      if ( !GetRowData(row,&rebarData) )
      {
         return false;
      }

     vRebarData.push_back(rebarData);
   }
   return true;
}

void CBridgeDescDeckRebarGrid::EnableMats(BOOL bEnableTop,BOOL bEnableBottom)
{
   m_bEnableTopMat    = bEnableTop;
   m_bEnableBottomMat = bEnableBottom;
}

BOOL CBridgeDescDeckRebarGrid::OnValidateCell(ROWCOL nRow, ROWCOL nCol)
{
	CString s;
	CGXControl* pControl = GetControl(nRow, nCol);
	pControl->GetCurrentText(s);

   if ( s.IsEmpty() )
   {
      SetWarningText(_T("Value must be a number"));
      return FALSE;
   }

   if ( nCol == 1 )
   {
      CString strPier = GetCellValue(nRow,1);
      PierIndexType pierIdx = _tstoi(strPier) - 1;

      CBridgeDescDeckReinforcementPage* pParent = (CBridgeDescDeckReinforcementPage*)GetParent();
      ASSERT(pParent);

      CBridgeDescDlg* pGrandParent = (CBridgeDescDlg*)(pParent->GetParent());
      ASSERT( pGrandParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );
      const CPierData2* pPier = pGrandParent->m_BridgeDesc.GetPier(pierIdx);

      if ( pPier->IsBoundaryPier() )
      {
         // can't have reinforcement running over a boundary pier if there is a hinge or roller connection.
         // Boundary piers are between groups and hinge/roller connection means there is a hinge point in
         // the superstructure. If bars go over the pier, there wouldn't be a hinge
         if ( pPier->GetBoundaryConditionType() == pgsTypes::bctHinge || pPier->GetBoundaryConditionType() == pgsTypes::bctRoller )
         {
            CString strMsg;
            strMsg.Format(_T("Pier %d has a hinge/roller type connection. It cannot have supplimental reinforcement. Remove this row from the Supplemental Reinforcement Grid"),LABEL_PIER(pierIdx));
            SetWarningText(strMsg);
            return FALSE;
         }
      }
   }

   if ( nCol == 4 )
   {
      CString strSize = GetCellValue(nRow,nCol-1);
      CString strSpacing = GetCellValue(nRow,nCol);
      Float64 spacing = _tstof(strSpacing);
      if ( strSize != _T("None") && IsLE(spacing,0.0) )
      {
         SetWarningText(_T("Bar spacing must be greater than zero"));
         return false;
      }
   }

   if ( nCol == 6 )
   {
      CString strAs = GetCellValue(nRow,6);
      Float64 As = _tstof(strAs);
      if ( As < 0 )
      {
         SetWarningText(_T("As must be greater than zero"));
         return FALSE;
      }
   }

   if ( nCol == 7 || nCol == 8 )
   {
      CString strCutoff = GetCellValue(nRow,7);
      Float64 leftCutoff = _tstof(strCutoff);

      strCutoff = GetCellValue(nRow,8);
      Float64 rightCutoff = _tstof(strCutoff);

      if ( IsLE(leftCutoff+rightCutoff,0.0) )
      {
         SetWarningText(_T("Bar length must be greater than zero"));
         return FALSE;
      }
   }

	return CGXGridWnd::OnValidateCell(nRow, nCol);
}
