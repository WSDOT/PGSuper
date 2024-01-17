///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#ifndef INCLUDED_DISTFACTORENGINEERIMPL_H_
#define INCLUDED_DISTFACTORENGINEERIMPL_H_

#include <IFace\DistFactorEngineer.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\StatusCenter.h>
#include <IFace\DistributionFactors.h>
#include <PGSuperException.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\BridgeDescription2.h>
#include <psgLib/LiveLoadDistributionCriteria.h>
#include <Beams\Interfaces.h>
#include <map>
#include <numeric>


// Side where overhang value was used if in equation
enum DfSide {dfLeft, dfRight};

// Details common for all beam types
struct BASE_LLDFDETAILS
{
   pgsTypes::LiveLoadDistributionFactorMethod Method;
   Float64 ControllingLocation; // for girder spacing and overhang, measured from left end of girder

   GirderIndexType gdrNum;
   bool bExteriorGirder;
   DfSide Side;

   GirderIndexType Nb;
   Float64 Savg;
   std::vector<Float64> gdrSpacings;
   Float64 leftCurbOverhang;  // CL girder to curb
   Float64 rightCurbOverhang;
   Float64 leftSlabOverhang;  // CL girder to edge of slab
   Float64 rightSlabOverhang;
   Float64 W; // to edges of slab

   Uint32   Nl;
   Float64 wLane;
   Float64 wCurbToCurb;

   Float64 skew1;
   Float64 skew2;
};

#define USE_CURRENT_FC -1

template <class T>
class CDistFactorEngineerImpl : public IDistFactorEngineer, public IInitialize 
{
public:
   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID) override;
   virtual Float64 GetMomentDF(const CSpanKey& spanKey,pgsTypes::LimitState ls) override;
   virtual Float64 GetMomentDF(const CSpanKey& spanKey,pgsTypes::LimitState ls,Float64 fcgdr) override;
   virtual Float64 GetNegMomentDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace) override;
   virtual Float64 GetShearDF(const CSpanKey& spanKey,pgsTypes::LimitState ls) override;
   virtual Float64 GetNegMomentDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr) override;
   virtual Float64 GetShearDF(const CSpanKey& spanKey,pgsTypes::LimitState ls,Float64 fcgdr) override;
   virtual bool Run1250Tests(const CSpanKey& spanKey,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile) override;
   virtual bool GetDFResultsEx(const CSpanKey& spanKey,pgsTypes::LimitState ls,
                               Float64* gpM, Float64* gpM1, Float64* gpM2,  // pos moment
                               Float64* gnM, Float64* gnM1, Float64* gnM2,  // neg moment, ahead face
                               Float64* gV,  Float64* gV1,  Float64* gV2) override;   // shear
   virtual Float64 GetSkewCorrectionFactorForMoment(const CSpanKey& spanKey,pgsTypes::LimitState ls) override;
   virtual Float64 GetSkewCorrectionFactorForShear(const CSpanKey& spanKey,pgsTypes::LimitState ls) override;

   enum DFParam { dfPierLeft, dfPierRight, dfSpan, dfReaction };

protected:
   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;
   StatusCallbackIDType m_scidRefinedAnalysis;

   struct PIERDETAILS : T
   {
      // Distribution factors for negative moment over pier
      WBFL::LRFD::ILiveLoadDistributionFactor::DFResult gM1;
      WBFL::LRFD::ILiveLoadDistributionFactor::DFResult gM2;
      Float64 gM;
   };

   struct SPANDETAILS : T
   {
      // Distribution factors for shear in the span
      WBFL::LRFD::ILiveLoadDistributionFactor::DFResult gV1;
      WBFL::LRFD::ILiveLoadDistributionFactor::DFResult gV2;
      Float64 gV;
      Float64 gVSkewCorrection;

      // Distribution factors for positive and negative moment in the span
      WBFL::LRFD::ILiveLoadDistributionFactor::DFResult gM1;
      WBFL::LRFD::ILiveLoadDistributionFactor::DFResult gM2;
      Float64 gM;
      Float64 gMSkewCorrection;
   };

   void GetIndicies(IndexType spanOrPierIdx,DFParam dfType,SpanIndexType& span,PierIndexType& pier,SpanIndexType& prev_span,SpanIndexType& next_span,PierIndexType& prev_pier,PierIndexType& next_pier);

   std::map<PierGirderHashType,PIERDETAILS> m_PierLLDF[2][2]; // first index is pier face type, second index is limit state type
   void GetPierDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr,PIERDETAILS* plldf);
   void GetPierDFRaw(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr,PIERDETAILS* plldf);

   std::map<SpanGirderHashType,SPANDETAILS> m_SpanLLDF[2]; // index is limit state type
   void GetSpanDF(const CSpanKey& spanKey,pgsTypes::LimitState ls,Float64 fcgdr,SPANDETAILS* plldf);
   void GetSpanDFRaw(const CSpanKey& spanKey,pgsTypes::LimitState ls,Float64 fcgdr,SPANDETAILS* plldf);

   Float64 GetEffectiveSpanLength(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType);
   void GetGirderSpacingAndOverhang(const CSpanKey& spanKey,DFParam dfType,BASE_LLDFDETAILS* pDetails,pgsPointOfInterest* pControllingPoi);

   virtual WBFL::LRFD::LiveLoadDistributionFactorBase* GetLLDFParameters(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType,Float64 fcgdr,T* plldf) = 0;

   void HandleRangeOfApplicabilityError(const WBFL::LRFD::XRangeOfApplicability& e);

   int LimitStateType(pgsTypes::LimitState ls);
};

////////////////////////////////////////


template <class T>
void CDistFactorEngineerImpl<T>::SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID)
{
	m_pBroker = pBroker;
	m_StatusGroupID = statusGroupID;

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   m_scidRefinedAnalysis = pStatusCenter->RegisterCallback( new pgsRefinedAnalysisStatusCallback(m_pBroker) );
}

