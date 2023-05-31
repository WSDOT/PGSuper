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

#include "StdAfx.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\Allowables.h>
#include <IFace\PointOfInterest.h>
#include <IFace\Intervals.h>
#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\LoadFactors.h>

#include "PsForceEng.h"
#include "StrandDesignTool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   pgsStrandDesignTool
****************************************************************************/

////// local functions
//////////////////////
inline static Float64 GetPointY(StrandIndexType idx, IPoint2dCollection* points)
{
   CComPtr<IPoint2d> point;
   points->get_Item(idx,&point);
   Float64 curr_y;
   point->get_Y(&curr_y);
   return curr_y;
}


void SortDebondLevels(std::vector<DebondLevelType>& rDebondLevelsAtSections)
{
   DebondLevelType max_level=0;
   for(auto rit=rDebondLevelsAtSections.rbegin(); rit!=rDebondLevelsAtSections.rend(); rit++)
   {
      DebondLevelType& rlvl = *rit;

      if (rlvl < 0)
      {
         ATLASSERT(false); // some design functions set negative levels, this should have been handled earlier
         rlvl *= -1;
      }
 
      if (rlvl < max_level)
      {
         rlvl = max_level;
      }
      else
      {
         max_level = rlvl;
      }
   }
}

// starting default to use about 2/3 straight and 1/3 harped, and if not possible; use whatever fits
static const Float64 DefaultHarpedRatio = 1.0/3.0;

pgsStrandDesignTool::pgsStrandDesignTool(SHARED_LOGFILE lf) :
LOGFILE(lf),
m_pArtifact(nullptr),
m_pBroker(nullptr),
m_StatusGroupID(INVALID_ID),
m_DoDesignForStrandSlope(false),
m_AllowableStrandSlope(0.0),
m_DoDesignForHoldDownForce(false),
m_AllowableHoldDownForce(0.0),
m_HoldDownFriction(0.0),
m_bTotalHoldDownForce(true),
m_MinimumFinalMzEccentricity(Float64_Max),
m_HarpedRatio(DefaultHarpedRatio),
m_MinPermanentStrands(0),
m_MinSlabOffset(0.0),
m_AbsoluteMinimumSlabOffset(0.0),
m_ConcreteAccuracy(WBFL::Units::ConvertToSysUnits(100,WBFL::Units::Measure::PSI)),
m_bConfigDirty(true),
m_pGirderEntry(nullptr),
m_MaxFci(0),
m_MaxFc(0)
{
}

void pgsStrandDesignTool::Initialize(IBroker* pBroker, StatusGroupIDType statusGroupID, pgsSegmentDesignArtifact* pArtif)
{
   ATLASSERT(pBroker);

   // Cache a whole bunch of stuff that does not change during design
   m_pBroker = pBroker;
   m_StatusGroupID = statusGroupID;

   m_bConfigDirty = true; // cache is dirty

   m_pArtifact = pArtif;
 
   m_SegmentKey = m_pArtifact->GetSegmentKey();

   m_DesignOptions = pArtif->GetDesignOptions();

   m_StrandFillType = m_DesignOptions.doStrandFillType;

   m_HarpedRatio = DefaultHarpedRatio;

   // Every design starts from the same point regardless of the current state of bridge
   // description input
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ISegmentData,pSegmentData);
   GET_IFACE(IPretensionForce,pPrestressForce);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   const CGirderMaterial* pSegmentMaterial = pSegmentData->GetSegmentMaterial(m_SegmentKey);

   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(m_SegmentKey.groupIndex);
   m_pGirderEntry = pGroup->GetGirder(m_SegmentKey.girderIndex)->GetGirderLibraryEntry();
   m_GirderEntryName = m_pGirderEntry->GetName(); // cache name for performance

   // Allocate and Initialize raised straight design tool if required
   if( m_DesignOptions.doDesignForFlexure == dtDesignFullyBondedRaised ||
       m_DesignOptions.doDesignForFlexure == dtDesignForDebondingRaised )
   {
      m_pRaisedStraightStrandDesignTool = std::shared_ptr<pgsRaisedStraightStrandDesignTool>(new pgsRaisedStraightStrandDesignTool(LOGGER,m_pGirderEntry));
      m_pRaisedStraightStrandDesignTool->Initialize(m_pBroker,m_StatusGroupID,m_pArtifact);
   }
   else if (m_pRaisedStraightStrandDesignTool)
   {
      // Don't need tool for this design type
      m_pRaisedStraightStrandDesignTool.reset();
   }

   // Initialize concrete strength range
   LOG(_T("Initializing concrete strength range"));
   if (m_DesignOptions.doDesignConcreteStrength == cdPreserveStrength)
   {
      LOG(_T("Concrete strengths are fixed"));
      m_MinFci = pSegmentMaterial->Concrete.Fci;
      m_MaxFci = pSegmentMaterial->Concrete.Fci;
      m_MinFc = pSegmentMaterial->Concrete.Fc;
      m_MaxFc = pSegmentMaterial->Concrete.Fc;
   }
   else
   {
      if (pSegmentMaterial->Concrete.Type == pgsTypes::PCI_UHPC)
      {
         LOG(_T("PCI-UHPC Concrete"));
         GET_IFACE(IEAFDisplayUnits, pDisplayUnits);
         m_MinFci = IS_SI_UNITS(pDisplayUnits) ? WBFL::Units::ConvertToSysUnits(28.0, WBFL::Units::Measure::MPa) : WBFL::Units::ConvertToSysUnits(4.0, WBFL::Units::Measure::KSI); // minimum per LRFD 5.4.2.1
         m_MaxFci = m_DesignOptions.maxFci; // this is from the design strategy defined in the girder
         m_MinFc = WBFL::Units::ConvertToSysUnits(17.4, WBFL::Units::Measure::KSI);
         m_MaxFc = m_DesignOptions.maxFc;// this is from the design strategy defined in the girder

         if (m_MaxFci < m_MinFci) m_MaxFci = m_MinFci;
         if (m_MaxFc < m_MinFc) m_MaxFc = m_MinFc;
      }
      else if (pSegmentMaterial->Concrete.Type == pgsTypes::UHPC)
      {
         ATLASSERT(false); // not supporting UHPC design yet
         // need to look at concrete strength range requirements for UHPC - there is a min f'ci hard limit that PCI UHPC does not have
         // this is just cobbled together - still needs to be reviewed for accuracy
         LOG(_T("UHPC Concrete"));
         GET_IFACE(IEAFDisplayUnits, pDisplayUnits);
         m_MinFci = WBFL::Units::ConvertToSysUnits(14.0, WBFL::Units::Measure::KSI); // minimum per GS 1.9.1.2 (but could be override by owner - override not implemented yet)
         m_MaxFci = m_DesignOptions.maxFci; // this is from the design strategy defined in the girder
         m_MinFc = WBFL::Units::ConvertToSysUnits(17.5, WBFL::Units::Measure::KSI); // GS 1.1.1
         m_MaxFc = m_DesignOptions.maxFc;// this is from the design strategy defined in the girder

         if (m_MaxFci < m_MinFci) m_MaxFci = m_MinFci;
         if (m_MaxFc < m_MinFc) m_MaxFc = m_MinFc;
      }
      else
      {
         LOG(_T("Conventional Concrete"));
         GET_IFACE(IEAFDisplayUnits, pDisplayUnits);
         m_MinFci = IS_SI_UNITS(pDisplayUnits) ? WBFL::Units::ConvertToSysUnits(28.0, WBFL::Units::Measure::MPa) : WBFL::Units::ConvertToSysUnits(4.0, WBFL::Units::Measure::KSI); // minimum per LRFD 5.4.2.1
         m_MaxFci = m_DesignOptions.maxFci; // this is from the design strategy defined in the girder
         m_MinFc = IS_SI_UNITS(pDisplayUnits) ? WBFL::Units::ConvertToSysUnits(34.5, WBFL::Units::Measure::MPa) : WBFL::Units::ConvertToSysUnits(5.0, WBFL::Units::Measure::KSI); // agreed by wsdot and txdot
         m_MaxFc = m_DesignOptions.maxFc;// this is from the design strategy defined in the girder
      }
   }

   LOG(_T("fci = ") << WBFL::Units::ConvertFromSysUnits(m_MinFci, WBFL::Units::Measure::KSI) << _T(" ksi - ") << WBFL::Units::ConvertFromSysUnits(m_MaxFci, WBFL::Units::Measure::KSI) << _T(" ksi"));
   LOG(_T("fc  = ") << WBFL::Units::ConvertFromSysUnits(m_MinFc,  WBFL::Units::Measure::KSI) << _T(" ksi - ") << WBFL::Units::ConvertFromSysUnits(m_MaxFc,  WBFL::Units::Measure::KSI) << _T(" ksi"));

   // Set concrete strength 
   Float64 ifc = GetMinimumConcreteStrength();

   // Initialize concrete type with a minimum strength
   if (::IsStructuralDeck(pDeck->GetDeckType()))
   {
      Float64 slab_fc = pDeck->Concrete.Fc;
      ifc = Max(slab_fc, ifc);
   }

   WBFL::Materials::SimpleConcrete conc(_T("Design Concrete"), ifc, pSegmentMaterial->Concrete.StrengthDensity, 
                      pSegmentMaterial->Concrete.WeightDensity, lrfdConcreteUtil::ModE((WBFL::Materials::ConcreteType)(pSegmentMaterial->Concrete.Type),ifc,  pSegmentMaterial->Concrete.StrengthDensity, false ),
                      0.0,0.0); // we don't need the modulus of rupture for shear or flexur. Just use 0.0
   conc.SetMaxAggregateSize(pSegmentMaterial->Concrete.MaxAggregateSize);
   conc.SetFiberLength(pSegmentMaterial->Concrete.FiberLength);
   conc.SetType((WBFL::Materials::ConcreteType)pSegmentMaterial->Concrete.Type);
   conc.HasAggSplittingStrength(pSegmentMaterial->Concrete.bHasFct);
   conc.SetAggSplittingStrength(pSegmentMaterial->Concrete.Fct);

   m_pArtifact->SetConcrete(conc);

   if (pSegmentMaterial->Concrete.bUserEc)
   {
      m_pArtifact->SetUserEc(pSegmentMaterial->Concrete.Ec);
   }

   if (pSegmentMaterial->Concrete.bUserEci)
   {
      m_pArtifact->SetUserEci(pSegmentMaterial->Concrete.Eci);
   }


   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

   m_FcControl.Init(ifc,lastIntervalIdx);

   // Initialize release strength
   Float64 ifci = GetMinimumReleaseStrength();
   m_ReleaseStrengthResult = ConcSuccess; // assume that no rebar is needed to make 4 ksi
   m_pArtifact->SetReleaseStrength( ifci );
   m_FciControl.Init(ifci,releaseIntervalIdx);

   if (pBridge->GetHaunchInputDepthType() == pgsTypes::hidACamber)
   {
      Float64 tSlab = pBridge->GetGrossSlabDepth(pgsPointOfInterest(m_SegmentKey,0.00));
      m_AbsoluteMinimumSlabOffset = tSlab;

      m_bIsDesignSlabOffset = m_DesignOptions.doDesignSlabOffset != sodPreserveHaunch;
      if (IsNonstructuralDeck(pBridge->GetDeckType()) || !m_bIsDesignSlabOffset)
      {
         // if there is no deck, set the artifact value to the current value
         PierIndexType startPierIdx,endPierIdx;
         pBridge->GetGirderGroupPiers(m_SegmentKey.groupIndex,&startPierIdx,&endPierIdx);
         ATLASSERT(endPierIdx == startPierIdx + 1);

         m_pArtifact->SetSlabOffset(pgsTypes::metStart,pBridge->GetSlabOffset(m_SegmentKey,pgsTypes::metStart));
         m_pArtifact->SetSlabOffset(pgsTypes::metEnd,pBridge->GetSlabOffset(m_SegmentKey,pgsTypes::metEnd));
      }
      else
      {
         // Determine absolute minimum A
         Float64 min_haunch;
         if (!m_pGirderEntry->GetMinHaunchAtBearingLines(&min_haunch))
         {
            min_haunch = 0.0;
         }

         m_AbsoluteMinimumSlabOffset = tSlab + min_haunch;

         Float64 defaultA = Max(m_AbsoluteMinimumSlabOffset,1.5 * tSlab);

         m_pArtifact->SetSlabOffset(pgsTypes::metStart,defaultA);
         m_pArtifact->SetSlabOffset(pgsTypes::metEnd,defaultA);
      }

      // Set initial design for AssumedExcessCamber here. Design is only for haunch load determination
      GET_IFACE_NOCHECK(ISpecification,pSpec);
      m_bIsDesignExcessCamber = ((m_DesignOptions.doDesignSlabOffset == sodDesignHaunch) && pSpec->IsAssumedExcessCamberForLoad()) ? true : false;

      // don't let tolerance be impossible
      if (m_bIsDesignExcessCamber)
      {
         m_AssumedExcessCamberTolerance = min(WBFL::Units::ConvertToSysUnits(0.5,WBFL::Units::Measure::Inch),pSpec->GetCamberTolerance());
      }
      else
      {
         m_AssumedExcessCamberTolerance = WBFL::Units::ConvertToSysUnits(0.5,WBFL::Units::Measure::Inch); // doesn't matter
      }

      Float64 assumedExcessCamber = pBridge->GetAssumedExcessCamber(m_SegmentKey.groupIndex,m_SegmentKey.girderIndex);
      m_pArtifact->SetAssumedExcessCamber(assumedExcessCamber);
   }
   else
   {
      // Don't touch haunch design if not A dim input
      m_bIsDesignSlabOffset = false;
      m_bIsDesignExcessCamber = false;
   }

   // Initialize Prestressing
   m_pArtifact->SetNumStraightStrands( 0 );
   m_pArtifact->SetPjackStraightStrands( 0 );
   m_pArtifact->SetNumHarpedStrands( 0 );
   m_pArtifact->SetPjackHarpedStrands( 0 );
   m_pArtifact->SetNumTempStrands( 0 );
   m_pArtifact->SetPjackTempStrands( 0 );
   m_pArtifact->SetUsedMaxPjackStraightStrands( false );
   m_pArtifact->SetUsedMaxPjackHarpedStrands( false );
   m_pArtifact->SetUsedMaxPjackTempStrands( false );

   m_MinSlabOffset = 0.0;

   InitDebondData();

   // Get area of an individual prestressing strand
   m_aps[pgsTypes::Straight]  = pSegmentData->GetStrandMaterial(m_SegmentKey,pgsTypes::Straight)->GetNominalArea();
   m_aps[pgsTypes::Harped]    = pSegmentData->GetStrandMaterial(m_SegmentKey,pgsTypes::Harped)->GetNominalArea();
   m_aps[pgsTypes::Temporary] = pSegmentData->GetStrandMaterial(m_SegmentKey,pgsTypes::Temporary)->GetNominalArea();

   m_SegmentLength         = pBridge->GetSegmentLength(m_SegmentKey);
   m_SpanLength            = pBridge->GetSegmentSpanLength(m_SegmentKey);
   m_StartConnectionLength = pBridge->GetSegmentStartEndDistance(m_SegmentKey);
   m_XFerLength[pgsTypes::Straight]  = pPrestressForce->GetTransferLength(m_SegmentKey,pgsTypes::Straight,pgsTypes::tltMinimum);
   m_XFerLength[pgsTypes::Harped]    = pPrestressForce->GetTransferLength(m_SegmentKey,pgsTypes::Harped, pgsTypes::tltMinimum);
   m_XFerLength[pgsTypes::Temporary] = pPrestressForce->GetTransferLength(m_SegmentKey,pgsTypes::Temporary, pgsTypes::tltMinimum);

   // harped offsets, hold-down and max strand slopes
   InitHarpedPhysicalBounds(pSegmentData->GetStrandMaterial(m_SegmentKey,pgsTypes::Harped));

   // Compute and Cache mid-zone boundaries
   ComputeMidZoneBoundaries();

   // lay out possible debond levels
   ComputeDebondLevels(pPrestressForce);

   // locate and cache points of interest for design
   ValidatePointsOfInterest();

   // Compute minimum number of strands to start design from
   ComputeMinStrands();

   GET_IFACE(IStrandGeometry,pStrandGeom);
   pStrandGeom->GetHarpedStrandControlHeights(m_SegmentKey,&m_HgStart,&m_HgHp1,&m_HgHp2,&m_HgEnd);
}

void pgsStrandDesignTool::InitReleaseStrength(Float64 fci,IntervalIndexType intervalIdx)
{
   m_FciControl.Init(fci,intervalIdx);
   m_pArtifact->SetReleaseStrength(fci);
   m_bConfigDirty = true; // cache is dirty
}

void pgsStrandDesignTool::InitFinalStrength(Float64 fc,IntervalIndexType intervalIdx)
{
   m_FcControl.Init(fc,intervalIdx);
   m_pArtifact->SetConcreteStrength(fc);
   m_bConfigDirty = true; // cache is dirty
}


void pgsStrandDesignTool::RestoreDefaults(bool retainProportioning, bool justAddedRaisedStrands)
{
   // set fill order back to user-requested while attempting to retain current number of strands
   StrandIndexType np = GetNumPermanentStrands();

   if (!retainProportioning && 0 < np)
   {
      if (justAddedRaisedStrands)
      {
         ATLASSERT(m_pRaisedStraightStrandDesignTool);
         // Just added raised strands - reset number of strands to max of current
         // and a newly computed minimum number of strands
         ComputeMinStrands();

         // This will allow algorithm to rebalance strands and not have to start from scratch
         StrandIndexType np = this->GetNumPermanentStrands();

         StrandIndexType npnew = max(np, this->GetMinimumPermanentStrands());

         // this is tricky - we just resequenced the strand order to account for raised strands, and we can't count on the 
         // current number of strands to fit into the new fill order - make sure:
         StrandIndexType npras = m_pRaisedStraightStrandDesignTool->GetNextNumPermanentStrands(npnew);
         npras = m_pRaisedStraightStrandDesignTool->GetPreviousNumPermanentStrands(npras);
         if (npras != npnew)
         {
            npnew = npras; // old nsrands did not fit into new ordering
         }

         SetNumPermanentStrands(npnew);

         // Also, conscrete strength may be out of wack. Init back to min
         InitReleaseStrength( GetMinimumReleaseStrength(), m_FciControl.Interval() );
         InitFinalStrength( GetMinimumConcreteStrength(), m_FcControl.Interval() );

      }
      else if (m_StrandFillType != m_DesignOptions.doStrandFillType)
      {
         m_StrandFillType = m_DesignOptions.doStrandFillType;
         m_HarpedRatio = DefaultHarpedRatio;

         // match current strands as close as possible
         StrandIndexType npnew = GetNextNumPermanentStrands(np-1);
         ATLASSERT(0 < npnew);

         SetNumPermanentStrands(npnew);
      }
      else if (m_StrandFillType == ftMinimizeHarping && m_HarpedRatio != DefaultHarpedRatio)
      {
         m_HarpedRatio = DefaultHarpedRatio;

         // match current strands as close as possible
         StrandIndexType npnew = GetNextNumPermanentStrands(np-1);
         ATLASSERT(0 < npnew);

         SetNumPermanentStrands(npnew);
      }
   }
}

Float64 pgsStrandDesignTool::GetSegmentLength() const
{
   return m_SegmentLength;
}

bool pgsStrandDesignTool::IsDesignDebonding() const
{
   return dtDesignForDebonding       == m_DesignOptions.doDesignForFlexure ||
          dtDesignForDebondingRaised == m_DesignOptions.doDesignForFlexure;
}

bool pgsStrandDesignTool::IsDesignHarping() const
{
   return dtDesignForHarping == m_DesignOptions.doDesignForFlexure;
}

bool pgsStrandDesignTool::IsDesignRaisedStraight() const
{
   return dtDesignFullyBondedRaised  == m_DesignOptions.doDesignForFlexure ||
          dtDesignForDebondingRaised == m_DesignOptions.doDesignForFlexure;
}

bool pgsStrandDesignTool::SetNumTempStrands(StrandIndexType num)
{
   m_pArtifact->SetNumTempStrands(num);
   m_bConfigDirty = true; // cache is dirty

   UpdateJackingForces();
   return true;
}

bool pgsStrandDesignTool::SetNumPermanentStrands(StrandIndexType numPerm)
{
   ATLASSERT(m_MinPermanentStrands <= numPerm);

   if (GetMaxPermanentStrands() < numPerm)
   {
      ATLASSERT(false);
      return false;
   }
   else if (numPerm == 0)
   {
      m_pArtifact->SetNumHarpedStrands(0);
      m_pArtifact->SetNumStraightStrands(0);
      UpdateJackingForces();
      m_bConfigDirty = true; // cache is dirty

      LOG(_T("** Set Np=0"));
      return true;
   }
   else if (m_pRaisedStraightStrandDesignTool)
   {
      // Raised strand design - let tool do work
      StrandIndexType ns, nh;
      m_pRaisedStraightStrandDesignTool->ComputeNumStrands(numPerm, &ns, &nh);
      LOG (_T("Using raised resequenced fill order to make Ns=")<<ns<<_T(", Nh=")<<nh<<_T(" from ")<< numPerm << _T(" with ")<<m_pRaisedStraightStrandDesignTool->GetNumUsedRaisedStrandLocations()<<_T(" raised grid locations"));

      m_bConfigDirty = true; // cache is dirty

      m_pArtifact->SetNumStraightStrands(ns);
      m_pArtifact->SetNumHarpedStrands(nh); // keep in sync, even though we may not use this value
      m_pArtifact->SetRaisedAdjustableStrands( m_pRaisedStraightStrandDesignTool->CreateStrandFill(GirderLibraryEntry::stAdjustable, nh) );

      // update jacking forces
      UpdateJackingForces();

   }
   else
   {
      // proportion strands
      StrandIndexType ns, nh;

      if (m_StrandFillType == ftGridOrder)
      {
         GET_IFACE(IStrandGeometry,pStrandGeom);
         StrandIndexType uns, unh;
         if (!pStrandGeom->ComputeNumPermanentStrands(numPerm,m_SegmentKey, &uns, &unh))
         {
            ATLASSERT(false); // caller should have figured out if numPerm is valid
            return false;
         }

         ns = uns;
         nh = unh;

         LOG (_T("Using grid fill order to make Ns=")<<ns<<_T(", Nh=")<<nh<<_T(" from ")<< numPerm);
      }
      else if (m_StrandFillType == ftMinimizeHarping)
      {
         if (ComputeNextNumProportionalStrands(numPerm-1, &ns, &nh) == INVALID_INDEX)
         {
            ATLASSERT(false);
            return false;
         }

         LOG (_T("Using ")<<m_HarpedRatio<<_T(" harped ratio fill order to make Ns=")<<ns<<_T(", Nh=")<<nh<<_T(" from ")<< numPerm);
      }
      else
      {
         ATLASSERT(false);
         return false;
      }

      StrandIndexType nh_old = m_pArtifact->GetNumHarpedStrands();

      m_bConfigDirty = true; // cache is dirty

      m_pArtifact->SetNumHarpedStrands(nh);
      m_pArtifact->SetNumStraightStrands(ns);

      // update jacking forces
      UpdateJackingForces();

      // if we had no harped strands and now we have some, set offsets to maximize harp
      if (IsDesignHarping() && nh_old == 0 && 0 < nh)
      {
         ResetHarpedStrandConfiguration();
      }

      // Make sure we are within offset bounds. Force if necessary
      return KeepHarpedStrandsInBounds();

   } 

   LOG(_T("** Set Np=")<<GetNumPermanentStrands()<<_T(", Ns=")<<GetNs()<<_T(", Nh=")<<GetNh());

   return true;
}

bool pgsStrandDesignTool::SetNumStraightHarped(StrandIndexType ns, StrandIndexType nh)
{
   ATLASSERT(IsDesignHarping());
   ATLASSERT(!m_pRaisedStraightStrandDesignTool); // should never call for this design type
   ATLASSERT(0 <= ns && 0 <= nh);
   LOG (_T("SetNumStraightHarped:: Ns = ")<<ns<<_T(" Nh = ")<<nh);
   ATLASSERT(m_MinPermanentStrands <= ns+nh);

   // If this is being called, we are probably changing our strand fill. adjust likewise
   GET_IFACE(IStrandGeometry,pStrandGeom);

   // make sure numbers of strands are valid
   if (0 < ns && ns != pStrandGeom->GetNextNumStrands(m_SegmentKey,pgsTypes::Straight, ns-1))
   {
      ATLASSERT(false);
      LOG (_T("Ns is invalid"));
      return false;
   }

   if (0 < nh && nh != pStrandGeom->GetNextNumStrands(m_SegmentKey,pgsTypes::Harped, nh-1))
   {
      ATLASSERT(false);
      LOG (_T("Nh is invalid"));
      return false;
   }

   // change fill order for future strand setting and set our numbers of strands
   if (m_StrandFillType != ftMinimizeHarping)
   {
      LOG (_T("Changing fill type to ftMinimizeHarping"));
      m_StrandFillType = ftMinimizeHarping;
   }

   if (nh == 0)
   {
      m_HarpedRatio = 0.0;
   }
   else if (ns == 0)
   {
      m_HarpedRatio=Float64_Max;
   }
   else
   {
      m_HarpedRatio = (Float64)(nh)/(Float64)(ns+nh);
   }

   LOG (_T("Setting harped ratio to ")<<m_HarpedRatio);

   m_bConfigDirty = true; // cache is dirty

   StrandIndexType nh_old = m_pArtifact->GetNumHarpedStrands(); 

   m_pArtifact->SetNumHarpedStrands(nh);
   m_pArtifact->SetNumStraightStrands(ns);


   // if we had no harped strands and now we have some, set offsets to maximize harp
   if (IsDesignHarping() && nh_old == 0 && 0 < nh)
   {
      ResetHarpedStrandConfiguration();
   }

   // update jacking forces
   UpdateJackingForces();

   // Make sure we are within offset bounds. Force if necessary
   LOG(_T("** Set Np=")<<GetNumPermanentStrands()<<_T(", Ns=")<<GetNs()<<_T(", Nh=")<<GetNh());
   return KeepHarpedStrandsInBounds();
}

StrandIndexType pgsStrandDesignTool::GetNumPermanentStrands() const
{
   return m_pArtifact->GetNumStraightStrands() + m_pArtifact->GetNumHarpedStrands();
}

StrandIndexType pgsStrandDesignTool::GetNumTotalStrands() const
{
   return GetNumPermanentStrands() + GetNt();
}

StrandIndexType pgsStrandDesignTool::GetMaxPermanentStrands() const
{
   if (m_pRaisedStraightStrandDesignTool)
   {
      return m_pRaisedStraightStrandDesignTool->GetMaxPermanentStrands();
   }
   else
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);
      return pStrandGeom->GetMaxNumPermanentStrands(m_SegmentKey);
   }
}

StrandIndexType pgsStrandDesignTool::GetNh() const
{
   return m_pArtifact->GetNumHarpedStrands();
}

StrandIndexType pgsStrandDesignTool::GetNs() const
{
   return m_pArtifact->GetNumStraightStrands();
}

StrandIndexType pgsStrandDesignTool::GetNt() const
{
   return m_pArtifact->GetNumTempStrands();;
}

void pgsStrandDesignTool::SetConcreteAccuracy(Float64 accuracy)
{
   m_ConcreteAccuracy = accuracy;
}

Float64 pgsStrandDesignTool::GetConcreteAccuracy() const
{
   return m_ConcreteAccuracy;
}

const GDRCONFIG& pgsStrandDesignTool::GetSegmentConfiguration() const
{
   if (m_bConfigDirty)
   {
      m_CachedConfig = m_pArtifact->GetSegmentConfiguration();
      m_bConfigDirty = false;
   }

   return m_CachedConfig;
}

StrandIndexType pgsStrandDesignTool::GetNextNumPermanentStrands(StrandIndexType prevNum) const
{
   if (prevNum == INVALID_INDEX)
   {
      return 0; // trivial case
   }
   else if ( GetMaxPermanentStrands() <= prevNum)
   {
      return INVALID_INDEX;
   }
   else if (m_pRaisedStraightStrandDesignTool)
   {
      // raised straight
      return m_pRaisedStraightStrandDesignTool->GetNextNumPermanentStrands(prevNum);
   }
   else if (m_StrandFillType==ftGridOrder)
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);

      return pStrandGeom->GetNextNumPermanentStrands(m_SegmentKey, prevNum);
   }
   else if (m_StrandFillType==ftMinimizeHarping)
   {
      StrandIndexType ns, nh;
      return ComputeNextNumProportionalStrands(prevNum, &ns, &nh);
   }
   else
   {
      ATLASSERT(false);
      return INVALID_INDEX;
   }
}

