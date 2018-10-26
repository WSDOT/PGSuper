///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// TimelineGrid.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "TimelineGrid.h"
#include "TimelineEventDlg.h"

#include "EditTimelineDlg.h"
#include <PgsExt\TimelineManager.h>

#include <System\Tokenizer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CTimelineGrid, CS_DBLCLKS, 0, 0, 0);

/////////////////////////////////////////////////////////////////////////////
// CTimelineGrid

CTimelineGrid::CTimelineGrid()
{
//   RegisterClass();
}

CTimelineGrid::~CTimelineGrid()
{
}

BEGIN_MESSAGE_MAP(CTimelineGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CTimelineGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CTimelineGrid message handlers

void CTimelineGrid::CustomInit()
{
// Initialize the grid. For CWnd based grids this call is // 
// essential. For view based grids this initialization is done 
// in OnInitialUpdate.
	Initialize( );
   GetParam()->EnableUndo(FALSE);

		// Turn off selecting whole columns when clicking on a column header
	GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELTABLE & ~GX_SELCOL));

   // no row moving
	GetParam()->EnableMoveRows(FALSE);
   GetParam()->EnableMoveCols(FALSE);

   const int num_rows = 0;
   const int num_cols = 3;

	SetRowCount(num_rows);
	SetColCount(num_cols);

   ROWCOL col = 0;

   // disable left side
	SetStyleRange(CGXRange(0,0,num_rows,0), CGXStyle()
			.SetControl(GX_IDS_CTRL_HEADER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   // set text along top row
	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetValue(_T("Event"))
		);

	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Day"))
		);

	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Description"))
		);

	SetStyleRange(CGXRange(0,col++), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T(""))
		);

   // don't allow users to resize grids
   GetParam( )->EnableTrackColWidth(0); 
   GetParam( )->EnableTrackRowHeight(0); 

	EnableIntelliMouse();

   EnableGridToolTips();

   GetParam()->EnableUndo(TRUE);

	SetFocus();
}

void CTimelineGrid::Refresh()
{
   CRowColArray selection;
   ROWCOL nSelRows = GetSelectedRows(selection,TRUE);

   CEditTimelineDlg* pParent = (CEditTimelineDlg*)GetParent();
   EventIndexType nEvents = pParent->m_TimelineManager.GetEventCount();

   if ( 0 < GetRowCount() )
      RemoveRows(1,GetRowCount());

   for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pParent->m_TimelineManager.GetEventByIndex(eventIdx);
      AddEvent(pTimelineEvent);
   }

   ResizeColWidthsToFit(CGXRange(0,0,GetRowCount(),GetColCount()));

   for ( ROWCOL selIdx = 0; selIdx < nSelRows; selIdx++ )
   {
      ROWCOL row = selection.GetAt(selIdx);
      SelectRange(CGXRange().SetRows(row));
   }
}

void CTimelineGrid::AddEvent(const CTimelineEvent* pTimelineEvent)
{
   ROWCOL row = GetRowCount()+1;

   GetParam()->EnableUndo(FALSE);
   InsertRows(row,1);

   ROWCOL col = 0;

   CEditTimelineDlg* pParent = (CEditTimelineDlg*)GetParent();
   EventIndexType eventIdx = IndexType(row-1);

	SetStyleRange(CGXRange(row,col++), CGXStyle()
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
		);

   SetStyleRange(CGXRange(row,col++), CGXStyle()
         .SetHorizontalAlignment(DT_RIGHT)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(pTimelineEvent->GetDay())
		);

	SetStyleRange(CGXRange(row,col++), CGXStyle()
         .SetHorizontalAlignment(DT_LEFT)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(pTimelineEvent->GetDescription())
		);

	SetStyleRange(CGXRange(row,col++), CGXStyle()
		.SetControl(GX_IDS_CTRL_PUSHBTN)
		.SetChoiceList(_T("Edit"))
		);

   GetParam()->EnableUndo(TRUE);
}

