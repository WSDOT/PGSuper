///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

// SelectGirderSegmentDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "resource.h"
#include "SelectGirderSegmentDlg.h"


#include <IFace/Tools.h>
#include <IFace\Project.h>
#include <PsgLib\BridgeDescription2.h>



// CSelectGirderSegmentDlg dialog

IMPLEMENT_DYNAMIC(CSelectGirderSegmentDlg, CDialog)

CSelectGirderSegmentDlg::CSelectGirderSegmentDlg(CWnd* pParent /*=nullptr*/)
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
   
   auto pBroker = EAFGetBroker();
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
   {
      pcbGroup->SetCurSel((int)m_Group);
   }
   else
   {
      pcbGroup->SetCurSel(0);
   }

   OnGroupChanged();

   CDialog::OnInitDialog();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CSelectGirderSegmentDlg::OnGroupChanged()
{
   
   auto pBroker = EAFGetBroker();
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
   {
      selGdrIdx = 0; // there are fewer girders in this group
   }

   const CSplicedGirderData* pGirder = pGroup->GetGirder(selGrpIdx);

   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   if ( nSegments <= selSegIdx )
   {
      selSegIdx = 0; // there are fewer segments in this girder
   }

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