StrandIndexType pgsStrandDesignTool::GetPreviousNumPermanentStrands(StrandIndexType nextNum) const
{
   StrandIndexType maxNum = GetMaxPermanentStrands();

   if (nextNum == 0 || nextNum == INVALID_INDEX)
   {
      ATLASSERT(false);
      return INVALID_INDEX; // trivial case
   }
   else if ( maxNum < nextNum )
   {
      return maxNum;
   }
   else if (m_pRaisedStraightStrandDesignTool)
   {
      // raised straight
      return m_pRaisedStraightStrandDesignTool->GetPreviousNumPermanentStrands(nextNum);
   }
   else if (m_StrandFillType==ftGridOrder)
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);

      return pStrandGeom->GetPreviousNumPermanentStrands(m_SegmentKey, nextNum);
   }
   else if (m_StrandFillType==ftMinimizeHarping)
   {
      StrandIndexType befNum = nextNum-1;
      StrandIndexType answer;
      do
      {
         StrandIndexType ns, nh;
         answer =  ComputeNextNumProportionalStrands(befNum, &ns, &nh);
         befNum--;
      }
      while (answer==nextNum);

      return answer;
   }
   else
   {
      ATLASSERT(false);
      return INVALID_INDEX;
   }
}

StrandIndexType pgsStrandDesignTool::ComputeNextNumProportionalStrands(StrandIndexType prevNum, StrandIndexType* pns, StrandIndexType* pnh) const
{
   ATLASSERT(!m_pRaisedStraightStrandDesignTool);

   if (prevNum < 0 || GetMaxPermanentStrands() <= prevNum)
   {
      ATLASSERT(0 < prevNum);
      *pnh = INVALID_INDEX;
      *pns = INVALID_INDEX;
      return INVALID_INDEX;
   }

   GET_IFACE(IStrandGeometry,pStrandGeom);

   // try to use current strand ratio, and if not possible; use whatever fits
   StrandIndexType nh_max = pStrandGeom->GetMaxStrands(m_SegmentKey,pgsTypes::Harped);
   StrandIndexType ns_max = pStrandGeom->GetMaxStrands(m_SegmentKey,pgsTypes::Straight);

   if ( nh_max == 0 )
   {
      StrandIndexType nst = pStrandGeom->GetNextNumStrands(m_SegmentKey,pgsTypes::Straight, prevNum);
      if (0 < nst)
      {
         *pnh = 0;
         *pns = nst;
         return nst;
      }
      else
      {
         *pnh = INVALID_INDEX;
         *pns = INVALID_INDEX;
         return INVALID_INDEX;
      }
   }
   else if ( ns_max == 0 )
   {
      StrandIndexType nsh = pStrandGeom->GetNextNumStrands(m_SegmentKey,pgsTypes::Harped, prevNum);
      if (0 < nsh)
      {
         *pnh = nsh;
         *pns = 0;
         return nsh;
      }
      else
      {
         *pnh = INVALID_INDEX;
         *pns = INVALID_INDEX;
         return INVALID_INDEX;
      }
   }
   else
   {
      StrandIndexType ns;
      // use current ratio to compute strands
      if (m_HarpedRatio == Float64_Max)
      {
         ns = 0;
      }
      else
      {
         if (m_HarpedRatio == 0.0)
         {
            ns = Min(prevNum, ns_max-1);
         }
         else
         {
            Float64 fra = 1.0 - m_HarpedRatio;
            StrandIndexType s = (StrandIndexType)ceil(fra*prevNum);
            ns = Min( Max(s, (StrandIndexType)1), ns_max-1);
         }

         ns = pStrandGeom->GetNextNumStrands(m_SegmentKey,pgsTypes::Straight, ns-1 );
         ATLASSERT(0 < ns && ns != INVALID_INDEX); 
      }

      StrandIndexType nh = (prevNum < ns ? INVALID_INDEX : prevNum - ns);
      if ( nh != INVALID_INDEX )
      {
         // need some harped strands
         if ( nh < nh_max )
         {
            *pnh = pStrandGeom->GetNextNumStrands(m_SegmentKey,pgsTypes::Harped, nh);
            *pns = ns;
            ATLASSERT(*pnh+ns > prevNum); // new must always be more
            return *pnh + ns;
         }
         else
         {
            // not enough harped strands to make it, must take from straight
            *pnh = nh_max;
            ATLASSERT(nh_max <= prevNum);
            ns = prevNum - nh_max;
            *pns = pStrandGeom->GetNextNumStrands(m_SegmentKey,pgsTypes::Straight, ns);
            if (0 < *pns)
            {
               ATLASSERT(prevNum < (*pns + *pnh)); // new must always be more
               return *pns + *pnh;
            }
            else
            {
               // not enough strands available
               ATLASSERT(false);
               return INVALID_INDEX;
            }
         }
      }
      else
      {
         // got all we need from straight strands
         *pnh = 0;
         *pns = ns;
         ATLASSERT(prevNum < *pns); // new must always be more
         return *pns;
      }
   }
}

bool pgsStrandDesignTool::IsValidNumPermanentStrands(StrandIndexType num) const
{
   ATLASSERT(num != INVALID_INDEX);
   if (num == 0)
   {
      return true;
   }
   else if (m_pRaisedStraightStrandDesignTool)
   {
      return m_pRaisedStraightStrandDesignTool->IsValidNumPermanentStrands(num);
   }
   else
   {
      return num == GetNextNumPermanentStrands(num-1);
   }
}

void pgsStrandDesignTool::SetMinimumPermanentStrands(StrandIndexType num)
{
   ATLASSERT(0 <= num && num != INVALID_INDEX);
   m_MinPermanentStrands = num;
}

StrandIndexType pgsStrandDesignTool::GetMinimumPermanentStrands() const
{
   // Make sure raised strand tool can control minimum number of strands
   if(m_pRaisedStraightStrandDesignTool)
   {
      StrandIndexType rmin = m_pRaisedStraightStrandDesignTool->GetMinimumPermanentStrands();
      if( m_MinPermanentStrands < rmin)
      {
         m_MinPermanentStrands = rmin;
      }
   }

   return m_MinPermanentStrands;
}

Float64 pgsStrandDesignTool::ComputeEccentricity(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx) const
{
   ATLASSERT(poi.GetSegmentKey() == m_SegmentKey);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);
   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();
   ATLASSERT(castDiaphragmIntervalIdx != INVALID_INDEX);

   // NOTE: Can't use the following code block. If the original input (before design) does not have
   // temporary strands then the install and remove intervals will be INVALID_INDEX. If the interval
   // in question is before the deck is cast, then include temporary strands. Otherwise, the deck
   // has been cast and the temporary strands have been removed
   //IntervalIndexType tsInstallIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(m_SegmentKey);
   //IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(m_SegmentKey);
   //bool bIncTempStrands = (tsInstallIntervalIdx <= intervalIdx && intervalIdx < tsRemovalIntervalIdx) ? true : false;
   bool bIncTempStrands = (intervalIdx < castDiaphragmIntervalIdx ? true : false); // temporary strands are assumed to be removed just prior to casting intermediate diaphragms
   // NOTE: When temporary strands are post-tensioned they are installed after release. If we are including temporary strands in the computation,
   // and we don't know when they are installed (see note above), we have to estimate the interval. The earliest PT-TTS can be installed is immediately after
   // release. For this reason, add one to release interval
   IntervalIndexType eccIntervalIdx = releaseIntervalIdx + (bIncTempStrands ? 1 : 0);

   const GDRCONFIG& config = GetSegmentConfiguration();

   GET_IFACE(IStrandGeometry,pStrandGeom);
   return pStrandGeom->GetEccentricity(eccIntervalIdx, poi, bIncTempStrands, &config).Y();
}

Float64 pgsStrandDesignTool::GetTransferLength(pgsTypes::StrandType strandType) const
{
   if ( strandType == pgsTypes::Permanent )
   {
      strandType = pgsTypes::Straight;
   }

   return m_XFerLength[strandType];
}

void pgsStrandDesignTool::ComputePermanentStrandsRequiredForPrestressForce(const pgsPointOfInterest& poi,InitialDesignParameters* pDesignParams) const
{
   ATLASSERT(poi.GetSegmentKey() == m_SegmentKey);

   LOG(_T("Compute number of strands required to handle a force of ")<<WBFL::Units::ConvertFromSysUnits(pDesignParams->Preqd,WBFL::Units::Measure::Kip) << _T(" kip"));

   GDRCONFIG guess = GetSegmentConfiguration();

   // Compute the maximum allowable jacking force for the trial number of strands
   Float64 fpjMax, PjMax;

   StrandIndexType ns = GetNs();
   StrandIndexType nh = GetNh();
   StrandIndexType nt = GetNt();

   ATLASSERT(guess.PrestressConfig.GetStrandCount(pgsTypes::Straight) == ns);
   ATLASSERT(guess.PrestressConfig.GetStrandCount(pgsTypes::Harped) == nh);
   ATLASSERT(guess.PrestressConfig.GetStrandCount(pgsTypes::Temporary) == nt);

   if ( ns + nh + nt == 0)
   {
      PjMax                            = 0.0;
      fpjMax                           = 0.0;
   }
   else
   {
      GET_IFACE(IPretensionForce,pPrestressForce);

      guess.PrestressConfig.Pjack[pgsTypes::Straight]  = pPrestressForce->GetPjackMax(m_SegmentKey,pgsTypes::Straight,ns);
      guess.PrestressConfig.Pjack[pgsTypes::Harped]    = pPrestressForce->GetPjackMax(m_SegmentKey,pgsTypes::Harped,nh);
      guess.PrestressConfig.Pjack[pgsTypes::Temporary] = pPrestressForce->GetPjackMax(m_SegmentKey,pgsTypes::Temporary,nt);
      
      PjMax  = guess.PrestressConfig.Pjack[pgsTypes::Straight] + 
               guess.PrestressConfig.Pjack[pgsTypes::Harped]   + 
               guess.PrestressConfig.Pjack[pgsTypes::Temporary];

      fpjMax = PjMax/(m_aps[pgsTypes::Straight]*ns + m_aps[pgsTypes::Harped]*nh + m_aps[pgsTypes::Temporary]*nt);
   }

   LOG(_T("Maximum jacking stress for this strand configuration = ") << WBFL::Units::ConvertFromSysUnits(fpjMax,WBFL::Units::Measure::KSI) << _T(" KSI"));

   // Estimate prestress loss
   pgsPsForceEng psfeng;
   psfeng.SetStatusGroupID(m_StatusGroupID);
   psfeng.SetBroker(m_pBroker);
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   Float64 loss;
   if ( pDesignParams->task.intervalIdx < liveLoadIntervalIdx )
   {
      loss = psfeng.GetEffectivePrestressLoss(poi,pgsTypes::Permanent,pDesignParams->task.intervalIdx,pgsTypes::End,true/*apply elastic gain reduction*/, &guess);
   }
   else
   {
      loss = psfeng.GetEffectivePrestressLossWithLiveLoad(poi,pgsTypes::Permanent,pDesignParams->task.limitState,INVALID_INDEX/*controlling live load*/,true/*include elastic effects*/, true/*apply elastic gain reduction*/, &guess);
   }

#if defined _DEBUG
   GET_IFACE(ILosses,pILosses);
   if ( pDesignParams->task.intervalIdx < liveLoadIntervalIdx )
   {
      Float64 check_loss = pILosses->GetEffectivePrestressLoss(poi,pgsTypes::Permanent,pDesignParams->task.intervalIdx,pgsTypes::End,&guess);
      ATLASSERT(IsEqual(loss,check_loss));
   }
   else
   {
      Float64 check_loss = pILosses->GetEffectivePrestressLossWithLiveLoad(poi,pgsTypes::Permanent,pDesignParams->task.limitState, INVALID_INDEX/*controlling live load*/, true/*include elastic effects*/, true/*apply elastic gain reduction*/, &guess);
      ATLASSERT(IsEqual(loss,check_loss));
   }
#endif // _DEBUG

   LOG(_T("Estimated losses for this strand configuration = ") << WBFL::Units::ConvertFromSysUnits(loss,WBFL::Units::Measure::KSI) << _T(" KSI"));

   // Required strand stress
   Float64 fstrand = fpjMax - loss;
   LOG(_T("Required strand stress = ") << WBFL::Units::ConvertFromSysUnits(fstrand,WBFL::Units::Measure::KSI) << _T(" KSI"));

   // Estimate number of prestressing strands
   Float64 Aps;
   if (fstrand == 0.0)
   {
      Aps = 0.0;
   }
   else
   {
      Aps = -pDesignParams->Preqd/fstrand; // Total required area of prestressing
   }

   LOG(_T("Strand Area = ") << WBFL::Units::ConvertFromSysUnits(m_aps[pgsTypes::Straight],WBFL::Units::Measure::Inch2) << _T(" in^2 per strand"));
   LOG(_T("Required area of prestressing = ") << WBFL::Units::ConvertFromSysUnits(Aps,WBFL::Units::Measure::Inch2) << _T(" in^2"));

   // NOTE: This is a little bit of hack. The original design assumes all strands are the same size. However, straight
   // and harped strands can now be different sizes. Generally, harped strands will be smaller and carry less force
   // per strand. Assuming the smaller strand will provided a larger estimate of number of strands and will
   // thus result in a conservative design. For this reason, the minimum strand area is used to estimate
   // number of strands required.
   Float64 aps = Min(m_aps[pgsTypes::Straight],m_aps[pgsTypes::Harped]);
   Float64 fN = Aps/aps;
   StrandIndexType N = (StrandIndexType)ceil(fN);
   N = Max(N,(StrandIndexType)1); // Must be zero or more strands

   LOG(_T("Required number of permanent strands (float) = ") << fN);
   LOG(_T("Required number of permanent strands = ") << N);

   N = GetNextNumPermanentStrands(N-1);
   LOG(_T("Actual number of permanent strands = ") << N);

   ATLASSERT(fN <= N || INVALID_INDEX == N);

   pDesignParams->Np = N;
   pDesignParams->fN = fN;
}

StrandIndexType pgsStrandDesignTool::GuessInitialStrands()
{
   LOG(_T(""));

   // Intialize with low number of strands to force tension to control
   StrandIndexType Np = GetNextNumPermanentStrands(m_MinPermanentStrands);

   if (Np < 1)
   {
      Np = 0;
      LOG(_T("No permanent strands defined in section"));
   }

   LOG(_T("Make initial guess of permanent strands using a couple, Np = ") << Np);
   
   StrandIndexType ns = SetNumPermanentStrands(Np) ? Np : INVALID_INDEX;
   if (ns != INVALID_INDEX)
   {
      bool st = ResetEndZoneStrandConfig(); 
      ATLASSERT(st);
   }

   return ns;
}

// return artifact updated with current design information
void pgsStrandDesignTool::UpdateJackingForces() const
{
   // Compute Jacking Force
   GET_IFACE(IPretensionForce,pPrestressForce);

   Float64 PjS, PjH, PjT;
   PjS  = pPrestressForce->GetPjackMax(m_SegmentKey,pgsTypes::Straight, GetNs());
   PjH  = pPrestressForce->GetPjackMax(m_SegmentKey,pgsTypes::Harped,   GetNh());
   PjT  = pPrestressForce->GetPjackMax(m_SegmentKey,pgsTypes::Temporary,GetNt());

   // Save the initial design in the design artifact.
   m_bConfigDirty = true; // cache is dirty

   m_pArtifact->SetPjackStraightStrands( PjS );
   m_pArtifact->SetUsedMaxPjackStraightStrands( true );

   m_pArtifact->SetPjackHarpedStrands( PjH );
   m_pArtifact->SetUsedMaxPjackHarpedStrands( true );

   m_pArtifact->SetPjackTempStrands( PjT );
   m_pArtifact->SetUsedMaxPjackTempStrands( true );
}

bool pgsStrandDesignTool::ResetEndZoneStrandConfig()
{
   if ( IsDesignHarping() )
   {
      return ResetHarpedStrandConfiguration();
   }
   else if( IsDesignDebonding() )
   {
      // Set debonding to max allowable
      bool bResult = MaximizeDebonding();
      if (bResult)
      {
         bResult = LayoutDebonding(m_MaxPhysicalDebondLevels);
      }

      return bResult;
   }
   else
   {
      // straight design, nothing to do
      return true;
   }
}

bool pgsStrandDesignTool::ResetHarpedStrandConfiguration()
{
   ATLASSERT(this->IsDesignHarping());
   LOG(_T("Raising harped strands to maximum height at girder ends"));

   StrandIndexType nh = m_pArtifact->GetNumHarpedStrands();
   if (nh == 0)
   {
      m_bConfigDirty = true; // cache is dirty
      m_pArtifact->SetHarpStrandOffsetEnd(pgsTypes::metStart,0.0);
      m_pArtifact->SetHarpStrandOffsetHp(pgsTypes::metStart,0.0);
      m_pArtifact->SetHarpStrandOffsetHp(pgsTypes::metEnd,0.0);
      m_pArtifact->SetHarpStrandOffsetEnd(pgsTypes::metEnd,0.0);
   }
   else
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);
      ConfigStrandFillVector fillvec = pStrandGeom->ComputeStrandFill(m_SegmentKey, pgsTypes::Harped, nh);

      // if allowed, put end strands at top and harp point strands at lowest input position
      Float64 end_offset_inc = this->GetHarpedEndOffsetIncrement(pStrandGeom);

      Float64 end_lower_bound, end_upper_bound;
      // cant adjust if we're not allowed.
      if (0.0 < end_offset_inc)
      {
         pStrandGeom->GetHarpedEndOffsetBoundsEx(m_GirderEntryName.c_str(), pgsTypes::metStart,pgsTypes::asHarped, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd, fillvec, &end_lower_bound, &end_upper_bound);
         m_pArtifact->SetHarpStrandOffsetEnd(pgsTypes::metStart,end_upper_bound);

         pStrandGeom->GetHarpedEndOffsetBoundsEx(m_GirderEntryName.c_str(), pgsTypes::metEnd,pgsTypes::asHarped, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd, fillvec, &end_lower_bound, &end_upper_bound);
         m_pArtifact->SetHarpStrandOffsetEnd(pgsTypes::metEnd,end_upper_bound);

         m_bConfigDirty = true; // cache is dirty
      }

      Float64 hp_offset_inc = this->GetHarpedHpOffsetIncrement(pStrandGeom);

      // cant adjust if we're not allowed.
      Float64 hp_lower_bound, hp_upper_bound;
      if (0.0 < hp_offset_inc)
      {
         pStrandGeom->GetHarpedHpOffsetBoundsEx(m_GirderEntryName.c_str(), pgsTypes::metStart, pgsTypes::asHarped, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd, fillvec, &hp_lower_bound, &hp_upper_bound);
         m_pArtifact->SetHarpStrandOffsetHp(pgsTypes::metStart,hp_lower_bound);

         pStrandGeom->GetHarpedHpOffsetBoundsEx(m_GirderEntryName.c_str(), pgsTypes::metEnd, pgsTypes::asHarped, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd, fillvec, &hp_lower_bound, &hp_upper_bound);
         m_pArtifact->SetHarpStrandOffsetHp(pgsTypes::metEnd,hp_lower_bound);

         m_bConfigDirty = true; // cache is dirty
      }

      if (m_DoDesignForStrandSlope)
      {
         if ( !AdjustForStrandSlope() )
         {
            return false;
         }
      }

      if (m_DoDesignForHoldDownForce)
      {
         if ( !AdjustForHoldDownForce() )
         {
            return false;
         }
      }

   }

   return true;
}

void pgsStrandDesignTool::ComputeMinStrands()
{
   // It's possible for users to enter strands at the beginning of the fill sequence that have
   // negative eccentricity. This is typically for hanging stirrups. If this occurs then design will crap out

   LOG(_T("Compute m_MinPermanentStrands so next num strands give positive ecc"));

   m_MinPermanentStrands = GetNextNumPermanentStrands(0);
   if(m_MinPermanentStrands == INVALID_INDEX)
   {
      return;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);

   PoiList mid_pois;
   GetDesignPoi(releaseIntervalIdx, POI_5L | POI_RELEASED_SEGMENT, &mid_pois);
   if (mid_pois.empty())
   {
      ATLASSERT(false); // no-midspan? this shouldn't happen, but take a default and carry on
   }
   else
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);

      GDRCONFIG config = m_pArtifact->GetSegmentConfiguration();

      StrandIndexType ns_prev = 1;
      StrandIndexType ns_curr = GetNextNumPermanentStrands(ns_prev);

      // Compute min number of strands in order to get a positive ecc
      int nIter = 0;
      while (0 < ns_curr)
      {
         StrandIndexType ns, nh;
         if (m_pRaisedStraightStrandDesignTool)
         {
            m_pRaisedStraightStrandDesignTool->ComputeNumStrands(ns_curr, &ns, &nh);
            if (ns==INVALID_INDEX)
            {
               ATLASSERT(0); // caller should have figured out if numPerm is valid
               m_MinPermanentStrands = 0;
               return;
            }
         }
         else if (m_StrandFillType == ftGridOrder)
         {
            if (!pStrandGeom->ComputeNumPermanentStrands(ns_curr,m_SegmentKey, &ns, &nh))
            {
               ATLASSERT(false); // caller should have figured out if numPerm is valid
               m_MinPermanentStrands = 0;
               return;
            }
         }
         else if (m_StrandFillType == ftMinimizeHarping)
         {
            if (ComputeNextNumProportionalStrands(ns_curr-1, &ns, &nh) == INVALID_INDEX)
            {
               ATLASSERT(false);
               m_MinPermanentStrands = 0;
               return;
            }
         }
         else
         {
            ATLASSERT(false);
         }

         ConfigStrandFillVector sfillvec = pStrandGeom->ComputeStrandFill(m_SegmentKey, pgsTypes::Straight, ns);
         //ConfigStrandFillVector hfillvec = pStrandGeom->ComputeStrandFill(m_SegmentKey, pgsTypes::Harped, nh);
         ConfigStrandFillVector hfillvec;
         if (m_pRaisedStraightStrandDesignTool)
         {
            hfillvec = m_pRaisedStraightStrandDesignTool->CreateStrandFill(GirderLibraryEntry::stAdjustable, nh);
         }
         else
         {
            hfillvec = pStrandGeom->ComputeStrandFill(m_SegmentKey, pgsTypes::Harped, nh);
         }

         config.PrestressConfig.SetStrandFill(pgsTypes::Straight, sfillvec);
         config.PrestressConfig.Debond[pgsTypes::Straight].clear(); // clear out any debonding if we are in design loop
         config.PrestressConfig.SetStrandFill(pgsTypes::Harped,   hfillvec);

         Float64 ecc = pStrandGeom->GetEccentricity(releaseIntervalIdx,mid_pois[0],pgsTypes::Permanent,&config).Y();
         LOG(_T("Computed ecc = ")<<ecc<<_T(" for ns=")<<ns<<_T(" nh=")<<nh);
         if ( ::IsGT(0.0,ecc) )
         {
            if (nIter == 0)
            {
               // Setting strand to a minimal number seems to give optimal results for cases without top strands
               m_MinPermanentStrands = GetNextNumPermanentStrands(0);
               LOG(_T("Eccentricity positive on first iteration - m_MinPermanentStrands = ") << m_MinPermanentStrands << _T(" Success"));
            }
            else
            {
               // TRICKY: Just finding the point where eccentricity is postitive turns out not to be enough.
               //         The design algorithm will likely get stuck. So we double it.
               m_MinPermanentStrands = GetNextNumPermanentStrands(2*ns_prev);
               LOG(_T("Found m_MinPermanentStrands = ") << m_MinPermanentStrands << _T("Success"));
            }

            break;
         }
         else
         {
            ns_prev = ns_curr;
            ns_curr = GetNextNumPermanentStrands(ns_prev);

            if (ns_curr == INVALID_INDEX)
            {
               LOG(_T("**WARNING: Could not find number of strands to create positive eccentricity. The end is likely near..."));
            }
         }

         nIter++;
      }
   }
}


bool pgsStrandDesignTool::AdjustForStrandSlope()
{
   ATLASSERT(!m_pRaisedStraightStrandDesignTool);
   ATLASSERT(m_DoDesignForStrandSlope); // should not be calling this


   GET_IFACE(IStrandGeometry,pStrandGeom);
   const GDRCONFIG& config = GetSegmentConfiguration();
   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;

      pgsPointOfInterest poi(m_SegmentKey, endType == pgsTypes::metStart ? 0.0 : m_SegmentLength);
      Float64 slope = pStrandGeom->GetMaxStrandSlope(poi, &config);

#if defined _DEBUG
      if (slope != DBL_MAX && slope != -DBL_MAX)
      {
         if (endType == pgsTypes::metStart)
         {
            ATLASSERT(slope < 0); // we got the slope at the start of the girder
                                  // it should be going downward to the right... this is a negative slope
         }
         else
         {
            ATLASSERT(0 < slope); // we got the slope at the end of the girder
                                  // it should be going upward to the right... this is a positive slope
         }
      }
#endif
      slope = fabs(slope); // all the code below is based on the slope being a positive value so get its absolute value


      LOG(_T("Design for Maximum Strand Slope at ") << (pgsTypes::metStart ? _T("start") : _T("end")) << _T(" of girder"));
      LOG(_T("Maximum Strand Slope 1 : ") << m_AllowableStrandSlope);
      LOG(_T("Actual  Strand Slope 1 : ") << slope);

      Float64 adj = 0.0;
      if ( slope < m_AllowableStrandSlope)
      {
         LOG(_T("Strand slope needs adjusting"));
         LOG(_T("Current Start offset = ")<< WBFL::Units::ConvertFromSysUnits(config.PrestressConfig.EndOffset[pgsTypes::metStart],WBFL::Units::Measure::Inch) << _T(" in"));
         LOG(_T("Current HP1  offset = ")<< WBFL::Units::ConvertFromSysUnits(config.PrestressConfig.HpOffset[pgsTypes::metStart],WBFL::Units::Measure::Inch) << _T(" in"));
         LOG(_T("Current HP2  offset = ")<< WBFL::Units::ConvertFromSysUnits(config.PrestressConfig.HpOffset[pgsTypes::metEnd],WBFL::Units::Measure::Inch) << _T(" in"));
         LOG(_T("Current End  offset = ")<< WBFL::Units::ConvertFromSysUnits(config.PrestressConfig.EndOffset[pgsTypes::metEnd],WBFL::Units::Measure::Inch) << _T(" in"));

         if ( !AdjustStrandsForSlope(m_AllowableStrandSlope, slope, endType, config.PrestressConfig.GetStrandCount(pgsTypes::Harped), pStrandGeom))
         {
            LOG(_T("** DESIGN FAILED ** We cannot adjust Strands to design for allowable strand slope"));
            m_pArtifact->SetOutcome(pgsSegmentDesignArtifact::StrandSlopeOutOfRange);
            return false;
         }

         LOG(_T("** Adjusted Start offset = ")<< WBFL::Units::ConvertFromSysUnits(GetHarpStrandOffsetEnd(pgsTypes::metStart) ,WBFL::Units::Measure::Inch) << _T(" in"));
         LOG(_T("** Adjusted HP1   offset = ")<< WBFL::Units::ConvertFromSysUnits(GetHarpStrandOffsetHp(pgsTypes::metStart) ,WBFL::Units::Measure::Inch) << _T(" in"));
         LOG(_T("** Adjusted HP2   offset = ")<< WBFL::Units::ConvertFromSysUnits(GetHarpStrandOffsetHp(pgsTypes::metEnd) ,WBFL::Units::Measure::Inch) << _T(" in"));
         LOG(_T("** Adjusted End   offset = ")<< WBFL::Units::ConvertFromSysUnits(GetHarpStrandOffsetEnd(pgsTypes::metEnd) ,WBFL::Units::Measure::Inch) << _T(" in"));
         LOG(_T("New slope is 1 : ")<< pStrandGeom->GetMaxStrandSlope( pgsPointOfInterest(m_SegmentKey,0.00), &GetSegmentConfiguration()) );
      }
   }

   return true;
}

