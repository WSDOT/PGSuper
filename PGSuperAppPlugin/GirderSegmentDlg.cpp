///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "GirderSegmentDlg.h"

#include <IFace\Project.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\Bridge.h>

#include <LRFD\RebarPool.h>


#include "PGSuperDocBase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define IDC_CHECKBOX 100

// CGirderSegmentDlg

IMPLEMENT_DYNAMIC(CGirderSegmentDlg, CPropertySheet)

CGirderSegmentDlg::CGirderSegmentDlg(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T(""), pParentWnd, iSelectPage)
{
   m_bEditingInGirder = false;
   Init(pBridgeDesc,segmentKey);
}

CGirderSegmentDlg::CGirderSegmentDlg(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey,const std::vector<EditSplicedGirderExtension>& editSplicedGirderExtensions,CWnd* pParentWnd, UINT iSelectPage) :
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
   m_TendonsPage.m_psp.dwFlags        |= PSP_HASHELP;
   m_RebarPage.m_psp.dwFlags          |= PSP_HASHELP;
   m_StirrupsPage.m_psp.dwFlags       |= PSP_HASHELP;
   m_LiftingPage.m_psp.dwFlags        |= PSP_HASHELP;

   AddPage(&m_GeneralPage);
   AddPage(&m_StrandsPage);
   AddPage(&m_TendonsPage);
   AddPage(&m_RebarPage);
   AddPage(&m_StirrupsPage);

   // initialize the dialog data
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);
   m_Group = *pGroup;
   m_Girder = *pGirder;
   m_Girder.SetGirderGroup(&m_Group);
   m_SegmentKey = segmentKey;
   m_SegmentID = pSegment->GetID();

   m_TimelineMgr = *(pBridgeDesc->GetTimelineManager());

   m_StrandsPage.Init(m_Girder.GetSegment(segmentKey.segmentIndex));
   m_TendonsPage.Init(m_Girder.GetSegment(segmentKey.segmentIndex));

   AddPage( &m_LiftingPage );
}

void CGirderSegmentDlg::Init(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey)
{
   CommonInit(pBridgeDesc,segmentKey);
   CreateExtensionPages();
}

void CGirderSegmentDlg::Init(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey,const std::vector<EditSplicedGirderExtension>& editSplicedGirderExtensions)
{
   CommonInit(pBridgeDesc,segmentKey);
   CreateExtensionPages(editSplicedGirderExtensions);
}

void CGirderSegmentDlg::CreateExtensionPages()
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   std::map<IDType,IEditSegmentCallback*> callbacks = pDoc->GetEditSegmentCallbacks();
   std::map<IDType,IEditSegmentCallback*>::iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditSegmentCallback*>::iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditSegmentCallback* pCallback = callbackIter->second;
      CPropertyPage* pPage = pCallback->CreatePropertyPage(this);
      if ( pPage )
      {
         m_ExtensionPages.emplace_back(pCallback,pPage);
         AddPage(pPage);
      }
   }
}

void CGirderSegmentDlg::CreateExtensionPages(const std::vector<EditSplicedGirderExtension>& editSplicedGirderExtensions)
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   m_SplicedGirderExtensionPages = editSplicedGirderExtensions;
   std::sort(m_SplicedGirderExtensionPages.begin(), m_SplicedGirderExtensionPages.end());


   std::map<IDType,IEditSegmentCallback*> callbacks = pDoc->GetEditSegmentCallbacks();
   std::map<IDType,IEditSegmentCallback*>::iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditSegmentCallback*>::iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditSegmentCallback* pEditSegmentCallback = callbackIter->second;
      IDType editSplicedGirderCallbackID = pEditSegmentCallback->GetEditSplicedGirderCallbackID();
      CPropertyPage* pPage = nullptr;
      if ( editSplicedGirderCallbackID == INVALID_ID )
      {
         pPage = pEditSegmentCallback->CreatePropertyPage(this);
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
            pPage = pEditSegmentCallback->CreatePropertyPage(this,pSplicedGirderPage);
         }
      }

      if ( pPage )
      {
         m_ExtensionPages.emplace_back(pEditSegmentCallback,pPage);
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

std::unique_ptr<CEAFTransaction> CGirderSegmentDlg::GetExtensionPageTransaction()
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

void CGirderSegmentDlg::NotifyExtensionPages()
{
   std::vector<std::pair<IEditSegmentCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   std::vector<std::pair<IEditSegmentCallback*,CPropertyPage*>>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      IEditSegmentCallback* pCallback = pageIter->first;
      CPropertyPage* pPage = pageIter->second;
      auto pTxn = pCallback->OnOK(pPage,this);
      if ( pTxn )
      {
         m_Macro.AddTransaction(std::move(pTxn));
      }
   }
}

void CGirderSegmentDlg::NotifySplicedGirderExtensionPages()
{
   // This gets called when this dialog is created from the spliced girder grid and it is closed with IDOK
   // It gives the spliced girder dialog extension pages to sync their data with whatever got changed in 
   // the extension pages in this dialog
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

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
         std::vector<EditSplicedGirderExtension>::iterator found(std::find(m_SplicedGirderExtensionPages.begin(),m_SplicedGirderExtensionPages.end(),key));
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
   if (pSegment->Strands.GetStrandDefinitionType() == pgsTypes::sdtDirectSelection)
   {
      // first get in girderdata format
      const CDirectStrandFillCollection* pDirectFillData(nullptr);
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

      if(pDirectFillData!=nullptr)
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
               ATLASSERT(false); 

            it++;
         }
      }

      return vec;
   }
   else
   {
      // Continuous fill
      StrandIndexType Ns = pSegment->Strands.GetStrandCount(type);

      return pStrandGeometry->ComputeStrandFill(m_SegmentKey, type, Ns);
   }
}

BOOL CGirderSegmentDlg::OnInitDialog()
{
   BOOL bResult = CPropertySheet::OnInitDialog();

   CString strTitle;
   strTitle.Format(_T("Segment Details for Group %d Girder %s Segment %d"),LABEL_GROUP(m_SegmentKey.groupIndex),LABEL_GIRDER(m_SegmentKey.girderIndex),LABEL_SEGMENT(m_SegmentKey.segmentIndex));
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
   {
      strTxt = _T("Copy to all segments in this girder");
   }
   else
   {
      strTxt.Format(_T("Copy to Segment %d of all girders in Group %d"),LABEL_SEGMENT(m_SegmentKey.segmentIndex),LABEL_GROUP(m_SegmentKey.groupIndex));
   }

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
