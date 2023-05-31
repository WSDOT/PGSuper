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

#ifndef INCLUDED_PGSEXT_CONSTRUCTABILITYARTIFACT_H_
#define INCLUDED_PGSEXT_CONSTRUCTABILITYARTIFACT_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

#include <PgsExt\PointOfInterest.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   pgsSegmentConstructabilityArtifact

   Artifact for constructability checks


DESCRIPTION
   Artifact for constructability checks.
   Constructability consists of
   1) "A" dimension check + warning if stirrups may be short
   2) Girder Global Rollover Stability for non-plumb girders

LOG
   rab : 12.01.1998 : Created file
*****************************************************************************/
class pgsConstructabilityArtifact;

// Artifact for an individual span along a girderline
class PGSEXTCLASS pgsSegmentConstructabilityArtifact
{
public:
   friend pgsConstructabilityArtifact;

   // GROUP: ENUM
   // Status for A dimension check along the girder. Not for check at bearing CL's
   enum SlabOffsetStatusType { Pass, Fail, Excessive, NA };

   // Status for Haunch geometry check 
   enum HaunchGeometryStatusType { hgNA, hgNAPrintOnly, hgPass, hgInsufficient, hgExcessive };

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsSegmentConstructabilityArtifact(const CSegmentKey& segmentKey);

