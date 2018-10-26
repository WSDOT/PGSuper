// StressTendonDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "StressTendonDlg.h"

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>

// CStressTendonDlg dialog

IMPLEMENT_DYNAMIC(CStressTendonDlg, CDialog)

CStressTendonDlg::CStressTendonDlg(const CTimelineManager* pTimelineMgr,EventIndexType eventIdx,CWnd* pParent /*=NULL*/)
	: CDialog(CStressTendonDlg::IDD, pParent),
   m_pTimelineMgr(pTimelineMgr),
   m_EventIndex(eventIdx)
{
}

CStressTendonDlg::~CStressTendonDlg()
{
}

void CStressTendonDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   if ( pDX->m_bSaveAndValidate )
   {
      std::vector<std::pair<CGirderKey,DuctIndexType>> tendons;

      std::vector<std::pair<CGirderKey,DuctIndexType>>::const_iterator iter(m_TargetTendons.begin()); 
      std::vector<std::pair<CGirderKey,DuctIndexType>>::const_iterator iterEnd(m_TargetTendons.end()); 
      for (; iter != iterEnd; iter++ )
      {
         const CGirderKey& girderKey = iter->first;
         DuctIndexType ductIdx = iter->second;

         tendons.push_back( std::make_pair(girderKey,ductIdx) );
      }

      m_StressTendonActivity.Clear();
      m_StressTendonActivity.Enable(true);
      m_StressTendonActivity.AddTendons(tendons);
   }
}


BEGIN_MESSAGE_MAP(CStressTendonDlg, CDialog)
   ON_BN_CLICKED(IDC_MOVE_RIGHT, &CStressTendonDlg::OnMoveRight)
   ON_BN_CLICKED(IDC_MOVE_LEFT,  &CStressTendonDlg::OnMoveLeft)
END_MESSAGE_MAP()


// CStressTendonDlg message handlers

