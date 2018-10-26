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

// PierDetailsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PierDetailsDlg.h"
#include <PgsExt\BridgeDescription.h>

#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPierDetailsDlg

IMPLEMENT_DYNAMIC(CPierDetailsDlg, CPropertySheet)

CPierDetailsDlg::CPierDetailsDlg(const CPierData* pPier,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T(""), pParentWnd, iSelectPage)
{
   Init();
   if ( pPier )
      SetPierData(pPier);
}

CPierDetailsDlg::~CPierDetailsDlg()
{
//   m_pPierData->Unlink();
}

void CPierDetailsDlg::SetPierData(const CPierData* pPier)
{
   m_pBridge = pPier->GetBridgeDescription();
   m_pPierData = pPier;
   m_pPrevSpan = m_pPierData->GetPrevSpan();
   m_pNextSpan = m_pPierData->GetNextSpan();

   m_PierLayoutPage.Init(pPier);
   m_PierConnectionsPage.Init(pPier);
   m_PierGirderSpacingPage.Init(pPier);

   m_ConnectionType                   = pPier->GetConnectionType();
   m_SpacingType                      = pPier->GetBridgeDescription()->GetGirderSpacingType();
   m_GirderSpacingMeasurementLocation = pPier->GetBridgeDescription()->GetMeasurementLocation();


   // Set dialog title
   CString strTitle;
   strTitle.Format(_T("%s %d Details"),
      m_pPrevSpan == NULL || m_pNextSpan == NULL ? _T("Abutment") : _T("Pier"),
      LABEL_PIER(pPier->GetPierIndex()));
   
   SetTitle(strTitle);
}

BEGIN_MESSAGE_MAP(CPierDetailsDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CPierDetailsDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_KICKIDLE,OnKickIdle)
END_MESSAGE_MAP()

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
	if( pPage != NULL )
	{
		//ASSERT_VALID(pPage);
		return pPage->SendMessage( WM_KICKIDLE, wp, lp );
	}
	else
		return 0;
}

INT_PTR CPierDetailsDlg::DoModal()
{
   CEAFDocument* pEAFDoc = EAFGetDocument();
   CPGSuperDoc* pDoc = (CPGSuperDoc*)pEAFDoc;
   
   std::vector<std::pair<IEditPierCallback*,CPropertyPage*>> extensionPages;

   std::map<IDType,IEditPierCallback*> callbacks = pDoc->GetEditPierCallbacks();
   std::map<IDType,IEditPierCallback*>::iterator callbackIter(callbacks.begin());
   std::map<IDType,IEditPierCallback*>::iterator callbackIterEnd(callbacks.end());
   for ( ; callbackIter != callbackIterEnd; callbackIter++ )
   {
      IEditPierCallback* pCallback = callbackIter->second;
      CPropertyPage* pPage = pCallback->CreatePropertyPage(this);
      if ( pPage )
      {
         extensionPages.push_back( std::make_pair(pCallback,pPage) );
         AddPage(pPage);
      }
   }

   INT_PTR result = CPropertySheet::DoModal();

   std::vector<std::pair<IEditPierCallback*,CPropertyPage*>>::iterator pageIter(extensionPages.begin());
   std::vector<std::pair<IEditPierCallback*,CPropertyPage*>>::iterator pageIterEnd(extensionPages.end());
   for ( ; pageIter != pageIterEnd; pageIter++ )
   {
      IEditPierCallback* pCallback = pageIter->first;
      CPropertyPage* pPage = pageIter->second;
      pCallback->DestroyPropertyPage(result,pPage,this);
   }

   return result;
}


/////////////////////////////////////////////////////////////////////////////
// CPierDetailsDlg message handlers
void CPierDetailsDlg::Init()
{
   m_psh.dwFlags                         |= PSH_HASHELP | PSH_NOAPPLYNOW;
   m_PierLayoutPage.m_psp.dwFlags        |= PSP_HASHELP;
   m_PierConnectionsPage.m_psp.dwFlags   |= PSP_HASHELP;
   m_PierGirderSpacingPage.m_psp.dwFlags |= PSP_HASHELP;

   AddPage(&m_PierLayoutPage);
   AddPage(&m_PierConnectionsPage);
   AddPage(&m_PierGirderSpacingPage);
}