bool pgsStrandDesignTool::AdjustForHoldDownForce()
{
   LOG(_T("Design for Maximum Hold Down Force"));

   ATLASSERT(m_DoDesignForHoldDownForce); // should not be calling this

   StrandIndexType Nh = m_pArtifact->GetNumHarpedStrands();
   if (Nh == 0)
   {
      LOG(_T("No harped strands, no hold down force"));
      return true;
   }

   GET_IFACE(IBridge, pBridge);
   Float64 Ls = pBridge->GetSegmentLength(m_SegmentKey);

   // could use interface to get hdf directly, but need raw values to adjust
   const GDRCONFIG& config = GetSegmentConfiguration();

   GET_IFACE(IPretensionForce,pPrestressForce);
   pgsPointOfInterest poi; // poi where max hold down force occurs
   Float64 slope; // slope associated with max hold down force
   Float64 maxHFT = pPrestressForce->GetHoldDownForce(m_SegmentKey, m_bTotalHoldDownForce, &slope, &poi, &config);

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType strandStressingIntervalIdx = pIntervals->GetStressStrandInterval(m_SegmentKey);
   Float64 Ph = pPrestressForce->GetPrestressForce(poi, pgsTypes::Harped, strandStressingIntervalIdx, pgsTypes::Start, pgsTypes::tltMinimum, &config);
   Float64 strand_force = Ph * (1 + m_HoldDownFriction);
   if (!m_bTotalHoldDownForce)
   {
      strand_force /= Nh;
   }

#if defined _DEBUG
   // verify total harped strand force, slope, and vertical (hold down) harped strand force
   // are all consistent
   GET_IFACE(IStrandGeometry, pStrandGeometry);
   Float64 _slope;
   if (m_bTotalHoldDownForce)
   {
      _slope = pStrandGeometry->GetAvgStrandSlope(poi, &config);
   }
   else
   {
      _slope = pStrandGeometry->GetMaxStrandSlope(poi, &config);
   }
   ATLASSERT(IsEqual(slope, _slope));
   Float64 Pv = strand_force / sqrt(1 + _slope*_slope);
   ATLASSERT(IsEqual(Pv, maxHFT));
#endif

   LOG(_T("P Harp = ") << WBFL::Units::ConvertFromSysUnits(strand_force, WBFL::Units::Measure::Kip) << _T(" kip"));
   LOG(_T("Current slope = 1 : ") << slope);
   LOG(_T("Actual  HD = ") << WBFL::Units::ConvertFromSysUnits(maxHFT, WBFL::Units::Measure::Kip) << _T(" kip"));
   LOG(_T("Maximum HD = ") << WBFL::Units::ConvertFromSysUnits(m_AllowableHoldDownForce,WBFL::Units::Measure::Kip) << _T(" kip"));

   Float64 adj = 0.0;
   if ( m_AllowableHoldDownForce < maxHFT )
   {
      LOG(_T("Hold down force exceeds max, strands need adjustment"));

      pgsTypes::MemberEndType endType = (poi.GetDistFromStart() < Ls / 2 ? pgsTypes::metStart : pgsTypes::metEnd);

      // slope required for allowable hold down
      Float64 sl_reqd = sqrt( (strand_force*strand_force)/(m_AllowableHoldDownForce*m_AllowableHoldDownForce)-1.0 );
      if (endType == pgsTypes::metStart)
      {
         sl_reqd *= -1;
      }

      ATLASSERT(::BinarySign(slope) == ::BinarySign(sl_reqd));
      ATLASSERT(fabs(slope) < fabs(sl_reqd));

      LOG(_T("Slope required = 1 : ")<< sl_reqd);
      LOG(_T("Current Start offset = ")<< WBFL::Units::ConvertFromSysUnits(config.PrestressConfig.EndOffset[pgsTypes::metStart],WBFL::Units::Measure::Inch) << _T(" in"));
      LOG(_T("Current HP1 offset = ")<< WBFL::Units::ConvertFromSysUnits(config.PrestressConfig.HpOffset[pgsTypes::metStart],WBFL::Units::Measure::Inch) << _T(" in"));
      LOG(_T("Current HP2 offset = ")<< WBFL::Units::ConvertFromSysUnits(config.PrestressConfig.HpOffset[pgsTypes::metEnd],WBFL::Units::Measure::Inch) << _T(" in"));
      LOG(_T("Current End offset = ")<< WBFL::Units::ConvertFromSysUnits(config.PrestressConfig.EndOffset[pgsTypes::metEnd],WBFL::Units::Measure::Inch) << _T(" in"));

      GET_IFACE(IStrandGeometry, pStrandGeom);
      if ( !AdjustStrandsForSlope(sl_reqd, slope, endType, Nh, pStrandGeom) )
      {
         LOG(_T("** DESIGN FAILED ** We cannot adjust Strands to design for allowable hold down"));
         m_pArtifact->SetOutcome(pgsSegmentDesignArtifact::ExceededMaxHoldDownForce);
         return false;
      }

      LOG(_T("** Adjusted Start offset = ")<< WBFL::Units::ConvertFromSysUnits(GetHarpStrandOffsetEnd(pgsTypes::metStart) ,WBFL::Units::Measure::Inch) << _T(" in"));
      LOG(_T("** Adjusted HP1   offset = ")<< WBFL::Units::ConvertFromSysUnits(GetHarpStrandOffsetHp(pgsTypes::metStart) ,WBFL::Units::Measure::Inch) << _T(" in"));
      LOG(_T("** Adjusted HP2   offset = ")<< WBFL::Units::ConvertFromSysUnits(GetHarpStrandOffsetHp(pgsTypes::metEnd) ,WBFL::Units::Measure::Inch) << _T(" in"));
      LOG(_T("** Adjusted End   offset = ")<< WBFL::Units::ConvertFromSysUnits(GetHarpStrandOffsetEnd(pgsTypes::metEnd) ,WBFL::Units::Measure::Inch) << _T(" in"));
      if (m_bTotalHoldDownForce)
      {
         LOG(_T("New avg slope is 1 : ") << pStrandGeom->GetAvgStrandSlope(poi, &GetSegmentConfiguration()));
      }
      else
      {
         LOG(_T("New max strand slope is 1 : ") << pStrandGeom->GetMaxStrandSlope(poi, &GetSegmentConfiguration()));
      }
   }

   return true;
}

bool pgsStrandDesignTool::AdjustStrandsForSlope(Float64 sl_reqd, Float64 slope, pgsTypes::MemberEndType endType,StrandIndexType nh, IStrandGeometry* pStrandGeom)
{
   // compute adjustment distance
   Float64 X1, X2, X3, X4;
   pStrandGeom->GetHarpingPointLocations(m_SegmentKey, &X1, &X2, &X3, &X4);
   Float64 adj1 = (X2-X1) * (1/fabs(slope) - 1/fabs(sl_reqd));
   Float64 adj2 = (X4-X3) * (1/fabs(slope) - 1/fabs(sl_reqd));
   Float64 adj = Max(adj1,adj2);

   LOG(_T("Vertical adjustment required to acheive slope = ")<< WBFL::Units::ConvertFromSysUnits(adj,WBFL::Units::Measure::Inch) << _T(" in"));

   // try to adjust end first
   Float64 end_offset_inc = this->GetHarpedEndOffsetIncrement(pStrandGeom);
   if (0.0 < end_offset_inc && !m_DesignOptions.doForceHarpedStrandsStraight)
   {
      LOG(_T("Attempt to adjust hold down by lowering at ") << (endType == pgsTypes::metStart ? _T("start") : _T("end")) << _T(" ends"));
      Float64 curr_adj = m_pArtifact->GetHarpStrandOffsetEnd(endType);

      const GDRCONFIG& config = GetSegmentConfiguration();

      Float64 end_lower_bound, end_upper_bound;
      pStrandGeom->GetHarpedEndOffsetBoundsEx(m_GirderEntryName.c_str(), endType, pgsTypes::asHarped, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd, config.PrestressConfig.GetStrandFill(pgsTypes::Harped), &end_lower_bound, &end_upper_bound);

      // adjust by round increment
      Float64 end_adj = ::CeilOff(adj, end_offset_inc);
      LOG(_T("End adjustment rounded to increment = ")<< WBFL::Units::ConvertFromSysUnits(end_adj,WBFL::Units::Measure::Inch) << _T(" in"));

      Float64 max_adj = curr_adj - end_lower_bound;
      if (0 < max_adj)
      {
         if (end_adj <= max_adj)
         {
            LOG(_T("Entire adjustment for slope can be taken at girder end - doing so"));
            m_pArtifact->SetHarpStrandOffsetEnd(endType,curr_adj - end_adj);
            m_bConfigDirty = true; // cache is dirty
            adj = 0.0;
         }
         else
         {
            LOG(_T("Partial adjustment for slope can be taken at girder end. Adjusting to ")<< WBFL::Units::ConvertFromSysUnits(end_lower_bound,WBFL::Units::Measure::Inch) << _T(" in"));
            m_pArtifact->SetHarpStrandOffsetEnd(endType,end_lower_bound);
            m_bConfigDirty = true; // cache is dirty
            adj -= (curr_adj-end_lower_bound);
            LOG(_T("reminder of adjustment required = ")<< WBFL::Units::ConvertFromSysUnits(adj,WBFL::Units::Measure::Inch) << _T(" in"));
         }
      }
      else
      {
         LOG(_T("Strands at end already adjusted as low as possible"));
      }
   }

   // we've done what we can at the end. see if we need to adjust at hp
   Float64 hp_offset_inc = this->GetHarpedHpOffsetIncrement(pStrandGeom);
   if (0.0 < adj && 0.0 < hp_offset_inc)
   {
      LOG(_T("Attempt to adjust Strand slope by raising at ") << (endType == pgsTypes::metStart ? _T("left") : _T("right")) << _T(" HP"));
      Float64 curr_adj = m_pArtifact->GetHarpStrandOffsetHp(endType);
      Float64 hp_lower_bound, hp_upper_bound;

      const GDRCONFIG& config = GetSegmentConfiguration();
      pStrandGeom->GetHarpedHpOffsetBoundsEx(m_GirderEntryName.c_str(), endType, pgsTypes::asHarped, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd, config.PrestressConfig.GetStrandFill(pgsTypes::Harped), &hp_lower_bound, &hp_upper_bound);

      Float64 max_adj = hp_upper_bound - curr_adj;
      if (0 < max_adj)
      {
         if (adj <= max_adj)
         {
            LOG(_T("Entire adjustment for slope can be taken at girder HP - doing so"));
            m_pArtifact->SetHarpStrandOffsetHp(endType,curr_adj + adj);
            m_bConfigDirty = true; // cache is dirty
            adj = 0.0;
         }
      }
      else
      {
         LOG(_T("Strands at HP already adjusted as high as possible - cannot acheive target slope reduction"));
      }
   }

   // see if we made our adjustment
   return (adj == 0.0) ? true : false;
}

bool pgsStrandDesignTool::SwapStraightForHarped()
{
   ATLASSERT(!m_pRaisedStraightStrandDesignTool);
   LOG(_T("Attempting to change strand proportions by moving straight strands into harped pattern"));
   StrandIndexType Ns = GetNs();
   StrandIndexType Nh = GetNh();
   if ( Ns < 2 )
   {
      return false;
   }

   return SetNumStraightHarped(Ns-2,Nh+2);
}

bool pgsStrandDesignTool::AddStrands()
{
   LOG(_T("Attempting to add permanent strands"));

   StrandIndexType Ns = GetNs();
   StrandIndexType Nh = GetNh();
   StrandIndexType Np = Ns + Nh;


   LOG(_T("Current configuration -> Ns = ") << Ns << _T(" Nh = ") << Nh <<_T(" Nperm = ") <<Np);

   StrandIndexType nextNp = this->GetNextNumPermanentStrands(Np);
   if ( nextNp != INVALID_INDEX )
   {
      LOG(_T("Adding ") << (nextNp - Np) << _T(" permanent strands"));
      if (!SetNumPermanentStrands(nextNp))
      {
         return false;
      }

   }
   else
   {
      LOG(_T("Number of strands exceed maximum for this girder"));
      return false;
   }

   Ns = GetNs();
   Nh = GetNh();

   LOG(_T("** Successfully added strands -> Ns = ") << Ns << _T(" Nh = ") << Nh);

   return KeepHarpedStrandsInBounds();
}

bool pgsStrandDesignTool::AddTempStrands()
{
   LOG(_T("Attempting to add temporary strands"));

   GET_IFACE(IStrandGeometry,pStrandGeom);

   StrandIndexType Nt = m_pArtifact->GetNumTempStrands();

   LOG(_T("Current configuration -> Nt = ") << Nt);
   StrandIndexType nextNt = pStrandGeom->GetNextNumStrands(m_SegmentKey,pgsTypes::Temporary,Nt);
   if ( nextNt == 0 || nextNt == INVALID_INDEX )
   {
      LOG(_T("Number of temp strand exceeds maximum for this girder"));
      return false;
   }
   else
   {
      GET_IFACE(IPretensionForce,pPrestressForce);

      LOG(_T("Adding ") << (nextNt - Nt) << _T(" temporary strands"));
      LOG(_T("** Successfully added strands -> Nt = ") << nextNt);
      m_pArtifact->SetNumTempStrands( nextNt );
      m_pArtifact->SetPjackTempStrands( pPrestressForce->GetPjackMax(m_SegmentKey,pgsTypes::Temporary,nextNt) );
      m_pArtifact->SetUsedMaxPjackTempStrands( true );
      m_bConfigDirty = true; // cache is dirty

      return true;
   }
}

bool pgsStrandDesignTool::AddRaisedStraightStrands()
{
   LOG(_T("** Attempting to add raised straight strands"));
   if ( IsDesignRaisedStraight() )
   {
      ATLASSERT(m_pRaisedStraightStrandDesignTool); // better be alive

      return m_pRaisedStraightStrandDesignTool->AddRaisedStraightStrands();
   }
   else
   {
      // not raised straight design
      LOG(_T("Raised straight strands design not available for this girder type"));
      return false;
   }
}

void pgsStrandDesignTool::SimplifyDesignFillOrder(pgsSegmentDesignArtifact* pArtifact) const
{
   // This only can happen for raised straight designs. For this case, it is assummed that
   // we filled using direct fill. However, if we did not, we simply used girder fill order
   if ( IsDesignRaisedStraight() )
   {
      ATLASSERT(m_pRaisedStraightStrandDesignTool); // better be alive
      if(m_pRaisedStraightStrandDesignTool->GetNumUsedRaisedStrandLocations() == 0)
      {
         LOG(_T("** A raised strand design was specified, but no raised strands were used. Change fill type back to grid order."));
         arDesignOptions options = pArtifact->GetDesignOptions();

         options.doStrandFillType = ftGridOrder;

         // Set design strategy to simpler version
         if( dtDesignFullyBondedRaised  == options.doDesignForFlexure )
         {
            options.doDesignForFlexure = dtDesignFullyBonded;
         }
         else if ( dtDesignForDebondingRaised == options.doDesignForFlexure )
         {
            options.doDesignForFlexure = dtDesignForDebonding;
         }

         pArtifact->SetDesignOptions(options);
      }
   }
}

// predicate class to sort debond info by location
class DebondInfoSorter
{
public:
   int operator() (const DEBONDCONFIG& db1,const DEBONDCONFIG& db2)
   {
      if (db1.DebondLength[pgsTypes::metStart] == db2.DebondLength[pgsTypes::metStart])
      {
         return db1.strandIdx < db2.strandIdx;
      }
      else
      {
         return db1.DebondLength[pgsTypes::metStart] < db2.DebondLength[pgsTypes::metStart];
      }
   }
};

void pgsStrandDesignTool::DumpDesignParameters() const
{
#if defined ENABLE_LOGGING

   GET_IFACE(IStrandGeometry,pStrandGeom);

   const GDRCONFIG& config = GetSegmentConfiguration();
   const ConfigStrandFillVector& fillvec = config.PrestressConfig.GetStrandFill(pgsTypes::Harped);

   Float64 start_offset = pStrandGeom->ComputeHarpedOffsetFromAbsoluteEnd(m_GirderEntryName.c_str(), pgsTypes::metStart, pgsTypes::asHarped, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd, fillvec, hsoTOP2TOP, m_pArtifact->GetHarpStrandOffsetEnd(pgsTypes::metStart));
   Float64 hp1_offset  = pStrandGeom->ComputeHarpedOffsetFromAbsoluteHp(m_GirderEntryName.c_str(), pgsTypes::metStart, pgsTypes::asHarped, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd, fillvec,  hsoBOTTOM2BOTTOM, m_pArtifact->GetHarpStrandOffsetHp(pgsTypes::metStart));
   Float64 hp2_offset  = pStrandGeom->ComputeHarpedOffsetFromAbsoluteHp(m_GirderEntryName.c_str(), pgsTypes::metEnd, pgsTypes::asHarped, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd, fillvec,  hsoBOTTOM2BOTTOM, m_pArtifact->GetHarpStrandOffsetHp(pgsTypes::metEnd));
   Float64 end_offset = pStrandGeom->ComputeHarpedOffsetFromAbsoluteEnd(m_GirderEntryName.c_str(), pgsTypes::metEnd, pgsTypes::asHarped, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd, fillvec, hsoTOP2TOP, m_pArtifact->GetHarpStrandOffsetEnd(pgsTypes::metEnd));

   LOG(_T(""));
   LOG(_T("---------------------------------------------------------------"));
   LOG(_T("Current design parameters"));
   LOG(_T(""));
   LOG(_T("f'c  = ") << WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetConcreteStrength(),WBFL::Units::Measure::KSI) << _T(" KSI"));
   LOG(_T("f'ci = ") << WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetReleaseStrength(),WBFL::Units::Measure::KSI) << (m_ReleaseStrengthResult==ConcSuccessWithRebar?_T(" KSI - Min Rebar Required "):_T(" KSI")) );
   LOG(_T("Np = ") << m_pArtifact->GetNumStraightStrands()+ m_pArtifact->GetNumHarpedStrands());
   LOG(_T("Ns = ") << m_pArtifact->GetNumStraightStrands() << _T("   Pjack = ") << WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetPjackStraightStrands(), WBFL::Units::Measure::Kip) << _T(" Kip"));
   LOG(_T("Nh = ") << m_pArtifact->GetNumHarpedStrands() << _T("   Pjack = ") << WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetPjackHarpedStrands(), WBFL::Units::Measure::Kip) << _T(" Kip"));
   LOG(_T("Nt = ") << m_pArtifact->GetNumTempStrands() << _T("   Pjack = ") << WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetPjackTempStrands(), WBFL::Units::Measure::Kip) << _T(" Kip"));
   LOG(_T("HP Offset at Start = ") << WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetHarpStrandOffsetEnd(pgsTypes::metStart),WBFL::Units::Measure::Inch) << _T(" in") << _T(" (From top = ") << WBFL::Units::ConvertFromSysUnits(start_offset,WBFL::Units::Measure::Inch) << _T(" in)"));
   LOG(_T("HP Offset at HP1  = ") << WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetHarpStrandOffsetHp(pgsTypes::metStart),WBFL::Units::Measure::Inch) << _T(" in") << _T(" (From bottom = ") << WBFL::Units::ConvertFromSysUnits(hp1_offset,WBFL::Units::Measure::Inch) << _T(" in)"));
   LOG(_T("HP Offset at HP2  = ") << WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetHarpStrandOffsetHp(pgsTypes::metEnd),WBFL::Units::Measure::Inch) << _T(" in") << _T(" (From bottom = ") << WBFL::Units::ConvertFromSysUnits(hp2_offset,WBFL::Units::Measure::Inch) << _T(" in)"));
   LOG(_T("HP Offset at End = ") << WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetHarpStrandOffsetEnd(pgsTypes::metEnd),WBFL::Units::Measure::Inch) << _T(" in") << _T(" (From top = ") << WBFL::Units::ConvertFromSysUnits(end_offset,WBFL::Units::Measure::Inch) << _T(" in)"));
   LOG(_T("Slab Offset at Start = ") << WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetSlabOffset(pgsTypes::metStart),WBFL::Units::Measure::Inch) << _T(" in"));
   LOG(_T("Slab Offset at End   = ") << WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetSlabOffset(pgsTypes::metEnd),WBFL::Units::Measure::Inch) << _T(" in"));
   LOG(_T("Assumed excess Camber   = ") << WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetAssumedExcessCamber(),WBFL::Units::Measure::Inch) << _T(" in"));
   LOG(_T("Pick Point = ") << WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetLeftLiftingLocation(), WBFL::Units::Measure::Feet) << _T(" ft"));
   LOG(_T("Leading Overhang  = ") << WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetLeadingOverhang(), WBFL::Units::Measure::Feet) << _T(" ft"));
   LOG(_T("Trailing Overhang = ") << WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetTrailingOverhang(), WBFL::Units::Measure::Feet) << _T(" ft"));

   DebondConfigCollection dbinfo = m_pArtifact->GetStraightStrandDebondInfo();
   if (!dbinfo.empty())
   {
      LOG(_T("Debonding Information:"));
      // sort debonding by section location. no need to output right and left ends because we are always symmetric
      std::sort(dbinfo.begin(), dbinfo.end(), DebondInfoSorter());

      bool loop = true;
      DebondConfigIterator it = dbinfo.begin();
      while(loop)
      {
         Float64 curr_loc = it->DebondLength[pgsTypes::metStart];
         Float64 last_loc = curr_loc;
         std::_tostringstream os;
         os<<_T("    Strands Debonded at ")<< WBFL::Units::ConvertFromSysUnits(curr_loc, WBFL::Units::Measure::Feet) << _T(" ft: ");
         while(curr_loc == last_loc)
         {
            os << it->strandIdx << _T(", ");

            it++;
            if (it == dbinfo.end())
            {
               loop = false;
               break;
            }
            else
            {
               last_loc = curr_loc;
               curr_loc = it->DebondLength[pgsTypes::metStart];
            }
         }

         std::_tstring str(os.str());
         IndexType n = str.size();
         if (0 < n)
         {
            str.erase(n-2,2); // get rid of trailing _T(", ")
         }

         LOG( str );
      }
   }
   LOG(_T("---------------------------------------------------------------"));
   LOG(_T(""));
#endif
}

void pgsStrandDesignTool::FillArtifactWithFlexureValues()
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(IIntervals,pIntervals);

   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   m_pArtifact->SetNumStraightStrands(pStrandGeom->GetStrandCount(m_SegmentKey,pgsTypes::Straight));
   m_pArtifact->SetNumHarpedStrands(pStrandGeom->GetStrandCount(m_SegmentKey,pgsTypes::Harped));

   m_bConfigDirty = true; // cache is dirty

   m_pArtifact->SetPjackHarpedStrands(pStrandGeom->GetPjack(m_SegmentKey,pgsTypes::Harped));
   m_pArtifact->SetPjackStraightStrands(pStrandGeom->GetPjack(m_SegmentKey,pgsTypes::Straight));
   m_pArtifact->SetUsedMaxPjackStraightStrands( true );
   m_pArtifact->SetUsedMaxPjackHarpedStrands( true );

   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
      Float64 offsetEnd, offsetHP;
      pStrandGeom->GetHarpStrandOffsets(m_SegmentKey,endType,&offsetEnd,&offsetHP);
      m_pArtifact->SetHarpStrandOffsetHp(endType,offsetHP);
      m_pArtifact->SetHarpStrandOffsetEnd(endType,offsetEnd);
   }

   m_pArtifact->SetReleaseStrength(pMaterial->GetSegmentDesignFc(m_SegmentKey,releaseIntervalIdx));
   m_pArtifact->SetConcreteStrength(pMaterial->GetSegmentDesignFc(m_SegmentKey, liveLoadIntervalIdx));

   PierIndexType startPierIdx, endPierIdx;
   pBridge->GetGirderGroupPiers(m_SegmentKey.groupIndex,&startPierIdx,&endPierIdx);
   ATLASSERT(endPierIdx == startPierIdx+1);

   m_pArtifact->SetSlabOffset(pgsTypes::metStart,pBridge->GetSlabOffset(m_SegmentKey,pgsTypes::metStart));
   m_pArtifact->SetSlabOffset(pgsTypes::metEnd,  pBridge->GetSlabOffset(m_SegmentKey,pgsTypes::metEnd));

   m_pArtifact->SetAssumedExcessCamber(pBridge->GetAssumedExcessCamber(m_SegmentKey.groupIndex,m_SegmentKey.girderIndex));

   GDRCONFIG config = pBridge->GetSegmentConfiguration(m_SegmentKey);
   m_pArtifact->SetStraightStrandDebondInfo( config.PrestressConfig.Debond[pgsTypes::Straight] );

   m_bConfigDirty = true; // cache is dirty
}

bool pgsStrandDesignTool::UpdateConcreteStrength(Float64 fcRequired, const StressCheckTask& task,pgsTypes::StressLocation stressLocation)
{
   if (fcRequired < 0) // there is not required concrete strength that works
      return false;

   Float64 fc_current = m_pArtifact->GetConcreteStrength();
   LOG(_T("Update Final Concrete Strength if needed. f'c required = ")<< WBFL::Units::ConvertFromSysUnits(fcRequired,WBFL::Units::Measure::KSI) << _T(" KSI f'c current = ")<< WBFL::Units::ConvertFromSysUnits(fc_current,WBFL::Units::Measure::KSI) << _T(" KSI"));;

   Float64 fc_max = GetMaximumConcreteStrength();

   if (fcRequired != fc_max)
   {
      // round up to nearest 100psi
      fcRequired = CeilOff(fcRequired, m_ConcreteAccuracy );
      LOG(_T("Round up to nearest 100psi. New Required value is now = ")<< WBFL::Units::ConvertFromSysUnits(fcRequired,WBFL::Units::Measure::KSI) << _T(" KSI"));;
   }

   if (fc_max < fcRequired)
   {
      ATLASSERT(false); // should be checked by caller
      LOG(_T("FAILED - f'c cannot exceed ")<< WBFL::Units::ConvertFromSysUnits(fc_max,WBFL::Units::Measure::KSI) << _T(" KSI"));
      return false;
   }

   Float64 fc_min = GetMinimumConcreteStrength();
   if (fcRequired < fc_min)
   {
      LOG(_T("f'c required less than minimum.  No need to update f'c"));
      return false;
   }

   Float64 fci = m_pArtifact->GetReleaseStrength();
   if ( fcRequired < fci )
   {
      LOG(_T("f'c required less than f'ci. Release controls - Set f'c to f'ci"));
      fcRequired = CeilOff(fci, m_ConcreteAccuracy );;
   }

   Float64 newfc;
   if ( m_FcControl.DoUpdate(fcRequired, task,stressLocation,&newfc) )
   {
      m_pArtifact->SetConcreteStrength(newfc);
      m_bConfigDirty = true; // cache is dirty
      LOG(_T("** Updated Final Concrete Strength to ")<< WBFL::Units::ConvertFromSysUnits(newfc,WBFL::Units::Measure::KSI) << _T(" KSI"));
   }
   else
   {
      LOG(_T("A higher concrete strength is required for a different design element. Don't update f'c"));
      return false; // nothing changed
   }

   return true;
}

bool pgsStrandDesignTool::UpdateConcreteStrengthForShear(Float64 fcRequired,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState)
{
   Float64 fc_current = m_pArtifact->GetConcreteStrength();
   LOG(_T("Update Final Concrete Strength for shear stress requirement. f'c required = ")<< WBFL::Units::ConvertFromSysUnits(fcRequired,WBFL::Units::Measure::KSI) << _T(" KSI f'c current = ")<< WBFL::Units::ConvertFromSysUnits(fc_current,WBFL::Units::Measure::KSI) << _T(" KSI"));;

   // round up to nearest 100psi
   fcRequired = CeilOff(fcRequired, m_ConcreteAccuracy );
   LOG(_T("Round up to nearest 100psi. New Required value is now = ")<< WBFL::Units::ConvertFromSysUnits(fcRequired,WBFL::Units::Measure::KSI) << _T(" KSI"));

   Float64 fc_min = GetMinimumConcreteStrength();
   if (fcRequired < fc_min)
   {
      LOG(_T("Required concrete stress is less than minimum. Setting concrete strength to minimum: ") << WBFL::Units::ConvertFromSysUnits(fc_min, WBFL::Units::Measure::KSI) << _T(" KSI"));
      fcRequired = fc_min;
   }

   Float64 fc_max = GetMaximumConcreteStrength();
   if (fc_max < fcRequired)
   {
      ATLASSERT(false); // should be checked by caller
      LOG(_T("FAILED - f'c cannot exceed ")<< WBFL::Units::ConvertFromSysUnits(fc_max,WBFL::Units::Measure::KSI) << _T(" KSI"));
      return false;
   }

   if (fc_current < fcRequired)
   {
   m_FcControl.DoUpdateForShear(fcRequired, intervalIdx, limitState);
   m_pArtifact->SetConcreteStrength(fcRequired);
   m_bConfigDirty = true; // cache is dirty
      LOG(_T("** Updated Final Concrete Strength to ") << WBFL::Units::ConvertFromSysUnits(fcRequired, WBFL::Units::Measure::KSI) << _T(" KSI"));
   }
   else
   {
      LOG(_T("Required f'c is less that current f'c - no change"));
   }

   return true;
}

