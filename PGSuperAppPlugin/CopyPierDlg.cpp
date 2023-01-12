///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// CopyPierDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"
#include "CopyPierDlg.h"
#include "CopyPierPropertiesCallbacks.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Selection.h>
#include <IFace\Transactions.h>
#include <IFace\EditByUI.h>

#include <PgsExt\MacroTxn.h>
#include <PgsExt\BridgeDescription2.h>
#include <EAF\EAFCustSiteVars.h>

#include <IReportManager.h>
#include <Reporting\CopyPierPropertiesReportSpecification.h>
#include <Reporting\CopyPierPropertiesChapterBuilder.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCopyPierDlg dialog


CCopyPierDlg::CCopyPierDlg(IBroker* pBroker, const std::map<IDType,ICopyPierPropertiesCallback*>&  rCopyPierPropertiesCallbacks, IDType selectedID, CWnd* pParent /*=nullptr*/)
	: CDialog(CCopyPierDlg::IDD, pParent),
   m_pBroker(pBroker),
   m_CopyPierPropertiesCallbacks(rCopyPierPropertiesCallbacks)
{
	//{{AFX_DATA_INIT(CCopyPierDlg)
	//}}AFX_DATA_INIT

   // keep selection around
   GET_IFACE(ISelection,pSelection);
   m_FromSelection = pSelection->GetSelection();

   // Special case here if selected ID is INVALID_ID
   if (INVALID_ID == selectedID)
   {
      for (auto callback : rCopyPierPropertiesCallbacks)
      {
         m_SelectedIDs.insert(callback.first);
      }
   }
   else
   {
      m_SelectedIDs.insert(selectedID);
   }
}

BEGIN_MESSAGE_MAP(CCopyPierDlg, CDialog)
	//{{AFX_MSG_MAP(CCopyPierDlg)
	ON_WM_SIZE()
   ON_CBN_SELCHANGE(IDC_FROM_PIER,OnFromPierChanged)
   ON_CBN_SELCHANGE(IDC_TO_PIER,OnToPierChanged)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_PRINT, OnPrint)
	//}}AFX_MSG_MAP
   ON_COMMAND_RANGE(CCS_CMENU_BASE, CCS_CMENU_MAX, OnCmenuSelected)
   ON_WM_DESTROY()
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
   ON_LBN_SELCHANGE(IDC_PROPERTY_LIST, &CCopyPierDlg::OnLbnSelchangePropertyList)
   ON_CLBN_CHKCHANGE(IDC_PROPERTY_LIST, &CCopyPierDlg::OnLbnChkchangePropertyList)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCopyPierDlg message handlers

