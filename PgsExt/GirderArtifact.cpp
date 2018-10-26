///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
      return 0;

   const std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>& vArtifacts(found->second);
   return vArtifacts.size();
}

const pgsFlexuralCapacityArtifact* pgsGirderArtifact::GetPositiveMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,CollectionIndexType artifactIdx) const
{
   std::map<IntervalIndexType,std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>>::const_iterator found;
   found = m_FlexuralCapacityArtifacts[ls].find(intervalIdx);
   if ( found == m_FlexuralCapacityArtifacts[ls].end() )
      return NULL;

   const std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>& vArtifacts(found->second);
   const pgsFlexuralCapacityArtifact* pArtifact = &(vArtifacts[artifactIdx].first);
   return pArtifact;
}

const pgsFlexuralCapacityArtifact* pgsGirderArtifact::GetNegativeMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,CollectionIndexType artifactIdx) const
{
   std::map<IntervalIndexType,std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>>::const_iterator found;
   found = m_FlexuralCapacityArtifacts[ls].find(intervalIdx);
   if ( found == m_FlexuralCapacityArtifacts[ls].end() )
      return NULL;

   const std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>& vArtifacts(found->second);
   const pgsFlexuralCapacityArtifact* pArtifact = &(vArtifacts[artifactIdx].second);
   return pArtifact;
}

const pgsFlexuralCapacityArtifact* pgsGirderArtifact::FindPositiveMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi) const
{
   std::map<IntervalIndexType,std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>>::const_iterator found;
   found = m_FlexuralCapacityArtifacts[ls].find(intervalIdx);
   if ( found == m_FlexuralCapacityArtifacts[ls].end() )
      return NULL;

   const std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>& vArtifacts(found->second);
   std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>::const_iterator iter(vArtifacts.begin());
   std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>::const_iterator iterEnd(vArtifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const pgsFlexuralCapacityArtifact* pArtifact = &(iter->first);
      if ( pArtifact->GetPointOfInterest() == poi )
         return pArtifact;
   }

   return NULL;
}

const pgsFlexuralCapacityArtifact* pgsGirderArtifact::FindNegativeMomentFlexuralCapacityArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsPointOfInterest& poi) const
{
   std::map<IntervalIndexType,std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>>::const_iterator found;
   found = m_FlexuralCapacityArtifacts[ls].find(intervalIdx);
   if ( found == m_FlexuralCapacityArtifacts[ls].end() )
      return NULL;

   const std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>& vArtifacts(found->second);
   std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>::const_iterator iter(vArtifacts.begin());
   std::vector<std::pair<pgsFlexuralCapacityArtifact,pgsFlexuralCapacityArtifact>>::const_iterator iterEnd(vArtifacts.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const pgsFlexuralCapacityArtifact* pArtifact = &(iter->second);
      if ( pArtifact->GetPointOfInterest() == poi )
         return pArtifact;
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
      return NULL;

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

void pgsGirderArtifact::AddClosurePourArtifact(const pgsClosurePourArtifact& artifact)
{
   ATLASSERT(artifact.GetClosurePourKey().groupIndex  == m_GirderKey.groupIndex);
   ATLASSERT(artifact.GetClosurePourKey().girderIndex == m_GirderKey.girderIndex);
   m_ClosurePourArtifacts.insert(artifact);
}

const pgsClosurePourArtifact* pgsGirderArtifact::GetClosurePourArtifact(SegmentIndexType segIdx) const
{
   pgsClosurePourArtifact key(CSegmentKey(m_GirderKey,segIdx));
   std::set<pgsClosurePourArtifact>::const_iterator found = m_ClosurePourArtifacts.find(key);
   ATLASSERT(found != m_ClosurePourArtifacts.end());
   if ( found == m_ClosurePourArtifacts.end() )
      return NULL;

   return &(*found);
}

pgsClosurePourArtifact* pgsGirderArtifact::GetClosurePourArtifact(SegmentIndexType segIdx)
{
   pgsClosurePourArtifact key(CSegmentKey(m_GirderKey,segIdx));
   std::set<pgsClosurePourArtifact>::iterator found = m_ClosurePourArtifacts.find(key);
   if ( found == m_ClosurePourArtifacts.end() )
   {
      std::pair<std::set<pgsClosurePourArtifact>::iterator,bool> result = m_ClosurePourArtifacts.insert(key);
      ATLASSERT(result.second == true);
      found = result.first;
   }

   ATLASSERT(found != m_ClosurePourArtifacts.end());
   return &(*found);
}

void pgsGirderArtifact::SetTendonStressArtifact(const pgsTendonStressArtifact& artifact)
{
   m_TendonStressArtifact = artifact;
}

const pgsTendonStressArtifact* pgsGirderArtifact::GetTendonStressArtifact() const
{
   return &m_TendonStressArtifact;
}

pgsTendonStressArtifact* pgsGirderArtifact::GetTendonStressArtifact()
{
   return &m_TendonStressArtifact;
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

Float64 pgsGirderArtifact::GetRequiredConcreteStrength() const
{
   Float64 f = 0;
   std::set<pgsSegmentArtifact>::const_iterator iter(m_SegmentArtifacts.begin());
   std::set<pgsSegmentArtifact>::const_iterator end(m_SegmentArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsSegmentArtifact& artifact = *iter;
      f = max(f,artifact.GetRequiredConcreteStrength());
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
      f = max(f,artifact.GetRequiredReleaseStrength());
   }
   return f;
}

bool pgsGirderArtifact::Passed() const
{
   if ( !m_TendonStressArtifact.Passed() )
      return false;

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
               return false;

            if ( !nmArtifact.Passed() )
               return false;
         }
      }
   }

   std::set<pgsSegmentArtifact>::const_iterator iter(m_SegmentArtifacts.begin());
   std::set<pgsSegmentArtifact>::const_iterator end(m_SegmentArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsSegmentArtifact& artifact = *iter;
      if ( !artifact.Passed() )
         return false;
   }

   std::vector<pgsDeflectionCheckArtifact>::const_iterator deflIter(m_DeflectionCheckArtifact.begin());
   std::vector<pgsDeflectionCheckArtifact>::const_iterator deflIterEnd(m_DeflectionCheckArtifact.end());
   for ( ; deflIter != deflIterEnd; deflIter++ )
   {
      const pgsDeflectionCheckArtifact& artifact = *deflIter;
      if ( !artifact.Passed() )
         return false;
   }

   return true;
}

void pgsGirderArtifact::MakeCopy(const pgsGirderArtifact& rOther)
{
   m_GirderKey                 = rOther.m_GirderKey;
   m_TendonStressArtifact      = rOther.m_TendonStressArtifact;

   for ( IndexType lsIdx = 0; lsIdx < (IndexType)(pgsTypes::LimitStateCount); lsIdx++ )
   {
      m_FlexuralCapacityArtifacts[lsIdx] = rOther.m_FlexuralCapacityArtifacts[lsIdx];
   }

   m_SegmentArtifacts          = rOther.m_SegmentArtifacts;
   m_ClosurePourArtifacts      = rOther.m_ClosurePourArtifacts;
   m_DeflectionCheckArtifact   = rOther.m_DeflectionCheckArtifact;
}

void pgsGirderArtifact::MakeAssignment(const pgsGirderArtifact& rOther)
{
   MakeCopy( rOther );
}
