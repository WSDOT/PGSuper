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

#include "stdafx.h"
#include "resource.h"
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

   GET_IFACE(IBridgeMaterial,pMaterial);
   m_MinNWCDensity = pMaterial->GetNWCDensityLimit();
   m_bIsStrengthNWC = true;
   m_bIsDensityNWC = true;
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
   DDX_UnitValueAndTag(pDX, IDC_AGG_SIZE, IDC_AGG_SIZE_UNIT, m_AggSize, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueGreaterThanZero(pDX, IDC_AGG_SIZE, m_AggSize, pDisplayUnits->GetComponentDimUnit() );
   DDX_Text(pDX, IDC_K1, m_K1 );
   DDV_GreaterThanZero(pDX,IDC_K1, m_K1);

   if ( pDX->m_bSaveAndValidate && m_ctrlEcCheck.GetCheck() == 1 )
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
   }

   if (!pDX->m_bSaveAndValidate)
   {
      ShowStrengthDensity(!m_bUserEc);
   }

   if ( m_Ds < m_MinNWCDensity )
   {
      m_bIsStrengthNWC = false;
   }

   if ( m_Dw < m_MinNWCDensity )
   {
      m_bIsDensityNWC = false;
   }
}


BEGIN_MESSAGE_MAP(CConcreteDetailsDlg, CDialog)
	//{{AFX_MSG_MAP(CConcreteDetailsDlg)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	ON_BN_CLICKED(IDC_MOD_E, OnUserEc)
	ON_EN_CHANGE(IDC_FC, OnChangeFc)
	ON_EN_CHANGE(IDC_DS, OnChangeDs)
	ON_EN_CHANGE(IDC_DW, OnChangeDw)
	ON_EN_CHANGE(IDC_K1, OnChangeK1)
	ON_BN_CLICKED(IDC_COPY_MATERIAL, OnCopyMaterial)
	//}}AFX_MSG_MAP
   ON_WM_CTLCOLOR()
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

void CConcreteDetailsDlg::OnChangeFc() 
{
   UpdateEc();
}

void CConcreteDetailsDlg::OnChangeDs() 
{
   UpdateEc();
   CWnd* pWnd = GetDlgItem(IDC_NWC_NOTE);
   pWnd->Invalidate();
}

void CConcreteDetailsDlg::OnChangeDw()
{
   CWnd* pWnd = GetDlgItem(IDC_NWC_NOTE);
   pWnd->Invalidate();
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
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

         const unitPressure& stress_unit = pDisplayUnits->GetStressUnit().UnitOfMeasure;
         const unitDensity& density_unit = pDisplayUnits->GetDensityUnit().UnitOfMeasure;

         fc       = ::ConvertToSysUnits(fc,      stress_unit);
         density  = ::ConvertToSysUnits(density, density_unit);

         ec = k1*lrfdConcreteUtil::ModE(fc,density,false);

         strEc.Format("%s",FormatDimension(ec,pDisplayUnits->GetModEUnit(),false));
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

HBRUSH CConcreteDetailsDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

   // TODO:  Change any attributes of the DC here
   if ( pWnd->GetDlgCtrlID() == IDC_DS && 0 < pWnd->GetWindowTextLength())
   {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      try
      {
         CDataExchange dx(this,TRUE);

         Float64 value;
         DDX_UnitValue(&dx, IDC_DS, value, pDisplayUnits->GetDensityUnit() );

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
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

      try
      {
         CDataExchange dx(this,TRUE);

         Float64 value;
         DDX_UnitValue(&dx, IDC_DW, value, pDisplayUnits->GetDensityUnit() );

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

void CConcreteDetailsDlg::OnOK()
{
   CDialog::OnOK();

   if ( !(m_bIsStrengthNWC && m_bIsDensityNWC) )
      AfxMessageBox(IDS_NWC_MESSAGE,MB_OK | MB_ICONINFORMATION);
}
