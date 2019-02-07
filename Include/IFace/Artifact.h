///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
class pgsSegmentArtifact;
class pgsGirderDesignArtifact;

class pgsRatingArtifact;

class stbLiftingCheckArtifact;
class pgsHaulingAnalysisArtifact;

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
   // Returns a GirderArtifact which captures the full specification check for a girder.
   virtual const pgsGirderArtifact* GetGirderArtifact(const CGirderKey& girderKey) const = 0;

   // Returns a SegmentArtifact which captures the full specification check for an individual segment.
   virtual const pgsSegmentArtifact* GetSegmentArtifact(const CSegmentKey& segmentKey) const = 0;

   // Returns a LiftingAnalysisArtifact which captures the specification checks related to lifting for an individual segment.
   virtual const stbLiftingCheckArtifact* GetLiftingCheckArtifact(const CSegmentKey& segmentKey) const = 0;

   // Returns a HaulingAnalysisArtifact which captures the specification checks related to hauling for an individual segment.
   virtual const pgsHaulingAnalysisArtifact* GetHaulingAnalysisArtifact(const CSegmentKey& segmentKey) const = 0;

   // Creates a DesignArtifact for the specified girder
   virtual const pgsGirderDesignArtifact* CreateDesignArtifact(const CGirderKey& girderKey,const std::vector<arDesignOptions>& options) const = 0;

   // If the specified girder was previously designed, its DesignArtifact is returned, otherwise nullptr
   virtual const pgsGirderDesignArtifact* GetDesignArtifact(const CGirderKey& girderKey) const = 0;

   // Creates a LiftingAnalysisArtifact for the specified segment based on the specified lifting configuration
   virtual void CreateLiftingCheckArtifact(const CSegmentKey& segmentKey,Float64 supportLoc,stbLiftingCheckArtifact* pArtifact) const = 0;

   // Creates a HaulingAnalysisArtifact for the specified segment based on the specified hauling configuration
   virtual const pgsHaulingAnalysisArtifact* CreateHaulingAnalysisArtifact(const CSegmentKey& segmentKey,Float64 leftSupportLoc,Float64 rightSupportLoc) const = 0;

   // Returns the RatingArtifact for the specified girder
   virtual const pgsRatingArtifact* GetRatingArtifact(const CGirderKey& girderKey,pgsTypes::LoadRatingType ratingType,VehicleIndexType vehicleIdx) const = 0;
};

#endif // INCLUDED_IFACE_ARTIFACT_H_

