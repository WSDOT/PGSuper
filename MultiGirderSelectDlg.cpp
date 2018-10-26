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
//
#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperColors.h"
#include "HtmlHelp\HelpTopics.hh"
#include "MultiGirderSelectDlg.h"
#include <IFace\Bridge.h>
#include "PGSuperAppPlugin\resource.h"

// CMultiGirderSelectDlg dialog

IMPLEMENT_DYNAMIC(CMultiGirderSelectDlg, CDialog)

CMultiGirderSelectDlg::CMultiGirderSelectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMultiGirderSelectDlg::IDD, pParent)
{
   m_pGrid = new CMultiGirderSelectGrid();
}

CMultiGirderSelectDlg::~CMultiGirderSelectDlg()
{
   delete m_pGrid;
}

void CMultiGirderSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   if (pDX->m_bSaveAndValidate)
   {
      m_SelGdrs = m_pGrid->GetData();
   }
}

BEGIN_MESSAGE_MAP(CMultiGirderSelectDlg, CDialog)
   ON_BN_CLICKED(IDC_SELECT_ALL, &CMultiGirderSelectDlg::OnBnClickedSelectAll)
   ON_BN_CLICKED(IDC_CLEAR_ALL, &CMultiGirderSelectDlg::OnBnClickedClearAll)
END_MESSAGE_MAP()

BOOL CMultiGirderSelectDlg::OnInitDialog()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CDialog::OnInitDialog();

 	m_pGrid->SubclassDlgItem(IDC_SELECT_GRID, this);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IBridge,pBridge);

   // need list of girders/spans
   SpanGirderOnCollection coll;
   SpanIndexType ns = pBridge->GetSpanCount();
   for (SpanIndexType is=0; is<ns; is++)
   {
      GirderIndexType ng = pBridge->GetGirderCount(is);
      std::vector<bool> gdrson;
      gdrson.assign(ng, false); // set all to false

      coll.push_back(gdrson);
   }

   // set selected girders
   for(std::vector<SpanGirderHashType>::iterator it = m_SelGdrs.begin(); it != m_SelGdrs.end(); it++)
   {
      SpanIndexType span;
      GirderIndexType gdr;
      UnhashSpanGirder(*it, &span, &gdr);

      if (span<ns)
      {
         std::vector<bool>& rgdrson = coll[span];
         if (gdr < (GirderIndexType)rgdrson.size())
         {
            rgdrson[gdr] = true;
         }
         else
         {
            ATLASSERT(0); // might be a problem?
         }
      }
      else
      {
         ATLASSERT(0); // might be a problem?
      }
   }

   m_pGrid->CustomInit(coll,pgsGirderLabel::GetGirderLabel);

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CMultiGirderSelectDlg::OnBnClickedSelectAll()
{
   m_pGrid->SetAllValues(true);
}

void CMultiGirderSelectDlg::OnBnClickedClearAll()
{
   m_pGrid->SetAllValues(false);
}
