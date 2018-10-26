///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
#include <psgLib\RatingLibraryEntry.h>
#include "resource.h"
#include "RatingDialog.h"

#include <Units\Units.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CURRENT_VERSION 1.0

CLiveLoadFactorModel::CLiveLoadFactorModel()
{
   m_Wlower = ::ConvertToSysUnits(100.0,unitMeasure::Kip);
   m_Wupper = ::ConvertToSysUnits(150.0,unitMeasure::Kip);

   m_LiveLoadFactorType     = pgsTypes::gllBilinear;
   m_LiveLoadFactorModifier = pgsTypes::gllmInterpolate;

   m_ADTT[0] = 100;
   m_gLL_Lower[0] = 1.4;
   m_gLL_Upper[0] = 1.4;
   m_gLL_Service[0] = 1.0;

   m_ADTT[1] = 1000;
   m_gLL_Lower[1] = 1.65;
   m_gLL_Upper[1] = 1.65;
   m_gLL_Service[1] = 1.0;

   m_ADTT[2] = 5000;
   m_gLL_Lower[2] = 1.80;
   m_gLL_Upper[2] = 1.80;
   m_gLL_Service[2] = 1.0;

   m_ADTT[3] = -1; // unknown
   m_gLL_Lower[3] = 1.80;
   m_gLL_Upper[3] = 1.80;
   m_gLL_Service[3] = 1.0;

   m_bAllowUserOverride = false;
}

bool CLiveLoadFactorModel::operator!=(const CLiveLoadFactorModel& other) const
{
   return !operator==(other);
}

bool CLiveLoadFactorModel::operator==(const CLiveLoadFactorModel& other) const
{
   if ( !IsEqual(m_Wlower,other.m_Wlower) )
      return false;

   if ( !IsEqual(m_Wupper,other.m_Wupper) )
      return false;

   if ( m_LiveLoadFactorType != other.m_LiveLoadFactorType )
      return false;

   if ( m_LiveLoadFactorModifier != other.m_LiveLoadFactorModifier )
      return false;

   if ( m_bAllowUserOverride != other.m_bAllowUserOverride )
      return false;

   if ( m_LiveLoadFactorType == pgsTypes::gllSingleValue )
   {
      if ( m_ADTT[0] != other.m_ADTT[0] )
         return false;

      if ( !IsEqual(m_gLL_Lower[0],other.m_gLL_Lower[0]) )
         return false;

      if ( !IsEqual(m_gLL_Service[0],other.m_gLL_Service[0]) )
         return false;
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllStepped || m_LiveLoadFactorType == pgsTypes::gllLinear )
   {
      if ( m_ADTT[0] != other.m_ADTT[0] )
         return false;

      if ( m_ADTT[1] != other.m_ADTT[1] )
         return false;

      if ( m_ADTT[3] != other.m_ADTT[3] )
         return false;

      if ( !IsEqual(m_gLL_Lower[0],other.m_gLL_Lower[0]) )
         return false;

      if ( !IsEqual(m_gLL_Lower[1],other.m_gLL_Lower[1]) )
         return false;

      if ( !IsEqual(m_gLL_Lower[3],other.m_gLL_Lower[3]) )
         return false;


      if ( !IsEqual(m_gLL_Service[0],other.m_gLL_Service[0]) )
         return false;

      if ( !IsEqual(m_gLL_Service[1],other.m_gLL_Service[1]) )
         return false;

      if ( !IsEqual(m_gLL_Service[3],other.m_gLL_Service[3]) )
         return false;
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllBilinear )
   {
      if ( m_ADTT[0] != other.m_ADTT[0] )
         return false;

      if ( m_ADTT[1] != other.m_ADTT[1] )
         return false;

      if ( m_ADTT[2] != other.m_ADTT[2] )
         return false;

      if ( m_ADTT[3] != other.m_ADTT[3] )
         return false;

      if ( !IsEqual(m_gLL_Lower[0],other.m_gLL_Lower[0]) )
         return false;

      if ( !IsEqual(m_gLL_Lower[1],other.m_gLL_Lower[1]) )
         return false;

      if ( !IsEqual(m_gLL_Lower[2],other.m_gLL_Lower[2]) )
         return false;

      if ( !IsEqual(m_gLL_Lower[3],other.m_gLL_Lower[3]) )
         return false;


      if ( !IsEqual(m_gLL_Service[0],other.m_gLL_Service[0]) )
         return false;

      if ( !IsEqual(m_gLL_Service[1],other.m_gLL_Service[1]) )
         return false;

      if ( !IsEqual(m_gLL_Service[2],other.m_gLL_Service[2]) )
         return false;

      if ( !IsEqual(m_gLL_Service[3],other.m_gLL_Service[3]) )
         return false;
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllBilinearWithWeight )
   {
      if ( m_ADTT[0] != other.m_ADTT[0] )
         return false;

      if ( m_ADTT[1] != other.m_ADTT[1] )
         return false;

      if ( m_ADTT[2] != other.m_ADTT[2] )
         return false;

      if ( m_ADTT[3] != other.m_ADTT[3] )
         return false;

      if ( !IsEqual(m_gLL_Lower[0],other.m_gLL_Lower[0]) )
         return false;

      if ( !IsEqual(m_gLL_Lower[1],other.m_gLL_Lower[1]) )
         return false;

      if ( !IsEqual(m_gLL_Lower[2],other.m_gLL_Lower[2]) )
         return false;

      if ( !IsEqual(m_gLL_Lower[3],other.m_gLL_Lower[3]) )
         return false;

      if ( !IsEqual(m_gLL_Upper[0],other.m_gLL_Upper[0]) )
         return false;

      if ( !IsEqual(m_gLL_Upper[1],other.m_gLL_Upper[1]) )
         return false;

      if ( !IsEqual(m_gLL_Upper[2],other.m_gLL_Upper[2]) )
         return false;

      if ( !IsEqual(m_gLL_Upper[3],other.m_gLL_Upper[3]) )
         return false;



      if ( !IsEqual(m_gLL_Service[0],other.m_gLL_Service[0]) )
         return false;

      if ( !IsEqual(m_gLL_Service[1],other.m_gLL_Service[1]) )
         return false;

      if ( !IsEqual(m_gLL_Service[2],other.m_gLL_Service[2]) )
         return false;

      if ( !IsEqual(m_gLL_Service[3],other.m_gLL_Service[3]) )
         return false;
   }

   return true;
}

