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

// DesignOutcomeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "DesignOutcomeDlg.h"

#include <IFace/Tools.h>
#include <EAF/AutoProgress.h>

#include <IFace\Artifact.h>
#include <EAF/EAFReportManager.h>

#include <EAF\EAFApp.h>
#include <EAF\EAFDocument.h>
#include <EAF\EAFCustSiteVars.h>


/////////////////////////////////////////////////////////////////////////////
// CDesignOutcomeDlg dialog
CDesignOutcomeDlg::CDesignOutcomeDlg(std::shared_ptr<CMultiGirderReportSpecification>& pRptSpec,const std::vector<CGirderKey>& girderKeys, arSlabOffsetDesignType designADim, CWnd* pParent /*=nullptr*/)
	: CDialog(CDesignOutcomeDlg::IDD, pParent), m_pRptSpec(pRptSpec), 
   m_GirderKeys(girderKeys), 
   m_DesignADimType(designADim), 
   m_SoSelectionType(sodtDoNotDesign)
{
	//{{AFX_DATA_INIT(CDesignOutcomeDlg)
	//}}AFX_DATA_INIT
}

void CDesignOutcomeDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CDesignOutcomeDlg)
   DDX_Control(pDX, IDC_STATIC_TEXT, m_Static);
   DDX_Control(pDX, IDOK, m_Ok);
   DDX_Control(pDX, IDCANCEL, m_Cancel);
   DDX_Control(pDX, IDC_PRINT, m_Print);
   DDX_Control(pDX, ID_HELP, m_Help);
   DDX_Control(pDX, IDC_CHECK_ADESIGN, m_ADesignCheckBox);
   DDX_Control(pDX, IDC_DESIGNA_FROM, m_ADesignFromCombo);
   DDX_Control(pDX, IDC_DESIGNA_STATIC, m_ADesignStatic);
   DDX_Control(pDX, IDC_DESIGNA_TO, m_ADesignToCombo);
   //}}AFX_DATA_MAP

   if (pDX->m_bSaveAndValidate)
   {
      m_SoSelectionType = sodtDoNotDesign;
      if(m_ADesignCheckBox.GetCheck() == BST_CHECKED)
      {
         int fromsel = m_ADesignFromCombo.GetCurSel();
         if (fromsel == CB_ERR || (int)m_GirderKeys.size() < fromsel )
         {
            ATLASSERT(0);
            return;
         }

         if (fromsel==m_GirderKeys.size())
         {
           m_SoSelectionType = sodtAllSelectedGirders;
         }
         else
         {
            m_FromSpanIdx   = m_GirderKeys[fromsel].groupIndex;
            m_FromGirderIdx = m_GirderKeys[fromsel].girderIndex;

            int tosel = m_ADesignToCombo.GetCurSel();
            if (tosel==CB_ERR)
            {
               ATLASSERT(0);
               return;
            }

            if (tosel==0)
            {
               m_SoSelectionType = sodtBridge;
            }
            else if (tosel==1)
            {
              m_SoSelectionType = sodtPier;
            }
            else if (tosel==2)
            {
              m_SoSelectionType = sodtGirder;
            }
            else
            {
               ATLASSERT(0);
            }
         }
      }
   }
}


BEGIN_MESSAGE_MAP(CDesignOutcomeDlg, CDialog)
	//{{AFX_MSG_MAP(CDesignOutcomeDlg)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_PRINT, OnPrint)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_CHECK_ADESIGN, &CDesignOutcomeDlg::OnBnClickedCheckAdesign)
   ON_CBN_SELCHANGE(IDC_DESIGNA_FROM, &CDesignOutcomeDlg::OnCbnSelchangeDesignaFrom)
   ON_COMMAND_RANGE(CCS_CMENU_BASE, CCS_CMENU_MAX, OnCmenuSelected)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDesignOutcomeDlg message handlers

void CDesignOutcomeDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);

   m_pBrowser->FitToParent();
}

void CDesignOutcomeDlg::OnPrint() 
{
   m_pBrowser->Print(true);
}

void CDesignOutcomeDlg::OnCancel() 
{
   CleanUp();
	CDialog::OnCancel();
}

void CDesignOutcomeDlg::OnOK() 
{
   CleanUp();
	CDialog::OnOK();
}

void CDesignOutcomeDlg::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_DIALOG_DESIGNCOMPLETE );
}

LRESULT CDesignOutcomeDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
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

