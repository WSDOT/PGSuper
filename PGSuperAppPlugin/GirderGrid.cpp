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

// GirderGrid.cpp : implementation file
//

#include "stdafx.h"
#include "GirderGrid.h"
#include "SplicedGirderDescDlg.h"
#include "GirderDescDlg.h"

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\PrecastSegmentData.h>
#include <PgsExt\ClosureJointData.h>

#include "GirderSegmentDlg.h"
#include "ClosureJointDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

GRID_IMPLEMENT_REGISTER(CGirderGrid, CS_DBLCLKS, 0, 0, 0);
const ROWCOL nEditCol = 1;

class CRowType : public CGXAbstractUserAttribute
{
public:
   enum RowType { Segment, Closure };
   CRowType(RowType type,CollectionIndexType idx)
   {
      m_Index = idx;
      m_Type = type;
   }

   virtual CGXAbstractUserAttribute* Clone() const
   {
      return new CRowType(m_Type,m_Index);
   }

   RowType m_Type;
   CollectionIndexType m_Index;
};

/////////////////////////////////////////////////////////////////////////////
// CGirderGrid

CGirderGrid::CGirderGrid()
{
//   RegisterClass();
}

CGirderGrid::~CGirderGrid()
{
}

int CGirderGrid::GetColWidth(ROWCOL nCol)
{
   CRect rect = GetGridRect( );
   return rect.Width()/2;
}

BEGIN_MESSAGE_MAP(CGirderGrid, CGXGridWnd)
	//{{AFX_MSG_MAP(CGirderGrid)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CGirderGrid::CustomInit()
{
   // Initialize the grid. For CWnd based grids this call is // 
   // essential. For view based grids this initialization is done 
   // in OnInitialUpdate.
	this->Initialize( );

	this->GetParam( )->EnableUndo(FALSE);

   const int num_rows=0;
   const int num_cols=1;

	this->SetRowCount(num_rows);
	this->SetColCount(num_cols);

	// Turn off selecting whole columns when clicking on a column header
	this->GetParam()->EnableSelection((WORD) (GX_SELFULL & ~GX_SELCOL & ~GX_SELTABLE & ~GX_SELROW));

   SetMergeCellsMode(gxnMergeEvalOnDisplay);
   SetScrollBarMode(SB_HORZ,gxnDisabled);
   SetScrollBarMode(SB_VERT,gxnAutomatic | gxnEnabled | gxnEnhanced);

   // no row moving
	this->GetParam()->EnableMoveRows(FALSE);

   // set text along top row
	this->SetStyleRange(CGXRange(0,0), CGXStyle()
         .SetWrapText(TRUE)
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetEnabled(FALSE)          // disables usage as current cell
			.SetValue(_T("Type"))
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		);

	this->SetStyleRange(CGXRange(0,1), CGXStyle()
         .SetWrapText(TRUE)
			.SetEnabled(FALSE)          // disables usage as current cell
         .SetHorizontalAlignment(DT_CENTER)
         .SetVerticalAlignment(DT_VCENTER)
			.SetValue(_T("Type"))
         .SetMergeCell(GX_MERGE_HORIZONTAL | GX_MERGE_COMPVALUE)
		);

   // don't allow users to resize grids
   this->GetParam( )->EnableTrackColWidth(0); 
   this->GetParam( )->EnableTrackRowHeight(0); 

	this->EnableIntelliMouse();
	this->SetFocus();

   CSplicedGirderGeneralPage* pParentPage = (CSplicedGirderGeneralPage*)GetParent();
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)(pParentPage->GetParent());

   SegmentIndexType nSegments = pParent->m_pGirder->GetSegmentCount();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CString strLabel;
      strLabel.Format(_T("Segment %d"),LABEL_SEGMENT(segIdx));
      AddRow(strLabel,CRowType(CRowType::Segment,segIdx));

      if ( segIdx < nSegments-1 )
      {
         //CClosureJointData* pClosure = pParent->m_pGirder->GetClosureJoint(segIdx);
         AddRow(_T("Closure Joint"),CRowType(CRowType::Closure,segIdx));
      }
   }

   this->HideRows(0,0);

	this->GetParam( )->EnableUndo(TRUE);
}

void CGirderGrid::AddRow(LPCTSTR lpszGirderName,CGXAbstractUserAttribute& rowType)
{
	ROWCOL nRow = 0;
   nRow = GetRowCount() + 1;

   GetParam()->EnableUndo(FALSE);
	InsertRows(nRow, 1);

   SetStyleRange(CGXRange(nRow,0), CGXStyle()
      .SetHorizontalAlignment(DT_LEFT)
      .SetInterior(::GetSysColor(COLOR_BTNFACE)) // dialog face color
      .SetTextColor(::GetSysColor(COLOR_WINDOWTEXT)) // COLOR_GRAYTEXT
      .SetReadOnly(TRUE)
      .SetEnabled(FALSE)
      .SetValue(lpszGirderName)
      );

   SetStyleRange(CGXRange(nRow,nEditCol), CGXStyle()
		.SetControl(GX_IDS_CTRL_PUSHBTN)
		.SetChoiceList(_T("Edit"))
      .SetUserAttribute(0,rowType)
      );

   GetParam()->EnableUndo(TRUE);
}

