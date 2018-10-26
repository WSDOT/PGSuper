///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include "..\PGSuperException.h"
#include <PgsExt\StatusItem.h>
#include <PgsExt\BridgeDescription.h>
#include <map>

// Side where overhang value was used if in equation
enum DfSide {dfLeft, dfRight};

// Details common for all beam types
struct BASE_LLDFDETAILS
{
   Int16 Method;
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

template <class T>
class CDistFactorEngineerImpl : public IDistFactorEngineer
{
public:
   virtual void SetBroker(IBroker* pBroker,StatusGroupIDType statusGroupID);
   virtual double GetMomentDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls);
   virtual double GetNegMomentDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace);
   virtual double GetShearDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls);
   virtual double GetReactionDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls);
   virtual double GetMomentDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr);
   virtual double GetNegMomentDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr);
   virtual double GetShearDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr);
   virtual double GetReactionDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr);
   virtual bool Run1250Tests(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile);
   virtual bool GetDFResultsEx(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,
                               Float64* gpM, Float64* gpM1, Float64* gpM2,  // pos moment
                               Float64* gnM, Float64* gnM1, Float64* gnM2,  // neg moment, ahead face
                               Float64* gV,  Float64* gV1,  Float64* gV2,   // shear
                               Float64* gR,  Float64* gR1,  Float64* gR2 );  // reaction

   enum DFParam { dfPierLeft, dfPierRight, dfSpan, dfReaction };

protected:
   IBroker* m_pBroker;
   StatusGroupIDType m_StatusGroupID;
   StatusCallbackIDType m_scidRefinedAnalysis;

   struct REACTIONDETAILS : T
   {
      // Distribution factors for reaction at pier
      lrfdILiveLoadDistributionFactor::DFResult gR1;
      lrfdILiveLoadDistributionFactor::DFResult gR2;
      Float64 gR;
   };

   struct PIERDETAILS : T
   {
      // Distribution factors for negative moment over pier
      lrfdILiveLoadDistributionFactor::DFResult gM1;
      lrfdILiveLoadDistributionFactor::DFResult gM2;
      Float64 gM;
   };

   struct SPANDETAILS : T
   {
      // Distribution factors for shear in the span
      lrfdILiveLoadDistributionFactor::DFResult gV1;
      lrfdILiveLoadDistributionFactor::DFResult gV2;
      Float64 gV;

      // Distribution factors for positive and negative moment in the span
      lrfdILiveLoadDistributionFactor::DFResult gM1;
      lrfdILiveLoadDistributionFactor::DFResult gM2;
      Float64 gM;
   };

   void GetIndicies(IndexType spanOrPier,DFParam dfType,SpanIndexType& span,PierIndexType& pier,SpanIndexType& prev_span,SpanIndexType& next_span,PierIndexType& prev_pier,PierIndexType& next_pier);

   std::map<PierGirderHashType,PIERDETAILS> m_PierLLDF[2][2]; // first index is pier face, second index is limit state type
   void GetPierDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr,PIERDETAILS* plldf);

   std::map<PierGirderHashType,REACTIONDETAILS> m_ReactionLLDF[2]; // index is limit state type
   void GetPierReactionDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr,REACTIONDETAILS* plldf);

   std::map<SpanGirderHashType,SPANDETAILS> m_SpanLLDF[2]; // index is limit state type
   void GetSpanDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr,SPANDETAILS* plldf);

   Float64 GetEffectiveSpanLength(IndexType spanOrPier,GirderIndexType gdr,DFParam dfType);
   void GetGirderSpacingAndOverhang(SpanIndexType span,GirderIndexType gdr,DFParam dfType,BASE_LLDFDETAILS* pDetails);

   virtual lrfdLiveLoadDistributionFactorBase* GetLLDFParameters(IndexType spanOrPier,GirderIndexType gdr,DFParam dfType,Float64 fcgdr,T* plldf) = 0;

   void HandleRangeOfApplicabilityError(const lrfdXRangeOfApplicability& e);

   lrfdTypes::LimitState MapLimitState(pgsTypes::LimitState ls);
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
double CDistFactorEngineerImpl<T>::GetMomentDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls)
{
   SPANDETAILS lldf;
   GetSpanDF(span,gdr,ls,-1,&lldf);
   return lldf.gM;
}

template <class T>
double CDistFactorEngineerImpl<T>::GetMomentDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr)
{
   SPANDETAILS lldf;
   GetSpanDF(span,gdr,ls,fcgdr,&lldf);
   return lldf.gM;
}

template <class T>
double CDistFactorEngineerImpl<T>::GetNegMomentDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace)
{
   PIERDETAILS lldf;
   GetPierDF(pier,gdr,ls,pierFace,-1,&lldf);
   return lldf.gM;
}

