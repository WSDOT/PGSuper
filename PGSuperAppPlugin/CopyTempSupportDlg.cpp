///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

// CopyTempSupportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "PGSpliceDoc.h"
#include "CopyTempSupportDlg.h"
#include "CopyTempSupportPropertiesCallbacks.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Selection.h>
#include <IFace\Transactions.h>
#include <IFace\EditByUI.h>

#include <PgsExt\MacroTxn.h>
#include <PgsExt\BridgeDescription2.h>
#include <EAF\EAFCustSiteVars.h>

#include <IReportManager.h>
#include <Reporting\CopyTempSupportPropertiesReportSpecification.h>
#include <Reporting\CopyTempSupportPropertiesChapterBuilder.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCopyTempSupportDlg dialog
CCopyTempSupportDlg::CCopyTempSupportDlg(IBroker* pBroker, const std::map<IDType,ICopyTemporarySupportPropertiesCallback*>&  rCopyTempSupportPropertiesCallbacks, IDType selectedID, CWnd* pParent /*=nullptr*/)
	: CDialog(CCopyTempSupportDlg::IDD, pParent),
   m_pBroker(pBroker),
   m_CopyTempSupportPropertiesCallbacks(rCopyTempSupportPropertiesCallbacks)
{
	//{{AFX_DATA_INIT(CCopyTempSupportDlg)
	//}}AFX_DATA_INIT

   // keep selection around
   GET_IFACE(ISelection,pSelection);
   m_FromSelection = pSelection->GetSelection();

   // Special case here if selected ID is INVALID_ID
   if (INVALID_ID == selectedID)
   {
      for (auto callback : rCopyTempSupportPropertiesCallbacks)
      {
         m_SelectedIDs.insert(callback.first);
      }
   }
   else
   {
      m_SelectedIDs.insert(selectedID);
   }
}

BEGIN_MESSAGE_MAP(CCopyTempSupportDlg, CDialog)
	//{{AFX_MSG_MAP(CCopyTempSupportDlg)
	ON_WM_SIZE()
   ON_CBN_SELCHANGE(IDC_FROM_PIER,OnFromTempSupportChanged)
   ON_CBN_SELCHANGE(IDC_TO_PIER,OnToTempSupportChanged)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	ON_BN_CLICKED(IDC_EDIT, OnEdit)
	ON_BN_CLICKED(IDC_PRINT, OnPrint)
	//}}AFX_MSG_MAP
   ON_COMMAND_RANGE(CCS_CMENU_BASE, CCS_CMENU_MAX, OnCmenuSelected)
   ON_WM_DESTROY()
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
   ON_LBN_SELCHANGE(IDC_PROPERTY_LIST, &CCopyTempSupportDlg::OnLbnSelchangePropertyList)
   ON_CLBN_CHKCHANGE(IDC_PROPERTY_LIST, &CCopyTempSupportDlg::OnLbnChkchangePropertyList)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCopyTempSupportDlg message handlers

BOOL CCopyTempSupportDlg::OnInitDialog() 
{
   // Want to keep our size GE original size
   CRect rect;
   GetWindowRect(&rect);
   m_cxMin = rect.Width();
   m_cyMin = rect.Height();

   // set up report window
   GET_IFACE(IReportManager, pReportMgr);
   CReportDescription rptDesc = pReportMgr->GetReportDescription(_T("Copy Temporary Support Properties Report"));
   std::shared_ptr<CReportSpecificationBuilder> pRptSpecBuilder = pReportMgr->GetReportSpecificationBuilder(rptDesc);
   std::shared_ptr<CReportSpecification> pRptSpec = pRptSpecBuilder->CreateDefaultReportSpec(rptDesc);

   m_pRptSpec = std::dynamic_pointer_cast<CCopyTempSupportPropertiesReportSpecification, CReportSpecification>(pRptSpec);

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_COPY_PROPERTIES),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

   CDialog::OnInitDialog();

   SetWindowText(_T("Copy Temporary Support Properties"));

   InitSelectedPropertyList();
   OnFromTempSupportChangedNoUpdate();

   EnableToolTips(TRUE);

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
      // Share placement with copy pier dialog
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

void CCopyTempSupportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCopyTempSupportDlg)
   DDX_Control(pDX, IDC_FROM_PIER,   m_FromTempSupport);
   DDX_Control(pDX, IDC_TO_PIER,     m_ToTempSupport);
   DDX_Control(pDX, IDC_PROPERTY_LIST,   m_SelectedPropertyTypesCL);
	//}}AFX_DATA_MAP

   if ( pDX->m_bSaveAndValidate )
   {
      m_FromTempSupportIdx = GetFromTempSupport();
      m_ToTempSupports  = GetToTempSupports();

      // Save selection for next time we open
      m_FromSelection.Type = CSelection::TemporarySupport;
      m_FromSelection.tsID = m_FromTempSupportIdx;
   }
   else
   {
      FillComboBoxes(m_FromTempSupport,false);
      FillComboBoxes(m_ToTempSupport, true );

      int curFromsel = 0;
      if ( m_FromSelection.Type == CSelection::TemporarySupport  )
      {
         curFromsel = (int)m_FromSelection.tsID;
      }

      m_FromTempSupport.SetCurSel(curFromsel);
      m_ToTempSupport.SetCurSel(0);
   }
}

void CCopyTempSupportDlg::OnFromTempSupportChanged() 
{
   OnFromTempSupportChangedNoUpdate();
   EnableCopyNow();
   UpdateReport(); // Report needs to show newly selected girder
}

void CCopyTempSupportDlg::OnFromTempSupportChangedNoUpdate()
{

   int curSel = m_FromTempSupport.GetCurSel();
   if ( curSel != CB_ERR )
   {
      PierIndexType TempSupportIdx = (PierIndexType)m_FromTempSupport.GetItemData(curSel);
      FillComboBoxes(m_ToTempSupport, true, TempSupportIdx );
   }
   else
   {
      ATLASSERT(0);
   }

   m_ToTempSupport.SetCurSel(1);
}

void CCopyTempSupportDlg::OnToTempSupportChanged()
{
   EnableCopyNow();
}

PierIndexType CCopyTempSupportDlg::GetFromTempSupport()
{
   int curSel = m_FromTempSupport.GetCurSel();
   if (curSel != CB_ERR)
   {
      PierIndexType TempSupportIdx = (PierIndexType)m_FromTempSupport.GetItemData(curSel);
      return TempSupportIdx;
   }
   else
   {
      ATLASSERT(0);
      return INVALID_INDEX;
   }
}

std::vector<PierIndexType> CCopyTempSupportDlg::GetToTempSupports()
{
   int curSel = m_ToTempSupport.GetCurSel();
   if (curSel != CB_ERR)
   {
      PierIndexType TempSupportIdx = (PierIndexType)m_ToTempSupport.GetItemData(curSel);
      if (ALL_PIERS == TempSupportIdx)
      {
         std::vector<PierIndexType> vec;
         GET_IFACE(IBridgeDescription,pIBridgeDesc);
         const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

         GroupIndexType nTempSupports = pBridgeDesc->GetTemporarySupportCount();
         for (PierIndexType iTempSupport = 0; iTempSupport < nTempSupports; iTempSupport++)
         {
            vec.push_back(iTempSupport);
         }

         return vec;
      }
      else
      {
         return std::vector<PierIndexType>(1,TempSupportIdx);
      }
   }
   else
   {
      ATLASSERT(0);
      return std::vector<PierIndexType>(1,INVALID_INDEX);
   }
}

void CCopyTempSupportDlg::OnSize(UINT nType, int cx, int cy)
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