BOOL CCopyPierDlg::OnInitDialog() 
{
   // Want to keep our size GE original size
   CRect rect;
   GetWindowRect(&rect);
   m_cxMin = rect.Width();
   m_cyMin = rect.Height();

   // set up report window
   GET_IFACE(IReportManager, pReportMgr);
   CReportDescription rptDesc = pReportMgr->GetReportDescription(_T("Copy Pier Properties Report"));
   std::shared_ptr<CReportSpecificationBuilder> pRptSpecBuilder = pReportMgr->GetReportSpecificationBuilder(rptDesc);
   std::shared_ptr<CReportSpecification> pRptSpec = pRptSpecBuilder->CreateDefaultReportSpec(rptDesc);

   m_pRptSpec = std::dynamic_pointer_cast<CCopyPierPropertiesReportSpecification, CReportSpecification>(pRptSpec);

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_COPY_PROPERTIES),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

   CDialog::OnInitDialog();

   InitSelectedPropertyList();
   OnFromPierChangedNoUpdate();

   EnableToolTips(TRUE);

   // set up reporting window
   UpdateReportData();

   GET_IFACE(IReportManager,pRptMgr);
   std::shared_ptr<CReportSpecificationBuilder> nullSpecBuilder;
   m_pBrowser = pRptMgr->CreateReportBrowser(GetSafeHwnd(),pRptSpec,nullSpecBuilder);
   m_pBrowser->GetBrowserWnd()->ModifyStyle(0,WS_BORDER );

   // restore the size of the window
   {
      CEAFApp* pApp = EAFGetApp();
      WINDOWPLACEMENT wp;
      if (pApp->ReadWindowPlacement(CString("Window Positions"),CString("CopyPierDialog"),&wp))
      {
         HMONITOR hMonitor = MonitorFromRect(&wp.rcNormalPosition, MONITOR_DEFAULTTONULL); // get the monitor that has maximum overlap with the dialog rectangle (returns null if none)
         if (hMonitor != NULL)
         {
            // if dialog is within a monitor, set its position... otherwise the default position will be sued
            SetWindowPos(NULL, wp.rcNormalPosition.left, wp.rcNormalPosition.top, wp.rcNormalPosition.right - wp.rcNormalPosition.left, wp.rcNormalPosition.bottom - wp.rcNormalPosition.top, 0);
         }
      }
   }

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCopyPierDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCopyPierDlg)
   DDX_Control(pDX, IDC_FROM_PIER,   m_FromPier);
   DDX_Control(pDX, IDC_TO_PIER,     m_ToPier);
   DDX_Control(pDX, IDC_PROPERTY_LIST,   m_SelectedPropertyTypesCL);
	//}}AFX_DATA_MAP

   if ( pDX->m_bSaveAndValidate )
   {
      m_FromPierIdx = GetFromPier();
      m_ToPiers  = GetToPiers();

      // Save selection for next time we open
      m_FromSelection.Type = CSelection::Pier;
      m_FromSelection.PierIdx = m_FromPierIdx;
   }
   else
   {
      FillComboBoxes(m_FromPier,false);
      FillComboBoxes(m_ToPier, true );

      int curFromsel = 0;
      if ( m_FromSelection.Type == CSelection::Pier  )
      {
         curFromsel = (int)m_FromSelection.PierIdx;
      }

      m_FromPier.SetCurSel(curFromsel);
      m_ToPier.SetCurSel(0);
   }
}

void CCopyPierDlg::OnFromPierChanged() 
{
   OnFromPierChangedNoUpdate();
   EnableCopyNow();
   UpdateReport(); // Report needs to show newly selected girder
}

void CCopyPierDlg::OnFromPierChangedNoUpdate()
{
   int curSel = m_FromPier.GetCurSel();
   if ( curSel != CB_ERR )
   {
      PierIndexType PierIdx = (PierIndexType)m_FromPier.GetItemData(curSel);
      FillComboBoxes(m_ToPier, true, PierIdx );
   }
   else
   {
      ATLASSERT(0);
   }

   m_ToPier.SetCurSel(1);
}

void CCopyPierDlg::OnToPierChanged()
{
   EnableCopyNow();
}

PierIndexType CCopyPierDlg::GetFromPier()
{
   int curSel = m_FromPier.GetCurSel();
   if (curSel != CB_ERR)
   {
      PierIndexType PierIdx = (PierIndexType)m_FromPier.GetItemData(curSel);
      return PierIdx;
   }
   else
   {
      ATLASSERT(0);
      return INVALID_INDEX;
   }
}

std::vector<PierIndexType> CCopyPierDlg::GetToPiers()
{
   int curSel = m_ToPier.GetCurSel();
   if (curSel != CB_ERR)
   {
      PierIndexType PierIdx = (PierIndexType)m_ToPier.GetItemData(curSel);
      if (ALL_PIERS == PierIdx)
      {
         std::vector<PierIndexType> vec;
         GET_IFACE(IBridgeDescription,pIBridgeDesc);
         const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

         GroupIndexType nPiers = pBridgeDesc->GetPierCount();
         for (PierIndexType iPier = 0; iPier < nPiers; iPier++)
         {
            vec.push_back(iPier);
         }

         return vec;
      }
      else
      {
         return std::vector<PierIndexType>(1,PierIdx);
      }
   }
   else
   {
      ATLASSERT(0);
      return std::vector<PierIndexType>(1,INVALID_INDEX);
   }
}

void CCopyPierDlg::OnSize(UINT nType, int cx, int cy)
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

