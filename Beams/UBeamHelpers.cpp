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

#include "stdafx.h"
#include "UBeamHelpers.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void GetSlope(IUBeam* beam,Float64* slope)
{
   Float64 d1, d4, d5;
   Float64 w1, w2, w5;
   beam->get_D1(&d1);
   beam->get_D4(&d4);
   beam->get_D5(&d5);
   beam->get_W1(&w1);
   beam->get_W2(&w2);
   beam->get_W5(&w5);

   Float64 rise = d1 - d4 - d5;
   Float64 run = (w2 - w1)/2 - w5;

   if ( IsZero(run) )
      *slope = DBL_MAX;
   else
      *slope = rise/run;
}

Float64 ComputeSlope(Float64 t,Float64 d1,Float64 d6,Float64 w1,Float64 w2,Float64 w4)
{
   WBFL::Math::BrentsRootFinder rootfinder;
   Float64 slope;
   try
   {
      slope = rootfinder.FindRootInRange(SlopeFn(t,d1,d6,w1,w2,w4),0,1000,0.00001);
   }
   catch(...)
   {
      slope = DBL_MAX;
   }

   return slope;
}
