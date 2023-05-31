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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\HaunchDepthInputConversionTool.h>

#include <MathEx.h>
#include <GenericBridge\Helpers.h>

#include <IFace/Bridge.h>
#include <IFace/PointOfInterest.h>
#include <IFace/Project.h>
#include <IFace/Alignment.h>
#include <IFace\Intervals.h>
#include <IFace\AnalysisResults.h>
#include <IFace\DocumentType.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

inline bool CompareFloatVectors(const std::vector<Float64>& vec1, const std::vector<Float64>& vec2)
{
   if (vec1.size() != vec2.size())
   {
      return false;
   }
   else
   {
      std::vector<Float64>::const_iterator it1 = vec1.begin();
      std::vector<Float64>::const_iterator it2 = vec2.begin();
      while (it1 != vec1.end())
      {
         if (!IsEqual(*it1,*it2))
         {
            return false;
         }

         it1++;
         it2++;
      }
   }

   return true;
}

// Function to compute haunches along a segment (or span) using function. Assumes vector is empty going in
inline std::vector<Float64> LayoutHaunches(Float64 startLoc,Float64 segmentLength,int numValues,std::shared_ptr<WBFL::Math::Function> pHaunchDepthFunction )
{
   std::vector<Float64> haunchVector;
   haunchVector.reserve(numValues);
   if (numValues > 1)
   {

      for (int i = 0; i < numValues; i++)
      {
         Float64 segLoc = startLoc + segmentLength * (Float64)i / (Float64)(numValues - 1);
         Float64 haunch = pHaunchDepthFunction->Evaluate(segLoc);
         haunchVector.push_back(haunch);
      }
   }
   else
   {
      ATLASSERT(numValues == 1);
      // constant haunches along segment. Take value at mid-point
      Float64 haunch = pHaunchDepthFunction->Evaluate(startLoc + segmentLength/2.0);
      haunchVector.push_back(haunch);
   }

   return haunchVector;
}

HaunchDepthInputConversionTool::HaunchDepthInputConversionTool(const CBridgeDescription2* pBridgeDescr,CComPtr<IBroker> pBroker, bool bIsAtLoadTime):
m_pBridgeDescr(pBridgeDescr),
m_pBroker(pBroker),
m_bIsAtLoadTime(bIsAtLoadTime),
m_WasGeometricsInitialized(false)
{
}

