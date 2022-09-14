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

#include <Math\Math.h>


void GetSlope(IUBeam* beam,Float64* slope);

class SlopeFn : public WBFL::Math::Function
{
public:
   SlopeFn(Float64 t,Float64 d1,Float64 d6,Float64 w1,Float64 w2,Float64 w4)
   {
      T  = t;
      D1 = d1;
      D6 = d6;
      W1 = w1;
      W2 = w2;
      W4 = w4;
   }

   Float64 Evaluate(Float64 x) const
   {
      return T*sqrt(x*x+1) + D6 + ((W2/2) - W4 - (W1)/2)*x - D1;
   }

  std::unique_ptr<WBFL::Math::Function> Clone() const
   {
      ASSERT(false); // should never get here
      return nullptr;
   }

private:
   Float64 T, D1, D6, W1, W2, W4;
};

// computes the web slope based on dimensions from the parent_version 1.2 data format
Float64 ComputeSlope(Float64 t,Float64 d1,Float64 d6,Float64 w1,Float64 w2,Float64 w4);
