///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include "CopyGirderPropertiesCallbacks.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Selection.h>
#include <IFace\Transactions.h>
#include <IFace\EditByUI.h>

#include <PgsExt\MacroTxn.h>
#include <PgsExt\BridgeDescription2.h>
#include <EAF\EAFCustSiteVars.h>

#include <IReportManager.h>
#include <Reporting\CopyGirderPropertiesReportSpecification.h>
#include <Reporting\CopyGirderPropertiesChapterBuilder.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCopyGirderDlg dialog

CCopyGirderDlg::CCopyGirderDlg(IBroker* pBroker, const std::map<IDType,ICopyGirderPropertiesCallback*>&  rcopyGirderPropertiesCallbacks, IDType selectedID, CWnd* pParent)
	: CDialog(CCopyGirderDlg::IDD, pParent),
   m_pBroker(pBroker),
   m_CopyGirderPropertiesCallbacks(rcopyGirderPropertiesCallbacks)
{
	//{{AFX_DATA_INIT(CCopyGirderDlg)
	//}}AFX_DATA_INIT

   // keep selection around
   GET_IFACE(ISelection,pSelection);
   m_FromSelection = pSelection->GetSelection();

   CEAFDocument* pDoc = EAFGetDocument();
   m_bIsPGSplice = pDoc->IsKindOf(RUNTIME_CLASS(CPGSpliceDoc)) != FALSE;

   // Special case here if selected ID is INVALID_ID
   if (INVALID_ID == selectedID)
   {
      for (auto callback : rcopyGirderPropertiesCallbacks)
      {
         m_SelectedIDs.insert(callback.first);
      }
   }
   else
   {
      m_SelectedIDs.insert(selectedID);
   }
}

void CCopyGirderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCopyGirderDlg)
   DDX_Control(pDX, IDC_FROM_SPAN,   m_FromGroup);
   DDX_Control(pDX, IDC_FROM_GIRDER, m_FromGirder);
   DDX_Control(pDX, IDC_TO_SPAN,     m_ToGroup);
   DDX_Control(pDX, IDC_TO_GIRDER,   m_ToGirder);
   DDX_Control(pDX, IDC_PROPERTY_LIST,   m_SelectedPropertyTypesCL);
	//}}AFX_DATA_MAP

   if ( pDX->m_bSaveAndValidate )
   {
      m_FromGirderKey = GetFromGirder();
      m_ToGirderKeys  = GetToGirders();

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

      if ( m_FromSelection.Type == CSelection::Pier || m_FromSelection.Type == CSelection::Segment )
      {
         m_FromGroup.SetCurSel((int)m_FromSelection.GroupIdx);
         m_FromGirder.SetCurSel((int)m_FromSelection.GirderIdx);

         m_ToGroup.SetCurSel((int)m_FromSelection.GroupIdx+ (m_bIsPGSplice? 0:1));
      }
   }
}

BEGIN_MESSAGE_MAP(CCopyGirderDlg, CDialog)
	//{{AFX_MSG_MAP(CCopyGirderDlg)
	ON_WM_SIZE()
   ON_CBN_SELCHANGE(IDC_FROM_SPAN,OnFromGroupChanged)
   ON_CBN_SELCHANGE(IDC_TO_SPAN,OnToGroupChanged)
   ON_CBN_SELCHANGE(IDC_TO_GIRDER,OnToGirderChanged)
   ON_CBN_SELCHANGE(IDC_FROM_GIRDER,OnFromGirderChanged)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_PRINT, OnPrint)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_RADIO1, &CCopyGirderDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_RADIO2, &CCopyGirderDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_SELECT_GIRDERS, &CCopyGirderDlg::OnBnClickedSelectGirders)
   ON_COMMAND_RANGE(CCS_CMENU_BASE, CCS_CMENU_MAX, OnCmenuSelected)
   ON_WM_DESTROY()
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
   ON_CLBN_CHKCHANGE(IDC_PROPERTY_LIST, &CCopyGirderDlg::OnLbnChkchangePropertyList)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCopyGirderDlg message handlers

