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

#pragma once
#include <PgsExt\PgsExtExp.h>
#include <PgsExt\SegmentKey.h>
#include <Reporter\Reporter.h>

class pgsPoiMgr;

// POI Attributes
typedef Uint64 PoiAttributeType;

/////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE: There are several bits available for future expansion... see below
/////////////////////////////////////////////////////////////////////////////////////////////////

// Attributes with POI_xxxx_SEGMENT or POI_GIRDER qualify attributes such as 10th points, H, 15H, FACEOFSUPPORT
// If the POI_xxxx_SEGMENT attribute is set, the meaning of, for example POI_MIDSPAN, is mid-span of the segment.
// If the POI_GIRDER attribute is set, it would mean mid-span of the assembled girder

// NOTE: PGSuper Version 3.0
// POI_TABULAR, POI_GRAPHICAL, and POI_ALLOUTPUT have been removed... we always report and graph all POIs
// POI_FLEXURESTRESS, POI_FLEXURECAPACITY, POI_SHEAR, POI_DISPLACEMENT, and POI_ALLACTIONS have been removed... we always analyze all effects at all POI

// Start at bit 64 and work backwards

// Reference
#define POI_RELEASED_SEGMENT       0x8000000000000000 // segment at time of release
#define POI_LIFT_SEGMENT           0x4000000000000000 // segment is being lifted
#define POI_STORAGE_SEGMENT        0x2000000000000000 // segment during storage (not necessiarly stored at ends)
#define POI_HAUL_SEGMENT           0x1000000000000000 // segment is being hauled
#define POI_ERECTED_SEGMENT        0x0800000000000000 // erected segment before assembly into a girder
#define POI_GIRDER                 0x0400000000000000 // fully assembled girder
// Special Points
#define POI_CRITSECTSHEAR1         0x0200000000000000 // critical section for shear, for strength I limit state
#define POI_CRITSECTSHEAR2         0x0100000000000000 // critical section for shear, for strength II limit state
#define POI_HARPINGPOINT           0x0080000000000000 // harping point
#define POI_CONCLOAD               0x0040000000000000 // point of application of a concentrated load
#define POI_MIDSPAN                0x0020000000000000 // POI is at the middle of the span
#define POI_H                      0x0010000000000000 // POI at h from end of girder
#define POI_15H                    0x0008000000000000 // POI at 1.5h from end of girder
#define POI_PSXFER                 0x0004000000000000 // POI at end of prestress transfer length
#define POI_PSDEV                  0x0002000000000000 // POI at end of prestress development length
#define POI_DEBOND                 0x0001000000000000 // POI at debond location
#define POI_DECKBARCUTOFF          0x0000800000000000 // POI at negative moment reinforcement cutoff point
#define POI_BARCUTOFF              0x0000400000000000 // POI at negative moment reinforcement cutoff point
#define POI_BARDEVELOP             0x0000200000000000 // POI at girder moment reinforcement development point
#define POI_PICKPOINT              0x0000100000000000 // POI at lifting pick point
#define POI_BUNKPOINT              0x0000080000000000 // POI at hauling bunk point
#define POI_FACEOFSUPPORT          0x0000040000000000 // POI at face of support
#define POI_CLOSURE                0x0000020000000000 // POI at center of closure pour
// Section Changes
#define POI_SECTCHANGE_TRANSITION  0x0000010000000000
#define POI_SECTCHANGE_RIGHTFACE   0x0000008000000000
#define POI_SECTCHANGE_LEFTFACE    0x0000004000000000
#define POI_SECTCHANGE POI_SECTCHANGE_LEFTFACE | POI_SECTCHANGE_RIGHTFACE | POI_SECTCHANGE_TRANSITION
// intermediate pier and temporary supports
#define POI_TEMPSUPPORT            0x0000002000000000
#define POI_INTERMEDIATE_PIER      0x0000001000000000 // POI at a pier that occurs between the ends of a segment
#define POI_PIER                   0x0000000800000000 // POI at a pier that occurs between groups
#define POI_STIRRUP_ZONE           0x0000000400000000 // Stirrup Zone Boundary

// The following POI attributes are undefined/unused.
// If a new attribute is needed, take it from this list
// starting with the highest value.
// **** NOTE ****
// If a new POI is defined, don't forget to update pgsPoiMgr::AndFind and pgsPoiMgr::OrFind
//#define POI_UNDEFINED23            0x0000000200000000
//#define POI_UNDEFINED22            0x0000000100000000
//#define POI_UNDEFINED21            0x0000000080000000
//#define POI_UNDEFINED20            0x0000000040000000
//#define POI_UNDEFINED19            0x0000000020000000
//#define POI_UNDEFINED18            0x0000000010000000
//#define POI_UNDEFINED17            0x0000000008000000
//#define POI_UNDEFINED16            0x0000000004000000
//#define POI_UNDEFINED15            0x0000000002000000
//#define POI_UNDEFINED14            0x0000000001000000
//#define POI_UNDEFINED13            0x0000000000800000
//#define POI_UNDEFINED12            0x0000000000400000
//#define POI_UNDEFINED11            0x0000000000200000
//#define POI_UNDEFINED10            0x0000000000100000
//#define POI_UNDEFINED9             0x0000000000080000
//#define POI_UNDEFINED8             0x0000000000040000
//#define POI_UNDEFINED7             0x0000000000020000
//#define POI_UNDEFINED6             0x0000000000010000
//#define POI_UNDEFINED5             0x0000000000008000
//#define POI_UNDEFINED4             0x0000000000004000
//#define POI_UNDEFINED3             0x0000000000002000
//#define POI_UNDEFINED2             0x0000000000001000
//#define POI_UNDEFINED1             0x0000000000000800

