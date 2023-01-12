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

#include "StdAfx.h"
#include <psgLib\RatingLibraryEntry.h>
#include "resource.h"
#include "RatingDialog.h"

#include <Units\Units.h>

#include <EAF\EAFApp.h>
#include <psgLib\LibraryEntryDifferenceItem.h>

#include <boost\algorithm\string\replace.hpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CURRENT_VERSION 4.0

CLiveLoadFactorModel::CLiveLoadFactorModel()
{
   m_Wlower = WBFL::Units::ConvertToSysUnits(100.0,WBFL::Units::Measure::Kip);
   m_Wupper = WBFL::Units::ConvertToSysUnits(150.0,WBFL::Units::Measure::Kip);

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
   {
      return false;
   }

   if ( !IsEqual(m_Wupper,other.m_Wupper) )
   {
      return false;
   }

   if ( m_LiveLoadFactorType != other.m_LiveLoadFactorType )
   {
      return false;
   }

   if ( m_LiveLoadFactorModifier != other.m_LiveLoadFactorModifier )
   {
      return false;
   }

   if ( m_bAllowUserOverride != other.m_bAllowUserOverride )
   {
      return false;
   }

   if ( m_LiveLoadFactorType == pgsTypes::gllSingleValue )
   {
      if ( m_ADTT[0] != other.m_ADTT[0] )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[0],other.m_gLL_Lower[0]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[0],other.m_gLL_Service[0]) )
      {
         return false;
      }
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllStepped || m_LiveLoadFactorType == pgsTypes::gllLinear )
   {
      if ( m_ADTT[0] != other.m_ADTT[0] )
      {
         return false;
      }

      if ( m_ADTT[1] != other.m_ADTT[1] )
      {
         return false;
      }

      if ( m_ADTT[3] != other.m_ADTT[3] )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[0],other.m_gLL_Lower[0]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[1],other.m_gLL_Lower[1]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[3],other.m_gLL_Lower[3]) )
      {
         return false;
      }


      if ( !IsEqual(m_gLL_Service[0],other.m_gLL_Service[0]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[1],other.m_gLL_Service[1]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[3],other.m_gLL_Service[3]) )
      {
         return false;
      }
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllBilinear )
   {
      if ( m_ADTT[0] != other.m_ADTT[0] )
      {
         return false;
      }

      if ( m_ADTT[1] != other.m_ADTT[1] )
      {
         return false;
      }

      if ( m_ADTT[2] != other.m_ADTT[2] )
      {
         return false;
      }

      if ( m_ADTT[3] != other.m_ADTT[3] )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[0],other.m_gLL_Lower[0]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[1],other.m_gLL_Lower[1]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[2],other.m_gLL_Lower[2]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[3],other.m_gLL_Lower[3]) )
      {
         return false;
      }


      if ( !IsEqual(m_gLL_Service[0],other.m_gLL_Service[0]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[1],other.m_gLL_Service[1]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[2],other.m_gLL_Service[2]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[3],other.m_gLL_Service[3]) )
      {
         return false;
      }
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllBilinearWithWeight )
   {
      if ( m_ADTT[0] != other.m_ADTT[0] )
      {
         return false;
      }

      if ( m_ADTT[1] != other.m_ADTT[1] )
      {
         return false;
      }

      if ( m_ADTT[2] != other.m_ADTT[2] )
      {
         return false;
      }

      if ( m_ADTT[3] != other.m_ADTT[3] )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[0],other.m_gLL_Lower[0]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[1],other.m_gLL_Lower[1]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[2],other.m_gLL_Lower[2]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[3],other.m_gLL_Lower[3]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Upper[0],other.m_gLL_Upper[0]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Upper[1],other.m_gLL_Upper[1]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Upper[2],other.m_gLL_Upper[2]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Upper[3],other.m_gLL_Upper[3]) )
      {
         return false;
      }



      if ( !IsEqual(m_gLL_Service[0],other.m_gLL_Service[0]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[1],other.m_gLL_Service[1]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[2],other.m_gLL_Service[2]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[3],other.m_gLL_Service[3]) )
      {
         return false;
      }
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
   {
      return m_gLL_Lower[0];
   }

   if ( m_LiveLoadFactorType == pgsTypes::gllStepped )
   {
      if ( adtt < 0 ) // unknown
      {
         return m_gLL_Lower[3];
      }

      if ( adtt < m_ADTT[0] )
      {
         return m_gLL_Lower[0];
      }
      else
      {
         return m_gLL_Lower[1];
      }
   }

   if ( m_LiveLoadFactorType == pgsTypes::gllLinear )
   {
      if ( adtt < 0 ) // unknown
      {
         return m_gLL_Lower[3];
      }

      if ( adtt <= m_ADTT[0] )
      {
         return m_gLL_Lower[0];
      }

      if ( m_ADTT[1] <= adtt )
      {
         return m_gLL_Lower[1];
      }

      // adtt is between values
      if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
      {
         return m_gLL_Lower[1];
      }
      else
      {
         return ::LinInterp(adtt-m_ADTT[0],m_gLL_Lower[0],m_gLL_Lower[1],m_ADTT[1] - m_ADTT[0]);
      }
   }

   if ( m_LiveLoadFactorType == pgsTypes::gllBilinear )
   {
      if ( adtt < 0 ) // unknown
      {
         return m_gLL_Lower[3];
      }

      if ( adtt <= m_ADTT[0] )
      {
         return m_gLL_Lower[0];
      }

      if ( m_ADTT[2] <= adtt )
      {
         return m_gLL_Lower[2];
      }

      if ( adtt <= m_ADTT[1] )
      {
         // between ADTT[0] and ADTT[1]
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
         {
            return m_gLL_Lower[1];
         }
         else
         {
            return ::LinInterp(adtt-m_ADTT[0],m_gLL_Lower[0],m_gLL_Lower[1],m_ADTT[1] - m_ADTT[0]);
         }
      }

      if ( adtt < m_ADTT[2] )
      {
         // between ADTT[1] and ADTT[2]
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
         {
            return m_gLL_Lower[2];
         }
         else
         {
            return ::LinInterp(adtt-m_ADTT[1],m_gLL_Lower[1],m_gLL_Lower[2],m_ADTT[2] - m_ADTT[1]);
         }
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
            {
               return m_gLL_Lower[3];
            }
            else if ( m_Wupper <= W )
            {
               return m_gLL_Upper[3];
            }
            else
            {
               return ::LinInterp(W-m_Wlower,m_gLL_Lower[4],m_gLL_Upper[4],m_Wupper-m_Wlower);
            }
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
            {
               return m_gLL_Lower[0];
            }
            else if ( m_Wupper <= W )
            {
               return m_gLL_Upper[0];
            }
            else
            {
               return ::LinInterp(W-m_Wlower,m_gLL_Lower[0],m_gLL_Upper[0],m_Wupper-m_Wlower);
            }
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
            {
               return m_gLL_Lower[2];
            }
            else if ( m_Wupper <= W )
            {
               return m_gLL_Upper[2];
            }
            else
            {
               return ::LinInterp(W-m_Wlower,m_gLL_Lower[2],m_gLL_Upper[2],m_Wupper-m_Wlower);
            }
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
            {
               return g_Lower;
            }
            else if ( m_Wupper <= W )
            {
               return g_Upper;
            }
            else
            {
               return ::LinInterp(W-m_Wlower,g_Lower,g_Upper,m_Wupper-m_Wlower);
            }
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
            {
               return g_Lower;
            }
            else if ( m_Wupper <= W )
            {
               return g_Upper;
            }
            else
            {
               return ::LinInterp(W-m_Wlower,g_Lower,g_Upper,m_Wupper-m_Wlower);
            }
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
      {
         return m_gLL_Service[0];
      }
      else
      {
         return m_gLL_Service[1];
      }
   }

   if ( m_LiveLoadFactorType == pgsTypes::gllLinear )
   {
      if ( adtt < 0 ) // unknown
      {
         return m_gLL_Service[3];
      }

      if ( adtt <= m_ADTT[0] )
      {
         return m_gLL_Service[0];
      }

      if ( m_ADTT[1] <= adtt )
      {
         return m_gLL_Service[1];
      }

      // adtt is between values
      if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
      {
         return m_gLL_Service[1];
      }
      else
      {
         return ::LinInterp(adtt-m_ADTT[0],m_gLL_Service[0],m_gLL_Service[1],m_ADTT[1] - m_ADTT[0]);
      }
   }

   if ( m_LiveLoadFactorType == pgsTypes::gllBilinear || m_LiveLoadFactorType == pgsTypes::gllBilinearWithWeight)
   {
      // truck weight does not effect service load factors
      if ( adtt < 0 ) // unknown
      {
         return m_gLL_Service[3];
      }

      if ( adtt <= m_ADTT[0] )
      {
         return m_gLL_Service[0];
      }

      if ( m_ADTT[2] <= adtt )
      {
         return m_gLL_Service[2];
      }

      if ( adtt <= m_ADTT[1] )
      {
         // between ADTT[0] and ADTT[1]
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
         {
            return m_gLL_Service[1];
         }
         else
         {
            return ::LinInterp(adtt-m_ADTT[0],m_gLL_Service[0],m_gLL_Service[1],m_ADTT[1] - m_ADTT[0]);
         }
      }

      if ( adtt < m_ADTT[2] )
      {
         // between ADTT[1] and ADTT[2]
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
         {
            return m_gLL_Service[2];
         }
         else
         {
            return ::LinInterp(adtt-m_ADTT[1],m_gLL_Service[1],m_gLL_Service[2],m_ADTT[2] - m_ADTT[1]);
         }
      }
   }


   ASSERT(false); // should not get here

   return -9999; // something obviously bogus until this gets implemented
}