BOOL CCopyGirderDlg::OnInitDialog() 
{
   // Want to keep our size GE original size
   CRect rect;
   GetWindowRect(&rect);
   m_cxMin = rect.Width();
   m_cyMin = rect.Height();

   // set up report window
   GET_IFACE(IReportManager, pReportMgr);
   CReportDescription rptDesc = pReportMgr->GetReportDescription(_T("Copy Girder Properties Report"));
   std::shared_ptr<CReportSpecificationBuilder> pRptSpecBuilder = pReportMgr->GetReportSpecificationBuilder(rptDesc);
   std::shared_ptr<CReportSpecification> pRptSpec = pRptSpecBuilder->CreateDefaultReportSpec(rptDesc);

   m_pRptSpec = std::dynamic_pointer_cast<CCopyGirderPropertiesReportSpecification, CReportSpecification>(pRptSpec);

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_COPY_PROPERTIES),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

   CEAFDocument* pDoc = EAFGetDocument();

   CComboBox* pcbFromGroup = (CComboBox*)GetDlgItem(IDC_FROM_SPAN);
   CComboBox* pcbFromGirder = (CComboBox*)GetDlgItem(IDC_FROM_GIRDER);
   CComboBox* pcbToGroup = (CComboBox*)GetDlgItem(IDC_TO_SPAN);
   CComboBox* pcbToGirder = (CComboBox*)GetDlgItem(IDC_TO_GIRDER);
   FillComboBoxes(*pcbFromGroup,*pcbFromGirder,false,false);
   FillComboBoxes(*pcbToGroup,  *pcbToGirder,  m_bIsPGSplice ? false : true, true );


   CDialog::OnInitDialog();

   InitSelectedPropertyList();

   OnFromGroupChangedNoUpdate();
   OnToGroupChangedNoUpdate();

   EnableToolTips(TRUE);

   if ( m_bIsPGSplice )
   {
      // in PGSplice, copying can only happen within a group
      // disable the to group combo box and keep it in sync with the from group combo box
      m_ToGroup.EnableWindow(FALSE);

      // don't allow multi-select. if desired, it will be difficult to modify the grid control
      GetDlgItem(IDC_RADIO2)->ShowWindow(FALSE); 
      GetDlgItem(IDC_SELECT_GIRDERS)->ShowWindow(FALSE); 
   }

   // set up reporting window
   UpdateReportData();

   GET_IFACE(IReportManager,pRptMgr);
   std::shared_ptr<CReportSpecificationBuilder> nullSpecBuilder;
   m_pBrowser = pRptMgr->CreateReportBrowser(GetSafeHwnd(),pRptSpec,nullSpecBuilder);
   m_pBrowser->GetBrowserWnd()->ModifyStyle(0,WS_BORDER);

   // restore the size of the window
   {
      CEAFApp* pApp = EAFGetApp();
      WINDOWPLACEMENT wp;
      if (pApp->ReadWindowPlacement(CString("Window Positions"),CString("CopyGirderDialog"),&wp))
      {
         CWnd* pDesktop = GetDesktopWindow();
         //CRect rDesktop;
         //pDesktop->GetWindowRect(&rDesktop); // this is the size of one monitor.... use GetSystemMetrics to get the entire desktop
         CRect rDesktop(0, 0, GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_CYVIRTUALSCREEN));
         CRect rDlg(wp.rcNormalPosition);
         if (rDesktop.PtInRect(rDlg.TopLeft()) && rDesktop.PtInRect(rDlg.BottomRight()))
         {
            // if dialog is within the desktop area, set its position... otherwise the default position will be sued
            SetWindowPos(NULL, wp.rcNormalPosition.left, wp.rcNormalPosition.top, wp.rcNormalPosition.right - wp.rcNormalPosition.left, wp.rcNormalPosition.bottom - wp.rcNormalPosition.top, 0);
         }
      }
   }

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCopyGirderDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

   if (m_pBrowser)
   {
      CRect clientRect;
      GetClientRect( &clientRect );

      CRect sizeRect(0,0,7,7);
      MapDialogRect(&sizeRect);

      CRect hiddenRect;
      GetDlgItem(IDC_BROWSER)->GetWindowRect(&hiddenRect);
      ScreenToClient(hiddenRect);
      m_pBrowser->Move(hiddenRect.TopLeft());
      m_pBrowser->Size(hiddenRect.Size());

      // bottom buttons
      CRect btnSizeRect(0,0,50,14);
      MapDialogRect( &btnSizeRect );

      CRect btnRect;
      btnRect.bottom = clientRect.bottom - sizeRect.Height();
      btnRect.right  = clientRect.right  - LONG(sizeRect.Width() * 3); 
      btnRect.left   = btnRect.right   - btnSizeRect.Width();
      btnRect.top    = btnRect.bottom  - btnSizeRect.Height();

      CButton* pButton = (CButton*)GetDlgItem(ID_HELP);
      pButton->MoveWindow( btnRect, FALSE );

      CRect printRect(btnRect); // put print button directly above Help

      CRect horizOffsetRect(0,0,66,0); // horizontal spacing between buttons
      MapDialogRect( &horizOffsetRect );
      CSize horizOffset(-1*horizOffsetRect.Width(),0);

      btnRect += horizOffset;
      pButton = (CButton*)GetDlgItem(IDCANCEL);
      pButton->MoveWindow( btnRect, FALSE );

      btnRect += horizOffset;
      pButton = (CButton*)GetDlgItem(IDOK);
      pButton->MoveWindow( btnRect, FALSE );

      CSize vertOffset(0, int(btnSizeRect.Height() * 1.75));
      printRect -= vertOffset;
      pButton = (CButton*)GetDlgItem(IDC_PRINT);
      pButton->MoveWindow( printRect, FALSE );

      Invalidate();
   }
}

