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

// RatingOptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuper.h"
#include "RatingOptionsDlg.h"
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

// CRatingOptionsDlg

IMPLEMENT_DYNAMIC(CRatingOptionsDlg, CPropertySheet)

CRatingOptionsDlg::CRatingOptionsDlg(CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet("Load Rating Options", pParentWnd, iSelectPage)
{
   Init();
}

CRatingOptionsDlg::~CRatingOptionsDlg()
{
}

void CRatingOptionsDlg::GetLoadFactorToolTip(CString& strTip,pgsTypes::LimitState ls)
{
   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2(broker,IRatingSpecification,pRatingSpec);
   GET_IFACE2(broker,ILibrary,pLibrary);
   const RatingLibraryEntry* pRatingEntry = pLibrary->GetRatingEntry(m_GeneralPage.m_Data.CriteriaName.c_str());
   Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,m_GeneralPage.m_Data.ADTT,pRatingEntry,true);
   if ( gLL < 0 )
   {
      AfxFormatString1(strTip,IDS_LIVE_LOAD_FACTOR_TOOLTIP,"The live load factor is a function of the axle weights on the bridge");
   }
   else
   {
      CString strLF;
      strLF.Format("The computed live load factor is %0.2f",gLL);
      AfxFormatString1(strTip,IDS_LIVE_LOAD_FACTOR_TOOLTIP,strLF);
   }
}

BEGIN_MESSAGE_MAP(CRatingOptionsDlg, CPropertySheet)
END_MESSAGE_MAP()


// CRatingOptionsDlg message handlers
void CRatingOptionsDlg::Init()
{
   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;

   m_GeneralPage.m_psp.dwFlags |= PSP_HASHELP;
   m_DesignPage.m_psp.dwFlags  |= PSP_HASHELP;
   m_LegalPage.m_psp.dwFlags   |= PSP_HASHELP;
   m_PermitPage.m_psp.dwFlags  |= PSP_HASHELP;

   AddPage( &m_GeneralPage );
   AddPage( &m_DesignPage );
   AddPage( &m_LegalPage );
   AddPage( &m_PermitPage );
}
