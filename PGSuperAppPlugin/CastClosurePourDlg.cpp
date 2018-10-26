// CastClosurePourDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "CastClosurePourDlg.h"

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosurePourData.h>

#include <EAF\EAFDisplayUnits.h>


bool IsTSIndex(IndexType key) { return MAX_INDEX/2 <= key ? true : false; }
SupportIndexType EncodeTSIndex(SupportIndexType tsIdx) { return MAX_INDEX-tsIdx; }
SupportIndexType DecodeTSIndex(SupportIndexType tsIdx) { return MAX_INDEX-tsIdx; }


// CCastClosurePourDlg dialog

IMPLEMENT_DYNAMIC(CCastClosurePourDlg, CDialog)

CCastClosurePourDlg::CCastClosurePourDlg(const CTimelineManager* pTimelineMgr,CWnd* pParent /*=NULL*/)
	: CDialog(CCastClosurePourDlg::IDD, pParent)
{
   m_pTimelineMgr = pTimelineMgr;
   m_pBridgeDesc = m_pTimelineMgr->GetBridgeDescription();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   m_pDisplayUnits = pDisplayUnits;
}

CCastClosurePourDlg::~CCastClosurePourDlg()
{
}

void CCastClosurePourDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   if ( pDX->m_bSaveAndValidate )
   {
      // Data coming out of dialog
      m_CastClosurePours.Clear();
      m_CastClosurePours.Enable(true);

      CListBox* pTargetList = (CListBox*)GetDlgItem(IDC_TARGET_LIST);
      int nItems = pTargetList->GetCount();
      for ( int itemIdx = 0; itemIdx < nItems; itemIdx++ )
      {
         PierIndexType key = (PierIndexType)pTargetList->GetItemData(itemIdx);
         if ( IsTSIndex(key) )
         {
            SupportIndexType tsIdx = DecodeTSIndex(key);
            const CTemporarySupportData* pTS = m_pBridgeDesc->GetTemporarySupport(tsIdx);
            SupportIDType tsID = pTS->GetID();
            m_CastClosurePours.AddTempSupport(tsID);
         }
         else
         {
            const CPierData2* pPier = m_pBridgeDesc->GetPier(key);
            PierIDType pierID = pPier->GetID();
            m_CastClosurePours.AddPier(pierID);
         }
      }

      Float64 age;
      DDX_Text(pDX,IDC_AGE,age);
      m_CastClosurePours.SetConcreteAgeAtContinuity(age);
   }
   else
   {
      Float64 age = m_CastClosurePours.GetConcreteAgeAtContinuity();
      DDX_Text(pDX,IDC_AGE,age);
   }
}


BEGIN_MESSAGE_MAP(CCastClosurePourDlg, CDialog)
   ON_BN_CLICKED(IDC_MOVE_RIGHT, &CCastClosurePourDlg::OnMoveRight)
   ON_BN_CLICKED(IDC_MOVE_LEFT, &CCastClosurePourDlg::OnMoveLeft)
END_MESSAGE_MAP()


// CCastClosurePourDlg message handlers

BOOL CCastClosurePourDlg::OnInitDialog()
{
   FillSourceList();
   FillTargetList();

   CDialog::OnInitDialog();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CCastClosurePourDlg::OnMoveRight()
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
         CString label( GetLabel(pTS,m_pDisplayUnits) );
         pTargetList->SetItemData(pTargetList->AddString(label),key);
      }
      else
      {
         const CPierData2* pPier = m_pBridgeDesc->GetPier(key);
         CString label( GetLabel(pPier,m_pDisplayUnits) );
         pTargetList->SetItemData(pTargetList->AddString(label),key);
      }
   }

   for ( int i = nCount-1; 0 <= i; i-- )
   {
      pSourceList->DeleteString(arrSelected.GetAt(i));
   }
}

void CCastClosurePourDlg::OnMoveLeft()
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
         CString label( GetLabel(pTS,m_pDisplayUnits) );
         pSourceList->SetItemData(pSourceList->AddString(label),key);
      }
      else
      {
         const CPierData2* pPier = m_pBridgeDesc->GetPier(key);
         CString label( GetLabel(pPier,m_pDisplayUnits) );
         pSourceList->SetItemData(pSourceList->AddString(label),key);
      }
   }

   for ( int i = nCount-1; 0 <= i; i-- )
   {
      pTargetList->DeleteString(arrSelected.GetAt(i));
   }
}

void CCastClosurePourDlg::FillSourceList()
{
   CListBox* pSourceList = (CListBox*)GetDlgItem(IDC_SOURCE_LIST);

   const CSplicedGirderData* pGirder = m_pBridgeDesc->GetGirderGroup(GroupIndexType(0))->GetGirder(0);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
   const CClosurePourData* pClosure = pSegment->GetRightClosure();

   while ( pClosure )
   {
      if ( pClosure->GetPier() )
      {
         PierIDType pierID = pClosure->GetPier()->GetID();
         if ( !m_pTimelineMgr->IsClosurePourAtPier(pierID) )
         {
            PierIndexType pierIdx = pClosure->GetPier()->GetIndex();
            CString label(GetLabel(pClosure->GetPier(),m_pDisplayUnits));
            pSourceList->SetItemData(pSourceList->AddString(label),pierIdx);
         }
      }
      else
      {
         ATLASSERT(pClosure->GetTemporarySupport());
         SupportIDType tsID = pClosure->GetTemporarySupport()->GetID();
         if ( !m_pTimelineMgr->IsClosurePourAtTempSupport(tsID) )
         {
            SupportIndexType tsIdx = pClosure->GetTemporarySupport()->GetIndex();
            CString label( GetLabel(pClosure->GetTemporarySupport(),m_pDisplayUnits) );
            pSourceList->SetItemData(pSourceList->AddString(label), EncodeTSIndex(tsIdx) );
         }
      }

      if ( pClosure->GetRightSegment() )
         pClosure = pClosure->GetRightSegment()->GetRightClosure();
      else
         pClosure = NULL;
   }
}

void CCastClosurePourDlg::FillTargetList()
{
   CListBox* pTargetList = (CListBox*)GetDlgItem(IDC_TARGET_LIST);

   const std::set<PierIDType>& piers = m_CastClosurePours.GetPiers();
   std::set<PierIDType>::const_iterator pierIter(piers.begin());
   std::set<PierIDType>::const_iterator pierIterEnd(piers.end());
   for ( ; pierIter != pierIterEnd; pierIter++ )
   {
      PierIDType pierID = *pierIter;
      const CPierData2* pPier = m_pBridgeDesc->FindPier(pierID);
      PierIndexType pierIdx = pPier->GetIndex();

      CString label( GetLabel(pPier,m_pDisplayUnits) );
      pTargetList->SetItemData(pTargetList->AddString(label),pierIdx);
   }

   const std::set<SupportIDType>& tempSupports = m_CastClosurePours.GetTempSupports();
   std::set<SupportIDType>::const_iterator tsIter(tempSupports.begin());
   std::set<SupportIDType>::const_iterator tsIterEnd(tempSupports.end());
   for ( ; tsIter != tsIterEnd; tsIter++ )
   {
      SupportIDType tsID = *tsIter;
      const CTemporarySupportData* pTS = m_pBridgeDesc->FindTemporarySupport(tsID);
      SupportIndexType tsIdx = pTS->GetIndex();

      CString label( GetLabel(pTS,m_pDisplayUnits) );

      pTargetList->SetItemData(pTargetList->AddString(label),EncodeTSIndex(tsIdx));
   }
}