void CCopyGirderDlg::FillComboBoxes(CComboBox& cbGroup,CComboBox& cbGirder, bool bIncludeAllGroups, bool bIncludeAllGirders)
{
   cbGroup.ResetContent();

   CString strGroupLabel;
   BOOL bIsPGSuper = !m_bIsPGSplice;
   if (bIsPGSuper)
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
      if (bIsPGSuper)
      {
         str.Format(_T("%s %s"), strGroupLabel, LABEL_SPAN(grpIdx));
      }
      else
      {
         str.Format(_T("%s %d"), strGroupLabel, LABEL_GROUP(grpIdx));
      }

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

void CCopyGirderDlg::UpdateReportData()
{
   GET_IFACE(IReportManager,pReportMgr);
   std::shared_ptr<CReportBuilder> pBuilder = pReportMgr->GetReportBuilder( m_pRptSpec->GetReportName() );

   CGirderKey gdrKey = GetFromGirder();

   std::vector<ICopyGirderPropertiesCallback*> callbacks = GetSelectedCopyGirderPropertiesCallbacks();

   // We know we put at least one of our own chapter builders into the report builder. Find it and set its data
   CollectionIndexType numchs = pBuilder->GetChapterBuilderCount();
   for (CollectionIndexType ich = 0; ich < numchs; ich++)
   {
      std::shared_ptr<CChapterBuilder> pChb = pBuilder->GetChapterBuilder(ich);
      std::shared_ptr<CCopyGirderPropertiesChapterBuilder> pRptCpBuilder = std::dynamic_pointer_cast<CCopyGirderPropertiesChapterBuilder,CChapterBuilder>(pChb);

      if (pRptCpBuilder)
      {
         pRptCpBuilder->SetCopyGirderProperties(callbacks, gdrKey);
      }
   }
}

void CCopyGirderDlg::UpdateReport()
{
   if ( m_pBrowser )
   {
      UpdateReportData();

      GET_IFACE(IReportManager,pReportMgr);
      std::shared_ptr<CReportBuilder> pBuilder = pReportMgr->GetReportBuilder( m_pRptSpec->GetReportName() );

      std::shared_ptr<CReportSpecification> pRptSpec = std::dynamic_pointer_cast<CReportSpecification,CCopyGirderPropertiesReportSpecification>(m_pRptSpec);

      std::shared_ptr<rptReport> pReport = pBuilder->CreateReport( pRptSpec );
      m_pBrowser->UpdateReport( pReport, true );
   }
}

std::vector<ICopyGirderPropertiesCallback*> CCopyGirderDlg::GetSelectedCopyGirderPropertiesCallbacks()
{
   // double duty here. saving selected ids and returning callbacks
   m_SelectedIDs.clear();
   std::vector<ICopyGirderPropertiesCallback*> callbacks;

   int nProps = m_SelectedPropertyTypesCL.GetCount();
   for ( int ch = 0; ch < nProps; ch++ )
   {
      if ( m_SelectedPropertyTypesCL.GetCheck( ch ) == 1 )
      {
         IDType id = (IDType)m_SelectedPropertyTypesCL.GetItemData(ch);
         m_SelectedIDs.insert(id);

         std::map<IDType, ICopyGirderPropertiesCallback*>::const_iterator it = m_CopyGirderPropertiesCallbacks.find(id);
         if (it != m_CopyGirderPropertiesCallbacks.end())
         {
            callbacks.push_back(it->second);
         }
         else
         {
            ATLASSERT(0); 
         }
      }
   }

   return callbacks;
}

void CCopyGirderDlg::OnFromGroupChanged() 
{
   OnFromGroupChangedNoUpdate();

   EnableCopyNow();
   UpdateReport(); // Report needs to show newly selected girder
}

void CCopyGirderDlg::OnFromGroupChangedNoUpdate()
{
   int curSel = m_FromGroup.GetCurSel();
   if (curSel != CB_ERR)
   {
      GroupIndexType groupIdx = (GroupIndexType)m_FromGroup.GetItemData(curSel);
      FillGirderComboBox(m_FromGirder, groupIdx, false);
   }
   else
   {
      FillGirderComboBox(m_FromGirder, 0, false);
   }
}

void CCopyGirderDlg::OnToGroupChanged() 
{
   OnToGroupChangedNoUpdate();
   EnableCopyNow();
}

void CCopyGirderDlg::OnToGroupChangedNoUpdate()
{
   int curSel = m_ToGroup.GetCurSel();
   if (curSel != CB_ERR)
   {
      GroupIndexType groupIdx = (GroupIndexType)m_ToGroup.GetItemData(curSel);
      FillGirderComboBox(m_ToGirder, groupIdx, true);
   }
   else
   {
      FillGirderComboBox(m_ToGirder, 0, true);
   }
}

void CCopyGirderDlg::OnToGirderChanged()
{
   EnableCopyNow();
}

void CCopyGirderDlg::OnFromGirderChanged()
{
   EnableCopyNow();

   UpdateReport();
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

   EnableCopyNow();
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

      EnableCopyNow();
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
   pMacro->Name(_T("Copy Girder Properties Macro"));

   std::vector<ICopyGirderPropertiesCallback*> callbacks = GetSelectedCopyGirderPropertiesCallbacks();
   for (auto callback : callbacks)
   {
      txnTransaction* pTxn = callback->CreateCopyTransaction(m_FromGirderKey, m_ToGirderKeys);
      pMacro->AddTransaction(pTxn);
   }

   if (pMacro->GetTxnCount() > 0)
   {
      GET_IFACE(IEAFTransactions, pTransactions);
      pTransactions->Execute(pMacro);
   }

   UpdateReport();
   EnableCopyNow();
}

void CCopyGirderDlg::OnEdit()
{
   CGirderKey fromKey = GetFromGirder();

   GET_IFACE(IEditByUI, pEditByUI);
   UINT tab = 0; // use if nothing is selected
   std::vector<ICopyGirderPropertiesCallback*> callbacks = GetSelectedCopyGirderPropertiesCallbacks();
   if (!callbacks.empty())
   {
      tab = callbacks.front()->GetGirderEditorTabIndex();
   }

   pEditByUI->EditGirderDescription(fromKey, tab);

   UpdateReport(); // we update whether any changes are made or not
   EnableCopyNow();
}

void CCopyGirderDlg::OnPrint() 
{
   m_pBrowser->Print(true);
}

void CCopyGirderDlg::EnableCopyNow()
{
   // Must be able to copy all to girders before enabling control
   CGirderKey copyFrom = GetFromGirder();
   std::vector<CGirderKey> copyTo = GetToGirders();

   BOOL bEnable;
   if (-1 != IsAllSelectedInList())
   {
      bEnable = TRUE; // can always copy if all is selected
   }
   else
   {
      std::vector<ICopyGirderPropertiesCallback*> callbacks = GetSelectedCopyGirderPropertiesCallbacks();

      bEnable = callbacks.empty() ? FALSE : TRUE;
      for (auto callback : callbacks)
      {
         bEnable &= callback->CanCopy(copyFrom, copyTo) ? TRUE : FALSE;
      }
   }

   GetDlgItem(IDOK)->EnableWindow(bEnable);
}

void CCopyGirderDlg::CleanUp()
{
   if ( m_pBrowser )
   {
      m_pBrowser = std::shared_ptr<CReportBrowser>();
   }

   // save the size of the window
   WINDOWPLACEMENT wp;
   wp.length = sizeof wp;
   {
      CEAFApp* pApp = EAFGetApp();
      if (GetWindowPlacement(&wp))
      {
         wp.flags = 0;
         wp.showCmd = SW_SHOWNORMAL;
         pApp->WriteWindowPlacement(CString("Window Positions"),CString("CopyGirderDialog"),&wp);
      }
   }
}

LRESULT CCopyGirderDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
   // prevent the dialog from getting smaller than the original size
   if (message == WM_SIZING)
   {
      LPRECT rect = (LPRECT)lParam;
      int cx = rect->right - rect->left;
      int cy = rect->bottom - rect->top;

      if (cx < m_cxMin || cy < m_cyMin)
      {
         // prevent the dialog from moving right or down
         if (wParam == WMSZ_BOTTOMLEFT ||
            wParam == WMSZ_LEFT ||
            wParam == WMSZ_TOP ||
            wParam == WMSZ_TOPLEFT ||
            wParam == WMSZ_TOPRIGHT)
         {
            CRect r;
            GetWindowRect(&r);
            rect->left = r.left;
            rect->top = r.top;
         }

         if (cx < m_cxMin)
         {
            rect->right = rect->left + m_cxMin;
         }

         if (cy < m_cyMin)
         {
            rect->bottom = rect->top + m_cyMin;
         }

         return TRUE;
      }
   }

   return CDialog::WindowProc(message, wParam, lParam);
}

