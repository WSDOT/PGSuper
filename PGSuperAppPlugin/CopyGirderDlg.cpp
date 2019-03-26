///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#include "stdafx.h"
#include "PGSuperApp.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"
#include "CopyGirderDlg.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Selection.h>
#include <IFace\Transactions.h>

#include <PgsExt\MacroTxn.h>
#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCopyGirderDlg dialog


CCopyGirderDlg::CCopyGirderDlg(IBroker* pBroker, std::map<IDType,ICopyGirderPropertiesCallback*>& rCopyGirderPropertiesCallbacks, CWnd* pParent /*=nullptr*/)
	: CDialog(CCopyGirderDlg::IDD, pParent),
   m_pBroker(pBroker),
   m_rCopyGirderPropertiesCallbacks(rCopyGirderPropertiesCallbacks)
{
	//{{AFX_DATA_INIT(CCopyGirderDlg)
	//}}AFX_DATA_INIT

   // keep selection around
   GET_IFACE(ISelection,pSelection);
   m_FromSelection = pSelection->GetSelection();
}

std::vector<IDType> CCopyGirderDlg::GetCallbackIDs()
{
   return m_CallbackIDs;
}


void CCopyGirderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCopyGirderDlg)
   DDX_Control(pDX, IDC_LIST, m_PropertiesList);

   DDX_Control(pDX, IDC_FROM_SPAN,   m_FromGroup);
   DDX_Control(pDX, IDC_FROM_GIRDER, m_FromGirder);
   DDX_Control(pDX, IDC_TO_SPAN,     m_ToGroup);
   DDX_Control(pDX, IDC_TO_GIRDER,   m_ToGirder);
	//}}AFX_DATA_MAP

   if ( pDX->m_bSaveAndValidate )
   {
      m_FromGirderKey = GetFromGirder();
      m_ToGirderKeys  = GetToGirders();

      m_CallbackIDs.clear();
      int nItems = m_PropertiesList.GetCount();
      for ( int idx = 0; idx < nItems; idx++ )
      {
         if ( m_PropertiesList.GetCheck(idx) == BST_CHECKED )
         {
            IDType callbackID = (IDType)m_PropertiesList.GetItemData(idx);
            m_CallbackIDs.push_back(callbackID);
         }
      }

      // Save selection for next time we open
      m_FromSelection.Type = CSelection::Girder;
      m_FromSelection.GirderIdx = m_FromGirderKey.girderIndex;
      m_FromSelection.GroupIdx = m_FromGirderKey.groupIndex;
   }
   else
   {
      // start with combo box for To girder
      CButton* pBut = (CButton*)GetDlgItem(IDC_RADIO1);
      pBut->SetCheck(BST_CHECKED);
      GetDlgItem(IDC_SELECT_GIRDERS)->EnableWindow(false);

      if ( m_FromSelection.Type == CSelection::Girder || m_FromSelection.Type == CSelection::Segment )
      {
         m_FromGroup.SetCurSel((int)m_FromSelection.GroupIdx);
         OnFromGroupChanged();
         m_FromGirder.SetCurSel((int)m_FromSelection.GirderIdx);

         m_ToGroup.SetCurSel((int)m_FromSelection.GroupIdx+1);
         OnToGroupChanged();
      }

      CopyToSelectionChanged();
   }
}


BEGIN_MESSAGE_MAP(CCopyGirderDlg, CDialog)
	//{{AFX_MSG_MAP(CCopyGirderDlg)
   ON_CBN_SELCHANGE(IDC_FROM_SPAN,OnFromGroupChanged)
   ON_CBN_SELCHANGE(IDC_TO_SPAN,OnToGroupChanged)
   ON_CBN_SELCHANGE(IDC_TO_GIRDER,OnToGirderChanged)
   ON_CBN_SELCHANGE(IDC_FROM_GIRDER,OnFromGirderChanged)
	ON_BN_CLICKED(ID_HELP, OnHelp)
   ON_BN_CLICKED(IDC_RADIO1, &CCopyGirderDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_RADIO2, &CCopyGirderDlg::OnBnClickedRadio)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_SELECT_GIRDERS, &CCopyGirderDlg::OnBnClickedSelectGirders)
   ON_CLBN_CHKCHANGE(IDC_LIST,&CCopyGirderDlg::OnCopyItemStateChanged)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCopyGirderDlg message handlers

BOOL CCopyGirderDlg::OnInitDialog() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_COPY_PROPERTIES),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

   CEAFDocument* pDoc = EAFGetDocument();

   CComboBox* pcbFromGroup = (CComboBox*)GetDlgItem(IDC_FROM_SPAN);
   CComboBox* pcbFromGirder = (CComboBox*)GetDlgItem(IDC_FROM_GIRDER);
   CComboBox* pcbToGroup = (CComboBox*)GetDlgItem(IDC_TO_SPAN);
   CComboBox* pcbToGirder = (CComboBox*)GetDlgItem(IDC_TO_GIRDER);
   FillComboBoxes(*pcbFromGroup,*pcbFromGirder,false,false);
   FillComboBoxes(*pcbToGroup,  *pcbToGirder,  pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) ? false : true, true );

   CDialog::OnInitDialog();

   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) )
   {
      // in PGSplice, copying can only happen within a group
      // disable the to group combo box and keep it in sync with the from group combo box
      m_ToGroup.EnableWindow(FALSE);
   }


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

   CopyToSelectionChanged();
   EnableCopyNow(TRUE);
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
   EnableCopyNow(TRUE);
}