template <class T>
Float64 CDistFactorEngineerImpl<T>::GetMomentDF(const CSpanKey& spanKey,pgsTypes::LimitState ls)
{
   SPANDETAILS lldf;
   GetSpanDF(spanKey,ls,USE_CURRENT_FC,&lldf);
   return lldf.gM;
}

template <class T>
Float64 CDistFactorEngineerImpl<T>::GetMomentDF(const CSpanKey& spanKey,pgsTypes::LimitState ls,Float64 fcgdr)
{
   SPANDETAILS lldf;
   GetSpanDF(spanKey,ls,fcgdr,&lldf);
   return lldf.gM;
}

template <class T>
Float64 CDistFactorEngineerImpl<T>::GetNegMomentDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace)
{
   PIERDETAILS lldf;
   GetPierDF(pierIdx,gdrIdx,ls,pierFace,USE_CURRENT_FC,&lldf);
   return lldf.gM;
}

template <class T>
Float64 CDistFactorEngineerImpl<T>::GetNegMomentDF(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr)
{
   PIERDETAILS lldf;
   GetPierDF(pierIdx,gdrIdx,ls,pierFace,fcgdr,&lldf);
   return lldf.gM;
}

template <class T>
Float64 CDistFactorEngineerImpl<T>::GetShearDF(const CSpanKey& spanKey,pgsTypes::LimitState ls)
{
   SPANDETAILS lldf;
   GetSpanDF(spanKey,ls,USE_CURRENT_FC,&lldf);
   return lldf.gV;
}

template <class T>
Float64 CDistFactorEngineerImpl<T>::GetShearDF(const CSpanKey& spanKey,pgsTypes::LimitState ls,Float64 fcgdr)
{
   SPANDETAILS lldf;
   GetSpanDF(spanKey,ls,fcgdr,&lldf);
   return lldf.gV;
}


