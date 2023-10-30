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
#include <PgsExt\ConstructabilityArtifact.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsSegmentConstructabilityArtifact
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
pgsSegmentConstructabilityArtifact::pgsSegmentConstructabilityArtifact(const CSegmentKey& segmentKey):
m_bIsSlabOffsetApplicable(false)
{
   m_SegmentKey = segmentKey;

   m_ProvidedStart = 0;
   m_ProvidedEnd = 0;
   m_Required = 0;
   m_SlabOffsetWarningTolerance = WBFL::Units::ConvertToSysUnits(0.25,WBFL::Units::Measure::Inch); // WSDOT standard
   m_MinimumRequiredFillet = 0;
   m_ProvidedFillet = 0;

   m_LeastHaunch = 0;
   m_LeastHaunchLocation = 0;

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
   m_FinishedElevationControllingInterval = INVALID_INDEX;
   m_FinishedElevationTolerance = 0;
   m_FinishedElevationStation = 0;
   m_FinishedElevationOffset = 0;
   m_DesignElevation = 0;
   m_FinishedElevation = 0;

   m_bIsMinimumHaunchCheckApplicable = false;
   m_MinimumHaunchCheckControllingInterval = INVALID_INDEX;
   m_MinimumAllowableHaunchDepth = 0;
   m_MinimumHaunchStation = 0;
   m_MinimumHaunchOffset = 0;
   m_MinimumHaunchDepth = 0;
}

pgsSegmentConstructabilityArtifact::pgsSegmentConstructabilityArtifact(const pgsSegmentConstructabilityArtifact& rOther)
{
   MakeCopy(rOther);
}

pgsSegmentConstructabilityArtifact::~pgsSegmentConstructabilityArtifact()
{
}

