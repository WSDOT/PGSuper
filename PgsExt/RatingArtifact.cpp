///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include <PgsExt\RatingArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsRatingCapacityArtifact
****************************************************************************/
pgsRatingArtifact::pgsRatingArtifact(pgsTypes::LoadRatingType ratingType) :
   m_RatingType(ratingType)
{
}

pgsRatingArtifact::pgsRatingArtifact(const pgsRatingArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsRatingArtifact::~pgsRatingArtifact()
{
}

pgsRatingArtifact& pgsRatingArtifact::operator=(const pgsRatingArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

pgsTypes::LoadRatingType pgsRatingArtifact::GetLoadRatingType() const
{
   return m_RatingType;
}

void pgsRatingArtifact::AddArtifact(const pgsPointOfInterest& poi,const pgsMomentRatingArtifact& artifact,bool bPositiveMoment)
{
   if ( bPositiveMoment )
   {
      m_PositiveMomentRatings.emplace_back(poi,artifact);
   }
   else
   {
      m_NegativeMomentRatings.emplace_back(poi,artifact);
   }
}

void pgsRatingArtifact::AddArtifact(const pgsPointOfInterest& poi,const pgsShearRatingArtifact& artifact)
{
   m_ShearRatings.emplace_back(poi,artifact);
}

void pgsRatingArtifact::AddArtifact(const pgsPointOfInterest& poi,const pgsStressRatingArtifact& artifact)
{
   m_StressRatings.emplace_back(poi,artifact);
}

void pgsRatingArtifact::AddArtifact(const pgsPointOfInterest& poi,const pgsYieldStressRatioArtifact& artifact,bool bPositiveMoment)
{
   if ( bPositiveMoment )
   {
      m_PositiveMomentYieldStressRatios.emplace_back(poi,artifact);
   }
   else
   {
      m_NegativeMomentYieldStressRatios.emplace_back(poi,artifact);
   }
}

const pgsRatingArtifact::MomentRatings& pgsRatingArtifact::GetMomentRatings(bool bPositiveMoment) const
{
   return (bPositiveMoment ? m_PositiveMomentRatings : m_NegativeMomentRatings);
}

const pgsRatingArtifact::ShearRatings& pgsRatingArtifact::GetShearRatings() const
{
   return m_ShearRatings;
}

const pgsRatingArtifact::StressRatings& pgsRatingArtifact::GetStressRatings() const
{
   return m_StressRatings;
}

const pgsRatingArtifact::YieldStressRatios& pgsRatingArtifact::GetYieldStressRatios(bool bPositiveMoment) const
{
   return (bPositiveMoment ? m_PositiveMomentYieldStressRatios : m_NegativeMomentYieldStressRatios);
}

Float64 pgsRatingArtifact::GetMomentRatingFactorEx(bool bPositiveMoment,const pgsMomentRatingArtifact** ppArtifact) const
{
   Float64 RF = DBL_MAX;
   (*ppArtifact) = nullptr;

   MomentRatings::const_iterator iter;
   const MomentRatings* pRatings = (bPositiveMoment ? &m_PositiveMomentRatings : &m_NegativeMomentRatings);

   for ( const auto& item : *pRatings)
   {
      const auto& artifact(item.second);
      Float64 rating_factor = artifact.GetRatingFactor();
      if ( rating_factor < RF )
      {
         RF = rating_factor;
         (*ppArtifact) = &artifact;
      }
   }

   if ( *ppArtifact == nullptr && 0 < pRatings->size() )
   {
      ATLASSERT(RF == DBL_MAX);
      (*ppArtifact) = &(pRatings->front().second);
   }

   return RF;
}

Float64 pgsRatingArtifact::GetMomentRatingFactor(bool bPositiveMoment) const
{
   const pgsMomentRatingArtifact* pArtifact;
   return GetMomentRatingFactorEx(bPositiveMoment,&pArtifact);
}

Float64 pgsRatingArtifact::GetShearRatingFactorEx(const pgsShearRatingArtifact** ppArtifact) const
{
   Float64 RF = DBL_MAX;
   (*ppArtifact) = nullptr;

   for ( const auto& item : m_ShearRatings)
   {
      const auto& artifact(item.second);
      Float64 rating_factor = artifact.GetRatingFactor();
      if ( rating_factor < RF )
      {
         RF = rating_factor;
         *ppArtifact = &artifact;
      }
   }

   if ( *ppArtifact == nullptr && 0 < m_ShearRatings.size() )
   {
      ATLASSERT(RF == DBL_MAX);
      (*ppArtifact) = &(m_ShearRatings.front().second);
   }

   return RF;
}

Float64 pgsRatingArtifact::GetShearRatingFactor() const
{
   const pgsShearRatingArtifact* pArtifact;
   return GetShearRatingFactorEx(&pArtifact);
}

Float64 pgsRatingArtifact::GetStressRatingFactorEx(const pgsStressRatingArtifact** ppArtifact) const
{
   Float64 RF = DBL_MAX;
   (*ppArtifact) = nullptr;

   for ( const auto& item : m_StressRatings)
   {
      const auto& artifact(item.second);
      Float64 rating_factor = artifact.GetRatingFactor();
      if ( rating_factor < RF )
      {
         RF = rating_factor;
         *ppArtifact = &artifact;
      }
   }

   if ( *ppArtifact == nullptr && 0 < m_StressRatings.size() )
   {
      ATLASSERT(RF == DBL_MAX);
      (*ppArtifact) = &(m_StressRatings.front().second);
   }

   return RF;
}

Float64 pgsRatingArtifact::GetStressRatingFactor() const
{
   const pgsStressRatingArtifact* pArtifact;
   return GetStressRatingFactorEx(&pArtifact);
}

Float64 pgsRatingArtifact::GetYieldStressRatioEx(bool bPositiveMoment,const pgsYieldStressRatioArtifact** ppArtifact) const
{
   Float64 RF = DBL_MAX;
   (*ppArtifact) = nullptr;

   const YieldStressRatios* pRatios = (bPositiveMoment ? &m_PositiveMomentYieldStressRatios : &m_NegativeMomentYieldStressRatios);
   for( const auto& item : *pRatios)
   {
      const auto& artifact(item.second);
      Float64 ratio = artifact.GetStressRatio();
      if ( ratio < RF )
      {
         RF = ratio;
         *ppArtifact = &artifact;
      }
   }

   if ( *ppArtifact == nullptr && 0 < pRatios->size() )
   {
      ATLASSERT(RF == DBL_MAX);
      (*ppArtifact) = &(pRatios->front().second);
   }

   return RF;
}

Float64 pgsRatingArtifact::GetYieldStressRatio(bool bPositiveMoment) const
{
   const pgsYieldStressRatioArtifact* pArtifact;
   return GetYieldStressRatioEx(bPositiveMoment,&pArtifact);
}

Float64 pgsRatingArtifact::GetRatingFactor() const
{
   const pgsMomentRatingArtifact* pPositiveMoment;
   const pgsMomentRatingArtifact* pNegativeMoment;
   const pgsShearRatingArtifact* pShear;
   const pgsStressRatingArtifact* pStress;
   const pgsYieldStressRatioArtifact* pYieldStressPositiveMoment;
   const pgsYieldStressRatioArtifact* pYieldStressNegativeMoment;

   return GetRatingFactorEx(&pPositiveMoment,&pNegativeMoment,&pShear,&pStress,&pYieldStressPositiveMoment,&pYieldStressNegativeMoment);
}

Float64 pgsRatingArtifact::GetRatingFactorEx(const pgsMomentRatingArtifact** ppPositiveMoment,const pgsMomentRatingArtifact** ppNegativeMoment,
                                             const pgsShearRatingArtifact** ppShear,
                                             const pgsStressRatingArtifact** ppStress,
                                             const pgsYieldStressRatioArtifact** ppYieldStressPositiveMoment,const pgsYieldStressRatioArtifact** ppYieldStressNegativeMoment) const
{
   Float64 RF_pM = GetMomentRatingFactorEx(true,ppPositiveMoment);
   Float64 RF_nM = GetMomentRatingFactorEx(false,ppNegativeMoment);
   Float64 RF_V  = GetShearRatingFactorEx(ppShear);
   Float64 RF_f  = GetStressRatingFactorEx(ppStress);
   Float64 RF_ys_pM = GetYieldStressRatioEx(true,ppYieldStressPositiveMoment);
   Float64 RF_ys_nM = GetYieldStressRatioEx(false,ppYieldStressNegativeMoment);

   Float64 RF = DBL_MAX;
   int i = -1; // initialize to an invalid value so that we know if a rating factor wasn't found
   if ( RF_pM < RF )
   {
      RF = RF_pM;
      i = 0;
   }

   if ( RF_nM < RF )
   {
      RF = RF_nM;
      i = 1;
   }

   if ( RF_V < RF )
   {
      RF = RF_V;
      i = 2;
   }

   if ( RF_f < RF )
   {
      RF = RF_f;
      i = 3;
   }

   if ( RF_ys_pM < RF )
   {
      RF = RF_ys_pM;
      i = 4;
   }

   if ( RF_ys_nM < RF )
   {
      RF = RF_ys_nM;
      i = 5;
   }

   // nullptr out all but the controlling rating
   if ( i == 0 )
   {
      //(*ppPositiveMoment)            = nullptr;
      (*ppNegativeMoment)            = nullptr;
      (*ppShear)                     = nullptr;
      (*ppStress)                    = nullptr;
      (*ppYieldStressPositiveMoment) = nullptr;
      (*ppYieldStressNegativeMoment) = nullptr;
   }
   else if ( i == 1 )
   {
      (*ppPositiveMoment)            = nullptr;
      //(*ppNegativeMoment)            = nullptr;
      (*ppShear)                     = nullptr;
      (*ppStress)                    = nullptr;
      (*ppYieldStressPositiveMoment) = nullptr;
      (*ppYieldStressNegativeMoment) = nullptr;
   }
   else if ( i == 2 )
   {
      (*ppPositiveMoment)            = nullptr;
      (*ppNegativeMoment)            = nullptr;
      //(*ppShear)                     = nullptr;
      (*ppStress)                    = nullptr;
      (*ppYieldStressPositiveMoment) = nullptr;
      (*ppYieldStressNegativeMoment) = nullptr;
   }
   else if ( i == 3 )
   {
      (*ppPositiveMoment)            = nullptr;
      (*ppNegativeMoment)            = nullptr;
      (*ppShear)                     = nullptr;
      //(*ppStress)                    = nullptr;
      (*ppYieldStressPositiveMoment) = nullptr;
      (*ppYieldStressNegativeMoment) = nullptr;
   }
   else if ( i == 4 )
   {
      (*ppPositiveMoment)            = nullptr;
      (*ppNegativeMoment)            = nullptr;
      (*ppShear)                     = nullptr;
      (*ppStress)                    = nullptr;
      //(*ppYieldStressPositiveMoment) = nullptr;
      (*ppYieldStressNegativeMoment) = nullptr;
   }
   else if ( i == 5 )
   {
      (*ppPositiveMoment)            = nullptr;
      (*ppNegativeMoment)            = nullptr;
      (*ppShear)                     = nullptr;
      (*ppStress)                    = nullptr;
      (*ppYieldStressPositiveMoment) = nullptr;
      //(*ppYieldStressNegativeMoment) = nullptr;
   }
   else
   {
      // ??? rating wasn't done?
      // this can happen if we are rating with a negative moment only truck
      // but the bridge is simple span... we run the truck because we want
      // reactions, but the reating factors are DBL_MAX...
      // since all types of ratings control equally, use positive moment as controlling factor
      ATLASSERT(i == -1);
      //(*ppPositiveMoment)            = nullptr;
      (*ppNegativeMoment)            = nullptr;
      (*ppShear)                     = nullptr;
      (*ppStress)                    = nullptr;
      (*ppYieldStressPositiveMoment) = nullptr;
      (*ppYieldStressNegativeMoment) = nullptr;
   }

   return RF;
}

bool pgsRatingArtifact::IsLoadPostingRequired() const
{
   return (::IsLegalRatingType(m_RatingType) && !::IsEmergencyRatingType(m_RatingType) && GetRatingFactor() < 1.0 ? true : false);
}

void pgsRatingArtifact::GetSafePostingLoad(Float64* pPostingLoad,Float64* pWeight,Float64* pRF,std::_tstring* pVehicle) const
{
   Float64 posting_load = DBL_MAX;
   for (const auto& item : m_PositiveMomentRatings)
   {
      const auto& artifact(item.second);

      std::_tstring strVehicle = artifact.GetVehicleName();
      Float64 W = artifact.GetVehicleWeight();
      Float64 RF = artifact.GetRatingFactor();
      Float64 spl = W*(RF - 0.3)/0.7; // safe posting load MBE Eq 6A.8.3-1
      if ( spl < 0 )
      {
         spl = 0;
      }

      if ( spl < posting_load )
      {
         posting_load = spl;
         *pWeight = W;
         *pRF = RF;
         *pVehicle = strVehicle;
      }
   }

   for (const auto& item : m_NegativeMomentRatings)
   {
      const auto& artifact(item.second);

      std::_tstring strVehicle = artifact.GetVehicleName();
      Float64 W = artifact.GetVehicleWeight();
      Float64 RF = artifact.GetRatingFactor();
      Float64 spl = W*(RF - 0.3)/0.7;
      if ( spl < 0 )
      {
         spl = 0;
      }

      if ( spl < posting_load )
      {
         posting_load = spl;
         *pWeight = W;
         *pRF = RF;
         *pVehicle = strVehicle;
      }
   }

   for ( const auto& item : m_ShearRatings)
   {
      const auto& artifact(item.second);

      std::_tstring strVehicle = artifact.GetVehicleName();
      Float64 W = artifact.GetVehicleWeight();
      Float64 RF = artifact.GetRatingFactor();
      Float64 spl = W*(RF - 0.3)/0.7;
      if ( spl < 0 )
      {
         spl = 0;
      }

      if ( spl < posting_load )
      {
         posting_load = spl;
         *pWeight = W;
         *pRF = RF;
         *pVehicle = strVehicle;
      }
   }

   for(const auto& item : m_StressRatings)
   {
      const auto& artifact(item.second);

      std::_tstring strVehicle = artifact.GetVehicleName();
      Float64 W = artifact.GetVehicleWeight();
      Float64 RF = artifact.GetRatingFactor();
      Float64 spl = W*(RF - 0.3)/0.7;
      if ( spl < 0 )
      {
         spl = 0;
      }

      if ( spl < posting_load )
      {
         posting_load = spl;
         *pWeight = W;
         *pRF = RF;
         *pVehicle = strVehicle;
      }
   }

   for ( const auto& item : m_PositiveMomentYieldStressRatios)
   {
      const auto& artifact(item.second);

      std::_tstring strVehicle = artifact.GetVehicleName();
      Float64 W = artifact.GetVehicleWeight();
      Float64 RF = artifact.GetStressRatio();
      Float64 spl = W*(RF - 0.3)/0.7;
      if ( spl < 0 )
      {
         spl = 0;
      }

      if ( spl < posting_load )
      {
         posting_load = spl;
         *pWeight = W;
         *pRF = RF;
         *pVehicle = strVehicle;
      }
   }

   for (const auto& item : m_NegativeMomentYieldStressRatios)
   {
      const auto& artifact(item.second);

      std::_tstring strVehicle = artifact.GetVehicleName();
      Float64 W = artifact.GetVehicleWeight();
      Float64 RF = artifact.GetStressRatio();
      Float64 spl = W*(RF - 0.3)/0.7;
      if ( spl < 0 )
      {
         spl = 0;
      }

      if ( spl < posting_load )
      {
         posting_load = spl;
         *pWeight = W;
         *pRF = RF;
         *pVehicle = strVehicle;
      }
   }

   *pPostingLoad = posting_load;
}

void pgsRatingArtifact::MakeCopy(const pgsRatingArtifact& rOther)
{
   m_RatingType = rOther.m_RatingType;
   m_PositiveMomentRatings = rOther.m_PositiveMomentRatings;
   m_NegativeMomentRatings = rOther.m_NegativeMomentRatings;
   m_ShearRatings  = rOther.m_ShearRatings;
   m_StressRatings = rOther.m_StressRatings;
   m_PositiveMomentYieldStressRatios = rOther.m_PositiveMomentYieldStressRatios;
   m_NegativeMomentYieldStressRatios = rOther.m_NegativeMomentYieldStressRatios;
}

void pgsRatingArtifact::MakeAssignment(const pgsRatingArtifact& rOther)
{
   MakeCopy( rOther );
}
