///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

CSpanDetailsDlg::CSpanDetailsDlg(const CSpanData2* pSpan,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T(""), pParentWnd, iSelectPage)
{
   Init();

   if ( pSpan )
      SetSpanData(pSpan);
}

CSpanDetailsDlg::~CSpanDetailsDlg()
{
}

void CSpanDetailsDlg::SetSpanData(const CSpanData2* pSpan)
{
   m_pBridgeDesc = pSpan->GetBridgeDescription();
   m_pPrevPier = pSpan->GetPrevPier();
   m_pSpanData = pSpan;
   m_pNextPier = pSpan->GetNextPier();

   ATLASSERT(m_pPrevPier->IsBoundaryPier());
   ATLASSERT(m_pNextPier->IsBoundaryPier());

   m_PierConnectionType[pgsTypes::metStart] = m_pPrevPier->GetPierConnectionType();
   m_PierConnectionType[pgsTypes::metEnd  ] = m_pNextPier->GetPierConnectionType();

   m_pGirderGroup = m_pBridgeDesc->GetGirderGroup(m_pSpanData);

   m_SpanLayoutPage.Init(pSpan);

   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      m_StartPierPage.Init(m_pPrevPier);
      m_EndPierPage.Init(m_pNextPier);
      m_GirderLayoutPage.Init(pSpan);
   }

   // Set dialog title
   CString strTitle;
   strTitle.Format(_T("Span %d Details"),LABEL_SPAN(pSpan->GetIndex()));
   SetTitle(strTitle);


   CString strStartPierLabel(m_pPrevPier->GetPrevSpan() == NULL ? _T("Abut.") : _T("Pier"));
   m_strStartPierTitle.Format(_T("%s %d Connections"),strStartPierLabel,LABEL_PIER(m_pPrevPier->GetIndex()));
   m_StartPierPage.m_psp.dwFlags |= PSP_USETITLE;
   m_StartPierPage.m_psp.pszTitle = m_strStartPierTitle.GetBuffer();

   CString strEndPierLabel(m_pNextPier->GetNextSpan() == NULL ? _T("Abut.") : _T("Pier"));
   m_strEndPierTitle.Format(_T("%s %d Connections"),strEndPierLabel,LABEL_PIER(m_pNextPier->GetIndex()));
   m_EndPierPage.m_psp.dwFlags |= PSP_USETITLE;
   m_EndPierPage.m_psp.pszTitle = m_strEndPierTitle.GetBuffer();
}

pgsTypes::PierConnectionType CSpanDetailsDlg::GetPierConnectionType(PierIndexType pierIdx)
{
   if ( m_pPrevPier->GetIndex() == pierIdx )
      return m_PierConnectionType[pgsTypes::metStart];
   else
      return m_PierConnectionType[pgsTypes::metEnd];
}

void CSpanDetailsDlg::SetPierConnectionType(PierIndexType pierIdx,pgsTypes::PierConnectionType type)
{
   if ( m_pPrevPier->GetIndex() == pierIdx )
      m_PierConnectionType[pgsTypes::metStart] = type;
   else
      m_PierConnectionType[pgsTypes::metEnd] = type;
}

pgsTypes::PierSegmentConnectionType CSpanDetailsDlg::GetSegmentConnectionType(PierIndexType pierIdx)
{
   if ( m_pPrevPier->GetIndex() == pierIdx )
      return m_SegmentConnectionType[pgsTypes::metStart];
   else
      return m_SegmentConnectionType[pgsTypes::metEnd];
}

void CSpanDetailsDlg::SetSegmentConnectionType(PierIndexType pierIdx,pgsTypes::PierSegmentConnectionType type)
{
   if ( m_pPrevPier->GetIndex() == pierIdx )
      m_SegmentConnectionType[pgsTypes::metStart] = type;
   else
      m_SegmentConnectionType[pgsTypes::metEnd] = type;
}

const CSpanData2* CSpanDetailsDlg::GetPrevSpan(PierIndexType pierIdx)
{
   if ( m_pPrevPier->GetIndex() == pierIdx )
      return m_pPrevPier->GetPrevSpan();
   else
      return m_pNextPier->GetPrevSpan();
}

const CSpanData2* CSpanDetailsDlg::GetNextSpan(PierIndexType pierIdx)
{
   if ( m_pPrevPier->GetIndex() == pierIdx )
      return m_pPrevPier->GetNextSpan();
   else
      return m_pNextPier->GetNextSpan();
}

const CBridgeDescription2* CSpanDetailsDlg::GetBridgeDescription()
{
   return m_pBridgeDesc;
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

   AddPage(&m_SpanLayoutPage);

   // Even though connections and girder spacing aren't defined by span,
   // spans and groups are the same thing for PGSuper documents so it
   // make sense to present the input in this form. The user is expecting it
   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      m_StartPierPage.m_psp.dwFlags |= PSP_HASHELP;
      AddPage(&m_StartPierPage);

      m_EndPierPage.m_psp.dwFlags |= PSP_HASHELP;
      AddPage(&m_EndPierPage);

      m_GirderLayoutPage.m_psp.dwFlags |= PSP_HASHELP;
      AddPage(&m_GirderLayoutPage);
   }
}