// The lower 12 bits are reserved for 10th point attributes
#define POI_10L                    0x0000000000000400 //1.0L
#define POI_9L                     0x0000000000000200 //0.9L
#define POI_8L                     0x0000000000000100 //0.8L
#define POI_7L                     0x0000000000000080 //0.7L
#define POI_6L                     0x0000000000000040 //0.6L
#define POI_5L                     0x0000000000000020 //0.5L
#define POI_4L                     0x0000000000000010 //0.4L
#define POI_3L                     0x0000000000000008 //0.3L
#define POI_2L                     0x0000000000000004 //0.2L
#define POI_1L                     0x0000000000000002 //0.1L
#define POI_0L                     0x0000000000000001 //0.0L

#define POI_ALLSPECIAL        POI_CRITSECTSHEAR1 | POI_CRITSECTSHEAR2 | POI_HARPINGPOINT | POI_CONCLOAD | \
                              POI_MIDSPAN | POI_H | POI_15H | POI_PSXFER | POI_PSDEV | POI_DEBOND |  \
                              POI_BARCUTOFF | POI_FACEOFSUPPORT | POI_SECTCHANGE | POI_INTERMEDIATE_PIER | POI_PIER | POI_TEMPSUPPORT
                             // note PICKPOINT and BUNKPOINT skipped on purpose


#define POI_TENTH_POINTS POI_0L | POI_1L | POI_2L | POI_3L | POI_4L | POI_5L | POI_6L | POI_7L | POI_8L | POI_9L | POI_10L


/*****************************************************************************
CLASS 
   pgsPointOfInterest

   Represents a point in the structure that is of interest to either the
   internals of the program or the end user.


DESCRIPTION
   Represents a point in the structure that is of interest to either the
   internals of the program or the end user.

   A point of interest is described by its attributes and its location.  The
   attributes describe what the POI is used for and its location describe
   where it is located in the bridge.

   The location of a point of interest is defined by its segment key
   and distance along the segment. The segment key defines a unique
   segment in a girder which belongs to a group. The distance along
   the segment is measured from the left end face of the segment.

   |CL Pier
   |
   |      distance
   |   |<---------->|
   |   |            | POI
   |   +------------*----------/
   |   |                       \
   |   +-----------------------/


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 10.20.1998 : Created file
*****************************************************************************/

