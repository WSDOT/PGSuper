///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


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

void pgsGirderArtifact::AddFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,
                                                    const pgsFlexuralCapacityArtifact& pmArtifact,
                                                    const pgsFlexuralCapacityArtifact& nmArtifact)
{
   if ( m_FlexuralCapacityArtifacts[ls].size() < intervalIdx )
   {
      for ( IntervalIndexType idx = m_FlexuralCapacityArtifacts[ls].size(); idx <= intervalIdx; idx++ )
      {
         std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>> vArtifacts;
         m_FlexuralCapacityArtifacts[ls].insert(std::make_pair(idx,vArtifacts));
      }
   }

   std::map<IntervalIndexType,std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>>::iterator found;
   found = m_FlexuralCapacityArtifacts[ls].find(intervalIdx);

   std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>& vArtifacts(found->second);
   ATLASSERT(pmArtifact.GetPointOfInterest() == nmArtifact.GetPointOfInterest());
   vArtifacts.push_back(std::make_pair(pmArtifact,nmArtifact));
}

CollectionIndexType pgsGirderArtifact::GetFlexuralCapacityArtifactCount(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const
{
   std::map<IntervalIndexType,std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>>::const_iterator found;
   found = m_FlexuralCapacityArtifacts[ls].find(intervalIdx);
   if ( found == m_FlexuralCapacityArtifacts[ls].end() )
   {
      return 0;
   }

   const std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>& vArtifacts(found->second);
   return vArtifacts.size();
}

const pgsFlexuralCapacityArtifact* pgsGirderArtifact::GetPositiveMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,CollectionIndexType artifactIdx) const
{
   std::map<IntervalIndexType,std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>>::const_iterator found;
   found = m_FlexuralCapacityArtifacts[ls].find(intervalIdx);
   if ( found == m_FlexuralCapacityArtifacts[ls].end() )
   {
      return NULL;
   }

   const std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>& vArtifacts(found->second);
   const pgsFlexuralCapacityArtifact* pArtifact = &(vArtifacts[artifactIdx].first);
   return pArtifact;
}

const pgsFlexuralCapacityArtifact* pgsGirderArtifact::GetNegativeMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,CollectionIndexType artifactIdx) const
{
   std::map<IntervalIndexType,std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>>::const_iterator found;
   found = m_FlexuralCapacityArtifacts[ls].find(intervalIdx);
   if ( found == m_FlexuralCapacityArtifacts[ls].end() )
   {
      return NULL;
   }

   const std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>& vArtifacts(found->second);
   const pgsFlexuralCapacityArtifact* pArtifact = &(vArtifacts[artifactIdx].second);
   return pArtifact;
}

const pgsFlexuralCapacityArtifact* pgsGirderArtifact::FindPositiveMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi) const
{
   std::map<IntervalIndexType,std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>>::const_iterator found;
   found = m_FlexuralCapacityArtifacts[ls].find(intervalIdx);
   if ( found == m_FlexuralCapacityArtifacts[ls].end() )
   {
      return NULL;
   }

   const std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>& vArtifacts(found->second);
   std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>::const_iterator iter(vArtifacts.begin());
   std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>::const_iterator iterEnd(vArtifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const pgsFlexuralCapacityArtifact* pArtifact = &(iter->first);
      if ( pArtifact->GetPointOfInterest() == poi )
      {
         return pArtifact;
      }
   }

   return NULL;
}

const pgsFlexuralCapacityArtifact* pgsGirderArtifact::FindNegativeMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi) const
{
   std::map<IntervalIndexType,std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>>::const_iterator found;
   found = m_FlexuralCapacityArtifacts[ls].find(intervalIdx);
   if ( found == m_FlexuralCapacityArtifacts[ls].end() )
   {
      return NULL;
   }

   const std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>& vArtifacts(found->second);
   std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>::const_iterator iter(vArtifacts.begin());
   std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>::const_iterator iterEnd(vArtifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const pgsFlexuralCapacityArtifact* pArtifact = &(iter->second);
      if ( pArtifact->GetPointOfInterest() == poi )
      {
         return pArtifact;
      }
   }

   return NULL;
}

