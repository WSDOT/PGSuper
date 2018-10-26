// ErectSegmentsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "ErectSegmentsDlg.h"

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>

// CErectSegmentsDlg dialog

//////////////////////////////////////////////////////////////////
// NOTE
//
// All segments at a position in a group are assumed to be erected
// at the same time. That is, Segment #2 for all girders in Group #1 
// are erected at the same time.
//
// The erect segment activity keeps track of the IDs of each individual
// girder that is erected.
//
// This dialog does some complex things so that it can list
// Group 1, Segment 2 while getting and setting the activity
// data for all the girders in the effected group.

IMPLEMENT_DYNAMIC(CErectSegmentsDlg, CDialog)

CErectSegmentsDlg::CErectSegmentsDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,CWnd* pParent /*=NULL*/)
	: CDialog(CErectSegmentsDlg::IDD, pParent),
   m_EventIndex(eventIdx)
{
   m_TimelineMgr = timelineMgr;
   m_pBridgeDesc = m_TimelineMgr.GetBridgeDescription();
}

CErectSegmentsDlg::~CErectSegmentsDlg()
{
}

void CErectSegmentsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   DDX_Control(pDX,IDC_SOURCE_LIST,m_lbSource);
   DDX_Control(pDX,IDC_TARGET_LIST,m_lbTarget);

   if ( pDX->m_bSaveAndValidate )
   {
      int nItems = m_lbTarget.GetCount();
      for ( int itemIdx = 0; itemIdx < nItems; itemIdx++ )
      {
         CTimelineItemIDDataPtr* pItemData = (CTimelineItemIDDataPtr*)m_lbTarget.GetItemDataPtr(itemIdx);
         SegmentIDType segmentID = pItemData->m_ID;

         m_TimelineMgr.SetSegmentErectionEventByIndex(segmentID,m_EventIndex);
      }

      nItems = m_lbSource.GetCount();
      for ( int itemIdx = 0; itemIdx < nItems; itemIdx++ )
      {
         CTimelineItemIDDataPtr* pItemData = (CTimelineItemIDDataPtr*)m_lbSource.GetItemDataPtr(itemIdx);
         if ( pItemData->m_State == CTimelineItemDataPtr::Used )
         {
            continue;
         }

         SegmentIDType segmentID = pItemData->m_ID;

         m_TimelineMgr.SetSegmentErectionEventByIndex(segmentID,INVALID_INDEX);
      }
   }
}


BEGIN_MESSAGE_MAP(CErectSegmentsDlg, CDialog)
   ON_BN_CLICKED(IDC_MOVE_RIGHT, &CErectSegmentsDlg::OnMoveToTargetList)
   ON_BN_CLICKED(IDC_MOVE_LEFT, &CErectSegmentsDlg::OnMoveToSourceList)
END_MESSAGE_MAP()


// CErectSegmentsDlg message handlers

BOOL CErectSegmentsDlg::OnInitDialog()
{
   m_lbSource.Initialize(CTimelineItemListBox::Source,CTimelineItemListBox::ID,&m_lbTarget);
   m_lbTarget.Initialize(CTimelineItemListBox::Target,CTimelineItemListBox::ID,&m_lbSource);

   CDialog::OnInitDialog();

   FillLists();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CErectSegmentsDlg::OnMoveToTargetList()
{
   m_lbSource.MoveSelectedItemsToBuddy();
}

void CErectSegmentsDlg::OnMoveToSourceList()
{
   m_lbTarget.MoveSelectedItemsToBuddy();
}

void CErectSegmentsDlg::FillLists()
{
   GroupIndexType nGroups = m_pBridgeDesc->GetGirderGroupCount();
   for( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(grpIdx);

      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         SegmentIndexType nSegments = pGirder->GetSegmentCount();
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
            SegmentIDType segmentID = pSegment->GetID();

            bool bIsSegmentErected = m_TimelineMgr.IsSegmentErected(segmentID);
            EventIndexType erectionEventIdx = m_TimelineMgr.GetSegmentErectionEventIndex(segmentID);

            CString label;
            label.Format(_T("Group %d, Girder %s, Segment %d"),LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_SEGMENT(segIdx));

            if ( erectionEventIdx == m_EventIndex )
            {
               // erected during this event, put it in the target list
               m_lbTarget.AddItem(label,CTimelineItemDataPtr::Used,segmentID);
            }
            else
            {
               // not erected during this event, put it in the source list
               CTimelineItemDataPtr::State state = (bIsSegmentErected ? CTimelineItemDataPtr::Used : CTimelineItemDataPtr::Unused);
               m_lbSource.AddItem(label,state,segmentID);
            }
         } // segment loop
      } // girder loop
   } // group loop
}