class PGSEXTCLASS pgsPointOfInterest
{
public:
   pgsPointOfInterest();
   pgsPointOfInterest(const CSegmentKey& segmentKey,Float64 distFromStartOfSegment,PoiAttributeType attrib=0);
   pgsPointOfInterest(const CSegmentKey& segmentKey,Float64 distFromStartOfSegment,Float64 Xs,Float64 Xg,Float64 Xgp,PoiAttributeType attrib=0);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsPointOfInterest(const pgsPointOfInterest& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsPointOfInterest();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsPointOfInterest& operator = (const pgsPointOfInterest& rOther);

   //------------------------------------------------------------------------
   // Returns true if this poi is less than rOther, based on location
   bool operator<(const pgsPointOfInterest& rOther) const;

   //------------------------------------------------------------------------
   // Returns true if this poi is less than rOther, based on location
   bool operator==(const pgsPointOfInterest& rOther) const;


   //------------------------------------------------------------------------
   // A convient way to set the location of the POI.
   void SetLocation(const CSegmentKey& segmentKey,Float64 distFromStart,Float64 Xs,Float64 Xg,Float64 Xgp);

   // Offsets the POI by X
   void Offset(Float64 X);

   //------------------------------------------------------------------------
   // Sets the identifier for this POI. This identifier must be unique 
   // and should not be modified after the POI is added to a POI manager.
   void SetID(PoiIDType id)
   {
      m_ID = id;
   }

   //------------------------------------------------------------------------
   // Returns the unique identifier for this POI.
   PoiIDType GetID() const
   {
      return m_ID;
   }

   void SetSegmentKey(const CSegmentKey& segmentKey)
   {
      m_SegmentKey = segmentKey;
   }

   const CSegmentKey& GetSegmentKey() const
   {
      return m_SegmentKey;
   }

   //------------------------------------------------------------------------
   // Sets the location of this poi, measured from the start of the segment.
   void SetDistFromStart(Float64 distFromStart);

   //------------------------------------------------------------------------
   // Returns the location of this poi, measured from the start of the segment.
   Float64 GetDistFromStart() const
   {
      return m_DistFromStart;
   }

   // Set/Get the location of this poi in Segment Coordinates
   void SetSegmentCoordinate(Float64 Xs);
   Float64 GetSegmentCoordinate() const;

   // returns true if the segment coordinate has been set
   bool HasSegmentCoordinate() const;

   // Set/Get the location of this poi in Girder Coordinates
   void SetGirderCoordinate(Float64 Xg);
   Float64 GetGirderCoordinate() const;

   // returns true if the girder coordinate has been set
   bool HasGirderCoordinate() const;

   // Set/Get the location of this poi in Girder Path Coordinates
   void SetGirderPathCoordinate(Float64 Xgp);
   Float64 GetGirderPathCoordinate() const;

   // returns true if the girder path coordinate has been set
   bool HasGirderPathCoordinate() const;

   //------------------------------------------------------------------------
   // Sets the POI attributes
   void SetAttributes(PoiAttributeType attrib);

   //------------------------------------------------------------------------
   // Returns the attributes of this POI.
   PoiAttributeType GetAttributes() const;

   //------------------------------------------------------------------------
   // Removes attributes from this POI.
   void RemoveAttributes(PoiAttributeType attrib);

   PoiAttributeType GetReference() const;
   static PoiAttributeType GetReference(PoiAttributeType attrib);

   // prevents POI merging
   bool CanMerge() const
   {
      return m_bCanMerge;
   }

   void CanMerge(bool bCanMerge)
   {
      m_bCanMerge = bCanMerge;
   }

   //------------------------------------------------------------------------
   // Merge attributes for this POI with another's
   void MergeAttributes(const pgsPointOfInterest& rOther);

   // This POI is a tenth point on this segment
   void MakeTenthPoint(PoiAttributeType poiReference,Uint16 tenthPoint);

   //------------------------------------------------------------------------
   // Tolerance for comparing distance from start.
   static void SetTolerance(Float64 tol);
   static Float64 GetTolerance();

   //------------------------------------------------------------------------
   // Returns true if this poi is at a harping point
   bool IsHarpingPoint() const;

   //------------------------------------------------------------------------
   // Returns true if this poi is at the point of application of a concentrated
   // load.
   bool IsConcentratedLoad() const;

   //------------------------------------------------------------------------
   bool IsMidSpan(PoiAttributeType reference) const;

   //------------------------------------------------------------------------
   // Returns true if this poi is h from the end of the girder or face of support.
   bool IsAtH(PoiAttributeType reference) const;

   //------------------------------------------------------------------------
   // Returns true if this poi is 1.5h from the end of the girder or face of support.
   bool IsAt15H(PoiAttributeType reference) const;

   //------------------------------------------------------------------------
   // Returns 1-11 if this point is a tenth point, zero if not.
   // 1 is start , 11 is end
   Uint16 IsTenthPoint(PoiAttributeType reference) const;

   bool HasAttribute(PoiAttributeType attribute) const;

   bool AtSamePlace(const pgsPointOfInterest& other) const;

protected:
   //------------------------------------------------------------------------
   void MakeCopy(const pgsPointOfInterest& rOther);

   //------------------------------------------------------------------------
   virtual void MakeAssignment(const pgsPointOfInterest& rOther);

   bool IsValidReference(PoiAttributeType reference) const;


   //------------------------------------------------------------------------
   // utility functions for inserting and extracting tenth point information
   // into attributies. 
   static void SetAttributeTenthPoint(Uint16 tenthPoint, PoiAttributeType* attribute);
   static Uint16 GetAttributeTenthPoint(PoiAttributeType attribute);

protected:
   PoiIDType m_ID;
   CSegmentKey m_SegmentKey;
   Float64 m_DistFromStart; // distance from left end of segment (left face of the actual bridge member)
   
   bool m_bHasSegmentCoordinate; // tracks if m_Xs has been set
   Float64 m_Xs; // location of this POI in segment coordiantes (X=0 is at the CL Pier/CL TS at the start of this segment)

   bool m_bHasGirderCoordinate; // tracks if m_Xg has been set
   Float64 m_Xg; // location of this POI in girder coordinates (X=0 is at the left face of the first segment in the girder)

   bool m_bHasGirderPathCoordinate; // tracks if m_Xgp has been set
   Float64 m_Xgp; // location of this POI in girder path coordinates (X=0 is the CL Pier TS at the start of the first segment in the girder)

   bool m_bCanMerge;

   PoiAttributeType m_Attributes;

   static Float64 ms_Tol;

   friend pgsPoiMgr; // This guy sets the POI id.

public:
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns true if the object is in a valid state, otherwise returns false.
   virtual bool AssertValid() const;

   std::_tstring m_strAttributes;
   void UpdateAttributeString();
#endif // _DEBUG
};
