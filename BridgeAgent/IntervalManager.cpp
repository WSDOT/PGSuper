///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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

#include "stdafx.h"
#include "IntervalManager.h"

#include <EAF\EAFUtilities.h>
#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderLabel.h>

#include <PgsExt\DistributedLoadData.h>
#include <PgsExt\MomentLoadData.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if defined ASSERT_VALID
#undef ASSERT_VALID
#endif

#if defined _DEBUG
#define ASSERT_VALID AssertValid()
#else
#define ASSERT_VALID
#endif


void CIntervalManager::BuildIntervals(const CTimelineManager* pTimelineMgr,bool bTimeStepMethod)
{
   // this method builds the analysis interval sequence for each girder as well as defines
   // the stage model for the generic bridge model. there is one analysis interval sequence
   // for each girder and only one stage sequence for the generic bridge model.
   m_bTimeStepMethod = bTimeStepMethod;

   // reset everything
   m_CastDeckInterval.clear();
   m_CompositeDeckInterval.clear();
   m_LiveLoadInterval.clear();
   m_OverlayInterval.clear();
   m_RailingSystemInterval.clear();;

#pragma Reminder("UPDATE: need zero duration interval for installation of temporary strands if they are post-tensioned")
   // also need generic TemporaryStrandInstallationInterval method

   m_StressStrandIntervals.clear();
   m_ReleaseIntervals.clear();

   m_IntervalSequences.clear();
   m_StageMap.clear();
   m_IntervalMap.clear();
   m_TempSupportRemovalIntervals.clear();
   m_SegmentErectionIntervals.clear();
   m_SegmentErectionSequenceIntervalLimits.clear();
   m_StrandStressingSequenceIntervalLimits.clear();
   m_ReleaseSequenceIntervalLimits.clear();
   m_RemoveTemporaryStrandsIntervals.clear();

   m_StressTendonIntervals.clear();

   StageIndexType stageIdx = 0;

   // initialize the interval sequences.. one per girder
   // start off with an empty vector mapped to the girder key. 
   // the vectors will be filled with interval indicies as this function progresses
   const CBridgeDescription2* pBridgeDesc = pTimelineMgr->GetBridgeDescription();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         CGirderKey girderKey(grpIdx,gdrIdx);
         m_IntervalSequences.insert(std::make_pair(girderKey,std::vector<CInterval>()));
      }
   }

   // work through all the events in the timeline and build analysis intervals and modeling stages
   EventIndexType nEvents = pTimelineMgr->GetEventCount();
   for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);

      if ( pTimelineEvent->GetConstructSegmentsActivity().IsEnabled() )
      {
         // Precast segments are constructed during this activity

         // Model this with 3 intervals... stress strands, release prestress, place into storage
         Float64 start = pTimelineEvent->GetDay();
         Float64 duration = pTimelineEvent->GetConstructSegmentsActivity().GetRelaxationTime();

         // stress strands during segment construction. strands relax during this interval
         CInterval stressStrandInterval;
         stressStrandInterval.StartEventIdx = eventIdx;
         stressStrandInterval.EndEventIdx   = eventIdx;
         stressStrandInterval.Start = start;
         stressStrandInterval.Duration = duration;
         stressStrandInterval.End = start + duration;
         stressStrandInterval.Middle = 0.5*(stressStrandInterval.Start + stressStrandInterval.End);
         stressStrandInterval.Description = _T("Tension strand and cast girder segment");

         // release prestress is a sudden loading.... zero length interval
         CInterval releaseInterval(stressStrandInterval);
         releaseInterval.Start = stressStrandInterval.End;
         releaseInterval.Duration = 0;
         releaseInterval.Middle = releaseInterval.Start;
         releaseInterval.Description = _T("Prestress Release");

         // lift segment
         CInterval liftSegmentInterval(releaseInterval);
         liftSegmentInterval.Start = releaseInterval.End;
         liftSegmentInterval.Duration = 0;
         liftSegmentInterval.Middle = liftSegmentInterval.Start;
         liftSegmentInterval.Description = _T("Lift segments");

         // placing into storage changes boundary conditions... treat as sudden change in loading
         CInterval storageInterval(liftSegmentInterval);
         storageInterval.Start = liftSegmentInterval.End;
         storageInterval.Duration = 0;
         storageInterval.Middle = storageInterval.Start;
         storageInterval.Description = _T("Place segments into storage");

         // get a mapping of every girder and its segments that are constructed during this activity.
         std::map<CGirderKey,std::vector<CSegmentKey>> girderKeys;
         const std::set<SegmentIDType>& segments = pTimelineEvent->GetConstructSegmentsActivity().GetSegments();
         std::set<SegmentIDType>::const_iterator segIter(segments.begin());
         std::set<SegmentIDType>::const_iterator segIterEnd(segments.end());
         for ( ; segIter != segIterEnd; segIter++ )
         {
            SegmentIDType segmentID = *segIter;
            const CPrecastSegmentData* pSegment = pBridgeDesc->FindSegment(segmentID);
            CSegmentKey segmentKey(pSegment->GetSegmentKey());
            std::map<CGirderKey,std::vector<CSegmentKey>>::iterator found(girderKeys.find(segmentKey));
            if ( found == girderKeys.end() )
            {
               // this is the first segment for this girder... create the segment list
               // and add it to the map
               std::vector<CSegmentKey> segmentKeys;
               segmentKeys.push_back(segmentKey);
               girderKeys.insert(std::make_pair(segmentKey,segmentKeys));
            }
            else
            {
               // segments for this girder have already been encountered...
               // add this segment to the list for this girder
               std::vector<CSegmentKey>& segmentKeys(found->second);
               segmentKeys.push_back(segmentKey);
            }
         }

         // update the interval sequences for each girder that has segments constructed during this activity
         // and update the access strand stress and release maps for each segment
         std::map<CGirderKey,std::vector<CSegmentKey>>::iterator gdrIter(girderKeys.begin());
         std::map<CGirderKey,std::vector<CSegmentKey>>::iterator gdrIterEnd(girderKeys.end());
         for ( ; gdrIter != gdrIterEnd; gdrIter++ )
         {
            const CGirderKey& girderKey(gdrIter->first);
            std::vector<CSegmentKey>& segments(gdrIter->second);

            std::map<CGirderKey,std::vector<CInterval>>::iterator found(m_IntervalSequences.find(girderKey));
            ATLASSERT( found != m_IntervalSequences.end());
            std::vector<CInterval>& vIntervals(found->second);

            // use one analysis interval each for all segments that have strands stressed, released,
            // and have been placed into storage during this activity
            vIntervals.push_back(stressStrandInterval);
            IntervalIndexType stressStrandIntervalIdx = vIntervals.size()-1;
            AddToStageMap(girderKey,stressStrandIntervalIdx,stageIdx++);

            vIntervals.push_back(releaseInterval);
            IntervalIndexType releaseIntervalIdx = vIntervals.size()-1;
            AddToStageMap(girderKey,releaseIntervalIdx,stageIdx++);

            vIntervals.push_back(liftSegmentInterval);
            IntervalIndexType liftSegmentIntervalIdx = vIntervals.size()-1;
            AddToStageMap(girderKey,liftSegmentIntervalIdx,stageIdx++);

            vIntervals.push_back(storageInterval);
            IntervalIndexType storageIntervalIdx = vIntervals.size()-1;
            AddToStageMap(girderKey,storageIntervalIdx,stageIdx++);

            // store the intervals when strands are stressed and release for a segment
            // this makes look up easier when we want this information
            std::vector<CSegmentKey>::iterator segIter(segments.begin());
            std::vector<CSegmentKey>::iterator segIterEnd(segments.end());
            for ( ; segIter != segIterEnd; segIter++ )
            {
               CSegmentKey& segmentKey(*segIter);
               m_StressStrandIntervals.insert(std::make_pair(segmentKey,stressStrandIntervalIdx));
               m_ReleaseIntervals.insert(std::make_pair(segmentKey,releaseIntervalIdx));
            } // next segment

            // this is for keeping track of when the strands are stressed for the first and last segments constructed for this girder
            std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::iterator strandStressingFound(m_StrandStressingSequenceIntervalLimits.find(girderKey));
            if ( strandStressingFound == m_StrandStressingSequenceIntervalLimits.end() )
            {
               // this is the first segment from the girder have strands stressed
               m_StrandStressingSequenceIntervalLimits.insert(std::make_pair(girderKey,std::make_pair(stressStrandIntervalIdx,stressStrandIntervalIdx)));
            }
            else
            {
               // a segment from this girder has already had its strands stressed.. update the record
               strandStressingFound->second.first  = Min(strandStressingFound->second.first, stressStrandIntervalIdx);
               strandStressingFound->second.second = Max(strandStressingFound->second.second,stressStrandIntervalIdx);
            }

            // this is for keeping track of when the strands are released for the first and last segments constructed for this girder
            std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::iterator releaseFound(m_ReleaseSequenceIntervalLimits.find(girderKey));
            if ( releaseFound == m_ReleaseSequenceIntervalLimits.end() )
            {
               // this is the first segment from the girder to have its strands released
               m_ReleaseSequenceIntervalLimits.insert(std::make_pair(girderKey,std::make_pair(releaseIntervalIdx,releaseIntervalIdx)));
            }
            else
            {
               // a segment from this girder has already had its strands released.. update the record
               releaseFound->second.first  = Min(releaseFound->second.first, releaseIntervalIdx);
               releaseFound->second.second = Max(releaseFound->second.second,releaseIntervalIdx);
            }
         } // next girder
      } // if activity

      if ( pTimelineEvent->GetErectSegmentsActivity().IsEnabled() )
      {
#pragma Reminder("UPDATE: need to use better descriptions if segments are erected in multiple intervals")

         // handling of erect segment activity is similiar to construction.

         // Haul segment to bridge site
         CInterval haulSegmentInterval;
         haulSegmentInterval.StartEventIdx = eventIdx;
         haulSegmentInterval.EndEventIdx   = eventIdx;
         haulSegmentInterval.Start         = pTimelineEvent->GetDay();
         haulSegmentInterval.End           = haulSegmentInterval.Start;
         haulSegmentInterval.Middle        = haulSegmentInterval.Start;
         haulSegmentInterval.Duration      = 0;
         haulSegmentInterval.Description   = _T("Haul segments");

         // this is an abrupt change in boundary condition, treat as sudden change in loading
         CInterval erectSegmentInterval;
         erectSegmentInterval.StartEventIdx = eventIdx;
         erectSegmentInterval.EndEventIdx   = eventIdx;
         erectSegmentInterval.Start         = pTimelineEvent->GetDay();
         erectSegmentInterval.End           = erectSegmentInterval.Start;
         erectSegmentInterval.Middle        = erectSegmentInterval.Start;
         erectSegmentInterval.Duration      = 0;
         erectSegmentInterval.Description   = _T("Erect segments");


         std::map<CGirderKey,std::vector<CSegmentKey>> girderKeys;
         const CErectSegmentActivity& activity = pTimelineEvent->GetErectSegmentsActivity();
         std::set<SegmentIDType> erectedSegments(activity.GetSegments());
         std::set<SegmentIDType>::iterator segIter(erectedSegments.begin());
         std::set<SegmentIDType>::iterator segIterEnd(erectedSegments.end());
         for ( ; segIter != segIterEnd; segIter++ )
         {
            SegmentIDType segmentID = *segIter;
            const CPrecastSegmentData* pSegment = pBridgeDesc->FindSegment(segmentID);
            CSegmentKey segmentKey(pSegment->GetSegmentKey());
            std::map<CGirderKey,std::vector<CSegmentKey>>::iterator found(girderKeys.find(segmentKey));
            if ( found == girderKeys.end() )
            {
               std::vector<CSegmentKey> segmentKeys;
               segmentKeys.push_back(segmentKey);
               girderKeys.insert(std::make_pair(segmentKey,segmentKeys));
            }
            else
            {
               std::vector<CSegmentKey>& segmentKeys(found->second);
               segmentKeys.push_back(segmentKey);
            }
         }

         // update the interval sequences for each girder that has segments constructed during this activity
         // and update the access strand stress and release maps for each segment
         bool bTempStrands = false;
         std::map<CGirderKey,std::vector<CSegmentKey>>::iterator gdrIter(girderKeys.begin());
         std::map<CGirderKey,std::vector<CSegmentKey>>::iterator gdrIterEnd(girderKeys.end());
         for ( ; gdrIter != gdrIterEnd; gdrIter++ )
         {
            const CGirderKey& girderKey(gdrIter->first);
            std::vector<CSegmentKey>& segments(gdrIter->second);

            std::map<CGirderKey,std::vector<CInterval>>::iterator found(m_IntervalSequences.find(girderKey));
            ATLASSERT( found != m_IntervalSequences.end());
            std::vector<CInterval>& vIntervals(found->second);

            vIntervals.push_back(haulSegmentInterval);
            IntervalIndexType haulSegmentIntervalIdx = vIntervals.size()-1;
            AddToStageMap(girderKey,haulSegmentIntervalIdx,stageIdx++);

            vIntervals.push_back(erectSegmentInterval);
            IntervalIndexType erectSegmentIntervalIdx = vIntervals.size()-1;
            AddToStageMap(girderKey,erectSegmentIntervalIdx,stageIdx++);

            // keep track of when segments are erected. also keep track of when the first
            // and last segment of a girder are erected. this make looking up this information easier
            std::vector<CSegmentKey>::iterator segIter(segments.begin());
            std::vector<CSegmentKey>::iterator segIterEnd(segments.end());
            for ( ; segIter != segIterEnd; segIter++ )
            {
               // this is when a given segment is erected
               CSegmentKey& segmentKey(*segIter);
               m_SegmentErectionIntervals.insert(std::make_pair(segmentKey,erectSegmentIntervalIdx));

               // this is for keeping track of when the first and last segments in a girder are erected
               std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::iterator found(m_SegmentErectionSequenceIntervalLimits.find(girderKey));
               if ( found == m_SegmentErectionSequenceIntervalLimits.end() )
               {
                  // this is the first segment from the girder to be erected
                  m_SegmentErectionSequenceIntervalLimits.insert(std::make_pair(girderKey,std::make_pair(erectSegmentIntervalIdx,erectSegmentIntervalIdx)));
               }
               else
               {
                  // a segment from this girder has already been erected.. update the record
                  found->second.first  = Min(found->second.first, erectSegmentIntervalIdx);
                  found->second.second = Max(found->second.second,erectSegmentIntervalIdx);
               }

               // determine if there are any segments with temporary strands that are erected
               // during this activity. analysis intervals for temporary strand removal occur
               // after all segments in this activity have been erected. since all segments
               // erected during this activity are being processed, it is easy to determine if
               // analysis intervals for temporary strand removal need to be created later
               const CPrecastSegmentData* pSegment = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetSegment(segmentKey.segmentIndex);
               ATLASSERT(pSegment);
               if ( 0 < pSegment->Strands.GetStrandCount(pgsTypes::Temporary) )
               {
                  bTempStrands = true;
               }
            } // next segment
         } // next gdr

         if ( bTempStrands )
         {
            // at least one segment that is erected in this activity has temporary strands

            // remove temporary strands
            CInterval removeTempStrandInterval;
            removeTempStrandInterval.StartEventIdx = eventIdx;
            removeTempStrandInterval.EndEventIdx   = eventIdx;
            removeTempStrandInterval.Start         = pTimelineEvent->GetDay();
            removeTempStrandInterval.End           = removeTempStrandInterval.Start;
            removeTempStrandInterval.Middle        = removeTempStrandInterval.Start;
            removeTempStrandInterval.Duration      = 0;
            removeTempStrandInterval.Description   = _T("Remove temporary strands");

            // temporary strands are assumed to be removed at the same time
            // so there is only one stage in the generic bridge model for this case
            // (per erect segment activity)
            StageIndexType removeTempStrandStageIdx = stageIdx++;
            std::map<CGirderKey,std::vector<CSegmentKey>>::iterator gdrIter(girderKeys.begin());
            std::map<CGirderKey,std::vector<CSegmentKey>>::iterator gdrIterEnd(girderKeys.end());
            for ( ; gdrIter != gdrIterEnd; gdrIter++ )
            {
               const CGirderKey& girderKey(gdrIter->first);
               std::vector<CSegmentKey>& segments(gdrIter->second);

               std::map<CGirderKey,std::vector<CInterval>>::iterator found(m_IntervalSequences.find(girderKey));
               ATLASSERT( found != m_IntervalSequences.end());
               std::vector<CInterval>& vIntervals(found->second);

               IntervalIndexType removeTempStrandsIntervalIdx = vIntervals.size(); // this will be the interval
               // if there are temporary strands to remove

               bool bRemoveTempStrands = false; // do any segments in this girder, that are erected during this activity, have temporary strands?
               std::vector<CSegmentKey>::iterator segIter(segments.begin());
               std::vector<CSegmentKey>::iterator segIterEnd(segments.end());
               for ( ; segIter != segIterEnd; segIter++ )
               {
                  CSegmentKey& segmentKey(*segIter);

                  const CPrecastSegmentData* pSegment = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex)->GetGirder(segmentKey.girderIndex)->GetSegment(segmentKey.segmentIndex);
                  ATLASSERT(pSegment);
                  if ( 0 < pSegment->Strands.GetStrandCount(pgsTypes::Temporary) )
                  {
                     bRemoveTempStrands = true;
                     m_RemoveTemporaryStrandsIntervals.insert(std::make_pair(segmentKey,removeTempStrandsIntervalIdx));
                  }
               } // next segment

               if ( bRemoveTempStrands )
               {
                  // at least one segment in this girder, that is erected during this activity, has temporary
                  // strands. create a temporary strand removal analysis interval. this analysis interval
                  // applies to all segments, for this activity, in this girder that have temporary strands
                  vIntervals.push_back(removeTempStrandInterval);
                  ATLASSERT(removeTempStrandsIntervalIdx == vIntervals.size()-1);
                  AddToStageMap(girderKey,removeTempStrandsIntervalIdx,removeTempStrandStageIdx);
               }
            } // next gdr
         }
      } // end of segment erection

      if ( pTimelineEvent->GetCastClosureJointActivity().IsEnabled() && pTimelineEvent->GetCastDeckActivity().IsEnabled() )
      {
         // closure joints and deck are cast at the same time

         CInterval castDeckInterval;

         // this is a sudden change in loading
         castDeckInterval.StartEventIdx = eventIdx;
         castDeckInterval.EndEventIdx   = eventIdx;
         castDeckInterval.Start         = pTimelineEvent->GetDay();
         castDeckInterval.End           = castDeckInterval.Start;
         castDeckInterval.Middle        = castDeckInterval.Start;
         castDeckInterval.Duration      = 0;

         if ( 0 < pTimelineEvent->GetCastClosureJointActivity().GetPierCount() || 0 < pTimelineEvent->GetCastClosureJointActivity().GetTemporarySupportCount() )
         {
            castDeckInterval.Description   = _T("Cast closure joints and deck");
         }
         else
         {
            // there aren't piers or temporary supports so it is just deck casting
            castDeckInterval.Description   = _T("Cast deck");
         }

         CInterval cureDeckInterval;
         cureDeckInterval.StartEventIdx = eventIdx;
         cureDeckInterval.EndEventIdx   = eventIdx;
         cureDeckInterval.Start = castDeckInterval.End;
         cureDeckInterval.Duration = Max(pTimelineEvent->GetCastClosureJointActivity().GetConcreteAgeAtContinuity(),pTimelineEvent->GetCastDeckActivity().GetConcreteAgeAtContinuity());
         cureDeckInterval.End = cureDeckInterval.Start + cureDeckInterval.Duration;
         cureDeckInterval.Middle = 0.5*(cureDeckInterval.Start + cureDeckInterval.End);
         cureDeckInterval.Description = _T("Deck curing");

         CInterval compositeDeckInterval;
         compositeDeckInterval.StartEventIdx = eventIdx;
         compositeDeckInterval.EndEventIdx   = eventIdx;
         compositeDeckInterval.Start = cureDeckInterval.End;
         compositeDeckInterval.Duration = 0;
         compositeDeckInterval.End = compositeDeckInterval.Start + compositeDeckInterval.Duration;
         compositeDeckInterval.Middle = 0.5*(compositeDeckInterval.Start + compositeDeckInterval.End);
         compositeDeckInterval.Description = _T("Composite Deck");

         // deck casting applies to all girders
         StageIndexType castDeckStageIdx      = stageIdx++;
         StageIndexType cureDeckStageIdx      = stageIdx++;
         StageIndexType compositeDeckStageIdx = stageIdx++;

         bool bCureDeckStageUsed = true; // if the deck curing stage is not used, we need to decrement stageIdx to compensate for the increment when assign a stage index above

         GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
         for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
         {
            GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
            for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
            {
               CGirderKey girderKey(grpIdx,gdrIdx);
               std::map<CGirderKey,std::vector<CInterval>>::iterator found(m_IntervalSequences.find(girderKey));
               ATLASSERT( found != m_IntervalSequences.end());
               std::vector<CInterval>& vIntervals(found->second);

               vIntervals.push_back(castDeckInterval);
               IntervalIndexType castDeckIntervalIdx = vIntervals.size()-1; // this is the same for all interval sequences
               m_CastDeckInterval.insert(std::make_pair(girderKey,castDeckIntervalIdx));
               AddToStageMap(girderKey,castDeckIntervalIdx,castDeckStageIdx);

               if ( bTimeStepMethod )
               {
                  if ( 0 < cureDeckInterval.Duration )
                  {
                     vIntervals.push_back(cureDeckInterval);
                     IntervalIndexType cureDeckIntervalIdx = vIntervals.size()-1;
                     AddToStageMap(girderKey,cureDeckIntervalIdx,cureDeckStageIdx);

                     vIntervals.push_back(compositeDeckInterval);
                     IntervalIndexType compositeDeckIntervalIdx = vIntervals.size()-1;
                     AddToStageMap(girderKey,compositeDeckIntervalIdx,compositeDeckStageIdx);
                     m_CompositeDeckInterval.insert(std::make_pair(girderKey,compositeDeckIntervalIdx));
                  }
                  else
                  {
                     vIntervals.push_back(compositeDeckInterval);
                     IntervalIndexType compositeDeckIntervalIdx = vIntervals.size()-1;

                     // there is no curing duration so the composite deck state is the curing deck stage
                     AddToStageMap(girderKey,compositeDeckIntervalIdx,cureDeckStageIdx);
                     m_CompositeDeckInterval.insert(std::make_pair(girderKey,compositeDeckIntervalIdx));
                     bCureDeckStageUsed = false;
                  }
               }
               else
               {
                  vIntervals.push_back(compositeDeckInterval);
                  IntervalIndexType compositeDeckIntervalIdx = vIntervals.size()-1;

                  // there is no curing duration so the composite deck stage is the curing deck stage
                  AddToStageMap(girderKey,compositeDeckIntervalIdx,cureDeckStageIdx);
                  m_CompositeDeckInterval.insert(std::make_pair(girderKey,compositeDeckIntervalIdx));
                  bCureDeckStageUsed = false; // not using the cure deck stage... will adjust stageIdx below
               }
            } // next gdr
         } // next group

         if ( bCureDeckStageUsed == false )
         {
            stageIdx--; // didn't use the cure deck stage so back up the stage index by one
         }
      }
      else
      {
         // closure joint and/or deck are cast during different timeline events

         if ( pTimelineEvent->GetCastClosureJointActivity().IsEnabled() )
         {
            // closure joints are cast during this timeline event
            CInterval castClosureInterval;

            // this is an abrupt change in boundary condition, treat as sudden change in loading
            castClosureInterval.StartEventIdx = eventIdx;
            castClosureInterval.EndEventIdx   = eventIdx;
            castClosureInterval.Start         = pTimelineEvent->GetDay();
            castClosureInterval.End           = castClosureInterval.Start;
            castClosureInterval.Middle        = castClosureInterval.Start;
            castClosureInterval.Duration      = 0;
            castClosureInterval.Description   = _T("Cast closure joints");

            CInterval cureClosureInterval;
            cureClosureInterval.StartEventIdx = eventIdx;
            cureClosureInterval.EndEventIdx   = eventIdx;
            cureClosureInterval.Start = castClosureInterval.End;
            cureClosureInterval.Duration = pTimelineEvent->GetCastClosureJointActivity().GetConcreteAgeAtContinuity();
            cureClosureInterval.End = cureClosureInterval.Start + cureClosureInterval.Duration;
            cureClosureInterval.Middle = 0.5*(cureClosureInterval.Start + cureClosureInterval.End);
            cureClosureInterval.Description = _T("Closure joints curing");

            GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
            for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
            {
               GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
               for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
               {
                  CGirderKey girderKey(grpIdx,gdrIdx);
                  std::map<CGirderKey,std::vector<CInterval>>::iterator found(m_IntervalSequences.find(girderKey));
                  ATLASSERT( found != m_IntervalSequences.end());
                  std::vector<CInterval>& vIntervals(found->second);

                  vIntervals.push_back(castClosureInterval);
                  IntervalIndexType castClosureIntervalIdx = vIntervals.size()-1;
                  AddToStageMap(girderKey,castClosureIntervalIdx,stageIdx++);

                  if ( bTimeStepMethod && 0 < cureClosureInterval.Duration )
                  {
                     vIntervals.push_back(cureClosureInterval);
                     IntervalIndexType compositeClosureIntervalIdx = vIntervals.size()-1;
                     AddToStageMap(girderKey,compositeClosureIntervalIdx,stageIdx++);
                  }
               }
            }
         } // end if - cast closure activity

         if ( pTimelineEvent->GetCastDeckActivity().IsEnabled() )
         {
            CInterval castDeckInterval;

            // this is a sudden change in loading
            castDeckInterval.StartEventIdx = eventIdx;
            castDeckInterval.EndEventIdx   = eventIdx;
            castDeckInterval.Start         = pTimelineEvent->GetDay();
            castDeckInterval.End           = castDeckInterval.Start;
            castDeckInterval.Middle        = castDeckInterval.Start;
            castDeckInterval.Duration      = 0;
            castDeckInterval.Description   = _T("Cast Deck");

            CInterval cureDeckInterval;
            cureDeckInterval.StartEventIdx = eventIdx;
            cureDeckInterval.EndEventIdx   = eventIdx;
            cureDeckInterval.Start = castDeckInterval.End;
            cureDeckInterval.Duration = pTimelineEvent->GetCastDeckActivity().GetConcreteAgeAtContinuity();
            cureDeckInterval.End = cureDeckInterval.Start + cureDeckInterval.Duration;
            cureDeckInterval.Middle = 0.5*(cureDeckInterval.Start + cureDeckInterval.End);
            cureDeckInterval.Description = _T("Deck curing");

            CInterval compositeDeckInterval;
            compositeDeckInterval.StartEventIdx = eventIdx;
            compositeDeckInterval.EndEventIdx   = eventIdx;
            compositeDeckInterval.Start = cureDeckInterval.End;
            compositeDeckInterval.Duration = 0;
            compositeDeckInterval.End = compositeDeckInterval.Start + compositeDeckInterval.Duration;
            compositeDeckInterval.Middle = 0.5*(compositeDeckInterval.Start + compositeDeckInterval.End);
            compositeDeckInterval.Description = _T("Composite Deck");

            // deck casting applies to all girders
            StageIndexType castDeckStageIdx      = stageIdx++;
            StageIndexType cureDeckStageIdx      = stageIdx++;
            StageIndexType compositeDeckStageIdx = stageIdx++;

            bool bCureDeckStageUsed = true; // if the deck curing stage is not used, we need to decrement stageIdx to compensate for the increment when assign a stage index above
            bool bCompositeDeckStageUsed = true; // if the composite deck stage is not used, we need to decrement stageIdx to compensate for the increment when assign a stage index above

            GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
            for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
            {
               GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
               for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
               {
                  CGirderKey girderKey(grpIdx,gdrIdx);
                  std::map<CGirderKey,std::vector<CInterval>>::iterator found(m_IntervalSequences.find(girderKey));
                  ATLASSERT( found != m_IntervalSequences.end());
                  std::vector<CInterval>& vIntervals(found->second);

                  vIntervals.push_back(castDeckInterval);
                  IntervalIndexType castDeckIntervalIdx = vIntervals.size()-1;
                  m_CastDeckInterval.insert(std::make_pair(girderKey,castDeckIntervalIdx));
                  AddToStageMap(girderKey,castDeckIntervalIdx,castDeckStageIdx);

                  if ( bTimeStepMethod )
                  {
                     if ( 0 < cureDeckInterval.Duration )
                     {
                        vIntervals.push_back(cureDeckInterval);
                        IntervalIndexType cureDeckIntervalIdx = vIntervals.size()-1;
                        AddToStageMap(girderKey,cureDeckIntervalIdx,cureDeckStageIdx);

                        vIntervals.push_back(compositeDeckInterval);
                        IntervalIndexType compositeDeckIntervalIdx = vIntervals.size()-1;
                        AddToStageMap(girderKey,compositeDeckIntervalIdx,compositeDeckStageIdx);
                        m_CompositeDeckInterval.insert(std::make_pair(girderKey,compositeDeckIntervalIdx));
                     }
                     else
                     {
                        vIntervals.push_back(compositeDeckInterval);
                        IntervalIndexType compositeDeckIntervalIdx = vIntervals.size()-1;

                        // there is no curing duration so the composite deck state is the curing deck stage
                        AddToStageMap(girderKey,compositeDeckIntervalIdx,cureDeckStageIdx);
                        m_CompositeDeckInterval.insert(std::make_pair(girderKey,compositeDeckIntervalIdx));
                        bCureDeckStageUsed = false;
                     }
                  }
                  else
                  {
                     vIntervals.push_back(compositeDeckInterval);
                     IntervalIndexType compositeDeckIntervalIdx = vIntervals.size()-1;

                     // there is no curing duration so the composite deck state is the curing deck stage
                     AddToStageMap(girderKey,compositeDeckIntervalIdx,cureDeckStageIdx);
                     m_CompositeDeckInterval.insert(std::make_pair(girderKey,compositeDeckIntervalIdx));
                     bCureDeckStageUsed = false;
                  }
               }// next gdr
            } // next grp
            
            if ( bCureDeckStageUsed == false )
            {
               stageIdx--; // didn't use the cure deck stage so back up the stage index by one
            }
            
            if ( bCompositeDeckStageUsed == false )
            {
               stageIdx--; // didn't use the composite deck stage so back up the stage index by one
            }

         } // end if cast deck activity
      } // end if cast deck and closure activities

      if ( pTimelineEvent->GetApplyLoadActivity().IsEnabled() )
      {
         CInterval applyLoadInterval;

         bool bIncrementStage = true;

         // this is an abrupt change in loading
         applyLoadInterval.StartEventIdx = eventIdx;
         applyLoadInterval.EndEventIdx   = eventIdx;
         applyLoadInterval.Start         = pTimelineEvent->GetDay();
         applyLoadInterval.End           = applyLoadInterval.Start;
         applyLoadInterval.Middle        = applyLoadInterval.Start;
         applyLoadInterval.Duration      = 0;

         bool bRailing  = pTimelineEvent->GetApplyLoadActivity().IsRailingSystemLoadApplied();
         bool bOverlay  = pTimelineEvent->GetApplyLoadActivity().IsOverlayLoadApplied();
         bool bLiveLoad = pTimelineEvent->GetApplyLoadActivity().IsLiveLoadApplied();
         bool bUserLoad = pTimelineEvent->GetApplyLoadActivity().IsUserLoadApplied();

         bool bHasRailingBeenApplied  = false;
         bool bHasOverlayBeenApplied  = false;
         bool bHasLiveLoadBeenApplied = false;

         if ( bUserLoad && !bRailing && !bOverlay && !bLiveLoad )
         {
            applyLoadInterval.Description = _T("User Defined Loading Applied");
         }

         if ( bRailing && !bHasRailingBeenApplied )
         {
            // railing system load is applied in this activity and it has not yet
            // been accounted for

            if ( bOverlay )
            {
               // overlay and railing system applied at the same time (not a future overlay)
               applyLoadInterval.Description = _T("Install Railing System and Overlay");
               bHasRailingBeenApplied = true;
               bHasOverlayBeenApplied = true; // overlay had already been accounted for..
            }
            else
            {
               // railing system applied by itself
               applyLoadInterval.Description   = _T("Install Railing System");
               bHasRailingBeenApplied = true;
            }
         }

         if ( bOverlay && !bHasOverlayBeenApplied )
         {
            // overlay load is applied in this activity and it has not yet 
            // been accounted for
            if ( bLiveLoad && !bHasLiveLoadBeenApplied )
            {
               // overaly and live load has been applied at the same tine
               // not a future overlay
               applyLoadInterval.Description = _T("Open to Traffic");
               bHasOverlayBeenApplied = true;
               bHasLiveLoadBeenApplied = true;
            }
            else
            {
               // overlay is applied in its own interval
               applyLoadInterval.Description   = _T("Install Overlay");
               bHasOverlayBeenApplied = true;
            }
         }

         if ( bLiveLoad && !bHasLiveLoadBeenApplied )
         {
            // live load is applied in its own interval
            applyLoadInterval.Description   = _T("Open to Traffic");
            bHasLiveLoadBeenApplied = true;
         }

         GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
         for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
         {
            GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
            for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
            {
               CGirderKey girderKey(grpIdx,gdrIdx);
               std::map<CGirderKey,std::vector<CInterval>>::iterator found(m_IntervalSequences.find(girderKey));
               ATLASSERT( found != m_IntervalSequences.end());
               std::vector<CInterval>& vIntervals(found->second);

               // some sort of loading was applied during this activity
               // create the interval and the stage
               IntervalIndexType applyLoadIntervalIdx;
               if ( pTimelineEvent->GetCastDeckActivity().IsEnabled() )
               {
                  // for Non-timestep analysis (PGSuper w/o time-step), if the user defined load
                  // is applied at the same time as when the deck is cast, don't add a new interval... go
                  // back to the deck casting interval and use it. The deck casting interval is two intervals before the end

                  if ( m_bTimeStepMethod )
                  {
                     // (one back is composite deck, two back is deck curing, three back is cast deck)
                     applyLoadIntervalIdx = vIntervals.size()-3;
                  }
                  else
                  {
                     // (one back is composite deck, two back is cast deck)
                     applyLoadIntervalIdx = vIntervals.size()-2;
                  }
                  bIncrementStage = false;
               }
               else
               {
                  vIntervals.push_back(applyLoadInterval);
                  applyLoadIntervalIdx = vIntervals.size()-1;
                  AddToStageMap(girderKey,applyLoadIntervalIdx,stageIdx);
               }

               // if the loading was use railing, overlay, or live load, keep a record
               // so it is easier to look up this information later
               if ( bUserLoad )
               {
                  CComPtr<IBroker> pBroker;
                  EAFGetBroker(&pBroker);
                  GET_IFACE2(pBroker,IUserDefinedLoadData,pUserDefinedLoadData);

                  IndexType nUserLoads = pTimelineEvent->GetApplyLoadActivity().GetUserLoadCount();
                  for ( IndexType userLoadIdx = 0; userLoadIdx < nUserLoads; userLoadIdx++ )
                  {
                     LoadIDType userLoadID = pTimelineEvent->GetApplyLoadActivity().GetUserLoadID(userLoadIdx);

                     UserLoads::LoadCase loadCase;

                     const CPointLoadData*       pPointLoad       = pUserDefinedLoadData->FindPointLoad(userLoadID);
                     const CDistributedLoadData* pDistributedLoad = pUserDefinedLoadData->FindDistributedLoad(userLoadID);
                     const CMomentLoadData*      pMomentLoad      = pUserDefinedLoadData->FindMomentLoad(userLoadID);

                     if ( pPointLoad )
                     {
                        loadCase = pPointLoad->m_LoadCase;
                     }
                     else if ( pDistributedLoad )
                     {
                        loadCase = pDistributedLoad->m_LoadCase;
                     }
                     else
                     {
                        ATLASSERT(pMomentLoad);
                        loadCase = pMomentLoad->m_LoadCase;
                     }

                     if ( loadCase != UserLoads::LL_IM )
                     {
                        CUserLoadKey key(girderKey,eventIdx);
                        m_UserLoadInterval[loadCase].insert(std::make_pair(key,applyLoadIntervalIdx));
                     }
                  }
               }

               if ( bHasRailingBeenApplied )
               {
                  m_RailingSystemInterval.insert(std::make_pair(girderKey,applyLoadIntervalIdx));
               }

               if ( bHasOverlayBeenApplied )
               {
                  m_OverlayInterval.insert(std::make_pair(girderKey,applyLoadIntervalIdx));
               }

               if ( bHasLiveLoadBeenApplied )
               {
                  m_LiveLoadInterval.insert(std::make_pair(girderKey,applyLoadIntervalIdx));
               }
            } // next gdrIdx
         } // next grpIdx

         if ( bIncrementStage )
         {
            stageIdx++;
         }
      } // end if loading activity

      if ( pTimelineEvent->GetStressTendonActivity().IsEnabled() && 
           pTimelineEvent->GetRemoveTempSupportsActivity().IsEnabled() )
      {
         CInterval stressTendonInterval;

         // this is an abrupt change in boundary condition, treat as sudden change in loading
         std::vector<CGirderKey> vGirderKeys;
         const CStressTendonActivity& stressTendon = pTimelineEvent->GetStressTendonActivity();
         const std::set<CTendonKey>& tendons( stressTendon.GetTendons() );
         std::set<CTendonKey>::const_iterator tendonIterBegin(tendons.begin());
         std::set<CTendonKey>::const_iterator tendonIter = tendonIterBegin;
         std::set<CTendonKey>::const_iterator tendonIterEnd(tendons.end());
         for ( ; tendonIter != tendonIterEnd; tendonIter++ )
         {
            const CTendonKey& tendonKey(*tendonIter);
            GirderIDType girderID = tendonKey.girderID;
            DuctIndexType ductIdx = tendonKey.ductIdx;

            const CSplicedGirderData* pGirder = pBridgeDesc->FindGirder(girderID);
            CGirderKey girderKey(pGirder->GetGirderKey());

            vGirderKeys.push_back(girderKey);

            CString strDescription;
            if ( tendonIter == tendonIterBegin )
            {
               strDescription.Format(_T("Stress tendon %d and remove temporary supports"),LABEL_DUCT(ductIdx));
            }
            else
            {
               strDescription.Format(_T("Stress tendon %d"),LABEL_DUCT(ductIdx));
            }

            stressTendonInterval.StartEventIdx = eventIdx;
            stressTendonInterval.EndEventIdx   = eventIdx;
            stressTendonInterval.Start         = pTimelineEvent->GetDay();
            stressTendonInterval.End           = stressTendonInterval.Start;
            stressTendonInterval.Middle        = stressTendonInterval.Start;
            stressTendonInterval.Duration      = 0;
            stressTendonInterval.Description   = strDescription;


            std::map<CGirderKey,std::vector<CInterval>>::iterator found(m_IntervalSequences.find(girderKey));
            ATLASSERT( found != m_IntervalSequences.end());
            std::vector<CInterval>& vIntervals(found->second);

            vIntervals.push_back(stressTendonInterval);
            IntervalIndexType stressTendonIntervalIdx = vIntervals.size()-1;
            m_StressTendonIntervals.insert(std::make_pair(CTendonKey(girderKey,ductIdx),stressTendonIntervalIdx));
            AddToStageMap(girderKey,stressTendonIntervalIdx,stageIdx++);

            // keep track of the intervals when temporary supports are removed
            // since the interval sequence for each girder can be different, we have to keep
            // the a map of temporary support removal to interval index for each girder
            IntervalIndexType removeTempSupportIntervalIdx = stressTendonIntervalIdx;
            const CRemoveTemporarySupportsActivity& removeTS = pTimelineEvent->GetRemoveTempSupportsActivity();
            const std::vector<SupportIDType>& tsIDs(removeTS.GetTempSupports());
            std::vector<SupportIDType>::const_iterator tsIter(tsIDs.begin());
            std::vector<SupportIDType>::const_iterator tsIterEnd(tsIDs.end());
            for ( ; tsIter != tsIterEnd; tsIter++ )
            {
               SupportIDType tsID(*tsIter);
               CTempSupportKey key(girderKey,tsID);
               m_TempSupportRemovalIntervals.insert(std::make_pair(key,removeTempSupportIntervalIdx));
            }
         }

         // need to add temporary support removal for all girders not covered above
         // Example... create a bridge model that doesn't have any tendons in one girder. that girder
         // will not be accounted for above so we have to add a temporary support removal interval here
         vGirderKeys.erase(std::unique(vGirderKeys.begin(),vGirderKeys.end()),vGirderKeys.end());
         CInterval removeTempSupportInterval;
         removeTempSupportInterval.StartEventIdx = eventIdx;
         removeTempSupportInterval.EndEventIdx   = eventIdx;
         removeTempSupportInterval.Start         = pTimelineEvent->GetDay();
         removeTempSupportInterval.End           = removeTempSupportInterval.Start;
         removeTempSupportInterval.Middle        = removeTempSupportInterval.Start;
         removeTempSupportInterval.Duration      = 0;
         removeTempSupportInterval.Description   = _T("Remove temporary supports");

         GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
         for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
         {
            GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
            for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
            {
               CGirderKey girderKey(grpIdx,gdrIdx);
               if ( std::find(vGirderKeys.begin(),vGirderKeys.end(),girderKey) != vGirderKeys.end() )
               {
                  continue;
               }

               std::map<CGirderKey,std::vector<CInterval>>::iterator found(m_IntervalSequences.find(girderKey));
               ATLASSERT( found != m_IntervalSequences.end());
               std::vector<CInterval>& vIntervals(found->second);

               vIntervals.push_back(removeTempSupportInterval);
               IntervalIndexType removeTempSupportIntervalIdx = vIntervals.size() - 1;
               AddToStageMap(girderKey,removeTempSupportIntervalIdx,stageIdx);

               const CRemoveTemporarySupportsActivity& removeTS = pTimelineEvent->GetRemoveTempSupportsActivity();
               std::vector<SupportIDType> tsIDs(removeTS.GetTempSupports());
               std::vector<SupportIDType>::iterator iter(tsIDs.begin());
               std::vector<SupportIDType>::iterator end(tsIDs.end());
               for ( ; iter != end; iter++ )
               {
                  SupportIDType tsID(*iter);
                  CTempSupportKey key(girderKey,tsID);
                  m_TempSupportRemovalIntervals.insert(std::make_pair(key,removeTempSupportIntervalIdx));
               }
            }// next gdr
         } // next group
      }
      else
      {
         if ( pTimelineEvent->GetStressTendonActivity().IsEnabled() )
         {
            // this is an abrupt change in boundary condition, treat as sudden change in loading
            // create an interval for each duct that is stressed in this activity so we capture
            // the effect of stressing one tendon has on the other. record the stressing event
            const CStressTendonActivity& stressTendon = pTimelineEvent->GetStressTendonActivity();
            const std::set<CTendonKey>& tendons( stressTendon.GetTendons() );
            std::set<CTendonKey>::const_iterator tendonIter(tendons.begin());
            std::set<CTendonKey>::const_iterator tendonIterEnd(tendons.end());
            for ( ; tendonIter != tendonIterEnd; tendonIter++ )
            {
               const CTendonKey& tendonKey(*tendonIter);
               GirderIDType girderID = tendonKey.girderID;
               DuctIndexType ductIdx = tendonKey.ductIdx;

               const CSplicedGirderData* pGirder = pBridgeDesc->FindGirder(girderID);
               CGirderKey girderKey(pGirder->GetGirderKey());

               CInterval stressTendonInterval;

               CString strDescription;
               strDescription.Format(_T("Stress tendon %d"),LABEL_DUCT(ductIdx));
               stressTendonInterval.StartEventIdx = eventIdx;
               stressTendonInterval.EndEventIdx   = eventIdx;
               stressTendonInterval.Start         = pTimelineEvent->GetDay();
               stressTendonInterval.End           = stressTendonInterval.Start;
               stressTendonInterval.Middle        = stressTendonInterval.Start;
               stressTendonInterval.Duration      = 0;
               stressTendonInterval.Description   = strDescription;

               std::map<CGirderKey,std::vector<CInterval>>::iterator found(m_IntervalSequences.find(girderKey));
               ATLASSERT( found != m_IntervalSequences.end());
               std::vector<CInterval>& vIntervals(found->second);
               vIntervals.push_back(stressTendonInterval);
               IntervalIndexType stressTendonIntervalIdx = vIntervals.size()-1;
               m_StressTendonIntervals.insert(std::make_pair(CTendonKey(girderKey,ductIdx),stressTendonIntervalIdx));
               AddToStageMap(girderKey,stressTendonIntervalIdx,stageIdx++);
            }
         }

         if ( pTimelineEvent->GetRemoveTempSupportsActivity().IsEnabled() )
         {
            // this is an abrupt change in boundary condition, treat as sudden change in loading
            CInterval removeTempSupportInterval;
            removeTempSupportInterval.StartEventIdx = eventIdx;
            removeTempSupportInterval.EndEventIdx   = eventIdx;
            removeTempSupportInterval.Start         = pTimelineEvent->GetDay();
            removeTempSupportInterval.End           = removeTempSupportInterval.Start;
            removeTempSupportInterval.Middle        = removeTempSupportInterval.Start;
            removeTempSupportInterval.Duration      = 0;
            removeTempSupportInterval.Description   = _T("Remove temporary supports");

            GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
            for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
            {
               GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
               for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
               {
                  CGirderKey girderKey(grpIdx,gdrIdx);
                  std::map<CGirderKey,std::vector<CInterval>>::iterator found(m_IntervalSequences.find(girderKey));
                  ATLASSERT( found != m_IntervalSequences.end());
                  std::vector<CInterval>& vIntervals(found->second);

                  vIntervals.push_back(removeTempSupportInterval);
                  IntervalIndexType removeTempSupportIntervalIdx = vIntervals.size() - 1;
                  AddToStageMap(girderKey,removeTempSupportIntervalIdx,stageIdx);

                  const CRemoveTemporarySupportsActivity& removeTS = pTimelineEvent->GetRemoveTempSupportsActivity();
                  std::vector<SupportIDType> tsIDs(removeTS.GetTempSupports());
                  std::vector<SupportIDType>::iterator iter(tsIDs.begin());
                  std::vector<SupportIDType>::iterator end(tsIDs.end());
                  for ( ; iter != end; iter++ )
                  {
                     SupportIDType tsID(*iter);
                     CTempSupportKey key(girderKey,tsID);
                     m_TempSupportRemovalIntervals.insert(std::make_pair(key,removeTempSupportIntervalIdx));
                  }
               }// next gdr
            } // next group
            stageIdx++;
         }
      }

      // At the end of every event, create a general time step
      // that goes from the end of last interval for the current event
      // to the start of the next event
      if ( bTimeStepMethod )
      {
         bool bAddedInterval = false;
         GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
         for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
         {
            GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
            for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
            {
               CGirderKey girderKey(grpIdx,gdrIdx);
               std::map<CGirderKey,std::vector<CInterval>>::iterator found(m_IntervalSequences.find(girderKey));
               ATLASSERT( found != m_IntervalSequences.end());
               std::vector<CInterval>& vIntervals(found->second);

               CInterval time_step;
               time_step.StartEventIdx = eventIdx;
               time_step.Start = vIntervals.back().End; // starts at the end of the previous interval

               time_step.EndEventIdx = eventIdx;
               if ( eventIdx < nEvents-1 )
               {
                  time_step.EndEventIdx++; // this time step ends at the start of the next event unless this is the last event
               }

               time_step.End   = pTimelineMgr->GetStart(time_step.EndEventIdx); // ends at the start of the next event

               time_step.Duration = time_step.End - time_step.Start;
               time_step.Middle = 0.5*(time_step.Start + time_step.End);
               time_step.Description = _T("Time Step");
               if ( 0.0 < time_step.Duration )
               {
                  vIntervals.push_back(time_step);
                  IntervalIndexType timeStepIntervalIdx = vIntervals.size()-1;
                  AddToStageMap(girderKey,timeStepIntervalIdx,stageIdx);
                  bAddedInterval = true;
               }
            }// next girder
         } // next group

         if ( bAddedInterval )
         {
            stageIdx++; 
         }
      }
      else
      {
         // not doing time step, so end this event with the start of the next event
         // this provides continuity in the time intervals
         GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
         for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
         {
            GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirderCount();
            for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
            {
               CGirderKey girderKey(grpIdx,gdrIdx);
               std::map<CGirderKey,std::vector<CInterval>>::iterator found(m_IntervalSequences.find(girderKey));
               ATLASSERT( found != m_IntervalSequences.end());
               std::vector<CInterval>& vIntervals(found->second);

               if ( eventIdx < nEvents-1 )
               {
                  vIntervals.back().End = pTimelineMgr->GetStart(eventIdx+1);
               }
               else
               {
                  vIntervals.back().End = vIntervals.back().Start;
               }
            }
         }
      }
   }

   // Build the interval map... this is basically the opposite of the stage map.
   // The stage map provides a lookup of the interval to stage mapping for a girder
   // the interval map provides a lookup of the girder to interval mapping for a stage
   // For a girder, what generic bridge model stage goes with intervalIdx? use StageMap
   // For a stage, what are the analysis intervals for each girder? use IntervalMap
   std::map<CGirderKey,std::map<IntervalIndexType,StageIndexType>>::iterator stageMapIter(m_StageMap.begin());
   std::map<CGirderKey,std::map<IntervalIndexType,StageIndexType>>::iterator stageMapIterEnd(m_StageMap.end());
   for ( ; stageMapIter != stageMapIterEnd; stageMapIter++ )
   {
      const CGirderKey& girderKey(stageMapIter->first);
      std::map<IntervalIndexType,StageIndexType>& vIntervalMap(stageMapIter->second);
      std::map<IntervalIndexType,StageIndexType>::iterator intervalMapIter(vIntervalMap.begin());
      std::map<IntervalIndexType,StageIndexType>::iterator intervalMapIterEnd(vIntervalMap.end());
      for ( ; intervalMapIter != intervalMapIterEnd; intervalMapIter++ )
      {
         IntervalIndexType intervalIdx = intervalMapIter->first;
         StageIndexType    stageIdx    = intervalMapIter->second;

         std::map<StageIndexType,std::map<CGirderKey,IntervalIndexType>>::iterator found(m_IntervalMap.find(stageIdx));
         if ( found == m_IntervalMap.end() )
         {
            // this is a new stage
            std::map<CGirderKey,IntervalIndexType> vGirderIntervalMap;
            vGirderIntervalMap.insert(std::make_pair(girderKey,intervalIdx));
            m_IntervalMap.insert(std::make_pair(stageIdx,vGirderIntervalMap));
         }
         else
         {
            // this stage has already been encountered
            std::map<CGirderKey,IntervalIndexType>& vGirderIntervalMap(found->second);
            vGirderIntervalMap.insert(std::make_pair(girderKey,intervalIdx));
         }
      }
   }

   // If we aren't doing type step analysis, this is a PGSuper project
   // Append the old Bridge Site names to the interval descriptions for
   // continuity with previous versions
   if ( !bTimeStepMethod )
   {
      std::map<CGirderKey,std::vector<CInterval>>::iterator seqIter(m_IntervalSequences.begin());
      std::map<CGirderKey,std::vector<CInterval>>::iterator seqIterEnd(m_IntervalSequences.end());
      for ( ; seqIter != seqIterEnd; seqIter++ )
      {
         CGirderKey girderKey(seqIter->first);
         std::vector<CInterval>& vIntervals(seqIter->second);
         IntervalIndexType releaseIntervalIdx = GetPrestressReleaseInterval(CSegmentKey(girderKey,0));
         vIntervals[releaseIntervalIdx].Description += _T(" (Casting Yard)");

         IntervalIndexType castDeckIntervalIdx = GetCastDeckInterval(girderKey);
         vIntervals[castDeckIntervalIdx].Description += _T(" (Bridge Site 1)");

         IntervalIndexType installRailingIntervalIdx = GetInstallRailingSystemInterval(girderKey);
         vIntervals[installRailingIntervalIdx].Description += _T(" (Bridge Site 2)");

         IntervalIndexType liveLoadInteralIdx = GetLiveLoadInterval(girderKey);
         vIntervals[liveLoadInteralIdx].Description += _T(" (Bridge Site 3)");
      }
   }


   ASSERT_VALID;
}

