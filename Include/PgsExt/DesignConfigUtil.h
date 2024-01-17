///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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
#include <PgsExt\StrandData.h>
#include <PgsExt\ShearData.h>
#include <PgsExt\PrecastSegmentData.h>
#include <IFace\Bridge.h>

// Various utility functions that operate on CONFIG objects in PGSuperTypes.h

//////////////////////////////////////////////////////////////////////////
///////////// Functions for Strand Design...  ////////////////////////////
//////////////////////////////////////////////////////////////////////////

// convert config fill to compacted CDirectStrandFillCollection
CDirectStrandFillCollection PGSEXTFUNC ConvertConfigToDirectStrandFill(const ConfigStrandFillVector& rconfigfil);
ConfigStrandFillVector PGSEXTFUNC ConvertDirectToConfigFill(IStrandGeometry* pStrandGeometry, pgsTypes::StrandType type, LPCTSTR strGirderName, const CDirectStrandFillCollection& coll);
ConfigStrandFillVector PGSEXTFUNC ConvertDirectToConfigFill(IStrandGeometry* pStrandGeometry, pgsTypes::StrandType type, const CSegmentKey& segmentKey, const CDirectStrandFillCollection& coll);

// mean/lean compute class for computing strand ordering information
class PGSEXTCLASS ConfigStrandFillTool
{
public:
   // Return position indices (as from IStrandGeometry::GetStrandPositions) for our fill for given grid index. 
   // One or both may be INVALID_INDEX if no strands filled at gridIndex
   void GridIndexToStrandPositionIndex(GridIndexType gridIndex, StrandIndexType* pStrand1, StrandIndexType* pStrand2) const;


   // Get grid index for a given strand position index
   // Also get the other strand position index if two strands are located at this grid position, or INVALID_INDEX if only one
   void StrandPositionIndexToGridIndex(StrandIndexType strandPosIndex, GridIndexType* pGridIndex, StrandIndexType* pOtherStrandPosition) const;

   // Only way to construct us is with a valid fill vector
   ConfigStrandFillTool(const ConfigStrandFillVector& rFillVec);
private:
   ConfigStrandFillTool();

   std::vector< std::pair<StrandIndexType, StrandIndexType> > m_CoordinateIndices;
};

//////////////////////////////////////////////////////////////////////////
///////////// Functions for Stirrup Design... ////////////////////////////
//////////////////////////////////////////////////////////////////////////
template<class IteratorType>
ZoneIndexType GetZoneIndexAtLocation(Float64 location, Float64 girderLength, Float64 startSupportLoc, Float64 endSupportLoc,
                             bool bSymmetrical, IteratorType rItBegin, IteratorType rItEnd, ZoneIndexType collSize)
{
   if (collSize == 0)
   {
      return INVALID_INDEX;
   }

   ZoneIndexType zone = 0;

   if (bSymmetrical)
   {
      bool on_left=true;

      if (girderLength/2. < location)  // zones are symmetric about mid-span
      {
         location = girderLength-location;
         on_left=false;
      }
      ATLASSERT(0 <= location);

      Float64 end_dist;
      if (on_left)
      {
         end_dist = startSupportLoc;
      }
      else
      {
         end_dist = girderLength-endSupportLoc;
      }

      // find zone
      Float64 ezloc=0.0;
      ZoneIndexType znum=0;
      bool found=false;
      // need to give value a little 'shove' so that pois inboard from the support
      // are assigned the zone to the left, and pois outboard from the support
      // are assigned the zone to the right.
      const Float64 tol = 1.0e-6;
      for (IteratorType it = rItBegin; it != rItEnd; it++)
      {
         ezloc += it->ZoneLength;
         if (location < end_dist+tol)
         {
            if (location+tol < ezloc)
            {
               found = true;
               break;
            }
         }
         else
         {
            if (location < ezloc+tol)
            {
               found = true;
               break;
            }
         }
         znum++;
      }

      if (found)
      {
         zone = znum;
      }
      else
      {
         // not found - must be in middle
         ATLASSERT(collSize != 0); // -1 will cause an underflow 
         zone = collSize-1;
      }
   }
   else
   {
      // not symmetrical
      // find zone
      Float64 ezloc=0.0;
      ZoneIndexType znum=0;
      bool found=false;
      // need to give value a little 'shove' so that pois inboard from the support
      // are assigned the zone to the left, and pois outboard from the support
      // are assigned the zone to the right.
      const Float64 tol = 1.0e-6;
      for (IteratorType it = rItBegin; it != rItEnd; it++)
      {
         ezloc += it->ZoneLength;
         if (location < startSupportLoc + tol)
         {
            if (location+tol<ezloc)
            {
               found = true;
               break;
            }
         }
         else if (location + tol > endSupportLoc)
         {
            if (location<ezloc+tol)
            {
               found = true;
               break;
            }
         }
         else
         {
            if (location<ezloc+tol)
            {
               found = true;
               break;
            }
         }

         znum++;
      }

      if (found)
      {
         zone = znum;
      }
      else
      {
         zone = collSize-1; // extend last zone to end of girder
      }
   }

   return zone;
}


