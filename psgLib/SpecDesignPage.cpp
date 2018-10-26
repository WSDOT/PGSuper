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

// SpecDesignPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "SpecDesignPage.h"

#include "SpecMainSheet.h"
#include "..\htmlhelp\HelpTopics.hh"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecDesignPage dialog
IMPLEMENT_DYNCREATE(CSpecDesignPage, CPropertyPage)


CSpecDesignPage::CSpecDesignPage(CWnd* pParent /*=NULL*/)
	: CPropertyPage(CSpecDesignPage::IDD)
{
	//{{AFX_DATA_INIT(CSpecDesignPage)
	m_CheckA = FALSE;
	m_CheckHauling = FALSE;
	m_CheckHoldDown = FALSE;
	m_CheckLifting = FALSE;
	m_CheckSlope = FALSE;
	m_DesignA = FALSE;
	m_DesignHauling = FALSE;
	m_DesignHoldDown = FALSE;
	m_DesignLifting = FALSE;
	m_DesignSlope = FALSE;
	m_CheckSplitting = FALSE;
	m_DesignSplitting = FALSE;
	m_CheckConfinement = FALSE;
	m_DesignConfinement = FALSE;
	//}}AFX_DATA_INIT
}


void CSpecDesignPage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   // dad is a friend of the entry. use him to transfer data.
   CSpecMainSheet* pDad = (CSpecMainSheet*)GetParent();


   if (!pDX->m_bSaveAndValidate)
   {
      pDad->UploadDesignData(pDX);
   }

	//{{AFX_DATA_MAP(CSpecDesignPage)
	DDX_Check(pDX, IDC_CHECK_A, m_CheckA);
	DDX_Check(pDX, IDC_CHECK_HAULING, m_CheckHauling);
	DDX_Check(pDX, IDC_CHECK_HD, m_CheckHoldDown);
	DDX_Check(pDX, IDC_CHECK_LIFTING, m_CheckLifting);
	DDX_Check(pDX, IDC_CHECK_SLOPE, m_CheckSlope);
	DDX_Check(pDX, IDC_DESIGN_A, m_DesignA);
	DDX_Check(pDX, IDC_DESIGN_HAULING, m_DesignHauling);
	DDX_Check(pDX, IDC_DESIGN_HD, m_DesignHoldDown);
	DDX_Check(pDX, IDC_DESIGN_LIFTING, m_DesignLifting);
	DDX_Check(pDX, IDC_DESIGN_SLOPE, m_DesignSlope);
	DDX_Check(pDX, IDC_CHECK_SPLITTING, m_CheckSplitting);
	DDX_Check(pDX, IDC_DESIGN_SPLITTING, m_DesignSplitting);
	DDX_Check(pDX, IDC_CHECK_CONFINEMENT, m_CheckConfinement);
	DDX_Check(pDX, IDC_DESIGN_CONFINEMENT, m_DesignConfinement);
	//}}AFX_DATA_MAP

   DDX_Radio(pDX,IDC_RADIO_FILL_PERMANENT,m_FillMethod);


   if (pDX->m_bSaveAndValidate)
   {
      pDad->DownloadDesignData(pDX);
   }
   else
   {
      OnCheckA();
      OnCheckHauling();
      OnCheckHd();
      OnCheckLifting();
      OnCheckSlope();
      OnCheckSplitting();
      OnCheckConfinement();
   }
}


BEGIN_MESSAGE_MAP(CSpecDesignPage, CPropertyPage)
	//{{AFX_MSG_MAP(CSpecDesignPage)
	ON_BN_CLICKED(IDC_CHECK_A, OnCheckA)
	ON_BN_CLICKED(IDC_CHECK_HAULING, OnCheckHauling)
	ON_BN_CLICKED(IDC_CHECK_HD, OnCheckHd)
	ON_BN_CLICKED(IDC_CHECK_LIFTING, OnCheckLifting)
	ON_BN_CLICKED(IDC_CHECK_SLOPE, OnCheckSlope)
	ON_BN_CLICKED(IDC_CHECK_SPLITTING, OnCheckSplitting)
	ON_BN_CLICKED(IDC_CHECK_CONFINEMENT, OnCheckConfinement)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecDesignPage message handlers

// Don't allow design if check is disabled.
inline void CheckDesignCtrl(int idc, int idd, CWnd* pme)
{
	CButton* pbut = (CButton*)pme->GetDlgItem(idc);
 	CButton* pbut2 = (CButton*)pme->GetDlgItem(idd);
   if (pbut->GetCheck()==0)
   {
      pbut2->SetCheck(0);
      pbut2->EnableWindow(FALSE);
   }
   else
   {
      pbut2->EnableWindow(TRUE);
   }

}

void CSpecDesignPage::OnCheckA() 
{
   CheckDesignCtrl(IDC_CHECK_A, IDC_DESIGN_A, this);
}

void CSpecDesignPage::OnCheckHauling() 
{
   CheckDesignCtrl(IDC_CHECK_HAULING, IDC_DESIGN_HAULING, this);
}

void CSpecDesignPage::OnCheckHd() 
{
   CheckDesignCtrl(IDC_CHECK_HD, IDC_DESIGN_HD, this);

}

void CSpecDesignPage::OnCheckLifting() 
{
   CheckDesignCtrl(IDC_CHECK_LIFTING, IDC_DESIGN_LIFTING, this);
}

void CSpecDesignPage::OnCheckSlope() 
{
   CheckDesignCtrl(IDC_CHECK_SLOPE, IDC_DESIGN_SLOPE, this);
}

void CSpecDesignPage::OnCheckSplitting() 
{
   CheckDesignCtrl(IDC_CHECK_SPLITTING, IDC_DESIGN_SPLITTING, this);
}

void CSpecDesignPage::OnCheckConfinement() 
{
   CheckDesignCtrl(IDC_CHECK_CONFINEMENT, IDC_DESIGN_CONFINEMENT, this);
}


LRESULT CSpecDesignPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_SPEC_DESIGN );
   return TRUE;
}
