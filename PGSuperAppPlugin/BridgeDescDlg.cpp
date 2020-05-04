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

// BridgeDescDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "resource.h"
#include "BridgeDescDlg.h"
#include <PgsExt\DeckRebarData.h>

#include "PGSuperDocBase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDlg

IMPLEMENT_DYNAMIC(CBridgeDescDlg, CPropertySheet)

CBridgeDescDlg::CBridgeDescDlg(const CBridgeDescription2& bridgeDesc,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T("Bridge Description"), pParentWnd, iSelectPage)
{
   SetBridgeDescription(bridgeDesc);
   Init();
}

CBridgeDescDlg::~CBridgeDescDlg()
{
   DestroyExtensionPages();
}

INT_PTR CBridgeDescDlg::DoModal()
{
   INT_PTR result = CPropertySheet::DoModal();
   if ( result == IDOK )
   {
      NotifyExtensionPages();
   }

   return result;
}

void CBridgeDescDlg::SetBridgeDescription(const CBridgeDescription2& bridgeDesc)
{
   m_BridgeDesc = bridgeDesc;
}

const CBridgeDescription2& CBridgeDescDlg::GetBridgeDescription()
{
   return m_BridgeDesc;
}

void CBridgeDescDlg::Init()
{
   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;

   m_GeneralPage.m_psp.dwFlags        |= PSP_HASHELP;
   m_FramingPage.m_psp.dwFlags        |= PSP_HASHELP;
   m_RailingSystemPage.m_psp.dwFlags  |= PSP_HASHELP;
   m_DeckDetailsPage.m_psp.dwFlags    |= PSP_HASHELP;
   m_DeckRebarPage.m_psp.dwFlags      |= PSP_HASHELP;
   m_EnvironmentalPage.m_psp.dwFlags  |= PSP_HASHELP;
   m_BridgeDescrBearings.m_psp.dwFlags      |= PSP_HASHELP;

   AddPage(&m_GeneralPage );
   AddPage(&m_FramingPage );
   AddPage(&m_RailingSystemPage );
   AddPage(&m_DeckDetailsPage );
   AddPage(&m_DeckRebarPage );
   AddPage(&m_BridgeDescrBearings );
   AddPage(&m_EnvironmentalPage );

   CreateExtensionPages();
}

void CBridgeDescDlg::CreateExtensionPages()
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   const std::map<IDType,IEditBridgeCallback*>& callbacks = pDoc->GetEditBridgeCallbacks();
   std::map<IDType,IEditBridgeCallback*>::const_iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditBridgeCallback*>::const_iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditBridgeCallback* pCallback = callbackIter->second;
      CPropertyPage* pPage = pCallback->CreatePropertyPage(this);
      if ( pPage )
      {
         EditBridgeExtension extension;
         extension.callbackID = callbackIter->first;
         extension.pCallback = pCallback;
         extension.pPage = pPage;
         m_ExtensionPages.push_back(extension);
         AddPage(pPage);
      }
   }
}

void CBridgeDescDlg::DestroyExtensionPages()
{
   std::vector<EditBridgeExtension>::iterator extIter(m_ExtensionPages.begin());
   std::vector<EditBridgeExtension>::iterator extIterEnd(m_ExtensionPages.end());
   for ( ; extIter != extIterEnd; extIter++ )
   {
      CPropertyPage* pPage = extIter->pPage;
      delete pPage;
   }
   m_ExtensionPages.clear();
}

const std::vector<EditBridgeExtension>& CBridgeDescDlg::GetExtensionPages() const
{
   return m_ExtensionPages;
}

std::vector<EditBridgeExtension>& CBridgeDescDlg::GetExtensionPages()
{
   return m_ExtensionPages;
}

txnTransaction* CBridgeDescDlg::GetExtensionPageTransaction()
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

