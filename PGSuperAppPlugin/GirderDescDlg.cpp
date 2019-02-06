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

// GirderDescDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperDocBase.h"
#include "GirderDescDlg.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\GirderHandling.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IDC_CHECKBOX 100

/////////////////////////////////////////////////////////////////////////////
// CGirderDescDlg

IMPLEMENT_DYNAMIC(CGirderDescDlg, CPropertySheet)

CGirderDescDlg::CGirderDescDlg(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T(""), pParentWnd, iSelectPage)
{
   CString strTitle;
   strTitle.Format(_T("Girder Details for Span %d, Girder %s"),LABEL_SPAN(segmentKey.groupIndex),LABEL_GIRDER(segmentKey.girderIndex));
   SetTitle(strTitle);

   Init(pBridgeDesc,segmentKey);
}

CGirderDescDlg::~CGirderDescDlg()
{
   DestroyExtensionPages();
}

bool CGirderDescDlg::HasDeck() const
{
   return IsStructuralDeck(m_DeckType);
}

INT_PTR CGirderDescDlg::DoModal()
{
   INT_PTR result = CPropertySheet::DoModal();
   if ( result == IDOK )
   {
      NotifyExtensionPages();
   }

   return result;
}

void CGirderDescDlg::Init(const CBridgeDescription2* pBridgeDesc,const CSegmentKey& segmentKey)
{
    m_SegmentKey = segmentKey;

   ATLASSERT(m_SegmentKey.segmentIndex == 0); // this is a PGSuper dialog

  m_bApplyToAll = false;

   m_psh.dwFlags |= PSH_HASHELP | PSH_NOAPPLYNOW;

   m_General.m_psp.dwFlags   |= PSP_HASHELP;
   m_Prestress.m_psp.dwFlags |= PSP_HASHELP;
   m_Shear.m_psp.dwFlags     |= PSP_HASHELP;
   m_LongRebar.m_psp.dwFlags |= PSP_HASHELP;
   m_Lifting.m_psp.dwFlags   |= PSP_HASHELP;
   m_Debond.m_psp.dwFlags    |= PSP_HASHELP;

   AddPage( &m_General );
   AddPage( &m_Prestress );

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   m_GirderSpacingType = pBridgeDesc->GetGirderSpacingType();

   m_DeckType = pBridgeDesc->GetDeckDescription()->GetDeckType();

   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);
   
   m_Girder = *pGirder;
   m_pSegment = m_Girder.GetSegment(segmentKey.segmentIndex);
   m_SegmentKey = segmentKey;
   m_SegmentID = pSegment->GetID();

   m_TimelineMgr = *(pBridgeDesc->GetTimelineManager());

   if( m_pSegment->Strands.GetStrandDefinitionType() == CStrandData::sdtDirectRowInput || m_pSegment->Strands.GetStrandDefinitionType() == CStrandData::sdtDirectStrandInput)
   {
      AddAdditionalPropertyPages( false, false );
   }
   else
   {
      GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);
      GET_IFACE2(pBroker,ISpecification,pSpec);
      GET_IFACE2(pBroker,ILibrary,pLib);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
      AddAdditionalPropertyPages( pSpecEntry->AllowStraightStrandExtensions(), pStrandGeom->CanDebondStrands(m_SegmentKey,pgsTypes::Straight) );
   }

   m_SpanGdrDetailsBearingsPage.m_psp.dwFlags |= PSP_HASHELP;
   AddPage(&m_SpanGdrDetailsBearingsPage);

   CreateExtensionPages();
}

void CGirderDescDlg::CreateExtensionPages()
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSDocBase* pDoc = (CPGSDocBase*)pEAFDoc;

   const std::map<IDType,IEditGirderCallback*>& callbacks = pDoc->GetEditGirderCallbacks();
   std::map<IDType,IEditGirderCallback*>::const_iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditGirderCallback*>::const_iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditGirderCallback* pCallback = callbackIter->second;
      CPropertyPage* pPage = pCallback->CreatePropertyPage(this);
      if ( pPage )
      {
         m_ExtensionPages.emplace_back(pCallback,pPage);
         AddPage(pPage);
      }
   }
}

void CGirderDescDlg::DestroyExtensionPages()
{
   std::vector<std::pair<IEditGirderCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   std::vector<std::pair<IEditGirderCallback*,CPropertyPage*>>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      CPropertyPage* pPage = pageIter->second;
      delete pPage;
   }
   m_ExtensionPages.clear();
}

txnTransaction* CGirderDescDlg::GetExtensionPageTransaction()
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

