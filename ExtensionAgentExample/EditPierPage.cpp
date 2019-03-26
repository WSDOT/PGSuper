///////////////////////////////////////////////////////////////////////
// ExtensionAgentExample - Extension Agent Example Project for PGSuper
// Copyright © 1999-2019  Washington State Department of Transportation
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

// EditPierPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "EditPierPage.h"

#include <MFCTools\CustomDDX.h>
#include <IFace\ExtendUI.h>


// CEditPierPage dialog

IMPLEMENT_DYNAMIC(CEditPierPage, CPropertyPage)

CEditPierPage::CEditPierPage(IEditPierData* pEditPierData)
	: CPropertyPage(CEditPierPage::IDD)
{
   m_pEditPierData = pEditPierData;
   m_bCheck = true;

   m_psp.dwFlags |= PSP_HASHELP;
}

CEditPierPage::~CEditPierPage()
{
}

void CEditPierPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

   DDX_Check_Bool(pDX,IDC_CHECK,m_bCheck);
}


BEGIN_MESSAGE_MAP(CEditPierPage, CPropertyPage)
   ON_BN_CLICKED(IDC_CHECK, &CEditPierPage::OnBnClickedCheck)
END_MESSAGE_MAP()


// CEditPierPage message handlers

void CEditPierPage::OnBnClickedCheck()
{
   // TODO: Add your control notification handler code here
   AfxMessageBox(_T("Clicked"));
}

BOOL CEditPierPage::OnSetActive()
{
   // TODO: Add your specialized code here and/or call the base class
   CString strPierData;
   strPierData.Format(_T("# Girders Back = %d\n# Girders Ahead = %d"),m_pEditPierData->GetGirderCount(pgsTypes::Back),m_pEditPierData->GetGirderCount(pgsTypes::Ahead));
   GetDlgItem(IDC_PIER_DATA)->SetWindowText(strPierData);

   return CPropertyPage::OnSetActive();
}
