///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

#include "PGSuperAppPlugin\stdafx.h"
#include "Utilities.h"

pgsTypes::SupportedBeamSpacing ToggleGirderSpacingType(pgsTypes::SupportedBeamSpacing spacing)
{
   if ( spacing == pgsTypes::sbsUniform )      // uniform to general
      spacing = pgsTypes::sbsGeneral; 
   else if ( spacing == pgsTypes::sbsGeneral ) // general to uniform
      spacing = pgsTypes::sbsUniform;
   else if ( spacing == pgsTypes::sbsUniformAdjacent ) // uniform adjacent to general adjacent
      spacing = pgsTypes::sbsGeneralAdjacent;
   else if ( spacing == pgsTypes::sbsGeneralAdjacent ) // general adjacent to uniform adjacent
      spacing = pgsTypes::sbsUniformAdjacent;
   else if (spacing == pgsTypes::sbsUniformAdjacentWithTopWidth) // uniform adjacent to general adjacent with top width
      spacing = pgsTypes::sbsGeneralAdjacentWithTopWidth;
   else if (spacing == pgsTypes::sbsGeneralAdjacentWithTopWidth) // general adjacent to uniform adjacent with top width
      spacing = pgsTypes::sbsUniformAdjacentWithTopWidth;
   else
      ATLASSERT(false); // is there a new spacing type???

   return spacing;
}