StageIndexType CIntervalManager::GetStageCount() const
{
   return m_IntervalMap.size(); // key is stage so total size is the number of stages
}

StageIndexType CIntervalManager::GetStage(const CGirderKey& girderKey,IntervalIndexType intervalIdx) const
{
   ASSERT_GIRDER_KEY(girderKey);
   ATLASSERT(intervalIdx != INVALID_INDEX);

   std::map<CGirderKey,std::map<IntervalIndexType,StageIndexType>>::const_iterator foundGirder(m_StageMap.find(girderKey));
   ATLASSERT(foundGirder != m_StageMap.end());

   const std::map<IntervalIndexType,StageIndexType>& vIntervalMap(foundGirder->second);

   std::map<IntervalIndexType,StageIndexType>::const_iterator foundInterval(vIntervalMap.find(intervalIdx));
   ATLASSERT(foundInterval != vIntervalMap.end());

   StageIndexType stageIdx = foundInterval->second;
   return stageIdx;
}

const std::map<IntervalIndexType,StageIndexType>& CIntervalManager::GetStageMap(const CGirderKey& girderKey) const
{
   std::map<CGirderKey,std::map<IntervalIndexType,StageIndexType>>::const_iterator found(m_StageMap.find(girderKey));
   ATLASSERT(found != m_StageMap.end());
   return found->second;
}