void CLiveLoadFactorModel::SetVehicleWeight(Float64 Wlower,Float64 Wupper)
{
   m_Wlower = Wlower;
   m_Wupper = Wupper;
}

void CLiveLoadFactorModel::GetVehicleWeight(Float64* pWlower,Float64* pWupper) const
{
   *pWlower = m_Wlower;
   *pWupper = m_Wupper;
}

void CLiveLoadFactorModel::SetLiveLoadFactorType(pgsTypes::LiveLoadFactorType gllType)
{
   m_LiveLoadFactorType = gllType;
}

pgsTypes::LiveLoadFactorType CLiveLoadFactorModel::GetLiveLoadFactorType() const
{
   return m_LiveLoadFactorType;
}

void CLiveLoadFactorModel::SetLiveLoadFactorModifier(pgsTypes::LiveLoadFactorModifier gllModifier)
{
   m_LiveLoadFactorModifier = gllModifier;
}

pgsTypes::LiveLoadFactorModifier CLiveLoadFactorModel::GetLiveLoadFactorModifier() const
{
   return m_LiveLoadFactorModifier;
}

void CLiveLoadFactorModel::AllowUserOverride(bool bAllow)
{
   m_bAllowUserOverride = bAllow;
}

bool CLiveLoadFactorModel::AllowUserOverride() const
{
   return m_bAllowUserOverride;
}

void CLiveLoadFactorModel::SetADTT(Int16 adtt1, Int16 adtt2, Int16 adtt3, Int16 adtt4)
{
   m_ADTT[0] = adtt1;
   m_ADTT[1] = adtt2;
   m_ADTT[2] = adtt3;
   m_ADTT[3] = adtt4;
}

void CLiveLoadFactorModel::GetADTT(Int16* adtt1, Int16* adtt2, Int16* adtt3, Int16* adtt4) const
{
   *adtt1 = m_ADTT[0];
   *adtt2 = m_ADTT[1];
   *adtt3 = m_ADTT[2];
   *adtt4 = m_ADTT[3];
}

void CLiveLoadFactorModel::SetLowerLiveLoadFactor(Float64 gll1,Float64 gll2,Float64 gll3,Float64 gll4)
{
   m_gLL_Lower[0] = gll1;
   m_gLL_Lower[1] = gll2;
   m_gLL_Lower[2] = gll3;
   m_gLL_Lower[3] = gll4;
}

void CLiveLoadFactorModel::GetLowerLiveLoadFactor(Float64* gll1,Float64* gll2,Float64* gll3,Float64* gll4) const
{
   *gll1 = m_gLL_Lower[0];
   *gll2 = m_gLL_Lower[1];
   *gll3 = m_gLL_Lower[2];
   *gll4 = m_gLL_Lower[3];
}

void CLiveLoadFactorModel::SetUpperLiveLoadFactor(Float64 gll1,Float64 gll2,Float64 gll3,Float64 gll4)
{
   m_gLL_Upper[0] = gll1;
   m_gLL_Upper[1] = gll2;
   m_gLL_Upper[2] = gll3;
   m_gLL_Upper[3] = gll4;
}

void CLiveLoadFactorModel::GetUpperLiveLoadFactor(Float64* gll1,Float64* gll2,Float64* gll3,Float64* gll4) const
{
   *gll1 = m_gLL_Upper[0];
   *gll2 = m_gLL_Upper[1];
   *gll3 = m_gLL_Upper[2];
   *gll4 = m_gLL_Upper[3];
}

void CLiveLoadFactorModel::SetServiceLiveLoadFactor(Float64 gll1,Float64 gll2,Float64 gll3,Float64 gll4)
{
   m_gLL_Service[0] = gll1;
   m_gLL_Service[1] = gll2;
   m_gLL_Service[2] = gll3;
   m_gLL_Service[3] = gll4;
}

