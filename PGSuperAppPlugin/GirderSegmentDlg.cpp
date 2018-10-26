///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

// GirderSegmentDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "GirderSegmentDlg.h"

#include <IFace\Project.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Bridge.h>

#include <LRFD\RebarPool.h>

#include <PgsExt\BridgeDescription2.h>

#include "PGSuperDocBase.h"

#define IDC_CHECKBOX 100

// CGirderSegmentDlg

IMPLEMENT_DYNAMIC(CGirderSegmentDlg, CPropertySheet)

CGirderSegmentDlg::CGirderSegmentDlg(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T(""), pParentWnd, iSelectPage)
{
   m_bEditingInGirder = false;
   Init(pBridgeDesc,segmentKey);
}

CGirderSegmentDlg::CGirderSegmentDlg(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey,const std::set<EditSplicedGirderExtension>& editSplicedGirderExtensions,CWnd* pParentWnd, UINT iSelectPage) :
CPropertySheet(_T(""),pParentWnd,iSelectPage)
{
   m_bEditingInGirder = true;
   Init(pBridgeDesc,segmentKey,editSplicedGirderExtensions);
}

CGirderSegmentDlg::~CGirderSegmentDlg()
{
   DestroyExtensionPages();
}

INT_PTR CGirderSegmentDlg::DoModal()
{
   INT_PTR result = CPropertySheet::DoModal();
   if ( result == IDOK )
   {
      if ( 0 < m_SplicedGirderExtensionPages.size() )
         NotifySplicedGirderExtensionPages();
      else
         NotifyExtensionPages();
   }

   return result;
}


BEGIN_MESSAGE_MAP(CGirderSegmentDlg, CPropertySheet)
   WBFL_ON_PROPSHEET_OK
	ON_MESSAGE(WM_KICKIDLE,OnKickIdle)
END_MESSAGE_MAP()


// CGirderSegmentDlg message handlers
void CGirderSegmentDlg::CommonInit(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey)
{
   m_bCopyToAll = false;

   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;

   m_GeneralPage.m_psp.dwFlags        |= PSP_HASHELP;
   m_StrandsPage.m_psp.dwFlags        |= PSP_HASHELP;
   m_RebarPage.m_psp.dwFlags          |= PSP_HASHELP;
   m_StirrupsPage.m_psp.dwFlags       |= PSP_HASHELP;
   m_LiftingPage.m_psp.dwFlags        |= PSP_HASHELP;

   AddPage(&m_GeneralPage);
   AddPage(&m_StrandsPage);
   AddPage(&m_RebarPage);
   AddPage(&m_StirrupsPage);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   // initialize the dialog data
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);
   m_Girder = *pGirder;
   m_SegmentKey = segmentKey;
   m_SegmentID = pSegment->GetID();

   GET_IFACE2(pBroker,IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   GET_IFACE2(pBroker,IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);

   // don't add page if both hauling and lifting checks are disabled
   if (pGirderLiftingSpecCriteria->IsLiftingAnalysisEnabled() || pGirderHaulingSpecCriteria->IsHaulingAnalysisEnabled())
   {
      AddPage( &m_LiftingPage );
   }
}

void CGirderSegmentDlg::Init(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey)
{
   CommonInit(pBridgeDesc,segmentKey);
   CreateExtensionPages();
}

void CGirderSegmentDlg::Init(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey,const std::set<EditSplicedGirderExtension>& editSplicedGirderExtensions)
{
   CommonInit(pBridgeDesc,segmentKey);
   CreateExtensionPages(editSplicedGirderExtensions);
}

void CGirderSegmentDlg::CreateExtensionPages()
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)pEAFDoc;

   std::map<IDType,IEditSegmentCallback*> callbacks = pDoc->GetEditSegmentCallbacks();
   std::map<IDType,IEditSegmentCallback*>::iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditSegmentCallback*>::iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditSegmentCallback* pCallback = callbackIter->second;
      CPropertyPage* pPage = pCallback->CreatePropertyPage(this);
      if ( pPage )
      {
         m_ExtensionPages.push_back( std::make_pair(pCallback,pPage) );
         AddPage(pPage);
      }
   }
}

