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
#include <PgsExt\LiftHaulConstants.h>

Float64 CompareConcreteStrength(Float64 maxConc, Float64 newConc)
{
   // NO_AVAILABLE_CONCRETE_STRENGTH is magic number meaning no possible value
   if (maxConc == NO_AVAILABLE_CONCRETE_STRENGTH)
   {
      return NO_AVAILABLE_CONCRETE_STRENGTH;
   }
   else if (newConc < 0)
   {
      return NO_AVAILABLE_CONCRETE_STRENGTH;
   }
   else
   {
      return Max(maxConc, newConc);
   }
}
