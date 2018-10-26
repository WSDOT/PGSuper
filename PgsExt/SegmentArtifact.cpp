///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#include <PgsExt\SegmentArtifact.h>
#include <EAF\EAFUtilities.h>
#include <IFace\Intervals.h>
#include <IFace\PointOfInterest.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

pgsSegmentArtifact::pgsSegmentArtifact(const CSegmentKey& segmentKey) :
m_SegmentKey(segmentKey)
{
   m_pLiftingCheckArtifact = nullptr;
   m_pHaulingAnalysisArtifact = nullptr;
}

pgsSegmentArtifact::pgsSegmentArtifact(const pgsSegmentArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsSegmentArtifact::~pgsSegmentArtifact()
{
}

pgsSegmentArtifact& pgsSegmentArtifact::operator= (const pgsSegmentArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

bool pgsSegmentArtifact::operator<(const pgsSegmentArtifact& rOther) const
{
   return m_SegmentKey < rOther.m_SegmentKey;
}

void pgsSegmentArtifact::SetStrandStressArtifact(const pgsStrandStressArtifact& artifact)
{
   m_StrandStressArtifact = artifact;
}

const pgsStrandStressArtifact* pgsSegmentArtifact::GetStrandStressArtifact() const
{
   return &m_StrandStressArtifact;
}

pgsStrandStressArtifact* pgsSegmentArtifact::GetStrandStressArtifact()
{
   return &m_StrandStressArtifact;
}

void pgsSegmentArtifact::SetStrandSlopeArtifact(const pgsStrandSlopeArtifact& artifact)
{
   m_StrandSlopeArtifact = artifact;
}

const pgsStrandSlopeArtifact* pgsSegmentArtifact::GetStrandSlopeArtifact() const
{
   return &m_StrandSlopeArtifact;
}

pgsStrandSlopeArtifact* pgsSegmentArtifact::GetStrandSlopeArtifact()
{
   return &m_StrandSlopeArtifact;
}

void pgsSegmentArtifact::SetHoldDownForceArtifact(const pgsHoldDownForceArtifact& artifact)
{
   m_HoldDownForceArtifact = artifact;
}

const pgsHoldDownForceArtifact* pgsSegmentArtifact::GetHoldDownForceArtifact() const
{
   return &m_HoldDownForceArtifact;
}

pgsHoldDownForceArtifact* pgsSegmentArtifact::GetHoldDownForceArtifact()
{
   return &m_HoldDownForceArtifact;
}

void pgsSegmentArtifact::AddFlexuralStressArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress,
                                                   const pgsFlexuralStressArtifact& artifact)
{
   // Need to be using a valid ID in the artifact
   ATLASSERT(artifact.GetPointOfInterest().GetID() != INVALID_ID);

   std::vector<pgsFlexuralStressArtifact>& artifacts( GetFlexuralStressArtifacts(intervalIdx,ls,stress) );
   artifacts.push_back(artifact);
   std::sort(artifacts.begin(),artifacts.end());
}

CollectionIndexType pgsSegmentArtifact::GetFlexuralStressArtifactCount(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress) const
{
   const std::vector<pgsFlexuralStressArtifact>& artifacts( GetFlexuralStressArtifacts(intervalIdx,ls,stress) );
   return artifacts.size();
}

const pgsFlexuralStressArtifact* pgsSegmentArtifact::GetFlexuralStressArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress,CollectionIndexType idx) const
{
   const std::vector<pgsFlexuralStressArtifact>& artifacts( GetFlexuralStressArtifacts(intervalIdx,ls,stress) );
   if ( idx < artifacts.size() )
   {
      return &artifacts[idx];
   }
   else
   {
      return nullptr;
   }
}

pgsFlexuralStressArtifact* pgsSegmentArtifact::GetFlexuralStressArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress,CollectionIndexType idx)
{
   std::vector<pgsFlexuralStressArtifact>& artifacts( GetFlexuralStressArtifacts(intervalIdx,ls,stress) );
   if ( idx < artifacts.size() )
   {
      return &artifacts[idx];
   }
   else
   {
      return nullptr;
   }
}