const std::map<CGirderKey,IntervalIndexType>& CIntervalManager::GetIntervalMap(StageIndexType stageIdx) const
{
   std::map<StageIndexType,std::map<CGirderKey,IntervalIndexType>>::const_iterator found(m_IntervalMap.find(stageIdx));
   ATLASSERT(found != m_IntervalMap.end());
   return found->second;
}

IntervalIndexType CIntervalManager::GetIntervalCount(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::map<CGirderKey,std::vector<CInterval>>::const_iterator found(m_IntervalSequences.find(girderKey));
   ATLASSERT( found != m_IntervalSequences.end());
   const std::vector<CInterval>& vIntervals(found->second);
   return vIntervals.size();
}

EventIndexType CIntervalManager::GetStartEvent(const CGirderKey& girderKey,IntervalIndexType idx) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::map<CGirderKey,std::vector<CInterval>>::const_iterator found(m_IntervalSequences.find(girderKey));
   ATLASSERT( found != m_IntervalSequences.end());
   const std::vector<CInterval>& vIntervals(found->second);
   return vIntervals[idx].StartEventIdx;
}

EventIndexType CIntervalManager::GetEndEvent(const CGirderKey& girderKey,IntervalIndexType idx) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::map<CGirderKey,std::vector<CInterval>>::const_iterator found(m_IntervalSequences.find(girderKey));
   ATLASSERT( found != m_IntervalSequences.end());
   const std::vector<CInterval>& vIntervals(found->second);
   return vIntervals[idx].EndEventIdx;
}

