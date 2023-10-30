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
#pragma once

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\BridgeDescription2.h>
#include <Math\PiecewiseFunction.h>

/// <summary>
/// This tool converts the current haunch data in the CBridgeDescription2 passed to a new layout. It is primarily intended
/// for use in the haunch dialogs, but is also useful for converting data from old input files
/// </summary>

class PGSEXTCLASS HaunchDepthInputConversionTool 
{
public:
   // Will change the bridgedescr passed in for the specified group. If you don't want it changed, pass in a copy
   // The caller owns the bridgedescr object and is responsible for its lifetime.
   // Note that the conversion can occur at project load time. We have to make special considerations here because the bridge model is not built yet
   HaunchDepthInputConversionTool(const CBridgeDescription2* pBridgeDescr, CComPtr<IBroker> pBroker,bool bIsAtLoadTime);

   // Geometric functions to convert haunch data in the bridgedesc to the new format. If the bool returned is true, a conversion took place. If false, no change was required
   std::pair<bool, CBridgeDescription2> ConvertToSlabOffsetInput(pgsTypes::SlabOffsetType newSlabOffsetType);
   std::pair<bool,CBridgeDescription2> ConvertToDirectHaunchInput(pgsTypes::HaunchInputLocationType newHaunchInputLocationType,pgsTypes::HaunchLayoutType newHaunchLayoutType,pgsTypes::HaunchInputDistributionType newHaunchInputDistributionType, bool forceInit=false);

   // Condense direct haunch input if haunches are equal across bridge or segments/spans. Returns true if changes
   bool CondenseDirectHaunchInput(CBridgeDescription2* pBridgeDescr);


   // Function to design haunches of all segments along a girderline or an individual segment based on the currently input haunch layout. 
   // rDesignGirderKey is girder where design is to be applied.
   // sourceGirderIdx is index of girderline where design is to take place. This allows you to design one girder and apply the design to another.
   // Return value is a new bridgedescr with designed values
   std::pair<bool,CBridgeDescription2> DesignHaunches(const CGirderKey& rDesignGirderKey, GirderIndexType sourceGirderIdx, pgsTypes::HaunchInputDistributionType inputDistributionType, bool bApply2AllGdrs);

private:
   // Compute span ends, segment ends, and current haunch layout along girderline(s) using a piecewise-linear approx
   void InitializeGeometrics(bool bSingleGirderLineOnly);
   bool m_WasGeometricsInitialized;

   HaunchDepthInputConversionTool();

   const CBridgeDescription2* m_pBridgeDescr;
   CComPtr<IBroker> m_pBroker;
   bool m_bIsAtLoadTime;

   struct GirderlineHaunchLayout
   {
      // Layouts below are all in girderline coordinates
      // Layout of haunch depths for each girderline for bridgedescr just before conversion 
      std::shared_ptr<WBFL::Math::Function> m_pHaunchDepths; // A piecewise linear function mostly along segment tenth points
      // Location of ends of spans (cl piers) 
      std::vector< std::pair<Float64,Float64> > m_SpanEnds;
      // Location of ends of segments 
      std::vector<std::pair<Float64,Float64> > m_SegmentEnds; // (start, end)
   };

   // Layout for all girderlines
   std::vector< GirderlineHaunchLayout> m_GirderlineHaunchLayouts;
};