BOOL CStressTendonDlg::OnInitDialog()
{
   FillSourceList();
   FillTargetList();

   CDialog::OnInitDialog();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CStressTendonDlg::OnMoveRight()
{
   // Move from source to target lists
   CListBox* pSourceList = (CListBox*)GetDlgItem(IDC_SOURCE_LIST);
   int nCount = pSourceList->GetSelCount();
   CArray<int,int> arrSelected;
   arrSelected.SetSize(nCount);
   pSourceList->GetSelItems(nCount,arrSelected.GetData());

   const CBridgeDescription2* pBridgeDesc = m_pTimelineMgr->GetBridgeDescription();

   CListBox* pTargetList = (CListBox*)GetDlgItem(IDC_TARGET_LIST);
   for ( int i = 0; i < nCount; i++ )
   {
      int sel = arrSelected.GetAt(i);
      const CGirderKey& girderKey = m_SourceTendons[sel].first;
      DuctIndexType ductIdx = m_SourceTendons[sel].second;

      CString label;
      label.Format(_T("Group %d, Girder %s, Duct %d"),LABEL_GROUP(girderKey.groupIndex),LABEL_GIRDER(girderKey.girderIndex),LABEL_DUCT(ductIdx));

      int idx = pTargetList->AddString(label);

      m_TargetTendons.push_back( std::make_pair(girderKey,ductIdx) );
      std::sort(m_TargetTendons.begin(),m_TargetTendons.end());
   }

   for ( int i = nCount-1; 0 <= i; i-- )
   {
      int idx = arrSelected.GetAt(i);
      pSourceList->DeleteString(idx);

      m_SourceTendons.erase(m_SourceTendons.begin()+idx);
   }
}

void CStressTendonDlg::OnMoveLeft()
{
   // Move from target to source lists
   CListBox* pTargetList = (CListBox*)GetDlgItem(IDC_TARGET_LIST);
   int nCount = pTargetList->GetSelCount();
   CArray<int,int> arrSelected;
   arrSelected.SetSize(nCount);
   pTargetList->GetSelItems(nCount,arrSelected.GetData());

   const CBridgeDescription2* pBridgeDesc = m_pTimelineMgr->GetBridgeDescription();

   CListBox* pSourceList = (CListBox*)GetDlgItem(IDC_SOURCE_LIST);
   for ( int i = 0; i < nCount; i++ )
   {
      int sel = arrSelected.GetAt(i);
      const CGirderKey& girderKey = m_TargetTendons[sel].first;
      DuctIndexType ductIdx = m_TargetTendons[sel].second;

      CString label;
      label.Format(_T("Group %d, Girder %s, Duct %d"),LABEL_GROUP(girderKey.groupIndex),LABEL_GIRDER(girderKey.girderIndex),LABEL_DUCT(ductIdx));

      pSourceList->AddString(label);
      m_SourceTendons.push_back(std::make_pair(girderKey,ductIdx));
      std::sort(m_SourceTendons.begin(),m_SourceTendons.end());
   }

   for ( int i = nCount-1; 0 <= i; i-- )
   {
      int idx = arrSelected.GetAt(i);
      pTargetList->DeleteString(idx);

      m_TargetTendons.erase(m_TargetTendons.begin()+idx);
   }
}

void CStressTendonDlg::FillSourceList()
{
   CListBox* pSourceList = (CListBox*)GetDlgItem(IDC_SOURCE_LIST);
   m_SourceTendons.clear();

   const CBridgeDescription2* pBridgeDesc = m_pTimelineMgr->GetBridgeDescription();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx); 
         const CPTData* pPTData = pGirder->GetPostTensioning();

         CGirderKey girderKey(pGirder->GetGirderKey());

         DuctIndexType nDucts = pPTData->GetDuctCount();
         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            bool bIsTendonStressed = m_pTimelineMgr->IsTendonStressed(girderKey,ductIdx);
            if ( bIsTendonStressed && m_pTimelineMgr->GetStressTendonEventIndex(girderKey,ductIdx) == m_EventIndex )
            {
               // if the tendon is marked as stress and if it is stressed during this event, make sure it is
               // marked as stressed in the activity. If it is not stressed in this activity
               // the it is not stressed (the global timeline manager hasn't been updated yet)
               if ( !m_StressTendonActivity.IsTendonStressed(girderKey,ductIdx) )
                  bIsTendonStressed = false;
            }
            if ( !bIsTendonStressed )
            {
               CString label;
               label.Format(_T("Group %d, Girder %s, Duct %d"),LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_DUCT(ductIdx));

               int idx = pSourceList->AddString(label);
               m_SourceTendons.push_back(std::make_pair(girderKey,ductIdx));
            }
         } // duct loop
      } // girder loop
   } // group loop
   std::sort(m_SourceTendons.begin(),m_SourceTendons.end());
}

void CStressTendonDlg::FillTargetList()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CListBox* pTargetList = (CListBox*)GetDlgItem(IDC_TARGET_LIST);
   m_TargetTendons.clear();

   std::vector<std::pair<CGirderKey,DuctIndexType>> tendons = m_StressTendonActivity.GetTendons();
   std::vector<std::pair<CGirderKey,DuctIndexType>>::iterator iter(tendons.begin());
   std::vector<std::pair<CGirderKey,DuctIndexType>>::iterator iterEnd(tendons.end());

   for ( ; iter != iterEnd; iter++ )
   {
      std::pair<CGirderKey,DuctIndexType> p = *iter;
      const CGirderKey& girderKey(p.first);
      DuctIndexType ductIdx = p.second;

      CString label;
      label.Format(_T("Group %d, Girder %s, Duct %d"),LABEL_GROUP(girderKey.groupIndex),LABEL_GIRDER(girderKey.girderIndex),LABEL_DUCT(ductIdx));

      pTargetList->AddString(label);
      m_TargetTendons.push_back(std::make_pair(girderKey,ductIdx));
   }
   std::sort(m_TargetTendons.begin(),m_TargetTendons.end());
}