const pgsFlexuralStressArtifact* pgsSegmentArtifact::GetFlexuralStressArtifactAtPoi(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress,PoiIDType poiID) const
{
   const std::vector<pgsFlexuralStressArtifact>& artifacts( GetFlexuralStressArtifacts(intervalIdx,ls,stress) );
   std::vector<pgsFlexuralStressArtifact>::const_iterator iter(artifacts.begin());
   std::vector<pgsFlexuralStressArtifact>::const_iterator end(artifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsFlexuralStressArtifact& artifact(*iter);
      if ( artifact.GetPointOfInterest().GetID() == poiID )
      {
         return &artifact;
      }
   }

   return nullptr;
}

const pgsStirrupCheckArtifact* pgsSegmentArtifact::GetStirrupCheckArtifact() const
{
   return &m_StirrupCheckArtifact;
}

pgsStirrupCheckArtifact* pgsSegmentArtifact::GetStirrupCheckArtifact()
{
   return &m_StirrupCheckArtifact;
}

const pgsPrecastIGirderDetailingArtifact* pgsSegmentArtifact::GetPrecastIGirderDetailingArtifact() const
{
   return &m_PrecastIGirderDetailingArtifact;
}

pgsPrecastIGirderDetailingArtifact* pgsSegmentArtifact::GetPrecastIGirderDetailingArtifact()
{
   return &m_PrecastIGirderDetailingArtifact;
}

void pgsSegmentArtifact::SetSegmentStabilityArtifact(const pgsSegmentStabilityArtifact& artifact)
{
   m_StabilityArtifact = artifact;
}

const pgsSegmentStabilityArtifact* pgsSegmentArtifact::GetSegmentStabilityArtifact() const
{
   return &m_StabilityArtifact;
}

pgsSegmentStabilityArtifact* pgsSegmentArtifact::GetSegmentStabilityArtifact()
{
   return &m_StabilityArtifact;
}

void pgsSegmentArtifact::SetLiftingCheckArtifact(const stbLiftingCheckArtifact* artifact)
{
   m_pLiftingCheckArtifact = artifact;
}

const stbLiftingCheckArtifact* pgsSegmentArtifact::GetLiftingCheckArtifact() const
{
   return m_pLiftingCheckArtifact;
}

void pgsSegmentArtifact::SetHaulingAnalysisArtifact(const pgsHaulingAnalysisArtifact* artifact)
{
   m_pHaulingAnalysisArtifact = artifact;
}

const pgsHaulingAnalysisArtifact* pgsSegmentArtifact::GetHaulingAnalysisArtifact() const
{
   return m_pHaulingAnalysisArtifact;
}

bool pgsSegmentArtifact::IsFlexuralStressCheckApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stressType,pgsTypes::StressLocation stressLocation) const
{
   std::vector<pgsFlexuralStressArtifact>& vArtifacts(GetFlexuralStressArtifacts(intervalIdx,ls,stressType));
   std::vector<pgsFlexuralStressArtifact>::iterator iter(vArtifacts.begin());
   std::vector<pgsFlexuralStressArtifact>::iterator iterEnd(vArtifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      pgsFlexuralStressArtifact& artifact(*iter);
      if ( artifact.IsApplicable(stressLocation) )
      {
         return true;
      }
   }

   return false;
}

bool pgsSegmentArtifact::WasWithRebarAllowableStressUsed(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressLocation stressLocation,PoiAttributeType attribute) const
{
   ATLASSERT(attribute == 0 || attribute == POI_CLOSURE);
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2_NOCHECK(pBroker,IPointOfInterest,pPoi); // sometimes there aren't any artifacts so this doesn't get used

   std::vector<pgsFlexuralStressArtifact>& vArtifacts(GetFlexuralStressArtifacts(intervalIdx,ls,pgsTypes::Tension));
   std::vector<pgsFlexuralStressArtifact>::iterator iter(vArtifacts.begin());
   std::vector<pgsFlexuralStressArtifact>::iterator iterEnd(vArtifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      pgsFlexuralStressArtifact& artifact(*iter);
      const pgsPointOfInterest& poi(artifact.GetPointOfInterest());
      CClosureKey closureKey;
      bool bIsClosure = pPoi->IsInClosureJoint(poi,&closureKey);
      if ( (attribute == POI_CLOSURE &&  bIsClosure) || // test if attribute is POI_CLOSURE and artifact is for a closure -OR-
           (attribute == 0           && !bIsClosure)    // test if attribute is 0 and artifact is not for a closure
          )
      {
         if ( artifact.WasWithRebarAllowableStressUsed(stressLocation) )
         {
            return true;
         }
      }
   }

   return false;
}

