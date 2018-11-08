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

// AssExcessCamberSame4BridgeDlg.cpp : implementation file
//
#include "stdafx.h"
#include "resource.h"
#include "AssExcessCamberSame4BridgeDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include "PGSuperUnits.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CAssExcessCamberSame4BridgeDlg dialog

IMPLEMENT_DYNAMIC(CAssExcessCamberSame4BridgeDlg, CDialog)

CAssExcessCamberSame4BridgeDlg::CAssExcessCamberSame4BridgeDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CAssExcessCamberSame4BridgeDlg::IDD, pParent),
   m_bDoEnable(true)
{
}

CAssExcessCamberSame4BridgeDlg::~CAssExcessCamberSame4BridgeDlg()
{
}

void CAssExcessCamberSame4BridgeDlg::DoDataExchange(CDataExchange* pDX)
{
   if (m_bDoEnable)
   {
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker, IEAFDisplayUnits, pDisplayUnits);

      DDX_UnitValueAndTag(pDX, IDC_ASSEXCESSCAMBER, IDC_ASSEXCESSCAMBER_UNITS, m_AssExcessCamber, pDisplayUnits->GetComponentDimUnit());
   }
   else
   {
      GetDlgItem(IDC_ASSEXCESSCAMBER)->EnableWindow(FALSE);
      GetDlgItem(IDC_ASSEXCESSCAMBER)->SetWindowText(_T(""));
      GetDlgItem(IDC_ASSEXCESSCAMBER_UNITS)->EnableWindow(FALSE);
      GetDlgItem(IDC_ASSEXCESSCAMBER_LABEL)->EnableWindow(FALSE);
   }

	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAssExcessCamberSame4BridgeDlg, CDialog)
END_MESSAGE_MAP()

void CAssExcessCamberSame4BridgeDlg::DownloadData(HaunchInputData* pData, CDataExchange* pDX)
{
   UpdateData(TRUE);

   pData->m_AssExcessCamberType = pgsTypes::aecBridge;
   pData->m_SingleAssExcessCamber = m_AssExcessCamber;
}