void CGirderDescDlg::NotifyExtensionPages()
{
   std::vector<std::pair<IEditGirderCallback*,CPropertyPage*>>::iterator pageIter(m_ExtensionPages.begin());
   std::vector<std::pair<IEditGirderCallback*,CPropertyPage*>>::iterator pageIterEnd(m_ExtensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      IEditGirderCallback* pCallback = pageIter->first;
      CPropertyPage* pPage = pageIter->second;
      txnTransaction* pTxn = pCallback->OnOK(pPage,this);
      if ( pTxn )
      {
         m_Macro.AddTransaction(pTxn);
      }
   }
}

BEGIN_MESSAGE_MAP(CGirderDescDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CGirderDescDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
      WBFL_ON_PROPSHEET_OK
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_KICKIDLE,OnKickIdle)
END_MESSAGE_MAP()

LRESULT CGirderDescDlg::OnKickIdle(WPARAM wp, LPARAM lp)
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
// CGirderDescDlg message handlers


void CGirderDescDlg::InitialzePages()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(m_SegmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(m_SegmentKey.girderIndex);
   m_Girder = *pGirder;
   m_pSegment = m_Girder.GetSegment(m_SegmentKey.segmentIndex);
   m_strGirderName = pGroup->GetGirderName(m_SegmentKey.girderIndex);

   m_ConditionFactorType = pGirder->GetConditionFactorType();
   m_ConditionFactor     = pGirder->GetConditionFactor();

   // Setup girder data for our pages
   m_General.m_bUseSameGirderType = pBridgeDesc->UseSameGirderForEntireBridge();
   m_General.m_SlabOffsetType = pBridgeDesc->GetSlabOffsetType();
   m_General.m_SlabOffset[pgsTypes::metStart] = m_pSegment->GetSlabOffset(pgsTypes::metStart);
   m_General.m_SlabOffset[pgsTypes::metEnd] = m_pSegment->GetSlabOffset(pgsTypes::metEnd);

   // assumed excess camber
   GET_IFACE2(pBroker,ISpecification, pSpec );
   m_bCanAssumedExcessCamberInputBeEnabled = pSpec->IsAssumedExcessCamberInputEnabled();

   m_General.m_AssumedExcessCamberType = m_bCanAssumedExcessCamberInputBeEnabled ? pIBridgeDesc->GetAssumedExcessCamberType() : pgsTypes::aecBridge;
   m_General.m_AssumedExcessCamber     =  m_bCanAssumedExcessCamberInputBeEnabled ? pIBridgeDesc->GetAssumedExcessCamber(m_SegmentKey.groupIndex,m_SegmentKey.girderIndex) : 0.0;

   // shear page
   m_Shear.m_CurGrdName = pGirder->GetGirderName();
   m_Shear.m_ShearData  = m_pSegment->ShearData;

   // longitudinal rebar page
   m_LongRebar.m_CurGrdName = m_Shear.m_CurGrdName;

   // Bearings
   const CPierData2* pStartPier = pGroup->GetPier(pgsTypes::metStart);
   const CPierData2* pEndPier = pGroup->GetPier(pgsTypes::metEnd);
   m_SpanGdrDetailsBearingsPage.Initialize(pBridgeDesc, pStartPier, pEndPier, m_SegmentKey.girderIndex);
}

BOOL CGirderDescDlg::OnOK()
{
   UpdateData(TRUE); // calls DoDataExchange
   return FALSE; // MUST RETURN FALSE!!!!
}

void CGirderDescDlg::DoDataExchange(CDataExchange* pDX)
{
   CPropertySheet::DoDataExchange(pDX);
   DDX_Check_Bool(pDX,IDC_CHECKBOX,m_bApplyToAll);
}

BOOL CGirderDescDlg::OnInitDialog() 
{
	InitialzePages();

	BOOL bResult = CPropertySheet::OnInitDialog();

   // Set the dialog icon
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_EDIT_GIRDER),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

   SetDebondTabName();

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
   m_CheckBox.Create(_T("Copy to all girders in this span"),WS_CHILD | WS_VISIBLE | BS_LEFTTEXT | BS_RIGHT | BS_AUTOCHECKBOX,rect,this,IDC_CHECKBOX);
   m_CheckBox.SetFont(GetFont());

   UpdateData(FALSE); // calls DoDataExchange

   return bResult;
}

