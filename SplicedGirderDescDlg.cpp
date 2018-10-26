///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

// SplicedGirderDescDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\Resource.h"
#include "PGSuperDocBase.h"
#include "SplicedGirderDescDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IDC_CHECKBOX 100

/////////////////////////////////////////////////////////////////////////////
// CSplicedGirderDescDlg

IMPLEMENT_DYNAMIC(CSplicedGirderDescDlg, CPropertySheet)

CSplicedGirderDescDlg::CSplicedGirderDescDlg(const CGirderKey& girderKey,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T(""), pParentWnd, iSelectPage)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   m_BridgeDescription = *pBridgeDesc;
   m_pGirder = m_BridgeDescription.GetGirderGroup(girderKey.groupIndex)->GetGirder(girderKey.girderIndex);
   m_GirderKey = girderKey;
   m_GirderID = m_pGirder->GetID();

   CString strTitle;
   strTitle.Format(_T("Girder Details for Group %d, Girder %s"),LABEL_GROUP(m_GirderKey.groupIndex),LABEL_GIRDER(m_GirderKey.girderIndex));
   SetTitle(strTitle);

   Init();
}

CSplicedGirderDescDlg::~CSplicedGirderDescDlg()
{
   DestroyExtensionPages();
}

INT_PTR CSplicedGirderDescDlg::DoModal()
{
   INT_PTR result = CPropertySheet::DoModal();
   if ( result == IDOK )
   {
      NotifyExtensionPages();
   }

   return result;
}

void CSplicedGirderDescDlg::Init()
{
   m_bApplyToAll = false;

   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;

   m_General.m_psp.dwFlags   |= PSP_HASHELP;

   AddPage( &m_General );

   CreateExtensionPages();
}

void CSplicedGirderDescDlg::CreateExtensionPages()
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   std::map<IDType,IEditSplicedGirderCallback*> callbacks = pDoc->GetEditSplicedGirderCallbacks();
   std::map<IDType,IEditSplicedGirderCallback*>::iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditSplicedGirderCallback*>::iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditSplicedGirderCallback* pCallback = callbackIter->second;
      CPropertyPage* pPage = pCallback->CreatePropertyPage(this);
      if ( pPage )
      {
         EditSplicedGirderExtension extension;
         extension.callbackID = callbackIter->first;
         extension.pCallback = pCallback;
         extension.pPage = pPage;
         m_ExtensionPages.insert(extension);
         AddPage(pPage);
      }
   }
}

void CSplicedGirderDescDlg::DestroyExtensionPages()
{
   std::set<EditSplicedGirderExtension>::iterator extIter(m_ExtensionPages.begin());
   std::set<EditSplicedGirderExtension>::iterator extIterEnd(m_ExtensionPages.end());
   for ( ; extIter != extIterEnd; extIter++ )
   {
      CPropertyPage* pPage = extIter->pPage;
      delete pPage;
   }
   m_ExtensionPages.clear();
}

txnTransaction* CSplicedGirderDescDlg::GetExtensionPageTransaction()
{
   if ( 0 < m_Macro.GetTxnCount() )
      return m_Macro.CreateClone();
   else
      return NULL;
}

void CSplicedGirderDescDlg::NotifyExtensionPages()
{
   std::set<EditSplicedGirderExtension>::iterator pageIter(m_ExtensionPages.begin());
   std::set<EditSplicedGirderExtension>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      IEditSplicedGirderCallback* pCallback = pageIter->pCallback;
      CPropertyPage* pPage = pageIter->pPage;
      txnTransaction* pTxn = pCallback->OnOK(pPage,this);
      if ( pTxn )
      {
         m_Macro.AddTransaction(pTxn);
      }
   }
}

const std::set<EditSplicedGirderExtension>& CSplicedGirderDescDlg::GetExtensionPages() const
{
   return m_ExtensionPages;
}

std::set<EditSplicedGirderExtension>& CSplicedGirderDescDlg::GetExtensionPages()
{
   return m_ExtensionPages;
}

