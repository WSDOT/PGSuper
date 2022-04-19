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

#pragma once

// SYSTEM INCLUDES
//
#include <PgsExt\PgsExtExp.h>

// PROJECT INCLUDES
//
#include <PGSuperTypes.h>
#include <IFace\Project.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

// Function to perform rounding of required slab offset depending on spec method. Returns rounded value
inline Float64 RoundSlabOffsetValue(ISpecification* pSpec, Float64 rawValue)
{
   pgsTypes::SlabOffsetRoundingMethod method;
   Float64 tolerance;
   pSpec->GetRequiredSlabOffsetRoundingParameters(&method, &tolerance);

   // Round slab offset using specified method and tolerance
   Float64 slab_offset_round = rawValue;
   if (!::IsZero(tolerance))
   {
      if (pgsTypes::sormRoundNearest == method)
      {
         slab_offset_round = RoundOff(rawValue, tolerance);
      }
      else if (pgsTypes::sormRoundUp == method)
      {
         slab_offset_round = CeilOff(rawValue, tolerance);
      }
      else
      {
         ATLASSERT(0); // new method??
      }
   }

   return slab_offset_round;
}