template <class T>
double CDistFactorEngineerImpl<T>::GetNegMomentDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr)
{
   PIERDETAILS lldf;
   GetPierDF(pier,gdr,ls,pierFace,fcgdr,&lldf);
   return lldf.gM;
}

template <class T>
double CDistFactorEngineerImpl<T>::GetShearDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls)
{
   SPANDETAILS lldf;
   GetSpanDF(span,gdr,ls,-1,&lldf);
   return lldf.gV;
}

template <class T>
double CDistFactorEngineerImpl<T>::GetShearDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr)
{
   SPANDETAILS lldf;
   GetSpanDF(span,gdr,ls,fcgdr,&lldf);
   return lldf.gV;
}

template <class T>
double CDistFactorEngineerImpl<T>::GetReactionDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls)
{
   REACTIONDETAILS lldf;
   GetPierReactionDF(pier,gdr,ls,-1,&lldf);
   return lldf.gR;
}

template <class T>
double CDistFactorEngineerImpl<T>::GetReactionDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr)
{
   REACTIONDETAILS lldf;
   GetPierReactionDF(pier,gdr,ls,fcgdr,&lldf);
   return lldf.gR;
}


template <class T>
void CDistFactorEngineerImpl<T>::GetPierReactionDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr,REACTIONDETAILS* plldf)
{
   std::map<PierGirderHashType,REACTIONDETAILS>::iterator found;
   found = m_ReactionLLDF[LimitStateType(ls)].find(HashPierGirder(pier,gdr));
   if ( found != m_ReactionLLDF[LimitStateType(ls)].end() && fcgdr < 0 )
   {
      *plldf = (*found).second;
      return; // We already have the distribution factors for this girder
   }

   std::auto_ptr<lrfdLiveLoadDistributionFactorBase> pLLDF( GetLLDFParameters(pier,gdr,dfReaction,fcgdr,plldf) );

   // get method used to compute factors, may be lever override
   GET_IFACE(IBridgeDescription,pBridgeDesc);
   pgsTypes::DistributionFactorMethod df_method = pBridgeDesc->GetBridgeDescription()->GetDistributionFactorMethod();

   lrfdILiveLoadDistributionFactor::Location loc;
   loc =  plldf->bExteriorGirder ? lrfdILiveLoadDistributionFactor::ExtGirder : lrfdILiveLoadDistributionFactor::IntGirder;

   try
   {
      if (df_method == pgsTypes::Calculated)
      {
         // Reaction distribution factor
         plldf->gR1 = pLLDF->ReactionDFEx(loc,
                              lrfdILiveLoadDistributionFactor::OneLoadedLane,
                              MapLimitState(ls));

         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI  )
         {
            plldf->gR2 = pLLDF->ReactionDFEx(loc,
                                 lrfdILiveLoadDistributionFactor::TwoOrMoreLoadedLanes,
                                 MapLimitState(ls));
         }
         else
         {
            plldf->gR2.mg = 0;
         }
      }
      else if (df_method == pgsTypes::LeverRule)
      {
         // Reaction distribution factor
         plldf->gR1 = pLLDF->DistributeReactionByLeverRule(loc, lrfdILiveLoadDistributionFactor::OneLoadedLane);

         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
         {
            plldf->gR2 = pLLDF->DistributeReactionByLeverRule(loc, lrfdILiveLoadDistributionFactor::TwoOrMoreLoadedLanes);
         }
         else
         {
            plldf->gR2.mg = 0;
         }
      }
      else
      {
         ATLASSERT(0);
      }

      // see if we need to compare with lanes/beams
      GET_IFACE(ISpecification, pSpec);
      GET_IFACE(ILibrary, pLib);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

      if (pSpecEntry->LimitDistributionFactorsToLanesBeams())
      {
         // Compare results with lanes/beams and override if needed
         lrfdILiveLoadDistributionFactor::DFResult glb1 = pLLDF->GetLanesBeamsMethod(1,Int16(plldf->Nb));
         lrfdILiveLoadDistributionFactor::DFResult glb2 = pLLDF->GetLanesBeamsMethod(plldf->Nl,Int16(plldf->Nb));

         // Reaction
         if (plldf->gR1.mg < glb1.mg)
         {
            plldf->gR1.mg = glb1.mg;
            plldf->gR1.ControllingMethod = LANES_DIV_BEAMS | LANES_BEAMS_OVERRIDE;
            plldf->gR1.LanesBeamsData = glb1.LanesBeamsData;
         }

         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
         {
            if (plldf->gR2.mg < glb2.mg)
            {
               plldf->gR2.mg = glb2.mg;
               plldf->gR2.ControllingMethod = LANES_DIV_BEAMS | LANES_BEAMS_OVERRIDE;
               plldf->gR2.LanesBeamsData = glb2.LanesBeamsData;
            }
         }
      }

      // controlling
      if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
      {
         plldf->gR  = max(plldf->gR1.mg,plldf->gR2.mg);
      }
      else
      {
         plldf->gR = plldf->gR1.mg;
      }

   }
   catch( const lrfdXRangeOfApplicability& e)
   {
      HandleRangeOfApplicabilityError(e);
   }

   m_ReactionLLDF[LimitStateType(ls)].insert( std::make_pair(HashPierGirder(pier,gdr),*plldf) );
}

