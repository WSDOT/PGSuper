// SelectGirderSegmentDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperAppPlugin\Resource.h"
#include "PGSuperAppPlugin\SelectGirderSegmentDlg.h"


#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>

// CSelectGirderSegmentDlg dialog

IMPLEMENT_DYNAMIC(CSelectGirderSegmentDlg, CDialog)

CSelectGirderSegmentDlg::CSelectGirderSegmentDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSelectGirderSegmentDlg::IDD, pParent)
{
   m_Group   = INVALID_INDEX;
   m_Girder  = INVALID_INDEX;
   m_Segment = INVALID_INDEX;
}

CSelectGirderSegmentDlg::~CSelectGirderSegmentDlg()
{
}

void CSelectGirderSegmentDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   DDX_CBIndex(pDX, IDC_GROUP,   (int&)m_Group);
	DDX_CBIndex(pDX, IDC_GIRDER,  (int&)m_Girder);
	DDX_CBIndex(pDX, IDC_SEGMENT, (int&)m_Segment);
}


BEGIN_MESSAGE_MAP(CSelectGirderSegmentDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_GROUP, &CSelectGirderSegmentDlg::OnGroupChanged)
END_MESSAGE_MAP()


// CSelectGirderSegmentDlg message handlers

BOOL CSelectGirderSegmentDlg::OnInitDialog()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CComboBox* pcbGroup = (CComboBox*)GetDlgItem(IDC_GROUP);
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CString strGroup;
      strGroup.Format(_T("Group %d"),LABEL_GROUP(grpIdx));
      pcbGroup->AddString(strGroup);
   }
   if ( m_Group != INVALID_INDEX )
      pcbGroup->SetCurSel((int)m_Group);
   else
      pcbGroup->SetCurSel(0);

   OnGroupChanged();

   CDialog::OnInitDialog();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CSelectGirderSegmentDlg::OnGroupChanged()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   CComboBox* pcbGroup   = (CComboBox*)GetDlgItem(IDC_GROUP);
   CComboBox* pcbGirder  = (CComboBox*)GetDlgItem(IDC_GIRDER);
   CComboBox* pcbSegment = (CComboBox*)GetDlgItem(IDC_SEGMENT);

   // Get the current selection
   GroupIndexType selGrpIdx   = pcbGroup->GetCurSel();
   GirderIndexType selGdrIdx  = pcbGirder->GetCurSel();
   SegmentIndexType selSegIdx = pcbSegment->GetCurSel();

   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(selGrpIdx);
   GirderIndexType nGirders = pGroup->GetGirderCount();
   if ( nGirders <= selGdrIdx )
      selGdrIdx = 0; // there are fewer girders in this group

   const CSplicedGirderData* pGirder = pGroup->GetGirder(selGrpIdx);

   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   if ( nSegments <= selSegIdx )
      selSegIdx = 0; // there are fewer segments in this girder

   pcbGirder->ResetContent();
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   { 
      CString strGirder;
      strGirder.Format(_T("Girder %s"),LABEL_GIRDER(gdrIdx));
      pcbGirder->AddString(strGirder);
   }

   pcbSegment->ResetContent();
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CString strSeg;
      strSeg.Format(_T("Segment %d"),LABEL_SEGMENT(segIdx));
      pcbSegment->AddString(strSeg);
   }

   pcbGirder->SetCurSel((int)selGdrIdx);
   pcbSegment->SetCurSel((int)selSegIdx);
}
