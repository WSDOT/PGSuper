// ConstructSegmentsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "ConstructSegmentsDlg.h"

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>


#include <EAF\EAFDisplayUnits.h>

// CConstructSegmentsDlg dialog

IMPLEMENT_DYNAMIC(CConstructSegmentsDlg, CDialog)

CConstructSegmentsDlg::CConstructSegmentsDlg(const CTimelineManager* pTimelineMgr,EventIndexType eventIdx,CWnd* pParent /*=NULL*/)
	: CDialog(CConstructSegmentsDlg::IDD, pParent),
   m_pTimelineMgr(pTimelineMgr),
   m_EventIndex(eventIdx)
{
   m_pBridgeDesc = m_pTimelineMgr->GetBridgeDescription();
}

CConstructSegmentsDlg::~CConstructSegmentsDlg()
{
}

void CConstructSegmentsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   if ( pDX->m_bSaveAndValidate )
   {
      m_ConstructSegments.Clear();

      CListBox* pTargetList = (CListBox*)GetDlgItem(IDC_TARGET_LIST);
      int nItems = pTargetList->GetCount();
      m_ConstructSegments.Enable(nItems == 0 ? false : true);

      for ( int itemIdx = 0; itemIdx < nItems; itemIdx++ )
      {
         SegmentIDType segID = (SegmentIDType)pTargetList->GetItemData(itemIdx);
         const CPrecastSegmentData* pSegment = m_pBridgeDesc->FindSegment(segID);
         const CSegmentKey& segmentKey(pSegment->GetSegmentKey());

         // add the segment at segmentKey.segmentIndex for all girders in this group
         const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
         GirderIndexType nGirders = pGroup->GetGirderCount();
         for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
         {
            const CPrecastSegmentData* pThisSegment = pGroup->GetGirder(gdrIdx)->GetSegment(segmentKey.segmentIndex);
            SegmentIDType thisSegmentID = pThisSegment->GetID();
            m_ConstructSegments.AddSegment(thisSegmentID);
         }
      }

      Float64 relax, age;
      DDX_Text(pDX,IDC_RELAXATION_TIME,relax);
      DDX_Text(pDX,IDC_AGE,age);

      if ( relax < age )
      {
         pDX->PrepareEditCtrl(IDC_AGE);
         AfxMessageBox(_T("Concrete age at release must be less than or equal to the time between strand stressing and release"),MB_ICONWARNING);
         pDX->Fail();
      }


      m_ConstructSegments.SetRelaxationTime(relax);
      m_ConstructSegments.SetAgeAtRelease(age);
   }
   else
   {
      Float64 relax = m_ConstructSegments.GetRelaxationTime();
      Float64 age   = m_ConstructSegments.GetAgeAtRelease();
      DDX_Text(pDX,IDC_RELAXATION_TIME,relax);
      DDX_Text(pDX,IDC_AGE,age);
   }
}


BEGIN_MESSAGE_MAP(CConstructSegmentsDlg, CDialog)
   ON_BN_CLICKED(IDC_MOVE_RIGHT, &CConstructSegmentsDlg::OnMoveRight)
   ON_BN_CLICKED(IDC_MOVE_LEFT, &CConstructSegmentsDlg::OnMoveLeft)
END_MESSAGE_MAP()


// CConstructSegmentsDlg message handlers

BOOL CConstructSegmentsDlg::OnInitDialog()
{
   FillSourceList();
   FillTargetList();

   CDialog::OnInitDialog();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CConstructSegmentsDlg::OnMoveRight()
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

      CString label;
      label.Format(_T("Group %d, Girder %s, Segment %d"),LABEL_GROUP(sourceSegment.groupIndex),LABEL_GIRDER(sourceSegment.girderIndex),LABEL_SEGMENT(sourceSegment.segmentIndex));
      int idx = pTargetList->AddString(label);
      pTargetList->SetItemData(idx,segID);
   }

   for ( int i = nCount-1; 0 <= i; i-- )
   {
      int sel = arrSelected.GetAt(i);
      pSourceList->DeleteString(sel);
   }
}

void CConstructSegmentsDlg::OnMoveLeft()
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

      CString label;
      label.Format(_T("Group %d, Girder %s, Segment %d"),LABEL_GROUP(targetSegment.groupIndex),LABEL_GIRDER(targetSegment.girderIndex),LABEL_SEGMENT(targetSegment.segmentIndex));

      int idx = pSourceList->AddString(label);
      pSourceList->SetItemData(idx,segID);
   }

   for ( int i = nCount-1; 0 <= i; i-- )
   {
      int sel = arrSelected.GetAt(i);
      pTargetList->DeleteString(sel);
   }
}

void CConstructSegmentsDlg::FillSourceList()
{
   CListBox* pSourceList = (CListBox*)GetDlgItem(IDC_SOURCE_LIST);

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
            SegmentIDType segID = pSegment->GetID();

            bool bIsSegmentConstructed = m_pTimelineMgr->IsSegmentConstructed(segID);
            EventIndexType constructEventIdx = m_pTimelineMgr->GetSegmentConstructionEventIndex(segID);
            if ( bIsSegmentConstructed && constructEventIdx == m_EventIndex )
            {
               if ( !m_ConstructSegments.HasSegment(segID) )
                  bIsSegmentConstructed = false;
            }
            if ( !bIsSegmentConstructed )
            {
               CString label;
               label.Format(_T("Group %d, Girder %s, Segment %d"),LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_SEGMENT(segIdx));
               int idx = pSourceList->AddString(label);
               pSourceList->SetItemData(idx,segID);
            }
         } // segment loop
      } // girder loop
   } // group loop
}

void CConstructSegmentsDlg::FillTargetList()
{
   CListBox* pTargetList = (CListBox*)GetDlgItem(IDC_TARGET_LIST);

   const std::set<SegmentIDType>& segments = m_ConstructSegments.GetSegments();
   std::set<SegmentIDType>::const_iterator iter(segments.begin());
   std::set<SegmentIDType>::const_iterator iterEnd(segments.end());
   std::set<CSegmentKey> targets; // keep track of previously encountered segments at this position in the girder group
   for ( ; iter != iterEnd; iter++ )
   {
      SegmentIDType segID = *iter;
      const CPrecastSegmentData* pSegment = m_pBridgeDesc->FindSegment(segID);
      CSegmentKey segmentKey(pSegment->GetSegmentKey());

      std::pair<std::set<CSegmentKey>::iterator,bool> result( targets.insert(segmentKey) );
      if (result.second) // is true if inserted, false if the set already contains this segment key
      {
         CString label;
         label.Format(_T("Group %d, Girder %s, Segment %d"),LABEL_GROUP(segmentKey.groupIndex),LABEL_GIRDER(segmentKey.girderIndex),LABEL_SEGMENT(segmentKey.segmentIndex));
         int idx = pTargetList->AddString(label);
         pTargetList->SetItemData(idx,segID);
      }
   }
}
