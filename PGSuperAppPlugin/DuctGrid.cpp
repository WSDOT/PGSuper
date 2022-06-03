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

// DuctGrid.cpp : implementation file
//

#include "stdafx.h"
#include "DuctGrid.h"
#include "SplicedGirderDescDlg.h"
#include "GirderDescDlg.h"

#include "LinearDuctDlg.h"
#include "ParabolicDuctDlg.h"
#include "OffsetDuctDlg.h"

#include "TimelineEventDlg.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\BeamFactory.h>
#include <IFace\Allowables.h>
#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\BridgeDescription2.h>

#include <LRFD\PsStrand.h>

#include "PGSuperUnits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CDuctGrid, CS_DBLCLKS, 0, 0, 0);

ROWCOL nDuctTypeCol   = 0;
ROWCOL nNumStrandCol  = 0;
ROWCOL nJackEndCol    = 0;
ROWCOL nPjackCheckCol = 0;
ROWCOL nPjackCol      = 0;
ROWCOL nPjackUserCol  = 0;
ROWCOL nDuctGeomTypeCol = 0;
ROWCOL nDuctGeomEditCol = 0;
ROWCOL nEventCol = 0;

/////////////////////////////////////////////////////////////////////////////
// CDuctGrid

CDuctGrid::CDuctGrid()
{
//   RegisterClass();
   m_PrevStressTendonEventIdx = INVALID_INDEX;
}

CDuctGrid::~CDuctGrid()
{
}

BEGIN_MESSAGE_MAP(CDuctGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CDuctGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CDuctGrid::GetColWidth(ROWCOL nCol)
{
	CRect rect = GetGridRect( );
   if ( nCol == nPjackUserCol )
   {
      return 0;
   }

   if ( nCol == 0 )
   {
      return rect.Width()/18;
   }
   else if ( nCol == nDuctTypeCol )
   {
      return rect.Width()/9;
   }
   else if ( nCol == nEventCol )
   {
      return 2*rect.Width()/9;
   }
   else if ( nCol == nPjackCheckCol )
   {
      return 20; // want it to just fit the check box
   }

   return rect.Width()/9;
}

void CDuctGrid::CustomInit(CSplicedGirderData* pGirder)
{
   m_pPTData = pGirder->GetPostTensioning();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CSplicedGirderGeneralPage* pParentPage = (CSplicedGirderGeneralPage*)GetParent();
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)pParentPage->GetParent();

   const CTimelineManager* pTimelineMgr = pParent->m_BridgeDescription.GetTimelineManager();
   m_LastSegmentErectionEventIdx  = pTimelineMgr->GetLastSegmentErectionEventIndex();


   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
	this->Initialize( );

	this->GetParam( )->EnableUndo(FALSE);

   const int num_rows=0;
   const int num_cols=9;

	this->SetRowCount(num_rows);
	this->SetColCount(num_cols);

   // Turn off selecting whole columns when clicking on a column header
	this->GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE));

   SetMergeCellsMode(gxnMergeEvalOnDisplay);

   // no row moving
	this->GetParam()->EnableMoveRows(FALSE);

   ROWCOL col = 0;

   // disable left side
	this->SetStyleRange(CGXRange(0,col,num_rows,col), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetValue(_T("Duct"))
		);
   col++;

   // set text along top row
   nDuctTypeCol = col++;
	this->SetStyleRange(CGXRange(0,nDuctTypeCol), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetValue(_T("Type"))
		);

   nDuctGeomTypeCol = col++;
   this->SetStyleRange(CGXRange(0,nDuctGeomTypeCol), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Shape"))
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		);

   nDuctGeomEditCol = col++;
	this->SetStyleRange(CGXRange(0,nDuctGeomEditCol), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Shape"))
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		);

   nNumStrandCol = col++;
   this->SetStyleRange(CGXRange(0,nNumStrandCol), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetValue(_T("#\nStrands"))
		);

   nPjackCheckCol = col++;
   CString strPjack;
   strPjack.Format(_T("Pjack\n(%s)"),pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure.UnitTag().c_str());
	this->SetStyleRange(CGXRange(0,nPjackCheckCol), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strPjack)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		);

   nPjackCol = col++;
	this->SetStyleRange(CGXRange(0,nPjackCol), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(strPjack)
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		);

   // this will be a hidden cell that holds the last user input Pjack
   nPjackUserCol = col++;
	this->SetStyleRange(CGXRange(0,nPjackUserCol), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("last user pjack"))
		);
   this->HideCols(nPjackUserCol,nPjackUserCol);

   nJackEndCol = col++;
	this->SetStyleRange(CGXRange(0,nJackEndCol), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Jacking\nEnd"))
		);

   nEventCol = col++;
	this->SetStyleRange(CGXRange(0,nEventCol), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Event"))
		);


   // don't allow users to resize grids
   this->GetParam( )->EnableTrackColWidth(0); 
   this->GetParam( )->EnableTrackRowHeight(0); 

	this->EnableIntelliMouse();
	this->SetFocus();

   // make it so that text fits correctly in header row
	this->ResizeRowHeightsToFit(CGXRange(0,0,0,num_cols));
   this->ResizeColWidthsToFit(CGXRange(0,0,0,num_cols));
	this->GetParam( )->EnableUndo(TRUE);
}

