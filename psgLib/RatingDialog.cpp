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

// RatingDialog.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "RatingDialog.h"
#include <MfcTools\CustomDDX.h>

#include "..\htmlhelp\HelpTopics.hh"

#include <EAF\EAFApp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRatingDialog

IMPLEMENT_DYNAMIC(CRatingDialog, CPropertySheet)

CRatingDialog::CRatingDialog( RatingLibraryEntry& rentry,
                                   bool allowEditing,
                                   CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T("Load Rating Criteria"), pParentWnd, iSelectPage),
   m_Entry(rentry),
   m_AllowEditing(allowEditing)
{
   m_LiveLoadFactorsPage[0] = new CLiveLoadFactorsPage(_T("Design - Inventory"),pgsTypes::lrDesign_Inventory);
   m_LiveLoadFactorsPage[1] = new CLiveLoadFactorsPage(_T("Design - Operating"),pgsTypes::lrDesign_Operating);
   m_LiveLoadFactorsPage[2] = new CLiveLoadFactorsPage(_T("Legal - Routine"),pgsTypes::lrLegal_Routine);
   m_LiveLoadFactorsPage[3] = new CLiveLoadFactorsPage(_T("Legal - Special"),pgsTypes::lrLegal_Special);

   m_PermitLiveLoadFactorsPage[0] = new CLiveLoadFactorsPage(_T("Permit - Routine"),pgsTypes::lrPermit_Routine);
   m_PermitLiveLoadFactorsPage[1] = new CLiveLoadFactorsPage(_T("Permit - Special, Single trip with escort"),pgsTypes::lrPermit_Special,pgsTypes::ptSingleTripWithEscort);
   m_PermitLiveLoadFactorsPage[2] = new CLiveLoadFactorsPage(_T("Permit - Special, Single trip with traffic"),pgsTypes::lrPermit_Special,pgsTypes::ptSingleTripWithTraffic);
   m_PermitLiveLoadFactorsPage[3] = new CLiveLoadFactorsPage(_T("Permit - Special, Multiple trips with traffic"),pgsTypes::lrPermit_Special,pgsTypes::ptMultipleTripWithTraffic);

   Init();
}

CRatingDialog::~CRatingDialog()
{
   for ( int i = 0; i < 4; i++ )
      delete m_LiveLoadFactorsPage[i];

   for ( int i = 0; i < 4; i++ )
      delete m_PermitLiveLoadFactorsPage[i];
}


BEGIN_MESSAGE_MAP(CRatingDialog, CPropertySheet)
	//{{AFX_MSG_MAP(CRatingDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRatingDialog message handlers
void CRatingDialog::Init()
{
   // Turn on help for the property sheet
   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;

   m_RatingDescriptionPage.m_psp.dwFlags |= PSP_HASHELP;
   AddPage(&m_RatingDescriptionPage);

   for ( int i = 0; i < 4; i++ )
   {
      m_LiveLoadFactorsPage[i]->m_psp.dwFlags |= PSP_HASHELP;
      AddPage(m_LiveLoadFactorsPage[i]);
   }

   for ( int i = 0; i < 4; i++ )
   {
      m_PermitLiveLoadFactorsPage[i]->m_psp.dwFlags |= PSP_HASHELP;
      AddPage(m_PermitLiveLoadFactorsPage[i]);
   }
}


BOOL CRatingDialog::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();
	
   // disable OK button if editing not allowed
   CString head;
   GetWindowText(head);
   head += _T(" - ");
   head += m_Entry.GetName().c_str();
	if (!m_AllowEditing)
   {
      CWnd* pbut = GetDlgItem(IDOK);
      ASSERT(pbut);
      pbut->EnableWindow(m_AllowEditing);
      head += _T(" (Read Only)");
   }
   SetWindowText(head);
	
	return bResult;
}

//lrfrVersionMgr::Version CRatingDialog::GetSpecVersion()
//{
//   return m_SpecDescrPage.GetSpecVersion();
//}

