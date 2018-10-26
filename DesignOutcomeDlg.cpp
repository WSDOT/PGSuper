///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "DesignOutcomeDlg.h"

#include <EAF\EAFAutoProgress.h>

#include <IFace\Artifact.h>
#include <IReportManager.h>

#include <EAF\EAFApp.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDesignOutcomeDlg message handlers

void CDesignOutcomeDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
   ATLASSERT(m_Ok.m_hWnd!=0);

   // Convert a 7du x 7du rect into pixels
   CRect sizeRect(0,0,7,7);
   MapDialogRect(&sizeRect);

   CRect btnSizeRect(0,0,50,14);
   MapDialogRect( &btnSizeRect );

   CRect btnResultSizeRect(0,0,95,14);
   MapDialogRect( &btnResultSizeRect );

   CRect clientRect;
   GetClientRect( &clientRect );

   // Figure out the print button's new position
   CRect printRect;
   printRect.bottom = clientRect.bottom - sizeRect.Height();
   printRect.right  = clientRect.right  - sizeRect.Width();
   printRect.left   = printRect.right   - btnSizeRect.Width();
   printRect.top    = printRect.bottom  - btnSizeRect.Height();

   // Figure out the help button's new position
   CRect helpRect;
   helpRect.bottom = printRect.bottom;
   helpRect.right  = printRect.left   - sizeRect.Width();
   helpRect.left   = helpRect.right  - btnSizeRect.Width();
   helpRect.top    = printRect.top;

   // Figure out the ok button's new position
   CRect okRect;
   okRect.bottom = printRect.bottom;
   okRect.top    = printRect.top;
   okRect.left   = clientRect.left + sizeRect.Width();
   okRect.right  = okRect.left    + btnResultSizeRect.Width();

   CRect cancelRect;
   cancelRect.bottom = printRect.bottom;
   cancelRect.top    = printRect.top;
   cancelRect.left   = okRect.right  + sizeRect.Width();
   cancelRect.right  = okRect.right + btnResultSizeRect.Width();

   // Figure out the static box and its controls' new positions
   CRect staticRect;
   staticRect.bottom = printRect.top - sizeRect.Height()/10;
   staticRect.right  = printRect.right;
   staticRect.left   = clientRect.left  + sizeRect.Width();
   staticRect.top    = staticRect.bottom - btnSizeRect.Height();

   // A Design options control positions
   CRect aDimCheckSizeRect(0,0,172,14);
   MapDialogRect( &aDimCheckSizeRect );

   CRect ADimCheckRect;
   ADimCheckRect.bottom = staticRect.top - sizeRect.Height();
   ADimCheckRect.top    = ADimCheckRect.bottom - btnSizeRect.Height();
   ADimCheckRect.left   = clientRect.left  + sizeRect.Width();
   ADimCheckRect.right  = ADimCheckRect.left + aDimCheckSizeRect.Width();

   CRect aDimComboRect(0,0,95,14);
   MapDialogRect( &aDimComboRect );

   CRect ADimFromRect;
   ADimFromRect.bottom = ADimCheckRect.bottom;
   ADimFromRect.top    = ADimCheckRect.top;
   ADimFromRect.left   = ADimCheckRect.right;
   ADimFromRect.right  = ADimFromRect.left + aDimComboRect.Width();

   CRect ADimStaticRect;
   ADimStaticRect.bottom = ADimCheckRect.bottom;
   ADimStaticRect.top    = ADimCheckRect.top;
   ADimStaticRect.left   = ADimFromRect.right;
   ADimStaticRect.right  = ADimStaticRect.left + 2*sizeRect.Width();

   CRect AToRect;
   AToRect.bottom = ADimCheckRect.bottom;
   AToRect.top    = ADimCheckRect.top;
   AToRect.left   = ADimStaticRect.right;
   AToRect.right  = AToRect.left + aDimComboRect.Width();

   // Figure out the browser's new position
   CRect browserRect;
   browserRect.left   = clientRect.left + sizeRect.Width();
   browserRect.top    = clientRect.top  + sizeRect.Height();
   browserRect.right  = printRect.right;
   browserRect.bottom = ADimCheckRect.top - sizeRect.Height();

   m_Ok.MoveWindow( okRect, FALSE );
   m_Cancel.MoveWindow( cancelRect, FALSE );
   m_Help.MoveWindow( helpRect, FALSE );
   m_Print.MoveWindow( printRect, FALSE );
   m_Static.MoveWindow( staticRect, FALSE );

   m_ADesignCheckBox.MoveWindow( ADimCheckRect, FALSE );
   m_ADesignFromCombo.MoveWindow( ADimFromRect, FALSE );
   m_ADesignStatic.MoveWindow( ADimStaticRect, FALSE );
   m_ADesignToCombo.MoveWindow( AToRect, FALSE );

   m_pBrowser->Move(browserRect.TopLeft());
   m_pBrowser->Size( browserRect.Size() );

   Invalidate();
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

BOOL CDesignOutcomeDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

   // don't show A dim controls if no A design
   if (m_DesignADimType==dtNoDesign)
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
         str.Format(_T("Span %d, Girder %s"), LABEL_SPAN(it->groupIndex), LABEL_GIRDER(it->girderIndex));
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

   CComPtr<IBroker> pBroker;
   m_pRptSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IReportManager,pRptMgr);
   std::shared_ptr<CReportSpecification> pRptSpec = std::dynamic_pointer_cast<CReportSpecification,CMultiGirderReportSpecification>(m_pRptSpec);
   std::shared_ptr<CReportSpecificationBuilder> nullSpecBuilder;
   m_pBrowser = pRptMgr->CreateReportBrowser(GetSafeHwnd(),pRptSpec,nullSpecBuilder);

   // restore the size of the window
   {
      CEAFApp* pApp = EAFGetApp();
      WINDOWPLACEMENT wp;
      if (pApp->ReadWindowPlacement(CString("Settings"),CString("DesignOutcome"),&wp))
      {
         CRect rect(wp.rcNormalPosition);
         SetWindowPos(nullptr,0,0,rect.Size().cx,rect.Size().cy,SWP_NOMOVE);
      }
   }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDesignOutcomeDlg::CleanUp()
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
         pApp->WriteWindowPlacement(CString("Settings"),CString("DesignOutcome"),&wp);
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
         str2.Format(_T("all girders in Span %d"), LABEL_SPAN(m_GirderKeys[cursel].groupIndex));
         m_ADesignToCombo.AddString(str2);

         CString str;
         str.Format(_T("Span %d, Girder %s Only"), LABEL_SPAN(m_GirderKeys[cursel].groupIndex), LABEL_GIRDER(m_GirderKeys[cursel].girderIndex));
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