void CGirderSegmentDlg::CreateExtensionPages(const std::set<EditSplicedGirderExtension>& editSplicedGirderExtensions)
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)pEAFDoc;

   m_SplicedGirderExtensionPages = editSplicedGirderExtensions;


   std::map<IDType,IEditSegmentCallback*> callbacks = pDoc->GetEditSegmentCallbacks();
   std::map<IDType,IEditSegmentCallback*>::iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditSegmentCallback*>::iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditSegmentCallback* pEditSegmentCallback = callbackIter->second;
      IDType editSplicedGirderCallbackID = pEditSegmentCallback->GetEditSplicedGirderCallbackID();
      CPropertyPage* pPage = NULL;
      if ( editSplicedGirderCallbackID == INVALID_ID )
      {
         pPage = pEditSegmentCallback->CreatePropertyPage(this);
      }
      else
      {
         EditSplicedGirderExtension key;
         key.callbackID = editSplicedGirderCallbackID;
         std::set<EditSplicedGirderExtension>::const_iterator found(m_SplicedGirderExtensionPages.find(key));
         if ( found != m_SplicedGirderExtensionPages.end() )
         {
            const EditSplicedGirderExtension& extension = *found;
            CPropertyPage* pSplicedGirderPage = extension.pPage;
            pPage = pEditSegmentCallback->CreatePropertyPage(this,pSplicedGirderPage);
         }
      }

      if ( pPage )
      {
         m_ExtensionPages.push_back( std::make_pair(pEditSegmentCallback,pPage) );
         AddPage(pPage);
      }
   }
}

void CGirderSegmentDlg::DestroyExtensionPages()
{
   std::vector<std::pair<IEditSegmentCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   std::vector<std::pair<IEditSegmentCallback*,CPropertyPage*>>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      CPropertyPage* pPage = pageIter->second;
      delete pPage;
   }
   m_ExtensionPages.clear();
}

txnTransaction* CGirderSegmentDlg::GetExtensionPageTransaction()
{
   if ( 0 < m_Macro.GetTxnCount() )
      return m_Macro.CreateClone();
   else
      return NULL;
}

void CGirderSegmentDlg::NotifyExtensionPages()
{
   std::vector<std::pair<IEditSegmentCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   std::vector<std::pair<IEditSegmentCallback*,CPropertyPage*>>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      IEditSegmentCallback* pCallback = pageIter->first;
      CPropertyPage* pPage = pageIter->second;
      txnTransaction* pTxn = pCallback->OnOK(pPage,this);
      if ( pTxn )
      {
         m_Macro.AddTransaction(pTxn);
      }
   }
}

void CGirderSegmentDlg::NotifySplicedGirderExtensionPages()
{
   // This gets called when this dialog is created from the spliced girder grid and it is closed with IDOK
   // It gives the spliced girder dialog extension pages to sync their data with whatever got changed in 
   // the extension pages in this dialog
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSuperDocBase* pDoc = (CPGSuperDocBase*)pEAFDoc;

   std::map<IDType,IEditSegmentCallback*> callbacks = pDoc->GetEditSegmentCallbacks();
   std::map<IDType,IEditSegmentCallback*>::iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditSegmentCallback*>::iterator callbackIterEnd(callbacks.end());
   std::vector<std::pair<IEditSegmentCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   for ( ; callbackIter != callbackIterEnd; callbackIter++, pageIter++ )
   {
      IEditSegmentCallback* pCallback = callbackIter->second;
      IDType editSplicedGirderCallbackID = pCallback->GetEditSplicedGirderCallbackID();
      CPropertyPage* pSegmentPage = pageIter->second;

      if ( editSplicedGirderCallbackID != INVALID_ID )
      {
         EditSplicedGirderExtension key;
         key.callbackID = editSplicedGirderCallbackID;
         std::set<EditSplicedGirderExtension>::iterator found(m_SplicedGirderExtensionPages.find(key));
         if ( found != m_SplicedGirderExtensionPages.end() )
         {
            EditSplicedGirderExtension& extension = *found;
            CPropertyPage* pSplicedGirderPage = extension.pPage;
            extension.pCallback->EditSegment_OnOK(pSplicedGirderPage,pSegmentPage);
         }
      }
   }
}

