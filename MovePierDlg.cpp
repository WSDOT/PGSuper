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

// MovePierDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "MovePierDlg.h"
#include <MFCTools\CustomDDX.h>
#include "PGSuperUnits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMovePierDlg dialog


CMovePierDlg::CMovePierDlg(PierIndexType pierIdx,double fromStation,double toStation,double prevPierStation,double nextPierStation,long nSpans,const unitStationFormat& stationFormat,CWnd* pParent /*=NULL*/)
	: CDialog(CMovePierDlg::IDD, pParent),
   m_StationFormat(stationFormat)
{
   m_PierIdx = pierIdx;
   m_nSpans  = nSpans;
   m_FromStation = fromStation;
   m_ToStation = toStation;
   m_PrevPierStation = prevPierStation;
   m_NextPierStation = nextPierStation;
   m_Option = pgsTypes::MoveBridge;


	//{{AFX_DATA_INIT(CMovePierDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CMovePierDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMovePierDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   DDX_CBItemData(pDX,IDC_RESIZEOPTIONS,m_Option);
}


BEGIN_MESSAGE_MAP(CMovePierDlg, CDialog)
	//{{AFX_MSG_MAP(CMovePierDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMovePierDlg message handlers

BOOL CMovePierDlg::OnInitDialog() 
{

   CString strFromStation = FormatStation(m_StationFormat,m_FromStation);
   CString strToStation   = FormatStation(m_StationFormat,m_ToStation);

   CString strName = (m_PierIdx == 0 || m_PierIdx == m_nSpans ? _T("Abutment") : _T("Pier"));

   CStatic* pStatic = (CStatic*)GetDlgItem(IDC_DESCRIPTION);
   CString strDescription;
   strDescription.Format(_T("Move %s %d from %s to %s"),strName,m_PierIdx+1,strFromStation,strToStation);
   pStatic->SetWindowText(strDescription);

   CString strOptions[4];
   pgsTypes::MovePierOption options[4];
   strOptions[0] = _T("Move bridge, retain all span lengths");
   options[0] = pgsTypes::MoveBridge;

   int nOptions = 1;

   if ( 1 < m_nSpans &&  // must have more than one span... moving an interior pier
        m_PierIdx != 0 && m_PierIdx != m_nSpans &&  // can't be first or last pier
        m_PrevPierStation < m_ToStation && m_ToStation < m_NextPierStation ) // can't move pier beyond adjacent piers
   {
      options[nOptions] = pgsTypes::AdjustAdjacentSpans;
      strOptions[nOptions++].Format(_T("Adjust length of Span %d and %d"),LABEL_SPAN(m_PierIdx-1),LABEL_SPAN(m_PierIdx));
   }

   if ( m_PierIdx == 0 && m_ToStation < m_NextPierStation )
   {
      // adjust length of first span only
      options[nOptions] = pgsTypes::AdjustNextSpan;
      if ( m_nSpans == 1 )
         strOptions[nOptions++].Format(_T("Adjust length of Span %d by moving %s %d"), LABEL_SPAN(m_PierIdx-1),strName,LABEL_SPAN(m_PierIdx));
      else
         strOptions[nOptions++].Format(_T("Adjust length of Span %d, retain length of all other spans"),LABEL_SPAN(m_PierIdx));
   }
   else if ( m_PierIdx == m_nSpans && m_PrevPierStation < m_ToStation )
   {
      // adjust length of last span only
      options[nOptions] = pgsTypes::AdjustPrevSpan;
      if ( m_nSpans == 1 )
         strOptions[nOptions++].Format(_T("Adjust length of Span %d by moving %s %d"),m_PierIdx,strName,m_PierIdx+1);
      else
         strOptions[nOptions++].Format(_T("Adjust length of Span %d, retain length of all other spans"),m_PierIdx-1);
   }
   else if ( 0 < m_PierIdx && m_PierIdx < m_nSpans )
   {
      if ( m_PrevPierStation < m_ToStation )
      {
         // adjust length of previous span only
         options[nOptions] = pgsTypes::AdjustPrevSpan;
         strOptions[nOptions++].Format(_T("Adjust length of Span %d, retain length of all other spans"),m_PierIdx);
      }

      if ( m_ToStation < m_NextPierStation )
      {
         // adjust length of next span only
         options[nOptions] = pgsTypes::AdjustNextSpan;
         strOptions[nOptions++].Format(_T("Adjust length of Span %d, retain length of all other spans"),m_PierIdx+1);
      }
   }

   CComboBox* pOptions = (CComboBox*)GetDlgItem(IDC_RESIZEOPTIONS);

   for ( int i = 0; i < nOptions; i++ )
   {
      int idx = pOptions->AddString(strOptions[i]); 
      pOptions->SetItemData(idx,(DWORD)options[i]);
   }

	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
