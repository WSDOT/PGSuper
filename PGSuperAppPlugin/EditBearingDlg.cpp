///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

// EditBearingDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "EditBearingDlg.h"

#include <EAF\EAFMainFrame.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#include "PGSuperUnits.h"

#include <PgsExt\BridgeDescription2.h>

IMPLEMENT_DYNAMIC(CEditBearingDlg, CDialog)

CEditBearingDlg::CEditBearingDlg(const CBridgeDescription2* pBridgeDesc, CWnd* pParent /*=nullptr*/)
	: CDialog(CEditBearingDlg::IDD, pParent)
{
   InitializeData(pBridgeDesc);
}

CEditBearingDlg::CEditBearingDlg(const BearingInputData* pBearingData, CWnd* pParent /*=nullptr*/)
	: CDialog(CEditBearingDlg::IDD, pParent)
{
   m_BearingInputData = *pBearingData;
}

CEditBearingDlg::~CEditBearingDlg()
{
}

void CEditBearingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   INIT_UV_PROTOTYPE( rptLengthUnitValue,  cmpdim,  pDisplayUnits->GetComponentDimUnit(), true );

   DDX_CBItemData(pDX, IDC_BRG_TYPE, m_BearingInputData.m_BearingType);

   if (pDX->m_bSaveAndValidate)
   {
      switch(m_BearingInputData.m_BearingType)
      {
      case pgsTypes::brtBridge:
         m_BearingSame4BridgeDlg.DownloadData(&m_BearingInputData,pDX);
         break;
      case pgsTypes::brtPier:
         m_BearingPierByPierDlg.DownloadData(&m_BearingInputData,pDX);
         break;
      case pgsTypes::brtGirder:
         m_BearingGdrByGdrDlg.DownloadData(&m_BearingInputData,pDX);
         break;
      default:
         ATLASSERT(0);
         break;
      };

   }
   else
   {
      // Set data values in embedded dialogs
      m_BearingSame4BridgeDlg.UploadData(m_BearingInputData);
      m_BearingPierByPierDlg.UploadData(this->m_BearingInputData);
      m_BearingGdrByGdrDlg.UploadData(this->m_BearingInputData);
   }
}


BEGIN_MESSAGE_MAP(CEditBearingDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_BRG_TYPE, &CEditBearingDlg::OnCbnSelchangeBrType)
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


// CEditBearingDlg message handlers

BOOL CEditBearingDlg::OnInitDialog()
{
   // Embed dialogs for into current. A discription may be found at
   // http://www.codeproject.com/KB/dialog/embedded_dialog.aspx

   // Set up embedded dialogs
   {
      CWnd* pBox = GetDlgItem(IDC_BRG_BOX);
      pBox->ShowWindow(SW_HIDE);

      CRect boxRect;
      pBox->GetWindowRect(&boxRect);
      ScreenToClient(boxRect);

      VERIFY(m_BearingSame4BridgeDlg.Create(CBearingSame4BridgeDlg::IDD, this));
      VERIFY(m_BearingSame4BridgeDlg.SetWindowPos( GetDlgItem(IDC_BRG_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE));//|SWP_NOMOVE));

      VERIFY(m_BearingPierByPierDlg.Create(CBearingPierByPierDlg::IDD, this));
      VERIFY(m_BearingPierByPierDlg.SetWindowPos( GetDlgItem(IDC_BRG_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE));//|SWP_NOMOVE));

      VERIFY(m_BearingGdrByGdrDlg.Create(CBearingGdrByGdrDlg::IDD, this));
      VERIFY(m_BearingGdrByGdrDlg.SetWindowPos( GetDlgItem(IDC_BRG_BOX), boxRect.left, boxRect.top, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE));//|SWP_NOMOVE));
   }

   // Slab offset type combo
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_BRG_TYPE);
   int sqidx = pBox->AddString( BearingTypeAsString(pgsTypes::brtBridge));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::brtBridge);
   sqidx = pBox->AddString( BearingTypeAsString(pgsTypes::brtPier));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::brtPier);
   sqidx = pBox->AddString( BearingTypeAsString(pgsTypes::brtGirder));
   pBox->SetItemData(sqidx,(DWORD)pgsTypes::brtGirder);

   CDialog::OnInitDialog();

   OnCbnSelchangeBrType();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditBearingDlg::InitializeData(const CBridgeDescription2* pBridgeDesc)
{
   m_BearingInputData.CopyFromBridgeDescription(pBridgeDesc);
}

void CEditBearingDlg::OnCbnSelchangeBrType()
{
   CComboBox* pBox =(CComboBox*)GetDlgItem(IDC_BRG_TYPE);
   
   m_BearingInputData.m_BearingType = (pgsTypes::BearingType)pBox->GetItemData(pBox->GetCurSel());

   switch(m_BearingInputData.m_BearingType)
   {
   case pgsTypes::brtBridge:
      m_BearingSame4BridgeDlg.ShowWindow(SW_SHOW);
      m_BearingPierByPierDlg.ShowWindow(SW_HIDE);
      m_BearingGdrByGdrDlg.ShowWindow(SW_HIDE);

      break;
   case  pgsTypes::brtPier:
      m_BearingSame4BridgeDlg.ShowWindow(SW_HIDE);
      m_BearingPierByPierDlg.ShowWindow(SW_SHOW);
      m_BearingGdrByGdrDlg.ShowWindow(SW_HIDE);
      break;
   case  pgsTypes::brtGirder:
      m_BearingSame4BridgeDlg.ShowWindow(SW_HIDE);
      m_BearingPierByPierDlg.ShowWindow(SW_HIDE);
      m_BearingGdrByGdrDlg.ShowWindow(SW_SHOW);
      break;
   default:
      ATLASSERT(0);
      break;
   };
}

void CEditBearingDlg::ModifyBridgeDescr(CBridgeDescription2* pBridgeDesc)
{
   m_BearingInputData.CopyToBridgeDescription(pBridgeDesc);
}

void CEditBearingDlg::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_EDIT_BEARINGS);
}