Float64 CIntervalManager::GetTime(const CGirderKey& girderKey,IntervalIndexType idx,pgsTypes::IntervalTimeType timeType) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::map<CGirderKey,std::vector<CInterval>>::const_iterator found(m_IntervalSequences.find(girderKey));
   ATLASSERT( found != m_IntervalSequences.end());
   const std::vector<CInterval>& vIntervals(found->second);

   Float64 time;
   switch(timeType)
   {
   case pgsTypes::Start:
      time = vIntervals[idx].Start;
      break;

   case pgsTypes::Middle:
      time = vIntervals[idx].Middle;
      break;

   case pgsTypes::End:
      time = vIntervals[idx].End;
      break;

   default:
      ATLASSERT(false);
   }

   return time;
}

Float64 CIntervalManager::GetDuration(const CGirderKey& girderKey,IntervalIndexType idx) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::map<CGirderKey,std::vector<CInterval>>::const_iterator found(m_IntervalSequences.find(girderKey));
   ATLASSERT( found != m_IntervalSequences.end());
   const std::vector<CInterval>& vIntervals(found->second);
   return vIntervals[idx].Duration;
}

LPCTSTR CIntervalManager::GetDescription(const CGirderKey& girderKey,IntervalIndexType idx) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::map<CGirderKey,std::vector<CInterval>>::const_iterator found(m_IntervalSequences.find(girderKey));
   ATLASSERT( found != m_IntervalSequences.end());
   const std::vector<CInterval>& vIntervals(found->second);
   return vIntervals[idx].Description.c_str();
}

