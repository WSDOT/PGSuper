///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

// PostTensioningPage.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PostTensioningPage.h"
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFUtilities.h>
#include <EAF\EAFDocument.h>
#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CPostTensioningPage dialog

IMPLEMENT_DYNAMIC(CPostTensioningPage, CPropertyPage)

CPostTensioningPage::CPostTensioningPage()
	: CPropertyPage(CPostTensioningPage::IDD)
{

}

CPostTensioningPage::~CPostTensioningPage()
{
}

void CPostTensioningPage::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CPropertyPage::DoDataExchange(pDX);

   DDX_UnitValueAndTag(pDX, IDC_ANCHORSET_PT,  IDC_ANCHORSET_PT_TAG,  Dset_PT, pDisplayUnits->GetComponentDimUnit());
   DDX_UnitValueAndTag(pDX, IDC_WOBBLE_PT, IDC_WOBBLE_PT_TAG, WobbleFriction_PT, pDisplayUnits->GetPerLengthUnit());
   DDX_Text(pDX,IDC_FRICTION_PT,FrictionCoefficient_PT);

   DDX_UnitValueAndTag(pDX, IDC_ANCHORSET_TTS,  IDC_ANCHORSET_TTS_TAG,  Dset_TTS, pDisplayUnits->GetComponentDimUnit());
   DDX_UnitValueAndTag(pDX, IDC_WOBBLE_TTS, IDC_WOBBLE_TTS_TAG, WobbleFriction_TTS, pDisplayUnits->GetPerLengthUnit());
   DDX_Text(pDX,IDC_FRICTION_TTS,FrictionCoefficient_TTS);

   DDX_Control(pDX, IDC_DESCRIPTION, m_ctrlDescription);
}


BEGIN_MESSAGE_MAP(CPostTensioningPage, CPropertyPage)
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


// CPostTensioningPage message handlers

BOOL CPostTensioningPage::OnInitDialog()
{
   CPropertyPage::OnInitDialog();

   // TODO:  Add extra initialization here
   BOOL bShow;
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILossParameters,pLossParameters);
   if ( pLossParameters->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      m_ctrlDescription.SetWindowText(_T("These parameters are used for determining initial losses in post-tensioned tendons and temporary strands"));
      bShow = TRUE;
   }
   else
   {
      m_ctrlDescription.SetWindowText(_T("These parameters are used for determining initial losses in post-tensioned temporary strands"));
      bShow = FALSE;
   }

   GetDlgItem(IDC_TENDON_LABEL)->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_ANCHORSET_PT)->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_ANCHORSET_PT_TAG)->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_WOBBLE_PT)->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_WOBBLE_PT_TAG)->ShowWindow(bShow ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_FRICTION_PT)->ShowWindow(bShow ? SW_SHOW : SW_HIDE);

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CPostTensioningPage::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_POST_TENSIONING);
}