BOOL CDesignOutcomeDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

   CRect rect;
   GetWindowRect(&rect);
   m_cxMin = rect.Width();
   m_cyMin = rect.Height();

   // don't show A dim controls if no A design
   if (m_DesignADimType==sodPreserveHaunch)
   {
      m_ADesignCheckBox.ShowWindow(SW_HIDE);
      m_ADesignFromCombo.ShowWindow(SW_HIDE);
      m_ADesignStatic.ShowWindow(SW_HIDE);
      m_ADesignToCombo.ShowWindow(SW_HIDE);
   }
   else
   {
      // fill from combo box
      for (std::vector<CGirderKey>::const_iterator it=m_GirderKeys.begin(); it!=m_GirderKeys.end(); it++)
      {
         CString str;
         str.Format(_T("Span %s, Girder %s"), LABEL_SPAN(it->groupIndex), LABEL_GIRDER(it->girderIndex));
         m_ADesignFromCombo.AddString(str);

         m_ADesignCheckBox.SetCheck(BST_CHECKED);
      }

      if (m_GirderKeys.size() > 1)
      {
         m_ADesignFromCombo.AddString(_T("All designed girders"));
      }

      m_ADesignFromCombo.SetCurSel(0); // this will fill and set up To box

      OnBnClickedCheckAdesign();
   }

   
   auto pBroker = m_pRptSpec->GetBroker();

   GET_IFACE2(pBroker,IEAFReportManager,pRptMgr);
   std::shared_ptr<WBFL::Reporting::ReportSpecification> pRptSpec = std::dynamic_pointer_cast<WBFL::Reporting::ReportSpecification,CMultiGirderReportSpecification>(m_pRptSpec);
   std::shared_ptr<WBFL::Reporting::ReportSpecificationBuilder> nullSpecBuilder;
   CWnd* pWnd = GetDlgItem(IDC_BROWSER);
   m_pBrowser = pRptMgr->CreateReportBrowser(pWnd->GetSafeHwnd(), WS_BORDER, pRptSpec, nullSpecBuilder);

   // restore the size of the window
   {
      CEAFApp* pApp = EAFGetApp();
      WINDOWPLACEMENT wp;
      if (pApp->ReadWindowPlacement(CString("Window Positions"),CString("DesignOutcome"),&wp))
      {
         HMONITOR hMonitor = MonitorFromRect(&wp.rcNormalPosition, MONITOR_DEFAULTTONULL); // get the monitor that has maximum overlap with the dialog rectangle (returns null if none)
         if (hMonitor != NULL)
         {
            // if dialog is within a monitor, set its position... otherwise the default position will be sued
            SetWindowPos(NULL, wp.rcNormalPosition.left, wp.rcNormalPosition.top, wp.rcNormalPosition.right - wp.rcNormalPosition.left, wp.rcNormalPosition.bottom - wp.rcNormalPosition.top, 0);
         }
      }
   }

   // If the design outcome is "Not Supported", the design artifact doesn't really have good information in it
   // We don't want the user to accept the design and updated the model with junk.
   // Make "reject" the only option
   GET_IFACE2(pBroker, IArtifact, pIArtifact);
   for (const auto& girderKey : m_GirderKeys)
   {
      const pgsGirderDesignArtifact* pGdrArtifact = pIArtifact->GetDesignArtifact(girderKey);
      const pgsSegmentDesignArtifact* pArtifact = pGdrArtifact->GetSegmentDesignArtifact(0);
      if (pArtifact->GetOutcome() == pgsSegmentDesignArtifact::DesignNotSupported_Losses || pArtifact->GetOutcome() == pgsSegmentDesignArtifact::DesignNotSupported_Strands)
      {
         m_Ok.EnableWindow(FALSE);
         break;
      }
   }


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDesignOutcomeDlg::CleanUp()
{
   if ( m_pBrowser )
   {
      m_pBrowser = std::shared_ptr<WBFL::Reporting::ReportBrowser>();
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
         pApp->WriteWindowPlacement(CString("Window Positions"),CString("DesignOutcome"),&wp);
      }
   }
}

void CDesignOutcomeDlg::OnBnClickedCheckAdesign()
{
   BOOL bEnable = m_ADesignCheckBox.GetCheck() == BST_CHECKED;
   m_ADesignFromCombo.EnableWindow(bEnable);
   m_ADesignToCombo.EnableWindow(bEnable);

   OnCbnSelchangeDesignaFrom();
}

void CDesignOutcomeDlg::OnCbnSelchangeDesignaFrom()
{
   int cursel = m_ADesignFromCombo.GetCurSel();
   if (cursel ==  CB_ERR)
   {
      cursel = m_ADesignFromCombo.SetCurSel(0);
   }

   int numGirders = (int)m_GirderKeys.size();

   if ( numGirders > 1 && cursel == numGirders)
   {
      // Special case: "all designed girders" was selected. Only one option here
      m_ADesignToCombo.ResetContent();
      m_ADesignToCombo.AddString(_T("To corresponding girders"));
      m_ADesignToCombo.SetCurSel(0);
   }
   else
   {
      if (0 <= cursel && cursel < (int)m_GirderKeys.size())
      {
         // Fill To box
         int tosel = m_ADesignToCombo.GetCurSel();
         tosel = tosel==CB_ERR ? 0 : tosel;

         m_ADesignToCombo.ResetContent();
         m_ADesignToCombo.AddString(_T("the entire bridge"));

         CString str2;
         str2.Format(_T("all girders in Span %s"), LABEL_SPAN(m_GirderKeys[cursel].groupIndex));
         m_ADesignToCombo.AddString(str2);

         CString str;
         str.Format(_T("Span %s, Girder %s Only"), LABEL_SPAN(m_GirderKeys[cursel].groupIndex), LABEL_GIRDER(m_GirderKeys[cursel].girderIndex));
         m_ADesignToCombo.AddString(str);

         m_ADesignToCombo.SetCurSel(tosel);
      }
      else
      {
         ::AfxMessageBox(_T("Error filling combo box")); // something horribly wrong here.
      }
   }
}

bool CDesignOutcomeDlg::GetSlabOffsetDesign(SlabOffsetDesignSelectionType* pSoSelectionType, SpanIndexType* pFromSpan, GirderIndexType* pFromGirder)
{
   *pSoSelectionType = m_SoSelectionType;
   *pFromSpan = m_FromSpanIdx;
   *pFromGirder = m_FromGirderIdx;

   return (m_SoSelectionType != sodtDoNotDesign);
}

void CDesignOutcomeDlg::OnCmenuSelected(UINT id)
{
  UINT cmd = id-CCS_CMENU_BASE ;

  switch(cmd)
  {
  case CCS_RB_EDIT:
//     OnEdit();
     break;

  case CCS_RB_FIND:
     m_pBrowser->Find();
     break;

  case CCS_RB_SELECT_ALL:
     m_pBrowser->SelectAll();
     break;

  case CCS_RB_COPY:
     m_pBrowser->Copy();
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
