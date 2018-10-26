///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
   LeftStoragePoint     = 0;
   RightStoragePoint    = 0;
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
   if ( !IsEqual(LeftStoragePoint,rOther.LeftStoragePoint) )
      return false;

   if ( !IsEqual(RightStoragePoint,rOther.RightStoragePoint) )
      return false;

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

      hr = pStrLoad->BeginUnit(_T("HandlingData"));

      Float64 version;
      pStrLoad->get_Version(&version);
      if ( 1.0 < version )
      {
         hr = pStrLoad->get_Property(_T("LeftStoragePoint"),&var);
         LeftStoragePoint = var.dblVal;

         hr = pStrLoad->get_Property(_T("RightStoragePoint"),&var);
         RightStoragePoint = var.dblVal;
      }

      hr = pStrLoad->get_Property(_T("LeftLiftPoint"),&var);
      LeftLiftPoint = var.dblVal;

      hr = pStrLoad->get_Property(_T("RightLiftPoint"),&var);
      RightLiftPoint = var.dblVal;

      hr = pStrLoad->get_Property(_T("LeadingSupportPoint"),&var);
      LeadingSupportPoint = var.dblVal;

      hr = pStrLoad->get_Property(_T("TrailingSupportPoint"),&var);
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

   pStrSave->BeginUnit(_T("HandlingData"),2.0);
   pStrSave->put_Property(_T("LeftStoragePoint"),CComVariant(LeftStoragePoint));
   pStrSave->put_Property(_T("RightStoragePoint"),CComVariant(RightStoragePoint));
   pStrSave->put_Property(_T("LeftLiftPoint"),CComVariant(LeftLiftPoint));
   pStrSave->put_Property(_T("RightLiftPoint"),CComVariant(RightLiftPoint));
   pStrSave->put_Property(_T("LeadingSupportPoint"),CComVariant(LeadingSupportPoint));
   pStrSave->put_Property(_T("TrailingSupportPoint"),CComVariant(TrailingSupportPoint));
   pStrSave->EndUnit(); // HandlingData

   return hr;
}

void CHandlingData::MakeCopy(const CHandlingData& rOther)
{
   LeftStoragePoint     = rOther.LeftStoragePoint;
   RightStoragePoint    = rOther.RightStoragePoint;
   LeftLiftPoint        = rOther.LeftLiftPoint;
   RightLiftPoint       = rOther.RightLiftPoint;
   LeadingSupportPoint  = rOther.LeadingSupportPoint;
   TrailingSupportPoint = rOther.TrailingSupportPoint;
}

void CHandlingData::MakeAssignment(const CHandlingData& rOther)
{
   MakeCopy( rOther );
}
