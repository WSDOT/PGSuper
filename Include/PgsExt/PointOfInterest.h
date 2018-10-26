///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
#include <Reporter\Reporter.h>

class pgsPoiMgr;

// POI Attributes
typedef Uint64 PoiAttributeType;

// Start at bit 64 and work backwards

// Visibility
#define POI_TABULAR           0x8000000000000000 // POI used in tabular reports
#define POI_GRAPHICAL         0x4000000000000000 // POI used to create graphs

// cross section transition point
#define POI_SECTCHANGE        0x2000000000000000

// Special Points
#define POI_CRITSECTSHEAR1    0x1000000000000000 // critical section for shear, for strength I limit state
#define POI_CRITSECTSHEAR2    0x0800000000000000 // critical section for shear, for strength II limit state
#define POI_HARPINGPOINT      0x0400000000000000 // harping point
#define POI_CONCLOAD          0x0200000000000000 // point of application of a concentrated load
#define POI_MIDSPAN           0x0100000000000000 // POI is at the middle of the span
#define POI_H                 0x0080000000000000 // POI at h from end of girder
#define POI_15H               0x0040000000000000 // POI at 1.5h from end of girder
#define POI_PSXFER            0x0020000000000000 // POI at end of prestress transfer length
#define POI_PSDEV             0x0010000000000000 // POI at end of prestress development length
#define POI_DEBOND            0x0008000000000000 // POI at debond location
#define POI_BARCUTOFF         0x0004000000000000 // POI at negative moment reinforcement cutoff point
#define POI_PICKPOINT         0x0002000000000000 // POI at lifting pick point
#define POI_BUNKPOINT         0x0001000000000000 // POI at hauling bunk point
#define POI_FACEOFSUPPORT     0x0000800000000000 // POI at face of support

// Structural Action
#define POI_FLEXURESTRESS     0x0000400000000000
#define POI_FLEXURECAPACITY   0x0000200000000000
#define POI_SHEAR             0x0000100000000000
#define POI_DISPLACEMENT      0x0000080000000000

#define POI_ALLACTIONS        POI_FLEXURESTRESS | POI_FLEXURECAPACITY | POI_SHEAR | POI_DISPLACEMENT

#define POI_ALLOUTPUT         POI_TABULAR | POI_GRAPHICAL

#define POI_ALL               POI_ALLOUTPUT | POI_ALLACTIONS | POI_ALLSPECIAL

#define POI_ALLSPECIAL        POI_CRITSECTSHEAR1 | POI_CRITSECTSHEAR2 | POI_HARPINGPOINT | POI_CONCLOAD | \
                              POI_MIDSPAN | POI_H | POI_15H | POI_PSXFER | POI_PSDEV | POI_DEBOND |  \
                              POI_BARCUTOFF | POI_FACEOFSUPPORT | POI_SECTCHANGE 
                             // note PICKPOINT and BUNKPOINT skipped on purpose

// The lower 16 bits are reserved for 10th point attributes
// 0x0000 0000 0000 0001 0.0L
// 0x0000 0000 0000 0002 0.1L
// 0x0000 0000 0000 0004 0.2L
// 0x0000 0000 0000 0008 0.3L
// 0x0000 0000 0000 0010 0.4L
// 0x0000 0000 0000 0020 0.5L
// 0x0000 0000 0000 0040 0.6L
// 0x0000 0000 0000 0080 0.7L
// 0x0000 0000 0000 0100 0.8L
// 0x0000 0000 0000 0200 0.9L
// 0x0000 0000 0000 0400 1.0L
// 0x0000 0000 0000 0800 - Unused
// 0x0000 0000 0000 1000 - Unused
// 0x0000 0000 0000 2000 - Unused
// 0x0000 0000 0000 4000 - Unused
// 0x0000 0000 0000 8000 - Unused
// 0x0000 0000 0001 0000 - If set, L = Lspan, if clear, L = Lgirder