template <class T>
void CDistFactorEngineerImpl<T>::GetPierDF(PierIndexType pierIdx, GirderIndexType gdrIdx, pgsTypes::LimitState ls, pgsTypes::PierFaceType pierFace, Float64 fcgdr, PIERDETAILS* plldf)
{
   auto found = m_PierLLDF[pierFace][LimitStateType(ls)].find(HashPierGirder(pierIdx, gdrIdx));
   if (found != m_PierLLDF[pierFace][LimitStateType(ls)].end() && fcgdr == USE_CURRENT_FC)
   {
      *plldf = (*found).second;
      return; // We already have the distribution factors for this girder
   }

   // Need to compute. First determine if we need to deal with rule that forces exterior LLDF's never to be less than the adjacent interior beam
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
   const CGirderGroupData* pGroup = pPier->GetGirderGroup(pierFace);
   GirderIndexType nGirders = pGroup->GetGirderCount();
   bool bExteriorGirder = pGroup->IsExteriorGirder(gdrIdx);

   GET_IFACE(ISpecification, pSpec);
   GET_IFACE(ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& live_load_distribution_criteria = pSpecEntry->GetLiveLoadDistributionCriteria();

   // Go ahead and get raw calculated details for this girder. We will modify if needed in next section
   GetPierDFRaw(pierIdx, gdrIdx, ls, pierFace, fcgdr, plldf);

   if (bExteriorGirder && 2 < nGirders && live_load_distribution_criteria.bExteriorBeamLiveLoadDistributionGTInteriorBeam)
   {
      // Exterior-interior rule applies. Compute factors for adjacent interior beam
      GirderIndexType adjGdrIndex = gdrIdx == 0 ? 1 : nGirders - 2;

      PIERDETAILS adjdet;
      GetPierDFRaw(pierIdx, adjGdrIndex, ls, pierFace, fcgdr, &adjdet);

      if ((plldf->gM+TOLERANCE) < adjdet.gM)
      {
         plldf->gM = adjdet.gM;
         plldf->gM1 = adjdet.gM1;
         plldf->gM1.ControllingMethod |= WBFL::LRFD::INTERIOR_OVERRIDE;
         plldf->gM2 = adjdet.gM2;
         plldf->gM2.ControllingMethod |= WBFL::LRFD::INTERIOR_OVERRIDE;
      }
   }

   // Store in cache
   m_PierLLDF[pierFace][LimitStateType(ls)].insert( std::make_pair(HashPierGirder(pierIdx,gdrIdx),*plldf) );
}

template <class T>
void CDistFactorEngineerImpl<T>::GetPierDFRaw(PierIndexType pierIdx,GirderIndexType gdrIdx,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr,PIERDETAILS* plldf)
{
   DFParam dfParam = (pierFace == pgsTypes::Back ? dfPierLeft : dfPierRight);
   std::unique_ptr<WBFL::LRFD::LiveLoadDistributionFactorBase> pLLDF( GetLLDFParameters(pierIdx,gdrIdx,dfParam,fcgdr,plldf) );

   // Get method used to compute factors
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::DistributionFactorMethod df_method = pIBridgeDesc->GetLiveLoadDistributionFactorMethod();

   // See if Bridge-Wide range of applicability rule was breached. We might switch to lever rule for all computations.
   if (df_method == pgsTypes::Calculated && pLLDF->GetBridgeWideRangeOfApplicabilityIssue() != 0)
   {
      GET_IFACE(ILiveLoads,pLiveLoads);
      WBFL::LRFD::RangeOfApplicabilityAction action = pLiveLoads->GetRangeOfApplicabilityAction();
      if (action == WBFL::LRFD::RangeOfApplicabilityAction::IgnoreUseLeverRule)
      {
         // force the df method to lever rule
         df_method = pgsTypes::LeverRule;
      }
   }

   WBFL::LRFD::ILiveLoadDistributionFactor::Location loc;
   loc =  plldf->bExteriorGirder ? WBFL::LRFD::ILiveLoadDistributionFactor::Location::ExtGirder : WBFL::LRFD::ILiveLoadDistributionFactor::Location::IntGirder;

   try
   {
      WBFL::LRFD::LimitState lrfdls = PGSLimitStateToLRFDLimitState(ls);

      // Negative moment distribution factor
      if (df_method == pgsTypes::Calculated)
      {
         plldf->gM1 = pLLDF->MomentDFEx(loc, WBFL::LRFD::ILiveLoadDistributionFactor::NumLoadedLanes::One, lrfdls);

         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
         {
            plldf->gM2 = pLLDF->MomentDFEx(loc, WBFL::LRFD::ILiveLoadDistributionFactor::NumLoadedLanes::TwoOrMore,  lrfdls);
         }
         else
         {
            plldf->gM2.mg = 0;
         }
      }
      else if (df_method == pgsTypes::LeverRule)
      {
         plldf->gM1 = pLLDF->DistributeMomentByLeverRule(loc, WBFL::LRFD::ILiveLoadDistributionFactor::NumLoadedLanes::One, ls!=pgsTypes::FatigueI);

         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
         {
            plldf->gM2 = pLLDF->DistributeMomentByLeverRule(loc, WBFL::LRFD::ILiveLoadDistributionFactor::NumLoadedLanes::TwoOrMore, true);
         }
         else
         {
            plldf->gM2.mg = 0;
         }
      }
      else
      {
         ATLASSERT(false);
      }

      // see if we need to compare with lanes/beams
      GET_IFACE(ISpecification, pSpec);
      GET_IFACE(ILibrary, pLib);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
      const auto& live_load_distribution_criteria = pSpecEntry->GetLiveLoadDistributionCriteria();
      if (live_load_distribution_criteria.bLimitDistributionFactorsToLanesBeams)
      {
         // Compare results with lanes/beams and override if needed
         WBFL::LRFD::ILiveLoadDistributionFactor::DFResult glb1 = pLLDF->GetLanesBeamsMethod(1,GirderIndexType(plldf->Nb), ls != pgsTypes::FatigueI);
         WBFL::LRFD::ILiveLoadDistributionFactor::DFResult glb2 = pLLDF->GetLanesBeamsMethod(plldf->Nl,GirderIndexType(plldf->Nb),ls != pgsTypes::FatigueI);

         // Moment
         if (plldf->gM1.mg < glb1.mg)
         {
            plldf->gM1.mg = glb1.mg;
            plldf->gM1.ControllingMethod = WBFL::LRFD::LANES_DIV_BEAMS | WBFL::LRFD::LANES_BEAMS_OVERRIDE;
            plldf->gM1.LanesBeamsData = glb1.LanesBeamsData;
         }

         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
         {
            if (plldf->gM2.mg < glb2.mg)
            {
               plldf->gM2.mg = glb2.mg;
               plldf->gM2.ControllingMethod = WBFL::LRFD::LANES_DIV_BEAMS | WBFL::LRFD::LANES_BEAMS_OVERRIDE;
               plldf->gM2.LanesBeamsData = glb2.LanesBeamsData;
            }
         }
      }

      // controlling
      if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
      {
         if ( plldf->gM1.ControllingMethod & WBFL::LRFD::OVERRIDE_USING_MULTILANE_FACTOR)
         {
            // Case where multi-lane factor is always used (e.g., TxDOT U Beams)
            plldf->gM = plldf->gM2.mg;
         }
         else
         {
            plldf->gM = Max(plldf->gM1.mg, plldf->gM2.mg);
         }
      }
      else
      {
         plldf->gM = plldf->gM1.mg;
      }
   }
   catch( const WBFL::LRFD::XRangeOfApplicability& e)
   {
      HandleRangeOfApplicabilityError(e);
   }
}

template <class T>
void CDistFactorEngineerImpl<T>::GetSpanDF(const CSpanKey& spanKey, pgsTypes::LimitState ls, Float64 fcgdr, SPANDETAILS* plldf)
{
   auto found = m_SpanLLDF[LimitStateType(ls)].find(::HashSpanGirder(spanKey.spanIndex,spanKey.girderIndex));
   if ( found != m_SpanLLDF[LimitStateType(ls)].end() && fcgdr == USE_CURRENT_FC )
   {
      *plldf = (*found).second;
      return; // We already have the distribution factors for this girder
   }

   // Need to compute. First determine if we need to deal with rule that forces exterior LLDF's never to be less than the adjacent interior beam
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   GirderIndexType nGirders = pGroup->GetGirderCount();
   bool bExteriorGirder = pGroup->IsExteriorGirder(spanKey.girderIndex);

   GET_IFACE(ISpecification, pSpec);
   GET_IFACE(ILibrary, pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& live_load_distribution_criteria = pSpecEntry->GetLiveLoadDistributionCriteria();

   // Go ahead and get raw calculated details for this girder. We will modify if needed in next section
   GetSpanDFRaw(spanKey, ls, fcgdr,  plldf);

   if (bExteriorGirder && 2 < nGirders && live_load_distribution_criteria.bExteriorBeamLiveLoadDistributionGTInteriorBeam)
   {
      // Exterior-interior rule applies. Compute factors for adjacent interior beam
      GirderIndexType adjIndex = spanKey.girderIndex == 0 ? 1 : nGirders - 2;
      CSpanKey adjSpanKey(spanKey.spanIndex, adjIndex);

      SPANDETAILS adjdet;
      GetSpanDFRaw(adjSpanKey, ls, fcgdr,  &adjdet);

      // moment and shear are treated separately
      if ((plldf->gM+TOLERANCE) < adjdet.gM)
      {
         plldf->gM = adjdet.gM;
         plldf->gM1 = adjdet.gM1;
         plldf->gM1.ControllingMethod |= WBFL::LRFD::INTERIOR_OVERRIDE;
         plldf->gM2 = adjdet.gM2;
         plldf->gM2.ControllingMethod |= WBFL::LRFD::INTERIOR_OVERRIDE;
         plldf->gMSkewCorrection = adjdet.gMSkewCorrection;
      }

      if ((plldf->gV+TOLERANCE) < adjdet.gV)
      {
         plldf->gV = adjdet.gV;
         plldf->gV1 = adjdet.gV1;
         plldf->gV1.ControllingMethod |= WBFL::LRFD::INTERIOR_OVERRIDE;
         plldf->gV2 = adjdet.gV2;
         plldf->gV2.ControllingMethod |= WBFL::LRFD::INTERIOR_OVERRIDE;
         plldf->gVSkewCorrection = adjdet.gVSkewCorrection;
      }
   }

   // Save factors in cache
   m_SpanLLDF[LimitStateType(ls)].insert( std::make_pair(::HashSpanGirder(spanKey.spanIndex,spanKey.girderIndex),*plldf) );
}

template <class T>
void CDistFactorEngineerImpl<T>::GetSpanDFRaw(const CSpanKey& spanKey,pgsTypes::LimitState ls,Float64 fcgdr,SPANDETAILS* plldf)
{
   std::unique_ptr<WBFL::LRFD::LiveLoadDistributionFactorBase> pLLDF(GetLLDFParameters(spanKey.spanIndex,spanKey.girderIndex,dfSpan,fcgdr,plldf));

   // Get method used to compute factors
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::DistributionFactorMethod df_method = pIBridgeDesc->GetLiveLoadDistributionFactorMethod();

   // See if Bridge-Wide range of applicability rule was breached. We might switch to lever rule for all computations.
   if (df_method == pgsTypes::Calculated && pLLDF->GetBridgeWideRangeOfApplicabilityIssue() != 0)
   {
      GET_IFACE(ILiveLoads,pLiveLoads);
      WBFL::LRFD::RangeOfApplicabilityAction action = pLiveLoads->GetRangeOfApplicabilityAction();
      if (action == WBFL::LRFD::RangeOfApplicabilityAction::IgnoreUseLeverRule)
      {
         // force the df method to lever rule
         df_method = pgsTypes::LeverRule;
      }
   }

   WBFL::LRFD::ILiveLoadDistributionFactor::Location loc;
   loc =  plldf->bExteriorGirder ? WBFL::LRFD::ILiveLoadDistributionFactor::Location::ExtGirder : WBFL::LRFD::ILiveLoadDistributionFactor::Location::IntGirder;

   try
   {
      WBFL::LRFD::LimitState lrfdls = PGSLimitStateToLRFDLimitState(ls);

      if (df_method == pgsTypes::Calculated)
      {
         plldf->gM1 = pLLDF->MomentDFEx(loc, WBFL::LRFD::ILiveLoadDistributionFactor::NumLoadedLanes::One, lrfdls);

         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI)
         {
            plldf->gM2 = pLLDF->MomentDFEx(loc, WBFL::LRFD::ILiveLoadDistributionFactor::NumLoadedLanes::TwoOrMore, lrfdls);
         }
         else
         {
            plldf->gM2.mg = 0;
         }


         plldf->gV1 = pLLDF->ShearDFEx(loc, WBFL::LRFD::ILiveLoadDistributionFactor::NumLoadedLanes::One, lrfdls);

         if ( 2 <= plldf->Nl && ls != pgsTypes::FatigueI)
         {
            plldf->gV2 = pLLDF->ShearDFEx(loc, WBFL::LRFD::ILiveLoadDistributionFactor::NumLoadedLanes::TwoOrMore, lrfdls);
         }
         else
         {
            plldf->gV2.mg = 0;
         }
      }
      else if (df_method == pgsTypes::LeverRule)
      {
         plldf->gM1 = pLLDF->DistributeMomentByLeverRule(loc, WBFL::LRFD::ILiveLoadDistributionFactor::NumLoadedLanes::One, ls != pgsTypes::FatigueI);
         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
         {
            plldf->gM2 = pLLDF->DistributeMomentByLeverRule(loc, WBFL::LRFD::ILiveLoadDistributionFactor::NumLoadedLanes::TwoOrMore, true);
         }
         else
         {
            plldf->gM2.mg = 0;
         }


         plldf->gV1 = pLLDF->DistributeShearByLeverRule(loc, WBFL::LRFD::ILiveLoadDistributionFactor::NumLoadedLanes::One, ls != pgsTypes::FatigueI);

         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
         {
            plldf->gV2 = pLLDF->DistributeShearByLeverRule(loc, WBFL::LRFD::ILiveLoadDistributionFactor::NumLoadedLanes::TwoOrMore, true);
         }
         else
         {
            plldf->gV2.mg = 0;
         }
      }
      else
      {
         ATLASSERT(false); // should not be here?
      }

      // see if we need to compare with lanes/beams
      GET_IFACE(ISpecification, pSpec);
      GET_IFACE(ILibrary, pLib);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
      const auto& live_load_distribution_criteria = pSpecEntry->GetLiveLoadDistributionCriteria();
      if (live_load_distribution_criteria.bLimitDistributionFactorsToLanesBeams)
      {
         // Compare results with lanes/beams and override if needed
         WBFL::LRFD::ILiveLoadDistributionFactor::DFResult glb1 = pLLDF->GetLanesBeamsMethod(1,GirderIndexType(plldf->Nb), ls != pgsTypes::FatigueI);
         WBFL::LRFD::ILiveLoadDistributionFactor::DFResult glb2 = pLLDF->GetLanesBeamsMethod(plldf->Nl,GirderIndexType(plldf->Nb), ls != pgsTypes::FatigueI);

         // Moment
         if ( plldf->gM1.mg < glb1.mg)
         {
            plldf->gM1.mg = glb1.mg;
            plldf->gM1.ControllingMethod = WBFL::LRFD::LANES_DIV_BEAMS | WBFL::LRFD::LANES_BEAMS_OVERRIDE;
            plldf->gM1.LanesBeamsData = glb1.LanesBeamsData;
         }

         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
         {
            if (plldf->gM2.mg < glb2.mg)
            {
               plldf->gM2.mg = glb2.mg;
               plldf->gM2.ControllingMethod = WBFL::LRFD::LANES_DIV_BEAMS | WBFL::LRFD::LANES_BEAMS_OVERRIDE;
               plldf->gM2.LanesBeamsData = glb2.LanesBeamsData;
            }
         }

         // Shear
         if (plldf->gV1.mg < glb1.mg)
         {
            plldf->gV1.mg = glb1.mg;
            plldf->gV1.ControllingMethod = WBFL::LRFD::LANES_DIV_BEAMS | WBFL::LRFD::LANES_BEAMS_OVERRIDE;
            plldf->gV1.LanesBeamsData = glb1.LanesBeamsData;
         }

         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
         {
            if (plldf->gV2.mg < glb2.mg)
            {
               plldf->gV2.mg = glb2.mg;
               plldf->gV2.ControllingMethod = WBFL::LRFD::LANES_DIV_BEAMS | WBFL::LRFD::LANES_BEAMS_OVERRIDE;
               plldf->gV2.LanesBeamsData = glb2.LanesBeamsData;
            }
         }
      }

      // controlling
      if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
      {
         if ( plldf->gM1.ControllingMethod & WBFL::LRFD::OVERRIDE_USING_MULTILANE_FACTOR)
         {
            // Case where multi-lane factor is always used (e.g., TxDOT U Beams)
            plldf->gM = plldf->gM2.mg;
            plldf->gV = plldf->gV2.mg;
         }
         else
         {
            plldf->gM = Max(plldf->gM1.mg, plldf->gM2.mg);
            plldf->gV = Max(plldf->gV1.mg, plldf->gV2.mg);
         }
      }
      else
      {
         plldf->gM = plldf->gM1.mg;
         plldf->gV = plldf->gV1.mg;
      }

      // skew corrections

      plldf->gMSkewCorrection = pLLDF->MomentSkewCorrectionFactor();
      plldf->gVSkewCorrection = pLLDF->ShearSkewCorrectionFactor();

   }
   catch( const WBFL::LRFD::XRangeOfApplicability& e)
   {
      HandleRangeOfApplicabilityError(e);
   }

}

template <class T>
void CDistFactorEngineerImpl<T>::HandleRangeOfApplicabilityError(const WBFL::LRFD::XRangeOfApplicability& e)
{
   GET_IFACE(IEAFStatusCenter,     pStatusCenter);

   LPCTSTR msg = _T("Live Load Distribution Factors could not be calculated");
   pgsRefinedAnalysisStatusItem* pStatusItem = new pgsRefinedAnalysisStatusItem(m_StatusGroupID,m_scidRefinedAnalysis,msg);
   pStatusCenter->Add(pStatusItem);

   std::_tostringstream os;
   std::_tstring errmsg = e.GetErrorMessage();
   os << _T("Live Load Distribution Factors could not be calculated for the following reason") << std::endl;
   os << errmsg << std::endl;
   os << _T("A refined method of analysis is required for this bridge") << std::endl;
   os << _T("See Status Center for Details");
   THROW_UNWIND(os.str().c_str(),XREASON_REFINEDANALYSISREQUIRED);
}

template <class T>
int CDistFactorEngineerImpl<T>::LimitStateType(pgsTypes::LimitState ls)
{
   // maps specific limit state to a general limit state type
   // strength and service limit states, return 0
   // fatigue limit state, return 1
   if ( IsStrengthLimitState(ls) || IsServiceLimitState(ls) )
   {
      return 0;
   }
   else if ( IsFatigueLimitState(ls) )
   {
      return 1;
   }
   else
   {
      ATLASSERT(false); // should never get here
      return 0;
   }
}

template <class T>
void CDistFactorEngineerImpl<T>::GetIndicies(IndexType spanOrPierIdx,DFParam dfType,SpanIndexType& span,PierIndexType& pier,SpanIndexType& prev_span,SpanIndexType& next_span,PierIndexType& prev_pier,PierIndexType& next_pier)
{
   GET_IFACE(IBridge,pBridge);

   SpanIndexType nSpans = pBridge->GetSpanCount();
   PierIndexType nPiers = pBridge->GetPierCount();

   if ( dfType == dfSpan )
   {
      span = (SpanIndexType)spanOrPierIdx;

      prev_pier = span;
      next_pier = prev_pier + 1;
   }
   else
   {
      pier = (PierIndexType)spanOrPierIdx;
      prev_span = (pier == 0 ? INVALID_INDEX : pier-1);
      next_span = (pier == nPiers-1 ? INVALID_INDEX : prev_span + 1);

      span = (dfType == dfPierLeft ? prev_span : next_span);
      if ( span == INVALID_INDEX )
      {
         span = prev_span;
      }
   }
}

template <class T>
void CDistFactorEngineerImpl<T>::GetGirderSpacingAndOverhang(const CSpanKey& spanKey,DFParam dfType,BASE_LLDFDETAILS* pDetails, pgsPointOfInterest* pControllingPoi)
{
   GET_IFACE(IBridge,                      pBridge);
   GET_IFACE(ILibrary,                     pLib);
   GET_IFACE(ISpecification,               pSpec);
   GET_IFACE(IBarriers,                    pBarriers);
   GET_IFACE(ILiveLoadDistributionFactors, pLLDF);
   GET_IFACE(IPointOfInterest,             pPoi);

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
   pDetails->Nb = pGroup->GetGirderCount();

   pDetails->gdrNum = spanKey.girderIndex;
   pDetails->bExteriorGirder = pGroup->IsExteriorGirder(spanKey.girderIndex);
   pDetails->Side = (spanKey.girderIndex <= pDetails->Nb/2) ? dfLeft : dfRight; // center goes left

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& live_load_distribution_criteria = pSpecEntry->GetLiveLoadDistributionCriteria();
   pDetails->Method = live_load_distribution_criteria.LldfMethod;

   Float64 dist_to_section_along_cl_span, curb_to_curb;
   Uint32 Nl = pLLDF->GetNumberOfDesignLanesEx(spanKey.spanIndex,&dist_to_section_along_cl_span,&curb_to_curb);
   pDetails->wCurbToCurb = curb_to_curb;
   pDetails->Nl = Nl;

   // Get sampling locations for values.
   Float64 span_length = pBridge->GetSpanLength(spanKey);

   Float64 span_fraction_for_girder_spacing = live_load_distribution_criteria.GirderSpacingLocation;
   Float64 loc1 = span_fraction_for_girder_spacing*span_length;
   Float64 loc2 = (1-span_fraction_for_girder_spacing)*span_length;

   Float64 ctrl_loc_from_gdr;
   if ( dist_to_section_along_cl_span <= pBridge->GetSpanLength(spanKey.spanIndex)/2 )
   {
      ctrl_loc_from_gdr = Min(loc1,loc2);
   }
   else
   {
      ctrl_loc_from_gdr = Max(loc1,loc2);
   }

   CSegmentKey segmentKey;
   Float64 Xs;
   pPoi->ConvertSpanPointToSegmentCoordiante(spanKey,ctrl_loc_from_gdr,&segmentKey,&Xs);

   pDetails->ControllingLocation = ctrl_loc_from_gdr;

   pgsPointOfInterest ctrl_poi = pPoi->GetPointOfInterest(segmentKey,Xs);

   if (!pPoi->IsOnSegment(ctrl_poi))
   {
#if defined _DEBUG
      CClosureKey closureKey;
      bool bIsInClosureJoint = pPoi->IsInClosureJoint(ctrl_poi, &closureKey);
      bool bIsInBoundaryPierDiaphragm = pPoi->IsInBoundaryPierDiaphragm(ctrl_poi);
      ATLASSERT(bIsInClosureJoint || bIsInBoundaryPierDiaphragm);
#endif

      Float64 Ls = pBridge->GetSegmentLength(segmentKey);
      ctrl_poi = pPoi->GetPointOfInterest(segmentKey, Ls);

      ATLASSERT(segmentKey == ctrl_poi.GetSegmentKey());
   }

   *pControllingPoi = ctrl_poi;

   // station of controlling loc
   Float64 ctrl_station, ctrl_offset;
   pBridge->GetStationAndOffset(ctrl_poi,&ctrl_station, &ctrl_offset);

   // IBridge functions need span distance measured from CL pier to our girder station
   PierIndexType pierIdx = spanKey.spanIndex;
   Float64 pier_station = pBridge->GetPierStation(pierIdx); // station of start pier

   Float64 Xspan = ctrl_station - pier_station;

   // Lane info
   pDetails->wLane = WBFL::LRFD::Utility::GetDesignLaneWidth( pDetails->wCurbToCurb );

   // overhangs
   if(IsOverlayDeck(pBridge->GetDeckType()))
   {
      // Use top width of beam as deck, for lack of better ideas
      GET_IFACE(IGirder,pGirder);
      Float64 wLeft, wRight;
      pGirder->GetTopWidth(pgsPointOfInterest(CSegmentKey(pGroup->GetIndex(), 0, segmentKey.segmentIndex),Xs),&wLeft,&wRight);
      pDetails->leftSlabOverhang = wLeft;
      pGirder->GetTopWidth(pgsPointOfInterest(CSegmentKey(pGroup->GetIndex(),pDetails->Nb-1,segmentKey.segmentIndex),Xs),&wLeft,&wRight);
      pDetails->rightSlabOverhang = wRight;
   }
   else
   {
      pDetails->leftSlabOverhang  = pBridge->GetLeftSlabOverhang(spanKey.spanIndex,Xspan);
      pDetails->rightSlabOverhang = pBridge->GetRightSlabOverhang(spanKey.spanIndex,Xspan);
   }

   pDetails->leftCurbOverhang  = pDetails->leftSlabOverhang  - pBarriers->GetInterfaceWidth(pgsTypes::tboLeft);
   pDetails->rightCurbOverhang = pDetails->rightSlabOverhang - pBarriers->GetInterfaceWidth(pgsTypes::tboRight);

   // Girder spacings
   ATLASSERT(pDetails->gdrSpacings.empty());
   Float64 tot_spc = 0.0;

   if (pDetails->Nb == 1)
   {
      pDetails->Savg = 0.0;
   }
   else
   {
      // Spacings normal to alignment
      std::vector<Float64> vSpacing = pBridge->GetGirderSpacing(spanKey.spanIndex,Xspan);
      pDetails->gdrSpacings = vSpacing;
      tot_spc = std::accumulate(vSpacing.begin(),vSpacing.end(),0.0);

      ATLASSERT(pDetails->Nb-1 == pDetails->gdrSpacings.size());

// Commented block below would give spacings normal to girder
//      for (Int16 ig=0; ig<(pDetails->Nb-1); ig++)
//      {
//         Float64 left_spacing, right_spacing;
//         pBridge->GetSpacingAlongGirder(span, ig, ctrl_loc, &left_spacing, &right_spacing);
//         pDetails->gdrSpacings.push_back(right_spacing);
//      }

      // Average will be average of adjacent spacings
      if (pDetails->gdrNum == 0)
      {
         pDetails->Savg = pDetails->gdrSpacings.front();
      }
      else if (pDetails->gdrNum == pDetails->Nb-1)
      {
         pDetails->Savg = pDetails->gdrSpacings.back();
      }
      else
      {
         pDetails->Savg = (pDetails->gdrSpacings[pDetails->gdrNum-1] + pDetails->gdrSpacings[pDetails->gdrNum])/2.0;
      }
   }

   // Total bridge width
   pDetails->W = tot_spc + pDetails->leftSlabOverhang + pDetails->rightSlabOverhang;

   // Skews
   // Determine span/pier index... This is the index of a pier and the next span.
   // If this is the last pier, span index is for the last span
   SpanIndexType curr_span = INVALID_INDEX;
   PierIndexType curr_pier = INVALID_INDEX;
   SpanIndexType prev_span = INVALID_INDEX;
   SpanIndexType next_span = INVALID_INDEX;
   PierIndexType prev_pier = INVALID_INDEX;
   PierIndexType next_pier = INVALID_INDEX;
   GetIndicies(spanKey.spanIndex,dfType,curr_span,curr_pier,prev_span,next_span,prev_pier,next_pier);

   CComPtr<IAngle> skew1, skew2;
   if ( dfType == dfSpan )
   {
      // if this is a span DF... use the skews at each end
      pBridge->GetPierSkew(prev_pier,&skew1);
      pBridge->GetPierSkew(next_pier,&skew2);
   }
   else
   {
      // if this is a pier DF... use the skew at this pier
      pBridge->GetPierSkew(curr_pier,&skew1);
      pBridge->GetPierSkew(curr_pier,&skew2);
   }

   skew1->get_Value(&(pDetails->skew1));
   skew2->get_Value(&(pDetails->skew2));
}

template <class T>
Float64 CDistFactorEngineerImpl<T>::GetEffectiveSpanLength(IndexType spanOrPierIdx,GirderIndexType gdrIdx,DFParam dfType)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // Determine span/pier index... This is the index of a pier and the next span.
   // If this is the last pier, span index is for the last span
   SpanIndexType nSpans = pBridgeDesc->GetSpanCount();
   PierIndexType nPiers = nSpans+1;

   SpanIndexType span = INVALID_INDEX;
   PierIndexType pier = INVALID_INDEX;
   SpanIndexType prev_span = INVALID_INDEX;
   SpanIndexType next_span = INVALID_INDEX;
   PierIndexType prev_pier = INVALID_INDEX;
   PierIndexType next_pier = INVALID_INDEX;
   GetIndicies(spanOrPierIdx,dfType,span,pier,prev_span,next_span,prev_pier,next_pier);

   const CSpanData2* pPrevSpan = pBridgeDesc->GetSpan(prev_span);
   const CSpanData2* pNextSpan = pBridgeDesc->GetSpan(next_span);
   const CGirderGroupData* pPrevGroup = pBridgeDesc->GetGirderGroup(pPrevSpan);
   const CGirderGroupData* pNextGroup = pBridgeDesc->GetGirderGroup(pNextSpan);

   // deal with girder index when there are different number of girders in each span
   GirderIndexType prev_span_gdr_idx = gdrIdx;
   GirderIndexType next_span_gdr_idx = gdrIdx;
   if ( prev_span != INVALID_INDEX && pPrevGroup->GetGirderCount() <= gdrIdx )
   {
      prev_span_gdr_idx = pPrevGroup->GetGirderCount()-1;
   }

   ATLASSERT(prev_span_gdr_idx != INVALID_INDEX);

   if ( next_span != INVALID_INDEX && pNextGroup->GetGirderCount() <= gdrIdx )
   {
      next_span_gdr_idx = pNextGroup->GetGirderCount()-1;
   }

   ATLASSERT(next_span_gdr_idx != INVALID_INDEX);

   Float64 L = 0;
   if ( dfType == dfSpan )
   {
      L = pBridge->GetSpanLength(span,gdrIdx);
   }
   else
   {
      bool bIntegralLeft, bIntegralRight;
      pBridge->IsIntegralAtPier(pier,&bIntegralLeft,&bIntegralRight);

      bool bContinuousLeft, bContinuousRight;
      pBridge->IsContinuousAtPier(pier,&bContinuousLeft,&bContinuousRight);

      Float64 l1, l2;
      if ( dfType == dfPierLeft )
      {
         l1 = pBridge->GetSpanLength(prev_span,prev_span_gdr_idx);

         if (next_span != ALL_SPANS && ((bIntegralLeft && bIntegralRight) || (bContinuousLeft && bContinuousRight)) )
         {
            l2 = pBridge->GetSpanLength(next_span,next_span_gdr_idx);
         }
         else
         {
            l2 = l1;
         }
      }
      else if ( dfType == dfPierRight )
      {
         l1 = pBridge->GetSpanLength(next_span,next_span_gdr_idx);

         if (prev_span != ALL_SPANS && ((bIntegralLeft && bIntegralRight) || (bContinuousLeft && bContinuousRight)) )
         {
            l2 = pBridge->GetSpanLength(prev_span,prev_span_gdr_idx);
         }
         else
         {
            l2 = l1;
         }
      }
      else
      {
         ATLASSERT(dfType == dfReaction);
         if ( prev_span != INVALID_INDEX && next_span != INVALID_INDEX )
         {
            l1 = pBridge->GetSpanLength(prev_span,prev_span_gdr_idx);
            l2 = pBridge->GetSpanLength(next_span,next_span_gdr_idx);
         }
         else
         {
            if ( prev_span != INVALID_INDEX )
            {
               l1 = pBridge->GetSpanLength(prev_span,prev_span_gdr_idx);
               l2 = l1;
            }
            else
            {
               l1 = pBridge->GetSpanLength(next_span,next_span_gdr_idx);
               l2 = l1;
            }
         }
      }

      // use the average span length (if l1 and l2 are equal, then the average is l2
      L = ( l1 + l2 )/2;
   }

   return L;
}

