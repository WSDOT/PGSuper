///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// CopyGirderDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperAppPlugin\Resource.h"
#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"
#include "CopyGirderDlg.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Selection.h>

#include <PgsExt\BridgeDescription2.h>

#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCopyGirderDlg dialog


CCopyGirderDlg::CCopyGirderDlg(IBroker* pBroker, CWnd* pParent /*=NULL*/)
	: CDialog(CCopyGirderDlg::IDD, pParent),
   m_pBroker(pBroker)
{
	//{{AFX_DATA_INIT(CCopyGirderDlg)
	//}}AFX_DATA_INIT
}


void CCopyGirderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCopyGirderDlg)
	DDX_Control(pDX, IDC_COPY_GIRDER, m_DoCopyGirder);
	DDX_Control(pDX, IDC_COPY_MATERIAL, m_DoCopyMaterial);
	DDX_Control(pDX, IDC_COPY_TRANSVERSE, m_DoCopyTransverse);
	DDX_Control(pDX, IDC_COPY_PRESTRESSING, m_DoCopyPrestressing);
	DDX_Control(pDX, IDC_COPY_HANDLING, m_DoCopyHandling);
	DDX_Control(pDX, IDC_COPY_SLABOFFSET, m_DoCopySlabOffset);
   DDX_Control(pDX, IDC_LONGITUDINAL_REBAR, m_DoCopyLongitudinalRebar);

   DDX_Control(pDX, IDC_FROM_SPAN,   m_FromGroup);
   DDX_Control(pDX, IDC_FROM_GIRDER, m_FromGirder);
   DDX_Control(pDX, IDC_TO_SPAN,     m_ToGroup);
   DDX_Control(pDX, IDC_TO_GIRDER,   m_ToGirder);
	//}}AFX_DATA_MAP

   DDX_Check(pDX, IDC_COPY_GIRDER,       m_bCopyGirder);
   DDX_Check(pDX, IDC_COPY_TRANSVERSE,   m_bCopyTransverse);
   DDX_Check(pDX, IDC_COPY_PRESTRESSING, m_bCopyPrestressing);
   DDX_Check(pDX, IDC_COPY_HANDLING,     m_bCopyHandling);
   DDX_Check(pDX, IDC_COPY_MATERIAL,     m_bCopyMaterial);
   DDX_Check(pDX, IDC_LONGITUDINAL_REBAR, m_bCopyLongitudinalRebar);
   DDX_Check(pDX, IDC_COPY_SLABOFFSET,   m_bCopySlabOffset);

   if ( pDX->m_bSaveAndValidate )
   {
      m_FromGirderKey = GetFromGirder();
      m_ToGirderKeys  = GetToGirders();
   }
   else
   {
      // start with combo box for To girder
      CButton* pBut = (CButton*)GetDlgItem(IDC_RADIO1);
      pBut->SetCheck(BST_CHECKED);
      GetDlgItem(IDC_SELECT_GIRDERS)->EnableWindow(false);
   }
}


BEGIN_MESSAGE_MAP(CCopyGirderDlg, CDialog)
	//{{AFX_MSG_MAP(CCopyGirderDlg)
   ON_CBN_SELCHANGE(IDC_FROM_SPAN,OnFromGroupChanged)
   ON_CBN_SELCHANGE(IDC_TO_SPAN,OnToGroupChanged)
   ON_CBN_SELCHANGE(IDC_TO_GIRDER,OnToGirderChanged)
	ON_BN_CLICKED(IDC_COPY_PRESTRESSING, OnCopyPrestressing)
	ON_BN_CLICKED(IDC_COPY_HANDLING, OnCopyHandling)
	ON_BN_CLICKED(IDC_COPY_SLABOFFSET, OnCopySlabOffset)
	ON_BN_CLICKED(IDC_COPY_TRANSVERSE, OnCopyTransverse)
   ON_BN_CLICKED(IDC_LONGITUDINAL_REBAR, OnCopyLongitudinalRebar)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	ON_BN_CLICKED(IDC_COPY_MATERIAL, OnCopyMaterial)
	ON_BN_CLICKED(IDC_COPY_GIRDER, OnCopyGirder)
   ON_BN_CLICKED(IDC_RADIO1, &CCopyGirderDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_RADIO2, &CCopyGirderDlg::OnBnClickedRadio)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_SELECT_GIRDERS, &CCopyGirderDlg::OnBnClickedSelectGirders)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCopyGirderDlg message handlers