bool pgsStrandDesignTool::UpdateReleaseStrength(Float64 fciRequired,ConcStrengthResultType strengthResult,const StressCheckTask& task,pgsTypes::StressLocation stressLocation)
{
   LOG(_T("Update Concrete Strength if needed. f'ci required = ")<< WBFL::Units::ConvertFromSysUnits(fciRequired,WBFL::Units::Measure::KSI) << _T(" KSI"));;

   Float64 fci_current = m_pArtifact->GetConcreteStrength();

   Float64 fci_min = GetMinimumReleaseStrength();
   if( fciRequired < fci_min )
   {
      LOG(_T("f'c min = ") << WBFL::Units::ConvertFromSysUnits(fci_min,WBFL::Units::Measure::KSI) << _T(" KSI"));
      LOG(_T("f'ci cannot be less than min"));

      fciRequired = fci_min;
      LOG(_T("f'ci required now = ") << WBFL::Units::ConvertFromSysUnits(fciRequired,WBFL::Units::Measure::KSI) << _T(" KSI"));
   }


   fciRequired = CeilOff(fciRequired, m_ConcreteAccuracy );
   LOG(_T("Round up to nearest 100psi. Required value is now = ")<< WBFL::Units::ConvertFromSysUnits(fciRequired,WBFL::Units::Measure::KSI) << _T(" KSI"));;

   LOG(_T("Required fully adjusted f'ci now = ")<< WBFL::Units::ConvertFromSysUnits(fciRequired,WBFL::Units::Measure::KSI) << _T(" KSI, Current = ")<< WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetReleaseStrength(), WBFL::Units::Measure::KSI) << _T(" KSI"));

   Float64 fci;
   if ( m_FciControl.DoUpdate(fciRequired,task,stressLocation,&fci) )
   {
      LOG(_T("** Setting new release strength to  = ")<< WBFL::Units::ConvertFromSysUnits(fci, WBFL::Units::Measure::KSI) << _T(" KSI"));;
      m_pArtifact->SetReleaseStrength(fci);
      m_bConfigDirty = true; // cache is dirty

      ATLASSERT(strengthResult != ConcFailed); // this should always be blocked

      // new compression controlled values cannot override the need for minimum rebar
      if (task.stressType == pgsTypes::Tension)
      {
         m_ReleaseStrengthResult = strengthResult;
      }
   }
   else
   {
      // allow tension to override the need to use min rebar even if concrete strenth doesn't need change
      if (task.stressType == pgsTypes::Tension && strengthResult == ConcSuccessWithRebar)
      {
         if (m_ReleaseStrengthResult != ConcSuccessWithRebar)
         {
            // Note: This logic here might require more treatment if designs are coming up requiring
            //       min rebar when is is not desired. If this is the case, some serious thought must
            //       be put into m_FciControl.
            LOG(_T("Tensile demand requires minimum rebar - allow it, even though current strength does not require"));
            m_ReleaseStrengthResult = strengthResult;
         }
      }

      LOG(_T("A higher release strength is required for a different design element. Don't update f'ci"));
      return false; // nothing changed
   }

   // Release strength can drive final.
   if (UpdateConcreteStrength(fci,task,stressLocation))
   {
      LOG(_T("** Concrete strength changed by change in release strength"));
   }

   return true;
}

ConcStrengthResultType pgsStrandDesignTool::ComputeRequiredConcreteStrength(Float64 fControl,const StressCheckTask& task,Float64* pfc) const
{
   LOG(_T("Entering ComputeRequiredConcreteStrength"));
   Float64 fc_reqd;

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(GetSegmentKey());

   ConcStrengthResultType result = ConcSuccess;

#pragma Reminder("UPDATE - Call IAllowableConcreteStress::GetRequiredConcreteStrength instead of having duplicate code here")
   // IAllowableConcreteStress::GetRequiredConcreteStrength isn't exactly the same as this. It doesn't treat the tension with rebar case
   // the same. Also, the current return information doesn't tell use if the concrete strength required for tension is for with or without rebar case.
   pgsPointOfInterest dummyPOI(m_SegmentKey,0.0);
   if ( task.stressType == pgsTypes::Compression )
   {
      GET_IFACE(IAllowableConcreteStress,pAllowStress);
      Float64 c = -pAllowStress->GetAllowableCompressionStressCoefficient(dummyPOI,pgsTypes::TopGirder,task);
      fc_reqd = fControl/c;
      LOG(c << _T("F demand (compression) = ") << WBFL::Units::ConvertFromSysUnits(fControl,WBFL::Units::Measure::KSI) << _T(" KSI") << _T(" --> f'c (req'd unrounded) = ") << WBFL::Units::ConvertFromSysUnits(fc_reqd,WBFL::Units::Measure::KSI) << _T(" KSI"));
   }
   else
   {
      GET_IFACE(IMaterials,pMaterials);
      if (pMaterials->GetSegmentConcreteType(GetSegmentKey()) == pgsTypes::PCI_UHPC)
      {
         if (task.intervalIdx == releaseIntervalIdx)
         {
            const auto& pConcrete = pMaterials->GetSegmentConcrete(GetSegmentKey());
            const lrfdLRFDConcreteBase* pLRFDConcrete = dynamic_cast<const lrfdLRFDConcreteBase*>(pConcrete.get());
            Float64 f_fc = pLRFDConcrete->GetFirstCrackingStrength();
            Float64 fc = GetConcreteStrength();
            fc_reqd = pow(1.5 * fControl / fc, 2) * fc;
            // stress limit = (2/3)ffc*sqrt(f'ci/fc);
         }
         else
         {
            fc_reqd = 0; // stress limit = (2/3)(ffc) which is not a function of f'c
         }
      }
      else if (pMaterials->GetSegmentConcreteType(GetSegmentKey()) == pgsTypes::UHPC)
      {
         fc_reqd = 0; // stress limit = gamma.u * ft,loc which is not a function of f'c
      }
      else
      {
         Float64 lambda = pMaterials->GetSegmentLambda(GetSegmentKey());

         fc_reqd = -1;
         if (0 < fControl)
         {
            Float64 t, fmax;
            bool bfMax;

            GET_IFACE(IAllowableConcreteStress, pAllowStress);
            pAllowStress->GetAllowableTensionStressCoefficient(dummyPOI, pgsTypes::TopGirder, task, false/*without rebar*/, false, &t, &bfMax, &fmax);
            if (0 < t)
            {
               LOG(_T("f allow coeff = ") << WBFL::Units::ConvertFromSysUnits(t, WBFL::Units::Measure::SqrtKSI) << _T("_/f'c = ") << WBFL::Units::ConvertFromSysUnits(fControl, WBFL::Units::Measure::KSI));
               fc_reqd = pow(fControl / (lambda * t), 2);

               if (bfMax && fmax < fControl)
               {
                  // allowable stress is limited to value lower than needed
                  if (task.intervalIdx == releaseIntervalIdx)
                  {
                     // try getting the alternative allowable if rebar is used
                     bool bCheckMaxAlt;
                     Float64 fMaxAlt;
                     Float64 talt;

                     ATLASSERT(task.limitState == pgsTypes::ServiceI && task.stressType == pgsTypes::Tension);

                     pAllowStress->GetAllowableTensionStressCoefficient(dummyPOI, pgsTypes::TopGirder, task, true/*with rebar*/, false/*in other than precompressed tensile zone*/, &talt, &bCheckMaxAlt, &fMaxAlt);
                     fc_reqd = pow(fControl / (lambda * talt), 2);
                     result = ConcSuccessWithRebar;
                     LOG(_T("Min rebar is required to achieve required strength"));
                  }
                  else
                  {
                     LOG(_T("Required strength is greater than spec defined upper limit of ") << WBFL::Units::ConvertFromSysUnits(fmax, WBFL::Units::Measure::KSI) << _T(", cannot achieve strength"));
                     fc_reqd = -1;
                     return ConcFailed;
                  }
               }
            }
            else
            {
               // stress coeff is zero, no tensile capacity
               fc_reqd = -1;
               LOG(_T("WARNING: Have applied tension with zero tension allowed - Should not happen"));
               //ATLASSERT(false);
               return ConcFailed;
            }

            LOG(_T("F demand (tension) = ") << WBFL::Units::ConvertFromSysUnits(fControl, WBFL::Units::Measure::KSI) << _T(" KSI") << _T(" --> f'c (req'd unrounded) = ") << WBFL::Units::ConvertFromSysUnits(fc_reqd, WBFL::Units::Measure::KSI) << (result == ConcSuccessWithRebar ? _T(" KSI, min rebar required") : _T(" KSI")));

         }
         else
         {
            // this is a tension case, but the controlling stress is compressive
            // the lowest required concrete strength is 0.
            fc_reqd = 0;
         }
      }
   }

   Float64 fc_min = (task.intervalIdx == releaseIntervalIdx) ? GetMinimumReleaseStrength() : GetMinimumConcreteStrength();

   Float64 fc_max = (task.intervalIdx == releaseIntervalIdx) ? GetMaximumReleaseStrength() : GetMaximumConcreteStrength();
   if ( fc_reqd < fc_min )
   {
      fc_reqd = fc_min;
      LOG(_T("Required strength less than minimum... setting f'c = ") << WBFL::Units::ConvertFromSysUnits(fc_reqd,WBFL::Units::Measure::KSI) << _T(" KSI"));
   }
   else if ( fc_max < fc_reqd )
   {
      // try setting to max if current strength is not already there
      if (GetConcreteStrength() < fc_max)
      {
         fc_reqd = fc_max;
         LOG(_T("Required strength exceeds that allowed by 5.4.2.1 - try setting f'c to max for one iteration = ") << WBFL::Units::ConvertFromSysUnits(fc_reqd,WBFL::Units::Measure::KSI) << _T(" KSI"));
      }
      else
      {
         // we've tried the max and it didn't work, time to punt
         LOG(_T("*** Required strength exceeds that allowed by 5.4.2.1 ***- and we've tried max - Punt"));
         return ConcFailed;
      }
   }

   *pfc = fc_reqd;

   LOG(_T("Exiting ComputeRequiredConcreteStrength"));
   return result;
}

bool pgsStrandDesignTool::Bump500(const StressCheckTask& task,pgsTypes::StressLocation stressLocation)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(m_SegmentKey);

   LOG(_T("Bump 500psi"));
   Float64 five_ksi = WBFL::Units::ConvertToSysUnits(0.5,WBFL::Units::Measure::KSI);
   Float64 fc = GetConcreteStrength();
   Float64 fci = GetReleaseStrength();
   LOG(_T("current f'c  = ") << WBFL::Units::ConvertFromSysUnits(fc,WBFL::Units::Measure::KSI) << _T(" KSI") );
   LOG(_T("current f'ci = ") << WBFL::Units::ConvertFromSysUnits(fci,WBFL::Units::Measure::KSI) << _T(" KSI") );
   fc  += five_ksi;
   LOG(_T("target f'c  = ") << WBFL::Units::ConvertFromSysUnits(fc,WBFL::Units::Measure::KSI) << _T(" KSI") );

   Float64 fc_max = GetMaximumConcreteStrength();
   if (fc_max < fc)
   {
      LOG(_T("Final Strength Exceeds Maximum of ")<<WBFL::Units::ConvertFromSysUnits(fc_max,WBFL::Units::Measure::KSI) << _T(" KSI - Bump 500 failed") );
      return false;
   }

   if (!UpdateConcreteStrength(fc,task,stressLocation))
   {
      LOG(_T("Failed increasing concrete strength"));
      return false;
   }

   if (task.intervalIdx == releaseIntervalIdx || task.intervalIdx == liftSegmentIntervalIdx)
   {
      fci += five_ksi;
      LOG(_T("target f'ci = ") << WBFL::Units::ConvertFromSysUnits(fci,WBFL::Units::Measure::KSI) << _T(" KSI") );

      Float64 fci_max = GetMaximumReleaseStrength();
      if (fci_max < fci)
      {
         LOG(_T("Release Strength Exceeds Maximum of ")<<WBFL::Units::ConvertFromSysUnits(fci_max,WBFL::Units::Measure::KSI) << _T(" KSI - Bump 500 failed") );
         return false;
      }
      else if (!UpdateReleaseStrength(fci,m_ReleaseStrengthResult,task,stressLocation))
      {
         LOG(_T("Failed increasing concrete release strength"));
         return false;
      }
   }

   LOG(_T("** Bump 500psi Succeeded"));
   return true;
}

bool pgsStrandDesignTool::IsDesignSlabOffset() const 
{
   return m_bIsDesignSlabOffset;
}

void pgsStrandDesignTool::SetSlabOffset(pgsTypes::MemberEndType end,Float64 offset)
{
   ATLASSERT(m_MinSlabOffset <= offset);
   ATLASSERT(IsDesignSlabOffset());

   LOG(_T("** Set slab offset to ") <<WBFL::Units::ConvertFromSysUnits(offset,WBFL::Units::Measure::Inch) << (end == pgsTypes::metStart ? _T(" at Start of Girder") : _T(" at End of Girder")));
   m_pArtifact->SetSlabOffset(end,offset);
   m_bConfigDirty = true; // cache is dirty
}

Float64 pgsStrandDesignTool::GetSlabOffset(pgsTypes::MemberEndType end) const
{
   ATLASSERT(IsDesignSlabOffset());
   return m_pArtifact->GetSlabOffset(end);
}

void pgsStrandDesignTool::SetMinimumSlabOffset(Float64 offset)
{
   ATLASSERT(IsDesignSlabOffset());
   ATLASSERT(0.0 <= offset);
   m_MinSlabOffset = offset;
}

Float64 pgsStrandDesignTool::GetMinimumSlabOffset() const
{
   ATLASSERT(IsDesignSlabOffset());
   return max(m_MinSlabOffset,m_AbsoluteMinimumSlabOffset);
}

Float64 pgsStrandDesignTool::GetAbsoluteMinimumSlabOffset() const
{
   ATLASSERT(IsDesignSlabOffset());
   ATLASSERT(m_AbsoluteMinimumSlabOffset>0.0);
   return m_AbsoluteMinimumSlabOffset;
}

bool pgsStrandDesignTool::IsDesignExcessCamber() const
{
   return m_bIsDesignExcessCamber;
}

Float64 pgsStrandDesignTool::GetAssumedExcessCamberTolerance() const
{
   return m_AssumedExcessCamberTolerance;
}

void pgsStrandDesignTool::SetAssumedExcessCamber(Float64 camber)
{
   LOG(_T("** Set assumed excess camber to ") <<WBFL::Units::ConvertFromSysUnits(camber,WBFL::Units::Measure::Inch));
   m_pArtifact->SetAssumedExcessCamber(camber);
   m_bConfigDirty = true; // cache is dirty
}

Float64 pgsStrandDesignTool::GetAssumedExcessCamber() const
{
   return m_pArtifact->GetAssumedExcessCamber();
}

void pgsStrandDesignTool::SetLiftingLocations(Float64 left,Float64 right)
{
   LOG(_T("** Lifting locations set to left = ")<<WBFL::Units::ConvertFromSysUnits(left,WBFL::Units::Measure::Feet)<<_T(", right = ")<<WBFL::Units::ConvertFromSysUnits(right,WBFL::Units::Measure::Feet)<< _T(" ft") );
   m_pArtifact->SetLiftingLocations(left, right);
   m_bConfigDirty = true; // cache is dirty
}

Float64 pgsStrandDesignTool::GetLeftLiftingLocation() const
{
   return m_pArtifact->GetLeftLiftingLocation();
}

Float64 pgsStrandDesignTool::GetRightLiftingLocation() const
{
   return m_pArtifact->GetRightLiftingLocation();
}

void pgsStrandDesignTool::SetTruckSupportLocations(Float64 left,Float64 right)
{
   LOG(_T("** Hauling locations set to left = ")<<WBFL::Units::ConvertFromSysUnits(left,WBFL::Units::Measure::Feet)<<_T(", right = ")<<WBFL::Units::ConvertFromSysUnits(right,WBFL::Units::Measure::Feet)<< _T(" ft") );
   m_pArtifact->SetTruckSupportLocations(left, right);
   m_bConfigDirty = true; // cache is dirty
}

Float64 pgsStrandDesignTool::GetLeadingOverhang() const
{
   return m_pArtifact->GetLeadingOverhang();
}

Float64 pgsStrandDesignTool::GetTrailingOverhang() const
{
   return m_pArtifact->GetTrailingOverhang();
}

void pgsStrandDesignTool::SetHaulTruck(LPCTSTR lpszHaulTruck)
{
   LOG(_T("** Haul Truck set to ") << lpszHaulTruck);
   m_pArtifact->SetHaulTruck(lpszHaulTruck);
   m_bConfigDirty = true; // cache is dirty
}

LPCTSTR pgsStrandDesignTool::GetHaulTruck() const
{
   return m_pArtifact->GetHaulTruck();
}

Float64 pgsStrandDesignTool::GetConcreteStrength() const
{
   return m_pArtifact->GetConcreteStrength();
}

Float64 pgsStrandDesignTool::GetReleaseStrength() const
{
   return m_pArtifact->GetReleaseStrength();
}

Float64 pgsStrandDesignTool::GetReleaseStrength(ConcStrengthResultType* pStrengthResult) const
{
   *pStrengthResult = m_ReleaseStrengthResult;
   return m_pArtifact->GetReleaseStrength();
}

bool pgsStrandDesignTool::DoesReleaseRequireAdditionalRebar() const
{
   return m_ReleaseStrengthResult == ConcSuccessWithRebar ? true : false;
}

Float64 pgsStrandDesignTool::GetPjackStraightStrands() const
{
   return m_pArtifact->GetPjackStraightStrands();
}

Float64 pgsStrandDesignTool::GetPjackTempStrands() const
{
   return m_pArtifact->GetPjackTempStrands();
}

Float64 pgsStrandDesignTool::GetPjackHarpedStrands() const
{
   return m_pArtifact->GetPjackHarpedStrands();
}

void pgsStrandDesignTool::SetOutcome(pgsSegmentDesignArtifact::Outcome outcome)
{
   m_pArtifact->SetOutcome(outcome);
}

Float64 pgsStrandDesignTool::GetHarpStrandOffsetEnd(pgsTypes::MemberEndType endType) const
{
   return m_pArtifact->GetHarpStrandOffsetEnd(endType);
}

Float64 pgsStrandDesignTool::GetHarpStrandOffsetHp(pgsTypes::MemberEndType endType) const
{
   return m_pArtifact->GetHarpStrandOffsetHp(endType);
}

void pgsStrandDesignTool::SetHarpStrandOffsetEnd(pgsTypes::MemberEndType endType,Float64 off)
{
   // set offset, but make sure it stays within bounds
   LOG(_T("Attempt to offset harped strands at ends to   = ") << WBFL::Units::ConvertFromSysUnits(off, WBFL::Units::Measure::Inch) << _T(" in"));
   m_pArtifact->SetHarpStrandOffsetEnd(endType,off);
   m_bConfigDirty = true; // cache is dirty

   bool st = KeepHarpedStrandsInBounds();
   ATLASSERT(st);

   LOG(_T("** Strands Actually offset to  = ") << WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetHarpStrandOffsetEnd(endType), WBFL::Units::Measure::Inch) << _T(" in"));
}

void pgsStrandDesignTool::SetHarpStrandOffsetHp(pgsTypes::MemberEndType endType,Float64 off)
{
   LOG(_T("Attempt to offset harped strands at HPs to   = ") << WBFL::Units::ConvertFromSysUnits(off, WBFL::Units::Measure::Inch) << _T(" in"));
   m_pArtifact->SetHarpStrandOffsetHp(endType,off);
   m_bConfigDirty = true; // cache is dirty

   bool st = KeepHarpedStrandsInBounds();
   ATLASSERT(st);

   LOG(_T("** Strands Actually offset to  = ") << WBFL::Units::ConvertFromSysUnits(m_pArtifact->GetHarpStrandOffsetHp(endType), WBFL::Units::Measure::Inch) << _T(" in"));
}

bool pgsStrandDesignTool::KeepHarpedStrandsInBounds()
{
   if ( !IsDesignHarping() )
   {
      return true;
   }

   LOG(_T("Make sure harped strand patterns stay in bounds"));

   StrandIndexType nh = m_pArtifact->GetNumHarpedStrands();
   if (0 < nh)
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);

      Float64 end_offset_inc = this->GetHarpedEndOffsetIncrement(pStrandGeom);

      ConfigStrandFillVector fillvec = pStrandGeom->ComputeStrandFill(m_SegmentKey, pgsTypes::Harped, nh);

      Float64 end_lower_bound, end_upper_bound;
      // cant adjust if we're not allowed.
      if (0.0 <= end_offset_inc)
      {
         for ( int i = 0; i < 2; i++ )
         {
            pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
            pStrandGeom->GetHarpedEndOffsetBoundsEx(m_GirderEntryName.c_str(), endType, pgsTypes::asHarped, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd, fillvec, &end_lower_bound, &end_upper_bound);

            Float64 end_offset = m_pArtifact->GetHarpStrandOffsetEnd(endType);
            if (end_upper_bound < end_offset)
            {
               m_pArtifact->SetHarpStrandOffsetEnd(endType,end_upper_bound);
            }
            else if (end_offset < end_lower_bound)
            {
               m_pArtifact->SetHarpStrandOffsetEnd(endType,end_lower_bound);
            }
         }

         m_bConfigDirty = true; // cache is dirty
      }

      Float64 hp_offset_inc = this->GetHarpedHpOffsetIncrement(pStrandGeom);

      // cant adjust if we're not allowed.
      Float64 hp_lower_bound, hp_upper_bound;
      if (0.0 < hp_offset_inc)
      {
         for ( int i = 0; i < 2; i++ )
         {
            pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
            pStrandGeom->GetHarpedHpOffsetBoundsEx(m_GirderEntryName.c_str(), endType, pgsTypes::asHarped, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd, fillvec, &hp_lower_bound, &hp_upper_bound);

            Float64 hp_offset = m_pArtifact->GetHarpStrandOffsetHp(endType);
            if (hp_upper_bound < hp_offset)
            {
               m_pArtifact->SetHarpStrandOffsetHp(endType,hp_upper_bound);
            }
            else if (hp_offset < hp_lower_bound)
            {
               m_pArtifact->SetHarpStrandOffsetHp(endType,hp_lower_bound);
            }
         }

         m_bConfigDirty = true; // cache is dirty
      }

      if (m_DoDesignForStrandSlope)
      {
         if ( !AdjustForStrandSlope() )
         {
            return false;
         }
      }

      if (m_DoDesignForHoldDownForce)
      {
         if ( !AdjustForHoldDownForce() )
         {
            return false;
         }
      }
   }

   return true;
}

//void pgsStrandDesignTool::GetEndOffsetBounds(Float64* pLower, Float64* pUpper) const
//{
//   GET_IFACE(IStrandGeometry,pStrandGeom);
//   StrandIndexType nh = m_pArtifact->GetNumHarpedStrands();
//   if (0 < nh)
//   {
//      Float64 end_offset_inc = GetHarpedEndOffsetIncrement(pStrandGeom);
//      if (0.0 < end_offset_inc)
//      {
//         const GDRCONFIG& config = GetSegmentConfiguration();
//         const ConfigStrandFillVector& fillvec = config.PrestressConfig.GetStrandFill(pgsTypes::Harped);
//
//         pStrandGeom->GetHarpedEndOffsetBoundsEx(m_GirderEntryName.c_str(), pgsTypes::asHarped, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd, fillvec, pLower, pUpper);
//      }
//      else
//      {
//         // adjustment not allowed
//         *pLower=0;
//         *pUpper=0;
//      }
//   }
//   else
//   {
//      // should be smart enough not to call this with no hs
//      ATLASSERT(false);
//      *pLower=0;
//      *pUpper=0;
//   }
//}
//
//void pgsStrandDesignTool::GetHpOffsetBounds(Float64* pLower, Float64* pUpper) const
//{
//   GET_IFACE(IStrandGeometry,pStrandGeom);
//   StrandIndexType nh = m_pArtifact->GetNumHarpedStrands();
//   if (0 < nh)
//   {
//      Float64 offset_inc = this->GetHarpedHpOffsetIncrement(pStrandGeom);
//      if (0.0 < offset_inc)
//      {
//         const GDRCONFIG& config = GetSegmentConfiguration();
//         const ConfigStrandFillVector& fillvec = config.PrestressConfig.GetStrandFill(pgsTypes::Harped);
//
//         pStrandGeom->GetHarpedHpOffsetBoundsEx(m_GirderEntryName.c_str(), pgsTypes::asHarped, m_HgStart, m_HgHp1, m_HgHp2, m_HgEnd, fillvec, pLower, pUpper);
//      }
//      else
//      {
//         // adjustment not allowed
//         *pLower=0;
//         *pUpper=0;
//      }
//   }
//   else
//   {
//      // should be smart enough not to call this with no hs
//      ATLASSERT(false);
//      *pLower=0;
//      *pUpper=0;
//   }
//}

Float64 pgsStrandDesignTool::GetHarpedHpOffsetIncrement(IStrandGeometry* pStrandGeom) const
{
   Float64 offset_inc = pStrandGeom->GetHarpedHpOffsetIncrement(m_GirderEntryName.c_str(), pgsTypes::asHarped);
   return offset_inc;
}

Float64 pgsStrandDesignTool::GetHarpedEndOffsetIncrement(IStrandGeometry* pStrandGeom) const
{
   Float64 offset_inc = pStrandGeom->GetHarpedEndOffsetIncrement(m_GirderEntryName.c_str(), pgsTypes::asHarped);
   return offset_inc;
}