void CCopyGirderDlg::OnToGirderChanged()
{
   CopyToSelectionChanged();
   EnableCopyNow(TRUE);
}

void CCopyGirderDlg::OnFromGirderChanged()
{
   CopyToSelectionChanged();
   EnableCopyNow(TRUE);
}

void CCopyGirderDlg::CopyToSelectionChanged() 
{
   CGirderKey copyFrom = GetFromGirder();
   std::vector<CGirderKey> copyTo = GetToGirders();

   std::map<IDType,int> buttonStates;
   int nItems = m_PropertiesList.GetCount();
   for ( int idx = 0; idx < nItems; idx++ )
   {
      IDType callbackID = (IDType)m_PropertiesList.GetItemData(idx);
      buttonStates.insert(std::make_pair(callbackID,m_PropertiesList.GetCheck(idx)));
   }

   m_PropertiesList.ResetContent();

   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   const std::map<IDType,ICopyGirderPropertiesCallback*>& callbacks = pDoc->GetCopyGirderPropertiesCallbacks();
   std::map<IDType,ICopyGirderPropertiesCallback*>::const_iterator iter(callbacks.begin());
   std::map<IDType,ICopyGirderPropertiesCallback*>::const_iterator end(callbacks.end());
   for ( ; iter != end; iter++ )
   {
      IDType callbackID = iter->first;
      ICopyGirderPropertiesCallback* pCallback = iter->second;
      if ( pCallback->CanCopy(copyFrom,copyTo) )
      {
         LPCTSTR strName = pCallback->GetName();
         int idx = m_PropertiesList.AddString(strName);
         m_PropertiesList.SetItemData(idx,(DWORD_PTR)callbackID);

         int checked = BST_CHECKED;
         std::map<IDType,int>::iterator found = buttonStates.find(callbackID);
         if ( found != buttonStates.end() )
         {
            checked = found->second;
         }
         m_PropertiesList.SetCheck(idx,checked);
      }
   }
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

void CCopyGirderDlg::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_DIALOG_COPYGDRPROPERTIES );
}

void CCopyGirderDlg::OnBnClickedRadio()
{
   BOOL enab_sgl = IsDlgButtonChecked(IDC_RADIO1) == BST_CHECKED ? TRUE : FALSE;
   BOOL enab_mpl = enab_sgl ? FALSE : TRUE;

   GetDlgItem(IDC_TO_SPAN)->EnableWindow(enab_sgl);
   GetDlgItem(IDC_TO_GIRDER)->EnableWindow(enab_sgl);

   GetDlgItem(IDC_SELECT_GIRDERS)->EnableWindow(enab_mpl);

   if ( enab_mpl && m_MultiDialogSelections.size() == 0 )
   {
      OnBnClickedSelectGirders();
   }
   else
   {
      CopyToSelectionChanged();
   }
   EnableCopyNow(TRUE);
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
      EnableCopyNow(TRUE);
   }
   else
   {
      if (m_MultiDialogSelections.empty())
      {
         // was cancelled and nothing selected. Go back to single selection
         CButton* pBut = (CButton*)GetDlgItem(IDC_RADIO1);
         pBut->SetCheck(BST_CHECKED);
         pBut = (CButton*)GetDlgItem(IDC_RADIO2);
         pBut->SetCheck(BST_UNCHECKED);

         OnBnClickedRadio();
      }
   }
}

void CCopyGirderDlg::OnOK()
{
   // CDialog::OnOK(); // we are completely bypassing the default implementation and doing our own thing
   // want the dialog to stay open until the Close buttom is pressed


   UpdateData(TRUE);

   // execute transactions
   pgsMacroTxn* pMacro = new pgsMacroTxn;
   pMacro->Name(_T("Copy Girder Properties"));

   std::vector<IDType> callbackIDs = GetCallbackIDs();
   std::vector<IDType>::iterator iter(callbackIDs.begin());
   std::vector<IDType>::iterator end(callbackIDs.end());
   for ( ; iter != end; iter++ )
   {
      IDType callbackID = *iter;
      std::map<IDType,ICopyGirderPropertiesCallback*>::iterator found(m_rCopyGirderPropertiesCallbacks.find(callbackID));
      ATLASSERT(found != m_rCopyGirderPropertiesCallbacks.end());
      ICopyGirderPropertiesCallback* pCallback = found->second;

      txnTransaction* pTxn = pCallback->CreateCopyTransaction(m_FromGirderKey,m_ToGirderKeys);
      if ( pTxn )
      {
         pMacro->AddTransaction(pTxn);
      }
   }

   if ( 0 < pMacro->GetTxnCount() )
   {
      GET_IFACE(IEAFTransactions,pTransactions);
      pTransactions->Execute(pMacro);
   }

   CopyToSelectionChanged();
   EnableCopyNow(FALSE);
}

void CCopyGirderDlg::OnCopyItemStateChanged()
{
   EnableCopyNow(TRUE);
}

void CCopyGirderDlg::EnableCopyNow(BOOL bEnable)
{
   GetDlgItem(IDOK)->EnableWindow(bEnable);
}