void CLiveLoadFactorModel::GetServiceLiveLoadFactor(Float64* gll1,Float64* gll2,Float64* gll3,Float64* gll4) const
{
   *gll1 = m_gLL_Service[0];
   *gll2 = m_gLL_Service[1];
   *gll3 = m_gLL_Service[2];
   *gll4 = m_gLL_Service[3];
}

Float64 CLiveLoadFactorModel::GetStrengthLiveLoadFactor(Int16 adtt,Float64 W) const
{
   if ( m_LiveLoadFactorType == pgsTypes::gllSingleValue )
      return m_gLL_Lower[0];

   if ( m_LiveLoadFactorType == pgsTypes::gllStepped )
   {
      if ( adtt < 0 ) // unknown
      {
         return m_gLL_Lower[3];
      }

      if ( adtt < m_ADTT[0] )
         return m_gLL_Lower[0];
      else
         return m_gLL_Lower[1];
   }

   if ( m_LiveLoadFactorType == pgsTypes::gllLinear )
   {
      if ( adtt < 0 ) // unknown
      {
         return m_gLL_Lower[3];
      }

      if ( adtt < m_ADTT[0] )
         return m_gLL_Lower[0];

      if ( m_ADTT[1] < adtt )
         return m_gLL_Lower[1];

      // adtt is between values
      if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
         return m_gLL_Lower[1];
      else
         return ::LinInterp(adtt-m_ADTT[0],m_gLL_Lower[0],m_gLL_Lower[1],m_ADTT[1] - m_ADTT[0]);
   }

   if ( m_LiveLoadFactorType == pgsTypes::gllBilinear )
   {
      if ( adtt < 0 ) // unknown
      {
         return m_gLL_Lower[3];
      }

      if ( adtt < m_ADTT[0] )
         return m_gLL_Lower[0];

      if ( m_ADTT[2] < adtt )
         return m_gLL_Lower[2];

      if ( adtt < m_ADTT[1] )
      {
         // between ADTT[0] and ADTT[1]
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
            return m_gLL_Lower[1];
         else
            return ::LinInterp(adtt-m_ADTT[0],m_gLL_Lower[0],m_gLL_Lower[1],m_ADTT[1] - m_ADTT[0]);
      }

      if ( adtt < m_ADTT[2] )
      {
         // between ADTT[1] and ADTT[2]
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
            return m_gLL_Lower[2];
         else
            return ::LinInterp(adtt-m_ADTT[1],m_gLL_Lower[1],m_gLL_Lower[2],m_ADTT[2] - m_ADTT[1]);
      }
   }


   if ( m_LiveLoadFactorType == pgsTypes::gllBilinearWithWeight )
   {
      if ( adtt < 0 ) // unknown
      {
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
         {
            return m_gLL_Upper[3];
         }
         else
         {
            if ( W < m_Wlower )
               return m_gLL_Lower[3];
            else if ( m_Wupper <= W )
               return m_gLL_Upper[3];
            else
               return ::LinInterp(W-m_Wlower,m_gLL_Lower[4],m_gLL_Upper[4],m_Wupper-m_Wlower);
         }
      }

      if ( adtt < m_ADTT[0] )
      {
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
         {
            return m_gLL_Upper[0];
         }
         else
         {
            if ( W < m_Wlower )
               return m_gLL_Lower[0];
            else if ( m_Wupper <= W )
               return m_gLL_Upper[0];
            else
               return ::LinInterp(W-m_Wlower,m_gLL_Lower[0],m_gLL_Upper[0],m_Wupper-m_Wlower);
         }
      }

      if ( m_ADTT[2] < adtt )
      {
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
         {
            return m_gLL_Upper[2];
         }
         else
         {
            if ( W < m_Wlower )
               return m_gLL_Lower[2];
            else if ( m_Wupper <= W )
               return m_gLL_Upper[2];
            else
               return ::LinInterp(W-m_Wlower,m_gLL_Lower[2],m_gLL_Upper[2],m_Wupper-m_Wlower);
         }
      }

      if ( adtt < m_ADTT[1] )
      {
         // between ADTT[0] and ADTT[1]
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
         {
            return m_gLL_Upper[1];
         }
         else
         {
            // interpolate on ADTT
            Float64 g_Lower = ::LinInterp(adtt-m_ADTT[0],m_gLL_Lower[0],m_gLL_Lower[1],m_ADTT[1] - m_ADTT[0]);
            Float64 g_Upper = ::LinInterp(adtt-m_ADTT[0],m_gLL_Upper[0],m_gLL_Upper[1],m_ADTT[1] - m_ADTT[0]);

            // interpolate on weight
            if ( W < m_Wlower )
               return g_Lower;
            else if ( m_Wupper <= W )
               return g_Upper;
            else
               return ::LinInterp(W-m_Wlower,g_Lower,g_Upper,m_Wupper-m_Wlower);
         }
      }

      if ( adtt < m_ADTT[2] )
      {
         // between ADTT[1] and ADTT[2]
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
         {
            return m_gLL_Upper[2];
         }
         else
         {
            // interpolate on ADTT
            Float64 g_Lower = ::LinInterp(adtt-m_ADTT[1],m_gLL_Lower[1],m_gLL_Lower[2],m_ADTT[2] - m_ADTT[1]);
            Float64 g_Upper = ::LinInterp(adtt-m_ADTT[1],m_gLL_Upper[1],m_gLL_Upper[2],m_ADTT[2] - m_ADTT[1]);

            // interpolate on weight
            if ( W < m_Wlower )
               return g_Lower;
            else if ( m_Wupper <= W )
               return g_Upper;
            else
               return ::LinInterp(W-m_Wlower,g_Lower,g_Upper,m_Wupper-m_Wlower);
         }
      }
   }

   ASSERT(false); // should not get here

   return -9999; // something obviously bogus until this gets implemented
}