#define POI_GIRDER_POINT 0x0000000000020000
#define POI_SPAN_POINT   0x0000000000010000

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
   //------------------------------------------------------------------------
   pgsPointOfInterest();

   //------------------------------------------------------------------------
   pgsPointOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 distFromStart,PoiAttributeType attrib = POI_ALLACTIONS | POI_ALLOUTPUT);
   pgsPointOfInterest(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,Float64 distFromStart,PoiAttributeType attrib = POI_ALLACTIONS | POI_ALLOUTPUT);
   pgsPointOfInterest(std::set<pgsTypes::Stage> stages,SpanIndexType span,GirderIndexType gdr,Float64 distFromStart,PoiAttributeType attrib = POI_ALLACTIONS | POI_ALLOUTPUT);

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
   // A convient way to set the location of the POI. Calls SetSpan(), SetGirder(),
   // and SetDistFromStart().
   void SetLocation(SpanIndexType span,GirderIndexType gdr,Float64 distFromStart);

   //------------------------------------------------------------------------
   // Returns the identifier assigned in the constructor.
   PoiIDType GetID() const;

   //------------------------------------------------------------------------
   // Sets the span number of this poi.
   void SetSpan(SpanIndexType span);

   //------------------------------------------------------------------------
   // Returns the span number for this poi.
   SpanIndexType GetSpan() const;

   //------------------------------------------------------------------------
   // Sets the girder index for this poi.
   void SetGirder(GirderIndexType gdr);

   //------------------------------------------------------------------------
   // Returns the girder index for this poi.
   GirderIndexType GetGirder() const;

   //------------------------------------------------------------------------
   // Sets the location of this poi, measured from the start of the girder.
   void SetDistFromStart(Float64 distFromStart);

   //------------------------------------------------------------------------
   // Returns the location of this poi, measured from the start of the girder.
   Float64 GetDistFromStart() const;

   //------------------------------------------------------------------------
   // Sets the attributes of this POI
   void SetAttributes(PoiAttributeType attrib);

   //------------------------------------------------------------------------
   // Returns the attributes of this POI.
   PoiAttributeType GetAttributes() const;

   void MakeTenthPoint(PoiAttributeType basisType,Uint16 tenthPoint);

   //------------------------------------------------------------------------
   // Tolerance for comparing distance from start.
   static void SetTolerance(Float64 tol);
   static Float64 GetTolerance();

   //------------------------------------------------------------------------
   // Returns true if this poi is used for flexure.
   bool IsFlexureCapacity() const;
   bool IsFlexureStress() const;

   //------------------------------------------------------------------------
   // Returns true if this poi is used for shear.
   bool IsShear() const;

   //------------------------------------------------------------------------
   // Returns true if this poi is used for displacement.
   bool IsDisplacement() const;

   //------------------------------------------------------------------------
   // Returns true if this poi is at a harping point
   bool IsHarpingPoint() const;

   //------------------------------------------------------------------------
   // Returns true if this poi is at the point of application of a concentrated
   // load.
   bool IsConcentratedLoad() const;

   //------------------------------------------------------------------------
   bool IsMidSpan() const;

   //------------------------------------------------------------------------
   // Returns true if this poi is used for tabular reports.
   bool IsTabular() const;

   //------------------------------------------------------------------------
   // Returns true if this poi is used for creating graphics.
   bool IsGraphical() const;

   //------------------------------------------------------------------------
   // Returns true if this poi is h from the end of the girder or face of support.
   bool IsAtH() const;

   //------------------------------------------------------------------------
   // Returns true if this poi is 1.5h from the end of the girder or face of support.
   bool IsAt15H() const;

   //------------------------------------------------------------------------
   // Returns 1-11 if this point is a tenth point, zero if not.
   // 1 is start , 11 is end
   Uint16 IsATenthPoint(PoiAttributeType basisType) const;

   bool HasAttribute(PoiAttributeType attribute) const;

   //------------------------------------------------------------------------
   // utility functions for inserting and extracting tenth point information
   // into attributies. 
   static void SetAttributeTenthPoint(Uint16 tenthPoint, PoiAttributeType* attribute);
   static Uint16 GetAttributeTenthPoint(PoiAttributeType attribute);

   void AddStage(pgsTypes::Stage stage);
   void AddStages(std::set<pgsTypes::Stage> stages);
   void RemoveStage(pgsTypes::Stage stage);
   bool HasStage(pgsTypes::Stage stage) const;
   std::set<pgsTypes::Stage> GetStages() const;
   Uint32 GetStageCount() const;