txnEditPierData CPierDetailsDlg::GetEditPierData()
{
   txnEditPierData editPierData(m_pPierData); // initialize with current data
   
   editPierData.Station                             = GetStation();
   editPierData.Orientation                         = GetOrientation();

   editPierData.ConnectionType                                   = GetConnectionType();
   editPierData.BearingOffset[pgsTypes::Back]                    = GetBearingOffset(pgsTypes::Back);
   editPierData.BearingOffset[pgsTypes::Ahead]                   = GetBearingOffset(pgsTypes::Ahead);
   editPierData.BearingOffsetMeasurementType[pgsTypes::Back]     = GetBearingOffsetMeasurementType(pgsTypes::Back);
   editPierData.BearingOffsetMeasurementType[pgsTypes::Ahead]    = GetBearingOffsetMeasurementType(pgsTypes::Ahead);
   editPierData.EndDistance[pgsTypes::Back]                      = GetEndDistance(pgsTypes::Back);
   editPierData.EndDistance[pgsTypes::Ahead]                     = GetEndDistance(pgsTypes::Ahead);
   editPierData.EndDistanceMeasurementType[pgsTypes::Back]       = GetEndDistanceMeasurementType(pgsTypes::Back);
   editPierData.EndDistanceMeasurementType[pgsTypes::Ahead]      = GetEndDistanceMeasurementType(pgsTypes::Ahead);
   editPierData.SupportWidth[pgsTypes::Back]                     = GetSupportWidth(pgsTypes::Back);
   editPierData.SupportWidth[pgsTypes::Ahead]                    = GetSupportWidth(pgsTypes::Ahead);


   editPierData.nGirders[pgsTypes::Back]            = GetGirderCount(pgsTypes::Back);
   editPierData.nGirders[pgsTypes::Ahead]           = GetGirderCount(pgsTypes::Ahead);
   editPierData.GirderSpacing[pgsTypes::Back]       = GetSpacing(pgsTypes::Back);
   editPierData.GirderSpacing[pgsTypes::Ahead]      = GetSpacing(pgsTypes::Ahead);

   editPierData.GirderSpacingType                   = GetSpacingType();

   editPierData.UseSameNumberOfGirdersInAllSpans    = UseSameNumberOfGirdersInAllSpans();
   editPierData.GirderMeasurementLocation           = GetMeasurementLocation();

   editPierData.SlabOffsetType                      = GetSlabOffsetType();
   editPierData.SlabOffset[pgsTypes::Back]          = GetSlabOffset(pgsTypes::Back);
   editPierData.SlabOffset[pgsTypes::Ahead]         = GetSlabOffset(pgsTypes::Ahead);

   editPierData.DiaphragmHeight[pgsTypes::Back]       = GetDiaphragmHeight(pgsTypes::Back);
   editPierData.DiaphragmWidth[pgsTypes::Back]        = GetDiaphragmWidth(pgsTypes::Back);
   editPierData.DiaphragmLoadType[pgsTypes::Back]     = GetDiaphragmLoadType(pgsTypes::Back);
   editPierData.DiaphragmLoadLocation[pgsTypes::Back] = GetDiaphragmLoadLocation(pgsTypes::Back);

   editPierData.DiaphragmHeight[pgsTypes::Ahead]       = GetDiaphragmHeight(pgsTypes::Ahead);
   editPierData.DiaphragmWidth[pgsTypes::Ahead]        = GetDiaphragmWidth(pgsTypes::Ahead);
   editPierData.DiaphragmLoadType[pgsTypes::Ahead]     = GetDiaphragmLoadType(pgsTypes::Ahead);
   editPierData.DiaphragmLoadLocation[pgsTypes::Ahead] = GetDiaphragmLoadLocation(pgsTypes::Ahead);
   
   return editPierData;
}

pgsTypes::PierConnectionType CPierDetailsDlg::GetConnectionType(PierIndexType pierIdx)
{
   return m_ConnectionType;
}

void CPierDetailsDlg::SetConnectionType(PierIndexType pierIdx,pgsTypes::PierConnectionType type)
{
   m_ConnectionType = type;
}

const CSpanData* CPierDetailsDlg::GetPrevSpan(PierIndexType pierIdx)
{
   return m_pPrevSpan;
}

const CSpanData* CPierDetailsDlg::GetNextSpan(PierIndexType pierIdx)
{
   return m_pNextSpan;
}

const CBridgeDescription* CPierDetailsDlg::GetBridgeDescription()
{
   return m_pBridge;
}

pgsTypes::PierConnectionType CPierDetailsDlg::GetConnectionType()
{
   return m_ConnectionType;
}

Float64 CPierDetailsDlg::GetBearingOffset(pgsTypes::PierFaceType face)
{
   return m_PierConnectionsPage.m_BearingOffset[face];
}

ConnectionLibraryEntry::BearingOffsetMeasurementType CPierDetailsDlg::GetBearingOffsetMeasurementType(pgsTypes::PierFaceType face)
{
   return m_PierConnectionsPage.m_BearingOffsetMeasurementType;
}

