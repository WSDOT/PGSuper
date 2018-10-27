///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

// BoundaryConditionDlg.cpp : implementation file
//

#include <PgsExt\PgsExtLib.h>
#include "BoundaryConditionDlg.h"
#include <MFCTools\CustomDDX.h>
#include <EAF\EAFUtilities.h>
#include <IFace\Project.h>

#include <PgsExt\PierData2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CBoundaryConditionDlg dialog

IMPLEMENT_DYNAMIC(CBoundaryConditionDlg, CDialog)

CBoundaryConditionDlg::CBoundaryConditionDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CBoundaryConditionDlg::IDD, pParent)
{
   m_BoundaryCondition = pgsTypes::bctContinuousAfterDeck;
}

CBoundaryConditionDlg::~CBoundaryConditionDlg()
{
}

void CBoundaryConditionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   DDX_Control(pDX,IDC_BOUNDARY_CONDITION,m_cbBoundaryCondition);
   DDX_CBItemData(pDX, IDC_BOUNDARY_CONDITION, m_BoundaryCondition);
}


BEGIN_MESSAGE_MAP(CBoundaryConditionDlg, CDialog)
END_MESSAGE_MAP()


// CBoundaryConditionDlg message handlers

BOOL CBoundaryConditionDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   ATLASSERT(pIBridgeDesc->GetPier(m_PierIdx)->IsBoundaryPier());

   std::vector<pgsTypes::BoundaryConditionType> connections = pIBridgeDesc->GetBoundaryConditionTypes(m_PierIdx);

   m_cbBoundaryCondition.SetPierType(PIERTYPE_INTERMEDIATE);
   m_cbBoundaryCondition.Initialize(true,connections);

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}
