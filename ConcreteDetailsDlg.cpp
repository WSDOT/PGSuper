///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#include "stdafx.h"
#include "pgsuper.h"
#include "PGSuperUnits.h"
#include "ConcreteDetailsDlg.h"
#include "HtmlHelp\HelpTopics.hh"
#include <MfcTools\CustomDDX.h>
#include <System\Tokenizer.h>
#include "CopyConcreteEntry.h"
#include <Lrfd\Lrfd.h>
#include <IFace\DisplayUnits.h>

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
}


void CConcreteDetailsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConcreteDetailsDlg)
	DDX_Control(pDX, IDC_DS_UNIT, m_ctrlStrengthDensityUnit);
	DDX_Control(pDX, IDC_DS_TITLE, m_ctrlStrengthDensityTitle);
	//}}AFX_DATA_MAP

	DDX_Control(pDX, IDC_K1,      m_ctrlK1);
	DDX_Control(pDX, IDC_EC,      m_ctrlEc);
	DDX_Control(pDX, IDC_MOD_E,   m_ctrlEcCheck);
	DDX_Control(pDX, IDC_FC,      m_ctrlFc);
	DDX_Control(pDX, IDC_DS,      m_ctrlStrengthDensity);

   CComPtr<IBroker> pBroker;
   AfxGetBroker(&pBroker);
   GET_IFACE2(pBroker,IDisplayUnits,pDispUnits);

   DDX_UnitValueAndTag(pDX, IDC_FC, IDC_FC_UNIT, m_Fc, pDispUnits->GetStressUnit() );
   DDV_UnitValueGreaterThanZero(pDX, m_Fc, pDispUnits->GetStressUnit() );

   DDX_Check_Bool(pDX, IDC_MOD_E, m_bUserEc);
   if (m_bUserEc || !pDX->m_bSaveAndValidate)
   {
      DDX_UnitValueAndTag(pDX, IDC_EC, IDC_EC_UNIT, m_Ec, pDispUnits->GetModEUnit() );
      DDV_UnitValueGreaterThanZero(pDX, m_Ec, pDispUnits->GetModEUnit() );
   }

   DDX_UnitValueAndTag(pDX, IDC_DS, IDC_DS_UNIT, m_Ds, pDispUnits->GetDensityUnit() );
   DDV_UnitValueGreaterThanZero(pDX, m_Ds, pDispUnits->GetDensityUnit() );
   DDX_UnitValueAndTag(pDX, IDC_DW, IDC_DW_UNIT, m_Dw, pDispUnits->GetDensityUnit() );
   DDV_UnitValueGreaterThanZero(pDX, m_Dw, pDispUnits->GetDensityUnit() );
   DDX_UnitValueAndTag(pDX, IDC_AGG_SIZE, IDC_AGG_SIZE_UNIT, m_AggSize, pDispUnits->GetComponentDimUnit() );
   DDV_UnitValueGreaterThanZero(pDX, m_AggSize, pDispUnits->GetComponentDimUnit() );
   DDX_Text(pDX, IDC_K1, m_K1 );
   DDV_GreaterThanZero(pDX,m_K1);

   if ( pDX->m_bSaveAndValidate && m_ctrlEcCheck.GetCheck() == 1 )
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
   }

   if (!pDX->m_bSaveAndValidate)
   {
      ShowStrengthDensity(!m_bUserEc);
   }
}


BEGIN_MESSAGE_MAP(CConcreteDetailsDlg, CDialog)
	//{{AFX_MSG_MAP(CConcreteDetailsDlg)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	ON_BN_CLICKED(IDC_MOD_E, OnUserEc)
	ON_EN_CHANGE(IDC_FC, OnChangeSlabFc)
	ON_EN_CHANGE(IDC_DS, OnChangeSlabDs)
	ON_EN_CHANGE(IDC_K1, OnChangeK1)
	ON_BN_CLICKED(IDC_COPY_MATERIAL, OnCopyMaterial)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConcreteDetailsDlg message handlers

BOOL CConcreteDetailsDlg::OnInitDialog() 
{
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
   BOOL bEnable = m_ctrlEcCheck.GetCheck();

   if (bEnable == FALSE)
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
   GetDlgItem(IDC_EC)->EnableWindow(bEnable);
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(bEnable);
}

void CConcreteDetailsDlg::ShowStrengthDensity(bool enable)
{
   int code = enable ? SW_SHOW : SW_HIDE;
   m_ctrlStrengthDensity.ShowWindow(code);
   m_ctrlStrengthDensityUnit.ShowWindow(code);
   m_ctrlStrengthDensityTitle.ShowWindow(code);
}

void CConcreteDetailsDlg::OnChangeSlabFc() 
{
   UpdateEc();
}

void CConcreteDetailsDlg::OnChangeSlabDs() 
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
      CString strFc, strDensity, strK1;
      m_ctrlFc.GetWindowText(strFc);
      m_ctrlStrengthDensity.GetWindowText(strDensity);
      m_ctrlK1.GetWindowText(strK1);

      CString strEc = UpdateEc(strFc,strDensity,strK1);
      m_ctrlEc.SetWindowText(strEc);
   }
}

CString CConcreteDetailsDlg::UpdateEc(const CString& strFc,const CString& strDensity,const CString& strK1)
{
  CString strEc;
   double fc, density, k1;
   double ec = 0;
   if (sysTokenizer::ParseDouble(strFc, &fc) && 
       sysTokenizer::ParseDouble(strDensity,&density) &&
       sysTokenizer::ParseDouble(strK1,&k1) &&
       0 < density && 0 < fc && 0 < k1
       )
   {
         CComPtr<IBroker> pBroker;
         AfxGetBroker(&pBroker);
         GET_IFACE2(pBroker,IDisplayUnits,pDispUnits);

         const unitPressure& stress_unit = pDispUnits->GetStressUnit().UnitOfMeasure;
         const unitDensity& density_unit = pDispUnits->GetDensityUnit().UnitOfMeasure;

         fc       = ::ConvertToSysUnits(fc,      stress_unit);
         density  = ::ConvertToSysUnits(density, density_unit);

         ec = k1*lrfdConcreteUtil::ModE(fc,density,false);

         strEc.Format("%s",FormatDimension(ec,pDispUnits->GetModEUnit(),false));
   }


   return strEc;
}

void CConcreteDetailsDlg::OnCopyMaterial() 
{
	CCopyConcreteEntry dlg(false, this);
   int result = dlg.DoModal();
   if ( result < 0 )
   {
      ::AfxMessageBox("There are no Concrete Material Entries in the library",MB_OK);
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
         m_K1      = entry->GetK1();
         m_bUserEc = entry->UserEc();
         m_Ec      = entry->GetEc();

         CDataExchange dx(this,FALSE);
         DoDataExchange(&dx);
         OnUserEc();
      }
   }
}
