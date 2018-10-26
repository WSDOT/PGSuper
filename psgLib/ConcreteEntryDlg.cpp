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

// ConcreteEntryDlg.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "ConcreteEntryDlg.h"
#include <MfcTools\CustomDDX.h>
#include <Colors.h>

#include <EAF\EAFApp.h>

#include "..\htmlhelp\helptopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConcreteEntryDlg dialog


CConcreteEntryDlg::CConcreteEntryDlg(bool allowEditing, CWnd* pParent /*=NULL*/)
	: CDialog(CConcreteEntryDlg::IDD, pParent),
   m_AllowEditing(allowEditing)
{
	//{{AFX_DATA_INIT(CConcreteEntryDlg)
	m_EntryName = _T("");
	//}}AFX_DATA_INIT
   CEAFApp* pApp;
   {
      AFX_MANAGE_STATE(AfxGetAppModuleState());
      pApp = (CEAFApp*)AfxGetApp();
   }

   m_MinNWCDensity = pApp->GetUnitsMode() == eafTypes::umUS ? ::ConvertToSysUnits(135.0,unitMeasure::LbfPerFeet3) : ::ConvertToSysUnits(2150.,unitMeasure::KgPerMeter3);
   m_bIsStrengthNWC = true;
   m_bIsDensityNWC = true;
}


void CConcreteEntryDlg::DoDataExchange(CDataExchange* pDX)
{
   CEAFApp* pApp;
   {
      AFX_MANAGE_STATE(AfxGetAppModuleState());
      pApp = (CEAFApp*)AfxGetApp();
   }
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConcreteEntryDlg)
	DDX_Text(pDX, IDC_ENTRY_NAME, m_EntryName);
	//}}AFX_DATA_MAP

   if (pDX->m_bSaveAndValidate)
   {
      if (m_EntryName.IsEmpty())
      {
         AfxMessageBox("Concrete Name cannot be blank");
         pDX->Fail();
      }
   }

   DDX_UnitValueAndTag(pDX, IDC_FC, IDC_FC_T, m_Fc, pDisplayUnits->Stress );
   DDV_UnitValueGreaterThanZero(pDX, IDC_FC, m_Fc, pDisplayUnits->Stress );

   DDX_Check_Bool(pDX, IDC_MOD_E, m_bUserEc);
   if (m_bUserEc || !pDX->m_bSaveAndValidate)
   {
      DDX_UnitValueAndTag(pDX, IDC_EC, IDC_EC_T, m_Ec, pDisplayUnits->Stress );
      DDV_UnitValueGreaterThanZero(pDX, IDC_EC, m_Ec, pDisplayUnits->Stress );

      if (!pDX->m_bSaveAndValidate)
      {
         CWnd* pwnd = (CWnd*)GetDlgItem(IDC_EC);
         pwnd->GetWindowText(m_InitialEc);
      }
   }

   DDX_UnitValueAndTag(pDX, IDC_DS, IDC_DS_T, m_Ds, pDisplayUnits->Density);
   DDV_UnitValueGreaterThanZero(pDX, IDC_DS, m_Ds, pDisplayUnits->Density );
   DDX_UnitValueAndTag(pDX, IDC_DW, IDC_DW_T, m_Dw, pDisplayUnits->Density );
   DDV_UnitValueGreaterThanZero(pDX, IDC_DW, m_Dw, pDisplayUnits->Density );
   DDX_UnitValueAndTag(pDX, IDC_AGG_SIZE, IDC_AGG_SIZE_T, m_AggSize, pDisplayUnits->ComponentDim );
   DDV_UnitValueGreaterThanZero(pDX, IDC_AGG_SIZE, m_AggSize, pDisplayUnits->ComponentDim );
   DDX_Text(pDX, IDC_K1, m_K1 );
   DDV_GreaterThanZero(pDX,IDC_K1,m_K1);

   if ( m_Ds < m_MinNWCDensity )
   {
      m_bIsStrengthNWC = false;
   }

   if ( m_Dw < m_MinNWCDensity )
   {
      m_bIsDensityNWC = false;
   }
}