template <class T>
void CDistFactorEngineerImpl<T>::GetPierDF(PierIndexType pier,GirderIndexType gdr,pgsTypes::LimitState ls,pgsTypes::PierFaceType pierFace,Float64 fcgdr,PIERDETAILS* plldf)
{
   std::map<PierGirderHashType,PIERDETAILS>::iterator found;
   found = m_PierLLDF[pierFace][LimitStateType(ls)].find(HashPierGirder(pier,gdr));
   if ( found != m_PierLLDF[pierFace][LimitStateType(ls)].end() && fcgdr < 0 )
   {
      *plldf = (*found).second;
      return; // We already have the distribution factors for this girder
   }

   DFParam dfParam = (pierFace == pgsTypes::Back ? dfPierLeft : dfPierRight);
   std::auto_ptr<lrfdLiveLoadDistributionFactorBase> pLLDF( GetLLDFParameters(pier,gdr,dfParam,fcgdr,plldf) );

   // get method used to compute factors, may be lever override
   GET_IFACE(IBridgeDescription,pBridgeDesc);
   pgsTypes::DistributionFactorMethod df_method = pBridgeDesc->GetBridgeDescription()->GetDistributionFactorMethod();

   lrfdILiveLoadDistributionFactor::Location loc;
   loc =  plldf->bExteriorGirder ? lrfdILiveLoadDistributionFactor::ExtGirder : lrfdILiveLoadDistributionFactor::IntGirder;

   try
   {
      // Negative moment distribution factor
      if (df_method == pgsTypes::Calculated)
      {
         plldf->gM1 = pLLDF->MomentDFEx(loc,
                      lrfdILiveLoadDistributionFactor::OneLoadedLane,
                      MapLimitState(ls));

         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
         {
            plldf->gM2 = pLLDF->MomentDFEx(loc,
                         lrfdILiveLoadDistributionFactor::TwoOrMoreLoadedLanes,
                         MapLimitState(ls));
         }
         else
         {
            plldf->gM2.mg = 0;
         }
      }
      else if (df_method == pgsTypes::LeverRule)
      {
         plldf->gM1 = pLLDF->DistributeMomentByLeverRule(loc, lrfdILiveLoadDistributionFactor::OneLoadedLane);

         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
         {
            plldf->gM2 = pLLDF->DistributeMomentByLeverRule(loc, lrfdILiveLoadDistributionFactor::TwoOrMoreLoadedLanes);
         }
         else
         {
            plldf->gM2.mg = 0;
         }
      }
      else
      {
         ATLASSERT(0);
      }

      // see if we need to compare with lanes/beams
      GET_IFACE(ISpecification, pSpec);
      GET_IFACE(ILibrary, pLib);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

      if (pSpecEntry->LimitDistributionFactorsToLanesBeams())
      {
         // Compare results with lanes/beams and override if needed
         lrfdILiveLoadDistributionFactor::DFResult glb1 = pLLDF->GetLanesBeamsMethod(1,Int16(plldf->Nb));
         lrfdILiveLoadDistributionFactor::DFResult glb2 = pLLDF->GetLanesBeamsMethod(plldf->Nl,Int16(plldf->Nb));

         // Moment
         if (plldf->gM1.mg < glb1.mg)
         {
            plldf->gM1.mg = glb1.mg;
            plldf->gM1.ControllingMethod = LANES_DIV_BEAMS | LANES_BEAMS_OVERRIDE;
            plldf->gM1.LanesBeamsData = glb1.LanesBeamsData;
         }

         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
         {
            if (plldf->gM2.mg < glb2.mg)
            {
               plldf->gM2.mg = glb2.mg;
               plldf->gM2.ControllingMethod = LANES_DIV_BEAMS | LANES_BEAMS_OVERRIDE;
               plldf->gM2.LanesBeamsData = glb2.LanesBeamsData;
            }
         }
      }

      // controlling
      if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
      {
         plldf->gM  = max(plldf->gM1.mg,plldf->gM2.mg);
      }
      else
      {
         plldf->gM = plldf->gM1.mg;
      }
   }
   catch( const lrfdXRangeOfApplicability& e)
   {
      HandleRangeOfApplicabilityError(e);
   }

   m_PierLLDF[pierFace][LimitStateType(ls)].insert( std::make_pair(HashPierGirder(pier,gdr),*plldf) );
}


