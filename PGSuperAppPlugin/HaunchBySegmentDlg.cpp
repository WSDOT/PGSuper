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

// HaunchSame4Bridge.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "HaunchBySegmentDlg.h"
#include "EditHaunchDlg.h"

#include <IFace\DocumentType.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



// CHaunchBySegmentDlg dialog

IMPLEMENT_DYNAMIC(CHaunchBySegmentDlg, CDialog)

CHaunchBySegmentDlg::CHaunchBySegmentDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CHaunchBySegmentDlg::IDD, pParent)
{
   m_pGrid = nullptr;
}

CHaunchBySegmentDlg::~CHaunchBySegmentDlg()
{
}

void CHaunchBySegmentDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IDocumentType, pDocType);
   if (pDocType->IsPGSuperDocument())
   {
      if (FALSE == m_pGrid->UpdateData(pDX->m_bSaveAndValidate))
      {
         pDX->Fail();
      }
   }
   else
   {
      CGXTabBeam& beam = m_TabWnd.GetBeam();
      auto nTabs = beam.GetCount();
      for (auto i = 0; i < nTabs; i++)
      {
         auto& tabInfo = beam.GetTab(i);
         CHaunchSegmentGrid* pGrid = (CHaunchSegmentGrid*)tabInfo.pExtra;
         if (FALSE == pGrid->UpdateData(pDX->m_bSaveAndValidate))
         {
            pDX->Fail();
         }
      }
   }
}

BEGIN_MESSAGE_MAP(CHaunchBySegmentDlg, CDialog)
END_MESSAGE_MAP()

BOOL CHaunchBySegmentDlg::OnInitDialog()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IDocumentType, pDocType);
   if (pDocType->IsPGSuperDocument())
   {
      m_pGrid = new CHaunchSegmentGrid;
      m_pGrid->SubclassDlgItem(IDC_HAUNCH_GRID, this);
      m_pGrid->CustomInit(ALL_GROUPS);
   }
   else
   {
      m_TabWnd.SubclassDlgItem(IDC_HAUNCH_GRID, this);
      CEditHaunchACamberDlg* pParent = (CEditHaunchACamberDlg*)GetParent();
      const CBridgeDescription2* pBridge = pParent->GetBridgeDesc();
      GroupIndexType nGroups = pBridge->GetGirderGroupCount();
      for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
      {
         CString strLabel;
         strLabel.Format(_T("Group %d"), LABEL_GROUP(grpIdx));

         auto pGrid = std::make_unique<CHaunchSegmentGrid>();
         pGrid->Create(0, CRect(0, 0, 1, 1), &m_TabWnd, m_TabWnd.GetNextID());

         m_TabWnd.AttachWnd(pGrid.get(), strLabel);

         pGrid->CustomInit(grpIdx);

         pGrid.release();
      }
   }


   CDialog::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}
