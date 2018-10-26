///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// SpanLayoutPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperDoc.h"
#include "SpanLayoutPage.h"
#include "SpanDetailsDlg.h"
#include <MfcTools\CustomDDX.h>
#include <EAF\EAFDisplayUnits.h>
#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpanLayoutPage property page

IMPLEMENT_DYNCREATE(CSpanLayoutPage, CPropertyPage)

CSpanLayoutPage::CSpanLayoutPage() : CPropertyPage(CSpanLayoutPage::IDD)
{
	//{{AFX_DATA_INIT(CSpanLayoutPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_SpanLength = -1;
}

CSpanLayoutPage::~CSpanLayoutPage()
{
}

void CSpanLayoutPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpanLayoutPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_UnitValueAndTag(pDX,IDC_SPAN_LENGTH,IDC_SPAN_LENGTH_UNIT,m_SpanLength,pDisplayUnits->GetSpanLengthUnit());
}


BEGIN_MESSAGE_MAP(CSpanLayoutPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpanLayoutPage)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpanLayoutPage message handlers
void CSpanLayoutPage::Init(const CSpanData* pSpan)
{
   m_SpanLength = pSpan->GetSpanLength();
}

BOOL CSpanLayoutPage::OnInitDialog() 
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   SpanIndexType spanIdx = pParent->m_pSpanData->GetSpanIndex();
   PierIndexType prevPierIdx = (PierIndexType)spanIdx;
   PierIndexType nextPierIdx = prevPierIdx+1;
   
   CPropertyPage::OnInitDialog();

   const CSpanData* pPrevSpan = pParent->m_pSpanData->GetPrevPier()->GetPrevSpan();
   const CSpanData* pNextSpan = pParent->m_pSpanData->GetNextPier()->GetNextSpan();

   CString strPrevPierType(pPrevSpan ? _T("Pier") : _T("Abutment"));
   CString strNextPierType(pNextSpan ? _T("Pier") : _T("Abutment"));

   CString strSpanLabel;
   strSpanLabel.Format(_T("Span %d"),LABEL_SPAN(spanIdx));
   GetDlgItem(IDC_SPAN_LABEL)->SetWindowText(strSpanLabel);

   CString strSpanLengthBasis;
   strSpanLengthBasis.Format(_T("Span length is measured along the alignment between the %s Line at %s %d and the %s Line at %s %d."),strPrevPierType,strPrevPierType,LABEL_PIER(prevPierIdx),strNextPierType,strNextPierType,LABEL_PIER(nextPierIdx));
   GetDlgItem(IDC_SPAN_LENGTH_BASIS)->SetWindowText(strSpanLengthBasis);

   CString strSpanLengthNote;
   strSpanLengthNote.Format(_T("The length of Span %d is changed by moving all piers after %s %d. Only the length of Span %d is changed."),LABEL_SPAN(spanIdx),strPrevPierType,LABEL_PIER(prevPierIdx),LABEL_SPAN(spanIdx));
   GetDlgItem(IDC_SPAN_LENGTH_NOTE)->SetWindowText(strSpanLengthNote);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

Float64 CSpanLayoutPage::GetSpanLength()
{
   return m_SpanLength;
}

void CSpanLayoutPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_SPANDETAILS_GENERAL );
}