template <class T>
void CDistFactorEngineerImpl<T>::GetSpanDF(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,Float64 fcgdr,SPANDETAILS* plldf)
{
   std::map<SpanGirderHashType,SPANDETAILS>::iterator found;
   found = m_SpanLLDF[LimitStateType(ls)].find(HashSpanGirder(span,gdr));
   if ( found != m_SpanLLDF[LimitStateType(ls)].end() && fcgdr < 0 )
   {
      *plldf = (*found).second;
      return; // We already have the distribution factors for this girder
   }

   // get method used to compute factors, may be lever override
   GET_IFACE(IBridgeDescription,pBridgeDesc);
   pgsTypes::DistributionFactorMethod df_method = pBridgeDesc->GetBridgeDescription()->GetDistributionFactorMethod();

   std::auto_ptr<lrfdLiveLoadDistributionFactorBase> pLLDF( GetLLDFParameters(span,gdr,dfSpan,fcgdr,plldf) );

   lrfdILiveLoadDistributionFactor::Location loc;
   loc =  plldf->bExteriorGirder ? lrfdILiveLoadDistributionFactor::ExtGirder : lrfdILiveLoadDistributionFactor::IntGirder;

   try
   {
      if (df_method == pgsTypes::Calculated)
      {
         plldf->gM1 = pLLDF->MomentDFEx(loc,
                            lrfdILiveLoadDistributionFactor::OneLoadedLane,
                            MapLimitState(ls));

         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI)
         {
            plldf->gM2 = pLLDF->MomentDFEx(loc,
                               lrfdILiveLoadDistributionFactor::TwoOrMoreLoadedLanes,
                               MapLimitState(ls));
         }
         else
         {
            plldf->gM2.mg = 0;
         }


         plldf->gV1 = pLLDF->ShearDFEx(loc,
                           lrfdILiveLoadDistributionFactor::OneLoadedLane,
                           MapLimitState(ls));

         if ( 2 <= plldf->Nl && ls != pgsTypes::FatigueI)
         {
            plldf->gV2 = pLLDF->ShearDFEx(loc,
                              lrfdILiveLoadDistributionFactor::TwoOrMoreLoadedLanes,
                              MapLimitState(ls));
         }
         else
         {
            plldf->gV2.mg = 0;
         }
      }
      else if (df_method == pgsTypes::LeverRule)
      {
         plldf->gM1 = pLLDF->DistributeMomentByLeverRule(loc, lrfdILiveLoadDistributionFactor::OneLoadedLane);
         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
         {
            plldf->gM2 = pLLDF->DistributeMomentByLeverRule(loc, lrfdILiveLoadDistributionFactor::TwoOrMoreLoadedLanes);
         }
         else
         {
            plldf->gM2.mg = 0;
         }


         plldf->gV1 = pLLDF->DistributeShearByLeverRule(loc, lrfdILiveLoadDistributionFactor::OneLoadedLane);

         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
         {
            plldf->gV2 = pLLDF->DistributeShearByLeverRule(loc, lrfdILiveLoadDistributionFactor::TwoOrMoreLoadedLanes);
         }
         else
         {
            plldf->gV2.mg = 0;
         }
      }
      else
      {
         ATLASSERT(0); // should not be here?
      }

      // see if we need to compare with lanes/beams
      GET_IFACE(ISpecification, pSpec);
      GET_IFACE(ILibrary, pLib);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

      if (pSpecEntry->LimitDistributionFactorsToLanesBeams())
      {
         // Compare results with lanes/beams and override if needed
         lrfdILiveLoadDistributionFactor::DFResult glb1 = pLLDF->GetLanesBeamsMethod(1,Int16(plldf->Nb));
         lrfdILiveLoadDistributionFactor::DFResult glb2 = pLLDF->GetLanesBeamsMethod(plldf->Nl,Int16(plldf->Nb));

         // Moment
         if ( plldf->gM1.mg < glb1.mg)
         {
            plldf->gM1.mg = glb1.mg;
            plldf->gM1.ControllingMethod = LANES_DIV_BEAMS | LANES_BEAMS_OVERRIDE;
            plldf->gM1.LanesBeamsData = glb1.LanesBeamsData;
         }

         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
         {
            if (plldf->gM2.mg < glb2.mg)
            {
               plldf->gM2.mg = glb2.mg;
               plldf->gM2.ControllingMethod = LANES_DIV_BEAMS | LANES_BEAMS_OVERRIDE;
               plldf->gM2.LanesBeamsData = glb2.LanesBeamsData;
            }
         }

         // Shear
         if (plldf->gV1.mg < glb1.mg)
         {
            plldf->gV1.mg = glb1.mg;
            plldf->gV1.ControllingMethod = LANES_DIV_BEAMS | LANES_BEAMS_OVERRIDE;
            plldf->gV1.LanesBeamsData = glb1.LanesBeamsData;
         }

         if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
         {
            if (plldf->gV2.mg < glb2.mg)
            {
               plldf->gV2.mg = glb2.mg;
               plldf->gV2.ControllingMethod = LANES_DIV_BEAMS | LANES_BEAMS_OVERRIDE;
               plldf->gV2.LanesBeamsData = glb2.LanesBeamsData;
            }
         }
      }

      // controlling
      if ( 2 <= plldf->Nl  && ls != pgsTypes::FatigueI )
      {
         plldf->gM  = max(plldf->gM1.mg,plldf->gM2.mg);
         plldf->gV  = max(plldf->gV1.mg,plldf->gV2.mg);
      }
      else
      {
         plldf->gM = plldf->gM1.mg;
         plldf->gV = plldf->gV1.mg;
      }

   }
   catch( const lrfdXRangeOfApplicability& e)
   {
      HandleRangeOfApplicabilityError(e);
   }

   m_SpanLLDF[LimitStateType(ls)].insert( std::make_pair(HashSpanGirder(span,gdr),*plldf) );
}

