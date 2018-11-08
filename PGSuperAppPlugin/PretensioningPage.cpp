///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

// PretensioningPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PretensioningPage.h"

#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#include <IFace\Project.h>
#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CPretensioningPage dialog

IMPLEMENT_DYNAMIC(CPretensioningPage, CPropertyPage)

CPretensioningPage::CPretensioningPage()
	: CPropertyPage(CPretensioningPage::IDD)
{

}

CPretensioningPage::~CPretensioningPage()
{
}

void CPretensioningPage::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CPropertyPage::DoDataExchange(pDX);

   DDX_Check_Bool(pDX,IDC_USE_LUMPSUM,bUseLumpSumLosses);
   DDX_UnitValueAndTag(pDX, IDC_FINAL, IDC_FINAL_TAG, FinalLosses, pDisplayUnits->GetStressUnit());
	DDX_UnitValueAndTag(pDX, IDC_BEFORE_XFER, IDC_BEFORE_XFER_TAG, BeforeXferLosses, pDisplayUnits->GetStressUnit());
	DDX_UnitValueAndTag(pDX, IDC_AFTER_XFER, IDC_AFTER_XFER_TAG, AfterXferLosses, pDisplayUnits->GetStressUnit());
   DDX_UnitValueAndTag(pDX, IDC_LIFTING, IDC_LIFTING_TAG, LiftingLosses, pDisplayUnits->GetStressUnit());
   DDX_UnitValueAndTag(pDX, IDC_SHIPPING, IDC_SHIPPING_TAG, ShippingLosses, pDisplayUnits->GetStressUnit());
   DDX_UnitValueAndTag(pDX, IDC_BEFORE_TEMP_STRAND_REMOVAL, IDC_BEFORE_TEMP_STRAND_REMOVAL_TAG, BeforeTempStrandRemovalLosses, pDisplayUnits->GetStressUnit());
   DDX_UnitValueAndTag(pDX, IDC_AFTER_TEMP_STRAND_REMOVAL,  IDC_AFTER_TEMP_STRAND_REMOVAL_TAG,  AfterTempStrandRemovalLosses, pDisplayUnits->GetStressUnit());
   DDX_UnitValueAndTag(pDX, IDC_AFTER_DECK_PLACEMENT,  IDC_AFTER_DECK_PLACEMENT_TAG,  AfterDeckPlacementLosses, pDisplayUnits->GetStressUnit());
   DDX_UnitValueAndTag(pDX, IDC_AFTER_SIDL,  IDC_AFTER_SIDL_TAG,  AfterSIDLLosses, pDisplayUnits->GetStressUnit());

}


BEGIN_MESSAGE_MAP(CPretensioningPage, CPropertyPage)
	ON_COMMAND(ID_HELP, OnHelp)
   ON_BN_CLICKED(IDC_USE_LUMPSUM, &CPretensioningPage::OnUseLumpSumLosses)
END_MESSAGE_MAP()


