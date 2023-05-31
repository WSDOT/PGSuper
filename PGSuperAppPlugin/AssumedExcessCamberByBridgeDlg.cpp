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

// AssumedExcessCamberByBridgeDlg.cpp : implementation file
//
#include "stdafx.h"
#include "resource.h"
#include "AssumedExcessCamberByBridgeDlg.h"
#include "EditHaunchACamberDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include "PGSuperUnits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CAssumedExcessCamberByBridgeDlg dialog

IMPLEMENT_DYNAMIC(CAssumedExcessCamberByBridgeDlg, CDialog)

CAssumedExcessCamberByBridgeDlg::CAssumedExcessCamberByBridgeDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CAssumedExcessCamberByBridgeDlg::IDD, pParent)
{
}

CAssumedExcessCamberByBridgeDlg::~CAssumedExcessCamberByBridgeDlg()
{
}

void CAssumedExcessCamberByBridgeDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

   CEditHaunchACamberDlg* pParent = (CEditHaunchACamberDlg*)GetParent();
   CBridgeDescription2* pBridge = pParent->GetBridgeDesc();
   Float64 assumedExcessCamber = pBridge->GetAssumedExcessCamber();
   DDX_UnitValueAndTag(pDX, IDC_ASSUMED_EXCESS_CAMBER, IDC_ASSUMED_EXCESS_CAMBER_UNITS, assumedExcessCamber, pDisplayUnits->GetComponentDimUnit());

   if (pDX->m_bSaveAndValidate && pParent->GetAssumedExcessCamberType() == pgsTypes::aecBridge)
   {
      pBridge->SetAssumedExcessCamber(assumedExcessCamber);
   }
}

BEGIN_MESSAGE_MAP(CAssumedExcessCamberByBridgeDlg, CDialog)
END_MESSAGE_MAP()

BOOL CAssumedExcessCamberByBridgeDlg::OnInitDialog()
{
   CEditHaunchACamberDlg* pParent = (CEditHaunchACamberDlg*)GetParent();
   if (!pParent->m_bCanAssumedExcessCamberInputBeEnabled)
   {
      GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER)->EnableWindow(FALSE);
      GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_LABEL)->EnableWindow(FALSE);  
      GetDlgItem(IDC_ASSUMED_EXCESS_CAMBER_UNITS)->EnableWindow(FALSE);
   }
   CDialog::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}
