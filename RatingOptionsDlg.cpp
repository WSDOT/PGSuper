///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperAppPlugin\Resource.h"
#include "RatingOptionsDlg.h"
#include <IFace\Project.h>
#include <IFace\RatingSpecification.h>

#include "PGSuperDocBase.h"

// CRatingOptionsDlg

IMPLEMENT_DYNAMIC(CRatingOptionsDlg, CPropertySheet)

CRatingOptionsDlg::CRatingOptionsDlg(CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T("Load Rating Options"), pParentWnd, iSelectPage)
{
   Init();
}

CRatingOptionsDlg::~CRatingOptionsDlg()
{
   DestroyExtensionPages();
}

INT_PTR CRatingOptionsDlg::DoModal()
{
   INT_PTR result = CPropertySheet::DoModal();
   if ( result == IDOK )
   {
      NotifyExtensionPages();
   }

   return result;
}

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
   
   CreateExtensionPages();
}

void CRatingOptionsDlg::GetLoadFactorToolTip(CString& strTip,pgsTypes::LimitState ls)
{
   GetLoadFactorToolTip(strTip,ls,pgsTypes::ptMultipleTripWithTraffic/*doesn't really matter, it isn't used with these limit states*/);
}

void CRatingOptionsDlg::GetLoadFactorToolTip(CString& strTip,pgsTypes::LimitState ls,pgsTypes::SpecialPermitType specialPermitType)
{
   CComPtr<IBroker> broker;
   EAFGetBroker(&broker);
   GET_IFACE2(broker,IRatingSpecification,pRatingSpec);
   GET_IFACE2(broker,ILibrary,pLibrary);
   const RatingLibraryEntry* pRatingEntry = pLibrary->GetRatingEntry(m_GeneralPage.m_Data.CriteriaName.c_str());
   Float64 gLL = pRatingSpec->GetLiveLoadFactor(ls,specialPermitType,m_GeneralPage.m_Data.ADTT,pRatingEntry,true);
   if ( gLL < 0 )
   {
      AfxFormatString1(strTip,IDS_LIVE_LOAD_FACTOR_TOOLTIP,_T("The live load factor is a function of the axle weights on the bridge"));
   }
   else
   {
      CString strLF;
      strLF.Format(_T("The computed live load factor is %0.2f"),gLL);
      AfxFormatString1(strTip,IDS_LIVE_LOAD_FACTOR_TOOLTIP,strLF);
   }
}

BEGIN_MESSAGE_MAP(CRatingOptionsDlg, CPropertySheet)
	ON_MESSAGE(WM_KICKIDLE,OnKickIdle)
END_MESSAGE_MAP()


// CRatingOptionsDlg message handlers

LRESULT CRatingOptionsDlg::OnKickIdle(WPARAM wp, LPARAM lp)
{
   // The CPropertySheet::OnKickIdle method calls GetActivePage()
   // which doesn't work with extension pages. Since GetActivePage
   // is not virtual, we have to replace the implementation of
   // OnKickIdle.
   // The same problem exists with OnCommandHelp

	ASSERT_VALID(this);

	CPropertyPage* pPage = GetPage(GetActiveIndex());

	/* Forward the message on to the active page of the property sheet */
	if( pPage != nullptr )
	{
		//ASSERT_VALID(pPage);
		return pPage->SendMessage( WM_KICKIDLE, wp, lp );
	}
	else
   {
		return 0;
   }
}

BOOL CRatingOptionsDlg::OnInitDialog()
{
	CPropertySheet::OnInitDialog();

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_RF),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CRatingOptionsDlg::CreateExtensionPages()
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   const std::map<IDType,IEditLoadRatingOptionsCallback*>& callbacks = pDoc->GetEditLoadRatingOptionsCallbacks();
   std::map<IDType,IEditLoadRatingOptionsCallback*>::const_iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditLoadRatingOptionsCallback*>::const_iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditLoadRatingOptionsCallback* pCallback = callbackIter->second;
      CPropertyPage* pPage = pCallback->CreatePropertyPage(this);
      if ( pPage )
      {
         EditLoadRatingOptionsExtension extension;
         extension.callbackID = callbackIter->first;
         extension.pCallback = pCallback;
         extension.pPage = pPage;
         m_ExtensionPages.insert(extension);
         AddPage(pPage);
      }
   }
}

void CRatingOptionsDlg::DestroyExtensionPages()
{
   std::set<EditLoadRatingOptionsExtension>::iterator extIter(m_ExtensionPages.begin());
   std::set<EditLoadRatingOptionsExtension>::iterator extIterEnd(m_ExtensionPages.end());
   for ( ; extIter != extIterEnd; extIter++ )
   {
      CPropertyPage* pPage = extIter->pPage;
      delete pPage;
   }
   m_ExtensionPages.clear();
}

txnTransaction* CRatingOptionsDlg::GetExtensionPageTransaction()
{
   if ( 0 < m_Macro.GetTxnCount() )
   {
      return m_Macro.CreateClone();
   }
   else
   {
      return nullptr;
   }
}

void CRatingOptionsDlg::NotifyExtensionPages()
{
   std::set<EditLoadRatingOptionsExtension>::iterator pageIter(m_ExtensionPages.begin());
   std::set<EditLoadRatingOptionsExtension>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      IEditLoadRatingOptionsCallback* pCallback = pageIter->pCallback;
      CPropertyPage* pPage = pageIter->pPage;
      txnTransaction* pTxn = pCallback->OnOK(pPage,this);
      if ( pTxn )
      {
         m_Macro.AddTransaction(pTxn);
      }
   }
}
