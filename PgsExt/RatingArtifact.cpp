///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#if defined _USE_MULTITHREADING
#include <future>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Gets the controlling rating factor from a collection of rating factor artifacts
//
// NOTE: Some artifact collections can be the same length (e.g. all of the flexure ones... shear is a different length)
// The LoadRater makes them the same length, this object has no guarentee that will always be true
// If there are collections that are the same length, it would be more efficient to loop over all of them at one time
// rather than loop over each one by itself as done here
template <class C,class T>
Float64 GetControllingRatingFactor(const C& artifacts, const T** ppControllingArtifact)
{
   Float64 RF = DBL_MAX;
   (*ppControllingArtifact) = nullptr;

   for (const auto& item : artifacts)
   {
      const auto& artifact(item.second);
      Float64 rf = artifact.GetRatingFactor();
      if (rf < RF)
      {
         RF = rf;
         *ppControllingArtifact = &artifact;
      }
   }

   if (*ppControllingArtifact == nullptr && 0 < artifacts.size())
   {
      // a controlling rating wasn't found (all ratings are DBL_MAX)
      // so just use the first artifact as the controlling one
      ATLASSERT(RF == DBL_MAX);
      (*ppControllingArtifact) = &(artifacts.front().second);
   }

   return RF;
}

template <class T>
void ResetCache(bool* pbCacheFlag, Float64* pCachedRF, T* pCachedArtifact)
{
   *pbCacheFlag = false;
   *pCachedRF = DBL_MAX;
   *pCachedArtifact = nullptr;
}

struct PostingLoad
{
   Float64 Load;
   Float64 RF;
   Float64 VehicleWeight;
   std::_tstring VehicleName;
   PostingLoad() : Load(DBL_MAX), RF(1.0), VehicleWeight(0.0), VehicleName(_T("Unknown"))
   {
   }
};

template<class T>
PostingLoad ComputeSafePostingLoad(const T& ratingArtifacts)
{
   PostingLoad postingLoad;

   for (const auto& item : ratingArtifacts)
   {
      const auto& artifact = item.second;
      const auto& strVehicle = artifact.GetVehicleName();
      auto W = artifact.GetVehicleWeight();
      auto RF = artifact.GetRatingFactor();
      Float64 spl = W*(RF - 0.3) / 0.7; // safe posting load, MBE Equation 6A.8.3-1
      if (spl < 0)
      {
         spl = 0;
      }

      if (spl < postingLoad.Load)
      {
         postingLoad.Load = spl;
         postingLoad.RF = RF;
         postingLoad.VehicleWeight = W;
         postingLoad.VehicleName = strVehicle;
      }
   }

   return postingLoad;
}

/****************************************************************************
CLASS
   pgsRatingCapacityArtifact
****************************************************************************/
pgsRatingArtifact::pgsRatingArtifact(pgsTypes::LoadRatingType ratingType) :
   m_RatingType(ratingType)
{
   ResetCache(&m_bPositiveMomentRatingCached,&m_RF_PositiveMoment,&m_pControllingPositiveMoment);
   ResetCache(&m_bNegativeMomentRatingCached,&m_RF_NegativeMoment,&m_pControllingNegativeMoment);
   ResetCache(&m_bShearRatingCached,&m_RF_Shear,&m_pControllingShear);
   ResetCache(&m_bStressRatingCached, &m_RF_Stress, &m_pControllingStress);
   ResetCache(&m_bPositiveYieldStressRatingCached,&m_RF_PositiveMomentYieldStress,&m_pControllingPositiveMomentYieldStress);
   ResetCache(&m_bNegativeYieldStressRatingCached, &m_RF_NegativeMomentYieldStress, &m_pControllingNegativeMomentYieldStress);
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
      ResetCache(&m_bPositiveMomentRatingCached, &m_RF_PositiveMoment, &m_pControllingPositiveMoment);
   }
   else
   {
      m_NegativeMomentRatings.emplace_back(poi,artifact);
      ResetCache(&m_bNegativeMomentRatingCached, &m_RF_NegativeMoment, &m_pControllingNegativeMoment);
   }
}

void pgsRatingArtifact::AddArtifact(const pgsPointOfInterest& poi,const pgsShearRatingArtifact& artifact)
{
   m_ShearRatings.emplace_back(poi,artifact);
   ResetCache(&m_bShearRatingCached, &m_RF_Shear, &m_pControllingShear);
}

