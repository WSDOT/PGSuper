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

// PierDetailsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PierDetailsDlg.h"
#include <PgsExt\BridgeDescription2.h>

#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPierDetailsDlg

IMPLEMENT_DYNAMIC(CPierDetailsDlg, CPropertySheet)

CPierDetailsDlg::CPierDetailsDlg(const CPierData2* pPier,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T(""), pParentWnd, iSelectPage)
{
   SetPierData(pPier);
   Init();
}

CPierDetailsDlg::~CPierDetailsDlg()
{
}

void CPierDetailsDlg::SetPierData(const CPierData2* pPier)
{
   if ( pPier == NULL )
      return;

   m_pBridge = pPier->GetBridgeDescription();
   m_pPierData = pPier;
   m_pPrevSpan = m_pPierData->GetPrevSpan();
   m_pNextSpan = m_pPierData->GetNextSpan();

   m_PierLayoutPage.Init(pPier);

   if ( pPier->IsBoundaryPier() )
   {
      m_PierConnectionsPage.Init(pPier);
      m_PierGirderSpacingPage.Init(pPier);
   }
   else
   {
      m_ClosurePourGeometryPage.Init(pPier);
      m_GirderSegmentSpacingPage.Init(pPier);
   }


   // Set dialog title
   CString strTitle;
   strTitle.Format(_T("%s %d Details"),
      m_pPrevSpan == NULL || m_pNextSpan == NULL ? _T("Abutment") : _T("Pier"),
      LABEL_PIER(pPier->GetIndex()));
   
   SetTitle(strTitle);
}

