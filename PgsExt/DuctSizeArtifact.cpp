///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include <PgsExt\DuctSizeArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsDuctSizeArtifact
****************************************************************************/

pgsDuctSizeArtifact::pgsDuctSizeArtifact()
{
   m_Apt = 0;
   m_Aduct = 0;
   m_Kmax = 0;

   m_OD = 0;
   m_Tmax = 0;
   m_MinGrossThickness = 0;

   m_R = 0;
   m_Rmin = 0;
}

pgsDuctSizeArtifact::pgsDuctSizeArtifact(const pgsDuctSizeArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsDuctSizeArtifact::~pgsDuctSizeArtifact()
{
}

pgsDuctSizeArtifact& pgsDuctSizeArtifact::operator= (const pgsDuctSizeArtifact& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void pgsDuctSizeArtifact::SetRadiusOfCurvature(Float64 r,Float64 rMin)
{
   m_R = r;
   m_Rmin = rMin;
}

void pgsDuctSizeArtifact::GetRadiusOfCurvature(Float64* pR,Float64* pRmin) const
{
   *pR = m_R;
   *pRmin = m_Rmin;
}

void pgsDuctSizeArtifact::SetDuctArea(Float64 Apt,Float64 Aduct,Float64 Kmax)
{
   m_Apt = Apt;
   m_Aduct = Aduct;
   m_Kmax = Kmax;
}

void pgsDuctSizeArtifact::GetDuctArea(Float64* pApt,Float64* pAduct,Float64* pKmax) const
{
   *pApt = m_Apt;
   *pAduct = m_Aduct;
   *pKmax = m_Kmax;
}

void pgsDuctSizeArtifact::SetDuctSize(Float64 OD,Float64 minGrossThickness,Float64 Tmax)
{
   m_OD = OD;
   m_MinGrossThickness = minGrossThickness;
   m_Tmax = Tmax;
}

void pgsDuctSizeArtifact::GetDuctSize(Float64* pOD,Float64* pMinGrossThickness,Float64* pTmax) const
{
   *pOD = m_OD;
   *pMinGrossThickness = m_MinGrossThickness;
   *pTmax = m_Tmax;
}

bool pgsDuctSizeArtifact::Passed() const
{
   return PassedDuctArea() && PassedDuctSize() & PassedRadiusOfCurvature();
}

bool pgsDuctSizeArtifact::PassedRadiusOfCurvature() const
{
   return m_Rmin <= m_R ? true : false;
}

bool pgsDuctSizeArtifact::PassedDuctArea() const
{
   return m_Kmax*m_Apt <= m_Aduct;
}

bool pgsDuctSizeArtifact::PassedDuctSize() const
{
   return m_OD <= m_Tmax*m_MinGrossThickness;
}

void pgsDuctSizeArtifact::MakeCopy(const pgsDuctSizeArtifact& rOther)
{
   m_Apt = rOther.m_Apt;
   m_Aduct = rOther.m_Aduct;
   m_Kmax = rOther.m_Kmax;

   m_OD = rOther.m_OD;
   m_Tmax = rOther.m_Tmax;
   m_MinGrossThickness = rOther.m_MinGrossThickness;

   m_R = rOther.m_R;
   m_Rmin = rOther.m_Rmin;
}

void pgsDuctSizeArtifact::MakeAssignment(const pgsDuctSizeArtifact& rOther)
{
   MakeCopy( rOther );
}
