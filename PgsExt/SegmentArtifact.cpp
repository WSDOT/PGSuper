///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

void pgsSegmentArtifact::SetPlantHandlingWeightArtifact(const pgsPlantHandlingWeightArtifact& artifact)
{
   m_PlantHandlingWeightArtifact = artifact;
}

const pgsPlantHandlingWeightArtifact* pgsSegmentArtifact::GetPlantHandlingWeightArtifact() const
{
   return &m_PlantHandlingWeightArtifact;
}

pgsPlantHandlingWeightArtifact* pgsSegmentArtifact::GetPlantHandlingWeightArtifact()
{
   return &m_PlantHandlingWeightArtifact;
}

void pgsSegmentArtifact::AddFlexuralStressArtifact(const pgsFlexuralStressArtifact& artifact)
{
   // Need to be using a valid ID in the artifact
   ATLASSERT(artifact.GetPointOfInterest().GetID() != INVALID_ID);

   auto& artifacts( GetFlexuralStressArtifacts(artifact.GetTask()) );
   artifacts.push_back(artifact);
   std::sort(std::begin(artifacts),std::end(artifacts));
}

CollectionIndexType pgsSegmentArtifact::GetFlexuralStressArtifactCount(const StressCheckTask& task) const
{
   const auto& artifacts( GetFlexuralStressArtifacts(task) );
   return artifacts.size();
}

const pgsFlexuralStressArtifact* pgsSegmentArtifact::GetFlexuralStressArtifact(const StressCheckTask& task,CollectionIndexType idx) const
{
   const auto& artifacts( GetFlexuralStressArtifacts(task) );
   if ( idx < artifacts.size() )
   {
      return &artifacts[idx];
   }
   else
   {
      return nullptr;
   }
}

pgsFlexuralStressArtifact* pgsSegmentArtifact::GetFlexuralStressArtifact(const StressCheckTask& task,CollectionIndexType idx)
{
   auto& artifacts( GetFlexuralStressArtifacts(task) );
   if ( idx < artifacts.size() )
   {
      return &artifacts[idx];
   }
   else
   {
      return nullptr;
   }
}