Float64 CPierDetailsDlg::GetEndDistance(pgsTypes::PierFaceType face)
{
   return m_PierConnectionsPage.m_EndDistance[face];
}

ConnectionLibraryEntry::EndDistanceMeasurementType CPierDetailsDlg::GetEndDistanceMeasurementType(pgsTypes::PierFaceType face)
{
   return m_PierConnectionsPage.m_EndDistanceMeasurementType; 
}

Float64 CPierDetailsDlg::GetSupportWidth(pgsTypes::PierFaceType face)
{
   return m_PierConnectionsPage.m_SupportWidth[face];
}

pgsTypes::MovePierOption CPierDetailsDlg::GetMovePierOption()
{
   return m_PierLayoutPage.m_MovePierOption;
}

Float64 CPierDetailsDlg::GetStation()
{
   return m_PierLayoutPage.m_Station;
}   

LPCTSTR CPierDetailsDlg::GetOrientation()
{
   return m_PierLayoutPage.m_strOrientation.c_str();
}

CGirderSpacing CPierDetailsDlg::GetSpacing(pgsTypes::PierFaceType pierFace)
{
   CGirderSpacing spacing = m_PierGirderSpacingPage.m_GirderSpacingGrid[pierFace].GetGirderSpacingData().m_GirderSpacing;
   spacing.SetRefGirder(m_PierGirderSpacingPage.m_RefGirderIdx[pierFace]);
   spacing.SetRefGirderOffset(m_PierGirderSpacingPage.m_RefGirderOffset[pierFace]);
   spacing.SetRefGirderOffsetType(m_PierGirderSpacingPage.m_RefGirderOffsetType[pierFace]);
   return spacing;
}

GirderIndexType CPierDetailsDlg::GetGirderCount(pgsTypes::PierFaceType pierFace)
{
   return m_PierGirderSpacingPage.m_nGirders[pierFace];
}

bool CPierDetailsDlg::UseSameNumberOfGirdersInAllSpans()
{
   return m_PierGirderSpacingPage.m_bUseSameNumGirders;
}

pgsTypes::SupportedBeamSpacing CPierDetailsDlg::GetSpacingType()
{
   return m_SpacingType;
}

pgsTypes::MeasurementLocation CPierDetailsDlg::GetMeasurementLocation(pgsTypes::PierFaceType pierFace)
{
   pgsTypes::MeasurementLocation ml;
   pgsTypes::MeasurementType mt;
   UnhashGirderSpacing(m_PierGirderSpacingPage.m_GirderSpacingMeasure[pierFace],&ml,&mt);

   return ml;
}

pgsTypes::MeasurementType CPierDetailsDlg::GetMeasurementType(pgsTypes::PierFaceType pierFace)
{
   pgsTypes::MeasurementLocation ml;
   pgsTypes::MeasurementType mt;
   UnhashGirderSpacing(m_PierGirderSpacingPage.m_GirderSpacingMeasure[pierFace],&ml,&mt);

   return mt;
}

pgsTypes::MeasurementLocation CPierDetailsDlg::GetMeasurementLocation()
{
   return m_GirderSpacingMeasurementLocation;
}

bool CPierDetailsDlg::AllowConnectionChange(pgsTypes::PierFaceType side, const CString& conectionName)
{
   return m_PierGirderSpacingPage.AllowConnectionChange(side, conectionName);
}

pgsTypes::SlabOffsetType CPierDetailsDlg::GetSlabOffsetType()
{
   return m_PierGirderSpacingPage.m_SlabOffsetType;
}

Float64 CPierDetailsDlg::GetSlabOffset(pgsTypes::PierFaceType pierFace)
{
   return m_PierGirderSpacingPage.m_SlabOffset[pierFace];
}

Float64 CPierDetailsDlg::GetDiaphragmHeight(pgsTypes::PierFaceType pierFace)
{
   return m_PierConnectionsPage.m_DiaphragmHeight[pierFace];
}

Float64 CPierDetailsDlg::GetDiaphragmWidth(pgsTypes::PierFaceType pierFace)
{
   return m_PierConnectionsPage.m_DiaphragmWidth[pierFace];
}

ConnectionLibraryEntry::DiaphragmLoadType CPierDetailsDlg::GetDiaphragmLoadType(pgsTypes::PierFaceType pierFace)
{
   return m_PierConnectionsPage.m_DiaphragmLoadType[pierFace];
}

Float64 CPierDetailsDlg::GetDiaphragmLoadLocation(pgsTypes::PierFaceType pierFace)
{
   return m_PierConnectionsPage.m_DiaphragmLoadLocation[pierFace];
}

