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

// HaunchSame4BridgeDlg.cpp : implementation file
//
#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "HaunchSame4BridgeDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include "PGSuperUnits.h"

// CHaunchSame4BridgeDlg dialog

IMPLEMENT_DYNAMIC(CHaunchSame4BridgeDlg, CDialog)

CHaunchSame4BridgeDlg::CHaunchSame4BridgeDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CHaunchSame4BridgeDlg::IDD, pParent)
{

}

CHaunchSame4BridgeDlg::~CHaunchSame4BridgeDlg()
{
}

void CHaunchSame4BridgeDlg::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_UnitValueAndTag( pDX, IDC_SLAB_OFFSET, IDC_SLAB_OFFSET_UNITS, m_ADim, pDisplayUnits->GetComponentDimUnit() );

	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CHaunchSame4BridgeDlg, CDialog)
END_MESSAGE_MAP()

void CHaunchSame4BridgeDlg::DownloadData(Float64 minA,CString& minValError,HaunchInputData* pData,CDataExchange* pDX)
{
   UpdateData(TRUE);

   if (m_ADim < minA)
   {
      AfxMessageBox( minValError, MB_ICONEXCLAMATION);
      pDX->Fail();
   }

   pData->m_SlabOffsetType = pgsTypes::sotBridge;
   pData->m_SingleSlabOffset = m_ADim;
}
