///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSEXT_DECKDESCRIPTION_H_
#define INCLUDED_PGSEXT_DECKDESCRIPTION_H_

#include <WBFLCore.h>
#include <MathEx.h>
#include <PGSuperTypes.h>
#include <PgsExt\DeckPoint.h>
#include <PgsExt\DeckRebarData.h>

class CBridgeDescription;


///////////////////////////////////////////////////////
// NOTE: 
// This class only exists to load old PGSuper files.
///////////////////////////////////////////////////////

/*****************************************************************************
CLASS 
   CDeckDescription

   Utility class for describing the deck.

DESCRIPTION
   Utility class for describing the deck.

LOG
   rab : 04.30.2008 : Created file
*****************************************************************************/

class PGSEXTCLASS CDeckDescription
{
public:
   CDeckDescription();
   CDeckDescription(const CDeckDescription& rOther);
   ~CDeckDescription();

   CDeckDescription& operator = (const CDeckDescription& rOther);
   bool operator == (const CDeckDescription& rOther) const;
   bool operator != (const CDeckDescription& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress,pgsTypes::SlabOffsetType* pSlabOffsetType,Float64* pSlabOffset);
   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   void SetBridgeDescription(const CBridgeDescription* pBridge);
   const CBridgeDescription* GetBridgeDescription() const;

   pgsTypes::SupportedDeckType DeckType;
   pgsTypes::AdjacentTransverseConnectivity TransverseConnectivity; // only used if SupportedBeamSpacing==sbsUniformAdjacent or sbsGeneralAdjacent
   Float64 GrossDepth; // Cast Depth if SIP
   pgsTypes::DeckOverhangTaper OverhangTaper;
   Float64 OverhangEdgeDepth; // depth of overhang at edge of slab
   Float64 Fillet;
   Float64 PanelDepth; // depth of SIP panel
   Float64 PanelSupport; // Width of SIP panel support (deduct this from roughened surface width
                         // for horizontal shear capacity)

   // Slab Concrete Material
   pgsTypes::ConcreteType SlabConcreteType;
   Float64 SlabFc;
   Float64 SlabWeightDensity;
   Float64 SlabStrengthDensity;
   Float64 SlabMaxAggregateSize;
   Float64 SlabEcK1;
   Float64 SlabEcK2;
   Float64 SlabShrinkageK1;
   Float64 SlabShrinkageK2;
   Float64 SlabCreepK1;
   Float64 SlabCreepK2;
   Float64 SlabEc;
   bool    SlabUserEc;
   bool    SlabHasFct;
   Float64 SlabFct;

   // Wearing Sutface
   pgsTypes::WearingSurfaceType WearingSurface;
   Float64 OverlayWeight; // if bInputAsDepthAndDesity is true, this value is computed from OverlayDepth and OverlayDensity and is always valid
   Float64 OverlayDepth;  // undefined if bInputAsDepthAndDensity is false
   Float64 OverlayDensity;// undefined if bInputAsDepthAndDensity is false
   bool bInputAsDepthAndDensity; // indicates if the input is by depth and density (true) or pressure load/OverlayWeight (false)
   Float64 SacrificialDepth; 

   // Rating
   pgsTypes::ConditionFactorType Condition;
   Float64 ConditionFactor;

   CDeckRebarData DeckRebarData;

   std::vector<CDeckPoint> DeckEdgePoints;


protected:
   void MakeCopy(const CDeckDescription& rOther);
   void MakeAssignment(const CDeckDescription& rOther);

   const CBridgeDescription* m_pBridgeDesc;
};


#endif // INCLUDED_PGSEXT_DECKDESCRIPTION_H_
