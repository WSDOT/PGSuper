///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <PsgLib\HandlingData.h>

/****************************************************************************
CLASS
   CHandlingData
****************************************************************************/


CHandlingData::CHandlingData()
{
   LeftReleasePoint     = -1; // -1 means supported at left end at release
   RightReleasePoint    = -1; // -1 means supported at right end at release
   LeftStoragePoint     = -1; // -1 means storage at final bearing locations
   RightStoragePoint    = -1; // -1 means storage at final bearing locations
   LeftLiftPoint        = 0;
   RightLiftPoint       = 0;
   LeadingSupportPoint  = 0;
   TrailingSupportPoint = 0;
   pHaulTruckLibraryEntry = nullptr;
}

bool CHandlingData::operator==(const CHandlingData& rOther) const
{
   if ( !IsEqual(LeftReleasePoint,rOther.LeftReleasePoint) )
   {
      return false;
   }

   if ( !IsEqual(RightReleasePoint,rOther.RightReleasePoint) )
   {
      return false;
   }

   if ( !IsEqual(LeftStoragePoint,rOther.LeftStoragePoint) )
   {
      return false;
   }

   if ( !IsEqual(RightStoragePoint,rOther.RightStoragePoint) )
   {
      return false;
   }

   if ( !IsEqual(LeftLiftPoint,rOther.LeftLiftPoint) )
   {
      return false;
   }

   if ( !IsEqual(RightLiftPoint,rOther.RightLiftPoint) )
   {
      return false;
   }

   if ( !IsEqual(LeadingSupportPoint,rOther.LeadingSupportPoint) )
   {
      return false;
   }

   if ( !IsEqual(TrailingSupportPoint,rOther.TrailingSupportPoint) )
   {
      return false;
   }

   if ( HaulTruckName != rOther.HaulTruckName )
   {
      return false;
   }

   return true;
}

bool CHandlingData::operator!=(const CHandlingData& rOther) const
{
   return !operator==(rOther);
}

HRESULT CHandlingData::Load(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress)
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

      if ( 2.0 < version )
      {
         hr = pStrLoad->get_Property(_T("LeftReleasePoint"),&var);
         LeftReleasePoint = var.dblVal;

         hr = pStrLoad->get_Property(_T("RightReleasePoint"),&var);
         RightReleasePoint = var.dblVal;
      }

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

      if ( 3 < version )
      {
         // added in version 4
         var.vt = VT_BSTR;
         hr = pStrLoad->get_Property(_T("HaulTruck"),&var);
         HaulTruckName = OLE2T(var.bstrVal);
      }

      hr = pStrLoad->EndUnit(); // HandlingData
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   return hr;
}

HRESULT CHandlingData::Save(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("HandlingData"),4.0);
   pStrSave->put_Property(_T("LeftReleasePoint"),CComVariant(LeftReleasePoint)); // added in version 3
   pStrSave->put_Property(_T("RightReleasePoint"),CComVariant(RightReleasePoint)); // added in version 3
   pStrSave->put_Property(_T("LeftStoragePoint"),CComVariant(LeftStoragePoint)); // added in version 2
   pStrSave->put_Property(_T("RightStoragePoint"),CComVariant(RightStoragePoint)); // added in version 2
   pStrSave->put_Property(_T("LeftLiftPoint"),CComVariant(LeftLiftPoint));
   pStrSave->put_Property(_T("RightLiftPoint"),CComVariant(RightLiftPoint));
   pStrSave->put_Property(_T("LeadingSupportPoint"),CComVariant(LeadingSupportPoint));
   pStrSave->put_Property(_T("TrailingSupportPoint"),CComVariant(TrailingSupportPoint));
   pStrSave->put_Property(_T("HaulTruck"),CComVariant(HaulTruckName.c_str())); // added in version 4
   pStrSave->EndUnit(); // HandlingData

   return hr;
}

#if defined _DEBUG
void CHandlingData::AssertValid()
{
   //ATLASSERT(!HaulTruckName.empty());
}
#endif
