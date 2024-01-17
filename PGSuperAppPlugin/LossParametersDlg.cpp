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

// LossParametersDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "LossParametersDlg.h"
#include <EAF\EAFUtilities.h>
#include <EAF\EAFDocument.h>
#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CLossParametersDlg

IMPLEMENT_DYNAMIC(CLossParametersDlg, CPropertySheet)

CLossParametersDlg::CLossParametersDlg(CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T("Configure Prestress Losses"), pParentWnd, iSelectPage)
{
   Init();
}

CLossParametersDlg::~CLossParametersDlg()
{
}


BEGIN_MESSAGE_MAP(CLossParametersDlg, CPropertySheet)
END_MESSAGE_MAP()


// CLossParametersDlg message handlers
void CLossParametersDlg::Init()
{
   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;

   m_Pretensioning.m_psp.dwFlags  |= PSP_HASHELP;
   m_PostTensioning.m_psp.dwFlags |= PSP_HASHELP;
   m_TimeStepProperties.m_psp.dwFlags |= PSP_HASHELP;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ILossParameters,pLossParameters);
   if ( pLossParameters->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      // General lump sum is only available if we aren't doing a time-step analysis
      AddPage(&m_Pretensioning);
   }

   AddPage(&m_PostTensioning);

   if ( pLossParameters->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      AddPage(&m_TimeStepProperties);
   }
}
