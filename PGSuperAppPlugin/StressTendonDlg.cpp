// StressTendonDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "StressTendonDlg.h"

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>

// CStressTendonDlg dialog

class CDuctItemDataPtr : public CTimelineItemIndexDataPtr
{
public:
   GirderIDType m_GirderID;
};

IMPLEMENT_DYNAMIC(CStressTendonDlg, CDialog)

CStressTendonDlg::CStressTendonDlg(const CTimelineManager& timelineMgr,EventIndexType eventIdx,CWnd* pParent /*=NULL*/)
	: CDialog(CStressTendonDlg::IDD, pParent),
   m_EventIndex(eventIdx)
{
   m_TimelineMgr = timelineMgr;
}

CStressTendonDlg::~CStressTendonDlg()
{
}

void CStressTendonDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   DDX_Control(pDX,IDC_SOURCE_LIST,m_lbSource);
   DDX_Control(pDX,IDC_TARGET_LIST,m_lbTarget);

   if ( pDX->m_bSaveAndValidate )
   {
      int nItems = m_lbTarget.GetCount();
      for ( int itemIdx = 0; itemIdx < nItems; itemIdx++ )
      {
         CDuctItemDataPtr* pItemData = (CDuctItemDataPtr*)m_lbTarget.GetItemDataPtr(itemIdx);
         GirderIDType girderID = pItemData->m_GirderID;
         DuctIndexType ductIdx = pItemData->m_Index;

         m_TimelineMgr.SetStressTendonEventByIndex(girderID,ductIdx,m_EventIndex);
      }

      nItems = m_lbSource.GetCount();
      for ( int itemIdx = 0; itemIdx < nItems; itemIdx++ )
      {
         CDuctItemDataPtr* pItemData = (CDuctItemDataPtr*)m_lbSource.GetItemDataPtr(itemIdx);
         if ( pItemData->m_State == CTimelineItemDataPtr::Used )
         {
            continue;
         }
         GirderIDType girderID = pItemData->m_GirderID;
         DuctIndexType ductIdx = pItemData->m_Index;

         m_TimelineMgr.SetStressTendonEventByIndex(girderID,ductIdx,INVALID_INDEX);
      }
   }
}


BEGIN_MESSAGE_MAP(CStressTendonDlg, CDialog)
   ON_BN_CLICKED(IDC_MOVE_RIGHT, &CStressTendonDlg::OnMoveToTargetList)
   ON_BN_CLICKED(IDC_MOVE_LEFT,  &CStressTendonDlg::OnMoveToSourceList)
END_MESSAGE_MAP()


// CStressTendonDlg message handlers

BOOL CStressTendonDlg::OnInitDialog()
{
   m_lbSource.Initialize(CTimelineItemListBox::Source,CTimelineItemListBox::Index,&m_lbTarget);
   m_lbTarget.Initialize(CTimelineItemListBox::Target,CTimelineItemListBox::Index,&m_lbSource);

   CDialog::OnInitDialog();

   FillLists();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CStressTendonDlg::OnMoveToTargetList()
{
   m_lbSource.MoveSelectedItemsToBuddy();
}

void CStressTendonDlg::OnMoveToSourceList()
{
   m_lbTarget.MoveSelectedItemsToBuddy();
}

void CStressTendonDlg::FillLists()
{
   const CBridgeDescription2* pBridgeDesc = m_TimelineMgr.GetBridgeDescription();

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
            CString label;
            label.Format(_T("Group %d, Girder %s, Duct %d"),LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_DUCT(ductIdx));

            bool bIsTendonStressed = m_TimelineMgr.IsTendonStressed(girderID,ductIdx);
            EventIndexType stressingEventIdx = m_TimelineMgr.GetStressTendonEventIndex(girderID,ductIdx);

            CDuctItemDataPtr* pItemData = new CDuctItemDataPtr;
            pItemData->m_GirderID = girderID;
            pItemData->m_Index = ductIdx;
            if ( stressingEventIdx == m_EventIndex )
            {
               pItemData->m_State = CTimelineItemDataPtr::Used;
               m_lbTarget.AddItem(label,pItemData);
            }
            else
            {
               CTimelineItemDataPtr::State state = (bIsTendonStressed ? CTimelineItemDataPtr::Used : CTimelineItemDataPtr::Unused);
               pItemData->m_State = state;
               m_lbSource.AddItem(label,pItemData);
            }
         }
      }
   }
}
