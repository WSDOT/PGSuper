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

class CSegmentKey;
class CGirderKey;

#include <PGSuperTypes.h> // for pgsTypes::ProductForceType

/*****************************************************************************
INTERFACE
   IIntervals

   Interface to get information about time-step analysis intervals

DESCRIPTION
*****************************************************************************/
// {FC41CE74-7B33-4f9d-8DF4-2DC16FA8E68D}
DEFINE_GUID(IID_IIntervals, 
0xfc41ce74, 0x7b33, 0x4f9d, 0x8d, 0xf4, 0x2d, 0xc1, 0x6f, 0xa8, 0xe6, 0x8d);
interface IIntervals : IUnknown
{
   // returns the number of time-step intervals
   virtual IntervalIndexType GetIntervalCount() const = 0;

   // returns the timeline event index at the start of the interval
   virtual EventIndexType GetStartEvent(IntervalIndexType idx) const = 0; 

   // returns the timeline event index at the end of the interval
   virtual EventIndexType GetEndEvent(IntervalIndexType idx) const = 0; 

   // returns the specified time for an interval
   virtual Float64 GetTime(IntervalIndexType idx,pgsTypes::IntervalTimeType timeType) const = 0;

   // returns the duration of the interval
   virtual Float64 GetDuration(IntervalIndexType idx) const = 0;

   // returns the interval description
   virtual std::_tstring GetDescription(IntervalIndexType idx) const = 0;

   // returns the index of the first interval that starts with the specified event index
   virtual IntervalIndexType GetInterval(EventIndexType eventIdx) const = 0;

   // returns the index of the interval when a pier is erected
   virtual IntervalIndexType GetErectPierInterval(PierIndexType pierIdx) const = 0;

   // returns the index of the interval when the prestressing strands are stressed for the first segment 
   // that is constructed for this girder
   virtual IntervalIndexType GetFirstStressStrandInterval(const CGirderKey& girderKey) const = 0;

   // returns the index of the interval when the prestressing strands are stressed for the last segment 
   // that is constructed for this girder
   virtual IntervalIndexType GetLastStressStrandInterval(const CGirderKey& girderKey) const = 0;

   // returns the index of the interval when the prestressing strands are stressed
   virtual IntervalIndexType GetStressStrandInterval(const CSegmentKey& segmentKey) const = 0;

   // returns the index of the interval when the prestressing strands are released for the first segment 
   // that is constructed for this girder
   virtual IntervalIndexType GetFirstPrestressReleaseInterval(const CGirderKey& girderKey) const = 0;

   // returns the index of the interval when the prestressing strands are released for the last segment 
   // that is constructed for this girder
   virtual IntervalIndexType GetLastPrestressReleaseInterval(const CGirderKey& girderKey) const = 0;

   // returns the index of the interval when the prestressing is release
   // to the girder (girder has reached release strength).
   virtual IntervalIndexType GetPrestressReleaseInterval(const CSegmentKey& segmentKey) const = 0;

   // returns the index of the interval when a segment is lifted from the casting bed
   virtual IntervalIndexType GetLiftSegmentInterval(const CSegmentKey& segmentKey) const = 0;

   virtual IntervalIndexType GetFirstLiftSegmentInterval(const CGirderKey& girderKey) const = 0;
   virtual IntervalIndexType GetLastLiftSegmentInterval(const CGirderKey& girderKey) const = 0;

   // returns the index of the interval when the segments are place in storage
   virtual IntervalIndexType GetStorageInterval(const CSegmentKey& segmentKey) const = 0;

   virtual IntervalIndexType GetFirstStorageInterval(const CGirderKey& girderKey) const = 0;
   virtual IntervalIndexType GetLastStorageInterval(const CGirderKey& girderKey) const = 0;

   // returns the index of the interval when a segment is hauled to the bridge site
   virtual IntervalIndexType GetHaulSegmentInterval(const CSegmentKey& segmentKey) const = 0;

   // returns true if a segment is hauled during this interval
   virtual bool IsHaulSegmentInterval(IntervalIndexType intervalIdx) const = 0;

   // returns the index of the interval when the first precast segment for a specified girder is erected
   virtual IntervalIndexType GetFirstSegmentErectionInterval(const CGirderKey& girderKey) const = 0;

