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

// SpanDetailsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "SpanDetailsDlg.h"

#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpanDetailsDlg

IMPLEMENT_DYNAMIC(CSpanDetailsDlg, CPropertySheet)

CSpanDetailsDlg::CSpanDetailsDlg(const CSpanData* pSpan,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T(""), pParentWnd, iSelectPage)
{
   ASSERT(pSpan != NULL);
   SetSpanData(pSpan);
   Init();
}

CSpanDetailsDlg::CSpanDetailsDlg(const CSpanData* pSpan,const std::set<EditBridgeExtension>& editBridgeExtensions,CWnd* pParentWnd, UINT iSelectPage)
{
   ASSERT(pSpan != NULL);
   SetSpanData(pSpan);
   Init(editBridgeExtensions);
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
      if ( 0 < m_BridgeExtensionPages.size() )
         NotifyBridgeExtensionPages();
      else
         NotifyExtensionPages();
   }

   return result;
}

void CSpanDetailsDlg::SetSpanData(const CSpanData* pSpan)
{
   m_pBridgeDesc = pSpan->GetBridgeDescription();
   m_pPrevPier = pSpan->GetPrevPier();
   m_pSpanData = pSpan;
   m_pNextPier = pSpan->GetNextPier();

   m_ConnectionType[pgsTypes::metStart] = m_pPrevPier->GetConnectionType();
   m_ConnectionType[pgsTypes::metEnd]  = m_pNextPier->GetConnectionType();
   m_SpanLayoutPage.Init(pSpan);
   m_GirderLayoutPage.Init(pSpan);

   m_StartPierPage.Init(m_pPrevPier);
   m_EndPierPage.Init(m_pNextPier);
   m_GirderLayoutPage.Init(pSpan);

   // Set dialog title
   CString strTitle;
   strTitle.Format(_T("Span %d Details"),LABEL_SPAN(pSpan->GetSpanIndex()));
   SetTitle(strTitle);


   CString strStartPierLabel(m_pPrevPier->GetPrevSpan() == NULL ? _T("Abut.") : _T("Pier"));
   m_strStartPierTitle.Format(_T("%s %d Connections"),strStartPierLabel,LABEL_PIER(m_pPrevPier->GetPierIndex()));
   m_StartPierPage.m_psp.dwFlags |= PSP_USETITLE;
   m_StartPierPage.m_psp.pszTitle = m_strStartPierTitle.GetBuffer();

   CString strEndPierLabel(m_pNextPier->GetNextSpan() == NULL ? _T("Abut.") : _T("Pier"));
   m_strEndPierTitle.Format(_T("%s %d Connections"),strEndPierLabel,LABEL_PIER(m_pNextPier->GetPierIndex()));
   m_EndPierPage.m_psp.dwFlags |= PSP_USETITLE;
   m_EndPierPage.m_psp.pszTitle = m_strEndPierTitle.GetBuffer();
}

pgsTypes::PierConnectionType CSpanDetailsDlg::GetConnectionType(PierIndexType pierIdx)
{
   if ( m_pPrevPier->GetPierIndex() == pierIdx )
      return m_ConnectionType[pgsTypes::metStart];
   else
      return m_ConnectionType[pgsTypes::metEnd];
}

void CSpanDetailsDlg::SetConnectionType(PierIndexType pierIdx,pgsTypes::PierConnectionType type)
{
   if ( m_pPrevPier->GetPierIndex() == pierIdx )
      m_ConnectionType[pgsTypes::metStart] = type;
   else
      m_ConnectionType[pgsTypes::metEnd] = type;
}

const CSpanData* CSpanDetailsDlg::GetPrevSpan(PierIndexType pierIdx)
{
   if ( m_pPrevPier->GetPierIndex() == pierIdx )
      return m_pPrevPier->GetPrevSpan();
   else
      return m_pNextPier->GetPrevSpan();
}

const CSpanData* CSpanDetailsDlg::GetNextSpan(PierIndexType pierIdx)
{
   if ( m_pPrevPier->GetPierIndex() == pierIdx )
      return m_pPrevPier->GetNextSpan();
   else
      return m_pNextPier->GetNextSpan();
}