bool CLiveLoadFactorModel::SaveMe(WBFL::System::IStructuredSave* pSave)
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

bool CLiveLoadFactorModel::LoadMe(WBFL::System::IStructuredLoad* pLoad)
{
   if ( !pLoad->BeginUnit(_T("LoadFactors")) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   int value;
   if ( !pLoad->Property(_T("LiveLoadFactorType"),&value) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   m_LiveLoadFactorType = (pgsTypes::LiveLoadFactorType)value;

   if ( m_LiveLoadFactorType == pgsTypes::gllSingleValue )
   {
      if ( !pLoad->Property(_T("LiveLoadFactor"),&m_gLL_Lower[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor_Service"),&m_gLL_Service[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllStepped || m_LiveLoadFactorType == pgsTypes::gllLinear )
   {
      if ( !pLoad->Property(_T("ADTT1"),&m_ADTT[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor1"),&m_gLL_Lower[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor2"),&m_gLL_Lower[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT"),&m_gLL_Lower[3]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor1_Service"),&m_gLL_Service[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor2_Service"),&m_gLL_Service[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT_Service"),&m_gLL_Service[3]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorModifier"),&value) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
      
      m_LiveLoadFactorModifier = (pgsTypes::LiveLoadFactorModifier)value;
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllBilinear )
   {
      if ( !pLoad->Property(_T("ADTT1"),&m_ADTT[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor1"),&m_gLL_Lower[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("ADTT2"),&m_ADTT[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor2"),&m_gLL_Lower[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("ADTT3"),&m_ADTT[2]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor3"),&m_gLL_Lower[2]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT"),&m_gLL_Lower[3]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }


      if ( !pLoad->Property(_T("LiveLoadFactor1_Service"),&m_gLL_Service[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor2_Service"),&m_gLL_Service[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor3_Service"),&m_gLL_Service[2]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT_Service"),&m_gLL_Service[3]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorModifier"),&value) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
      
      m_LiveLoadFactorModifier = (pgsTypes::LiveLoadFactorModifier)value;
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllBilinearWithWeight )
   {
      if ( !pLoad->Property(_T("VehicleWeight1"),&m_Wlower) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("VehicleWeight2"),&m_Wupper) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("ADTT1"),&m_ADTT[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor1_Lower"),&m_gLL_Lower[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor1_Upper"),&m_gLL_Upper[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("ADTT2"),&m_ADTT[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor2_Lower"),&m_gLL_Lower[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor2_Upper"),&m_gLL_Upper[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("ADTT3"),&m_ADTT[2]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor3_Lower"),&m_gLL_Lower[2]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor3_Upper"),&m_gLL_Upper[2]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT_Lower"),&m_gLL_Lower[3]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT_Upper"),&m_gLL_Upper[3]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }


      if ( !pLoad->Property(_T("LiveLoadFactor1_Service"),&m_gLL_Service[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor2_Service"),&m_gLL_Service[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor3_Service"),&m_gLL_Service[2]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT_Service"),&m_gLL_Service[3]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorModifier"),&value) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
      
      m_LiveLoadFactorModifier = (pgsTypes::LiveLoadFactorModifier)value;
   }
   else
   {
      ATLASSERT(false); // should never get here
   }

   if ( !pLoad->Property(_T("AllowUserOverride"),&m_bAllowUserOverride) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if ( !pLoad->EndUnit() )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   return true;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CLiveLoadFactorModel2::CLiveLoadFactorModel2()
{
   m_PWRlower = WBFL::Units::ConvertToSysUnits(2.0,WBFL::Units::Measure::KipPerFoot);
   m_PWRupper = WBFL::Units::ConvertToSysUnits(3.0,WBFL::Units::Measure::KipPerFoot);

   m_LiveLoadFactorType     = pgsTypes::gllLinear;
   m_LiveLoadFactorModifier = pgsTypes::gllmInterpolate;

   m_ADTT[0] = 100;
   m_gLL_Lower[0] = 1.4;
   m_gLL_Middle[0] = 1.4;
   m_gLL_Upper[0] = 1.4;
   m_gLL_Service[0] = 1.0;

   m_ADTT[1] = 1000;
   m_gLL_Lower[1] = 1.65;
   m_gLL_Upper[1] = 1.65;
   m_gLL_Middle[1] = 1.65;
   m_gLL_Service[1] = 1.0;

   m_ADTT[2] = 5000;
   m_gLL_Lower[2] = 1.80;
   m_gLL_Middle[2] = 1.80;
   m_gLL_Upper[2] = 1.80;
   m_gLL_Service[2] = 1.0;

   m_ADTT[3] = -1; // unknown
   m_gLL_Lower[3] = 1.80;
   m_gLL_Middle[3] = 1.80;
   m_gLL_Upper[3] = 1.80;
   m_gLL_Service[3] = 1.0;

   m_bAllowUserOverride = false;
}

bool CLiveLoadFactorModel2::operator!=(const CLiveLoadFactorModel2& other) const
{
   return !operator==(other);
}

bool CLiveLoadFactorModel2::operator==(const CLiveLoadFactorModel2& other) const
{
   if ( !IsEqual(m_PWRlower,other.m_PWRlower) )
   {
      return false;
   }

   if ( !IsEqual(m_PWRupper,other.m_PWRupper) )
   {
      return false;
   }

   if ( m_LiveLoadFactorType != other.m_LiveLoadFactorType )
   {
      return false;
   }

   if ( m_LiveLoadFactorModifier != other.m_LiveLoadFactorModifier )
   {
      return false;
   }

   if ( m_bAllowUserOverride != other.m_bAllowUserOverride )
   {
      return false;
   }

   if ( m_LiveLoadFactorType == pgsTypes::gllSingleValue )
   {
      if ( m_ADTT[0] != other.m_ADTT[0] )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[0],other.m_gLL_Lower[0]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[0],other.m_gLL_Service[0]) )
      {
         return false;
      }
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllStepped || m_LiveLoadFactorType == pgsTypes::gllLinear )
   {
      if ( m_ADTT[0] != other.m_ADTT[0] )
      {
         return false;
      }

      if ( m_ADTT[1] != other.m_ADTT[1] )
      {
         return false;
      }

      if ( m_ADTT[3] != other.m_ADTT[3] )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[0],other.m_gLL_Lower[0]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[1],other.m_gLL_Lower[1]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[3],other.m_gLL_Lower[3]) )
      {
         return false;
      }


      if ( !IsEqual(m_gLL_Service[0],other.m_gLL_Service[0]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[1],other.m_gLL_Service[1]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[3],other.m_gLL_Service[3]) )
      {
         return false;
      }
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllBilinear )
   {
      if ( m_ADTT[0] != other.m_ADTT[0] )
      {
         return false;
      }

      if ( m_ADTT[1] != other.m_ADTT[1] )
      {
         return false;
      }

      if ( m_ADTT[2] != other.m_ADTT[2] )
      {
         return false;
      }

      if ( m_ADTT[3] != other.m_ADTT[3] )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[0],other.m_gLL_Lower[0]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[1],other.m_gLL_Lower[1]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[2],other.m_gLL_Lower[2]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[3],other.m_gLL_Lower[3]) )
      {
         return false;
      }


      if ( !IsEqual(m_gLL_Service[0],other.m_gLL_Service[0]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[1],other.m_gLL_Service[1]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[2],other.m_gLL_Service[2]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[3],other.m_gLL_Service[3]) )
      {
         return false;
      }
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllBilinearWithWeight )
   {
      if ( m_ADTT[0] != other.m_ADTT[0] )
      {
         return false;
      }

      if ( m_ADTT[1] != other.m_ADTT[1] )
      {
         return false;
      }

      if ( m_ADTT[2] != other.m_ADTT[2] )
      {
         return false;
      }

      if ( m_ADTT[3] != other.m_ADTT[3] )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[0],other.m_gLL_Lower[0]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[1],other.m_gLL_Lower[1]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[2],other.m_gLL_Lower[2]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Lower[3],other.m_gLL_Lower[3]) )
      {
         return false;
      }


      if ( !IsEqual(m_gLL_Middle[0],other.m_gLL_Middle[0]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Middle[1],other.m_gLL_Middle[1]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Middle[2],other.m_gLL_Middle[2]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Middle[3],other.m_gLL_Middle[3]) )
      {
         return false;
      }


      if ( !IsEqual(m_gLL_Upper[0],other.m_gLL_Upper[0]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Upper[1],other.m_gLL_Upper[1]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Upper[2],other.m_gLL_Upper[2]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Upper[3],other.m_gLL_Upper[3]) )
      {
         return false;
      }



      if ( !IsEqual(m_gLL_Service[0],other.m_gLL_Service[0]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[1],other.m_gLL_Service[1]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[2],other.m_gLL_Service[2]) )
      {
         return false;
      }

      if ( !IsEqual(m_gLL_Service[3],other.m_gLL_Service[3]) )
      {
         return false;
      }
   }

   return true;
}

void CLiveLoadFactorModel2::SetPermitWeightRatio(Float64 PWRlower,Float64 PWRupper)
{
   m_PWRlower = PWRlower;
   m_PWRupper = PWRupper;
}

void CLiveLoadFactorModel2::GetPermitWeightRatio(Float64* pPWRlower,Float64* pPWRupper) const
{
   *pPWRlower = m_PWRlower;
   *pPWRupper = m_PWRupper;
}

void CLiveLoadFactorModel2::SetLiveLoadFactorType(pgsTypes::LiveLoadFactorType gllType)
{
   m_LiveLoadFactorType = gllType;
}

pgsTypes::LiveLoadFactorType CLiveLoadFactorModel2::GetLiveLoadFactorType() const
{
   return m_LiveLoadFactorType;
}

void CLiveLoadFactorModel2::SetLiveLoadFactorModifier(pgsTypes::LiveLoadFactorModifier gllModifier)
{
   m_LiveLoadFactorModifier = gllModifier;
}

pgsTypes::LiveLoadFactorModifier CLiveLoadFactorModel2::GetLiveLoadFactorModifier() const
{
   return m_LiveLoadFactorModifier;
}

void CLiveLoadFactorModel2::AllowUserOverride(bool bAllow)
{
   m_bAllowUserOverride = bAllow;
}

bool CLiveLoadFactorModel2::AllowUserOverride() const
{
   return m_bAllowUserOverride;
}

void CLiveLoadFactorModel2::SetADTT(Int16 adtt1, Int16 adtt2, Int16 adtt3, Int16 adtt4)
{
   m_ADTT[0] = adtt1;
   m_ADTT[1] = adtt2;
   m_ADTT[2] = adtt3;
   m_ADTT[3] = adtt4;
}

void CLiveLoadFactorModel2::GetADTT(Int16* adtt1, Int16* adtt2, Int16* adtt3, Int16* adtt4) const
{
   *adtt1 = m_ADTT[0];
   *adtt2 = m_ADTT[1];
   *adtt3 = m_ADTT[2];
   *adtt4 = m_ADTT[3];
}

void CLiveLoadFactorModel2::SetLowerLiveLoadFactor(Float64 gll1,Float64 gll2,Float64 gll3,Float64 gll4)
{
   m_gLL_Lower[0] = gll1;
   m_gLL_Lower[1] = gll2;
   m_gLL_Lower[2] = gll3;
   m_gLL_Lower[3] = gll4;
}

void CLiveLoadFactorModel2::GetLowerLiveLoadFactor(Float64* gll1,Float64* gll2,Float64* gll3,Float64* gll4) const
{
   *gll1 = m_gLL_Lower[0];
   *gll2 = m_gLL_Lower[1];
   *gll3 = m_gLL_Lower[2];
   *gll4 = m_gLL_Lower[3];
}

void CLiveLoadFactorModel2::SetMiddleLiveLoadFactor(Float64 gll1,Float64 gll2,Float64 gll3,Float64 gll4)
{
   m_gLL_Middle[0] = gll1;
   m_gLL_Middle[1] = gll2;
   m_gLL_Middle[2] = gll3;
   m_gLL_Middle[3] = gll4;
}

void CLiveLoadFactorModel2::GetMiddleLiveLoadFactor(Float64* gll1,Float64* gll2,Float64* gll3,Float64* gll4) const
{
   *gll1 = m_gLL_Middle[0];
   *gll2 = m_gLL_Middle[1];
   *gll3 = m_gLL_Middle[2];
   *gll4 = m_gLL_Middle[3];
}

void CLiveLoadFactorModel2::SetUpperLiveLoadFactor(Float64 gll1,Float64 gll2,Float64 gll3,Float64 gll4)
{
   m_gLL_Upper[0] = gll1;
   m_gLL_Upper[1] = gll2;
   m_gLL_Upper[2] = gll3;
   m_gLL_Upper[3] = gll4;
}

void CLiveLoadFactorModel2::GetUpperLiveLoadFactor(Float64* gll1,Float64* gll2,Float64* gll3,Float64* gll4) const
{
   *gll1 = m_gLL_Upper[0];
   *gll2 = m_gLL_Upper[1];
   *gll3 = m_gLL_Upper[2];
   *gll4 = m_gLL_Upper[3];
}

void CLiveLoadFactorModel2::SetServiceLiveLoadFactor(Float64 gll1,Float64 gll2,Float64 gll3,Float64 gll4)
{
   m_gLL_Service[0] = gll1;
   m_gLL_Service[1] = gll2;
   m_gLL_Service[2] = gll3;
   m_gLL_Service[3] = gll4;
}

void CLiveLoadFactorModel2::GetServiceLiveLoadFactor(Float64* gll1,Float64* gll2,Float64* gll3,Float64* gll4) const
{
   *gll1 = m_gLL_Service[0];
   *gll2 = m_gLL_Service[1];
   *gll3 = m_gLL_Service[2];
   *gll4 = m_gLL_Service[3];
}

Float64 CLiveLoadFactorModel2::GetStrengthLiveLoadFactor(Int16 adtt,Float64 PWR) const
{
   if ( m_LiveLoadFactorType == pgsTypes::gllSingleValue )
   {
      return m_gLL_Lower[0];
   }

   if ( m_LiveLoadFactorType == pgsTypes::gllStepped )
   {
      if ( adtt < 0 ) // unknown
      {
         return m_gLL_Lower[3];
      }

      if ( adtt < m_ADTT[0] )
      {
         return m_gLL_Lower[0];
      }
      else
      {
         return m_gLL_Lower[1];
      }
   }

   if ( m_LiveLoadFactorType == pgsTypes::gllLinear )
   {
      if ( adtt < 0 ) // unknown
      {
         return m_gLL_Lower[3];
      }

      if ( adtt <= m_ADTT[0] )
      {
         return m_gLL_Lower[0];
      }

      if ( m_ADTT[1] <= adtt )
      {
         return m_gLL_Lower[1];
      }

      // adtt is between values
      if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
      {
         return m_gLL_Lower[1];
      }
      else
      {
         return ::LinInterp(adtt-m_ADTT[0],m_gLL_Lower[0],m_gLL_Lower[1],m_ADTT[1] - m_ADTT[0]);
      }
   }

   if ( m_LiveLoadFactorType == pgsTypes::gllBilinear )
   {
      if ( adtt < 0 ) // unknown
      {
         return m_gLL_Lower[3];
      }

      if ( adtt <= m_ADTT[0] )
      {
         return m_gLL_Lower[0];
      }

      if ( m_ADTT[2] <= adtt )
      {
         return m_gLL_Lower[2];
      }

      if ( adtt <= m_ADTT[1] )
      {
         // between ADTT[0] and ADTT[1]
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
         {
            return m_gLL_Lower[1];
         }
         else
         {
            return ::LinInterp(adtt-m_ADTT[0],m_gLL_Lower[0],m_gLL_Lower[1],m_ADTT[1] - m_ADTT[0]);
         }
      }

      if ( adtt < m_ADTT[2] )
      {
         // between ADTT[1] and ADTT[2]
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
         {
            return m_gLL_Lower[2];
         }
         else
         {
            return ::LinInterp(adtt-m_ADTT[1],m_gLL_Lower[1],m_gLL_Lower[2],m_ADTT[2] - m_ADTT[1]);
         }
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
            if ( PWR < m_PWRlower )
            {
               return m_gLL_Lower[3];
            }
            else if ( m_PWRupper <= PWR )
            {
               return m_gLL_Upper[3];
            }
            else
            {
               return m_gLL_Middle[3];
            }
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
            if ( PWR < m_PWRlower )
            {
               return m_gLL_Lower[0];
            }
            else if ( m_PWRupper <= PWR )
            {
               return m_gLL_Upper[0];
            }
            else
            {
               return m_gLL_Middle[0];
            }
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
            if ( PWR < m_PWRlower )
            {
               return m_gLL_Lower[2];
            }
            else if ( m_PWRupper <= PWR )
            {
               return m_gLL_Upper[2];
            }
            else
            {
               return m_gLL_Middle[2];
            }
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
            Float64 g_Lower  = ::LinInterp(adtt-m_ADTT[0],m_gLL_Lower[0], m_gLL_Lower[1], m_ADTT[1] - m_ADTT[0]);
            Float64 g_Middle = ::LinInterp(adtt-m_ADTT[0],m_gLL_Middle[0],m_gLL_Middle[1],m_ADTT[1] - m_ADTT[0]);
            Float64 g_Upper  = ::LinInterp(adtt-m_ADTT[0],m_gLL_Upper[0], m_gLL_Upper[1], m_ADTT[1] - m_ADTT[0]);

            // interpolate on permit weight ratio
            if ( PWR < m_PWRlower )
            {
               return g_Lower;
            }
            else if ( m_PWRupper <= PWR )
            {
               return g_Upper;
            }
            else
            {
               return g_Middle;
            }
         }
      }

      if ( adtt <= m_ADTT[2] )
      {
         // between ADTT[1] and ADTT[2]
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
         {
            return m_gLL_Upper[2];
         }
         else
         {
            // interpolate on ADTT
            Float64 g_Lower  = ::LinInterp(adtt-m_ADTT[1],m_gLL_Lower[1], m_gLL_Lower[2], m_ADTT[2] - m_ADTT[1]);
            Float64 g_Middle = ::LinInterp(adtt-m_ADTT[1],m_gLL_Middle[1],m_gLL_Middle[2],m_ADTT[2] - m_ADTT[1]);
            Float64 g_Upper  = ::LinInterp(adtt-m_ADTT[1],m_gLL_Upper[1], m_gLL_Upper[2], m_ADTT[2] - m_ADTT[1]);

            // interpolate on permit weight ratio
            if ( PWR < m_PWRlower )
            {
               return g_Lower;
            }
            else if ( m_PWRupper <= PWR )
            {
               return g_Upper;
            }
            else
            {
               return g_Middle;
            }
         }
      }
   }

   ASSERT(false); // should not get here

   return -9999; // something obviously bogus until this gets implemented
}


Float64 CLiveLoadFactorModel2::GetServiceLiveLoadFactor(Int16 adtt) const
{
   if ( m_LiveLoadFactorType == pgsTypes::gllSingleValue )
   {
      return m_gLL_Service[0];
   }

   if ( m_LiveLoadFactorType == pgsTypes::gllStepped )
   {
      if ( adtt < 0 ) // unknown
      {
         return m_gLL_Service[3];
      }

      if ( adtt < m_ADTT[0] )
      {
         return m_gLL_Service[0];
      }
      else
      {
         return m_gLL_Service[1];
      }
   }

   if ( m_LiveLoadFactorType == pgsTypes::gllLinear )
   {
      if ( adtt < 0 ) // unknown
      {
         return m_gLL_Service[3];
      }

      if ( adtt < m_ADTT[0] )
      {
         return m_gLL_Service[0];
      }

      if ( m_ADTT[1] < adtt )
      {
         return m_gLL_Service[1];
      }

      // adtt is between values
      if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
      {
         return m_gLL_Service[1];
      }
      else
      {
         return ::LinInterp(adtt-m_ADTT[0],m_gLL_Service[0],m_gLL_Service[1],m_ADTT[1] - m_ADTT[0]);
      }
   }

   if ( m_LiveLoadFactorType == pgsTypes::gllBilinear || m_LiveLoadFactorType == pgsTypes::gllBilinearWithWeight)
   {
      // truck weight does not effect service load factors
      if ( adtt < 0 ) // unknown
      {
         return m_gLL_Service[3];
      }

      if ( adtt < m_ADTT[0] )
      {
         return m_gLL_Service[0];
      }

      if ( m_ADTT[2] < adtt )
      {
         return m_gLL_Service[2];
      }

      if ( adtt < m_ADTT[1] )
      {
         // between ADTT[0] and ADTT[1]
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
         {
            return m_gLL_Service[1];
         }
         else
         {
            return ::LinInterp(adtt-m_ADTT[0],m_gLL_Service[0],m_gLL_Service[1],m_ADTT[1] - m_ADTT[0]);
         }
      }

      if ( adtt <= m_ADTT[2] )
      {
         // between ADTT[1] and ADTT[2]
         if ( m_LiveLoadFactorModifier == pgsTypes::gllmRoundUp )
         {
            return m_gLL_Service[2];
         }
         else
         {
            return ::LinInterp(adtt-m_ADTT[1],m_gLL_Service[1],m_gLL_Service[2],m_ADTT[2] - m_ADTT[1]);
         }
      }
   }


   ASSERT(false); // should not get here

   return -9999; // something obviously bogus until this gets implemented
}

bool CLiveLoadFactorModel2::SaveMe(WBFL::System::IStructuredSave* pSave)
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
      pSave->Property(_T("PermitWeightRatio1"),m_PWRlower);
      pSave->Property(_T("PermitWeightRatio2"),m_PWRupper);

      pSave->Property(_T("ADTT1"),m_ADTT[0]);
      pSave->Property(_T("LiveLoadFactor1_Lower"),m_gLL_Lower[0]);
      pSave->Property(_T("LiveLoadFactor1_Middle"),m_gLL_Middle[0]);
      pSave->Property(_T("LiveLoadFactor1_Upper"),m_gLL_Upper[0]);
      pSave->Property(_T("ADTT2"),m_ADTT[1]);
      pSave->Property(_T("LiveLoadFactor2_Lower"),m_gLL_Lower[1]);
      pSave->Property(_T("LiveLoadFactor2_Middle"),m_gLL_Middle[1]);
      pSave->Property(_T("LiveLoadFactor2_Upper"),m_gLL_Upper[1]);
      pSave->Property(_T("ADTT3"),m_ADTT[2]);
      pSave->Property(_T("LiveLoadFactor3_Lower"),m_gLL_Lower[2]);
      pSave->Property(_T("LiveLoadFactor3_Middle"),m_gLL_Middle[2]);
      pSave->Property(_T("LiveLoadFactor3_Upper"),m_gLL_Upper[2]);
      pSave->Property(_T("LiveLoadFactorUnknownADTT_Lower"),m_gLL_Lower[3]);
      pSave->Property(_T("LiveLoadFactorUnknownADTT_Middle"),m_gLL_Middle[3]);
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

bool CLiveLoadFactorModel2::LoadMe(WBFL::System::IStructuredLoad* pLoad)
{
   if ( !pLoad->BeginUnit(_T("LoadFactors")) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   int value;
   if ( !pLoad->Property(_T("LiveLoadFactorType"),&value) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   m_LiveLoadFactorType = (pgsTypes::LiveLoadFactorType)value;

   if ( m_LiveLoadFactorType == pgsTypes::gllSingleValue )
   {
      if ( !pLoad->Property(_T("LiveLoadFactor"),&m_gLL_Lower[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor_Service"),&m_gLL_Service[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllStepped || m_LiveLoadFactorType == pgsTypes::gllLinear )
   {
      if ( !pLoad->Property(_T("ADTT1"),&m_ADTT[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor1"),&m_gLL_Lower[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor2"),&m_gLL_Lower[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT"),&m_gLL_Lower[3]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor1_Service"),&m_gLL_Service[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor2_Service"),&m_gLL_Service[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT_Service"),&m_gLL_Service[3]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorModifier"),&value) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
      
      m_LiveLoadFactorModifier = (pgsTypes::LiveLoadFactorModifier)value;
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllBilinear )
   {
      if ( !pLoad->Property(_T("ADTT1"),&m_ADTT[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor1"),&m_gLL_Lower[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("ADTT2"),&m_ADTT[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor2"),&m_gLL_Lower[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("ADTT3"),&m_ADTT[2]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor3"),&m_gLL_Lower[2]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT"),&m_gLL_Lower[3]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }


      if ( !pLoad->Property(_T("LiveLoadFactor1_Service"),&m_gLL_Service[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor2_Service"),&m_gLL_Service[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor3_Service"),&m_gLL_Service[2]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT_Service"),&m_gLL_Service[3]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorModifier"),&value) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
      
      m_LiveLoadFactorModifier = (pgsTypes::LiveLoadFactorModifier)value;
   }
   else if ( m_LiveLoadFactorType == pgsTypes::gllBilinearWithWeight )
   {
      if ( !pLoad->Property(_T("PermitWeightRatio1"),&m_PWRlower) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("PermitWeightRatio2"),&m_PWRupper) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("ADTT1"),&m_ADTT[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor1_Lower"),&m_gLL_Lower[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor1_Middle"),&m_gLL_Middle[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor1_Upper"),&m_gLL_Upper[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("ADTT2"),&m_ADTT[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor2_Lower"),&m_gLL_Lower[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor2_Middle"),&m_gLL_Middle[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor2_Upper"),&m_gLL_Upper[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("ADTT3"),&m_ADTT[2]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor3_Lower"),&m_gLL_Lower[2]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor3_Middle"),&m_gLL_Middle[2]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor3_Upper"),&m_gLL_Upper[2]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT_Lower"),&m_gLL_Lower[3]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT_Middle"),&m_gLL_Middle[3]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT_Upper"),&m_gLL_Upper[3]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }


      if ( !pLoad->Property(_T("LiveLoadFactor1_Service"),&m_gLL_Service[0]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor2_Service"),&m_gLL_Service[1]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactor3_Service"),&m_gLL_Service[2]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorUnknownADTT_Service"),&m_gLL_Service[3]) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }

      if ( !pLoad->Property(_T("LiveLoadFactorModifier"),&value) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
      
      m_LiveLoadFactorModifier = (pgsTypes::LiveLoadFactorModifier)value;
   }
   else
   {
      ATLASSERT(false); // should never get here
   }

   if ( !pLoad->Property(_T("AllowUserOverride"),&m_bAllowUserOverride) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if ( !pLoad->EndUnit() )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   return true;
}



/****************************************************************************
CLASS
   RatingLibraryEntry
****************************************************************************/
CString RatingLibraryEntry::GetLoadRatingType(pgsTypes::LoadRatingType ratingType)
{
   LPCTSTR lpszRatingType;
   switch(ratingType)
   {
   case pgsTypes::lrDesign_Inventory:
      lpszRatingType = _T("Design - Inventory");
      break;

   case pgsTypes::lrDesign_Operating:
      lpszRatingType = _T("Design - Operating");
      break;

   case pgsTypes::lrLegal_Routine:
      lpszRatingType = _T("Legal - Routine");
      break;

   case pgsTypes::lrLegal_Special:
      lpszRatingType = _T("Legal - Special");
      break;

   case pgsTypes::lrLegal_Emergency:
      lpszRatingType = _T("Legal - Emergency");
      break;

   case pgsTypes::lrPermit_Routine:
      lpszRatingType = _T("Permit - Routine");
      break;

   case pgsTypes::lrPermit_Special:
      lpszRatingType = _T("Permit - Special");
      break;

   default:
      ATLASSERT(false); // should never get here
   }
   return lpszRatingType;
}

CString RatingLibraryEntry::GetSpecialPermitType(pgsTypes::SpecialPermitType permitType)
{
   LPCTSTR lpszPermitType;
   switch(permitType)
   {
   case pgsTypes::ptSingleTripWithEscort:
      lpszPermitType = _T("Permit - Special, Single trip with escort");
      break;

   case pgsTypes::ptSingleTripWithTraffic:
      lpszPermitType = _T("Permit - Special, Single trip with traffic");
      break;

   case pgsTypes::ptMultipleTripWithTraffic:
      lpszPermitType = _T("Permit - Special, Multiple trips with traffic");
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return lpszPermitType;
}

RatingLibraryEntry::RatingLibraryEntry() :
m_bUseCurrentSpecification(true),
m_SpecificationVersion(lrfrVersionMgr::SecondEditionWith2016Interims)
{
   // default for LRFR before 2013
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
   m_LiveLoadFactorModels[pgsTypes::lrLegal_Special].SetADTT(100, 1000, 5000, -1);
   m_LiveLoadFactorModels[pgsTypes::lrLegal_Special].SetLowerLiveLoadFactor(1.15, 1.40, 1.60, 1.60);

   m_LiveLoadFactorModels[pgsTypes::lrLegal_Emergency].SetLiveLoadFactorType(pgsTypes::gllSingleValue);
   m_LiveLoadFactorModels[pgsTypes::lrLegal_Emergency].SetLowerLiveLoadFactor(1.3, -1, -1, -1);
   m_LiveLoadFactorModels[pgsTypes::lrLegal_Emergency].SetServiceLiveLoadFactor(1.0, -1, -1, -1);

   m_LiveLoadFactorModels[pgsTypes::lrPermit_Routine].SetLiveLoadFactorType(pgsTypes::gllBilinearWithWeight);
   m_LiveLoadFactorModels[pgsTypes::lrPermit_Routine].SetADTT(100,1000,5000,-1);
   m_LiveLoadFactorModels[pgsTypes::lrPermit_Routine].SetVehicleWeight(WBFL::Units::ConvertToSysUnits(100,WBFL::Units::Measure::Kip),WBFL::Units::ConvertToSysUnits(150,WBFL::Units::Measure::Kip));
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


   // default for LRFR 2013 and later
   m_LiveLoadFactorModels2[pgsTypes::lrDesign_Inventory].SetLiveLoadFactorType(pgsTypes::gllSingleValue);
   m_LiveLoadFactorModels2[pgsTypes::lrDesign_Inventory].SetLowerLiveLoadFactor(1.75,-1,-1,-1);
   m_LiveLoadFactorModels2[pgsTypes::lrDesign_Inventory].SetServiceLiveLoadFactor(0.8,-1,-1,-1);

   m_LiveLoadFactorModels2[pgsTypes::lrDesign_Operating].SetLiveLoadFactorType(pgsTypes::gllSingleValue);
   m_LiveLoadFactorModels2[pgsTypes::lrDesign_Operating].SetLowerLiveLoadFactor(1.35,-1,-1,-1);
   m_LiveLoadFactorModels2[pgsTypes::lrDesign_Operating].SetServiceLiveLoadFactor(0.8,-1,-1,-1);

   m_LiveLoadFactorModels2[pgsTypes::lrLegal_Routine].SetLiveLoadFactorType(pgsTypes::gllLinear);
   m_LiveLoadFactorModels2[pgsTypes::lrLegal_Routine].SetADTT(1000,5000,-1,-1);
   m_LiveLoadFactorModels2[pgsTypes::lrLegal_Routine].SetLowerLiveLoadFactor(1.30,1.45,-1,1.45);

   m_LiveLoadFactorModels2[pgsTypes::lrLegal_Special].SetLiveLoadFactorType(pgsTypes::gllLinear);
   m_LiveLoadFactorModels2[pgsTypes::lrLegal_Special].SetADTT(1000,5000,-1,-1);
   m_LiveLoadFactorModels2[pgsTypes::lrLegal_Special].SetLowerLiveLoadFactor(1.30,1.45,-1,1.45);

   m_LiveLoadFactorModels2[pgsTypes::lrLegal_Emergency].SetLiveLoadFactorType(pgsTypes::gllSingleValue);
   m_LiveLoadFactorModels2[pgsTypes::lrLegal_Emergency].SetLowerLiveLoadFactor(1.3, -1, -1, -1);
   m_LiveLoadFactorModels2[pgsTypes::lrLegal_Emergency].SetServiceLiveLoadFactor(1.0, -1, -1, -1);

   m_LiveLoadFactorModels2[pgsTypes::lrPermit_Routine].SetLiveLoadFactorType(pgsTypes::gllBilinearWithWeight);
   m_LiveLoadFactorModels2[pgsTypes::lrPermit_Routine].SetADTT(100,1000,5000,-1);
   m_LiveLoadFactorModels2[pgsTypes::lrPermit_Routine].SetPermitWeightRatio(WBFL::Units::ConvertToSysUnits(2.0,WBFL::Units::Measure::KipPerFoot),WBFL::Units::ConvertToSysUnits(3.0,WBFL::Units::Measure::KipPerFoot));
   m_LiveLoadFactorModels2[pgsTypes::lrPermit_Routine].SetLowerLiveLoadFactor(1.30,1.35,1.40,1.40);
   m_LiveLoadFactorModels2[pgsTypes::lrPermit_Routine].SetMiddleLiveLoadFactor(1.20,1.25,1.35,1.35);
   m_LiveLoadFactorModels2[pgsTypes::lrPermit_Routine].SetUpperLiveLoadFactor(1.15,1.20,1.30,1.30);

   m_SpecialPermitLiveLoadFactorModels2[pgsTypes::ptSingleTripWithEscort].SetLiveLoadFactorType(pgsTypes::gllSingleValue);
   m_SpecialPermitLiveLoadFactorModels2[pgsTypes::ptSingleTripWithEscort].SetLowerLiveLoadFactor(1.10,-1,-1,-1);

   m_SpecialPermitLiveLoadFactorModels2[pgsTypes::ptSingleTripWithTraffic].SetLiveLoadFactorType(pgsTypes::gllSingleValue);
   m_SpecialPermitLiveLoadFactorModels2[pgsTypes::ptSingleTripWithTraffic].SetLowerLiveLoadFactor(1.20,-1,-1,-1);

   m_SpecialPermitLiveLoadFactorModels2[pgsTypes::ptMultipleTripWithTraffic].SetLiveLoadFactorType(pgsTypes::gllSingleValue);
   m_SpecialPermitLiveLoadFactorModels2[pgsTypes::ptMultipleTripWithTraffic].SetLowerLiveLoadFactor(1.40,-1,-1,-1);
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

bool RatingLibraryEntry::Edit(bool allowEditing,int nPage)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // exchange data with dialog
   // make a temporary copy of this and have the dialog work on it.
   RatingLibraryEntry tmp(*this);

   CRatingDialog dlg(tmp, allowEditing);
   dlg.SetActivePage(nPage);
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

bool RatingLibraryEntry::SaveMe(WBFL::System::IStructuredSave* pSave)
{
   pSave->BeginUnit(_T("RatingLibraryEntry"), CURRENT_VERSION);

   pSave->Property(_T("Name"), GetName().c_str());
   pSave->Property(_T("Description"), GetDescription(false).c_str());
   pSave->Property(_T("UseCurrentSpecification"), m_bUseCurrentSpecification); // added in version 3
   pSave->Property(_T("SpecificationVersion"), lrfrVersionMgr::GetVersionString(m_SpecificationVersion,true));

   //pSave->Property(_T("AlwaysRate"), m_bAlwaysRate); // removed in version 4

   pSave->BeginUnit(_T("LiveLoadFactors_Design_Inventory"),1.0);
   if ( m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      m_LiveLoadFactorModels[pgsTypes::lrDesign_Inventory].SaveMe(pSave);
   }
   else
   {
      m_LiveLoadFactorModels2[pgsTypes::lrDesign_Inventory].SaveMe(pSave);
   }
   pSave->EndUnit();

   pSave->BeginUnit(_T("LiveLoadFactors_Design_Operating"),1.0);
   if ( m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      m_LiveLoadFactorModels[pgsTypes::lrDesign_Operating].SaveMe(pSave);
   }
   else
   {
      m_LiveLoadFactorModels2[pgsTypes::lrDesign_Operating].SaveMe(pSave);
   }
   pSave->EndUnit();

   pSave->BeginUnit(_T("LiveLoadFactors_Legal_Routine"),1.0);
   if ( m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      m_LiveLoadFactorModels[pgsTypes::lrLegal_Routine].SaveMe(pSave);
   }
   else
   {
      m_LiveLoadFactorModels2[pgsTypes::lrLegal_Routine].SaveMe(pSave);
   }
   pSave->EndUnit();

   pSave->BeginUnit(_T("LiveLoadFactors_Legal_Special"),1.0);
   if ( m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      m_LiveLoadFactorModels[pgsTypes::lrLegal_Special].SaveMe(pSave);
   }
   else
   {
      m_LiveLoadFactorModels2[pgsTypes::lrLegal_Special].SaveMe(pSave);
   }
   pSave->EndUnit();

   // Added in version 2.0
   pSave->BeginUnit(_T("LiveLoadFactors_Legal_Emergency"), 1.0);
   if (m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims)
   {
      m_LiveLoadFactorModels[pgsTypes::lrLegal_Emergency].SaveMe(pSave);
   }
   else
   {
      m_LiveLoadFactorModels2[pgsTypes::lrLegal_Emergency].SaveMe(pSave);
   }
   pSave->EndUnit();

   pSave->BeginUnit(_T("LiveLoadFactors_Permit_Routine"),1.0);
   if ( m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      m_LiveLoadFactorModels[pgsTypes::lrPermit_Routine].SaveMe(pSave);
   }
   else
   {
      m_LiveLoadFactorModels2[pgsTypes::lrPermit_Routine].SaveMe(pSave);
   }
   pSave->EndUnit();

   pSave->BeginUnit(_T("LiveLoadFactors_Permit_SingleTripWithEscort"),1.0);
   if ( m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptSingleTripWithEscort].SaveMe(pSave);
   }
   else
   {
      m_SpecialPermitLiveLoadFactorModels2[pgsTypes::ptSingleTripWithEscort].SaveMe(pSave);
   }
   pSave->EndUnit();

   pSave->BeginUnit(_T("LiveLoadFactors_Permit_SingleTripWithTraffic"),1.0);
   if ( m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptSingleTripWithTraffic].SaveMe(pSave);
   }
   else
   {
      m_SpecialPermitLiveLoadFactorModels2[pgsTypes::ptSingleTripWithTraffic].SaveMe(pSave);
   }
   pSave->EndUnit();

   pSave->BeginUnit(_T("LiveLoadFactors_Permit_MultipleTripWithTraffic"),1.0);
   if ( m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptMultipleTripWithTraffic].SaveMe(pSave);
   }
   else
   {
      m_SpecialPermitLiveLoadFactorModels2[pgsTypes::ptMultipleTripWithTraffic].SaveMe(pSave);
   }
   pSave->EndUnit();

   pSave->EndUnit(); // RatingLibraryEntry

   return true;
}

bool RatingLibraryEntry::LoadMe(WBFL::System::IStructuredLoad* pLoad)
{
   if ( !pLoad->BeginUnit(_T("RatingLibraryEntry")) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   Float64 version = pLoad->GetVersion();
   if (version < 1.0 || CURRENT_VERSION < version)
   {
      THROW_LOAD(BadVersion,pLoad);
   }

   std::_tstring name;
   if(!pLoad->Property(_T("Name"),&name))
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   SetName(name.c_str());

   if(!pLoad->Property(_T("Description"),&name))
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   SetDescription(name.c_str());

   if (2 < version)
   {
      // Added in version 3
      if (!pLoad->Property(_T("UseCurrentSpecification"), &m_bUseCurrentSpecification))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }
   else
   {
      m_bUseCurrentSpecification = false;
   }

   std::_tstring strSpecVersion;
   if( !pLoad->Property(_T("SpecificationVersion"),&strSpecVersion) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   try
   {
      m_SpecificationVersion = lrfrVersionMgr::GetVersion(strSpecVersion.c_str());
   }
   catch(...)
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   // option to always rate after a design was removed in version 4
   bool doRate;
   if (4 > version && !pLoad->Property(_T("AlwaysRate"),&doRate) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if ( !pLoad->BeginUnit(_T("LiveLoadFactors_Design_Inventory")) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if ( m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      if ( !m_LiveLoadFactorModels[pgsTypes::lrDesign_Inventory].LoadMe(pLoad) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }
   else
   {
      if ( !m_LiveLoadFactorModels2[pgsTypes::lrDesign_Inventory].LoadMe(pLoad) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }

   if ( !pLoad->EndUnit() )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }


   if ( !pLoad->BeginUnit(_T("LiveLoadFactors_Design_Operating")) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if ( m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      if ( !m_LiveLoadFactorModels[pgsTypes::lrDesign_Operating].LoadMe(pLoad) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }
   else
   {
      if ( !m_LiveLoadFactorModels2[pgsTypes::lrDesign_Operating].LoadMe(pLoad) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }

   if ( !pLoad->EndUnit() )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if ( !pLoad->BeginUnit(_T("LiveLoadFactors_Legal_Routine")) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if ( m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      if ( !m_LiveLoadFactorModels[pgsTypes::lrLegal_Routine].LoadMe(pLoad) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }
   else
   {
      if ( !m_LiveLoadFactorModels2[pgsTypes::lrLegal_Routine].LoadMe(pLoad) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }

   if ( !pLoad->EndUnit() )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if ( !pLoad->BeginUnit(_T("LiveLoadFactors_Legal_Special")) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if ( m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      if ( !m_LiveLoadFactorModels[pgsTypes::lrLegal_Special].LoadMe(pLoad) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }
   else
   {
      if ( !m_LiveLoadFactorModels2[pgsTypes::lrLegal_Special].LoadMe(pLoad) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }

   if ( !pLoad->EndUnit() )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if (1.0 < version)
   {
      // added in version 2
      if (!pLoad->BeginUnit(_T("LiveLoadFactors_Legal_Emergency")))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      if (m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims)
      {
         if (!m_LiveLoadFactorModels[pgsTypes::lrLegal_Emergency].LoadMe(pLoad))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }
      else
      {
         if (!m_LiveLoadFactorModels2[pgsTypes::lrLegal_Emergency].LoadMe(pLoad))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
      }

      if (!pLoad->EndUnit())
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if ( !pLoad->BeginUnit(_T("LiveLoadFactors_Permit_Routine")) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if ( m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      if ( !m_LiveLoadFactorModels[pgsTypes::lrPermit_Routine].LoadMe(pLoad) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }
   else
   {
      if ( !m_LiveLoadFactorModels2[pgsTypes::lrPermit_Routine].LoadMe(pLoad) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }

   if ( !pLoad->EndUnit() )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if ( !pLoad->BeginUnit(_T("LiveLoadFactors_Permit_SingleTripWithEscort")) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if ( m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      if ( !m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptSingleTripWithEscort].LoadMe(pLoad) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }
   else
   {
      if ( !m_SpecialPermitLiveLoadFactorModels2[pgsTypes::ptSingleTripWithEscort].LoadMe(pLoad) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }

   if ( !pLoad->EndUnit() )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if ( !pLoad->BeginUnit(_T("LiveLoadFactors_Permit_SingleTripWithTraffic")) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if ( m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      if ( !m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptSingleTripWithTraffic].LoadMe(pLoad) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }
   else
   {
      if ( !m_SpecialPermitLiveLoadFactorModels2[pgsTypes::ptSingleTripWithTraffic].LoadMe(pLoad) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }

   if ( !pLoad->EndUnit() )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if ( !pLoad->BeginUnit(_T("LiveLoadFactors_Permit_MultipleTripWithTraffic")) )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if ( m_SpecificationVersion < lrfrVersionMgr::SecondEditionWith2013Interims )
   {
      if ( !m_SpecialPermitLiveLoadFactorModels[pgsTypes::ptMultipleTripWithTraffic].LoadMe(pLoad) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }
   else
   {
      if ( !m_SpecialPermitLiveLoadFactorModels2[pgsTypes::ptMultipleTripWithTraffic].LoadMe(pLoad) )
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
   }

   if ( !pLoad->EndUnit() )
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   if ( !pLoad->EndUnit() )  // RatingLibraryEntry
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   return true;

}

bool RatingLibraryEntry::IsEqual(const RatingLibraryEntry& rOther,bool bConsiderName) const
{
   std::vector<pgsLibraryEntryDifferenceItem*> vDifferences;
   bool bMustRename;
   return Compare(rOther,vDifferences,bMustRename,true,bConsiderName);
}

bool RatingLibraryEntry::Compare(const RatingLibraryEntry& rOther, std::vector<pgsLibraryEntryDifferenceItem*>& vDifferences, bool& bMustRename, bool bReturnOnFirstDifference, bool considerName) const
{
   CEAFApp* pApp = EAFGetApp();
   const WBFL::Units::IndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();

   bMustRename = false;

   if ( m_Description != rOther.m_Description )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Description"),m_Description.c_str(),rOther.m_Description.c_str()));
   }

   if (m_bUseCurrentSpecification != rOther.m_bUseCurrentSpecification || m_SpecificationVersion != rOther.m_SpecificationVersion)
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Rating Criteria Basis"), lrfrVersionMgr::GetVersionString(m_SpecificationVersion), lrfrVersionMgr::GetVersionString(rOther.m_SpecificationVersion)));
   }

   if ( lrfrVersionMgr::GetVersion() < lrfrVersionMgr::SecondEditionWith2013Interims)
   {
      int n = (int)pgsTypes::lrLoadRatingTypeCount;
      for ( int i = 0; i < n; i++ )
      {
         pgsTypes::LoadRatingType ratingType = (pgsTypes::LoadRatingType)i;
         if ( m_LiveLoadFactorModels[ratingType] != rOther.m_LiveLoadFactorModels[ratingType] )
         {
            RETURN_ON_DIFFERENCE;
            CString str;
            str.Format(_T("Live Load Factors are different for %s"),RatingLibraryEntry::GetLoadRatingType(ratingType));
            vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(str,_T(""),_T("")));
         }
      }

      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::SpecialPermitType permitType = (pgsTypes::SpecialPermitType)i;
         if ( m_SpecialPermitLiveLoadFactorModels[permitType] != rOther.m_SpecialPermitLiveLoadFactorModels[permitType] )
         {
            RETURN_ON_DIFFERENCE;
            CString str;
            str.Format(_T("Live Load Factors are different for %s"),RatingLibraryEntry::GetSpecialPermitType(permitType));
            vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(str,_T(""),_T("")));
         }
      }
   }
   else
   {
      int n = (int)pgsTypes::lrLoadRatingTypeCount;
      for ( int i = 0; i < n; i++ )
      {
         pgsTypes::LoadRatingType ratingType = (pgsTypes::LoadRatingType)i;
         if ( m_LiveLoadFactorModels2[ratingType] != rOther.m_LiveLoadFactorModels2[ratingType] )
         {
            RETURN_ON_DIFFERENCE;
            CString str;
            str.Format(_T("Live Load Factors are different for %s"),RatingLibraryEntry::GetLoadRatingType(ratingType));
            vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(str,_T(""),_T("")));
         }
      }

      for ( int i = 0; i < 3; i++ )
      {
         pgsTypes::SpecialPermitType permitType = (pgsTypes::SpecialPermitType)i;
         if ( m_SpecialPermitLiveLoadFactorModels2[permitType] != rOther.m_SpecialPermitLiveLoadFactorModels2[permitType] )
         {
            RETURN_ON_DIFFERENCE;
            CString str;
            str.Format(_T("Live Load Factors are different for %s"),RatingLibraryEntry::GetSpecialPermitType(permitType));
            vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(str,_T(""),_T("")));
         }
      }
   }

   if (considerName &&  GetName() != rOther.GetName() )
   {
      RETURN_ON_DIFFERENCE;
      vDifferences.push_back(new pgsLibraryEntryDifferenceStringItem(_T("Name"),GetName().c_str(),rOther.GetName().c_str()));
   }

   return vDifferences.size() == 0 ? true : false;
}

void RatingLibraryEntry::MakeCopy(const RatingLibraryEntry& rOther)
{
   m_Description          = rOther.m_Description;
   m_bUseCurrentSpecification = rOther.m_bUseCurrentSpecification;
   m_SpecificationVersion = rOther.m_SpecificationVersion;

   m_LiveLoadFactorModels  = rOther.m_LiveLoadFactorModels;
   m_LiveLoadFactorModels2 = rOther.m_LiveLoadFactorModels2;
   m_SpecialPermitLiveLoadFactorModels  = rOther.m_SpecialPermitLiveLoadFactorModels;
   m_SpecialPermitLiveLoadFactorModels2 = rOther.m_SpecialPermitLiveLoadFactorModels2;
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

std::_tstring RatingLibraryEntry::GetDescription(bool bApplySymbolSubstitution) const
{
   if (bApplySymbolSubstitution)
   {
      std::_tstring description(m_Description);
      std::_tstring strSubstitute(lrfrVersionMgr::GetCodeString());
      strSubstitute += _T(", ");
      strSubstitute += lrfrVersionMgr::GetVersionString();
      boost::replace_all(description, _T("%MBE%"), strSubstitute);
      return description;
   }
   else
   {
      return m_Description;
   }
}

void RatingLibraryEntry::UseCurrentSpecification(bool bUseCurrent)
{
   m_bUseCurrentSpecification = bUseCurrent;
}

bool RatingLibraryEntry::UseCurrentSpecification() const
{
   return m_bUseCurrentSpecification;
}

void RatingLibraryEntry::SetSpecificationVersion(lrfrVersionMgr::Version version)
{
   m_SpecificationVersion = version;
}

lrfrVersionMgr::Version RatingLibraryEntry::GetSpecificationVersion() const
{
   if (m_bUseCurrentSpecification)
   {
      return (lrfrVersionMgr::Version)((int)lrfrVersionMgr::LastVersion - 1);
   }
   else
   {
      return m_SpecificationVersion;
   }
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

void RatingLibraryEntry::SetLiveLoadFactorModel2(pgsTypes::LoadRatingType ratingType,const CLiveLoadFactorModel2& model)
{
   ASSERT(ratingType != pgsTypes::lrPermit_Special);
   m_LiveLoadFactorModels2[ratingType] = model;
}

const CLiveLoadFactorModel2& RatingLibraryEntry::GetLiveLoadFactorModel2(pgsTypes::LoadRatingType ratingType) const
{
   ASSERT(ratingType != pgsTypes::lrPermit_Special);
   return m_LiveLoadFactorModels2[ratingType];
}

void RatingLibraryEntry::SetLiveLoadFactorModel2(pgsTypes::SpecialPermitType permitType,const CLiveLoadFactorModel2& model)
{
   m_SpecialPermitLiveLoadFactorModels2[permitType] = model;
}

const CLiveLoadFactorModel2& RatingLibraryEntry::GetLiveLoadFactorModel2(pgsTypes::SpecialPermitType permitType) const
{
   return m_SpecialPermitLiveLoadFactorModels2[permitType];
}
