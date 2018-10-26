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

// BridgeDescEnvironmental.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "BridgeDescEnvironmental.h"
#include "BridgeDescDlg.h"
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescEnvironmental property page

IMPLEMENT_DYNCREATE(CBridgeDescEnvironmental, CPropertyPage)

CBridgeDescEnvironmental::CBridgeDescEnvironmental() : CPropertyPage(CBridgeDescEnvironmental::IDD)
{
	//{{AFX_DATA_INIT(CBridgeDescEnvironmental)
	m_Exposure = -1;
	m_RelHumidity = 0.0;
	//}}AFX_DATA_INIT
}

CBridgeDescEnvironmental::~CBridgeDescEnvironmental()
{
}

void CBridgeDescEnvironmental::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBridgeDescEnvironmental)
	DDX_Radio(pDX, IDC_EXPOSURE, m_Exposure);
	DDX_Text(pDX, IDC_RELHUMIDITY, m_RelHumidity);
	DDV_MinMaxDouble(pDX, m_RelHumidity, 0., 100.);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBridgeDescEnvironmental, CPropertyPage)
	//{{AFX_MSG_MAP(CBridgeDescEnvironmental)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescMisc message handlers

BOOL CBridgeDescEnvironmental::OnInitDialog() 
{
	
	CPropertyPage::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CBridgeDescEnvironmental::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_BRIDGEDESC_ENVIRONMENTAL );
}