void pgsRatingArtifact::AddArtifact(const pgsPointOfInterest& poi,const pgsStressRatingArtifact& artifact)
{
   m_StressRatings.emplace_back(poi,artifact);
   ResetCache(&m_bStressRatingCached, &m_RF_Stress, &m_pControllingStress);
}

void pgsRatingArtifact::AddArtifact(const pgsPointOfInterest& poi,const pgsYieldStressRatioArtifact& artifact,bool bPositiveMoment)
{
   if ( bPositiveMoment )
   {
      m_PositiveMomentYieldStressRatios.emplace_back(poi,artifact);
      ResetCache(&m_bPositiveYieldStressRatingCached, &m_RF_PositiveMomentYieldStress, &m_pControllingPositiveMomentYieldStress);
   }
   else
   {
      m_NegativeMomentYieldStressRatios.emplace_back(poi,artifact);
      ResetCache(&m_bNegativeYieldStressRatingCached, &m_RF_NegativeMomentYieldStress, &m_pControllingNegativeMomentYieldStress);
   }
}

void pgsRatingArtifact::AddArtifact(const pgsPointOfInterest& poi, const pgsLongReinfShearArtifact& artifact)
{
   m_LongitudinalReinforcementForShear.emplace_back(poi, artifact);
}

const pgsMomentRatingArtifact* pgsRatingArtifact::GetMomentRatingArtifact(const pgsPointOfInterest& poi, bool bPositiveMoment) const
{
   auto& ratings = (bPositiveMoment ? m_PositiveMomentRatings : m_NegativeMomentRatings);
   auto found = std::find_if(ratings.begin(), ratings.end(), [&poi](const auto& pair) {return pair.first == poi;});
   if (found == ratings.end())
   {
      return nullptr;
   }
   return &(found->second);
}

const pgsShearRatingArtifact* pgsRatingArtifact::GetShearRatingArtifact(const pgsPointOfInterest& poi) const
{
   auto found = std::find_if(m_ShearRatings.begin(), m_ShearRatings.end(), [&poi](const auto& pair) {return poi == pair.first;});
   if (found == m_ShearRatings.end())
   {
      return nullptr;
   }
   return &(found->second);
}

const pgsStressRatingArtifact* pgsRatingArtifact::GetStressRatingArtifact(const pgsPointOfInterest& poi) const
{
   auto found = std::find_if(m_StressRatings.begin(), m_StressRatings.end(), [&poi](const auto& pair) {return poi == pair.first;});
   if (found == m_StressRatings.end())
   {
      return nullptr;
   }
   return &(found->second);
}

const pgsYieldStressRatioArtifact* pgsRatingArtifact::GetYieldStressRatioArtifact(const pgsPointOfInterest& poi, bool bPositiveMoment) const
{
   auto& ratings = (bPositiveMoment ? m_PositiveMomentYieldStressRatios : m_NegativeMomentYieldStressRatios);
   auto found = std::find_if(ratings.begin(), ratings.end(), [&poi](const auto& pair) {return poi == pair.first;});
   if (found == ratings.end())
   {
      return nullptr;
   }
   return &(found->second);
}