enum PrimaryStirrupType {getVerticalStirrup, getHorizShearStirrup};

Float64 PGSEXTFUNC GetPrimaryStirrupAvs(const STIRRUPCONFIG& config, PrimaryStirrupType type, Float64 location, Float64 gdrLength, Float64 leftSupportLoc,Float64 rgtSupportLoc, WBFL::Materials::Rebar::Size* pSize, Float64* pSingleBarArea, Float64* pNBars, Float64* pSpacing);
Float64 PGSEXTFUNC GetAdditionalHorizInterfaceAvs(const STIRRUPCONFIG& config, Float64 location, Float64 gdrLength, Float64 leftSupportLoc,Float64 rgtSupportLoc, WBFL::Materials::Rebar::Size* pSize, Float64* pSingleBarArea, Float64* pNBars, Float64* pSpacing);
void PGSEXTFUNC GetConfinementInfoFromStirrupConfig(const STIRRUPCONFIG& config, Float64 reqdStartZl, WBFL::Materials::Rebar::Size* pStartRBsiz, Float64* pStartZL, Float64* pStartS, Float64 reqdEndZl,   WBFL::Materials::Rebar::Size* pEndRBsiz,   Float64* pEndZL,   Float64* pEndS);
Float64 PGSEXTFUNC GetPrimaryAvLeftEnd(const STIRRUPCONFIG& config, WBFL::Materials::Rebar::Type barType, WBFL::Materials::Rebar::Grade barGrade,Float64 gdrLength, Float64 rangeLength);
Float64 PGSEXTFUNC GetPrimaryAvRightEnd(const STIRRUPCONFIG& config, WBFL::Materials::Rebar::Type barType, WBFL::Materials::Rebar::Grade barGrade,Float64 gdrLength, Float64 rangeLength);
void PGSEXTFUNC GetSplittingAvFromStirrupConfig(const STIRRUPCONFIG& config, WBFL::Materials::Rebar::Type barType, WBFL::Materials::Rebar::Grade barGrade, Float64 gdrLength, Float64 reqdStartZl, Float64* pStartAv,Float64 reqdEndZl,   Float64* pEndAv);
void PGSEXTFUNC WriteShearDataToStirrupConfig(const CShearData2* pShearData, STIRRUPCONFIG& rConfig);
bool PGSEXTFUNC DoAllStirrupsEngageDeck( const STIRRUPCONFIG& config);


// Some other utilities for dealing with strands. Perhaps we should have called this file strandutil.h
inline ConfigStrandFillVector ComputeHarpedStrandFillVector(const CSegmentKey& segmentKey,const CPrecastSegmentData& segmentData, IStrandGeometry* pStrandGeometry)
{
   if (segmentData.Strands.GetStrandDefinitionType() == pgsTypes::sdtDirectSelection )
   {
      ConfigStrandFillVector vec = ConvertDirectToConfigFill(pStrandGeometry, pgsTypes::Harped, 
                                   segmentKey,
                                   *segmentData.Strands.GetDirectStrandFillHarped());

      return vec;
   }
   else
   {
      // Num harped is in a control
      StrandIndexType Nh = segmentData.Strands.GetStrandCount(pgsTypes::Harped);

      return pStrandGeometry->ComputeStrandFill(segmentKey, pgsTypes::Harped, Nh);
   }
}

