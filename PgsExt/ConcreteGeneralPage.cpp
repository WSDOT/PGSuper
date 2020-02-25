///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

// ConcreteGeneralPage.cpp : implementation file
//

#include <PgsExt\PgsExtLib.h>
#include "resource.h"
#include <PgsExt\ConcreteGeneralPage.h>
#include <PgsExt\ConcreteDetailsDlg.h>

#include "CopyConcreteEntry.h"
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#include <PGSuperColors.h>
#include "..\Documentation\PGSuper.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConcreteGeneralPage dialog


CConcreteGeneralPage::CConcreteGeneralPage() : CPropertyPage()
{
	//{{AFX_DATA_INIT(CConcreteGeneralPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   Construct(IDD_CONCRETE_DETAILS);

   m_MinNWCDensity = lrfdConcreteUtil::GetNWCDensityLimit();
   m_MaxLWCDensity = lrfdConcreteUtil::GetLWCDensityLimit();
}


void CConcreteGeneralPage::DoDataExchange(CDataExchange* pDX)
{
   m_bErrorInDDX = false;
   try
   {
	   CPropertyPage::DoDataExchange(pDX);
	   //{{AFX_DATA_MAP(CConcreteDetailsDlg)
	   DDX_Control(pDX, IDC_DS_UNIT, m_ctrlStrengthDensityUnit);
	   DDX_Control(pDX, IDC_DS_TITLE, m_ctrlStrengthDensityTitle);
	   //}}AFX_DATA_MAP

      DDX_CBItemData(pDX, IDC_CONCRETE_TYPE, m_Type);

	   DDX_Control(pDX, IDC_EC,      m_ctrlEc);
	   DDX_Control(pDX, IDC_MOD_E,   m_ctrlEcCheck);
	   DDX_Control(pDX, IDC_FC,      m_ctrlFc);
	   DDX_Control(pDX, IDC_DS,      m_ctrlStrengthDensity);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CConcreteDetailsDlg* pParent = (CConcreteDetailsDlg*)GetParent();
      if ( pParent->m_bFinalProperties )
      {
         DDX_UnitValueAndTag(pDX, IDC_FC, IDC_FC_UNIT, pParent->m_fc28, pDisplayUnits->GetStressUnit() );
         DDV_UnitValueGreaterThanZero(pDX,IDC_FC, pParent->m_fc28, pDisplayUnits->GetStressUnit() );

         DDX_Check_Bool(pDX, IDC_MOD_E, pParent->m_bUserEc28);
         if (pParent->m_bUserEc28 || !pDX->m_bSaveAndValidate)
         {
            DDX_UnitValueAndTag(pDX, IDC_EC, IDC_EC_UNIT, pParent->m_Ec28, pDisplayUnits->GetModEUnit() );
            DDV_UnitValueGreaterThanZero(pDX, IDC_EC, pParent->m_Ec28, pDisplayUnits->GetModEUnit() );
         }
      }
      else
      {
         DDX_UnitValueAndTag(pDX, IDC_FC, IDC_FC_UNIT, pParent->m_fci, pDisplayUnits->GetStressUnit() );
         DDV_UnitValueGreaterThanZero(pDX,IDC_FC, pParent->m_fci, pDisplayUnits->GetStressUnit() );

         DDX_Check_Bool(pDX, IDC_MOD_E, pParent->m_bUserEci);
         if (pParent->m_bUserEci || !pDX->m_bSaveAndValidate)
         {
            DDX_UnitValueAndTag(pDX, IDC_EC, IDC_EC_UNIT, pParent->m_Eci, pDisplayUnits->GetModEUnit() );
            DDV_UnitValueGreaterThanZero(pDX, IDC_EC, pParent->m_Eci, pDisplayUnits->GetModEUnit() );
         }
      }

      DDX_UnitValueAndTag(pDX, IDC_DS, IDC_DS_UNIT, m_Ds, pDisplayUnits->GetDensityUnit() );
      DDV_UnitValueGreaterThanZero(pDX, IDC_DS, m_Ds, pDisplayUnits->GetDensityUnit() );

      DDX_UnitValueAndTag(pDX, IDC_DW, IDC_DW_UNIT, m_Dw, pDisplayUnits->GetDensityUnit() );
      DDV_UnitValueGreaterThanZero(pDX, IDC_DW, m_Dw, pDisplayUnits->GetDensityUnit() );

      // Ds <= Dw
      if (m_ctrlEcCheck.GetCheck() == BST_UNCHECKED)
      {
         DDV_UnitValueLimitOrMore(pDX, IDC_DW, m_Dw, m_Ds, pDisplayUnits->GetDensityUnit());
      }

      DDX_UnitValueAndTag(pDX, IDC_AGG_SIZE, IDC_AGG_SIZE_UNIT, m_AggSize, pDisplayUnits->GetComponentDimUnit() );
      DDV_UnitValueGreaterThanZero(pDX, IDC_AGG_SIZE, m_AggSize, pDisplayUnits->GetComponentDimUnit() );
      
      if ( pDX->m_bSaveAndValidate && m_ctrlEcCheck.GetCheck() == BST_CHECKED )
      {
         m_ctrlEc.GetWindowText(m_strUserEc);
      }

      if (!pDX->m_bSaveAndValidate)
      {
         if ( pParent->m_bFinalProperties )
         {
            ShowStrengthDensity(!pParent->m_bUserEc28);
         }
         else
         {
            ShowStrengthDensity(!pParent->m_bUserEci);
         }
      }
   }
   catch(...)
   {
      m_bErrorInDDX = true;
      throw;
   }
}