const CBridgeDescription* CSpanDetailsDlg::GetBridgeDescription()
{
   return m_pBridgeDesc;
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
	if( pPage != NULL )
	{
		//ASSERT_VALID(pPage);
		return pPage->SendMessage( WM_KICKIDLE, wp, lp );
	}
	else
		return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CSpanDetailsDlg message handlers
void CSpanDetailsDlg::CommonInit()
{
   m_psh.dwFlags                       |= PSH_HASHELP | PSH_NOAPPLYNOW;

   m_SpanLayoutPage.m_psp.dwFlags      |= PSP_HASHELP;
   AddPage(&m_SpanLayoutPage);

   m_StartPierPage.m_psp.dwFlags |= PSP_HASHELP;
   AddPage(&m_StartPierPage);

   m_EndPierPage.m_psp.dwFlags |= PSP_HASHELP;
   AddPage(&m_EndPierPage);

   m_GirderLayoutPage.m_psp.dwFlags |= PSP_HASHELP;
   AddPage(&m_GirderLayoutPage);
}

void CSpanDetailsDlg::Init()
{
   CommonInit();
   CreateExtensionPages();
}

void CSpanDetailsDlg::Init(const std::set<EditBridgeExtension>& editBridgeExtensions)
{
   CommonInit();
   CreateExtensionPages(editBridgeExtensions);
}

void CSpanDetailsDlg::CreateExtensionPages()
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSuperDoc* pDoc = (CPGSuperDoc*)pEAFDoc;

   const std::map<IDType,IEditSpanCallback*>& callbacks = pDoc->GetEditSpanCallbacks();
   std::map<IDType,IEditSpanCallback*>::const_iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditSpanCallback*>::const_iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditSpanCallback* pCallback = callbackIter->second;
      CPropertyPage* pPage = pCallback->CreatePropertyPage(this);
      if ( pPage )
      {
         m_ExtensionPages.push_back( std::make_pair(pCallback,pPage) );
         AddPage(pPage);
      }
   }
}

void CSpanDetailsDlg::CreateExtensionPages(const std::set<EditBridgeExtension>& editBridgeExtensions)
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSuperDoc* pDoc = (CPGSuperDoc*)pEAFDoc;

   m_BridgeExtensionPages = editBridgeExtensions;

   const std::map<IDType,IEditSpanCallback*>& callbacks = pDoc->GetEditSpanCallbacks();
   std::map<IDType,IEditSpanCallback*>::const_iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditSpanCallback*>::const_iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditSpanCallback* pEditSpanCallback = callbackIter->second;
      IDType editBridgeCallbackID = pEditSpanCallback->GetEditBridgeCallbackID();
      CPropertyPage* pPage = NULL;
      if ( editBridgeCallbackID == INVALID_ID )
      {
         pPage = pEditSpanCallback->CreatePropertyPage(this);
      }
      else
      {
         EditBridgeExtension key;
         key.callbackID = editBridgeCallbackID;
         std::set<EditBridgeExtension>::const_iterator found(m_BridgeExtensionPages.find(key));
         if ( found != m_BridgeExtensionPages.end() )
         {
            const EditBridgeExtension& extension = *found;
            CPropertyPage* pBridgePage = extension.pPage;
            pPage = pEditSpanCallback->CreatePropertyPage(this,pBridgePage);
         }
      }

      if ( pPage )
      {
         m_ExtensionPages.push_back( std::make_pair(pEditSpanCallback,pPage) );
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
   if ( 0 < m_Macro.GetTxnCount() )
      return m_Macro.CreateClone();
   else
      return NULL;
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
         std::set<EditBridgeExtension>::iterator found(m_BridgeExtensionPages.find(key));
         if ( found != m_BridgeExtensionPages.end() )
         {
            EditBridgeExtension& extension = *found;
            CPropertyPage* pBridgePage = extension.pPage;
            extension.pCallback->EditSpan_OnOK(pBridgePage,pSpanPage);
         }
      }
   }
}


