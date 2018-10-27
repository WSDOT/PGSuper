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

// PierDetailsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PierDetailsDlg.h"
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\ClosureJointData.h>

#include "PGSuperDocBase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPierDetailsDlg

IMPLEMENT_DYNAMIC(CPierDetailsDlg, CPropertySheet)

CPierDetailsDlg::CPierDetailsDlg(const CBridgeDescription2* pBridge,PierIndexType pierIdx,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T(""), pParentWnd, iSelectPage)
{
   Init(pBridge,pierIdx);
   InitPages();
}

CPierDetailsDlg::CPierDetailsDlg(const CBridgeDescription2* pBridge,PierIndexType pierIdx,const std::vector<EditBridgeExtension>& editBridgeExtensions,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T(""), pParentWnd, iSelectPage)
{
   Init(pBridge,pierIdx);
   InitPages(editBridgeExtensions);
}

CPierDetailsDlg::~CPierDetailsDlg()
{
   DestroyExtensionPages();
}

INT_PTR CPierDetailsDlg::DoModal()
{
   INT_PTR result = CPropertySheet::DoModal();
   if ( result == IDOK )
   {
      NotifyExtensionPages();
      NotifyBridgeExtensionPages();
   }

   return result;
}

CBridgeDescription2* CPierDetailsDlg::GetBridgeDescription()
{
   return &m_BridgeDesc;
}

void CPierDetailsDlg::CommonInitPages()
{
   m_psh.dwFlags                            |= PSH_HASHELP | PSH_NOAPPLYNOW;
   m_PierLocationPage.m_psp.dwFlags         |= PSP_HASHELP;
   m_AbutmentConnectionsPage.m_psp.dwFlags  |= PSP_HASHELP;
   m_PierLayoutPage.m_psp.dwFlags           |= PSP_HASHELP;
   m_PierConnectionsPage.m_psp.dwFlags      |= PSP_HASHELP;
   m_PierGirderSpacingPage.m_psp.dwFlags    |= PSP_HASHELP;

   m_ClosureJointGeometryPage.m_psp.dwFlags |= PSP_HASHELP;
   m_GirderSegmentSpacingPage.m_psp.dwFlags |= PSP_HASHELP;

   AddPage(&m_PierLocationPage);
   AddPage(&m_PierLayoutPage);

   if ( m_pPier->IsBoundaryPier() )
   {
      if ( m_pPier->IsAbutment() )
      {
         AddPage(&m_AbutmentConnectionsPage);
      }
      else
      {
         AddPage(&m_PierConnectionsPage);
      }
      AddPage(&m_PierGirderSpacingPage);
   }
   else
   {
      AddPage(&m_ClosureJointGeometryPage);
      AddPage(&m_GirderSegmentSpacingPage);
   }
}

void CPierDetailsDlg::InitPages()
{
   CommonInitPages();
   CreateExtensionPages();
}

void CPierDetailsDlg::InitPages(const std::vector<EditBridgeExtension>& editBridgeExtensions)
{
   CommonInitPages();
   CreateExtensionPages(editBridgeExtensions);
}

void CPierDetailsDlg::Init(const CBridgeDescription2* pBridge,PierIndexType pierIdx)
{
   m_BridgeDesc = *pBridge;
   m_pPier  = m_BridgeDesc.GetPier(pierIdx);
   m_pSpan[pgsTypes::Back]  = m_pPier->GetSpan(pgsTypes::Back);
   m_pSpan[pgsTypes::Ahead] = m_pPier->GetSpan(pgsTypes::Ahead);

   m_PierLocationPage.Init(m_pPier);

   if ( m_pPier->IsBoundaryPier() )
   {
      m_AbutmentConnectionsPage.Init(m_pPier);
      m_PierLayoutPage.Init(m_pPier);
      m_PierConnectionsPage.Init(m_pPier);
      m_PierGirderSpacingPage.Init(this);
   }
   else
   {
      m_PierLayoutPage.Init(m_pPier);
      m_ClosureJointGeometryPage.Init(m_pPier);
      m_GirderSegmentSpacingPage.Init(m_pPier);
   }

   // Set dialog title
   CString strTitle;
   strTitle.Format(_T("%s %d Details"),
      m_pPier->IsAbutment() ? _T("Abutment") : _T("Pier"),
      LABEL_PIER(pierIdx));
   
   SetTitle(strTitle);
}

BEGIN_MESSAGE_MAP(CPierDetailsDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CPierDetailsDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_KICKIDLE,OnKickIdle)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPierDetailsDlg message handlers

LRESULT CPierDetailsDlg::OnKickIdle(WPARAM wp, LPARAM lp)
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

