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

// SpecHaulingErectionPage.cpp : implementation file
//

#include "stdafx.h"
#include "psgLib\psglib.h"
#include "SpecHaulingErectionPage.h"
#include "SpecMainSheet.h"
#include <EAF\EAFDocument.h>
#include <psgLib/HaulingCriteria.h>

#include <MFCTools\MFCTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecHaulingErectionPage property page

IMPLEMENT_DYNCREATE(CSpecHaulingErectionPage, CPropertyPage)

CSpecHaulingErectionPage::CSpecHaulingErectionPage() : CPropertyPage(CSpecHaulingErectionPage::IDD,IDS_HAULING_ERECTION),
m_BeforeInit(true)
{
	//{{AFX_DATA_INIT(CSpecHaulingErectionPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSpecHaulingErectionPage::~CSpecHaulingErectionPage()
{
}

void CSpecHaulingErectionPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecHaulingErectionPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   // dad is a friend of the entry. use him to transfer data.
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();

   if (m_BeforeInit)
   {
      // First time through we must fill both dialogs with data
      ATLASSERT(!pDX->m_bSaveAndValidate);

      DDX_CBEnum(pDX,IDC_HAULING_METHOD, m_HaulingAnalysisMethod);

      CDataExchange DxDlgW(&m_WsdotHaulingDlg, pDX->m_bSaveAndValidate);
      pDad->ExchangeWsdotHaulingData(&DxDlgW);

      CDataExchange DxDlgK(&m_KdotHaulingDlg, pDX->m_bSaveAndValidate);
      pDad->ExchangeKdotHaulingData(&DxDlgK);
   }

   SwapDialogs();

   if (pDX->m_bSaveAndValidate)
   {
      // On way out - save data from appropriate dialog
      if (m_HaulingAnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT)
      {
         CDataExchange DxDlg(&m_WsdotHaulingDlg, pDX->m_bSaveAndValidate);
         pDad->ExchangeWsdotHaulingData(&DxDlg);
      }
      else
      {
         CDataExchange DxDlg(&m_KdotHaulingDlg, pDX->m_bSaveAndValidate);
         pDad->ExchangeKdotHaulingData(&DxDlg);
      }
   }
}


BEGIN_MESSAGE_MAP(CSpecHaulingErectionPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecHaulingErectionPage)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(ID_HELP,OnHelp)
   ON_CBN_SELCHANGE(IDC_HAULING_METHOD, &CSpecHaulingErectionPage::OnCbnSelchangeHaulingMethod)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecHaulingErectionPage message handlers
void CSpecHaulingErectionPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_PROJECT_CRITERIA_HAULING );
}

BOOL CSpecHaulingErectionPage::OnInitDialog() 
{
   // Some initial data
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   m_HaulingAnalysisMethod = pDad->m_Entry.GetHaulingCriteria().AnalysisMethod;

   // Embed dialogs for wsdot/kdot editing into current. A description may be found at
   // http://www.codeproject.com/KB/dialog/embedded_dialog.aspx
   CWnd* pBox = GetDlgItem(IDC_STATIC_BOUNDS);
   pBox->ShowWindow(SW_HIDE);

   CRect boxRect;
   pBox->GetWindowRect(&boxRect);
   ScreenToClient(boxRect);

   VERIFY(m_WsdotHaulingDlg.Create(CWsdotHaulingDlg::IDD, this));
   VERIFY(m_WsdotHaulingDlg.SetWindowPos( GetDlgItem(IDC_STATIC_BOUNDS), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE));//|SWP_NOMOVE));

   VERIFY(m_KdotHaulingDlg.Create(CKdotHaulingDlg::IDD, this));
   VERIFY(m_KdotHaulingDlg.SetWindowPos( GetDlgItem(IDC_STATIC_BOUNDS), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE));//|SWP_NOMOVE));

	CPropertyPage::OnInitDialog();

   m_WsdotHaulingDlg.OnCbnSelchangeWindType();

   m_BeforeInit = false;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecHaulingErectionPage::SwapDialogs()
{
   if (m_HaulingAnalysisMethod == pgsTypes::HaulingAnalysisMethod::WSDOT)
   {
      m_WsdotHaulingDlg.ShowWindow(SW_SHOW);
      m_KdotHaulingDlg.ShowWindow(SW_HIDE);
   }
   else
   {
      m_WsdotHaulingDlg.ShowWindow(SW_HIDE);
      m_KdotHaulingDlg.ShowWindow(SW_SHOW);
   }
}

void CSpecHaulingErectionPage::OnCbnSelchangeHaulingMethod()
{
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_HAULING_METHOD);
   int idx = pBox->GetCurSel();
   m_HaulingAnalysisMethod = (pgsTypes::HaulingAnalysisMethod)idx;

   SwapDialogs();
}

BOOL CSpecHaulingErectionPage::OnSetActive()
{
   // Disable controls if hauling not enabled
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();
   BOOL enableChild = pDad->m_Entry.GetHaulingCriteria().bCheck ? TRUE : FALSE;
   EnableControls(enableChild);

   m_WsdotHaulingDlg.OnSetActive();
   m_KdotHaulingDlg.OnSetActive();

   return CPropertyPage::OnSetActive();
}

inline BOOL CALLBACK EnableChildWindow(HWND hwnd,LPARAM lParam)
{
   ::EnableWindow(hwnd,(int)lParam);
   return TRUE;
}

void CSpecHaulingErectionPage::EnableControls(BOOL bEnable)
{
   EnumChildWindows(GetSafeHwnd(),EnableChildWindow,bEnable);
}
