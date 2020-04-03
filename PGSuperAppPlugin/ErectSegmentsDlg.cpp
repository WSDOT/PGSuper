// ErectSegmentsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "ErectSegmentsDlg.h"

#include <IFace\Project.h>
#include <IFace\DocumentType.h>
#include <PgsExt\BridgeDescription2.h>

#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


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

CErectSegmentsDlg::CErectSegmentsDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent /*=nullptr*/)
	: CDialog(CErectSegmentsDlg::IDD, pParent),
   m_EventIndex(eventIdx),
   m_bReadOnly(bReadOnly)
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
   ON_BN_CLICKED(ID_HELP, &CErectSegmentsDlg::OnHelp)
END_MESSAGE_MAP()


// CErectSegmentsDlg message handlers

BOOL CErectSegmentsDlg::OnInitDialog()
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

      GetDlgItem(IDOK)->ShowWindow(SW_HIDE);
      GetDlgItem(IDCANCEL)->SetWindowText(_T("Close"));
      SetDefID(IDCANCEL);
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   if ( pDocType->IsPGSuperDocument() )
   {
      SetWindowText(_T("Erect Girders"));
      GetDlgItem(IDC_SOURCE_LABEL)->SetWindowText(_T("Girders to be erected"));
      GetDlgItem(IDC_TARGET_LABEL)->SetWindowText(_T("Girders erected during this activity"));
      GetDlgItem(IDC_NOTE)->SetWindowText(_T("This activity includes hauling the precast girders from the storage site to the bridge site and the removal of temporary strands, if present."));
   }
   else
   {
      SetWindowText(_T("Erect Segments"));
      GetDlgItem(IDC_SOURCE_LABEL)->SetWindowText(_T("Segments to be erected"));
      GetDlgItem(IDC_TARGET_LABEL)->SetWindowText(_T("Segments erected during this activity"));
      GetDlgItem(IDC_NOTE)->SetWindowText(_T("This activity includes hauling the precast segments from the storage site to the bridge site and the removal of temporary strands, if present."));
   }

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

            bool bIsSegmentErected = m_TimelineMgr.IsSegmentErected(segmentID);
            EventIndexType erectionEventIdx = m_TimelineMgr.GetSegmentErectionEventIndex(segmentID);

            CString label;
            if ( pDocType->IsPGSuperDocument() )
            {
               label.Format(_T("Span %s, Girder %s"),LABEL_SPAN(grpIdx),LABEL_GIRDER(gdrIdx));
            }
            else
            {
               label.Format(_T("Group %d, Girder %s, Segment %d"),LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_SEGMENT(segIdx));
            }

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

void CErectSegmentsDlg::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_ERECT_SEGMENTS);
}
