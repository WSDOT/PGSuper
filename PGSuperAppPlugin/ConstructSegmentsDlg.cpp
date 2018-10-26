// ConstructSegmentsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "ConstructSegmentsDlg.h"

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>


#include <EAF\EAFDisplayUnits.h>

// CConstructSegmentsDlg dialog

IMPLEMENT_DYNAMIC(CConstructSegmentsDlg, CDialog)

CConstructSegmentsDlg::CConstructSegmentsDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,CWnd* pParent /*=NULL*/)
	: CDialog(CConstructSegmentsDlg::IDD, pParent),
   m_EventIndex(eventIdx)
{
   m_TimelineMgr = timelineMgr;
   m_pBridgeDesc = m_TimelineMgr.GetBridgeDescription();
}

CConstructSegmentsDlg::~CConstructSegmentsDlg()
{
}

void CConstructSegmentsDlg::DoDataExchange(CDataExchange* pDX)
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

         m_TimelineMgr.SetSegmentConstructionEventByIndex(segmentID,m_EventIndex);
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

         m_TimelineMgr.SetSegmentConstructionEventByIndex(segmentID,INVALID_INDEX);
      }

      CTimelineEvent* pEvent = m_TimelineMgr.GetEventByIndex(m_EventIndex);
      Float64 relaxationTime;
      Float64 ageAtRelease;
      DDX_Text(pDX,IDC_RELAXATION_TIME,relaxationTime);
      DDX_Text(pDX,IDC_AGE,ageAtRelease);

      if ( relaxationTime < ageAtRelease )
      {
         pDX->PrepareEditCtrl(IDC_AGE);
         AfxMessageBox(_T("Concrete age at release must be less than or equal to the time between strand stressing and release"),MB_ICONWARNING);
         pDX->Fail();
      }

      pEvent->GetConstructSegmentsActivity().SetRelaxationTime(relaxationTime);
      pEvent->GetConstructSegmentsActivity().SetAgeAtRelease(ageAtRelease);
   }
   else
   {
      CTimelineEvent* pEvent = m_TimelineMgr.GetEventByIndex(m_EventIndex);
      Float64 relaxationTime = pEvent->GetConstructSegmentsActivity().GetRelaxationTime();
      Float64 ageAtRelease = pEvent->GetConstructSegmentsActivity().GetAgeAtRelease();
      DDX_Text(pDX,IDC_RELAXATION_TIME,relaxationTime);
      DDX_Text(pDX,IDC_AGE,ageAtRelease);
   }
}


BEGIN_MESSAGE_MAP(CConstructSegmentsDlg, CDialog)
   ON_BN_CLICKED(IDC_MOVE_RIGHT, &CConstructSegmentsDlg::OnMoveToTargetList)
   ON_BN_CLICKED(IDC_MOVE_LEFT, &CConstructSegmentsDlg::OnMoveToSourceList)
END_MESSAGE_MAP()


// CConstructSegmentsDlg message handlers

BOOL CConstructSegmentsDlg::OnInitDialog()
{
   m_lbSource.Initialize(CTimelineItemListBox::Source,CTimelineItemListBox::ID,&m_lbTarget);
   m_lbTarget.Initialize(CTimelineItemListBox::Target,CTimelineItemListBox::ID,&m_lbSource);

   CDialog::OnInitDialog();

   FillLists();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CConstructSegmentsDlg::OnMoveToTargetList()
{
   m_lbSource.MoveSelectedItemsToBuddy();
}

void CConstructSegmentsDlg::OnMoveToSourceList()
{
   m_lbTarget.MoveSelectedItemsToBuddy();
}

void CConstructSegmentsDlg::FillLists()
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

            bool bIsSegmentConstructed = m_TimelineMgr.IsSegmentConstructed(segmentID);
            EventIndexType constructEventIdx = m_TimelineMgr.GetSegmentConstructionEventIndex(segmentID);

            CString label;
            label.Format(_T("Group %d, Girder %s, Segment %d"),LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_SEGMENT(segIdx));

            if ( constructEventIdx == m_EventIndex )
            {
               m_lbTarget.AddItem(label,CTimelineItemDataPtr::Used,segmentID);
            }
            else
            {
               CTimelineItemDataPtr::State state = (bIsSegmentConstructed ? CTimelineItemDataPtr::Used : CTimelineItemDataPtr::Unused);
               m_lbSource.AddItem(label,state,segmentID);
            }
         } // segment loop
      } // girder loop
   } // group loop
}