BEGIN_MESSAGE_MAP(CConcreteEntryDlg, CDialog)
	//{{AFX_MSG_MAP(CConcreteEntryDlg)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	ON_BN_CLICKED(IDC_MOD_E, OnModE)
	//}}AFX_MSG_MAP
   ON_EN_CHANGE(IDC_DS, &CConcreteEntryDlg::OnChangeDs)
   ON_EN_CHANGE(IDC_DW, &CConcreteEntryDlg::OnChangeDw)
   ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConcreteEntryDlg message handlers
LRESULT CConcreteEntryDlg::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_CONCRETE_ENTRY_DIALOG );
   return TRUE;
}


BOOL CConcreteEntryDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   // disable OK button if editing not allowed
   CString head;
   GetWindowText(head);
   head += " - ";
   head += m_EntryName;
	if (!m_AllowEditing)
   {
      CWnd* pbut = GetDlgItem(IDOK);
      ASSERT(pbut);
      pbut->EnableWindow(m_AllowEditing);
      head += " (Read Only)";
   }
   SetWindowText(head);
	
   OnModE();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CConcreteEntryDlg::OnModE()
{
   CButton* pchk = (CButton*)GetDlgItem(IDC_MOD_E);
   ASSERT(pchk);
   BOOL ischk = pchk->GetCheck();

   GetDlgItem(IDC_EC)->EnableWindow(ischk);
   GetDlgItem(IDC_EC_T)->EnableWindow(ischk);

   CWnd* pwnd = (CWnd*)GetDlgItem(IDC_EC);
   if (!ischk)
   {
      pwnd->GetWindowText(m_InitialEc);
      pwnd->SetWindowText("Computed");
   }
   else
   {
      pwnd->SetWindowText(m_InitialEc);
   }
}

void CConcreteEntryDlg::OnChangeDs()
{
   CWnd* pWnd = GetDlgItem(IDC_NWC_NOTE);
   pWnd->Invalidate();
}

void CConcreteEntryDlg::OnChangeDw()
{
   CWnd* pWnd = GetDlgItem(IDC_NWC_NOTE);
   pWnd->Invalidate();
}

HBRUSH CConcreteEntryDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   CEAFApp* pApp;
   {
      AFX_MANAGE_STATE(AfxGetAppModuleState());
      pApp = (CEAFApp*)AfxGetApp();
   }
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

   if ( pWnd->GetDlgCtrlID() == IDC_DS && 0 < pWnd->GetWindowTextLength())
   {
      try
      {
         CDataExchange dx(this,TRUE);

         Float64 value;
         DDX_UnitValue(&dx, IDC_DS, value, pDisplayUnits->Density );

         if (value < m_MinNWCDensity )
         {
            m_bIsStrengthNWC = false;
            pDC->SetTextColor( RED );
         }
         else
         {
            m_bIsStrengthNWC = true;
         }
      }
      catch(...)
      {
      }
   }
   else if ( pWnd->GetDlgCtrlID() == IDC_DW && 0 < pWnd->GetWindowTextLength() )
   {
      try
      {
         CDataExchange dx(this,TRUE);

         Float64 value;
         DDX_UnitValue(&dx, IDC_DW, value, pDisplayUnits->Density);

         if (value < m_MinNWCDensity )
         {
            m_bIsDensityNWC = false;
            pDC->SetTextColor( RED );
         }
         else
         {
            m_bIsDensityNWC = true;
         }
      }
      catch(...)
      {
      }
   }
   else if ( pWnd->GetDlgCtrlID() == IDC_NWC_NOTE )
   {
      if ( !(m_bIsStrengthNWC && m_bIsDensityNWC) )
         pDC->SetTextColor( RED );
   }

   return hbr;
}

void CConcreteEntryDlg::OnOK()
{
   CDialog::OnOK();

   if ( !(m_bIsStrengthNWC && m_bIsDensityNWC) )
      AfxMessageBox(IDS_NWC_MESSAGE,MB_OK | MB_ICONINFORMATION);
}
