///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
//
// KdotHaulingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "KdotHaulingDlg.h"
#include "SpecHaulingErectionPage.h"

// CKdotHaulingDlg dialog

IMPLEMENT_DYNAMIC(CKdotHaulingDlg, CDialog)

CKdotHaulingDlg::CKdotHaulingDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CKdotHaulingDlg::IDD, pParent)
{
}

CKdotHaulingDlg::~CKdotHaulingDlg()
{
}

void CKdotHaulingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CKdotHaulingDlg, CDialog)
   ON_BN_CLICKED(IDC_CHECK_HAULING_TENS_MAX, &CKdotHaulingDlg::OnBnClickedCheckHaulingTensMax)
   ON_BN_CLICKED(IDC_IS_SUPPORT_LESS_THAN, &CKdotHaulingDlg::OnBnClickedIsSupportLessThan)
END_MESSAGE_MAP()


void CKdotHaulingDlg::HideControls(bool hide)
{
   bool enable = !hide;

   // disable all of the windows
   EnumChildWindows(GetSafeHwnd(),CKdotHaulingDlg::EnableWindows,(LPARAM)enable);

   if (enable)
   {
      DoCheckMax();
      DoCheckMinLocation();
   }
}

BOOL CKdotHaulingDlg::EnableWindows(HWND hwnd,LPARAM lParam)
{
   ::EnableWindow(hwnd,(BOOL)lParam);
   return TRUE;
}

void CKdotHaulingDlg::OnBnClickedCheckHaulingTensMax()
{
   DoCheckMax();
}

void CKdotHaulingDlg::DoCheckMax()
{
   if(m_IsHaulingEnabled)
   {
      CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_HAULING_TENS_MAX);
      ASSERT(pchk);
      BOOL ischk = pchk->GetCheck() == BST_CHECKED;

      CWnd* pwnd = GetDlgItem(IDC_HAULING_TENS_MAX);
      ASSERT(pwnd);
      pwnd->EnableWindow(ischk);
      pwnd = GetDlgItem(IDC_HAULING_TENS_MAX_UNITS);
      ASSERT(pwnd);
      pwnd->EnableWindow(ischk);
   }
}

void CKdotHaulingDlg::DoCheckMinLocation()
{
   if(m_IsHaulingEnabled)
   {
      CButton* pchk = (CButton*)GetDlgItem(IDC_IS_SUPPORT_LESS_THAN);
      ASSERT(pchk);
      BOOL ischk = pchk->GetCheck() == BST_CHECKED;

      CWnd* pwnd = GetDlgItem(IDC_SUPPORT_LESS_THAN);
      ASSERT(pwnd);
      pwnd->EnableWindow(ischk);

      pwnd = GetDlgItem(IDC_SUPPORT_LESS_THAN_UNIT);
      ASSERT(pwnd);
      pwnd->EnableWindow(ischk);

   }
}

void CKdotHaulingDlg::OnBnClickedIsSupportLessThan()
{
   DoCheckMinLocation();
}

void CKdotHaulingDlg::OnOK()
{
   // Don't allow enter key to close dialog
   CSpecHaulingErectionPage* parent = (CSpecHaulingErectionPage*)GetParent();
   parent->OnOK();
}

void CKdotHaulingDlg::OnCancel()
{
   CSpecHaulingErectionPage* parent = (CSpecHaulingErectionPage*)GetParent();
   parent->OnCancel();
}