BEGIN_MESSAGE_MAP(CConcreteGeneralPage, CPropertyPage)
	//{{AFX_MSG_MAP(CConcreteGeneralPage)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	ON_BN_CLICKED(IDC_MOD_E, OnUserEc)
	ON_EN_CHANGE(IDC_FC, OnChangeFc)
	ON_EN_CHANGE(IDC_DS, OnChangeDs)
	ON_BN_CLICKED(IDC_COPY_MATERIAL, OnCopyMaterial)
   ON_CBN_SELCHANGE(IDC_CONCRETE_TYPE,OnConcreteType)
	//}}AFX_MSG_MAP
   ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConcreteDetailsDlg message handlers

BOOL CConcreteGeneralPage::OnInitDialog() 
{
   CConcreteDetailsDlg* pParent = (CConcreteDetailsDlg*)GetParent();
   if ( pParent->m_bFinalProperties )
   {
      GetDlgItem(IDC_FC_LABEL)->SetWindowText(_T("Strength - f'c"));
      GetDlgItem(IDC_DS_TITLE)->SetWindowText(_T("Unit Weight (used to compute Ec)"));
      GetDlgItem(IDC_MOD_E)->SetWindowText(_T("Mod. Elasticity, Ec"));
   }
   else
   {
      GetDlgItem(IDC_FC_LABEL)->SetWindowText(_T("Strength - f'ci"));
      GetDlgItem(IDC_DS_TITLE)->SetWindowText(_T("Unit Weight (used to compute Eci)"));
      GetDlgItem(IDC_MOD_E)->SetWindowText(_T("Mod. Elasticity, Eci"));
   }

   CComboBox* pcbConcreteType = (CComboBox*)GetDlgItem(IDC_CONCRETE_TYPE);
   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SeventhEditionWith2016Interims )
   {  
      int idx = pcbConcreteType->AddString(_T("Normal weight"));
      pcbConcreteType->SetItemData(idx,(DWORD_PTR)pgsTypes::Normal);

      idx = pcbConcreteType->AddString(_T("All lightweight"));
      pcbConcreteType->SetItemData(idx,(DWORD_PTR)pgsTypes::AllLightweight);

      idx = pcbConcreteType->AddString(_T("Sand lightweight"));
      pcbConcreteType->SetItemData(idx, (DWORD_PTR)pgsTypes::SandLightweight);

      idx = pcbConcreteType->AddString(_T("UHPC"));
      pcbConcreteType->SetItemData(idx, (DWORD_PTR)pgsTypes::UHPC);
   }
   else
   {
      int idx = pcbConcreteType->AddString(_T("Normal weight"));
      pcbConcreteType->SetItemData(idx,(DWORD_PTR)pgsTypes::Normal);

      idx = pcbConcreteType->AddString(_T("Lightweight"));
      pcbConcreteType->SetItemData(idx,(DWORD_PTR)pgsTypes::SandLightweight);

      idx = pcbConcreteType->AddString(_T("UHPC"));
      pcbConcreteType->SetItemData(idx, (DWORD_PTR)pgsTypes::UHPC);

      ATLASSERT( m_Type == pgsTypes::Normal || m_Type == pgsTypes::SandLightweight || m_Type == pgsTypes::UHPC );
   }

	CPropertyPage::OnInitDialog();
	
   BOOL bEnable = m_ctrlEcCheck.GetCheck() == BST_CHECKED ? TRUE : FALSE;
   GetDlgItem(IDC_EC)->EnableWindow(bEnable);
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(bEnable);

   OnConcreteType();

   if ( !pParent->m_bEnableCopyFromLibrary )
   {
      GetDlgItem(IDC_COPY_MATERIAL)->ShowWindow(SW_HIDE);
   }

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConcreteGeneralPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_CONCRETE_GENERAL );
}