void CTimelineGrid::OnClickedButtonRowCol(ROWCOL nRow,ROWCOL nCol)
{
   if ( nCol != 3 )
      return;

   CEditTimelineDlg* pParent = (CEditTimelineDlg*)GetParent();
   CTimelineEventDlg dlg(&pParent->m_TimelineManager,TRUE);
   EventIndexType eventIdx = (IndexType)(nRow-1);
   dlg.m_EventIdx = eventIdx;
   dlg.m_TimelineEvent = *pParent->m_TimelineManager.GetEventByIndex(eventIdx);
   dlg.m_TimelineEvent.SetID( pParent->m_TimelineManager.GetEventByIndex(eventIdx)->GetID() );
   if ( dlg.DoModal() == IDOK )
   {
      bool bDone = false;
      bool bAdjustTimeline = false;
      while ( !bDone )
      {
         int result = pParent->m_TimelineManager.SetEventByIndex(eventIdx,dlg.m_TimelineEvent,bAdjustTimeline);
         if ( result == TLM_SUCCESS )
         {
            bDone = true;
            Refresh();
         }
         else
         {
            CString strProblem;
            if (result == TLM_OVERLAPS_PREVIOUS_EVENT )
               strProblem = _T("This event begins before the activities in the previous event have completed.");
            else
               strProblem = _T("The activities in this event end after the next event begins.");

            CString strRemedy(_T("Should the timeline be adjusted to accomodate this event?"));

            CString strMsg;
            strMsg.Format(_T("%s\n\n%s"),strProblem,strRemedy);
            if ( AfxMessageBox(strMsg,MB_OKCANCEL | MB_ICONQUESTION) == IDOK )
            {
               bAdjustTimeline = true;
            }
            else
            {
               return;
            }
         }
      }
   }
}

void CTimelineGrid::RemoveEvents()
{
   GetParam()->EnableUndo(FALSE);

   CEditTimelineDlg* pParent = (CEditTimelineDlg*)GetParent();

   CRowColArray selection;
   ROWCOL nSelRows = GetSelectedRows(selection,TRUE);

   if ( nSelRows == 0 )
      return;

   // keep a copy in case we have to roll back
   CTimelineManager timelineManager = pParent->m_TimelineManager;

   for ( int i = nSelRows-1; 0 <= i; i-- ) // work backwords
   {
      ROWCOL row = selection.GetAt(i);
      EventIndexType eventIdx = IndexType(row-1);

      int result = pParent->m_TimelineManager.RemoveEventByIndex(eventIdx);
      if ( result != TLM_SUCCESS )
      {
         CString strMsg;
         switch(result)
         {
         case TLM_CAST_DECK_ACTIVITY_REQUIRED:
            strMsg.Format(_T("Cannot remove Event %d because the bridge has a composite deck and this event includes the deck casting activity."),LABEL_EVENT(eventIdx));
            break;
         
         case TLM_OVERLAY_ACTIVITY_REQUIRED:
            strMsg.Format(_T("Cannot remove Event %d because the bridge has an overlay and this event includes the overlay loading activity."),LABEL_EVENT(eventIdx));
            break;
         
         case TLM_RAILING_SYSTEM_ACTIVITY_REQUIRED:
            strMsg.Format(_T("Cannot remove Event %d because it contains the traffic barrier/railing system loading activity."),LABEL_EVENT(eventIdx));
            break;

         case TLM_LIVELOAD_ACTIVITY_REQUIRED:
            strMsg.Format(_T("Cannot remove Event %d because it contains the live load loading activity."),LABEL_EVENT(eventIdx));
            break;

         case TLM_USER_LOAD_ACTIVITY_REQUIRED:
            strMsg.Format(_T("Cannot remove Event %d because it contains a loading activity for user defined loads."),LABEL_EVENT(eventIdx));
            break;

         case TLM_CONSTRUCT_SEGMENTS_ACTIVITY_REQUIRED:
            strMsg.Format(_T("Cannot remove Event %d because it contains the segment construction activity."),LABEL_EVENT(eventIdx));
            break;

         case TLM_ERECT_PIERS_ACTIVITY_REQUIRED:
            strMsg.Format(_T("Cannot remove Event %d because it contains the Pier/Temporary Support erection activity."),LABEL_EVENT(eventIdx));
            break;

         case TLM_ERECT_SEGMENTS_ACTIVITY_REQUIRED:
            strMsg.Format(_T("Cannot remove Event %d because it contains the segment erection activity."),LABEL_EVENT(eventIdx));
            break;

         case TLM_REMOVE_TEMPORARY_SUPPORTS_ACTIVITY_REQUIRED:
            strMsg.Format(_T("Cannot remove Event %d because it contains an activity for removing temporary supports."),LABEL_EVENT(eventIdx));
            break;

         case TLM_CAST_CLOSURE_POUR_ACTIVITY_REQUIRED:
            strMsg.Format(_T("Cannot remove Event %d because it contains an activity for casting closure pours."),LABEL_EVENT(eventIdx));
            break;

         case TLM_STRESS_TENDONS_ACTIVITY_REQUIRED:
            strMsg.Format(_T("Cannot remove Event %d because it contains the activity for stressing tendons."),LABEL_EVENT(eventIdx));
            break;

         default:
            ATLASSERT(false); // should never get here
            strMsg = _T("An unknown error has occured");
            break;
         }
         AfxMessageBox(strMsg,MB_OK | MB_ICONEXCLAMATION);

         // roll back
         pParent->m_TimelineManager = timelineManager;

         // leave before the grid is actually changed
         return;
      }
   }

   RemoveRows(selection.GetAt(0),selection.GetAt(nSelRows-1));

   GetParam()->EnableUndo(TRUE);
}

