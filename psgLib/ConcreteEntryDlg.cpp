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

   m_MinNWCDensity = ::ConvertToSysUnits(135.0,unitMeasure::LbfPerFeet3);
   m_MaxLWCDensity = ::ConvertToSysUnits(120.0,unitMeasure::LbfPerFeet3);
}


void CConcreteEntryDlg::DoDataExchange(CDataExchange* pDX)
{
   m_bErrorInDDX = false;
   try
   {
      CEAFApp* pApp = EAFGetApp();
      const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

      CDialog::DoDataExchange(pDX);
	   //{{AFX_DATA_MAP(CConcreteEntryDlg)
	   DDX_Text(pDX, IDC_ENTRY_NAME, m_EntryName);
	   //}}AFX_DATA_MAP

      if (pDX->m_bSaveAndValidate)
      {
         if (m_EntryName.IsEmpty())
         {
            AfxMessageBox(_T("Concrete Name cannot be blank"));
            pDX->Fail();
         }
      }

      DDX_CBItemData(pDX, IDC_CONCRETE_TYPE, m_Type);

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

      DDX_Check_Bool(pDX, IDC_HAS_AGG_STRENGTH, m_bHasFct );
      DDX_UnitValueAndTag(pDX, IDC_AGG_STRENGTH, IDC_AGG_STRENGTH_T, m_Fct, pDisplayUnits->Stress );
      if ( m_bHasFct || !pDX->m_bSaveAndValidate )
      {
         if ( !pDX->m_bSaveAndValidate )
         {
            CWnd* pWnd = GetDlgItem(IDC_AGG_STRENGTH);
            pWnd->GetWindowText(m_strFct);
         }
      }


      DDX_UnitValueAndTag(pDX, IDC_DS, IDC_DS_T, m_Ds, pDisplayUnits->Density);
      DDV_UnitValueGreaterThanZero(pDX, IDC_DS, m_Ds, pDisplayUnits->Density );

      DDX_UnitValueAndTag(pDX, IDC_DW, IDC_DW_T, m_Dw, pDisplayUnits->Density );
      DDV_UnitValueGreaterThanZero(pDX, IDC_DW, m_Dw, pDisplayUnits->Density );

      // Ds <= Dw
      DDV_UnitValueLimitOrMore(pDX, IDC_DW, m_Dw, m_Ds, pDisplayUnits->Density );

      DDX_UnitValueAndTag(pDX, IDC_AGG_SIZE, IDC_AGG_SIZE_T, m_AggSize, pDisplayUnits->ComponentDim );
      DDV_UnitValueGreaterThanZero(pDX, IDC_AGG_SIZE, m_AggSize, pDisplayUnits->ComponentDim );

      DDX_Text(pDX, IDC_EC_K1, m_EccK1 );
      DDV_GreaterThanZero(pDX,IDC_EC_K1,m_EccK1);

      DDX_Text(pDX, IDC_EC_K2, m_EccK2 );
      DDV_GreaterThanZero(pDX,IDC_EC_K2,m_EccK2);

      DDX_Text(pDX, IDC_CREEP_K1, m_CreepK1 );
      DDV_GreaterThanZero(pDX,IDC_CREEP_K1,m_CreepK1);

      DDX_Text(pDX, IDC_CREEP_K2, m_CreepK2 );
      DDV_GreaterThanZero(pDX,IDC_CREEP_K2,m_CreepK2);

      DDX_Text(pDX, IDC_SHRINKAGE_K1, m_ShrinkageK1 );
      DDV_GreaterThanZero(pDX,IDC_SHRINKAGE_K1,m_ShrinkageK1);

      DDX_Text(pDX, IDC_SHRINKAGE_K2, m_ShrinkageK2 );
      DDV_GreaterThanZero(pDX,IDC_SHRINKAGE_K2,m_ShrinkageK2);
   }
   catch(...)
   {
      m_bErrorInDDX = true;
      throw;
   }
}


BEGIN_MESSAGE_MAP(CConcreteEntryDlg, CDialog)
	//{{AFX_MSG_MAP(CConcreteEntryDlg)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	ON_BN_CLICKED(IDC_MOD_E, OnModE)
   ON_BN_CLICKED(IDC_HAS_AGG_STRENGTH,OnAggSplittingStrengthClicked)
   ON_CBN_SELCHANGE(IDC_CONCRETE_TYPE,OnConcreteType)
	//}}AFX_MSG_MAP
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
   CComboBox* pcbConcreteType = (CComboBox*)GetDlgItem(IDC_CONCRETE_TYPE);
   int idx = pcbConcreteType->AddString(_T("Normal weight"));
   pcbConcreteType->SetItemData(idx,(DWORD_PTR)pgsTypes::Normal);

   idx = pcbConcreteType->AddString(_T("All lightweight"));
   pcbConcreteType->SetItemData(idx,(DWORD_PTR)pgsTypes::AllLightweight);

   idx = pcbConcreteType->AddString(_T("Sand lightweight"));
   pcbConcreteType->SetItemData(idx,(DWORD_PTR)pgsTypes::SandLightweight);

	CDialog::OnInitDialog();
	
   // disable OK button if editing not allowed
   CString head;
   GetWindowText(head);
   head += _T(" - ");
   head += m_EntryName;
	if (!m_AllowEditing)
   {
      CWnd* pbut = GetDlgItem(IDOK);
      ASSERT(pbut);
      pbut->EnableWindow(m_AllowEditing);
      head += _T(" (Read Only)");
   }
   SetWindowText(head);
	
   OnConcreteType();
   OnModE();
   OnAggSplittingStrengthClicked();

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
      pwnd->SetWindowText(_T("Computed"));
   }
   else
   {
      pwnd->SetWindowText(m_InitialEc);
   }
}