template <class T>
void CDistFactorEngineerImpl<T>::HandleRangeOfApplicabilityError(const lrfdXRangeOfApplicability& e)
{
   GET_IFACE(IEAFStatusCenter,     pStatusCenter);

   LPCTSTR msg = _T("Live Load Distribution Factors could not be calculated");
   pgsRefinedAnalysisStatusItem* pStatusItem = new pgsRefinedAnalysisStatusItem(m_StatusGroupID,m_scidRefinedAnalysis,msg);
   pStatusCenter->Add(pStatusItem);

   std::_tostringstream os;
   std::_tstring errmsg;
   e.GetErrorMessage( &errmsg );
   os << _T("Live Load Distribution Factors could not be calculated for the following reason") << std::endl;
   os << errmsg << std::endl;
   os << _T("A refined method of analysis is required for this bridge") << std::endl;
   os << _T("See Status Center for Details");
   THROW_UNWIND(os.str().c_str(),XREASON_REFINEDANALYSISREQUIRED);
}

template <class T>
lrfdTypes::LimitState CDistFactorEngineerImpl<T>::MapLimitState(pgsTypes::LimitState ls)
{
   lrfdTypes::LimitState lrfdLimitState;
   switch(ls)
   {
   case pgsTypes::ServiceI:      lrfdLimitState = lrfdTypes::ServiceI;     break;
   case pgsTypes::ServiceIA:     lrfdLimitState = lrfdTypes::ServiceIA;    break;
   case pgsTypes::ServiceIII:    lrfdLimitState = lrfdTypes::ServiceIII;   break;
   case pgsTypes::StrengthI:     lrfdLimitState = lrfdTypes::StrengthI;    break;
   case pgsTypes::StrengthII:    lrfdLimitState = lrfdTypes::StrengthII;   break;
   case pgsTypes::FatigueI:      lrfdLimitState = lrfdTypes::FatigueI;     break;
   default:
      ATLASSERT(false); // should never get here
   }

   return lrfdLimitState;
}

template <class T>
int CDistFactorEngineerImpl<T>::LimitStateType(pgsTypes::LimitState ls)
{
   // maps specific limit state to a general limit state type
   // strength and service limit states, return 0
   // fatigue limit state, return 1
   if ( IsStrengthLimitState(ls) || IsServiceLimitState(ls) )
      return 0;
   else if ( IsFatigueLimitState(ls) )
      return 1;
   else
   {
      ATLASSERT(false); // should never get here
      return 0;
   }
}

template <class T>
void CDistFactorEngineerImpl<T>::GetIndicies(IndexType spanOrPier,DFParam dfType,SpanIndexType& span,PierIndexType& pier,SpanIndexType& prev_span,SpanIndexType& next_span,PierIndexType& prev_pier,PierIndexType& next_pier)
{
   GET_IFACE(IBridge,pBridge);

   SpanIndexType nSpans = pBridge->GetSpanCount();
   PierIndexType nPiers = pBridge->GetPierCount();

   if ( dfType == dfSpan )
   {
      span = (SpanIndexType)spanOrPier;

      prev_pier = span;
      next_pier = prev_pier + 1;
   }
   else
   {
      pier = (PierIndexType)spanOrPier;
      prev_span = (pier == 0 ? INVALID_INDEX : pier-1);
      next_span = (pier == nPiers-1 ? INVALID_INDEX : prev_span + 1);

      span = (dfType == dfPierLeft ? prev_span : next_span);
      if ( span == INVALID_INDEX )
         span = prev_span;
   }
}