BOOL CCopyGirderDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // prestressing and longitudinal rebar can only be copied if
   // the source and destination girders are the same time
   UINT chkMainReinforcement = BST_CHECKED;

   if ( pBridgeDesc->UseSameGirderForEntireBridge() )
   {
      m_DoCopyGirder.SetCheck(BST_UNCHECKED);
      m_DoCopyGirder.EnableWindow(FALSE);

      chkMainReinforcement = BST_CHECKED;
   }
   else
   {
      m_DoCopyGirder.SetCheck(BST_CHECKED);
      chkMainReinforcement = BST_UNCHECKED;
   }

   m_DoCopyPrestressing.SetCheck(chkMainReinforcement);
   m_DoCopyLongitudinalRebar.SetCheck(chkMainReinforcement);
   m_DoCopyTransverse.SetCheck(BST_CHECKED);
   m_DoCopyHandling.SetCheck(BST_CHECKED);
   m_DoCopyMaterial.SetCheck(BST_CHECKED);

   if ( pBridgeDesc->GetSlabOffsetType() == pgsTypes::sotBridge )
   {
      m_DoCopySlabOffset.SetCheck(BST_UNCHECKED);
      m_DoCopySlabOffset.EnableWindow(FALSE);
   }
   else
   {
      m_DoCopySlabOffset.SetCheck(BST_CHECKED);
   }

   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
   {
      m_DoCopyPrestressing.SetWindowText(_T("Copy Prestressing and Post-Tensioning"));
      m_DoCopyGirder.SetWindowText(_T("Copy Segment Variation"));
      m_DoCopyGirder.SetCheck(BST_CHECKED);
      m_DoCopyGirder.EnableWindow(TRUE);
   }

   FillComboBoxes(m_FromGroup,m_FromGirder,false,false);
   FillComboBoxes(m_ToGroup,  m_ToGirder,  pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) ? false : true, true );
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
   {
      // in PGSplice, copying can only happen within a group
      // disable the to group combo box and keep it in sync with the from group combo box
      m_ToGroup.EnableWindow(FALSE);
   }

   GET_IFACE(ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();
   if ( selection.Type == CSelection::Girder || selection.TemporarySupport == CSelection::Segment )
   {
      m_FromGroup.SetCurSel((int)selection.GroupIdx);
      OnFromGroupChanged();
      m_FromGirder.SetCurSel((int)selection.GirderIdx);

      m_ToGroup.SetCurSel((int)selection.GroupIdx+1);
      OnToGroupChanged();
   }
	
   UpdateApply();


   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_COPY_PROPERTIES),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCopyGirderDlg::FillComboBoxes(CComboBox& cbGroup,CComboBox& cbGirder, bool bIncludeAllGroups, bool bIncludeAllGirders)
{
   cbGroup.ResetContent();

   CString strGroupLabel;
   if ( EAFGetDocument()->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      strGroupLabel = _T("Span");
   }
   else
   {
      strGroupLabel = _T("Group");
   }

   if ( bIncludeAllGroups )
   {
      CString strItem;
      strItem.Format(_T("All %ss"),strGroupLabel);
      int idx = cbGroup.AddString(strItem);
      cbGroup.SetItemData(idx,ALL_GROUPS);
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CString str;
      str.Format(_T("%s %d"),strGroupLabel,LABEL_GROUP(grpIdx));

      int idx = cbGroup.AddString(str);
      cbGroup.SetItemData(idx,grpIdx);
   }

   cbGroup.SetCurSel(0);

   FillGirderComboBox(cbGirder,0,bIncludeAllGirders);
}

void CCopyGirderDlg::FillGirderComboBox(CComboBox& cbGirder,GroupIndexType grpIdx,bool bIncludeAllGirders)
{
   int curSel = cbGirder.GetCurSel();

   cbGirder.ResetContent();

   if ( bIncludeAllGirders )
   {
      int idx = cbGirder.AddString(_T("All Girders"));
      cbGirder.SetItemData(idx,ALL_GIRDERS);
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx == ALL_GROUPS ? 0 : grpIdx)->GetGirderCount();
   for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      CString str;
      str.Format(_T("Girder %s"),LABEL_GIRDER(gdrIdx));

      int idx = cbGirder.AddString(str);
      cbGirder.SetItemData(idx,gdrIdx);
   }

   if ( curSel != CB_ERR )
      curSel = cbGirder.SetCurSel(curSel);

   if ( curSel == CB_ERR )
      cbGirder.SetCurSel(0);
}