void CCopyGirderDlg::OnDestroy()
{
   CDialog::OnDestroy();

   CleanUp();
}

BOOL CCopyGirderDlg::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);
      switch(nID)
      {
      case IDC_FROM_SPAN:
      case IDC_FROM_GIRDER:
         m_strTip = _T("The selected \"From\" girder is highlighted in Yellow in Comparison report");
         break;
      case IDC_EDIT:
         m_strTip = _T("Edit the selected \"From\" girder");
         break;

      default:
         return FALSE;
      }

      ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
      pTTT->lpszText = m_strTip.GetBuffer();
      pTTT->hinst = nullptr;
      return TRUE;
   }
   return FALSE;
}

void CCopyGirderDlg::OnCmenuSelected(UINT id)
{
  UINT cmd = id-CCS_CMENU_BASE ;

  switch(cmd)
  {
  case CCS_RB_EDIT:
     OnEdit();
     break;

  case CCS_RB_FIND:
     m_pBrowser->Find();
     break;

  case CCS_RB_SELECT_ALL:
     m_pBrowser->SelectAll();
     break;

  case CCS_RB_PRINT:
     m_pBrowser->Print(true);
     break;

  case CCS_RB_REFRESH:
     m_pBrowser->Refresh();
     break;

  case CCS_RB_VIEW_SOURCE:
     m_pBrowser->ViewSource();
     break;

  case CCS_RB_VIEW_BACK:
     m_pBrowser->Back();
     break;

  case CCS_RB_VIEW_FORWARD:
     m_pBrowser->Forward();
     break;

  default:
     // must be a toc anchor
     ATLASSERT(cmd>=CCS_RB_TOC);
     m_pBrowser->NavigateAnchor(cmd-CCS_RB_TOC);
  }
}

