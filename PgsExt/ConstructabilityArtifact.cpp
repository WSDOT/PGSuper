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

#include <PgsExt\PgsExtLib.h>
#include <PgsExt\ConstructabilityArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsSpanConstructabilityArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsSpanConstructabilityArtifact::pgsSpanConstructabilityArtifact():
m_bIsSlabOffsetApplicable(false)
{
   m_ProvidedStart = 0;
   m_ProvidedEnd = 0;
   m_Required = 0;
   m_SlabOffsetWarningTolerance = ::ConvertToSysUnits(0.25,unitMeasure::Inch); // WSDOT standard
   m_MinimumRequiredFillet = 0;
   m_ProvidedFillet = 0;

   m_LeastHaunch = 0;
   m_LeastHaunchLocation = 0;

   m_ProvidedAtBearingCLs  = 0;
   m_RequiredAtBearingCLs  = 0;
   m_HaunchAtBearingCLsApplicable = sobappNA;

   m_bIsPrecamberApplicable = false;
  
   m_bIsBottomFlangeClearanceApplicable = false;
   m_C = 0;
   m_Cmin = 0;

   m_ComputedExcessCamber = 0;
   m_AssumedExcessCamber = 0;
   m_AssumedMinimumHaunchDepth = Float64_Max;
   m_HaunchGeometryTolerance = 0;
   m_bIsHaunchGeometryCheckApplicable = false;


   m_bIsFinishedElevationApplicable = false;
   m_FinishedElevationTolerance = 0;
   m_Station = 0;
   m_Offset = 0;
   m_DesignElevation = 0;
   m_FinishedElevation = 0;
}