BOOL CTimelineGrid::OnValidateCell(ROWCOL nRow,ROWCOL nCol)
{
   if ( nCol == 1 )
   {
      CEditTimelineDlg* pParent = (CEditTimelineDlg*)GetParent();
      EventIndexType eventIdx = IndexType(nRow-1);

      CGXControl* pControl = GetControl(nRow,nCol);
      CString strDay;
      pControl->GetCurrentText(strDay);
      Float64 day;
      VERIFY(sysTokenizer::ParseDouble(strDay,&day));

      bool bDone = false;
      bool bAdjustTimeline = false;
      while ( !bDone )
      {
         int result = pParent->m_TimelineManager.AdjustDayByIndex(eventIdx,day,bAdjustTimeline);
         if ( result == TLM_SUCCESS )
         {
            bDone = true;
         }
         else
         {
            CString strProblem;
            if (result == TLM_OVERLAPS_PREVIOUS_EVENT )
               strProblem = _T("This event begins before the activities in the previous event have completed.");
            else
               strProblem = _T("The activities in this event end after the next event begins.");

            CString strRemedy(_T("Should the timeline be adjusted to accomodate this event?"));

            CString strMsg;
            strMsg.Format(_T("%s\n\n%s"),strProblem,strRemedy);
            if ( AfxMessageBox(strMsg,MB_OKCANCEL | MB_ICONQUESTION) == IDOK )
            {
               bAdjustTimeline = true;
            }
            else
            {
               TransferCurrentCell(FALSE); // reverts to the current value
               return FALSE; // don't change the cell
            }
         }
      }
   }

   return CGXGridWnd::OnValidateCell(nRow,nCol);
}

BOOL CTimelineGrid::OnEndEditing(ROWCOL nRow,ROWCOL nCol)
{
   // The function Refresh(), called below, causes recursion into
   // this function. To avoid that problem, we'll keep a static
   // variable to keep track if this function has already been entered
   static bool bHasThisMethodBeenCalled = false;
   if ( bHasThisMethodBeenCalled )
      return TRUE;

   bHasThisMethodBeenCalled = true;
   if ( nCol == 1 )
   {
      Refresh();
   }
   else if ( nCol == 2 )
   {
      CEditTimelineDlg* pParent = (CEditTimelineDlg*)GetParent();
      EventIndexType eventIdx = IndexType(nRow-1);

      CString description = GetValueRowCol(nRow,nCol);
      pParent->m_TimelineManager.GetEventByIndex(eventIdx)->SetDescription(description);
   }

   bHasThisMethodBeenCalled = false;
   return CGXGridWnd::OnEndEditing(nRow,nCol);
}
