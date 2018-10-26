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

// SpanConnectionsPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "SpanConnectionsPage.h"
#include "SpanDetailsDlg.h"
#include "HtmlHelp\HelpTopics.hh"

#include <IFace\Bridge.h>
#include <IFace\Project.h>

#include <PgsExt\BridgeDescription.h>

#include <MfcTools\CustomDDX.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpanConnectionsPage property page

IMPLEMENT_DYNCREATE(CSpanConnectionsPage, CPropertyPage)

CSpanConnectionsPage::CSpanConnectionsPage() : CPropertyPage(CSpanConnectionsPage::IDD)
{
	//{{AFX_DATA_INIT(CSpanConnectionsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSpanConnectionsPage::~CSpanConnectionsPage()
{
}

void CSpanConnectionsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpanConnectionsPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   DDX_Control(pDX, IDC_AHEAD_BOUNDARY_CONDITIONS, m_cbAheadBoundaryCondition);
   DDX_Control(pDX, IDC_BACK_BOUNDARY_CONDITIONS,  m_cbBackBoundaryCondition);

   DDX_CBStringExactCase(pDX, IDC_PREV_PIER_BACK_CONNECTION,  pParent->m_PrevPierConnectionName[pgsTypes::Back]);
   DDX_CBStringExactCase(pDX, IDC_PREV_PIER_AHEAD_CONNECTION, pParent->m_PrevPierConnectionName[pgsTypes::Ahead]);
   DDX_CBStringExactCase(pDX, IDC_NEXT_PIER_BACK_CONNECTION,  pParent->m_NextPierConnectionName[pgsTypes::Back] );
   DDX_CBStringExactCase(pDX, IDC_NEXT_PIER_AHEAD_CONNECTION, pParent->m_NextPierConnectionName[pgsTypes::Ahead] );

   DDX_CBItemData(pDX, IDC_AHEAD_BOUNDARY_CONDITIONS, pParent->m_ConnectionType[pgsTypes::Ahead]);
   DDX_CBItemData(pDX, IDC_BACK_BOUNDARY_CONDITIONS,  pParent->m_ConnectionType[pgsTypes::Back] );
}


BEGIN_MESSAGE_MAP(CSpanConnectionsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpanConnectionsPage)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_CBN_SELCHANGE(IDC_PREV_PIER_AHEAD_CONNECTION, OnSelchangePrevPierAheadConnection)
	ON_CBN_SELCHANGE(IDC_PREV_PIER_BACK_CONNECTION, OnSelchangePrevPierBackConnection)
	ON_CBN_SELCHANGE(IDC_NEXT_PIER_BACK_CONNECTION, OnSelchangeNextPierBackConnection)
	ON_CBN_SELCHANGE(IDC_NEXT_PIER_AHEAD_CONNECTION, OnSelchangeNextPierAheadConnection)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpanConnectionsPage message handlers

BOOL CSpanConnectionsPage::OnInitDialog() 
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   InitializeComboBoxes();

   CPropertyPage::OnInitDialog();

   m_cbAheadBoundaryCondition.SetPierType(PIERTYPE_INTERMEDIATE);
   m_cbBackBoundaryCondition.SetPierType(PIERTYPE_INTERMEDIATE);

   if ( pParent->m_pSpanData->GetPrevPier()->GetPrevSpan() == NULL )
      m_cbAheadBoundaryCondition.SetPierType(PIERTYPE_START);

   if ( pParent->m_pSpanData->GetNextPier()->GetNextSpan() == NULL )
      m_cbBackBoundaryCondition.SetPierType(PIERTYPE_END);

   LabelGroupBoxes();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpanConnectionsPage::InitializeComboBoxes()
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   CComboBox* pcbPrevPierBackConnections  = (CComboBox*)GetDlgItem(IDC_PREV_PIER_BACK_CONNECTION);
   CComboBox* pcbPrevPierAheadConnections = (CComboBox*)GetDlgItem(IDC_PREV_PIER_AHEAD_CONNECTION);
   CComboBox* pcbNextPierBackConnections  = (CComboBox*)GetDlgItem(IDC_NEXT_PIER_BACK_CONNECTION);
   CComboBox* pcbNextPierAheadConnections = (CComboBox*)GetDlgItem(IDC_NEXT_PIER_AHEAD_CONNECTION);

   CComboBox* pcbAheadBoundaryConditions = (CComboBox*)GetDlgItem(IDC_AHEAD_BOUNDARY_CONDITIONS);
   CComboBox* pcbBackBoundaryConditions  = (CComboBox*)GetDlgItem(IDC_BACK_BOUNDARY_CONDITIONS);

   if ( pParent->m_pSpanData->GetPrevPier()->GetPrevSpan() )
   {
      FillWithConnections(pcbPrevPierBackConnections);
   }
   else
   {
      pcbPrevPierBackConnections->EnableWindow(FALSE);
   }

   FillWithConnections(pcbPrevPierAheadConnections);
   FillWithConnections(pcbNextPierBackConnections);

   if ( pParent->m_pSpanData->GetNextPier()->GetNextSpan() )
   {
      FillWithConnections(pcbNextPierAheadConnections);
   }
   else
   {
      pcbNextPierAheadConnections->EnableWindow(FALSE);
   }


   SpanIndexType spanIdx = pParent->m_pSpanData->GetSpanIndex();
   PierIndexType prevPierIdx = spanIdx;
   PierIndexType nextPierIdx = spanIdx+1;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);
   PierIndexType nPiers = pBridge->GetPierCount();

   FillWithBoundaryConditions(pcbAheadBoundaryConditions,prevPierIdx == 0 ? false : true);
   FillWithBoundaryConditions(pcbBackBoundaryConditions, nextPierIdx == nPiers-1 ? false : true);
}

