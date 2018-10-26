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

#ifndef INCLUDED_LIBRARYENTRYOBSERVER_H_
#define INCLUDED_LIBRARYENTRYOBSERVER_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#include <PsgLib\ConcreteLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\RatingLibraryEntry.h>
#include <PsgLib\TrafficBarrierEntry.h>
#include <PsgLib\LiveLoadLibraryEntry.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class CProjectAgentImp;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsLibraryEntryObserver

   Helper class of the Project Agent for observing changes to library entires.


DESCRIPTION
   Helper class of the Project Agent for observing changes to library
   entires.  The user can modify library entries using the library editor (or
   other means when implemented).  This, in effect, is the same as modifying
   the bridge model.  As such, we need to observe the changes and fire change
   events when they occur.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 11.02.1998 : Created file
*****************************************************************************/

class pgsLibraryEntryObserver : public ConcreteLibraryEntryObserver,
                                public GirderLibraryEntryObserver,
                                public SpecLibraryEntryObserver,
                                public RatingLibraryEntryObserver,
                                public TrafficBarrierEntryObserver,
                                public LiveLoadLibraryEntryObserver
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsLibraryEntryObserver( CProjectAgentImp* pAgent = 0);

   //------------------------------------------------------------------------
   // Destructor
   ~pgsLibraryEntryObserver();

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   virtual void Update(ConcreteLibraryEntry* pSubject, Int32 hint);
   virtual void Update(GirderLibraryEntry* pSubject, Int32 hint);
   virtual void Update(SpecLibraryEntry* pSubject, Int32 hint);
   virtual void Update(RatingLibraryEntry* pSubject, Int32 hint);
   virtual void Update(TrafficBarrierEntry* pSubject, Int32 hint);
   virtual void Update(LiveLoadLibraryEntry* pSubject,Int32 hint);

   // GROUP: ACCESS
   void SetAgent(CProjectAgentImp* pAgent);

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
   void ClearStatusItems();

private:
   // GROUP: DATA MEMBERS
   CProjectAgentImp* m_pAgent;

   // GROUP: LIFECYCLE

   // Prevent accidental copying and assignment
   pgsLibraryEntryObserver(const pgsLibraryEntryObserver&);
   pgsLibraryEntryObserver& operator=(const pgsLibraryEntryObserver&);

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

#endif // INCLUDED_LIBRARYENTRYOBSERVER_H_