void CGirderDescDlg::SetDebondTabName()
{
   int index = GetPageIndex(&m_Debond);
   if ( index < 0 )
      return; // not using the debond tab

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   CTabCtrl* pTab = GetTabControl();
   TC_ITEM ti;
   ti.mask = TCIF_TEXT;

   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);
   bool bCanDebond = pStrandGeometry->CanDebondStrands(m_strGirderName.c_str(),pgsTypes::Straight);
   if ( bCanDebond )
   {
      ti.pszText = _T("Debonding");
   }
   else
   {
      ti.pszText = _T("Strand Extensions");
   }
   
   pTab->SetItem(index,&ti);
}

StrandIndexType CGirderDescDlg::GetStraightStrandCount()
{
   return m_pSegment->Strands.GetStrandCount(pgsTypes::Straight);
}

StrandIndexType CGirderDescDlg::GetHarpedStrandCount()
{
   return m_pSegment->Strands.GetStrandCount(pgsTypes::Harped);
}

void CGirderDescDlg::SetSegment(const CPrecastSegmentData& segment)
{
   *m_pSegment = segment;
   m_Shear.m_ShearData = m_pSegment->ShearData;
}

const CPrecastSegmentData* CGirderDescDlg::GetSegment()
{
   m_pSegment->ShearData = m_Shear.m_ShearData;
   return m_pSegment;
}

void CGirderDescDlg::SetConditionFactor(pgsTypes::ConditionFactorType conditionFactorType,Float64 conditionFactor)
{
   m_ConditionFactorType = conditionFactorType;
   m_ConditionFactor     = conditionFactor;
}

pgsTypes::ConditionFactorType CGirderDescDlg::GetConditionFactorType()
{
   return m_ConditionFactorType;
}

Float64 CGirderDescDlg::GetConditionFactor()
{
   return m_ConditionFactor;
}

ConfigStrandFillVector CGirderDescDlg::ComputeStrandFillVector(pgsTypes::StrandType type)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeometry);

   if (m_pSegment->Strands.GetStrandDefinitionType() == CStrandData::sdtDirectSelection)
   {
      // first get in girderdata format
      const CDirectStrandFillCollection* pDirectFillData(nullptr);
      if (type==pgsTypes::Straight)
      {
         pDirectFillData = m_pSegment->Strands.GetDirectStrandFillStraight();
      }
      else if (type==pgsTypes::Harped)
      {
         pDirectFillData = m_pSegment->Strands.GetDirectStrandFillHarped();
      }
      if (type==pgsTypes::Temporary)
      {
         pDirectFillData = m_pSegment->Strands.GetDirectStrandFillTemporary();
      }

      // Convert girderdata to config
      // Start with unfilled grid 
      ConfigStrandFillVector vec(pStrandGeometry->ComputeStrandFill(m_strGirderName.c_str(), type, 0));
      StrandIndexType gridsize = vec.size();

      if(pDirectFillData!=nullptr)
      {
         CDirectStrandFillCollection::const_iterator it = pDirectFillData->begin();
         CDirectStrandFillCollection::const_iterator itend = pDirectFillData->end();
         while(it != itend)
         {
            StrandIndexType idx = it->permStrandGridIdx;
            if (idx < gridsize)
            {
               vec[idx] = it->numFilled;
            }
            else
            {
               ATLASSERT(false);
            }

            it++;
         }
      }

      return vec;
   }
   else
   {
      // Continuous fill
      StrandIndexType Ns = m_pSegment->Strands.GetStrandCount(type);

      return pStrandGeometry->ComputeStrandFill(m_strGirderName.c_str(), type, Ns);
   }
}

void CGirderDescDlg::AddAdditionalPropertyPages(bool bAllowExtendedStrands,bool bIsDebonding)
{
   if ( bAllowExtendedStrands || bIsDebonding )
   {
      AddPage( &m_Debond );
   }

   AddPage( &m_LongRebar );
   AddPage( &m_Shear );
   AddPage( &m_Lifting );
}

void CGirderDescDlg::OnGirderTypeChanged(bool bAllowExtendedStrands,bool bIsDebonding)
{
   // Remove all but the first two pages
   int nps = GetPageCount();
   for (int ip=nps-1; ip>1; ip--)
   {
      RemovePage(ip);
   }

   AddAdditionalPropertyPages(bAllowExtendedStrands,bIsDebonding);
   SetDebondTabName();

   std::vector<std::pair<IEditGirderCallback*,CPropertyPage*>>::iterator iter(m_ExtensionPages.begin());
   std::vector<std::pair<IEditGirderCallback*,CPropertyPage*>>::iterator end(m_ExtensionPages.end());
   for ( ; iter != end; iter++ )
   {
      AddPage(iter->second);
   }
}