std::pair<bool,CBridgeDescription2> HaunchDepthInputConversionTool::ConvertToSlabOffsetInput(pgsTypes::SlabOffsetType newSlabOffsetType)
{
   auto  convertedBridgeDescrPair = std::make_pair(false, *m_pBridgeDescr); // don't need conversion by default

   if (!(m_pBridgeDescr->GetHaunchInputDepthType() == pgsTypes::hidACamber && m_pBridgeDescr->GetSlabOffsetType() == newSlabOffsetType))
   {
      convertedBridgeDescrPair.first = true; // go to work

      convertedBridgeDescrPair.second.SetHaunchInputDepthType(pgsTypes::hidACamber);

      if (m_pBridgeDescr->GetHaunchInputDepthType() == pgsTypes::hidACamber)
      {
         pgsTypes::SlabOffsetType currentSlabOffsetType = m_pBridgeDescr->GetSlabOffsetType();
         // Converting from one type of "A" camber to another
         // We Don't need to build geometry data structures for these cases
         if (currentSlabOffsetType == newSlabOffsetType)
         {
            // Nothing to do here
            return convertedBridgeDescrPair;
         }
         else
         {
            // We are converting
            convertedBridgeDescrPair.first = true;

            // Slab offsets are at different locations
            if (pgsTypes::sotBridge == newSlabOffsetType)
            {
               // same value for entire bridge. Take value from group 0, girder 0, segment 0
               const CGirderGroupData* pGroup = m_pBridgeDescr->GetGirderGroup((GroupIndexType)0);
               const CSplicedGirderData* pGirder = pGroup->GetGirder(0);
               const CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
               Float64 back,ahead;
               pSegment->GetSlabOffset(&back,&ahead,true);

               convertedBridgeDescrPair.second.SetSlabOffsetType(newSlabOffsetType);
               convertedBridgeDescrPair.second.SetSlabOffset(ahead);
            }
            else if (pgsTypes::sotBearingLine == newSlabOffsetType)
            {
               if (pgsTypes::sotBridge == currentSlabOffsetType)
               {
                  Float64 slabOffset = m_pBridgeDescr->GetSlabOffset();

                  convertedBridgeDescrPair.second.SetSlabOffsetType(newSlabOffsetType);

                  PierIndexType pierCnt = m_pBridgeDescr->GetPierCount();
                  for (PierIndexType iPier = 0; iPier < pierCnt; iPier++)
                  {
                     CPierData2* pPier = convertedBridgeDescrPair.second.GetPier(iPier);
                     pPier->SetSlabOffset(slabOffset,slabOffset);
                  }
               }
               else
               {
                  ATLASSERT(pgsTypes::sotSegment == currentSlabOffsetType);
                  // BIG assumption here: Slab offsets are only used in PGSuper models, and in these models segments always terminate at piers
                  // Take values from segments in girder 0 and assign to piers
                  PierIndexType pierCnt = m_pBridgeDescr->GetPierCount();
                  for (PierIndexType iPier = 0; iPier < pierCnt; iPier++)
                  {
                     Float64 backSlabOffset(0),aheadSlabOffset(0);
                     if (iPier == 0)
                     {
                        const CGirderGroupData* pGroup = m_pBridgeDescr->GetGirderGroup((GroupIndexType)0);
                        const CSplicedGirderData* pGirder = pGroup->GetGirder(0);
                        const CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
                        aheadSlabOffset = pSegment->GetSlabOffset(pgsTypes::metStart);
                        backSlabOffset = aheadSlabOffset;
                     }
                     else if (iPier == pierCnt - 1)
                     {
                        const CGirderGroupData* pGroup = m_pBridgeDescr->GetGirderGroup((GroupIndexType)iPier - 1);
                        const CSplicedGirderData* pGirder = pGroup->GetGirder(0);
                        const CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
                        backSlabOffset = pSegment->GetSlabOffset(pgsTypes::metEnd);
                        aheadSlabOffset = backSlabOffset;
                     }
                     else
                     {
                        const CGirderGroupData* pGroup = m_pBridgeDescr->GetGirderGroup((GroupIndexType)iPier - 1);
                        const CSplicedGirderData* pGirder = pGroup->GetGirder(0);
                        const CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
                        backSlabOffset = pSegment->GetSlabOffset(pgsTypes::metEnd);

                        pGroup = m_pBridgeDescr->GetGirderGroup((GroupIndexType)iPier);
                        pGirder = pGroup->GetGirder(0);
                        pSegment = pGirder->GetSegment(0);
                        aheadSlabOffset = pSegment->GetSlabOffset(pgsTypes::metStart);
                     }

                     CPierData2* pPier = convertedBridgeDescrPair.second.GetPier(iPier);
                     pPier->SetSlabOffset(backSlabOffset,aheadSlabOffset);
                  }
               }
            }
            else
            {
               ATLASSERT(pgsTypes::sotSegment == newSlabOffsetType);
               if (pgsTypes::sotBridge == currentSlabOffsetType)
               {
                  Float64 slabOffset = m_pBridgeDescr->GetSlabOffset();

                  GroupIndexType groupCnt = m_pBridgeDescr->GetGirderGroupCount();
                  for (GroupIndexType groupIdx = 0; groupIdx < groupCnt; groupIdx++)
                  {
                     CGirderGroupData* pGroup = convertedBridgeDescrPair.second.GetGirderGroup(groupIdx);
                     GirderIndexType girderCnt = pGroup->GetGirderCount();
                     for (GirderIndexType iGirder = 0; iGirder < girderCnt; iGirder++)
                     {
                        CSplicedGirderData* pGirder = pGroup->GetGirder(iGirder);
                        SegmentIndexType segmentCnt = pGirder->GetSegmentCount();
                        for (SegmentIndexType segmentIdxment = 0; segmentIdxment < segmentCnt; segmentIdxment++)
                        {
                           CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentIdxment);
                           pSegment->SetSlabOffset(slabOffset,slabOffset);
                        }
                     }
                  }
               }
               else
               {
                  ATLASSERT(pgsTypes::sotBearingLine == currentSlabOffsetType);

                  convertedBridgeDescrPair.second.SetSlabOffsetType(pgsTypes::sotBearingLine);

                  // Again, we make the assumption here that in PGSuper groups end a piers
                  GroupIndexType groupCnt = m_pBridgeDescr->GetGirderGroupCount();
                  for (GroupIndexType groupIdx = 0; groupIdx < groupCnt; groupIdx++)
                  {
                     Float64 startOffset,endOffset;
                     const CPierData2* pStartPier = m_pBridgeDescr->GetPier(groupIdx);
                     pStartPier->GetSlabOffset(pgsTypes::Ahead,&startOffset);

                     const CPierData2* pEndPier = m_pBridgeDescr->GetPier(groupIdx + 1);
                     pEndPier->GetSlabOffset(pgsTypes::Back,&endOffset);

                     // set our converted bridge values
                     CGirderGroupData* pGroup = convertedBridgeDescrPair.second.GetGirderGroup(groupIdx);
                     GirderIndexType girderCnt = pGroup->GetGirderCount();
                     for (GirderIndexType iGirder = 0; iGirder < girderCnt; iGirder++)
                     {
                        CSplicedGirderData* pGirder = pGroup->GetGirder(iGirder);
                        SegmentIndexType segmentCnt = pGirder->GetSegmentCount();
                        for (SegmentIndexType segmentIdxment = 0; segmentIdxment < segmentCnt; segmentIdxment++)
                        {
                           CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentIdxment);
                           pSegment->SetSlabOffset(startOffset,endOffset);
                        }
                     }
                  }
               }
            }
         }
      }
      else
      {
         // Conversion from direct haunch input to "A" - all we can do is get "A" at ends and assume that assumed excess camber is parabolic from CL
         // Build haunch data for existing condition
         InitializeGeometrics(pgsTypes::sotBridge == newSlabOffsetType);

         GET_IFACE(IBridge,pBridge);

         if (pgsTypes::sotBridge == newSlabOffsetType)
         {
            convertedBridgeDescrPair.second.SetHaunchInputDepthType(pgsTypes::hidACamber);
            convertedBridgeDescrPair.second.SetSlabOffsetType(pgsTypes::sotBridge);

            // Use data from girder 0, segment 0 for entire bridge
            const CGirderGroupData* pGroup = m_pBridgeDescr->GetGirderGroup((GroupIndexType)0);
            const CSplicedGirderData* pGirder = pGroup->GetGirder(0);
            const CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
            const CSegmentKey& segmentKey = pSegment->GetSegmentKey();

            Float64 segmentStartEndDist = pBridge->GetSegmentStartEndDistance(segmentKey);
            Float64 segmentEndEndDist = pBridge->GetSegmentEndEndDistance(segmentKey);
            Float64 segLength = segmentEndEndDist - segmentStartEndDist;

            const GirderlineHaunchLayout& rLayout = m_GirderlineHaunchLayouts[0];

            Float64 segStartLocOnGirderLine = rLayout.m_SegmentEnds[0].first;
            Float64 segEndLocOnGirderLine = rLayout.m_SegmentEnds[0].second;

            Float64 segStartBearingLoc = segStartLocOnGirderLine + segmentStartEndDist;
            Float64 segEndBearingLoc = segEndLocOnGirderLine - segmentEndEndDist;

            // Haunch depths at segment bearing locations
            Float64 haunchStart = rLayout.m_pHaunchDepths->Evaluate(segStartBearingLoc);
            Float64 haunchEnd = rLayout.m_pHaunchDepths->Evaluate(segEndBearingLoc);

            pgsPointOfInterest poiStart(segmentKey,0.0);
            Float64 slabDepthStart = pBridge->GetGrossSlabDepth(poiStart);
            pgsPointOfInterest poiEnd(segmentKey,segLength);
            Float64 slabDepthEnd = pBridge->GetGrossSlabDepth(poiEnd);

            Float64 AStart = haunchStart + slabDepthStart;
            Float64 AEnd = haunchEnd + slabDepthEnd;

            // Set our values
            convertedBridgeDescrPair.second.SetSlabOffset((AStart + AEnd) / 2.0); // use average of start and end values
         }
         else if (pgsTypes::sotBearingLine == newSlabOffsetType)
         {
            // Use data from girder 0, for bearing lines. Since we are converting to PGSuper, each group/span/segment are the same
            const GirderlineHaunchLayout& rLayout = m_GirderlineHaunchLayouts[0];

            convertedBridgeDescrPair.second.SetHaunchInputDepthType(pgsTypes::hidACamber);
            convertedBridgeDescrPair.second.SetSlabOffsetType(pgsTypes::sotBearingLine);

            GroupIndexType numGroups = m_pBridgeDescr->GetGirderGroupCount();
            for (GroupIndexType groupIdx = 0; groupIdx < numGroups; groupIdx++)
            {
               const CGirderGroupData* pGroup = m_pBridgeDescr->GetGirderGroup(groupIdx);
               const CSplicedGirderData* pGirder = pGroup->GetGirder(0);
               const CPrecastSegmentData* pSegment = pGirder->GetSegment(0);
               const CSegmentKey& segmentKey = pSegment->GetSegmentKey();

               Float64 segmentStartEndDist = pBridge->GetSegmentStartEndDistance(segmentKey);
               Float64 segmentEndEndDist = pBridge->GetSegmentEndEndDistance(segmentKey);
               Float64 segLength = segmentEndEndDist - segmentStartEndDist;

               Float64 segStartLocOnGirderLine = rLayout.m_SegmentEnds[0].first;
               Float64 segEndLocOnGirderLine = rLayout.m_SegmentEnds[0].second;

               Float64 segStartBearingLoc = segStartLocOnGirderLine + segmentStartEndDist;
               Float64 segEndBearingLoc = segEndLocOnGirderLine - segmentEndEndDist;

               // Haunch depths at segment bearing locations
               Float64 haunchStart = rLayout.m_pHaunchDepths->Evaluate(segStartBearingLoc);
               Float64 haunchEnd = rLayout.m_pHaunchDepths->Evaluate(segEndBearingLoc);

               pgsPointOfInterest poiStart(segmentKey,0.0);
               Float64 slabDepthStart = pBridge->GetGrossSlabDepth(poiStart);
               pgsPointOfInterest poiEnd(segmentKey,segLength);
               Float64 slabDepthEnd = pBridge->GetGrossSlabDepth(poiEnd);

               Float64 AStart = haunchStart + slabDepthStart;
               Float64 AEnd = haunchEnd + slabDepthEnd;

               // Set our values (assume group/span/pier relationship in pgsuper)
               CPierData2* pconvertedStartPier = convertedBridgeDescrPair.second.GetPier(groupIdx);
               pconvertedStartPier->SetSlabOffset(pgsTypes::Ahead,AStart);

               CPierData2* pconvertedEndPier = convertedBridgeDescrPair.second.GetPier(groupIdx + 1);
               pconvertedEndPier->SetSlabOffset(pgsTypes::Back,AEnd);
            }
         }
         else
         {
            ATLASSERT(pgsTypes::sotSegment == newSlabOffsetType);
            convertedBridgeDescrPair.second.SetHaunchInputDepthType(pgsTypes::hidACamber);
            convertedBridgeDescrPair.second.SetSlabOffsetType(pgsTypes::sotSegment);

            GroupIndexType numGroups = m_pBridgeDescr->GetGirderGroupCount();
            for (GroupIndexType groupIdx = 0; groupIdx < numGroups; groupIdx++)
            {
               const CGirderGroupData* pGroup = m_pBridgeDescr->GetGirderGroup(groupIdx);

               GirderIndexType numGdrs = pGroup->GetGirderCount();
               for (GirderIndexType gdrIdx = 0; gdrIdx < numGdrs; gdrIdx++)
               {
                  const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
                  ATLASSERT(pGirder->GetSegmentCount() == 1);
                  const CPrecastSegmentData* pSegment = pGirder->GetSegment(0); // assume one segment per girder

                  const CSegmentKey& segmentKey = pSegment->GetSegmentKey();
                  Float64 segmentStartEndDist = pBridge->GetSegmentStartEndDistance(segmentKey);
                  Float64 segmentEndEndDist = pBridge->GetSegmentEndEndDistance(segmentKey);
                  Float64 segLength = segmentEndEndDist - segmentStartEndDist;

                  const GirderlineHaunchLayout& rLayout = m_GirderlineHaunchLayouts[gdrIdx];

                  Float64 segStartLocOnGirderLine = rLayout.m_SegmentEnds[0].first;
                  Float64 segEndLocOnGirderLine = rLayout.m_SegmentEnds[0].second;

                  Float64 segStartBearingLoc = segStartLocOnGirderLine + segmentStartEndDist;
                  Float64 segEndBearingLoc = segEndLocOnGirderLine - segmentEndEndDist;

                  // Compute Haunch depths at segment bearing locations
                  Float64 haunchStart = rLayout.m_pHaunchDepths->Evaluate(segStartBearingLoc);
                  Float64 haunchEnd = rLayout.m_pHaunchDepths->Evaluate(segEndBearingLoc);

                  pgsPointOfInterest poiStart(segmentKey,0.0);
                  Float64 slabDepthStart = pBridge->GetGrossSlabDepth(poiStart);
                  pgsPointOfInterest poiEnd(segmentKey,segLength);
                  Float64 slabDepthEnd = pBridge->GetGrossSlabDepth(poiEnd);

                  Float64 AStart = haunchStart + slabDepthStart;
                  Float64 AEnd = haunchEnd + slabDepthEnd;

                  // Set our values (assume group/span/pier relationship in pgsuper)
                  CGirderGroupData* pConvertedGroup = convertedBridgeDescrPair.second.GetGirderGroup(groupIdx);
                  CSplicedGirderData* pConvertedGirder = pConvertedGroup->GetGirder(gdrIdx);
                  CPrecastSegmentData* pConvertedSegment = pConvertedGirder->GetSegment(0); // assume one segment per girder

                  pConvertedSegment->SetSlabOffset(AStart,AEnd);
               }
            }
         }
      }
   } // needsConversion

   return convertedBridgeDescrPair;
}