template <class T>
void CDistFactorEngineerImpl<T>::GetGirderSpacingAndOverhang(SpanIndexType span,GirderIndexType gdr,DFParam dfType,BASE_LLDFDETAILS* pDetails)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder,pGirder);
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   GET_IFACE(IBarriers, pBarriers);
   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);

   pDetails->Nb = pBridge->GetGirderCount(span);

   pDetails->gdrNum = gdr;
   pDetails->bExteriorGirder = pBridge->IsExteriorGirder(span,gdr);
   pDetails->Side = (gdr <= pDetails->Nb/2) ? dfLeft : dfRight; // center goes left

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   pDetails->Method = pSpecEntry->GetLiveLoadDistributionMethod();

   Float64 dist_to_section_along_cl_span, curb_to_curb;
   Uint32 Nl = pLLDF->GetNumberOfDesignLanesEx(span,&dist_to_section_along_cl_span,&curb_to_curb);
   pDetails->wCurbToCurb = curb_to_curb;
   pDetails->Nl = Nl;

   // Get sampling locations for values. This will occur at max curb width
   Float64 span_length = pBridge->GetSpanLength(span,gdr);

   Float64 span_fraction_for_girder_spacing = pSpecEntry->GetLLDFGirderSpacingLocation();
   Float64 loc1 = span_fraction_for_girder_spacing*span_length;
   Float64 loc2 = (1-span_fraction_for_girder_spacing)*span_length;

   Float64 ctrl_loc_from_gdr;
   if ( dist_to_section_along_cl_span <= pBridge->GetSpanLength(span)/2 )
      ctrl_loc_from_gdr = min(loc1,loc2);
   else
      ctrl_loc_from_gdr = max(loc1,loc2);

   ctrl_loc_from_gdr += pBridge->GetGirderStartConnectionLength(span,gdr);

   pgsPointOfInterest ctrl_poi(span,gdr,ctrl_loc_from_gdr);

   // station of controlling loc
   Float64 ctrl_station, ctrl_offset;
   pBridge->GetStationAndOffset(ctrl_poi,&ctrl_station, &ctrl_offset);

   // IBridge functions need span distance measured from CL pier to our girder station
   Float64 pier_station = pBridge->GetPierStation(span); // station of start pier

   Float64 ctrl_pier_span_loc = ctrl_station - pier_station;

   // Lane info
   pDetails->wLane = lrfdUtility::GetDesignLaneWidth( pDetails->wCurbToCurb );

   // overhangs
   if(pBridge->GetDeckType() == pgsTypes::sdtNone)
   {
      // Use top width of beam as deck, for lack of better ideas
      pDetails->leftSlabOverhang  = pGirder->GetTopWidth(pgsPointOfInterest(span,0,ctrl_loc_from_gdr))/2.0;
      pDetails->rightSlabOverhang = pGirder->GetTopWidth(pgsPointOfInterest(span,pDetails->Nb-1,ctrl_loc_from_gdr))/2.0;
   }
   else
   {
      pDetails->leftSlabOverhang  = pBridge->GetLeftSlabGirderOverhang(span,ctrl_pier_span_loc);
      pDetails->rightSlabOverhang = pBridge->GetRightSlabGirderOverhang(span,ctrl_pier_span_loc);
   }

   pDetails->leftCurbOverhang  = pDetails->leftSlabOverhang  - pBarriers->GetInterfaceWidth(pgsTypes::tboLeft);
   pDetails->rightCurbOverhang = pDetails->rightSlabOverhang - pBarriers->GetInterfaceWidth(pgsTypes::tboRight);

   // Girder spacings
   ATLASSERT(pDetails->gdrSpacings.empty());
   Float64 tot_spc=0.0;

   if (pDetails->Nb==1)
   {
      pDetails->Savg = 0.0;
   }
   else
   {
      // Spacings normal to alignment
      std::vector<SpaceBetweenGirder> vSpacing = pBridge->GetGirderSpacing(span, ctrl_pier_span_loc);
      for (std::vector<SpaceBetweenGirder>::iterator itsg=vSpacing.begin(); itsg!=vSpacing.end(); itsg++)
      {
         const SpaceBetweenGirder& rspace = *itsg;
         for (GirderIndexType ispc = rspace.firstGdrIdx; ispc < rspace.lastGdrIdx; ispc++)
         {
            pDetails->gdrSpacings.push_back(rspace.spacing);
            tot_spc+=rspace.spacing;
         }
      }

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
   GetIndicies(span,dfType,curr_span,curr_pier,prev_span,next_span,prev_pier,next_pier);

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

   pDetails->ControllingLocation = ctrl_loc_from_gdr;
}