IntervalIndexType CIntervalManager::GetInterval(const CGirderKey& girderKey,EventIndexType eventIdx) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::map<CGirderKey,std::vector<CInterval>>::const_iterator found(m_IntervalSequences.find(girderKey));
   ATLASSERT( found != m_IntervalSequences.end());
   const std::vector<CInterval>& vIntervals(found->second);

   IntervalIndexType nIntervals = vIntervals.size();
   for ( IntervalIndexType i = 0; i < nIntervals; i++ )
   {
      if ( vIntervals[i].StartEventIdx == eventIdx )
      {
         return i;
      }
   }

   // if this is the last event, the return the last interval
   if ( vIntervals.back().EndEventIdx == eventIdx )
   {
      return vIntervals.size()-1;
   }

   ATLASSERT(false); // event not found... can't determine interval
   return INVALID_INDEX;
}

IntervalIndexType CIntervalManager::GetCastDeckInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::map<CGirderKey,IntervalIndexType>::const_iterator found(m_CastDeckInterval.find(girderKey));
   ATLASSERT(found != m_CastDeckInterval.end());
   return found->second;
}

IntervalIndexType CIntervalManager::GetCompositeDeckInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::map<CGirderKey,IntervalIndexType>::const_iterator found(m_CompositeDeckInterval.find(girderKey));
   ATLASSERT(found != m_CompositeDeckInterval.end());
   return found->second;
}