void CCopyGirderDlg::OnFromGroupChanged() 
{
   int curSel = m_FromGroup.GetCurSel();
   if ( curSel != CB_ERR )
   {
      GroupIndexType groupIdx = (GroupIndexType)m_FromGroup.GetItemData(curSel);
      FillGirderComboBox(m_FromGirder,groupIdx,false);
   }
   else
   {
      FillGirderComboBox(m_FromGirder,0,false);
   }

   if ( EAFGetDocument()->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
   {
      m_ToGroup.SetCurSel(m_FromGroup.GetCurSel());
   }
}

void CCopyGirderDlg::OnToGroupChanged() 
{
   int curSel = m_ToGroup.GetCurSel();
   if ( curSel != CB_ERR )
   {
      GroupIndexType groupIdx = (GroupIndexType)m_ToGroup.GetItemData(curSel);
      FillGirderComboBox(m_ToGirder,groupIdx,true);
   }
   else
   {
      FillGirderComboBox(m_ToGirder,0,true);
   }

   CopyToSelectionChanged();
}

void CCopyGirderDlg::OnToGirderChanged()
{
   CopyToSelectionChanged();
}

void CCopyGirderDlg::CopyToSelectionChanged() 
{
   // if the source and any of the destination girders are not the same type
   // the prestressing and longitudinal reinforcement data can't be copied
   CGirderKey copyFrom = GetFromGirder();
   std::vector<CGirderKey> copyTo = GetToGirders();

   GroupIndexType fromGroup = copyFrom.groupIndex;
   GirderIndexType fromGirder = copyFrom.girderIndex;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(fromGroup);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(fromGirder);

   std::_tstring strFromGirder( pGirder->GetGirderName() );

   BOOL bCanCopy = TRUE;

   std::vector<CGirderKey>::iterator iter;
   for ( iter = copyTo.begin(); iter != copyTo.end(); iter++ )
   {
      CGirderKey& dwTo = *iter;
      GroupIndexType toGroup = dwTo.groupIndex;
      GirderIndexType toGirder = dwTo.girderIndex;

      std::_tstring strToGirder( pBridgeDesc->GetGirderGroup(toGroup)->GetGirder(toGirder)->GetGirderName() );

      if ( strFromGirder != strToGirder )
      {
         bCanCopy = FALSE;
         break;
      }
   }
   m_DoCopyPrestressing.SetCheck(bCanCopy ? BST_CHECKED : BST_UNCHECKED);
   m_DoCopyPrestressing.EnableWindow(bCanCopy);
   m_DoCopyLongitudinalRebar.SetCheck(bCanCopy ? BST_CHECKED : BST_UNCHECKED);
   m_DoCopyLongitudinalRebar.EnableWindow(bCanCopy);

   UpdateApply();
}

void CCopyGirderDlg::UpdateApply()
{
   BOOL enable = TRUE;
   if (m_DoCopyTransverse.GetCheck()==1   ||
       m_DoCopyGirder.GetCheck()==1 ||
       m_DoCopyPrestressing.GetCheck()==1 ||
       m_DoCopyHandling.GetCheck()==1     ||
       m_DoCopyMaterial.GetCheck()==1     ||
       m_DoCopySlabOffset.GetCheck()==1     ||
       m_DoCopyLongitudinalRebar.GetCheck() == 1)
   {
      enable = TRUE;
   }
   else
   {
      enable = FALSE;
   }
   CWnd* butok = GetDlgItem(IDOK);
   ASSERT(butok!=0);
   butok->EnableWindow(enable);
}

CGirderKey CCopyGirderDlg::GetFromGirder()
{
   GroupIndexType grpIdx = 0;
   int sel = m_FromGroup.GetCurSel();
   if ( sel != CB_ERR )
   {
      grpIdx = (GroupIndexType)m_FromGroup.GetItemData(sel);
   }

   GirderIndexType gdrIdx = 0;
   sel = m_FromGirder.GetCurSel();
   if( sel != CB_ERR )
   {
      gdrIdx = (GirderIndexType)m_FromGirder.GetItemData(sel);
   }

   return CGirderKey(grpIdx,gdrIdx);
}

std::vector<CGirderKey> CCopyGirderDlg::GetToGirders()
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   std::vector<CGirderKey> vec;

   // See which control to get data from
   BOOL enab_combo = IsDlgButtonChecked(IDC_RADIO1) == BST_CHECKED ? TRUE : FALSE;

   if ( enab_combo )
   {
   GroupIndexType firstGroupIdx, lastGroupIdx;
   int sel = m_ToGroup.GetCurSel();
   firstGroupIdx = (GroupIndexType)m_ToGroup.GetItemData(sel);
   if ( firstGroupIdx == ALL_GROUPS )
   {
      firstGroupIdx = 0;
      lastGroupIdx  = pBridgeDesc->GetGirderGroupCount()-1;
   }
   else
   {
      lastGroupIdx = firstGroupIdx;
   }

   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

      GirderIndexType firstGdr, lastGdr;
      sel = m_ToGirder.GetCurSel();
      firstGdr = (GirderIndexType)m_ToGirder.GetItemData(sel);
      if ( firstGdr == ALL_GIRDERS )
      {
         firstGdr = 0;
         lastGdr = pGroup->GetGirderCount()-1;
      }
      else
      {
         lastGdr = firstGdr;
      }

      GirderIndexType nGirders = pGroup->GetGirderCount();
      for (GirderIndexType gdrIdx = firstGdr; gdrIdx <= lastGdr; gdrIdx++ )
      {
         GirderIndexType realGdrIdx = gdrIdx;
         if ( nGirders <= gdrIdx )
            realGdrIdx = nGirders-1;

         vec.push_back( CGirderKey(grpIdx,realGdrIdx) );
      }
   }
   }
   else
   {
      // data is in grid
      vec = m_MultiDialogSelections;
   }

   return vec;
}

