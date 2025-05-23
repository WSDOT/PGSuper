///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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

// ConstructionLoadDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "ConstructionLoadDlg.h"
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CConstructionLoadDlg dialog

IMPLEMENT_DYNAMIC(CConstructionLoadDlg, CDialog)

CConstructionLoadDlg::CConstructionLoadDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CConstructionLoadDlg::IDD, pParent)
{

}

CConstructionLoadDlg::~CConstructionLoadDlg()
{
}

void CConstructionLoadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_UnitValueAndTag(pDX,IDC_LOAD,IDC_LOAD_UNIT,m_Load,pDisplayUnits->GetOverlayWeightUnit());
}


BEGIN_MESSAGE_MAP(CConstructionLoadDlg, CDialog)
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


// CConstructionLoadDlg message handlers

BOOL CConstructionLoadDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CConstructionLoadDlg::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_CONSTRUCTION_LOADS );
}
