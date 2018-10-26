///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include <Reporter\Reporter.h>

class pgsPoiMgr;

// POI Attributes
typedef Uint64 PoiAttributeType;

// Start at bit 64 and work backwards

// Visibility
#define POI_TABULAR           0x8000000000000000 // POI used in tabular reports
#define POI_GRAPHICAL         0x4000000000000000 // POI used to create graphs

// cross section transition point
//#define POI_SECTCHANGE        0x2000000000000000 (unused, see below for POI_SECTCHANGE)

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

// section changes
#define POI_SECTCHANGE_LEFTFACE    0x0000000000002000
#define POI_SECTCHANGE_RIGHTFACE   0x0000000000004000
#define POI_SECTCHANGE_TRANSITION  0x0000000000008000
#define POI_SECTCHANGE POI_SECTCHANGE_LEFTFACE | POI_SECTCHANGE_RIGHTFACE | POI_SECTCHANGE_TRANSITION

#define POI_ALLACTIONS        POI_FLEXURESTRESS | POI_FLEXURECAPACITY | POI_SHEAR | POI_DISPLACEMENT

#define POI_ALLOUTPUT         POI_TABULAR | POI_GRAPHICAL

#define POI_ALL               POI_ALLOUTPUT | POI_ALLACTIONS | POI_ALLSPECIAL

#define POI_ALLSPECIAL        POI_CRITSECTSHEAR1 | POI_CRITSECTSHEAR2 | POI_HARPINGPOINT | POI_CONCLOAD | \
                              POI_MIDSPAN | POI_H | POI_15H | POI_PSXFER | POI_PSDEV | POI_DEBOND |  \
                              POI_BARCUTOFF | POI_FACEOFSUPPORT | POI_SECTCHANGE 
                             // note PICKPOINT and BUNKPOINT skipped on purpose

// The lower 16 bits are reserved for 10th point attributes
#define POI_0L  0x0000000000000001 //0.0L
#define POI_1L  0x0000000000000002 //0.1L
#define POI_2L  0x0000000000000004 //0.2L
#define POI_3L  0x0000000000000008 //0.3L
#define POI_4L  0x0000000000000010 //0.4L
#define POI_5L  0x0000000000000020 //0.5L
#define POI_6L  0x0000000000000040 //0.6L
#define POI_7L  0x0000000000000080 //0.7L
#define POI_8L  0x0000000000000100 //0.8L
#define POI_9L  0x0000000000000200 //0.9L
#define POI_10L 0x0000000000000400 //1.0L

#define POI_TENTH_POINTS POI_0L | POI_1L | POI_2L | POI_3L | POI_4L | POI_5L | POI_6L | POI_7L | POI_8L | POI_9L | POI_10L

