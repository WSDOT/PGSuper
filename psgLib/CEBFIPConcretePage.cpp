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
// ACIConcretePage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "ConcreteEntryDlg.h"
#include "CEBFIPConcretePage.h"
#include "..\htmlhelp\helptopics.hh"

#include <MFCTools\CustomDDX.h>
#include <Material\CEBFIPConcrete.h>
#include <EAF\EAFApp.h>

// CCEBFIPConcretePage dialog

IMPLEMENT_DYNAMIC(CCEBFIPConcretePage, CPropertyPage)

CCEBFIPConcretePage::CCEBFIPConcretePage() : CPropertyPage(IDD_CEBFIP_CONCRETE)
{

}

CCEBFIPConcretePage::~CCEBFIPConcretePage()
{
}

void CCEBFIPConcretePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   DDX_CBItemData(pDX,IDC_CEMENT_TYPE,m_CementType);
}


BEGIN_MESSAGE_MAP(CCEBFIPConcretePage, CPropertyPage)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()


// CCEBFIPConcretePage message handlers

BOOL CCEBFIPConcretePage::OnInitDialog() 
{
   CComboBox* pcbCementType = (CComboBox*)GetDlgItem(IDC_CEMENT_TYPE);
   pcbCementType->SetItemData(pcbCementType->AddString(_T("Rapid Hardening, High Strength Cemetn (RS)")),pgsTypes::RS);
   pcbCementType->SetItemData(pcbCementType->AddString(_T("Normal Hardening Cement (N)")),pgsTypes::N);
   pcbCementType->SetItemData(pcbCementType->AddString(_T("Rapid Hardening Cement (R)")),pgsTypes::R);
   pcbCementType->SetItemData(pcbCementType->AddString(_T("Slowly Hardening Cement (SL)")),pgsTypes::SL);

	CPropertyPage::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CCEBFIPConcretePage::OnCommandHelp(WPARAM, LPARAM lParam)
{
#pragma Reminder("UPDATE: Update help file reference for this topic")
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_CONCRETE_ENTRY_DIALOG );
   return TRUE;
}