void CCopyPierDlg::FillComboBoxes(CComboBox& cbPier, bool bIncludeAllPiers, PierIndexType excludeIdx)
{
   cbPier.ResetContent();

   if ( bIncludeAllPiers )
   {
      CString strItem (_T("All Piers"));
      int idx = cbPier.AddString(strItem);
      cbPier.SetItemData(idx,ALL_PIERS);
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GroupIndexType nPiers = pBridgeDesc->GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      bool isAbut = pierIdx == 0 || pierIdx == nPiers - 1;

      // Don't add pier with exclude idx
      if (pierIdx != excludeIdx)
      {
         CString str = pgsPierLabel::GetPierLabelEx(isAbut, pierIdx).c_str();

         int idx = cbPier.AddString(str);
         cbPier.SetItemData(idx, pierIdx);
      }
   }

   cbPier.SetCurSel(0);
}

void CCopyPierDlg::UpdateReportData()
{
   GET_IFACE(IReportManager,pReportMgr);
   std::shared_ptr<CReportBuilder> pBuilder = pReportMgr->GetReportBuilder( m_pRptSpec->GetReportName() );

   PierIndexType pierIdx = GetFromPier();
   std::vector<PierIndexType> toPiers = GetToPiers();

   std::vector<ICopyPierPropertiesCallback*> callbacks = GetSelectedCopyPierPropertiesCallbacks();

   // We know we put at least one of our own chapter builders into the report builder. Find it and set its data
   CollectionIndexType numchs = pBuilder->GetChapterBuilderCount();
   for (CollectionIndexType ich = 0; ich < numchs; ich++)
   {
      std::shared_ptr<CChapterBuilder> pChb = pBuilder->GetChapterBuilder(ich);
      std::shared_ptr<CCopyPierPropertiesChapterBuilder> pRptCpBuilder = std::dynamic_pointer_cast<CCopyPierPropertiesChapterBuilder,CChapterBuilder>(pChb);

      if (pRptCpBuilder)
      {
         pRptCpBuilder->SetCopyPierProperties(callbacks, pierIdx, toPiers);
      }
   }
}

void CCopyPierDlg::UpdateReport()
{
   if ( m_pBrowser )
   {
      UpdateReportData();

      GET_IFACE(IReportManager,pReportMgr);
      std::shared_ptr<CReportBuilder> pBuilder = pReportMgr->GetReportBuilder( m_pRptSpec->GetReportName() );

      std::shared_ptr<CReportSpecification> pRptSpec = std::dynamic_pointer_cast<CReportSpecification,CCopyPierPropertiesReportSpecification>(m_pRptSpec);

      std::shared_ptr<rptReport> pReport = pBuilder->CreateReport( pRptSpec );
      m_pBrowser->UpdateReport( pReport, true );
   }
}

std::vector<ICopyPierPropertiesCallback*> CCopyPierDlg::GetSelectedCopyPierPropertiesCallbacks()
{
   // double duty here. saving selected ids and returning callbacks
   m_SelectedIDs.clear();
   std::vector<ICopyPierPropertiesCallback*> callbacks;

   int nProps = m_SelectedPropertyTypesCL.GetCount();
   for ( int ch = 0; ch < nProps; ch++ )
   {
      if ( m_SelectedPropertyTypesCL.GetCheck( ch ) == 1 )
      {
         IDType id = (IDType)m_SelectedPropertyTypesCL.GetItemData(ch);
         m_SelectedIDs.insert(id);

         std::map<IDType, ICopyPierPropertiesCallback*>::const_iterator it = m_CopyPierPropertiesCallbacks.find(id);
         if (it != m_CopyPierPropertiesCallbacks.end())
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

void CCopyPierDlg::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_DIALOG_COPYPIERPROPERTIES );
}

void CCopyPierDlg::OnOK()
{
   // CDialog::OnOK(); // we are completely bypassing the default implementation and doing our own thing
   // want the dialog to stay open until the Close buttom is pressed

   UpdateData(TRUE);

   // execute transactions
   pgsMacroTxn* pMacro = new pgsMacroTxn;
   pMacro->Name(_T("Copy Pier Properties"));

   std::vector<ICopyPierPropertiesCallback*> callbacks = GetSelectedCopyPierPropertiesCallbacks();
   for (auto callback : callbacks)
   {
      txnTransaction* pTxn = callback->CreateCopyTransaction(m_FromPierIdx, m_ToPiers);
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

void CCopyPierDlg::OnCopyItemStateChanged()
{
   EnableCopyNow();
}

void CCopyPierDlg::OnEdit()
{
   PierIndexType fromIdx = GetFromPier();

   GET_IFACE(IEditByUI, pEditByUI);
   UINT tab = 0; // use if nothing is selected
   std::vector<ICopyPierPropertiesCallback*> callbacks = GetSelectedCopyPierPropertiesCallbacks();
   if (!callbacks.empty())
   {
      tab = callbacks.front()->GetPierEditorTabIndex();
   }

   pEditByUI->EditPierDescription(fromIdx, tab);

   UpdateReport(); // we update whether any changes are made or not
   EnableCopyNow();
}

void CCopyPierDlg::OnPrint() 
{
   m_pBrowser->Print(true);
}

void CCopyPierDlg::EnableCopyNow()
{
   // Must be able to copy all to girders before enabling control
   PierIndexType copyFrom = GetFromPier();
   std::vector<PierIndexType> copyTo = GetToPiers();

   BOOL bEnable;
   if (-1 != IsAllSelectedInList())
   {
      bEnable = TRUE; // can always copy if all is selected
   }
   else
   {
      std::vector<ICopyPierPropertiesCallback*> callbacks = GetSelectedCopyPierPropertiesCallbacks();

      bEnable = callbacks.empty() ? FALSE : TRUE;
      for (auto callback : callbacks)
      {
         bEnable &= callback->CanCopy(copyFrom, copyTo) ? TRUE : FALSE;
      }
   }

   GetDlgItem(IDOK)->EnableWindow(bEnable);
}

void CCopyPierDlg::CleanUp()
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
         pApp->WriteWindowPlacement(CString("Window Positions"),CString("CopyPierDialog"),&wp);
      }
   }
}