const pgsFlexuralStressArtifact* pgsSegmentArtifact::GetFlexuralStressArtifactAtPoi(const StressCheckTask& task,PoiIDType poiID) const
{
   const auto& vArtifacts( GetFlexuralStressArtifacts(task) );
   for ( const auto& artifact : vArtifacts)
   {
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

void pgsSegmentArtifact::SetTendonStressArtifact(DuctIndexType ductIdx, const pgsTendonStressArtifact& artifact)
{
   m_TendonStressArtifacts.insert(std::make_pair(ductIdx, artifact));
}

const pgsTendonStressArtifact* pgsSegmentArtifact::GetTendonStressArtifact(DuctIndexType ductIdx) const
{
   std::map<DuctIndexType, pgsTendonStressArtifact>::const_iterator found(m_TendonStressArtifacts.find(ductIdx));
   ATLASSERT(found != m_TendonStressArtifacts.end());
   return &(found->second);
}

pgsTendonStressArtifact* pgsSegmentArtifact::GetTendonStressArtifact(DuctIndexType ductIdx)
{
   return &m_TendonStressArtifacts[ductIdx];
}

void pgsSegmentArtifact::SetDuctSizeArtifact(DuctIndexType ductIdx, const pgsDuctSizeArtifact& artifact)
{
   m_DuctSizeArtifacts.insert(std::make_pair(ductIdx, artifact));
}

const pgsDuctSizeArtifact* pgsSegmentArtifact::GetDuctSizeArtifact(DuctIndexType ductIdx) const
{
   std::map<DuctIndexType, pgsDuctSizeArtifact>::const_iterator found(m_DuctSizeArtifacts.find(ductIdx));
   ATLASSERT(found != m_DuctSizeArtifacts.end());
   return &(found->second);
}

pgsDuctSizeArtifact* pgsSegmentArtifact::GetDuctSizeArtifact(DuctIndexType ductIdx)
{
   return &m_DuctSizeArtifacts[ductIdx];
}

bool pgsSegmentArtifact::WasWithRebarAllowableStressUsed(const StressCheckTask& task,pgsTypes::StressLocation stressLocation,PoiAttributeType attribute) const
{
   ATLASSERT(attribute == 0 || attribute == POI_CLOSURE);
   
   if (task.stressType == pgsTypes::Compression)
   {
      ATLASSERT(false); // why are you asking for this for compression? it doesn't apply
      return false;
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2_NOCHECK(pBroker,IPointOfInterest,pPoi); // sometimes there aren't any artifacts so this doesn't get used

   const auto& vArtifacts(GetFlexuralStressArtifacts(task));
   for ( const auto& artifact : vArtifacts)
   {
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

bool pgsSegmentArtifact::WasSegmentWithRebarAllowableStressUsed(const StressCheckTask& task) const
{
   if (task.stressType == pgsTypes::Compression)
   {
      ATLASSERT(false); // why are you asking for this for compression? it doesn't apply
      return false;
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   const auto& vArtifacts(GetFlexuralStressArtifacts(task));
   for ( const auto& artifact : vArtifacts)
   {
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

bool pgsSegmentArtifact::WasClosureJointWithRebarAllowableStressUsed(const StressCheckTask& task,bool bIsInPTZ) const
{
   if (task.stressType == pgsTypes::Compression)
   {
      ATLASSERT(false); // why are you asking for this for compression? it doesn't apply
      return false;
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   const auto& vArtifacts(GetFlexuralStressArtifacts(task));
   for ( const auto& artifact : vArtifacts)
   {
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

bool pgsSegmentArtifact::WasDeckWithRebarAllowableStressUsed(const StressCheckTask& task) const
{
   if (task.stressType == pgsTypes::Compression)
   {
      ATLASSERT(false); // why are you asking for this for compression? it doesn't apply
      return false;
   }

   const auto& vArtifacts(GetFlexuralStressArtifacts(task));
   for ( const auto& artifact : vArtifacts)
   {
      const pgsPointOfInterest& poi(artifact.GetPointOfInterest());
      if ( artifact.WasWithRebarAllowableStressUsed(pgsTypes::TopDeck) || 
           artifact.WasWithRebarAllowableStressUsed(pgsTypes::BottomDeck) )
      {
         return true;
      }
   }

   return false;
}

bool pgsSegmentArtifact::IsWithRebarAllowableStressApplicable(const StressCheckTask& task,pgsTypes::StressLocation stressLocation,PoiAttributeType attribute) const
{
   if (task.stressType == pgsTypes::Compression)
   {
      return false;
   }

   ATLASSERT(attribute == 0 || attribute == POI_CLOSURE);
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   const auto& vArtifacts(GetFlexuralStressArtifacts(task));
   for( const auto& artifact : vArtifacts)
   {
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

bool pgsSegmentArtifact::IsSegmentWithRebarAllowableStressApplicable(const StressCheckTask& task) const
{
   if (task.stressType == pgsTypes::Compression)
   {
      ATLASSERT(false); // why are you asking for this for compression? it doesn't apply
      return false;
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   const auto& vArtifacts(GetFlexuralStressArtifacts(task));
   for( const auto& artifact : vArtifacts)
   {
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

bool pgsSegmentArtifact::IsClosureJointWithRebarAllowableStressApplicable(const StressCheckTask& task,bool bIsInPTZ) const
{
   if (task.stressType == pgsTypes::Compression)
   {
      ATLASSERT(false); // why are you asking for this for compression? it doesn't apply
      return false;
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   const auto& vArtifacts(GetFlexuralStressArtifacts(task));
   for( const auto& artifact : vArtifacts)
   {
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

bool pgsSegmentArtifact::IsDeckWithRebarAllowableStressApplicable(const StressCheckTask& task) const
{
   if (task.stressType == pgsTypes::Compression)
   {
      ATLASSERT(false); // why are you asking for this for compression? it doesn't apply
      return false;
   }

   const auto& vArtifacts(GetFlexuralStressArtifacts(task));
   for( const auto& artifact : vArtifacts)
   {
      const pgsPointOfInterest& poi(artifact.GetPointOfInterest());
      if ( artifact.IsWithRebarAllowableStressApplicable(pgsTypes::TopDeck) || 
           artifact.IsWithRebarAllowableStressApplicable(pgsTypes::BottomDeck) )
      {
         return true;
      }
   }

   return false;
}

pgsDebondArtifact* pgsSegmentArtifact::GetDebondArtifact()
{
   return &m_DebondArtifact;
}

const pgsDebondArtifact* pgsSegmentArtifact::GetDebondArtifact() const
{
   return &m_DebondArtifact;
}

bool pgsSegmentArtifact::Passed() const
{
   // if any one fails, then the whole things fails. Return false on first failure or true if we get to the end
   if (!m_StabilityArtifact.Passed())
   {
      return false;
   }

   if (!m_HoldDownForceArtifact.Passed())
   {
      return false;
   }

   if (!m_PlantHandlingWeightArtifact.Passed())
   {
      return false;
   }

   if (!m_StrandSlopeArtifact.Passed())
   {
      return false;
   }

   if (!m_StrandStressArtifact.Passed())
   {
      return false;
   }

   if (!DidFlexuralStressPass())
   {
      return false;
   }

   if (!m_StirrupCheckArtifact.Passed())
   {
      return false;
   }

   if (!m_PrecastIGirderDetailingArtifact.Passed())
   {
      return false;
   }

   if (m_pLiftingCheckArtifact != nullptr && !m_pLiftingCheckArtifact->Passed())
   {
      return false;
   }

   if (m_pHaulingAnalysisArtifact != nullptr)
   {
      if (!m_pHaulingAnalysisArtifact->Passed(pgsTypes::CrownSlope))
      {
         return false;
      }

      if (!m_pHaulingAnalysisArtifact->Passed(pgsTypes::Superelevation))
      {
         return false;
      }
   }

   if (!m_DebondArtifact.Passed())
   {
      return false;
   }

   for (const auto& item : m_TendonStressArtifacts)
   {
      const auto& artifact(item.second);
      if (!artifact.Passed())
      {
         return false;
      }
   }

   for (const auto& item : m_DuctSizeArtifacts)
   {
      const auto& artifact(item.second);
      if (!artifact.Passed())
      {
         return false;
      }
   }

   return true;
}

bool pgsSegmentArtifact::DidFlexuralStressPass() const
{
   for (const auto& item : m_FlexuralStressArtifacts)
   {
      for (const auto& artifact : item.second)
      {
         if (!artifact.BeamPassed() || !artifact.DeckPassed())
         {
            return false; // the whole thing doesn't pass when a single artifact doesn't pass... no need to keep going
         }
      }
   }
   return true;
}

bool pgsSegmentArtifact::DidSegmentFlexuralStressesPass() const
{
   for (const auto& item : m_FlexuralStressArtifacts)
   {
      for (const auto& artifact : item.second)
      {
         if (!artifact.BeamPassed())
         {
            return false; // the whole thing doesn't pass when a single artifact doesn't pass... no need to keep going
         }
      }
   }
   return true;
}

bool pgsSegmentArtifact::DidDeckFlexuralStressesPass() const
{
   for (const auto& item : m_FlexuralStressArtifacts)
   {
      for (const auto& artifact : item.second)
      {
         if (!artifact.DeckPassed())
         {
            return false; // the whole thing doesn't pass when a single artifact doesn't pass... no need to keep going
         }
      }
   }
   return true;
}

Float64 pgsSegmentArtifact::GetRequiredSegmentConcreteStrength(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);

   Float64 fc_reqd = 0;

   for( const auto& item : m_FlexuralStressArtifacts)
   {
      const auto& key(item.first);

      if ( key.intervalIdx == intervalIdx && key.limitState == ls )
      {
         for( const auto& artifact : item.second)
         {
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

   for (const auto& item : m_FlexuralStressArtifacts)
   {
      const auto& key(item.first);

      if (key.intervalIdx == intervalIdx && key.limitState == ls)
      {
         for (const auto& artifact : item.second)
         {
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

   for (const auto& item : m_FlexuralStressArtifacts)
   {
      const auto& key(item.first);

      if (key.intervalIdx == intervalIdx && key.limitState == ls)
      {
         for (const auto& artifact : item.second)
         {
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

   for(const auto& item : m_FlexuralStressArtifacts)
   {
      const auto& key = item.first;

      if ( key.intervalIdx < haulingIntervalIdx )
      {
         continue; // don't check if this is before hauling (basically we want final concrete strength cases)
      }

      for( const auto& artifact : item.second)
      {
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

   for ( const auto& item : m_FlexuralStressArtifacts)
   {
      const auto& key = item.first;

      if ( key.intervalIdx < haulingIntervalIdx )
      {
         continue; // don't check if this is before hauling (basically we want final concrete strength cases)
      }

      for( const auto& artifact : item.second)
      {
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
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   GET_IFACE2(pBroker,IIntervals,pIntervals);

   for ( const auto& item : m_FlexuralStressArtifacts)
   {
      const auto& key = item.first;

      for ( const auto& artifact : item.second)
      {
         IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(artifact.GetPointOfInterest());
         IntervalIndexType compositeDeckIntervalIdx = (deckCastingRegionIdx == INVALID_INDEX ? INVALID_INDEX : pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx));

         if (key.intervalIdx < compositeDeckIntervalIdx)
         {
            continue; // don't check if this is before the deck is composite.
         }

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

   for ( const auto& item : m_FlexuralStressArtifacts)
   {
      const auto& key = item.first;

      if ( haulingIntervalIdx <= key.intervalIdx )
      {
         continue; // don't check if this is after hauling (basically we want release concrete strength cases)
      }

      for ( const auto& artifact : item.second)
      {
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
   m_PlantHandlingWeightArtifact = rOther.m_PlantHandlingWeightArtifact;
   m_StabilityArtifact               = rOther.m_StabilityArtifact;

   m_pLiftingCheckArtifact    = rOther.m_pLiftingCheckArtifact;
   m_pHaulingAnalysisArtifact = rOther.m_pHaulingAnalysisArtifact;

   m_DebondArtifact = rOther.m_DebondArtifact;

   m_TendonStressArtifacts = rOther.m_TendonStressArtifacts;
   m_DuctSizeArtifacts = rOther.m_DuctSizeArtifacts;
}

void pgsSegmentArtifact::MakeAssignment(const pgsSegmentArtifact& rOther)
{
   MakeCopy( rOther );
}

std::vector<pgsFlexuralStressArtifact>& pgsSegmentArtifact::GetFlexuralStressArtifacts(const StressCheckTask& task) const
{
   auto found(m_FlexuralStressArtifacts.find(task));
   if ( found == m_FlexuralStressArtifacts.end() )
   {
      auto result(m_FlexuralStressArtifacts.insert(std::make_pair(task,std::vector<pgsFlexuralStressArtifact>())));
      found = result.first;
   }

   return found->second;
}