void CRatingDialog::ExchangeDescriptionData(CDataExchange* pDX)
{
   // specification type
   DDX_CBItemData(pDX,IDC_SPECIFICATION,m_Entry.m_SpecificationVersion);

   DDX_Check_Bool(pDX,IDC_ALWAYS_RATE,m_Entry.m_bAlwaysRate);

   if (pDX->m_bSaveAndValidate)
   {
	   DDX_Text(pDX, IDC_NAME, m_Name);
      if (m_Name.IsEmpty())
      {
         AfxMessageBox(_T("Name cannot be blank"));
         pDX->Fail();
      }
      m_Entry.SetName(m_Name);

	   DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_Description);
      m_Entry.SetDescription(m_Description);
   }
   else
   {
      // name
      m_Name = m_Entry.GetName().c_str();
	   DDX_Text(pDX, IDC_NAME, m_Name);

      m_Description = m_Entry.GetDescription().c_str();
	   DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_Description);
   }
}

void CRatingDialog::ExchangeLoadFactorData(CDataExchange* pDX,pgsTypes::LoadRatingType ratingType)
{
   ATLASSERT(ratingType != pgsTypes::lrPermit_Special);
   if ( pDX->m_bSaveAndValidate )
   {
      CLiveLoadFactorModel model;
      ExchangeLoadFactorData(pDX,&model);
      m_Entry.SetLiveLoadFactorModel(ratingType,model);
   }
   else
   {
      CLiveLoadFactorModel model = m_Entry.GetLiveLoadFactorModel(ratingType);
      ExchangeLoadFactorData(pDX,&model);
   }
}

void CRatingDialog::ExchangeLoadFactorData(CDataExchange* pDX,pgsTypes::SpecialPermitType permitType)
{
   if ( pDX->m_bSaveAndValidate )
   {
      CLiveLoadFactorModel model;
      ExchangeLoadFactorData(pDX,&model);
      m_Entry.SetLiveLoadFactorModel(permitType,model);
   }
   else
   {
      CLiveLoadFactorModel model = m_Entry.GetLiveLoadFactorModel(permitType);
      ExchangeLoadFactorData(pDX,&model);
   }
}

