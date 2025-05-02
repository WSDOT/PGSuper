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

#pragma once
#include "psgLibLib.h"
#include <PsgLib\Keys.h>
#include <Reporter\Reporter.h>
#include <array>

class pgsPoiMgr;

// POI Attributes
typedef Uint64 PoiAttributeType;

/////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE: There are several bits available for future expansion... see below
//       See "Supporting Documents/PointOfInterest_64bit_layout.vsd" for bit layout
/////////////////////////////////////////////////////////////////////////////////////////////////

//
// Referenced Attributes (stored in the lower 32 bits)
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

#define POI_PICKPOINT              0x00002000 // POI at lifting pick point
#define POI_BUNKPOINT              0x00001000 // POI at hauling bunk point

#define POI_CANTILEVER             0x00000800 // POI is on the cantilevered portion of a span


// The lower 12 bits are reserved for 10th point attributes
// L, for 10th Points, is measured between points of support
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
// Non-referenced Attributes (stored in the upper 32 bits)
//

// unused non-referenced poi
// **** NOTE ****
// If a new POI is defined, don't forget to update pgsPoiMgr::AndFind and pgsPoiMgr::OrFind
//#define POI_NONREF9                0x8000000000000000
//#define POI_NONREF8                0x4000000000000000
//#define POI_NONREF7                0x2000000000000000

// Duct boundary POI
#define POI_DUCT_START               0x1000000000000000 // Start of a duct/tendon
#define POI_DUCT_END                 0x0800000000000000 // End of a duct/tendon
#define POI_DUCT_BOUNDARY            POI_DUCT_START | POI_DUCT_END

// Casting region boundary POI
#define POI_CASTING_BOUNDARY_START   0x0400000000000000 // Start of a deck casting region
#define POI_CASTING_BOUNDARY_END     0x0200000000000000 // End of a deck casting region
#define POI_CASTING_BOUNDARY POI_CASTING_BOUNDARY_START | POI_CASTING_BOUNDARY_END

// Special points of interest
#define POI_START_FACE               0x0100000000000000 // Start face of a segment (left end)
#define POI_END_FACE                 0x0080000000000000 // End face of a segment (right end)
#define POI_STIRRUP_ZONE             0x0040000000000000 // Stirrup Zone Boundary
#define POI_CRITSECTSHEAR1           0x0020000000000000 // critical section for shear, for strength I limit state
#define POI_CRITSECTSHEAR2           0x0010000000000000 // critical section for shear, for strength II limit state
#define POI_HARPINGPOINT             0x0008000000000000 // harping point
#define POI_CONCLOAD                 0x0004000000000000 // point of application of a concentrated load
#define POI_DIAPHRAGM                0x0002000000000000 // point of application of a diaphragm load
#define POI_PSXFER                   0x0001000000000000 // POI at end of prestress transfer length
#define POI_PSDEV                    0x0000800000000000 // POI at end of prestress development length
#define POI_DEBOND                   0x0000400000000000 // POI at debond location
#define POI_DECKBARCUTOFF            0x0000200000000000 // POI at negative moment reinforcement cutoff point
#define POI_BARCUTOFF                0x0000100000000000 // POI at negative moment reinforcement cutoff point
#define POI_BARDEVELOP               0x0000080000000000 // POI at girder moment reinforcement development point
#define POI_H                        0x0000040000000000 // POI at h from face of support
#define POI_15H                      0x0000020000000000 // POI at 1.5h from face of support
#define POI_FACEOFSUPPORT            0x0000010000000000 // POI at face of support
#define POI_CLOSURE                  0x0000008000000000 // POI at center of closure joint

#define POI_SPECIAL POI_START_FACE | POI_END_FACE | POI_STIRRUP_ZONE | POI_CRITSECTSHEAR1 | POI_CRITSECTSHEAR2 | POI_HARPINGPOINT | POI_CONCLOAD | \
                    POI_DIAPHRAGM | POI_PSXFER | POI_PSDEV | POI_DEBOND | POI_DECKBARCUTOFF | POI_BARCUTOFF | POI_BARDEVELOP | \
                    POI_H | POI_15H | POI_FACEOFSUPPORT | POI_CLOSURE

// Section Changes
#define POI_SECTCHANGE_TRANSITION    0x0000004000000000
#define POI_SECTCHANGE_RIGHTFACE     0x0000002000000000
#define POI_SECTCHANGE_LEFTFACE      0x0000001000000000

#define POI_SECTCHANGE POI_SECTCHANGE_LEFTFACE | POI_SECTCHANGE_RIGHTFACE | POI_SECTCHANGE_TRANSITION

