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

// ClosureJointDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin.h"
#include "ClosureJointDlg.h"

#include <LRFD\RebarPool.h>

#include "PGSuperDocBase.h"


#define IDC_CHECKBOX 100

// CClosureJointDlg

IMPLEMENT_DYNAMIC(CClosureJointDlg, CPropertySheet)

CClosureJointDlg::CClosureJointDlg(const CBridgeDescription2* pBridgeDesc,const CClosureKey& closureKey,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T("Closure Joint Details"), pParentWnd, iSelectPage),
   m_ClosureKey(closureKey)
{
   m_bEditingInGirder = false;
   Init(pBridgeDesc);
}

CClosureJointDlg::CClosureJointDlg(const CBridgeDescription2* pBridgeDesc,const CClosureKey& closureKey, const std::vector<EditSplicedGirderExtension>& editSplicedGirderExtensions,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T("Closure Joint Details"), pParentWnd, iSelectPage),
   m_ClosureKey(closureKey)
{
   m_bEditingInGirder = true;
   Init(pBridgeDesc,editSplicedGirderExtensions);
}

CClosureJointDlg::~CClosureJointDlg()
{
   DestroyExtensionPages();
}


INT_PTR CClosureJointDlg::DoModal()
{
   INT_PTR result = CPropertySheet::DoModal();
   if ( result == IDOK )
   {
      if ( 0 < m_SplicedGirderExtensionPages.size() )
      {
         NotifySplicedGirderExtensionPages();
      }
      else
      {
         NotifyExtensionPages();
      }
   }

   return result;
}

BEGIN_MESSAGE_MAP(CClosureJointDlg, CPropertySheet)
   WBFL_ON_PROPSHEET_OK
	ON_MESSAGE(WM_KICKIDLE,OnKickIdle)
END_MESSAGE_MAP()


// CClosureJointDlg message handlers
void CClosureJointDlg::CommonInit(const CBridgeDescription2* pBridgeDesc)
{
   m_bCopyToAllClosureJoints = false;

   m_psh.dwFlags                |= PSH_HASHELP | PSH_NOAPPLYNOW;
   m_General.m_psp.dwFlags      |= PSP_HASHELP;
   m_Longitudinal.m_psp.dwFlags |= PSP_HASHELP;
   m_Stirrups.m_psp.dwFlags     |= PSP_HASHELP;

   AddPage(&m_General);
   AddPage(&m_Longitudinal);
   AddPage(&m_Stirrups);

   // initialize the dialog data
   m_pBridgeDesc = pBridgeDesc;
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(m_ClosureKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(m_ClosureKey.girderIndex);
   const CClosureJointData* pClosureJoint = pGirder->GetClosureJoint(m_ClosureKey.segmentIndex);
   m_ClosureID = pClosureJoint->GetID();
   m_PierID = pClosureJoint->GetPierID();
   m_TempSupportID = pClosureJoint->GetTemporarySupportID();
   m_ClosureJoint = *pClosureJoint;
   
   m_TimelineMgr = *(pBridgeDesc->GetTimelineManager());
}

void CClosureJointDlg::Init(const CBridgeDescription2* pBridgeDesc)
{
   CommonInit(pBridgeDesc);
   CreateExtensionPages();
}

void CClosureJointDlg::Init(const CBridgeDescription2* pBridgeDesc,const std::vector<EditSplicedGirderExtension>& editSplicedGirderExtensions)
{
   CommonInit(pBridgeDesc);
   CreateExtensionPages(editSplicedGirderExtensions);
}

void CClosureJointDlg::CreateExtensionPages()
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   std::map<IDType,IEditClosureJointCallback*> callbacks = pDoc->GetEditClosureJointCallbacks();
   std::map<IDType,IEditClosureJointCallback*>::iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditClosureJointCallback*>::iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditClosureJointCallback* pCallback = callbackIter->second;
      CPropertyPage* pPage = pCallback->CreatePropertyPage(this);
      if ( pPage )
      {
         m_ExtensionPages.push_back( std::make_pair(pCallback,pPage) );
         AddPage(pPage);
      }
   }
}

void CClosureJointDlg::CreateExtensionPages(const std::vector<EditSplicedGirderExtension>& editSplicedGirderExtensions)
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   m_SplicedGirderExtensionPages = editSplicedGirderExtensions;
   std::sort(m_SplicedGirderExtensionPages.begin(), m_SplicedGirderExtensionPages.end());


   std::map<IDType,IEditClosureJointCallback*> callbacks = pDoc->GetEditClosureJointCallbacks();
   std::map<IDType,IEditClosureJointCallback*>::iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditClosureJointCallback*>::iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditClosureJointCallback* pEditClosureJointCallback = callbackIter->second;
      IDType editSplicedGirderCallbackID = pEditClosureJointCallback->GetEditSplicedGirderCallbackID();
      CPropertyPage* pPage = nullptr;
      if ( editSplicedGirderCallbackID == INVALID_ID )
      {
         pPage = pEditClosureJointCallback->CreatePropertyPage(this);
      }
      else
      {
         EditSplicedGirderExtension key;
         key.callbackID = editSplicedGirderCallbackID;
         std::vector<EditSplicedGirderExtension>::const_iterator found(std::find(m_SplicedGirderExtensionPages.begin(),m_SplicedGirderExtensionPages.end(),key));
         if ( found != m_SplicedGirderExtensionPages.end() )
         {
            const EditSplicedGirderExtension& extension = *found;
            CPropertyPage* pSplicedGirderPage = extension.pPage;
            pPage = pEditClosureJointCallback->CreatePropertyPage(this,pSplicedGirderPage);
         }
      }

      if ( pPage )
      {
         m_ExtensionPages.push_back( std::make_pair(pEditClosureJointCallback,pPage) );
         AddPage(pPage);
      }
   }
}