void CConcreteGeneralPage::OnUserEc()
{
   // Ec check box was clicked
   BOOL bIsChecked = m_ctrlEcCheck.GetCheck() == BST_CHECKED ? TRUE : FALSE;

   if (bIsChecked == FALSE)
   {
      // unchecked... compute Ec based on parameters
      m_ctrlEc.GetWindowText(m_strUserEc);
      UpdateEc();
      ShowStrengthDensity(true);
   }
   else
   {
      // check, restore user input value
      m_ctrlEc.SetWindowText(m_strUserEc);
      ShowStrengthDensity(false);
   }

   // enable/disable the Ec input and unit controls
   GetDlgItem(IDC_EC)->EnableWindow(bIsChecked);
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(bIsChecked);
}

void CConcreteGeneralPage::ShowStrengthDensity(bool enable)
{
   int code = enable ? SW_SHOW : SW_HIDE;
   m_ctrlStrengthDensity.ShowWindow(code);
   m_ctrlStrengthDensityUnit.ShowWindow(code);
   m_ctrlStrengthDensityTitle.ShowWindow(code);
}

void CConcreteGeneralPage::OnChangeFc() 
{
   UpdateEc();
}

void CConcreteGeneralPage::OnChangeDs() 
{
   UpdateEc();
}

void CConcreteGeneralPage::UpdateEc()
{
   // update modulus
   if (m_ctrlEcCheck.GetCheck() == BST_UNCHECKED)
   {
      // need to manually parse strength and density values
      CString strFc, strDensity, strK1, strK2;
      m_ctrlFc.GetWindowText(strFc);
      m_ctrlStrengthDensity.GetWindowText(strDensity);

      CConcreteDetailsDlg* pParent = (CConcreteDetailsDlg*)GetParent();

      strK1.Format(_T("%f"),pParent->m_AASHTO.m_EccK1);
      strK2.Format(_T("%f"),pParent->m_AASHTO.m_EccK2);

      CString strEc = CConcreteDetailsDlg::UpdateEc(strFc,strDensity,strK1,strK2);
      m_ctrlEc.SetWindowText(strEc);
   }
}

