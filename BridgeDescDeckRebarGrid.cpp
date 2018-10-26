///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#include "stdafx.h"
#include "BridgeDescDeckRebarGrid.h"
#include "PGSuperDoc.h"
#include "BridgeDescDlg.h"
#include "BridgeDescDeckReinforcementPage.h"
#include <Units\Measure.h>

#include <EAF\EAFDisplayUnits.h>

#include <algorithm>

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

   switch (nCol)
   {
   case 0:
      return rect.Width()/16;

   case 1:
   case 2:
   case 3:
   case 4:
   case 5:
   case 6:
   case 7:
      return 15*rect.Width()/112;

   default:
      return CGXGridWnd::GetColWidth(nCol);
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
	nRow = max(1, nRow);

	InsertRows(nRow, 1);
   SetRowStyle(nRow);

   // Set some default data
   CDeckRebarData::NegMomentRebarData rebarData;
   rebarData.PierIdx = 1;
   rebarData.Mat = CDeckRebarData::TopMat;
   rebarData.LumpSum = 0;
   rebarData.RebarKey = INVALID_BAR_SIZE;
   rebarData.Spacing = ::ConvertToSysUnits(18,unitMeasure::Inch);
   rebarData.LeftCutoff = ::ConvertToSysUnits(10,unitMeasure::Feet);
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
   const int num_cols = 7;

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

   CString cv;
   cv.Format("As\n%s", pDisplayUnits->GetAvOverSUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,3), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

	SetStyleRange(CGXRange(0,4), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Bar"))
		);

   cv.Format("Spacing\n%s",pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,5), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv.Format("Left Cutoff\n%s",pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,6), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(cv)
		);

   cv.Format("Right Cutoff\n%s", pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str());
	SetStyleRange(CGXRange(0,7), CGXStyle()
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

	GetParam( )->EnableUndo(TRUE);
}

