///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <PgsExt\Keys.h>
#include <Reporter\Reporter.h>

class pgsPoiMgr;

// POI Attributes
typedef Uint32 PoiAttributeType; // NOTE: if more bits are needed use a 64-bit type

/////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE: There are several bits available for future expansion... see below
/////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE: PGSuper Version 3.0
// POI_TABULAR, POI_GRAPHICAL, and POI_ALLOUTPUT have been removed... we always report and graph all POIs
// POI_FLEXURESTRESS, POI_FLEXURECAPACITY, POI_SHEAR, POI_DEFLECTION, and POI_ALLACTIONS have been removed... we always analyze all effects at all POI

//
// Referenced Attributes
//

// Attributes with POI_xxxx_SEGMENT or POI_SPAN qualify 10th point attributes.

// Reference
#define POI_RELEASED_SEGMENT       0x80000000 // segment at time of release
#define POI_LIFT_SEGMENT           0x40000000 // segment is being lifted
#define POI_STORAGE_SEGMENT        0x20000000 // segment during storage (not necessiarly stored at ends)
#define POI_HAUL_SEGMENT           0x10000000 // segment is being hauled
#define POI_ERECTED_SEGMENT        0x08000000 // erected segment before assembly into a girder
#define POI_SPAN                   0x04000000 // location in fully assembled girder

// unused referenced poi
// **** NOTE ****
// If a new POI is defined, don't forget to update pgsPoiMgr::AndFind and pgsPoiMgr::OrFind
//#define POI_REFERENCED13         0x02000000
//#define POI_REFERENCED12         0x01000000
//#define POI_REFERENCED11         0x00800000
//#define POI_REFERENCED10         0x00400000
//#define POI_REFERENCED9          0x00200000
//#define POI_REFERENCED8          0x00100000
//#define POI_REFERENCED7          0x00080000
//#define POI_REFERENCED6          0x00040000
//#define POI_REFERENCED5          0x00020000
//#define POI_REFERENCED4          0x00010000
//#define POI_REFERENCED3          0x00008000
//#define POI_REFERENCED2          0x00004000
//#define POI_REFERENCED1          0x00002000

#define POI_PICKPOINT              0x00001000 // POI at lifting pick point
#define POI_BUNKPOINT              0x00000800 // POI at hauling bunk point


// The lower 12 bits are reserved for 10th point attributes
#define POI_10L                    0x00000400 //1.0L
#define POI_9L                     0x00000200 //0.9L
#define POI_8L                     0x00000100 //0.8L
#define POI_7L                     0x00000080 //0.7L
#define POI_6L                     0x00000040 //0.6L
#define POI_5L                     0x00000020 //0.5L
#define POI_4L                     0x00000010 //0.4L
#define POI_3L                     0x00000008 //0.3L
#define POI_2L                     0x00000004 //0.2L
#define POI_1L                     0x00000002 //0.1L
#define POI_0L                     0x00000001 //0.0L

#define POI_TENTH_POINTS POI_0L | POI_1L | POI_2L | POI_3L | POI_4L | POI_5L | POI_6L | POI_7L | POI_8L | POI_9L | POI_10L

//
// Non-referenced Attributes
//

// unused non-referenced poi
// **** NOTE ****
// If a new POI is defined, don't forget to update pgsPoiMgr::AndFind and pgsPoiMgr::OrFind
//#define POI_NONREF10               0x80000000
//#define POI_NONREF9                0x40000000
//#define POI_NONREF8                0x20000000
//#define POI_NONREF7                0x10000000
//#define POI_NONREF6                0x08000000
//#define POI_NONREF5                0x04000000
//#define POI_NONREF4                0x02000000
//#define POI_NONREF3                0x01000000
//#define POI_NONREF2                0x00800000
//#define POI_NONREF1                0x00400000

