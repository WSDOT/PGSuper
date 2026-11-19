///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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
#include <PsgLib\PierPointData.h>
#include <cmath>

/****************************************************************************
CLASS
   CPierPointData
****************************************************************************/

CPierPointData::CPierPointData(CPierData2* pPier) :
m_pPier(pPier)
{
   m_X = WBFL::Units::ConvertToSysUnits(0.0,WBFL::Units::Measure::Feet);
   m_Y = WBFL::Units::ConvertToSysUnits(10.0,WBFL::Units::Measure::Feet);
}

CPierPointData::CPierPointData(const CPierPointData& rOther)
{
   MakeCopy(rOther);
}

CPierPointData::~CPierPointData()
{
}

CPierPointData& CPierPointData::operator= (const CPierPointData& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool CPierPointData::operator==(const CPierPointData& rOther) const
{
   if ( !IsEqual(m_X,rOther.m_X) )
   {
      return false;
   }

   if ( !IsEqual(m_Y,rOther.m_Y) )
   {
      return false;
   }

   return m_TransitionType == rOther.m_TransitionType && IsEqual(m_Radius, rOther.m_Radius) &&
       m_IsEndPoint == rOther.m_IsEndPoint;
}

bool CPierPointData::operator!=(const CPierPointData& rOther) const
{
   return !operator==(rOther);
}

HRESULT CPierPointData::Save(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress)
{
   HRESULT hr = S_OK;

   pStrSave->BeginUnit(_T("PierPoint"),1.0);

   pStrSave->put_Property(_T("X"),CComVariant(m_X));
   pStrSave->put_Property(_T("Y"),CComVariant(m_Y));
   pStrSave->put_Property(_T("Transition"),CComVariant(m_TransitionType));
   pStrSave->put_Property(_T("Radius"),CComVariant(m_Radius));

   pStrSave->put_Property(_T("EndPoint"),CComVariant(m_IsEndPoint ? VARIANT_TRUE : VARIANT_FALSE));

   pStrSave->EndUnit(); // Pier Point

   return hr;
}

HRESULT CPierPointData::Load(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress)
{
   CHRException hr;

   try
   {
      CComVariant var;
      hr = pStrLoad->BeginUnit(_T("PierPoint"));

      var.vt = VT_R8;

      hr = pStrLoad->get_Property(_T("X"),&var);
      m_X = var.dblVal;

      hr = pStrLoad->get_Property(_T("Y"),&var);
      m_Y = var.dblVal;
      var.vt = VT_I4;
      hr = pStrLoad->get_Property(_T("Transition"), &var);
      if (var.lVal < ttStraight || var.lVal > ttArcDownward) throw E_FAIL;
      m_TransitionType = static_cast<TransitionType>(var.lVal);
      var.vt = VT_R8;
      hr = pStrLoad->get_Property(_T("Radius"), &var);
      m_Radius = var.dblVal;
      if (!std::isfinite(m_Radius) || m_Radius < 0 ||
          (m_TransitionType != ttStraight && m_Radius == 0)) throw E_FAIL;
      var.vt = VT_BOOL;
      hr = pStrLoad->get_Property(_T("EndPoint"), &var);
      m_IsEndPoint = var.boolVal != VARIANT_FALSE;

      hr = pStrLoad->EndUnit(); // Pier Point
   }
   catch (HRESULT)
   {
      ATLASSERT(false);
      THROW_LOAD(InvalidFileFormat,pStrLoad);
   }

   PGS_ASSERT_VALID;

   return S_OK;
}

void CPierPointData::SetPier(CPierData2* pPier)
{
   m_pPier = pPier;
}

const CPierData2* CPierPointData::GetPier() const
{
   return m_pPier;
}

CPierData2* CPierPointData::GetPier()
{
   return m_pPier;
}

void CPierPointData::MakeCopy(const CPierPointData& rOther)
{
   m_pPier = rOther.m_pPier;
   m_X = rOther.m_X;
   m_Y = rOther.m_Y;
   m_TransitionType = rOther.m_TransitionType;
   m_Radius = rOther.m_Radius;
   m_IsEndPoint = rOther.m_IsEndPoint;

   PGS_ASSERT_VALID;
}

void CPierPointData::MakeAssignment(const CPierPointData& rOther)
{
   MakeCopy( rOther );
}

void CPierPointData::Set_X(Float64 ppX)
{
   m_X = ppX;
}

Float64 CPierPointData::Get_X() const
{
   return m_X;
}

void CPierPointData::Set_Y(Float64 ppY)
{
   m_Y = ppY;
}

Float64 CPierPointData::Get_Y() const
{
   return m_Y;
}

void CPierPointData::SetTransition(TransitionType type, Float64 radius)
{
   m_TransitionType = type;
   m_Radius = type == ttStraight ? 0 : radius;
}

CPierPointData::TransitionType CPierPointData::GetTransitionType() const
{
   return m_TransitionType;
}

Float64 CPierPointData::GetRadius() const
{
   return m_Radius;
}

void CPierPointData::UpdateEndPoint(std::vector<CPierPointData>& points, Float64 length, Float64 depth)
{
   if (points.empty() || !points.back().IsEndPoint())
   {
      CPierPointData endPoint;
      endPoint.SetEndPoint(true);
      points.push_back(endPoint);
   }
   points.back().Set_X(length);
   points.back().Set_Y(depth);
}

#if defined _DEBUG
void CPierPointData::AssertValid() const
{
}
#endif