void CBridgeDescDeckRebarGrid::SetRowStyle(ROWCOL nRow)
{
   GetParam()->EnableUndo(FALSE);

   CBridgeDescDeckReinforcementPage* pParent = (CBridgeDescDeckReinforcementPage*)GetParent();
   ASSERT(pParent);

   CBridgeDescDlg* pGrandParent = (CBridgeDescDlg*)(pParent->GetParent());
   ASSERT( pGrandParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   PierIndexType nPiers = pGrandParent->m_BridgeDesc.GetPierCount();
   CString strPiers;
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      if ( pierIdx == 0 )
      {
         strPiers.Format("%d",pierIdx+1);
      }
      else
      {
         CString str = strPiers;
         strPiers.Format("%s\n%d",str,pierIdx+1);
      }
   }

	this->SetStyleRange(CGXRange(nRow,1), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(strPiers)
			.SetValue(strPiers.Left(strPiers.Find("\n",0)))
         .SetHorizontalAlignment(DT_RIGHT)
         );

   CString strMats = (m_bEnableTopMat && m_bEnableBottomMat ? "Top\nBottom" : "Top");
   this->SetStyleRange(CGXRange(nRow,2), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(strMats)
			.SetValue(_T("Top"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

   this->SetStyleRange(CGXRange(nRow,3), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         );

	this->SetStyleRange(CGXRange(nRow,4), CGXStyle()
			.SetControl(GX_IDS_CTRL_CBS_DROPDOWNLIST)
			.SetChoiceList(_T("None\n#4\n#5\n#6\n#7\n#8\n#9"))
			.SetValue(_T("None"))
         .SetHorizontalAlignment(DT_RIGHT)
         );

   this->SetStyleRange(CGXRange(nRow,5), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         );

   this->SetStyleRange(CGXRange(nRow,6), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         );

   this->SetStyleRange(CGXRange(nRow,7), CGXStyle()
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
        return GetValueRowCol(nRow, nCol);
}

bool CBridgeDescDeckRebarGrid::GetRowData(ROWCOL nRow, CDeckRebarData::NegMomentRebarData* pRebarData)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   // pier index
   CString strPier = GetCellValue(nRow,1);
   pRebarData->PierIdx = atoi(strPier) - 1;

   // mat
   CString strMat = GetCellValue(nRow,2);
   pRebarData->Mat = (strMat == "Top" ? CDeckRebarData::TopMat : CDeckRebarData::BottomMat);

   // lump sum area
   CString strAs = GetCellValue(nRow,3);
   double As = atof(strAs);
   As = ::ConvertToSysUnits(As,pDisplayUnits->GetAvOverSUnit().UnitOfMeasure);
   pRebarData->LumpSum = As;

   // bar size
   CString strBar = GetCellValue(nRow,4);
   if ( strBar == "None" )
      pRebarData->RebarKey = INVALID_BAR_SIZE;
   else
      pRebarData->RebarKey = atoi( strBar.Right(1) );
   
   // spacing
   CString strSpacing = GetCellValue(nRow,5);
   double spacing = atof(strSpacing);
   spacing = ::ConvertToSysUnits(spacing,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   pRebarData->Spacing = spacing;

   // left offset
   CString strCutoff = GetCellValue(nRow,6);
   double cutoff = atof(strCutoff);
   cutoff = ::ConvertToSysUnits(cutoff,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   pRebarData->LeftCutoff = cutoff;

   // right offset
   strCutoff = GetCellValue(nRow,7);
   cutoff = atof(strCutoff);
   cutoff = ::ConvertToSysUnits(cutoff,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   pRebarData->RightCutoff = cutoff;

   return true;
}

void CBridgeDescDeckRebarGrid::PutRowData(ROWCOL nRow, const CDeckRebarData::NegMomentRebarData& rebarData)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   // pier index
   SetValueRange(CGXRange(nRow,1),rebarData.PierIdx+1L);

   // Mat
   SetValueRange(CGXRange(nRow,2),rebarData.Mat == CDeckRebarData::TopMat ? "Top" : "Bottom");

   // lump sum area
   double As = rebarData.LumpSum;
   As = ::ConvertFromSysUnits(As,pDisplayUnits->GetAvOverSUnit().UnitOfMeasure);
   SetValueRange(CGXRange(nRow,3),As);

   // bar size
   if ( rebarData.RebarKey == INVALID_BAR_SIZE )
   {
      SetValueRange(CGXRange(nRow,4),"None");
   }
   else
   {
      CString strValue;
      strValue.Format("#%d",rebarData.RebarKey);
      SetValueRange(CGXRange(nRow,4),strValue);
   }

   // spacing
   double spacing = rebarData.Spacing;
   spacing = ::ConvertFromSysUnits(spacing,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
   SetValueRange(CGXRange(nRow,5),spacing);

   // left cutoff
   double cutoff = rebarData.LeftCutoff;
   cutoff = ::ConvertFromSysUnits(cutoff,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   SetValueRange(CGXRange(nRow,6),cutoff);

   // right cutoff
   cutoff = rebarData.RightCutoff;
   cutoff = ::ConvertFromSysUnits(cutoff,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   SetValueRange(CGXRange(nRow,7),cutoff);

   GetParam()->SetLockReadOnly(TRUE);
	GetParam()->EnableUndo(TRUE);
}

void CBridgeDescDeckRebarGrid::FillGrid(const std::vector<CDeckRebarData::NegMomentRebarData>& vRebarData)
{
	GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   // remove all but top row
   ROWCOL rows = GetRowCount();
   if (rows>=1)
	   RemoveRows(1, rows);

   int size = vRebarData.size();

   // size grid
   for (int i = 0; i < size; i++)
	   AddRow();

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

void CBridgeDescDeckRebarGrid::GetRebarData(std::vector<CDeckRebarData::NegMomentRebarData>& vRebarData)
{
   vRebarData.clear();

  ROWCOL rows = GetRowCount();
  for ( ROWCOL row = 1; row <= rows; row++ )
  {
     CDeckRebarData::NegMomentRebarData rebarData;
     GetRowData(row,&rebarData);
     vRebarData.push_back(rebarData);
  }
}

void CBridgeDescDeckRebarGrid::EnableMats(BOOL bEnableTop,BOOL bEnableBottom)
{
   m_bEnableTopMat    = bEnableTop;
   m_bEnableBottomMat = bEnableBottom;
}