Float64 CLiveLoadFactorModel::GetServiceLiveLoadFactor(Int16 adtt) const
{
   if ( m_LiveLoadFactorType == pgsTypes::gllSingleValue )
      return m_gLL_Service[0];

   if ( m_LiveLoadFactorType == pgsTypes::gllStepped )
   {
      if ( adtt < 0 ) // unknown
      {
         return m_gLL_Service[3];
      }

      if ( adtt < m_ADTT[0] )
         return m_gLL_Service[0];
      else
         return m_gLL_Service[1];
   }

   if ( m_LiveLoadFactorType == pgsTypes::gllLinear )
   {
      if ( adtt < 0 ) // unknown
      {
         return m_gLL_Service[3];
      }

      if ( adtt < m_ADTT[0] )
         return m_gLL_Service[0];

      if ( m_ADTT[1] < adtt )
         return m_gLL_Service[1];

      // adtt is between values
      if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
         return m_gLL_Service[1];
      else
         return ::LinInterp(adtt-m_ADTT[0],m_gLL_Service[0],m_gLL_Service[1],m_ADTT[1] - m_ADTT[0]);
   }

   if ( m_LiveLoadFactorType == pgsTypes::gllBilinear || m_LiveLoadFactorType == pgsTypes::gllBilinearWithWeight)
   {
      // truck weight does not effect service load factors
      if ( adtt < 0 ) // unknown
      {
         return m_gLL_Service[3];
      }

      if ( adtt < m_ADTT[0] )
         return m_gLL_Service[0];

      if ( m_ADTT[2] < adtt )
         return m_gLL_Service[2];

      if ( adtt < m_ADTT[1] )
      {
         // between ADTT[0] and ADTT[1]
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
            return m_gLL_Service[1];
         else
            return ::LinInterp(adtt-m_ADTT[0],m_gLL_Service[0],m_gLL_Service[1],m_ADTT[1] - m_ADTT[0]);
      }

      if ( adtt < m_ADTT[2] )
      {
         // between ADTT[1] and ADTT[2]
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
            return m_gLL_Service[2];
         else
            return ::LinInterp(adtt-m_ADTT[1],m_gLL_Service[1],m_gLL_Service[2],m_ADTT[2] - m_ADTT[1]);
      }
   }


   ASSERT(false); // should not get here

   return -9999; // something obviously bogus until this gets implemented
}