void CDuctGrid::AddDuct(EventIndexType stressingEvent)
{
   CDuctData duct;
   m_pPTData->AddDuct(duct,stressingEvent);
   AddDuct(duct,stressingEvent);
}

void CDuctGrid::AddDuct(const CDuctData& duct,EventIndexType stressingEvent)
{
   ROWCOL nRow = GetRowCount()+1;
   GetParam()->EnableUndo(FALSE);

   InsertRows(nRow,1);
   SetRowStyle(nRow);
   UpdateStyleRange(CGXRange(nRow,nDuctTypeCol));
   
   RefreshRowHeading(1,GetRowCount());
   SetDuctData(nRow,duct,stressingEvent);
   UpdateNumStrandsList(nRow);

	Invalidate();
   GetParam()->EnableUndo(TRUE);
}

void CDuctGrid::DeleteDuct()
{
	CGXRangeList* pSelList = GetParam()->GetRangeList();
	if (pSelList->IsAnyCellFromCol(0) )
	{
		CGXRange range = pSelList->GetHead();
		range.ExpandRange(1, 0, GetRowCount(), 0);

      for ( ROWCOL row = range.bottom; range.top <= row; row-- )
      {
         if ( m_pPTData->CanRemoveDuct(DuctIndexType(row-1)) )
         {
            m_pPTData->RemoveDuct(DuctIndexType(row-1));
            CGXGridWnd::RemoveRows(row,row);
         }
      }
	}
}

void CDuctGrid::SetRowStyle(ROWCOL row)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILibraryNames,pLibNames);

   std::vector<std::_tstring> vNames;
   pLibNames->EnumDuctNames(&vNames);

   std::_tstring strDuctNames;
   std::vector<std::_tstring>::iterator begin(vNames.begin());
   std::vector<std::_tstring>::iterator iter(begin);
   std::vector<std::_tstring>::iterator end(vNames.end());
   for ( ; iter != end; iter++ )
   {
      if ( iter != begin )
      {
         strDuctNames += _T("\n");
      }
      strDuctNames += (*iter);
   }

	SetStyleRange(CGXRange(row,nDuctTypeCol), CGXStyle()
	   .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
		.SetChoiceList(strDuctNames.c_str())
		.SetValue(0L)
      .SetHorizontalAlignment(DT_RIGHT)
      );

   SetStyleRange(CGXRange(row,nNumStrandCol), CGXStyle()
      .SetControl(GX_IDS_CTRL_SPINEDIT)
      .SetHorizontalAlignment(DT_RIGHT)
      );

   SetStyleRange(CGXRange(row,nJackEndCol), CGXStyle()
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
   	.SetChoiceList(_T("Left\nRight\nBoth"))
		.SetValue(0L)
      );

   // 4, 5, and 6 are for Pjack
   SetStyleRange(CGXRange(row,nPjackCheckCol), CGXStyle()
      .SetControl(GX_IDS_CTRL_CHECKBOX3D)
      .SetHorizontalAlignment(DT_LEFT)
      );

   CString strDuctType = _T("Linear\nParabolic");
   // offset ducts not supported yet
   //if ( row != 1 )
   //   strDuctType += _T("\nOffset");

   SetStyleRange(CGXRange(row,nDuctGeomTypeCol), CGXStyle()
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
   	.SetChoiceList(strDuctType)
		.SetValue(0L)
      );

   SetStyleRange(CGXRange(row,nDuctGeomEditCol), CGXStyle()
      .SetControl(GX_IDS_CTRL_PUSHBTN)
      .SetChoiceList(_T("Edit"))
      );

   UpdateEventList(row);
}

