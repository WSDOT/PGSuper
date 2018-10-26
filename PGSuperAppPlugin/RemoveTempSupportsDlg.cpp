// RemoveTempSupportsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "RemoveTempSupportsDlg.h"

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>

#include "CastClosureJointDlg.h"// for label methods

// CRemoveTempSupportsDlg dialog

IMPLEMENT_DYNAMIC(CRemoveTempSupportsDlg, CDialog)

CRemoveTempSupportsDlg::CRemoveTempSupportsDlg(const CTimelineManager* pTimelineMgr,EventIndexType eventIdx,CWnd* pParent /*=NULL*/)
	: CDialog(CRemoveTempSupportsDlg::IDD, pParent),
   m_pTimelineMgr(pTimelineMgr)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   pBroker->GetInterface(IID_IEAFDisplayUnits,(IUnknown**)&m_pDisplayUnits);

   m_EventIndex = eventIdx;

   m_pBridgeDesc = m_pTimelineMgr->GetBridgeDescription();
}

CRemoveTempSupportsDlg::~CRemoveTempSupportsDlg()
{
}

void CRemoveTempSupportsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   if ( pDX->m_bSaveAndValidate )
   {
      m_RemoveTempSupports.Clear();
      m_RemoveTempSupports.Enable(true);

      CListBox* pTargetList = (CListBox*)GetDlgItem(IDC_TARGET_LIST);
      int nItems = pTargetList->GetCount();
      for ( int itemIdx = 0; itemIdx < nItems; itemIdx++ )
      {
         SupportIDType key = (SupportIDType)pTargetList->GetItemData(itemIdx);
         m_RemoveTempSupports.AddTempSupport(key);
      }
   }
}


BEGIN_MESSAGE_MAP(CRemoveTempSupportsDlg, CDialog)
   ON_BN_CLICKED(IDC_MOVE_RIGHT, &CRemoveTempSupportsDlg::OnMoveRight)
   ON_BN_CLICKED(IDC_MOVE_LEFT, &CRemoveTempSupportsDlg::OnMoveLeft)
END_MESSAGE_MAP()


// CRemoveTempSupportsDlg message handlers

BOOL CRemoveTempSupportsDlg::OnInitDialog()
{
   FillSourceList();
   FillTargetList();

   CDialog::OnInitDialog();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CRemoveTempSupportsDlg::OnMoveRight()
{
   // Move from source to target lists
   CListBox* pSourceList = (CListBox*)GetDlgItem(IDC_SOURCE_LIST);
   int nCount = pSourceList->GetSelCount();
   CArray<int,int> arrSelected;
   arrSelected.SetSize(nCount);
   pSourceList->GetSelItems(nCount,arrSelected.GetData());

   CListBox* pTargetList = (CListBox*)GetDlgItem(IDC_TARGET_LIST);
   for ( int i = 0; i < nCount; i++ )
   {
      int sel = arrSelected.GetAt(i);

      SupportIDType tsID = (SupportIDType)pSourceList->GetItemData(sel);
      const CTemporarySupportData* pTS = m_pBridgeDesc->FindTemporarySupport(tsID);
      CString label(GetLabel(pTS,m_pDisplayUnits));
      pTargetList->SetItemData(pTargetList->AddString(label),tsID);
   }

   for ( int i = nCount-1; 0 <= i; i-- )
   {
      pSourceList->DeleteString(arrSelected.GetAt(i));
   }
}

void CRemoveTempSupportsDlg::OnMoveLeft()
{
   // Move from target to source lists
   CListBox* pTargetList = (CListBox*)GetDlgItem(IDC_TARGET_LIST);
   int nCount = pTargetList->GetSelCount();
   CArray<int,int> arrSelected;
   arrSelected.SetSize(nCount);
   pTargetList->GetSelItems(nCount,arrSelected.GetData());

   CListBox* pSourceList = (CListBox*)GetDlgItem(IDC_SOURCE_LIST);
   for ( int i = 0; i < nCount; i++ )
   {
      int sel = arrSelected.GetAt(i);

      SupportIDType tsID = (SupportIDType)pTargetList->GetItemData(sel);
      const CTemporarySupportData* pTS = m_pBridgeDesc->FindTemporarySupport(tsID);
      CString label(GetLabel(pTS,m_pDisplayUnits));
      pSourceList->SetItemData(pSourceList->AddString(label),tsID);
   }

   for ( int i = nCount-1; 0 <= i; i-- )
   {
      pTargetList->DeleteString(arrSelected.GetAt(i));
   }
}

void CRemoveTempSupportsDlg::FillSourceList()
{
   CListBox* pSourceList = (CListBox*)GetDlgItem(IDC_SOURCE_LIST);

   SupportIndexType nTS = m_pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      const CTemporarySupportData* pTS = m_pBridgeDesc->GetTemporarySupport(tsIdx);
      SupportIDType tsID = pTS->GetID();
      bool bIsTemporarySupportRemoved = m_pTimelineMgr->IsTemporarySupportRemoved(tsID);
      EventIndexType erectEventIdx, removeEventIdx;
      m_pTimelineMgr->GetTempSupportEvents(tsID,&erectEventIdx,&removeEventIdx);
      if ( bIsTemporarySupportRemoved && removeEventIdx == m_EventIndex )
      {
         if ( !m_RemoveTempSupports.HasTempSupport(tsID) )
            bIsTemporarySupportRemoved = false;
      }
      if ( !bIsTemporarySupportRemoved )
      {
         CString label( GetLabel(pTS,m_pDisplayUnits) );
         pSourceList->SetItemData(pSourceList->AddString(label),tsID);
      }
   }
}

void CRemoveTempSupportsDlg::FillTargetList()
{
   CListBox* pTargetList = (CListBox*)GetDlgItem(IDC_TARGET_LIST);

   const std::vector<SupportIDType>& tempSupports = m_RemoveTempSupports.GetTempSupports();
   std::vector<SupportIDType>::const_iterator tsIter;
   for ( tsIter = tempSupports.begin(); tsIter != tempSupports.end(); tsIter++ )
   {
      SupportIDType tsID = *tsIter;
      const CTemporarySupportData* pTS = m_pBridgeDesc->FindTemporarySupport(tsID);
      CString label( GetLabel(pTS,m_pDisplayUnits) );
      pTargetList->SetItemData(pTargetList->AddString(label),pTS->GetID());
   }
}
