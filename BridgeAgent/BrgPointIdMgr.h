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

#ifndef INCLUDED_BRGPOINTIDMGR_H_
#define INCLUDED_BRGPOINTIDMGR_H_

// SYSTEM INCLUDES
//
#if !defined INCLUDED_VECTOR_
#include <vector>
#define INCLUDED_VECTOR_
#endif

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   CBrgPointIdMgr

   Manages cogo point id's for the centerline bearing - alignment
   intersection points.


DESCRIPTION
   Manages cogo point id's for the centerline bearing - alignment
   intersection points.

LOG
   rab : 08.21.1998 : Created file
*****************************************************************************/

class CBrgPointIdMgr
{
public:
   enum Location { Start, End };

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CBrgPointIdMgr();

   //------------------------------------------------------------------------
   // Destructor
   ~CBrgPointIdMgr();

   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   void Reset();

   // GROUP: ACCESS

   //------------------------------------------------------------------------
   Int32 GetId( SpanIndexType span, Location loc );

   //------------------------------------------------------------------------
   void SetId( SpanIndexType span, Location loc, Int32 id);

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   std::vector<Int32> m_Ids;

   // GROUP: LIFECYCLE

   // Prevent accidental copying and assignment
   CBrgPointIdMgr(const CBrgPointIdMgr&);
   CBrgPointIdMgr& operator=(const CBrgPointIdMgr&) = delete;

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_BRGPOINTIDMGR_H_
