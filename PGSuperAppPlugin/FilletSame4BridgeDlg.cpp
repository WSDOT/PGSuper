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

// FilletSame4BridgeDlg.cpp : implementation file
//
#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "FilletSame4BridgeDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include "PGSuperUnits.h"

// CFilletSame4BridgeDlg dialog

IMPLEMENT_DYNAMIC(CFilletSame4BridgeDlg, CDialog)

CFilletSame4BridgeDlg::CFilletSame4BridgeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFilletSame4BridgeDlg::IDD, pParent)
{
}

CFilletSame4BridgeDlg::~CFilletSame4BridgeDlg()
{
}

void CFilletSame4BridgeDlg::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_UnitValueAndTag( pDX, IDC_FILLET, IDC_FILLET_UNITS, m_Fillet, pDisplayUnits->GetComponentDimUnit() );

	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CFilletSame4BridgeDlg, CDialog)
END_MESSAGE_MAP()

void CFilletSame4BridgeDlg::DownloadData(HaunchInputData* pData, CDataExchange* pDX)
{
   UpdateData(TRUE);

   if (m_Fillet < 0.0)
   {
      AfxMessageBox(_T("Fillet must be zero or greater"), MB_ICONEXCLAMATION);
      pDX->Fail();
   }

   pData->m_FilletType = pgsTypes::fttBridge;
   pData->m_SingleFillet = m_Fillet;
}