// intermediate pier and temporary supports
#define POI_INTERMEDIATE_TEMPSUPPORT 0x0000000800000000 // POI at a temporary support that occurs between the ends of a segment
#define POI_INTERMEDIATE_PIER        0x0000000400000000 // POI at a pier that occurs between the ends of a segment
#define POI_BOUNDARY_PIER            0x0000000200000000 // POI at a pier that occurs between groups
#define POI_ABUTMENT                 0x0000000100000000 // POI at CL Bearing at start/end abutment

#define POI_SUPPORTS POI_INTERMEDIATE_TEMPSUPPORT | POI_INTERMEDIATE_PIER | POI_BOUNDARY_PIER | POI_ABUTMENT


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

LOG
   rab : 10.20.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS pgsPointOfInterest
{
public:
   pgsPointOfInterest();
   pgsPointOfInterest(const CSegmentKey& segmentKey,Float64 Xpoi,PoiAttributeType attrib=0);
   pgsPointOfInterest(const CSegmentKey& segmentKey,Float64 Xpoi,Float64 Xs,Float64 Xg,Float64 Xgp,PoiAttributeType attrib=0);
   pgsPointOfInterest(const pgsPointOfInterest& rOther);

   virtual ~pgsPointOfInterest();

   pgsPointOfInterest& operator = (const pgsPointOfInterest& rOther);

   // Operators defined relative positions between points of interest
   bool operator<(const pgsPointOfInterest& rOther) const;
   bool operator<=(const pgsPointOfInterest& rOther) const;
   bool operator==(const pgsPointOfInterest& rOther) const;
   bool operator!=(const pgsPointOfInterest& rOther) const;

   // A convenient way to set the location of the POI.
   void SetLocation(const CSegmentKey& segmentKey,Float64 Xpoi,Float64 Xs,Float64 Xg,Float64 Xgp);

   // A convenient way to set the location of the POI. 
   void SetLocation(const CSegmentKey& segmentKey,Float64 Xpoi);

   // Offsets the POI by X
   void Offset(Float64 X);

   // Sets the identifier for this POI. This identifier must be unique 
   // and should not be modified after the POI is added to a POI manager.
   void SetID(PoiIDType id)
   {
      m_ID = id;
   }

   // Returns the unique identifier for this POI.
   PoiIDType GetID() const
   {
      return m_ID;
   }

   // Sets the segment key
   void SetSegmentKey(const CSegmentKey& segmentKey)
   {
      m_SegmentKey = segmentKey;
   }

   // Returns the segment key
   const CSegmentKey& GetSegmentKey() const
   {
      return m_SegmentKey;
   }

   // Sets the location of this poi, measured from the start of the segment.
   void SetDistFromStart(Float64 distFromStart,bool bRetainAttributes = false);

   // Returns the location of this poi, measured from the start of the segment.
   Float64 GetDistFromStart() const
   {
      return m_Xpoi;
   }

   // Returns true if at the same place as other.
   // Attributes such as left and right faces are not considered
   bool AtSamePlace(const pgsPointOfInterest& other) const;

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

   // Set/Get the location of this poi in Girderline Coordinates
   void SetGirderlineCoordinate(Float64 Xgp);
   Float64 GetGirderlineCoordinate() const;

   // returns true if the girder line coordinate has been set
   bool HasGirderlineCoordinate() const;

   // Set/Get the span location of this poi
   void SetSpanPoint(SpanIndexType spanIdx,Float64 Xspan);
   void GetSpanPoint(SpanIndexType* pSpanIdx,Float64* pXspan) const;

   // returns true if the span key has been set
   bool HasSpanPoint() const;

   // Set/Get the deck casting region of this poi
   void SetDeckCastingRegion(IndexType deckCastingRegionIdx);
   IndexType GetDeckCastingRegion() const;

   // returns true if the deck casting region has been set
   bool HasDeckCastingRegion() const;

   // Removes all attributes from this POI
   void ClearAttributes();

   // Sets the non-referenced POI attributes
   void SetNonReferencedAttributes(PoiAttributeType attrib);
        
   // Sets the referenced POI attributes
   void SetReferencedAttributes(PoiAttributeType attrib);

   // Returns the non-referenced attributes of this POI.
   PoiAttributeType GetNonReferencedAttributes() const;

   // Returns the referenced attributes of this POI.
   PoiAttributeType GetReferencedAttributes(PoiAttributeType refAttribute) const;

   // Removes attributes from this POI.
   void RemoveAttributes(PoiAttributeType attrib);

   // returns all of the reference attributes encoded in attrib
   static PoiAttributeType GetReference(PoiAttributeType attrib);

   // returns true if attrib is one of the reference attribute types
   static bool IsReferenceAttribute(PoiAttributeType attrib);

   // Merge attributes for this POI with another's. Returns true if successful
   // If unsuccessful, the attributes of this POI are not changed
   bool MergeAttributes(const pgsPointOfInterest& rOther);

   // This POI is a tenth point on this segment
   void MakeTenthPoint(PoiAttributeType poiReference,Uint16 tenthPoint);

   // Returns true if this poi is at a harping point
   bool IsHarpingPoint() const;

   // Returns true if this poi is at the point of application of a concentrated load.
   bool IsConcentratedLoad() const;

   // Returns true if this poi is the mid span point for the reference type
   bool IsMidSpan(PoiAttributeType reference) const;

   // Returns true if this poi is h from the face of support.
   bool IsAtH() const;

   // Returns true if this poi is 1.5h from the face of support.
   bool IsAt15H() const;

   // Returns 1-11 if this point is a tenth point, zero if not.
   // 1 is start , 11 is end
   Uint16 IsTenthPoint(PoiAttributeType reference) const;

   // Returns true if this poi has a particular attribute
   bool HasAttribute(PoiAttributeType attribute) const;

   // Returns true if this poi has attributes
   bool HasAttributes() const;

   // Returns a string of attribute codes.
   std::_tstring GetAttributes(PoiAttributeType reference,bool bIncludeMarkup) const;

protected:
   void MakeCopy(const pgsPointOfInterest& rOther);
   void MakeAssignment(const pgsPointOfInterest& rOther);

   // utility functions for inserting and extracting tenth point information
   // into attributies. 
   static void SetAttributeTenthPoint(Uint16 tenthPoint, PoiAttributeType* attribute);
   static Uint16 GetAttributeTenthPoint(PoiAttributeType attribute);

protected:

   PoiIDType m_ID;
   CSegmentKey m_SegmentKey;
   Float64 m_Xpoi; // distance from left end of segment (left face of the actual bridge member), AKA Segment Coordinate
   
#if defined _DEBUG || defined _BETA_VERSION
   // NOTE: this is a weird place to put this, but it makes it easy to see the attributes string in the debugger
   std::_tstring m_strAttributes;
   void UpdateAttributeString();
#endif // _DEBUG

   bool m_bHasSegmentPathCoordinate; // tracks if m_Xsp has been set
   Float64 m_Xsp; // location of this POI in segment path coordinates (X=0 is at the CL Pier/CL TS at the start of this segment)

   bool m_bHasGirderCoordinate; // tracks if m_Xg has been set
   Float64 m_Xg; // location of this POI in girder coordinates (X=0 is at the left face of the first segment in the girder)

   bool m_bHasGirderPathCoordinate; // tracks if m_Xgp has been set
   Float64 m_Xgp; // location of this POI in girder path coordinates (X=0 is the CL Pier TS at the start of the first segment in the girder)

   bool m_bHasGirderlineCoordinate; // tracks if m_Xgl has been set
   Float64 m_Xgl; // location of this POI in girder line coordinates

   bool m_bHasSpanPoint; // tracks if m_SpanIdx and m_Xspan has been set
   SpanIndexType m_SpanIdx; // span index in Span coordinate system
   Float64 m_Xspan; // location in Span coordinate system

   bool m_bHasDeckCastingRegion; // tracks if the deck casting region has been set
   IndexType m_DeckCastingRegionIdx;  // deck casting region this POI is located in

   // returns the index of the reference attribute
   static IndexType GetIndex(PoiAttributeType refAttribute);

   std::array<PoiAttributeType,6> m_RefAttributes; // referenced attributes (10th points)
   PoiAttributeType m_Attributes; // non-referenced attributes

   friend pgsPoiMgr; // This guy sets the POI id.

public:
#if defined _DEBUG || defined _BETA_VERSION
   // Returns true if the object is in a valid state, otherwise returns false.
   virtual bool AssertValid() const;
#endif
};

// We use POIs more than anything else and they are really just keys. We don't
// want to construct, copy, destroy them all the time. This typedef defines a
// container that holds references to POIs. This reduces overhead. The caveat
// is that the lifetime of the POI must be greater than the lifetime of the container.
// You are required to ensure proper lifetime semantics.
//
// This list is easily used in iteration as follows
// for ( const pgsPointOfInterest& poi : myPoiList)
// {...}
// using pgsPointOfInterest instead of auto causes the reference_wrapper's type conversion
// operator to be invoked and thus we can treat the elements of the container as pois
typedef std::vector<std::reference_wrapper<const pgsPointOfInterest>> PoiList;

// Coverts a vector of POIs into a PoiList. You are required to ensure the POIs in the
// vector outlive the PoiList object.
void PSGLIBFUNC MakePoiList(const std::vector<pgsPointOfInterest>& vPoi,PoiList* pPoiList);

// Makes a vector of POIs that are copied from the POI list.
void PSGLIBFUNC MakePoiVector(const PoiList& vPoiList, std::vector<pgsPointOfInterest>* pvPoi);