   //------------------------------------------------------------------------
   // Copy constructor
   pgsSegmentConstructabilityArtifact(const pgsSegmentConstructabilityArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsSegmentConstructabilityArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsSegmentConstructabilityArtifact& operator = (const pgsSegmentConstructabilityArtifact& rOther);

   // GROUP: OPERATIONS

   // GROUP: ACCESS
   SegmentIndexType GetSegment() const
   {
      return m_SegmentKey.segmentIndex;
   }

   //------------------------------------------------------------------------
   // Slab offset check along girder
   void SetSlabOffsetApplicability(bool bSet);
   bool IsSlabOffsetApplicable() const;
   void SetProvidedSlabOffset(Float64 startA, Float64 endA);
   void GetProvidedSlabOffset(Float64* pStartA, Float64* pEndA) const;
   bool AreSlabOffsetsSameAtEnds() const;
   // Required slab offset valid only for case when start and end values are equal
   void SetRequiredSlabOffset(Float64 reqd);
   Float64 GetRequiredSlabOffset() const;
   // least haunch depth and its location measured from left end of girder
   void SetLeastHaunchDepth(Float64 location, Float64 leastA);
   void GetLeastHaunchDepth(Float64* pLocation, Float64* pLeastA) const;
   void SetExcessSlabOffsetWarningTolerance(Float64 tol);
   Float64 GetExcessSlabOffsetWarningTolerance() const;
   SlabOffsetStatusType SlabOffsetStatus() const;
   bool SlabOffsetPassed() const;

   // Check for mininum required fillet defined in girder library
   void SetRequiredMinimumFillet(Float64 reqd);
   Float64 GetRequiredMinimumFillet() const;
   void SetProvidedFillet(Float64 provided);
   Float64 GetProvidedFillet() const;
   bool MinimumFilletPassed() const;

   // do stirrup lengths need to be checked because of the roadway geometry
   // (excessive haunch along the girder could lead to short stirrups in the field.
   //  special detailing may be required)
   void CheckStirrupLength(bool bCheck);
   bool CheckStirrupLength() const;

   // Precamber check
   void SetPrecamberApplicability(bool bSet);
   bool IsPrecamberApplicable() const;
   void SetPrecamber(const CSegmentKey& segmentKey, Float64 limit, Float64 value);
   void GetPrecamber(const CSegmentKey& segmentKey, Float64* pLimit, Float64* pValue) const;
   bool PrecamberPassed(const CSegmentKey& segmentKey) const;
   bool PrecamberPassed() const;

   // Bottom flange clearance
   void SetBottomFlangeClearanceApplicability(bool bSet);
   bool IsBottomFlangeClearanceApplicable() const;
   void SetBottomFlangeClearanceParameters(Float64 C,Float64 Cmin);
   void GetBottomFlangeClearanceParameters(Float64* pC,Float64* pCmin) const;
   bool BottomFlangeClearancePassed() const;

   // haunch geometry check
   void SetHaunchGeometryCheckApplicability(bool bSet);
   bool IsHaunchGeometryCheckApplicable() const;
   void SetAssumedExcessCamber(Float64 value);
   Float64 GetAssumedExcessCamber() const;
   void SetAssumedMinimumHaunchDepth(Float64 value); // minimum haunch along girder used to compute parabolic load
   Float64 GetAssumedMinimumHaunchDepth() const;
   void SetComputedExcessCamber(Float64 value);
   Float64 GetComputedExcessCamber() const;
   void SetHaunchGeometryTolerance(Float64 value);
   Float64 GetHaunchGeometryTolerance() const;
   HaunchGeometryStatusType HaunchGeometryStatus() const;
   bool HaunchGeometryPassed() const;

   // Finished Elevation Check
   // (used only for no-deck bridges, and direct haunch input)
   void SetFinishedElevationApplicability(bool bSet);
   bool GetFinishedElevationApplicability() const;
   void SetFinishedElevationControllingInterval(IntervalIndexType interval);
   IntervalIndexType GetFinishedElevationControllingInterval() const;
   void SetFinishedElevationTolerance(Float64 tol);
   Float64 GetFinishedElevationTolerance() const;
   void SetMaxFinishedElevation(Float64 station, Float64 offset, const pgsPointOfInterest& poi, Float64 designElevation, Float64 finishedElevation);
   void GetMaxFinishedElevation(Float64* pStation, Float64* pOffset, pgsPointOfInterest* pPoi, Float64* pDesignElevation, Float64* pFinishedElevation) const;
   bool FinishedElevationPassed() const;

   // Min haunch depth along girder vs fillet check.
   // (only for direct haunch input)
   void SetMinimumHaunchDepthApplicability(bool bSet);
   bool GetMinimumHaunchDepthApplicability() const;
   void SetMinimumHaunchDepthControllingInterval(IntervalIndexType interval);
   IntervalIndexType GetMinimumHaunchDepthControllingInterval() const;
   void SetMinimumAllowableHaunchDepth(Float64 haunchDepth);
   Float64 GetMinimumAllowableHaunchDepth() const;
   void SetMinimumHaunchDepth(Float64 station,Float64 offset,const pgsPointOfInterest& poi,Float64 MinimumHaunchDepth);
   void GetMinimumHaunchDepth(Float64* pStation,Float64* pOffset,pgsPointOfInterest* pPoi,Float64* pMinimumHaunchDepth) const;
   bool MinimumHaunchDepthPassed() const;

   bool Passed() const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsSegmentConstructabilityArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsSegmentConstructabilityArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   CSegmentKey m_SegmentKey;
   Float64 m_ProvidedStart; // The actual slab offsets
   Float64 m_ProvidedEnd;
   Float64 m_Required; // The required required slab offset
   Float64 m_SlabOffsetWarningTolerance; // if offset is greater than tolerance + required, issue a warning
   bool m_bCheckStirrupLength;
   Float64 m_ProvidedFillet;
   Float64 m_LeastHaunch;
   Float64 m_LeastHaunchLocation;
   Float64 m_MinimumRequiredFillet;
   bool m_bIsSlabOffsetApplicable;

   bool m_bIsPrecamberApplicable;
   std::map<CSegmentKey, std::pair<Float64, Float64>> m_Precamber; // first is precamber limit, second is precamber value

   bool m_bIsBottomFlangeClearanceApplicable;
   Float64 m_C;
   Float64 m_Cmin;

   Float64 m_ComputedExcessCamber;
   Float64 m_AssumedExcessCamber;
   Float64 m_AssumedMinimumHaunchDepth;
   Float64 m_HaunchGeometryTolerance;
   bool m_bIsHaunchGeometryCheckApplicable;

   // Finished elevation check data
   bool m_bIsFinishedElevationApplicable;
   IntervalIndexType m_FinishedElevationControllingInterval;
   Float64 m_FinishedElevationTolerance;
   Float64 m_FinishedElevationStation;
   Float64 m_FinishedElevationOffset;
   pgsPointOfInterest m_FinishedElevationPoi;
   Float64 m_DesignElevation; // this is the elevation determined by the profile
   Float64 m_FinishedElevation; // this is the top of girder elevation 

   // Minimum haunch depth vs fillet check (direct haunch input only)
   bool m_bIsMinimumHaunchCheckApplicable;
   IntervalIndexType m_MinimumHaunchCheckControllingInterval;
   pgsPointOfInterest m_MinimumHaunchPoi;
   Float64 m_MinimumAllowableHaunchDepth;
   Float64 m_MinimumHaunchStation;
   Float64 m_MinimumHaunchOffset;
   Float64 m_MinimumHaunchDepth; // minimum haunch depth along girder 

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};


// Artifacts for all spans along a girderline
class PGSEXTCLASS pgsConstructabilityArtifact
{
public:
   // Applicability for haunch check at CL bearing
   enum HaunchBearingCLApplicabilityType { hbcAppYes,hbcAppNA,hbcAppNAPrintOnly };

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsConstructabilityArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsConstructabilityArtifact(const pgsConstructabilityArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsConstructabilityArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsConstructabilityArtifact& operator = (const pgsConstructabilityArtifact& rOther);

   // Segment artifacts we own
   void ClearArtifacts();
   void AddSegmentArtifact(const pgsSegmentConstructabilityArtifact& artifact);
   // Get artifact - Pointer invalid if collection is changed. Do not hold onto pointer
   const pgsSegmentConstructabilityArtifact& GetSegmentArtifact(SegmentIndexType segIdx) const;

   // Calls down into spans for global checks
   bool IsSlabOffsetApplicable() const;
   bool SlabOffsetPassed() const;

   bool IsPrecamberApplicable() const;
   bool PrecamberPassed() const;

   bool IsBottomFlangeClearanceApplicable() const;
   bool BottomFlangeClearancePassed() const;

   bool MinimumFilletPassed() const;
   bool HaunchGeometryPassed() const;

   bool CheckStirrupLength() const;

   bool IsFinishedElevationApplicable() const;
   bool FinishedElevationPassed() const;

   bool MinimumHaunchDepthPassed() const;

   // Haunch depth check at bearing centerlines at ends of group
   bool IsHaunchAtBearingCLsApplicable() const;
   void SetHaunchBearingCLApplicability(HaunchBearingCLApplicabilityType bSet);
   HaunchBearingCLApplicabilityType GetHaunchBearingCLApplicability() const;
   void SetRequiredHaunchAtBearingCLs(Float64 reqd);
   Float64 GetRequiredHaunchAtBearingCLs() const;
   void SetProvidedHaunchAtBearingCLs(Float64 startEnd,Float64 endEnd);
   void GetProvidedHaunchAtBearingCLs(Float64* pStartEnd,Float64* pEndEnd) const;
   bool HaunchAtBearingCLsPassed() const;

   bool Passed() const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsConstructabilityArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsConstructabilityArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   std::vector<pgsSegmentConstructabilityArtifact> m_SegmentArtifacts;

   std::vector<Float64> m_ProvidedAtBearingCLs{ 0.0,0.0 }; // The haunch depth at start and end of group
   Float64 m_RequiredAtBearingCLs; // The required haunch depth
   HaunchBearingCLApplicabilityType m_HaunchBearingCLApplicability;
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_PGSEXT_CONSTRUCTABILITYARTIFACT_H_
