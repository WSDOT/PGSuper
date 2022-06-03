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
//
// KdotHaulingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "KdotHaulingDlg.h"
#include "SpecHaulingErectionPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CKdotHaulingDlg dialog

IMPLEMENT_DYNAMIC(CKdotHaulingDlg, CDialog)

CKdotHaulingDlg::CKdotHaulingDlg(CWnd* pParent /*=nullptr*/)
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

BOOL CKdotHaulingDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   OnBnClickedCheckHaulingTensMax();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}


BEGIN_MESSAGE_MAP(CKdotHaulingDlg, CDialog)
   ON_BN_CLICKED(IDC_CHECK_HAULING_TENSION_MAX, &CKdotHaulingDlg::OnBnClickedCheckHaulingTensMax)
END_MESSAGE_MAP()

void CKdotHaulingDlg::OnBnClickedCheckHaulingTensMax()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_CHECK_HAULING_TENSION_MAX);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck() == BST_CHECKED;

   CWnd* pwnd = GetDlgItem(IDC_HAULING_TENSION_MAX);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);
   pwnd = GetDlgItem(IDC_HAULING_TENSION_MAX_UNIT);
   ASSERT(pwnd);
   pwnd->EnableWindow(ischk);
}
