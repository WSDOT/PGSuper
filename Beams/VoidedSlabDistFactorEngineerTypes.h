///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

struct VOIDEDSLAB_J_SOLID
{
   Float64 A;
   Float64 Ip;
};

struct VOIDEDSLAB_J_VOID
{
   Float64 Ao;
   typedef std::pair<Float64,Float64> Element; // first = s, second = t
   std::vector<Element> Elements;
   Float64 S_over_T; // Sum of s/t for all the elements
};

struct VOIDEDSLAB_LLDFDETAILS : public BASE_LLDFDETAILS
{
   Float64 L;
   Float64 I;
   Float64 b;
   Float64 d;
   Float64 leftDe;
   Float64 rightDe;
   Float64 J;
   Float64 PossionRatio;
   pgsTypes::AdjacentTransverseConnectivity TransverseConnectivity;

   Int16 nVoids;

   VOIDEDSLAB_J_SOLID Jsolid;
   VOIDEDSLAB_J_VOID  Jvoid;
};