IntervalIndexType CIntervalManager::GetFirstStressStrandInterval(const CGirderKey& girderKey) const
{
   if ( girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS )
   {
      IntervalIndexType intervalIdx = MAX_INDEX;
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iter(m_StrandStressingSequenceIntervalLimits.begin());
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iterEnd(m_StrandStressingSequenceIntervalLimits.end());
      for ( ; iter != iterEnd; iter++ )
      {
         if ( (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == ALL_GIRDERS) ||
              (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == iter->first.girderIndex) ||
              (girderKey.groupIndex == iter->first.groupIndex && girderKey.girderIndex == ALL_GIRDERS) 
            )
         {
            intervalIdx = Min(intervalIdx,iter->second.first);
         }
      }
      return intervalIdx;
   }
   else
   {
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator found(m_StrandStressingSequenceIntervalLimits.find(girderKey));
      ATLASSERT(found != m_StrandStressingSequenceIntervalLimits.end());
      return found->second.first;
   }
}

IntervalIndexType CIntervalManager::GetLastStressStrandInterval(const CGirderKey& girderKey) const
{
   if ( girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS )
   {
      IntervalIndexType intervalIdx = MAX_INDEX;
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iter(m_StrandStressingSequenceIntervalLimits.begin());
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iterEnd(m_StrandStressingSequenceIntervalLimits.end());
      for ( ; iter != iterEnd; iter++ )
      {
         if ( (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == ALL_GIRDERS) ||
              (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == iter->first.girderIndex) ||
              (girderKey.groupIndex == iter->first.groupIndex && girderKey.girderIndex == ALL_GIRDERS) 
            )
         {
            intervalIdx = Min(intervalIdx,iter->second.second);
         }
      }
      return intervalIdx;
   }
   else
   {
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator found(m_StrandStressingSequenceIntervalLimits.find(girderKey));
      ATLASSERT(found != m_StrandStressingSequenceIntervalLimits.end());
      return found->second.second;
   }
}