// 0x0000 0000 0000 0800 - Unused
// 0x0000 0000 0000 1000 - Unused

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
   pgsPointOfInterest();
   pgsPointOfInterest(SpanIndexType span,GirderIndexType gdr,Float64 distFromStart);
   pgsPointOfInterest(pgsTypes::Stage stage,SpanIndexType span,GirderIndexType gdr,Float64 distFromStart,PoiAttributeType attrib = POI_ALLACTIONS | POI_ALLOUTPUT);
   pgsPointOfInterest(const std::vector<pgsTypes::Stage>& stages,SpanIndexType span,GirderIndexType gdr,Float64 distFromStart,PoiAttributeType attrib = POI_ALLACTIONS | POI_ALLOUTPUT);

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
   PoiIDType GetID() const
   {
      return m_ID;
   }

   //------------------------------------------------------------------------
   // Sets the span number of this poi.
   void SetSpan(SpanIndexType span)
   {
      m_Span = span;
   }

   //------------------------------------------------------------------------
   // Returns the span number for this poi.
   SpanIndexType GetSpan() const
   {
      return m_Span;
   }

   //------------------------------------------------------------------------
   // Sets the girder index for this poi.
   void SetGirder(GirderIndexType gdr)
   {
      m_Girder = gdr;
   }

   //------------------------------------------------------------------------
   // Returns the girder index for this poi.
   GirderIndexType GetGirder() const
   {
      return m_Girder;
   }

   //------------------------------------------------------------------------
   // Sets the location of this poi, measured from the start of the girder.
   void SetDistFromStart(Float64 distFromStart)
   {
      ATLASSERT( !(distFromStart < 0) );
      m_DistFromStart = distFromStart;
   }

   //------------------------------------------------------------------------
   // Returns the location of this poi, measured from the start of the girder.
   Float64 GetDistFromStart() const
   {
      return m_DistFromStart;
   }
   //------------------------------------------------------------------------
   // Sets the POI stage attributes
   void SetAttributes(pgsTypes::Stage stage,PoiAttributeType attrib);

   //------------------------------------------------------------------------
   // Sets the POI attributes for a list of stages
   // The same attribute is applied to all stages
   void SetAttributes(const std::vector<pgsTypes::Stage>& stages,PoiAttributeType attrib);

   //------------------------------------------------------------------------
   // Returns the attributes of this POI.
   PoiAttributeType GetAttributes(pgsTypes::Stage stage) const;

   //------------------------------------------------------------------------
   // Merge stage attributes for this POI with another's
   void MergeStageAttributes(const pgsPointOfInterest& rOther);

   void MakeTenthPoint(pgsTypes::Stage stage,Uint16 tenthPoint);
   void MakeTenthPoint(const std::vector<pgsTypes::Stage>& stages,Uint16 tenthPoint);

   //------------------------------------------------------------------------
   // Tolerance for comparing distance from start.
   static void SetTolerance(Float64 tol);
   static Float64 GetTolerance();

   //------------------------------------------------------------------------
   // Returns true if this poi is used for flexure.
   bool IsFlexureCapacity(pgsTypes::Stage stage) const;
   bool IsFlexureStress(pgsTypes::Stage stage) const;

   //------------------------------------------------------------------------
   // Returns true if this poi is used for shear.
   bool IsShear(pgsTypes::Stage stage) const;

   //------------------------------------------------------------------------
   // Returns true if this poi is used for displacement.
   bool IsDisplacement(pgsTypes::Stage stage) const;

   //------------------------------------------------------------------------
   // Returns true if this poi is at a harping point
   bool IsHarpingPoint(pgsTypes::Stage stage) const;

   //------------------------------------------------------------------------
   // Returns true if this poi is at the point of application of a concentrated
   // load.
   bool IsConcentratedLoad(pgsTypes::Stage stage) const;

   //------------------------------------------------------------------------
   bool IsMidSpan(pgsTypes::Stage stage) const;

   //------------------------------------------------------------------------
   // Returns true if this poi is used for tabular reports.
   bool IsTabular(pgsTypes::Stage stage) const;

   //------------------------------------------------------------------------
   // Returns true if this poi is used for creating graphics.
   bool IsGraphical(pgsTypes::Stage stage) const;

   //------------------------------------------------------------------------
   // Returns true if this poi is h from the end of the girder or face of support.
   bool IsAtH(pgsTypes::Stage stage) const;

   //------------------------------------------------------------------------
   // Returns true if this poi is 1.5h from the end of the girder or face of support.
   bool IsAt15H(pgsTypes::Stage stage) const;

   //------------------------------------------------------------------------
   // Returns 1-11 if this point is a tenth point, zero if not.
   // 1 is start , 11 is end
   Uint16 IsATenthPoint(pgsTypes::Stage stage) const;

   bool HasAttribute(pgsTypes::Stage stage,PoiAttributeType attribute) const;

   //------------------------------------------------------------------------
   // utility functions for inserting and extracting tenth point information
   // into attributies. 
   static void SetAttributeTenthPoint(Uint16 tenthPoint, PoiAttributeType* attribute);
   static Uint16 GetAttributeTenthPoint(PoiAttributeType attribute);

   void AddStage(pgsTypes::Stage stage,PoiAttributeType attribute);
   void AddStages(const std::vector<pgsTypes::Stage>& stages,PoiAttributeType attribute);
   void RemoveStage(pgsTypes::Stage stage);
   bool HasStage(pgsTypes::Stage stage) const;
   std::vector<pgsTypes::Stage> GetStages() const;
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

   static Float64 ms_Tol;

   // Small class to store stage information for POI. Previously, we used a map here, but 
   // performance penalties were huge. 
   // Note: We may not need to use the bool here if an atribute of zero means the stage is not in use
   //       for this POI
   struct StageData
   {
      bool isSet;
      PoiAttributeType Attribute;

      StageData():
      isSet(false), Attribute(0)
      {;}

      void SetVal(bool set, PoiAttributeType attrib)
      {
         isSet = set;
         Attribute = attrib;
      }
   };

   StageData m_Stages[pgsTypes::MaxStages];

   friend pgsPoiMgr; // This guy sets the POI id.

public:
   #if defined _DEBUG
   //------------------------------------------------------------------------
   // Returns true if the object is in a valid state, otherwise returns false.
   virtual bool AssertValid() const;
#endif // _DEBUG
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
   rptPointOfInterest(const unitLength* pUnitOfMeasure = 0,
                      Float64 zeroTolerance = 0.,
                      bool bShowUnitTag = true);

   //------------------------------------------------------------------------
   rptPointOfInterest(const rptPointOfInterest& rOther);

   //------------------------------------------------------------------------
   rptPointOfInterest& operator = (const rptPointOfInterest& rOther);

   //------------------------------------------------------------------------
   virtual rptReportContent* CreateClone() const;

   //------------------------------------------------------------------------
   virtual rptReportContent& SetValue(pgsTypes::Stage stage,const pgsPointOfInterest& poi,Float64 endOffset = 0.0);

   //------------------------------------------------------------------------
   std::_tstring AsString() const;

   // Prefixes the POI with Span s Girder g
   void IncludeSpanAndGirder(bool bIncludeSpanAndGirder);
   bool IncludeSpanAndGirder() const;

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
   pgsPointOfInterest m_POI;
   pgsTypes::Stage m_Stage;
   bool m_bPrefixAttributes;
   bool m_bIncludeSpanAndGirder;
};
