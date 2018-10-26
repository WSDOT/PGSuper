///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSEXT_LIFTHAULCONSTANTS_H_
#define INCLUDED_PGSEXT_LIFTHAULCONSTANTS_H_

// enum to indicate which flange cracks first
enum CrackedFlange { TopFlange, BottomFlange };

enum GirderOrientation{ Plumb, Inclined };

// Directions of applied impact
enum ImpactDir {idUp, idNone, idDown, SIZE_OF_IMPACTDIR};

// Functor class for giving tolerance for finding floats in map containers
class Float64_less
{
public:
   bool operator() (const Float64 d1, const Float64 d2) const {return d1+1.0e-9 < d2;} 
};

// Function for comparing two required concrete strengths. -1 means infinite
inline Float64 CompareConcreteStrength(Float64 maxConc, Float64 newConc)
{
   // -1 is magic number meaning no possible value
   if (maxConc==-1.0)
   {
      return -1.0;
   }
   else if (newConc<0)
   {
      return -1.0;
   }
   else
   {
      return max(maxConc, newConc);
   }
}

#endif // INCLUDED_PGSEXT_LIFTHAULCONSTANTS_H_