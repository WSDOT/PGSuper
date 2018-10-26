// ConstructSegmentsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "ConstructSegmentsDlg.h"

#include <IFace\Project.h>
#include <IFace\DocumentType.h>
#include <PgsExt\BridgeDescription2.h>

#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

// CConstructSegmentsDlg dialog

IMPLEMENT_DYNAMIC(CConstructSegmentsDlg, CDialog)

CConstructSegmentsDlg::CConstructSegmentsDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent /*=NULL*/)
	: CDialog(CConstructSegmentsDlg::IDD, pParent),
   m_EventIndex(eventIdx),
   m_bReadOnly(bReadOnly)
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
      DDV_GreaterThanZero(pDX,IDC_AGE,ageAtRelease);

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
   ON_BN_CLICKED(ID_HELP, &CConstructSegmentsDlg::OnHelp)
END_MESSAGE_MAP()


// CConstructSegmentsDlg message handlers

BOOL CConstructSegmentsDlg::OnInitDialog()
{
   m_lbSource.Initialize(CTimelineItemListBox::Source,CTimelineItemListBox::ID,&m_lbTarget);
   m_lbTarget.Initialize(CTimelineItemListBox::Target,CTimelineItemListBox::ID,&m_lbSource);

   CDialog::OnInitDialog();

   FillLists();

   if ( m_bReadOnly )
   {
      GetDlgItem(IDC_SOURCE_LIST)->EnableWindow(FALSE);
      GetDlgItem(IDC_TARGET_LIST)->EnableWindow(FALSE);
      GetDlgItem(IDC_MOVE_RIGHT)->EnableWindow(FALSE);
      GetDlgItem(IDC_MOVE_LEFT)->EnableWindow(FALSE);
      GetDlgItem(IDC_RELAXATION_TIME)->EnableWindow(FALSE);
      GetDlgItem(IDC_RELAXATION_TIME_UNIT)->EnableWindow(FALSE);
      GetDlgItem(IDC_AGE)->EnableWindow(FALSE);
      GetDlgItem(IDC_AGE_UNIT)->EnableWindow(FALSE);

      GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
      GetDlgItem(IDCANCEL)->SetWindowText(_T("Close"));
      SetDefID(IDCANCEL);
   }


   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   if ( pDocType->IsPGSuperDocument() )
   {
      SetWindowText(_T("Construct Girders"));
      GetDlgItem(IDC_SOURCE_LABEL)->SetWindowText(_T("Girders to be constructed"));
      GetDlgItem(IDC_TARGET_LABEL)->SetWindowText(_T("Girders constructed during this activity"));
      GetDlgItem(IDC_NOTE)->SetWindowText(_T("This activity includes lifting of the precast girders from the casting bed and placing them into storage."));
   }
   else
   {
      SetWindowText(_T("Construct Segments"));
      GetDlgItem(IDC_SOURCE_LABEL)->SetWindowText(_T("Segments to be constructed"));
      GetDlgItem(IDC_TARGET_LABEL)->SetWindowText(_T("Segments constructed during this activity"));
      GetDlgItem(IDC_NOTE)->SetWindowText(_T("This activity includes lifting of the precast segments from the casting bed and placing them into storage."));
   }

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
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IDocumentType,pDocType);

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
            if ( pDocType->IsPGSuperDocument() )
            {
               label.Format(_T("Span %d, Girder %s"),LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx));
            }
            else
            {
               label.Format(_T("Group %d, Girder %s, Segment %d"),LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_SEGMENT(segIdx));
            }

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

void CConstructSegmentsDlg::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_CONSTRUCT_SEGMENTS);
}
