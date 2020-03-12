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

// ConcreteEntryGeneralPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "ConcreteEntryDlg.h"
#include "ConcreteEntryGeneralPage.h"
#include <MfcTools\CustomDDX.h>
#include <Colors.h>

#include <EAF\EAFApp.h>
#include <EAF\EAFDocument.h>

#include <Lrfd\Concreteutil.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConcreteEntryDlg dialog


CConcreteEntryGeneralPage::CConcreteEntryGeneralPage(): CPropertyPage(IDD_CONCRETE_GENERAL)
{
	//{{AFX_DATA_INIT(CConcreteEntryDlg)
	//}}AFX_DATA_INIT
   m_EntryName = _T("");

   m_MinNWCDensity = lrfdConcreteUtil::GetNWCDensityLimit();
   m_MaxLWCDensity = lrfdConcreteUtil::GetLWCDensityLimit();

   lrfdConcreteUtil::GetUHPCStrengthRange(&m_MinFcUHPC, &m_MaxFcUHPC);
}


void CConcreteEntryGeneralPage::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   m_bErrorInDDX = false;
   try
   {
      CEAFApp* pApp = EAFGetApp();
      const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

      CPropertyPage::DoDataExchange(pDX);

	   //{{AFX_DATA_MAP(CConcreteEntryGeneralPage)
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

      DDX_UnitValueAndTag(pDX, IDC_DS, IDC_DS_T, m_Ds, pDisplayUnits->Density);
      DDV_UnitValueGreaterThanZero(pDX, IDC_DS, m_Ds, pDisplayUnits->Density );

      DDX_UnitValueAndTag(pDX, IDC_DW, IDC_DW_T, m_Dw, pDisplayUnits->Density );
      DDV_UnitValueGreaterThanZero(pDX, IDC_DW, m_Dw, pDisplayUnits->Density );

      // Ds <= Dw
      DDV_UnitValueLimitOrMore(pDX, IDC_DW, m_Dw, m_Ds, pDisplayUnits->Density );

      DDX_UnitValueAndTag(pDX, IDC_AGG_SIZE, IDC_AGG_SIZE_T, m_AggSize, pDisplayUnits->ComponentDim );
      DDV_UnitValueGreaterThanZero(pDX, IDC_AGG_SIZE, m_AggSize, pDisplayUnits->ComponentDim );
   }
   catch(...)
   {
      m_bErrorInDDX = true;
      throw;
   }
}


BEGIN_MESSAGE_MAP(CConcreteEntryGeneralPage, CPropertyPage)
	//{{AFX_MSG_MAP(CConcreteEntryGeneralPage)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	ON_BN_CLICKED(IDC_MOD_E, OnModE)
   ON_CBN_SELCHANGE(IDC_CONCRETE_TYPE,OnConcreteType)
	//}}AFX_MSG_MAP
   ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConcreteEntryGeneralPage message handlers
void CConcreteEntryGeneralPage::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_CONCRETE_GENERAL );
}


BOOL CConcreteEntryGeneralPage::OnInitDialog() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComboBox* pcbConcreteType = (CComboBox*)GetDlgItem(IDC_CONCRETE_TYPE);
   int idx = pcbConcreteType->AddString(ConcreteLibraryEntry::GetConcreteType(pgsTypes::Normal));
   pcbConcreteType->SetItemData(idx,(DWORD_PTR)pgsTypes::Normal);

   idx = pcbConcreteType->AddString(ConcreteLibraryEntry::GetConcreteType(pgsTypes::AllLightweight));
   pcbConcreteType->SetItemData(idx,(DWORD_PTR)pgsTypes::AllLightweight);

   idx = pcbConcreteType->AddString(ConcreteLibraryEntry::GetConcreteType(pgsTypes::SandLightweight));
   pcbConcreteType->SetItemData(idx,(DWORD_PTR)pgsTypes::SandLightweight);

   idx = pcbConcreteType->AddString(ConcreteLibraryEntry::GetConcreteType(pgsTypes::UHPC));
   pcbConcreteType->SetItemData(idx, (DWORD_PTR)pgsTypes::UHPC);


	CPropertyPage::OnInitDialog();
	
   OnConcreteType();
   OnModE();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CConcreteEntryGeneralPage::OnModE()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

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

pgsTypes::ConcreteType CConcreteEntryGeneralPage::GetConreteType()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComboBox* pcbConcreteType = (CComboBox*)GetDlgItem(IDC_CONCRETE_TYPE);
   pgsTypes::ConcreteType type = (pgsTypes::ConcreteType)pcbConcreteType->GetItemData(pcbConcreteType->GetCurSel());
   return type;
}

void CConcreteEntryGeneralPage::OnConcreteType()
{
   GetDlgItem(IDC_DS)->Invalidate();
   GetDlgItem(IDC_DW)->Invalidate();
}

HBRUSH CConcreteEntryGeneralPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

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

void CConcreteEntryGeneralPage::OnOK()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPropertyPage::OnOK();

   if ( !m_bErrorInDDX && !IsDensityInRange(m_Ds,m_Type))
   {
      AfxMessageBox(m_Type == pgsTypes::Normal? IDS_NWC_MESSAGE : IDS_LWC_MESSAGE, MB_OK | MB_ICONINFORMATION);
   }

   if (!m_bErrorInDDX && !IsStrengthInRange(m_Fc, m_Type))
   {
      AfxMessageBox(_T("The concrete strength is not in the normal range for UHPC.\nThe concrete will be treated as UHPC."));
   }
}

bool CConcreteEntryGeneralPage::IsDensityInRange(Float64 density,pgsTypes::ConcreteType type)
{
   CEAFApp* pApp = EAFGetApp();
   if ( type == pgsTypes::UHPC )
   {
      return true; // no density range for UHPC
   }
   else if ( type == pgsTypes::Normal )
   {
      return IsLE(m_MinNWCDensity,density);
   }
   else
   {
      return IsLE(density,m_MaxLWCDensity);
   }
}

bool CConcreteEntryGeneralPage::IsStrengthInRange(Float64 fc, pgsTypes::ConcreteType type)
{
   if (type == pgsTypes::UHPC)
   {
      return InRange(m_MinFcUHPC, fc, m_MaxFcUHPC);
   }
   else
   {
      return true; // no range limit for other concrete types
   }
}
#ifdef _DEBUG
void CConcreteEntryGeneralPage::AssertValid() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPropertyPage::AssertValid();
}
#endif