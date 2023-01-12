///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// SpecDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "SpecDlg.h"

#include <IFace\DocumentType.h>
#include <IFace\Project.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpecDlg dialog


CSpecDlg::CSpecDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CSpecDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSpecDlg)
	//}}AFX_DATA_INIT
	m_Spec = _T("");
}


void CSpecDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpecDlg)
	//}}AFX_DATA_MAP
   std::_tstring strSpec = m_Spec;
   DDX_CBStringExactCase(pDX, IDC_SPEC, strSpec);

   if ( pDX->m_bSaveAndValidate )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      GET_IFACE2(pBroker, IDocumentType, pDocType);
      bool bIsPGSplice = pDocType->IsPGSpliceDocument();

      GET_IFACE2(pBroker, ILibrary, pLib);

      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpec.c_str() );
      if ( (bIsPGSplice && pSpecEntry->GetLossMethod() != LOSSES_TIME_STEP) )
      {
         pDX->PrepareCtrl(IDC_SPEC);
         AfxMessageBox(_T("Prestress loss method must be set to time-step in the project criteria for spliced girder analysis.\n\nSelect a different project criteria"));
         pDX->Fail();
      }
      else
      {
         if (pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP && 
             pgsTypes::hlcZeroCamber != pSpecEntry->GetHaunchLoadComputationType())
         {
            pDX->PrepareCtrl(IDC_SPEC);
            AfxMessageBox(_T("Haunch load computation must be set to assume \"Zero Camber\" when the Time-Step method is used for losses.\n\nSelect a different project criteria"));
            pDX->Fail();
         }

         m_Spec = strSpec;
      }
   }
}


BEGIN_MESSAGE_MAP(CSpecDlg, CDialog)
	//{{AFX_MSG_MAP(CSpecDlg)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecDlg message handlers

BOOL CSpecDlg::OnInitDialog() 
{
   CComboBox* pBox = (CComboBox*)GetDlgItem( IDC_SPEC );
   ASSERT( pBox );

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, ILibraryNames, pLibNames );

   std::vector<std::_tstring> specs;
   pLibNames->EnumSpecNames( &specs );

   for (const auto& spec : specs)
   {
      pBox->AddString( spec.c_str() );
   }

   CDialog::OnInitDialog();
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpecDlg::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_DIALOG_DESIGNCRITERIA );
}