Float64 pgsStrandDesignTool::GetPrestressForceAtLifting(const GDRCONFIG &guess,const pgsPointOfInterest& poi) const
{
   Float64 distFromStart = poi.GetDistFromStart();
   ATLASSERT( !IsZero(distFromStart) && !IsEqual(distFromStart,m_SegmentLength));

   LOG(_T("Compute total prestessing force at Release in end-zone for the current configuration at ") << WBFL::Units::ConvertFromSysUnits(distFromStart, WBFL::Units::Measure::Feet) << _T(" ft along girder"));

   Float64 xFerFactor;
   Float64 xferLength = GetTransferLength(pgsTypes::Permanent);
   if (distFromStart < xferLength)
   {
      xFerFactor = distFromStart/xferLength;
   }
   else if (m_SegmentLength-distFromStart < xferLength)
   {
      xFerFactor = (m_SegmentLength-distFromStart)/xferLength;
   }
   else
   {
      xFerFactor = 1.0;
   }
   LOG(_T("Transfer Length Factor = ")<<xFerFactor);

   // Compute the maximum allowable jacking force for the trial number of strands
   StrandIndexType ns = guess.PrestressConfig.GetStrandCount(pgsTypes::Straight);
   StrandIndexType nh = guess.PrestressConfig.GetStrandCount(pgsTypes::Harped);
   StrandIndexType nt = guess.PrestressConfig.GetStrandCount(pgsTypes::Temporary);
   StrandIndexType np = ns + nh + nt;

   if ( np == 0)
   {
      ATLASSERT(false); // this really shouldn't happen in a design algoritm
      return 0.0;
   }

   Float64 pj  =  guess.PrestressConfig.Pjack[pgsTypes::Straight] + 
                  guess.PrestressConfig.Pjack[pgsTypes::Harped]   + 
                  guess.PrestressConfig.Pjack[pgsTypes::Temporary];

   Float64 Aps = m_aps[pgsTypes::Straight]*ns + m_aps[pgsTypes::Harped]*nh + m_aps[pgsTypes::Temporary]*nt;
   Float64 fpj =  pj/Aps;

   LOG(_T("Average jacking stress for this strand configuration = ") << WBFL::Units::ConvertFromSysUnits(fpj,WBFL::Units::Measure::KSI) << _T(" KSI"));

   // Estimate prestress loss
   pgsPsForceEng psfeng;
   psfeng.SetStatusGroupID(m_StatusGroupID);
   psfeng.SetBroker(m_pBroker);
   ATLASSERT(poi.GetSegmentKey() == m_SegmentKey);
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftingIntervalIdx = pIntervals->GetLiftSegmentInterval(m_SegmentKey);
   Float64 loss = psfeng.GetEffectivePrestressLoss(poi,pgsTypes::Permanent,liftingIntervalIdx,pgsTypes::End, true/*apply elastic gain reduction*/, &guess);

   LOG(_T("Estimated losses at lifting for this strand configuration = ")
      << WBFL::Units::ConvertFromSysUnits(loss,WBFL::Units::Measure::KSI) << _T(" KSI"));

   // Average stress after losses
   Float64 fstrand = fpj - loss;
   LOG(_T("Average strand stress at lifting = ") << WBFL::Units::ConvertFromSysUnits(fstrand,WBFL::Units::Measure::KSI) << _T(" KSI"));

   Float64 force = fstrand * xFerFactor * (m_aps[pgsTypes::Straight]*ns + m_aps[pgsTypes::Harped]*nh + m_aps[pgsTypes::Temporary]*nt);

   LOG(_T("Total force at lifting = (") << WBFL::Units::ConvertFromSysUnits(fstrand,WBFL::Units::Measure::KSI) << _T(" ksi)(") << xFerFactor << _T(")")
      << _T("[") << WBFL::Units::ConvertFromSysUnits(m_aps[pgsTypes::Straight],WBFL::Units::Measure::Inch2) << _T(" in^2)(") << ns << _T(") + (") 
      << WBFL::Units::ConvertFromSysUnits(m_aps[pgsTypes::Harped],WBFL::Units::Measure::Inch2) << _T(" in^2)(") << nh << _T(") + ") 
      << WBFL::Units::ConvertFromSysUnits(m_aps[pgsTypes::Temporary],WBFL::Units::Measure::Inch2) << _T("in^2)(") << nt << _T(")] = ") << WBFL::Units::ConvertFromSysUnits(force,WBFL::Units::Measure::Kip) << _T(" kip"));

   return force;
}

Float64 pgsStrandDesignTool::GetPrestressForceMidZone(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const
{
   LOG(_T("Compute total prestessing force in mid-zone for the current configuration"));

   ATLASSERT(poi.GetSegmentKey() == m_SegmentKey);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetFirstCastDeckInterval();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   // NOTE: can't use temp strand removal interval because it is based on the original input which may not have temp strands
   //IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(poi.GetSegmentKey());

   UpdateJackingForces();
   GDRCONFIG guess = GetSegmentConfiguration();

   // Compute the maximum allowable jacking force for the trial number of strands
   StrandIndexType ns = GetNs();
   StrandIndexType nh = GetNh();
   StrandIndexType nt = intervalIdx < castDeckIntervalIdx ? GetNt() : 0;
   StrandIndexType np = ns + nh + nt;

   if ( np == 0)
   {
      ATLASSERT(false); // this really shouldn't happen in a design algoritm
      return 0.0;
   }

   Float64 pj  =  guess.PrestressConfig.Pjack[pgsTypes::Straight] + guess.PrestressConfig.Pjack[pgsTypes::Harped];

   if (intervalIdx < castDeckIntervalIdx)
   {
      pj += guess.PrestressConfig.Pjack[pgsTypes::Temporary];
   }

   Float64 Aps = m_aps[pgsTypes::Straight]*ns + m_aps[pgsTypes::Harped]*nh + m_aps[pgsTypes::Temporary]*nt;
   Float64 fpj =  pj/Aps;

   LOG(_T("Average jacking stress for this strand configuration = ") << WBFL::Units::ConvertFromSysUnits(fpj,WBFL::Units::Measure::KSI) << _T(" KSI"));

   // Estimate prestress loss
   pgsPsForceEng psfeng;
   psfeng.SetStatusGroupID(m_StatusGroupID);
   psfeng.SetBroker(m_pBroker);
   Float64 loss;
   if ( intervalIdx < liveLoadIntervalIdx )
   {
      loss = psfeng.GetEffectivePrestressLoss(poi,pgsTypes::Permanent,intervalIdx,pgsTypes::End, true/*apply elastic gain reduction*/, &guess);
   }
   else
   {
      loss = psfeng.GetEffectivePrestressLossWithLiveLoad(poi,pgsTypes::Permanent,pgsTypes::ServiceIII,INVALID_INDEX/*controlling live load*/, true/*include elastic effects*/, true/*apply elastic gain reduction*/, &guess);
   }

   if (intervalIdx == releaseIntervalIdx)
   {
      LOG(_T("Estimated Release losses for this strand configuration = ") << WBFL::Units::ConvertFromSysUnits(loss,WBFL::Units::Measure::KSI) << _T(" KSI"));
   }
   else if ( intervalIdx < liveLoadIntervalIdx )
   {
      LOG(_T("Estimated Final losses for this strand configuration = ") << WBFL::Units::ConvertFromSysUnits(loss,WBFL::Units::Measure::KSI) << _T(" KSI"));
   }
   else
   {
      LOG(_T("Estimated Final losses for this strand configuration = ") << WBFL::Units::ConvertFromSysUnits(loss,WBFL::Units::Measure::KSI) << _T(" KSI"));
   }

   // Average stress after losses
   Float64 fstrand = fpj - loss;
   LOG(_T("average strand stress after losses = ") << WBFL::Units::ConvertFromSysUnits(fstrand,WBFL::Units::Measure::KSI) << _T(" KSI"));

   Float64 force = fstrand*Aps;
   LOG(_T("Total force at final = ") << WBFL::Units::ConvertFromSysUnits(force,WBFL::Units::Measure::Kip) << _T(" kip"));

   return force;
}


Float64 pgsStrandDesignTool::ComputeEndOffsetForEccentricity(const pgsPointOfInterest& poi, Float64 ecc) const
{
   ATLASSERT(poi.GetSegmentKey() == m_SegmentKey);

   StrandIndexType ns = GetNs();
   StrandIndexType nh = GetNh();
   StrandIndexType nt = GetNt();

   GDRCONFIG guess = GetSegmentConfiguration();

   GET_IFACE(IStrandGeometry,pStrandGeom);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);

   Float64 ecc_ss = pStrandGeom->GetEccentricity(releaseIntervalIdx, poi, pgsTypes::Straight, &guess).Y();
   Float64 ecc_ts = pStrandGeom->GetEccentricity(releaseIntervalIdx, poi, pgsTypes::Temporary, &guess).Y();

   // compute hs eccentricities for end offsets of 1.0 and -1.0, and extrapolate the required offset
   guess.PrestressConfig.EndOffset[pgsTypes::metStart] = 1.0;
   guess.PrestressConfig.EndOffset[pgsTypes::metEnd]   = 1.0;
   Float64 ecc_hs_p1 = pStrandGeom->GetEccentricity(releaseIntervalIdx, poi, pgsTypes::Harped, &guess).Y();

   guess.PrestressConfig.EndOffset[pgsTypes::metStart] = -1.0;
   guess.PrestressConfig.EndOffset[pgsTypes::metEnd]   = -1.0;
   Float64 ecc_hs_m1 = pStrandGeom->GetEccentricity(releaseIntervalIdx, poi, pgsTypes::Harped, &guess).Y();

   Float64 As = pStrandGeom->GetStrandArea(poi, releaseIntervalIdx, pgsTypes::Straight, &guess);
   Float64 Ah = pStrandGeom->GetStrandArea(poi, releaseIntervalIdx, pgsTypes::Harped, &guess);
   Float64 At = pStrandGeom->GetStrandArea(poi, releaseIntervalIdx, pgsTypes::Temporary, &guess);
   Float64 Aps = As + Ah + At;
   ATLASSERT(0.0 < Aps);

   Float64 ecc_p1 = (As*ecc_ss + At*ecc_ts + Ah*ecc_hs_p1)/Aps;
   Float64 ecc_m1 = (As*ecc_ss + At*ecc_ts + Ah*ecc_hs_m1)/Aps;

   WBFL::Math::CoordMapper1d mapper(-1.0, ecc_m1, 1.0, ecc_p1);

   Float64 off = mapper.GetA(ecc);

   guess.PrestressConfig.EndOffset[pgsTypes::metStart] = off;
   guess.PrestressConfig.EndOffset[pgsTypes::metEnd] = off;

   Float64 new_ecc = pStrandGeom->GetEccentricity(releaseIntervalIdx, poi, true, &guess).Y();
   ATLASSERT(IsEqual(ecc,new_ecc,0.01));

   return off;
}

Float64 pgsStrandDesignTool::ComputeHpOffsetForEccentricity(const pgsPointOfInterest& poi, Float64 ecc,IntervalIndexType intervalIdx) const
{
   ATLASSERT(poi.GetSegmentKey() == m_SegmentKey);

   if (ecc == Float64_Max)
   {
      // Eating our own dog food here. If above is true, then we are dealing with ecc as an uninitialized value of 
      // m_MinimumFinalMzEccentricity(Float64_Max), set above in our constructor. This means that we are having trouble
      // determining a minimum number of strands to satisfy bottom mid-span tensile stresses, and the main algorithm
      // is grasping for straws. At any rate, lowering the strand bundle at the HP is not an option.
      return -Float64_Max;
   }
   else
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);
      IntervalIndexType castDeckIntervalIdx = pIntervals->GetFirstCastDeckInterval();
      ATLASSERT(castDeckIntervalIdx != INVALID_INDEX);
      // NOTE: Can't use the following code block. If the original input (before design) does not have
      // temporary strands then the install and remove intervals will be INVALID_INDEX. If the interval
      // in question is before the deck is cast, then include temporary strands. Otherwise, the deck
      // has been cast and the temporary strands have been removed
      //IntervalIndexType tsInstallIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(m_SegmentKey);
      //IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(m_SegmentKey);
      //bool bIncTempStrands = (tsInstallIntervalIdx <= intervalIdx && intervalIdx < tsRemovalIntervalIdx) ? true : false;
      bool bIncTempStrands = (intervalIdx < castDeckIntervalIdx ? true : false);
      // NOTE: When temporary strands are post-tensioned they are installed after release. If we are including temporary strands in the computation,
      // and we don't know when they are installed (see note above), we have to estimate the interval. The earliest PT-TTS can be installed is immediately after
      // release. For this reason, add one to release interval
      IntervalIndexType eccIntervalIdx = releaseIntervalIdx + (bIncTempStrands ? 1 : 0);

      GDRCONFIG guess = GetSegmentConfiguration();

      GET_IFACE(IStrandGeometry,pStrandGeom);

      Float64 ecc_ss = pStrandGeom->GetEccentricity(eccIntervalIdx, poi, pgsTypes::Straight, &guess).Y();
      Float64 ecc_ts = bIncTempStrands ? pStrandGeom->GetEccentricity(eccIntervalIdx, poi, pgsTypes::Temporary, &guess).Y() : 0.0;

      // compute hs eccentricities for hp offsets of +1.0 and -1.0, and extrapolate the required offset
      guess.PrestressConfig.HpOffset[pgsTypes::metStart] = 1.0;
      guess.PrestressConfig.HpOffset[pgsTypes::metEnd] = 1.0;
      Float64 ecc_hs_p1 = pStrandGeom->GetEccentricity(eccIntervalIdx, poi, pgsTypes::Harped, &guess).Y();
      guess.PrestressConfig.HpOffset[pgsTypes::metStart] = -1.0;
      guess.PrestressConfig.HpOffset[pgsTypes::metEnd] = -1.0;
      Float64 ecc_hs_m1 = pStrandGeom->GetEccentricity(eccIntervalIdx, poi, pgsTypes::Harped, &guess).Y();

      Float64 As = pStrandGeom->GetStrandArea(poi, releaseIntervalIdx, pgsTypes::Straight, &guess);
      Float64 Ah = pStrandGeom->GetStrandArea(poi, releaseIntervalIdx, pgsTypes::Harped, &guess);
      Float64 At = (bIncTempStrands ? pStrandGeom->GetStrandArea(poi, releaseIntervalIdx, pgsTypes::Temporary, &guess) : 0.0);
      Float64 Aps = As + Ah + At;
      ATLASSERT(0.0 < Aps);

      Float64 ecc_p1 = (As*ecc_ss + At*ecc_ts + Ah*ecc_hs_p1) / Aps;
      Float64 ecc_m1 = (As*ecc_ss + At*ecc_ts + Ah*ecc_hs_m1) / Aps;

      WBFL::Math::CoordMapper1d mapper(-1.0, ecc_m1, 1.0, ecc_p1);

      Float64 off = mapper.GetA(ecc);

      guess.PrestressConfig.HpOffset[pgsTypes::metStart] = off;
      guess.PrestressConfig.HpOffset[pgsTypes::metEnd] = off;
      Float64 new_ecc = pStrandGeom->GetEccentricity(eccIntervalIdx, poi, bIncTempStrands, &guess).Y();
      ATLASSERT(IsEqual(ecc,new_ecc,0.01));

      return off;
   }
}

bool pgsStrandDesignTool::ComputeMinHarpedForEndZoneEccentricity(const pgsPointOfInterest& poi, Float64 eccTarget, IntervalIndexType intervalIdx, StrandIndexType* pNs, StrandIndexType* pNh) const
{
   // don't do anything if we aren't in minimize harped mode
   if (m_DesignOptions.doStrandFillType != ftMinimizeHarping)
   {
      LOG(_T("We do not minimize harped strands unless m_StrandFillType==ftMinimizeHarping"));
      return false;
   }

   ATLASSERT(poi.GetSegmentKey() == m_SegmentKey);

   LOG(_T("Attempting to swap harped for straight to acheive an ecc = ")<< WBFL::Units::ConvertFromSysUnits(eccTarget, WBFL::Units::Measure::Inch) << _T(" in"));
   LOG(_T("At ")<< WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart(), WBFL::Units::Measure::Feet) << _T(" feet from left end of girder"));

   GET_IFACE(IStrandGeometry,pStrandGeom);

   // get the current eccentricity and make sure our target is lower (bigger)
   Float64 curr_ecc = ComputeEccentricity(poi,intervalIdx);
   LOG(_T("Current ecc = ")<< WBFL::Units::ConvertFromSysUnits(curr_ecc, WBFL::Units::Measure::Inch) << _T(" in"));
   if (eccTarget < curr_ecc)
   {
      // If code hits here, we have a bug upstream computing lifing concrete strength - need to revisit
      ATLASSERT(false); // this should never happen
      LOG(_T("Warning - current eccentricity is already larger than requested"));
      return false;
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);
   IntervalIndexType nonCompoisteIntervalIdx = pIntervals->GetLastNoncompositeInterval();
   ATLASSERT(nonCompoisteIntervalIdx != INVALID_INDEX);
   // NOTE: Can't use the following code block. If the original input (before design) does not have
   // temporary strands then the install and remove intervals will be INVALID_INDEX. If the interval
   // in question is before the deck is cast, then include temporary strands. Otherwise, the deck
   // has been cast and the temporary strands have been removed
   //IntervalIndexType tsInstallIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(m_SegmentKey);
   //IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(m_SegmentKey);
   //bool bIncTempStrands = (tsInstallIntervalIdx <= intervalIdx && intervalIdx < tsRemovalIntervalIdx) ? true : false;
   bool bIncTempStrands = (intervalIdx < nonCompoisteIntervalIdx ? true : false);
   // NOTE: When temporary strands are post-tensioned they are installed after release. If we are including temporary strands in the computation,
   // and we don't know when they are installed (see note above), we have to estimate the interval. The earliest PT-TTS can be installed is immediately after
   // release. For this reason, add one to release interval
   IntervalIndexType eccIntervalIdx = releaseIntervalIdx + (bIncTempStrands ? 1 : 0);

   GDRCONFIG guess = GetSegmentConfiguration();

   StrandIndexType ns_orig = GetNs();
   StrandIndexType nh_orig = GetNh();
   StrandIndexType ns_prev = ns_orig;
   StrandIndexType nh_prev = nh_orig;
   StrandIndexType Np = ns_prev+nh_prev;  // need to maintain this or be one bigger
   LOG(_T("Current Ns = ") << ns_prev << _T(" Nh = ") << nh_prev << _T(" Np = ") << Np);

   pgsPointOfInterest ms_poi(m_SegmentKey,m_SegmentLength/2.0);

   // smallest number of harped strands we can have
   StrandIndexType min_nh = pStrandGeom->GetNextNumStrands(m_SegmentKey,pgsTypes::Harped, 0);

   if (nh_orig <= min_nh)
   {
      LOG(_T("No harped strands to lose - try another method"));
      return false;
   }

   // loop until our eccentricity gets larger than the target
   StrandIndexType Ns, Nh;
   Float64 eccDiffPrev = eccTarget - curr_ecc; // difference between the target eccentricity and the eccentricy
                                               // for the previous guess
   while(curr_ecc < eccTarget)
   {
      // try to swap harped for straight
      Ns = pStrandGeom->GetNextNumStrands(m_SegmentKey, pgsTypes::Straight, ns_prev);
      if (Ns == INVALID_INDEX)
      {
         //no more straight strands left to fill
         break;
      }

      Nh = Np - Ns;
      Nh = pStrandGeom->GetPrevNumStrands(m_SegmentKey,pgsTypes::Harped, Nh+1);
      if (Nh == INVALID_INDEX)
      {
         //no more harped strands left
         break;
      }

      // we must end up with at least Np strands
      StrandIndexType remain = Np - (Ns+Nh);
      if (0 < remain)
      {
         ATLASSERT(remain==1);
         Ns = pStrandGeom->GetNextNumStrands(m_SegmentKey,pgsTypes::Straight, Ns);
         if (Ns == INVALID_INDEX)
         {
            // last ditch is to see if there is a harped increment we missed
            StrandIndexType nhd = pStrandGeom->GetNextNumStrands(m_SegmentKey,pgsTypes::Harped, Nh);
            if (nhd != nh_prev)
            {
               Nh = nhd;
            }
            else
            {
               break;
            }
         }
      }

      // finally can compute our new eccentricity
      ConfigStrandFillVector sfillvec = pStrandGeom->ComputeStrandFill(m_SegmentKey, pgsTypes::Straight, Ns);
      ConfigStrandFillVector hfillvec = pStrandGeom->ComputeStrandFill(m_SegmentKey, pgsTypes::Harped, Nh);

      guess.PrestressConfig.SetStrandFill(pgsTypes::Straight, sfillvec);
      guess.PrestressConfig.SetStrandFill(pgsTypes::Harped,   hfillvec);

      Float64 new_ecc = pStrandGeom->GetEccentricity(eccIntervalIdx, poi, bIncTempStrands, &guess).Y();

      LOG(_T("Try ns = ")<< Ns <<_T(" nh = ")<< Nh <<_T(" np = ")<< Nh+Ns);
      LOG(_T("New ecc = ")<< WBFL::Units::ConvertFromSysUnits(new_ecc, WBFL::Units::Measure::Inch) << _T(" in, Target ecc = ")<< WBFL::Units::ConvertFromSysUnits(eccTarget, WBFL::Units::Measure::Inch) << _T(" in"));

      // our new ecc had better be more than our previous, or this strategy isn't working
      if (new_ecc < curr_ecc)
      {
         LOG(_T("Swapping Harped strands for straight does not increase eccentricity - quit this waste of time!"));
         return false;
      }
      
      // we have to guard against causing mid-zone Bottom Service tension to go out of bounds. Odd case, but it happens for WF42G
      Float64 ms_ecc = pStrandGeom->GetEccentricity(eccIntervalIdx, ms_poi, false, &guess).Y();
      LOG(_T("New Eccentricity in mid-zone, without temp strands, is ") <<WBFL::Units::ConvertFromSysUnits( ms_ecc , WBFL::Units::Measure::Inch)<< _T(" in"));
      LOG(_T("Minimum ecc for release tension mz = ") <<WBFL::Units::ConvertFromSysUnits( GetMinimumFinalMidZoneEccentricity() , WBFL::Units::Measure::Inch)<< _T(" in"));

      if (ms_ecc < GetMinimumFinalMidZoneEccentricity())
      {
         LOG(_T("Swapping harped for straight voilates minimum mid-zone eccentricity for bottom service tension. Abort this strategy"));
         return false;
      }

      if ( eccTarget < new_ecc )
      {
         // we've overshot the target eccentricity (basically, we've found the answer)
         // we are after the eccentricity that best matches the target. compute the
         // difference between the eccentricity using the new strand configuration (new_ecc)
         // and the target eccentricity. Compare this to the difference between the
         // previous eccentricity (curr_ecc) and the target. Use the number of strands associated
         // with the eccentricity that is closest to the target
         if ( (new_ecc - eccTarget) < (eccTarget - curr_ecc) )
         {
            LOG(_T("We overshot the target eccentricity, but we are closer than the previous guess so keep the result"));
            ns_prev = Ns;
            nh_prev = Nh;
         }
#if defined _DEBUG
         else
         {
            // the previous guess was better than this one so don't update the _T("prev") values.
            // the result will be the actual previous number of straight and harped strands
            LOG(_T("Increased Ecc by reducing harped strands - Success"));
         }
#endif

         break;
      }


      // update control variables for the next loop
      ns_prev = Ns;
      nh_prev = Nh;
      curr_ecc = new_ecc;

      if (nh_prev <= min_nh)
      {
         LOG(_T("No more harped strands to lose"));
         break;
      }
   }

   if (ns_prev != GetNs() || nh_prev != GetNh())
   {
      // we updated 
      LOG(_T("Succeeded reducing harped strands ns = ")<<ns_prev<<_T(" nh = ")<<nh_prev<<_T(" np = ")<< ns_prev+nh_prev);
      *pNs = ns_prev;
      *pNh = nh_prev;
      return true;
   }
   else
   {
      LOG(_T("Number of harped strands not reduced - try another method"));
      return false; // nothing changed
   }
}

bool pgsStrandDesignTool::ComputeAddHarpedForMidZoneReleaseEccentricity(const pgsPointOfInterest& poi, Float64 eccMax, Float64 eccMin, StrandIndexType* pNs, StrandIndexType* pNh) const
{
   ATLASSERT(poi.GetSegmentKey() == m_SegmentKey);

   LOG(_T("Attempting to swap straight for harped to raise ecc to at least = ")<< WBFL::Units::ConvertFromSysUnits(eccMax, WBFL::Units::Measure::Inch) << _T(" in"));
   LOG(_T("Attempting to swap straight for harped to raise ecc to at most  = ")<< WBFL::Units::ConvertFromSysUnits(eccMin, WBFL::Units::Measure::Inch) << _T(" in"));
   LOG(_T("At ")<< WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart(), WBFL::Units::Measure::Inch) << _T(" in from left end of girder"));

   // NOTE: If TxDOT starts designing for lifting, we need to change UI to allow non-standard fill and lifting

   // Lifting analysis swaps the other way - 
   if (m_DesignOptions.doDesignLifting)
   {
      LOG(_T("Lifting analysis Enabled - Attempting to swap straight for harped to raise ecc is not compatible with lifting goals"));
      return false;
   }
   else
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);

      GDRCONFIG guess = GetSegmentConfiguration();

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);

      // get the current eccentricity and make sure our target is higher (smaller)
      Float64 curr_ecc = pStrandGeom->GetEccentricity(releaseIntervalIdx, poi, true, &guess).Y();
      LOG(_T("Current ecc = ")<< WBFL::Units::ConvertFromSysUnits(curr_ecc, WBFL::Units::Measure::Inch) << _T(" in"));
      if (curr_ecc < eccMin)
      {
         ATLASSERT(false); // this probably should never happen
         LOG(_T("Warning - current eccentricity is already smaller than requested"));
         return false;
      }

      StrandIndexType ns_orig = GetNs();
      StrandIndexType nh_orig = GetNh();
      StrandIndexType ns_prev = ns_orig;
      StrandIndexType nh_prev = nh_orig;
      StrandIndexType nhs = ns_prev+nh_prev;  // need to maintain this or be one bigger
      StrandIndexType nt = GetNt();
      LOG(_T("Current ns = ")<<ns_prev<<_T(" nh = ")<<nh_prev<<_T(" nt = ")<<nt<<_T(" np = ")<< nhs);

      // largest number of harped strands we can have
      StrandIndexType max_nh = pStrandGeom->GetMaxStrands(m_SegmentKey,pgsTypes::Harped);
      if ( max_nh <= nh_orig)
      {
         LOG(_T("Harped pattern is full - cannot add any more"));
         return false;
      }
      else if (ns_orig <= 0)
      {
         LOG(_T("No straight strands to trade"));
         return false;
      }

      Float64 ecc_ts = 0 < nt ? pStrandGeom->GetEccentricity(releaseIntervalIdx, poi, pgsTypes::Temporary, &guess).Y() : 0.0;

      // loop until our eccentricity gets smaller than the target, then step back
      StrandIndexType ns, nh;
      bool succeeded=false;
      while(eccMin < curr_ecc)
      {
         // try to swap straight to harped
         nh = pStrandGeom->GetNextNumStrands(m_SegmentKey,pgsTypes::Harped, nh_prev);
         if (max_nh <= nh)
         {
            //no more harped locations left to fill
            LOG(_T("All harped locations are full"));
            break;
         }

         ns = nhs - nh;
         ns = pStrandGeom->GetPrevNumStrands(m_SegmentKey,pgsTypes::Straight, ns+1);
         if (ns == INVALID_INDEX)
         {
            //no more straight strands left
            LOG(_T("No straight strands left to trade"));
            break;
         }

         // we must end up with at least nhs strands
         if ((ns+nh) < nhs)
         {
            nh = pStrandGeom->GetNextNumStrands(m_SegmentKey,pgsTypes::Harped, nh);
            if (nh == INVALID_INDEX)
            {
               LOG(_T("No harped locations left"));
               break;
            }
         }

         // finally can compute our new eccentricity
         ConfigStrandFillVector sfillvec = pStrandGeom->ComputeStrandFill(m_SegmentKey, pgsTypes::Straight, ns);
         ConfigStrandFillVector hfillvec = pStrandGeom->ComputeStrandFill(m_SegmentKey, pgsTypes::Harped, nh);

         guess.PrestressConfig.SetStrandFill(pgsTypes::Straight, sfillvec);
         guess.PrestressConfig.SetStrandFill(pgsTypes::Harped,   hfillvec);
         Float64 new_ecc = pStrandGeom->GetEccentricity(releaseIntervalIdx, poi, true, &guess).Y();

         LOG(_T("Try ns = ")<<ns<<_T(" nh = ")<<nh<<_T(" np = ")<< nh+ns);
         LOG(_T("New ecc = ")<< WBFL::Units::ConvertFromSysUnits(new_ecc, WBFL::Units::Measure::Inch) << _T(" in, Target ecc = ")<< WBFL::Units::ConvertFromSysUnits(eccMax, WBFL::Units::Measure::Inch) << _T(" in"));

         // our new ecc had better be less than our previous, or this strategy isn't working
         if (curr_ecc < new_ecc)
         {
            LOG(_T("Swapping Straight strands for Harped does not decrease eccentricity - quit this waste of time!"));
            return false;
         }

         curr_ecc = new_ecc;

         if (curr_ecc < eccMax)
         {
            // We have achieved our objective. Might be able to go further, but not needed
            LOG(_T("Target ecc achieved at ns = ")<<ns<<_T(" nh = ")<<nh);
            succeeded = true;
            ns_prev = ns;
            nh_prev = nh;
            break;
         }

         if (curr_ecc < eccMin)
         {
            LOG(_T("New eccentricity is below minimum - this is as far as we can go"));
            if (ns_prev == ns_orig && nh_prev == nh_orig)
            {
                  LOG(_T("First try to increase harped strands overshot min - strategy Failed"));
            }
            else
            {
               LOG(_T("Increased Ecc by reducing harped strands. But did not reach target - Limited Success"));
            }

            break;
         }

         ns_prev = ns;
         nh_prev = nh;

         if (max_nh <= nh)
         {
            LOG(_T("No more harped strands to add"));
            break;
         }
         else if (ns == INVALID_INDEX || ns == 0)
         {
            LOG(_T("No more straight strands to trade from"));
            break;
         }
      }

      if (ns_prev != GetNs() || nh_prev != GetNh())
      {
         // we updated 
         if (succeeded)
         {
            LOG(_T("Target ecc was achieved"));
         }

         LOG(_T("Succeeded Adding harped strands ns = ")<<ns_prev<<_T(" nh = ")<<nh_prev<<_T(" np = ")<< ns_prev+nh_prev);
         *pNs = ns_prev;
         *pNh = nh_prev;
         return true;
      }
      else
      {
         LOG(_T("Number of harped strands not increased - try another method"));
         return false; // nothing changed
      }
   }
}