txnEditSpanData CSpanDetailsDlg::GetEditSpanData()
{
   txnEditSpanData editSpanData(m_pSpanData); // initialize with current data

   // General Layout
   editSpanData.SpanLength = GetSpanLength();

   // Spacing
   editSpanData.nGirders                           = GetGirderCount();
   editSpanData.bSameNumberOfGirdersInAllSpans     = UseSameNumGirders();
   editSpanData.bSameGirderType                    = UseSameGirderType();
   editSpanData.GirderSpacingType                  = GetGirderSpacingType();
   editSpanData.GirderMeasurementLocation          = GetMeasurementLocation();
   editSpanData.GirderTypes                        = GetGirderTypes();

   // more spacing below

   // Connections and Spacing
   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::MemberEndType end = (i == 0 ? pgsTypes::metStart : pgsTypes::metEnd);

      // Boundary conditions
      editSpanData.m_ConnectionType[end] = GetConnectionType(end);

      // Spacing
      editSpanData.GirderSpacing[end == pgsTypes::metStart ? pgsTypes::Ahead : pgsTypes::Back] = GetGirderSpacing(end);

      // Connections
      for ( int j = 0; j < 2; j++ )
      {
         pgsTypes::PierFaceType face = (j == 0 ? pgsTypes::Back : pgsTypes::Ahead);
         editSpanData.m_EndDistanceMeasurementType[end][face]   = GetEndDistanceMeasurementType(end,face);
         editSpanData.m_EndDistance[end][face]                  = GetEndDistance(end,face);
         editSpanData.m_BearingOffsetMeasurementType[end][face] = GetBearingOffsetMeasurementType(end,face);
         editSpanData.m_BearingOffset[end][face]                = GetBearingOffset(end,face);
         editSpanData.m_SupportWidth[end][face]                 = GetSupportWidth(end,face);

         // Connection
         editSpanData.m_DiaphragmHeight[end][face]       = GetDiaphragmHeight(end,face);
         editSpanData.m_DiaphragmWidth[end][face]        = GetDiaphragmWidth(end,face);
         editSpanData.m_DiaphragmLoadType[end][face]     = GetDiaphragmLoadType(end,face);
         editSpanData.m_DiaphragmLoadLocation[end][face] = GetDiaphragmLoadLocation(end,face);
      }
   }

   editSpanData.SlabOffsetType = GetSlabOffsetType();
   editSpanData.SlabOffset[pgsTypes::metStart] = GetSlabOffset(pgsTypes::metStart);
   editSpanData.SlabOffset[pgsTypes::metEnd]   = GetSlabOffset(pgsTypes::metEnd);

   return editSpanData;
}

Float64 CSpanDetailsDlg::GetSpanLength()
{
   return m_SpanLayoutPage.GetSpanLength();
}

pgsTypes::PierConnectionType CSpanDetailsDlg::GetConnectionType(pgsTypes::MemberEndType end)
{
   return m_ConnectionType[end];
}


Float64 CSpanDetailsDlg::GetDiaphragmHeight(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face)
{
   if ( end == pgsTypes::metStart )
      return m_StartPierPage.m_DiaphragmHeight[face];
   else
      return m_EndPierPage.m_DiaphragmHeight[face];
}

Float64 CSpanDetailsDlg::GetDiaphragmWidth(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face)
{
   if ( end == pgsTypes::metStart )
      return m_StartPierPage.m_DiaphragmWidth[face];
   else
      return m_EndPierPage.m_DiaphragmWidth[face];
}

ConnectionLibraryEntry::DiaphragmLoadType CSpanDetailsDlg::GetDiaphragmLoadType(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face)
{
   if ( end == pgsTypes::metStart )
      return m_StartPierPage.m_DiaphragmLoadType[face];
   else
      return m_EndPierPage.m_DiaphragmLoadType[face];
}

Float64 CSpanDetailsDlg::GetDiaphragmLoadLocation(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face)
{
   if ( end == pgsTypes::metStart )
      return m_StartPierPage.m_DiaphragmLoadLocation[face];
   else
      return m_EndPierPage.m_DiaphragmLoadLocation[face];
}

ConnectionLibraryEntry::EndDistanceMeasurementType CSpanDetailsDlg::GetEndDistanceMeasurementType(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face)
{
   if ( end == pgsTypes::metStart )
      return m_StartPierPage.m_EndDistanceMeasurementType;
   else
      return m_EndPierPage.m_EndDistanceMeasurementType;
}

Float64 CSpanDetailsDlg::GetEndDistance(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face)
{
   if ( end == pgsTypes::metStart )
      return m_StartPierPage.m_EndDistance[face];
   else
      return m_EndPierPage.m_EndDistance[face];
}

ConnectionLibraryEntry::BearingOffsetMeasurementType CSpanDetailsDlg::GetBearingOffsetMeasurementType(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face)
{
   if ( end == pgsTypes::metStart )
      return m_StartPierPage.m_BearingOffsetMeasurementType;
   else
      return m_EndPierPage.m_BearingOffsetMeasurementType;
}

