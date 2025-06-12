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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\GirderArtifact.h>

#define POSITIVE 0
#define NEGATIVE 1


pgsGirderArtifact::pgsGirderArtifact(const CGirderKey& girderKey) :
m_GirderKey(girderKey)
{
}

pgsGirderArtifact::pgsGirderArtifact(const pgsGirderArtifact& other)
{
   MakeCopy(other);
}

pgsGirderArtifact& pgsGirderArtifact::operator= (const pgsGirderArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

const CGirderKey& pgsGirderArtifact::GetGirderKey() const
{
   return m_GirderKey;
}

void pgsGirderArtifact::AddPositiveMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls, const pgsFlexuralCapacityArtifact& artifact)
{
   AddFlexuralCapacityArtifact(&m_FlexuralCapacityArtifacts[POSITIVE][ls], intervalIdx, artifact);
}

void pgsGirderArtifact::AddNegativeMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx, pgsTypes::LimitState ls, const pgsFlexuralCapacityArtifact& artifact)
{
   AddFlexuralCapacityArtifact(&m_FlexuralCapacityArtifacts[NEGATIVE][ls], intervalIdx, artifact);
}

IndexType pgsGirderArtifact::GetPositiveMomentFlexuralCapacityArtifactCount(IntervalIndexType intervalIdx, pgsTypes::LimitState ls) const
{
   return GetFlexuralCapacityArtifactCount(&m_FlexuralCapacityArtifacts[POSITIVE][ls], intervalIdx);
}

IndexType pgsGirderArtifact::GetNegativeMomentFlexuralCapacityArtifactCount(IntervalIndexType intervalIdx, pgsTypes::LimitState ls) const
{
   return GetFlexuralCapacityArtifactCount(&m_FlexuralCapacityArtifacts[NEGATIVE][ls], intervalIdx);
}

const pgsFlexuralCapacityArtifact* pgsGirderArtifact::GetPositiveMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,IndexType artifactIdx) const
{
   return GetFlexuralCapacityArtifact(&m_FlexuralCapacityArtifacts[POSITIVE][ls], intervalIdx, artifactIdx);
}

const pgsFlexuralCapacityArtifact* pgsGirderArtifact::GetNegativeMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,IndexType artifactIdx) const
{
   return GetFlexuralCapacityArtifact(&m_FlexuralCapacityArtifacts[NEGATIVE][ls], intervalIdx, artifactIdx);
}

const pgsFlexuralCapacityArtifact* pgsGirderArtifact::FindPositiveMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi) const
{
   return FindFlexuralCapacityArtifact(&m_FlexuralCapacityArtifacts[POSITIVE][ls], intervalIdx, poi);
}

const pgsFlexuralCapacityArtifact* pgsGirderArtifact::FindNegativeMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi) const
{
   return FindFlexuralCapacityArtifact(&m_FlexuralCapacityArtifacts[NEGATIVE][ls], intervalIdx, poi);
}

void pgsGirderArtifact::AddSegmentArtifact(const pgsSegmentArtifact& artifact)
{
   ATLASSERT(artifact.GetSegmentKey().groupIndex  == m_GirderKey.groupIndex);
   ATLASSERT(artifact.GetSegmentKey().girderIndex == m_GirderKey.girderIndex);
   m_SegmentArtifacts.insert(std::make_pair(artifact.GetSegmentKey(),artifact));
}

const pgsSegmentArtifact* pgsGirderArtifact::GetSegmentArtifact(SegmentIndexType segIdx) const
{
   CSegmentKey segmentKey(m_GirderKey,segIdx);
   std::map<CSegmentKey,pgsSegmentArtifact>::const_iterator found = m_SegmentArtifacts.find(segmentKey);
   ATLASSERT(found != m_SegmentArtifacts.end());
   if ( found == m_SegmentArtifacts.end() )
   {
      return nullptr;
   }

   ATLASSERT(found->first == found->second.GetSegmentKey());
   return &(found->second);
}