bool pgsSegmentArtifact::WasSegmentWithRebarAllowableStressUsed(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   std::vector<pgsFlexuralStressArtifact>& vArtifacts(GetFlexuralStressArtifacts(intervalIdx,ls,pgsTypes::Tension));
   std::vector<pgsFlexuralStressArtifact>::iterator iter(vArtifacts.begin());
   std::vector<pgsFlexuralStressArtifact>::iterator iterEnd(vArtifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      pgsFlexuralStressArtifact& artifact(*iter);
      const pgsPointOfInterest& poi(artifact.GetPointOfInterest());
      CClosureKey closureKey;
      bool bIsClosure = pPoi->IsInClosureJoint(poi,&closureKey);
      if ( !bIsClosure )
      {
         if ( artifact.WasWithRebarAllowableStressUsed(pgsTypes::TopGirder) ||
              artifact.WasWithRebarAllowableStressUsed(pgsTypes::BottomGirder))
         {
            return true;
         }
      }
   }

   return false;
}

bool pgsSegmentArtifact::WasClosureJointWithRebarAllowableStressUsed(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bIsInPTZ) const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   std::vector<pgsFlexuralStressArtifact>& vArtifacts(GetFlexuralStressArtifacts(intervalIdx,ls,pgsTypes::Tension));
   std::vector<pgsFlexuralStressArtifact>::iterator iter(vArtifacts.begin());
   std::vector<pgsFlexuralStressArtifact>::iterator iterEnd(vArtifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      pgsFlexuralStressArtifact& artifact(*iter);
      const pgsPointOfInterest& poi(artifact.GetPointOfInterest());
      CClosureKey closureKey;
      bool bIsClosure = pPoi->IsInClosureJoint(poi,&closureKey);
      if ( bIsClosure )
      {
         if ( (artifact.IsInPrecompressedTensileZone(pgsTypes::TopGirder) == bIsInPTZ && // PTZ status of top matches bIsInPTZ
               artifact.WasWithRebarAllowableStressUsed(pgsTypes::TopGirder)) // AND allowable with rebar was used in top of girder
               ||                                                             // -OR-
              (artifact.IsInPrecompressedTensileZone(pgsTypes::BottomGirder) == bIsInPTZ && // PTZ status of bottom matches bIsInPTZ
               artifact.WasWithRebarAllowableStressUsed(pgsTypes::BottomGirder)) // AND allowable with rebar was used in bot of girder
            )
         {
            return true;
         }
      }
   }

   return false;
}

bool pgsSegmentArtifact::WasDeckWithRebarAllowableStressUsed(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const
{
   std::vector<pgsFlexuralStressArtifact>& vArtifacts(GetFlexuralStressArtifacts(intervalIdx,ls,pgsTypes::Tension));
   std::vector<pgsFlexuralStressArtifact>::iterator iter(vArtifacts.begin());
   std::vector<pgsFlexuralStressArtifact>::iterator iterEnd(vArtifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      pgsFlexuralStressArtifact& artifact(*iter);
      const pgsPointOfInterest& poi(artifact.GetPointOfInterest());
      if ( artifact.WasWithRebarAllowableStressUsed(pgsTypes::TopDeck) || 
           artifact.WasWithRebarAllowableStressUsed(pgsTypes::BottomDeck) )
      {
         return true;
      }
   }

   return false;
}

bool pgsSegmentArtifact::IsWithRebarAllowableStressApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressLocation stressLocation,PoiAttributeType attribute) const
{
   ATLASSERT(attribute == 0 || attribute == POI_CLOSURE);
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2_NOCHECK(pBroker,IPointOfInterest,pPoi); // sometimes there aren't any artifacts so this doesn't get used

   std::vector<pgsFlexuralStressArtifact>& vArtifacts(GetFlexuralStressArtifacts(intervalIdx,ls,pgsTypes::Tension));
   std::vector<pgsFlexuralStressArtifact>::iterator iter(vArtifacts.begin());
   std::vector<pgsFlexuralStressArtifact>::iterator iterEnd(vArtifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      pgsFlexuralStressArtifact& artifact(*iter);
      const pgsPointOfInterest& poi(artifact.GetPointOfInterest());
      CClosureKey closureKey;
      bool bIsClosure = pPoi->IsInClosureJoint(poi,&closureKey);
      if ( (attribute == POI_CLOSURE &&  bIsClosure) || // test if attribute is POI_CLOSURE and artifact is for a closure -OR-
           (attribute == 0           && !bIsClosure)    // test if attribute is 0 and artifact is not for a closure
          )
      {
         if ( artifact.IsWithRebarAllowableStressApplicable(stressLocation) )
         {
            return true;
         }
      }
   }

   return false;
}

