///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#include "stdafx.h"
#include "pgsuper.h"
#include "SpanDetailsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpanDetailsDlg

IMPLEMENT_DYNAMIC(CSpanDetailsDlg, CPropertySheet)

CSpanDetailsDlg::CSpanDetailsDlg(const CSpanData* pSpan,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet("", pParentWnd, iSelectPage)
{
   Init();

   if ( pSpan )
      SetSpanData(pSpan);
}

CSpanDetailsDlg::~CSpanDetailsDlg()
{
}

void CSpanDetailsDlg::SetSpanData(const CSpanData* pSpan)
{
   m_pBridgeDesc = pSpan->GetBridgeDescription();
   m_pPrevPier = pSpan->GetPrevPier();
   m_pSpanData = pSpan;
   m_pNextPier = pSpan->GetNextPier();

   m_SpanLayoutPage.Init(pSpan);
   m_GirderLayoutPage.Init(pSpan);

   const CPierData* pPrevPier = pSpan->GetPrevPier();
   const CPierData* pNextPier = pSpan->GetNextPier();

   m_PrevPierConnectionName[pgsTypes::Back]  = pPrevPier->GetConnection(pgsTypes::Back);
   m_PrevPierConnectionName[pgsTypes::Ahead] = pPrevPier->GetConnection(pgsTypes::Ahead);
   m_NextPierConnectionName[pgsTypes::Back]  = pNextPier->GetConnection(pgsTypes::Back);
   m_NextPierConnectionName[pgsTypes::Ahead] = pNextPier->GetConnection(pgsTypes::Ahead);

   m_ConnectionType[pgsTypes::Ahead] = pPrevPier->GetConnectionType();
   m_ConnectionType[pgsTypes::Back]  = pNextPier->GetConnectionType();

   // Set dialog title
   CString strTitle;
   strTitle.Format("Span %d Details",pSpan->GetSpanIndex()+1);
   SetTitle(strTitle);
}

BEGIN_MESSAGE_MAP(CSpanDetailsDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CSpanDetailsDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpanDetailsDlg message handlers
void CSpanDetailsDlg::Init()
{
   m_psh.dwFlags                       |= PSH_HASHELP | PSH_NOAPPLYNOW;
   m_SpanLayoutPage.m_psp.dwFlags      |= PSP_HASHELP;
   m_SpanConnectionsPage.m_psp.dwFlags |= PSP_HASHELP;
   m_GirderLayoutPage.m_psp.dwFlags    |= PSP_HASHELP;

   AddPage(&m_SpanLayoutPage);
   AddPage(&m_SpanConnectionsPage);
   AddPage(&m_GirderLayoutPage);
}

txnEditSpanData CSpanDetailsDlg::GetEditSpanData()
{
   txnEditSpanData editSpanData(m_pSpanData); // initialize with current data

   editSpanData.bSameGirderType                = UseSameGirderType();
   editSpanData.bSameNumberOfGirdersInAllSpans = UseSameNumGirders();

   editSpanData.GirderMeasurementLocation = GetMeasurementLocation();
   
   editSpanData.PrevPierConnection[pgsTypes::Back]  = GetPrevPierConnection(pgsTypes::Back);
   editSpanData.PrevPierConnection[pgsTypes::Ahead] = GetPrevPierConnection(pgsTypes::Ahead);
   editSpanData.NextPierConnection[pgsTypes::Back]  = GetNextPierConnection(pgsTypes::Back);
   editSpanData.NextPierConnection[pgsTypes::Ahead] = GetNextPierConnection(pgsTypes::Ahead);

   editSpanData.ConnectionType[pgsTypes::Back]  = GetConnectionType(pgsTypes::Back);
   editSpanData.ConnectionType[pgsTypes::Ahead] = GetConnectionType(pgsTypes::Ahead);

   editSpanData.bSameGirderSpacingAtEachEnd    = UseSameGirderSpacingAtEachEnd();
   editSpanData.GirderSpacingType              = GetGirderSpacingType();
   editSpanData.GirderSpacing[pgsTypes::Back]  = GetGirderSpacing(pgsTypes::Back);
   editSpanData.GirderSpacing[pgsTypes::Ahead] = GetGirderSpacing(pgsTypes::Ahead);

   editSpanData.GirderTypes = GetGirderTypes();
   editSpanData.nGirders    = GetGirderCount();
   
   editSpanData.SpanLength = GetSpanLength();

   editSpanData.SlabOffsetType = GetSlabOffsetType();
   editSpanData.SlabOffset[pgsTypes::metStart] = GetSlabOffset(pgsTypes::metStart);
   editSpanData.SlabOffset[pgsTypes::metEnd]   = GetSlabOffset(pgsTypes::metEnd);

   return editSpanData;
}

double CSpanDetailsDlg::GetSpanLength()
{
   return m_SpanLayoutPage.GetSpanLength();
}

pgsTypes::PierConnectionType CSpanDetailsDlg::GetConnectionType(pgsTypes::PierFaceType pierFace)
{
   return m_ConnectionType[pierFace];
}

const char* CSpanDetailsDlg::GetPrevPierConnection(pgsTypes::PierFaceType pierFace)
{
   return m_PrevPierConnectionName[pierFace];
}

const char* CSpanDetailsDlg::GetNextPierConnection(pgsTypes::PierFaceType pierFace)
{
   return m_NextPierConnectionName[pierFace];
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

bool CSpanDetailsDlg::UseSameGirderSpacingAtEachEnd()
{
   return m_GirderLayoutPage.m_bUseSameSpacingAtBothEnds;
}

CGirderSpacing CSpanDetailsDlg::GetGirderSpacing(pgsTypes::PierFaceType pierFace)
{
   CGirderSpacing gdrSpacing = m_GirderLayoutPage.m_SpacingGrid[pierFace].GetGirderSpacingData().m_GirderSpacing;
   gdrSpacing.SetMeasurementLocation( GetMeasurementLocation(pierFace) );
   gdrSpacing.SetMeasurementType( GetMeasurementType(pierFace) );
   gdrSpacing.SetRefGirder( GetRefGirder(pierFace) );
   gdrSpacing.SetRefGirderOffset( GetRefGirderOffset(pierFace) );
   gdrSpacing.SetRefGirderOffsetType( GetRefGirderOffsetType(pierFace) );
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

pgsTypes::MeasurementLocation CSpanDetailsDlg::GetMeasurementLocation(pgsTypes::PierFaceType pierFace)
{
   pgsTypes::MeasurementLocation ml;
   pgsTypes::MeasurementType mt;
   UnhashGirderSpacing(m_GirderLayoutPage.m_GirderSpacingMeasure[pierFace],&ml,&mt);

   return ml;
}

pgsTypes::MeasurementType CSpanDetailsDlg::GetMeasurementType(pgsTypes::PierFaceType pierFace)
{
   pgsTypes::MeasurementLocation ml;
   pgsTypes::MeasurementType mt;
   UnhashGirderSpacing(m_GirderLayoutPage.m_GirderSpacingMeasure[pierFace],&ml,&mt);

   return mt;
}

bool CSpanDetailsDlg::AllowConnectionChange(pgsTypes::PierFaceType side, const CString& conectionName)
{
   return m_GirderLayoutPage.AllowConnectionChange(side, conectionName);
}

pgsTypes::MeasurementLocation CSpanDetailsDlg::GetMeasurementLocation()
{
   return m_GirderLayoutPage.m_GirderSpacingMeasurementLocation;
}

GirderIndexType CSpanDetailsDlg::GetRefGirder(pgsTypes::PierFaceType pierFace)
{
   return m_GirderLayoutPage.m_RefGirderIdx[pierFace];
}

double CSpanDetailsDlg::GetRefGirderOffset(pgsTypes::PierFaceType pierFace)
{
   return m_GirderLayoutPage.m_RefGirderOffset[pierFace];
}

pgsTypes::OffsetMeasurementType CSpanDetailsDlg::GetRefGirderOffsetType(pgsTypes::PierFaceType pierFace)
{
   return m_GirderLayoutPage.m_RefGirderOffsetType[pierFace];
}

pgsTypes::SlabOffsetType CSpanDetailsDlg::GetSlabOffsetType()
{
   return m_GirderLayoutPage.m_SlabOffsetType;
}

Float64 CSpanDetailsDlg::GetSlabOffset(pgsTypes::MemberEndType end)
{
   return m_GirderLayoutPage.m_SlabOffset[end];
}