pgsSegmentArtifact* pgsGirderArtifact::GetSegmentArtifact(SegmentIndexType segIdx)
{
   CSegmentKey segmentKey(m_GirderKey,segIdx);
   std::map<CSegmentKey,pgsSegmentArtifact>::iterator found = m_SegmentArtifacts.find(segmentKey);
   if ( found == m_SegmentArtifacts.end() )
   {
      pgsSegmentArtifact artifact(segmentKey);
      std::pair<std::map<CSegmentKey,pgsSegmentArtifact>::iterator,bool> result = m_SegmentArtifacts.insert(std::make_pair(segmentKey,artifact));
      ATLASSERT(result.second == true);
      found = result.first;
   }

   ATLASSERT(found != m_SegmentArtifacts.end());
   return &(found->second);
}

void pgsGirderArtifact::SetConstructabilityArtifact(const pgsConstructabilityArtifact& artifact)
{
   m_ConstructabilityArtifact = artifact;
}

const pgsConstructabilityArtifact* pgsGirderArtifact::GetConstructabilityArtifact() const
{
   return &m_ConstructabilityArtifact;
}

pgsConstructabilityArtifact* pgsGirderArtifact::GetConstructabilityArtifact()
{
   return &m_ConstructabilityArtifact;
}

void pgsGirderArtifact::SetTendonStressArtifact(DuctIndexType ductIdx,const pgsTendonStressArtifact& artifact)
{
   m_TendonStressArtifacts.insert(std::make_pair(ductIdx,artifact));
}

const pgsTendonStressArtifact* pgsGirderArtifact::GetTendonStressArtifact(DuctIndexType ductIdx) const
{
   std::map<DuctIndexType,pgsTendonStressArtifact>::const_iterator found(m_TendonStressArtifacts.find(ductIdx));
   ATLASSERT(found != m_TendonStressArtifacts.end());
   return &(found->second);
}

pgsTendonStressArtifact* pgsGirderArtifact::GetTendonStressArtifact(DuctIndexType ductIdx)
{
   return &m_TendonStressArtifacts[ductIdx];
}

void pgsGirderArtifact::SetDuctSizeArtifact(DuctIndexType ductIdx,const pgsDuctSizeArtifact& artifact)
{
   m_DuctSizeArtifacts.insert(std::make_pair(ductIdx,artifact));
}

const pgsDuctSizeArtifact* pgsGirderArtifact::GetDuctSizeArtifact(DuctIndexType ductIdx) const
{
   std::map<DuctIndexType,pgsDuctSizeArtifact>::const_iterator found(m_DuctSizeArtifacts.find(ductIdx));
   ATLASSERT(found != m_DuctSizeArtifacts.end());
   return &(found->second);
}

pgsDuctSizeArtifact* pgsGirderArtifact::GetDuctSizeArtifact(DuctIndexType ductIdx)
{
   return &m_DuctSizeArtifacts[ductIdx];
}

void pgsGirderArtifact::AddDeflectionCheckArtifact(const pgsDeflectionCheckArtifact& artifact)
{
   ATLASSERT(artifact.GetSpan() != INVALID_INDEX);
   m_DeflectionCheckArtifact.push_back(artifact);
}

IndexType pgsGirderArtifact::GetDeflectionCheckArtifactCount()
{
   return m_DeflectionCheckArtifact.size();
}

pgsDeflectionCheckArtifact* pgsGirderArtifact::GetDeflectionCheckArtifact(IndexType idx)
{
   return &m_DeflectionCheckArtifact[idx];
}

const pgsDeflectionCheckArtifact* pgsGirderArtifact::GetDeflectionCheckArtifact(IndexType idx) const
{
   return &m_DeflectionCheckArtifact[idx];
}