template <class T>
double CDistFactorEngineerImpl<T>::GetEffectiveSpanLength(IndexType spanOrPier,GirderIndexType gdr,DFParam dfType)
{
   GET_IFACE(IBridge,pBridge);

   // Determine span/pier index... This is the index of a pier and the next span.
   // If this is the last pier, span index is for the last span
   SpanIndexType nSpans = pBridge->GetSpanCount();
   PierIndexType nPiers = nSpans+1;

   SpanIndexType span = INVALID_INDEX;
   PierIndexType pier = INVALID_INDEX;
   SpanIndexType prev_span = INVALID_INDEX;
   SpanIndexType next_span = INVALID_INDEX;
   PierIndexType prev_pier = INVALID_INDEX;
   PierIndexType next_pier = INVALID_INDEX;
   GetIndicies(spanOrPier,dfType,span,pier,prev_span,next_span,prev_pier,next_pier);

   // deal with girder index when there are different number of girders in each span
   GirderIndexType prev_span_gdr_idx = gdr;
   GirderIndexType next_span_gdr_idx = gdr;
   if ( prev_span != ALL_SPANS && pBridge->GetGirderCount(prev_span) <= gdr )
      prev_span_gdr_idx = pBridge->GetGirderCount(prev_span)-1;

   ATLASSERT(prev_span_gdr_idx != ALL_GIRDERS);

   if ( next_span != ALL_SPANS && pBridge->GetGirderCount(next_span) <= gdr )
      next_span_gdr_idx = pBridge->GetGirderCount(next_span)-1;

   ATLASSERT(next_span_gdr_idx != ALL_GIRDERS);

   double L = 0;
   if ( dfType == dfSpan )
   {
      L = pBridge->GetSpanLength(span,gdr);
   }
   else
   {
      bool bIntegralLeft, bIntegralRight;
      pBridge->IsIntegralAtPier(pier,&bIntegralLeft,&bIntegralRight);

      bool bContinuousLeft, bContinuousRight;
      pBridge->IsContinuousAtPier(pier,&bContinuousLeft,&bContinuousRight);

      double l1, l2;
      if ( dfType == dfPierLeft )
      {
         l1 = pBridge->GetSpanLength(prev_span,prev_span_gdr_idx);

         if (next_span != ALL_SPANS && ((bIntegralLeft && bIntegralRight) || (bContinuousLeft && bContinuousRight)) )
            l2 = pBridge->GetSpanLength(next_span,next_span_gdr_idx);
         else
            l2 = l1;
      }
      else if ( dfType == dfPierRight )
      {
         l1 = pBridge->GetSpanLength(next_span,next_span_gdr_idx);

         if (prev_span != ALL_SPANS && ((bIntegralLeft && bIntegralRight) || (bContinuousLeft && bContinuousRight)) )
            l2 = pBridge->GetSpanLength(prev_span,prev_span_gdr_idx);
         else
            l2 = l1;
      }
      else
      {
         ATLASSERT(dfType == dfReaction);
         if ( prev_span != ALL_SPANS && next_span != ALL_SPANS )
         {
            l1 = pBridge->GetSpanLength(prev_span,prev_span_gdr_idx);
            l2 = pBridge->GetSpanLength(next_span,next_span_gdr_idx);
         }
         else
         {
            if ( prev_span != ALL_SPANS )
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
bool CDistFactorEngineerImpl<T>::Run1250Tests(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,LPCTSTR pid,LPCTSTR bridgeId,std::_tofstream& resultsFile, std::_tofstream& poiFile)
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {
      const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
      if ( !pSpan->IsInteriorGirder(gdr) )
      {
         double M,V;
         M = pSpan->GetLLDFPosMoment(gdr,ls);
         V = pSpan->GetLLDFShear(gdr,ls);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12054, 0.0, ")<<M<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12055, 0.0, ")<<M<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12056, 0.0, ")<<V<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12057, 0.0, ")<<V<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12058, 0.0, ")<<M<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12059, 0.0, ")<<M<<_T(", 2, ")<<gdr<<std::endl;
      }
      else
      {
         double M,V;
         M = pSpan->GetLLDFPosMoment(gdr,ls);
         V = pSpan->GetLLDFShear(gdr,ls);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12024, 0.0, ")<<M<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12025, 0.0, ")<<M<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12026, 0.0, ")<<V<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12027, 0.0, ")<<V<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12028, 0.0, ")<<M<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12029, 0.0, ")<<M<<_T(", 2, ")<<gdr<<std::endl;
      }
   }
   else
   {
      SPANDETAILS gdet;
      GetSpanDF(span,gdr,ls,-1,&gdet);

      if ( gdet.bExteriorGirder )
      {
         // exterior girder
         lrfdILiveLoadDistributionFactor::DFResult mde1 = gdet.gM1;
         lrfdILiveLoadDistributionFactor::DFResult mde2 = gdet.gM2;
         lrfdILiveLoadDistributionFactor::DFResult sde1 = gdet.gV1;
         lrfdILiveLoadDistributionFactor::DFResult sde2 = gdet.gV2;
         lrfdILiveLoadDistributionFactor::DFResult dde1 = mde1;  // deflection same as moment
         lrfdILiveLoadDistributionFactor::DFResult dde2 = mde2;

         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12024, 0.0, ")<<mde1.mg<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12025, 0.0, ")<<mde2.mg<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12026, 0.0, ")<<sde1.mg<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12027, 0.0, ")<<sde2.mg<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12028, 0.0, ")<<dde1.mg<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12029, 0.0, ")<<dde2.mg<<_T(", 2, ")<<gdr<<std::endl;
      }
      else
      {
         // interior girder
         lrfdILiveLoadDistributionFactor::DFResult mdi1 = gdet.gM1;
         lrfdILiveLoadDistributionFactor::DFResult mdi2 = gdet.gM2;
         lrfdILiveLoadDistributionFactor::DFResult sdi1 = gdet.gV1;
         lrfdILiveLoadDistributionFactor::DFResult sdi2 = gdet.gV2; 
         lrfdILiveLoadDistributionFactor::DFResult ddi1 = mdi1;
         lrfdILiveLoadDistributionFactor::DFResult ddi2 = mdi2;

         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12054, 0.0, ")<<mdi1.mg<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12055, 0.0, ")<<mdi2.mg<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12056, 0.0, ")<<sdi1.mg<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12057, 0.0, ")<<sdi2.mg<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12058, 0.0, ")<<ddi1.mg<<_T(", 2, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 12059, 0.0, ")<<ddi2.mg<<_T(", 2, ")<<gdr<<std::endl;
      }
   }

   return true;
}
template <class T>
bool CDistFactorEngineerImpl<T>::GetDFResultsEx(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,
                                                Float64* gpM, Float64* gpM1, Float64* gpM2,  // pos moment
                                                Float64* gnM, Float64* gnM1, Float64* gnM2,  // neg moment, ahead face
                                                Float64* gV,  Float64* gV1,  Float64* gV2,   // shear
                                                Float64* gR,  Float64* gR1,  Float64* gR2 )  // reaction
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if ( pBridgeDesc->GetDistributionFactorMethod() == pgsTypes::DirectlyInput )
   {

      const CSpanData* pSpan = pBridgeDesc->GetSpan(span);

      double pM, nM, V;
      pM = pSpan->GetLLDFPosMoment(gdr,ls);
      nM = pSpan->GetLLDFNegMoment(gdr,ls);
      V  = pSpan->GetLLDFShear(gdr,ls);
   
      *gpM  = pM;
      *gpM1 = pM;
      *gpM2 = pM;
      *gnM  = nM;
      *gnM1 = nM;
      *gnM2 = nM;
      *gV   = V;
      *gV1  = V;
      *gV2  = V;
      *gR   = V;
      *gR1  = V;
      *gR2  = V;
   }
   else
   {
      SPANDETAILS gdet;
      GetSpanDF(span,gdr,ls,-1,&gdet);

      *gpM  = gdet.gM;
      *gpM1 = gdet.gM1.mg;
      *gpM2 = gdet.gM2.mg;
      *gV   = gdet.gV;
      *gV1  = gdet.gV1.mg;
      *gV2  = gdet.gV2.mg;

      PIERDETAILS pdet;
      GetPierDF(span,gdr,ls,pgsTypes::Ahead,-1,&pdet);

      *gnM  = pdet.gM;
      *gnM1 = pdet.gM1.mg;
      *gnM2 = pdet.gM2.mg;

      REACTIONDETAILS rdet;
      GetPierReactionDF(span,gdr,ls,-1,&rdet);

      *gR   = rdet.gR;
      *gR1  = rdet.gR1.mg;
      *gR2  = rdet.gR2.mg;
   }

   return true;
}

#endif // INCLUDED_DISTFACTORENGINEERIMPL_H_