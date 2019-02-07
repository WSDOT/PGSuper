///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include <PgsExt\DeckPoint.h>
#include <Units\SysUnits.h>
#include <StdIo.h>

#include <WbflAtlExt.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CDeckPoint
****************************************************************************/


CDeckPoint::CDeckPoint()
{
   Station = 0;
   LeftEdge = 0;
   RightEdge = 0;
   MeasurementType = pgsTypes::omtBridge;
   LeftTransitionType = pgsTypes::dptLinear;
   RightTransitionType = pgsTypes::dptLinear;
}

CDeckPoint::CDeckPoint(const CDeckPoint& rOther)
{
   MakeCopy(rOther);
}

CDeckPoint::~CDeckPoint()
{
}

//======================== OPERATORS  =======================================
CDeckPoint& CDeckPoint::operator= (const CDeckPoint& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CDeckPoint::operator == (const CDeckPoint& rOther) const
{
   if ( !IsEqual(Station,rOther.Station) )
   {
      return false;
   }

   if ( !IsEqual(LeftEdge,rOther.LeftEdge) )
   {
      return false;
   }
   
   if ( !IsEqual(RightEdge,rOther.RightEdge) )
   {
      return false;
   }

   if ( MeasurementType != rOther.MeasurementType )
   {
      return false;
   }

   if ( LeftTransitionType != rOther.LeftTransitionType )
   {
      return false;
   }

   if ( RightTransitionType != rOther.RightTransitionType )
   {
      return false;
   }

   return true;
}

bool CDeckPoint::operator != (const CDeckPoint& rOther) const
{
   return !operator==( rOther );
}

bool CDeckPoint::operator < (const CDeckPoint& rOther) const
{
   return Station < rOther.Station;
}

//======================== OPERATIONS =======================================
HRESULT CDeckPoint::Load(IStructuredLoad* pStrLoad,IProgress* pProgress)
{
   USES_CONVERSION;

  CHRException hr;

   try
   {
      hr = pStrLoad->BeginUnit(_T("DeckPoint"));
      Float64 version;
      hr = pStrLoad->get_Version(&version);

      CComVariant var;

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Station"), &var );
      Station = var.dblVal;

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("LeftEdge"), &var );
      LeftEdge = var.dblVal;

      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("RightEdge"), &var );
      RightEdge = var.dblVal;

      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("MeasurementType"), &var );
      MeasurementType = (pgsTypes::OffsetMeasurementType)var.lVal;

      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("LeftTransitionType"), &var );
      LeftTransitionType = (pgsTypes::DeckPointTransitionType)var.lVal;

      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("RightTransitionType"), &var );
      RightTransitionType = (pgsTypes::DeckPointTransitionType)var.lVal;

      hr = pStrLoad->EndUnit();
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   return hr;
}

Float64 CDeckPoint::GetWidth() const
{
   return RightEdge + LeftEdge;
}

HRESULT CDeckPoint::Save(IStructuredSave* pStrSave,IProgress* pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("DeckPoint"),1.0);

   pStrSave->put_Property(_T("Station"),             CComVariant(Station));
   pStrSave->put_Property(_T("LeftEdge"),            CComVariant(LeftEdge));
   pStrSave->put_Property(_T("RightEdge"),           CComVariant(RightEdge));
   pStrSave->put_Property(_T("MeasurementType"),     CComVariant(MeasurementType));
   pStrSave->put_Property(_T("LeftTransitionType"),  CComVariant(LeftTransitionType));
   pStrSave->put_Property(_T("RightTransitionType"), CComVariant(RightTransitionType));

   pStrSave->EndUnit();

   return hr;
}
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CDeckPoint::MakeCopy(const CDeckPoint& rOther)
{
   Station             = rOther.Station;
   LeftEdge            = rOther.LeftEdge;
   RightEdge           = rOther.RightEdge;
   MeasurementType     = rOther.MeasurementType;
   LeftTransitionType  = rOther.LeftTransitionType;
   RightTransitionType = rOther.RightTransitionType;
}

void CDeckPoint::MakeAssignment(const CDeckPoint& rOther)
{
   MakeCopy( rOther );
}