BEGIN_MESSAGE_MAP(CPierDetailsDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CPierDetailsDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPierDetailsDlg message handlers
void CPierDetailsDlg::Init()
{
   m_psh.dwFlags                            |= PSH_HASHELP | PSH_NOAPPLYNOW;
   m_PierLayoutPage.m_psp.dwFlags           |= PSP_HASHELP;
   m_PierConnectionsPage.m_psp.dwFlags      |= PSP_HASHELP;
   m_PierGirderSpacingPage.m_psp.dwFlags    |= PSP_HASHELP;

   m_ClosurePourGeometryPage.m_psp.dwFlags  |= PSP_HASHELP;
   m_GirderSegmentSpacingPage.m_psp.dwFlags |= PSP_HASHELP;

   AddPage(&m_PierLayoutPage);

   if ( m_pPierData->IsBoundaryPier() )
   {
      AddPage(&m_PierConnectionsPage);
      AddPage(&m_PierGirderSpacingPage);
   }
   else
   {
      AddPage(&m_ClosurePourGeometryPage);
      AddPage(&m_GirderSegmentSpacingPage);
   }
}

txnEditPierData CPierDetailsDlg::GetEditPierData()
{
   txnEditPierData editPierData(m_pPierData); // initialize with current data
   
   editPierData.Station                             = GetStation();
   editPierData.Orientation                         = GetOrientation();
   editPierData.ErectionEventIndex                  = GetErectionEventIndex();

   editPierData.PierConnectionType                               = GetPierConnectionType();
   editPierData.SegmentConnectionType                            = GetSegmentConnectionType();
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

   if ( m_pPierData->IsBoundaryPier() )
   {
      editPierData.nGirders[pgsTypes::Back]            = GetGirderCount(pgsTypes::Back);
      editPierData.nGirders[pgsTypes::Ahead]           = GetGirderCount(pgsTypes::Ahead);
      editPierData.UseSameNumberOfGirdersInAllGroups   = UseSameNumberOfGirdersInAllGroups();

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
   }
   else
   {
      editPierData.DiaphragmHeight[pgsTypes::Back]       = 0.0;
      editPierData.DiaphragmWidth[pgsTypes::Back]        = 0.0;
      editPierData.DiaphragmLoadType[pgsTypes::Back]     = ConnectionLibraryEntry::DontApply;
      editPierData.DiaphragmLoadLocation[pgsTypes::Back] = 0.0;

      editPierData.DiaphragmHeight[pgsTypes::Ahead]       = 0.0;
      editPierData.DiaphragmWidth[pgsTypes::Ahead]        = 0.0;
      editPierData.DiaphragmLoadType[pgsTypes::Ahead]     = ConnectionLibraryEntry::DontApply;
      editPierData.DiaphragmLoadLocation[pgsTypes::Ahead] = 0.0;
   }

   editPierData.GirderSpacing[pgsTypes::Back]       = GetSpacing(pgsTypes::Back);
   editPierData.GirderSpacing[pgsTypes::Ahead]      = GetSpacing(pgsTypes::Ahead);

   editPierData.GirderSpacingType                   = GetSpacingType();

   editPierData.GirderMeasurementLocation           = GetMeasurementLocation();
   
   return editPierData;
}

pgsTypes::PierConnectionType CPierDetailsDlg::GetPierConnectionType(PierIndexType pierIdx)
{
   return GetPierConnectionType();
}

void CPierDetailsDlg::SetPierConnectionType(PierIndexType pierIdx,pgsTypes::PierConnectionType type)
{
   m_PierConnectionsPage.m_PierConnectionType = type;
}

pgsTypes::PierSegmentConnectionType CPierDetailsDlg::GetSegmentConnectionType(PierIndexType pierIdx)
{
   return GetSegmentConnectionType();
}

void CPierDetailsDlg::SetSegmentConnectionType(PierIndexType pierIdx,pgsTypes::PierSegmentConnectionType type)
{
   m_ClosurePourGeometryPage.m_PierConnectionType = type;
}

const CSpanData2* CPierDetailsDlg::GetPrevSpan(PierIndexType pierIdx)
{
   return m_pPrevSpan;
}

const CSpanData2* CPierDetailsDlg::GetNextSpan(PierIndexType pierIdx)
{
   return m_pNextSpan;
}

const CBridgeDescription2* CPierDetailsDlg::GetBridgeDescription()
{
   return m_pBridge;
}

bool CPierDetailsDlg::IsInteriorPier()
{
   return m_pPierData->IsInteriorPier();
}

bool CPierDetailsDlg::IsBoundaryPier()
{
   return m_pPierData->IsBoundaryPier();
}

bool CPierDetailsDlg::HasSpacing()
{
   return ( IsBoundaryPier() || 
          ( IsInteriorPier() && (GetSegmentConnectionType() == pgsTypes::psctContinousClosurePour || GetSegmentConnectionType() == pgsTypes::psctIntegralClosurePour)) );
}

pgsTypes::PierConnectionType CPierDetailsDlg::GetPierConnectionType()
{
   return m_PierConnectionsPage.m_PierConnectionType;
}

pgsTypes::PierSegmentConnectionType CPierDetailsDlg::GetSegmentConnectionType()
{
   return m_ClosurePourGeometryPage.m_PierConnectionType;
}

Float64 CPierDetailsDlg::GetBearingOffset(pgsTypes::PierFaceType face)
{
   if ( m_pPierData->IsBoundaryPier() )
      return m_PierConnectionsPage.m_BearingOffset[face];
   else
      return m_ClosurePourGeometryPage.m_BearingOffset;
}

ConnectionLibraryEntry::BearingOffsetMeasurementType CPierDetailsDlg::GetBearingOffsetMeasurementType(pgsTypes::PierFaceType face)
{
   if ( m_pPierData->IsBoundaryPier() )
      return m_PierConnectionsPage.m_BearingOffsetMeasurementType;
   else
      return m_ClosurePourGeometryPage.m_BearingOffsetMeasurementType;
}

Float64 CPierDetailsDlg::GetEndDistance(pgsTypes::PierFaceType face)
{
   if ( m_pPierData->IsBoundaryPier() )
      return m_PierConnectionsPage.m_EndDistance[face];
   else
      return m_ClosurePourGeometryPage.m_EndDistance;
}

ConnectionLibraryEntry::EndDistanceMeasurementType CPierDetailsDlg::GetEndDistanceMeasurementType(pgsTypes::PierFaceType face)
{
   if ( m_pPierData->IsBoundaryPier() )
      return m_PierConnectionsPage.m_EndDistanceMeasurementType; 
   else
      return m_ClosurePourGeometryPage.m_EndDistanceMeasurementType;
}

Float64 CPierDetailsDlg::GetSupportWidth(pgsTypes::PierFaceType face)
{
   if ( m_pPierData->IsBoundaryPier() )
      return m_PierConnectionsPage.m_SupportWidth[face];
   else
      return m_ClosurePourGeometryPage.m_SupportWidth;
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

EventIndexType CPierDetailsDlg::GetErectionEventIndex()
{
   return (EventIndexType)m_PierLayoutPage.m_ErectionEventIndex;
}

CGirderSpacing2 CPierDetailsDlg::GetSpacing(pgsTypes::PierFaceType pierFace)
{
   if ( m_pPierData->IsBoundaryPier() )
   {
      CGirderSpacing2 spacing = m_PierGirderSpacingPage.m_GirderSpacingGrid[pierFace].GetGirderSpacingData().m_GirderSpacing;
      spacing.SetRefGirder(m_PierGirderSpacingPage.m_RefGirderIdx[pierFace]);
      spacing.SetRefGirderOffset(m_PierGirderSpacingPage.m_RefGirderOffset[pierFace]);
      spacing.SetRefGirderOffsetType(m_PierGirderSpacingPage.m_RefGirderOffsetType[pierFace]);
      return spacing;
   }
   else
   {
      CGirderSpacing2 spacing = m_GirderSegmentSpacingPage.m_SpacingGrid.GetSpacingData();
      spacing.SetRefGirder(m_GirderSegmentSpacingPage.m_RefGirderIdx);
      spacing.SetRefGirderOffset(m_GirderSegmentSpacingPage.m_RefGirderOffset);
      spacing.SetRefGirderOffsetType(m_GirderSegmentSpacingPage.m_RefGirderOffsetType);
      return spacing;
   }
}

GirderIndexType CPierDetailsDlg::GetGirderCount(pgsTypes::PierFaceType pierFace)
{
   ATLASSERT(m_pPierData->IsBoundaryPier()); // this parameter only applies to boundary piers
   return m_PierGirderSpacingPage.m_nGirders[pierFace];
}

bool CPierDetailsDlg::UseSameNumberOfGirdersInAllGroups()
{
   ATLASSERT(m_pPierData->IsBoundaryPier()); // this parameter only applies to boundary piers
   return m_PierGirderSpacingPage.m_bUseSameNumGirders;
}

pgsTypes::SupportedBeamSpacing CPierDetailsDlg::GetSpacingType()
{
   if ( m_pPierData->IsBoundaryPier() )
      return m_PierGirderSpacingPage.m_SpacingType;
   else
      return m_GirderSegmentSpacingPage.m_GirderSpacingType;
}

pgsTypes::MeasurementLocation CPierDetailsDlg::GetMeasurementLocation(pgsTypes::PierFaceType pierFace)
{
   pgsTypes::MeasurementLocation ml;
   pgsTypes::MeasurementType mt;

   if ( m_pPierData->IsBoundaryPier() )
      UnhashGirderSpacing(m_PierGirderSpacingPage.m_GirderSpacingMeasure[pierFace],&ml,&mt);
   else
      UnhashGirderSpacing(m_GirderSegmentSpacingPage.m_GirderSpacingMeasure,&ml,&mt);

   return ml;
}

pgsTypes::MeasurementType CPierDetailsDlg::GetMeasurementType(pgsTypes::PierFaceType pierFace)
{
   pgsTypes::MeasurementLocation ml;
   pgsTypes::MeasurementType mt;

   if ( m_pPierData->IsBoundaryPier() )
      UnhashGirderSpacing(m_PierGirderSpacingPage.m_GirderSpacingMeasure[pierFace],&ml,&mt);
   else
      UnhashGirderSpacing(m_GirderSegmentSpacingPage.m_GirderSpacingMeasure,&ml,&mt);

   return mt;
}

pgsTypes::MeasurementLocation CPierDetailsDlg::GetMeasurementLocation()
{
   if ( m_pPierData->IsBoundaryPier() )
      return m_PierGirderSpacingPage.m_GirderSpacingMeasurementLocation;
   else
      return m_GirderSegmentSpacingPage.m_GirderSpacingMeasurementLocation;
}

bool CPierDetailsDlg::AllowConnectionChange(pgsTypes::PierFaceType side, const CString& connectionName)
{
   if ( m_pPierData->IsBoundaryPier() )
      return m_PierGirderSpacingPage.AllowConnectionChange(side, connectionName);
   else
      return m_GirderSegmentSpacingPage.AllowConnectionChange(side,connectionName);
}

pgsTypes::SlabOffsetType CPierDetailsDlg::GetSlabOffsetType()
{
   ATLASSERT(m_pPierData->IsBoundaryPier()); // this parameter only applies to boundary piers
   return m_PierGirderSpacingPage.m_SlabOffsetType;
}

Float64 CPierDetailsDlg::GetSlabOffset(pgsTypes::PierFaceType pierFace)
{
   ATLASSERT(m_pPierData->IsBoundaryPier()); // this parameter only applies to boundary piers
   return m_PierGirderSpacingPage.m_SlabOffset[pierFace];
}

Float64 CPierDetailsDlg::GetDiaphragmHeight(pgsTypes::PierFaceType pierFace)
{
   ATLASSERT(m_pPierData->IsBoundaryPier());
   return m_PierConnectionsPage.m_DiaphragmHeight[pierFace];
}

Float64 CPierDetailsDlg::GetDiaphragmWidth(pgsTypes::PierFaceType pierFace)
{
   ATLASSERT(m_pPierData->IsBoundaryPier());
   return m_PierConnectionsPage.m_DiaphragmWidth[pierFace];
}

ConnectionLibraryEntry::DiaphragmLoadType CPierDetailsDlg::GetDiaphragmLoadType(pgsTypes::PierFaceType pierFace)
{
   ATLASSERT(m_pPierData->IsBoundaryPier());
   return m_PierConnectionsPage.m_DiaphragmLoadType[pierFace];
}

Float64 CPierDetailsDlg::GetDiaphragmLoadLocation(pgsTypes::PierFaceType pierFace)
{
   ATLASSERT(m_pPierData->IsBoundaryPier());
   return m_PierConnectionsPage.m_DiaphragmLoadLocation[pierFace];
}
