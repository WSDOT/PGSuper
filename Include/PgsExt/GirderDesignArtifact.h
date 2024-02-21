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

#pragma once

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\SegmentDesignArtifact.h>

/*****************************************************************************
CLASS 
   pgsGirderDesignArtifact

   Artifact that contains the results of a design attempt.


DESCRIPTION
   Artifact that contains the results of a design attempt.

LOG
   rab : 12.09.1998 : Created file
*****************************************************************************/

// NOTE: Eventually the girder design artifact will be fleshed out with
// spliced girder design information. The segment design artifact takes care
// of the individual segments. This artifact will have designed post-tensioning
// closure joints (probably in a closure joint design artifact), etc.
class PGSEXTCLASS pgsGirderDesignArtifact
{
public:
   pgsGirderDesignArtifact(const CGirderKey& girderKey);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsGirderDesignArtifact(const pgsGirderDesignArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsGirderDesignArtifact();

   pgsGirderDesignArtifact& operator = (const pgsGirderDesignArtifact& rOther);

   const CGirderKey& GetGirderKey() const;
   void AddSegmentDesignArtifact(SegmentIndexType segIdx,const pgsSegmentDesignArtifact& segmentArtifact);
   const pgsSegmentDesignArtifact* GetSegmentDesignArtifact(SegmentIndexType segIdx) const;
   pgsSegmentDesignArtifact* GetSegmentDesignArtifact(SegmentIndexType segIdx);

protected:
   void MakeCopy(const pgsGirderDesignArtifact& rOther);
   void MakeAssignment(const pgsGirderDesignArtifact& rOther);

private:
   CGirderKey m_GirderKey;
   std::map<SegmentIndexType,pgsSegmentDesignArtifact> m_SegmentArtifacts;
};