ConfigStrandFillVector CGirderSegmentDlg::ComputeStrandFillVector(pgsTypes::StrandType type)
{
#pragma Reminder("UPDATE: this method is a duplicate of CGirderDescDlg")
   // find a way to make it one function

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   CPrecastSegmentData* pSegment = m_Girder.GetSegment(m_SegmentKey.segmentIndex);
   if (pSegment->Strands.NumPermStrandsType == CStrandData::npsDirectSelection)
   {
      // first get in girderdata format
      const CDirectStrandFillCollection* pDirectFillData(NULL);
      if (type==pgsTypes::Straight)
      {
         pDirectFillData = pSegment->Strands.GetDirectStrandFillStraight();
      }
      else if (type==pgsTypes::Harped)
      {
         pDirectFillData = pSegment->Strands.GetDirectStrandFillHarped();
      }
      if (type==pgsTypes::Temporary)
      {
         pDirectFillData = pSegment->Strands.GetDirectStrandFillTemporary();
      }

      // Convert girderdata to config
      // Start with unfilled grid 
      ConfigStrandFillVector vec(pStrandGeometry->ComputeStrandFill(m_Girder.GetGirderName(), type, 0));
      StrandIndexType gridsize = vec.size();

      if(pDirectFillData!=NULL)
      {
         CDirectStrandFillCollection::const_iterator it = pDirectFillData->begin();
         CDirectStrandFillCollection::const_iterator itend = pDirectFillData->end();
         while(it != itend)
         {
            StrandIndexType idx = it->permStrandGridIdx;
            if (idx<gridsize)
            {
               vec[idx] = it->numFilled;
            }
            else
               ATLASSERT(0); 

            it++;
         }
      }

      return vec;
   }
   else
   {
      // Continuous fill
      StrandIndexType Ns = pSegment->Strands.GetNstrands(type);

      return pStrandGeometry->ComputeStrandFill(m_SegmentKey, type, Ns);
   }
}

BOOL CGirderSegmentDlg::OnInitDialog()
{
   BOOL bResult = CPropertySheet::OnInitDialog();

   CString strTitle;
   strTitle.Format(_T("Group %d Girder %s Segment %d"),LABEL_GROUP(m_SegmentKey.groupIndex),LABEL_GIRDER(m_SegmentKey.girderIndex),LABEL_SEGMENT(m_SegmentKey.segmentIndex));
   SetWindowText(strTitle);

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
   CString strTxt;
   if ( m_bEditingInGirder )
      strTxt = _T("Copy to all segments in this girder");
   else
      strTxt.Format(_T("Copy to Segment %d of all girders in Group %d"),LABEL_SEGMENT(m_SegmentKey.segmentIndex),LABEL_GROUP(m_SegmentKey.groupIndex));

   m_CheckBox.Create(strTxt,WS_CHILD | WS_VISIBLE | BS_LEFTTEXT | BS_RIGHT | BS_AUTOCHECKBOX,rect,this,IDC_CHECKBOX);
   m_CheckBox.SetFont(GetFont());

   UpdateData(FALSE); // calls DoDataExchange

   return bResult;
}

void CGirderSegmentDlg::DoDataExchange(CDataExchange* pDX)
{
   CPropertySheet::DoDataExchange(pDX);
   DDX_Check_Bool(pDX,IDC_CHECKBOX,m_bCopyToAll);
}

BOOL CGirderSegmentDlg::OnOK()
{
   UpdateData(TRUE);

   return FALSE; // MUST RETURN FALSE
}

LRESULT CGirderSegmentDlg::OnKickIdle(WPARAM wp, LPARAM lp)
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
		return 0;
}