bool pgsGirderArtifact::WasWithRebarAllowableStressUsed(const StressCheckTask& task,pgsTypes::StressLocation stressLocation) const
{
   for (const auto& item : m_SegmentArtifacts)
   {
      const auto& artifact(item.second);
      ATLASSERT(item.first == artifact.GetSegmentKey());
      if (artifact.WasWithRebarAllowableStressUsed(task, stressLocation, 0) ||
         artifact.WasWithRebarAllowableStressUsed(task, stressLocation, POI_CLOSURE))
      {
         return true;
      }
   }
   return false;
}

bool pgsGirderArtifact::WasGirderWithRebarAllowableStressUsed(const StressCheckTask& task) const
{
   for ( const auto& item : m_SegmentArtifacts)
   {
      const auto& artifact(item.second);
      ATLASSERT(item.first == artifact.GetSegmentKey());
      if ( artifact.WasSegmentWithRebarAllowableStressUsed(task) ||
           artifact.WasClosureJointWithRebarAllowableStressUsed(task,true /*in PTZ*/) ||
           artifact.WasClosureJointWithRebarAllowableStressUsed(task,false /*not in PTZ*/))
      {
         return true;
      }
   }
   return false;
}

bool pgsGirderArtifact::WasDeckWithRebarAllowableStressUsed(const StressCheckTask& task) const
{
   for (const auto& item : m_SegmentArtifacts)
   {
      const auto& artifact(item.second);
      ATLASSERT(item.first == artifact.GetSegmentKey());
      if ( artifact.WasDeckWithRebarAllowableStressUsed(task) )
      {
         return true;
      }
   }
   return false;
}

bool pgsGirderArtifact::IsWithRebarAllowableStressApplicable(const StressCheckTask& task,pgsTypes::StressLocation stressLocation) const
{
   for (const auto& item : m_SegmentArtifacts)
   {
      const auto& artifact(item.second);
      ATLASSERT(item.first == artifact.GetSegmentKey());
      if ( artifact.IsWithRebarAllowableStressApplicable(task,stressLocation,0) ||
           artifact.IsWithRebarAllowableStressApplicable(task,stressLocation,POI_CLOSURE) )
      {
         return true;
      }
   }
   return false;
}

bool pgsGirderArtifact::IsGirderWithRebarAllowableStressApplicable(const StressCheckTask& task,pgsTypes::StressLocation stressLocation) const
{
   for (const auto& item : m_SegmentArtifacts)
   {
      const auto& artifact(item.second);
      ATLASSERT(item.first == artifact.GetSegmentKey());
      if ( artifact.IsSegmentWithRebarAllowableStressApplicable(task) ||
           artifact.IsClosureJointWithRebarAllowableStressApplicable(task,true /*in PTZ*/) ||
           artifact.IsClosureJointWithRebarAllowableStressApplicable(task,false /*not in PTZ*/))
      {
         return true;
      }
   }
   return false;
}

bool pgsGirderArtifact::IsDeckWithRebarAllowableStressApplicable(const StressCheckTask& task,pgsTypes::StressLocation stressLocation) const
{
   for (const auto& item : m_SegmentArtifacts)
   {
      const auto& artifact(item.second);
      ATLASSERT(item.first == artifact.GetSegmentKey());
      if ( artifact.IsDeckWithRebarAllowableStressApplicable(task) )
      {
         return true;
      }
   }
   return false;
}

Float64 pgsGirderArtifact::GetRequiredGirderConcreteStrength(const StressCheckTask& task) const
{
   Float64 f = 0;
   for (const auto& item : m_SegmentArtifacts)
   {
      const auto& artifact(item.second);
      ATLASSERT(item.first == artifact.GetSegmentKey());
      Float64 required = artifact.GetRequiredSegmentConcreteStrength(task);
      if ( required < 0 )
      {
         ATLASSERT(required == NO_AVAILABLE_CONCRETE_STRENGTH); // there is not a concrete strength that will work
         return required;
      }

      f = Max(f,required);
   }
   return f;
}