IntervalIndexType CIntervalManager::GetStressStrandInterval(const CSegmentKey& segmentKey) const
{
   ASSERT_SEGMENT_KEY(segmentKey); // must be a specific segment key
   std::map<CSegmentKey,IntervalIndexType>::const_iterator found(m_StressStrandIntervals.find(segmentKey));
   ATLASSERT( found != m_StressStrandIntervals.end());
   return found->second;
}

IntervalIndexType CIntervalManager::GetFirstPrestressReleaseInterval(const CGirderKey& girderKey) const
{
   if ( girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS )
   {
      IntervalIndexType intervalIdx = MAX_INDEX;
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iter(m_ReleaseSequenceIntervalLimits.begin());
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iterEnd(m_ReleaseSequenceIntervalLimits.end());
      for ( ; iter != iterEnd; iter++ )
      {
         if ( (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == ALL_GIRDERS) ||
              (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == iter->first.girderIndex) ||
              (girderKey.groupIndex == iter->first.groupIndex && girderKey.girderIndex == ALL_GIRDERS) 
            )
         {
            intervalIdx = Min(intervalIdx,iter->second.first);
         }
      }
      return intervalIdx;
   }
   else
   {
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator found(m_ReleaseSequenceIntervalLimits.find(girderKey));
      ATLASSERT(found != m_ReleaseSequenceIntervalLimits.end());
      return found->second.first;
   }
}

IntervalIndexType CIntervalManager::GetLastPrestressReleaseInterval(const CGirderKey& girderKey) const
{
   if ( girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS )
   {
      IntervalIndexType intervalIdx = MAX_INDEX;
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iter(m_ReleaseSequenceIntervalLimits.begin());
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator iterEnd(m_ReleaseSequenceIntervalLimits.end());
      for ( ; iter != iterEnd; iter++ )
      {
         if ( (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == ALL_GIRDERS) ||
              (girderKey.groupIndex == ALL_GROUPS && girderKey.girderIndex == iter->first.girderIndex) ||
              (girderKey.groupIndex == iter->first.groupIndex && girderKey.girderIndex == ALL_GIRDERS) 
            )
         {
            intervalIdx = Min(intervalIdx,iter->second.second);
         }
      }
      return intervalIdx;
   }
   else
   {
      std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator found(m_ReleaseSequenceIntervalLimits.find(girderKey));
      ATLASSERT(found != m_ReleaseSequenceIntervalLimits.end());
      return found->second.second;
   }
}

IntervalIndexType CIntervalManager::GetPrestressReleaseInterval(const CSegmentKey& segmentKey) const
{
   ASSERT_SEGMENT_KEY(segmentKey); // must be a specific segment key
   std::map<CSegmentKey,IntervalIndexType>::const_iterator found(m_ReleaseIntervals.find(segmentKey));
   ATLASSERT( found != m_ReleaseIntervals.end());
   return found->second;
}

IntervalIndexType CIntervalManager::GetLiftingInterval(const CSegmentKey& segmentKey) const
{
   return GetPrestressReleaseInterval(segmentKey) + 1;
}

IntervalIndexType CIntervalManager::GetStorageInterval(const CSegmentKey& segmentKey) const
{
   return GetPrestressReleaseInterval(segmentKey) + 2;
}

IntervalIndexType CIntervalManager::GetHaulingInterval(const CSegmentKey& segmentKey) const
{
   return GetErectSegmentInterval(segmentKey) - 1;
}

IntervalIndexType CIntervalManager::GetErectSegmentInterval(const CSegmentKey& segmentKey) const
{
   ASSERT_SEGMENT_KEY(segmentKey); // must be a specific segment key
   std::map<CSegmentKey,IntervalIndexType>::const_iterator found(m_SegmentErectionIntervals.find(segmentKey));
   ATLASSERT( found != m_SegmentErectionIntervals.end());
   return found->second;
}

bool CIntervalManager::IsSegmentErectionInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx) const
{
   ASSERT_GIRDER_KEY(girderKey);
   ATLASSERT(intervalIdx != INVALID_INDEX);

   std::map<CSegmentKey,IntervalIndexType>::const_iterator iter(m_SegmentErectionIntervals.begin());
   std::map<CSegmentKey,IntervalIndexType>::const_iterator iterEnd(m_SegmentErectionIntervals.end());
   for ( ; iter != iterEnd; iter++ )
   {
      const CSegmentKey& segmentKey = iter->first;
      if ( girderKey.IsEqual(segmentKey) )
      {
         IntervalIndexType erectionIntervalIdx = iter->second;
         if ( erectionIntervalIdx == intervalIdx )
         {
            return true;
         }
      }
   }

   return false;
}

IntervalIndexType CIntervalManager::GetTemporaryStrandRemovalInterval(const CSegmentKey& segmentKey) const
{
   std::map<CSegmentKey,IntervalIndexType>::const_iterator found(m_RemoveTemporaryStrandsIntervals.find(segmentKey));
   if ( found != m_RemoveTemporaryStrandsIntervals.end() )
   {
      return found->second;
   }
   else
   {
      return INVALID_INDEX; // there aren't any temporary strands in this segment
   }
}

IntervalIndexType CIntervalManager::GetFirstSegmentErectionInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey);
   std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator found(m_SegmentErectionSequenceIntervalLimits.find(girderKey));
   ATLASSERT(found != m_SegmentErectionSequenceIntervalLimits.end());
   return found->second.first;
}

IntervalIndexType CIntervalManager::GetLastSegmentErectionInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey);
   std::map<CGirderKey,std::pair<IntervalIndexType,IntervalIndexType>>::const_iterator found(m_SegmentErectionSequenceIntervalLimits.find(girderKey));
   ATLASSERT(found != m_SegmentErectionSequenceIntervalLimits.end());
   return found->second.second;
}

IntervalIndexType CIntervalManager::GetLiveLoadInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::map<CGirderKey,IntervalIndexType>::const_iterator found(m_LiveLoadInterval.find(girderKey));
   ATLASSERT(found != m_LiveLoadInterval.end());
   return found->second;
}

IntervalIndexType CIntervalManager::GetOverlayInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::map<CGirderKey,IntervalIndexType>::const_iterator found(m_OverlayInterval.find(girderKey));

   IntervalIndexType intervalIdx = INVALID_INDEX;
   if ( found != m_OverlayInterval.end() )
   {
      intervalIdx = found->second;
   }
   return intervalIdx;
}

IntervalIndexType CIntervalManager::GetInstallRailingSystemInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::map<CGirderKey,IntervalIndexType>::const_iterator found(m_RailingSystemInterval.find(girderKey));
   ATLASSERT(found != m_RailingSystemInterval.end());
   return found->second;
}

IntervalIndexType CIntervalManager::GetUserLoadInterval(const CGirderKey& girderKey,UserLoads::LoadCase loadCase,EventIndexType eventIdx) const
{
   ASSERT_GIRDER_KEY(girderKey);
   ASSERT(loadCase == UserLoads::DC || loadCase == UserLoads::DW);

   CUserLoadKey key(girderKey,eventIdx);
   std::map<CUserLoadKey,IntervalIndexType>::const_iterator found(m_UserLoadInterval[loadCase].find(key));
   if ( found == m_UserLoadInterval[loadCase].end() )
   {
      ATLASSERT(false);
      return INVALID_INDEX;
   }

   return found->second;
}

