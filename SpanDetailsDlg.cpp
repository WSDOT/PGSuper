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

// SpanDetailsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "SpanDetailsDlg.h"

#include <PgsExt\BridgeDescription2.h>

#include <EAF\EAFDocument.h>
#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpanDetailsDlg

IMPLEMENT_DYNAMIC(CSpanDetailsDlg, CPropertySheet)

CSpanDetailsDlg::CSpanDetailsDlg(const CBridgeDescription2* pBridgeDesc,SpanIndexType spanIdx,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T(""), pParentWnd, iSelectPage)
{
   Init(pBridgeDesc,spanIdx);
   InitPages();
}

CSpanDetailsDlg::CSpanDetailsDlg(const CBridgeDescription2* pBridgeDesc,SpanIndexType spanIdx,const std::vector<EditBridgeExtension>& editBridgeExtensions,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T(""), pParentWnd, iSelectPage)
{
   Init(pBridgeDesc,spanIdx);
   InitPages(editBridgeExtensions);
}

CSpanDetailsDlg::~CSpanDetailsDlg()
{
   DestroyExtensionPages();
}

INT_PTR CSpanDetailsDlg::DoModal()
{
   INT_PTR result = CPropertySheet::DoModal();
   if ( result == IDOK )
   {
      if (0 < m_BridgeExtensionPages.size())
      {
         NotifyBridgeExtensionPages();
      }
      else
      {
         NotifyExtensionPages();
      }
   }

   return result;
}

const CBridgeDescription2* CSpanDetailsDlg::GetBridgeDescription()
{
   return &m_BridgeDesc;
}

void CSpanDetailsDlg::CommonInitPages()
{
   m_psh.dwFlags                       |= PSH_HASHELP | PSH_NOAPPLYNOW;
   m_SpanLayoutPage.m_psp.dwFlags      |= PSP_HASHELP;

   AddPage(&m_SpanLayoutPage);

   // Even though connections and girder spacing aren't defined by span,
   // spans and groups are the same thing for PGSuper documents so it
   // make sense to present the input in this form. The user is expecting it
   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      m_GirderLayoutPage.m_psp.dwFlags |= PSP_HASHELP;
      AddPage(&m_GirderLayoutPage);

      m_StartPierPage.m_psp.dwFlags |= PSP_HASHELP;
      AddPage(&m_StartPierPage);

      m_EndPierPage.m_psp.dwFlags |= PSP_HASHELP;
      AddPage(&m_EndPierPage);
   }

   m_SpanGdrDetailsBearingsPage.m_psp.dwFlags |= PSP_HASHELP;
   AddPage(&m_SpanGdrDetailsBearingsPage);
}

void CSpanDetailsDlg::InitPages()
{
   CommonInitPages();
   CreateExtensionPages();
}

void CSpanDetailsDlg::InitPages(const std::vector<EditBridgeExtension>& editBridgeExtensions)
{
   CommonInitPages();
   CreateExtensionPages(editBridgeExtensions);
}

void CSpanDetailsDlg::Init(const CBridgeDescription2* pBridgeDesc,SpanIndexType spanIdx)
{
   // copy the bridge that we are operating on. we don't want to
   // change the actual bridge as the various UI elements manipulate
   // the bridge model. (If user presses Cancel button, we don't want
   // to have to undo changes to the original bridge model)
   m_BridgeDesc = *pBridgeDesc;

   m_pSpanData = m_BridgeDesc.GetSpan(spanIdx);
   m_pPrevPier = m_pSpanData->GetPrevPier();
   m_pNextPier = m_pSpanData->GetNextPier();

   m_pGirderGroup = m_BridgeDesc.GetGirderGroup(m_pSpanData);

   m_SpanLayoutPage.Init(this);

   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      ATLASSERT(m_pPrevPier->IsBoundaryPier());
      ATLASSERT(m_pNextPier->IsBoundaryPier());

      m_StartPierPage.Init(m_pPrevPier);
      m_EndPierPage.Init(m_pNextPier);
      m_GirderLayoutPage.Init(this);
   }

   m_SpanGdrDetailsBearingsPage.Initialize(&m_BridgeDesc, m_pPrevPier, m_pNextPier, INVALID_INDEX);

   // Set dialog title
   CString strTitle;
   strTitle.Format(_T("Span %d Details"),LABEL_SPAN(m_pSpanData->GetIndex()));
   SetTitle(strTitle);


   CString strStartPierLabel(m_pPrevPier->GetPrevSpan() == nullptr ? _T("Abut.") : _T("Pier"));
   m_strStartPierTitle.Format(_T("%s %d Connections"),strStartPierLabel,LABEL_PIER(m_pPrevPier->GetIndex()));
   m_StartPierPage.m_psp.dwFlags |= PSP_USETITLE;
   m_StartPierPage.m_psp.pszTitle = m_strStartPierTitle.GetBuffer();

   CString strEndPierLabel(m_pNextPier->GetNextSpan() == nullptr ? _T("Abut.") : _T("Pier"));
   m_strEndPierTitle.Format(_T("%s %d Connections"),strEndPierLabel,LABEL_PIER(m_pNextPier->GetIndex()));
   m_EndPierPage.m_psp.dwFlags |= PSP_USETITLE;
   m_EndPierPage.m_psp.pszTitle = m_strEndPierTitle.GetBuffer();
}


BEGIN_MESSAGE_MAP(CSpanDetailsDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CSpanDetailsDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_KICKIDLE,OnKickIdle)
END_MESSAGE_MAP()