Float64 CSpanDetailsDlg::GetBearingOffset(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face)
{
   if ( end == pgsTypes::metStart )
      return m_StartPierPage.m_BearingOffset[face];
   else
      return m_EndPierPage.m_BearingOffset[face];
}

Float64 CSpanDetailsDlg::GetSupportWidth(pgsTypes::MemberEndType end,pgsTypes::PierFaceType face)
{
   if ( end == pgsTypes::metStart )
      return m_StartPierPage.m_SupportWidth[face];
   else
      return m_EndPierPage.m_SupportWidth[face];
}

pgsTypes::SupportedBeamSpacing CSpanDetailsDlg::GetGirderSpacingType()
{
   return m_GirderLayoutPage.m_GirderSpacingType;
}

bool CSpanDetailsDlg::UseSameGirderType()
{
   return m_GirderLayoutPage.m_bUseSameGirderType;
}

bool CSpanDetailsDlg::UseSameNumGirders()
{
   return m_GirderLayoutPage.m_bUseSameNumGirders;
}

//bool CSpanDetailsDlg::UseSameGirderSpacingAtEachEnd()
//{
//   return m_GirderLayoutPage.m_bUseSameSpacingAtBothEnds;
//}

CGirderSpacing CSpanDetailsDlg::GetGirderSpacing(pgsTypes::MemberEndType end)
{
   CGirderSpacing gdrSpacing = m_GirderLayoutPage.m_SpacingGrid[end].GetGirderSpacingData().m_GirderSpacing;
   gdrSpacing.SetMeasurementLocation( GetMeasurementLocation(end) );
   gdrSpacing.SetMeasurementType( GetMeasurementType(end) );
   gdrSpacing.SetRefGirder( GetRefGirder(end) );
   gdrSpacing.SetRefGirderOffset( GetRefGirderOffset(end) );
   gdrSpacing.SetRefGirderOffsetType( GetRefGirderOffsetType(end) );
   return gdrSpacing;
}

CGirderTypes CSpanDetailsDlg::GetGirderTypes()
{
   return m_GirderLayoutPage.m_GirderNameGrid.m_GirderTypes;
}

GirderIndexType CSpanDetailsDlg::GetGirderCount()
{
   return m_GirderLayoutPage.m_nGirders;
}

pgsTypes::MeasurementLocation CSpanDetailsDlg::GetMeasurementLocation(pgsTypes::MemberEndType end)
{
   pgsTypes::MeasurementLocation ml;
   pgsTypes::MeasurementType mt;
   UnhashGirderSpacing(m_GirderLayoutPage.m_GirderSpacingMeasure[end],&ml,&mt);

   return ml;
}

pgsTypes::MeasurementType CSpanDetailsDlg::GetMeasurementType(pgsTypes::MemberEndType end)
{
   pgsTypes::MeasurementLocation ml;
   pgsTypes::MeasurementType mt;
   UnhashGirderSpacing(m_GirderLayoutPage.m_GirderSpacingMeasure[end],&ml,&mt);

   return mt;
}

bool CSpanDetailsDlg::AllowConnectionChange(pgsTypes::MemberEndType end, const CString& conectionName)
{
   return m_GirderLayoutPage.AllowConnectionChange(end, conectionName);
}

pgsTypes::MeasurementLocation CSpanDetailsDlg::GetMeasurementLocation()
{
   return m_GirderLayoutPage.m_GirderSpacingMeasurementLocation;
}

GirderIndexType CSpanDetailsDlg::GetRefGirder(pgsTypes::MemberEndType end)
{
   return m_GirderLayoutPage.m_RefGirderIdx[end];
}

Float64 CSpanDetailsDlg::GetRefGirderOffset(pgsTypes::MemberEndType end)
{
   return m_GirderLayoutPage.m_RefGirderOffset[end];
}

pgsTypes::OffsetMeasurementType CSpanDetailsDlg::GetRefGirderOffsetType(pgsTypes::MemberEndType end)
{
   return m_GirderLayoutPage.m_RefGirderOffsetType[end];
}

pgsTypes::SlabOffsetType CSpanDetailsDlg::GetSlabOffsetType()
{
   return m_GirderLayoutPage.m_SlabOffsetType;
}

Float64 CSpanDetailsDlg::GetSlabOffset(pgsTypes::MemberEndType end)
{
   return m_GirderLayoutPage.m_SlabOffset[end];
}