Float64 pgsStrandDesignTool::GetMinimumFinalMidZoneEccentricity() const
{
   return m_MinimumFinalMzEccentricity;
}

void pgsStrandDesignTool::SetMinimumFinalMidZoneEccentricity(Float64 ecc)
{
   m_MinimumFinalMzEccentricity = ecc;
}

Float64 pgsStrandDesignTool::GetMinimumReleaseStrength() const
{
   return m_MinFci;
}

Float64 pgsStrandDesignTool::GetMaximumReleaseStrength() const
{
   return m_MaxFci;
}


Float64 pgsStrandDesignTool::GetMinimumConcreteStrength() const
{
   return m_MinFc;
}

Float64 pgsStrandDesignTool::GetMaximumConcreteStrength() const
{
   return m_MaxFc;
}

arDesignStrandFillType pgsStrandDesignTool::GetOriginalStrandFillType() const
{
   return m_DesignOptions.doStrandFillType;
}

pgsSegmentDesignArtifact::ConcreteStrengthDesignState pgsStrandDesignTool::GetReleaseConcreteDesignState() const
{
   pgsSegmentDesignArtifact::ConcreteStrengthDesignState state;

   bool ismin = GetReleaseStrength() == GetMinimumReleaseStrength();

   state.SetStressState(ismin, m_SegmentKey, m_FciControl.Interval(), m_FciControl.StressType(), m_FciControl.LimitState(),m_FciControl.StressLocation());
   state.SetRequiredAdditionalRebar(m_ReleaseStrengthResult==ConcSuccessWithRebar);

   return state;
}

pgsSegmentDesignArtifact::ConcreteStrengthDesignState pgsStrandDesignTool::GetFinalConcreteDesignState() const
{
   pgsSegmentDesignArtifact::ConcreteStrengthDesignState state;

   bool ismin = GetConcreteStrength() == GetMinimumConcreteStrength();

   if (m_FcControl.ControllingAction()==pgsSegmentDesignArtifact::ConcreteStrengthDesignState::actStress)
   {
      // flexure controlled
      state.SetStressState(ismin, m_SegmentKey, m_FcControl.Interval(), m_FcControl.StressType(), m_FcControl.LimitState(),m_FcControl.StressLocation());
   }
   else
   {
      ATLASSERT(m_FcControl.ControllingAction()==pgsSegmentDesignArtifact::ConcreteStrengthDesignState::actShear);

      // shear stress controlled
      state.SetShearState(m_SegmentKey, m_FcControl.Interval(), m_FcControl.LimitState());
   }

   return state;
}

void pgsStrandDesignTool::GetMidZoneBoundaries(Float64* leftEnd, Float64* rightEnd) const
{
   // values are cached at initialization
   *leftEnd  = m_lftMz;
   *rightEnd = m_rgtMz;
}

pgsPointOfInterest pgsStrandDesignTool::GetPointOfInterest(const CSegmentKey& segmentKey, Float64 Xpoi) const
{
   return m_PoiMgr.GetPointOfInterest(segmentKey, Xpoi);
}

void pgsStrandDesignTool::GetPointsOfInterest(const CSegmentKey& segmentKey, PoiAttributeType attrib, PoiList* pPoiList) const
{
   m_PoiMgr.GetPointsOfInterest(segmentKey, attrib, POIFIND_OR, pPoiList);
}

void pgsStrandDesignTool::GetDesignPoi(IntervalIndexType intervalIdx, PoiList* pPoiList) const
{
   m_PoiMgr.GetPointsOfInterest(m_SegmentKey,pPoiList);
}

void pgsStrandDesignTool::GetDesignPoi(IntervalIndexType intervalIdx,PoiAttributeType attrib, PoiList* pPoiList) const
{
   m_PoiMgr.GetPointsOfInterest(m_SegmentKey, attrib, POIMGR_OR, pPoiList);

   if ( pgsPointOfInterest::IsReferenceAttribute(attrib) )
   {
      PoiList vPoi2;
      m_PoiMgr.GetPointsOfInterest(m_SegmentKey,POI_HARPINGPOINT,POIMGR_OR,&vPoi2);

      m_PoiMgr.MergePoiLists(*pPoiList, vPoi2,pPoiList);
   }
}

void pgsStrandDesignTool::GetDesignPoiEndZone(IntervalIndexType intervalIdx, PoiList* pPoiList) const
{
   PoiList vPoi;
   GetDesignPoi(intervalIdx, &vPoi);

   Float64 rgt_end, lft_end;
   GetMidZoneBoundaries(&lft_end, &rgt_end);

   pPoiList->reserve(vPoi.size());

   for ( const pgsPointOfInterest& poi : vPoi)
   {
      Float64 dist = poi.GetDistFromStart();

      // accept only pois in end zones
      if (dist <= lft_end || rgt_end <= dist)
      {
         pPoiList->push_back(poi);
      }
   }
}

void pgsStrandDesignTool::GetDesignPoiEndZone(IntervalIndexType intervalIdx,PoiAttributeType attrib, PoiList* pPoiList) const
{
   PoiList vPoi;
   GetDesignPoi(intervalIdx, attrib, &vPoi);

   Float64 rgt_end, lft_end;
   GetMidZoneBoundaries(&lft_end, &rgt_end);

   pPoiList->reserve(vPoi.size());

   for ( const pgsPointOfInterest& poi : vPoi)
   {
      Float64 dist = poi.GetDistFromStart();

      // accept only pois in end zones
      if (dist <= lft_end || rgt_end <= dist)
      {
         pPoiList->push_back(poi);
      }
   }
}

pgsPointOfInterest pgsStrandDesignTool::GetDebondSamplingPOI(IntervalIndexType intervalIdx) const
{
   PoiList vPoi;
   m_PoiMgr.GetPointsOfInterest(m_SegmentKey, &vPoi);

   // grab first poi past transfer length
   Float64 xferLength = GetTransferLength(pgsTypes::Permanent);
   Float64 bound = Max( xferLength, WBFL::Units::ConvertToSysUnits(2.0,WBFL::Units::Measure::Inch)); // value is fairly arbitrary, we just don't want end poi

   for ( const pgsPointOfInterest& poi : vPoi)
   {
      Float64 dist = poi.GetDistFromStart();

      // accept only pois in end zones
      if (bound <= dist)
      {
         return poi;
      }
   }

   ATLASSERT(false); // impossible?
   return pgsPointOfInterest( m_SegmentKey,1.0);
}

void pgsStrandDesignTool::GetLiftingDesignPointsOfInterest(const CSegmentKey& segmentKey,Float64 overhang,PoiAttributeType poiReference, std::vector<pgsPointOfInterest>* pvPoi,Uint32 mode) const
{
   return GetHandlingDesignPointsOfInterest(segmentKey,overhang,overhang,POI_LIFT_SEGMENT,POI_PICKPOINT,pvPoi,mode);
}

void pgsStrandDesignTool::GetHaulingDesignPointsOfInterest(const CSegmentKey& segmentKey,Uint16 nPnts,Float64 leftOverhang,Float64 rightOverhang,PoiAttributeType poiReference, std::vector<pgsPointOfInterest>* pvPoi,Uint32 mode) const
{
   return GetHandlingDesignPointsOfInterest(segmentKey,leftOverhang,rightOverhang,POI_HAUL_SEGMENT,POI_BUNKPOINT,pvPoi,mode);
}

void pgsStrandDesignTool::GetHandlingDesignPointsOfInterest(const CSegmentKey& segmentKey,Float64 leftOverhang,Float64 rightOverhang,PoiAttributeType poiReference,PoiAttributeType supportAttribute, std::vector<pgsPointOfInterest>* pvPoi,Uint32 mode) const
{
   m_PoiMgr.RemovePointsOfInterest(poiReference);

   Float64 left_support_point_loc = leftOverhang;
   bool do_add_left = true;
   Float64 right_support_point_loc = m_SegmentLength-rightOverhang;
   bool do_add_right = true;

   Float64 span_length = m_SegmentLength - leftOverhang - rightOverhang;
   ATLASSERT( 0 < span_length);

   // Remove any current support point poi's if not at different location
   PoiList vPoiList;
   m_PoiMgr.GetPointsOfInterest(segmentKey,poiReference | supportAttribute,POIMGR_AND,&vPoiList); // get all pois with the support attribute

   // vPoiList holds references to constant pois.... we want to change the pois
   // covert the list into copies so we can manipulate them
   std::vector<pgsPointOfInterest> vPoi;
   MakePoiVector(vPoiList,&vPoi);


   for (pgsPointOfInterest& poi : vPoi) 
   {
      Float64 poi_loc = poi.GetDistFromStart();
      if (IsEqual(poi_loc,left_support_point_loc) )
      {
         // poi is at the location of the left support, no need to re-add
         do_add_left = false; 
      }
      else if ( IsEqual(poi_loc,right_support_point_loc) )
      {
         // poi is at the location of the right support, no need to re-add
         do_add_right = false; 
      }
      else
      {
         // We need to unset support point at existing location
         PoiAttributeType attributes = poi.GetReferencedAttributes(poiReference);
         if ( attributes != supportAttribute || poi.GetNonReferencedAttributes() != 0 )
         {
            // if the only flag that is set is support point attribute, remove it
            // otherwise, clear the support point attribute bit and add the poi back
            WBFL::System::Flags<PoiAttributeType>::Clear(&attributes,supportAttribute);
            poi.SetReferencedAttributes(attributes);
            poi.SetID(INVALID_ID);
            VERIFY(m_PoiMgr.AddPointOfInterest(poi) != INVALID_ID);
         }
      }
   }

   if (do_add_left)
   {
      pgsPointOfInterest left_support_point(segmentKey,left_support_point_loc,supportAttribute | poiReference);
      VERIFY(m_PoiMgr.AddPointOfInterest(left_support_point) != INVALID_ID);
   }

   if (do_add_right)
   {
      pgsPointOfInterest right_support_point(segmentKey,right_support_point_loc,supportAttribute | poiReference);
      VERIFY(m_PoiMgr.AddPointOfInterest(right_support_point) != INVALID_ID);
   }

   // add POI at ends of segment
   pgsPointOfInterest poiStart(segmentKey,0.0,poiReference);
   VERIFY(m_PoiMgr.AddPointOfInterest(poiStart) != INVALID_ID);

   pgsPointOfInterest poiEnd(segmentKey,m_SegmentLength,poiReference);
   VERIFY(m_PoiMgr.AddPointOfInterest(poiEnd) != INVALID_ID);

   // add 10th point attributes

   // nth point POI between overhang support points
   const Float64 toler = +1.0e-6;
   Uint16 nPnts = 2;//10; // for design we only need the mid-point POI. Additional POI slow down the design considerably
   // Every time the support locations move, there are 10 new 10th point POI. Eventually, these POI get mapped into the LBAM
   // and FEM models. These are temporary POI, but they never get removed from the LBAM and FEM models. By the time design is
   // done there are 1000's of POI that get analyzed over and over and over.
   for ( Uint16 i = 0; i < nPnts+1; i++ )
   {
      Float64 dist = leftOverhang + span_length * ((Float64)i / (Float64)nPnts);

      PoiAttributeType attribute = 0;

      // Add a special attribute flag if poi is at a tenth point
      Uint16 tenthPoint = 0;
      Float64 val = Float64(i)/Float64(nPnts)+toler;
      Float64 modv = fmod(val, 0.1);
      if (IsZero(modv,2*toler) || modv==0.1)
      {
         tenthPoint = Uint16(10.*Float64(i)/Float64(nPnts) + 1);
      }
      
      pgsPointOfInterest poi(segmentKey,dist,attribute);
      poi.MakeTenthPoint(poiReference,tenthPoint);
      VERIFY(m_PoiMgr.AddPointOfInterest(poi) != INVALID_ID);
   }
   

   PoiList vPoi2;
   m_PoiMgr.GetPointsOfInterest(segmentKey, poiReference, mode, &vPoi2);

   // get the ps xfer, debond, and harping point locations
   m_PoiMgr.GetPointsOfInterest(segmentKey, POI_PSXFER | POI_BARDEVELOP | POI_DEBOND | POI_SECTCHANGE | POI_HARPINGPOINT, POIMGR_OR, &vPoi2);

   // v2.9 lifting design considered 10th point POI at release, so we will add them here
   m_PoiMgr.GetPointsOfInterest(segmentKey, POI_RELEASED_SEGMENT, POIMGR_OR, &vPoi2);

   m_PoiMgr.SortPoiList(&vPoi2);

   MakePoiVector(vPoi2,pvPoi);
}

void pgsStrandDesignTool::ClearHandingAttributes(pgsPointOfInterest& poi)
{
   PoiAttributeType attributes = poi.GetReferencedAttributes(POI_LIFT_SEGMENT);
   WBFL::System::Flags<PoiAttributeType>::Clear(&attributes,POI_PICKPOINT);
   if ( attributes != 0 )
   {
      poi.SetReferencedAttributes(attributes);
   }

   attributes = poi.GetReferencedAttributes(POI_HAUL_SEGMENT);
   WBFL::System::Flags<PoiAttributeType>::Clear(&attributes,POI_BUNKPOINT);
   if ( attributes != 0 )
   {
      poi.SetReferencedAttributes(attributes);
   }
}

void pgsStrandDesignTool::ValidatePointsOfInterest()
{
   // POI's are managed locally because the global list can be polluted with information about the current design
   m_PoiMgr.RemoveAll();

   // Get full list of POIs for the current model. For straight and harped designs, this is all we need.
   // However, for debond designs, we need to remove any POI's added for debonding and then
   // add our own POI's at all possible debond locations

   // start by putting pois into a temporary vector
   GET_IFACE(IPointOfInterest,pPoi);

   // Get all points of interest, regardless of stage and attributes
   PoiList vPoi;
   pPoi->GetPointsOfInterest(m_SegmentKey, &vPoi);

   if (m_DesignOptions.doDesignForFlexure == dtDesignForHarping || 
       m_DesignOptions.doDesignForFlexure == dtNoDesign)
   {
      // For Harped designs, add all vPoi 
      // The none option is also considered here because if we are designing for shear only (no flexure design)
      // we need the same POI.
      for ( pgsPointOfInterest poi : vPoi) // not using const reference because we want a copy since we will be altering it
      {
         // Clear any lifting or hauling attributes
         ClearHandingAttributes(poi);

         Float64 loc = poi.GetDistFromStart();
         if (0.0 < loc && loc < m_SegmentLength) // locations at ends of girder are of no interest to design
         {
            poi.SetID(INVALID_ID);
            VERIFY(m_PoiMgr.AddPointOfInterest(poi) != INVALID_ID);
         }
      }

      // It is possible that the psxfer location is outside of the girder span.
      // This can cause oddball end conditions to control, so add an 
      // artifical xfer poi at the support locations if this is the case
      GET_IFACE(IPretensionForce,pPrestress);
      GET_IFACE(IBridge,pBridge);

      const PoiAttributeType attrib_xfer = POI_PSXFER;

      // want longest transfer length
      Float64 xfer_length = Max(pPrestress->GetTransferLength(m_SegmentKey, pgsTypes::Straight, pgsTypes::tltMinimum), 
                                pPrestress->GetTransferLength(m_SegmentKey, pgsTypes::Harped, pgsTypes::tltMinimum));

      Float64 start_conn = pBridge->GetSegmentStartEndDistance(m_SegmentKey);
      if (xfer_length < start_conn)
      {
         pgsPointOfInterest pxfer(m_SegmentKey,start_conn,attrib_xfer);
         VERIFY(m_PoiMgr.AddPointOfInterest(pxfer) != INVALID_ID);
      }

      Float64 end_conn = pBridge->GetSegmentEndEndDistance(m_SegmentKey);

      if (xfer_length < end_conn)
      {
         pgsPointOfInterest pxfer(m_SegmentKey,m_SegmentLength-end_conn,attrib_xfer);
         VERIFY(m_PoiMgr.AddPointOfInterest(pxfer) != INVALID_ID);
      }

   }
   else
   {
      ATLASSERT(m_DesignOptions.doDesignForFlexure!=dtNoDesign);
      // Debonding or straight strands: add all pois except those at at debond and transfer locations
      for (pgsPointOfInterest poi : vPoi) // not using const reference because we want a copy since we will be altering it
      {
         // Clear any lifting or hauling attributes
         ClearHandingAttributes(poi);

         Float64 loc = poi.GetDistFromStart();
         if ( 0.0 < loc && loc < m_SegmentLength )
         {
            // if POI does not have the POI_DEBOND or POI_PSXFER attribute in the casting yard stage
            // then add it
            if ( ! ( poi.HasAttribute(POI_DEBOND) || 
                     poi.HasAttribute(POI_PSXFER) ) )
            {
               poi.SetID(INVALID_ID);
               VERIFY(m_PoiMgr.AddPointOfInterest(poi) != INVALID_ID);
            }
         }
      }

      // now add pois at all possible debond locations and their transfer points
      PoiAttributeType attrib_debond = POI_DEBOND;
      PoiAttributeType attrib_xfer   = POI_PSXFER;

      GET_IFACE(IBridge,pBridge);
      GET_IFACE(IPretensionForce,pPrestress);

      Float64 start_supp = pBridge->GetSegmentStartEndDistance(m_SegmentKey);
      Float64 end_supp   = m_SegmentLength - pBridge->GetSegmentEndEndDistance(m_SegmentKey);

      // left and right xfer from ends
      // loop because xfer length can be different if strand sizes are different
      for (int i = 0; i < 2; i++)
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
         Float64 xfer_length = pPrestress->GetTransferLength(m_SegmentKey, strandType, pgsTypes::tltMinimum);
         pgsPointOfInterest lxfer(m_SegmentKey, xfer_length, attrib_xfer);
         AddPOI(lxfer, start_supp, end_supp);

         pgsPointOfInterest rxfer(m_SegmentKey, m_SegmentLength - xfer_length, attrib_xfer);
         AddPOI(rxfer, start_supp, end_supp);
      }

      if (m_DesignOptions.doDesignForFlexure == dtDesignForDebonding || 
          m_DesignOptions.doDesignForFlexure == dtDesignForDebondingRaised)
      {
         // debonding at left and right ends
         Float64 leftEnd, rightEnd;
         GetMidZoneBoundaries(&leftEnd, &rightEnd);

         GET_IFACE(IDebondLimits,pDebondLimits);
         Float64 db_incr = pDebondLimits->GetMinDistanceBetweenDebondSections(m_SegmentKey);
      
         Int16 nincs = (Int16)floor((leftEnd + 1.0e-05)/db_incr); // we know both locs are equidistant from ends

         Float64 xfer_length = pPrestress->GetTransferLength(m_SegmentKey, pgsTypes::Straight, pgsTypes::tltMinimum);

         Float64 ldb_loc=0.0;
         Float64 rdb_loc=m_SegmentLength;
         for (Int32 inc=0; inc<nincs; inc++)
         {
            if (inc+1 == nincs)
            {
               // At end of debond zone.
               // This is a bit of a hack, but treat final debond point as a harping point for design considerations
                attrib_debond |= POI_HARPINGPOINT;
            }

            // left debond and xfer
            ldb_loc += db_incr;
            pgsPointOfInterest ldbpo(m_SegmentKey,ldb_loc,attrib_debond);
            AddPOI(ldbpo, start_supp, end_supp);

            Float64 lxferloc = ldb_loc + xfer_length;
            pgsPointOfInterest lxfpo(m_SegmentKey,lxferloc,attrib_xfer);
            AddPOI(lxfpo, start_supp, end_supp);

            // right debond and xfer
            rdb_loc -= db_incr;
            pgsPointOfInterest rdbpo(m_SegmentKey,rdb_loc,attrib_debond);
            AddPOI(rdbpo, start_supp, end_supp);

            Float64 rxferloc = rdb_loc - xfer_length;
            pgsPointOfInterest rxfpo(m_SegmentKey,rxferloc,attrib_xfer);
            AddPOI(rxfpo, start_supp, end_supp);
         }
      }
   }
}

void pgsStrandDesignTool::AddPOI(pgsPointOfInterest& rpoi, Float64 lft_conn, Float64 rgt_conn)
{
   VERIFY(m_PoiMgr.AddPointOfInterest(rpoi) != INVALID_ID);
}

void pgsStrandDesignTool::ComputeMidZoneBoundaries()
{
   LOG(_T("Entering ComputeMidZoneBoundaries"));
   // Mid-zone length along beam where positive bending typically controls
   // Harped designs use harping points. User debond rules for all others
   if (m_DesignOptions.doDesignForFlexure == dtDesignForHarping)
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);
      Float64 lhp, rhp;
      pStrandGeom->GetHarpingPointLocations(m_SegmentKey,&lhp,&rhp);

      m_lftMz  = lhp;
      m_rgtMz = rhp;

      LOG(_T("Mid-Zone boundaries are at harping points. Left = ")<< WBFL::Units::ConvertFromSysUnits(m_lftMz,WBFL::Units::Measure::Feet) << _T(" ft, Right = ")<< WBFL::Units::ConvertFromSysUnits(m_rgtMz,WBFL::Units::Measure::Feet) << _T(" ft"));

   }
   else if (m_DesignOptions.doDesignForFlexure == dtDesignFullyBondedRaised)
   {
      // Very simple assumption is that mid zone is between quarter points
      m_lftMz = 0.25 * m_SegmentLength;
      m_rgtMz = 0.75 * m_SegmentLength;
   }
   else
   {
      // Mid zone for debonding is envelope of girderLength/2 - dev length, and user-input limits
      GET_IFACE(ISegmentData,pSegmentData);
      GET_IFACE(IBridgeDescription,pIBridgeDesc);

      const CBridgeDescription2* pBridgeDesc  = pIBridgeDesc->GetBridgeDescription();

      // Need development length for design and this is a chicken and egg kinda thing
      // make some basic assumptions;

      // According to Article 5.6.3.1.1 (pre2017: 5.7.3.1.1), fpe must be at least 50% of fps  (where fps is fpu at the most)
      // Substitute into (5.11.4.2-1)
      // k = 2.0  (from 5.11.4.3)
      // We get:
      // ld = 2.0(fps - 2/3 * fps/2.0)db   = 4/3(fpu)db
      // 
      // This is highly conservative and has caused problems with short span designs, so reduce ld by
      // 15% (rdp - 11/17/2015, after running regression tests)
      // 

      // straight strands are ok here because we are dealing with debonding, which only happens with straight strands
      const auto* pStrand = pSegmentData->GetStrandMaterial(m_SegmentKey, pgsTypes::Straight); 
      ATLASSERT(pStrand != nullptr);

      // use US units
      Float64 db = WBFL::Units::ConvertFromSysUnits(pStrand->GetNominalDiameter(),WBFL::Units::Measure::Inch);
      Float64 fpu = WBFL::Units::ConvertFromSysUnits(pStrand->GetUltimateStrength(),WBFL::Units::Measure::KSI);

      Float64 dev_len = fpu*db*4.0/3.0 * 0.85; // 15% reduction is arbitrary, but fixes Mantis 485 and makes better designs.
                                               // More testing may allow more reduction.

      dev_len = WBFL::Units::ConvertToSysUnits(dev_len,WBFL::Units::Measure::Inch);
      LOG(_T("Approximate upper bound of development length = ")<< WBFL::Units::ConvertFromSysUnits(dev_len,WBFL::Units::Measure::Inch)<<_T(" in"));

      Float64 mz_end_len = m_SegmentLength/2.0 - dev_len;
      LOG(_T("Max debond length based on development length = ")<< WBFL::Units::ConvertFromSysUnits(mz_end_len,WBFL::Units::Measure::Inch)<<_T(" in"));

      bool bSpanFraction, buseHard;
      Float64 spanFraction, hardDistance;
      m_pGirderEntry->GetMaxDebondedLength(&bSpanFraction, &spanFraction, &buseHard, &hardDistance);

      if (bSpanFraction)
      {
         Float64 sflen = m_SegmentLength * spanFraction;
         LOG(_T("User-input min MZ fractional length = ")<< WBFL::Units::ConvertFromSysUnits(sflen,WBFL::Units::Measure::Inch)<<_T(" in"));

         mz_end_len = Min(mz_end_len, sflen);
      }

      if (buseHard)
      {
         mz_end_len = Min(mz_end_len, hardDistance);
         LOG(_T("User-input min MZ hard length = ")<< WBFL::Units::ConvertFromSysUnits(hardDistance,WBFL::Units::Measure::Inch)<<_T(" in"));
      }

      mz_end_len = Max(mz_end_len, 0.0); // can't be less than zero
      LOG(_T("Raw MZ end length = ")<< WBFL::Units::ConvertFromSysUnits(mz_end_len,WBFL::Units::Measure::Inch)<<_T(" in"));
      LOG(_T("Girder length = ")<< WBFL::Units::ConvertFromSysUnits(m_SegmentLength,WBFL::Units::Measure::Inch)<<_T(" in"));
 
      GET_IFACE(IDebondLimits, pDebondLimits);
      Float64 db_incr = pDebondLimits->GetMinDistanceBetweenDebondSections(m_SegmentKey);
      LOG(_T("Debond spacing increment = ")<< WBFL::Units::ConvertFromSysUnits(db_incr,WBFL::Units::Measure::Inch)<<_T(" in"));
   
      if (mz_end_len < db_incr)
      {
         // we can't debond because there is no room
         LOG(_T("**** No room for Debonding and no use trying - switch to straight strand design ****"));
         m_DesignOptions.doDesignForFlexure = dtDesignFullyBonded;
         m_lftMz = 0.0;
         m_rgtMz = m_SegmentLength;

         // cache debond section information
         m_NumDebondSections   = 1; // always have a section at beam ends
         m_DebondSectionLength = 0.0;
      }
      else
      {
         // mid-zone length must be a multiple of debond increment 
         Int16 nincs = (Int16)floor((mz_end_len + 1.0e-05)/db_incr);

         m_lftMz = nincs*db_incr;
         m_rgtMz = m_SegmentLength - m_lftMz;

         // cache debond section information
         m_NumDebondSections   = nincs+1; // always have a section at beam ends
         m_DebondSectionLength = db_incr;

         LOG(_T("Number of debond increments to MZ  = ")<< nincs);
      }
 
      LOG(_T("Left MZ location = ")<< WBFL::Units::ConvertFromSysUnits(m_lftMz,WBFL::Units::Measure::Inch)<<_T(" in"));
      LOG(_T("Right MZ location = ")<< WBFL::Units::ConvertFromSysUnits(m_rgtMz,WBFL::Units::Measure::Inch)<<_T(" in"));
      LOG(_T("Number of debond sections to MZ  = ")<< m_NumDebondSections);
      LOG(_T("Debond section length for design  = ")<< WBFL::Units::ConvertFromSysUnits(m_DebondSectionLength,WBFL::Units::Measure::Inch)<<_T(" in"));

      ATLASSERT(m_lftMz<m_SegmentLength/2.0);
   }
   LOG(_T("Exiting ComputeMidZoneBoundaries"));
}

