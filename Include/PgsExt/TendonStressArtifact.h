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

#pragma once
#include <PgsExt\PgsExtExp.h>


/*****************************************************************************
CLASS 
   pgsTendonStressArtifact

   Artifact for post-tension tendon stress checks.


DESCRIPTION
   Artifact for post-tension tendon stress checks. Records tendon stress and
   allowables at each poi along the tendon for the cases of
   1) Prior to seating
   2) Immediately after anchor set
   3) Service Limit State after losses


COPYRIGHT
   Copyright © 1997-2011
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 12.22.2011 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsTendonStressArtifact
{
public:
   pgsTendonStressArtifact();
   pgsTendonStressArtifact(const pgsTendonStressArtifact& rOther);
   virtual ~pgsTendonStressArtifact();

   pgsTendonStressArtifact& operator = (const pgsTendonStressArtifact& rOther);

   bool Passed() const;

protected:
   void MakeCopy(const pgsTendonStressArtifact& rOther);
   virtual void MakeAssignment(const pgsTendonStressArtifact& rOther);
};