void CSpanConnectionsPage::LabelGroupBoxes()
{
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();

   PierIndexType prevPierIdx = pParent->m_pPrevPier->GetPierIndex();
   PierIndexType nextPierIdx = pParent->m_pNextPier->GetPierIndex();

   CString strAhead;
   strAhead.Format(_T("%s %d"),
      pParent->m_pPrevPier->GetPrevSpan() == NULL ? _T("Abutment") : _T("Pier"),
      prevPierIdx+1);

   CWnd* pAheadGroup = GetDlgItem(IDC_AHEAD_GROUP);
   pAheadGroup->SetWindowText(strAhead);

   CString strBack;
   strBack.Format(_T("%s %d"),
      pParent->m_pNextPier->GetNextSpan() == NULL ? _T("Abutment") : _T("Pier"),
      nextPierIdx+1);

   CWnd* pBackGroup = GetDlgItem(IDC_BACK_GROUP);
   pBackGroup->SetWindowText(strBack);
}

void CSpanConnectionsPage::FillWithConnections(CComboBox* pCB)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2( pBroker, ILibraryNames, pLibNames);

   std::vector<std::_tstring> strNames;
   pLibNames->EnumGdrConnectionNames( &strNames );
   std::vector<std::_tstring>::iterator iter;

   for ( iter = strNames.begin(); iter != strNames.end(); iter++ )
   {
      const std::_tstring& strConnection = *iter;

      pCB->AddString( strConnection.c_str() );
   }
}

inline int AddCbData(CComboBox* pCB, pgsTypes::PierConnectionType type)
{
   int idx = pCB->AddString( CPierData::AsString(type) );
   pCB->SetItemData(idx,(DWORD)type);
   return idx;
}

void CSpanConnectionsPage::FillWithBoundaryConditions(CComboBox* pCB,bool bIncludeContinuity)
{
   AddCbData(pCB, pgsTypes::Hinged);
   AddCbData(pCB, pgsTypes::Roller);

   if ( bIncludeContinuity )
   {
      AddCbData(pCB, pgsTypes::ContinuousAfterDeck);
      AddCbData(pCB, pgsTypes::ContinuousBeforeDeck);
   }

   AddCbData(pCB, pgsTypes::IntegralAfterDeck);
   AddCbData(pCB, pgsTypes::IntegralBeforeDeck);

   if ( bIncludeContinuity )
   {
      AddCbData(pCB, pgsTypes::IntegralAfterDeckHingeBack);
      AddCbData(pCB, pgsTypes::IntegralBeforeDeckHingeBack);
      AddCbData(pCB, pgsTypes::IntegralAfterDeckHingeAhead);
      AddCbData(pCB, pgsTypes::IntegralBeforeDeckHingeAhead);
   }
}

void CSpanConnectionsPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_SPANDETAILS_CONNECTIONS );
}


void CSpanConnectionsPage::OnSelchangePrevPierBackConnection() 
{
	CComboBox* pBox = (CComboBox*)GetDlgItem( IDC_PREV_PIER_BACK_CONNECTION );
   CString str;
   pBox->GetWindowText(str);

   // ask spacing page if we can make this selection, and reset it if not
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   if (!pParent->AllowConnectionChange(pgsTypes::Back, str))
   {
      // revert
      pBox->SelectString(0,pParent->m_PrevPierConnectionName[pgsTypes::Back]);
   }
	
}

void CSpanConnectionsPage::OnSelchangePrevPierAheadConnection() 
{
	CComboBox* pBox = (CComboBox*)GetDlgItem( IDC_PREV_PIER_AHEAD_CONNECTION );
   CString str;
   pBox->GetWindowText(str);

   // ask spacing page if we can make this selection, and reset it if not
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   if (!pParent->AllowConnectionChange(pgsTypes::Ahead, str))
   {
      // revert
      pBox->SelectString(0,pParent->m_PrevPierConnectionName[pgsTypes::Ahead]);
   }
	
}

void CSpanConnectionsPage::OnSelchangeNextPierBackConnection() 
{
	CComboBox* pBox = (CComboBox*)GetDlgItem( IDC_NEXT_PIER_BACK_CONNECTION );
   CString conectionName;
   pBox->GetWindowText(conectionName);

   // ask spacing page if we can make this selection, and reset it if not
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   if (!pParent->AllowConnectionChange(pgsTypes::Back, conectionName))
   {
      // revert
      pBox->SelectString(0,pParent->m_NextPierConnectionName[pgsTypes::Back]);
   }
	
}

void CSpanConnectionsPage::OnSelchangeNextPierAheadConnection() 
{
	CComboBox* pBox = (CComboBox*)GetDlgItem( IDC_NEXT_PIER_AHEAD_CONNECTION );
   CString conectionName;
   pBox->GetWindowText(conectionName);

   // ask spacing page if we can make this selection, and reset it if not
   CSpanDetailsDlg* pParent = (CSpanDetailsDlg*)GetParent();
   if (!pParent->AllowConnectionChange(pgsTypes::Ahead, conectionName))
   {
      // revert
      pBox->SelectString(0,pParent->m_NextPierConnectionName[pgsTypes::Ahead]);
   }
	
}