void CCopyGirderDlg::InitSelectedPropertyList()
{
   // Clear out the list box
   m_SelectedPropertyTypesCL.ResetContent();

   for (const auto& CBpair : m_CopyGirderPropertiesCallbacks)
   {
      ICopyGirderPropertiesCallback* pCB = CBpair.second;
      int idx = m_SelectedPropertyTypesCL.AddString( pCB->GetName() );
      if ( idx != LB_ERR )
      {
         IDType id = CBpair.first;
         m_SelectedPropertyTypesCL.SetItemData(idx, id);

         if (m_SelectedIDs.find(id) != m_SelectedIDs.end())
         {
            m_SelectedPropertyTypesCL.SetCheck(idx, 1);
         }
         else
         {
            m_SelectedPropertyTypesCL.SetCheck(idx, 0);
         }
      }
   }

   UpdateSelectedPropertyList();

   m_SelectedPropertyTypesCL.SetCurSel(-1);
}

void CCopyGirderDlg::UpdateSelectedPropertyList()
{
   // see if "All Properties" is selected
   int all_pos = IsAllSelectedInList();
   bool is_all_selected = all_pos != -1;

   int nProps = m_SelectedPropertyTypesCL.GetCount();

   if (is_all_selected)
   {
      // "All" is selected. Set check and enable all in list
      for (int ch = 0; ch < nProps; ch++)
      {
         if (ch != all_pos)
         {
            m_SelectedPropertyTypesCL.SetCheck(ch, 1);
            m_SelectedPropertyTypesCL.Enable(ch, FALSE);
         }
      }
   }
   else
   {
      for (int ch = 0; ch < nProps; ch++)
      {
         m_SelectedPropertyTypesCL.Enable(ch, TRUE);
      }
   }

   EnableCopyNow();
}

void CCopyGirderDlg::OnLbnChkchangePropertyList()
{
   UpdateSelectedPropertyList();
   UpdateReport();
}

int CCopyGirderDlg::IsAllSelectedInList()
{
   int nProps = m_SelectedPropertyTypesCL.GetCount();
   for (int ch = 0; ch < nProps; ch++)
   {
      int chkval = m_SelectedPropertyTypesCL.GetCheck(ch);
      if (chkval == 1)
      {
         IDType id = (IDType)m_SelectedPropertyTypesCL.GetItemData(ch);
         std::map<IDType, ICopyGirderPropertiesCallback*>::const_iterator it = m_CopyGirderPropertiesCallbacks.find(id);

         if (nullptr != dynamic_cast<CCopyGirderAllProperties*>(it->second))
         {
            return ch;
         }
      }
   }

   return -1;
}
