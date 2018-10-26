///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
   m_PierGirderSpacingPage.Init(pPier);

   m_ConnectionName[pgsTypes::Back]   = pPier->GetConnection(pgsTypes::Back);
   m_ConnectionName[pgsTypes::Ahead]  = pPier->GetConnection(pgsTypes::Ahead);

   m_ConnectionType  = pPier->GetConnectionType();


   // Set dialog title
   CString strTitle;
   strTitle.Format(_T("%s %d Details"),
      m_pPrevSpan == NULL || m_pNextSpan == NULL ? _T("Abutment") : _T("Pier"),
      pPier->GetPierIndex()+1);
   
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
   editPierData.nGirders[pgsTypes::Back]            = GetNumGirders(pgsTypes::Back);
   editPierData.nGirders[pgsTypes::Ahead]           = GetNumGirders(pgsTypes::Ahead);
   editPierData.Connection[pgsTypes::Back]          = GetConnection(pgsTypes::Back);
   editPierData.Connection[pgsTypes::Ahead]         = GetConnection(pgsTypes::Ahead);
   editPierData.ConnectionType                      = GetConnectionType();
   editPierData.GirderSpacing[pgsTypes::Back]       = GetGirderSpacing(pgsTypes::Back);
   editPierData.GirderSpacing[pgsTypes::Ahead]      = GetGirderSpacing(pgsTypes::Ahead);
   editPierData.GirderSpacingType                   = GetGirderSpacingType();
   editPierData.UseSameNumberOfGirdersInAllSpans    = UseSameNumberOfGirdersInAllSpans();
   editPierData.GirderMeasurementLocation           = GetMeasurementLocation();

   editPierData.SlabOffsetType                      = GetSlabOffsetType();
   editPierData.SlabOffset[pgsTypes::Back]          = GetSlabOffset(pgsTypes::Back);
   editPierData.SlabOffset[pgsTypes::Ahead]         = GetSlabOffset(pgsTypes::Ahead);
   
   return editPierData;
}

LPCTSTR CPierDetailsDlg::GetConnection(pgsTypes::PierFaceType pierFace)
{
   return m_ConnectionName[pierFace];
}

pgsTypes::PierConnectionType CPierDetailsDlg::GetConnectionType()
{
   return m_ConnectionType;
}

pgsTypes::MovePierOption CPierDetailsDlg::GetMovePierOption()
{
   return m_PierLayoutPage.m_MovePierOption;
}

double CPierDetailsDlg::GetStation()
{
   return m_PierLayoutPage.m_Station;
}   

LPCTSTR CPierDetailsDlg::GetOrientation()
{
   return m_PierLayoutPage.m_strOrientation.c_str();
}

CGirderSpacing CPierDetailsDlg::GetGirderSpacing(pgsTypes::PierFaceType pierFace)
{
   CGirderSpacing spacing = m_PierGirderSpacingPage.m_GirderSpacingGrid[pierFace].GetGirderSpacingData().m_GirderSpacing;
   spacing.SetRefGirder(m_PierGirderSpacingPage.m_RefGirderIdx[pierFace]);
   spacing.SetRefGirderOffset(m_PierGirderSpacingPage.m_RefGirderOffset[pierFace]);
   spacing.SetRefGirderOffsetType(m_PierGirderSpacingPage.m_RefGirderOffsetType[pierFace]);
   return spacing;
}

GirderIndexType CPierDetailsDlg::GetNumGirders(pgsTypes::PierFaceType pierFace)
{
   return m_PierGirderSpacingPage.m_nGirders[pierFace];
}

bool CPierDetailsDlg::UseSameNumberOfGirdersInAllSpans()
{
   return m_PierGirderSpacingPage.m_bUseSameNumGirders;
}

pgsTypes::SupportedBeamSpacing CPierDetailsDlg::GetGirderSpacingType()
{
   return m_PierGirderSpacingPage.m_GirderSpacingType;
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
   return m_PierGirderSpacingPage.m_GirderSpacingMeasurementLocation;
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