// CPretensioningPage message handlers
BOOL CPretensioningPage::OnInitDialog()
{
   CPropertyPage::OnInitDialog();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );

   CString strMethod;
   switch( pSpecEntry->GetLossMethod() )
   {
   case LOSSES_AASHTO_REFINED:
      strMethod = _T("Losses calculated per Refined Estimate Method in accordance with AASHTO LRFD ") + CString(LrfdCw8th(_T("5.9.5.4"),_T("5.9.3.4")));
      break;
   case LOSSES_WSDOT_REFINED:
      strMethod = _T("Losses calculated per Refined Estimate Method in accordance with WSDOT Bridge Design Manual and AASHTO LRFD ") + CString(LrfdCw8th(_T("5.9.5.4"),_T("5.9.3.4")));
      break;
   case LOSSES_TXDOT_REFINED_2004:
      strMethod = _T("Losses calculated per Refined Estimate Method in accordance with TxDOT Bridge Design Manual and AASHTO LRFD ") + CString(LrfdCw8th(_T("5.9.5.4"),_T("5.9.3.4")));
      break;
   case LOSSES_TXDOT_REFINED_2013:
      strMethod = _T("Losses calculated per Refined Estimate Method in accordance with TxDOT Bridge Research Report 0-6374-2, June, 2013");
      break;
   case LOSSES_AASHTO_LUMPSUM:
   case LOSSES_AASHTO_LUMPSUM_2005:
      strMethod = _T("Losses calculated per Approximate Lump Sum Method in accordance with AASHTO LRFD ") + CString(LrfdCw8th(_T("5.9.5.3"),_T("5.9.3.3")));
      break;
   case LOSSES_WSDOT_LUMPSUM:
      strMethod = _T("Losses calculated per Approximate Lump Sum Method in accordance with WSDOT Bridge Design Manual and AASHTO LRFD ") + CString(LrfdCw8th(_T("5.9.5.3"),_T("5.9.3.3")));
      break;
   case LOSSES_TIME_STEP:
      strMethod = _T("Losses calculated with a time-step analysis");
      break;
   default:
      ATLASSERT(false); // Should never get here
   }

   CWnd* pWnd = GetDlgItem(IDC_LABEL);
   pWnd->SetWindowText(strMethod);

   OnUseLumpSumLosses();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CPretensioningPage::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_PRETENSIONING);
}

#define ENABLE_WINDOW(x) pWnd = GetDlgItem(x); pWnd->EnableWindow(bEnable)
void CPretensioningPage::OnUseLumpSumLosses()
{
   BOOL bEnable = IsDlgButtonChecked(IDC_USE_LUMPSUM);

   CWnd* pWnd;

   ENABLE_WINDOW(IDC_BEFORE_XFER_LABEL);
   ENABLE_WINDOW(IDC_BEFORE_XFER);
   ENABLE_WINDOW(IDC_BEFORE_XFER_TAG);

   ENABLE_WINDOW(IDC_AFTER_XFER_LABEL);
   ENABLE_WINDOW(IDC_AFTER_XFER);
   ENABLE_WINDOW(IDC_AFTER_XFER_TAG);

   ENABLE_WINDOW(IDC_LIFTING_LABEL);
   ENABLE_WINDOW(IDC_LIFTING);
   ENABLE_WINDOW(IDC_LIFTING_TAG);

   ENABLE_WINDOW(IDC_SHIPPING_LABEL);
   ENABLE_WINDOW(IDC_SHIPPING);
   ENABLE_WINDOW(IDC_SHIPPING_TAG);

   ENABLE_WINDOW(IDC_BEFORE_TEMP_STRAND_REMOVAL_LABEL);
   ENABLE_WINDOW(IDC_BEFORE_TEMP_STRAND_REMOVAL);
   ENABLE_WINDOW(IDC_BEFORE_TEMP_STRAND_REMOVAL_TAG);

   ENABLE_WINDOW(IDC_AFTER_TEMP_STRAND_REMOVAL_LABEL);
   ENABLE_WINDOW(IDC_AFTER_TEMP_STRAND_REMOVAL);
   ENABLE_WINDOW(IDC_AFTER_TEMP_STRAND_REMOVAL_TAG);

   ENABLE_WINDOW(IDC_AFTER_DECK_PLACEMENT_LABEL);
   ENABLE_WINDOW(IDC_AFTER_DECK_PLACEMENT);
   ENABLE_WINDOW(IDC_AFTER_DECK_PLACEMENT_TAG);

   ENABLE_WINDOW(IDC_AFTER_SIDL_LABEL);
   ENABLE_WINDOW(IDC_AFTER_SIDL);
   ENABLE_WINDOW(IDC_AFTER_SIDL_TAG);

   ENABLE_WINDOW(IDC_FINAL_LABEL);
   ENABLE_WINDOW(IDC_FINAL);
   ENABLE_WINDOW(IDC_FINAL_TAG);
}