pgsSpanConstructabilityArtifact::pgsSpanConstructabilityArtifact(const pgsSpanConstructabilityArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsSpanConstructabilityArtifact::~pgsSpanConstructabilityArtifact()
{
}

//======================== OPERATORS  =======================================
pgsSpanConstructabilityArtifact& pgsSpanConstructabilityArtifact::operator=(const pgsSpanConstructabilityArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void pgsSpanConstructabilityArtifact::SetProvidedSlabOffset(Float64 startA, Float64 endA)
{
   m_ProvidedStart = startA;
   m_ProvidedEnd   = endA;
}

void pgsSpanConstructabilityArtifact::GetProvidedSlabOffset(Float64* pStartA, Float64* pEndA) const
{
   *pStartA = m_ProvidedStart;
   *pEndA   = m_ProvidedEnd;
}

bool pgsSpanConstructabilityArtifact::AreSlabOffsetsSameAtEnds() const
{
   return IsEqual(m_ProvidedStart,m_ProvidedEnd);
}

void pgsSpanConstructabilityArtifact::SetRequiredSlabOffset(Float64 reqd)
{
   m_Required = reqd;
}

Float64 pgsSpanConstructabilityArtifact::GetRequiredSlabOffset() const
{
   return m_Required;
}

void pgsSpanConstructabilityArtifact::SetExcessSlabOffsetWarningTolerance(Float64 val)
{
   m_SlabOffsetWarningTolerance = val;
}

Float64 pgsSpanConstructabilityArtifact::GetExcessSlabOffsetWarningTolerance() const
{
   return m_SlabOffsetWarningTolerance;
}

void pgsSpanConstructabilityArtifact::SetRequiredMinimumFillet(Float64 reqd)
{
   m_MinimumRequiredFillet = reqd;
}

Float64 pgsSpanConstructabilityArtifact::GetRequiredMinimumFillet() const
{
   return m_MinimumRequiredFillet;
}

void pgsSpanConstructabilityArtifact::SetProvidedFillet(Float64 provided)
{
   m_ProvidedFillet = provided;
}

Float64 pgsSpanConstructabilityArtifact::GetProvidedFillet() const
{
   return m_ProvidedFillet;
}

bool pgsSpanConstructabilityArtifact::MinimumFilletPassed() const
{
   // min fillet uses same applicability as slab offset check
   if (m_bIsSlabOffsetApplicable)
   {
   return m_ProvidedFillet + TOLERANCE > m_MinimumRequiredFillet;
   }
   else
   {
      return true;
   }
}

void pgsSpanConstructabilityArtifact::SetSlabOffsetApplicability(bool bSet)
{
   m_bIsSlabOffsetApplicable = bSet;
}

bool pgsSpanConstructabilityArtifact::IsSlabOffsetApplicable() const
{
   return m_bIsSlabOffsetApplicable;
}

void pgsSpanConstructabilityArtifact::SetLeastHaunchDepth(Float64 location, Float64 leastA)
{
   m_LeastHaunchLocation = location;
   m_LeastHaunch = leastA;
}

void pgsSpanConstructabilityArtifact::GetLeastHaunchDepth(Float64* pLocation, Float64* pLeastA) const
{
   *pLocation = m_LeastHaunchLocation;
   *pLeastA = m_LeastHaunch;
}

pgsSpanConstructabilityArtifact::SlabOffsetStatusType pgsSpanConstructabilityArtifact::SlabOffsetStatus() const
{
   if (!m_bIsSlabOffsetApplicable)
   {
      return NA;
   }

   if (AreSlabOffsetsSameAtEnds())
   {
      if ( IsEqual(m_ProvidedStart,m_Required) )
      {
         return Pass;
      }

      if ( m_ProvidedStart < m_Required )
      {
         return Fail;
      }

      if ( (m_Required + m_SlabOffsetWarningTolerance) < m_ProvidedStart )
      {
         return Excessive;
      }

      return Pass;
   }
   else
   {
      // slab offsets different at ends. Use Least haunch vs fillet dimension as test
      if (m_LeastHaunch > m_ProvidedFillet + m_SlabOffsetWarningTolerance)
      {
         return Excessive;
      }
      else if (m_LeastHaunch > m_ProvidedFillet)
      {
         return Pass;
      }
      else
      {
         return Fail;
      }
   }
}

bool pgsSpanConstructabilityArtifact::SlabOffsetPassed() const
{
   return ( SlabOffsetStatus()==Fail ) ? false : true;
}

void pgsSpanConstructabilityArtifact::CheckStirrupLength(bool bCheck)
{
   m_bCheckStirrupLength = bCheck;
}

bool pgsSpanConstructabilityArtifact::CheckStirrupLength() const
{
   return m_bIsSlabOffsetApplicable && m_bCheckStirrupLength;
}

void pgsSpanConstructabilityArtifact::SetProvidedHaunchAtBearingCLs(Float64 provided)
{
   m_ProvidedAtBearingCLs = provided;
}

Float64 pgsSpanConstructabilityArtifact::GetProvidedHaunchAtBearingCLs() const
{
   return m_ProvidedAtBearingCLs;
}

void pgsSpanConstructabilityArtifact::SetRequiredHaunchAtBearingCLs(Float64 reqd)
{
   m_RequiredAtBearingCLs = reqd;
}

Float64 pgsSpanConstructabilityArtifact::GetRequiredHaunchAtBearingCLs() const
{
   return m_RequiredAtBearingCLs;
}

void pgsSpanConstructabilityArtifact::SetHaunchAtBearingCLsApplicability(SlabOffsetBearingCLApplicabilityType bSet)
{
   m_HaunchAtBearingCLsApplicable = bSet;
}

pgsSpanConstructabilityArtifact::SlabOffsetBearingCLApplicabilityType pgsSpanConstructabilityArtifact::GetHaunchAtBearingCLsApplicability() const
{
   return m_HaunchAtBearingCLsApplicable;
}

bool pgsSpanConstructabilityArtifact::HaunchAtBearingCLsPassed() const
{
   return m_HaunchAtBearingCLsApplicable!=pgsSpanConstructabilityArtifact::sobappYes ||
          m_ProvidedAtBearingCLs+TOLERANCE >= m_RequiredAtBearingCLs;
}

void pgsSpanConstructabilityArtifact::SetPrecamberApplicability(bool bSet)
{
   m_bIsPrecamberApplicable = bSet;
}

bool pgsSpanConstructabilityArtifact::IsPrecamberApplicable() const
{
   return m_bIsPrecamberApplicable;
}

void pgsSpanConstructabilityArtifact::SetPrecamber(const CSegmentKey& segmentKey, Float64 limit, Float64 value)
{
   m_Precamber.insert(std::make_pair(segmentKey, std::make_pair(limit, value)));
}

void pgsSpanConstructabilityArtifact::GetPrecamber(const CSegmentKey& segmentKey, Float64* pLimit, Float64* pValue) const
{
   const auto& found = m_Precamber.find(segmentKey);
   ATLASSERT(found != m_Precamber.end()); // asking for a segment whose precamber record hasn't been recorded
   *pLimit = found->second.first;
   *pValue = found->second.second;;
}

bool pgsSpanConstructabilityArtifact::PrecamberPassed(const CSegmentKey& segmentKey) const
{
   if (m_bIsPrecamberApplicable)
   {
      const auto& found = m_Precamber.find(segmentKey);
      ATLASSERT(found != m_Precamber.end()); // asking for a segment whose precamber record hasn't been recorded
      Float64 limit = found->second.first;
      Float64 value = found->second.second;
      return IsLE(value,limit);
   }

   return true;
}

bool pgsSpanConstructabilityArtifact::PrecamberPassed() const
{
   if (m_bIsPrecamberApplicable)
   {
      for (const auto& entry : m_Precamber)
      {
         Float64 limit = entry.second.first;
         Float64 value = entry.second.second;
         if ( IsGT(limit,fabs(value)) )
         {
            return false;
         }
      }
   }

   return true;
}

void pgsSpanConstructabilityArtifact::SetBottomFlangeClearanceApplicability(bool bSet)
{
   m_bIsBottomFlangeClearanceApplicable = bSet;
}

bool pgsSpanConstructabilityArtifact::IsBottomFlangeClearanceApplicable() const
{
   return m_bIsBottomFlangeClearanceApplicable;
}

void pgsSpanConstructabilityArtifact::SetBottomFlangeClearanceParameters(Float64 C,Float64 Cmin)
{
   m_C = C;
   m_Cmin = Cmin;
}

void pgsSpanConstructabilityArtifact::GetBottomFlangeClearanceParameters(Float64* pC,Float64* pCmin) const
{
   *pC = m_C;
   *pCmin = m_Cmin;
}

bool pgsSpanConstructabilityArtifact::BottomFlangeClearancePassed() const
{
   if ( !m_bIsBottomFlangeClearanceApplicable )
   {
      return true;
   }

   return ::IsGE(m_Cmin,m_C) ? true : false;
}

void pgsSpanConstructabilityArtifact::SetComputedExcessCamber(Float64 value)
{
   m_ComputedExcessCamber = value;
}

Float64 pgsSpanConstructabilityArtifact::GetComputedExcessCamber() const
{
   return m_ComputedExcessCamber;
}

void pgsSpanConstructabilityArtifact::SetAssumedExcessCamber(Float64 value)
{
   m_AssumedExcessCamber = value;
}

Float64 pgsSpanConstructabilityArtifact::GetAssumedExcessCamber() const
{
   return m_AssumedExcessCamber;
}

void pgsSpanConstructabilityArtifact::SetAssumedMinimumHaunchDepth(Float64 value)
{
   m_AssumedMinimumHaunchDepth = value;
}

Float64 pgsSpanConstructabilityArtifact::GetAssumedMinimumHaunchDepth() const
{
   return m_AssumedMinimumHaunchDepth;
}

void pgsSpanConstructabilityArtifact::SetHaunchGeometryCheckApplicability(bool bSet)
{
   m_bIsHaunchGeometryCheckApplicable = bSet;
}

bool pgsSpanConstructabilityArtifact::IsHaunchGeometryCheckApplicable() const
{
   return m_bIsHaunchGeometryCheckApplicable;
}

void pgsSpanConstructabilityArtifact::SetHaunchGeometryTolerance(Float64 value)
{
   m_HaunchGeometryTolerance = value;
}

Float64 pgsSpanConstructabilityArtifact::GetHaunchGeometryTolerance() const
{
   return m_HaunchGeometryTolerance;
}

pgsSpanConstructabilityArtifact::HaunchGeometryStatusType pgsSpanConstructabilityArtifact::HaunchGeometryStatus() const
{
   if (IsHaunchGeometryCheckApplicable())
   {
      // uses same applicability as slab offset check
      if (!m_bIsSlabOffsetApplicable)
      {
         return hgNAPrintOnly;
      }
      else
      {
         if (m_AssumedExcessCamber > m_ComputedExcessCamber + m_HaunchGeometryTolerance)
         {
            return hgExcessive;
         }
         else if (m_AssumedExcessCamber < m_ComputedExcessCamber - m_HaunchGeometryTolerance)
         {
            return hgInsufficient ;
         }
         else
         {
            return hgPass;
         }
      }
   }
   else
   {
      return hgNA;
   }
}

bool pgsSpanConstructabilityArtifact::HaunchGeometryPassed() const
{
   if (!IsHaunchGeometryCheckApplicable())
   {
      return true;
   }
   else
   {
      return HaunchGeometryStatus() == hgPass ||  HaunchGeometryStatus() == hgNAPrintOnly;
   }
}

bool pgsSpanConstructabilityArtifact::Passed() const
{
   if ( !SlabOffsetPassed() )
   {
      return false;
   }

   if (!HaunchAtBearingCLsPassed())
   {
      return false;
   }

   if (!MinimumFilletPassed())
   {
      return false;
   }

   if (!PrecamberPassed())
   {
      return false;
   }

   if (!BottomFlangeClearancePassed())
   {
      return false;
   }

   if (!HaunchGeometryPassed())
   {
      return false;
   }

   if (!FinishedElevationPassed())
   {
      return false;
   }

   return true;
}

 //======================== ACCESS     =======================================

void pgsSpanConstructabilityArtifact::SetFinishedElevationApplicability(bool bSet)
{
   m_bIsFinishedElevationApplicable = bSet;
}

bool pgsSpanConstructabilityArtifact::GetFinishedElevationApplicability() const
{
   return m_bIsFinishedElevationApplicable;
}

void pgsSpanConstructabilityArtifact::SetFinishedElevationTolerance(Float64 tol)
{
   m_FinishedElevationTolerance = tol;
}

Float64 pgsSpanConstructabilityArtifact::GetFinishedElevationTolerance() const
{
   return m_FinishedElevationTolerance;
}

void pgsSpanConstructabilityArtifact::SetMaxFinishedElevation(Float64 station, Float64 offset, const pgsPointOfInterest& poi, Float64 designElevation, Float64 finishedElevation)
{
   m_Station = station;
   m_Offset = offset;
   m_Poi = poi;
   m_DesignElevation = designElevation;
   m_FinishedElevation = finishedElevation;
}

void pgsSpanConstructabilityArtifact::GetMaxFinishedElevation(Float64* pStation, Float64* pOffset, pgsPointOfInterest* pPoi, Float64* pDesignElevation, Float64* pFinishedElevation) const
{
   *pStation = m_Station;
   *pOffset = m_Offset;
   *pPoi = m_Poi;
   *pDesignElevation = m_DesignElevation;
   *pFinishedElevation = m_FinishedElevation;
}

bool pgsSpanConstructabilityArtifact::FinishedElevationPassed() const
{
   return IsEqual(m_DesignElevation, m_FinishedElevation, m_FinishedElevationTolerance) ? true : false;
}

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsSpanConstructabilityArtifact::MakeCopy(const pgsSpanConstructabilityArtifact& rOther)
{
   m_Span     = rOther.m_Span;

   m_ProvidedStart = rOther.m_ProvidedStart;
   m_ProvidedEnd = rOther.m_ProvidedEnd;
   m_Required = rOther.m_Required;
   m_SlabOffsetWarningTolerance = rOther.m_SlabOffsetWarningTolerance;
   m_bCheckStirrupLength = rOther.m_bCheckStirrupLength;
   m_bIsSlabOffsetApplicable = rOther.m_bIsSlabOffsetApplicable;
   m_LeastHaunchLocation = rOther.m_LeastHaunchLocation;
   m_LeastHaunch = rOther.m_LeastHaunch;

   m_MinimumRequiredFillet = rOther.m_MinimumRequiredFillet;
   m_ProvidedFillet = rOther.m_ProvidedFillet;

   m_ProvidedAtBearingCLs  = rOther.m_ProvidedAtBearingCLs;
   m_RequiredAtBearingCLs  = rOther.m_RequiredAtBearingCLs;
   m_HaunchAtBearingCLsApplicable  = rOther.m_HaunchAtBearingCLsApplicable;

   m_bIsPrecamberApplicable = rOther.m_bIsPrecamberApplicable;
   m_Precamber = rOther.m_Precamber;

   m_bIsBottomFlangeClearanceApplicable = rOther.m_bIsBottomFlangeClearanceApplicable;
   m_C = rOther.m_C;
   m_Cmin = rOther.m_Cmin;

   m_ComputedExcessCamber = rOther.m_ComputedExcessCamber;
   m_AssumedExcessCamber = rOther.m_AssumedExcessCamber;
   m_HaunchGeometryTolerance = rOther.m_HaunchGeometryTolerance;
   m_bIsHaunchGeometryCheckApplicable = rOther.m_bIsHaunchGeometryCheckApplicable;
   m_AssumedMinimumHaunchDepth = rOther.m_AssumedMinimumHaunchDepth;

   m_bIsFinishedElevationApplicable = rOther.m_bIsFinishedElevationApplicable;
   m_FinishedElevationTolerance = rOther.m_FinishedElevationTolerance;
   m_Station = rOther.m_Station;
   m_Offset = rOther.m_Offset;
   m_Poi = rOther.m_Poi;
   m_DesignElevation = rOther.m_DesignElevation;
   m_FinishedElevation = rOther.m_FinishedElevation;
}

void pgsSpanConstructabilityArtifact::MakeAssignment(const pgsSpanConstructabilityArtifact& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

/****************************************************************************
CLASS
   pgsConstructabilityArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsConstructabilityArtifact::pgsConstructabilityArtifact()
{
}

pgsConstructabilityArtifact::pgsConstructabilityArtifact(const pgsConstructabilityArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsConstructabilityArtifact::~pgsConstructabilityArtifact()
{
}

//======================== OPERATORS  =======================================
pgsConstructabilityArtifact& pgsConstructabilityArtifact::operator=(const pgsConstructabilityArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

 //======================== ACCESS     =======================================
void pgsConstructabilityArtifact::ClearArtifacts()
{
   m_SpanArtifacts.clear();
}

void pgsConstructabilityArtifact::AddSpanArtifact(SpanIndexType span, const pgsSpanConstructabilityArtifact& artifact)
{
   m_SpanArtifacts.push_back(artifact);

   pgsSpanConstructabilityArtifact& art = m_SpanArtifacts.back();
   art.m_Span = span;
}

const pgsSpanConstructabilityArtifact*  pgsConstructabilityArtifact::GetSpanArtifact(SpanIndexType span) const
{
   for (const auto& artifact : m_SpanArtifacts)
   {
      if (artifact.m_Span == span)
      {
         return &artifact;
      }
   }

   return nullptr;
}

SpanIndexType pgsConstructabilityArtifact::GetSpans(SpanIndexType* pStartSpanIdx, SpanIndexType* pEndSpanIdx) const
{
   SpanIndexType ns = m_SpanArtifacts.size();
   if (ns==0)
   {
      *pStartSpanIdx = INVALID_INDEX;
      *pEndSpanIdx   = INVALID_INDEX;
   }
   else
   {
      SpanIndexType minSpan(MAX_INDEX), maxSpan(0);
      for (const auto& artifact : m_SpanArtifacts)
      {
         minSpan = min(minSpan, artifact.m_Span);
         maxSpan = max(maxSpan, artifact.m_Span);
      }

      *pStartSpanIdx = minSpan;
      *pEndSpanIdx   = maxSpan;
   }

   return ns;
}

bool pgsConstructabilityArtifact::Passed() const
{
   ATLASSERT(!m_SpanArtifacts.empty());
   for (const auto& artf : m_SpanArtifacts)
   {
      if (!artf.Passed())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::IsSlabOffsetApplicable() const
{
   ATLASSERT(!m_SpanArtifacts.empty());
   for (const auto& artf : m_SpanArtifacts)
   {
      if (!artf.IsSlabOffsetApplicable())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::SlabOffsetPassed() const
{
   ATLASSERT(!m_SpanArtifacts.empty());
   for (const auto& artf : m_SpanArtifacts)
   {
      if (!artf.SlabOffsetPassed())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::IsHaunchAtBearingCLsApplicable() const
{
   ATLASSERT(!m_SpanArtifacts.empty());
   for (const auto& artf : m_SpanArtifacts)
   {
      if (artf.GetHaunchAtBearingCLsApplicability() == pgsSpanConstructabilityArtifact::sobappYes)
      {
         return true;
      }
   }

   return false;
}

bool pgsConstructabilityArtifact::HaunchAtBearingCLsPassed() const
{
   ATLASSERT(!m_SpanArtifacts.empty());
   for (const auto& artf : m_SpanArtifacts)
   {
      if (!artf.HaunchAtBearingCLsPassed())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::IsPrecamberApplicable() const
{
   ATLASSERT(!m_SpanArtifacts.empty());
   for (const auto& artf : m_SpanArtifacts)
   {
      if (!artf.IsPrecamberApplicable())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::PrecamberPassed() const
{
   ATLASSERT(!m_SpanArtifacts.empty());
   for (const auto& artf : m_SpanArtifacts)
   {
      if (!artf.PrecamberPassed())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::IsBottomFlangeClearanceApplicable() const
{
   ATLASSERT(!m_SpanArtifacts.empty());
   for (const auto& artf : m_SpanArtifacts)
   {
      if (!artf.IsBottomFlangeClearanceApplicable())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::BottomFlangeClearancePassed() const
{
   ATLASSERT(!m_SpanArtifacts.empty());
   for (const auto& artf : m_SpanArtifacts)
   {
      if (!artf.BottomFlangeClearancePassed())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::MinimumFilletPassed() const
{
   ATLASSERT(!m_SpanArtifacts.empty());
   for (const auto& artf : m_SpanArtifacts)
   {
      if (!artf.MinimumFilletPassed())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::HaunchGeometryPassed() const
{
   ATLASSERT(!m_SpanArtifacts.empty());
   for (const auto& artf : m_SpanArtifacts)
   {
      if (!artf.HaunchGeometryPassed())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::IsFinishedElevationApplicable() const
{
   ATLASSERT(!m_SpanArtifacts.empty());
   for (const auto& artf : m_SpanArtifacts)
   {
      if (!artf.GetFinishedElevationApplicability())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::FinishedElevationPassed() const
{
   ATLASSERT(!m_SpanArtifacts.empty());
   for (const auto& artf : m_SpanArtifacts)
   {
      if (!artf.FinishedElevationPassed())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::CheckStirrupLength() const
{
   ATLASSERT(!m_SpanArtifacts.empty());
   for (const auto& artf : m_SpanArtifacts)
   {
      if (!artf.CheckStirrupLength())
         return false;
   }

   return true;
}

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsConstructabilityArtifact::MakeCopy(const pgsConstructabilityArtifact& rOther)
{
   m_SpanArtifacts = rOther.m_SpanArtifacts;
}

void pgsConstructabilityArtifact::MakeAssignment(const pgsConstructabilityArtifact& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