//======================== OPERATORS  =======================================
pgsSegmentConstructabilityArtifact& pgsSegmentConstructabilityArtifact::operator=(const pgsSegmentConstructabilityArtifact& rOther)
{
   if ( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void pgsSegmentConstructabilityArtifact::SetProvidedSlabOffset(Float64 startA, Float64 endA)
{
   m_ProvidedStart = startA;
   m_ProvidedEnd   = endA;
}

void pgsSegmentConstructabilityArtifact::GetProvidedSlabOffset(Float64* pStartA, Float64* pEndA) const
{
   *pStartA = m_ProvidedStart;
   *pEndA   = m_ProvidedEnd;
}

bool pgsSegmentConstructabilityArtifact::AreSlabOffsetsSameAtEnds() const
{
   return IsEqual(m_ProvidedStart,m_ProvidedEnd);
}

void pgsSegmentConstructabilityArtifact::SetRequiredSlabOffset(Float64 reqd)
{
   m_Required = reqd;
}

Float64 pgsSegmentConstructabilityArtifact::GetRequiredSlabOffset() const
{
   return m_Required;
}

void pgsSegmentConstructabilityArtifact::SetExcessSlabOffsetWarningTolerance(Float64 val)
{
   m_SlabOffsetWarningTolerance = val;
}

Float64 pgsSegmentConstructabilityArtifact::GetExcessSlabOffsetWarningTolerance() const
{
   return m_SlabOffsetWarningTolerance;
}

void pgsSegmentConstructabilityArtifact::SetRequiredMinimumFillet(Float64 reqd)
{
   m_MinimumRequiredFillet = reqd;
}

Float64 pgsSegmentConstructabilityArtifact::GetRequiredMinimumFillet() const
{
   return m_MinimumRequiredFillet;
}

void pgsSegmentConstructabilityArtifact::SetProvidedFillet(Float64 provided)
{
   m_ProvidedFillet = provided;
}

Float64 pgsSegmentConstructabilityArtifact::GetProvidedFillet() const
{
   return m_ProvidedFillet;
}

bool pgsSegmentConstructabilityArtifact::MinimumFilletPassed() const
{
   // min fillet uses same applicability as slab offset check
   if (m_bIsSlabOffsetApplicable || m_bIsMinimumHaunchCheckApplicable)
   {
   return m_ProvidedFillet + TOLERANCE > m_MinimumRequiredFillet;
   }
   else
   {
      return true;
   }
}

void pgsSegmentConstructabilityArtifact::SetSlabOffsetApplicability(bool bSet)
{
   m_bIsSlabOffsetApplicable = bSet;
}

bool pgsSegmentConstructabilityArtifact::IsSlabOffsetApplicable() const
{
   return m_bIsSlabOffsetApplicable;
}

void pgsSegmentConstructabilityArtifact::SetLeastHaunchDepth(Float64 location, Float64 leastA)
{
   m_LeastHaunchLocation = location;
   m_LeastHaunch = leastA;
}

void pgsSegmentConstructabilityArtifact::GetLeastHaunchDepth(Float64* pLocation, Float64* pLeastA) const
{
   *pLocation = m_LeastHaunchLocation;
   *pLeastA = m_LeastHaunch;
}

pgsSegmentConstructabilityArtifact::SlabOffsetStatusType pgsSegmentConstructabilityArtifact::SlabOffsetStatus() const
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
      if ( (m_ProvidedFillet + m_SlabOffsetWarningTolerance) < m_LeastHaunch)
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

bool pgsSegmentConstructabilityArtifact::SlabOffsetPassed() const
{
   return ( SlabOffsetStatus()==Fail ) ? false : true;
}

void pgsSegmentConstructabilityArtifact::CheckStirrupLength(bool bCheck)
{
   m_bCheckStirrupLength = bCheck;
}

bool pgsSegmentConstructabilityArtifact::CheckStirrupLength() const
{
   return m_bIsSlabOffsetApplicable && m_bCheckStirrupLength;
}

void pgsSegmentConstructabilityArtifact::SetPrecamberApplicability(bool bSet)
{
   m_bIsPrecamberApplicable = bSet;
}

bool pgsSegmentConstructabilityArtifact::IsPrecamberApplicable() const
{
   return m_bIsPrecamberApplicable;
}

void pgsSegmentConstructabilityArtifact::SetPrecamber(const CSegmentKey& segmentKey, Float64 limit, Float64 value)
{
   m_Precamber.insert(std::make_pair(segmentKey, std::make_pair(limit, value)));
}

void pgsSegmentConstructabilityArtifact::GetPrecamber(const CSegmentKey& segmentKey, Float64* pLimit, Float64* pValue) const
{
   const auto& found = m_Precamber.find(segmentKey);
   ATLASSERT(found != m_Precamber.end()); // asking for a segment whose precamber record hasn't been recorded
   *pLimit = found->second.first;
   *pValue = found->second.second;;
}

bool pgsSegmentConstructabilityArtifact::PrecamberPassed(const CSegmentKey& segmentKey) const
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

bool pgsSegmentConstructabilityArtifact::PrecamberPassed() const
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

void pgsSegmentConstructabilityArtifact::SetBottomFlangeClearanceApplicability(bool bSet)
{
   m_bIsBottomFlangeClearanceApplicable = bSet;
}

bool pgsSegmentConstructabilityArtifact::IsBottomFlangeClearanceApplicable() const
{
   return m_bIsBottomFlangeClearanceApplicable;
}

void pgsSegmentConstructabilityArtifact::SetBottomFlangeClearanceParameters(Float64 C,Float64 Cmin)
{
   m_C = C;
   m_Cmin = Cmin;
}

void pgsSegmentConstructabilityArtifact::GetBottomFlangeClearanceParameters(Float64* pC,Float64* pCmin) const
{
   *pC = m_C;
   *pCmin = m_Cmin;
}

bool pgsSegmentConstructabilityArtifact::BottomFlangeClearancePassed() const
{
   if ( !m_bIsBottomFlangeClearanceApplicable )
   {
      return true;
   }

   return ::IsGE(m_Cmin,m_C) ? true : false;
}

void pgsSegmentConstructabilityArtifact::SetComputedExcessCamber(Float64 value)
{
   m_ComputedExcessCamber = value;
}

Float64 pgsSegmentConstructabilityArtifact::GetComputedExcessCamber() const
{
   return m_ComputedExcessCamber;
}

void pgsSegmentConstructabilityArtifact::SetAssumedExcessCamber(Float64 value)
{
   m_AssumedExcessCamber = value;
}

Float64 pgsSegmentConstructabilityArtifact::GetAssumedExcessCamber() const
{
   return m_AssumedExcessCamber;
}

void pgsSegmentConstructabilityArtifact::SetAssumedMinimumHaunchDepth(Float64 value)
{
   m_AssumedMinimumHaunchDepth = value;
}

Float64 pgsSegmentConstructabilityArtifact::GetAssumedMinimumHaunchDepth() const
{
   return m_AssumedMinimumHaunchDepth;
}

void pgsSegmentConstructabilityArtifact::SetHaunchGeometryCheckApplicability(bool bSet)
{
   m_bIsHaunchGeometryCheckApplicable = bSet;
}

bool pgsSegmentConstructabilityArtifact::IsHaunchGeometryCheckApplicable() const
{
   return m_bIsHaunchGeometryCheckApplicable;
}

void pgsSegmentConstructabilityArtifact::SetHaunchGeometryTolerance(Float64 value)
{
   m_HaunchGeometryTolerance = value;
}

Float64 pgsSegmentConstructabilityArtifact::GetHaunchGeometryTolerance() const
{
   return m_HaunchGeometryTolerance;
}

pgsSegmentConstructabilityArtifact::HaunchGeometryStatusType pgsSegmentConstructabilityArtifact::HaunchGeometryStatus() const
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

bool pgsSegmentConstructabilityArtifact::HaunchGeometryPassed() const
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

bool pgsSegmentConstructabilityArtifact::Passed() const
{
   if ( !SlabOffsetPassed() )
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

   if (!MinimumHaunchDepthPassed())
   {
      return false;
   }

   return true;
}

 //======================== ACCESS     =======================================

void pgsSegmentConstructabilityArtifact::SetFinishedElevationApplicability(bool bSet)
{
   m_bIsFinishedElevationApplicable = bSet;
}

bool pgsSegmentConstructabilityArtifact::GetFinishedElevationApplicability() const
{
   return m_bIsFinishedElevationApplicable;
}

void pgsSegmentConstructabilityArtifact::SetFinishedElevationControllingInterval(IntervalIndexType interval)
{
   m_FinishedElevationControllingInterval = interval;
}

IntervalIndexType pgsSegmentConstructabilityArtifact::GetFinishedElevationControllingInterval() const
{
   return m_FinishedElevationControllingInterval;
}

void pgsSegmentConstructabilityArtifact::SetFinishedElevationTolerance(Float64 tol)
{
   m_FinishedElevationTolerance = tol;
}

Float64 pgsSegmentConstructabilityArtifact::GetFinishedElevationTolerance() const
{
   return m_FinishedElevationTolerance;
}

void pgsSegmentConstructabilityArtifact::SetMaxFinishedElevation(Float64 station, Float64 offset, const pgsPointOfInterest& poi, Float64 designElevation, Float64 finishedElevation)
{
   ATLASSERT(m_SegmentKey == poi.GetSegmentKey());

   m_FinishedElevationStation = station;
   m_FinishedElevationOffset = offset;
   m_FinishedElevationPoi = poi;
   m_DesignElevation = designElevation;
   m_FinishedElevation = finishedElevation;
}

void pgsSegmentConstructabilityArtifact::GetMaxFinishedElevation(Float64* pStation, Float64* pOffset, pgsPointOfInterest* pPoi, Float64* pDesignElevation, Float64* pFinishedElevation) const
{
   *pStation = m_FinishedElevationStation;
   *pOffset = m_FinishedElevationOffset;
   *pPoi = m_FinishedElevationPoi;
   *pDesignElevation = m_DesignElevation;
   *pFinishedElevation = m_FinishedElevation;
}

bool pgsSegmentConstructabilityArtifact::FinishedElevationPassed() const
{
   return IsEqual(m_DesignElevation, m_FinishedElevation, m_FinishedElevationTolerance) ? true : false;
}

void pgsSegmentConstructabilityArtifact::SetMinimumHaunchDepthApplicability(bool bSet)
{
   m_bIsMinimumHaunchCheckApplicable = bSet;
}

bool pgsSegmentConstructabilityArtifact::GetMinimumHaunchDepthApplicability() const
{
   return m_bIsMinimumHaunchCheckApplicable;
}

void pgsSegmentConstructabilityArtifact::SetMinimumHaunchDepthControllingInterval(IntervalIndexType interval)
{
   m_MinimumHaunchCheckControllingInterval = interval;
}

IntervalIndexType pgsSegmentConstructabilityArtifact::GetMinimumHaunchDepthControllingInterval() const
{
   return m_MinimumHaunchCheckControllingInterval;
}

void pgsSegmentConstructabilityArtifact::SetMinimumAllowableHaunchDepth(Float64 haunchDepth)
{
   m_MinimumAllowableHaunchDepth = haunchDepth;
}

Float64 pgsSegmentConstructabilityArtifact::GetMinimumAllowableHaunchDepth() const
{
   return m_MinimumAllowableHaunchDepth;
}

void pgsSegmentConstructabilityArtifact::SetMinimumHaunchDepth(Float64 station,Float64 offset,const pgsPointOfInterest& poi,Float64 minimumHaunchDepth)
{
   m_MinimumHaunchStation = station;
   m_MinimumHaunchOffset = offset;
   m_MinimumHaunchPoi = poi;
   m_MinimumHaunchDepth = minimumHaunchDepth;
}

void pgsSegmentConstructabilityArtifact::GetMinimumHaunchDepth(Float64* pStation,Float64* pOffset,pgsPointOfInterest* pPoi,Float64* pMinimumHaunchDepth) const
{
   *pPoi = m_MinimumHaunchPoi;
   *pStation = m_MinimumHaunchStation;
   *pOffset = m_MinimumHaunchOffset;
   *pMinimumHaunchDepth = m_MinimumHaunchDepth;
}

bool pgsSegmentConstructabilityArtifact::MinimumHaunchDepthPassed() const
{
   if (m_MinimumAllowableHaunchDepth == 0 && m_MinimumHaunchDepth == 0) // default condition is a pass
   {
      return true;
   }
   else
   {
      return m_MinimumAllowableHaunchDepth < m_MinimumHaunchDepth+TOLERANCE;
   }
}

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsSegmentConstructabilityArtifact::MakeCopy(const pgsSegmentConstructabilityArtifact& rOther)
{
   m_SegmentKey = rOther.m_SegmentKey;

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
   m_FinishedElevationControllingInterval = rOther.m_FinishedElevationControllingInterval;
   m_FinishedElevationTolerance = rOther.m_FinishedElevationTolerance;
   m_FinishedElevationStation = rOther.m_FinishedElevationStation;
   m_FinishedElevationOffset = rOther.m_FinishedElevationOffset;
   m_FinishedElevationPoi = rOther.m_FinishedElevationPoi;
   m_DesignElevation = rOther.m_DesignElevation;
   m_FinishedElevation = rOther.m_FinishedElevation;

   m_bIsMinimumHaunchCheckApplicable = rOther.m_bIsMinimumHaunchCheckApplicable;
   m_MinimumHaunchCheckControllingInterval = rOther.m_MinimumHaunchCheckControllingInterval;
   m_MinimumHaunchStation = rOther.m_MinimumHaunchStation;
   m_MinimumHaunchOffset = rOther.m_MinimumHaunchOffset;
   m_MinimumHaunchPoi = rOther.m_MinimumHaunchPoi;
   m_MinimumAllowableHaunchDepth = rOther.m_MinimumAllowableHaunchDepth;
   m_MinimumHaunchDepth = rOther.m_MinimumHaunchDepth;
}

void pgsSegmentConstructabilityArtifact::MakeAssignment(const pgsSegmentConstructabilityArtifact& rOther)
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
   m_RequiredAtBearingCLs = 0;
   m_HaunchBearingCLApplicability = hbcAppNA;
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
   m_SegmentArtifacts.clear();
}

void pgsConstructabilityArtifact::AddSegmentArtifact(const pgsSegmentConstructabilityArtifact& artifact)
{
   m_SegmentArtifacts.push_back(artifact);
}

const pgsSegmentConstructabilityArtifact& pgsConstructabilityArtifact::GetSegmentArtifact(SegmentIndexType segmentIdx) const
{
   const auto& artifact = m_SegmentArtifacts[segmentIdx];
   ATLASSERT(artifact.GetSegment() == segmentIdx);
   return artifact;

   //for (const auto& artifact : m_SegmentArtifacts)
   //{
   //   if (artifact.GetSegment() == segmentIdx)
   //   {
   //      return &artifact;
   //   }
   //}

   //return nullptr;
}

void pgsConstructabilityArtifact::SetProvidedHaunchAtBearingCLs(Float64 startEnd, Float64 endEnd)
{
   m_ProvidedAtBearingCLs[pgsTypes::metStart] = startEnd;
   m_ProvidedAtBearingCLs[pgsTypes::metEnd] = endEnd;
}

void pgsConstructabilityArtifact::GetProvidedHaunchAtBearingCLs(Float64* pStartEnd,Float64* pEndEnd) const
{
   *pStartEnd = m_ProvidedAtBearingCLs[pgsTypes::metStart];
   *pEndEnd   = m_ProvidedAtBearingCLs[pgsTypes::metEnd];
}

void pgsConstructabilityArtifact::SetRequiredHaunchAtBearingCLs(Float64 reqd)
{
   m_RequiredAtBearingCLs = reqd;
}

Float64 pgsConstructabilityArtifact::GetRequiredHaunchAtBearingCLs() const
{
   return m_RequiredAtBearingCLs;
}

void pgsConstructabilityArtifact::SetHaunchBearingCLApplicability(HaunchBearingCLApplicabilityType bSet)
{
   m_HaunchBearingCLApplicability = bSet;
}

pgsConstructabilityArtifact::HaunchBearingCLApplicabilityType pgsConstructabilityArtifact::GetHaunchBearingCLApplicability() const
{
   return m_HaunchBearingCLApplicability;
}

bool pgsConstructabilityArtifact::HaunchAtBearingCLsPassed() const
{
   if (m_HaunchBearingCLApplicability != pgsConstructabilityArtifact::hbcAppYes)
   {
      return true;
   }
   else
   {
      return m_ProvidedAtBearingCLs[pgsTypes::metStart] + TOLERANCE >= m_RequiredAtBearingCLs &&
             m_ProvidedAtBearingCLs[pgsTypes::metEnd]   + TOLERANCE >= m_RequiredAtBearingCLs;
   }
}

bool pgsConstructabilityArtifact::Passed() const
{
   ATLASSERT(!m_SegmentArtifacts.empty());
   for (const auto& artf : m_SegmentArtifacts)
   {
      if (!artf.Passed())
         return false;
   }

   return HaunchAtBearingCLsPassed();
}

bool pgsConstructabilityArtifact::IsSlabOffsetApplicable() const
{
   ATLASSERT(!m_SegmentArtifacts.empty());
   for (const auto& artf : m_SegmentArtifacts)
   {
      if (!artf.IsSlabOffsetApplicable())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::SlabOffsetPassed() const
{
   ATLASSERT(!m_SegmentArtifacts.empty());
   for (const auto& artf : m_SegmentArtifacts)
   {
      if (!artf.SlabOffsetPassed())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::IsHaunchAtBearingCLsApplicable() const
{
   return m_HaunchBearingCLApplicability == hbcAppYes;
}

bool pgsConstructabilityArtifact::IsPrecamberApplicable() const
{
   ATLASSERT(!m_SegmentArtifacts.empty());
   for (const auto& artf : m_SegmentArtifacts)
   {
      if (!artf.IsPrecamberApplicable())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::PrecamberPassed() const
{
   ATLASSERT(!m_SegmentArtifacts.empty());
   for (const auto& artf : m_SegmentArtifacts)
   {
      if (!artf.PrecamberPassed())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::IsBottomFlangeClearanceApplicable() const
{
   ATLASSERT(!m_SegmentArtifacts.empty());
   for (const auto& artf : m_SegmentArtifacts)
   {
      if (!artf.IsBottomFlangeClearanceApplicable())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::BottomFlangeClearancePassed() const
{
   ATLASSERT(!m_SegmentArtifacts.empty());
   for (const auto& artf : m_SegmentArtifacts)
   {
      if (!artf.BottomFlangeClearancePassed())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::MinimumFilletPassed() const
{
   ATLASSERT(!m_SegmentArtifacts.empty());
   for (const auto& artf : m_SegmentArtifacts)
   {
      if (!artf.MinimumFilletPassed())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::HaunchGeometryPassed() const
{
   ATLASSERT(!m_SegmentArtifacts.empty());
   for (const auto& artf : m_SegmentArtifacts)
   {
      if (!artf.HaunchGeometryPassed())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::IsFinishedElevationApplicable() const
{
   ATLASSERT(!m_SegmentArtifacts.empty());
   for (const auto& artf : m_SegmentArtifacts)
   {
      if (!artf.GetFinishedElevationApplicability())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::FinishedElevationPassed() const
{
   ATLASSERT(!m_SegmentArtifacts.empty());
   for (const auto& artf : m_SegmentArtifacts)
   {
      if (!artf.FinishedElevationPassed())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::MinimumHaunchDepthPassed() const
{
   ATLASSERT(!m_SegmentArtifacts.empty());
   for (const auto& artf : m_SegmentArtifacts)
   {
      if (!artf.MinimumHaunchDepthPassed())
         return false;
   }

   return true;
}

bool pgsConstructabilityArtifact::CheckStirrupLength() const
{
   ATLASSERT(!m_SegmentArtifacts.empty());
   for (const auto& artf : m_SegmentArtifacts)
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
   m_SegmentArtifacts = rOther.m_SegmentArtifacts;

   m_HaunchBearingCLApplicability = rOther.m_HaunchBearingCLApplicability;
   m_ProvidedAtBearingCLs = rOther.m_ProvidedAtBearingCLs;
   m_RequiredAtBearingCLs = rOther.m_RequiredAtBearingCLs;
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
