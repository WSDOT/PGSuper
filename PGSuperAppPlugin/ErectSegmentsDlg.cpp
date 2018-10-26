// ErectSegmentsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "ErectSegmentsDlg.h"

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>

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

CErectSegmentsDlg::CErectSegmentsDlg(const CTimelineManager* pTimelineMgr,CWnd* pParent /*=NULL*/)
	: CDialog(CErectSegmentsDlg::IDD, pParent),
   m_pTimelineMgr(pTimelineMgr)
{
   m_pBridgeDesc = m_pTimelineMgr->GetBridgeDescription();
}

CErectSegmentsDlg::~CErectSegmentsDlg()
{
}

void CErectSegmentsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   if ( pDX->m_bSaveAndValidate )
   {
      m_ErectSegments.Clear();

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

      CListBox* pTargetList = (CListBox*)GetDlgItem(IDC_TARGET_LIST);
      int nItems = pTargetList->GetCount();
      m_ErectSegments.Enable(nItems == 0 ? false : true);

      for ( int itemIdx = 0; itemIdx < nItems; itemIdx++ )
      {
         SegmentIDType segID = (SegmentIDType)pTargetList->GetItemData(itemIdx);
         const CPrecastSegmentData* pSegment = pBridgeDesc->FindSegment(segID);
         const CSegmentKey& segmentKey(pSegment->GetSegmentKey());
         ATLASSERT(segmentKey.girderIndex == 0);

         // add the segment at segmentKey.segmentIndex for all girders in this group
         const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
         GirderIndexType nGirders = pGroup->GetGirderCount();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            const CPrecastSegmentData* pThisSegment = pGroup->GetGirder(gdrIdx)->GetSegment(segmentKey.segmentIndex);
            SegmentIDType thisSegmentID = pThisSegment->GetID();
            m_ErectSegments.AddSegment(thisSegmentID);
         }
      }
   }
}


BEGIN_MESSAGE_MAP(CErectSegmentsDlg, CDialog)
   ON_BN_CLICKED(IDC_MOVE_RIGHT, &CErectSegmentsDlg::OnMoveRight)
   ON_BN_CLICKED(IDC_MOVE_LEFT, &CErectSegmentsDlg::OnMoveLeft)
END_MESSAGE_MAP()


// CErectSegmentsDlg message handlers

BOOL CErectSegmentsDlg::OnInitDialog()
{
   FillSourceList();
   FillTargetList();

   CDialog::OnInitDialog();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CErectSegmentsDlg::OnMoveRight()
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
      SegmentIDType segID = (SegmentIDType)pSourceList->GetItemData(sel);
      const CPrecastSegmentData* pSegment = m_pBridgeDesc->FindSegment(segID);
      const CSegmentKey& sourceSegment(pSegment->GetSegmentKey());
      ATLASSERT(sourceSegment.girderIndex == 0);

      CString label;
      label.Format(_T("Group %d, Segment %d"),LABEL_GROUP(sourceSegment.groupIndex),LABEL_SEGMENT(sourceSegment.segmentIndex));
      int idx = pTargetList->AddString(label);
      pTargetList->SetItemData(idx,segID);
   }

   for ( int i = nCount-1; 0 <= i; i-- )
   {
      int sel = arrSelected.GetAt(i);
      pSourceList->DeleteString(sel);
   }
}

void CErectSegmentsDlg::OnMoveLeft()
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
      SegmentIDType segID = (SegmentIDType)pTargetList->GetItemData(sel);

      const CPrecastSegmentData* pSegment = m_pBridgeDesc->FindSegment(segID);
      const CSegmentKey& targetSegment(pSegment->GetSegmentKey());
      ATLASSERT(targetSegment.girderIndex == 0);

      CString label;
      label.Format(_T("Group %d, Segment %d"),LABEL_GROUP(targetSegment.groupIndex),LABEL_SEGMENT(targetSegment.segmentIndex));

      int idx = pSourceList->AddString(label);
      pSourceList->SetItemData(idx,segID);
   }

   for ( int i = nCount-1; 0 <= i; i-- )
   {
      int sel = arrSelected.GetAt(i);
      pTargetList->DeleteString(sel);
   }
}

void CErectSegmentsDlg::FillSourceList()
{
   CListBox* pSourceList = (CListBox*)GetDlgItem(IDC_SOURCE_LIST);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

      const CSplicedGirderData* pGirder = pGroup->GetGirder(0);
      SegmentIndexType nSegments = pGirder->GetSegmentCount();
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
         SegmentIDType segID = pSegment->GetID();

         if ( !m_pTimelineMgr->IsSegmentErected(segID) )
         {
            CString label;
            label.Format(_T("Group %d, Segment %d"),LABEL_GROUP(grpIdx),LABEL_SEGMENT(segIdx));
            int idx = pSourceList->AddString(label);
            pSourceList->SetItemData(idx,segID);
         }
      } // segment loop
   } // group loop
}

void CErectSegmentsDlg::FillTargetList()
{
   CListBox* pTargetList = (CListBox*)GetDlgItem(IDC_TARGET_LIST);

   const std::set<SegmentIDType>& segments = m_ErectSegments.GetSegments();
   std::set<SegmentIDType>::const_iterator iter(segments.begin());
   std::set<SegmentIDType>::const_iterator iterEnd(segments.end());
   std::set<CSegmentKey> targets; // keep track of previously encountered segments at this position in the girder group
   for ( ; iter != iterEnd; iter++ )
   {
      SegmentIDType segID = *iter;
      const CPrecastSegmentData* pSegment = m_pBridgeDesc->FindSegment(segID);
      CSegmentKey segmentKey(pSegment->GetSegmentKey());

      segmentKey.girderIndex = 0; // using girder 0 to represent all girders in this group
      std::pair<std::set<CSegmentKey>::iterator,bool> result( targets.insert(segmentKey) );
      if (result.second) // is true if inserted, false if the set already contains this segment key
      {
         CString label;
         label.Format(_T("Group %d, Segment %d"),LABEL_GROUP(segmentKey.groupIndex),LABEL_SEGMENT(segmentKey.segmentIndex));
         int idx = pTargetList->AddString(label);
         pTargetList->SetItemData(idx,segID);
      }
   }
}
