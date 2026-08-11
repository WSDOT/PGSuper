///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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

// AlignmentDescriptionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "AlignmentDescriptionDlg.h"
#include "PGSuperDocBase.h"


/////////////////////////////////////////////////////////////////////////////
// CAlignmentDescriptionDlg

IMPLEMENT_DYNAMIC(CAlignmentDescriptionDlg, CPropertySheet)

CAlignmentDescriptionDlg::CAlignmentDescriptionDlg(UINT nIDCaption,std::shared_ptr<WBFL::EAF::Broker> pBroker, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
   m_pBroker = pBroker;
   Init();
}

CAlignmentDescriptionDlg::CAlignmentDescriptionDlg(LPCTSTR pszCaption, std::shared_ptr<WBFL::EAF::Broker> pBroker,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
   m_pBroker = pBroker;
   Init();
}

CAlignmentDescriptionDlg::~CAlignmentDescriptionDlg()
{
   DestroyExtensionPages();
}

INT_PTR CAlignmentDescriptionDlg::DoModal()
{
   INT_PTR result = CPropertySheet::DoModal();
   if ( result == IDOK )
   {
      NotifyExtensionPages();
   }

   return result;
}


BEGIN_MESSAGE_MAP(CAlignmentDescriptionDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CAlignmentDescriptionDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
      ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAlignmentDescriptionDlg message handlers

void CAlignmentDescriptionDlg::Init()
{
   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;
   m_AlignmentPage.m_psp.dwFlags  |= PSP_HASHELP;
   m_ProfilePage.m_psp.dwFlags    |= PSP_HASHELP;
   m_CrownSlopePage.m_psp.dwFlags |= PSP_HASHELP;

   m_PageManager.AddPage(ALIGNMENTDLG_PAGE_HORIZONTAL_ALIGNMENT, &m_AlignmentPage);
   m_PageManager.AddPage(ALIGNMENTDLG_PAGE_PROFILE, &m_ProfilePage);
   m_PageManager.AddPage(ALIGNMENTDLG_PAGE_SUPERELEVATION, &m_CrownSlopePage);

   CreateExtensionPages();
   m_PageManager.Commit(this);
}

void CAlignmentDescriptionDlg::CreateExtensionPages()
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   const std::map<IDType,IEditAlignmentCallback*>& callbacks = pDoc->GetEditAlignmentCallbacks();
   std::map<IDType,IEditAlignmentCallback*>::const_iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditAlignmentCallback*>::const_iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditAlignmentCallback* pCallback = callbackIter->second;
      CPropertyPage* pPage = pCallback->CreatePropertyPage(this);
      if ( pPage )
      {
         m_ExtensionPages.emplace_back(pCallback,pPage);

         CString name(pCallback->GetPropertyPageName().c_str());
         if ( name.IsEmpty() )
         {
            name.Format(_T("Extension%d"), callbackIter->first);
         }
         m_PageManager.InsertExtensionPage(name, pPage, pCallback->GetPropertyPagePosition());
      }
   }
}

void CAlignmentDescriptionDlg::DestroyExtensionPages()
{
   std::vector<std::pair<IEditAlignmentCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   std::vector<std::pair<IEditAlignmentCallback*,CPropertyPage*>>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      CPropertyPage* pPage = pageIter->second;
      delete pPage;
   }
   m_ExtensionPages.clear();
}

std::unique_ptr<WBFL::EAF::Transaction> CAlignmentDescriptionDlg::GetExtensionPageTransaction()
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

void CAlignmentDescriptionDlg::NotifyExtensionPages()
{
   std::vector<std::pair<IEditAlignmentCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   std::vector<std::pair<IEditAlignmentCallback*,CPropertyPage*>>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      IEditAlignmentCallback* pCallback = pageIter->first;
      CPropertyPage* pPage = pageIter->second;
      auto pTxn = pCallback->OnOK(pPage,this);
      if ( pTxn )
      {
         m_Macro.AddTransaction(std::move(pTxn));
      }
   }
}

LRESULT CAlignmentDescriptionDlg::OnKickIdle(WPARAM wp, LPARAM lp)
{
   // The CPropertySheet::OnKickIdle method calls GetActivePage()
   // which doesn't work with extension pages. Since GetActivePage
   // is not virtual, we have to replace the implementation of
   // OnKickIdle.
   // The same problem exists with OnCommandHelp

	ASSERT_VALID(this);

	auto* pPage = GetPage(GetActiveIndex());

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