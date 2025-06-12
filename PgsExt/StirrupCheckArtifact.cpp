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
#include <PgsExt\StirrupCheckArtifact.h>


/****************************************************************************
CLASS
   pgsStirrupCheckArtifact
****************************************************************************/

pgsStirrupCheckArtifact::pgsStirrupCheckArtifact()
{
}

pgsStirrupCheckArtifact::pgsStirrupCheckArtifact(const pgsStirrupCheckArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsStirrupCheckArtifact::~pgsStirrupCheckArtifact()
{
}

pgsStirrupCheckArtifact& pgsStirrupCheckArtifact::operator= (const pgsStirrupCheckArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void pgsStirrupCheckArtifact::AddStirrupCheckAtPoisArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,const pgsStirrupCheckAtPoisArtifact& artifact)
{
   std::vector<pgsStirrupCheckAtPoisArtifact>& vArtifacts(GetStirrupCheckArtifacts(intervalIdx,ls));
   vArtifacts.push_back(artifact);
   std::sort(vArtifacts.begin(),vArtifacts.end());
}

IndexType pgsStirrupCheckArtifact::GetStirrupCheckAtPoisArtifactCount(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const
{
   const std::vector<pgsStirrupCheckAtPoisArtifact>& vArtifacts(GetStirrupCheckArtifacts(intervalIdx,ls));
   return vArtifacts.size();
}

const pgsStirrupCheckAtPoisArtifact* pgsStirrupCheckArtifact::GetStirrupCheckAtPoisArtifact(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,IndexType index) const
{
   const std::vector<pgsStirrupCheckAtPoisArtifact>& vArtifacts(GetStirrupCheckArtifacts(intervalIdx,ls));
   return &vArtifacts[index];
}

const pgsStirrupCheckAtPoisArtifact* pgsStirrupCheckArtifact::GetStirrupCheckAtPoisArtifactAtPOI(IntervalIndexType intervalIdx,pgsTypes::LimitState ls,PoiIDType poiID) const
{
   const std::vector<pgsStirrupCheckAtPoisArtifact>& vArtifacts(GetStirrupCheckArtifacts(intervalIdx,ls));
   std::vector<pgsStirrupCheckAtPoisArtifact>::const_iterator iter(vArtifacts.begin());
   std::vector<pgsStirrupCheckAtPoisArtifact>::const_iterator end(vArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsStirrupCheckAtPoisArtifact& artifact = *iter;
      if ( artifact.GetPointOfInterest().GetID() == poiID )
      {
         return &artifact;
      }
   }

   return nullptr;
}

void pgsStirrupCheckArtifact::SetConfinementArtifact(const pgsConfinementCheckArtifact& artifact)
{
   m_ConfinementArtifact = artifact;
}

const pgsConfinementCheckArtifact& pgsStirrupCheckArtifact::GetConfinementArtifact() const
{
   return m_ConfinementArtifact;
}

void pgsStirrupCheckArtifact::SetSplittingCheckArtifact(std::shared_ptr<pgsSplittingCheckArtifact> pArtifact)
{
   m_SplittingCheckArtifact = pArtifact;
}

const std::shared_ptr<pgsSplittingCheckArtifact> pgsStirrupCheckArtifact::GetSplittingCheckArtifact() const
{
   return m_SplittingCheckArtifact;
}

void pgsStirrupCheckArtifact::Clear()
{
   m_StirrupCheckAtPoisArtifacts.clear();
}

bool pgsStirrupCheckArtifact::Passed() const
{
   std::map<Key,std::vector<pgsStirrupCheckAtPoisArtifact>>::const_iterator item_iter(m_StirrupCheckAtPoisArtifacts.begin());
   std::map<Key,std::vector<pgsStirrupCheckAtPoisArtifact>>::const_iterator item_end(m_StirrupCheckAtPoisArtifacts.end());
   for ( ; item_iter != item_end; item_iter++ )
   {
      const std::pair<Key,std::vector<pgsStirrupCheckAtPoisArtifact>>& item(*item_iter);
      std::vector<pgsStirrupCheckAtPoisArtifact>::const_iterator iter(item.second.begin());
      std::vector<pgsStirrupCheckAtPoisArtifact>::const_iterator end(item.second.end());
      for ( ; iter != end; iter++ )
      {
         const pgsStirrupCheckAtPoisArtifact& artifact(*iter);
         if ( !artifact.Passed() )
         {
            return false;
         }
      }
   }

   bool bPassed = true;

   if (m_SplittingCheckArtifact)
   {
      bPassed &= m_SplittingCheckArtifact->Passed();
   }

   bPassed &= m_ConfinementArtifact.Passed();

   return bPassed;
}

void pgsStirrupCheckArtifact::MakeCopy(const pgsStirrupCheckArtifact& rOther)
{
   m_StirrupCheckAtPoisArtifacts   = rOther.m_StirrupCheckAtPoisArtifacts;
   m_ConfinementArtifact           = rOther.m_ConfinementArtifact;
   m_SplittingCheckArtifact         = rOther.m_SplittingCheckArtifact;
}

void pgsStirrupCheckArtifact::MakeAssignment(const pgsStirrupCheckArtifact& rOther)
{
   MakeCopy( rOther );
}

std::vector<pgsStirrupCheckAtPoisArtifact>& pgsStirrupCheckArtifact::GetStirrupCheckArtifacts(IntervalIndexType intervalIdx,pgsTypes::LimitState ls) const
{
   Key key;
   key.intervalIdx = intervalIdx;
   key.ls = ls;
   std::map<Key,std::vector<pgsStirrupCheckAtPoisArtifact>>::iterator found( m_StirrupCheckAtPoisArtifacts.find(key) );
   if ( found == m_StirrupCheckAtPoisArtifacts.end() )
   {
      std::vector<pgsStirrupCheckAtPoisArtifact> vArtifacts;
      std::pair<std::map<Key,std::vector<pgsStirrupCheckAtPoisArtifact>>::iterator,bool> results(m_StirrupCheckAtPoisArtifacts.insert(std::make_pair(key,vArtifacts)));
      found = results.first;
   }

   return found->second;
}
