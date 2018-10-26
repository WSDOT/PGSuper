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

// PierConnectionsPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "PierConnectionsPage.h"
#include "PierDetailsDlg.h"

#include "HtmlHelp\HelpTopics.hh"

#include <MfcTools\MfcTools.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPierConnectionsPage property page

IMPLEMENT_DYNCREATE(CPierConnectionsPage, CPropertyPage)

CPierConnectionsPage::CPierConnectionsPage() : CPropertyPage(CPierConnectionsPage::IDD)
{
	//{{AFX_DATA_INIT(CPierConnectionsPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CPierConnectionsPage::~CPierConnectionsPage()
{
}

void CPierConnectionsPage::DoDataExchange(CDataExchange* pDX)
{
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   
   CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPierConnectionsPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   DDX_Control(pDX,IDC_BOUNDARY_CONDITIONS,m_cbConnection);

   DDX_CBStringExactCase(pDX,IDC_AHEAD_CONNECTION, pParent->m_ConnectionName[pgsTypes::Ahead]);
   DDX_CBStringExactCase(pDX,IDC_BACK_CONNECTION,  pParent->m_ConnectionName[pgsTypes::Back]);

   DDX_CBItemData(pDX,IDC_BOUNDARY_CONDITIONS,pParent->m_ConnectionType);
}


BEGIN_MESSAGE_MAP(CPierConnectionsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CPierConnectionsPage)
	ON_CBN_SELCHANGE(IDC_AHEAD_CONNECTION, OnAheadChanged)
	ON_CBN_SELCHANGE(IDC_BACK_CONNECTION, OnBackChanged)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPierConnectionsPage message handlers

BOOL CPierConnectionsPage::OnInitDialog() 
{
   InitializeComboBoxes();

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   const CSpanData* pPrevSpan = pParent->m_pPrevSpan;
   const CSpanData* pNextSpan = pParent->m_pNextSpan;

   CString strTitle = (pPrevSpan==NULL || pNextSpan==NULL) ? "Abutment" : "Pier";
   CString strlab = "Connection on Ahead side of " + strTitle;
   GetDlgItem(IDC_AHEAD_CONNECTION_LABEL)->SetWindowText(strlab);
   strlab = "Connection on Back side of " + strTitle;
   GetDlgItem(IDC_BACK_CONNECTION_LABEL)->SetWindowText(strlab);
   strlab = "Boundary Condition at " + strTitle;
   GetDlgItem(IDC_BC_GROUP)->SetWindowText(strlab);

   CPropertyPage::OnInitDialog();

   m_cbConnection.SetPierType(PIERTYPE_INTERMEDIATE);

   if ( pPrevSpan == NULL )
   {
      // if there isn't a previous span, then only have input for the ahead side
      GetDlgItem(IDC_BACK_CONNECTION_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_BACK_CONNECTION)->ShowWindow(SW_HIDE);

      // move the ahead side group
      CRect rClient;
      GetDlgItem(IDC_BACK_CONNECTION_LABEL)->GetWindowRect(&rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_AHEAD_CONNECTION_LABEL)->MoveWindow(rClient);

      GetDlgItem(IDC_BACK_CONNECTION)->GetWindowRect(&rClient);
      ScreenToClient(rClient);
      GetDlgItem(IDC_AHEAD_CONNECTION)->MoveWindow(rClient);

      m_cbConnection.SetPierType(PIERTYPE_START);
   }

   if ( pNextSpan == NULL )
   {
      // if there isn't a next span, then only have input for the back side
      GetDlgItem(IDC_AHEAD_CONNECTION_LABEL)->ShowWindow(SW_HIDE);
      GetDlgItem(IDC_AHEAD_CONNECTION)->ShowWindow(SW_HIDE);

      m_cbConnection.SetPierType(PIERTYPE_END);
   }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPierConnectionsPage::InitializeComboBoxes()
{
   CComboBox* pcbBackConnections  = (CComboBox*)GetDlgItem(IDC_BACK_CONNECTION);
   CComboBox* pcbAheadConnections = (CComboBox*)GetDlgItem(IDC_AHEAD_CONNECTION);

   CComboBox* pcbBoundaryConditions = (CComboBox*)GetDlgItem(IDC_BOUNDARY_CONDITIONS);

   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   const CSpanData* pPrevSpan = pParent->m_pPrevSpan;
   const CSpanData* pNextSpan = pParent->m_pNextSpan;

   FillWithBoundaryConditions(pcbBoundaryConditions, (pPrevSpan && pNextSpan) ? true : false);

   if ( pPrevSpan )
   {
      FillWithConnections(pcbBackConnections);
   }

   if ( pNextSpan )
   {
      FillWithConnections(pcbAheadConnections);
   }
}

void CPierConnectionsPage::FillWithConnections(CComboBox* pCB)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2( pBroker, ILibraryNames, pLibNames);

   std::vector<std::string> strNames;
   pLibNames->EnumGdrConnectionNames( &strNames );
   std::vector<std::string>::iterator iter;

   for ( iter = strNames.begin(); iter != strNames.end(); iter++ )
   {
      const std::string& strConnection = *iter;

      pCB->AddString( strConnection.c_str() );
   }
}

inline int AddCbData(CComboBox* pCB, pgsTypes::PierConnectionType type)
{
   int idx = pCB->AddString( CPierData::AsString(type) );
   pCB->SetItemData(idx,(DWORD)type);
   return idx;
}

void CPierConnectionsPage::FillWithBoundaryConditions(CComboBox* pCB,bool bIncludeContinuity)
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


void CPierConnectionsPage::OnAheadChanged() 
{
	CComboBox* pBox = (CComboBox*)GetDlgItem( IDC_AHEAD_CONNECTION );
   CString str;
   pBox->GetWindowText(str);

   // ask spacing page if we can make this selection, and reset it if not
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   if (!pParent->AllowConnectionChange(pgsTypes::Ahead, str))
   {
      // revert
      pBox->SelectString(0,pParent->m_ConnectionName[pgsTypes::Ahead]);
   }
}

void CPierConnectionsPage::OnBackChanged() 
{
	CComboBox* pBox = (CComboBox*)GetDlgItem( IDC_BACK_CONNECTION );
   CString str;
   pBox->GetWindowText(str);

   // ask spacing page if we can make this selection, and reset it if not
   CPierDetailsDlg* pParent = (CPierDetailsDlg*)GetParent();
   if (!pParent->AllowConnectionChange(pgsTypes::Back, str))
   {
      // revert
      pBox->SelectString(0,pParent->m_ConnectionName[pgsTypes::Back]);
   }
}

void CPierConnectionsPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_PIERDETAILS_CONNECTIONS );
}