const pgsLongReinfShearArtifact* pgsRatingArtifact::GetLongitudinalReinforcementForShearArtifact(const pgsPointOfInterest& poi) const
{
   auto found = std::find_if(m_LongitudinalReinforcementForShear.begin(), m_LongitudinalReinforcementForShear.end(), [&poi](const auto& pair) {return poi == pair.first; });
   if (found == m_LongitudinalReinforcementForShear.end())
   {
      return nullptr;
   }
   return &(found->second);
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

const pgsRatingArtifact::LongitudinalReinforcementForShear& pgsRatingArtifact::GetLongitudinalReinforcementForShear() const
{
   return m_LongitudinalReinforcementForShear;
}

Float64 pgsRatingArtifact::GetMomentRatingFactorEx(bool bPositiveMoment,const pgsMomentRatingArtifact** ppArtifact) const
{
   if ( (bPositiveMoment && m_bPositiveMomentRatingCached) ||
        (!bPositiveMoment && m_bNegativeMomentRatingCached) )
   {
      *ppArtifact = (bPositiveMoment ? m_pControllingPositiveMoment : m_pControllingNegativeMoment);
      return (bPositiveMoment ? m_RF_PositiveMoment : m_RF_NegativeMoment);
   }

   const MomentRatings* pRatings = (bPositiveMoment ? &m_PositiveMomentRatings : &m_NegativeMomentRatings);
   Float64 rf = GetControllingRatingFactor(*pRatings, ppArtifact);
   if (bPositiveMoment)
   {
      m_RF_PositiveMoment = rf;
      m_pControllingPositiveMoment = *ppArtifact;
      m_bPositiveMomentRatingCached = true;
   }
   else
   {
      m_RF_NegativeMoment = rf;
      m_pControllingNegativeMoment = *ppArtifact;
      m_bNegativeMomentRatingCached = true;
   }
   return rf;
}

Float64 pgsRatingArtifact::GetMomentRatingFactor(bool bPositiveMoment) const
{
   const pgsMomentRatingArtifact* pArtifact;
   return GetMomentRatingFactorEx(bPositiveMoment,&pArtifact);
}

Float64 pgsRatingArtifact::GetShearRatingFactorEx(const pgsShearRatingArtifact** ppArtifact) const
{
   if (m_bShearRatingCached)
   {
      *ppArtifact = m_pControllingShear;
      return m_RF_Shear;
   }

   Float64 rf = GetControllingRatingFactor(m_ShearRatings, ppArtifact);
   m_RF_Shear = rf;
   m_pControllingShear = *ppArtifact;
   m_bShearRatingCached = true;
   return rf;
}

Float64 pgsRatingArtifact::GetShearRatingFactor() const
{
   const pgsShearRatingArtifact* pArtifact;
   return GetShearRatingFactorEx(&pArtifact);
}

Float64 pgsRatingArtifact::GetStressRatingFactorEx(const pgsStressRatingArtifact** ppArtifact) const
{
   if (m_bStressRatingCached)
   {
      *ppArtifact = m_pControllingStress;
      return m_RF_Stress;
   }

   Float64 rf = GetControllingRatingFactor(m_StressRatings, ppArtifact);
   m_RF_Stress = rf;
   m_pControllingStress = *ppArtifact;
   m_bStressRatingCached = true;
   return rf;
}

Float64 pgsRatingArtifact::GetStressRatingFactor() const
{
   const pgsStressRatingArtifact* pArtifact;
   return GetStressRatingFactorEx(&pArtifact);
}

Float64 pgsRatingArtifact::GetYieldStressRatioEx(bool bPositiveMoment, const pgsYieldStressRatioArtifact** ppArtifact) const
{
   if ((bPositiveMoment && m_bPositiveYieldStressRatingCached) ||
      (!bPositiveMoment && m_bNegativeYieldStressRatingCached))
   {
      *ppArtifact = (bPositiveMoment ? m_pControllingPositiveMomentYieldStress : m_pControllingNegativeMomentYieldStress);
      return (bPositiveMoment ? m_RF_PositiveMomentYieldStress : m_RF_NegativeMomentYieldStress);
   }

   const YieldStressRatios* pRatios = (bPositiveMoment ? &m_PositiveMomentYieldStressRatios : &m_NegativeMomentYieldStressRatios);
   Float64 rf = GetControllingRatingFactor(*pRatios, ppArtifact);
   if (bPositiveMoment)
   {
      m_RF_PositiveMomentYieldStress = rf;
      m_pControllingPositiveMomentYieldStress = *ppArtifact;
      m_bPositiveYieldStressRatingCached = true;
   }
   else
   {
      m_RF_NegativeMomentYieldStress = rf;
      m_pControllingNegativeMomentYieldStress = *ppArtifact;
      m_bNegativeMomentRatingCached = true;
   }
   return rf;
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
   // Make sure all individual rating factor types are computed and cached
#if defined _USE_MULTITHREADING
   std::vector<std::future<Float64>> vFutures;
   if (!m_bPositiveMomentRatingCached)
   {
      std::future<Float64> f(std::async([&,this]{return GetMomentRatingFactorEx(true, ppPositiveMoment);}));
      vFutures.push_back(std::move(f));
   }

   if (!m_bNegativeMomentRatingCached)
   {
      std::future<Float64> f(std::async([&,this] {return GetMomentRatingFactorEx(false, ppNegativeMoment);}));
      vFutures.push_back(std::move(f));
   }

   if (!m_bShearRatingCached)
   {
      std::future<Float64> f(std::async([&,this] {return GetShearRatingFactorEx(ppShear);}));
      vFutures.push_back(std::move(f));
   }

   if (!m_bStressRatingCached)
   {
      std::future<Float64> f(std::async([&,this] {return GetStressRatingFactorEx(ppStress);}));
      vFutures.push_back(std::move(f));
   }

   if (!m_bPositiveYieldStressRatingCached)
   {
      std::future<Float64> f(std::async([&,this] {return GetYieldStressRatioEx(true, ppYieldStressPositiveMoment);}));
      vFutures.push_back(std::move(f));
   }

   if (!m_bNegativeYieldStressRatingCached)
   {
      std::future<Float64> f(std::async([&,this] {return GetYieldStressRatioEx(false, ppYieldStressNegativeMoment);}));
      vFutures.push_back(std::move(f));
   }

   for (auto& f : vFutures)
   {
      f.wait();
   }
#else
   GetMomentRatingFactorEx(true, ppPositiveMoment);
   GetMomentRatingFactorEx(false, ppNegativeMoment);
   GetShearRatingFactorEx(ppShear);
   GetStressRatingFactorEx(ppStress);
   GetYieldStressRatioEx(true, ppYieldStressPositiveMoment);
   GetYieldStressRatioEx(false, ppYieldStressNegativeMoment);
#endif

   // Find the controlling rating factor

   Float64 RF = DBL_MAX;
   int i = -1; // initialize to an invalid value so that we know if a rating factor wasn't found
   if (m_RF_PositiveMoment < RF )
   {
      RF = m_RF_PositiveMoment;
      i = 0;
   }

   if (m_RF_NegativeMoment < RF )
   {
      RF = m_RF_NegativeMoment;
      i = 1;
   }

   if (m_RF_Shear < RF )
   {
      RF = m_RF_Shear;
      i = 2;
   }

   if (m_RF_Stress < RF )
   {
      RF = m_RF_Stress;
      i = 3;
   }

   if (m_RF_PositiveMomentYieldStress < RF )
   {
      RF = m_RF_PositiveMomentYieldStress;
      i = 4;
   }

   if (m_RF_NegativeMomentYieldStress < RF )
   {
      RF = m_RF_NegativeMomentYieldStress;
      i = 5;
   }

   // nullptr out all but the controlling rating
   if ( i == 0 )
   {
      (*ppPositiveMoment)            = m_pControllingPositiveMoment;
      (*ppNegativeMoment)            = nullptr;
      (*ppShear)                     = nullptr;
      (*ppStress)                    = nullptr;
      (*ppYieldStressPositiveMoment) = nullptr;
      (*ppYieldStressNegativeMoment) = nullptr;
   }
   else if ( i == 1 )
   {
      (*ppPositiveMoment)            = nullptr;
      (*ppNegativeMoment)            = m_pControllingNegativeMoment;
      (*ppShear)                     = nullptr;
      (*ppStress)                    = nullptr;
      (*ppYieldStressPositiveMoment) = nullptr;
      (*ppYieldStressNegativeMoment) = nullptr;
   }
   else if ( i == 2 )
   {
      (*ppPositiveMoment)            = nullptr;
      (*ppNegativeMoment)            = nullptr;
      (*ppShear)                     = m_pControllingShear;
      (*ppStress)                    = nullptr;
      (*ppYieldStressPositiveMoment) = nullptr;
      (*ppYieldStressNegativeMoment) = nullptr;
   }
   else if ( i == 3 )
   {
      (*ppPositiveMoment)            = nullptr;
      (*ppNegativeMoment)            = nullptr;
      (*ppShear)                     = nullptr;
      (*ppStress)                    = m_pControllingStress;
      (*ppYieldStressPositiveMoment) = nullptr;
      (*ppYieldStressNegativeMoment) = nullptr;
   }
   else if ( i == 4 )
   {
      (*ppPositiveMoment)            = nullptr;
      (*ppNegativeMoment)            = nullptr;
      (*ppShear)                     = nullptr;
      (*ppStress)                    = nullptr;
      (*ppYieldStressPositiveMoment) = m_pControllingPositiveMomentYieldStress;
      (*ppYieldStressNegativeMoment) = nullptr;
   }
   else if ( i == 5 )
   {
      (*ppPositiveMoment)            = nullptr;
      (*ppNegativeMoment)            = nullptr;
      (*ppShear)                     = nullptr;
      (*ppStress)                    = nullptr;
      (*ppYieldStressPositiveMoment) = nullptr;
      (*ppYieldStressNegativeMoment) = m_pControllingNegativeMomentYieldStress;
   }
   else
   {
      // ??? rating wasn't done?
      // this can happen if we are rating with a negative moment only truck
      // but the bridge is simple span... we run the truck because we want
      // reactions, but the reating factors are DBL_MAX...
      // since all types of ratings control equally, use positive moment as controlling factor
      ATLASSERT(i == -1);
      (*ppPositiveMoment)            = m_pControllingPositiveMoment;
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

void pgsRatingArtifact::GetSafePostingLoad(Float64* pPostingLoad, Float64* pWeight, Float64* pRF, std::_tstring* pVehicle) const
{
   PostingLoad postingLoad;
#if defined _USE_MULTITHREADING
   std::vector<std::future<PostingLoad>> vFutures;
   vFutures.emplace_back(std::async([&, this] {return ComputeSafePostingLoad(m_PositiveMomentRatings);}));
   vFutures.emplace_back(std::async([&, this] {return ComputeSafePostingLoad(m_NegativeMomentRatings);}));
   vFutures.emplace_back(std::async([&, this] {return ComputeSafePostingLoad(m_ShearRatings);}));
   vFutures.emplace_back(std::async([&, this] {return ComputeSafePostingLoad(m_StressRatings);}));
   vFutures.emplace_back(std::async([&, this] {return ComputeSafePostingLoad(m_PositiveMomentYieldStressRatios);}));
   vFutures.emplace_back(std::async([&, this] {return ComputeSafePostingLoad(m_NegativeMomentYieldStressRatios);}));

   for (auto& f : vFutures)
   {
      auto pl = f.get();
      if (pl.Load < postingLoad.Load)
      {
         postingLoad = pl;
      }
   }
#else
   PostingLoad pl_pM = ComputeSafePostingLoad(m_PositiveMomentRatings);
   PostingLoad pl_nM = ComputeSafePostingLoad(m_NegativeMomentRatings);
   PostingLoad pl_V = ComputeSafePostingLoad(m_ShearRatings);
   PostingLoad pl_f = ComputeSafePostingLoad(m_StressRatings);
   PostingLoad pl_YS_pM = ComputeSafePostingLoad(m_PositiveMomentYieldStressRatios);
   PostingLoad pl_YS_nM = ComputeSafePostingLoad(m_NegativeMomentYieldStressRatios);

   if (pl_pM.Load < postingLoad.Load)
   {
      postingLoad = pl_pM;
   }

   if (pl_nM.Load < postingLoad.Load)
   {
      postingLoad = pl_nM;
   }

   if (pl_V.Load < postingLoad.Load)
   {
      postingLoad = pl_V;
   }

   if (pl_f.Load < postingLoad.Load)
   {
      postingLoad = pl_f;
   }

   if (pl_YS_pM.Load < postingLoad.Load)
   {
      postingLoad = pl_YS_pM;
   }

   if (pl_YS_nM.Load < postingLoad.Load)
   {
      postingLoad = pl_YS_nM;
   }
#endif

   *pPostingLoad = postingLoad.Load;
   *pRF = postingLoad.RF;
   *pWeight = postingLoad.VehicleWeight;
   *pVehicle = postingLoad.VehicleName;
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
   m_LongitudinalReinforcementForShear = rOther.m_LongitudinalReinforcementForShear;

   m_bPositiveMomentRatingCached = rOther.m_bPositiveMomentRatingCached;
   m_RF_PositiveMoment = rOther.m_RF_PositiveMoment;
   m_pControllingPositiveMoment = rOther.m_pControllingPositiveMoment;

   m_bNegativeMomentRatingCached = rOther.m_bNegativeMomentRatingCached;
   m_RF_NegativeMoment = rOther.m_RF_NegativeMoment;
   m_pControllingNegativeMoment = rOther.m_pControllingNegativeMoment;

   m_bShearRatingCached = rOther.m_bShearRatingCached;
   m_RF_Shear = rOther.m_RF_Shear;
   m_pControllingShear = rOther.m_pControllingShear;

   m_bStressRatingCached = rOther.m_bStressRatingCached;
   m_RF_Stress = rOther.m_RF_Stress;
   m_pControllingStress = rOther.m_pControllingStress;

   m_bPositiveYieldStressRatingCached = rOther.m_bPositiveYieldStressRatingCached;
   m_RF_PositiveMomentYieldStress = rOther.m_RF_PositiveMomentYieldStress;
   m_pControllingPositiveMomentYieldStress = rOther.m_pControllingPositiveMomentYieldStress;

   m_bNegativeYieldStressRatingCached = rOther.m_bNegativeYieldStressRatingCached;
   m_RF_NegativeMomentYieldStress = rOther.m_RF_NegativeMomentYieldStress;
   m_pControllingNegativeMomentYieldStress = rOther.m_pControllingNegativeMomentYieldStress;
}

void pgsRatingArtifact::MakeAssignment(const pgsRatingArtifact& rOther)
{
   MakeCopy( rOther );
}