void CClosureJointDlg::DestroyExtensionPages()
{
   std::vector<std::pair<IEditClosureJointCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   std::vector<std::pair<IEditClosureJointCallback*,CPropertyPage*>>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      CPropertyPage* pPage = pageIter->second;
      delete pPage;
   }
   m_ExtensionPages.clear();
}

txnTransaction* CClosureJointDlg::GetExtensionPageTransaction()
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

void CClosureJointDlg::NotifySplicedGirderExtensionPages()
{
   // This gets called when this dialog is created from the spliced girder grid and it is closed with IDOK
   // It gives the spliced girder dialog extension pages to sync their data with whatever got changed in 
   // the extension pages in this dialog
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   std::map<IDType,IEditClosureJointCallback*> callbacks = pDoc->GetEditClosureJointCallbacks();
   std::map<IDType,IEditClosureJointCallback*>::iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditClosureJointCallback*>::iterator callbackIterEnd(callbacks.end());
   std::vector<std::pair<IEditClosureJointCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   for ( ; callbackIter != callbackIterEnd; callbackIter++, pageIter++ )
   {
      IEditClosureJointCallback* pCallback = callbackIter->second;
      IDType editSplicedGirderCallbackID = pCallback->GetEditSplicedGirderCallbackID();
      CPropertyPage* pClosureJointPage = pageIter->second;

      if ( editSplicedGirderCallbackID != INVALID_ID )
      {
         EditSplicedGirderExtension key;
         key.callbackID = editSplicedGirderCallbackID;
         std::vector<EditSplicedGirderExtension>::iterator found(std::find(m_SplicedGirderExtensionPages.begin(),m_SplicedGirderExtensionPages.end(),key));
         if ( found != m_SplicedGirderExtensionPages.end() )
         {
            EditSplicedGirderExtension& extension = *found;
            CPropertyPage* pSplicedGirderPage = extension.pPage;
            extension.pCallback->EditClosureJoint_OnOK(pSplicedGirderPage,pClosureJointPage);
         }
      }
   }
}

void CClosureJointDlg::NotifyExtensionPages()
{
   std::vector<std::pair<IEditClosureJointCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   std::vector<std::pair<IEditClosureJointCallback*,CPropertyPage*>>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      IEditClosureJointCallback* pCallback = pageIter->first;
      CPropertyPage* pPage = pageIter->second;
      txnTransaction* pTxn = pCallback->OnOK(pPage,this);
      if ( pTxn )
      {
         m_Macro.AddTransaction(pTxn);
      }
   }
}

BOOL CClosureJointDlg::OnInitDialog()
{
   BOOL bResult = CPropertySheet::OnInitDialog();


   // Build the OK button
   CWnd* pOK = GetDlgItem(IDOK);
   CRect rOK;
   pOK->GetWindowRect(&rOK);

   CRect wndPage;
   GetActivePage()->GetWindowRect(&wndPage);

   CRect rect;
   rect.left = wndPage.left;
   rect.top = rOK.top;
   rect.bottom = rOK.bottom;
   rect.right = rOK.left - 7;
   ScreenToClient(&rect);
   CString strTxt(m_bEditingInGirder ? _T("Copy to all closure joints in this girder") : _T("Copy to all closure joints at this support"));
   m_CheckBox.Create(strTxt,WS_CHILD | WS_VISIBLE | BS_LEFTTEXT | BS_RIGHT | BS_AUTOCHECKBOX,rect,this,IDC_CHECKBOX);
   m_CheckBox.SetFont(GetFont());

   UpdateData(FALSE); // calls DoDataExchange

   return bResult;
}

void CClosureJointDlg::DoDataExchange(CDataExchange* pDX)
{
   CPropertySheet::DoDataExchange(pDX);
   DDX_Check_Bool(pDX,IDC_CHECKBOX,m_bCopyToAllClosureJoints);
}

BOOL CClosureJointDlg::OnOK()
{
   UpdateData(TRUE);
   return FALSE; // MUST RETURN FALSE
}

LRESULT CClosureJointDlg::OnKickIdle(WPARAM wp, LPARAM lp)
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