bool pgsSegmentArtifact::IsSegmentWithRebarAllowableStressApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   std::vector<pgsFlexuralStressArtifact>& vArtifacts(GetFlexuralStressArtifacts(intervalIdx,ls,pgsTypes::Tension));
   std::vector<pgsFlexuralStressArtifact>::iterator iter(vArtifacts.begin());
   std::vector<pgsFlexuralStressArtifact>::iterator iterEnd(vArtifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      pgsFlexuralStressArtifact& artifact(*iter);
      const pgsPointOfInterest& poi(artifact.GetPointOfInterest());
      CClosureKey closureKey;
      bool bIsClosure = pPoi->IsInClosureJoint(poi,&closureKey);
      if ( !bIsClosure )
      {
         if ( artifact.IsWithRebarAllowableStressApplicable(pgsTypes::TopGirder) ||
              artifact.IsWithRebarAllowableStressApplicable(pgsTypes::BottomGirder))
         {
            return true;
         }
      }
   }

   return false;
}

bool pgsSegmentArtifact::IsClosureJointWithRebarAllowableStressApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,bool bIsInPTZ) const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   std::vector<pgsFlexuralStressArtifact>& vArtifacts(GetFlexuralStressArtifacts(intervalIdx,ls,pgsTypes::Tension));
   std::vector<pgsFlexuralStressArtifact>::iterator iter(vArtifacts.begin());
   std::vector<pgsFlexuralStressArtifact>::iterator iterEnd(vArtifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      pgsFlexuralStressArtifact& artifact(*iter);
      const pgsPointOfInterest& poi(artifact.GetPointOfInterest());
      CClosureKey closureKey;
      bool bIsClosure = pPoi->IsInClosureJoint(poi,&closureKey);
      if ( bIsClosure )
      {
         if ( (artifact.IsInPrecompressedTensileZone(pgsTypes::TopGirder) == bIsInPTZ && // PTZ status of top matches bIsInPTZ
               artifact.IsWithRebarAllowableStressApplicable(pgsTypes::TopGirder)) // AND allowable with rebar was used in top of girder
               ||                                                             // -OR-
              (artifact.IsInPrecompressedTensileZone(pgsTypes::BottomGirder) == bIsInPTZ && // PTZ status of bottom matches bIsInPTZ
               artifact.IsWithRebarAllowableStressApplicable(pgsTypes::BottomGirder)) // AND allowable with rebar was used in bot of girder
            )
         {
            return true;
         }
      }
   }

   return false;
}

bool pgsSegmentArtifact::IsDeckWithRebarAllowableStressApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const
{
   std::vector<pgsFlexuralStressArtifact>& vArtifacts(GetFlexuralStressArtifacts(intervalIdx,ls,pgsTypes::Tension));
   std::vector<pgsFlexuralStressArtifact>::iterator iter(vArtifacts.begin());
   std::vector<pgsFlexuralStressArtifact>::iterator iterEnd(vArtifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      pgsFlexuralStressArtifact& artifact(*iter);
      const pgsPointOfInterest& poi(artifact.GetPointOfInterest());
      if ( artifact.IsWithRebarAllowableStressApplicable(pgsTypes::TopDeck) || 
           artifact.IsWithRebarAllowableStressApplicable(pgsTypes::BottomDeck) )
      {
         return true;
      }
   }

   return false;
}

void pgsSegmentArtifact::SetCapacityWithRebar(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressLocation stressLocation,Float64 fAllow)
{
   GetAllowableWithRebar(intervalIdx,ls,stressLocation) = fAllow;
}

Float64 pgsSegmentArtifact::GetCapacityWithRebar(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressLocation stressLocation) const
{
   return GetAllowableWithRebar(intervalIdx,ls,stressLocation);
}

pgsDebondArtifact* pgsSegmentArtifact::GetDebondArtifact(pgsTypes::StrandType strandType)
{
   return &m_DebondArtifact[strandType];
}

const pgsDebondArtifact* pgsSegmentArtifact::GetDebondArtifact(pgsTypes::StrandType strandType) const
{
   return &m_DebondArtifact[strandType];
}

