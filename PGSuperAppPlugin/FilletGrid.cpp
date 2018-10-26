///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// FilletGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "FilletGrid.h"
#include "SelectItemDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\GirderGroupData.h>
#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CFilletGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CFilletGrid

CFilletGrid::CFilletGrid()
{
//   RegisterClass();
}

CFilletGrid::~CFilletGrid()
{
}

BEGIN_MESSAGE_MAP(CFilletGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CFilletGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CFilletGrid::CustomInit(CSplicedGirderData* pGirder)
{
   m_pGirder = pGirder;

   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
	this->Initialize( );

	this->GetParam( )->EnableUndo(FALSE);

   m_pGroup = pGirder->GetGirderGroup();
   SpanIndexType nSpansThisGroup = m_pGroup->GetSpanCount();
   SpanIndexType startSpanIdx = m_pGroup->GetPier(pgsTypes::metStart)->GetSpan(pgsTypes::Ahead)->GetIndex();

   const int num_rows=0;
   const int num_cols = (int)(nSpansThisGroup);

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
   SpanIndexType spanIdx = startSpanIdx;
   for ( SpanIndexType iSpan = 0; iSpan < nSpansThisGroup; iSpan++ )
   {
      CString strLabel;
      strLabel.Format(_T("Span %d"),LABEL_SPAN(spanIdx));

	   this->SetStyleRange(CGXRange(0,col++), CGXStyle()
            .SetWrapText(TRUE)
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
	   		.SetEnabled(FALSE)          // disables usage as current cell
            .SetValue(strLabel)
	   	);

      spanIdx++;
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   InsertRows(GetRowCount()+1,1);

   CString strLabel;
   strLabel.Format(_T("Fillet (%s)"),pDisplayUnits->GetComponentDimUnit().UnitOfMeasure.UnitTag().c_str());
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

void CFilletGrid::FillGrid()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   ROWCOL row = 1;
   ROWCOL col = 1;

   GirderIndexType gdrIdx = m_pGirder->GetIndex();

   SpanIndexType nSpansThisGroup = m_pGroup->GetSpanCount();
   const CPierData2* pPier = m_pGroup->GetPier(pgsTypes::metStart);

   for ( SpanIndexType iSpan = 0; iSpan < nSpansThisGroup; iSpan++ )
   {
      const CSpanData2* pSpan = pPier->GetSpan(pgsTypes::Ahead);

      Float64 fillet = pSpan->GetFillet(gdrIdx);

      CString strValue;
      strValue.Format(_T("%s"),::FormatDimension(fillet,pDisplayUnits->GetComponentDimUnit(),false));

	   SetStyleRange(CGXRange(row,col++), CGXStyle()
            .SetWrapText(TRUE)
            .SetHorizontalAlignment(DT_CENTER)
            .SetVerticalAlignment(DT_VCENTER)
            .SetValue(strValue)
		   );

      if (iSpan < nSpansThisGroup)
      {
         pPier = pSpan->GetPier(pgsTypes::metEnd);
      }
   }
}

CString CFilletGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CFilletGrid::UpdateFilletData()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   ROWCOL row = 1;
   ROWCOL nCol = GetColCount();

   GirderIndexType gdrIdx = m_pGirder->GetIndex();

   pgsTypes::FilletType ft = m_pGroup->GetBridgeDescription()->GetFilletType();

   SpanIndexType nSpansThisGroup = m_pGroup->GetSpanCount();
   CPierData2* pPier = m_pGroup->GetPier(pgsTypes::metStart);

   for ( ROWCOL col = 1; col <= nCol; col++ )
   {
      CSpanData2* pSpan = pPier->GetSpan(pgsTypes::Ahead);

      Float64 fillet = _tstof(GetCellValue(row,col));
      fillet = ::ConvertToSysUnits(fillet,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

      if (pgsTypes::fttGirder==ft)
      {
         pSpan->SetFillet(gdrIdx, fillet);
      }
      else
      {
         pSpan->SetFillet(fillet);
      }

      if (col < nCol)
      {
         pPier = pSpan->GetPier(pgsTypes::metEnd);
      }
   }
}

bool CFilletGrid::SelectSingleValue(Float64* pValue)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   ROWCOL row = 1;
   ROWCOL nCol = GetColCount();

   std::set<CString> uniqvals;
   for ( ROWCOL col = 1; col <= nCol; col++ )
   {
      uniqvals.insert(GetCellValue(row,col));
   }

   IndexType nvals = uniqvals.size();
   if (nvals==0)
   {
      ATLASSERT(0); // shouldn't happen
      return false;
   }
   else if (nvals==1)
   {
      // only one unique value. No need to ask
      Float64 offset = _tstof(*uniqvals.begin());
      offset = ::ConvertToSysUnits(offset,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
      *pValue = offset;
      return true;
   }
   else
   {
      // have mutiple values. must select one from dialog
      CSelectItemDlg dlg;
      dlg.m_ItemIdx = 0;
      dlg.m_strTitle = _T("Select Fillet Value");
      dlg.m_strLabel = _T("A single Fillet will be used for the entire bridge. Select a value.");
      
      IndexType ival=0;
      CString strItems;
      for (std::set<CString>::iterator it=uniqvals.begin(); it!=uniqvals.end(); it++)
      {
         strItems += *it;
         if (++ival<nvals)
         {
            strItems += _T("\n");
         }
      }

      dlg.m_strItems = strItems;
      if ( dlg.DoModal() == IDOK )
      {
         // Get single value
         std::set<CString>::iterator it=uniqvals.begin();
         for(IndexType i=0; i<dlg.m_ItemIdx; i++)
         {
            it++;
         }

         CString strValue = *it;

         // Set grid to show only single value for all cols
         row = 1;
         for ( ROWCOL col = 1; col <= nCol; col++ )
         {
	         SetStyleRange(CGXRange(row,col), CGXStyle().SetValue(strValue));
         }

         // return value
         Float64 offset = _tstof(*it);
         offset = ::ConvertToSysUnits(offset,pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
         *pValue = offset;
         return true;
      }
      else
      {
         return false;
      }
   }
}

void CFilletGrid::EnableWindow(BOOL bEnable)
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