void CDuctGrid::UpdateEventList(ROWCOL row)
{
   CSplicedGirderGeneralPage* pParentPage = (CSplicedGirderGeneralPage*)GetParent();
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)pParentPage->GetParent();

   const CTimelineManager* pTimelineMgr = pParent->m_BridgeDescription.GetTimelineManager();

   CString events;
   EventIndexType nEvents = pTimelineMgr->GetEventCount();
   for ( EventIndexType eventIdx = m_LastSegmentErectionEventIdx; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);
      CString label;
      if ( eventIdx == m_LastSegmentErectionEventIdx)
      {
         label.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),pTimelineEvent->GetDescription());
      }
      else
      {
         label.Format(_T("\nEvent %d: %s"),LABEL_EVENT(eventIdx),pTimelineEvent->GetDescription());
      }

      events += label;
   }

   events += _T("\n") + CString((LPCSTR)IDS_CREATE_NEW_EVENT);
   m_CreateEventIndex = nEvents - m_LastSegmentErectionEventIdx;

   SetStyleRange(CGXRange(row,nEventCol), CGXStyle()
      .SetControl(GX_IDS_CTRL_ZEROBASED_EX)
      .SetValue(0L) // cell value will be selected index
   	.SetChoiceList(events)
      );
}

void CDuctGrid::OnCalcPjack(ROWCOL nRow)
{
   // Calc Pj check box was clicked

   CString strCheck = GetCellValue(nRow,nPjackCheckCol);
   if ( strCheck == _T("1") )
   {
      // Box was unchecked... Replace the current value with the last user input Pjack

      // First make the cell writable
      GetParam()->SetLockReadOnly(FALSE);
      this->SetStyleRange(CGXRange(nRow,nPjackCol), 
         CGXStyle()
         .SetReadOnly(FALSE)
         .SetControl(GX_IDS_CTRL_EDIT)
         .SetInterior(::GetSysColor(COLOR_WINDOW))
         .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT))
         .SetEnabled(TRUE)
         .SetHorizontalAlignment(DT_RIGHT)
         );
      GetParam()->SetLockReadOnly(TRUE);

      Float64 LastPjack = _tstof(GetCellValue(nRow,nPjackUserCol));
      SetValueRange(CGXRange(nRow,nPjackCol),LastPjack);
   }
   else
   {
      // Box was checked... Save the last user input for Pjack
      Float64 LastPjack = _tstof(GetCellValue(nRow,nPjackCol));
      SetValueRange(CGXRange(nRow,nPjackUserCol),LastPjack);

      // Make the cell read only
      GetParam()->SetLockReadOnly(FALSE);
      this->SetStyleRange(CGXRange(nRow,nPjackCol), 
         CGXStyle()
         .SetReadOnly(TRUE)
         .SetControl(GX_IDS_CTRL_STATIC)
         .SetInterior(::GetSysColor(COLOR_BTNFACE))
         .SetTextColor(::GetSysColor(COLOR_GRAYTEXT))
         .SetEnabled(FALSE)
         .SetHorizontalAlignment(DT_RIGHT)
         );
      GetParam()->SetLockReadOnly(TRUE);

      // Set Pjack to the max value
      UpdateMaxPjack(nRow);
   }
}

void CDuctGrid::OnDuctTypeChanged(ROWCOL nRow)
{
   CSplicedGirderGeneralPage* pParent = (CSplicedGirderGeneralPage*)GetParent();
   DuctIndexType ductIdx = DuctIndexType(nRow-1);

   CDuctGeometry::GeometryType geomType = (CDuctGeometry::GeometryType)_tstoi(GetCellValue(nRow,nDuctGeomTypeCol));
   m_pPTData->GetDuct(ductIdx)->DuctGeometryType = geomType;

   pParent->Invalidate();
   pParent->UpdateWindow();
}

