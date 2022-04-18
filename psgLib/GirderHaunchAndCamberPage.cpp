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

// GirderHaunchAndCamberPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "GirderHaunchAndCamberPage.h"
#include "GirderMainSheet.h"
#include <EAF\EAFDocument.h>
#include <IFace\BeamFactory.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// CGirderHaunchAndCamberPage dialog

IMPLEMENT_DYNAMIC(CGirderHaunchAndCamberPage, CPropertyPage)

CGirderHaunchAndCamberPage::CGirderHaunchAndCamberPage()
	: CPropertyPage(CGirderHaunchAndCamberPage::IDD)
{
}

CGirderHaunchAndCamberPage::~CGirderHaunchAndCamberPage()
{
}

BOOL CGirderHaunchAndCamberPage::OnInitDialog()
{
   CPropertyPage::OnInitDialog();

   // Don't show factoring controls for spliced girders
   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   int sw = pDad->IsSplicedGirder() ? SW_HIDE : SW_SHOW;

   int cntrls[] = {IDC_STATIC_MULT1,IDC_STATIC_MULT2,IDC_STATIC_MULT3,IDC_STATIC_MULT4,IDC_STATIC_MULT5,
                   IDC_STATIC_MULT6,IDC_STATIC_MULT7,IDC_STATIC_MULT8,IDC_STATIC_MULT9,IDC_STATIC_MULT10,
                   IDC_STATIC_MULT11,IDC_ERECTION,IDC_CREEP,IDC_DIAPHRAGM,IDC_DECK_PANEL,IDC_SLAB,IDC_HAUNCH,IDC_BARRIER,666};
   int ic=0;
   while(cntrls[ic] != 666)
   {
      CWnd* pwnd = GetDlgItem(cntrls[ic]);
      ASSERT(pwnd);
      pwnd->ShowWindow(sw); 
      ic++;
   }

   CComPtr<IBeamFactory> factory;
   pDad->m_Entry.GetBeamFactory(&factory);
   if (factory->CanPrecamber())
   {
      GetDlgItem(IDC_PRECAMBER_LIMIT_NOTE)->SetWindowText(_T("Precamber limit"));
   }
   else
   {
      GetDlgItem(IDC_PRECAMBER_LIMIT_NOTE)->SetWindowText(_T("Precamber not supported for this girder type"));
      GetDlgItem(IDC_PRECAMBER_LIMIT_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_PRECAMBER_LIMIT)->ShowWindow(SW_HIDE);
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}



void CGirderHaunchAndCamberPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeHaunchData(pDX);

   if (!pDX->m_bSaveAndValidate)
   {
      OnBnClickedCheckCl();
   }
}


BEGIN_MESSAGE_MAP(CGirderHaunchAndCamberPage, CPropertyPage)
   ON_BN_CLICKED(IDC_CHECK_CL, &CGirderHaunchAndCamberPage::OnBnClickedCheckCl)
	ON_BN_CLICKED(ID_HELP,OnHelp)
END_MESSAGE_MAP()


// CGirderHaunchAndCamberPage message handlers

void CGirderHaunchAndCamberPage::OnBnClickedCheckCl()
{
   BOOL bEnable = IsDlgButtonChecked(IDC_CHECK_CL);
   GetDlgItem(IDC_MIN_HAUNCH_BC)->EnableWindow(bEnable);
   GetDlgItem(IDC_MIN_HAUNCH_BC_UNIT)->EnableWindow(bEnable);
   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
}

void CGirderHaunchAndCamberPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDER_HAUNCH_AND_CAMBER);
}