bool pgsSegmentArtifact::Passed() const
{
   bool bPassed = true;

   bPassed &= m_StabilityArtifact.Passed();
   bPassed &= m_HoldDownForceArtifact.Passed();
   bPassed &= m_StrandSlopeArtifact.Passed();
   bPassed &= m_StrandStressArtifact.Passed();
   bPassed &= DidSegmentFlexuralStressesPass();
   bPassed &= DidDeckFlexuralStressesPass();

   bPassed &= m_StirrupCheckArtifact.Passed();

   bPassed &= m_PrecastIGirderDetailingArtifact.Passed();

   if ( m_pLiftingCheckArtifact != nullptr )
   {
      bPassed &= m_pLiftingCheckArtifact->Passed();
   }
   if (m_pHaulingAnalysisArtifact != nullptr)
   {
      bPassed &= m_pHaulingAnalysisArtifact->Passed(pgsTypes::CrownSlope);
      bPassed &= m_pHaulingAnalysisArtifact->Passed(pgsTypes::Superelevation);
   }

   for ( Uint16 i = 0; i < 3; i++ )
   {
      bPassed &= m_DebondArtifact[i].Passed();
   }

   return bPassed;
}

bool pgsSegmentArtifact::DidSegmentFlexuralStressesPass() const
{
   bool bPassed = true;

   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator  i2;
   for ( i2 = m_FlexuralStressArtifacts.begin(); i2 != m_FlexuralStressArtifacts.end(); i2++ )
   {
      const std::pair<StressKey,std::vector<pgsFlexuralStressArtifact>>& item = *i2;
      std::vector<pgsFlexuralStressArtifact>::const_iterator iter(item.second.begin());
      std::vector<pgsFlexuralStressArtifact>::const_iterator end(item.second.end());
      for ( ; iter != end; iter++ )
      {
         const pgsFlexuralStressArtifact& artifact(*iter);
         bPassed &= artifact.BeamPassed();
      }
   }

   return bPassed;
}

bool pgsSegmentArtifact::DidDeckFlexuralStressesPass() const
{
   bool bPassed = true;

   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator  i2;
   for ( i2 = m_FlexuralStressArtifacts.begin(); i2 != m_FlexuralStressArtifacts.end(); i2++ )
   {
      const std::pair<StressKey,std::vector<pgsFlexuralStressArtifact>>& item = *i2;
      std::vector<pgsFlexuralStressArtifact>::const_iterator iter(item.second.begin());
      std::vector<pgsFlexuralStressArtifact>::const_iterator end(item.second.end());
      for ( ; iter != end; iter++ )
      {
         const pgsFlexuralStressArtifact& artifact(*iter);
         bPassed &= artifact.DeckPassed();
      }
   }

   return bPassed;
}

Float64 pgsSegmentArtifact::GetRequiredSegmentConcreteStrength(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   Float64 fc_reqd = 0;

   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator iter(m_FlexuralStressArtifacts.begin());
   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator end(m_FlexuralStressArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const std::pair<StressKey,std::vector<pgsFlexuralStressArtifact>>& item = *iter;
      const StressKey& key = item.first;

      if ( key.intervalIdx == intervalIdx && key.ls == ls )
      {
         std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIter(item.second.begin());
         std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIterEnd(item.second.end());
         for ( ; artifactIter != artifactIterEnd; artifactIter++ )
         {
            const pgsFlexuralStressArtifact& artifact(*artifactIter);
            const pgsPointOfInterest& poi(artifact.GetPointOfInterest());
            CClosureKey closureKey;
            if ( pPoi->IsInClosureJoint(poi,&closureKey) )
            {
               continue;
            }

            for ( int i = 0; i < 2; i++ )
            {
               pgsTypes::StressLocation stressLocation = (i == 0 ? pgsTypes::TopGirder : pgsTypes::BottomGirder);
               if ( artifact.IsApplicable(stressLocation) )
               {
                  Float64 fc = artifact.GetRequiredConcreteStrength(stressLocation);

                  if ( fc < 0 ) 
                  {
                     return fc;
                  }

                  if ( 0 < fc )
                  {
                     fc_reqd = Max(fc,fc_reqd);
                  }
               }
            }
         }
      }
   }

   return fc_reqd;
}