txnEditSpanData CSpanDetailsDlg::GetEditSpanData()
{
   txnEditSpanData editSpanData(m_pSpanData); // initialize with current data

   // General Layout
   editSpanData.m_SpanLength = GetSpanLength();

   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      // Spacing
      editSpanData.m_nGirders                         = GetGirderCount();
      editSpanData.m_bUseSameNumGirders               = UseSameNumGirders();
      editSpanData.m_bUseSameGirderType               = UseSameGirderType();
      editSpanData.m_GirderSpacingType                = GetGirderSpacingType();
      editSpanData.m_GirderSpacingMeasurementLocation = GetMeasurementLocation();
      editSpanData.m_GirderGroup                      = GetGirderGroup();
      // more spacing below

      // Connections and Spacing
      for ( int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType end = (i == 0 ? pgsTypes::metStart : pgsTypes::metEnd);

         // Connection
         editSpanData.m_ConnectionType[end]        = GetPierConnectionType(end);
         editSpanData.m_DiaphragmHeight[end]       = GetDiaphragmHeight(end);
         editSpanData.m_DiaphragmWidth[end]        = GetDiaphragmWidth(end);
         editSpanData.m_DiaphragmLoadType[end]     = GetDiaphragmLoadType(end);
         editSpanData.m_DiaphragmLoadLocation[end] = GetDiaphragmLoadLocation(end);

         // Spacing
         editSpanData.m_GirderSpacing[end] = GetGirderSpacing(end);

         // Connections
         for ( int j = 0; j < 2; j++ )
         {
            pgsTypes::PierFaceType face = (j == 0 ? pgsTypes::Back : pgsTypes::Ahead);
            editSpanData.m_EndDistanceMeasurementType[end][face]   = GetEndDistanceMeasurementType(end,face);
            editSpanData.m_EndDistance[end][face]                  = GetEndDistance(end,face);
            editSpanData.m_BearingOffsetMeasurementType[end][face] = GetBearingOffsetMeasurementType(end,face);
            editSpanData.m_BearingOffset[end][face]                = GetBearingOffset(end,face);
            editSpanData.m_SupportWidth[end][face]                 = GetSupportWidth(end,face);
         }
      }

   }

   editSpanData.m_SlabOffsetType = GetSlabOffsetType();
   editSpanData.m_SlabOffset[pgsTypes::metStart] = GetSlabOffset(pgsTypes::metStart);
   editSpanData.m_SlabOffset[pgsTypes::metEnd]   = GetSlabOffset(pgsTypes::metEnd);

   return editSpanData;
}

Float64 CSpanDetailsDlg::GetSpanLength()
{
   return m_SpanLayoutPage.GetSpanLength();
}

pgsTypes::PierConnectionType CSpanDetailsDlg::GetConnectionType(pgsTypes::MemberEndType end)
{
   return m_PierConnectionType[end];
}

Float64 CSpanDetailsDlg::GetDiaphragmHeight(pgsTypes::MemberEndType end)
{
   if ( end == pgsTypes::metStart )
      return m_StartPierPage.m_DiaphragmHeight[pgsTypes::Ahead];
   else
      return m_EndPierPage.m_DiaphragmHeight[pgsTypes::Back];
}

Float64 CSpanDetailsDlg::GetDiaphragmWidth(pgsTypes::MemberEndType end)
{
   if ( end == pgsTypes::metStart )
      return m_StartPierPage.m_DiaphragmWidth[pgsTypes::Ahead];
   else
      return m_EndPierPage.m_DiaphragmWidth[pgsTypes::Back];
}

ConnectionLibraryEntry::DiaphragmLoadType CSpanDetailsDlg::GetDiaphragmLoadType(pgsTypes::MemberEndType end)
{
   if ( end == pgsTypes::metStart )
      return m_StartPierPage.m_DiaphragmLoadType[pgsTypes::Ahead];
   else
      return m_EndPierPage.m_DiaphragmLoadType[pgsTypes::Back];
}

Float64 CSpanDetailsDlg::GetDiaphragmLoadLocation(pgsTypes::MemberEndType end)
{
   if ( end == pgsTypes::metStart )
      return m_StartPierPage.m_DiaphragmLoadLocation[pgsTypes::Ahead];
   else
      return m_EndPierPage.m_DiaphragmLoadLocation[pgsTypes::Back];
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

CGirderSpacing2 CSpanDetailsDlg::GetGirderSpacing(pgsTypes::MemberEndType end)
{
   CGirderSpacing2 gdrSpacing = m_GirderLayoutPage.m_SpacingGrid[end].GetGirderSpacingData().m_GirderSpacing;
   gdrSpacing.SetMeasurementLocation( GetMeasurementLocation(end) );
   gdrSpacing.SetMeasurementType( GetMeasurementType(end) );
   gdrSpacing.SetRefGirder( GetRefGirder(end) );
   gdrSpacing.SetRefGirderOffset( GetRefGirderOffset(end) );
   gdrSpacing.SetRefGirderOffsetType( GetRefGirderOffsetType(end) );

   return gdrSpacing;
}

const CGirderGroupData& CSpanDetailsDlg::GetGirderGroup() const
{
   return m_GirderLayoutPage.m_GirderNameGrid.m_GirderGroup;
}

GirderIndexType CSpanDetailsDlg::GetGirderCount() const
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