void CGirderGrid::OnClickedButtonRowCol(ROWCOL nRow,ROWCOL nCol)
{
   if ( nCol != nEditCol )
      return;

   CGXStyle style;
   GetStyleRowCol(nRow,nEditCol,style);
   if ( style.GetIncludeUserAttribute(0) )
   {
      const CRowType& rowType = dynamic_cast<const CRowType&>(style.GetUserAttribute(0));

      if ( rowType.m_Type == CRowType::Segment )
      {
         EditSegment(rowType.m_Index);
      }
      else
      {
         EditClosure(rowType.m_Index);
      }
   }
}

void CGirderGrid::EditSegment(SegmentIndexType segIdx)
{
   CSplicedGirderGeneralPage* pParentPage = (CSplicedGirderGeneralPage*)GetParent();
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)(pParentPage->GetParent());

   CGirderKey girderKey = pParent->m_GirderKey;
   CSegmentKey segmentKey(girderKey,segIdx);
   CGirderSegmentDlg dlg(&pParent->m_BridgeDescription,segmentKey,pParent->GetExtensionPages(),this);

#pragma Reminder("UPDATE: Clean up handling of shear data")
   // Shear data is kind of messy. It is the only data on the segment that we have to
   // set on the dialog and then get it for the transaction. Updated the dialog
   // so it works seamlessly for all cases
   dlg.m_StirrupsPage.m_ShearData = dlg.m_Girder.GetSegment(segIdx)->ShearData;

   if ( dlg.DoModal() == IDOK )
   {
      EventIndexType constructionEventIdx = dlg.m_TimelineMgr.GetSegmentConstructionEventIndex(dlg.m_SegmentID);
      EventIndexType erectionEventIdx = dlg.m_TimelineMgr.GetSegmentErectionEventIndex(dlg.m_SegmentID);

#pragma Reminder("UPDATE: Clean up handling of shear data")
      // Shear data is kind of messy. It is the only data on the segment that we have to
      // set on the dialog and then get it for the transaction. Updated the dialog
      // so it works seamlessly for all cases
      dlg.m_Girder.GetSegment(segIdx)->ShearData = dlg.m_StirrupsPage.m_ShearData;

      if ( dlg.m_bCopyToAll )
      {
         // copy the data for the segment that was edited to all the segments in this girder
         SegmentIndexType nSegments = pParent->m_pGirder->GetSegmentCount();
         for ( SegmentIndexType idx = 0; idx < nSegments; idx++ )
         {
            pParent->m_pGirder->SetSegment(idx,*dlg.m_Girder.GetSegment(segIdx));

            const CPrecastSegmentData* pSegment = pParent->m_pGirder->GetSegment(idx);
            SegmentIDType segID = pSegment->GetID();
            dlg.m_TimelineMgr.SetSegmentConstructionEventByIndex(segID,constructionEventIdx);
            dlg.m_TimelineMgr.SetSegmentErectionEventByIndex(segID,erectionEventIdx);
         }
      }
      else
      {
         pParent->m_pGirder->SetSegment(segIdx,*dlg.m_Girder.GetSegment(segIdx));
      }

      pParent->m_BridgeDescription.SetTimelineManager(&dlg.m_TimelineMgr);

      if ( dlg.WasEventCreated() )
      {
         pParentPage->EventCreated();
      }

      pParent->Invalidate();
      pParent->UpdateWindow();
   }
}

void CGirderGrid::EditClosure(CollectionIndexType idx)
{
   CSplicedGirderGeneralPage* pParentPage = (CSplicedGirderGeneralPage*)GetParent();
   CSplicedGirderDescDlg* pParent = (CSplicedGirderDescDlg*)(pParentPage->GetParent());

   CClosureKey closureKey(pParent->m_GirderKey,idx);
   CClosureJointDlg dlg(&(pParent->m_BridgeDescription),closureKey,pParent->GetExtensionPages(),this);

   if ( dlg.DoModal() == IDOK )
   {
      EventIndexType eventIdx = dlg.m_TimelineMgr.GetCastClosureJointEventIndex(dlg.m_ClosureID);

      if ( dlg.m_bCopyToAllClosureJoints )
      {
         // copy to all closure joints in this girder
         IndexType nCP = pParent->m_pGirder->GetClosureJointCount();
         for ( IndexType i = 0; i < nCP; i++ )
         {
            CClosureJointData* pCJ = pParent->m_pGirder->GetClosureJoint(i);
            pCJ->CopyClosureJointData(&dlg.m_ClosureJoint);
            IDType cjID = pCJ->GetID();
            dlg.m_TimelineMgr.SetCastClosureJointEventByIndex(cjID,eventIdx);
         }
      }
      else
      {
         pParent->m_pGirder->GetClosureJoint(idx)->CopyClosureJointData(&dlg.m_ClosureJoint);
      }

      pParent->m_BridgeDescription.SetTimelineManager(&dlg.m_TimelineMgr);

      if ( dlg.WasEventCreated() )
      {
         pParentPage->EventCreated();
      }

      pParent->Invalidate();
      pParent->UpdateWindow();
   }
}