void CConcreteEntryDlg::OnAggSplittingStrengthClicked()
{
   CButton* pButton = (CButton*)GetDlgItem(IDC_HAS_AGG_STRENGTH);
   ASSERT(pButton);
   BOOL bIsChecked = pButton->GetCheck();
   GetDlgItem(IDC_AGG_STRENGTH)->EnableWindow(bIsChecked);
   GetDlgItem(IDC_AGG_STRENGTH_T)->EnableWindow(bIsChecked);
}

pgsTypes::ConcreteType CConcreteEntryDlg::GetConreteType()
{
   CComboBox* pcbConcreteType = (CComboBox*)GetDlgItem(IDC_CONCRETE_TYPE);
   pgsTypes::ConcreteType type = (pgsTypes::ConcreteType)pcbConcreteType->GetItemData(pcbConcreteType->GetCurSel());
   return type;
}

void CConcreteEntryDlg::OnConcreteType()
{
   CComboBox* pcbConcreteType = (CComboBox*)GetDlgItem(IDC_CONCRETE_TYPE);
   pgsTypes::ConcreteType type = (pgsTypes::ConcreteType)pcbConcreteType->GetItemData(pcbConcreteType->GetCurSel());

   BOOL bEnable = (type == pgsTypes::Normal ? FALSE : TRUE);
   GetDlgItem(IDC_HAS_AGG_STRENGTH)->EnableWindow(bEnable);
   GetDlgItem(IDC_AGG_STRENGTH)->EnableWindow(bEnable);
   GetDlgItem(IDC_AGG_STRENGTH_T)->EnableWindow(bEnable);

   GetDlgItem(IDC_DS)->Invalidate();
   GetDlgItem(IDC_DW)->Invalidate();

   if ( bEnable )
      OnAggSplittingStrengthClicked();
}

HBRUSH CConcreteEntryDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

   if ( pWnd->GetDlgCtrlID() == IDC_DS && 0 < pWnd->GetWindowTextLength())
   {
      try
      {
         // if the user enters a number like .125, the first keystroke is ".". DDX_UnitValue calls
         // into MFC DDX_Text which calls AfxTextFloatFormat. This "." does not evaluate to a number
         // so an error message is displayed... this gets the user caught in an infinte loop of
         // pressing the OK button. The only way out is to crash the program.
         //
         // To work around this issue, we need to determine if the value in the field evaluates to
         // a number. If not, assume the density is not consistent with NWC and color the text red
         // otherwise, go on to normal processing.
	      const int TEXT_BUFFER_SIZE = 400;
	      TCHAR szBuffer[TEXT_BUFFER_SIZE];
         ::GetWindowText(GetDlgItem(IDC_DS)->GetSafeHwnd(), szBuffer, _countof(szBuffer));
		   Float64 d;
   		if (_sntscanf_s(szBuffer, _countof(szBuffer), _T("%lf"), &d) != 1)
         {
            pDC->SetTextColor( RED );
         }
         else
         {
            CDataExchange dx(this,TRUE);

            Float64 value;
            DDX_UnitValue(&dx, IDC_DS, value, pDisplayUnits->Density );

            if ( !IsDensityInRange(value,GetConreteType()) )
            {
               pDC->SetTextColor( RED );
            }
         }
      }
      catch(...)
      {
      }
   }

   return hbr;
}

void CConcreteEntryDlg::OnOK()
{
   CDialog::OnOK();

   if ( !m_bErrorInDDX && !IsDensityInRange(m_Ds,m_Type))
   {
      if (m_Type == pgsTypes::Normal)
         AfxMessageBox(IDS_NWC_MESSAGE,MB_OK | MB_ICONINFORMATION);
      else
         AfxMessageBox(IDS_LWC_MESSAGE,MB_OK | MB_ICONINFORMATION);
   }
}

bool CConcreteEntryDlg::IsDensityInRange(Float64 density,pgsTypes::ConcreteType type)
{
   CEAFApp* pApp = EAFGetApp();
   if ( type == pgsTypes::Normal )
   {
      return IsLE(m_MinNWCDensity,density);
   }
   else
   {
      return IsLE(density,m_MaxLWCDensity);
   }
}