bool CLiveLoadFactorModel::SaveMe(sysIStructuredSave* pSave)
{
   pSave->BeginUnit(_T("LoadFactors"),1.0);
   pSave->Property(_T("LiveLoadFactorType"),(long)m_LiveLoadFactorType);
   if ( m_LiveLoadFactorType == pgsTypes::gllSingleValue )
   {
      pSave->Property(_T("LiveLoadFactor"),m_gLL_Lower[0]);
      pSave->Property(_T("LiveLoadFactor_Service"),m_gLL_Service[0]);
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllStepped || m_LiveLoadFactorType == pgsTypes::gllLinear )
   {
      pSave->Property(_T("ADTT1"),m_ADTT[0]);
      pSave->Property(_T("LiveLoadFactor1"),m_gLL_Lower[0]);
      pSave->Property(_T("LiveLoadFactor2"),m_gLL_Lower[1]);
      pSave->Property(_T("LiveLoadFactorUnknownADTT"),m_gLL_Lower[3]);

      pSave->Property(_T("LiveLoadFactor1_Service"),m_gLL_Service[0]);
      pSave->Property(_T("LiveLoadFactor2_Service"),m_gLL_Service[1]);
      pSave->Property(_T("LiveLoadFactorUnknownADTT_Service"),m_gLL_Service[3]);

      pSave->Property(_T("LiveLoadFactorModifier"),(long)m_LiveLoadFactorModifier);
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllBilinear )
   {
      pSave->Property(_T("ADTT1"),m_ADTT[0]);
      pSave->Property(_T("LiveLoadFactor1"),m_gLL_Lower[0]);
      pSave->Property(_T("ADTT2"),m_ADTT[1]);
      pSave->Property(_T("LiveLoadFactor2"),m_gLL_Lower[1]);
      pSave->Property(_T("ADTT3"),m_ADTT[2]);
      pSave->Property(_T("LiveLoadFactor3"),m_gLL_Lower[2]);
      pSave->Property(_T("LiveLoadFactorUnknownADTT"),m_gLL_Lower[3]);

      pSave->Property(_T("LiveLoadFactor1_Service"),m_gLL_Service[0]);
      pSave->Property(_T("LiveLoadFactor2_Service"),m_gLL_Service[1]);
      pSave->Property(_T("LiveLoadFactor3_Service"),m_gLL_Service[2]);
      pSave->Property(_T("LiveLoadFactorUnknownADTT_Service"),m_gLL_Service[3]);

      pSave->Property(_T("LiveLoadFactorModifier"),(long)m_LiveLoadFactorModifier);
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllBilinearWithWeight )
   {
      pSave->Property(_T("VehicleWeight1"),m_Wlower);
      pSave->Property(_T("VehicleWeight2"),m_Wupper);

      pSave->Property(_T("ADTT1"),m_ADTT[0]);
      pSave->Property(_T("LiveLoadFactor1_Lower"),m_gLL_Lower[0]);
      pSave->Property(_T("LiveLoadFactor1_Upper"),m_gLL_Upper[0]);
      pSave->Property(_T("ADTT2"),m_ADTT[1]);
      pSave->Property(_T("LiveLoadFactor2_Lower"),m_gLL_Lower[1]);
      pSave->Property(_T("LiveLoadFactor2_Upper"),m_gLL_Upper[1]);
      pSave->Property(_T("ADTT3"),m_ADTT[2]);
      pSave->Property(_T("LiveLoadFactor3_Lower"),m_gLL_Lower[2]);
      pSave->Property(_T("LiveLoadFactor3_Upper"),m_gLL_Upper[2]);
      pSave->Property(_T("LiveLoadFactorUnknownADTT_Lower"),m_gLL_Lower[3]);
      pSave->Property(_T("LiveLoadFactorUnknownADTT_Upper"),m_gLL_Upper[3]);

      pSave->Property(_T("LiveLoadFactor1_Service"),m_gLL_Service[0]);
      pSave->Property(_T("LiveLoadFactor2_Service"),m_gLL_Service[1]);
      pSave->Property(_T("LiveLoadFactor3_Service"),m_gLL_Service[2]);
      pSave->Property(_T("LiveLoadFactorUnknownADTT_Service"),m_gLL_Service[3]);

      pSave->Property(_T("LiveLoadFactorModifier"),(long)m_LiveLoadFactorModifier);
   }
   else
   {
      ATLASSERT(false); // should never get here
   }

   pSave->Property(_T("AllowUserOverride"),m_bAllowUserOverride);

   pSave->EndUnit();

   return true;
}

bool CLiveLoadFactorModel::LoadMe(sysIStructuredLoad* pLoad)
{
   if ( !pLoad->BeginUnit(_T("LoadFactors")) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   int value;
   if ( !pLoad->Property(_T("LiveLoadFactorType"),&value) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   m_LiveLoadFactorType = (pgsTypes::LiveLoadFactorType)value;

   if ( m_LiveLoadFactorType == pgsTypes::gllSingleValue )
   {
      if ( !pLoad->Property(_T("LiveLoadFactor"),&m_gLL_Lower[0]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor_Service"),&m_gLL_Service[0]) )
         THROW_LOAD(InvalidFileFormat,pLoad);
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllStepped || m_LiveLoadFactorType == pgsTypes::gllLinear )
   {
      if ( !pLoad->Property(_T("ADTT1"),&m_ADTT[0]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor1"),&m_gLL_Lower[0]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor2"),&m_gLL_Lower[1]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT"),&m_gLL_Lower[3]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor1_Service"),&m_gLL_Service[0]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor2_Service"),&m_gLL_Service[1]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT_Service"),&m_gLL_Service[3]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactorModifier"),&value) )
         THROW_LOAD(InvalidFileFormat,pLoad);
      
      m_LiveLoadFactorModifier = (pgsTypes::LiveLoadFactorModifier)value;
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllBilinear )
   {
      if ( !pLoad->Property(_T("ADTT1"),&m_ADTT[0]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor1"),&m_gLL_Lower[0]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("ADTT2"),&m_ADTT[1]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor2"),&m_gLL_Lower[1]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("ADTT3"),&m_ADTT[2]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor3"),&m_gLL_Lower[2]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT"),&m_gLL_Lower[3]) )
         THROW_LOAD(InvalidFileFormat,pLoad);


      if ( !pLoad->Property(_T("LiveLoadFactor1_Service"),&m_gLL_Service[0]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor2_Service"),&m_gLL_Service[1]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor3_Service"),&m_gLL_Service[2]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT_Service"),&m_gLL_Service[3]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactorModifier"),&value) )
         THROW_LOAD(InvalidFileFormat,pLoad);
      
      m_LiveLoadFactorModifier = (pgsTypes::LiveLoadFactorModifier)value;
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllBilinearWithWeight )
   {
      if ( !pLoad->Property(_T("VehicleWeight1"),&m_Wlower) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("VehicleWeight2"),&m_Wupper) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("ADTT1"),&m_ADTT[0]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor1_Lower"),&m_gLL_Lower[0]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor1_Upper"),&m_gLL_Upper[0]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("ADTT2"),&m_ADTT[1]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor2_Lower"),&m_gLL_Lower[1]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor2_Upper"),&m_gLL_Upper[1]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("ADTT3"),&m_ADTT[2]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor3_Lower"),&m_gLL_Lower[2]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor3_Upper"),&m_gLL_Upper[2]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT_Lower"),&m_gLL_Lower[3]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT_Upper"),&m_gLL_Upper[3]) )
         THROW_LOAD(InvalidFileFormat,pLoad);


      if ( !pLoad->Property(_T("LiveLoadFactor1_Service"),&m_gLL_Service[0]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor2_Service"),&m_gLL_Service[1]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactor3_Service"),&m_gLL_Service[2]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT_Service"),&m_gLL_Service[3]) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      if ( !pLoad->Property(_T("LiveLoadFactorModifier"),&value) )
         THROW_LOAD(InvalidFileFormat,pLoad);
      
      m_LiveLoadFactorModifier = (pgsTypes::LiveLoadFactorModifier)value;
   }
   else
   {
      ATLASSERT(false); // should never get here
   }

   if ( !pLoad->Property(_T("AllowUserOverride"),&m_bAllowUserOverride) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   return true;
}