void CCopyGirderDlg::OnCopyPrestressing() 
{
   UpdateApply();
}

void CCopyGirderDlg::OnCopyHandling() 
{
   UpdateApply();
}

void CCopyGirderDlg::OnCopySlabOffset() 
{
   UpdateApply();
}

void CCopyGirderDlg::OnCopyTransverse()  
{
   UpdateApply();
}

void CCopyGirderDlg::OnCopyLongitudinalRebar()
{
   UpdateApply();
}

void CCopyGirderDlg::OnHelp()
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DIALOG_COPYGDRPROPERTIES );
}

void CCopyGirderDlg::OnCopyMaterial() 
{
   UpdateApply();
}

void CCopyGirderDlg::OnCopyGirder() 
{
   UpdateApply();	
}

void CCopyGirderDlg::OnBnClickedRadio()
{
   BOOL enab_sgl = IsDlgButtonChecked(IDC_RADIO1) == BST_CHECKED ? TRUE : FALSE;
   BOOL enab_mpl = enab_sgl ? FALSE : TRUE;

   GetDlgItem(IDC_TO_SPAN)->EnableWindow(enab_sgl);
   GetDlgItem(IDC_TO_GIRDER)->EnableWindow(enab_sgl);

   GetDlgItem(IDC_SELECT_GIRDERS)->EnableWindow(enab_mpl);

   CopyToSelectionChanged();
}

void CCopyGirderDlg::OnBnClickedSelectGirders()
{
   CMultiGirderSelectDlg dlg;
   std::vector<CGirderKey>::iterator iter(m_MultiDialogSelections.begin());
   std::vector<CGirderKey>::iterator end(m_MultiDialogSelections.end());
   for ( ; iter != end; iter++ )
   {
      CGirderKey& girderKey(*iter);
      dlg.m_GirderKeys.push_back( girderKey );
   }

   if (dlg.DoModal()==IDOK)
   {
      // update button text
      CString msg;
      msg.Format(_T("Select Girders\n(%d Selected)"), dlg.m_GirderKeys.size());
      GetDlgItem(IDC_SELECT_GIRDERS)->SetWindowText(msg);

      m_MultiDialogSelections.clear();
      std::vector<CGirderKey>::const_iterator iter(dlg.m_GirderKeys.begin());
      std::vector<CGirderKey>::const_iterator end(dlg.m_GirderKeys.end());
      for ( ; iter != end; iter++ )
      {
         const CGirderKey& girderKey(*iter);
         m_MultiDialogSelections.push_back(girderKey);
      }

      CopyToSelectionChanged();
   }
}
