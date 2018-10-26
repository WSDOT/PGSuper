///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

#ifndef INCLUDED_POIMGR_H_
#define INCLUDED_POIMGR_H_

// SYSTEM INCLUDES
//
#if !defined INCLUDED_SET_
#include <set>
#define INCLUDED_SET_
#endif

// PROJECT INCLUDES
//

#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#if !defined INCLUDED_PGSEXT_POINTOFINTEREST_H_
#include <PgsExt\PointOfInterest.h>
#endif

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//
#define POIMGR_AND  1
#define POIMGR_OR 2

/*****************************************************************************
CLASS 
   pgsPoiMgr

   Point of interest manager.  Objects of this class manage points of
   interest.


DESCRIPTION
   Point of interest manager.  Objects of this class manage points of
   interest.  Management includes storage and retreival, and elimination of
   duplicates.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 10.20.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsPoiMgr
{
public:
   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsPoiMgr();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsPoiMgr(const pgsPoiMgr& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsPoiMgr();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsPoiMgr& operator = (const pgsPoiMgr& rOther);

   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // Adds a point of interest.
   PoiIDType AddPointOfInterest(const pgsPointOfInterest& poi);

   //------------------------------------------------------------------------
   // Removes a point of interest that exactly matches the specified poi.
   void RemovePointOfInterest(const pgsPointOfInterest& poi);

   //------------------------------------------------------------------------
   // Removes a point of interest from the specified location.  The tolerance
   // setting is used in the decision on whether to remove a poi or not.
   void RemovePointOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 distFromStart);

   //------------------------------------------------------------------------
   // Removes all points of interest.
   void RemoveAll();

   //------------------------------------------------------------------------
   // Returns the point of interest at the specified location. If not found,
   // returns a default poi
   pgsPointOfInterest GetPointOfInterest(SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64 distFromStart);

   //------------------------------------------------------------------------
   // Returns the point of interest at the specified location and stage. If not found,
   // returns a default poi
   pgsPointOfInterest GetPointOfInterest(pgsTypes::Stage stage,SpanIndexType spanIdx,GirderIndexType gdrIdx,Float64 distFromStart);

   //------------------------------------------------------------------------
   // Returns the point of interest nearest to the specified location, regardless of stage.
   pgsPointOfInterest GetNearestPointOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 distFromStart);

   //------------------------------------------------------------------------
   // Returns the point of interest nearest to the specified location that is defined in at least the given stage
   pgsPointOfInterest GetNearestPointOfInterest(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,Float64 distFromStart);

   //------------------------------------------------------------------------
   // Returns the point of interest at the specified location or a default if not found. 
   pgsPointOfInterest GetPointOfInterest(PoiIDType id) const;

   //------------------------------------------------------------------------
   // Returns a vector of pointers to Points of Interest that have the
   // specified attributes for the given stage.
   void GetPointsOfInterest(SpanIndexType span,GirderIndexType gdr,pgsTypes::Stage stage,PoiAttributeType attrib,Uint32 mode,std::vector<pgsPointOfInterest>* pPois) const;

   //------------------------------------------------------------------------
   // Returns a vector of pointers to Points of Interest that belong to the specified stage
   // and have the specified attributes for the specified girder.
   // must match attribute in all stages
   void GetPointsOfInterest(SpanIndexType span,GirderIndexType gdr,const std::vector<pgsTypes::Stage>& stages,PoiAttributeType attrib,Uint32 mode,std::vector<pgsPointOfInterest>* pPois) const;

   std::vector<pgsPointOfInterest> GetPointsOfInterest(SpanIndexType span,GirderIndexType gdr) const;

   void GetTenthPointPOIs(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,std::vector<pgsPointOfInterest>* pPois) const;

   //------------------------------------------------------------------------
   // Replaces a previously defined point of interest
   bool ReplacePointOfInterest(PoiIDType ID,const pgsPointOfInterest& poi);

   // GROUP: ACCESS

   //------------------------------------------------------------------------
   // Sets the tolerance for eliminating duplicate points of interest.  If two
   // points of interest, on the same girder, are with this tolerance of each
   // other,  they are considered to be at the same point.  These std::vector<pgsPointOfInterest>'s are merged,
   // maintaining the attributes of both std::vector<pgsPointOfInterest>.
   //
   // Changing the tolerance does not effect previously stored points of interest.
   void SetTolerance(Float64 tol);

   //------------------------------------------------------------------------
   // Returns the std::vector<pgsPointOfInterest> tolerance.
   Float64 GetTolerance() const;

   // GROUP: INQUIRY

   //------------------------------------------------------------------------
   // Returns the number of points of interest.
   Uint32 GetPointOfInterestCount() const;

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsPoiMgr& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsPoiMgr& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   static PoiIDType ms_NextID;
   std::vector<pgsPointOfInterest> m_Poi;
   Float64 m_Tolerance;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   bool AtSamePlace(const pgsPointOfInterest& a,const pgsPointOfInterest& b);
   pgsPointOfInterest Merge(const pgsPointOfInterest& a,const pgsPointOfInterest& b);
   void AndFind(SpanIndexType span,GirderIndexType gdr,pgsTypes::Stage stage,PoiAttributeType attrib,std::vector<pgsPointOfInterest>* pPois) const;
   void AndFind(SpanIndexType span,GirderIndexType gdr,const std::vector<pgsTypes::Stage>& stages,PoiAttributeType attrib,std::vector<pgsPointOfInterest>* pPois) const;
   bool AndFind(const pgsPointOfInterest& poi,SpanIndexType span,GirderIndexType gdr,pgsTypes::Stage stage,PoiAttributeType attrib) const;
   void OrFind(SpanIndexType span,GirderIndexType gdr,pgsTypes::Stage stage,PoiAttributeType attrib,std::vector<pgsPointOfInterest>* pPois) const;
   void OrFind(SpanIndexType span,GirderIndexType gdr,const std::vector<pgsTypes::Stage>& stages,PoiAttributeType attrib,std::vector<pgsPointOfInterest>* pPois) const;
   bool OrFind(const pgsPointOfInterest& poi,SpanIndexType span,GirderIndexType gdr,pgsTypes::Stage stage,PoiAttributeType attrib) const;

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

#endif // INCLUDED_POIMGR_H_