void CRatingDialog::ExchangeLoadFactorData(CDataExchange* pDX,CLiveLoadFactorModel* pModel)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   DDX_UnitValueAndTag(pDX,IDC_LOWER_VEHICLE_WEIGHT,IDC_LOWER_VEHICLE_WEIGHT_UNIT,pModel->m_Wlower,pDisplayUnits->GeneralForce);
   DDX_UnitValueAndTag(pDX,IDC_UPPER_VEHICLE_WEIGHT,IDC_UPPER_VEHICLE_WEIGHT_UNIT,pModel->m_Wupper,pDisplayUnits->GeneralForce);

   DDX_CBEnum(pDX,IDC_LL_METHOD,pModel->m_LiveLoadFactorType);
   DDX_CBEnum(pDX,IDC_INTERPOLATE,pModel->m_LiveLoadFactorModifier);

   DDX_Check_Bool(pDX,IDC_USER_OVERRIDE,pModel->m_bAllowUserOverride);

   DDX_Text(pDX,IDC_ADTT1,pModel->m_ADTT[0]);
   DDX_Text(pDX,IDC_ADTT2,pModel->m_ADTT[1]);
   DDX_Text(pDX,IDC_ADTT3,pModel->m_ADTT[2]);
   //DDX_Text(pDX,IDC_ADTT4,pModel->m_ADTT[3]); // this is the Unknown ADTT case

   DDX_Text(pDX,IDC_LF_LOWER1,pModel->m_gLL_Lower[0]);
   DDX_Text(pDX,IDC_LF_LOWER2,pModel->m_gLL_Lower[1]);
   DDX_Text(pDX,IDC_LF_LOWER3,pModel->m_gLL_Lower[2]);
   DDX_Text(pDX,IDC_LF_LOWER4,pModel->m_gLL_Lower[3]);

   DDX_Text(pDX,IDC_LF_UPPER1,pModel->m_gLL_Upper[0]);
   DDX_Text(pDX,IDC_LF_UPPER2,pModel->m_gLL_Upper[1]);
   DDX_Text(pDX,IDC_LF_UPPER3,pModel->m_gLL_Upper[2]);
   DDX_Text(pDX,IDC_LF_UPPER4,pModel->m_gLL_Upper[3]);

   DDX_Text(pDX,IDC_LF_SERVICE_1,pModel->m_gLL_Service[0]);
   DDX_Text(pDX,IDC_LF_SERVICE_2,pModel->m_gLL_Service[1]);
   DDX_Text(pDX,IDC_LF_SERVICE_3,pModel->m_gLL_Service[2]);
   DDX_Text(pDX,IDC_LF_SERVICE_4,pModel->m_gLL_Service[3]);

   if ( pDX->m_bSaveAndValidate )
   {
      if ( pModel->m_LiveLoadFactorType == pgsTypes::gllSingleValue )
      {
         DDV_GreaterThanZero(pDX,IDC_LF_SERVICE_1,pModel->m_gLL_Service[0]);
         DDV_GreaterThanZero(pDX,IDC_LF_LOWER1,pModel->m_gLL_Lower[0]);
      }
      else if ( pModel->m_LiveLoadFactorType == pgsTypes::gllStepped )
      {
         DDV_GreaterThanZero(pDX,IDC_LF_SERVICE_1,pModel->m_gLL_Service[0]);
         DDV_GreaterThanZero(pDX,IDC_LF_SERVICE_2,pModel->m_gLL_Service[1]);
         DDV_GreaterThanZero(pDX,IDC_LF_SERVICE_4,pModel->m_gLL_Service[3]);

         DDV_GreaterThanZero(pDX,IDC_LF_LOWER1,pModel->m_gLL_Lower[0]);
         DDV_GreaterThanZero(pDX,IDC_LF_LOWER2,pModel->m_gLL_Lower[1]);
         DDV_GreaterThanZero(pDX,IDC_LF_LOWER4,pModel->m_gLL_Lower[3]);

         DDV_LimitOrMore(pDX,IDC_ADTT1,pModel->m_ADTT[0],0);
      }
      else if ( pModel->m_LiveLoadFactorType == pgsTypes::gllLinear )
      {
         DDV_GreaterThanZero(pDX,IDC_LF_SERVICE_1,pModel->m_gLL_Service[0]);
         DDV_GreaterThanZero(pDX,IDC_LF_SERVICE_2,pModel->m_gLL_Service[1]);
         DDV_GreaterThanZero(pDX,IDC_LF_SERVICE_4,pModel->m_gLL_Service[3]);

         DDV_GreaterThanZero(pDX,IDC_LF_LOWER1,pModel->m_gLL_Lower[0]);
         DDV_GreaterThanZero(pDX,IDC_LF_LOWER2,pModel->m_gLL_Lower[1]);
         DDV_GreaterThanZero(pDX,IDC_LF_LOWER4,pModel->m_gLL_Lower[3]);

         DDV_LimitOrMore(pDX,IDC_ADTT1,pModel->m_ADTT[0],0);
         DDV_LimitOrMore(pDX,IDC_ADTT2,pModel->m_ADTT[1],0);

         if ( pModel->m_ADTT[1] < pModel->m_ADTT[0] )
         {
            pDX->PrepareEditCtrl(IDC_ADTT2);
            AfxMessageBox(_T("The ADTT < value must be less than the ADTT > value"),MB_OK | MB_ICONEXCLAMATION);
            pDX->Fail();
         }
      }
      else if ( pModel->m_LiveLoadFactorType == pgsTypes::gllBilinear )
      {
         DDV_GreaterThanZero(pDX,IDC_LF_SERVICE_1,pModel->m_gLL_Service[0]);
         DDV_GreaterThanZero(pDX,IDC_LF_SERVICE_2,pModel->m_gLL_Service[1]);
         DDV_GreaterThanZero(pDX,IDC_LF_SERVICE_3,pModel->m_gLL_Service[2]);
         DDV_GreaterThanZero(pDX,IDC_LF_SERVICE_4,pModel->m_gLL_Service[3]);

         DDV_GreaterThanZero(pDX,IDC_LF_LOWER1,pModel->m_gLL_Lower[0]);
         DDV_GreaterThanZero(pDX,IDC_LF_LOWER2,pModel->m_gLL_Lower[1]);
         DDV_GreaterThanZero(pDX,IDC_LF_LOWER3,pModel->m_gLL_Lower[2]);
         DDV_GreaterThanZero(pDX,IDC_LF_LOWER4,pModel->m_gLL_Lower[3]);

         DDV_LimitOrMore(pDX,IDC_ADTT1,pModel->m_ADTT[0],0);
         DDV_LimitOrMore(pDX,IDC_ADTT2,pModel->m_ADTT[1],0);
         DDV_LimitOrMore(pDX,IDC_ADTT3,pModel->m_ADTT[2],0);

         if ( !(pModel->m_ADTT[0] < pModel->m_ADTT[1]) ||
              !(pModel->m_ADTT[1] < pModel->m_ADTT[2]) )
         {
            pDX->PrepareEditCtrl(IDC_ADTT1);
            AfxMessageBox(_T("ADTT values are incorrect"),MB_OK | MB_ICONEXCLAMATION);
            pDX->Fail();
         }
      }
      else if ( pModel->m_LiveLoadFactorType == pgsTypes::gllBilinearWithWeight )
      {
         DDV_GreaterThanZero(pDX,IDC_LF_SERVICE_1,pModel->m_gLL_Service[0]);
         DDV_GreaterThanZero(pDX,IDC_LF_SERVICE_2,pModel->m_gLL_Service[1]);
         DDV_GreaterThanZero(pDX,IDC_LF_SERVICE_3,pModel->m_gLL_Service[2]);
         DDV_GreaterThanZero(pDX,IDC_LF_SERVICE_4,pModel->m_gLL_Service[3]);

         DDV_GreaterThanZero(pDX,IDC_LF_LOWER1,pModel->m_gLL_Lower[0]);
         DDV_GreaterThanZero(pDX,IDC_LF_LOWER2,pModel->m_gLL_Lower[1]);
         DDV_GreaterThanZero(pDX,IDC_LF_LOWER3,pModel->m_gLL_Lower[2]);
         DDV_GreaterThanZero(pDX,IDC_LF_LOWER4,pModel->m_gLL_Lower[3]);

         DDV_GreaterThanZero(pDX,IDC_LF_UPPER1,pModel->m_gLL_Upper[0]);
         DDV_GreaterThanZero(pDX,IDC_LF_UPPER2,pModel->m_gLL_Upper[1]);
         DDV_GreaterThanZero(pDX,IDC_LF_UPPER3,pModel->m_gLL_Upper[2]);
         DDV_GreaterThanZero(pDX,IDC_LF_UPPER4,pModel->m_gLL_Upper[3]);

         DDV_LimitOrMore(pDX,IDC_ADTT1,pModel->m_ADTT[0],0);
         DDV_LimitOrMore(pDX,IDC_ADTT2,pModel->m_ADTT[1],0);
         DDV_LimitOrMore(pDX,IDC_ADTT3,pModel->m_ADTT[2],0);

         DDV_UnitValueZeroOrMore(pDX,IDC_LOWER_VEHICLE_WEIGHT,pModel->m_Wlower,pDisplayUnits->GeneralForce);
         DDV_UnitValueZeroOrMore(pDX,IDC_UPPER_VEHICLE_WEIGHT,pModel->m_Wupper,pDisplayUnits->GeneralForce);

         if ( !(pModel->m_ADTT[0] < pModel->m_ADTT[1]) ||
              !(pModel->m_ADTT[1] < pModel->m_ADTT[2]) )
         {
            pDX->PrepareEditCtrl(IDC_ADTT1);
            AfxMessageBox(_T("ADTT values are incorrect"),MB_OK | MB_ICONEXCLAMATION);
            pDX->Fail();
         }

         if ( IsLE(pModel->m_Wupper,pModel->m_Wlower) )
         {
            pDX->PrepareEditCtrl(IDC_UPPER_VEHICLE_WEIGHT);
            AfxMessageBox(_T("Vehicle weight must be greater than the lower limit value"),MB_OK | MB_ICONEXCLAMATION);
            pDX->Fail();
         }
      }
   }
}