LRESULT CSpanDetailsDlg::OnKickIdle(WPARAM wp, LPARAM lp)
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
// CSpanDetailsDlg message handlers
void CSpanDetailsDlg::CreateExtensionPages()
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   const std::map<IDType,IEditSpanCallback*>& callbacks = pDoc->GetEditSpanCallbacks();
   std::map<IDType,IEditSpanCallback*>::const_iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditSpanCallback*>::const_iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditSpanCallback* pCallback = callbackIter->second;
      CPropertyPage* pPage = pCallback->CreatePropertyPage(this);
      if ( pPage )
      {
         m_ExtensionPages.emplace_back(pCallback,pPage);
         AddPage(pPage);
      }
   }
}

void CSpanDetailsDlg::CreateExtensionPages(const std::vector<EditBridgeExtension>& editBridgeExtensions)
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   m_BridgeExtensionPages = editBridgeExtensions;
   std::sort(m_BridgeExtensionPages.begin(), m_BridgeExtensionPages.end());

   const std::map<IDType,IEditSpanCallback*>& callbacks = pDoc->GetEditSpanCallbacks();
   std::map<IDType,IEditSpanCallback*>::const_iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditSpanCallback*>::const_iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditSpanCallback* pEditSpanCallback = callbackIter->second;
      IDType editBridgeCallbackID = pEditSpanCallback->GetEditBridgeCallbackID();
      CPropertyPage* pPage = nullptr;
      if ( editBridgeCallbackID == INVALID_ID )
      {
         pPage = pEditSpanCallback->CreatePropertyPage(this);
      }
      else
      {
         EditBridgeExtension key;
         key.callbackID = editBridgeCallbackID;
         std::vector<EditBridgeExtension>::const_iterator found(std::find(m_BridgeExtensionPages.begin(), m_BridgeExtensionPages.end(),key));
         if ( found != m_BridgeExtensionPages.end() )
         {
            const EditBridgeExtension& extension = *found;
            CPropertyPage* pBridgePage = extension.pPage;
            pPage = pEditSpanCallback->CreatePropertyPage(this,pBridgePage);
         }
      }

      if ( pPage )
      {
         m_ExtensionPages.emplace_back(pEditSpanCallback,pPage);
         AddPage(pPage);
      }
   }
}

void CSpanDetailsDlg::DestroyExtensionPages()
{
   std::vector<std::pair<IEditSpanCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   std::vector<std::pair<IEditSpanCallback*,CPropertyPage*>>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      CPropertyPage* pPage = pageIter->second;
      delete pPage;
   }
   m_ExtensionPages.clear();
}

txnTransaction* CSpanDetailsDlg::GetExtensionPageTransaction()
{
   if (0 < m_Macro.GetTxnCount())
   {
      return m_Macro.CreateClone();
   }
   else
   {
      return nullptr;
   }
}

void CSpanDetailsDlg::NotifyExtensionPages()
{
   std::vector<std::pair<IEditSpanCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   std::vector<std::pair<IEditSpanCallback*,CPropertyPage*>>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      IEditSpanCallback* pCallback = pageIter->first;
      CPropertyPage* pPage = pageIter->second;
      txnTransaction* pTxn = pCallback->OnOK(pPage,this);
      if ( pTxn )
      {
         m_Macro.AddTransaction(pTxn);
      }
   }
}

void CSpanDetailsDlg::NotifyBridgeExtensionPages()
{
   // This gets called when this dialog is created from the framing grid and it is closed with IDOK
   // It gives the bridge dialog extension pages to sync their data with whatever got changed in 
   // the extension pages in this dialog
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSuperDoc* pDoc = (CPGSuperDoc*)pEAFDoc;

   const std::map<IDType,IEditSpanCallback*>& callbacks = pDoc->GetEditSpanCallbacks();
   std::map<IDType,IEditSpanCallback*>::const_iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditSpanCallback*>::const_iterator callbackIterEnd(callbacks.end());
   std::vector<std::pair<IEditSpanCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   for ( ; callbackIter != callbackIterEnd; callbackIter++, pageIter++ )
   {
      IEditSpanCallback* pCallback = callbackIter->second;
      IDType editBridgeCallbackID = pCallback->GetEditBridgeCallbackID();
      CPropertyPage* pSpanPage = pageIter->second;

      if ( editBridgeCallbackID != INVALID_ID )
      {
         EditBridgeExtension key;
         key.callbackID = editBridgeCallbackID;
         std::vector<EditBridgeExtension>::iterator found(std::find(m_BridgeExtensionPages.begin(), m_BridgeExtensionPages.end(),key));
         if ( found != m_BridgeExtensionPages.end() )
         {
            EditBridgeExtension& extension = *found;
            CPropertyPage* pBridgePage = extension.pPage;
            extension.pCallback->EditSpan_OnOK(pBridgePage,pSpanPage);
         }
      }
   }
}


pgsTypes::BoundaryConditionType CSpanDetailsDlg::GetConnectionType(pgsTypes::MemberEndType end)
{
   pgsTypes::BoundaryConditionType connectionType;
   if (end == pgsTypes::metStart)
   {
      connectionType = m_pPrevPier->GetBoundaryConditionType();
   }
   else
   {
      connectionType = m_pNextPier->GetBoundaryConditionType();
   }

   return connectionType;
}

GirderIndexType CSpanDetailsDlg::GetGirderCount()
{
   return m_pGirderGroup->GetGirderCount();
}
