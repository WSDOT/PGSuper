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

// ConcreteDetailsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include "ConcreteDetailsDlg.h"
#include "HtmlHelp\HelpTopics.hh"
#include <MfcTools\CustomDDX.h>
#include <System\Tokenizer.h>
#include "CopyConcreteEntry.h"
#include <Lrfd\Lrfd.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>

#include <PGSuperColors.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConcreteDetailsDlg dialog


CConcreteDetailsDlg::CConcreteDetailsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConcreteDetailsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConcreteDetailsDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   EAFGetBroker(&m_pBroker);
   GET_IFACE(IBridgeMaterialEx,pMaterial);
   m_MinNWCDensity = pMaterial->GetNWCDensityLimit();
   m_MaxLWCDensity = pMaterial->GetLWCDensityLimit();
}


void CConcreteDetailsDlg::DoDataExchange(CDataExchange* pDX)
{
   m_bErrorInDDX = false;
   try
   {
	   CDialog::DoDataExchange(pDX);
	   //{{AFX_DATA_MAP(CConcreteDetailsDlg)
	   DDX_Control(pDX, IDC_DS_UNIT, m_ctrlStrengthDensityUnit);
	   DDX_Control(pDX, IDC_DS_TITLE, m_ctrlStrengthDensityTitle);
	   //}}AFX_DATA_MAP

      DDX_CBItemData(pDX, IDC_CONCRETE_TYPE, m_Type);

	   DDX_Control(pDX, IDC_EC_K1,   m_ctrlK1);
	   DDX_Control(pDX, IDC_EC_K2,   m_ctrlK2);
	   DDX_Control(pDX, IDC_EC,      m_ctrlEc);
	   DDX_Control(pDX, IDC_MOD_E,   m_ctrlEcCheck);
	   DDX_Control(pDX, IDC_FC,      m_ctrlFc);
	   DDX_Control(pDX, IDC_DS,      m_ctrlStrengthDensity);

      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      DDX_UnitValueAndTag(pDX, IDC_FC, IDC_FC_UNIT, m_Fc, pDisplayUnits->GetStressUnit() );
      DDV_UnitValueGreaterThanZero(pDX,IDC_FC, m_Fc, pDisplayUnits->GetStressUnit() );

      DDX_Check_Bool(pDX, IDC_MOD_E, m_bUserEc);
      if (m_bUserEc || !pDX->m_bSaveAndValidate)
      {
         DDX_UnitValueAndTag(pDX, IDC_EC, IDC_EC_UNIT, m_Ec, pDisplayUnits->GetModEUnit() );
         DDV_UnitValueGreaterThanZero(pDX, IDC_EC, m_Ec, pDisplayUnits->GetModEUnit() );
      }

      DDX_UnitValueAndTag(pDX, IDC_DS, IDC_DS_UNIT, m_Ds, pDisplayUnits->GetDensityUnit() );
      DDV_UnitValueGreaterThanZero(pDX, IDC_DS, m_Ds, pDisplayUnits->GetDensityUnit() );

      DDX_UnitValueAndTag(pDX, IDC_DW, IDC_DW_UNIT, m_Dw, pDisplayUnits->GetDensityUnit() );
      DDV_UnitValueGreaterThanZero(pDX, IDC_DW, m_Dw, pDisplayUnits->GetDensityUnit() );

      // Ds <= Dw
      DDV_UnitValueLimitOrMore(pDX, IDC_DW, m_Dw, m_Ds, pDisplayUnits->GetDensityUnit() );

      DDX_UnitValueAndTag(pDX, IDC_AGG_SIZE, IDC_AGG_SIZE_UNIT, m_AggSize, pDisplayUnits->GetComponentDimUnit() );
      DDV_UnitValueGreaterThanZero(pDX, IDC_AGG_SIZE, m_AggSize, pDisplayUnits->GetComponentDimUnit() );
      
      DDX_Text(pDX, IDC_EC_K1, m_EccK1 );
      DDV_GreaterThanZero(pDX,IDC_EC_K1, m_EccK1);
      
      DDX_Text(pDX, IDC_EC_K2, m_EccK2 );
      DDV_GreaterThanZero(pDX,IDC_EC_K2, m_EccK2);
      
      DDX_Text(pDX, IDC_CREEP_K1, m_CreepK1 );
      DDV_GreaterThanZero(pDX,IDC_CREEP_K1, m_CreepK1);
      
      DDX_Text(pDX, IDC_CREEP_K2, m_CreepK2 );
      DDV_GreaterThanZero(pDX,IDC_CREEP_K2, m_CreepK2);
      
      DDX_Text(pDX, IDC_SHRINKAGE_K1, m_ShrinkageK1 );
      DDV_GreaterThanZero(pDX,IDC_SHRINKAGE_K1, m_ShrinkageK1);
      
      DDX_Text(pDX, IDC_SHRINKAGE_K2, m_ShrinkageK2 );
      DDV_GreaterThanZero(pDX,IDC_SHRINKAGE_K2, m_ShrinkageK2);

      if ( pDX->m_bSaveAndValidate && m_ctrlEcCheck.GetCheck() == 1 )
      {
         m_ctrlEc.GetWindowText(m_strUserEc);
      }

      if (!pDX->m_bSaveAndValidate)
      {
         ShowStrengthDensity(!m_bUserEc);
      }

      DDX_Check_Bool(pDX, IDC_HAS_AGG_STRENGTH, m_bHasFct );
      DDX_UnitValueAndTag(pDX, IDC_AGG_STRENGTH, IDC_AGG_STRENGTH_T, m_Fct, pDisplayUnits->GetStressUnit() );
      if ( m_bHasFct || !pDX->m_bSaveAndValidate )
      {
         if ( !pDX->m_bSaveAndValidate )
         {
            CWnd* pWnd = GetDlgItem(IDC_AGG_STRENGTH);
            pWnd->GetWindowText(m_strFct);
         }
      }
   }
   catch(...)
   {
      m_bErrorInDDX = true;
      throw;
   }
}


