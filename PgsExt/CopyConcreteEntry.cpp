///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

// CopyConcreteEntry.cpp : implementation file
//

#include <PgsExt\PgsExtLib.h>
#include "resource.h"

#include "CopyConcreteEntry.h"
#include "..\Documentation\PGSuper.hh"

#include <PsgLib\ConcreteLibraryEntry.h>


#include <AgentTools.h>
#include <IFace\Project.h>
#include <EAF\EAFUtilities.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCopyConcreteEntry dialog


CCopyConcreteEntry::CCopyConcreteEntry(bool isPrestressed, CWnd* pParent /*=nullptr*/)
	: CDialog(CCopyConcreteEntry::IDD, pParent),
   m_IsPrestressed(isPrestressed) ,
   m_ConcreteEntry(nullptr)
{
	//{{AFX_DATA_INIT(CCopyConcreteEntry)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CCopyConcreteEntry::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCopyConcreteEntry)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

	DDX_CBStringExactCase(pDX, IDC_CONC_ENTRIES, m_Concrete);

   if ( pDX->m_bSaveAndValidate )
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      GET_IFACE2(pBroker,ILibrary,pLib);
      m_ConcreteEntry = pLib->GetConcreteEntry(m_Concrete);
   }
}


BEGIN_MESSAGE_MAP(CCopyConcreteEntry, CDialog)
	//{{AFX_MSG_MAP(CCopyConcreteEntry)
	ON_BN_CLICKED(IDHELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCopyConcreteEntry message handlers

BOOL CCopyConcreteEntry::OnInitDialog() 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2( pBroker, ILibraryNames, pLibNames);
   std::vector<std::_tstring> names;
   pLibNames->EnumConcreteNames(&names);

   CComboBox* pBox = (CComboBox*)GetDlgItem( IDC_CONC_ENTRIES );
   ASSERT( pBox );
   if ( names.size() == 0 )
   {
      pBox->EnableWindow(FALSE);
      GetDlgItem(IDOK)->EnableWindow(FALSE);
      SendMessage(DM_SETDEFID, IDCANCEL); // make the Cancel button default
      GetDlgItem(IDC_LABEL)->SetWindowText(_T("Concrete Library is empty"));
   }
   else
   {

      bool first = true;
      std::vector<std::_tstring>::iterator iter;
      for ( iter = names.begin(); iter < names.end(); iter++ )
      {
         CString conc( (*iter).c_str() );
         pBox->AddString( conc );

         if (first)
         {
            m_Concrete = conc;
            first = false;
         }
      }
   }

	CDialog::OnInitDialog();

   // hide message if not prestressed
   if (!m_IsPrestressed)
   {
      CWnd* cw = GetDlgItem(IDC_WARNING);
      cw->ShowWindow(SW_HIDE);
      cw = GetDlgItem(IDC_WARNOTE);
      cw->ShowWindow(SW_HIDE);
   }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCopyConcreteEntry::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_COPY_CONCRETE );
}