void pgsStrandDesignTool::InitHarpedPhysicalBounds(const WBFL::Materials::PsStrand* pstrand)
{
   m_bConfigDirty = true; // cache is dirty

   // Initialize strand offsets
   m_pArtifact->SetHarpStrandOffsetEnd(pgsTypes::metStart,0.00);
   m_pArtifact->SetHarpStrandOffsetHp(pgsTypes::metStart,0.00);
   m_pArtifact->SetHarpStrandOffsetHp(pgsTypes::metEnd,0.00);
   m_pArtifact->SetHarpStrandOffsetEnd(pgsTypes::metEnd,0.00);

   // intialize data for strand slope and hold down checks
   GET_IFACE(IStrandGeometry,pStrandGeom);
   // no use if there are no harped strands
   StrandIndexType nh_max = pStrandGeom->GetMaxStrands(m_SegmentKey,pgsTypes::Harped);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   bool bCheck,bDesign;
   Float64 s50, s60, s70;
   pSpecEntry->GetMaxStrandSlope(&bCheck,&bDesign,&s50,&s60,&s70);

   m_DoDesignForStrandSlope = bDesign && (0 < nh_max ? true : false);

   if ( m_DoDesignForStrandSlope )
   {
      if ( pstrand->GetSize() == WBFL::Materials::PsStrand::Size::D1778  )
      {
         m_AllowableStrandSlope = s70;
      }
      else if ( pstrand->GetSize() == WBFL::Materials::PsStrand::Size::D1524 )
      {
         m_AllowableStrandSlope = s60;
      }
      else
      {
         m_AllowableStrandSlope = s50;
      }

      LOG(_T("We will be designing for an allowable strand slope of 1:") << m_AllowableStrandSlope);
   }
   else
   {
      LOG(_T("Strand slope not a design criteria"));
   }

   // hold down
   int holdDownForceType;
   pSpecEntry->GetHoldDownForce(&bCheck,&bDesign,&holdDownForceType,&m_AllowableHoldDownForce,&m_HoldDownFriction);
   m_bTotalHoldDownForce = (holdDownForceType == HOLD_DOWN_TOTAL);

   m_DoDesignForHoldDownForce = bDesign && nh_max>0;

   if (m_DoDesignForHoldDownForce)
   {
      LOG(_T("We will be designing for harped strand hold down allowable: ") << WBFL::Units::ConvertFromSysUnits(m_AllowableHoldDownForce,WBFL::Units::Measure::Kip) << _T(" kips"));
   }
   else
   {
      LOG(_T("Hold down force not a design criteria"));
   }
}

//////////////////////////////// Debond Design //////////////////////////

// utility struct to temporarily store and sort rows
struct Row
{
   Float64 Elevation;
   StrandIndexType   MaxInRow;
   std::vector<StrandIndexType> StrandsFilled;
   std::vector<StrandIndexType> StrandsDebonded;

   Row():
      Elevation(0.0),
      MaxInRow(0)
      {;}

   Row(Float64 elev):
      Elevation(elev),
      MaxInRow(0)
      {;}

   bool operator==(const Row& rOther) const 
   { 
      return ::IsEqual(Elevation,rOther.Elevation); 
   }

   bool operator<(const Row& rOther) const 
   { 
      return ::IsLT(Elevation,rOther.Elevation); 
   }
};
typedef std::set<Row> RowSet;
typedef RowSet::iterator RowIter;


// utility struct to temporarily store debond level information
struct TempLevel
{
   std::pair<StrandIndexType,StrandIndexType> DebondsAtLevel;
   StrandIndexType MinStrandsForLevel;

   TempLevel(StrandIndexType firstStrand, StrandIndexType secondStrand, StrandIndexType totalForLevel):
      DebondsAtLevel(firstStrand, secondStrand), 
      MinStrandsForLevel(totalForLevel)
   {;}
};
typedef std::vector<TempLevel> TempLevelList;
typedef TempLevelList::iterator TempLevelIterator;


void pgsStrandDesignTool::InitDebondData()
{
   m_bConfigDirty = true; // cache is dirty

   m_pArtifact->ClearDebondInfo();

   m_DebondLevels.clear();

   m_NumDebondSections = 0;
   m_DebondSectionLength = 0;
   m_MaxPercentDebondSection = 0;
   m_MaxDebondSection = 0;
}

void pgsStrandDesignTool::ComputeDebondLevels(IPretensionForce* pPrestressForce)
{
   LOG(_T(""));
   LOG(_T("Enter ComputeDebondLevels"));
   LOG(_T("*************************"));

   if (m_DesignOptions.doDesignForFlexure != dtDesignForDebonding && 
       m_DesignOptions.doDesignForFlexure != dtDesignForDebondingRaised)
   {
      LOG(_T("Exiting ComputeDebondLevels - this is not a debond design"));
      return;
   }

   if (m_NumDebondSections <= 1)
   {
      // no mid-zone means no debonding
      LOG(_T("No mid-zone - Cannot build debond levels"));
      ATLASSERT(false);
      return;
   }

   GET_IFACE(IDebondLimits,pDebondLimits);
   // Debond levels represent all of the possible debonding schemes for the current section
   // Debonding is added in the same sequence as strand fill order. However, we must abide by the
   // debonding limit rules from the specification
   // First get the rules

   bool bCheckMaxPercentTotal = pDebondLimits->CheckMaxDebondedStrands(m_SegmentKey);
   Float64 db_max_percent_total = pDebondLimits->GetMaxDebondedStrands(m_SegmentKey);
   Float64 db_max_percent_row   = pDebondLimits->GetMaxDebondedStrandsPerRow(m_SegmentKey);

   pDebondLimits->GetMaxDebondedStrandsPerSection(m_SegmentKey, &m_MaxDebondSection10orLess, &m_MaxDebondSection, &m_bCheckMaxFraAtSection, &m_MaxPercentDebondSection);

   if (bCheckMaxPercentTotal)
   {
      LOG(_T("db_max_percent_total = ") << db_max_percent_total);
   }
   LOG(_T("db_max_percent_row = ")<<db_max_percent_row);
   LOG(_T("m_MaxDebondSection10orLess = ") << m_MaxDebondSection10orLess);
   LOG(_T("m_MaxDebondSection = ") << m_MaxDebondSection);
   if (m_bCheckMaxFraAtSection)
   {
      LOG(_T("m_MaxPercentDebondSection = ") << m_MaxPercentDebondSection);
   }

   GET_IFACE(IStrandGeometry,pStrandGeom);

   // coordinates of all straight strands
   StrandIndexType max_ss = pStrandGeom->GetMaxStrands(m_SegmentKey,pgsTypes::Straight);

   ConfigStrandFillVector sfillvec = pStrandGeom->ComputeStrandFill(m_SegmentKey, pgsTypes::Straight, max_ss);
   PRESTRESSCONFIG sconfig;
   sconfig.SetStrandFill(pgsTypes::Straight, sfillvec);

   pgsPointOfInterest poi_ms(m_SegmentKey, m_SegmentLength/2.0);
   CComPtr<IPoint2dCollection> ss_locations;
   pStrandGeom->GetStrandPositionsEx(poi_ms, sconfig, pgsTypes::Straight, &ss_locations);

   // First chore is to build a raw list of possible strands that can be debonded
   // Debondable list will be paired down by total, and row constraints in the next step
   CComPtr<IIndexArray> debondables;
   pStrandGeom->ListDebondableStrands(m_SegmentKey, sfillvec, pgsTypes::Straight, &debondables);

   // Build rows as we fill strands, and compute max available number of strands in any row.
   RowSet rows;

   typedef std::pair<StrandIndexType,StrandIndexType> StrandPair;
   std::vector<StrandPair> debondable_list;

   LOG(_T("Building list of debondable strands:"));
   IndexType num_debondable = 0;
   StrandIndexType currnum=0;
   while( currnum < max_ss )
   {
      StrandIndexType nextnum = pStrandGeom->GetNextNumStrands(m_SegmentKey, pgsTypes::Straight, currnum);

      StrandIndexType strands_in_lift = nextnum-currnum; // number of strands in current increment

      StrandIndexType is_debondable;

      // nextnum is a count. need to subtract 1 to get index
      debondables->get_Item(nextnum-1, &is_debondable);

      if (is_debondable != 0)
      {
         // could be one or two strands
         if ( strands_in_lift == 1 )
         {
            num_debondable++;
            debondable_list.push_back( StrandPair(nextnum-1, INVALID_INDEX) );
            LOG(nextnum-1);
         }
         else
         {
            num_debondable += 2;
            debondable_list.push_back( StrandPair(nextnum-2, nextnum-1) );
            LOG(nextnum-2<<_T(", ")<<nextnum-1);
         }
      }
      // count max number of strands in each row
      Float64 curr_y = GetPointY(nextnum-1, ss_locations);

      // fill row with current strands
      // note that logic here is same whether we insert a new row or find an existing one
      Row fill_test_row(curr_y);
      std::pair<RowIter, bool> curr_row_it = rows.insert( fill_test_row );

      Row& curr_row = const_cast<Row&>(*curr_row_it.first);
      curr_row.MaxInRow += strands_in_lift;

      // back to top
      currnum = nextnum;
   }

   // Straight adjustable strands can live in the same rows as debondable strands. We need to take this into consideration

   const GDRCONFIG& config = pgsStrandDesignTool::GetSegmentConfiguration();
   if (pgsTypes::asStraight == config.PrestressConfig.AdjustableStrandType)
   {
      // coordinates of all adj straight strands
      StrandIndexType max_adjss = pStrandGeom->GetMaxStrands(m_SegmentKey, pgsTypes::Harped);

      ConfigStrandFillVector adjsfillvec = pStrandGeom->ComputeStrandFill(m_SegmentKey, pgsTypes::Harped, max_adjss);
      sconfig.SetStrandFill(pgsTypes::Harped, adjsfillvec);

      CComPtr<IPoint2dCollection> adjss_locations;
      pStrandGeom->GetStrandPositionsEx(poi_ms, sconfig, pgsTypes::Harped, &adjss_locations);

      currnum = 0;
      while (currnum < max_adjss)
      {
         StrandIndexType nextnum = pStrandGeom->GetNextNumStrands(m_SegmentKey, pgsTypes::Harped, currnum);

         StrandIndexType strands_in_lift = nextnum - currnum; // number of strands in current increment

         // add to max number of strands in each row
         Float64 curr_y = GetPointY(nextnum - 1, adjss_locations);

         // fill row with current strands. if debond row exists
         Row fill_test_row(curr_y);
         RowIter curr_row_it = rows.find(fill_test_row);
         if (curr_row_it != rows.end())
         {
            Row& curr_row = const_cast<Row&>(*curr_row_it);
            curr_row.MaxInRow += strands_in_lift;
         }

         // back to top
         currnum = nextnum;
      }
   }

   // strands are located from the top down, in girder section coordinates.
   // in 2.9 they were located bottom up... need to add Hg to the strand location
   // to match the 2.9 log files
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);
   GET_IFACE(ISectionProperties,pSectProp);
   Float64 Hg = pSectProp->GetHg(releaseIntervalIdx,pgsPointOfInterest(m_SegmentKey,0.0));

   if (debondable_list.empty())
   {
      LOG(_T("No debondable strands - Cannot build debond levels"));
      ATLASSERT(false); // this should probably be vetted before here ?
      return;
   }
#ifdef ENABLE_LOGGING
   else
   {
      LOG(_T("Finished building debondable list of ")<<num_debondable<<_T(" strands"));
      LOG(_T("Max Strands per row in ")<<rows.size()<<_T(" rows as follows:"));
      for (RowIter riter=rows.begin(); riter!=rows.end(); riter++)
      {
         LOG(_T("elev = ")<<WBFL::Units::ConvertFromSysUnits(Hg + riter->Elevation,WBFL::Units::Measure::Inch)<<_T(" max strands = ")<<riter->MaxInRow);
      }
   }
#endif // ENABLE_LOGGING

   // Second step is to build temporary list containing all debond levels
   // that can be created
   TempLevelList temp_levels;

   // Set queue of possible strands to be debonded 
   std::vector<StrandPair>::iterator db_iter = debondable_list.begin();

   Int16 num_debonded = 0;

   // Simulate filling all straight strands and debond when possible
   LOG(_T("Build row information and debond levels"));
   currnum=0;
   while( currnum < max_ss )
   {
      StrandIndexType nextnum = pStrandGeom->GetNextNumStrands(m_SegmentKey, pgsTypes::Straight, currnum);

      Float64 curr_y = GetPointY(nextnum-1, ss_locations);

      // get row for current strand(s) and add to number filled
      Row fill_test_row(curr_y);
      RowIter curr_row_it = rows.find( fill_test_row );
      if ( curr_row_it == rows.end() )
      {
         ATLASSERT(false); // All rows should have been created in previous loop - this is a bug!!!
         return;
      }

      Row& curr_row = const_cast<Row&>(*curr_row_it);
      if (nextnum-currnum == 2)
      {
         curr_row.StrandsFilled.push_back(nextnum-2);
      }

      curr_row.StrandsFilled.push_back(nextnum-1);

      LOG(_T("nextnum = ")<<nextnum<<_T(" Y = ")<<WBFL::Units::ConvertFromSysUnits(Hg+curr_y,WBFL::Units::Measure::Inch)<<_T(" to fill = ")<< nextnum-currnum );

      // TRICKY: A nested loop here to try and debond as many strands on the queue as possible for the current fill
      //         Most of the time we will break out of this loop before our queus is depleted
      //         If not for this loop, we could end up with unused debondable strands
      while( db_iter != debondable_list.end() )
      {
         // See if we can debond current strand(s) on the queue
         LOG(_T("Attempt to debond ")<<db_iter->first<<_T(", ")<<db_iter->second);

         Int16 num_to_db = (db_iter->second == INVALID_INDEX) ? 1 : 2;
         // first check percentage of total
         Float64 percent_of_total = Float64(num_debonded+num_to_db) / nextnum;

         // can't debond strands out of order
         if ( nextnum-currnum==1 && db_iter->first+1  > nextnum  || // one strand to debond
              nextnum-currnum==2 && db_iter->second+1 > nextnum )   // two to debond
         {
            // not enough total straight strands yet. continue
            // break from inner loop because we need more strands
            LOG(_T("Debonding of strands ")<<db_iter->first<<_T(",")<<db_iter->second<<_T(" cannot occur until more strands are added in fill order."));
            break;
         }
         else if (bCheckMaxPercentTotal /*only do this if checking*/ && (db_max_percent_total < percent_of_total) )
         {
            // not enough total straight strands yet. continue
            // break from inner loop because we need more strands
            LOG(_T("Debonding of strands ")<<db_iter->first<<_T(",")<<db_iter->second<<_T(" cannot occur until more strands are added for % total"));
            break;
         }
         else
         {
            // we can add strands to the total, can we add them to this row?
            // get row elevation of strand(s) to be debonded
            Float64 curr_db_y = GetPointY(db_iter->first, ss_locations);

            Row db_test_row(curr_db_y);
            RowIter db_row_it = rows.find( db_test_row );
            if ( db_row_it != rows.end() )
            {
               Row& db_row = const_cast<Row&>(*db_row_it);
               StrandIndexType num_ss_in_row = db_row.StrandsFilled.size();
               StrandIndexType num_db_in_row = db_row.StrandsDebonded.size();

               Float64 new_db_percent = Float64(num_to_db + num_db_in_row) / num_ss_in_row;
               if (new_db_percent <= db_max_percent_row )
               {
                  // We can debond these strands, we have a new debond level
                  temp_levels.push_back( TempLevel(db_iter->first, db_iter->second, nextnum) ); 
                  LOG(_T("Created debond level ")<<temp_levels.size()<<_T(" with strands ")<<db_iter->first<<_T(",")<<db_iter->second<<_T(" and ")<<nextnum<<_T(" minimum strands"));
                  db_row.StrandsDebonded.push_back(db_iter->first);
                  num_debonded++;
                  if (num_to_db == 2)
                  {
                     db_row.StrandsDebonded.push_back(db_iter->second);
                     num_debonded++;
                  }

                  db_iter++; // pop from queue
               }
               else
               {
                  // See if we can ever debond any more strands in this row.
                  // If not, jump to next in queue
                  Float64 db_percent_max = Float64(num_to_db + num_db_in_row) / db_row.MaxInRow;
                  if ( db_max_percent_row < db_percent_max )
                  {
                     LOG(_T("Row at ")<<WBFL::Units::ConvertFromSysUnits(Hg+curr_db_y,WBFL::Units::Measure::Inch)<<_T(" (in), is full try next debondable in queue."));
                  
                     db_iter++;
                  }
                  else
                  {
                     // break from inner loop because we need more strands
                     LOG(_T("Cannot debond in row at ")<<WBFL::Units::ConvertFromSysUnits(Hg+curr_db_y,WBFL::Units::Measure::Inch)<<_T(" (in), until more strands are added"));
                     break;
                  }
               }
            }
            else
            {
               ATLASSERT(false); // bug all rows should have been created by now
            }
         }
      }

      if (db_iter == debondable_list.end())
      {
         LOG(_T("No more debondable strands - exiting debond level loop after filling ")<<nextnum<<_T(" strands"));
         break;
      }

      currnum = nextnum;
   }
   LOG(temp_levels.size()<<_T(" temporary debond levels created, ")<<num_debonded<<_T(" strands can be debonded"));

   // now we can build our _T("real") debond levels data structure
   // First gather some information required to compute debond level information

   // add an empty debond level at head of list. This is debond level 0
   DebondLevel db_level_z;
   db_level_z.Init(Hg,ss_locations);
   m_DebondLevels.push_back(db_level_z);

   // now add the rest
   std::vector<StrandIndexType> running_debonds;
   running_debonds.reserve(num_debonded);
   for (TempLevelIterator temp_iter = temp_levels.begin(); temp_iter!=temp_levels.end(); temp_iter++)
   {
      const TempLevel& tl = *temp_iter;

      // running list of debonded strands for each level
      running_debonds.push_back(tl.DebondsAtLevel.first);
      if (tl.DebondsAtLevel.second != INVALID_INDEX)
      {
         running_debonds.push_back(tl.DebondsAtLevel.second);
      }

      DebondLevel db_level;
      db_level.MinTotalStrandsRequired = tl.MinStrandsForLevel;
      // starnds for each debond level are cumulative
      db_level.StrandsDebonded.assign(running_debonds.begin(), running_debonds.end());

      // compute values for each debond level
      db_level.Init(Hg,ss_locations);

      m_DebondLevels.push_back(db_level);
   }
   // finished creating debond levels - dump them and get outta here
#ifdef _DEBUG
   DumpDebondLevels(Hg);
#endif

   LOG(_T("Exiting ComputeDebondLevels"));
   LOG(_T("****************************"));
}

void pgsStrandDesignTool::DebondLevel::Init(Float64 Hg,IPoint2dCollection* strandLocations)
{
   // Compute and store eccentricity
   StrandIndexType ns = StrandsDebonded.size();
   if (0 < ns)
   {
      Float64 strand_cg = 0.0;
      for (StrandIndexType strandIdx = 0; strandIdx < ns; strandIdx++)
      {
         StrandIndexType ssn = StrandsDebonded[strandIdx];
         Float64 curr_y = GetPointY(ssn, strandLocations);
         strand_cg += curr_y;
      }

      m_DebondedStrandsCg = strand_cg/ns;
   }
   else
   {
      m_DebondedStrandsCg = -Hg;
   }
}

Float64 pgsStrandDesignTool::DebondLevel::ComputeReliefStress(Float64 pePerStrandFullyBonded, Float64 pePerStrandDebonded, StrandIndexType nperm, StrandIndexType ntemp, Float64 cgFb,
                                                              Float64 Hg, Float64 Yb,  Float64 eccX, Float64 Ca, Float64 Cmx, Float64 Cmy, SHARED_LOGFILE LOGFILE) const
{
   // first compute stress from prestress due to fully bonded strand group
   StrandIndexType ntot = nperm + ntemp;
   Float64 e_fb  = Yb - (Hg + cgFb); // eccentricity of total strand group
   Float64 P_fb  = -1.0 * ntot * pePerStrandFullyBonded;
   Float64 Mx_fb =  P_fb * e_fb;
   Float64 My_fb =  P_fb * eccX;

   Float64 stress_fb = P_fb*Ca + Mx_fb*Cmx + My_fb*Cmy;

   // Now get cg and eccentricity of new group with strands debonded
   StrandIndexType nsdb = StrandsDebonded.size();
   StrandIndexType ns_new = ntot - nsdb;
   Float64 cg_new = (ntot * cgFb  - nsdb * m_DebondedStrandsCg) / ns_new;

   // stress due to prestress for _new debonded section
   Float64 e_new  = Yb - (Hg + cg_new);
   Float64 P_new  = -1.0 * ns_new * pePerStrandDebonded;
   Float64 Mx_new = P_new * e_new;
   Float64 My_new = P_new * eccX;

   Float64 stress_new = P_new*Ca + Mx_new*Cmx + My_new*Cmy;

   // difference is how much debonding will relieve the stress here
   Float64 stress_relief = stress_fb - stress_new;

   LOG(_T("ComputeReliefStress: pePerStrandDebonded=") << WBFL::Units::ConvertFromSysUnits(pePerStrandDebonded, WBFL::Units::Measure::Kip) << _T(" Kip, pePerStrandFullyBonded=")
      << WBFL::Units::ConvertFromSysUnits(pePerStrandFullyBonded, WBFL::Units::Measure::Kip) << _T(" Kip, nperm=") << nperm << _T(", ntemp=") << ntemp << _T(", cgFB=") << WBFL::Units::ConvertFromSysUnits(cgFb, WBFL::Units::Measure::Inch)
      << _T(" in, Hg=") << WBFL::Units::ConvertFromSysUnits(Hg, WBFL::Units::Measure::Inch) << _T(" in, Yb=") << WBFL::Units::ConvertFromSysUnits(Yb, WBFL::Units::Measure::Inch) << _T(" in"));
   LOG(_T("                     m_DebondedStrandsCg=") << WBFL::Units::ConvertFromSysUnits(m_DebondedStrandsCg, WBFL::Units::Measure::Inch)<< _T(" in, ntot=") << ntot << _T(" ndebonded=") << nsdb <<_T(", cg_new=") << WBFL::Units::ConvertFromSysUnits(cg_new, WBFL::Units::Measure::Inch) 
      << _T(" in, e_fb=") << WBFL::Units::ConvertFromSysUnits(e_fb, WBFL::Units::Measure::Inch) << _T(" in, e_new=") << WBFL::Units::ConvertFromSysUnits( e_new, WBFL::Units::Measure::Inch)  
      << _T("in, Pe=") << WBFL::Units::ConvertFromSysUnits((ntot-nsdb)*pePerStrandDebonded, WBFL::Units::Measure::Kip) << _T(" Kip"));
   LOG(_T("                     stress_fb=") << WBFL::Units::ConvertFromSysUnits(stress_fb, WBFL::Units::Measure::KSI) << _T(" ksi, stress_new=") << WBFL::Units::ConvertFromSysUnits(stress_new, WBFL::Units::Measure::KSI) << _T(" ksi, stress_relief=") << WBFL::Units::ConvertFromSysUnits(stress_relief, WBFL::Units::Measure::KSI));

   return stress_relief;
}

void pgsStrandDesignTool::DumpDebondLevels(Float64 Hg)
{
   LOG(_T("Dump of ")<<m_DebondLevels.size()<<_T(" debond levels:"));
   Int16 levn = 0;
   for (DebondLevelIterator it=m_DebondLevels.begin(); it!=m_DebondLevels.end(); it++)
   {
      LOG(_T("Debond Level ")<<levn);
      DebondLevel& level = *it;
      LOG(_T("   MinTotalStrandsRequired = ")<<level.MinTotalStrandsRequired);

      std::_tostringstream stream;
      for (std::vector<StrandIndexType>::iterator itss = level.StrandsDebonded.begin(); itss!=level.StrandsDebonded.end(); itss++)
      {
         stream <<*itss<<_T(", ");
      }
      // chop  final ,
      std::_tstring str(stream.str());
      std::_tstring::size_type n = str.size();
      if (0 < n)
      {
         str.erase(n-2,2);
      }
      LOG(_T("   Debonded Strands = ")<<str);
      LOG(_T("   DebondedStrandsCg = ")<<WBFL::Units::ConvertFromSysUnits(Hg+level.m_DebondedStrandsCg,WBFL::Units::Measure::Inch));
      levn++;
   }
}

bool pgsStrandDesignTool::MaximizeDebonding()
{
   LOG(_T("Entering MaximizeDebonding"));
   // Basic idea here is to apply the maximum amount of debonding we can based
   // on physical constraints. We then apply it in order
   // to minimize release and final concrete strength during the early design
   // stages. Maximizing debonding has the same affect as raising harped strands at ends.
   //
   // Once strengths are picked we can return and do _T("real") debond design

   // Get maximum debond levels for current number of straight strands, and sections
   StrandIndexType ns = GetNs();

   LOG(_T("max_sections = ")<< m_NumDebondSections<<_T(", ns = ")<<ns);

   std::vector<DebondLevelType> debond_levels;
   debond_levels.reserve(m_NumDebondSections);

   for (StrandIndexType strandIdx = 0; strandIdx < m_NumDebondSections; strandIdx++)
   {
      // number of leading sections is number of sections inboard from current is
      SectionIndexType num_leading_sections;

      // Can't actually debond at location zero (end of girder), so this can only have as
      // many leading sections as section 1
      if (strandIdx == 0)
      {
         ATLASSERT(2 <= m_NumDebondSections);
         num_leading_sections = m_NumDebondSections-2;
      }
      else
      {
         ATLASSERT(strandIdx+1 <= m_NumDebondSections);
         num_leading_sections = m_NumDebondSections-1 - strandIdx;
      }

      DebondLevelType db_level = GetMaxDebondLevel(ns, num_leading_sections);
      debond_levels.push_back(db_level);
   }

   LOG(_T("Debond levels before layout = ")<<DumpIntVector(debond_levels));

   // save our max levels
   m_MaxPhysicalDebondLevels = debond_levels;

   LOG(_T("Maximum Debond Layout = ")<<DumpIntVector(m_MaxPhysicalDebondLevels));
   LOG(_T("Exiting MaximizeDebonding"));
   return true;
}

Float64 pgsStrandDesignTool::GetDebondSectionLength() const
{
   return m_DebondSectionLength;
}

SectionIndexType pgsStrandDesignTool::GetMaxNumberOfDebondSections() const
{
   return m_NumDebondSections;
}

Float64 pgsStrandDesignTool::GetDebondSectionLocation(SectionIndexType sectionIdx, DebondEndType end) const
{
   ATLASSERT(sectionIdx>=0 && sectionIdx<m_NumDebondSections);

   if (end == dbLeft)
   {
      return m_DebondSectionLength*(sectionIdx);
   }
   else
   {
      return m_SegmentLength - m_DebondSectionLength*(sectionIdx);
   }
}

void pgsStrandDesignTool::GetDebondSectionForLocation(Float64 location, SectionIndexType* pOutBoardSectionIdx, SectionIndexType* pInBoardSectionIdx, Float64* pOutToInDistance) const
{
   Float64 max_deb_loc = (m_NumDebondSections-1) * m_DebondSectionLength;
   Float64 rgt_loc = m_SegmentLength - max_deb_loc;

   const Float64 fudge = 1.0e-5;
   // get distance from end
   if (rgt_loc < location+fudge)
   {
      // at right end
      location = m_SegmentLength - location;
   }

   if (0.0 <= location && location < max_deb_loc+fudge)
   {
      if (location <= fudge)
      {
         // cover the case where flooring an int below gets us into trouble
         *pInBoardSectionIdx  = 1;
         *pOutBoardSectionIdx = 0;
      }
      else
      {
         *pInBoardSectionIdx = SectionIndexType((location-fudge)/m_DebondSectionLength + 1);
         *pOutBoardSectionIdx = *pInBoardSectionIdx - 1;
      }
   }
   else
   {
      ATLASSERT(false); // asking for a location outside of debond zone
      *pInBoardSectionIdx  = INVALID_INDEX;
      *pOutBoardSectionIdx = INVALID_INDEX;
   }

   // distance location to outboard section
   *pOutToInDistance = location - (*pOutBoardSectionIdx) * m_DebondSectionLength;

   // save some later grief due to tolerancing
   if (IsEqual(*pOutToInDistance,m_DebondSectionLength, fudge))
   {
      *pOutToInDistance = m_DebondSectionLength;
   }
   else if (IsZero(*pOutToInDistance, fudge))
   {
      *pOutToInDistance = 0.0;
   }

   ATLASSERT(*pOutToInDistance >= 0.0);
   ATLASSERT(*pOutToInDistance <= m_DebondSectionLength);
}