protected:
   //------------------------------------------------------------------------
   void MakeCopy(const pgsPointOfInterest& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsPointOfInterest& rOther);

private:
   PoiIDType m_ID;
   SpanIndexType m_Span;
   GirderIndexType m_Girder;
   Float64 m_DistFromStart;
   PoiAttributeType m_Attributes;

   static Float64 ms_Tol;

   std::set<pgsTypes::Stage> m_Stages;

   friend pgsPoiMgr; // This guy sets the POI id.

public:
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

/*****************************************************************************
CLASS 
   rptPointOfInterest

   Report content for points of interest.


DESCRIPTION
   Report content for points of interest. Reports the distance from start,
   adjust for an end offset, and annotates special POI.


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 02.04.1999 : Created file
*****************************************************************************/
class PGSEXTCLASS rptPointOfInterest : public rptLengthUnitValue
{
public:
   //------------------------------------------------------------------------
   rptPointOfInterest(const pgsPointOfInterest& poi,
                      Float64 endOffest,
                      const unitLength* pUnitOfMeasure,
                      Float64 zeroTolerance = 0.,
                      bool bShowUnitTag = true,bool bSpanPOI  = true);

   //------------------------------------------------------------------------
   rptPointOfInterest(const unitLength* pUnitOfMeasure = 0,
                      Float64 zeroTolerance = 0.,
                      bool bShowUnitTag = true,bool bSpanPOI  = true);

   //------------------------------------------------------------------------
   rptPointOfInterest(const rptPointOfInterest& rOther);

   //------------------------------------------------------------------------
   rptPointOfInterest& operator = (const rptPointOfInterest& rOther);

   //------------------------------------------------------------------------
   virtual rptReportContent* CreateClone() const;

   //------------------------------------------------------------------------
   virtual rptReportContent& SetValue(const pgsPointOfInterest& poi,Float64 endOffset = 0.0);

   //------------------------------------------------------------------------
   std::string AsString() const;

   //------------------------------------------------------------------------
   // Make this a girder poi, that is, distance is measured from the
   // left end of the girder
   void MakeGirderPoi(bool bGirderPOI=true);

   //------------------------------------------------------------------------
   // Make this a span poi, that is, distance is measured from the
   // left end of the span (usually from the left bearing)
   void MakeSpanPoi(bool bSpanPOI=true);

   //------------------------------------------------------------------------
   // Returns true if this is a span poi
   bool IsSpanPoi() const;

   //------------------------------------------------------------------------
   // Returns true if this is a girder poi
   bool IsGirderPoi() const;

   //------------------------------------------------------------------------
   // If set to true, the poi attribute prefixes the distance value
   // eg  (HP) 2.34ft, otherwise it post-fixes the distance value 2.34ft (HP)
   void PrefixAttributes(bool bPrefixAttributes=true);

   //------------------------------------------------------------------------
   // Returns true if the poi is to be prefixed
   bool PrefixAttributes() const;

protected:
   void MakeCopy(const rptPointOfInterest& rOther);
   void MakeAssignment(const rptPointOfInterest& rOther);

private:
   pgsPointOfInterest m_Poi;
   bool m_bSpanPOI;
   bool m_bPrefixAttributes;
};