void pgsGirderArtifact::AddSegmentArtifact(const pgsSegmentArtifact& artifact)
{
   ATLASSERT(artifact.GetSegmentKey().groupIndex  == m_GirderKey.groupIndex);
   ATLASSERT(artifact.GetSegmentKey().girderIndex == m_GirderKey.girderIndex);
   m_SegmentArtifacts.insert(artifact);
}

const pgsSegmentArtifact* pgsGirderArtifact::GetSegmentArtifact(SegmentIndexType segIdx) const
{
   pgsSegmentArtifact key(CSegmentKey(m_GirderKey,segIdx));
   std::set<pgsSegmentArtifact>::const_iterator found = m_SegmentArtifacts.find(key);
   ATLASSERT(found != m_SegmentArtifacts.end());
   if ( found == m_SegmentArtifacts.end() )
   {
      return NULL;
   }

   return &(*found);
}

pgsSegmentArtifact* pgsGirderArtifact::GetSegmentArtifact(SegmentIndexType segIdx)
{
   pgsSegmentArtifact key(CSegmentKey(m_GirderKey,segIdx));
   std::set<pgsSegmentArtifact>::iterator found = m_SegmentArtifacts.find(key);
   if ( found == m_SegmentArtifacts.end() )
   {
      std::pair<std::set<pgsSegmentArtifact>::iterator,bool> result = m_SegmentArtifacts.insert(key);
      ATLASSERT(result.second == true);
      found = result.first;
   }

   ATLASSERT(found != m_SegmentArtifacts.end());
   return &(*found);
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

bool pgsGirderArtifact::WasWithRebarAllowableStressUsed(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressLocation stressLocation) const
{
   std::set<pgsSegmentArtifact>::const_iterator iter(m_SegmentArtifacts.begin());
   std::set<pgsSegmentArtifact>::const_iterator end(m_SegmentArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsSegmentArtifact& artifact = *iter;
      if ( artifact.WasWithRebarAllowableStressUsed(intervalIdx,ls,stressLocation,0) ||
           artifact.WasWithRebarAllowableStressUsed(intervalIdx,ls,stressLocation,POI_CLOSURE) )
      {
         return true;
      }
   }
   return false;
}

bool pgsGirderArtifact::WasGirderWithRebarAllowableStressUsed(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const
{
   std::set<pgsSegmentArtifact>::const_iterator iter(m_SegmentArtifacts.begin());
   std::set<pgsSegmentArtifact>::const_iterator end(m_SegmentArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsSegmentArtifact& artifact = *iter;
      if ( artifact.WasSegmentWithRebarAllowableStressUsed(intervalIdx,ls) ||
           artifact.WasClosureJointWithRebarAllowableStressUsed(intervalIdx,ls,true /*in PTZ*/) ||
           artifact.WasClosureJointWithRebarAllowableStressUsed(intervalIdx,ls,false /*not in PTZ*/))
      {
         return true;
      }
   }
   return false;
}

bool pgsGirderArtifact::WasDeckWithRebarAllowableStressUsed(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const
{
   std::set<pgsSegmentArtifact>::const_iterator iter(m_SegmentArtifacts.begin());
   std::set<pgsSegmentArtifact>::const_iterator end(m_SegmentArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsSegmentArtifact& artifact = *iter;
      if ( artifact.WasDeckWithRebarAllowableStressUsed(intervalIdx,ls) )
      {
         return true;
      }
   }
   return false;
}

bool pgsGirderArtifact::IsWithRebarAllowableStressApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressLocation stressLocation) const
{
   std::set<pgsSegmentArtifact>::const_iterator iter(m_SegmentArtifacts.begin());
   std::set<pgsSegmentArtifact>::const_iterator end(m_SegmentArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsSegmentArtifact& artifact = *iter;
      if ( artifact.IsWithRebarAllowableStressApplicable(intervalIdx,ls,stressLocation,0) ||
           artifact.IsWithRebarAllowableStressApplicable(intervalIdx,ls,stressLocation,POI_CLOSURE) )
      {
         return true;
      }
   }
   return false;
}

bool pgsGirderArtifact::IsGirderWithRebarAllowableStressApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressLocation stressLocation) const
{
   std::set<pgsSegmentArtifact>::const_iterator iter(m_SegmentArtifacts.begin());
   std::set<pgsSegmentArtifact>::const_iterator end(m_SegmentArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsSegmentArtifact& artifact = *iter;
      if ( artifact.IsSegmentWithRebarAllowableStressApplicable(intervalIdx,ls) ||
           artifact.IsClosureJointWithRebarAllowableStressApplicable(intervalIdx,ls,true /*in PTZ*/) ||
           artifact.IsClosureJointWithRebarAllowableStressApplicable(intervalIdx,ls,false /*not in PTZ*/))
      {
         return true;
      }
   }
   return false;
}

bool pgsGirderArtifact::IsDeckWithRebarAllowableStressApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressLocation stressLocation) const
{
   std::set<pgsSegmentArtifact>::const_iterator iter(m_SegmentArtifacts.begin());
   std::set<pgsSegmentArtifact>::const_iterator end(m_SegmentArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsSegmentArtifact& artifact = *iter;
      if ( artifact.IsDeckWithRebarAllowableStressApplicable(intervalIdx,ls) )
      {
         return true;
      }
   }
   return false;
}

