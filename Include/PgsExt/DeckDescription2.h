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
#pragma once

// SYSTEM INCLUDES
//
#include <WBFLCore.h>

#if !defined INCLUDED_MATHEX_H_
#include <MathEx.h>
#endif

// PROJECT INCLUDES
//
#include <PgsExt\PgsExtExp.h>
#include <PGSuperTypes.h>
#include <PgsExt\DeckPoint.h>
#include <PgsExt\DeckRebarData.h>
#include <PgsExt\ConcreteMaterial.h>


class CBridgeDescription2;

/*****************************************************************************
CLASS 
   CDeckDescription

   Utility class for describing the deck.

DESCRIPTION
   Utility class for describing the deck.

LOG
   rab : 04.30.2008 : Created file
*****************************************************************************/

class PGSEXTCLASS CDeckDescription2
{
   friend CBridgeDescription2;

public:
   CDeckDescription2();
   CDeckDescription2(const CDeckDescription2& rOther); // copies only data, not ID or Index
   ~CDeckDescription2();

   CDeckDescription2& operator = (const CDeckDescription2& rOther);
   void CopyDeckData(const CDeckDescription2* pDeck);
   bool operator == (const CDeckDescription2& rOther) const;
   bool operator != (const CDeckDescription2& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   void SetBridgeDescription(const CBridgeDescription2* pBridge);
   const CBridgeDescription2* GetBridgeDescription() const;

   pgsTypes::SupportedDeckType DeckType;
   pgsTypes::AdjacentTransverseConnectivity TransverseConnectivity; // only used if SupportedBeamSpacing==sbsUniformAdjacent or sbsGeneralAdjacent
   Float64 GrossDepth; // Cast Depth if SIP
   pgsTypes::DeckOverhangTaper OverhangTaper;
   Float64 OverhangEdgeDepth; // depth of overhang at edge of slab
   pgsTypes::HaunchShapeType HaunchShape; // square or filleted haunch
   Float64 PanelDepth; // depth of SIP panel
   Float64 PanelSupport; // Width of SIP panel support (deduct this from roughened surface width
                         // for horizontal shear capacity)

   // Slab Concrete Material
   // Fci and Eci not used
   CConcreteMaterial Concrete;

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

   Float64 GetMinWidth() const;
   Float64 GetMaxWidth() const;


protected:
   void MakeCopy(const CDeckDescription2& rOther,bool bCopyDataOnly);
   virtual void MakeAssignment(const CDeckDescription2& rOther);

   const CBridgeDescription2* m_pBridgeDesc;

private:
   Float64 m_LegacyFillet;
};
