///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
pgsRatingArtifact::pgsRatingArtifact()
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

void pgsRatingArtifact::AddArtifact(const pgsPointOfInterest& poi,const pgsMomentRatingArtifact& artifact,bool bPositiveMoment)
{
   if ( bPositiveMoment )
   {
      m_PositiveMomentRatings.push_back( std::make_pair(poi,artifact) );
   }
   else
   {
      m_NegativeMomentRatings.push_back( std::make_pair(poi,artifact) );
   }
}

void pgsRatingArtifact::AddArtifact(const pgsPointOfInterest& poi,const pgsShearRatingArtifact& artifact)
{
   m_ShearRatings.push_back( std::make_pair(poi,artifact) );
}

void pgsRatingArtifact::AddArtifact(const pgsPointOfInterest& poi,const pgsStressRatingArtifact& artifact)
{
   m_StressRatings.push_back( std::make_pair(poi,artifact) );
}

void pgsRatingArtifact::AddArtifact(const pgsPointOfInterest& poi,const pgsYieldStressRatioArtifact& artifact,bool bPositiveMoment)
{
   if ( bPositiveMoment )
   {
      m_PositiveMomentYieldStressRatios.push_back( std::make_pair(poi,artifact) );
   }
   else
   {
      m_NegativeMomentYieldStressRatios.push_back( std::make_pair(poi,artifact) );
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
   (*ppArtifact) = NULL;

   MomentRatings::const_iterator iter;
   const MomentRatings* pRatings = (bPositiveMoment ? &m_PositiveMomentRatings : &m_NegativeMomentRatings);

   for ( iter = pRatings->begin(); iter != pRatings->end(); iter++ )
   {
      const pgsMomentRatingArtifact& artifact = iter->second;
      Float64 rating_factor = artifact.GetRatingFactor();
      if ( rating_factor < RF )
      {
         RF = rating_factor;
         (*ppArtifact) = &artifact;
      }
   }

   if ( *ppArtifact == NULL && 0 < pRatings->size() )
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
   (*ppArtifact) = NULL;

   ShearRatings::const_iterator iter;
   for ( iter = m_ShearRatings.begin(); iter != m_ShearRatings.end(); iter++ )
   {
      const pgsShearRatingArtifact& artifact = iter->second;
      Float64 rating_factor = artifact.GetRatingFactor();
      if ( rating_factor < RF )
      {
         RF = rating_factor;
         *ppArtifact = &artifact;
      }
   }

   if ( *ppArtifact == NULL && 0 < m_ShearRatings.size() )
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
   (*ppArtifact) = NULL;

   StressRatings::const_iterator iter;
   for ( iter = m_StressRatings.begin(); iter != m_StressRatings.end(); iter++ )
   {
      const pgsStressRatingArtifact& artifact = iter->second;
      Float64 rating_factor = artifact.GetRatingFactor();
      if ( rating_factor < RF )
      {
         RF = rating_factor;
         *ppArtifact = &artifact;
      }
   }

   if ( *ppArtifact == NULL && 0 < m_StressRatings.size() )
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
   (*ppArtifact) = NULL;

   const YieldStressRatios* pRatios = (bPositiveMoment ? &m_PositiveMomentYieldStressRatios : &m_NegativeMomentYieldStressRatios);
   YieldStressRatios::const_iterator iter;
   for ( iter = pRatios->begin(); iter != pRatios->end(); iter++ )
   {
      const pgsYieldStressRatioArtifact& artifact = iter->second;
      Float64 ratio = artifact.GetStressRatio();
      if ( ratio < RF )
      {
         RF = ratio;
         *ppArtifact = &artifact;
      }
   }

   if ( *ppArtifact == NULL && 0 < pRatios->size() )
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

   // NULL out all but the controlling rating
   if ( i == 0 )
   {
      //(*ppPositiveMoment)            = NULL;
      (*ppNegativeMoment)            = NULL;
      (*ppShear)                     = NULL;
      (*ppStress)                    = NULL;
      (*ppYieldStressPositiveMoment) = NULL;
      (*ppYieldStressNegativeMoment) = NULL;
   }
   else if ( i == 1 )
   {
      (*ppPositiveMoment)            = NULL;
      //(*ppNegativeMoment)            = NULL;
      (*ppShear)                     = NULL;
      (*ppStress)                    = NULL;
      (*ppYieldStressPositiveMoment) = NULL;
      (*ppYieldStressNegativeMoment) = NULL;
   }
   else if ( i == 2 )
   {
      (*ppPositiveMoment)            = NULL;
      (*ppNegativeMoment)            = NULL;
      //(*ppShear)                     = NULL;
      (*ppStress)                    = NULL;
      (*ppYieldStressPositiveMoment) = NULL;
      (*ppYieldStressNegativeMoment) = NULL;
   }
   else if ( i == 3 )
   {
      (*ppPositiveMoment)            = NULL;
      (*ppNegativeMoment)            = NULL;
      (*ppShear)                     = NULL;
      //(*ppStress)                    = NULL;
      (*ppYieldStressPositiveMoment) = NULL;
      (*ppYieldStressNegativeMoment) = NULL;
   }
   else if ( i == 4 )
   {
      (*ppPositiveMoment)            = NULL;
      (*ppNegativeMoment)            = NULL;
      (*ppShear)                     = NULL;
      (*ppStress)                    = NULL;
      //(*ppYieldStressPositiveMoment) = NULL;
      (*ppYieldStressNegativeMoment) = NULL;
   }
   else if ( i == 5 )
   {
      (*ppPositiveMoment)            = NULL;
      (*ppNegativeMoment)            = NULL;
      (*ppShear)                     = NULL;
      (*ppStress)                    = NULL;
      (*ppYieldStressPositiveMoment) = NULL;
      //(*ppYieldStressNegativeMoment) = NULL;
   }
   else
   {
      // ??? rating wasn't done?
      // this can happen if we are rating with a negative moment only truck
      // but the bridge is simple span... we run the truck because we want
      // reactions, but the reating factors are DBL_MAX...
      // since all types of ratings control equally, use positive moment as controlling factor
      ATLASSERT(i == -1);
      //(*ppPositiveMoment)            = NULL;
      (*ppNegativeMoment)            = NULL;
      (*ppShear)                     = NULL;
      (*ppStress)                    = NULL;
      (*ppYieldStressPositiveMoment) = NULL;
      (*ppYieldStressNegativeMoment) = NULL;
   }

   return RF;
}

void pgsRatingArtifact::GetSafePostingLoad(Float64* pPostingLoad,Float64* pWeight,Float64* pRF,std::_tstring* pVehicle) const
{
   Float64 posting_load = DBL_MAX;
   MomentRatings::const_iterator moment_iter;
   for ( moment_iter = m_PositiveMomentRatings.begin(); moment_iter != m_PositiveMomentRatings.end(); moment_iter++ )
   {
      const pgsMomentRatingArtifact& artifact = moment_iter->second;

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

   for ( moment_iter = m_NegativeMomentRatings.begin(); moment_iter != m_NegativeMomentRatings.end(); moment_iter++ )
   {
      const pgsMomentRatingArtifact& artifact = moment_iter->second;

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

   ShearRatings::const_iterator shear_iter;
   for ( shear_iter = m_ShearRatings.begin(); shear_iter != m_ShearRatings.end(); shear_iter++ )
   {
      const pgsShearRatingArtifact& artifact = shear_iter->second;

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

   StressRatings::const_iterator stress_iter;
   for ( stress_iter = m_StressRatings.begin(); stress_iter != m_StressRatings.end(); stress_iter++ )
   {
      const pgsStressRatingArtifact& artifact = stress_iter->second;

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

   YieldStressRatios::const_iterator yield_stress_iter;
   for ( yield_stress_iter = m_PositiveMomentYieldStressRatios.begin(); yield_stress_iter != m_PositiveMomentYieldStressRatios.end(); yield_stress_iter++ )
   {
      const pgsYieldStressRatioArtifact& artifact = yield_stress_iter->second;

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

   for ( yield_stress_iter = m_NegativeMomentYieldStressRatios.begin(); yield_stress_iter != m_NegativeMomentYieldStressRatios.end(); yield_stress_iter++ )
   {
      const pgsYieldStressRatioArtifact& artifact = yield_stress_iter->second;

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