/****************************************************************************
CLASS
   RatingLibraryEntry
****************************************************************************/

RatingLibraryEntry::RatingLibraryEntry() :
m_SpecificationVersion(lrfrVersionMgr::FirstEditionWith2010Interims),
m_bAlwaysRate(false)
{
   SetDescription(_T("Default Rating Specification based on AASHTO MBE 1st Edition, 2008 with 2010 interim provisions"));

   m_LiveLoadFactorModels[pgsTypes::lrDesign_Inventory].SetLiveLoadFactorType(pgsTypes::gllSingleValue);
   m_LiveLoadFactorModels[pgsTypes::lrDesign_Inventory].SetLowerLiveLoadFactor(1.75,-1,-1,-1);
   m_LiveLoadFactorModels[pgsTypes::lrDesign_Inventory].SetServiceLiveLoadFactor(0.8,-1,-1,-1);

   m_LiveLoadFactorModels[pgsTypes::lrDesign_Operating].SetLiveLoadFactorType(pgsTypes::gllSingleValue);
   m_LiveLoadFactorModels[pgsTypes::lrDesign_Operating].SetLowerLiveLoadFactor(1.35,-1,-1,-1);
   m_LiveLoadFactorModels[pgsTypes::lrDesign_Operating].SetServiceLiveLoadFactor(0.8,-1,-1,-1);

   m_LiveLoadFactorModels[pgsTypes::lrLegal_Routine].SetLiveLoadFactorType(pgsTypes::gllBilinear);
   m_LiveLoadFactorModels[pgsTypes::lrLegal_Routine].SetADTT(100,1000,5000,-1);
   m_LiveLoadFactorModels[pgsTypes::lrLegal_Routine].SetLowerLiveLoadFactor(1.40,1.65,1.80,1.80);

   m_LiveLoadFactorModels[pgsTypes::lrLegal_Special].SetLiveLoadFactorType(pgsTypes::gllBilinear);
   m_LiveLoadFactorModels[pgsTypes::lrLegal_Special].SetADTT(100,1000,5000,-1);
   m_LiveLoadFactorModels[pgsTypes::lrLegal_Special].SetLowerLiveLoadFactor(1.15,1.40,1.60,1.60);

   m_LiveLoadFactorModels[pgsTypes::lrPermit_Routine].SetLiveLoadFactorType(pgsTypes::gllBilinearWithWeight);
   m_LiveLoadFactorModels[pgsTypes::lrPermit_Routine].SetADTT(100,1000,5000,-1);
   m_LiveLoadFactorModels[pgsTypes::lrPermit_Routine].SetVehicleWeight(::ConvertToSysUnits(100,unitMeasure::Kip),::ConvertToSysUnits(150,unitMeasure::Kip));
   m_LiveLoadFactorModels[pgsTypes::lrPermit_Routine].SetLowerLiveLoadFactor(1.40,1.60,1.80,1.80);
   m_LiveLoadFactorModels[pgsTypes::lrPermit_Routine].SetUpperLiveLoadFactor(1.10,1.20,1.30,1.30);

   m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptSingleTripWithEscort].SetLiveLoadFactorType(pgsTypes::gllSingleValue);
   m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptSingleTripWithEscort].SetLowerLiveLoadFactor(1.15,-1,-1,-1);

   m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptSingleTripWithTraffic].SetLiveLoadFactorType(pgsTypes::gllBilinear);
   m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptSingleTripWithTraffic].SetADTT(100,1000,5000,-1);
   m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptSingleTripWithTraffic].SetLowerLiveLoadFactor(1.35,1.40,1.50,1.50);

   m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptMultipleTripWithTraffic].SetLiveLoadFactorType(pgsTypes::gllBilinear);
   m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptMultipleTripWithTraffic].SetADTT(100,1000,5000,-1);
   m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptMultipleTripWithTraffic].SetLowerLiveLoadFactor(1.55,1.75,1.85,1.85);
}