std::pair<bool,CBridgeDescription2> HaunchDepthInputConversionTool::ConvertToDirectHaunchInput(pgsTypes::HaunchInputLocationType newHaunchInputLocationType,pgsTypes::HaunchLayoutType newHaunchLayoutType,pgsTypes::HaunchInputDistributionType newHaunchInputDistributionType,bool forceInit)
{
   auto  convertedBridgeDescrPair = std::make_pair(false,*m_pBridgeDescr); // no conversion by default

   bool doesntNeedConversion = (m_pBridgeDescr->GetHaunchInputDepthType()!=pgsTypes::hidACamber && m_pBridgeDescr->GetHaunchInputLocationType() == newHaunchInputLocationType &&
                                m_pBridgeDescr->GetHaunchLayoutType() == newHaunchLayoutType && m_pBridgeDescr->GetHaunchInputDistributionType() == newHaunchInputDistributionType);
   if (!doesntNeedConversion || forceInit)
   {
      convertedBridgeDescrPair.first = true;

      // This function does most of the hard working laying out haunchs along segments and spans
      bool isSingleGirderLine = pgsTypes::hilPerEach != newHaunchInputLocationType;
      InitializeGeometrics(isSingleGirderLine);

      convertedBridgeDescrPair.second.SetHaunchInputDepthType(pgsTypes::hidHaunchDirectly);

      if (pgsTypes::hltAlongSegments == newHaunchLayoutType)
      {
         convertedBridgeDescrPair.second.SetHaunchLayoutType(pgsTypes::hltAlongSegments);
         convertedBridgeDescrPair.second.SetHaunchInputDistributionType(newHaunchInputDistributionType);

         if (pgsTypes::hilSame4Bridge == newHaunchInputLocationType)
         {
            // Use values for girder 0, group 0, segment 0 for entire bridge
            const GirderlineHaunchLayout& rLayout = m_GirderlineHaunchLayouts[0];

            Float64 segmentLength = rLayout.m_SegmentEnds[0].second - rLayout.m_SegmentEnds[0].first;
            Float64 segmentStart = rLayout.m_SegmentEnds[0].first;

            std::vector<Float64> haunches = LayoutHaunches(segmentStart,segmentLength,(int)newHaunchInputDistributionType,rLayout.m_pHaunchDepths);

            convertedBridgeDescrPair.second.SetHaunchInputLocationType(pgsTypes::hilSame4Bridge);
            convertedBridgeDescrPair.second.SetDirectHaunchDepths(haunches);
         }
         else if (pgsTypes::hilSame4AllGirders == newHaunchInputLocationType || pgsTypes::hilPerEach == newHaunchInputLocationType)
         {
            convertedBridgeDescrPair.second.SetHaunchInputLocationType(newHaunchInputLocationType);
            convertedBridgeDescrPair.second.SetHaunchLayoutType(pgsTypes::hltAlongSegments);

            std::vector<std::size_t> segVecIdxs(m_GirderlineHaunchLayouts.size(), 0); // index into m_SegmentEnds vector
                                                                                      // m_SegmentEnds[gdrIdx] is for all segments for all groups
            GroupIndexType groupCnt = m_pBridgeDescr->GetGirderGroupCount();
            for (GroupIndexType groupIdx = 0; groupIdx < groupCnt; groupIdx++)
            {
               CGirderGroupData* pGroup = convertedBridgeDescrPair.second.GetGirderGroup(groupIdx);

               if (pgsTypes::hilSame4AllGirders == newHaunchInputLocationType)
               {
                  // Use values for girder 0, for all segments along bridge
                  const GirderIndexType gdrIdx = 0;
                  const GirderlineHaunchLayout& rLayout = m_GirderlineHaunchLayouts[gdrIdx];

                  CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);

                  SegmentIndexType numSegs = pGirder->GetSegmentCount();
                  for (SegmentIndexType segmentIdx = 0; segmentIdx < numSegs; segmentIdx++)
                  {
                     CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentIdx);

                     Float64 segmentLength = rLayout.m_SegmentEnds[ segVecIdxs[gdrIdx] ].second - rLayout.m_SegmentEnds[ segVecIdxs[gdrIdx] ].first;
                     Float64 segmentStart = rLayout.m_SegmentEnds[ segVecIdxs[gdrIdx] ].first;

                     std::vector<Float64> haunches = LayoutHaunches(segmentStart,segmentLength,(int)newHaunchInputDistributionType,rLayout.m_pHaunchDepths);

                     pGirder->SetDirectHaunchDepths(segmentIdx, haunches);
                     segVecIdxs[gdrIdx]++;
                  }
               }
               else
               {
                  // hilPerEach: Set to all girders and segments
                  GirderIndexType numGirders = pGroup->GetGirderCount();
                  for (GirderIndexType gdrIdx = 0; gdrIdx < numGirders; gdrIdx++)
                  {
                     const GirderlineHaunchLayout& rLayout = m_GirderlineHaunchLayouts[gdrIdx];

                     CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);

                     SegmentIndexType numSegs = pGirder->GetSegmentCount();
                     for (SegmentIndexType segmentIdx = 0; segmentIdx < numSegs; segmentIdx++)
                     {

                        CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentIdx);

                        Float64 segmentLength = rLayout.m_SegmentEnds[ segVecIdxs[gdrIdx] ].second - rLayout.m_SegmentEnds[ segVecIdxs[gdrIdx] ].first;
                        Float64 segmentStart = rLayout.m_SegmentEnds[segVecIdxs[gdrIdx]].first;

                        std::vector<Float64> haunches = LayoutHaunches(segmentStart,segmentLength,(int)newHaunchInputDistributionType,rLayout.m_pHaunchDepths);

                        pSegment->SetDirectHaunchDepths(haunches);
                        segVecIdxs[gdrIdx]++;
                     }
                  }
               }
            }
         }
         else
         {
            ATLASSERT(0);
         }
      }
      else
      {
         // haunch defined along spans
         ATLASSERT(pgsTypes::hltAlongSpans == newHaunchLayoutType);
         convertedBridgeDescrPair.second.SetHaunchLayoutType(pgsTypes::hltAlongSpans);
         convertedBridgeDescrPair.second.SetHaunchInputDistributionType(newHaunchInputDistributionType);

         if (pgsTypes::hilSame4Bridge == newHaunchInputLocationType)
         {
            // Use values for girder 0, group 0, segment 0 for entire bridge
            const GirderlineHaunchLayout& rLayout = m_GirderlineHaunchLayouts[0];

            Float64 spanLength = rLayout.m_SpanEnds[0].second - rLayout.m_SpanEnds[0].first;
            Float64 spanStart = rLayout.m_SpanEnds[0].first;

            // Use inline function to build haunches along span
            std::vector<Float64> haunches = LayoutHaunches(spanStart, spanLength, (int)newHaunchInputDistributionType, rLayout.m_pHaunchDepths);

            convertedBridgeDescrPair.second.SetHaunchInputLocationType(pgsTypes::hilSame4Bridge);
            convertedBridgeDescrPair.second.SetDirectHaunchDepths(haunches);
         }
         else if (pgsTypes::hilSame4AllGirders == newHaunchInputLocationType)
         {
            convertedBridgeDescrPair.second.SetHaunchInputLocationType(pgsTypes::hilSame4AllGirders);

            // Use values for girder 0, for all spans along bridge
            const GirderIndexType gdrIdx = 0;
            const GirderlineHaunchLayout& rLayout = m_GirderlineHaunchLayouts[gdrIdx];

            SpanIndexType spanCnt = m_pBridgeDescr->GetSpanCount();
            for (SpanIndexType spanIdx = 0; spanIdx < spanCnt; spanIdx++)
            {
               CSpanData2* pSpan = convertedBridgeDescrPair.second.GetSpan(spanIdx);
               GirderIndexType numGdrs = pSpan->GetGirderCount();
               Float64 spanLength = rLayout.m_SpanEnds[spanIdx].second - rLayout.m_SpanEnds[spanIdx].first;
               Float64 spanStart = rLayout.m_SpanEnds[spanIdx].first;

               std::vector<Float64> haunches = LayoutHaunches(spanStart,spanLength,(int)newHaunchInputDistributionType,rLayout.m_pHaunchDepths);

               pSpan->SetDirectHaunchDepths(haunches);
            }
         }
         else
         {
            ATLASSERT(pgsTypes::hilPerEach == newHaunchInputLocationType);
            convertedBridgeDescrPair.second.SetHaunchInputLocationType(pgsTypes::hilPerEach);

            std::vector<Float64> haunches(newHaunchInputDistributionType,0.0);

            SpanIndexType spanCnt = m_pBridgeDescr->GetSpanCount();
            for (SpanIndexType spanIdx = 0; spanIdx < spanCnt; spanIdx++)
            {
               CSpanData2* pSpan = convertedBridgeDescrPair.second.GetSpan(spanIdx);
               GirderIndexType numGdrs = pSpan->GetGirderCount();
               for (GirderIndexType gdrIdx = 0; gdrIdx < numGdrs; gdrIdx++)
               {
                  const GirderlineHaunchLayout& rLayout = m_GirderlineHaunchLayouts[gdrIdx];

                  Float64 spanLength = rLayout.m_SpanEnds[spanIdx].second - rLayout.m_SpanEnds[spanIdx].first;
                  Float64 spanStart  = rLayout.m_SpanEnds[spanIdx].first;

                  std::vector<Float64> haunches = LayoutHaunches(spanStart,spanLength,(int)newHaunchInputDistributionType,rLayout.m_pHaunchDepths);

                  pSpan->SetDirectHaunchDepths(gdrIdx,haunches);
               }
            }
         }
      }
   }

   return convertedBridgeDescrPair;
}

