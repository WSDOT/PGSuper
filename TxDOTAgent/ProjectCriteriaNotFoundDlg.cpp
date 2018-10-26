///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
// ProjectCriteriaNotFoundDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ProjectCriteriaNotFoundDlg.h"
#include "HtmlHelp\TogaHelp.hh"


// CProjectCriteriaNotFoundDlg dialog

IMPLEMENT_DYNAMIC(CProjectCriteriaNotFoundDlg, CDialog)

CProjectCriteriaNotFoundDlg::CProjectCriteriaNotFoundDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProjectCriteriaNotFoundDlg::IDD, pParent)
{

}

CProjectCriteriaNotFoundDlg::~CProjectCriteriaNotFoundDlg()
{
}

void CProjectCriteriaNotFoundDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);

   if (pDX->m_bSaveAndValidate)
   {
      CComboBox* ppcl_ctrl = (CComboBox*)GetDlgItem(IDC_PROJECT_CRITERIA);
      int idx = ppcl_ctrl->GetCurSel();
      if (idx!=CB_ERR)
      {
         ppcl_ctrl->GetLBText(idx,m_SelectedProjectCriteriaLibrary);
      }
      else
      {
         ::AfxMessageBox(_T("Please Select an Entry"),MB_OK);
         pDX->PrepareCtrl(IDC_PROJECT_CRITERIA);
         pDX->Fail();
      }
   }
}


BEGIN_MESSAGE_MAP(CProjectCriteriaNotFoundDlg, CDialog)
   ON_BN_CLICKED(IDHELP, &CProjectCriteriaNotFoundDlg::OnHelp)
END_MESSAGE_MAP()


// CProjectCriteriaNotFoundDlg message handlers

BOOL CProjectCriteriaNotFoundDlg::OnInitDialog()
{
   CWnd* pstatic = GetDlgItem(IDC_WARNING_MSG);

   CString msg, stmp;
   stmp.LoadString(IDS_PROJCRIT_ERROR);
   msg.Format(stmp,m_SelectedProjectCriteriaLibrary);
   pstatic->SetWindowText(msg);

   // put library entry names into control
   CComboBox* ppcl_ctrl = (CComboBox*)GetDlgItem(IDC_PROJECT_CRITERIA);
   for (libKeyListIterator it=m_Keys.begin(); it!=m_Keys.end(); it++)
   {
      ppcl_ctrl->AddString(it->c_str());
   }

   ppcl_ctrl->SetCurSel(0); // blindly select first item in list

   CDialog::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CProjectCriteriaNotFoundDlg::OnHelp()
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_NO_PROJ_CRIT );
}
