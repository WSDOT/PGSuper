///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
   pgsSpanConstructabilityArtifact

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
class PGSEXTCLASS pgsSpanConstructabilityArtifact
{
public:
   friend pgsConstructabilityArtifact;

   // GROUP: ENUM
   // Status for A dimension check along the girder. Not for check at bearing CL's
   enum SlabOffsetStatusType { Pass, Fail, Excessive, NA };

   // Status for Haunch geometry check 
   enum HaunchGeometryStatusType { hgNA, hgNAPrintOnly, hgPass, hgInsufficient, hgExcessive };

   // Applicability for A dimension check at CL bearing
   enum SlabOffsetBearingCLApplicabilityType { sobappYes, sobappNA, sobappNAPrintOnly };

   // GROUP: LIFECYCLE

   //------------------------------------------------------------------------
   // Default constructor
   pgsSpanConstructabilityArtifact();

   //------------------------------------------------------------------------
   // Copy constructor
   pgsSpanConstructabilityArtifact(const pgsSpanConstructabilityArtifact& rOther);

   //------------------------------------------------------------------------
   // Destructor
   virtual ~pgsSpanConstructabilityArtifact();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   pgsSpanConstructabilityArtifact& operator = (const pgsSpanConstructabilityArtifact& rOther);

   // GROUP: OPERATIONS

   // GROUP: ACCESS
   SpanIndexType GetSpan() const
   {
      return m_Span;
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

   // Slab offset check at bearing centerlines
   void SetRequiredHaunchAtBearingCLs(Float64 reqd);
   Float64 GetRequiredHaunchAtBearingCLs() const;
   void SetProvidedHaunchAtBearingCLs(Float64 provided);
   Float64 GetProvidedHaunchAtBearingCLs() const;
   void SetHaunchAtBearingCLsApplicability(SlabOffsetBearingCLApplicabilityType bSet);
   SlabOffsetBearingCLApplicabilityType GetHaunchAtBearingCLsApplicability() const;
   bool HaunchAtBearingCLsPassed() const;

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
   // (used only for no-deck bridges)
   void SetFinishedElevationApplicability(bool bSet);
   bool GetFinishedElevationApplicability() const;
   void SetFinishedElevationTolerance(Float64 tol);
   Float64 GetFinishedElevationTolerance() const;
   void SetMaxFinishedElevation(Float64 station, Float64 offset, const pgsPointOfInterest& poi, Float64 designElevation, Float64 finishedElevation);
   void GetMaxFinishedElevation(Float64* pStation, Float64* pOffset, pgsPointOfInterest* pPoi, Float64* pDesignElevation, Float64* pFinishedElevation) const;
   bool FinishedElevationPassed() const;

   bool Passed() const;

   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const pgsSpanConstructabilityArtifact& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const pgsSpanConstructabilityArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   SpanIndexType m_Span; // accessible only to our friend pgsConstructabilityArtifact

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

   Float64 m_ProvidedAtBearingCLs; // The actual slab offset
   Float64 m_RequiredAtBearingCLs; // The required required slab offset
   SlabOffsetBearingCLApplicabilityType m_HaunchAtBearingCLsApplicable;

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
   Float64 m_FinishedElevationTolerance;
   Float64 m_Station;
   Float64 m_Offset;
   pgsPointOfInterest m_Poi;
   Float64 m_DesignElevation; // this is the elevation determined by the profile
   Float64 m_FinishedElevation; // this is the top of girder elevation 

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

   // List of spans. returns number of spans owned
   SpanIndexType GetSpans(SpanIndexType* pStartSpanIdx, SpanIndexType* pEndSpanIdx) const;

   // 
   // Span artifacts we own
   void ClearArtifacts();
   void AddSpanArtifact(SpanIndexType span, const pgsSpanConstructabilityArtifact& artifact);
   // Get artifact - Pointer invalid if collection is changed. Do not hold onto pointer
   const pgsSpanConstructabilityArtifact* GetSpanArtifact(SpanIndexType span) const;

   // Calls down into spans for global checks
   bool IsSlabOffsetApplicable() const;
   bool SlabOffsetPassed() const;

   bool IsHaunchAtBearingCLsApplicable() const;
   bool HaunchAtBearingCLsPassed() const;

   bool IsPrecamberApplicable() const;
   bool PrecamberPassed() const;

   bool IsBottomFlangeClearanceApplicable() const;
   bool BottomFlangeClearancePassed() const;

   bool MinimumFilletPassed() const;
   bool HaunchGeometryPassed() const;

   bool CheckStirrupLength() const;

   bool IsFinishedElevationApplicable() const;
   bool FinishedElevationPassed() const;


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
   std::vector<pgsSpanConstructabilityArtifact> m_SpanArtifacts;
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_PGSEXT_CONSTRUCTABILITYARTIFACT_H_