void CDuctGrid::OnEditDuctGeometry(ROWCOL nRow)
{
   CSplicedGirderGeneralPage* pParent = (CSplicedGirderGeneralPage*)GetParent();

   CDuctGeometry::GeometryType geomType = (CDuctGeometry::GeometryType)_tstoi(GetCellValue(nRow,nDuctGeomTypeCol));
   DuctIndexType ductIdx = DuctIndexType(nRow-1);
   if ( geomType == CDuctGeometry::Linear )
   {
      CLinearDuctDlg dlg(pParent,m_pPTData,ductIdx);
      if ( dlg.DoModal() == IDOK )
      {
         m_pPTData->GetDuct(ductIdx)->LinearDuctGeometry = dlg.GetDuctGeometry();
         pParent->m_DrawTendons.SetMapMode(dlg.GetTendonControlMapMode());
         pParent->Invalidate();
         pParent->UpdateWindow();
      }
   }
   else if ( geomType == CDuctGeometry::Parabolic )
   {
      CParabolicDuctDlg dlg(pParent,m_pPTData,ductIdx);
      if ( dlg.DoModal() == IDOK )
      {
         m_pPTData->GetDuct(ductIdx)->ParabolicDuctGeometry = dlg.GetDuctGeometry();
         pParent->m_DrawTendons.SetMapMode(dlg.GetTendonControlMapMode());
         pParent->Invalidate();
         pParent->UpdateWindow();
      }
   }
   else if ( geomType == CDuctGeometry::Offset )
   {
      ATLASSERT(false);// not supported yet
      COffsetDuctDlg dlg(pParent,ductIdx);
      dlg.m_DuctGeometry = m_pPTData->GetDuct(ductIdx)->OffsetDuctGeometry;
      if ( dlg.DoModal() == IDOK )
      {
         m_pPTData->GetDuct(ductIdx)->OffsetDuctGeometry = dlg.m_DuctGeometry;
         pParent->Invalidate();
         pParent->UpdateWindow();
      }
   }
   else
   {
      ATLASSERT(false); // is there a new type?
   }
}

void CDuctGrid::OnClickedButtonRowCol(ROWCOL nRow,ROWCOL nCol)
{
   if ( nCol == nPjackCheckCol )
   {
      OnCalcPjack(nRow);
   }

   if ( nCol == nDuctGeomEditCol )
   {
      OnEditDuctGeometry(nRow);
   }
}

CString CDuctGrid::GetCellValue(ROWCOL nRow, ROWCOL nCol)
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

void CDuctGrid::RefreshRowHeading(ROWCOL rFrom,ROWCOL rTo)
{
   CSplicedGirderGeneralPage* pParentPage = (CSplicedGirderGeneralPage*)GetParent();
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)(pParentPage->GetParent());
   std::_tstring strGirderName = pParent->m_pGirder->GetGirderName();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,ILibrary,pLibrary);
   const GirderLibraryEntry* pGirderEntry = pLibrary->GetGirderEntry(strGirderName.c_str());
   CComPtr<IBeamFactory> factory;
   pGirderEntry->GetBeamFactory(&factory);
   WebIndexType nWebs = factory->GetWebCount(pGirderEntry->GetDimensions());

   for ( ROWCOL row = rFrom; row <= rTo; row++ )
   {
      DuctIndexType first_duct_in_row = nWebs*(row - 1);
      DuctIndexType last_duct_in_row  = first_duct_in_row + nWebs - 1;
      CString strLabel;

      if (nWebs == 1)
      {
         strLabel.Format(_T("%d"), LABEL_DUCT(first_duct_in_row));
      }
      else
      {
         strLabel.Format(_T("%d - %d"), LABEL_DUCT(first_duct_in_row), LABEL_DUCT(last_duct_in_row));
      }

      SetValueRange(CGXRange(row,0),strLabel);
   }
}

BOOL CDuctGrid::OnLButtonHitRowCol(ROWCOL nHitRow,ROWCOL nHitCol,ROWCOL nDragRow,ROWCOL nDragCol,CPoint point,UINT flags,WORD nHitState)
{
   if ( nHitCol == nEventCol )
   {
      m_PrevStressTendonEventIdx = (EventIndexType)_tstoi(GetCellValue(nHitRow,nHitCol).GetString()) + m_LastSegmentErectionEventIdx;
   }
   return CGXGridWnd::OnLButtonHitRowCol(nHitRow,nHitCol,nDragRow,nDragCol,point,flags,nHitState);
}