void CCopyTempSupportDlg::FillComboBoxes(CComboBox& cbTempSupport, bool bIncludeAllTempSupports, PierIndexType excludeIdx)
{
   cbTempSupport.ResetContent();

   if ( bIncludeAllTempSupports )
   {
      CString strItem (_T("All Temporary Supports"));
      int idx = cbTempSupport.AddString(strItem);
      cbTempSupport.SetItemData(idx,ALL_PIERS);
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GroupIndexType nTempSupports = pBridgeDesc->GetTemporarySupportCount();
   for ( PierIndexType TempSupportIdx = 0; TempSupportIdx < nTempSupports; TempSupportIdx++ )
   {
      bool isAbut = TempSupportIdx == 0 || TempSupportIdx == nTempSupports - 1;

      // Don't add TempSupport with exclude idx
      if (TempSupportIdx != excludeIdx)
      {
         const CTemporarySupportData* pTempSupport = pBridgeDesc->GetTemporarySupport(TempSupportIdx);

         CString strts;
         strts.Format(_T("%s %d"), CTemporarySupportData::AsString(pTempSupport->GetSupportType()), LABEL_TEMPORARY_SUPPORT( TempSupportIdx));

         int idx = cbTempSupport.AddString(strts);
         cbTempSupport.SetItemData(idx, TempSupportIdx);
      }
   }

   cbTempSupport.SetCurSel(0);
}

void CCopyTempSupportDlg::UpdateReportData()
{
   GET_IFACE(IReportManager,pReportMgr);
   std::shared_ptr<CReportBuilder> pBuilder = pReportMgr->GetReportBuilder( m_pRptSpec->GetReportName() );

   PierIndexType TempSupportIdx = GetFromTempSupport();
   std::vector<PierIndexType> toTempSupports = GetToTempSupports();

   std::vector<ICopyTemporarySupportPropertiesCallback*> callbacks = GetSelectedCopyTempSupportPropertiesCallbacks();

   // We know we put at least one of our own chapter builders into the report builder. Find it and set its data
   CollectionIndexType numchs = pBuilder->GetChapterBuilderCount();
   for (CollectionIndexType ich = 0; ich < numchs; ich++)
   {
      std::shared_ptr<CChapterBuilder> pChb = pBuilder->GetChapterBuilder(ich);
      std::shared_ptr<CCopyTempSupportPropertiesChapterBuilder> pRptCpBuilder = std::dynamic_pointer_cast<CCopyTempSupportPropertiesChapterBuilder,CChapterBuilder>(pChb);

      if (pRptCpBuilder)
      {
         pRptCpBuilder->SetCopyTempSupportProperties(callbacks, TempSupportIdx, toTempSupports);      }
   }
}

void CCopyTempSupportDlg::UpdateReport()
{
   if ( m_pBrowser )
   {
      UpdateReportData();

      GET_IFACE(IReportManager,pReportMgr);
      std::shared_ptr<CReportBuilder> pBuilder = pReportMgr->GetReportBuilder( m_pRptSpec->GetReportName() );

      std::shared_ptr<CReportSpecification> pRptSpec = std::dynamic_pointer_cast<CReportSpecification,CCopyTempSupportPropertiesReportSpecification>(m_pRptSpec);

      std::shared_ptr<rptReport> pReport = pBuilder->CreateReport( pRptSpec );
      m_pBrowser->UpdateReport( pReport, true );
   }
}

std::vector<ICopyTemporarySupportPropertiesCallback*> CCopyTempSupportDlg::GetSelectedCopyTempSupportPropertiesCallbacks()
{
   // double duty here. saving selected ids and returning callbacks
   m_SelectedIDs.clear();
   std::vector<ICopyTemporarySupportPropertiesCallback*> callbacks;

   int nProps = m_SelectedPropertyTypesCL.GetCount();
   for ( int ch = 0; ch < nProps; ch++ )
   {
      if ( m_SelectedPropertyTypesCL.GetCheck( ch ) == 1 )
      {
         IDType id = (IDType)m_SelectedPropertyTypesCL.GetItemData(ch);
         m_SelectedIDs.insert(id);

         std::map<IDType, ICopyTemporarySupportPropertiesCallback*>::const_iterator it = m_CopyTempSupportPropertiesCallbacks.find(id);
         if (it != m_CopyTempSupportPropertiesCallbacks.end())
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

void CCopyTempSupportDlg::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_DIALOG_COPYTEMPSUPPORTPROPERTIES );
}

void CCopyTempSupportDlg::OnOK()
{
   // CDialog::OnOK(); // we are completely bypassing the default implementation and doing our own thing
   // want the dialog to stay open until the Close buttom is pressed

   UpdateData(TRUE);

   // execute transactions
   std::unique_ptr<pgsMacroTxn> pMacro(std::make_unique<pgsMacroTxn>());
   pMacro->Name(_T("Copy Temporary Support Properties"));

   std::vector<ICopyTemporarySupportPropertiesCallback*> callbacks = GetSelectedCopyTempSupportPropertiesCallbacks();
   for (auto callback : callbacks)
   {
      auto txn = callback->CreateCopyTransaction(m_FromTempSupportIdx, m_ToTempSupports);
      pMacro->AddTransaction(std::move(txn));
   }

   if (0 < pMacro->GetTxnCount())
   {
      GET_IFACE(IEAFTransactions, pTransactions);
      pTransactions->Execute(std::move(pMacro));
   }

   UpdateReport();
   EnableCopyNow();
}

void CCopyTempSupportDlg::OnCopyItemStateChanged()
{
   EnableCopyNow();
}

void CCopyTempSupportDlg::OnEdit()
{
   PierIndexType fromIdx = GetFromTempSupport();

   GET_IFACE(IEditByUI, pEditByUI);
   UINT tab = 0; // use if nothing is selected
   std::vector<ICopyTemporarySupportPropertiesCallback*> callbacks = GetSelectedCopyTempSupportPropertiesCallbacks();
   if (!callbacks.empty())
   {
      tab = callbacks.front()->GetTempSupportEditorTabIndex();
   }

   pEditByUI->EditTemporarySupportDescription(fromIdx, tab);

   UpdateReport(); // we update whether any changes are made or not

   EnableCopyNow();
}

void CCopyTempSupportDlg::OnPrint() 
{
   m_pBrowser->Print(true);
}

void CCopyTempSupportDlg::EnableCopyNow()
{
   // Must be able to copy all to girders before enabling control
   PierIndexType copyFrom = GetFromTempSupport();
   std::vector<PierIndexType> copyTo = GetToTempSupports();

   BOOL bEnable;
   if (-1 != IsAllSelectedInList())
   {
      bEnable = TRUE; // can always copy if all is selected
   }
   else
   {
      std::vector<ICopyTemporarySupportPropertiesCallback*> callbacks = GetSelectedCopyTempSupportPropertiesCallbacks();

      bEnable = callbacks.empty() ? FALSE : TRUE;
      for (auto callback : callbacks)
      {
         bEnable &= callback->CanCopy(copyFrom, copyTo) ? TRUE : FALSE;
      }
   }

   GetDlgItem(IDOK)->EnableWindow(bEnable);
}

void CCopyTempSupportDlg::CleanUp()
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
         // Use same position as copy pier dialog to avoid user confusion
         pApp->WriteWindowPlacement(CString("Window Positions"),CString("CopyPierDialog"),&wp);
      }
   }
}