IntervalIndexType CIntervalManager::GetStressTendonInterval(const CGirderKey& girderKey,DuctIndexType ductIdx) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::map<CTendonKey,IntervalIndexType>::const_iterator found(m_StressTendonIntervals.find(CTendonKey(girderKey,ductIdx)));
   if ( found == m_StressTendonIntervals.end() )
   {
      ATLASSERT(false);
      return INVALID_INDEX;
   }

   return found->second;
}

IntervalIndexType CIntervalManager::GetFirstTendonStressingInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::set<IntervalIndexType> intervals;
   std::map<CTendonKey,IntervalIndexType>::const_iterator iter(m_StressTendonIntervals.begin());
   std::map<CTendonKey,IntervalIndexType>::const_iterator end(m_StressTendonIntervals.end());
   for ( ; iter != end; iter++ )
   {
      if ( girderKey == iter->first.girderKey )
      {
         intervals.insert(iter->second);
      }
   }

   if ( intervals.size() == 0 )
   {
      return INVALID_INDEX;
   }

   return *intervals.begin();
}

IntervalIndexType CIntervalManager::GetLastTendonStressingInterval(const CGirderKey& girderKey) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   std::set<IntervalIndexType> intervals;
   std::map<CTendonKey,IntervalIndexType>::const_iterator iter(m_StressTendonIntervals.begin());
   std::map<CTendonKey,IntervalIndexType>::const_iterator end(m_StressTendonIntervals.end());
   for ( ; iter != end; iter++ )
   {
      if ( girderKey == iter->first.girderKey )
      {
         intervals.insert(iter->second);
      }
   }

   if ( intervals.size() == 0 )
   {
      return INVALID_INDEX;
   }

   return *intervals.rbegin();
}

IntervalIndexType CIntervalManager::GetTemporarySupportRemovalInterval(const CGirderKey& girderKey,SupportIDType tsID) const
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
   CTempSupportKey key(girderKey,tsID);
   std::map<CTempSupportKey,IntervalIndexType>::const_iterator found(m_TempSupportRemovalIntervals.find(key) );
   if ( found == m_TempSupportRemovalIntervals.end() )
   {
      ATLASSERT(false);
      return INVALID_INDEX;
   }

   return found->second;
}

void CIntervalManager::AddToStageMap(const CGirderKey& girderKey,IntervalIndexType intervalIdx,StageIndexType stageIdx)
{
   ATLASSERT(intervalIdx != INVALID_INDEX);
   ATLASSERT(stageIdx    != INVALID_INDEX);

   std::map<CGirderKey,std::map<IntervalIndexType,StageIndexType>>::iterator found(m_StageMap.find(girderKey));
   if ( found == m_StageMap.end() )
   {
      // this is a new girder key
      std::map<IntervalIndexType,StageIndexType> intervalMap;
      std::pair<std::map<IntervalIndexType,StageIndexType>::iterator,bool> intervalMapInsertResult;
      intervalMapInsertResult = intervalMap.insert(std::make_pair(intervalIdx,stageIdx));
      ATLASSERT(intervalMapInsertResult.second == true);

      std::pair<std::map<CGirderKey,std::map<IntervalIndexType,StageIndexType>>::iterator,bool> stageMapInsertResult;
      stageMapInsertResult = m_StageMap.insert(std::make_pair(girderKey,intervalMap));
      ATLASSERT(stageMapInsertResult.second == true);
   }
   else
   {
      // this girder has already been processed... just add the interval/stage pair ot the map
      std::map<IntervalIndexType,StageIndexType>& intervalMap(found->second);
      std::pair<std::map<IntervalIndexType,StageIndexType>::iterator,bool> intervalMapInsertResult;
      intervalMapInsertResult = intervalMap.insert(std::make_pair(intervalIdx,stageIdx));
      ATLASSERT(intervalMapInsertResult.second == true);
   }
}

#if defined _DEBUG
void CIntervalManager::AssertValid() const
{
   // make sure the bridge level events like cast deck and composite deck all occur
   // at the same stage
   std::map<CGirderKey,IntervalIndexType>::const_iterator iter;
   std::map<CGirderKey,IntervalIndexType>::const_iterator end;

   std::set<StageIndexType> stages;
   iter = m_CastDeckInterval.begin();
   end  = m_CastDeckInterval.end();
   for ( ; iter != end; iter++ )
   {
      CGirderKey girderKey(iter->first);
      IntervalIndexType intervalIdx = iter->second;

      StageIndexType stageIdx = GetStage(girderKey,intervalIdx);
      stages.insert(stageIdx);
   }
   ATLASSERT(stages.size() == 1);

   stages.clear();
   iter = m_CompositeDeckInterval.begin();
   end  = m_CompositeDeckInterval.end();
   for ( ; iter != end; iter++ )
   {
      CGirderKey girderKey(iter->first);
      IntervalIndexType intervalIdx = iter->second;

      StageIndexType stageIdx = GetStage(girderKey,intervalIdx);
      stages.insert(stageIdx);
   }
   ATLASSERT(stages.size() == 1);

   stages.clear();
   iter = m_LiveLoadInterval.begin();
   end  = m_LiveLoadInterval.end();
   for ( ; iter != end; iter++ )
   {
      CGirderKey girderKey(iter->first);
      IntervalIndexType intervalIdx = iter->second;

      StageIndexType stageIdx = GetStage(girderKey,intervalIdx);
      stages.insert(stageIdx);
   }
   ATLASSERT(stages.size() == 1);

   if ( 0 < m_OverlayInterval.size() )
   {
      // overlay loading is optional so there may not be any entries to check
      stages.clear();
      iter = m_OverlayInterval.begin();
      end  = m_OverlayInterval.end();
      for ( ; iter != end; iter++ )
      {
         CGirderKey girderKey(iter->first);
         IntervalIndexType intervalIdx = iter->second;

         StageIndexType stageIdx = GetStage(girderKey,intervalIdx);
         stages.insert(stageIdx);
      }
      ATLASSERT(stages.size() == 1);
   }

   stages.clear();
   iter = m_RailingSystemInterval.begin();
   end  = m_RailingSystemInterval.end();
   for ( ; iter != end; iter++ )
   {
      CGirderKey girderKey(iter->first);
      IntervalIndexType intervalIdx = iter->second;

      StageIndexType stageIdx = GetStage(girderKey,intervalIdx);
      stages.insert(stageIdx);
   }
   ATLASSERT(stages.size() == 1);

   // the interval map should contain a consecutive sequence of stage indicies
   StageIndexType stageIdx = 0;
   std::map<StageIndexType,std::map<CGirderKey,IntervalIndexType>>::const_iterator intervalMapIter(m_IntervalMap.begin());
   std::map<StageIndexType,std::map<CGirderKey,IntervalIndexType>>::const_iterator intervalMapIterEnd(m_IntervalMap.end());
   for ( ; intervalMapIter != intervalMapIterEnd; intervalMapIter++, stageIdx++ )
   {
      ATLASSERT(stageIdx == intervalMapIter->first);
   }

   std::map<CGirderKey,std::vector<CInterval>>::const_iterator intervalSequenceIter(m_IntervalSequences.begin());
   std::map<CGirderKey,std::vector<CInterval>>::const_iterator intervalSequenceIterEnd(m_IntervalSequences.end());
   for ( ; intervalSequenceIter != intervalSequenceIterEnd; intervalSequenceIter++ )
   {
      CGirderKey girderKey(intervalSequenceIter->first);
      const std::vector<CInterval>& vIntervals(intervalSequenceIter->second);
      std::vector<CInterval>::const_iterator intervalIter(vIntervals.begin());
      std::vector<CInterval>::const_iterator intervalIterEnd(vIntervals.end());
      for ( ; intervalIter != intervalIterEnd; intervalIter++ )
      {
         const CInterval& interval(*intervalIter);
         ATLASSERT(interval.StartEventIdx != INVALID_INDEX);
         ATLASSERT(interval.EndEventIdx   != INVALID_INDEX);
      }
   }

}
#endif


CIntervalManager::CTempSupportKey::CTempSupportKey(const CGirderKey& girderKey,SupportIDType tsID) :
m_GirderKey(girderKey),m_tsID(tsID)
{
   ASSERT_GIRDER_KEY(girderKey); // must be a specific girder key
}

CIntervalManager::CTempSupportKey::CTempSupportKey(const CIntervalManager::CTempSupportKey& other) :
m_GirderKey(other.m_GirderKey),m_tsID(other.m_tsID)
{
   ASSERT_GIRDER_KEY(m_GirderKey); // must be a specific girder key
}

bool CIntervalManager::CTempSupportKey::operator<(const CIntervalManager::CTempSupportKey& other) const
{
   if ( m_GirderKey < other.m_GirderKey )
   {
      return true;
   }

   if ( m_GirderKey.IsEqual(other.m_GirderKey) && m_tsID < other.m_tsID )
   {
      return true;
   }

   return false;
}

CIntervalManager::CUserLoadKey::CUserLoadKey(const CGirderKey& girderKey,EventIndexType eventIdx) :
m_GirderKey(girderKey),m_EventIdx(eventIdx)
{
}

CIntervalManager::CUserLoadKey::CUserLoadKey(const CUserLoadKey& other) :
m_GirderKey(other.m_GirderKey),m_EventIdx(other.m_EventIdx)
{
}

bool CIntervalManager::CUserLoadKey::operator<(const CUserLoadKey& other) const
{
   if ( m_GirderKey < other.m_GirderKey )
   {
      return true;
   }

   if ( m_GirderKey.IsEqual(other.m_GirderKey) && m_EventIdx < other.m_EventIdx )
   {
      return true;
   }

   return false;
}