void CDuctGrid::OnModifyCell(ROWCOL nRow,ROWCOL nCol)
{
   if ( nCol == nDuctTypeCol )
   {
      // Duct size changed... Update the number of strands choice
      UpdateNumStrandsList(nRow);
   }
   else if ( nCol == nNumStrandCol )
   {
      // #of strand changed
      if ( ComputePjackMax(nRow) ) // check box for Calc Pjack is unchecked
      {
         UpdateMaxPjack(nRow);
      }
   }
   else if ( nCol == nEventCol )
   {
      EventIndexType idx = (EventIndexType)_tstoi(GetCellValue(nRow,nCol).GetString());
      if ( idx == m_CreateEventIndex )
      {
         EventIndexType eventIdx = CreateEvent();
         if ( eventIdx != INVALID_INDEX )
         {
            ROWCOL nRows = GetRowCount();
            for ( ROWCOL row = 1; row <= nRows; row++ )
            {
               idx = _tstoi(GetCellValue(row,nEventCol).GetString());
               UpdateEventList(row);
               SetStyleRange(CGXRange(row,nEventCol),CGXStyle().SetValue((LONG)idx));
            }

            SetStyleRange(CGXRange(nRow,nEventCol), CGXStyle().SetValue((LONG)eventIdx) );            
         }
         else
         {
             // revert to previous value
            SetStyleRange(CGXRange(nRow,nEventCol), CGXStyle().SetValue((LONG)(m_PrevStressTendonEventIdx-m_LastSegmentErectionEventIdx)) );
         }
      }
   }
   else if ( nCol == nDuctGeomTypeCol )
   {
      OnDuctTypeChanged(nRow);
   }
   else
   {
      __super::OnModifyCell(nRow, nCol);
   }
}

void CDuctGrid::UpdateNumStrandsList(ROWCOL nRow)
{
   // Get Current value for number of strands
   StrandIndexType nStrands = (StrandIndexType)_tstol(GetCellValue(nRow,nNumStrandCol));
   CString ductName = GetCellValue(nRow,nDuctTypeCol);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILibrary,pLib);
   const DuctLibraryEntry* pDuctEntry = pLib->GetDuctEntry(ductName);
   if ( pDuctEntry == nullptr )
   {
      // sometimes the duct name comes back as the index into the choice list
      // get the duct entry another way
      GET_IFACE2(pBroker,ILibraryNames,pLibNames);
      std::vector<std::_tstring> vNames;
      pLibNames->EnumDuctNames(&vNames);
      IndexType ductNameIdx = (IndexType)_tstol(ductName);
      pDuctEntry = pLib->GetDuctEntry(vNames[ductNameIdx].c_str());
      if ( pDuctEntry == nullptr )
      {
         return;
      }
   }
   Float64 A = pDuctEntry->GetInsideArea();

   CSplicedGirderGeneralPage* pParent = (CSplicedGirderGeneralPage*)GetParent();
   const matPsStrand* pStrand = pParent->GetStrand();
   Float64 aps = pStrand->GetNominalArea();

   // LRFD 5.4.6.2 Area of duct must be at least K times net area of prestressing steel
   GET_IFACE2(pBroker,IDuctLimits,pDuctLimits);
   Float64 K = pDuctLimits->GetTendonAreaLimit(pParent->GetInstallationType());

   StrandIndexType maxStrands = (StrandIndexType)fabs(A/(K*aps));

   SetStyleRange(CGXRange(nRow,nNumStrandCol), CGXStyle()
      .SetUserAttribute(GX_IDS_UA_SPINBOUND_MIN,0L)
      .SetUserAttribute(GX_IDS_UA_SPINBOUND_MAX,(LONG)maxStrands)
      .SetUserAttribute(GX_IDS_UA_SPINBOUND_WRAP,1L)
      );

   nStrands = Min(nStrands, maxStrands);
   SetValueRange(CGXRange(nRow,nNumStrandCol),(LONG)nStrands);

   // If the Calc Pjack box is unchecked, update the jacking force
   if ( GetCellValue(nRow,nPjackCheckCol) == _T("0") )
   {
      UpdateMaxPjack(nRow);
   }
}

