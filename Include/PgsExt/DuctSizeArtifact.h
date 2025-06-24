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


#include <PgsExt\PgsExtExp.h>

class PGSEXTCLASS pgsDuctSizeArtifact
{
public:
   pgsDuctSizeArtifact();
   ~pgsDuctSizeArtifact() = default;

   pgsDuctSizeArtifact& operator = (const pgsDuctSizeArtifact&) = default;

   void SetRadiusOfCurvature(Float64 r,Float64 rMin);
   void GetRadiusOfCurvature(Float64* pR,Float64* pRmin) const;
   
   void SetDuctArea(Float64 Apt,Float64 Aduct,Float64 Kmax);
   void GetDuctArea(Float64* pApt,Float64* pAduct,Float64* pKmax) const;

   void SetDuctSize(Float64 OD,Float64 minGrossThickness,Float64 Tmax);
   void GetDuctSize(Float64* pOD,Float64* pMinGrossThickness,Float64* pTmax) const;

   bool Passed() const;
   bool PassedRadiusOfCurvature() const;
   bool PassedDuctArea() const;
   bool PassedDuctSize() const;

private:
   Float64 m_Apt;
   Float64 m_Aduct;
   Float64 m_Kmax;

   Float64 m_OD;
   Float64 m_Tmax;
   Float64 m_MinGrossThickness;

   Float64 m_R;
   Float64 m_Rmin;
};
