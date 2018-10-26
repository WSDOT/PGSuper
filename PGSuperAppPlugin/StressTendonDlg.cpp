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
      std::set<CTendonKey> tendons;

      std::vector<CTendonKey>::const_iterator iter(m_TargetTendons.begin()); 
      std::vector<CTendonKey>::const_iterator iterEnd(m_TargetTendons.end()); 
      for (; iter != iterEnd; iter++ )
      {
         const CTendonKey& tendonKey(*iter);
         GirderIDType gdrID = tendonKey.girderID;
         DuctIndexType ductIdx = tendonKey.ductIdx;

         tendons.insert( CTendonKey(gdrID,ductIdx) );
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

      CTendonKey tendonKey = m_SourceTendons[sel];
      const CGirderKey& girderKey = tendonKey.girderKey;
      DuctIndexType ductIdx = tendonKey.ductIdx;

      CString label;
      label.Format(_T("Group %d, Girder %s, Duct %d"),LABEL_GROUP(girderKey.groupIndex),LABEL_GIRDER(girderKey.girderIndex),LABEL_DUCT(ductIdx));

      int idx = pTargetList->AddString(label);

      m_TargetTendons.push_back( tendonKey );
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
      CTendonKey& tendonKey = m_TargetTendons[sel];
      const CGirderKey& girderKey = tendonKey.girderKey;
      DuctIndexType ductIdx = tendonKey.ductIdx;

      CString label;
      label.Format(_T("Group %d, Girder %s, Duct %d"),LABEL_GROUP(girderKey.groupIndex),LABEL_GIRDER(girderKey.girderIndex),LABEL_DUCT(ductIdx));

      pSourceList->AddString(label);
      m_SourceTendons.push_back(tendonKey);
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

         GirderIDType girderID = pGirder->GetID();
         CGirderKey girderKey(pGirder->GetGirderKey());

         DuctIndexType nDucts = pPTData->GetDuctCount();
         for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
         {
            bool bIsTendonStressed = m_pTimelineMgr->IsTendonStressed(girderID,ductIdx);
            if ( bIsTendonStressed && m_pTimelineMgr->GetStressTendonEventIndex(girderID,ductIdx) == m_EventIndex )
            {
               // if the tendon is marked as stress and if it is stressed during this event, make sure it is
               // marked as stressed in the activity. If it is not stressed in this activity
               // the it is not stressed (the global timeline manager hasn't been updated yet)
               if ( !m_StressTendonActivity.IsTendonStressed(girderID,ductIdx) )
                  bIsTendonStressed = false;
            }
            if ( !bIsTendonStressed )
            {
               CString label;
               label.Format(_T("Group %d, Girder %s, Duct %d"),LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_DUCT(ductIdx));

               int idx = pSourceList->AddString(label);
               CTendonKey tendonKey(girderKey,ductIdx);
               tendonKey.girderID = girderID; // make sure we set the girderID too... we are using all 3 parameters in this dialo
               m_SourceTendons.push_back(tendonKey);
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

   const std::set<CTendonKey>& tendons = m_StressTendonActivity.GetTendons();
   std::set<CTendonKey>::const_iterator iter(tendons.begin());
   std::set<CTendonKey>::const_iterator iterEnd(tendons.end());

   for ( ; iter != iterEnd; iter++ )
   {
      const CTendonKey& key(*iter);
      GirderIDType girderID = key.girderID;
      DuctIndexType ductIdx = key.ductIdx;

      const CSplicedGirderData* pGirder = pIBridgeDesc->FindGirder(girderID);
      CGirderKey girderKey = pGirder->GetGirderKey();

      CString label;
      label.Format(_T("Group %d, Girder %s, Duct %d"),LABEL_GROUP(girderKey.groupIndex),LABEL_GIRDER(girderKey.girderIndex),LABEL_DUCT(ductIdx));

      pTargetList->AddString(label);
      CTendonKey tendonKey(girderKey,ductIdx);
      tendonKey.girderID = girderID; // make sure we set the girderID too... we are using all 3 parameters in this dialo
      m_TargetTendons.push_back(tendonKey);
   }
   std::sort(m_TargetTendons.begin(),m_TargetTendons.end());
}
