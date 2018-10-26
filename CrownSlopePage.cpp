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

// CrownSlopePage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "CrownSlopePage.h"
#include "AlignmentDescriptionDlg.h"
#include <MfcTools\CogoDDX.h>
#include "htmlhelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCrownSlopePage property page

IMPLEMENT_DYNCREATE(CCrownSlopePage, CPropertyPage)

CCrownSlopePage::CCrownSlopePage() : CPropertyPage(CCrownSlopePage::IDD)
{
	//{{AFX_DATA_INIT(CCrownSlopePage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CCrownSlopePage::~CCrownSlopePage()
{
}

IBroker* CCrownSlopePage::GetBroker()
{
   CAlignmentDescriptionDlg* pParent = (CAlignmentDescriptionDlg*)GetParent();
   return pParent->m_pBroker;
}

void CCrownSlopePage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCrownSlopePage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

   if ( pDX->m_bSaveAndValidate )
   {
      m_Grid.SortCrossSections();
      if ( !m_Grid.GetCrownSlopeData(m_RoadwaySectionData.Superelevations) )
      {
         AfxMessageBox(_T("Invalid super elevation parameters"));
         pDX->Fail();
      }

      if ( m_RoadwaySectionData.Superelevations.size () == 0 )
      {
         AfxMessageBox(_T("At least one section must be defined"));
         pDX->Fail();
      }
   }
   else
   {
      m_Grid.SetCrownSlopeData(m_RoadwaySectionData.Superelevations);
   }
}

BEGIN_MESSAGE_MAP(CCrownSlopePage, CPropertyPage)
	//{{AFX_MSG_MAP(CCrownSlopePage)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
	ON_BN_CLICKED(IDC_SORT, OnSort)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCrownSlopePage message handlers

void CCrownSlopePage::OnAdd() 
{
   m_Grid.AppendRow();	
}

void CCrownSlopePage::OnRemove() 
{
   m_Grid.RemoveRows();
}

void CCrownSlopePage::OnSort() 
{
   m_Grid.SortCrossSections();
}

BOOL CCrownSlopePage::OnInitDialog() 
{
	m_Grid.SubclassDlgItem(IDC_VCURVE_GRID, this);
   m_Grid.CustomInit();

   CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCrownSlopePage::OnHelp()
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_ALIGNMENT_SUPERELEVATION );
}