Float64 pgsSegmentArtifact::GetRequiredClosureJointConcreteStrength(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   Float64 fc_reqd = 0;

   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator iter(m_FlexuralStressArtifacts.begin());
   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator end(m_FlexuralStressArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const std::pair<StressKey,std::vector<pgsFlexuralStressArtifact>>& item = *iter;
      const StressKey& key = item.first;

      if ( key.intervalIdx == intervalIdx && key.ls == ls )
      {
         std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIter(item.second.begin());
         std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIterEnd(item.second.end());
         for ( ; artifactIter != artifactIterEnd; artifactIter++ )
         {
            const pgsFlexuralStressArtifact& artifact(*artifactIter);
            const pgsPointOfInterest& poi(artifact.GetPointOfInterest());
            CClosureKey closureKey;
            if ( !pPoi->IsInClosureJoint(poi,&closureKey) )
            {
               continue;
            }

            for ( int i = 0; i < 2; i++ )
            {
               pgsTypes::StressLocation stressLocation = (i == 0 ? pgsTypes::TopGirder : pgsTypes::BottomGirder);
               if ( artifact.IsApplicable(stressLocation) )
               {
                  Float64 fc = artifact.GetRequiredConcreteStrength(stressLocation);

                  if ( fc < 0 ) 
                  {
                     return fc;
                  }

                  if ( 0 < fc )
                  {
                     fc_reqd = Max(fc,fc_reqd);
                  }
               }
            }
         }
      }
   }

   return fc_reqd;
}

Float64 pgsSegmentArtifact::GetRequiredDeckConcreteStrength(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const
{
   Float64 fc_reqd = 0;

   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator iter(m_FlexuralStressArtifacts.begin());
   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator end(m_FlexuralStressArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const std::pair<StressKey,std::vector<pgsFlexuralStressArtifact>>& item = *iter;
      const StressKey& key = item.first;

      if ( key.intervalIdx == intervalIdx && key.ls == ls )
      {
         std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIter(item.second.begin());
         std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIterEnd(item.second.end());
         for ( ; artifactIter != artifactIterEnd; artifactIter++ )
         {
            const pgsFlexuralStressArtifact& artifact(*artifactIter);
            for ( int i = 0; i < 2; i++ )
            {
               pgsTypes::StressLocation stressLocation = (i == 0 ? pgsTypes::TopDeck : pgsTypes::BottomDeck);
               if ( artifact.IsApplicable(stressLocation) )
               {
                  Float64 fc = artifact.GetRequiredConcreteStrength(stressLocation);

                  if ( fc < 0 ) 
                  {
                     return fc;
                  }

                  if ( 0 < fc )
                  {
                     fc_reqd = Max(fc,fc_reqd);
                  }
               }
            }
         }
      }
   }

   return fc_reqd;
}

Float64 pgsSegmentArtifact::GetRequiredSegmentConcreteStrength() const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   Float64 fc_reqd = 0;

   IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(m_SegmentKey);

   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator iter(m_FlexuralStressArtifacts.begin());
   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator end(m_FlexuralStressArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const std::pair<StressKey,std::vector<pgsFlexuralStressArtifact>>& item = *iter;
      const StressKey& key = item.first;

      if ( key.intervalIdx < haulingIntervalIdx )
      {
         continue; // don't check if this is before hauling (basically we want final concrete strength cases)
      }

      std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIter(item.second.begin());
      std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIterEnd(item.second.end());
      for ( ; artifactIter != artifactIterEnd; artifactIter++ )
      {
         const pgsFlexuralStressArtifact& artifact(*artifactIter);
         const pgsPointOfInterest& poi(artifact.GetPointOfInterest());
         CClosureKey closureKey;
         if ( pPoi->IsInClosureJoint(poi,&closureKey) )
         {
            continue;
         }

         for ( int i = 0; i < 2; i++ )
         {
            pgsTypes::StressLocation stressLocation = (i == 0 ? pgsTypes::TopGirder : pgsTypes::BottomGirder );
            if ( artifact.IsApplicable(stressLocation) )
            {
               Float64 fc = artifact.GetRequiredConcreteStrength(stressLocation);

               if ( fc < 0 ) 
               {
                  return fc;
               }

               if ( 0 < fc )
               {
                  fc_reqd = Max(fc,fc_reqd);
               }
            }
         }
      }
   }

   if (m_pHaulingAnalysisArtifact != nullptr)
   {
      Float64 fc_reqd_hauling_comp1, fc_reqd_hauling_tens1, fc_reqd_hauling_tens_wbar1;
      m_pHaulingAnalysisArtifact->GetRequiredConcreteStrength(pgsTypes::CrownSlope,&fc_reqd_hauling_comp1,&fc_reqd_hauling_tens1, &fc_reqd_hauling_tens_wbar1);

      Float64 fc_reqd_hauling_comp2, fc_reqd_hauling_tens2, fc_reqd_hauling_tens_wbar2;
      m_pHaulingAnalysisArtifact->GetRequiredConcreteStrength(pgsTypes::Superelevation,&fc_reqd_hauling_comp2,&fc_reqd_hauling_tens2, &fc_reqd_hauling_tens_wbar2);

      Float64 fc_reqd_hauling = Max(fc_reqd_hauling_tens_wbar1,fc_reqd_hauling_comp2,fc_reqd_hauling_tens_wbar1,fc_reqd_hauling_comp2);

      if ( fc_reqd_hauling < 0 ) // there is no concrete strength that will work
      {
         return fc_reqd_hauling;
      }

      fc_reqd = Max(fc_reqd,fc_reqd_hauling);
   }

   return fc_reqd;
}