// Special Points
#define POI_CRITSECTSHEAR1           0x00200000 // critical section for shear, for strength I limit state
#define POI_CRITSECTSHEAR2           0x00100000 // critical section for shear, for strength II limit state
#define POI_HARPINGPOINT             0x00080000 // harping point
#define POI_CONCLOAD                 0x00040000 // point of application of a concentrated load
#define POI_PSXFER                   0x00020000 // POI at end of prestress transfer length
#define POI_PSDEV                    0x00010000 // POI at end of prestress development length
#define POI_DEBOND                   0x00008000 // POI at debond location
#define POI_DECKBARCUTOFF            0x00004000 // POI at negative moment reinforcement cutoff point
#define POI_BARCUTOFF                0x00002000 // POI at negative moment reinforcement cutoff point
#define POI_BARDEVELOP               0x00001000 // POI at girder moment reinforcement development point
#define POI_H                        0x00000800 // POI at h from face of support
#define POI_15H                      0x00000400 // POI at 1.5h from face of support
#define POI_FACEOFSUPPORT            0x00000200 // POI at face of support
#define POI_CLOSURE                  0x00000100 // POI at center of closure joint
// Section Changes
#define POI_SECTCHANGE_TRANSITION    0x00000080
#define POI_SECTCHANGE_RIGHTFACE     0x00000040
#define POI_SECTCHANGE_LEFTFACE      0x00000020
#define POI_SECTCHANGE POI_SECTCHANGE_LEFTFACE | POI_SECTCHANGE_RIGHTFACE | POI_SECTCHANGE_TRANSITION
// intermediate pier and temporary supports
#define POI_INTERMEDIATE_TEMPSUPPORT 0x00000010 // POI at a temporary support that occurs between the ends of a segment
#define POI_INTERMEDIATE_PIER        0x00000008 // POI at a pier that occurs between the ends of a segment
#define POI_BOUNDARY_PIER            0x00000004 // POI at a pier that occurs between groups
#define POI_ABUTMENT                 0x00000002 // POI at CL Bearing at start/end abutment
#define POI_STIRRUP_ZONE             0x00000001 // Stirrup Zone Boundary


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
   pgsPointOfInterest(const CSegmentKey& segmentKey,Float64 Xpoi,PoiAttributeType attrib=0);
   pgsPointOfInterest(const CSegmentKey& segmentKey,Float64 Xpoi,Float64 Xs,Float64 Xg,Float64 Xgp,PoiAttributeType attrib=0);

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
   bool operator<=(const pgsPointOfInterest& rOther) const;

   //------------------------------------------------------------------------
   // Returns true if this poi is less than rOther, based on location
   bool operator==(const pgsPointOfInterest& rOther) const;
   bool operator!=(const pgsPointOfInterest& rOther) const;

   //------------------------------------------------------------------------
   // A convient way to set the location of the POI.
   void SetLocation(const CSegmentKey& segmentKey,Float64 Xpoi,Float64 Xs,Float64 Xg,Float64 Xgp);

   //------------------------------------------------------------------------
   // A convient way to set the location of the POI. 
   void SetLocation(const CSegmentKey& segmentKey,Float64 Xpoi);

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
      return m_Xpoi;
   }

   // Set/Get the location of this poi in Segment Path Coordinates
   void SetSegmentPathCoordinate(Float64 Xsp);
   Float64 GetSegmentPathCoordinate() const;

   // returns true if the segment path coordinate has been set
   bool HasSegmentPathCoordinate() const;

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
   // Sets the non-referenced POI attributes
   void SetNonReferencedAttributes(PoiAttributeType attrib);
        
   //------------------------------------------------------------------------
   // Sets the referenced POI attributes
   void SetReferencedAttributes(PoiAttributeType attrib);

   //------------------------------------------------------------------------
   // Returns the non-referenced attributes of this POI.
   PoiAttributeType GetNonReferencedAttributes() const;

   //------------------------------------------------------------------------
   // Returns the referenced attributes of this POI.
   PoiAttributeType GetReferencedAttributes(PoiAttributeType refAttribute) const;

   //------------------------------------------------------------------------
   // Removes attributes from this POI.
   void RemoveAttributes(PoiAttributeType attrib);

   // returns all of the reference attributes encoded in attrib
   static PoiAttributeType GetReference(PoiAttributeType attrib);

   // returns true if attrib is one of the reference attribute types
   static bool IsReferenceAttribute(PoiAttributeType attrib);

   // prevents POI merging
   bool CanMerge() const;
   void CanMerge(bool bCanMerge);

   //------------------------------------------------------------------------
   // Merge attributes for this POI with another's. Returns true if successful
   bool MergeAttributes(const pgsPointOfInterest& rOther);

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
   // Returns true if this poi is h from the face of support.
   bool IsAtH() const;

   //------------------------------------------------------------------------
   // Returns true if this poi is 1.5h from the face of support.
   bool IsAt15H() const;

   //------------------------------------------------------------------------
   // Returns 1-11 if this point is a tenth point, zero if not.
   // 1 is start , 11 is end
   Uint16 IsTenthPoint(PoiAttributeType reference) const;

   // Returns true if this poi has a particular attribute
   bool HasAttribute(PoiAttributeType attribute) const;

   // Returns true if this poi has attributes
   bool HasAttributes() const;

   bool AtSamePlace(const pgsPointOfInterest& other) const;

   // Returns a string of attribute codes.
   std::_tstring GetAttributes(PoiAttributeType reference,bool bIncludeMarkup) const;


protected:
   //------------------------------------------------------------------------
   void MakeCopy(const pgsPointOfInterest& rOther);

   //------------------------------------------------------------------------
   virtual void MakeAssignment(const pgsPointOfInterest& rOther);

   //------------------------------------------------------------------------
   // utility functions for inserting and extracting tenth point information
   // into attributies. 
   static void SetAttributeTenthPoint(Uint16 tenthPoint, PoiAttributeType* attribute);
   static Uint16 GetAttributeTenthPoint(PoiAttributeType attribute);

protected:
   PoiIDType m_ID;
   CSegmentKey m_SegmentKey;
   Float64 m_Xpoi; // distance from left end of segment (left face of the actual bridge member)
   // AKA Segment Coorodinate
   
   bool m_bHasSegmentPathCoordinate; // tracks if m_Xsp has been set
   Float64 m_Xsp; // location of this POI in segment path coordiantes (X=0 is at the CL Pier/CL TS at the start of this segment)

   bool m_bHasGirderCoordinate; // tracks if m_Xg has been set
   Float64 m_Xg; // location of this POI in girder coordinates (X=0 is at the left face of the first segment in the girder)

   bool m_bHasGirderPathCoordinate; // tracks if m_Xgp has been set
   Float64 m_Xgp; // location of this POI in girder path coordinates (X=0 is the CL Pier TS at the start of the first segment in the girder)

   bool m_bCanMerge;

   static IndexType GetIndex(PoiAttributeType refAttribute);
   PoiAttributeType m_RefAttributes[6]; // referenced attributes (10th points)
   PoiAttributeType m_Attributes; // non-referenced attributes

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