DebondLevelType pgsStrandDesignTool::GetMaxDebondLevel(StrandIndexType numStrands, SectionIndexType numLeadingSections ) const
{
   ATLASSERT(0 <= numStrands);
   ATLASSERT(0 <= numLeadingSections);
//   LOG(_T("Entering GetMaxDebondLevel, numStrands = ")<<numStrands<<_T(" numLeadingSections = ")<<numLeadingSections);

   DebondLevelType num_levels = (DebondLevelType)m_DebondLevels.size();
   DebondLevelType level = 0;
   if ( 0 < numStrands && 0 < num_levels )
   {
      level = DebondLevelType(num_levels-1);
      // Find max level based on raw number of strands, and number of leading sections
      // Going backwards, as levels increase min number of strands
      for (auto dbit=m_DebondLevels.crbegin(); dbit!=m_DebondLevels.crend(); dbit++)
      {
         const DebondLevel& rlevel = *dbit;

         if (rlevel.MinTotalStrandsRequired <= numStrands)
         {
            // Have enough strands to work at this level, see if we have room section-wise
            StrandIndexType num_debonded = rlevel.StrandsDebonded.size();
            StrandIndexType max_debonds_at_section = Max(numStrands < 10 ? m_MaxDebondSection10orLess : m_MaxDebondSection, m_bCheckMaxFraAtSection ? StrandIndexType(num_debonded*m_MaxPercentDebondSection) : 0 );

            // allow int to floor
            SectionIndexType leading_sections_required = (num_debonded == 0 ? 0 : SectionIndexType((num_debonded-1)/max_debonds_at_section));
            if (leading_sections_required <= numLeadingSections)
            {
               break;
            }
         }

         level--;
      }
   }
   
//   LOG(_T("Exiting GetMaxDebondLevel, max level is = ")<<level);
   ATLASSERT(0 <= level);
   return level;
}

const std::vector<DebondLevelType>& pgsStrandDesignTool::GetMaxPhysicalDebonding() const
{
   return m_MaxPhysicalDebondLevels;
}

void pgsStrandDesignTool::RefineDebondLevels(std::vector<DebondLevelType>& rDebondLevelsAtSections) const
{
   SectionIndexType num_sects = GetMaxNumberOfDebondSections();
   LOG(_T("Entering RefineDebondLevels, max debond sections = ")<<num_sects);
   LOG(_T("List of raw levels at start ")<<DumpIntVector(rDebondLevelsAtSections));
   SectionIndexType test_size = rDebondLevelsAtSections.size();

   if (test_size != num_sects)
   {
      // should probably never happen
      ATLASSERT(false);
      rDebondLevelsAtSections.resize(num_sects, 0);
   }

   if (0 < test_size)
   {

      // levels must increase as we approach the end of beam, make this so, if it is not
      SortDebondLevels(rDebondLevelsAtSections);

      // make sure debond termination rules are enforced
      if ( !SmoothDebondLevelsAtSections(rDebondLevelsAtSections) )
      {
         LOG(_T("Unable to debond within section rules")); //  could we increase conc strength here?
         rDebondLevelsAtSections.clear();
         return;
      }

      LOG(_T("List of levels after smoothing     ")<<DumpIntVector(rDebondLevelsAtSections));
      LOG(_T("List of max physical debond levels ")<<DumpIntVector(m_MaxPhysicalDebondLevels));

      // check levels against physical max computed in MaximizeDebonding
      ATLASSERT(m_MaxPhysicalDebondLevels.size()==num_sects);
      auto mit = m_MaxPhysicalDebondLevels.begin();
      SectionIndexType sectno = 0;
      for(auto it = rDebondLevelsAtSections.begin(); it != rDebondLevelsAtSections.end(); it++)
      {
         DebondLevelType debond_level_at_section = *it;
         DebondLevelType max_debond_level = *mit;

         if (max_debond_level < debond_level_at_section)
         {
            ATLASSERT(false); // remove this after testing
            LOG(_T(" A debond level exceeds the maximum physical allowable at section ")<<sectno<<_T(" Design abort"));
            rDebondLevelsAtSections.clear();
            break;
         }

         mit++;
         sectno++;
      }
   }
   else
   {
      ATLASSERT(false); // should probably always of at least one debond section by the time we get here
   }

   LOG(_T("Exiting RefineDebondLevels"));
}

bool pgsStrandDesignTool::SmoothDebondLevelsAtSections(std::vector<DebondLevelType>& rDebondLevelsAtSections) const
{
   // make sure we abide to max bond terminations at a section
   // Get number of debonded strands (max level will be at end). Assuming that levels have been sorted
   DebondLevelType debond_level_at_girder_end = rDebondLevelsAtSections[0];
   StrandIndexType num_debonded = m_DebondLevels[debond_level_at_girder_end].StrandsDebonded.size();

   StrandIndexType numStrands = GetNumPermanentStrands();
   StrandIndexType max_db_term_at_section = Max(numStrands < 10 ? m_MaxDebondSection10orLess : m_MaxDebondSection, m_bCheckMaxFraAtSection ? StrandIndexType(num_debonded*m_MaxPercentDebondSection) : 0);
   LOG(_T("Max allowable debond terminations at a section = ")<<max_db_term_at_section);

   // iterate from mid-girder toward end
   std::vector<DebondLevelType>::reverse_iterator rit = rDebondLevelsAtSections.rbegin();
   StrandIndexType last_num_db=0;
   while( rit != rDebondLevelsAtSections.rend())
   {
      DebondLevelType debond_level = *rit;
      ATLASSERT(0 <= debond_level && debond_level < (DebondLevelType)m_DebondLevels.size());

      StrandIndexType num_db_at_lvl = m_DebondLevels[debond_level].StrandsDebonded.size();

      StrandIndexType num_db_term = num_db_at_lvl - last_num_db; // num debonds terminated at this section

      if (max_db_term_at_section < num_db_term)
      {
         // too many strands terminated here, see if we can debond upstream
         if ( rit != rDebondLevelsAtSections.rbegin() )
         {
            // What minimum debond level can fix out problem at the adjacent (inward) section
            DebondLevelType min_adjacent_level = GetMinAdjacentDebondLevel(debond_level, max_db_term_at_section); 

            std::vector<DebondLevelType>::reverse_iterator rit2 = rit;
            rit2--;
            DebondLevelType& rlvl_prv = *rit2;

            if (rlvl_prv < min_adjacent_level)
            {
               // set previous section's level and restart loop at previous location
               rlvl_prv = min_adjacent_level;

               rit = rit2;

               if (rit2 == rDebondLevelsAtSections.rbegin())
               {
                  last_num_db = 0;
               }
               else
               {
                  rit2--;
                  DebondLevelType lvl2 = *rit2;
                  last_num_db = m_DebondLevels[lvl2].StrandsDebonded.size();
               }
            }
            else
            {
               ATLASSERT(false); // This means there is a bug in the initial debond level computation. 
                             // the outermost if() should not fail if we have laid out levels properly
            }
         }
         else
         {
            // We have too much debonding at the mid-girdermost section
            // nothing we can do
            return false;
         }
      }
      else
      {
         last_num_db = num_db_at_lvl;
         rit++;
      }
   }

   return true;
}

DebondLevelType pgsStrandDesignTool::GetMinAdjacentDebondLevel(DebondLevelType currLevel, StrandIndexType maxDbsTermAtSection) const
{
   // We are at a certain debond level that exceeds section debonding limits. See what 
   // level toward mid-span can get us within limits
   ATLASSERT(0 < currLevel && currLevel < (DebondLevelType)m_DebondLevels.size());

   const DebondLevel& rcurr_lvl = m_DebondLevels[currLevel];
   StrandIndexType num_db_at_curr_level = rcurr_lvl.StrandsDebonded.size();
   
   ATLASSERT(maxDbsTermAtSection <= num_db_at_curr_level); // can't terminate more strands than we have at this level. calling routine on drugs.

   // Go after minimum level that has enough strands to alleviate our problem
   DebondLevelType min_lvl = -1;
   for (DebondLevelType levelIdx = 0; levelIdx < currLevel; levelIdx++)
   {
      const DebondLevel& rlvl = m_DebondLevels[levelIdx];
      StrandIndexType num_db = rlvl.StrandsDebonded.size();

      if ( (num_db_at_curr_level-num_db) <= maxDbsTermAtSection )
      {
         min_lvl = levelIdx;
         break;
      }
   }

   if (min_lvl == -1)
   {
      ATLASSERT(false); // something messed up with initial determination of debond levels
      min_lvl = 0;
   }

   return min_lvl;
}

bool pgsStrandDesignTool::LayoutDebonding(const std::vector<DebondLevelType>& rDebondLevelAtSections)
{
   LOG(_T("Entering LayoutDebonding. Debond Levels = ")<<DumpIntVector(rDebondLevelAtSections));
   DebondConfigCollection db_info;
   SectionIndexType num_sections_where_strands_are_debonded = rDebondLevelAtSections.size();
   if (0 < num_sections_where_strands_are_debonded)
   {
      ATLASSERT(num_sections_where_strands_are_debonded <= m_NumDebondSections);

#ifdef _DEBUG
      // check that debond levels decrease inward
      std::vector<DebondLevelType>::const_iterator it=rDebondLevelAtSections.begin();
      DebondLevelType prev_debond_level = *it;
      while ( it != rDebondLevelAtSections.end() )
      {
         DebondLevelType next_debond_level = *it;
         ATLASSERT(next_debond_level <= prev_debond_level);

         it++;
         prev_debond_level = next_debond_level;
      }
#endif

      // work from inside outward
      SectionIndexType debondSectionIndex = num_sections_where_strands_are_debonded-1;
      StrandIndexType last_num_of_debonded_strands = 0;
      for(std::vector<DebondLevelType>::const_reverse_iterator rit=rDebondLevelAtSections.rbegin(); rit!=rDebondLevelAtSections.rend(); rit++)
      {
         DebondLevelType lvl = *rit;

         const DebondLevel& rlvl = m_DebondLevels[lvl];

         StrandIndexType num_debonded_strands_this_section = rlvl.StrandsDebonded.size();

         if (last_num_of_debonded_strands < num_debonded_strands_this_section)
         {
            // we have strands to debond at this section
            Float64 debond_location_from_left_end= GetDebondSectionLocation(debondSectionIndex, dbLeft);

            LOG(_T("Debond required at section ")<<debondSectionIndex<<_T("at ")<<WBFL::Units::ConvertFromSysUnits(debond_location_from_left_end,WBFL::Units::Measure::Feet) << _T(" ft"));

            for (StrandIndexType debondedStrandIdx = last_num_of_debonded_strands; debondedStrandIdx < num_debonded_strands_this_section; debondedStrandIdx++)
            {
               StrandIndexType strandIndex = rlvl.StrandsDebonded[debondedStrandIdx];

               DEBONDCONFIG debondInfo;
               debondInfo.strandIdx = strandIndex;
               debondInfo.DebondLength[pgsTypes::metStart] = debond_location_from_left_end;
               debondInfo.DebondLength[pgsTypes::metEnd]   = debond_location_from_left_end;

               LOG(_T("   Debond ") << strandIndex);

               db_info.push_back(debondInfo);
            }
         }

         debondSectionIndex--;
         last_num_of_debonded_strands = num_debonded_strands_this_section;
      }
   }

   // set to our artifact
   m_pArtifact->SetStraightStrandDebondInfo(db_info);
   m_bConfigDirty = true; // cache is dirty

   LOG(_T("Exiting LayoutDebonding"));

   return true;
}

// Losses differences can cause debond to fail - fudge by:
// The fudge factors will cause a slight amount of over-debonding
static Float64 TensDebondFudge  = 1.0;
static Float64 ComprDebondFudge = 1.0; // fudge compression more because it's easier to get more compression strength

Float64 pgsStrandDesignTool::ComputePrestressForcePerStrand(const GDRCONFIG& fullyBondedConfig, const StressDemand& demand, const DebondLevel& lvl, IntervalIndexType interval, IPretensionForce* pPrestressForce) const
{
   // Create a config so we can get prestress force at debonded section
   GDRCONFIG config = fullyBondedConfig;
   Float64 dblength = demand.m_Poi.GetDistFromStart();
   for (const StrandIndexType& sindex : lvl.StrandsDebonded)
   {
      DEBONDCONFIG dbconfig;
      dbconfig.DebondLength[pgsTypes::metStart] = dblength;
      dbconfig.DebondLength[pgsTypes::metEnd] = dblength;
      dbconfig.strandIdx = sindex;

      config.PrestressConfig.Debond[pgsTypes::Straight].push_back(dbconfig);
   }

   Float64 debonded_strand_force = pPrestressForce->GetPrestressForcePerStrand(demand.m_Poi, pgsTypes::Permanent, interval, pgsTypes::End, &config );

   return debonded_strand_force;
}

void pgsStrandDesignTool::GetDebondLevelForTopTension(const StressDemand& demand, const GDRCONFIG& fullyBondedConfig, Float64 cgFullyBonded, IntervalIndexType interval, Float64 tensDemand, Float64 outboardDistance,
                                                      Float64 Hg, Float64 Yb, Float64 eccX, Float64 Ca, Float64 Cmx, Float64 Cmy,
                                                      DebondLevelType* pOutboardLevel, DebondLevelType* pInboardLevel) const
{
   ATLASSERT(outboardDistance<=m_DebondSectionLength);

   if ( 0 < tensDemand )
   {
      GET_IFACE(IPretensionForce,pPrestressForce);

      StrandIndexType nperm = fullyBondedConfig.PrestressConfig.GetStrandCount(pgsTypes::Permanent);
      StrandIndexType ntemp = fullyBondedConfig.PrestressConfig.GetStrandCount(pgsTypes::Temporary);

      // First determine minimum level required to alleviate demand
      Uint16 level=0;
      bool found = false;
      auto it = m_DebondLevels.begin();
      while(true)
      {
         level++;
         it++;
         if ( it != m_DebondLevels.end() )
         {
            const DebondLevel& lvl = *it;

           // can only attain level with min number of strands
           if (lvl.MinTotalStrandsRequired <= nperm)
           {
               Float64 debonded_strand_force = ComputePrestressForcePerStrand(fullyBondedConfig, demand, lvl, interval, pPrestressForce);

               LOG(_T("GetDebondLevelForTopTension: level  = ")<<level<<_T(", interval = ")<<interval<<_T(", strand force =") << WBFL::Units::ConvertFromSysUnits(debonded_strand_force,WBFL::Units::Measure::Kip) << _T(" kip"));

               // stress relief for lvl
               Float64 stress =  lvl.ComputeReliefStress(demand.m_PrestressForcePerStrand, debonded_strand_force, nperm, ntemp, cgFullyBonded, Hg, Yb, eccX, Ca, Cmx, Cmy, LOGGER);

              // if this makes it by fudge, call it good enough
              if ( tensDemand*TensDebondFudge < stress)
              {
                 found = true;
                 break;
              }
           }
         }
         else
         {
            // no level that alleviates the demand
            break;
         }
      }

      if (found)
      {
         // We have a level that works, and we know that it will alleviate stress if we put it inboard of our location.
         *pOutboardLevel = level;
         *pInboardLevel  = level;

         // But, let's try a little harder to see if the transfer effect allows less debonding
         if (outboardDistance <= 1.0e-5)
         {
            // location is so close to outboard section - call it fully transferred so we don't need inboard debonding
            *pInboardLevel = 0;
         }
         else if (outboardDistance < GetTransferLength(pgsTypes::Permanent))
         {
            // Location is between outboard section and transfer length. See if we can use next lower
            // level at inboard section location.
            // Note that we might try to improve this later by trying even less debonding inboard, but things are complicated enough as is.
            Float64 debonded_strand_force_l = ComputePrestressForcePerStrand(fullyBondedConfig, demand, m_DebondLevels[level], interval, pPrestressForce);

            Float64 outb_allev =  m_DebondLevels[level].ComputeReliefStress(demand.m_PrestressForcePerStrand, debonded_strand_force_l, nperm, ntemp, cgFullyBonded, Hg, Yb, eccX, Ca, Cmx, Cmy, LOGGER); // no fudge

            if (tensDemand < outb_allev)
            {
               // We might be below transfer line
               // Stress relief (alleviation) provided by next-lower level
               Float64 debonded_strand_force_l1 = ComputePrestressForcePerStrand(fullyBondedConfig, demand, m_DebondLevels[level], interval, pPrestressForce);

               Float64 inb_allev = m_DebondLevels[level-1].ComputeReliefStress(demand.m_PrestressForcePerStrand, debonded_strand_force_l1, nperm, ntemp, cgFullyBonded, Hg, Yb, eccX, Ca, Cmx, Cmy, LOGGER);

               Float64 transfer_provided = 1.0-(outboardDistance / GetTransferLength(pgsTypes::Permanent));
            
               Float64 allev_provided = inb_allev + transfer_provided*(outb_allev-inb_allev);

               if (tensDemand*TensDebondFudge < allev_provided )
               {
                  // we can use next-lower level at inboard section
                  *pInboardLevel = level-1;
               }
            }
         }
      }
      else
      {
         // no levels can alleviate demand - return negative of max
         *pOutboardLevel = -1 * ((DebondLevelType)m_DebondLevels.size()-1);
         *pInboardLevel  = *pOutboardLevel;
      }
   }
   else
   {
      ATLASSERT(false);
      *pOutboardLevel = -1;
      *pInboardLevel  = -1;
   }
}

void pgsStrandDesignTool::GetDebondLevelForBottomCompression(const StressDemand& demand, const GDRCONFIG& fullyBondedConfig, Float64 cgFullyBonded, IntervalIndexType interval, Float64 compDemand, Float64 outboardDistance,
                                                             Float64 Hg, Float64 Yb, Float64 eccX, Float64 Ca, Float64 Cmx, Float64 Cmy,
                                                             DebondLevelType* pOutboardLevel, DebondLevelType* pInboardLevel) const
{
   ATLASSERT(outboardDistance<=m_DebondSectionLength);

   if ( compDemand < 0 )
   {
      GET_IFACE(IPretensionForce,pPrestressForce);

      StrandIndexType nperm = fullyBondedConfig.PrestressConfig.GetStrandCount(pgsTypes::Permanent);
      StrandIndexType ntemp = fullyBondedConfig.PrestressConfig.GetStrandCount(pgsTypes::Temporary);

      // First determine minimum level required to alleviate demand
      DebondLevelType level=0;
      bool found = false;
      auto it = m_DebondLevels.cbegin();
      while(true)
      {
         level++;
         it++;
         if ( it != m_DebondLevels.cend() )
         {
           const DebondLevel& lvl = *it;

           Float64 debonded_strand_force = ComputePrestressForcePerStrand(fullyBondedConfig, demand, lvl, interval, pPrestressForce);

           LOG(_T("GetDebondLevelForBottomCompression: level  = ")<<level<<_T(", interval = ")<<interval<<_T(", strand force =") << WBFL::Units::ConvertFromSysUnits(debonded_strand_force,WBFL::Units::Measure::Kip) << _T(" kip"));

           // can only attain level with min number of strands
           if (lvl.MinTotalStrandsRequired <= nperm)
           {
               // stress relief for lvl
               Float64 stress =  lvl.ComputeReliefStress(demand.m_PrestressForcePerStrand, debonded_strand_force, nperm, ntemp, cgFullyBonded, Hg, Yb, eccX, Ca, Cmx, Cmy, LOGGER);

              // if this makes it by fudge, call it good enough
              if (stress < (compDemand * ComprDebondFudge))
              {
                 found = true;
                 break;
              }
           }
         }
         else
         {
            // no level that alleviates the demand
            break;
         }
      }

      if (found)
      {
         // We have a level that works, and we know that it will alleviate stress if we put it inboard of our location.
         *pOutboardLevel = level;
         *pInboardLevel  = level;

         // But, let's try a little harder to see if the transfer effect allows less debonding
         if (outboardDistance <= 1.0e-5)
         {
            // location is so close to outboard section - call it fully transferred so we don't need inboard debonding
            *pInboardLevel = 0;
         }
         else if (outboardDistance < GetTransferLength(pgsTypes::Permanent))
         {
            // Location is between outboard section and transfer length. See if we can use next lower
            // level at inboard section location.
            // Note that we might try to improve this later by trying even less debonding inboard, but things are complicated enough as is.

            Float64 debonded_strand_force_l = ComputePrestressForcePerStrand(fullyBondedConfig, demand, m_DebondLevels[level], interval, pPrestressForce);

            Float64 outb_allev =  m_DebondLevels[level].ComputeReliefStress(demand.m_PrestressForcePerStrand, debonded_strand_force_l, nperm, ntemp, cgFullyBonded, Hg, Yb, eccX, Ca, Cmx, Cmy, LOGGER); // no fudge

            if (outb_allev < compDemand)
            {
               // We might be below transfer line
               // Stress relief (alleviation) provided by next-lower level
               Float64 debonded_strand_force_l2 = ComputePrestressForcePerStrand(fullyBondedConfig, demand, m_DebondLevels[level-1], interval, pPrestressForce);

               Float64 inb_allev = m_DebondLevels[level-1].ComputeReliefStress(demand.m_PrestressForcePerStrand, debonded_strand_force_l2, nperm, ntemp, cgFullyBonded, Hg, Yb, eccX, Ca, Cmx, Cmy, LOGGER);

               Float64 transfer_provided = 1.0-(outboardDistance / GetTransferLength(pgsTypes::Permanent));
            
               Float64 allev_provided = inb_allev + transfer_provided*(outb_allev-inb_allev);

               if (allev_provided < (compDemand*ComprDebondFudge) )
               {
                  // we can use next-lower level at inboard section
                  *pInboardLevel = level-1;
               }
            }
         }
      }
      else
      {
         // no levels can alleviate demand - return negative of max
         *pOutboardLevel = -1 * ((DebondLevelType)m_DebondLevels.size()-1);
         *pInboardLevel  = *pOutboardLevel;
      }
   }
   else
   {
      ATLASSERT(false);
      *pOutboardLevel = -1;
      *pInboardLevel  = -1;
   }
}

std::vector<DebondLevelType> pgsStrandDesignTool::ComputeDebondsForDemand(const std::vector<StressDemand>& demands, const GDRCONFIG& fullyBondedConfig, Float64 cgFullyBonded,
                                                                          IntervalIndexType interval, Float64 allowTens, Float64 allowComp) const
{
   GET_IFACE(ISectionProperties,pSectProp);
   GET_IFACE(IStrandGeometry,pStrandGeom);

   std::vector<DebondLevelType> debond_levels;
   SectionIndexType max_db_sections = GetMaxNumberOfDebondSections();

   // set up our vector to return debond levels at each section
   debond_levels.assign(max_db_sections,0);

   for (auto sit=demands.cbegin(); sit!=demands.cend(); sit++)
   {
      const StressDemand& demand = *sit;

      LOG(_T("Debonding design for stresses at ")<<WBFL::Units::ConvertFromSysUnits(demand.m_Poi.GetDistFromStart(),WBFL::Units::Measure::Feet) << _T(" ft"));
      LOG(_T("Applied Top stress = ")<< WBFL::Units::ConvertFromSysUnits(demand.m_TopStress,WBFL::Units::Measure::KSI) << _T(" ksi, Bottom stress = ") << WBFL::Units::ConvertFromSysUnits(demand.m_BottomStress,WBFL::Units::Measure::KSI) << _T(" ksi."));

      // Section properties of beam - non-prismatic
      // using release interval because we are computing stress on the girder due to prestress which happens in this interval
      Float64 Yb = pSectProp->GetY(interval,demand.m_Poi,pgsTypes::BottomGirder);
      Float64 Hg = pSectProp->GetHg(interval, demand.m_Poi);

      // Need X eccentricity for My biaxial bending. This value will not change because the design algo can only define strand layouts that are symmetric about Y
      auto ecc = pStrandGeom->GetEccentricity(interval, demand.m_Poi, false, &fullyBondedConfig);

      if (allowTens < demand.m_TopStress || demand.m_BottomStress < allowComp)
      {
         // get debond increment this poi is just outside of
         SectionIndexType outboard_inc, inboard_inc;
         Float64 out_to_in_distance;
         GetDebondSectionForLocation(demand.m_Poi.GetDistFromStart(), &outboard_inc, &inboard_inc, &out_to_in_distance);
         ATLASSERT(0 <= outboard_inc && outboard_inc < max_db_sections);
         ATLASSERT(0 <= inboard_inc  && inboard_inc  < max_db_sections);

         // compute debond level required to alleviate stresses at top
         DebondLevelType out_db_level(0), in_db_level(0);
         if (allowTens < demand.m_TopStress)
         {
            // debonding needs to reduce stress by this much
            Float64 tens_demand = demand.m_TopStress - allowTens;

            // Get stress coefficients for top stress
            Float64 Cat, Ctx, Cty;
            pSectProp->GetStressCoefficients(interval, demand.m_Poi, pgsTypes::TopGirder, &fullyBondedConfig, &Cat, &Ctx, &Cty);

            DebondLevelType out_top_db_level, in_top_db_level;
            GetDebondLevelForTopTension(demand, fullyBondedConfig, cgFullyBonded, interval, tens_demand, out_to_in_distance, Hg, Yb, ecc.X(), Cat, Ctx, Cty, &out_top_db_level, &in_top_db_level);

            LOG(_T("Debonding needed to control top tensile overstress of ") << WBFL::Units::ConvertFromSysUnits(tens_demand,WBFL::Units::Measure::KSI) << _T(" KSI at ")<<WBFL::Units::ConvertFromSysUnits(demand.m_Poi.GetDistFromStart(),WBFL::Units::Measure::Feet) << _T(" ft. Outboard level required was ")<< out_top_db_level<<_T(" Inboard level required was ")<< in_top_db_level);

            if (out_top_db_level < 0)
            {
               ATLASSERT(false); // algorithm should avoid this
               LOG(_T("Debond design failed at location - continue, but failure is likely"));
               out_top_db_level *= -1;
            }

            if (in_top_db_level < 0)
            {
               ATLASSERT(false); // algorithm should avoid this
               LOG(_T("Debond design failed at location - continue, but failure is likely"));
               in_top_db_level *= -1;
            }

            out_db_level = Max(out_db_level, out_top_db_level);
            in_db_level  = Max(in_db_level,  in_top_db_level);
         }

         if (demand.m_BottomStress < allowComp)
         {
            // debonding needs to reduce stress by this much
            Float64 comp_demand = demand.m_BottomStress - allowComp;

            // Get stress coefficients for bottom stress
            Float64 Cab, Cbx, Cby;
            pSectProp->GetStressCoefficients(interval, demand.m_Poi, pgsTypes::BottomGirder, &fullyBondedConfig, &Cab, &Cbx, &Cby);

            DebondLevelType out_bot_db_level, in_bot_db_level;
            GetDebondLevelForBottomCompression(demand, fullyBondedConfig, cgFullyBonded, interval, comp_demand, out_to_in_distance, Hg, Yb, ecc.X(), Cab, Cbx, Cby, &out_bot_db_level, &in_bot_db_level);

            LOG(_T("Debonding needed to control bottom compressive overstress of ") << WBFL::Units::ConvertFromSysUnits(comp_demand,WBFL::Units::Measure::KSI) << _T(" KSI at ")<<WBFL::Units::ConvertFromSysUnits(demand.m_Poi.GetDistFromStart(),WBFL::Units::Measure::Feet) << _T(" ft. Outboard level required was ")<< out_bot_db_level<<_T(" Inboard level required was ")<< in_bot_db_level);

            if (out_bot_db_level < 0)
            {
               ATLASSERT(false); // algorithm should avoid this
               LOG(_T("Debond design failed at location - continue, but failure is likely"));
               out_bot_db_level *= -1;
            }

            if (in_bot_db_level < 0)
            {
               ATLASSERT(false); // algorithm should avoid this
               LOG(_T("Debond design failed at location - continue, but failure is likely"));
               in_bot_db_level *= -1;
            }

            out_db_level = Max(out_db_level, out_bot_db_level);
            in_db_level  = Max(in_db_level,  in_bot_db_level);

         }

         // replace level at current location only if new is larger
         auto curr_lvl = debond_levels[outboard_inc];
         if (curr_lvl < out_db_level)
         {
            debond_levels[outboard_inc] = out_db_level;
         }

         curr_lvl = debond_levels[inboard_inc];
         if (curr_lvl < in_db_level)
         {
            debond_levels[inboard_inc] = in_db_level;
         }
      }
   }

   LOG(_T("Smooth out our vector before we return it"));
   RefineDebondLevels(debond_levels);

   return debond_levels;
}