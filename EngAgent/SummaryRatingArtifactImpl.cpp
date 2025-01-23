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
#include "EngAgent.h"
#include "SummaryRatingArtifactImpl.h"
#include "EngAgentImp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/****************************************************************************
CLASS
   pgsSummaryRatingArtifactImpl
****************************************************************************/

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsSummaryRatingArtifactImpl::pgsSummaryRatingArtifactImpl(const std::vector<CGirderKey>& girderKeys,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx, IBroker* pBroker, const CEngAgentImp* pEngAgentImp):
   m_pBroker(pBroker),
   m_pEngAgentImp(pEngAgentImp),
   m_GirderKeys(girderKeys),
   m_RatingType(ratingType),
   m_VehicleIdx(vehicleIdx)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
   
Float64 pgsSummaryRatingArtifactImpl::GetMomentRatingFactor(bool bPositiveMoment) const
{
   const pgsMomentRatingArtifact* pArtifact;
   return GetMomentRatingFactorEx(bPositiveMoment, &pArtifact);
}

Float64 pgsSummaryRatingArtifactImpl::GetMomentRatingFactorEx(bool bPositiveMoment,const pgsMomentRatingArtifact** ppControllingArtifact) const
{
   Float64 RF = DBL_MAX;
   (*ppControllingArtifact) = nullptr;

   bool isFirst(true); // Want a valid artifact pointer even if all RF's are DBL_MAX
   for (auto girderKey : m_GirderKeys)
   {
      const pgsRatingArtifact* pArtifact = m_pEngAgentImp->GetRatingArtifact(girderKey, m_RatingType, m_VehicleIdx);

      const pgsMomentRatingArtifact* pMRArtifact;
      Float64 rf = pArtifact->GetMomentRatingFactorEx(bPositiveMoment, &pMRArtifact);
      if (rf < RF || isFirst)
      {
         RF = rf;
         *ppControllingArtifact = pMRArtifact;
      }

      isFirst = false;
   }

   return RF;
}

Float64 pgsSummaryRatingArtifactImpl::GetShearRatingFactor() const
{
   const pgsShearRatingArtifact* pArtifact;
   return GetShearRatingFactorEx(&pArtifact);
}

Float64 pgsSummaryRatingArtifactImpl::GetShearRatingFactorEx(const pgsShearRatingArtifact** ppControllingArtifact) const
{
   Float64 RF = DBL_MAX;
   (*ppControllingArtifact) = nullptr;

   bool isFirst(true); // Want a valid artifact pointer even if all RF's are DBL_MAX
   for (auto girderKey : m_GirderKeys)
   {
      const pgsRatingArtifact* pArtifact = m_pEngAgentImp->GetRatingArtifact(girderKey, m_RatingType, m_VehicleIdx);

      const pgsShearRatingArtifact* pVArtifact;
      Float64 rf = pArtifact->GetShearRatingFactorEx(&pVArtifact);
      if (rf < RF || isFirst)
      {
         RF = rf;
         *ppControllingArtifact = pVArtifact;
      }

      isFirst = false;
   }

   return RF;
}

Float64 pgsSummaryRatingArtifactImpl::GetStressRatingFactor() const
{
   const pgsStressRatingArtifact* pArtifact;
   return GetStressRatingFactorEx(&pArtifact);
}

Float64 pgsSummaryRatingArtifactImpl::GetStressRatingFactorEx(const pgsStressRatingArtifact** ppControllingArtifact) const
{
   Float64 RF = DBL_MAX;
   (*ppControllingArtifact) = nullptr;

   bool isFirst(true); // Want a valid artifact pointer even if all RF's are DBL_MAX
   for (auto girderKey : m_GirderKeys)
   {
      const pgsRatingArtifact* pArtifact = m_pEngAgentImp->GetRatingArtifact(girderKey, m_RatingType, m_VehicleIdx);

      const pgsStressRatingArtifact* pVArtifact;
      Float64 rf = pArtifact->GetStressRatingFactorEx(&pVArtifact);
      if (rf < RF || isFirst)
      {
         RF = rf;
         *ppControllingArtifact = pVArtifact;
      }

      isFirst = false;
   }

   return RF;
}

Float64 pgsSummaryRatingArtifactImpl::GetYieldStressRatio(bool bPositiveMoment) const
{
   const pgsYieldStressRatioArtifact* pArtifact;
   return GetYieldStressRatioEx(bPositiveMoment,&pArtifact);
}

Float64 pgsSummaryRatingArtifactImpl::GetYieldStressRatioEx(bool bPositiveMoment,const pgsYieldStressRatioArtifact** ppControllingArtifact) const
{
   Float64 RF = DBL_MAX;
   (*ppControllingArtifact) = nullptr;

   bool isFirst(true); // Want a valid artifact pointer even if all RF's are DBL_MAX
   for (auto girderKey : m_GirderKeys)
   {
      const pgsRatingArtifact* pArtifact = m_pEngAgentImp->GetRatingArtifact(girderKey, m_RatingType, m_VehicleIdx);

      const pgsYieldStressRatioArtifact* pVArtifact;
      Float64 rf = pArtifact->GetYieldStressRatioEx(bPositiveMoment, &pVArtifact);
      if (rf < RF || isFirst)
      {
         RF = rf;
         *ppControllingArtifact = pVArtifact;
      }

      isFirst = false;
   }

   return RF;
}