Float64 pgsSegmentArtifact::GetRequiredClosureJointConcreteStrength() const
{
   Float64 fc_reqd = 0;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(m_SegmentKey);

   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator iter(m_FlexuralStressArtifacts.begin());
   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator end(m_FlexuralStressArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const std::pair<StressKey,std::vector<pgsFlexuralStressArtifact>>& item = *iter;
      const StressKey& key = item.first;

      if ( key.intervalIdx < haulingIntervalIdx )
      {
         continue; // don't check if this is before hauling (basically we want final concrete strength cases)
      }

      std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIter(item.second.begin());
      std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIterEnd(item.second.end());
      for ( ; artifactIter != artifactIterEnd; artifactIter++ )
      {
         const pgsFlexuralStressArtifact& artifact(*artifactIter);
         const pgsPointOfInterest& poi(artifact.GetPointOfInterest());
         CClosureKey closureKey;
         if ( pPoi->IsInClosureJoint(poi,&closureKey) )
         {
            for ( int i = 0; i < 2; i++ )
            {
               pgsTypes::StressLocation stressLocation = (i == 0 ? pgsTypes::TopGirder : pgsTypes::BottomGirder );
               if ( artifact.IsApplicable(stressLocation) )
               {
                  Float64 fc = artifact.GetRequiredConcreteStrength(stressLocation);

                  if ( fc < 0 ) 
                  {
                     return fc;
                  }

                  if ( 0 < fc )
                  {
                     fc_reqd = Max(fc,fc_reqd);
                  }
               }
            }
         }
      }
   }

   return fc_reqd;
}

Float64 pgsSegmentArtifact::GetRequiredDeckConcreteStrength() const
{
   Float64 fc_reqd = 0;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator iter(m_FlexuralStressArtifacts.begin());
   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator end(m_FlexuralStressArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const std::pair<StressKey,std::vector<pgsFlexuralStressArtifact>>& item = *iter;
      const StressKey& key = item.first;

      if ( key.intervalIdx < compositeDeckIntervalIdx )
      {
         continue; // don't check if this is before the deck is composite.
      }

      std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIter(item.second.begin());
      std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIterEnd(item.second.end());
      for ( ; artifactIter != artifactIterEnd; artifactIter++ )
      {
         const pgsFlexuralStressArtifact& artifact(*artifactIter);

         for ( int i = 0; i < 2; i++ )
         {
            pgsTypes::StressLocation stressLocation = (i == 0 ? pgsTypes::TopDeck : pgsTypes::BottomDeck);
            if ( artifact.IsApplicable(stressLocation) )
            {
               Float64 fc = artifact.GetRequiredConcreteStrength(stressLocation);

               if ( fc < 0 ) 
               {
                  return fc;
               }

               if ( 0 < fc )
               {
                  fc_reqd = Max(fc,fc_reqd);
               }
            }
         }
      }
   }

   return fc_reqd;
}

