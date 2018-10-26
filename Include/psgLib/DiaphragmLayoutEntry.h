///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#ifndef INCLUDED_DIAPHRAGMLAYOUTENTRY_H_
#define INCLUDED_DIAPHRAGMLAYOUTENTRY_H_

// SYSTEM INCLUDES
//
#if !defined INCLUDED_VECTOR_
#include <vector>
#define INCLUDED_VECTOR_
#endif

#if !defined INCLUDED_MATHEX_H_
#include <MathEx.h>
#endif

// PROJECT INCLUDES
//
#include "psgLibLib.h"

#include <psgLib\ISupportIcon.h>
#include <libraryFw\LibraryEntry.h>

// LOCAL INCLUDES
//

#if !defined INCLUDED_SYSTEM_SUBJECTT_H_
#include <System\SubjectT.h>
#endif

// FORWARD DECLARATIONS
//
class DiaphragmLayoutEntry;
class DiaphragmLayoutEntryObserver;
#pragma warning(disable:4231)
PSGLIBTPL sysSubjectT<DiaphragmLayoutEntryObserver, DiaphragmLayoutEntry>;

// MISCELLANEOUS
//
/*****************************************************************************
CLASS 
   DiaphragmLayoutEntryObserver

   A pure virtual entry class for observing Diaphragm material entries.


DESCRIPTION
   This class may be used to describe observe Diaphragm  materials in a library.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 07.20.1998 : Created file
*****************************************************************************/
class PSGLIBCLASS DiaphragmLayoutEntryObserver
{
public:

   // GROUP: LIFECYCLE
   //------------------------------------------------------------------------
   // called by our subject to let us now he's changed, along with an optional
   // hint
   virtual void Update(DiaphragmLayoutEntry* pSubject, Int32 hint)=0;
};

/*****************************************************************************
CLASS 
   DiaphragmLayoutEntry

   A library entry class for diaphragm layouts


DESCRIPTION
   This class may be used to describe diaphragm layouts in a library.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 07.20.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS DiaphragmLayoutEntry : public libLibraryEntry, public ISupportIcon,
       public sysSubjectT<DiaphragmLayoutEntryObserver, DiaphragmLayoutEntry>
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // a struct that describes the layout
   struct DiaphragmLayout {
      Float64 EndOfRange;         // Longest span length that layout applies to
      Int32   NumberOfDiaphragms; // Number of evenly-spaced diaphragms in this range.
      bool operator==(const DiaphragmLayout& rOther) const
      {return ::IsEqual(EndOfRange, rOther.EndOfRange) &&
              NumberOfDiaphragms == rOther.NumberOfDiaphragms;} 
   };
   // Vector of layouts. Span lengths must be order from shortest to longest.
   typedef std::vector<DiaphragmLayout> DiaphragmLayoutVec;

   //------------------------------------------------------------------------
   // Default constructor
   DiaphragmLayoutEntry();

   //------------------------------------------------------------------------
   // Copy constructor
   DiaphragmLayoutEntry(const DiaphragmLayoutEntry& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~DiaphragmLayoutEntry();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   DiaphragmLayoutEntry& operator = (const DiaphragmLayoutEntry& rOther);

   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   // Get the icon for this entry
   virtual HICON GetIcon() const;

   //------------------------------------------------------------------------
   // Edit the entry
   virtual bool Edit(libUnitsMode::Mode mode, bool allowEditing);

   //------------------------------------------------------------------------
   // Save to structured storage
   virtual bool SaveMe(sysIStructuredSave* pSave);

   //------------------------------------------------------------------------
   // Load from structured storage
   virtual bool LoadMe(sysIStructuredLoad* pLoad);

    // GROUP: ACCESS
   //------------------------------------------------------------------------
   // SetDiaphragmLayout - set diaphram layout
   void SetDiaphragmLayout(const DiaphragmLayoutVec& vec);

   //------------------------------------------------------------------------
   // GetDiaphragmLayout - get diaphragm layout
   DiaphragmLayoutVec GetDiaphragmLayout() const;

   //------------------------------------------------------------------------
   // Equality - test if two entries are equal. Ignore names by default
   bool IsEqual(const DiaphragmLayoutEntry& rOther, bool considerName=false) const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   void MakeCopy(const DiaphragmLayoutEntry& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const DiaphragmLayoutEntry& rOther);
  // GROUP: ACCESS
  // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   DiaphragmLayoutVec m_DiaphragmLayoutVec;

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

#endif // INCLUDED_DIAPHRAGMLAYOUTENTRY_H_