Float64 pgsSummaryRatingArtifactImpl::GetRatingFactor() const
{
   Float64 RF = DBL_MAX;

   for (auto girderKey : m_GirderKeys)
   {
      const pgsRatingArtifact* pArtifact = m_pEngAgentImp->GetRatingArtifact(girderKey, m_RatingType, m_VehicleIdx);
      Float64 rf = pArtifact->GetRatingFactor();

      RF = min(RF, rf);
   }

   return RF;
}

Float64 pgsSummaryRatingArtifactImpl::GetRatingFactorEx(const pgsMomentRatingArtifact** ppPositiveMoment,const pgsMomentRatingArtifact** ppNegativeMoment,
                  const pgsShearRatingArtifact** ppShear,
                  const pgsStressRatingArtifact** ppStress) const
{
   // this may not look efficient, but we know that pgsRatingArtifact caches its answers, so it's not bad
   Float64 RF = DBL_MAX;

   // First find controlling girder
   Uint32 icontrol(0);
   Uint32 ngdrs = (Uint32)m_GirderKeys.size();
   for (Uint32 igdr = 0; igdr < ngdrs; igdr++)
   {
      const pgsRatingArtifact* pArtifact = m_pEngAgentImp->GetRatingArtifact(m_GirderKeys[igdr], m_RatingType, m_VehicleIdx);
      Float64 rf = pArtifact->GetRatingFactor();
      if (rf < RF)
      {
         RF = rf;
         icontrol = igdr;
      }
   }

   // get controlling artifact and return its values
   const pgsRatingArtifact* pArtifact = m_pEngAgentImp->GetRatingArtifact(m_GirderKeys[icontrol], m_RatingType, m_VehicleIdx);
   return pArtifact->GetRatingFactorEx(ppPositiveMoment, ppNegativeMoment, ppShear, ppStress);
}

Float64 pgsSummaryRatingArtifactImpl::GetYieldStressRatio(const pgsYieldStressRatioArtifact** ppYieldStressPositiveMoment, const pgsYieldStressRatioArtifact** ppYieldStressNegativeMoment) const
{
   // this may not look efficient, but we know that pgsRatingArtifact caches its answers, so it's not bad
   Float64 SR = DBL_MAX;

   // First find controlling girder
   Uint32 icontrol(0);
   Uint32 ngdrs = (Uint32)m_GirderKeys.size();
   for (Uint32 igdr = 0; igdr < ngdrs; igdr++)
   {
      const pgsRatingArtifact* pArtifact = m_pEngAgentImp->GetRatingArtifact(m_GirderKeys[igdr], m_RatingType, m_VehicleIdx);
      Float64 sr = pArtifact->GetYieldStressRatio();
      if (sr < SR)
      {
         SR = sr;
         icontrol = igdr;
      }
   }

   // get controlling artifact and return its values
   const pgsRatingArtifact* pArtifact = m_pEngAgentImp->GetRatingArtifact(m_GirderKeys[icontrol], m_RatingType, m_VehicleIdx);
   return pArtifact->GetYieldStressRatio(ppYieldStressPositiveMoment, ppYieldStressNegativeMoment);
}

bool pgsSummaryRatingArtifactImpl::IsLoadPostingRequired() const
{
   for (auto girderKey : m_GirderKeys)
   {
      const pgsRatingArtifact* pArtifact = m_pEngAgentImp->GetRatingArtifact(girderKey, m_RatingType, m_VehicleIdx);
      if (pArtifact->IsLoadPostingRequired())
      {
         return true;
      }
   }

   return false;
}

void pgsSummaryRatingArtifactImpl::GetSafePostingLoad(Float64* pPostingLoad,Float64* pWeight,Float64* pRF,std::_tstring* pVehicle) const
{
   Float64 PL = DBL_MAX;

   // First find controlling girder
   Uint32 icontrol(0);
   Uint32 ngdrs = (Uint32)m_GirderKeys.size();
   for (Uint32 igdr = 0; igdr < ngdrs; igdr++)
   {
      const pgsRatingArtifact* pArtifact = m_pEngAgentImp->GetRatingArtifact(m_GirderKeys[igdr], m_RatingType, m_VehicleIdx);
      Float64 pl, weight, rf;
      std::_tstring sVehicle;
      pArtifact->GetSafePostingLoad(&pl, &weight, &rf, &sVehicle);
      if (pl < PL)
      {
         PL = pl;
         icontrol = igdr;
      }
   }

   // get controlling artifact and return its values
   const pgsRatingArtifact* pArtifact = m_pEngAgentImp->GetRatingArtifact(m_GirderKeys[icontrol], m_RatingType, m_VehicleIdx);
   pArtifact->GetSafePostingLoad(pPostingLoad, pWeight, pRF, pVehicle);
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
