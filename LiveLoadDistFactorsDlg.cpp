///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// LiveLoadDistFactorsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "LiveLoadDistFactorsDlg.h"
#include <MFCTools\CustomDDX.h>
#include <..\htmlhelp\helptopics.hh>
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// functions for ordering distribution factor method
inline int GetIntForDfMethod(pgsTypes::DistributionFactorMethod method)
{
   if (method==pgsTypes::Calculated)
   {
      return 0;
   }
   else if (method==pgsTypes::DirectlyInput)
   {
      return 2;
   }
   else if (method==pgsTypes::LeverRule)
   {
      return 1;
   }
   else
   {
      ATLASSERT(0);
      return 0;
   }
}

inline pgsTypes::DistributionFactorMethod GetDfMethodForInt(int method)
{
   if (method==0)
   {
      return pgsTypes::Calculated;
   }
   else if (method==2)
   {
      return pgsTypes::DirectlyInput;
   }
   else if (method==1)
   {
      return pgsTypes::LeverRule;
   }
   else
   {
      ATLASSERT(0);
      return pgsTypes::Calculated;
   }
}

/////////////////////////////////////////////////////////////////////////////
// CLiveLoadDistFactorsDlg dialog


CLiveLoadDistFactorsDlg::CLiveLoadDistFactorsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLiveLoadDistFactorsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLiveLoadDistFactorsDlg)
	//}}AFX_DATA_INIT
}


void CLiveLoadDistFactorsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLiveLoadDistFactorsDlg)
	//}}AFX_DATA_MAP

   // translate df calculation method and action to get correct ordering
   int action, method;
   if ( !pDX->m_bSaveAndValidate )
   {
      action = GetIntForLldfAction(m_LldfRangeOfApplicabilityAction);
      method = GetIntForDfMethod(m_BridgeDesc.GetDistributionFactorMethod());
   }

   DDX_CBIndex(pDX, IDC_ROA_CB, action );
   DDX_Radio(pDX, IDC_LLDF_COMPUTE, method );

   if ( pDX->m_bSaveAndValidate )
   {
      m_LldfRangeOfApplicabilityAction = GetLldfActionForInt(action);

      m_BridgeDesc.SetDistributionFactorMethod(GetDfMethodForInt(method));
   }

   if ( pDX->m_bSaveAndValidate )
   {
      m_StrengthGrid.GetData(pgsTypes::StrengthI,&m_BridgeDesc);
      m_FatigueGrid.GetData(pgsTypes::FatigueI,&m_BridgeDesc);
   }
   else
   {
      m_StrengthGrid.FillGrid(pgsTypes::StrengthI,&m_BridgeDesc);
      m_FatigueGrid.FillGrid(pgsTypes::FatigueI,&m_BridgeDesc);
   }
}
 

BEGIN_MESSAGE_MAP(CLiveLoadDistFactorsDlg, CDialog)
	//{{AFX_MSG_MAP(CLiveLoadDistFactorsDlg)
	ON_BN_CLICKED(IDC_LLDF_COMPUTE, OnMethod)
	ON_BN_CLICKED(IDC_LLDF_INPUT, OnMethod)
	ON_BN_CLICKED(IDC_LLDF_LEVER_RULE, OnMethod)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLiveLoadDistFactorsDlg message handlers

BOOL CLiveLoadDistFactorsDlg::OnInitDialog() 
{
	m_StrengthGrid.SubclassDlgItem(IDC_STRENGTH_GRID, this);
   m_StrengthGrid.CustomInit();

	m_FatigueGrid.SubclassDlgItem(IDC_FATIGUE_GRID, this);
   m_FatigueGrid.CustomInit();

   CDialog::OnInitDialog();
	
   OnMethod();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CLiveLoadDistFactorsDlg::OnMethod() 
{
	BOOL bEnable = IsDlgButtonChecked(IDC_LLDF_INPUT);
   CLLDFGrid* pStrengthGrid = (CLLDFGrid*)GetDlgItem(IDC_STRENGTH_GRID);

   pStrengthGrid->Enable(bEnable);
   //pStrengthGrid->ShowWindow(bEnable ? SW_SHOW : SW_HIDE);

   CLLDFGrid* pFatigueGrid = (CLLDFGrid*)GetDlgItem(IDC_FATIGUE_GRID);

   pFatigueGrid->Enable(bEnable);
   //pFatigueGrid->ShowWindow(bEnable ? SW_SHOW : SW_HIDE);

   //GetDlgItem(IDC_STRENGTH_LABEL)->ShowWindow(bEnable ? SW_SHOW : SW_HIDE);
   //GetDlgItem(IDC_FATIGUE_LABEL)->ShowWindow(bEnable ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_STRENGTH_LABEL)->EnableWindow(bEnable);
   GetDlgItem(IDC_FATIGUE_LABEL)->EnableWindow(bEnable);

   bEnable = IsDlgButtonChecked(IDC_LLDF_COMPUTE);

   //GetDlgItem(IDC_ROA_STATIC)->ShowWindow(bEnable ? SW_SHOW : SW_HIDE);
   //GetDlgItem(IDC_ROA_CB)->ShowWindow(bEnable ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_ROA_STATIC)->EnableWindow(bEnable);
   GetDlgItem(IDC_ROA_CB)->EnableWindow(bEnable);
}

void CLiveLoadDistFactorsDlg::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DISTRIBUTION_FACTORS);
}
