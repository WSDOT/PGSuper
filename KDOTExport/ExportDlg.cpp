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
//
// CExportDlg.cpp : implementation file
//

#include "stdafx.h"

#include <ATLBase.h>

#include "ExportDlg.h"

#include <IFace\Bridge.h>

#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExportDlg dialog

/*--------------------------------------------------------------------*/
CExportDlg::CExportDlg(IBroker* pBroker,CWnd* pParent /*=nullptr*/)
	: CDialog(CExportDlg::IDD, pParent)
{
   m_pBroker  = pBroker;
	//{{AFX_DATA_INIT(CExportDlg)
	//}}AFX_DATA_INIT

   m_pGrid = new CMultiGirderSelectGrid();
}

/*--------------------------------------------------------------------*/
CExportDlg::~CExportDlg()
{
   delete m_pGrid;
}

/*--------------------------------------------------------------------*/
void CExportDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExportDlg)
	//}}AFX_DATA_MAP

   if ( pDX->m_bSaveAndValidate )
   {
      // Girders
      m_GirderKeys = m_pGrid->GetData();
      if ( m_GirderKeys.empty() )
      {
         pDX->PrepareCtrl(IDC_SELECT_GRID);
         AfxMessageBox(IDS_E_NOGIRDERS);
         pDX->Fail();
      }
   }
}

/*--------------------------------------------------------------------*/
BEGIN_MESSAGE_MAP(CExportDlg, CDialog)
	//{{AFX_MSG_MAP(CExportDlg)
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_SELECT_ALL, &CExportDlg::OnBnClickedSelectAll)
   ON_BN_CLICKED(IDC_CLEAR_ALL, &CExportDlg::OnBnClickedClearAll)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExportDlg message handlers


/*--------------------------------------------------------------------*/
BOOL CExportDlg::OnInitDialog() 
{
 	m_pGrid->SubclassDlgItem(IDC_SELECT_GRID, this);

	/* Perform the root dialog initialization */
	CDialog::OnInitDialog();

	/* Get interface pointer to Bridge Agent */
	GET_IFACE( IBridge, pBridge ); 

   GroupGirderOnCollection coll;
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      std::vector<bool> gdrson;
      gdrson.assign(nGirders, false); // set all to false

      coll.push_back(gdrson);
   }

   // set selected girders
   for(std::vector<CGirderKey>::iterator it = m_GirderKeys.begin(); it != m_GirderKeys.end(); it++)
   {
      CGirderKey& girderKey(*it);

      if (girderKey.groupIndex < nGroups)
      {
         std::vector<bool>& rgdrson = coll[girderKey.groupIndex];
         if (girderKey.girderIndex < (GirderIndexType)rgdrson.size())
         {
            rgdrson[girderKey.girderIndex] = true;
         }
         else
         {
            ATLASSERT(false); // might be a problem?
         }
      }
      else
      {
         ATLASSERT(false); // might be a problem?
      }
   }

   m_pGrid->CustomInit(coll,pgsGirderLabel::GetGirderLabel);

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


/*--------------------------------------------------------------------*/
void CExportDlg::OnHelp() 
{
   EAFHelp(_T("KDOT"),IDH_DIALOG_EXPORTTXDOTCADDATA);
}


void CExportDlg::OnBnClickedSelectAll()
{
   m_pGrid->SetAllValues(true);
}

void CExportDlg::OnBnClickedClearAll()
{
   m_pGrid->SetAllValues(false);
}