void CConcreteGeneralPage::OnCopyMaterial() 
{
   INT_PTR result;
   std::unique_ptr<CCopyConcreteEntry> pDlg;

   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      pDlg = std::make_unique<CCopyConcreteEntry>(false, this);
      result = pDlg->DoModal();
   }

   if ( result < 0 )
   {
      ::AfxMessageBox(_T("The Concrete library is empty"),MB_OK);
   }
   else if ( result == IDOK )
   {
      const ConcreteLibraryEntry* entry = pDlg->m_ConcreteEntry;

      if (entry != nullptr)
      {
         CConcreteDetailsDlg* pParent = (CConcreteDetailsDlg*)GetParent();

         if ( pParent->m_bFinalProperties )
         {
            pParent->m_fc28 = entry->GetFc();
            pParent->m_Ec28 = entry->GetEc();
            pParent->m_bUserEc28 = entry->UserEc();
         }
         else
         {
            pParent->m_fci = entry->GetFc();
            pParent->m_Eci = entry->GetEc();
         }

         m_Dw      = entry->GetWeightDensity();
         m_Ds      = entry->GetStrengthDensity();
         m_AggSize = entry->GetAggregateSize();
         m_Type    = entry->GetType();

         pParent->m_AASHTO.m_EccK1       = entry->GetModEK1();
         pParent->m_AASHTO.m_EccK2       = entry->GetModEK2();
         pParent->m_AASHTO.m_CreepK1     = entry->GetCreepK1();
         pParent->m_AASHTO.m_CreepK2     = entry->GetCreepK2();
         pParent->m_AASHTO.m_ShrinkageK1 = entry->GetShrinkageK1();
         pParent->m_AASHTO.m_ShrinkageK2 = entry->GetShrinkageK2();
         pParent->m_AASHTO.m_Fct         = entry->GetAggSplittingStrength();
         pParent->m_AASHTO.m_bHasFct     = entry->HasAggSplittingStrength();

         pParent->m_ACI.m_bUserParameters = entry->UserACIParameters();
         pParent->m_ACI.m_A               = entry->GetAlpha();
         pParent->m_ACI.m_B               = entry->GetBeta();
         pParent->m_ACI.m_CureMethod      = entry->GetCureMethod();
         pParent->m_ACI.m_CementType      = entry->GetACI209CementType();

         pParent->m_CEBFIP.m_CementType = entry->GetCEBFIPCementType();

         CDataExchange dx(this,FALSE);
         DoDataExchange(&dx);
         OnConcreteType();
         OnUserEc();
      }
   }
}

pgsTypes::ConcreteType CConcreteGeneralPage::GetConreteType()
{
   CComboBox* pcbConcreteType = (CComboBox*)GetDlgItem(IDC_CONCRETE_TYPE);
   pgsTypes::ConcreteType type = (pgsTypes::ConcreteType)pcbConcreteType->GetItemData(pcbConcreteType->GetCurSel());
   return type;
}

void CConcreteGeneralPage::OnConcreteType()
{
   CComboBox* pcbConcreteType = (CComboBox*)GetDlgItem(IDC_CONCRETE_TYPE);
   pgsTypes::ConcreteType type = (pgsTypes::ConcreteType)pcbConcreteType->GetItemData(pcbConcreteType->GetCurSel());

   GetDlgItem(IDC_DS)->Invalidate();
   GetDlgItem(IDC_DW)->Invalidate();
}

HBRUSH CConcreteGeneralPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
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

            CComPtr<IBroker> pBroker;
            EAFGetBroker(&pBroker);
            GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
            Float64 value;
            DDX_UnitValue(&dx, IDC_DS, value, pDisplayUnits->GetDensityUnit() );

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

void CConcreteGeneralPage::OnOK()
{
   CPropertyPage::OnOK();

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   if ( !m_bErrorInDDX && !IsDensityInRange(m_Ds,m_Type) )
   {
      AfxMessageBox(m_Type == pgsTypes::Normal ? IDS_NWC_MESSAGE : IDS_LWC_MESSAGE,MB_OK | MB_ICONINFORMATION);
   }
}

bool CConcreteGeneralPage::IsDensityInRange(Float64 density,pgsTypes::ConcreteType type)
{
   if ( type == pgsTypes::Normal || type == pgsTypes::UHPC)
   {
      return ( IsLE(m_MinNWCDensity,density) );
   }
   else
   {
      return ( IsLE(density,m_MaxLWCDensity) );
   }
}