std::pair<bool,CBridgeDescription2> HaunchDepthInputConversionTool::DesignHaunches(const CGirderKey& rDesignGirderKey,GirderIndexType sourceGirderIdx,pgsTypes::HaunchInputDistributionType inputDistributionType,bool bApply2AllGdrs)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IPointOfInterest,pPoi);
   GET_IFACE(IRoadway,pRoadway);
   GET_IFACE_NOCHECK(IGirder,pGirder);
   GET_IFACE(IDeformedGirderGeometry,pDeformedGirderGeometry);

   // First convert haunch data in bridgdescr2 so we can operate on individual segments/girderlines. 
   // We will compute design values at 1/10th points and convert to simpler format if needed

   // NOTE that function below will call this->Initialize(), so we don't need to call it here.
   auto convertedBridgeDescrPair = ConvertToDirectHaunchInput(pgsTypes::hilPerEach,pgsTypes::hltAlongSegments,pgsTypes::hidTenthPoints,true);
   ATLASSERT(convertedBridgeDescrPair.first);

   // Use a reference for readability
   CBridgeDescription2& rDesignBridgeDescr(convertedBridgeDescrPair.second);

   // Theory:
   // The design algo here is pretty simple 2-step process. 
   //     1. Compute the difference between the computed CL girder deck elevation (based on the current haunch depths) to
   //        the cl girder PGL, and add this difference to the current haunch depths. This results in design CL haunch.
   //     2. Take a second pass through the design haunch depths and make it that the minimum depth is slightly greater than the fillet
   //        accounting for cross-slope and girder orientation effects.
   // 
   // In reality the algo is iterative in order to refine the result, but I have a found that a single iteration does a pretty good job.
   // Users can make a second pass if a better solution is desired.

   // We will use our (tool's) computed haunch layout to get input haunch depths at POI locations
   const GirderlineHaunchLayout& rLayout = m_GirderlineHaunchLayouts[sourceGirderIdx];

   GroupIndexType startGrp,endGrp;
   if (rDesignGirderKey.groupIndex == ALL_GROUPS)
   {
      startGrp = 0;
      endGrp = pBridge->GetGirderGroupCount() - 1;
   }
   else
   {
      startGrp = rDesignGirderKey.groupIndex;
      endGrp = rDesignGirderKey.groupIndex;
   }

   // We design for the GCE
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType gceIntervalIdx = pIntervals->GetGeometryControlInterval();

   // Use a buffer of 1/16" for a fillet tolerance. This will ensure that our design doesn't miss by a tiny amount
   Float64 filletBuffer = WBFL::Units::ConvertToSysUnits(1.0 / 16.0,WBFL::Units::Measure::Inch);
   Float64 nominalFillet = pBridge->GetFillet() + filletBuffer;

   for (GroupIndexType iGrp = startGrp; iGrp <= endGrp; iGrp++)
   {
      SegmentIndexType numSegs = pBridge->GetSegmentCount(iGrp,rDesignGirderKey.girderIndex);

      // The goal is to have the smallest haunch depth possible based on fillet and cross slope effects.
      // Hence, we compute required CL haunches along each segment and the max fillet-cross slope effect and the adjustment
      // required to obtain the smallest haunch depth. However, the adjustment must be made to all haunches along the total group
      // or else we get jumps in haunch depth between segment ends. Below is the total group adjustment
      Float64 maxHaunchAdjustment4Group(-Float64_Max);

      // Design haunches computed using CL effects only for each segment 
      std::vector< std::vector<Float64> > designClHaunches;
      designClHaunches.assign(numSegs,std::vector<Float64>());

      for (SegmentIndexType iSeg = 0; iSeg < numSegs; iSeg++)
      {
         // Get points of interest at locations where we are going to compute design haunch depths (1/10th points on segment)
         PoiList vPoi;
         CSegmentKey segmentKey(iGrp, sourceGirderIdx, iSeg);

         PoiList vPoi2;
         pPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_TENTH_POINTS,&vPoi2);
         if (vPoi2.empty())
         {
            // We could have a non-existant location if there is a different number of girders per span (or group)
            continue;
         }

         // Remove first and last elements at segment since they are at support locations (they are not 10th points), and then add segment ends
         vPoi2.pop_back();
         vPoi2.erase(vPoi2.begin());
         vPoi.insert(vPoi.end(),vPoi2.begin(),vPoi2.end());
         pPoi->GetPointsOfInterest(segmentKey,POI_START_FACE | POI_END_FACE,&vPoi);
         pPoi->SortPoiList(&vPoi); // sorts and removes duplicates
         ATLASSERT(vPoi.size() == 11);

         // Compute CL haunch design values. Use reference for readability
         std::vector<Float64>& designHaunches (designClHaunches[iSeg] );
         designHaunches.reserve(vPoi.size());

         // Save minimum CL haunch and its index location. We will perform the fillet (crossSlope) analysis there since this should
         // be where the fillet most likely will control (that's our assumption)
         Float64 minClHaunch(Float64_Max);
         std::size_t idx(0), minClHaunchIdx(0);
         for (auto poi : vPoi)
         {
            Float64 xpoi = pPoi->ConvertPoiToGirderlineCoordinate(poi);

            // CL haunch depth we computed in our Init function
            Float64 currHaunch = rLayout.m_pHaunchDepths->Evaluate(xpoi);

            // roadway elevation
            Float64 station,offset;
            pBridge->GetStationAndOffset(poi,&station,&offset);
            Float64 rwelev = pRoadway->GetElevation(station,offset);

            // Current finished deck
            Float64 lftHaunch,clHaunch,rgtHaunch;
            Float64 clFinishedElev = pDeformedGirderGeometry->GetFinishedElevation(poi,gceIntervalIdx,&lftHaunch,&clHaunch,&rgtHaunch);

            Float64 designHaunch = currHaunch + (rwelev - clFinishedElev);

            if (designHaunch < minClHaunch)
            {
               minClHaunch = designHaunch;
               minClHaunchIdx = idx;
            }

            designHaunches.push_back(designHaunch);

            idx++;
         }

         // We now have CL haunch design values. However, it's possible that the new values are excessive, or will not pass the fillet check
         // because of cross slope and girder orientation effects. Get relative haunch depths due to cross slope at control location
         const pgsPointOfInterest& rPoiCtrl = vPoi[minClHaunchIdx];

         // Left and right haunch depths from function below account for both cross slope and girder orientation.
         Float64 lftHaunch,clHaunch,rgtHaunch;
         Float64 clFinishedElev = pDeformedGirderGeometry->GetFinishedElevation(rPoiCtrl,gceIntervalIdx,&lftHaunch,&clHaunch,&rgtHaunch);

         Float64 crossSlopeEffectLeft = clHaunch - lftHaunch;
         Float64 crossSlopeEffectRight = clHaunch - rgtHaunch;

         // Use slope effect that reduces haunch depth the most at edge
         Float64 slopeEffect(0.0);
         if (crossSlopeEffectLeft > 0.0 || crossSlopeEffectRight > 0.0)
         {
            slopeEffect = max(crossSlopeEffectLeft,crossSlopeEffectRight);
         }

         // Add slope affects with nominal fillet to determine minimum cl haunch anywhere along segment.
         Float64 minAllowCLHaunch = nominalFillet + slopeEffect;

         Float64 filletHaunchAdjustment = minAllowCLHaunch - minClHaunch;

         const CGirderGroupData* pGroup = rDesignBridgeDescr.GetGirderGroup(segmentKey.groupIndex);
         const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
         const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();

         // Optional min haunch check at CL bearings
         Float64 bearingHaunchAdjustment(0.0);
         Float64 minBearingHaunch;
         if (pGirderEntry->GetMinHaunchAtBearingLines(&minBearingHaunch))
         {
            // Add a buffer to min haunch value. This is because we don't get the exact CL bearing locations from GetPoi... below and the design can often miss by a smidge
            minBearingHaunch += WBFL::Units::ConvertToSysUnits(0.125,WBFL::Units::Measure::Inch);

            PoiList vSupportPois;
            pPoi->GetPointsOfInterest(segmentKey,POI_FACEOFSUPPORT,&vSupportPois);

            // check at all CL suport locations (this is an approximation, bearings might be slightly offset)
            for (const auto& poi : vSupportPois)
            {
               Float64 xpoi = pPoi->ConvertPoiToGirderlineCoordinate(poi);

               // CL haunch depth we computed in our Init function
               Float64 currHaunch = rLayout.m_pHaunchDepths->Evaluate(xpoi);

               if (currHaunch < minBearingHaunch)
               {
                  bearingHaunchAdjustment = max(bearingHaunchAdjustment, minBearingHaunch-currHaunch);
               }
            }
         }

         Float64 haunchSegAdjustment = max(filletHaunchAdjustment,bearingHaunchAdjustment);

         // Controlling adjustment along entire group
         maxHaunchAdjustment4Group = max(maxHaunchAdjustment4Group, haunchSegAdjustment);
      } // segments

      // Now we have unadjusted CL design haunch values. Adjust these to match haunch depths to fillet + buffer
      for (SegmentIndexType iSeg = 0; iSeg < numSegs; iSeg++)
      {
         std::vector<Float64>& segHaunches (designClHaunches[iSeg]);

         // Don't set if the segment wasn't designed. This will be the case when the number of girders is different, and we are in a group with girderlines that don't actually exist.
         if (segHaunches.size() > 0)
         {
            // Add slope adjustment to all haunch values
            std::for_each(segHaunches.begin(),segHaunches.end(),[maxHaunchAdjustment4Group](Float64& d) { d += maxHaunchAdjustment4Group; });

            // We have our design. Now set segment values
            CGirderGroupData* pGroup = rDesignBridgeDescr.GetGirderGroup(iGrp);
            if (bApply2AllGdrs)
            {
               // Set for all girders in group
               GirderIndexType numGdrs = pGroup->GetGirderCount();
               for (GirderIndexType gdrIdx = 0; gdrIdx < numGdrs; gdrIdx++)
               {
                  CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
                  CPrecastSegmentData* pSegment = pGirder->GetSegment(iSeg);
                  pSegment->SetDirectHaunchDepths(segHaunches);
               }
            }
            else
            {
               // set only for our design girderline
               CSplicedGirderData* pGirder = pGroup->GetGirder(rDesignGirderKey.girderIndex);
               CPrecastSegmentData* pSegment = pGirder->GetSegment(iSeg);
               pSegment->SetDirectHaunchDepths(segHaunches);
            }
         }
      }
   } // groups

   // simplify input if we can
   if (bApply2AllGdrs && rDesignGirderKey.groupIndex == ALL_GROUPS)
   {
      rDesignBridgeDescr.SetHaunchInputLocationType(pgsTypes::hilSame4AllGirders);
   }

   // Just finished a design at 1/10th points along segments. We may need to convert
   // to a different user-specified format. If so, we'll need another instance of ourself to do it.
   GET_IFACE(IDocumentType,pDocType);
   bool bIsPGSuper = pDocType->IsPGSuperDocument();
   if (bIsPGSuper || inputDistributionType != pgsTypes::hidTenthPoints)
   {
      // PGSuper only does haunch layouts by span
      pgsTypes::HaunchLayoutType haunchLayoutType = bIsPGSuper ? pgsTypes::hltAlongSpans : pgsTypes::hltAlongSegments;
      HaunchDepthInputConversionTool newTool(&rDesignBridgeDescr,m_pBroker,false);
      auto newPair = newTool.ConvertToDirectHaunchInput(rDesignBridgeDescr.GetHaunchInputLocationType(),haunchLayoutType,inputDistributionType,true);

      // Copy converted bridgedesc to our return value
      rDesignBridgeDescr = newPair.second;
   }

   return convertedBridgeDescrPair;
}