RatingLibraryEntry::RatingLibraryEntry(const RatingLibraryEntry& rOther) :
libLibraryEntry(rOther)
{
   MakeCopy(rOther);
}

RatingLibraryEntry::~RatingLibraryEntry()
{
}

RatingLibraryEntry& RatingLibraryEntry::operator= (const RatingLibraryEntry& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool RatingLibraryEntry::Edit(bool allowEditing)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // exchange data with dialog
   // make a temporary copy of this and have the dialog work on it.
   RatingLibraryEntry tmp(*this);

   CRatingDialog dlg(tmp, allowEditing);
   INT_PTR i = dlg.DoModal();
   if (i==IDOK)
   {
      *this = tmp;
      return true;
   }

   return false;
}

HICON  RatingLibraryEntry::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_RATING_ENTRY) );
}

bool RatingLibraryEntry::SaveMe(sysIStructuredSave* pSave)
{
   pSave->BeginUnit(_T("RatingLibraryEntry"), CURRENT_VERSION);

   pSave->Property(_T("Name"), GetName().c_str());
   pSave->Property(_T("Description"), GetDescription().c_str());

   switch (m_SpecificationVersion)
   {
   case lrfrVersionMgr::FirstEdition2008:
      pSave->Property(_T("SpecificationVersion"), _T("LRFR2008"));
      break;

   case lrfrVersionMgr::FirstEditionWith2010Interims:
      pSave->Property(_T("SpecificationVersion"), _T("LRFR2010"));
      break;

   default:
      ASSERT(0);
      pSave->Property(_T("SpecificationVersion"), _T("LRFR2008"));
   }

   pSave->Property(_T("AlwaysRate"),m_bAlwaysRate);

   pSave->BeginUnit(_T("LiveLoadFactors_Design_Inventory"),1.0);
   m_LiveLoadFactorModels[pgsTypes::lrDesign_Inventory].SaveMe(pSave);
   pSave->EndUnit();

   pSave->BeginUnit(_T("LiveLoadFactors_Design_Operating"),1.0);
   m_LiveLoadFactorModels[pgsTypes::lrDesign_Operating].SaveMe(pSave);
   pSave->EndUnit();

   pSave->BeginUnit(_T("LiveLoadFactors_Legal_Routine"),1.0);
   m_LiveLoadFactorModels[pgsTypes::lrLegal_Routine].SaveMe(pSave);
   pSave->EndUnit();

   pSave->BeginUnit(_T("LiveLoadFactors_Legal_Special"),1.0);
   m_LiveLoadFactorModels[pgsTypes::lrLegal_Special].SaveMe(pSave);
   pSave->EndUnit();

   pSave->BeginUnit(_T("LiveLoadFactors_Permit_Routine"),1.0);
   m_LiveLoadFactorModels[pgsTypes::lrPermit_Routine].SaveMe(pSave);
   pSave->EndUnit();

   pSave->BeginUnit(_T("LiveLoadFactors_Permit_SingleTripWithEscort"),1.0);
   m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptSingleTripWithEscort].SaveMe(pSave);
   pSave->EndUnit();

   pSave->BeginUnit(_T("LiveLoadFactors_Permit_SingleTripWithTraffic"),1.0);
   m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptSingleTripWithTraffic].SaveMe(pSave);
   pSave->EndUnit();

   pSave->BeginUnit(_T("LiveLoadFactors_Permit_MultipleTripWithTraffic"),1.0);
   m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptMultipleTripWithTraffic].SaveMe(pSave);
   pSave->EndUnit();

   pSave->EndUnit(); // RatingLibraryEntry

   return true;
}

bool RatingLibraryEntry::LoadMe(sysIStructuredLoad* pLoad)
{
   if ( !pLoad->BeginUnit(_T("RatingLibraryEntry")) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   Float64 version = pLoad->GetVersion();
   if (version < 1.0 || CURRENT_VERSION < version)
      THROW_LOAD(BadVersion,pLoad);

   std::_tstring name;
   if(!pLoad->Property(_T("Name"),&name))
      THROW_LOAD(InvalidFileFormat,pLoad);

   SetName(name.c_str());

   if(!pLoad->Property(_T("Description"),&name))
      THROW_LOAD(InvalidFileFormat,pLoad);

   SetDescription(name.c_str());

   std::_tstring strSpecVersion;
   if( !pLoad->Property(_T("SpecificationVersion"),&strSpecVersion) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if(strSpecVersion == _T("LRFR2008"))
      m_SpecificationVersion = lrfrVersionMgr::FirstEdition2008;
   else if(strSpecVersion == _T("LRFR2010"))
      m_SpecificationVersion = lrfrVersionMgr::FirstEditionWith2010Interims;
   else
      THROW_LOAD(InvalidFileFormat,pLoad);


   if ( !pLoad->Property(_T("AlwaysRate"),&m_bAlwaysRate) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !pLoad->BeginUnit(_T("LiveLoadFactors_Design_Inventory")) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !m_LiveLoadFactorModels[pgsTypes::lrDesign_Inventory].LoadMe(pLoad) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);


   if ( !pLoad->BeginUnit(_T("LiveLoadFactors_Design_Operating")) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !m_LiveLoadFactorModels[pgsTypes::lrDesign_Operating].LoadMe(pLoad) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !pLoad->BeginUnit(_T("LiveLoadFactors_Legal_Routine")) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !m_LiveLoadFactorModels[pgsTypes::lrLegal_Routine].LoadMe(pLoad) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !pLoad->BeginUnit(_T("LiveLoadFactors_Legal_Special")) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !m_LiveLoadFactorModels[pgsTypes::lrLegal_Special].LoadMe(pLoad) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !pLoad->BeginUnit(_T("LiveLoadFactors_Permit_Routine")) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !m_LiveLoadFactorModels[pgsTypes::lrPermit_Routine].LoadMe(pLoad) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !pLoad->BeginUnit(_T("LiveLoadFactors_Permit_SingleTripWithEscort")) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptSingleTripWithEscort].LoadMe(pLoad) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !pLoad->BeginUnit(_T("LiveLoadFactors_Permit_SingleTripWithTraffic")) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptSingleTripWithTraffic].LoadMe(pLoad) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !pLoad->BeginUnit(_T("LiveLoadFactors_Permit_MultipleTripWithTraffic")) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptMultipleTripWithTraffic].LoadMe(pLoad) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   if ( !pLoad->EndUnit() )  // RatingLibraryEntry
      THROW_LOAD(InvalidFileFormat,pLoad);

   return true;

}