/////////////////////////////////////////////////////////////////////////////
// CPierDetailsDlg message handlers
void CPierDetailsDlg::CreateExtensionPages()
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   const std::map<IDType,IEditPierCallback*>& callbacks = pDoc->GetEditPierCallbacks();
   std::map<IDType,IEditPierCallback*>::const_iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditPierCallback*>::const_iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditPierCallback* pEditPierCallback = callbackIter->second;
      CPropertyPage* pPage = pEditPierCallback->CreatePropertyPage(this);
      m_ExtensionPages.push_back( std::make_pair(pEditPierCallback,pPage) );
      if ( pPage )
      {
         AddPage(pPage);
      }
   }
}

void CPierDetailsDlg::CreateExtensionPages(const std::vector<EditBridgeExtension>& editBridgeExtensions)
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   m_BridgeExtensionPages = editBridgeExtensions;
   std::sort(m_BridgeExtensionPages.begin(), m_BridgeExtensionPages.end());


   const std::map<IDType,IEditPierCallback*>& callbacks = pDoc->GetEditPierCallbacks();
   std::map<IDType,IEditPierCallback*>::const_iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditPierCallback*>::const_iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditPierCallback* pEditPierCallback = callbackIter->second;
      IDType editBridgeCallbackID = pEditPierCallback->GetEditBridgeCallbackID();
      CPropertyPage* pPage = nullptr;
      if ( editBridgeCallbackID == INVALID_ID )
      {
         pPage = pEditPierCallback->CreatePropertyPage(this);
      }
      else
      {
         EditBridgeExtension key;
         key.callbackID = editBridgeCallbackID;
         std::vector<EditBridgeExtension>::const_iterator found(std::find(m_BridgeExtensionPages.begin(),m_BridgeExtensionPages.end(),key));
         if ( found != m_BridgeExtensionPages.end() )
         {
            const EditBridgeExtension& extension = *found;
            CPropertyPage* pBridgePage = extension.pPage;
            pPage = pEditPierCallback->CreatePropertyPage(this,pBridgePage);
         }
      }

      m_ExtensionPages.push_back( std::make_pair(pEditPierCallback,pPage) );

      if ( pPage )
      {
         AddPage(pPage);
      }
   }
}

void CPierDetailsDlg::DestroyExtensionPages()
{
   std::vector<std::pair<IEditPierCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   std::vector<std::pair<IEditPierCallback*,CPropertyPage*>>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      CPropertyPage* pPage = pageIter->second;
      if ( pPage )
      {
         delete pPage;
      }
   }
   m_ExtensionPages.clear();
}

txnTransaction* CPierDetailsDlg::GetExtensionPageTransaction()
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

void CPierDetailsDlg::NotifyExtensionPages()
{
   std::vector<std::pair<IEditPierCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   std::vector<std::pair<IEditPierCallback*,CPropertyPage*>>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      IEditPierCallback* pCallback = pageIter->first;
      CPropertyPage* pPage = pageIter->second;
      txnTransaction* pTxn = pCallback->OnOK(pPage,this);
      if ( pTxn )
      {
         m_Macro.AddTransaction(pTxn);
      }
   }
}

void CPierDetailsDlg::NotifyBridgeExtensionPages()
{
   // This gets called when this dialog is created from the framing grid and it is closed with IDOK
   // It gives the bridge dialog extension pages to sync their data with whatever got changed in 
   // the extension pages in this dialog
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   const std::map<IDType,IEditPierCallback*>& callbacks = pDoc->GetEditPierCallbacks();
   std::map<IDType,IEditPierCallback*>::const_iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditPierCallback*>::const_iterator callbackIterEnd(callbacks.end());
   std::vector<std::pair<IEditPierCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   for ( ; callbackIter != callbackIterEnd; callbackIter++, pageIter++ )
   {
      IEditPierCallback* pCallback = callbackIter->second;
      IDType editBridgeCallbackID = pCallback->GetEditBridgeCallbackID();
      CPropertyPage* pPierPage = pageIter->second;

      if ( editBridgeCallbackID != INVALID_ID )
      {
         EditBridgeExtension key;
         key.callbackID = editBridgeCallbackID;
         std::vector<EditBridgeExtension>::iterator found(std::find(m_BridgeExtensionPages.begin(),m_BridgeExtensionPages.end(),key));
         if ( found != m_BridgeExtensionPages.end() )
         {
            EditBridgeExtension& extension = *found;
            CPropertyPage* pBridgePage = extension.pPage;
            extension.pCallback->EditPier_OnOK(pBridgePage,pPierPage);
         }
      }
   }
}

pgsTypes::BoundaryConditionType CPierDetailsDlg::GetConnectionType()
{
   return m_pPier->GetBoundaryConditionType();
}

GirderIndexType CPierDetailsDlg::GetGirderCount(pgsTypes::PierFaceType pierFace)
{
   return (m_pSpan[pierFace] ? m_pSpan[pierFace]->GetGirderCount() : 0);
}
