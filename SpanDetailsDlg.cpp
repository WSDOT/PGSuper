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

CSpanDetailsDlg::CSpanDetailsDlg(const CBridgeDescription2* pBridgeDesc,SpanIndexType spanIdx,CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(_T(""), pParentWnd, iSelectPage)
{
   InitPages();
   Init(pBridgeDesc,spanIdx);
}

CSpanDetailsDlg::~CSpanDetailsDlg()
{
}

const CBridgeDescription2* CSpanDetailsDlg::GetBridgeDescription()
{
   return &m_BridgeDesc;
}

void CSpanDetailsDlg::InitPages()
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

void CSpanDetailsDlg::Init(const CBridgeDescription2* pBridgeDesc,SpanIndexType spanIdx)
{
   // copy the bridge that we are operating on. we don't want to
   // change the actual bridge as the various UI elements manipulate
   // the bridge model. (If user presses Cancel button, we don't want
   // to have to undo changes to the original bridge model)
   m_BridgeDesc = *pBridgeDesc;

   m_pSpanData = m_BridgeDesc.GetSpan(spanIdx);
   m_pPrevPier = m_pSpanData->GetPrevPier();
   m_pNextPier = m_pSpanData->GetNextPier();

   m_PierConnectionType[pgsTypes::metStart] = m_pPrevPier->GetPierConnectionType();
   m_PierConnectionType[pgsTypes::metEnd  ] = m_pNextPier->GetPierConnectionType();

   m_pGirderGroup = m_BridgeDesc.GetGirderGroup(m_pSpanData);

   m_SpanLayoutPage.Init(this);

   CEAFDocument* pDoc = EAFGetDocument();
   if ( pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      ATLASSERT(m_pPrevPier->IsBoundaryPier());
      ATLASSERT(m_pNextPier->IsBoundaryPier());

      m_StartPierPage.Init(m_pPrevPier);
      m_EndPierPage.Init(m_pNextPier);
      m_GirderLayoutPage.Init(this);
   }

   // Set dialog title
   CString strTitle;
   strTitle.Format(_T("Span %d Details"),LABEL_SPAN(m_pSpanData->GetIndex()));
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


BEGIN_MESSAGE_MAP(CSpanDetailsDlg, CPropertySheet)
	//{{AFX_MSG_MAP(CSpanDetailsDlg)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpanDetailsDlg message handlers
