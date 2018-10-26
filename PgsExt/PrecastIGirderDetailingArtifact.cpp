///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include <PgsExt\PrecastIGirderDetailingArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsPrecastIGirderDetailingArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsPrecastIGirderDetailingArtifact::pgsPrecastIGirderDetailingArtifact()
{
}

pgsPrecastIGirderDetailingArtifact::pgsPrecastIGirderDetailingArtifact(const pgsPrecastIGirderDetailingArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsPrecastIGirderDetailingArtifact::~pgsPrecastIGirderDetailingArtifact()
{
}

//======================== OPERATORS  =======================================
pgsPrecastIGirderDetailingArtifact& pgsPrecastIGirderDetailingArtifact::operator=(const pgsPrecastIGirderDetailingArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================

Float64 pgsPrecastIGirderDetailingArtifact::GetMinTopFlangeThickness() const
{
   return m_MinTopFlangeThickness;
}

void pgsPrecastIGirderDetailingArtifact::SetMinTopFlangeThickness(Float64 thick)
{
   m_MinTopFlangeThickness = thick;
}

Float64 pgsPrecastIGirderDetailingArtifact::GetMinWebThickness() const
{
   return m_MinWebThickness;
}

void pgsPrecastIGirderDetailingArtifact::SetMinWebThickness(Float64 thick) 
{
   m_MinWebThickness = thick;
}

Float64 pgsPrecastIGirderDetailingArtifact::GetMinBottomFlangeThickness() const
{
   return m_MinBottomFlangeThickness;
}

void pgsPrecastIGirderDetailingArtifact::SetMinBottomFlangeThickness(Float64 thick) 
{
   m_MinBottomFlangeThickness = thick;
}

Float64 pgsPrecastIGirderDetailingArtifact::GetProvidedTopFlangeThickness() const
{
   return m_ProvidedTopFlangeThickness;
}

void pgsPrecastIGirderDetailingArtifact::SetProvidedTopFlangeThickness(Float64 thick) 
{
   m_ProvidedTopFlangeThickness = thick;
}

Float64 pgsPrecastIGirderDetailingArtifact::GetProvidedWebThickness() const
{
   return m_ProvidedWebThickness;
}

void pgsPrecastIGirderDetailingArtifact::SetProvidedWebThickness(Float64 thick) 
{
   m_ProvidedWebThickness = thick;
}

Float64 pgsPrecastIGirderDetailingArtifact::GetProvidedBottomFlangeThickness() const
{
   return m_ProvidedBottomFlangeThickness;
}

void pgsPrecastIGirderDetailingArtifact::SetProvidedBottomFlangeThickness(Float64 thick)
{
   m_ProvidedBottomFlangeThickness = thick;
}

bool pgsPrecastIGirderDetailingArtifact::Passed() const
{
   if (m_ProvidedTopFlangeThickness < m_MinTopFlangeThickness && !IsZero(m_ProvidedTopFlangeThickness) )
      return false;

   if (m_ProvidedWebThickness < m_MinWebThickness && !IsZero(m_ProvidedWebThickness) )
      return false;

   if (m_ProvidedBottomFlangeThickness < m_MinBottomFlangeThickness && !IsZero(m_ProvidedBottomFlangeThickness))
      return false;

   return true;
}

 //======================== ACCESS     =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsPrecastIGirderDetailingArtifact::MakeCopy(const pgsPrecastIGirderDetailingArtifact& rOther)
{
   m_MinTopFlangeThickness = rOther.m_MinTopFlangeThickness;
   m_MinWebThickness = rOther.m_MinWebThickness;
   m_MinBottomFlangeThickness = rOther.m_MinBottomFlangeThickness;

   m_ProvidedTopFlangeThickness = rOther.m_ProvidedTopFlangeThickness;
   m_ProvidedWebThickness = rOther.m_ProvidedWebThickness;
   m_ProvidedBottomFlangeThickness = rOther.m_ProvidedBottomFlangeThickness;
}

void pgsPrecastIGirderDetailingArtifact::MakeAssignment(const pgsPrecastIGirderDetailingArtifact& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsPrecastIGirderDetailingArtifact::AssertValid() const
{
   return true;
}

void pgsPrecastIGirderDetailingArtifact::Dump(dbgDumpContext& os) const
{
   os << "Dump for pgsPrecastIGirderDetailingArtifact" << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsPrecastIGirderDetailingArtifact::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsPrecastIGirderDetailingArtifact");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsPrecastIGirderDetailingArtifact");

   TESTME_EPILOG("pgsPrecastIGirderDetailingArtifact");
}
#endif // _UNITTEST
