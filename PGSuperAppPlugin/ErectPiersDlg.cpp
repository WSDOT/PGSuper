// ErectPiersDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "ErectPiersDlg.h"

#include <IFace\Project.h>

// CErectPiersDlg dialog


// Pier and Temporary Support indicies are stored in the ItemData member of the
// list box controls. To differentiate between a Pier index and a Temporary Support index
// the Temporary Support indices are encoded/decoded with the following methods
#include <PGSuperAppPlugin\CastClosurePourDlg.h> // use Encode/Decode methods from CastClosurePourDlg

IMPLEMENT_DYNAMIC(CErectPiersDlg, CDialog)

CErectPiersDlg::CErectPiersDlg(const CTimelineManager* pTimelineMgr,CWnd* pParent /*=NULL*/)
	: CDialog(CErectPiersDlg::IDD, pParent),
   m_pTimelineMgr(pTimelineMgr)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   m_pDisplayUnits = pDisplayUnits;

   m_pBridgeDesc = m_pTimelineMgr->GetBridgeDescription();
}

CErectPiersDlg::~CErectPiersDlg()
{
}

void CErectPiersDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   if ( pDX->m_bSaveAndValidate )
   {
      m_ErectPiers.Clear();
      m_ErectPiers.Enable(true);

      CListBox* pTargetList = (CListBox*)GetDlgItem(IDC_TARGET_LIST);
      int nItems = pTargetList->GetCount();
      for ( int itemIdx = 0; itemIdx < nItems; itemIdx++ )
      {
         PierIndexType key = (PierIndexType)pTargetList->GetItemData(itemIdx);
         if ( IsTSIndex(key) )
         {
            SupportIndexType tsIdx = DecodeTSIndex(key);
            const CTemporarySupportData* pTS = m_pBridgeDesc->GetTemporarySupport(tsIdx);

            m_ErectPiers.AddTempSupport(pTS->GetID());
         }
         else
         {
            m_ErectPiers.AddPier(key);
         }
      }
   }
}


BEGIN_MESSAGE_MAP(CErectPiersDlg, CDialog)
   ON_BN_CLICKED(IDC_MOVE_RIGHT, &CErectPiersDlg::OnMoveRight)
   ON_BN_CLICKED(IDC_MOVE_LEFT, &CErectPiersDlg::OnMoveLeft)
END_MESSAGE_MAP()


// CErectPiersDlg message handlers

BOOL CErectPiersDlg::OnInitDialog()
{
   FillSourceList();
   FillTargetList();

   CDialog::OnInitDialog();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CErectPiersDlg::OnMoveRight()
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

      PierIndexType key = (PierIndexType)pSourceList->GetItemData(sel);
      if ( IsTSIndex(key) )
      {
         SupportIndexType tsIdx = DecodeTSIndex(key);

         const CTemporarySupportData* pTS = m_pBridgeDesc->GetTemporarySupport(tsIdx);
         CString label(GetLabel(pTS,m_pDisplayUnits));

         pTargetList->SetItemData(pTargetList->AddString(label),key);
      }
      else
      {
         const CPierData2* pPier = m_pBridgeDesc->FindPier(key);
         PierIndexType pierIdx = pPier->GetIndex();

         CString label(GetLabel(pPier,m_pDisplayUnits));
         pTargetList->SetItemData(pTargetList->AddString(label),key);
      }
   }

   for ( int i = nCount-1; 0 <= i; i-- )
   {
      pSourceList->DeleteString(arrSelected.GetAt(i));
   }
}

void CErectPiersDlg::OnMoveLeft()
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

      PierIndexType key = (PierIndexType)pTargetList->GetItemData(sel);
      if ( IsTSIndex(key) )
      {
         SupportIndexType tsIdx = DecodeTSIndex(key);

         const CTemporarySupportData* pTS = m_pBridgeDesc->GetTemporarySupport(tsIdx);
         CString label(GetLabel(pTS,m_pDisplayUnits));

         pSourceList->SetItemData(pSourceList->AddString(label),key);
      }
      else
      {
         const CPierData2* pPier = m_pBridgeDesc->FindPier(key);
         PierIndexType pierIdx = pPier->GetIndex();

         CString label(GetLabel(pPier,m_pDisplayUnits));
         pSourceList->SetItemData(pSourceList->AddString(label),key);
      }
   }

   for ( int i = nCount-1; 0 <= i; i-- )
   {
      pTargetList->DeleteString(arrSelected.GetAt(i));
   }
}

void CErectPiersDlg::FillSourceList()
{
   CListBox* pSourceList = (CListBox*)GetDlgItem(IDC_SOURCE_LIST);

   PierIndexType nPiers = m_pBridgeDesc->GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      const CPierData2* pPier = m_pBridgeDesc->GetPier(pierIdx);
      PierIDType pierID = pPier->GetID();
      if ( !m_pTimelineMgr->IsPierErected(pierID) )
      {
         CString label(GetLabel(pPier,m_pDisplayUnits));
         pSourceList->SetItemData(pSourceList->AddString(label),pierIdx);
      }
   }

   SupportIndexType nTS = m_pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      const CTemporarySupportData* pTS = m_pBridgeDesc->GetTemporarySupport(tsIdx);
      SupportIDType tsID = pTS->GetID();
      if ( !m_pTimelineMgr->IsTemporarySupportErected(tsID) )
      {
         CString label(GetLabel(pTS,m_pDisplayUnits));

         pSourceList->SetItemData(pSourceList->AddString(label),EncodeTSIndex(tsIdx));
      }
   }
}

void CErectPiersDlg::FillTargetList()
{
   CListBox* pTargetList = (CListBox*)GetDlgItem(IDC_TARGET_LIST);

   const std::set<PierIDType>& piers = m_ErectPiers.GetPiers();
   std::set<PierIDType>::const_iterator pierIter;
   for ( pierIter = piers.begin(); pierIter != piers.end(); pierIter++ )
   {
      PierIDType pierID = *pierIter;

      const CPierData2* pPier = m_pBridgeDesc->FindPier(pierID);
      PierIndexType pierIdx = pPier->GetIndex();

      CString label(GetLabel(pPier,m_pDisplayUnits));
      pTargetList->SetItemData(pTargetList->AddString(label),pierIdx);
   }

   const std::set<SupportIDType>& tempSupports = m_ErectPiers.GetTempSupports();
   std::set<SupportIDType>::const_iterator tsIter;
   for ( tsIter = tempSupports.begin(); tsIter != tempSupports.end(); tsIter++ )
   {
      SupportIDType tsID = *tsIter;
      const CTemporarySupportData* pTS = m_pBridgeDesc->FindTemporarySupport(tsID);
      CString label( GetLabel(pTS,m_pDisplayUnits) );

      pTargetList->SetItemData(pTargetList->AddString(label),EncodeTSIndex(pTS->GetIndex()));
   }
}