void CDuctGrid::UpdateMaxPjack(ROWCOL nRow)
{
   StrandIndexType nStrands = (StrandIndexType)_tstoi(GetCellValue(nRow,nNumStrandCol));
   CSplicedGirderGeneralPage* pParent = (CSplicedGirderGeneralPage*)GetParent();
   const matPsStrand* pStrand = pParent->GetStrand();

   Float64 Pjack = lrfdPsStrand::GetPjackPT(*pStrand,nStrands);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GetParam()->SetLockReadOnly(FALSE);
   SetValueRange(CGXRange(nRow,nPjackCol),FormatDimension(Pjack,pDisplayUnits->GetGeneralForceUnit(),false));
   GetParam()->SetLockReadOnly(TRUE);
}

bool CDuctGrid::ComputePjackMax(ROWCOL row)
{
   return (0 < _tstoi(GetCellValue(row,nPjackCheckCol)) ? false : true);
}

void CDuctGrid::OnStrandChanged()
{
   ROWCOL nRows = GetRowCount();
   for ( ROWCOL row = 0; row < nRows; row++ )
   {
      if ( ComputePjackMax(row+1) )
      {
         UpdateMaxPjack(row+1);
      }
   }
}

void CDuctGrid::OnInstallationTypeChanged()
{
   ROWCOL nRows = GetRowCount();
   for ( ROWCOL row = 0; row < nRows; row++ )
   {
      UpdateNumStrandsList(row+1);
   }
}

void CDuctGrid::GetDuctData(ROWCOL row,CDuctData& duct,EventIndexType& stressingEvent)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,ILibraryNames,pLibNames);
   std::vector<std::_tstring> vNames;
   pLibNames->EnumDuctNames(&vNames);

   IndexType ductNameIdx = _tstoi(GetCellValue(row,nDuctTypeCol));
   duct.Name             = vNames[ductNameIdx];
   duct.nStrands         = _tstoi(GetCellValue(row,nNumStrandCol));
   duct.JackingEnd       = (pgsTypes::JackingEndType)_tstoi(GetCellValue(row,nJackEndCol));
   duct.bPjCalc          = ComputePjackMax(row);
   duct.Pj               = _tstof(GetCellValue(row,nPjackCol));
   duct.Pj               = ::ConvertToSysUnits(duct.Pj,pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure);
   duct.LastUserPj       = _tstof(GetCellValue(row,nPjackUserCol));
   duct.LastUserPj       = ::ConvertToSysUnits(duct.LastUserPj,pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure);
   duct.DuctGeometryType = (CDuctGeometry::GeometryType)_tstoi(GetCellValue(row,nDuctGeomTypeCol));

   stressingEvent = (EventIndexType)_tstoi(GetCellValue(row,nEventCol)) + m_LastSegmentErectionEventIdx;
}

void CDuctGrid::SetDuctData(ROWCOL row,const CDuctData& duct,EventIndexType stressingEvent)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GetParam()->SetLockReadOnly(FALSE);

   GET_IFACE2(pBroker,ILibraryNames,pLibNames);
   std::vector<std::_tstring> vNames;
   pLibNames->EnumDuctNames(&vNames);
   std::vector<std::_tstring>::iterator iter(vNames.begin());
   std::vector<std::_tstring>::iterator end(vNames.end());
   short idx = 0;
   for ( ; iter != end; iter++, idx++ )
   {
      if ( duct.Name == *iter )
      {
         break;
      }
   }

   SetValueRange(CGXRange(row,nDuctTypeCol),    idx);
   SetValueRange(CGXRange(row,nNumStrandCol),   (LONG)duct.nStrands);
   SetValueRange(CGXRange(row,nJackEndCol),     (LONG)duct.JackingEnd);
   SetValueRange(CGXRange(row,nPjackCheckCol),  (LONG)!duct.bPjCalc);
   SetValueRange(CGXRange(row,nPjackCol),       ::ConvertFromSysUnits(duct.Pj,        pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure));
   SetValueRange(CGXRange(row,nPjackUserCol),   ::ConvertFromSysUnits(duct.LastUserPj,pDisplayUnits->GetGeneralForceUnit().UnitOfMeasure));
   SetValueRange(CGXRange(row,nDuctGeomTypeCol),(LONG)duct.DuctGeometryType);
   SetValueRange(CGXRange(row,nEventCol),       (LONG)(stressingEvent- m_LastSegmentErectionEventIdx));
   
   OnCalcPjack(row);

   GetParam()->SetLockReadOnly(TRUE);
}

