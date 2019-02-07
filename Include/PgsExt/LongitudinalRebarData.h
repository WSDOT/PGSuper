///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSEXT_LONGREBARDATA_H_
#define INCLUDED_PGSEXT_LONGREBARDATA_H_

// SYSTEM INCLUDES
//
#include <WBFLCore.h>

#if !defined INCLUDED_MATHEX_H_
#include <MathEx.h>
#endif

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <StrData.h>
#include <Material\Rebar.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class GirderLibraryEntry;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   CLongitudinalRebarData

   Utility class for longitudinal rebar description data.

DESCRIPTION
   Utility class for longitudinal rebar. This class encapsulates 
   the input data a row of rebar and implements the IStructuredLoad 
   and IStructuredSave persistence interfaces.

LOG
   rab : 02.08.2007 : Created file
*****************************************************************************/

class PGSEXTCLASS CLongitudinalRebarData
{
public:
   class PGSEXTCLASS RebarRow 
   {
   public:
      pgsTypes::RebarLayoutType BarLayout;
      Float64 DistFromEnd; // Only applicable to blFromLeft, blFromRight
      Float64 BarLength; //   Applicable to blFromLeft, blFromRight, blMidGirderLength, and blMidGirderEnds

      bool bExtendedLeft, bExtendedRight; // indicates the bars are extended out of the face of the girder. extended bars are assumed to be fully developed
      // only applicable for BarLayout = blFullLength and blFromLeft and blFromRight when DistFromEnd is 0.

      pgsTypes::FaceType  Face;
      matRebar::Size BarSize;
      CollectionIndexType NumberOfBars;
      Float64     Cover;
      Float64     BarSpacing;

      RebarRow():
         Face(pgsTypes::TopFace), BarSize(matRebar::bsNone), NumberOfBars(0), Cover(0), BarSpacing(0),
         BarLayout(pgsTypes::blFullLength), DistFromEnd(0), BarLength(0), bExtendedLeft(false), bExtendedRight(false)
      {;}

      bool operator==(const RebarRow& other) const;

      // Get locations of rebar start and end measured from left end of segment
      // given a segment length. Return false if entire bar is outside of segment.
      bool GetRebarStartEnd(Float64 segmentLength, Float64* pBarStart, Float64* pBarEnd) const;

      bool IsLeftEndExtended() const;
      bool IsRightEndExtended() const;
   };

   matRebar::Type BarType;
   matRebar::Grade BarGrade;
   std::vector<RebarRow> RebarRows;


   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Constructor
   CLongitudinalRebarData();

   //------------------------------------------------------------------------
   // Copy constructor
   CLongitudinalRebarData(const CLongitudinalRebarData& rOther);

   //------------------------------------------------------------------------
   // Destructor
   ~CLongitudinalRebarData();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CLongitudinalRebarData& operator = (const CLongitudinalRebarData& rOther);
   bool operator == (const CLongitudinalRebarData& rOther) const;
   bool operator != (const CLongitudinalRebarData& rOther) const;

   // GROUP: OPERATIONS

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   // copy shear data from a girder entry
   void CopyGirderEntryData(const GirderLibraryEntry* pGirderEntry);

   // GROUP: ACCESS
   // GROUP: INQUIRY

#if defined _DEBUG
   void AssertValid();
#endif

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CLongitudinalRebarData& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CLongitudinalRebarData& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_PGSEXT_LONGREBARDATA_H_