LRESULT CCopyTempSupportDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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


void CCopyTempSupportDlg::OnDestroy()
{
   CDialog::OnDestroy();

   CleanUp();
}

void CCopyTempSupportDlg::OnCmenuSelected(UINT id)
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

BOOL CCopyTempSupportDlg::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
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
         m_strTip = _T("The selected \"From\" support is highlighted in Yellow in Comparison report");
         break;
      case IDC_EDIT:
         m_strTip = _T("Edit the selected \"From\" temporary support");
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



void CCopyTempSupportDlg::InitSelectedPropertyList()
{
   // Clear out the list box
   m_SelectedPropertyTypesCL.ResetContent();

   for (const auto& CBpair : m_CopyTempSupportPropertiesCallbacks)
   {
      ICopyTemporarySupportPropertiesCallback* pCB = CBpair.second;
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

void CCopyTempSupportDlg::UpdateSelectedPropertyList()
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

void CCopyTempSupportDlg::OnLbnSelchangePropertyList()
{
   // Do nothing here, but this function seems necessary to catch message for OnLbnChkchangePropertyList below. Otherwise, it won't be called?
}

void CCopyTempSupportDlg::OnLbnChkchangePropertyList()
{
   UpdateSelectedPropertyList();
   UpdateReport();
}

int CCopyTempSupportDlg::IsAllSelectedInList()
{
   // "All" not an option - yet

   return -1;
}