void CBridgeDescDlg::NotifyExtensionPages()
{
   std::vector<EditBridgeExtension>::iterator pageIter(m_ExtensionPages.begin());
   std::vector<EditBridgeExtension>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      IEditBridgeCallback* pCallback = pageIter->pCallback;
      CPropertyPage* pPage = pageIter->pPage;
      txnTransaction* pTxn = pCallback->OnOK(pPage,this);
      if ( pTxn )
      {
         m_Macro.AddTransaction(pTxn);
      }
   }

   // Capture any transactions related to extenion pages on the Pier dialog opened from the framing grid
   PierIndexType nPiers = m_BridgeDesc.GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      std::vector<txnTransaction*> vTransactions = m_FramingPage.m_Grid.GetPierTransactions(pierIdx);
      std::vector<txnTransaction*>::iterator iter(vTransactions.begin());
      std::vector<txnTransaction*>::iterator end(vTransactions.end());
      for ( ; iter != end; iter++ )
      {
         txnTransaction* pTxn = *iter;
         m_Macro.AddTransaction(pTxn->CreateClone()); // must create a clone since the framing grid will delete the transactions
      }
   }

   // Capture any transactions related to extenion pages on the Span dialog opened from the framing grid
   SpanIndexType nSpans = m_BridgeDesc.GetSpanCount();
   for ( SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      std::vector<txnTransaction*> vTransactions = m_FramingPage.m_Grid.GetSpanTransactions(spanIdx);
      std::vector<txnTransaction*>::iterator iter(vTransactions.begin());
      std::vector<txnTransaction*>::iterator end(vTransactions.end());
      for ( ; iter != end; iter++ )
      {
         txnTransaction* pTxn = *iter;
         m_Macro.AddTransaction(pTxn->CreateClone()); // must create a clone since the framing grid will delete the transactions
      }
   }

   // Capture any transactions related to extenion pages on the Temporary Support dialog opened from the framing grid
   SupportIndexType nTS = m_BridgeDesc.GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      std::vector<txnTransaction*> vTransactions = m_FramingPage.m_Grid.GetTemporarySupportTransactions(tsIdx);
      std::vector<txnTransaction*>::iterator iter(vTransactions.begin());
      std::vector<txnTransaction*>::iterator end(vTransactions.end());
      for ( ; iter != end; iter++ )
      {
         txnTransaction* pTxn = *iter;
         m_Macro.AddTransaction(pTxn->CreateClone()); // must create a clone since the framing grid will delete the transactions
      }
   }
}

BEGIN_MESSAGE_MAP(CBridgeDescDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CBridgeDescDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
   WBFL_ON_PROPSHEET_OK
	ON_MESSAGE(WM_KICKIDLE,OnKickIdle)
END_MESSAGE_MAP()

LRESULT CBridgeDescDlg::OnKickIdle(WPARAM wp, LPARAM lp)
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

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescDlg message handlers
BOOL CBridgeDescDlg::OnInitDialog()
{
	CPropertySheet::OnInitDialog();

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_EDIT_BRIDGE),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

   UpdateData(FALSE); // calls DoDataExchange
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CBridgeDescDlg::OnOK()
{
   BOOL bOK = UpdateData(TRUE); // calls DoDataExchange
   return !bOK;
}

void CBridgeDescDlg::DoDataExchange(CDataExchange* pDX)
{
   // Call UpdateData on the active page because the
   // various pages depend on each other. This gets
   // all values out of the controls
   //
   // NOTE: if there is an error on the active page
   // the error message dialog gets displayed twice.
   // Once when this UpdateData is called and once
   // after this method returns and MFC processes
   // the OnKillActive message which inturn calls
   // UpdateData again.
   CPropertySheet::DoDataExchange(pDX);
   if ( pDX->m_bSaveAndValidate )
   {
      // force the active page to update its data
      auto* pPage = GetPage(GetActiveIndex());
      if ( pPage && !pPage->UpdateData(TRUE) )
      {
         pDX->Fail();
      }
   }
}