#define TEST(a,b) if ( a != b ) return false
#define TESTD(a,b) if ( !::IsEqual(a,b) ) return false

bool RatingLibraryEntry::IsEqual(const RatingLibraryEntry& rOther, bool considerName) const
{
   TEST (m_Description                , rOther.m_Description                );
   TEST (m_SpecificationVersion       , rOther.m_SpecificationVersion       );
   TEST (m_bAlwaysRate                , rOther.m_bAlwaysRate                );

   for ( int i = 0; i < 5; i++ )
   {
      if ( m_LiveLoadFactorModels[i] != rOther.m_LiveLoadFactorModels[i] )
         return false;
   }

   for ( int i = 0; i < 3; i++ )
   {
      if ( m_SpecialPermitLiveLoadFactorModels[i] != rOther.m_SpecialPermitLiveLoadFactorModels[i] )
         return false;
   }

   if (considerName)
   {
      if ( GetName() != rOther.GetName() )
         return false;
   }

   return true;
}

void RatingLibraryEntry::MakeCopy(const RatingLibraryEntry& rOther)
{
   m_Description          = rOther.m_Description;
   m_SpecificationVersion = rOther.m_SpecificationVersion;
   m_bAlwaysRate          = rOther.m_bAlwaysRate;

   for ( int i = 0; i < 5; i++ )
   {
      m_LiveLoadFactorModels[i] = rOther.m_LiveLoadFactorModels[i];
   }

   for ( int i = 0; i < 3; i++ )
   {
      m_SpecialPermitLiveLoadFactorModels[i] = rOther.m_SpecialPermitLiveLoadFactorModels[i];
   }
}

void RatingLibraryEntry::MakeAssignment(const RatingLibraryEntry& rOther)
{
   libLibraryEntry::MakeAssignment( rOther );
   MakeCopy( rOther );
}

void RatingLibraryEntry::SetDescription(LPCTSTR name)
{
   m_Description.erase();
   m_Description = name;
}

std::_tstring RatingLibraryEntry::GetDescription() const
{
   return m_Description;
}

void RatingLibraryEntry::SetSpecificationVersion(lrfrVersionMgr::Version version)
{
   m_SpecificationVersion = version;
}

lrfrVersionMgr::Version RatingLibraryEntry::GetSpecificationVersion() const
{
   return m_SpecificationVersion;
}

void RatingLibraryEntry::AlwaysLoadRate(bool bAlways)
{
   m_bAlwaysRate = bAlways;
}

bool RatingLibraryEntry::AlwaysLoadRate() const
{
   return m_bAlwaysRate;
}

void RatingLibraryEntry::SetLiveLoadFactorModel(pgsTypes::LoadRatingType ratingType,const CLiveLoadFactorModel& model)
{
   ASSERT(ratingType != pgsTypes::lrPermit_Special);
   m_LiveLoadFactorModels[ratingType] = model;
}

const CLiveLoadFactorModel& RatingLibraryEntry::GetLiveLoadFactorModel(pgsTypes::LoadRatingType ratingType) const
{
   ASSERT(ratingType != pgsTypes::lrPermit_Special);
   return m_LiveLoadFactorModels[ratingType];
}

void RatingLibraryEntry::SetLiveLoadFactorModel(pgsTypes::SpecialPermitType permitType,const CLiveLoadFactorModel& model)
{
   m_SpecialPermitLiveLoadFactorModels[permitType] = model;
}

const CLiveLoadFactorModel& RatingLibraryEntry::GetLiveLoadFactorModel(pgsTypes::SpecialPermitType permitType) const
{
   return m_SpecialPermitLiveLoadFactorModels[permitType];
}
