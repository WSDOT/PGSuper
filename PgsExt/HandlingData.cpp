///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2008  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\HandlingData.h>
#include <WbflAtlExt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CHandlingData
****************************************************************************/


CHandlingData::CHandlingData()
{
   LeftLiftPoint        = 0;
   RightLiftPoint       = 0;
   LeadingSupportPoint  = 0;
   TrailingSupportPoint = 0;
}

CHandlingData::CHandlingData(const CHandlingData& rOther)
{
   MakeCopy(rOther);
}

CHandlingData::~CHandlingData()
{
}

CHandlingData& CHandlingData::operator= (const CHandlingData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CHandlingData::operator==(const CHandlingData& rOther) const
{
   if ( !IsEqual(LeftLiftPoint,rOther.LeftLiftPoint) )
      return false;

   if ( !IsEqual(RightLiftPoint,rOther.RightLiftPoint) )
      return false;

   if ( !IsEqual(LeadingSupportPoint,rOther.LeadingSupportPoint) )
      return false;

   if ( !IsEqual(TrailingSupportPoint,rOther.TrailingSupportPoint) )
      return false;

   return true;
}

bool CHandlingData::operator!=(const CHandlingData& rOther) const
{
   return !operator==(rOther);
}

HRESULT CHandlingData::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;

   CHRException hr;

   try
   {
      CComVariant var;
      var.vt = VT_R8;

      hr = pStrLoad->BeginUnit("HandlingData");

      hr = pStrLoad->get_Property("LeftLiftPoint",&var);
      LeftLiftPoint = var.dblVal;

      hr = pStrLoad->get_Property("RightLiftPoint",&var);
      RightLiftPoint = var.dblVal;

      hr = pStrLoad->get_Property("LeadingSupportPoint",&var);
      LeadingSupportPoint = var.dblVal;

      hr = pStrLoad->get_Property("TrailingSupportPoint",&var);
      TrailingSupportPoint = var.dblVal;

      hr = pStrLoad->EndUnit(); // HandlingData
   }
   catch(...)
   {
      ATLASSERT(0);
   }

   return hr;
}

HRESULT CHandlingData::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit("HandlingData",1.0);
   pStrSave->put_Property("LeftLiftPoint",CComVariant(LeftLiftPoint));
   pStrSave->put_Property("RightLiftPoint",CComVariant(RightLiftPoint));
   pStrSave->put_Property("LeadingSupportPoint",CComVariant(LeadingSupportPoint));
   pStrSave->put_Property("TrailingSupportPoint",CComVariant(TrailingSupportPoint));
   pStrSave->EndUnit(); // HandlingData

   return hr;
}

void CHandlingData::MakeCopy(const CHandlingData& rOther)
{
   LeftLiftPoint        = rOther.LeftLiftPoint;
   RightLiftPoint       = rOther.RightLiftPoint;
   LeadingSupportPoint  = rOther.LeadingSupportPoint;
   TrailingSupportPoint = rOther.TrailingSupportPoint;
}

void CHandlingData::MakeAssignment(const CHandlingData& rOther)
{
   MakeCopy( rOther );
}