void CDuctGrid::EventCreated()
{
   // called when an event is created outside of this grid
   // DON'T CALL THIS METHOD FOR EVENTS CREATED FROM WITHIN THIS GRID
   ROWCOL nRows = GetRowCount();
   for ( ROWCOL row = 0; row < nRows; row++ )
   {
      UpdateEventList(row);
   }

   FillGrid();
}

void CDuctGrid::FillGrid()
{
   CSplicedGirderGeneralPage* pParent = (CSplicedGirderGeneralPage*)GetParent();

   // clear out the grid before filling it up.
   ROWCOL nRows = GetRowCount();
   if ( 0 < nRows )
   {
      RemoveRows(1,nRows);
   }

   ROWCOL row = 1;
   DuctIndexType nDucts = m_pPTData->GetDuctCount();
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      const CDuctData* pDuct = m_pPTData->GetDuct(ductIdx);
      EventIndexType eventIdx = pParent->m_TendonStressingEvent[ductIdx];
      AddDuct(*pDuct,eventIdx);
   }
}

void CDuctGrid::UpdatePTData()
{
   CSplicedGirderGeneralPage* pParent = (CSplicedGirderGeneralPage*)GetParent();
   ROWCOL nRows = GetRowCount();
   for ( ROWCOL row = 0; row < nRows; row++ )
   {
      CDuctData* pDuctData = m_pPTData->GetDuct((DuctIndexType)row);
      EventIndexType eventIdx;
      DuctIndexType ductIdx = (DuctIndexType)row;
      GetDuctData(row+1,*pDuctData,eventIdx);
      pParent->m_TendonStressingEvent[ductIdx] = eventIdx;
   }
}

void CDuctGrid::OnChangedSelection(const CGXRange* changedRect,BOOL bIsDragging,BOOL bKey)
{
   BOOL bEnable = TRUE;
   if (changedRect && changedRect->left == 0 )
   {
      for ( ROWCOL row = changedRect->top; row <= changedRect->bottom; row++ )
      {
         if ( !m_pPTData->CanRemoveDuct(DuctIndexType(row-1)) )
         {
            bEnable = FALSE;
            break;
         }
      }
   }
   else
   {
      bEnable = FALSE;
   }

   CWnd* pParent = GetParent();
   pParent->GetDlgItem(IDC_DELETE)->EnableWindow(bEnable);
}


EventIndexType CDuctGrid::CreateEvent()
{
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)(GetParent()->GetParent());
   CTimelineEventDlg dlg(*(pParent->m_BridgeDescription.GetTimelineManager()),INVALID_INDEX,FALSE, m_LastSegmentErectionEventIdx);
   if ( dlg.DoModal() == IDOK )
   {
      EventIndexType eventIdx;
      int result = pParent->m_BridgeDescription.GetTimelineManager()->AddTimelineEvent(*dlg.m_pTimelineEvent,true,&eventIdx);
      return eventIdx;
   }

   return INVALID_INDEX;
}

BOOL CDuctGrid::CanActivateGrid(BOOL bActivate)
{
   if ( !bActivate )
   {
      // make sure all the grid data is active
      ROWCOL nRows = GetRowCount();
      for ( ROWCOL row = 0; row < nRows; row++ )
      {
         // Verify all ducts have a stressing event defined
         CDuctData* pDuctData = m_pPTData->GetDuct((DuctIndexType)row);
         EventIndexType eventIdx;
         DuctIndexType ductIdx = (DuctIndexType)row;
         GetDuctData(row+1,*pDuctData,eventIdx);

         if ( pDuctData->nStrands == 0 )
         {
            CString strMsg;
            strMsg.Format(_T("Duct %d does not have any strands"),LABEL_DUCT(ductIdx));
            AfxMessageBox(strMsg);
            return FALSE;
         }

         if ( pDuctData->Pj < 0 )
         {
            CString strMsg;
            strMsg.Format(_T("The jacking force in Duct %d must be a positive value"),LABEL_DUCT(ductIdx));
            AfxMessageBox(strMsg);
            return FALSE;
         }

         if ( eventIdx == INVALID_INDEX )
         {
            AfxMessageBox(_T("Tendon stressing events must be defined"));
            return FALSE;
         }
      }
   }

   return CGXGridWnd::CanActivateGrid(bActivate);
}