Float64 pgsGirderArtifact::GetRequiredGirderConcreteStrength() const
{
   Float64 f = 0;
   for (const auto& item : m_SegmentArtifacts)
   {
      const auto& artifact(item.second);
      ATLASSERT(item.first == artifact.GetSegmentKey());
      Float64 required = artifact.GetRequiredSegmentConcreteStrength();
      if ( required < 0 )
      {
         ATLASSERT(required == NO_AVAILABLE_CONCRETE_STRENGTH); // there is not a concrete strength that will work
         return required;
      }

      f = Max(f,required);
   }
   return f;
}

Float64 pgsGirderArtifact::GetRequiredDeckConcreteStrength(const StressCheckTask& task) const
{
   Float64 f = 0;
   for (const auto& item : m_SegmentArtifacts)
   {
      const auto& artifact(item.second);
      ATLASSERT(item.first == artifact.GetSegmentKey());
      Float64 required = artifact.GetRequiredDeckConcreteStrength(task);
      if ( required < 0 )
      {
         ATLASSERT(required == NO_AVAILABLE_CONCRETE_STRENGTH); // there is not a concrete strength that will work
         return required;
      }

      f = Max(f,required);
   }
   return f;
}

Float64 pgsGirderArtifact::GetRequiredDeckConcreteStrength() const
{
   Float64 f = 0;
   for (const auto& item : m_SegmentArtifacts)
   {
      const auto& artifact(item.second);
      ATLASSERT(item.first == artifact.GetSegmentKey());
      Float64 required = artifact.GetRequiredDeckConcreteStrength();
      if ( required < 0 )
      {
         ATLASSERT(required == NO_AVAILABLE_CONCRETE_STRENGTH); // there is not a concrete strength that will work
         return required;
      }

      f = Max(f,required);
   }
   return f;
}

Float64 pgsGirderArtifact::GetRequiredReleaseStrength() const
{
   Float64 f = 0;
   for (const auto& item : m_SegmentArtifacts)
   {
      const auto& artifact(item.second);
      ATLASSERT(item.first == artifact.GetSegmentKey());
      Float64 required = artifact.GetRequiredReleaseStrength();
      if ( required < 0 )
      {
         ATLASSERT(required == NO_AVAILABLE_CONCRETE_STRENGTH); // there is not a concrete strength that will work
         return required;
      }

      f = Max(f,required);
   }
   return f;
}

bool pgsGirderArtifact::Passed() const
{
   for ( const auto& item : m_TendonStressArtifacts)
   {
      const auto& artifact(item.second);
      if ( !artifact.Passed() )
      {
         return false;
      }
   }

   for (const auto& item : m_DuctSizeArtifacts)
   {
      const auto& artifact(item.second);
      if ( !artifact.Passed() )
      {
         return false;
      }
   }

   for ( IndexType lsIdx = 0; lsIdx < (IndexType)(pgsTypes::LimitStateCount); lsIdx++ )
   {
      for ( const auto& item : m_FlexuralCapacityArtifacts[POSITIVE][lsIdx])
      {
         const auto& vArtifacts(item);
         for ( const auto& artifactItem : vArtifacts.second)
         {
            const auto& artifact(artifactItem);

            if ( !artifact.Passed() )
            {
               return false;
            }
         }
      }

      for (const auto& item : m_FlexuralCapacityArtifacts[NEGATIVE][lsIdx])
      {
         const auto& vArtifacts(item);
         for (const auto& artifactItem : vArtifacts.second)
         {
            const auto& artifact(artifactItem);

            if (!artifact.Passed())
            {
               return false;
            }
         }
      }
   }

   if ( !m_ConstructabilityArtifact.Passed() )
   {
      return false;
   }

   for ( const auto& item : m_SegmentArtifacts)
   {
      const auto& artifact(item.second);
      ATLASSERT(item.first == artifact.GetSegmentKey());
      if ( !artifact.Passed() )
      {
         return false;
      }
   }

   for ( const auto& artifact : m_DeflectionCheckArtifact)
   {
      if ( !artifact.Passed() )
      {
         return false;
      }
   }

   return true;
}

