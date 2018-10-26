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

CPierDetailsDlg::CPierDetailsDlg(const CBridgeDescription2* pBridge,PierIndexType pierIdx,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T(""), pParentWnd, iSelectPage)
{
   Init(pBridge,pierIdx);
   InitPages();
}

CPierDetailsDlg::~CPierDetailsDlg()
{
}

CBridgeDescription2* CPierDetailsDlg::GetBridgeDescription()
{
   return &m_BridgeDesc;
}

void CPierDetailsDlg::InitPages()
{
   m_psh.dwFlags                            |= PSH_HASHELP | PSH_NOAPPLYNOW;
   m_PierLayoutPage.m_psp.dwFlags           |= PSP_HASHELP;
   m_PierConnectionsPage.m_psp.dwFlags      |= PSP_HASHELP;
   m_PierGirderSpacingPage.m_psp.dwFlags    |= PSP_HASHELP;

   m_ClosureJointGeometryPage.m_psp.dwFlags |= PSP_HASHELP;
   m_GirderSegmentSpacingPage.m_psp.dwFlags |= PSP_HASHELP;

   AddPage(&m_PierLayoutPage);

   if ( m_pPierData->IsBoundaryPier() )
   {
      AddPage(&m_PierConnectionsPage);
      AddPage(&m_PierGirderSpacingPage);
   }
   else
   {
      AddPage(&m_ClosureJointGeometryPage);
      AddPage(&m_GirderSegmentSpacingPage);
   }
}

void CPierDetailsDlg::Init(const CBridgeDescription2* pBridge,PierIndexType pierIdx)
{
   m_BridgeDesc = *pBridge;
   m_pPierData  = m_BridgeDesc.GetPier(pierIdx);
   m_pPrevSpan  = m_pPierData->GetPrevSpan();
   m_pNextSpan  = m_pPierData->GetNextSpan();

   m_PierLayoutPage.Init(m_pPierData);

   if ( m_pPierData->IsBoundaryPier() )
   {
      m_PierConnectionsPage.Init(m_pPierData);
      m_PierGirderSpacingPage.Init(this);
   }
   else
   {
      m_ClosureJointGeometryPage.Init(m_pPierData);
      m_GirderSegmentSpacingPage.Init(m_pPierData);
   }

   // Set dialog title
   CString strTitle;
   strTitle.Format(_T("%s %d Details"),
      m_pPierData->IsAbutment() ? _T("Abutment") : _T("Pier"),
      LABEL_PIER(pierIdx));
   
   SetTitle(strTitle);
}

BEGIN_MESSAGE_MAP(CPierDetailsDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CPierDetailsDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPierDetailsDlg message handlers
