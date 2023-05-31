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

// HaunchByBridgeDlg.cpp : implementation file
//
#include "stdafx.h"
#include "resource.h"
#include "HaunchByBridgeDlg.h"
#include "EditHaunchACamberDlg.h"

#include <EAF\EAFDisplayUnits.h>
#include "PGSuperUnits.h"
#include <PgsExt\HaunchDepthInputConversionTool.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CHaunchByBridgeDlg dialog

IMPLEMENT_DYNAMIC(CHaunchByBridgeDlg, CDialog)

CHaunchByBridgeDlg::CHaunchByBridgeDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CHaunchByBridgeDlg::IDD, pParent)
{

}

CHaunchByBridgeDlg::~CHaunchByBridgeDlg()
{
}

void CHaunchByBridgeDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CEditHaunchACamberDlg* pParent = (CEditHaunchACamberDlg*)GetParent();
   CBridgeDescription2* pBridgeOrig = pParent->GetBridgeDesc();

   // Convert current haunch data if needed
   HaunchDepthInputConversionTool conversionTool(pBridgeOrig,pBroker,false);
   auto convPair = conversionTool.ConvertToSlabOffsetInput(pgsTypes::sotBridge);
   const CBridgeDescription2* pBridge = &convPair.second;

   Float64 slabOffset = pBridge->GetSlabOffset();
   DDX_UnitValueAndTag( pDX, IDC_SLAB_OFFSET, IDC_SLAB_OFFSET_UNITS, slabOffset, pDisplayUnits->GetComponentDimUnit() );

   if (pDX->m_bSaveAndValidate && pParent->GetSlabOffsetType() == pgsTypes::sotBridge)
   {
      // Get min slab offset value and build error message for too small of A
      Float64 minSlabOffset = pBridge->GetMinSlabOffset();
      if (::IsLT(slabOffset, minSlabOffset))
      {
         CString strMinValError;
         strMinValError.Format(_T("Slab Offset must be greater or equal to slab depth + fillet (%s)"), FormatDimension(minSlabOffset, pDisplayUnits->GetComponentDimUnit()));
         AfxMessageBox(strMinValError, MB_ICONERROR | MB_OK);
         pDX->PrepareEditCtrl(IDC_SLAB_OFFSET);
         pDX->Fail();
      }

      pBridgeOrig->SetSlabOffset(slabOffset);
   }
}


BEGIN_MESSAGE_MAP(CHaunchByBridgeDlg, CDialog)
END_MESSAGE_MAP()
