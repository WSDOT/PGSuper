///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
// WsdotHaulingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "WsdotHaulingDlg.h"
#include "SpecHaulingErectionPage.h"

// CWsdotHaulingDlg dialog

IMPLEMENT_DYNAMIC(CWsdotHaulingDlg, CDialog)

CWsdotHaulingDlg::CWsdotHaulingDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWsdotHaulingDlg::IDD, pParent)
{

}

CWsdotHaulingDlg::~CWsdotHaulingDlg()
{
}

BOOL CWsdotHaulingDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   OnCheckHaulingTensMax();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CWsdotHaulingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CWsdotHaulingDlg, CDialog)
	ON_BN_CLICKED(IDC_CHECK_HAULING_TENSION_MAX, OnCheckHaulingTensMax)
	ON_BN_CLICKED(IDC_LUMPSUM_METHOD, OnLumpSumMethod)
	ON_BN_CLICKED(IDC_PERAXLE_METHOD, OnPerAxleMethod)
END_MESSAGE_MAP()

// CWsdotHaulingDlg message handlers

void CWsdotHaulingDlg::OnCheckHaulingTensMax()
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

void CWsdotHaulingDlg::OnLumpSumMethod() 
{
   EnableLumpSumMethod(TRUE);
}

void CWsdotHaulingDlg::OnPerAxleMethod() 
{
   EnableLumpSumMethod(FALSE);
}

void CWsdotHaulingDlg::EnableLumpSumMethod(BOOL bEnable)
{
   GetDlgItem(IDC_ROLL_STIFFNESS)->EnableWindow(bEnable);
   GetDlgItem(IDC_ROLL_STIFFNESS_UNITS)->EnableWindow(bEnable);

   GetDlgItem(IDC_AXLE_WEIGHT)->EnableWindow(!bEnable);
   GetDlgItem(IDC_AXLE_WEIGHT_UNITS)->EnableWindow(!bEnable);
   GetDlgItem(IDC_AXLE_STIFFNESS)->EnableWindow(!bEnable);
   GetDlgItem(IDC_AXLE_STIFFNESS_UNITS)->EnableWindow(!bEnable);
   GetDlgItem(IDC_MIN_ROLL_STIFFNESS)->EnableWindow(!bEnable);
   GetDlgItem(IDC_MIN_ROLL_STIFFNESS_UNITS)->EnableWindow(!bEnable);
}
