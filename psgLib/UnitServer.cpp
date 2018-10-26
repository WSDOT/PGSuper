///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#include "StdAfx.h"
#include <PsgLib\UnitServer.h>
#include <Units\Units.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


HRESULT PSGLIBFUNC ConfigureUnitServer(IUnitServer* server)
{
   // this is not a general-purpose conversion. 
   // We assume a configuration and enforce it by assertion
   unitMass mass = unitSysUnitsMgr::GetMassUnit();
   if (mass.UnitTag() != _T("kg"))
   {
      ATLASSERT(0);
      return E_FAIL;
   }

   unitLength length = unitSysUnitsMgr::GetLengthUnit();
   if (length.UnitTag() != _T("m"))
   {
      ATLASSERT(0);
      return E_FAIL;
   }

   unitTime time = unitSysUnitsMgr::GetTimeUnit();
   if (time.UnitTag() != _T("sec"))
   {
      ATLASSERT(0);
      return E_FAIL;
   }

   unitTemperature temp = unitSysUnitsMgr::GetTemperatureUnit();
   if (temp.UnitTag() != _T("C"))
   {
      ATLASSERT(0);
      return E_FAIL;
   }

   unitAngle angle = unitSysUnitsMgr::GetAngleUnit();
   if (angle.UnitTag() != _T("rad"))
   {
      ATLASSERT(0);
      return E_FAIL;
   }

   HRESULT hr = server->SetBaseUnits(CComBSTR("kg"), CComBSTR("m"), CComBSTR("sec"), CComBSTR("C"), CComBSTR("rad"));
   ATLASSERT(SUCCEEDED(hr));

   return hr;
}