Float64 pgsSegmentArtifact::GetRequiredReleaseStrength() const
{
   Float64 fc_reqd = 0;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(m_SegmentKey);

   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator iter(m_FlexuralStressArtifacts.begin());
   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::const_iterator end(m_FlexuralStressArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const std::pair<StressKey,std::vector<pgsFlexuralStressArtifact>>& item = *iter;
      const StressKey& key = item.first;

      if ( haulingIntervalIdx <= key.intervalIdx )
      {
         continue; // don't check if this is after hauling (basically we want release concrete strength cases)
      }

      std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIter(item.second.begin());
      std::vector<pgsFlexuralStressArtifact>::const_iterator artifactIterEnd(item.second.end());
      for ( ; artifactIter != artifactIterEnd; artifactIter++ )
      {
         const pgsFlexuralStressArtifact& artifact(*artifactIter);
         Float64 fc = artifact.GetRequiredBeamConcreteStrength();

         if ( fc < 0 ) 
         {
            return fc;
         }

         if ( 0 < fc )
         {
            fc_reqd = Max(fc,fc_reqd);
         }
      }
   }
 
   if (m_pLiftingCheckArtifact != nullptr)
   {
      Float64 fc_reqd_lifting_comp = m_pLiftingCheckArtifact->RequiredFcCompression();
      Float64 fc_reqd_lifting_tens_norebar = m_pLiftingCheckArtifact->RequiredFcTension();
      Float64 fc_reqd_lifting_tens_withrebar = m_pLiftingCheckArtifact->RequiredFcTensionWithRebar();

      Float64 fc_reqd_lifting = Max(fc_reqd_lifting_comp,fc_reqd_lifting_tens_norebar,fc_reqd_lifting_tens_withrebar);

      fc_reqd = Max(fc_reqd,fc_reqd_lifting);
   }

   return fc_reqd;
}

const CSegmentKey& pgsSegmentArtifact::GetSegmentKey() const
{
   return m_SegmentKey;
}

void pgsSegmentArtifact::MakeCopy(const pgsSegmentArtifact& rOther)
{
   m_SegmentKey = rOther.m_SegmentKey;

   m_StrandStressArtifact            = rOther.m_StrandStressArtifact;
   m_FlexuralStressArtifacts         = rOther.m_FlexuralStressArtifacts;
   m_StirrupCheckArtifact            = rOther.m_StirrupCheckArtifact;
   m_PrecastIGirderDetailingArtifact = rOther.m_PrecastIGirderDetailingArtifact;
   m_StrandSlopeArtifact             = rOther.m_StrandSlopeArtifact;
   m_HoldDownForceArtifact           = rOther.m_HoldDownForceArtifact;
   m_StabilityArtifact               = rOther.m_StabilityArtifact;

   m_pLiftingCheckArtifact    = rOther.m_pLiftingCheckArtifact;
   m_pHaulingAnalysisArtifact = rOther.m_pHaulingAnalysisArtifact;

   for ( int i = 0; i < 4; i++ )
   {
      m_AllowableWithRebar[i] = rOther.m_AllowableWithRebar[i];
   }

   for ( Uint16 i = 0; i < 3; i++ )
      m_DebondArtifact[i] = rOther.m_DebondArtifact[i];
}

void pgsSegmentArtifact::MakeAssignment(const pgsSegmentArtifact& rOther)
{
   MakeCopy( rOther );
}

std::vector<pgsFlexuralStressArtifact>& pgsSegmentArtifact::GetFlexuralStressArtifacts(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stress) const
{
   StressKey key;
   key.intervalIdx = intervalIdx;
   key.ls = ls;
   key.stress = stress;

   std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::iterator found(m_FlexuralStressArtifacts.find(key));
   if ( found == m_FlexuralStressArtifacts.end() )
   {
      std::pair<std::map<StressKey,std::vector<pgsFlexuralStressArtifact>>::iterator,bool> result(m_FlexuralStressArtifacts.insert(std::make_pair(key,std::vector<pgsFlexuralStressArtifact>())));
      found = result.first;
   }

   return found->second;
}

Float64& pgsSegmentArtifact::GetAllowableWithRebar(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressLocation stressLocation) const
{
   StressKey key;
   key.intervalIdx = intervalIdx;
   key.ls = ls;
   key.stress = pgsTypes::Tension;
   std::map<StressKey,Float64>::iterator found(m_AllowableWithRebar[stressLocation].find(key));
   if ( found == m_AllowableWithRebar[stressLocation].end() )
   {
      std::pair<std::map<StressKey,Float64>::iterator,bool> result(m_AllowableWithRebar[stressLocation].insert(std::make_pair(key,-1.0)));
      found = result.first;
   }

   return found->second;
}