///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PostTensioningPage.h"
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFUtilities.h>
#include <EAF\EAFDocument.h>
#include "PGSuperDoc.h"

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

   DDX_UnitValueAndTag(pDX, IDC_ANCHORSET,  IDC_ANCHORSET_TAG,  Dset, pDisplayUnits->GetComponentDimUnit());
   DDX_UnitValueAndTag(pDX, IDC_WOBBLE, IDC_WOBBLE_TAG, WobbleFriction, pDisplayUnits->GetPerLengthUnit());
   DDX_Text(pDX,IDC_FRICTION,FrictionCoefficient);
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
   CEAFDocument* pEAFDoc = EAFGetDocument();
   if ( pEAFDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      m_ctrlDescription.SetWindowText(_T("These parameters are used for determining initial losses in post-tensioned temporary strands"));
   }
   else
   {
      m_ctrlDescription.SetWindowText(_T("These parameters are used for determining initial losses in post-tensioned tendons and temporary strands"));
   }

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CPostTensioningPage::OnHelp()
{
#pragma Reminder("UPDATE: implement Help button")
   //::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_PIERDETAILS_GIRDERSPACING );
}
