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

// HaulTruckDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "HaulTruckDlg.h"
#include <EAF\EAFApp.h>
#include <EAF\EAFDocument.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CHaulTruckDlg dialog

IMPLEMENT_DYNAMIC(CHaulTruckDlg, CDialog)

CHaulTruckDlg::CHaulTruckDlg(bool allowEditing,CWnd* pParent /*=nullptr*/)
	: CDialog(CHaulTruckDlg::IDD, pParent),
   m_bAllowEditing(allowEditing)
{

}

CHaulTruckDlg::~CHaulTruckDlg()
{
}

void CHaulTruckDlg::DoDataExchange(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   CDialog::DoDataExchange(pDX);

   DDX_Text(pDX,IDC_NAME,m_Name);

   DDX_UnitValueAndTag(pDX,IDC_HBG,IDC_HBG_UNIT,m_Hbg,pDisplayUnits->ComponentDim);
   DDV_GreaterThanZero(pDX,IDC_HBG,m_Hbg);

   DDX_UnitValueAndTag(pDX,IDC_HRC,IDC_HRC_UNIT,m_Hrc,pDisplayUnits->ComponentDim);
   DDV_GreaterThanZero(pDX,IDC_HRC,m_Hrc);

   DDX_UnitValueAndTag(pDX,IDC_WCC,IDC_WCC_UNIT,m_Wcc,pDisplayUnits->ComponentDim);
   DDV_GreaterThanZero(pDX,IDC_WCC,m_Wcc);

   DDX_UnitValueAndTag(pDX,IDC_KTHETA,IDC_KTHETA_UNIT,m_Ktheta,pDisplayUnits->MomentPerAngle);
   DDV_GreaterThanZero(pDX,IDC_KTHETA,m_Ktheta);

   DDX_UnitValueAndTag(pDX,IDC_LMAX,IDC_LMAX_UNIT,m_Lmax,pDisplayUnits->SpanLength);
   DDV_GreaterThanZero(pDX,IDC_LMAX,m_Lmax);

   DDX_UnitValueAndTag(pDX,IDC_OHMAX,IDC_OHMAX_UNIT,m_MaxOH,pDisplayUnits->SpanLength);
   DDV_GreaterThanZero(pDX,IDC_OHMAX,m_MaxOH);

   DDX_UnitValueAndTag(pDX,IDC_WMAX,IDC_WMAX_UNIT,m_MaxWeight,pDisplayUnits->GeneralForce);
   DDV_GreaterThanZero(pDX,IDC_WMAX,m_MaxWeight);
}


BEGIN_MESSAGE_MAP(CHaulTruckDlg, CDialog)
   ON_BN_CLICKED(IDHELP, &CHaulTruckDlg::OnBnClickedHelp)
END_MESSAGE_MAP()


// CHaulTruckDlg message handlers

BOOL CHaulTruckDlg::OnInitDialog() 
{
   // disable OK button if editing not allowed
   CString head;
   GetWindowText(head);
   head += _T(" - ");
   head += m_Name;
	if (!m_bAllowEditing)
   {
      CWnd* pOK = GetDlgItem(IDOK);
      pOK->ShowWindow(SW_HIDE);

      CWnd* pCancel = GetDlgItem(IDCANCEL);
      pCancel->SetWindowText(_T("Close"));

      head += _T(" (Read Only)");
   }
   SetWindowText(head);

   CDialog::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CHaulTruckDlg::OnBnClickedHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_HAUL_TRUCK_DIALOG );
}
