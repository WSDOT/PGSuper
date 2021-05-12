///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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
#include "stdafx.h"
#include "resource.h"
#include "PGSuperColors.h"
#include "MultiGirderSelectDlg.h"
#include <IFace\Bridge.h>

// CMultiGirderSelectDlg dialog

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CMultiGirderSelectDlg, CDialog)

CMultiGirderSelectDlg::CMultiGirderSelectDlg(CWnd* pParent /*=nullptr*/)
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
      m_GirderKeys = m_pGrid->GetData();

      if (m_GirderKeys.empty())
      {
         ::AfxMessageBox(_T("At least one girder must be selected"),MB_ICONEXCLAMATION | MB_OK),
         pDX->Fail();
      }
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

   // need list of groups/girders
   GroupGirderOnCollection coll;
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      std::vector<bool> gdrson;
      gdrson.assign(nGirders, false); // set all to false

      coll.push_back(gdrson);
   }

   // set selected girders
   for(std::vector<CGirderKey>::iterator it = m_GirderKeys.begin(); it != m_GirderKeys.end(); it++)
   {
      const CGirderKey& girderKey(*it);

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

void CMultiGirderSelectDlg::OnBnClickedSelectAll()
{
   m_pGrid->SetAllValues(true);
}

void CMultiGirderSelectDlg::OnBnClickedClearAll()
{
   m_pGrid->SetAllValues(false);
}