void HaunchDepthInputConversionTool::InitializeGeometrics(bool bSingleGirderLineOnly)
{
   if (!m_WasGeometricsInitialized)
   {
      GET_IFACE(IBridge,pBridge);
      GET_IFACE_NOCHECK(IPointOfInterest,pPoi);

      m_GirderlineHaunchLayouts.clear();

      // Build data structures with span and segment end layouts in girderline coord's
      GirderIndexType maxGirders = bSingleGirderLineOnly ? 1 : m_pBridgeDescr->GetMaxGirderCount();

      m_GirderlineHaunchLayouts.assign(maxGirders,GirderlineHaunchLayout());

      GroupIndexType groupCnt = m_pBridgeDescr->GetGirderGroupCount();

      // Segment ends
      for (GirderIndexType gdrIdx = 0; gdrIdx < maxGirders; gdrIdx++)
      {
         GirderlineHaunchLayout& rLayout = m_GirderlineHaunchLayouts[gdrIdx];
         for (GroupIndexType groupIdx = 0; groupIdx < groupCnt; groupIdx++)
         {
            const CGirderGroupData* pGroup = m_pBridgeDescr->GetGirderGroup(groupIdx);

            GirderIndexType locGdrIdx = min(gdrIdx,pGroup->GetGirderCount()-1); // don't let girders overflow

            const CSplicedGirderData* pGirder = pGroup->GetGirder(locGdrIdx);
            SegmentIndexType numSegs = pGirder->GetSegmentCount();
            for (SegmentIndexType segmentIdx = 0; segmentIdx < numSegs; segmentIdx++)
            {
               const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentIdx);
               CSegmentKey segmentKey(groupIdx,gdrIdx,segmentIdx);
               Float64 segLen = pBridge->GetSegmentLength(segmentKey);

               pgsPointOfInterest poiStart(segmentKey,0.0);
               Float64 xStart = pPoi->ConvertPoiToGirderlineCoordinate(poiStart);

               pgsPointOfInterest poiEnd(segmentKey,segLen);
               Float64 xEnd = pPoi->ConvertPoiToGirderlineCoordinate(poiEnd);

               rLayout.m_SegmentEnds.push_back( std::make_pair(xStart,xEnd) );
            }
         }
      }

      // Span ends
      SpanIndexType numSpans = m_pBridgeDescr->GetSpanCount();
      for (GirderIndexType gdrIdx = 0; gdrIdx < maxGirders; gdrIdx++)
      {
         GirderlineHaunchLayout& rLayout = m_GirderlineHaunchLayouts[gdrIdx];

         for (SpanIndexType spanIdx = 0; spanIdx < numSpans; spanIdx++)
         {
            GirderIndexType locGdrIdx = min(gdrIdx, pBridge->GetGirderCountBySpan(spanIdx)-1); // don't let gdridx overflow

            CSpanKey spanKey(spanIdx,locGdrIdx);
            Float64 spanLength = pBridge->GetSpanLength(spanKey); // this is cl pier to cl pier

            const CSpanData2* pSpan = m_pBridgeDescr->GetSpan(spanIdx);
            pgsPointOfInterest startPoi = pPoi->ConvertSpanPointToPoi(spanKey,0.0);
            Float64 XstartGirderline = pPoi->ConvertPoiToGirderlineCoordinate(startPoi);

            pgsPointOfInterest endPoi = pPoi->ConvertSpanPointToPoi(spanKey,spanLength);
            Float64 XendGirderline = pPoi->ConvertPoiToGirderlineCoordinate(endPoi);

            rLayout.m_SpanEnds.push_back( std::make_pair(XstartGirderline,XendGirderline) );
         }
      }

      // Now we can layout haunch depths along girderline in GL coordinates. The conversion process will use these for our new layout
      if (m_pBridgeDescr->GetHaunchInputDepthType() == pgsTypes::hidACamber)
      {
         GET_IFACE_NOCHECK(IGirder,pIGirder);

         for (GirderIndexType gdrIdx = 0; gdrIdx < maxGirders; gdrIdx++)
         {
            GirderlineHaunchLayout& rLayout = m_GirderlineHaunchLayouts[gdrIdx];

            // Layout haunch depth at 1/10 points along segment using piecewise linear approx
            std::shared_ptr< WBFL::Math::PiecewiseFunction> pPwFunc = std::make_shared<WBFL::Math::PiecewiseFunction>();
            rLayout.m_pHaunchDepths = pPwFunc;

            for (GroupIndexType groupIdx = 0; groupIdx < groupCnt; groupIdx++)
            {
               const CGirderGroupData* pGroup = m_pBridgeDescr->GetGirderGroup(groupIdx);

               GirderIndexType locGdrIdx = min(gdrIdx,pGroup->GetGirderCount() - 1); // don't let girders overflow

               const CSplicedGirderData* pGirder = pGroup->GetGirder(locGdrIdx);

               SegmentIndexType numSegs = pGirder->GetSegmentCount();
               for (SegmentIndexType segmentIdx = 0; segmentIdx < numSegs; segmentIdx++)
               {
                  const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentIdx);
                  const CSegmentKey& segmentKey = pSegment->GetSegmentKey();

                  Float64 segStartLoc = rLayout.m_SegmentEnds[segmentIdx].first;
                  Float64 segEndLoc = rLayout.m_SegmentEnds[segmentIdx].second;
                  Float64 segLength = segEndLoc - segStartLoc;

                  Float64 segmentStartEndDist = pBridge->GetSegmentStartEndDistance(segmentKey);
                  Float64 segmentEndEndDist = pBridge->GetSegmentEndEndDistance(segmentKey);

                  Float64 endBrgLoc = segLength - segmentEndEndDist;

                  Float64 AStart,AEnd;
                  pSegment->GetSlabOffset(&AStart,&AEnd);

                  pgsPointOfInterest poiStart(segmentKey,0.0), poiEnd(segmentKey,segLength);
                  Float64 slabDepthStart = pBridge->GetGrossSlabDepth(poiStart);
                  Float64 slabDepthEnd = pBridge->GetGrossSlabDepth(poiEnd);

                  Float64 haunchStart = AStart - slabDepthStart;
                  Float64 haunchEnd = AEnd - slabDepthEnd;

                  for (int iLoc = 0; iLoc <= 10; iLoc++)
                  {
                     Float64 segLoc = segLength * (Float64)iLoc / 10.0;
                     Float64 haunch = LinInterpLine(segmentStartEndDist,haunchStart,endBrgLoc,haunchEnd,segLoc);

                     Float64 segLocGirderLine = segLoc + segStartLoc;

                     pPwFunc->AddPoint(segLocGirderLine,haunch);
                  }
               }
            }
         }
      }
      else
      {
         // Haunch is defined by direct input
         if (m_pBridgeDescr->GetHaunchLayoutType() == pgsTypes::hltAlongSegments)
         {
            for (GirderIndexType gdrIdx = 0; gdrIdx < maxGirders; gdrIdx++)
            {
               GirderlineHaunchLayout& rLayout = m_GirderlineHaunchLayouts[gdrIdx];

               // Layout haunch depth at 1/10 points along segment using piecewise linear approx
               std::shared_ptr< WBFL::Math::PiecewiseFunction> pPwFunc = std::make_shared<WBFL::Math::PiecewiseFunction>();
               rLayout.m_pHaunchDepths = pPwFunc;

               std::size_t segVecIdx(0); // index into m_SegmentEnds vector, which crosses groups
               for (GroupIndexType groupIdx = 0; groupIdx < groupCnt; groupIdx++)
               {
                  const CGirderGroupData* pGroup = m_pBridgeDescr->GetGirderGroup(groupIdx);

                  GirderIndexType locGdrIdx = min(gdrIdx,pGroup->GetGirderCount() - 1); // don't let girders overflow

                  const CSplicedGirderData* pGirder = pGroup->GetGirder(locGdrIdx);
                  SegmentIndexType numSegs = pGirder->GetSegmentCount();
                  for (SegmentIndexType segmentIdx = 0; segmentIdx < numSegs; segmentIdx++)
                  {
                     const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentIdx);
                     const CSegmentKey& segmentKey = pSegment->GetSegmentKey();

                     Float64 segStartLoc = rLayout.m_SegmentEnds[segVecIdx].first;
                     Float64 segEndLoc = rLayout.m_SegmentEnds[segVecIdx].second;
                     Float64 segLength = segEndLoc - segStartLoc;

                     std::vector<Float64> segmentHaunches = pSegment->GetDirectHaunchDepths();

                     for (int iLoc = 0; iLoc <= 10; iLoc++)
                     {
                        Float64 segLoc = segLength * (Float64)iLoc / 10.0;
                        Float64 segLocGirderLine = segLoc + segStartLoc;

                        Float64 haunch = ::ComputeHaunchDepthAlongSegment(segLoc, segLength,segmentHaunches);

                        pPwFunc->AddPoint(segLocGirderLine,haunch);
                     }

                     segVecIdx++;
                  }
               }
            }
         }
         else
         {
            // layout is by spans
            pgsTypes::HaunchInputLocationType hlt = m_pBridgeDescr->GetHaunchInputLocationType();

            for (GirderIndexType gdrIdx = 0; gdrIdx < maxGirders; gdrIdx++)
            {
               GirderlineHaunchLayout& rLayout = m_GirderlineHaunchLayouts[gdrIdx];

               // Layout haunch depth at 1/20 points along span using piecewise linear approx
               std::shared_ptr< WBFL::Math::PiecewiseFunction> pPwFunc = std::make_shared<WBFL::Math::PiecewiseFunction>();
               rLayout.m_pHaunchDepths = pPwFunc;

               SpanIndexType numSpans = m_pBridgeDescr->GetSpanCount();
               for (SpanIndexType spanIdx = 0; spanIdx < numSpans; spanIdx++)
               {
                  const CSpanData2* pSpan = m_pBridgeDescr->GetSpan(spanIdx);

                  GirderIndexType locGdrIdx = min(gdrIdx,pSpan->GetGirderCount() - 1); // don't let girders overflow

                  GirderIndexType sourceGdrIdx = hlt == pgsTypes::hilPerEach ? locGdrIdx : 0;

                  std::vector<Float64> spanHaunches = pSpan->GetDirectHaunchDepths(sourceGdrIdx);

                  Float64 spanStartLoc = rLayout.m_SpanEnds[spanIdx].first;
                  Float64 spanEndLoc = rLayout.m_SpanEnds[spanIdx].second;
                  Float64 spanLength = spanEndLoc - spanStartLoc;

                  const int numparts = 20;
                  for (int iLoc = 0; iLoc <= numparts; iLoc++)
                  {
                     Float64 spanLoc = spanLength * (Float64)iLoc / (Float64)numparts;
                     Float64 spanLocGirderLine = spanLoc + spanStartLoc;

                     Float64 haunch = ::ComputeHaunchDepthAlongSegment(spanLoc,spanLength,spanHaunches);

                     pPwFunc->AddPoint(spanLocGirderLine,haunch);
                  }
               }
            }
         }
      }

      m_WasGeometricsInitialized = true;
   }
}