   // returns the index of the interval when the last precast segment for a specified girder is erected
   virtual IntervalIndexType GetLastSegmentErectionInterval(const CGirderKey& girderKey) const = 0;

   // returns the index of the interval when a specific segment is erected
   virtual IntervalIndexType GetErectSegmentInterval(const CSegmentKey& segmentKey) const = 0;

   // returns true if a segment is erected in the specified interval
   virtual bool IsSegmentErectionInterval(IntervalIndexType intervalIdx) const = 0;
   virtual bool IsSegmentErectionInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx) const = 0;

   // returns the index of the interval when temporary strands are stressed for a specific segment
   // returns INVALID_INDEX if the segment does not have temporary strands
   virtual IntervalIndexType GetTemporaryStrandStressingInterval(const CSegmentKey& segmentKey) const = 0;

   // returns the index of the interval when temporary strands are installed for a specific segment
   // returns INVALID_INDEX if the segment does not have temporary strands
   virtual IntervalIndexType GetTemporaryStrandInstallationInterval(const CSegmentKey& segmentKey) const = 0;

   // returns the index of the interval when temporary strands are removed from a specific segment
   // returns INVALID_INDEX if the segment does not have temporary strands
   virtual IntervalIndexType GetTemporaryStrandRemovalInterval(const CSegmentKey& segmentKey) const = 0;

   // returns the index of the interval when a closure joint is cast
   virtual IntervalIndexType GetCastClosureJointInterval(const CClosureKey& closureKey) const = 0;

   // retuns the interval when a closure joint becomes composite with the girder
   virtual IntervalIndexType GetCompositeClosureJointInterval(const CClosureKey& closureKey) const = 0;

   // returns the interval when the first closure joint becomes composite for this girder
   virtual IntervalIndexType GetFirstCompositeClosureJointInterval(const CGirderKey& girderKey) const = 0;

   // returns the interval when the last closure joint becomes composite for this girder
   virtual IntervalIndexType GetLastCompositeClosureJointInterval(const CGirderKey& girderKey) const = 0;

   // returns the interval when continuity occurs at a pier
   virtual void GetContinuityInterval(PierIndexType pierIdx,IntervalIndexType* pBack,IntervalIndexType* pAhead) const = 0;

   // returns the index of the interval when intermediate diaphragms are cast
   virtual IntervalIndexType GetCastIntermediateDiaphragmsInterval() const = 0;

   // returns the index of the interval when intermediate diaphragms become composite
   virtual IntervalIndexType GetCompositeIntermediateDiaphragmsInterval() const = 0;

   // returns the index of the interval when longitudinal joints are cast
   virtual IntervalIndexType GetCastLongitudinalJointInterval() const = 0;

   // returns the index of the interval when longitudinal joints become composite
   virtual IntervalIndexType GetCompositeLongitudinalJointInterval() const = 0;

   // returns the index of the interval when the deck and diaphragms are cast
   virtual IntervalIndexType GetCastDeckInterval(IndexType castingRegionIdx) const = 0;

   // returns the index of the interval when the first deck casting region is cast
   virtual IntervalIndexType GetFirstCastDeckInterval() const = 0;

   // returns the index of the interval when the last deck casting region is cast
   virtual IntervalIndexType GetLastCastDeckInterval() const = 0;

   // returns the index of the interval when the deck becomes composite
   virtual IntervalIndexType GetCompositeDeckInterval(IndexType castingRegionIdx) const = 0;

   // returns the index of the interval when the first deck casting region becomes composite
   virtual IntervalIndexType GetFirstCompositeDeckInterval() const = 0;

   // returns the index of the interval when the last deck casting region becomes composite
   virtual IntervalIndexType GetLastCompositeDeckInterval() const = 0;

   // returns the interval when shear keys are cast
   virtual IntervalIndexType GetCastShearKeyInterval() const = 0;

   // returns the interval when construction loads are applied
   virtual IntervalIndexType GetConstructionLoadInterval() const = 0;

   // returns the index of the interval when live load is first
   // applied to the structure. it is assumed that live
   // load can be applied to the structure at this interval and all
   // intervals thereafter
   virtual IntervalIndexType GetLiveLoadInterval() const = 0;

   // returns the index of the interval when load rating calculations are performed
   virtual IntervalIndexType GetLoadRatingInterval() const = 0;

   // returns the index of the interval when the overlay is
   // installed. 
   virtual IntervalIndexType GetOverlayInterval() const = 0;

   // returns the index of the interval when the railing system is constructed
   virtual IntervalIndexType GetInstallRailingSystemInterval() const = 0;

   // returns the index of the interval when plant installed segment tendons are tensioned for the specified segment
   virtual IntervalIndexType GetStressSegmentTendonInterval(const CSegmentKey& segmentKey) const = 0;

   // returns the index of the first interval when segment tendon stressing occurs
   virtual IntervalIndexType GetFirstSegmentTendonStressingInterval(const CGirderKey& girderKey) const = 0;

   // returns the index of the last interval when segment tendon stressing occurs
   virtual IntervalIndexType GetLastSegmentTendonStressingInterval(const CGirderKey& girderKey) const = 0;

   // returns the index of the first interval when tendon stressing occurs
   virtual IntervalIndexType GetFirstGirderTendonStressingInterval(const CGirderKey& girderKey) const = 0;

   // returns the index of the last interval when tendon stressing occurs
   virtual IntervalIndexType GetLastGirderTendonStressingInterval(const CGirderKey& girderKey) const = 0;

   // returns the index of the interval when the specified field installed girder tendon is stressed
   virtual IntervalIndexType GetStressGirderTendonInterval(const CGirderKey& girderKey,DuctIndexType ductIdx) const = 0;

   // returns true if a girder tendon is stressed during the specified interval
   virtual bool IsGirderTendonStressingInterval(const CGirderKey& girderKey, IntervalIndexType intervalIdx) const = 0;

   // returns true if a segment tendon is stressed during the specified interval
   virtual bool IsSegmentTendonStressingInterval(const CSegmentKey& segmentKey, IntervalIndexType intervalIdx) const = 0;

   // returns true if there is a change in prestressing during the specified interval
   virtual bool IsStressingInterval(const CGirderKey& girderKey,IntervalIndexType intervalIdx) const = 0;

   // returns the interval index when a temporary support is erected
   virtual IntervalIndexType GetTemporarySupportErectionInterval(SupportIndexType tsIdx) const = 0;

   // returns the interval index when a temporary support is removed
   virtual IntervalIndexType GetTemporarySupportRemovalInterval(SupportIndexType tsIdx) const = 0;

   // returns a vector of removal intervals for all the temporary supports in the specified group
   virtual std::vector<IntervalIndexType> GetTemporarySupportRemovalIntervals(GroupIndexType groupIdx) const = 0;

   // returns a vector of intervals when user defined loads are applied to this girder
   virtual std::vector<IntervalIndexType> GetUserDefinedLoadIntervals(const CSpanKey& spanKey) const = 0;

   // returns a vector of intervals when user defined loads are applied to this girder
   virtual std::vector<IntervalIndexType> GetUserDefinedLoadIntervals(const CSpanKey& spanKey,pgsTypes::ProductForceType pfType) const = 0;

   // returns true if a user defined load is applied in the specified interval
   virtual bool IsUserDefinedLoadingInterval(IntervalIndexType intervalIdx) const = 0;

   // Returns the interval when user defined loads are applied to the noncomposite section
   virtual IntervalIndexType GetNoncompositeUserLoadInterval() const = 0;

   // Returns the interval when user defined loads are applied to the compostie section
   virtual IntervalIndexType GetCompositeUserLoadInterval() const = 0;

   // returns the last interval when the girder is a non-composite section
   // If the girder is never made composite with other concrete elements, the 
   // interval of diaphragm casting is returned;
   virtual IntervalIndexType GetLastNoncompositeInterval() const = 0;

   // returns the interval when the last concrete element is made composite with the girder
   // Typically, this is the interval when the deck becomes composite. However, for no deck
   // bridges, it could be the interval when longitudinal joints become composite or
   // when intermediate diaphragms are composite if there are no concrete elements composite with the girder
   virtual IntervalIndexType GetLastCompositeInterval() const = 0;

   // Interval of "the" Geometry Control Event (GCE)
   virtual IntervalIndexType GetGeometryControlInterval() const = 0;

   // All intervals to report finished elevations
   virtual std::vector<IntervalIndexType> GetReportingGeometryControlIntervals() const = 0;

   // All intervals whem finished elevations are to be spec check'd
   virtual std::vector<IntervalIndexType> GetSpecCheckGeometryControlIntervals() const = 0;
};
