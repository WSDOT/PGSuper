///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// SlabOffsetGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "SlabOffsetGrid.h"

#include <EAF\EAFDisplayUnits.h>
#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\GirderGroupData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CSlabOffsetGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CSlabOffsetGrid

CSlabOffsetGrid::CSlabOffsetGrid()
{
//   RegisterClass();
}

CSlabOffsetGrid::~CSlabOffsetGrid()
{
}

BEGIN_MESSAGE_MAP(CSlabOffsetGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CSlabOffsetGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CSlabOffsetGrid::CustomInit(CSplicedGirderData* pGirder)
{
   m_pGirder = pGirder;

   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
	this->Initialize( );

	this->GetParam( )->EnableUndo(FALSE);

   m_pGroup = pGirder->GetGirderGroup();
   PierIndexType startPierIdx = m_pGroup->GetPierIndex(pgsTypes::metStart);
   PierIndexType endPierIdx   = m_pGroup->GetPierIndex(pgsTypes::metEnd);
   PierIndexType nPiersThisGroup = m_pGroup->GetPierCount();

   const int num_rows=0;
   const int num_cols = (int)(nPiersThisGroup);

	this->SetRowCount(num_rows);
	this->SetColCount(num_cols);

   // Turn off selecting whole columns when clicking on a column header
	this->GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   // no row moving
	this->GetParam()->EnableMoveRows(FALSE);

   ROWCOL col = 0;

   // disable left side
	this->SetStyleRange(CGXRange(0,col,num_rows,col), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetValue("")
		);
   col++;

   // set text along top row
   for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
   {
      CString strLabel;
      if ( m_pGroup->GetPier(pierIdx)->IsAbutment() )
         strLabel.Format(_T("Abut. %d"),LABEL_PIER(pierIdx));
      else
         strLabel.Format(_T("Pier %d"),LABEL_PIER(pierIdx));

	   this->SetStyleRange(CGXRange(0,col++), CGXStyle()
            .SetWrapText(TRUE)
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
	   		.SetEnabled(FALSE)          // disables usage as current cell
            .SetValue(strLabel)
	   	);
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);


   InsertRows(GetRowCount()+1,1);

   CString strLabel;
   strLabel.Format(_T("Slab Offset (%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(1,0), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetValue(strLabel)
		);

   // don't allow users to resize grids
   this->GetParam( )->EnableTrackColWidth(0); 
   this->GetParam( )->EnableTrackRowHeight(0); 

	this->EnableIntelliMouse();
	this->SetFocus();

   // make it so that text fits correctly in header row
	//this->ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));
   this->ResizeColWidthsToFit(CGXRange(0,0,1,0));
	this->GetParam( )->EnableUndo(TRUE);
}

void CSlabOffsetGrid::FillGrid()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   ROWCOL row = 1;
   ROWCOL col = 1;

   GirderIndexType gdrIdx = m_pGirder->GetIndex();
   PierIndexType startPierIdx = m_pGroup->GetPierIndex(pgsTypes::metStart);
   PierIndexType endPierIdx   = m_pGroup->GetPierIndex(pgsTypes::metEnd);
   for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
   {
      Float64 offset = m_pGroup->GetSlabOffset(pierIdx,gdrIdx);

      CString strValue;
      strValue.Format(_T("%s"),::FormatDimension(offset,pDisplayUnits->GetComponentDimUnit(),false));

	   SetStyleRange(CGXRange(row,col++), CGXStyle()
            .SetWrapText(TRUE)
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
            .SetValue(strValue)
		   );
   }
}

CString CSlabOffsetGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CSlabOffsetGrid::UpdateSlabOffsetData()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   ROWCOL row = 1;
   ROWCOL nCol = GetColCount();

   GirderIndexType gdrIdx = m_pGirder->GetIndex();
   PierIndexType pierIdx = m_pGroup->GetPierIndex(pgsTypes::metStart);

   for ( ROWCOL col = 1; col <= nCol; col++, pierIdx++ )
   {
      Float64 offset = _tstof(GetCellValue(row,col));
      offset = ::ConvertToSysUnits(offset,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
      m_pGroup->SetSlabOffset(pierIdx,gdrIdx,offset);
   }
}

void CSlabOffsetGrid::EnableWindow(BOOL bEnable)
{
   GetParam()->EnableUndo(FALSE);
   GetParam()->SetLockReadOnly(FALSE);

   CGXStyle enable_style;
   CGXStyle disable_style;
   enable_style.SetEnabled(TRUE)
        .SetReadOnly(FALSE)
        .SetInterior(::GetSysColor(COLOR_WINDOW))
        .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

   disable_style.SetEnabled(FALSE)
        .SetReadOnly(TRUE)
        .SetInterior(::GetSysColor(COLOR_BTNFACE))
        .SetTextColor(::GetSysColor(COLOR_GRAYTEXT));


   ROWCOL row = 1;
   ROWCOL nCol = GetColCount();

   for ( ROWCOL col = 1; col <= nCol; col++ )
   {
      if ( bEnable )
	      SetStyleRange(CGXRange(row,col), enable_style);
      else
	      SetStyleRange(CGXRange(row,col), disable_style);
   }

   GetParam()->SetLockReadOnly(FALSE);
   GetParam()->EnableUndo(TRUE);
}
