///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#ifndef INCLUDED_IFACE_ARTIFACT_H_
#define INCLUDED_IFACE_ARTIFACT_H_

/*****************************************************************************
COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

// SYSTEM INCLUDES
//
#if !defined INCLUDED_WBFLTYPES_H_
#include <WbflTypes.h>
#endif

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class pgsGirderArtifact;
class pgsDesignArtifact;
class pgsLiftingAnalysisArtifact;
class pgsHaulingAnalysisArtifact;

class pgsRatingArtifact;

// MISCELLANEOUS
//

/*****************************************************************************
INTERFACE
   IArtifact

   Interface to get girder artifacts

DESCRIPTION
   Interface to get girder artifacts
*****************************************************************************/
// {752DA970-6EA0-11d2-8EEB-006097DF3C68}
DEFINE_GUID(IID_IArtifact, 
0x752da970, 0x6ea0, 0x11d2, 0x8e, 0xeb, 0x0, 0x60, 0x97, 0xdf, 0x3c, 0x68);
interface IArtifact : IUnknown
{
   virtual const pgsGirderArtifact* GetArtifact(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual const pgsDesignArtifact* CreateDesignArtifact(SpanIndexType span,GirderIndexType gdr,arDesignOptions options) = 0;
   virtual const pgsDesignArtifact* GetDesignArtifact(SpanIndexType span,GirderIndexType gdr) = 0;
   virtual void CreateLiftingAnalysisArtifact(SpanIndexType span,GirderIndexType gdr,Float64 supportLoc,pgsLiftingAnalysisArtifact* pArtifact) = 0;
   virtual const pgsHaulingAnalysisArtifact* CreateHaulingAnalysisArtifact(SpanIndexType span,GirderIndexType gdr,Float64 leftSupportLoc,Float64 rightSupportLoc) = 0;

   virtual const pgsRatingArtifact* GetRatingArtifact(GirderIndexType gdrLineIdx,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIndex) = 0;
};

#endif // INCLUDED_IFACE_ARTIFACT_H_

