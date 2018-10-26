///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

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
   // Status for Fillet geometry check 
   enum HaunchGeometryStatusType { hgNA, hgPass, hgInsufficient, hgExcessive };

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
   void SetRequiredSlabOffset(Float64 reqd);
   Float64 GetRequiredSlabOffset() const;
   void SetProvidedSlabOffset(Float64 provided);
   Float64 GetProvidedSlabOffset() const;
   void SetSlabOffsetApplicability(bool bSet);
   bool IsSlabOffsetApplicable() const;
   void SetSlabOffsetWarningTolerance(Float64 tol);
   Float64 GetSlabOffsetWarningTolerance() const;
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
   void SetHaunchAtBearingCLsApplicability(bool bSet);
   bool IsHaunchAtBearingCLsApplicable() const;
   bool HaunchAtBearingCLsPassed() const;

   // Bottom flange clearance
   void SetBottomFlangeClearanceApplicability(bool bSet);
   bool IsBottomFlangeClearanceApplicable() const;
   void SetBottomFlangeClearanceParameters(Float64 C,Float64 Cmin);
   void GetBottomFlangeClearanceParameters(Float64* pC,Float64* pCmin) const;
   bool BottomFlangeClearancePassed() const;

   // Fillet dimension (haunch) geometry check
   void SetHaunchGeometryCheckApplicability(bool bSet);
   bool IsHaunchGeometryCheckApplicable() const;
   void SetUserInputFillet(Float64 value);
   Float64 GetUserInputFillet() const;
   void SetComputedFillet(Float64 value);
   Float64 GetComputedFillet() const;
   void SetHaunchGeometryTolerance(Float64 value);
   Float64 GetHaunchGeometryTolerance() const;
   HaunchGeometryStatusType HaunchGeometryStatus() const;
   bool HaunchGeometryPassed() const;

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
   virtual void MakeAssignment(const pgsSpanConstructabilityArtifact& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   SpanIndexType m_Span; // accessible only to our friend pgsConstructabilityArtifact

   Float64 m_Provided; // The actual slab offset
   Float64 m_Required; // The required required slab offset
   Float64 m_SlabOffsetWarningTolerance; // if offset is greater than tolerance + required, issue a warning
   bool m_bCheckStirrupLength;
   Float64 m_ProvidedFillet;
   Float64 m_MinimumRequiredFillet;
   bool m_bIsSlabOffsetApplicable;

   Float64 m_ProvidedAtBearingCLs; // The actual slab offset
   Float64 m_RequiredAtBearingCLs; // The required required slab offset
   bool m_bIsHaunchAtBearingCLsApplicable;

   bool m_bIsBottomFlangeClearanceApplicable;
   Float64 m_C;
   Float64 m_Cmin;

   Float64 m_UserInputFillet;
   Float64 m_ComputedFillet;
   Float64 m_HaunchGeometryTolerance;
   bool m_bIsHaunchGeometryCheckApplicable;

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

   bool IsBottomFlangeClearanceApplicable() const;
   bool BottomFlangeClearancePassed() const;

   bool MinimumFilletPassed() const;
   bool HaunchGeometryPassed() const;

   bool CheckStirrupLength() const;

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
   virtual void MakeAssignment(const pgsConstructabilityArtifact& rOther);

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