void pgsGirderArtifact::MakeCopy(const pgsGirderArtifact& rOther)
{
   m_GirderKey                 = rOther.m_GirderKey;
   m_TendonStressArtifacts     = rOther.m_TendonStressArtifacts;
   m_DuctSizeArtifacts         = rOther.m_DuctSizeArtifacts;

   for ( IndexType lsIdx = 0; lsIdx < (IndexType)(pgsTypes::LimitStateCount); lsIdx++ )
   {
      m_FlexuralCapacityArtifacts[POSITIVE][lsIdx] = rOther.m_FlexuralCapacityArtifacts[POSITIVE][lsIdx];
      m_FlexuralCapacityArtifacts[NEGATIVE][lsIdx] = rOther.m_FlexuralCapacityArtifacts[NEGATIVE][lsIdx];
   }

   m_SegmentArtifacts          = rOther.m_SegmentArtifacts;
   m_ConstructabilityArtifact  = rOther.m_ConstructabilityArtifact;
   m_DeflectionCheckArtifact   = rOther.m_DeflectionCheckArtifact;
}

void pgsGirderArtifact::MakeAssignment(const pgsGirderArtifact& rOther)
{
   MakeCopy( rOther );
}


void pgsGirderArtifact::AddFlexuralCapacityArtifact(FlexuralCapacityContainer* pArtifacts, IntervalIndexType intervalIdx, const pgsFlexuralCapacityArtifact& artifact)
{
   if (pArtifacts->size() < intervalIdx)
   {
      for (IntervalIndexType idx = pArtifacts->size(); idx <= intervalIdx; idx++)
      {
         std::vector<pgsFlexuralCapacityArtifact> vArtifacts;
         pArtifacts->insert(std::make_pair(idx, vArtifacts));
      }
   }

   auto found = pArtifacts->find(intervalIdx);

   std::vector<pgsFlexuralCapacityArtifact>& vArtifacts(found->second);
   vArtifacts.push_back(artifact);

   std::sort(vArtifacts.begin(), vArtifacts.end(), [](const auto& a, const auto& b) {return a.GetPointOfInterest() < b.GetPointOfInterest();});
}

IndexType pgsGirderArtifact::GetFlexuralCapacityArtifactCount(const FlexuralCapacityContainer* pArtifacts, IntervalIndexType intervalIdx) const
{
   const auto& found = pArtifacts->find(intervalIdx);
   if (found == pArtifacts->end())
   {
      return 0;
   }

   const auto& vArtifacts(found->second);
   return vArtifacts.size();
}

const pgsFlexuralCapacityArtifact* pgsGirderArtifact::GetFlexuralCapacityArtifact(const FlexuralCapacityContainer* pArtifacts, IntervalIndexType intervalIdx, IndexType artifactIdx) const
{
   const auto& found = pArtifacts->find(intervalIdx);
   if (found == pArtifacts->end())
   {
      return nullptr;
   }

   const auto& vArtifacts(found->second);
   const pgsFlexuralCapacityArtifact* pArtifact = &(vArtifacts[artifactIdx]);
   return pArtifact;
}

const pgsFlexuralCapacityArtifact* pgsGirderArtifact::FindFlexuralCapacityArtifact(const FlexuralCapacityContainer* pArtifacts, IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) const
{
   const auto& found = pArtifacts->find(intervalIdx);
   if (found == pArtifacts->end())
   {
      return nullptr;
   }

   const auto& vArtifacts(found->second);
   for (const auto& item : vArtifacts)
   {
      const auto& artifact(item);
      if (artifact.GetPointOfInterest().GetID() == poi.GetID())
      {
         return &artifact;
      }
   }

   return nullptr;
}
