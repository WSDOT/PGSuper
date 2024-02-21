///////////////////////////////////////////////////////////////////////
// IEPluginExample
// Copyright © 1999-2024  Washington State Department of Transportation
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

// PGSuperDataImporter.cpp : Implementation of CPGSuperDataImporter
#include "stdafx.h"
#include "IEPluginExample.h"
#include "PGSuperDataImporter.h"

#include "PGSuperInterfaces.h"

#include <EAF\EAFAutoProgress.h>


/////////////////////////////////////////////////////////////////////////////
// CPGSuperDataImporter
STDMETHODIMP CPGSuperDataImporter::Init(UINT nCmdID)
{
   return S_OK;
}

STDMETHODIMP CPGSuperDataImporter::GetMenuText(BSTR*  bstrText) const
{
   *bstrText = CComBSTR("Project Data from External Data Source into this PGSuper project");
   return S_OK;
}

STDMETHODIMP CPGSuperDataImporter::GetBitmapHandle(HBITMAP* phBmp) const
{
   *phBmp = nullptr;
   return S_OK;
}

STDMETHODIMP CPGSuperDataImporter::GetCommandHintText(BSTR*  bstrText) const
{
   *bstrText = CComBSTR("Status line hint text\nTool tip text");
   return S_OK;   
}

STDMETHODIMP CPGSuperDataImporter::Import(IBroker* pBroker)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   int st = AfxMessageBox(_T("Simulate importing data from an external source by changing the alignment. Click Yes to do it!"),MB_YESNO);

   if (st==IDYES)
   {
      CompoundCurveData hcData;
      hcData.PIStation = 15.;
      hcData.FwdTangent = 2.25;
      hcData.Radius = 600;
      hcData.bFwdTangent = true;
      hcData.EntrySpiral = 0;
      hcData.ExitSpiral = 0;

      AlignmentData2 alignmentData;

      alignmentData.CompoundCurves.push_back(hcData);
      alignmentData.Direction = 2;
      alignmentData.RefStation = 150.0;
      alignmentData.xRefPoint = 50;
      alignmentData.yRefPoint = 50;

      GET_IFACE2(pBroker,IEvents,pEvents);
      pEvents->HoldEvents();

      GET_IFACE2(pBroker,IRoadwayData,pRoadway);
      pRoadway->SetAlignmentData2(alignmentData);

      pEvents->FirePendingEvents();

      return S_OK;
   }
   else
   {
      return E_FAIL;
   }
}