BEGIN_MESSAGE_MAP(CSplicedGirderDescDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CGirderDescDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
      WBFL_ON_PROPSHEET_OK
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_KICKIDLE,OnKickIdle)
END_MESSAGE_MAP()

LRESULT CSplicedGirderDescDlg::OnKickIdle(WPARAM wp, LPARAM lp)
{
   // The CPropertySheet::OnKickIdle method calls GetActivePage()
   // which doesn't work with extension pages. Since GetActivePage
   // is not virtual, we have to replace the implementation of
   // OnKickIdle.
   // The same problem exists with OnCommandHelp

	ASSERT_VALID(this);

	CPropertyPage* pPage = GetPage(GetActiveIndex());

	/* Forward the message on to the active page of the property sheet */
	if( pPage != NULL )
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
// CSplicedGirderDescDlg message handlers
//
//
#pragma Reminder("UPDATE: this may be needed in CSplicedGirderGeneralPage")
// A couple DoUpdate calls were removed from CSplicedGirderGeneralPage when it was moved from this dialog to a property sheet
// To figure out where the DoUpdate calls should be, get the history of this file and then map the DoUpdate calls into
// CSplicedGirderGeneralPage
//void CGirderDescDlg::DoUpdate()
//{
//   CComPtr<IBroker> pBroker;
//   EAFGetBroker(&pBroker);
//
//   GET_IFACE2(pBroker,IShear,pShear);
//   GET_IFACE2(pBroker,ILongitudinalRebar,pLongitudinaRebar);
//   GET_IFACE2(pBroker,ISegmentLifting,pSegmentLifting);
//   GET_IFACE2(pBroker,ISegmentHauling,pSegmentHauling);
//   GET_IFACE2(pBroker,IBridge,pBridge);
//   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
//
//   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
//   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(m_SegmentKey.groupIndex);
//   const CSplicedGirderData* pGirder = pGroup->GetGirder(m_SegmentKey.girderIndex);
//   m_Segment = *pGirder->GetSegment(m_SegmentKey.segmentIndex);
//   m_strGirderName = pGroup->GetGirderName(m_SegmentKey.girderIndex);
//
//   m_ConditionFactorType = pGirder->GetConditionFactorType();
//   m_ConditionFactor     = pGirder->GetConditionFactor();
//
//   // Setup girder data for our pages
//   m_General.m_bUseSameGirderType = pBridgeDesc->UseSameGirderForEntireBridge();
//   m_General.m_SlabOffsetType = pBridgeDesc->GetSlabOffsetType();
//   m_General.m_SlabOffset[pgsTypes::metStart] = pGroup->GetSlabOffset(pGroup->GetPierIndex(pgsTypes::metStart),m_SegmentKey.girderIndex);
//   m_General.m_SlabOffset[pgsTypes::metEnd]   = pGroup->GetSlabOffset(pGroup->GetPierIndex(pgsTypes::metEnd),  m_SegmentKey.girderIndex);
//
//   // shear page
//   m_Shear.m_CurGrdName = pGirder->GetGirderName();
//   m_Shear.m_ShearData  = m_Segment.ShearData;
//
//   // longitudinal rebar page
//   m_LongRebar.m_CurGrdName = m_Shear.m_CurGrdName;
//}

BOOL CSplicedGirderDescDlg::OnOK()
{
   UpdateData(TRUE); // calls DoDataExchange
   return FALSE; // MUST RETURN FALSE!!!!
}

void CSplicedGirderDescDlg::DoDataExchange(CDataExchange* pDX)
{
   CPropertySheet::DoDataExchange(pDX);
   DDX_Check_Bool(pDX,IDC_CHECKBOX,m_bApplyToAll);
}

BOOL CSplicedGirderDescDlg::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();

   // Set the dialog icon
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_EDIT_GIRDER),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

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
   m_CheckBox.Create(_T("Copy to all girders in this group"),WS_CHILD | WS_VISIBLE | BS_LEFTTEXT | BS_RIGHT | BS_AUTOCHECKBOX,rect,this,IDC_CHECKBOX);
   m_CheckBox.SetFont(GetFont());

   UpdateData(FALSE); // calls DoDataExchange

   return bResult;
}
