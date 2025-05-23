///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#include <PsgLib\DuctLibraryEntry.h>
#include <PsgLib\HaulTruckLibraryEntry.h>

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

LOG
   rab : 11.02.1998 : Created file
*****************************************************************************/

class pgsLibraryEntryObserver : public ConcreteLibraryEntryObserver,
                                public GirderLibraryEntryObserver,
                                public SpecLibraryEntryObserver,
                                public RatingLibraryEntryObserver,
                                public TrafficBarrierEntryObserver,
                                public LiveLoadLibraryEntryObserver,
                                public DuctLibraryEntryObserver,
                                public HaulTruckLibraryEntryObserver
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
   virtual void Update(ConcreteLibraryEntry& subject, Int32 hint) override;
   virtual void Update(GirderLibraryEntry& subject, Int32 hint) override;
   virtual void Update(SpecLibraryEntry& subject, Int32 hint) override;
   virtual void Update(RatingLibraryEntry& subject, Int32 hint) override;
   virtual void Update(TrafficBarrierEntry& subject, Int32 hint) override;
   virtual void Update(LiveLoadLibraryEntry& subject,Int32 hint) override;
   virtual void Update(DuctLibraryEntry& subject,Int32 hint) override;
   virtual void Update(HaulTruckLibraryEntry& subject,Int32 hint) override;

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
   pgsLibraryEntryObserver(const pgsLibraryEntryObserver&) = delete;
   pgsLibraryEntryObserver& operator=(const pgsLibraryEntryObserver&) = delete;

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_LIBRARYENTRYOBSERVER_H_