bool pgsGirderArtifact::IsFlexuralStressCheckApplicable(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,pgsTypes::StressType stressType,pgsTypes::StressLocation stressLocation) const
{
   std::set<pgsSegmentArtifact>::const_iterator iter(m_SegmentArtifacts.begin());
   std::set<pgsSegmentArtifact>::const_iterator end(m_SegmentArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsSegmentArtifact& artifact = *iter;
      if ( artifact.IsFlexuralStressCheckApplicable(intervalIdx,ls,stressType,stressLocation) )
      {
         return true;
      }
   }
   return false;
}

Float64 pgsGirderArtifact::GetRequiredGirderConcreteStrength(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState) const
{
   Float64 f = 0;
   std::set<pgsSegmentArtifact>::const_iterator iter(m_SegmentArtifacts.begin());
   std::set<pgsSegmentArtifact>::const_iterator end(m_SegmentArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsSegmentArtifact& artifact = *iter;
      Float64 required = artifact.GetRequiredSegmentConcreteStrength(intervalIdx,limitState);
      if ( required < 0 )
      {
         ATLASSERT(required == -99999); // there is not a concrete strength that will work
         return required;
      }

      f = Max(f,required);
   }
   return f;
}

Float64 pgsGirderArtifact::GetRequiredGirderConcreteStrength() const
{
   Float64 f = 0;
   std::set<pgsSegmentArtifact>::const_iterator iter(m_SegmentArtifacts.begin());
   std::set<pgsSegmentArtifact>::const_iterator end(m_SegmentArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsSegmentArtifact& artifact = *iter;
      Float64 required = artifact.GetRequiredSegmentConcreteStrength();
      if ( required < 0 )
      {
         ATLASSERT(required == -99999); // there is not a concrete strength that will work
         return required;
      }

      f = Max(f,required);
   }
   return f;
}

Float64 pgsGirderArtifact::GetRequiredDeckConcreteStrength(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState) const
{
   Float64 f = 0;
   std::set<pgsSegmentArtifact>::const_iterator iter(m_SegmentArtifacts.begin());
   std::set<pgsSegmentArtifact>::const_iterator end(m_SegmentArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsSegmentArtifact& artifact = *iter;
      Float64 required = artifact.GetRequiredDeckConcreteStrength(intervalIdx,limitState);
      if ( required < 0 )
      {
         ATLASSERT(required == -99999); // there is not a concrete strength that will work
         return required;
      }

      f = Max(f,required);
   }
   return f;
}

Float64 pgsGirderArtifact::GetRequiredDeckConcreteStrength() const
{
   Float64 f = 0;
   std::set<pgsSegmentArtifact>::const_iterator iter(m_SegmentArtifacts.begin());
   std::set<pgsSegmentArtifact>::const_iterator end(m_SegmentArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsSegmentArtifact& artifact = *iter;
      Float64 required = artifact.GetRequiredDeckConcreteStrength();
      if ( required < 0 )
      {
         ATLASSERT(required == -99999); // there is not a concrete strength that will work
         return required;
      }

      f = Max(f,required);
   }
   return f;
}

