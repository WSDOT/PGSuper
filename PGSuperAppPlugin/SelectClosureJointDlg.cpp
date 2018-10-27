///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

// SelectClosureJointDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "SelectClosureJointDlg.h"



#include <PgsExt\GirderGroupData.h>
#include <PgsExt\PierData2.h>

#include "CastClosureJointDlg.h" // for Encode/Decode methods

// CSelectClosureJointDlg dialog

IMPLEMENT_DYNAMIC(CSelectClosureJointDlg, CDialog)

CSelectClosureJointDlg::CSelectClosureJointDlg(const CBridgeDescription2* pBridgeDesc,CWnd* pParent /*=nullptr*/)
	: CDialog(CSelectClosureJointDlg::IDD, pParent)
{   
   m_pBridgeDesc = pBridgeDesc;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2_NOCHECK(pBroker,IEAFDisplayUnits,pDisplayUnits);
   m_pDisplayUnits = pDisplayUnits;
}

CSelectClosureJointDlg::~CSelectClosureJointDlg()
{
}

void CSelectClosureJointDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   if (pDX->m_bSaveAndValidate)
   {
      // data coming out of dialog
      IndexType key;
      DDX_CBItemData(pDX,IDC_SUPPORTS,key);
      if ( IsTSIndex(key) )
      {
         SupportIndexType tsIdx = DecodeTSIndex(key);
         m_TempSupportID = m_pBridgeDesc->GetTemporarySupport(tsIdx)->GetID();
      }
      else
      {
         m_PierIdx = m_pBridgeDesc->GetPier(key)->GetIndex();
      }
   }
   else
   {
      // data going into dialog
      if ( m_PierIdx != INVALID_INDEX )
      {
         DDX_CBItemData(pDX,IDC_SUPPORTS,m_PierIdx);
      }
      else
      {
         SupportIndexType key = EncodeTSIndex(m_pBridgeDesc->FindTemporarySupport(m_TempSupportID)->GetIndex());
         DDX_CBItemData(pDX,IDC_SUPPORTS,key);
      }
   }

   DDX_CBItemData(pDX,IDC_GIRDERS,m_GirderIdx);
}


BEGIN_MESSAGE_MAP(CSelectClosureJointDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_SUPPORTS, &CSelectClosureJointDlg::OnSupportChanged)
END_MESSAGE_MAP()


BOOL CSelectClosureJointDlg::OnInitDialog()
{
   FillSupportComboBox();

   CDialog::OnInitDialog();

   OnSupportChanged();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

// CSelectClosureJointDlg message handlers

void CSelectClosureJointDlg::OnSupportChanged()
{
   // TODO: Add your control notification handler code here
   GroupIndexType grpIdx = INVALID_INDEX;
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SUPPORTS);
   int sel = pCB->GetCurSel();
   IndexType key = (IndexType)pCB->GetItemData(sel);
   if ( IsTSIndex(key) )
   {
      SupportIndexType tsIdx = DecodeTSIndex(key);
      const CTemporarySupportData* pTS = m_pBridgeDesc->GetTemporarySupport(tsIdx);
      const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(pTS->GetSpan());
      grpIdx = pGroup->GetIndex();
   }
   else
   {
      PierIndexType pierIdx = key;
      const CPierData2* pPier = m_pBridgeDesc->GetPier(pierIdx);
      const CGirderGroupData* pGroup = pPier->GetGirderGroup(pgsTypes::Back);
      // if a pier has a closure joint, it is in the middle of a group so it doesn't
      // matter if we ask for the ahead or back side groups
      grpIdx = pGroup->GetIndex();
   }

   ATLASSERT(grpIdx != INVALID_INDEX);
   FillGirderComboBox(grpIdx);
}

void CSelectClosureJointDlg::FillSupportComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SUPPORTS);

   PierIndexType nPiers = m_pBridgeDesc->GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      const CPierData2* pPier = m_pBridgeDesc->GetPier(pierIdx);
      if ( pPier->IsInteriorPier() && pPier->GetClosureJoint(m_GirderIdx) )
      {
         CString strLabel(GetLabel(pPier,m_pDisplayUnits));
         int idx = pCB->AddString(strLabel);
         pCB->SetItemData(idx,pierIdx);
      }
   }

   SupportIndexType nTS = m_pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      const CTemporarySupportData* pTS = m_pBridgeDesc->GetTemporarySupport(tsIdx);
      if ( pTS->GetClosureJoint(m_GirderIdx) )
      {
         CString strLabel(GetLabel(pTS,m_pDisplayUnits));
         int idx = pCB->AddString(strLabel);
         pCB->SetItemData(idx,EncodeTSIndex(tsIdx));
      }
   }
}

void CSelectClosureJointDlg::FillGirderComboBox(GroupIndexType grpIdx)
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GIRDERS);
   int sel = pCB->GetCurSel();
   pCB->ResetContent();

   const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(grpIdx);
   GirderIndexType nGirders = pGroup->GetGirderCount();
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      CString strLabel;
      strLabel.Format(_T("Girder %s"),LABEL_GIRDER(gdrIdx));
      pCB->SetItemData(pCB->AddString(strLabel),gdrIdx);
   }

   sel = pCB->SetCurSel(sel);
   if ( sel == CB_ERR )
      pCB->SetCurSel(0);
}