template <class T>
bool CDistFactorEngineerImpl<T>::Run1250Tests(const CSpanKey& spanKey,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);

      if ( pGroup->IsInteriorGirder(spanKey.girderIndex) )
      {
         Float64 M,V;
         M = pSpan->GetLLDFPosMoment(spanKey.girderIndex,ls);
         V = pSpan->GetLLDFShear(spanKey.girderIndex,ls);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12054, 0.0, ")<<M<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12055, 0.0, ")<<M<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12056, 0.0, ")<<V<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12057, 0.0, ")<<V<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12058, 0.0, ")<<M<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12059, 0.0, ")<<M<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
      }
      else
      {
         Float64 M,V;
         M = pSpan->GetLLDFPosMoment(spanKey.girderIndex,ls);
         V = pSpan->GetLLDFShear(spanKey.girderIndex,ls);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12024, 0.0, ")<<M<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12025, 0.0, ")<<M<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12026, 0.0, ")<<V<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12027, 0.0, ")<<V<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12028, 0.0, ")<<M<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12029, 0.0, ")<<M<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
      }
   }
   else
   {
      SPANDETAILS gdet;
      GetSpanDF(spanKey,ls,USE_CURRENT_FC,&gdet);

      if ( gdet.bExteriorGirder )
      {
         // exterior girder
         WBFL::LRFD::ILiveLoadDistributionFactor::DFResult mde1 = gdet.gM1;
         WBFL::LRFD::ILiveLoadDistributionFactor::DFResult mde2 = gdet.gM2;
         WBFL::LRFD::ILiveLoadDistributionFactor::DFResult sde1 = gdet.gV1;
         WBFL::LRFD::ILiveLoadDistributionFactor::DFResult sde2 = gdet.gV2;
         WBFL::LRFD::ILiveLoadDistributionFactor::DFResult dde1 = mde1;  // deflection same as moment
         WBFL::LRFD::ILiveLoadDistributionFactor::DFResult dde2 = mde2;

         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12024, 0.0, ")<<mde1.mg<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12025, 0.0, ")<<mde2.mg<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12026, 0.0, ")<<sde1.mg<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12027, 0.0, ")<<sde2.mg<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12028, 0.0, ")<<dde1.mg<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12029, 0.0, ")<<dde2.mg<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
      }
      else
      {
         // interior girder
         WBFL::LRFD::ILiveLoadDistributionFactor::DFResult mdi1 = gdet.gM1;
         WBFL::LRFD::ILiveLoadDistributionFactor::DFResult mdi2 = gdet.gM2;
         WBFL::LRFD::ILiveLoadDistributionFactor::DFResult sdi1 = gdet.gV1;
         WBFL::LRFD::ILiveLoadDistributionFactor::DFResult sdi2 = gdet.gV2; 
         WBFL::LRFD::ILiveLoadDistributionFactor::DFResult ddi1 = mdi1;
         WBFL::LRFD::ILiveLoadDistributionFactor::DFResult ddi2 = mdi2;

         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12054, 0.0, ")<<mdi1.mg<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12055, 0.0, ")<<mdi2.mg<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12056, 0.0, ")<<sdi1.mg<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12057, 0.0, ")<<sdi2.mg<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12058, 0.0, ")<<ddi1.mg<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12059, 0.0, ")<<ddi2.mg<<_T(", 2, ")<<spanKey.girderIndex<<std::endl;
      }
   }

   return true;
}
template <class T>
bool CDistFactorEngineerImpl<T>::GetDFResultsEx(const CSpanKey& spanKey,pgsTypes::LimitState ls,
                                                Float64* gpM, Float64* gpM1, Float64* gpM2,  // pos moment
                                                Float64* gnM, Float64* gnM1, Float64* gnM2,  // neg moment, ahead face
                                                Float64* gV,  Float64* gV1,  Float64* gV2 )   // shear
{
   PierIndexType pierIdx = spanKey.spanIndex;

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      const CSpanData2* pSpan = pBridgeDesc->GetSpan(spanKey.spanIndex);
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(pSpan);
      pgsTypes::GirderLocation gloc = pGroup->IsInteriorGirder(spanKey.girderIndex) ? pgsTypes::Interior : pgsTypes::Exterior;

      Float64 pM, nM, V;
      pM = pSpan->GetLLDFPosMoment(spanKey.girderIndex,ls);
      nM = pSpan->GetLLDFNegMoment(spanKey.girderIndex,ls);
      V  = pSpan->GetLLDFShear(spanKey.girderIndex,ls);
   
      *gpM  = pM;
      *gpM1 = pM;
      *gpM2 = pM;
      *gnM  = nM;
      *gnM1 = nM;
      *gnM2 = nM;
      *gV   = V;
      *gV1  = V;
      *gV2  = V;
   }
   else
   {
      SPANDETAILS gdet;
      GetSpanDF(spanKey,ls,USE_CURRENT_FC,&gdet);

      *gpM  = gdet.gM;
      *gpM1 = gdet.gM1.mg;
      *gpM2 = gdet.gM2.mg;
      *gV   = gdet.gV;
      *gV1  = gdet.gV1.mg;
      *gV2  = gdet.gV2.mg;

      PIERDETAILS pdet;
      GetPierDF(pierIdx,spanKey.girderIndex,ls,pgsTypes::Ahead,USE_CURRENT_FC,&pdet);

      *gnM  = pdet.gM;
      *gnM1 = pdet.gM1.mg;
      *gnM2 = pdet.gM2.mg;
   }

   return true;
}
template <class T>
Float64 CDistFactorEngineerImpl<T>::GetSkewCorrectionFactorForMoment(const CSpanKey& spanKey,pgsTypes::LimitState ls)
{
   SPANDETAILS lldf;
   GetSpanDF(spanKey,ls,-1,&lldf);
   return lldf.gMSkewCorrection;
}

template <class T>
Float64 CDistFactorEngineerImpl<T>::GetSkewCorrectionFactorForShear(const CSpanKey& spanKey,pgsTypes::LimitState ls)
{
   SPANDETAILS lldf;
   GetSpanDF(spanKey,ls,-1,&lldf);
   return lldf.gVSkewCorrection;
}

#endif // INCLUDED_DISTFACTORENGINEERIMPL_H_