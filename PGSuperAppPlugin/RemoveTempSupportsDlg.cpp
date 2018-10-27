// RemoveTempSupportsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "RemoveTempSupportsDlg.h"

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>

#include "CastClosureJointDlg.h"// for label methods

#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CRemoveTempSupportsDlg dialog

IMPLEMENT_DYNAMIC(CRemoveTempSupportsDlg, CDialog)

CRemoveTempSupportsDlg::CRemoveTempSupportsDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,BOOL bReadOnly,CWnd* pParent /*=nullptr*/)
	: CDialog(CRemoveTempSupportsDlg::IDD, pParent)
{
   m_TimelineMgr = timelineMgr;
   m_pBridgeDesc = m_TimelineMgr.GetBridgeDescription();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   pBroker->GetInterface(IID_IEAFDisplayUnits,(IUnknown**)&m_pDisplayUnits);

   m_EventIndex = eventIdx;
   m_bReadOnly = bReadOnly;
}

CRemoveTempSupportsDlg::~CRemoveTempSupportsDlg()
{
}

void CRemoveTempSupportsDlg::DoDataExchange(CDataExchange* pDX)
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
         SupportIDType tsID = pItemData->m_ID;

         EventIndexType erectionEventIdx, removalEventIdx;
         m_TimelineMgr.GetTempSupportEvents(tsID,&erectionEventIdx,&removalEventIdx);
         m_TimelineMgr.SetTempSupportEvents(tsID,erectionEventIdx,m_EventIndex);
      }

      nItems = m_lbSource.GetCount();
      for ( int itemIdx = 0; itemIdx < nItems; itemIdx++ )
      {
         CTimelineItemIDDataPtr* pItemData = (CTimelineItemIDDataPtr*)m_lbSource.GetItemDataPtr(itemIdx);
         if ( pItemData->m_State == CTimelineItemDataPtr::Used )
         {
            continue;
         }

         SupportIDType tsID = pItemData->m_ID;

         EventIndexType erectionEventIdx, removalEventIdx;
         m_TimelineMgr.GetTempSupportEvents(tsID,&erectionEventIdx,&removalEventIdx);
         m_TimelineMgr.SetTempSupportEvents(tsID,erectionEventIdx,INVALID_INDEX);
      }
   }
}


BEGIN_MESSAGE_MAP(CRemoveTempSupportsDlg, CDialog)
   ON_BN_CLICKED(IDC_MOVE_RIGHT, &CRemoveTempSupportsDlg::OnMoveToTargetList)
   ON_BN_CLICKED(IDC_MOVE_LEFT, &CRemoveTempSupportsDlg::OnMoveToSourceList)
   ON_BN_CLICKED(ID_HELP, &CRemoveTempSupportsDlg::OnHelp)
END_MESSAGE_MAP()


// CRemoveTempSupportsDlg message handlers

BOOL CRemoveTempSupportsDlg::OnInitDialog()
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

void CRemoveTempSupportsDlg::OnMoveToTargetList()
{
   m_lbSource.MoveSelectedItemsToBuddy();
}

void CRemoveTempSupportsDlg::OnMoveToSourceList()
{
   m_lbTarget.MoveSelectedItemsToBuddy();
}

void CRemoveTempSupportsDlg::FillLists()
{
   SupportIndexType nTS = m_pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      const CTemporarySupportData* pTS = m_pBridgeDesc->GetTemporarySupport(tsIdx);
      SupportIDType tsID = pTS->GetID();

      bool bIsTemporarySupportRemoved = m_TimelineMgr.IsTemporarySupportRemoved(tsID);
      EventIndexType erectEventIdx, removeEventIdx;
      m_TimelineMgr.GetTempSupportEvents(tsID,&erectEventIdx,&removeEventIdx);

      CString label( GetLabel(pTS,m_pDisplayUnits) );

      if ( removeEventIdx == m_EventIndex )
      {
         // removed during this event, put it in the target list
         m_lbTarget.AddItem(label,CTimelineItemDataPtr::Used,tsID);
      }
      else
      {
         // not removed during this event, put it in the source list
         CTimelineItemDataPtr::State state = (bIsTemporarySupportRemoved ? CTimelineItemDataPtr::Used : CTimelineItemDataPtr::Unused);
         m_lbSource.AddItem(label,state,tsID);
      }
   }
}

void CRemoveTempSupportsDlg::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_REMOVE_TEMPORARY_SUPPORTS);
}