BEGIN_MESSAGE_MAP(CConcreteDetailsDlg, CDialog)
	//{{AFX_MSG_MAP(CConcreteDetailsDlg)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	ON_BN_CLICKED(IDC_MOD_E, OnUserEc)
	ON_EN_CHANGE(IDC_FC, OnChangeFc)
	ON_EN_CHANGE(IDC_DS, OnChangeDs)
	ON_EN_CHANGE(IDC_K1, OnChangeK1)
	ON_BN_CLICKED(IDC_COPY_MATERIAL, OnCopyMaterial)
   ON_BN_CLICKED(IDC_HAS_AGG_STRENGTH,OnAggSplittingStrengthClicked)
   ON_CBN_SELCHANGE(IDC_CONCRETE_TYPE,OnConcreteType)
	//}}AFX_MSG_MAP
   ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConcreteDetailsDlg message handlers

BOOL CConcreteDetailsDlg::OnInitDialog() 
{
   CComboBox* pcbConcreteType = (CComboBox*)GetDlgItem(IDC_CONCRETE_TYPE);
   int idx = pcbConcreteType->AddString(_T("Normal weight"));
   pcbConcreteType->SetItemData(idx,(DWORD_PTR)pgsTypes::Normal);

   idx = pcbConcreteType->AddString(_T("All lightweight"));
   pcbConcreteType->SetItemData(idx,(DWORD_PTR)pgsTypes::AllLightweight);

   idx = pcbConcreteType->AddString(_T("Sand lightweight"));
   pcbConcreteType->SetItemData(idx,(DWORD_PTR)pgsTypes::SandLightweight);

	CDialog::OnInitDialog();
	
   BOOL bEnable = m_ctrlEcCheck.GetCheck();
   GetDlgItem(IDC_EC)->EnableWindow(bEnable);
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(bEnable);

   // Don't show the K1 parameter if the code is before 2005
   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEditionWith2005Interims )
   {
       GetDlgItem(IDC_K1_LABEL)->ShowWindow(FALSE);
       GetDlgItem(IDC_K1)->ShowWindow(FALSE);
   }

   OnConcreteType();

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConcreteDetailsDlg::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_CONCRETE_DETAILS );
}

