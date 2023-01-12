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

#ifndef INCLUDED_XSECTIONDATA_H_
#define INCLUDED_XSECTIONDATA_H_

// SYSTEM INCLUDES
//
#if !defined INCLUDED_MATHEX_H_
#include <MathEx.h>
#endif

// PROJECT INCLUDES
//

#include <PGSuperTypes.h>
#include <PgsExt\DeckRebarData.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class TrafficBarrierEntry;
class GirderLibraryEntry;
class ConcreteLibraryEntry;
class DiaphragmLayoutEntry;
struct ILibrary;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   CXSectionData

   Utility class for cross section description data.

DESCRIPTION
   Utility class for cross section description data. This class encapsulates all
   the input data for the bridge cross section and implements the IStructuredLoad 
   and IStructuredSave persistence interfaces.

   NOTE: This class was part of the PgsExt library. In PGSuper Version 2.1
   we went to a new method of describing the bridge and this class
   became obsolete for that purpose. This class has been moved into the project
   agent of the sole purpose of reading pgsuper project files that have
   data stored the format found in this class

LOG
   rab : 09.16.1998 : Created file
*****************************************************************************/

class CXSectionData
{
public:

   typedef enum MeasurementType
   {
      Normal    = 1, // measrued normal to the alignment
      AlongPier = 2, // measure along pier/abutment
   } MeasurementType;

   GirderIndexType GdrLineCount;
   Float64 GdrSpacing;
   MeasurementType GdrSpacingMeasurement;
   std::_tstring Girder; // name of girder from library
   pgsTypes::GirderOrientationType GirderOrientation;
   std::_tstring LeftTrafficBarrier; // name of traffic barrier from library
   std::_tstring RightTrafficBarrier; // name of traffic barrier from library

   pgsTypes::SupportedDeckType DeckType;
   pgsTypes::AdjacentTransverseConnectivity TransverseConnectivity; // only used if SupportedBeamSpacing==sbsUniformAdjacent or sbsGeneralAdjacent
   Float64 GrossDepth; // Cast Depth if SIP
   Float64 LeftOverhang;
   Float64 RightOverhang;
   pgsTypes::DeckOverhangTaper OverhangTaper;
   Float64 OverhangEdgeDepth; // depth of overhang at edge of slab
	Float64 SlabOffset; // "A" dimension
   Float64 Fillet;
   Float64 PanelDepth; // depth of SIP panel
   Float64 PanelSupport; // Width of SIP panel support (deduct this from roughened surface width
                         // for horizontal shear capacity)

   // Slab Concrete Material
   std::_tstring m_strGirderConcreteName; // in version 3 data block, concrete was defined by name
                                        // capture it here and then pass it on to the CGirderData
                                        // class load function so it can deal with it
   Float64 SlabFc;
   Float64 SlabWeightDensity;
   Float64 SlabStrengthDensity;
   Float64 SlabMaxAggregateSize;
   Float64 SlabEcK1;
   Float64 SlabEcK2;
   Float64 SlabCreepK1;
   Float64 SlabCreepK2;
   Float64 SlabShrinkageK1;
   Float64 SlabShrinkageK2;
   Float64 SlabEc;
   bool    SlabUserEc;

   pgsTypes::WearingSurfaceType WearingSurface;
	Float64 OverlayWeight;
   Float64 OverlayDepth;
   Float64 OverlayDensity;
	Float64 SacrificialDepth; 

   CDeckRebarData DeckRebarData;

   const TrafficBarrierEntry*  pLeftTrafficBarrierEntry;
   const TrafficBarrierEntry*  pRightTrafficBarrierEntry;
   const GirderLibraryEntry*   pGirderEntry;

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   CXSectionData();

   //------------------------------------------------------------------------
   // Copy constructor
   CXSectionData(const CXSectionData& rOther);

   //------------------------------------------------------------------------
   // Destructor
   ~CXSectionData();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CXSectionData& operator = (const CXSectionData& rOther);
   bool operator == (const CXSectionData& rOther) const;
   bool operator != (const CXSectionData& rOther) const;

   // GROUP: OPERATIONS

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress, ILibrary* library);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CXSectionData& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CXSectionData& rOther);

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

#endif // INCLUDED_XSECTIONDATA_H_
