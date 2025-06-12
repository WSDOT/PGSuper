// ErectPiersDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "ErectPiersDlg.h"

#include <IFace/Tools.h>
#include <IFace\Project.h>

#include <EAF\EAFDocument.h>

// CErectPiersDlg dialog


// Pier and Temporary Support indicies are stored in the ItemData member of the
// list box controls. To differentiate between a Pier index and a Temporary Support index
// the Temporary Support indices are encoded/decoded with the following methods
#include "CastClosureJointDlg.h" // use Encode/Decode methods from CastClosureJointDlg



IMPLEMENT_DYNAMIC(CErectPiersDlg, CDialog)

CErectPiersDlg::CErectPiersDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent /*=nullptr*/)
	: CDialog(CErectPiersDlg::IDD, pParent),
   m_EventIndex(eventIdx),
   m_bReadOnly(bReadOnly)
{
   m_TimelineMgr = timelineMgr;

   
   auto pBroker = EAFGetBroker();
   m_pDisplayUnits = pBroker->GetInterface<IEAFDisplayUnits>(IID_IEAFDisplayUnits);

   m_pBridgeDesc = m_TimelineMgr.GetBridgeDescription();
}

CErectPiersDlg::~CErectPiersDlg()
{
}

void CErectPiersDlg::DoDataExchange(CDataExchange* pDX)
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
         CTimelineItemIndexDataPtr* pItemData = (CTimelineItemIndexDataPtr*)m_lbTarget.GetItemDataPtr(itemIdx);
         if ( IsTSIndex(pItemData->m_Index) )
         {
            SupportIndexType tsIdx = DecodeTSIndex(pItemData->m_Index);
            const CTemporarySupportData* pTS = m_pBridgeDesc->GetTemporarySupport(tsIdx);
            SupportIDType tsID = pTS->GetID();
            EventIndexType erectionEventIdx,removalEventIdx;
            m_TimelineMgr.GetTempSupportEvents(tsID,&erectionEventIdx,&removalEventIdx);
            m_TimelineMgr.SetTempSupportEvents(tsID,m_EventIndex,removalEventIdx);
         }
         else
         {
            const CPierData2* pPier = m_pBridgeDesc->GetPier(pItemData->m_Index);
            PierIDType pierID = pPier->GetID();
            m_TimelineMgr.SetPierErectionEventByIndex(pierID,m_EventIndex);
         }
      }

      // Remove event information for everything in the source list that isn't used in another event
      nItems = m_lbSource.GetCount();
      for ( int itemIdx = 0; itemIdx < nItems; itemIdx++ )
      {
         CTimelineItemIndexDataPtr* pItemData = (CTimelineItemIndexDataPtr*)m_lbSource.GetItemDataPtr(itemIdx);
         if ( pItemData->m_State == CTimelineItemDataPtr::Used )
         {
            // item is used in a different event
            continue;
         }

         if ( IsTSIndex(pItemData->m_Index) )
         {
            SupportIndexType tsIdx = DecodeTSIndex(pItemData->m_Index);
            const CTemporarySupportData* pTS = m_pBridgeDesc->GetTemporarySupport(tsIdx);
            SupportIDType tsID = pTS->GetID();
            EventIndexType erectionEventIdx,removalEventIdx;
            m_TimelineMgr.GetTempSupportEvents(tsID,&erectionEventIdx,&removalEventIdx);
            m_TimelineMgr.SetTempSupportEvents(tsID,INVALID_INDEX,removalEventIdx);
         }
         else
         {
            const CPierData2* pPier = m_pBridgeDesc->GetPier(pItemData->m_Index);
            PierIDType pierID = pPier->GetID();
            m_TimelineMgr.SetPierErectionEventByIndex(pierID,INVALID_INDEX);
         }
      }
   }
}


BEGIN_MESSAGE_MAP(CErectPiersDlg, CDialog)
   ON_BN_CLICKED(IDC_MOVE_RIGHT, &CErectPiersDlg::OnMoveToTargetList)
   ON_BN_CLICKED(IDC_MOVE_LEFT, &CErectPiersDlg::OnMoveToSourceList)
   ON_BN_CLICKED(ID_HELP, &CErectPiersDlg::OnHelp)
END_MESSAGE_MAP()


// CErectPiersDlg message handlers

BOOL CErectPiersDlg::OnInitDialog()
{
   m_lbSource.Initialize(CTimelineItemListBox::Source,CTimelineItemListBox::Index,&m_lbTarget);
   m_lbTarget.Initialize(CTimelineItemListBox::Target,CTimelineItemListBox::Index,&m_lbSource);

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


void CErectPiersDlg::OnMoveToTargetList()
{
   m_lbSource.MoveSelectedItemsToBuddy();
}

void CErectPiersDlg::OnMoveToSourceList()
{
   m_lbTarget.MoveSelectedItemsToBuddy();
}

void CErectPiersDlg::FillLists()
{
   PierIndexType nPiers = m_pBridgeDesc->GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      const CPierData2* pPier = m_pBridgeDesc->GetPier(pierIdx);
      PierIDType pierID = pPier->GetID();

      bool bIsPierErected = m_TimelineMgr.IsPierErected(pierID);
      EventIndexType erectionEventIdx = m_TimelineMgr.GetPierErectionEventIndex(pierID);

      CString label( GetLabel(pPier,m_pDisplayUnits) );

      if ( erectionEventIdx == m_EventIndex )
      {
         // erected during this event, put it in the target list
         m_lbTarget.AddItem(label,CTimelineItemDataPtr::Used,pierIdx);
      }
      else
      {
         // not erected during this event, put it in the source list
         CTimelineItemDataPtr::State state = (bIsPierErected ? CTimelineItemDataPtr::Used : CTimelineItemDataPtr::Unused);
         m_lbSource.AddItem(label,state,pierIdx);
      }
   }

   SupportIndexType nTS = m_pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      const CTemporarySupportData* pTS = m_pBridgeDesc->GetTemporarySupport(tsIdx);
      SupportIDType tsID = pTS->GetID();

      bool bIsTemporarySupportErected = m_TimelineMgr.IsTemporarySupportErected(tsID);

      EventIndexType erectEventIdx, removeEventIdx;
      m_TimelineMgr.GetTempSupportEvents(tsID,&erectEventIdx,&removeEventIdx);
      
      CString label(GetLabel(pTS,m_pDisplayUnits));

      if ( erectEventIdx == m_EventIndex )
      {
         m_lbTarget.AddItem(label,CTimelineItemDataPtr::Used,EncodeTSIndex(tsIdx));
      }
      else
      {
         CTimelineItemDataPtr::State state = (bIsTemporarySupportErected ? CTimelineItemDataPtr::Used : CTimelineItemDataPtr::Unused);
         m_lbSource.AddItem(label,state,EncodeTSIndex(tsIdx));
      }
   }
}

void CErectPiersDlg::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_ERECT_PIERS);
}