Float64 pgsGirderArtifact::GetRequiredReleaseStrength() const
{
   Float64 f = 0;
   std::set<pgsSegmentArtifact>::const_iterator iter(m_SegmentArtifacts.begin());
   std::set<pgsSegmentArtifact>::const_iterator end(m_SegmentArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsSegmentArtifact& artifact = *iter;
      Float64 required = artifact.GetRequiredReleaseStrength();
      if ( required < 0 )
      {
         ATLASSERT(required == -99999); // there is not a concrete strength that will work
         return required;
      }

      f = Max(f,required);
   }
   return f;
}

bool pgsGirderArtifact::Passed() const
{
   std::map<DuctIndexType,pgsTendonStressArtifact>::const_iterator tendonStressIter(m_TendonStressArtifacts.begin());
   std::map<DuctIndexType,pgsTendonStressArtifact>::const_iterator tendonStressIterEnd(m_TendonStressArtifacts.end());
   for ( ; tendonStressIter != tendonStressIterEnd; tendonStressIter++ )
   {
      const pgsTendonStressArtifact& artifact = tendonStressIter->second;
      if ( !artifact.Passed() )
      {
         return false;
      }
   }

   std::map<DuctIndexType,pgsDuctSizeArtifact>::const_iterator ductSizeIter(m_DuctSizeArtifacts.begin());
   std::map<DuctIndexType,pgsDuctSizeArtifact>::const_iterator ductSizeIterEnd(m_DuctSizeArtifacts.end());
   for ( ; ductSizeIter != ductSizeIter; ductSizeIterEnd++ )
   {
      const pgsDuctSizeArtifact& artifact = ductSizeIter->second;
      if ( !artifact.Passed() )
      {
         return false;
      }
   }

   for ( IndexType lsIdx = 0; lsIdx < (IndexType)(pgsTypes::LimitStateCount); lsIdx++ )
   {
      std::map<IntervalIndexType,std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>>::const_iterator mcIter(m_FlexuralCapacityArtifacts[lsIdx].begin());
      std::map<IntervalIndexType,std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>>::const_iterator mcIterEnd(m_FlexuralCapacityArtifacts[lsIdx].end());
      for ( ; mcIter != mcIterEnd; mcIter++ )
      {
         const std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>& vArtifacts(mcIter->second);
         std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>::const_iterator artifactIter(vArtifacts.begin());
         std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>::const_iterator artifactIterEnd(vArtifacts.end());
         for ( ; artifactIter != artifactIterEnd; artifactIter++ )
         {
            const pgsFlexuralCapacityArtifact& pmArtifact(artifactIter->first);
            const pgsFlexuralCapacityArtifact& nmArtifact(artifactIter->second);

            if ( !pmArtifact.Passed() )
            {
               return false;
            }

            if ( !nmArtifact.Passed() )
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

   std::set<pgsSegmentArtifact>::const_iterator segIter(m_SegmentArtifacts.begin());
   std::set<pgsSegmentArtifact>::const_iterator segEnd(m_SegmentArtifacts.end());
   for ( ; segIter != segEnd; segIter++ )
   {
      const pgsSegmentArtifact& artifact = *segIter;
      if ( !artifact.Passed() )
      {
         return false;
      }
   }

   std::vector<pgsDeflectionCheckArtifact>::const_iterator deflIter(m_DeflectionCheckArtifact.begin());
   std::vector<pgsDeflectionCheckArtifact>::const_iterator deflIterEnd(m_DeflectionCheckArtifact.end());
   for ( ; deflIter != deflIterEnd; deflIter++ )
   {
      const pgsDeflectionCheckArtifact& artifact = *deflIter;
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
      m_FlexuralCapacityArtifacts[lsIdx] = rOther.m_FlexuralCapacityArtifacts[lsIdx];
   }

   m_SegmentArtifacts          = rOther.m_SegmentArtifacts;
   m_ConstructabilityArtifact  = rOther.m_ConstructabilityArtifact;
   m_DeflectionCheckArtifact   = rOther.m_DeflectionCheckArtifact;
}

void pgsGirderArtifact::MakeAssignment(const pgsGirderArtifact& rOther)
{
   MakeCopy( rOther );
}
