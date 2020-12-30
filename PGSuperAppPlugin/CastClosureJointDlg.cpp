// CastClosureJointDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "CastClosureJointDlg.h"

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>

#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


bool IsTSIndex(IndexType key) { return MAX_INDEX/2 <= key ? true : false; }
SupportIndexType EncodeTSIndex(SupportIndexType tsIdx) { return MAX_INDEX-tsIdx; }
SupportIndexType DecodeTSIndex(SupportIndexType tsIdx) { return MAX_INDEX-tsIdx; }


// CCastClosureJointDlg dialog

IMPLEMENT_DYNAMIC(CCastClosureJointDlg, CDialog)

CCastClosureJointDlg::CCastClosureJointDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent /*=nullptr*/)
	: CDialog(CCastClosureJointDlg::IDD, pParent)
{
   m_TimelineMgr = timelineMgr;
   m_EventIndex = eventIdx;
   m_bReadOnly = bReadOnly;

   m_pBridgeDesc = m_TimelineMgr.GetBridgeDescription();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   pBroker->GetInterface(IID_IEAFDisplayUnits,(IUnknown**)&m_pDisplayUnits);
}

CCastClosureJointDlg::~CCastClosureJointDlg()
{
}

void CCastClosureJointDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   DDX_Control(pDX,IDC_SOURCE_LIST,m_lbSource);
   DDX_Control(pDX,IDC_TARGET_LIST,m_lbTarget);

   if ( pDX->m_bSaveAndValidate )
   {
      // Set event information for everything in the target list
      int nItems = m_lbTarget.GetCount();
      for ( int itemIdx = 0; itemIdx < nItems; itemIdx++ )
      {
         CTimelineItemIDDataPtr* pItemData = (CTimelineItemIDDataPtr*)m_lbTarget.GetItemDataPtr(itemIdx);
         ClosureIDType closureID = pItemData->m_ID;
         m_TimelineMgr.SetCastClosureJointEventByIndex(closureID,m_EventIndex);
      }

      // Remove event information for everything in the source list that isn't used in another event
      nItems = m_lbSource.GetCount();
      for ( int itemIdx = 0; itemIdx < nItems; itemIdx++ )
      {
         CTimelineItemIDDataPtr* pItemData = (CTimelineItemIDDataPtr*)m_lbSource.GetItemDataPtr(itemIdx);
         if ( pItemData->m_State == CTimelineItemDataPtr::Used )
         {
            // item is used in a different event
            continue;
         }

         ClosureIDType closureID = pItemData->m_ID;
         m_TimelineMgr.SetCastClosureJointEventByIndex(closureID,INVALID_INDEX);
      }

      Float64 age;
      DDX_Text(pDX,IDC_AGE,age);
      DDV_GreaterThanZero(pDX,IDC_AGE,age);
      m_TimelineMgr.GetEventByIndex(m_EventIndex)->GetCastClosureJointActivity().SetTotalCuringDuration(age);

      Float64 cure_duration;
      DDX_Text(pDX,IDC_CURING,cure_duration);
      DDV_NonNegativeDouble(pDX,IDC_CURING,cure_duration);
      m_TimelineMgr.GetEventByIndex(m_EventIndex)->GetCastClosureJointActivity().SetActiveCuringDuration(cure_duration);
   }
   else
   {
      Float64 age = m_TimelineMgr.GetEventByIndex(m_EventIndex)->GetCastClosureJointActivity().GetTotalCuringDuration();
      DDX_Text(pDX,IDC_AGE,age);

      Float64 cure_duration = m_TimelineMgr.GetEventByIndex(m_EventIndex)->GetCastClosureJointActivity().GetActiveCuringDuration();
      DDX_Text(pDX,IDC_CURING,cure_duration);
   }
}


BEGIN_MESSAGE_MAP(CCastClosureJointDlg, CDialog)
   ON_BN_CLICKED(IDC_MOVE_RIGHT, &CCastClosureJointDlg::OnMoveToTargetList)
   ON_BN_CLICKED(IDC_MOVE_LEFT, &CCastClosureJointDlg::OnMoveToSourceList)
   ON_BN_CLICKED(ID_HELP, &CCastClosureJointDlg::OnHelp)
END_MESSAGE_MAP()


// CCastClosureJointDlg message handlers

BOOL CCastClosureJointDlg::OnInitDialog()
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

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CCastClosureJointDlg::OnMoveToTargetList()
{
   m_lbSource.MoveSelectedItemsToBuddy();
}

void CCastClosureJointDlg::OnMoveToSourceList()
{
   m_lbTarget.MoveSelectedItemsToBuddy();
}

void CCastClosureJointDlg::FillLists()
{
   GroupIndexType nGroups = m_pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CSplicedGirderData* pGirder = m_pBridgeDesc->GetGirderGroup(grpIdx)->GetGirder(0);
      const CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
      const CClosureJointData* pClosure = pSegment->GetClosureJoint(pgsTypes::metEnd);

      while ( pClosure )
      {
         EventIndexType castClosureJointEventIdx = m_TimelineMgr.GetCastClosureJointEventIndex(pClosure);

         if ( pClosure->GetPier() )
         {
            PierIDType pierID = pClosure->GetPier()->GetID();
            bool bHasClosure = m_TimelineMgr.IsClosureJointAtPier(pierID);

            CString label(GetLabel(pClosure->GetPier(),m_pDisplayUnits));

            if ( castClosureJointEventIdx == m_EventIndex )
            {
               // erected during this event, put it in the target list
               m_lbTarget.AddItem(label,CTimelineItemDataPtr::Used,pClosure->GetID());
            }
            else
            {
               // not erected during this event, put it in the source list
               CTimelineItemDataPtr::State state = (bHasClosure ? CTimelineItemDataPtr::Used : CTimelineItemDataPtr::Unused);
               m_lbSource.AddItem(label,state,pClosure->GetID());
            }
         }
         else
         {
            ATLASSERT(pClosure->GetTemporarySupport());
            SupportIDType tsID = pClosure->GetTemporarySupport()->GetID();
            bool bHasClosure = m_TimelineMgr.IsClosureJointAtTempSupport(tsID);

            CString label( GetLabel(pClosure->GetTemporarySupport(),m_pDisplayUnits) );

            if ( castClosureJointEventIdx == m_EventIndex )
            {
               m_lbTarget.AddItem(label,CTimelineItemDataPtr::Used,pClosure->GetID());
            }
            else
            {
               CTimelineItemDataPtr::State state = (bHasClosure ? CTimelineItemDataPtr::Used : CTimelineItemDataPtr::Unused);
               m_lbSource.AddItem(label,state,pClosure->GetID());
            }
         }

         if ( pClosure->GetRightSegment() )
         {
            pClosure = pClosure->GetRightSegment()->GetClosureJoint(pgsTypes::metEnd);
         }
         else
         {
            pClosure = nullptr;
         }
      }
   }
}

void CCastClosureJointDlg::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_CAST_CLOSURE_JOINTS);
}