inline void DealWithLegacyEndHarpedStrandAdjustment(const CSegmentKey& segmentKey,CPrecastSegmentData& segmentData, IStrandGeometry* pStrandGeometry)
{
   // New girder types are given legacy status so we must modify data here
   if(segmentData.Strands.GetHarpStrandOffsetMeasurementAtEnd() == hsoLEGACY)
   {
      if(0 < segmentData.Strands.GetStrandCount(pgsTypes::Harped))
      {
         ConfigStrandFillVector fill = ComputeHarpedStrandFillVector(segmentKey,segmentData,pStrandGeometry);

         for ( int i = 0; i < 2; i++ )
         {
            pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
            Float64 absol_offset = pStrandGeometry->ComputeAbsoluteHarpedOffsetEnd(segmentKey, endType, fill,
                                                                                  segmentData.Strands.GetHarpStrandOffsetMeasurementAtEnd(),
                                                                                  segmentData.Strands.GetHarpStrandOffsetAtEnd(endType));

            Float64 topcg_offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteEnd(segmentKey, endType, fill,
                                                                                      hsoCGFROMTOP, absol_offset);

            segmentData.Strands.SetHarpStrandOffsetAtEnd(endType,topcg_offset);
         }
      }

      segmentData.Strands.SetHarpStrandOffsetMeasurementAtEnd(hsoCGFROMTOP);
   }
}

inline void DealWithLegacyHpHarpedStrandAdjustment(const CSegmentKey& segmentKey,CPrecastSegmentData& segmentData, IStrandGeometry* pStrandGeometry)
{
   // New girder types are given legacy status so we must modify data here for UI
   if(segmentData.Strands.GetHarpStrandOffsetMeasurementAtHarpPoint() == hsoLEGACY)
   {
      if ( 0 < segmentData.Strands.GetStrandCount(pgsTypes::Harped) )
      {
         ConfigStrandFillVector fill = ComputeHarpedStrandFillVector(segmentKey, segmentData, pStrandGeometry);

         for ( int i = 0; i < 2; i++ )
         {
            pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
            Float64 absol_offset = pStrandGeometry->ComputeAbsoluteHarpedOffsetHp(segmentKey, endType, fill,
                                                                                  segmentData.Strands.GetHarpStrandOffsetMeasurementAtHarpPoint(),
                                                                                  segmentData.Strands.GetHarpStrandOffsetAtHarpPoint(endType));

            Float64 botcg_offset = pStrandGeometry->ComputeHarpedOffsetFromAbsoluteHp(segmentKey, endType, fill,
                                                                                      hsoCGFROMBOTTOM, absol_offset);

            segmentData.Strands.SetHarpStrandOffsetAtHarpPoint(endType,botcg_offset);
         }
      }

      segmentData.Strands.SetHarpStrandOffsetMeasurementAtHarpPoint(hsoCGFROMBOTTOM);
   }
}

inline void DealWithLegacyHarpedStrandAdjustment(const CSegmentKey& segmentKey,CPrecastSegmentData& segmentData, IStrandGeometry* pStrandGeometry)
{
   DealWithLegacyEndHarpedStrandAdjustment(segmentKey,segmentData, pStrandGeometry);
   DealWithLegacyHpHarpedStrandAdjustment(segmentKey,segmentData, pStrandGeometry);
}

// NOTE: The method below could be useful to a design algorithm sometime in the future.
/*
void  PGSEXTFUNC WriteLongitudinalRebarDataToConfig(const CLongitudinalRebarData& rRebarData, LONGITUDINALREBARCONFIG& rConfig)'
*/