bool HaunchDepthInputConversionTool::CondenseDirectHaunchInput(CBridgeDescription2* pBridgeDescr)
{
   if (pBridgeDescr->GetHaunchInputDepthType() == pgsTypes::hidACamber)
   {
      ATLASSERT(0); // shouldn't be here
      return false;
   }

   bool retVal = false;

   if (pgsTypes::hilSame4Bridge == pBridgeDescr->GetHaunchInputLocationType())
   {
      // nothing we can do
      return false;
   }
   else
   {
      if (pgsTypes::hltAlongSpans == pBridgeDescr->GetHaunchLayoutType())
      {
         SpanIndexType spanCnt = pBridgeDescr->GetSpanCount();

         if (pgsTypes::hilPerEach == pBridgeDescr->GetHaunchInputLocationType())
         {
            bool isSame = true;
            for (SpanIndexType iSpan = 0; iSpan < spanCnt; iSpan++)
            {
               CSpanData2* pSpan = pBridgeDescr->GetSpan(iSpan);
               std::vector<Float64> gdr1haunches = pSpan->GetDirectHaunchDepths(0,true);

               GirderIndexType girderCnt = pSpan->GetGirderCount();
               for (GirderIndexType iGirder = 1; iGirder < girderCnt; iGirder++)
               {
                  std::vector<Float64> haunches = pSpan->GetDirectHaunchDepths(iGirder,true);
                  if (!CompareFloatVectors(haunches,gdr1haunches))
                  {
                     isSame = false;
                     break;
                  }
               }

               if (!isSame)
               {
                  break;
               }
            }

            if (isSame)
            {
               // haunches are same in every span
               pBridgeDescr->SetHaunchInputLocationType(pgsTypes::hilSame4AllGirders);
               retVal = true;
            }
         }

         if (pgsTypes::hilPerEach == pBridgeDescr->GetHaunchInputLocationType())
         {
            bool isSame = true;

            CSpanData2* pSpan1 = pBridgeDescr->GetSpan(0);
            std::vector<Float64> span1haunches = pSpan1->GetDirectHaunchDepths(0,true);

            for (SpanIndexType iSpan = 1; iSpan < spanCnt; iSpan++)
            {
               CSpanData2* pSpan = pBridgeDescr->GetSpan(iSpan);
               std::vector<Float64> haunches = pSpan->GetDirectHaunchDepths(0,true);

               if (!CompareFloatVectors(haunches, span1haunches))
               {
                  isSame = false;
                  break;
               }
            }

            if (isSame)
            {
               // haunches are same throughout bridge
               pBridgeDescr->SetHaunchInputLocationType(pgsTypes::hilSame4Bridge);
               pBridgeDescr->SetDirectHaunchDepths(span1haunches);
               retVal = true;
            }
         }
      }
      else
      {
         // Haunches are at segments
         GroupIndexType groupCnt = pBridgeDescr->GetGirderGroupCount();

         if (pgsTypes::hilPerEach == pBridgeDescr->GetHaunchInputLocationType())
         {
            bool isSame = true;

            for (GroupIndexType iGroup = 0; iGroup < groupCnt; iGroup++)
            {
               const CGirderGroupData* pGroup = pBridgeDescr->GetGirderGroup(iGroup);
               const CSplicedGirderData* pGirder = pGroup->GetGirder(0); // assumes all girders in a group have same number of segments
               SegmentIndexType segmentCnt = pGirder->GetSegmentCount();

               for (SegmentIndexType iSeg = 0; iSeg < segmentCnt; iSeg++)
               {
                  GirderIndexType girderCnt = pGroup->GetGirderCount();
                  std::vector<Float64> gdr1Haunchs;
                  for (GirderIndexType iGirder = 0; iGirder < girderCnt; iGirder++)
                  {
                     const CSplicedGirderData* pGirder = pGroup->GetGirder(iGirder);
                     if (iGirder == 0)
                     {
                        gdr1Haunchs = pGirder->GetDirectHaunchDepths(iSeg,true);
                     }
                     else
                     {
                        std::vector<Float64> haunches = pGirder->GetDirectHaunchDepths(iSeg,true);
                        if (!CompareFloatVectors(haunches, gdr1Haunchs))
                        {
                           isSame = false;
                           break;
                        }
                     }
                  }
               }
            }

            if (isSame)
            {
               // haunches are same in every segmentidx
               pBridgeDescr->SetHaunchInputLocationType(pgsTypes::hilSame4AllGirders);
               retVal = true;
            }
         }

         if (pgsTypes::hilSame4AllGirders == pBridgeDescr->GetHaunchInputLocationType())
         {
            bool isSame = true;

            std::vector<Float64> seg1Haunches = pBridgeDescr->GetGirderGroup((GroupIndexType)0)->GetGirder(0)->GetDirectHaunchDepths(0,true);
            for (GroupIndexType iGroup = 0; iGroup < groupCnt; iGroup++)
            {
               const CGirderGroupData* pGroup = pBridgeDescr->GetGirderGroup(iGroup);
               const CSplicedGirderData* pGirder1 = pGroup->GetGirder(0); // assumes all girders in a group have same number of segments
               SegmentIndexType segmentCnt = pGirder1->GetSegmentCount();

               for (SegmentIndexType iSeg = 0; iSeg < segmentCnt; iSeg++)
               {
                  if (iSeg == 0 && iGroup==0)
                  {
                     seg1Haunches = pGirder1->GetDirectHaunchDepths(iSeg,true);
                  }
                  else
                  {
                     std::vector<Float64> haunches = pGirder1->GetDirectHaunchDepths(iSeg,true);
                     if (!CompareFloatVectors(haunches,seg1Haunches))
                     {
                        isSame = false;
                        break;
                     }
                  }
               }
            }

            if (isSame)
            {
               // haunches are same throughout bridge
               pBridgeDescr->SetHaunchInputLocationType(pgsTypes::hilSame4Bridge);
               pBridgeDescr->SetDirectHaunchDepths(seg1Haunches);
               retVal = true;
            }
         }
      }
   }

   return retVal;
}
