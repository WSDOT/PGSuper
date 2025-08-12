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

#pragma once

#include <PsgLib/PointOfInterest.h>


// Simple span bearing reactions can occur at back/ahead. Continuous and pier reactions are at mid
typedef enum PierReactionFaceType
{
   rftBack,
   rftMid,
   rftAhead
} PierReactionFaceType;

struct ReactionLocation
{
   PierIndexType        PierIdx;   // Index of the pier where reactions are reported
   PierReactionFaceType Face;      // Face of pier that reaction applies to
   CGirderKey           GirderKey; // GirderKey for the girder that provides the reaction
   pgsPointOfInterest   poi; // Point of interest for the reaction location (bearing)
   std::_tstring        PierLabel; // Label (Abutment 1, Pier 2, etc)

   bool operator==(const ReactionLocation& r) const
   {
      return ::IsEqual(PierIdx, r.PierIdx) && Face == r.Face && GirderKey == r.GirderKey ? true : false;
   };

};

