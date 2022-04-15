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

#ifndef INCLUDED_PGSEXT_LONGREBARINSTANCE_H_
#define INCLUDED_PGSEXT_LONGREBARINSTANCE_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif


// LOCAL INCLUDES
//
#if !defined INCLUDED_MATERIAL_REBAR_H_
#include <Material\Rebar.h>
#endif

#include <GeomModel/Primitives.h>

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsLongRebarInstance

   Instance of a rebar in a cross section.


DESCRIPTION
   Instance of a rebar in a cross section.

LOG
   rdp : 05.31.1999 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsLongRebarInstance
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsLongRebarInstance();


   //------------------------------------------------------------------------
   // full constructor
   pgsLongRebarInstance(const WBFL::Geometry::Point2d& rloc, const matRebar* pRebar, Float64 minCutoffLength);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsLongRebarInstance(const pgsLongRebarInstance& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsLongRebarInstance();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsLongRebarInstance& operator = (const pgsLongRebarInstance& rOther);

   // GROUP: OPERATIONS
   // GROUP: ACCESS

   //------------------------------------------------------------------------
   // Location in section where rebar occurs
   const WBFL::Geometry::Point2d& GetLocation() const;
   void SetLocation(const WBFL::Geometry::Point2d& loc);

   //------------------------------------------------------------------------
   // a pointer to this rebar's material
   const matRebar* GetRebar() const;
   void SetRebar(const matRebar* prebar);

   //------------------------------------------------------------------------
    // Nearest Distance from cut location to either end of the bar
   Float64 GetMinCutoffLength() const;
   void SetMinCutoffLength(Float64 length);

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsLongRebarInstance& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsLongRebarInstance& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
      WBFL::Geometry::Point2d       m_Location;
      const matRebar* m_pRebar;
      Float64         m_MinCutoffLength; // Distance from cut location
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

public:
   // GROUP: DEBUG
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns true if the object is in a valid state, otherwise returns false.
   virtual bool AssertValid() const;

   //------------------------------------------------------------------------
   // Dumps the contents of the object to the given dump context.
   virtual void Dump(dbgDumpContext& os) const;
   #endif // _DEBUG

   #if defined _UNITTEST
   //------------------------------------------------------------------------
   // Runs a self-diagnostic test.  Returns true if the test passed,
   // otherwise false.
   static bool TestMe(dbgLog& rlog);
   #endif // _UNITTEST
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_PGSEXT_LONGREBARINSTANCE_H_
