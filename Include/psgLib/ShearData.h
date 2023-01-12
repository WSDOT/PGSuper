///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSLIB_SHEARDATA_H_
#define INCLUDED_PGSLIB_SHEARDATA_H_

// SYSTEM INCLUDES
//
#include <WBFLCore.h>

#if !defined INCLUDED_MATHEX_H_
#include <MathEx.h>
#endif

// PROJECT INCLUDES
//
#include "psgLibLib.h"

#include <StrData.h>

// LOCAL INCLUDES
//
#include <psgLib\ShearZoneData.h>
#include <PsgLib\HorizontalInterfaceZoneData.h>

#include <Materials/Rebar.h>

// FORWARD DECLARATIONS
//
class GirderLibraryEntry;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   CShearData

   Utility class for shear zone description data.

DESCRIPTION
   Utility class for shear description data. This class encapsulates 
   the input data a single shear zone and implements the IStructuredLoad 
   and IStructuredSave persistence interfaces.

LOG
   rdp : 12.03.1998 : Created file
*****************************************************************************/

class PSGLIBCLASS CShearData2
{
public:
   WBFL::Materials::Rebar::Type  ShearBarType;
   WBFL::Materials::Rebar::Grade ShearBarGrade;

   bool  bIsRoughenedSurface;
   bool  bAreZonesSymmetrical;
   bool  bUsePrimaryForSplitting;

   typedef std::vector<CShearZoneData2> ShearZoneVec;
   typedef ShearZoneVec::iterator ShearZoneIterator;
   typedef ShearZoneVec::const_iterator ShearZoneConstIterator;
   typedef ShearZoneVec::reverse_iterator ShearZoneReverseIterator;
   typedef ShearZoneVec::const_reverse_iterator ShearZoneConstReverseIterator;
   ShearZoneVec ShearZones;

   // Horiz interface zones, splitting and confinement data added in v 9
   typedef std::vector<CHorizontalInterfaceZoneData> HorizontalInterfaceZoneVec;
   typedef HorizontalInterfaceZoneVec::iterator HorizontalInterfaceZoneIterator;
   typedef HorizontalInterfaceZoneVec::const_iterator HorizontalInterfaceZoneConstIterator;
   typedef HorizontalInterfaceZoneVec::reverse_iterator HorizontalInterfaceZoneReverseIterator;
   typedef HorizontalInterfaceZoneVec::const_reverse_iterator HorizontalInterfaceZoneConstReverseIterator;
   HorizontalInterfaceZoneVec HorizontalInterfaceZones;

   // Additional reinforcement at girder ends

   // Splitting
   WBFL::Materials::Rebar::Size SplittingBarSize;
   Float64 SplittingBarSpacing;
   Float64 SplittingZoneLength;
   Float64 nSplittingBars;

   // Confinement
   WBFL::Materials::Rebar::Size ConfinementBarSize;
   Float64 ConfinementBarSpacing;
   Float64 ConfinementZoneLength;

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Constructor
   CShearData2();

   //------------------------------------------------------------------------
   // Copy constructor
   CShearData2(const CShearData2& rOther);

   //------------------------------------------------------------------------
   // Destructor
   ~CShearData2();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CShearData2& operator = (const CShearData2& rOther);
   bool operator == (const CShearData2& rOther) const;
   bool operator != (const CShearData2& rOther) const;

   // GROUP: OPERATIONS

	HRESULT Load(WBFL::System::IStructuredLoad* pStrLoad);
	HRESULT Save(WBFL::System::IStructuredSave* pStrSave);

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
   void MakeCopy(const CShearData2& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CShearData2& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
//   static HRESULT ShearProc(IStructuredSave*,IStructuredLoad*,IProgress*,CShearData*);
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_PGSLIB_SHEARDATA_H_