void CConcreteDetailsDlg::OnUserEc()
{
   // Ec check box was clicked
   BOOL bIsChecked = m_ctrlEcCheck.GetCheck();

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

void CConcreteDetailsDlg::ShowStrengthDensity(bool enable)
{
   int code = enable ? SW_SHOW : SW_HIDE;
   m_ctrlStrengthDensity.ShowWindow(code);
   m_ctrlStrengthDensityUnit.ShowWindow(code);
   m_ctrlStrengthDensityTitle.ShowWindow(code);
}

void CConcreteDetailsDlg::OnChangeFc() 
{
   UpdateEc();
}

void CConcreteDetailsDlg::OnChangeDs() 
{
   UpdateEc();
}

void CConcreteDetailsDlg::OnChangeK1() 
{
   UpdateEc();
}

void CConcreteDetailsDlg::UpdateEc()
{
   // update modulus
   if (m_ctrlEcCheck.GetCheck() == 0)
   {
      // need to manually parse strength and density values
      CString strFc, strDensity, strK1, strK2;
      m_ctrlFc.GetWindowText(strFc);
      m_ctrlStrengthDensity.GetWindowText(strDensity);
      m_ctrlK1.GetWindowText(strK1);
      m_ctrlK2.GetWindowText(strK2);

      CString strEc = UpdateEc(strFc,strDensity,strK1,strK2);
      m_ctrlEc.SetWindowText(strEc);
   }
}

CString CConcreteDetailsDlg::UpdateEc(const CString& strFc,const CString& strDensity,const CString& strK1,const CString& strK2)
{
  CString strEc;
   double fc, density, k1,k2;
   double ec = 0;
   if (sysTokenizer::ParseDouble(strFc, &fc) && 
       sysTokenizer::ParseDouble(strDensity,&density) &&
       sysTokenizer::ParseDouble(strK1,&k1) &&
       sysTokenizer::ParseDouble(strK2,&k2) &&
       0 < density && 0 < fc && 0 < k1 && 0 < k2
       )
   {
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

         const unitPressure& stress_unit = pDisplayUnits->GetStressUnit().UnitOfMeasure;
         const unitDensity& density_unit = pDisplayUnits->GetDensityUnit().UnitOfMeasure;

         fc       = ::ConvertToSysUnits(fc,      stress_unit);
         density  = ::ConvertToSysUnits(density, density_unit);

         ec = k1*k2*lrfdConcreteUtil::ModE(fc,density,false);

         strEc.Format(_T("%s"),FormatDimension(ec,pDisplayUnits->GetModEUnit(),false));
   }


   return strEc;
}

void CConcreteDetailsDlg::OnCopyMaterial() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CCopyConcreteEntry dlg(false, this);
   int result = dlg.DoModal();
   if ( result < 0 )
   {
      ::AfxMessageBox(_T("The Concrete library is empty"),MB_OK);
   }
   else if ( result == IDOK )
   {
      const ConcreteLibraryEntry* entry = dlg.m_ConcreteEntry;

      if (entry != NULL)
      {
         m_Fc      = entry->GetFc();
         m_Dw      = entry->GetWeightDensity();
         m_Ds      = entry->GetStrengthDensity();
         m_AggSize = entry->GetAggregateSize();
         m_EccK1   = entry->GetModEK1();
         m_EccK2   = entry->GetModEK2();
         m_CreepK1   = entry->GetCreepK1();
         m_CreepK2   = entry->GetCreepK2();
         m_ShrinkageK1   = entry->GetShrinkageK1();
         m_ShrinkageK2   = entry->GetShrinkageK2();
         m_bUserEc = entry->UserEc();
         m_Ec      = entry->GetEc();
         m_Fct     = entry->GetAggSplittingStrength();
         m_bHasFct = entry->HasAggSplittingStrength();
         m_Type    = entry->GetType();

         CDataExchange dx(this,FALSE);
         DoDataExchange(&dx);
         OnConcreteType();
         OnUserEc();
      }
   }
}

void CConcreteDetailsDlg::OnAggSplittingStrengthClicked()
{
   CButton* pButton = (CButton*)GetDlgItem(IDC_HAS_AGG_STRENGTH);
   ASSERT(pButton);
   BOOL bIsChecked = pButton->GetCheck();
   GetDlgItem(IDC_AGG_STRENGTH)->EnableWindow(bIsChecked);
   GetDlgItem(IDC_AGG_STRENGTH_T)->EnableWindow(bIsChecked);
}

pgsTypes::ConcreteType CConcreteDetailsDlg::GetConreteType()
{
   CComboBox* pcbConcreteType = (CComboBox*)GetDlgItem(IDC_CONCRETE_TYPE);
   pgsTypes::ConcreteType type = (pgsTypes::ConcreteType)pcbConcreteType->GetItemData(pcbConcreteType->GetCurSel());
   return type;
}

void CConcreteDetailsDlg::OnConcreteType()
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

HBRUSH CConcreteDetailsDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
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

void CConcreteDetailsDlg::OnOK()
{
   CDialog::OnOK();

   if ( !m_bErrorInDDX && !IsDensityInRange(m_Ds,m_Type) )
   {
      if (m_Type == pgsTypes::Normal)
         AfxMessageBox(IDS_NWC_MESSAGE,MB_OK | MB_ICONINFORMATION);
      else
         AfxMessageBox(IDS_LWC_MESSAGE,MB_OK | MB_ICONINFORMATION);
   }
}

bool CConcreteDetailsDlg::IsDensityInRange(Float64 density,pgsTypes::ConcreteType type)
{
   if ( type == pgsTypes::Normal )
   {
      return ( IsLE(m_MinNWCDensity,density) );
   }
   else
   {
      return ( IsLE(density,m_MaxLWCDensity) );
   }
}
