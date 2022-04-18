///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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
#include <PgsExt\GirderDesignArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsGirderDesignArtifact
****************************************************************************/
pgsGirderDesignArtifact::pgsGirderDesignArtifact(const CGirderKey& girderKey) :
m_GirderKey(girderKey)
{
}

pgsGirderDesignArtifact::pgsGirderDesignArtifact(const pgsGirderDesignArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsGirderDesignArtifact::~pgsGirderDesignArtifact()
{
}

pgsGirderDesignArtifact& pgsGirderDesignArtifact::operator= (const pgsGirderDesignArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

const CGirderKey& pgsGirderDesignArtifact::GetGirderKey() const
{
   return m_GirderKey;
}

void pgsGirderDesignArtifact::AddSegmentDesignArtifact(SegmentIndexType segIdx,const pgsSegmentDesignArtifact& segmentArtifact)
{
   ATLASSERT(CSegmentKey(m_GirderKey,segIdx) == segmentArtifact.GetSegmentKey());
   std::map<SegmentIndexType,pgsSegmentDesignArtifact>::iterator found = m_SegmentArtifacts.find(segIdx);
   if ( found == m_SegmentArtifacts.end() )
   {
      // haven't attempted a design for this artifact yet so just add the artifact to the collection
      std::pair<std::map<SegmentIndexType,pgsSegmentDesignArtifact>::iterator,bool> result = m_SegmentArtifacts.insert(std::make_pair(segIdx,segmentArtifact));
      ATLASSERT(result.second == true);
   }
   else
   {
      // this segment was already designed so replace the previous artifact with this new one.

      // before we do the assignment, we need to capture any previous failed design attempts
      // in the older artifact. this is kind of a hack, but it works for now
      segmentArtifact.UpdateFailedFlexuralDesigns(found->second);
      found->second = segmentArtifact;
   }
}

const pgsSegmentDesignArtifact* pgsGirderDesignArtifact::GetSegmentDesignArtifact(SegmentIndexType segIdx) const
{
   std::map<SegmentIndexType,pgsSegmentDesignArtifact>::const_iterator found(m_SegmentArtifacts.find(segIdx));
   ATLASSERT(found != m_SegmentArtifacts.end());
   return &(found->second);
}

pgsSegmentDesignArtifact* pgsGirderDesignArtifact::GetSegmentDesignArtifact(SegmentIndexType segIdx)
{
   std::map<SegmentIndexType,pgsSegmentDesignArtifact>::iterator found(m_SegmentArtifacts.find(segIdx));
   ATLASSERT(found != m_SegmentArtifacts.end());
   return &(found->second);
}

void pgsGirderDesignArtifact::MakeCopy(const pgsGirderDesignArtifact& rOther)
{
   m_GirderKey = rOther.m_GirderKey;
   m_SegmentArtifacts = rOther.m_SegmentArtifacts;
}

void pgsGirderDesignArtifact::MakeAssignment(const pgsGirderDesignArtifact& rOther)
{
   MakeCopy( rOther );
}