LRESULT CCopyPierDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
   // prevent the dialog from getting smaller than the original size
   if ( message == WM_SIZING )
   {
      LPRECT rect = (LPRECT)lParam;
      int cx = rect->right - rect->left;
      int cy = rect->bottom - rect->top;

      if ( cx < m_cxMin || cy < m_cyMin )
      {
         // prevent the dialog from moving right or down
         if ( wParam == WMSZ_BOTTOMLEFT ||
              wParam == WMSZ_LEFT       ||
              wParam == WMSZ_TOP        ||
              wParam == WMSZ_TOPLEFT    ||
              wParam == WMSZ_TOPRIGHT )
         {
            CRect r;
            GetWindowRect(&r);
            rect->left = r.left;
            rect->top  = r.top;
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


void CCopyPierDlg::OnDestroy()
{
   CDialog::OnDestroy();

   CleanUp();
}

void CCopyPierDlg::OnCmenuSelected(UINT id)
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

BOOL CCopyPierDlg::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);
      switch(nID)
      {
      case IDC_FROM_PIER:
         m_strTip = _T("The selected \"From\" pier is highlighted in Yellow in Comparison report");
         break;
      case IDC_EDIT:
         m_strTip = _T("Edit the selected \"From\" pier");
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


void CCopyPierDlg::InitSelectedPropertyList()
{
   // Clear out the list box
   m_SelectedPropertyTypesCL.ResetContent();

   for (const auto& CBpair : m_CopyPierPropertiesCallbacks)
   {
      ICopyPierPropertiesCallback* pCB = CBpair.second;
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

void CCopyPierDlg::UpdateSelectedPropertyList()
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

void CCopyPierDlg::OnLbnSelchangePropertyList()
{
   // Do nothing here, but this function seems necessary to catch message for OnLbnChkchangePropertyList below. Otherwise, it won't be called?
}

void CCopyPierDlg::OnLbnChkchangePropertyList()
{
   UpdateSelectedPropertyList();
   UpdateReport();
}

int CCopyPierDlg::IsAllSelectedInList()
{
   int nProps = m_SelectedPropertyTypesCL.GetCount();
   for (int ch = 0; ch < nProps; ch++)
   {
      int chkval = m_SelectedPropertyTypesCL.GetCheck(ch);
      if (chkval == 1)
      {
         IDType id = (IDType)m_SelectedPropertyTypesCL.GetItemData(ch);
         std::map<IDType, ICopyPierPropertiesCallback*>::const_iterator it = m_CopyPierPropertiesCallbacks.find(id);

         if (nullptr != dynamic_cast<CCopyPierAllProperties*>(it->second))
         {
            return ch;
         }
      }
   }

   return -1;
}