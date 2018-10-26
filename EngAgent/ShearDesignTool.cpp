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

#include "StdAfx.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\Allowables.h>
#include <IFace\GirderHandlingPointOfInterest.h>
#include <IFace\TransverseReinforcementSpec.h>
#include <IFace\ShearCapacity.h>
#include <IFace\InterfaceShearRequirements.h>

#include <EAF\EAFDisplayUnits.h>

#include <PgsExt\BridgeDescription.h>
#include <Math\PwLinearFunction2dUsingPoints.h>
#include <psgLib\SpecLibraryEntry.h>
#include <MathEx.h>
#include <Lrfd\Rebar.h>

#include "ShearDesignTool.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Tolerance for stirrup spacing to fit withing min/max allowable spacings
static const Float64 SPACING_TOL=1.0e-5;

// StirrupZoneIter
//////////////////
// Very specialized class for walking stirrup zones. This class attempts to solve the 
// problem of walking from left-right and right-left down a collection of shear zones 
// toward the middle of the girder
// It uses the Iterator pattern as presented in the GOF Patterns book.
// Data returned at an iteration location
struct StirrupZoneItem
{
   Float64 startLocation; // start/end changes whether we are iterating left-right, or right-left
   Float64 endLocation;
   Float64 ConfinementTestLoc; // location to test if we're in confinement zone
   Float64 leftEnd;
   Float64 rightEnd;

   CShearZoneData* pShearZoneData;
};

class StirrupZoneIter
{
public:
   virtual void First()=0;
   virtual void Next()=0;
   virtual bool IsDone()=0;
   virtual StirrupZoneItem CurrentItem()=0;
};

// Iterate from left to girder centerline
class LeftRightStirrupZoneIter : public StirrupZoneIter
{
public:
   LeftRightStirrupZoneIter(CShearData::ShearZoneVec& rZoneVec, Float64 girderLength):
   m_ZoneVec(rZoneVec),
   m_GirderLength2(girderLength/2.0)
   {

   }

   void First()
   {
      m_Iter = m_ZoneVec.begin();
      m_ZoneStart = 0.0;

      m_bDone = m_Iter == m_ZoneVec.end();
   }

   void Next()
   {
      ATLASSERT(!m_bDone);
      Float64 zl = m_Iter->ZoneLength;
      m_ZoneStart += zl;

      if (m_ZoneStart >= m_GirderLength2)
      {
         m_bDone = true;
      }
      else
      {
         m_Iter++;
         if (m_Iter == m_ZoneVec.end())
         {
            m_bDone = true;
         }
      }
   }

   bool IsDone()
   {
      return m_bDone;
   }

   StirrupZoneItem CurrentItem()
   {
      ATLASSERT(!m_bDone);
      Float64 zl = m_Iter->ZoneLength;

      if (m_ZoneStart+zl >= m_GirderLength2)
      {
         // This is last zone because it extends beyond half-girder length
         zl = m_GirderLength2 - m_ZoneStart;
      }
      else
      {
         // Check if we are at end of collection
         CShearData::ShearZoneIterator iter(m_Iter);
         iter++;
         if (iter == m_ZoneVec.end())
         {
            zl = m_GirderLength2 - m_ZoneStart; // we are at end of collection - extend last zone to gl/2
         }
      }

      // fill up our item
      StirrupZoneItem item;
      item.startLocation      = m_ZoneStart;
      item.endLocation        = m_ZoneStart + zl;
      item.ConfinementTestLoc = m_ZoneStart + SPACING_TOL;
      item.leftEnd            = item.startLocation;
      item.rightEnd           = item.endLocation;

      item.pShearZoneData = &(*m_Iter);

      return item;
   }

private:
   CShearData::ShearZoneVec& m_ZoneVec;
   Float64 m_GirderLength2;

   Float64 m_ZoneStart;
   CShearData::ShearZoneIterator m_Iter;
   bool m_bDone;
};

// Iterate from right end to girder centerline
class RightLeftStirrupZoneIter : public StirrupZoneIter
{
public:
   RightLeftStirrupZoneIter(CShearData::ShearZoneVec& rZoneVec, Float64 girderLength):
   m_ZoneVec(rZoneVec),
   m_GirderLength(girderLength)
   {

   }

   void First()
   {
      m_StirrupZoneItems.clear();

      // Build list of StirrupZoneItem's along right half of girder. We will reverse iterate along this
      Float64 gl2 = m_GirderLength/2.0;
      Float64 zone_start(0.0);
      ZoneIndexType nzones = m_ZoneVec.size();
      bool quit(false);
      bool is_first(true); // haven't created first zone yet
      for(ZoneIndexType iz=0; iz<nzones; iz++)
      {
         CShearZoneData& rdata = m_ZoneVec[iz];

         Float64 zone_end = zone_start + rdata.ZoneLength;

         if(zone_end >= m_GirderLength || iz==nzones-1)
         {
            zone_end = m_GirderLength;
            quit = true;
         }

         // only want zones in right half
         if (zone_end > gl2)
         {
            if (is_first)
            {
               // First zone always starts at half-girder
               zone_start = gl2;
               is_first = false;
            }

            StirrupZoneItem item;
            item.startLocation      = zone_end; // we are going right-left
            item.endLocation        = zone_start;
            item.ConfinementTestLoc = zone_end - SPACING_TOL;
            item.leftEnd            = zone_start;
            item.rightEnd           = zone_end;

            item.pShearZoneData = &rdata;

            m_StirrupZoneItems.push_back(item);
         }

         if(quit)
         {
            break;
         }

         zone_start = zone_end;
      }

      m_rIter = m_StirrupZoneItems.rbegin();
   }

   void Next()
   {
      ATLASSERT(!IsDone());
      m_rIter++;
   }

   bool IsDone()
   {
      return m_rIter == m_StirrupZoneItems.rend();
   }

   StirrupZoneItem CurrentItem()
   {
      ATLASSERT(!IsDone());
      const StirrupZoneItem& ritem = *m_rIter;
      return ritem;
   }

private:
   CShearData::ShearZoneVec& m_ZoneVec;
   Float64 m_GirderLength;

   std::vector<StirrupZoneItem> m_StirrupZoneItems;
   std::vector<StirrupZoneItem>::reverse_iterator m_rIter;
};


/****************************************************************************
CLASS
   pgsShearDesignTool
****************************************************************************/
//////////////////////
pgsShearDesignTool::pgsShearDesignTool(SHARED_LOGFILE lf):
LOGFILE(lf),
m_pArtifact(NULL),
m_pBroker(NULL),
m_StatusGroupID(NULL)
{
}

void pgsShearDesignTool::Initialize(IBroker* pBroker, LongReinfShearChecker* pLongShearChecker,
                                    StatusGroupIDType statusGroupID, pgsDesignArtifact* pArtif, 
                                    Float64 startConfinementZl, Float64 endConfinementZl,
                                    bool bPermit, bool bDesignFromScratch)
{
   ATLASSERT(pBroker);

   // Cache a bunch of stuff that does not change during design
   m_pBroker = pBroker;
   m_pLongReinfShearChecker = pLongShearChecker;
   m_StatusGroupID = statusGroupID;
   m_pArtifact = pArtif;

   m_bIsPermit = bPermit;
   m_bDoDesignFromScratch = bDesignFromScratch;

   // set confinement zone lengths even if we don't need them
   m_StartConfinementZl = startConfinementZl;
   m_EndConfinementZl = endConfinementZl;

   m_LeftCSS  = 0.0;
   m_RightCSS = 0.0;

   m_Span = m_pArtifact->GetSpan();
   m_Girder = m_pArtifact->GetGirder();

   GET_IFACE(IBridge,pBridge);
   m_GirderLength          = pBridge->GetGirderLength(m_Span,m_Girder);
   m_SpanLength            = pBridge->GetSpanLength(m_Span,m_Girder);
   m_StartConnectionLength = pBridge->GetGirderStartConnectionLength(m_Span,m_Girder);
   m_EndConnectionLength   = pBridge->GetGirderEndConnectionLength(m_Span,m_Girder);

   m_LeftFosLocation  = m_StartConnectionLength + pBridge->GetGirderStartSupportWidth(m_Span,m_Girder)/2.0;
   m_RightFosLocation = m_GirderLength - m_EndConnectionLength - pBridge->GetGirderEndSupportWidth(m_Span,m_Girder)/2.0;

   m_LongReinfShearAs = 0.0;

   m_RequiredFcForShearStress = 0.0;

   GET_IFACE(IStirrupGeometry,pStirrupGeometry);
   m_bIsCurrentStirrupLayoutSymmetrical = pStirrupGeometry->AreStirrupZonesSymmetrical(m_Span,m_Girder);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   GET_IFACE(IBridgeMaterial,pMaterial);

   matRebar::Grade barGrade;
   matRebar::Type barType;
   pMaterial->GetTransverseRebarMaterial(m_Span,m_Girder,barType,barGrade);

   lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
   ATLASSERT(pool != NULL);

   // Shear Design Control items
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(m_Span);
   const GirderLibraryEntry* pGirderEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(m_Girder);

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   m_bDoDesignForConfinement = pSpecEntry->IsConfinementDesignEnabled();
   m_bDoDesignForSplitting   = pSpecEntry->IsSplittingDesignEnabled();

   m_BarLegCollection.clear();

   // Precompute bar areas for all possible bar combinations
   IndexType nb = pGirderEntry->GetNumStirrupSizeBarCombos();
   for(IndexType ib=0; ib<nb; ib++)
   {
      BarLegCombo cbo;
      pGirderEntry->GetStirrupSizeBarCombo(ib, &(cbo.m_Size), &(cbo.m_Legs));

      const matRebar* pRebar = pool->GetRebar(barType,barGrade,cbo.m_Size);

      cbo.m_Av = pRebar->GetNominalArea() * cbo.m_Legs;

      m_BarLegCollection.push_back(cbo);
   }

   m_AvailableBarSpacings.clear();

   IndexType ns = pGirderEntry->GetNumAvailableBarSpacings();
   for(IndexType is=0; is<ns; is++)
   {
      Float64 spac = pGirderEntry->GetAvailableBarSpacing(is);
      m_AvailableBarSpacings.push_back(spac);
   }

   m_MaxSpacingChangeInZone = pGirderEntry->GetMaxSpacingChangeInZone();
   m_MaxShearCapacityChangeInZone = pGirderEntry->GetMaxShearCapacityChangeInZone();
   pGirderEntry->GetMinZoneLength(&m_MinZoneLengthSpacings, &m_MinZoneLengthLength);
   m_IsCompositeDeck = pBridge->IsCompositeDeck();
   m_IsTopFlangeRoughened = pGirderEntry->GetShearData().bIsRoughenedSurface;
   m_DoExtendBarsIntoDeck = m_IsCompositeDeck ? pGirderEntry->GetExtendBarsIntoDeck() : false;
   m_DoBarsProvideSplittingCapacity = pGirderEntry->GetShearData().bUsePrimaryForSplitting;
   m_DoBarsActAsConfinement = pGirderEntry->GetBarsActAsConfinement();

   m_LongShearCapacityIncreaseMethod = pGirderEntry->GetLongShearCapacityIncreaseMethod();
   m_bIsLongShearCapacityIncreaseMethodProblem = m_LongShearCapacityIncreaseMethod==GirderLibraryEntry::isAddingRebar &&
                                                 !pSpecEntry->IncludeRebarForShear();

   // Compute maximum possible bar spacing for design
   GET_IFACE(ITransverseReinforcementSpec,pTransverseReinforcementSpec);
   Float64 S_over;
#pragma Reminder("BUG: need to get the real dv value")
   // The original code for this tool has a bug in it. It only got the max spacing values
   // without looking at the limitations on the max spacing values. The limitations are
   // in LRFD Equations 5.8.2.7-1 and -2. The m_SMaxMax cannot be more than 0.8dv 
   // and S_over cannot be more than 0.4dv.
   // When updating for LRFD 7th Edition, 2014, the GetMaxStirrupSpacing function was
   // updated to take in the dv parameter and consider the limiting spacing values.
   // Since this method never look at the limitations before, we are going to assume
   // that it is still ok.
   // To fix this bug, we need to get dv. However, which dv? The one for the current girder or
   // the one based on the flexural design?
   Float64 dv = 99999;
   pTransverseReinforcementSpec->GetMaxStirrupSpacing(dv,&m_SMaxMax, &S_over); // includes user-input limit
   m_SMaxMax = min( m_SMaxMax, m_AvailableBarSpacings.back() );

   // Compute splitting zone lengths if we need them
   if (m_bDoDesignForSplitting)
   {
      GET_IFACE(IGirder,pGdr);
      GET_IFACE(IPointOfInterest,pPOI);

      // Splitting is computed and end transfer locations
      std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(m_Span,m_Girder,pgsTypes::CastingYard,POI_PSXFER) );
      ATLASSERT(vPOI.size() != 0);
      pgsPointOfInterest start_poi( vPOI.front() );
      pgsPointOfInterest end_poi( vPOI.back() );

      Float64 start_h = pGdr->GetSplittingZoneHeight( start_poi );
      Float64 end_h   = pGdr->GetSplittingZoneHeight( end_poi );

      m_StartSplittingZl = pTransverseReinforcementSpec->GetSplittingZoneLength(start_h);
      m_EndSplittingZl   = pTransverseReinforcementSpec->GetSplittingZoneLength(end_h);
   }
   else
   {
      m_StartSplittingZl = 0.0;
      m_EndSplittingZl =0.0;
   }
}

void pgsShearDesignTool::ResetDesign(const std::vector<pgsPointOfInterest>& pois)
{
   ATLASSERT(m_pBroker!=NULL); // make sure Intialize was called

   // locate and cache points of interest for design
   ValidatePointsOfInterest(pois);

   // Reset check artifacts
   m_StirrupCheckArtifact.Clear();
}

std::vector<pgsPointOfInterest> pgsShearDesignTool::GetDesignPoi()
{
   ATLASSERT(!m_DesignPois.empty());
   return m_DesignPois;
}

pgsStirrupCheckArtifact* pgsShearDesignTool::GetStirrupCheckArtifact()
{
   return &m_StirrupCheckArtifact;
}

GDRCONFIG pgsShearDesignTool::GetGirderConfiguration()
{
   return m_pArtifact->GetGirderConfiguration();
}

void pgsShearDesignTool::DumpDesignParameters()
{
#ifdef _DEBUG

   GET_IFACE(IStrandGeometry,pStrandGeom);

   StrandIndexType Nh = m_pArtifact->GetNumHarpedStrands();

   LOG(_T(""));
   LOG(_T("---------------------------------------------------------------"));
   LOG(_T("pgsShearDesignTool: Current Shear design parameters"));
   LOG(_T(""));
   LOG(_T("f'c  = ") << ::ConvertFromSysUnits(m_pArtifact->GetConcreteStrength(),unitMeasure::KSI) << _T(" KSI"));
   LOG(_T("f'ci = ") << ::ConvertFromSysUnits(m_pArtifact->GetReleaseStrength(),unitMeasure::KSI) <<_T(" KSI"));
   LOG(_T("Np = ") << m_pArtifact->GetNumStraightStrands()+ m_pArtifact->GetNumHarpedStrands());
   LOG(_T("Ns = ") << m_pArtifact->GetNumStraightStrands() << _T("   Pjack = ") << ::ConvertFromSysUnits(m_pArtifact->GetPjackStraightStrands(), unitMeasure::Kip) << _T(" Kip"));
   LOG(_T("Nh = ") << m_pArtifact->GetNumHarpedStrands() << _T("   Pjack = ") << ::ConvertFromSysUnits(m_pArtifact->GetPjackHarpedStrands(), unitMeasure::Kip) << _T(" Kip"));
   LOG(_T("Nt = ") << m_pArtifact->GetNumTempStrands() << _T("   Pjack = ") << ::ConvertFromSysUnits(m_pArtifact->GetPjackTempStrands(), unitMeasure::Kip) << _T(" Kip"));

   LOG(_T("---------------------------------------------------------------"));
   LOG(_T(""));
#endif
}


bool pgsShearDesignTool::DoDesignForConfinement() const
{
   return m_bDoDesignForConfinement;
}

bool pgsShearDesignTool::DoDesignForSplitting() const
{
   return m_bDoDesignForSplitting;
}

bool pgsShearDesignTool::DoDesignFromScratch() const
{
   return m_bDoDesignFromScratch;
}

IndexType pgsShearDesignTool::GetNumStirrupSizeBarCombos() const
{
   return m_BarLegCollection.size();
}

void pgsShearDesignTool::GetStirrupSizeBarCombo(IndexType index, matRebar::Size* pSize, Float64* pNLegs, Float64* pAv) const
{
   ATLASSERT(index<GetNumStirrupSizeBarCombos());
   const BarLegCombo& rcombo = m_BarLegCollection[index];
   *pSize = rcombo.m_Size;
   *pNLegs = rcombo.m_Legs;
   *pAv = rcombo.m_Av;
}

IndexType pgsShearDesignTool::GetNumAvailableBarSpacings() const
{
   return m_AvailableBarSpacings.size();
}

Float64 pgsShearDesignTool::GetAvailableBarSpacing(IndexType index) const
{
   ATLASSERT(index<GetNumAvailableBarSpacings());

   return m_AvailableBarSpacings[index];
}

void pgsShearDesignTool::GetMinZoneLength(Uint32* pSpacings, Float64* pLength) const
{
   *pSpacings = m_MinZoneLengthSpacings;
   *pLength =  m_MinZoneLengthLength;
}

bool pgsShearDesignTool::GetIsTopFlangeRoughened() const
{
   return m_IsTopFlangeRoughened;
}

bool pgsShearDesignTool::GetExtendBarsIntoDeck() const
{
   return m_DoExtendBarsIntoDeck;
}

bool pgsShearDesignTool::GetIsCompositeDeck() const
{
   return m_IsCompositeDeck;
}

bool pgsShearDesignTool::GetDoPrimaryBarsProvideSplittingCapacity() const
{
   if(m_DoBarsProvideSplittingCapacity)
   {
      const pgsSplittingZoneArtifact* pSplittingArtifact = m_StirrupCheckArtifact.GetSplittingZoneArtifact();
      if(pSplittingArtifact->GetSplittingDirection()==pgsTypes::sdVertical) // primary are only vertical
         return true;
   }

   return false;
}

bool pgsShearDesignTool::GetBarsActAsConfinement() const
{
   return m_DoBarsActAsConfinement;
}

GirderLibraryEntry::LongShearCapacityIncreaseMethod pgsShearDesignTool::GetLongShearCapacityIncreaseMethod() const
{
   return m_LongShearCapacityIncreaseMethod;
}

void pgsShearDesignTool::ValidatePointsOfInterest(const std::vector<pgsPointOfInterest>& pois)
{
   // POI's are managed locally
   m_PoiMgr.RemoveAll();

   // Add all Poi's used from the flexure analysis. 
   std::vector<pgsPointOfInterest>::const_iterator poiIter(pois.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(pois.end());
   for ( ; poiIter != poiIterEnd; poiIter++)
   {
      const pgsPointOfInterest& poi = *poiIter;
      if (poi.HasAttribute(pgsTypes::BridgeSite3, POI_CRITSECTSHEAR1))
      {
         // Strip CSS's of their attribute
         pgsPointOfInterest newpoi = poi;
         PoiAttributeType nattrib = poi.GetAttributes(pgsTypes::BridgeSite3);
         nattrib = nattrib ^ POI_CRITSECTSHEAR1;
         newpoi.SetAttributes(pgsTypes::BridgeSite3, nattrib);

         m_PoiMgr.AddPointOfInterest(newpoi);
      }
      else
      {
         m_PoiMgr.AddPointOfInterest(poi);
      }
   }

   // Get CSS for current configuration and add POI to our list
   GDRCONFIG gconfig = GetGirderConfiguration();

   pgsPointOfInterest cssLeft, cssRight;
   GET_IFACE(IPointOfInterest,pPOI);
   pPOI->GetCriticalSection(pgsTypes::StrengthI,m_Span,m_Girder,gconfig,&cssLeft,&cssRight);
   m_PoiMgr.AddPointOfInterest(cssLeft);
   m_PoiMgr.AddPointOfInterest(cssRight);

   // Some additional pois
   const PoiAttributeType attrib = POI_ALLACTIONS | POI_ALLOUTPUT;

   std::vector<pgsTypes::Stage> stages;
   stages.reserve(8);
   stages.push_back(pgsTypes::CastingYard);
   stages.push_back(pgsTypes::Lifting);
   stages.push_back(pgsTypes::Hauling);
   stages.push_back(pgsTypes::GirderPlacement);
   stages.push_back(pgsTypes::TemporaryStrandRemoval);
   stages.push_back(pgsTypes::BridgeSite1);
   stages.push_back(pgsTypes::BridgeSite2);
   stages.push_back(pgsTypes::BridgeSite3);

   // Create some additional pois at 2H, 3H, 4H at each end of girder
   // This will give a better spacing layout for our stirrups
   GET_IFACE(IGirder,pGdr);
   Float64 hgLeft, hgRight; // height of girder at left and right ends
   hgLeft  = pGdr->GetHeight(pgsPointOfInterest(m_Span,m_Girder,0.00));
   hgRight = pGdr->GetHeight(pgsPointOfInterest(m_Span,m_Girder,m_GirderLength));

   pgsPointOfInterest poiL2H(stages, m_Span, m_Girder, m_LeftFosLocation+2.0*hgLeft, attrib);
   m_PoiMgr.AddPointOfInterest(poiL2H);
   pgsPointOfInterest poiL3H(stages, m_Span, m_Girder, m_LeftFosLocation+3.0*hgLeft, attrib);
   m_PoiMgr.AddPointOfInterest(poiL3H);
   pgsPointOfInterest poiL4H(stages, m_Span, m_Girder, m_LeftFosLocation+4.0*hgLeft, attrib);
   m_PoiMgr.AddPointOfInterest(poiL4H);

   pgsPointOfInterest poiR2H(stages, m_Span, m_Girder, m_RightFosLocation-2.0*hgRight, attrib);
   m_PoiMgr.AddPointOfInterest(poiR2H);
   pgsPointOfInterest poiR3H(stages, m_Span, m_Girder, m_RightFosLocation-3.0*hgRight, attrib);
   m_PoiMgr.AddPointOfInterest(poiR3H);
   pgsPointOfInterest poiR4H(stages, m_Span, m_Girder, m_RightFosLocation-4.0*hgRight, attrib);
   m_PoiMgr.AddPointOfInterest(poiR4H);

   // Add pois at the confinement and splitting locations if needed
   if (m_bDoDesignForConfinement)
   {
      if(m_StartConfinementZl > m_StartConnectionLength)
      {
         pgsPointOfInterest poistart(stages, m_Span, m_Girder, m_StartConfinementZl, attrib);
         m_PoiMgr.AddPointOfInterest(poistart);
      }

      if(m_EndConfinementZl > m_EndConnectionLength)
      {
         pgsPointOfInterest poiend(stages, m_Span, m_Girder, m_GirderLength-m_EndConfinementZl, attrib);
         m_PoiMgr.AddPointOfInterest(poiend);
      }
   }

   if (m_bDoDesignForSplitting)
   {
      if(m_StartSplittingZl > m_StartConnectionLength)
      {
         pgsPointOfInterest poistart(stages, m_Span, m_Girder, m_StartSplittingZl, attrib);
         m_PoiMgr.AddPointOfInterest(poistart);
      }

      if(m_EndSplittingZl > m_EndConnectionLength)
      {
         pgsPointOfInterest poiend(stages, m_Span, m_Girder, m_GirderLength-m_EndSplittingZl, attrib);
         m_PoiMgr.AddPointOfInterest(poiend);
      }
   }

   // For the special case where there is zero length supports: The flexure design does not add
   // pois at the very ends of the girder. Add them here
   if (m_StartConnectionLength==0.0 && m_LeftFosLocation==0.0)
   {
      pgsPointOfInterest poiend(stages, m_Span, m_Girder, 0.0, attrib);
      m_PoiMgr.AddPointOfInterest(poiend);
   }

   if (m_EndConnectionLength==0.0 && m_RightFosLocation==m_GirderLength)
   {
      pgsPointOfInterest poiend(stages, m_Span, m_Girder, m_GirderLength, attrib);
      m_PoiMgr.AddPointOfInterest(poiend);
   }

   // Update our Vector to allow ordered access to design pois
   m_DesignPois.clear();
   m_PoiMgr.GetPointsOfInterest(m_Span, m_Girder, pgsTypes::BridgeSite3, POI_SHEAR, POIMGR_OR, &m_DesignPois);
}

pgsShearDesignTool::ShearDesignOutcome pgsShearDesignTool::Validate()
{
   // Copy shear data from girder artifact. This is the data we design on
   m_ShearData = m_pArtifact->GetShearData();

   // Precompute and cache some needed values for design
   ShearDesignOutcome sd = ValidateVerticalAvsDemand();

   if (sd != sdFail)
   {
      ValidateHorizontalAvsDemand();
   }

   return sd;
}

pgsShearDesignTool::ShearDesignOutcome pgsShearDesignTool::ValidateVerticalAvsDemand()
{
   GET_IFACE(IShearCapacity,pShearCap);

   IndexType numpois = m_DesignPois.size();
   ATLASSERT(numpois>0);

   // Build envelope of strengthI and strengthII
   LOG(_T("----------------------------------------------------"));
   LOG(_T("Raw Vertical Av/S values at ")<<numpois<<_T(" pois"));
   LOG(_T("Location, avsStrengthI, avsStrengthII"));

   m_VertShearAvsDemandAtPois.clear();
   m_VertShearAvsDemandAtPois.reserve(numpois);

   // Av/s demand details are stored in spec check results
   bool was_strut_tie_reqd = false;
   Float64 FcRequiredForStrutTieStress = 0.0;
   std::vector<pgsPointOfInterest>::const_iterator i( m_DesignPois.begin());
   while ( i != m_DesignPois.end())
   {
      Float64 avs_val=0.0;

      const pgsPointOfInterest& poi = *i;
      Float64 location = poi.GetDistFromStart();

      // We don't design for demands outside of the CSS's
      if (location >= m_LeftCSS && location <= m_RightCSS)
      {
         pgsStirrupCheckAtPoisArtifactKey keyI(pgsTypes::BridgeSite3, pgsTypes::StrengthI, location);

         const pgsStirrupCheckAtPoisArtifact* pStrI_artf = m_StirrupCheckArtifact.GetStirrupCheckAtPoisArtifact(keyI);
         if(pStrI_artf!=NULL)
         {
            const pgsVerticalShearArtifact* pvertart = pStrI_artf->GetVerticalShearArtifact();

            avs_val = pvertart->GetAvOverSReqd();

            if( pvertart->IsStrutAndTieRequired(pgsTypes::metStart) || pvertart->IsStrutAndTieRequired(pgsTypes::metEnd))
            {
               was_strut_tie_reqd = true;

               // Compute concrete strength required if shear stress forces strut and tie at CSS
               GET_IFACE(IShearCapacity,pShearCapacity);
               GDRCONFIG config = this->GetGirderConfiguration(); // current design

               SHEARCAPACITYDETAILS scd;
               pShearCapacity->GetShearCapacityDetails( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, config, &scd );

               Float64 fcreq = fabs(scd.vfc * config.Fc / 0.18); // LRFD 5.8.3.2

               // FUDGE: Give concrete strength a small bump up. Found some regression tests that were missing
               //        by just a smidge...
               fcreq *= 1.01;

               FcRequiredForStrutTieStress = max(FcRequiredForStrutTieStress, fcreq);
            }
         }
         else
         {
            ATLASSERT(0); // this should never happen
            continue;
         }
         
         if (m_bIsPermit)
         {
            // Envelope str I & II
            Float64 avs_II;
            pgsStirrupCheckAtPoisArtifactKey keyII(pgsTypes::BridgeSite3, pgsTypes::StrengthII, location);

            const pgsStirrupCheckAtPoisArtifact* pStrII_artf = m_StirrupCheckArtifact.GetStirrupCheckAtPoisArtifact(keyII);
            if(pStrII_artf!=NULL)
            {
               const pgsVerticalShearArtifact* pvertart = pStrII_artf->GetVerticalShearArtifact();

               avs_II = pvertart->GetAvOverSReqd();

               if( pvertart->IsStrutAndTieRequired(pgsTypes::metStart) || pvertart->IsStrutAndTieRequired(pgsTypes::metEnd))
               {
                  was_strut_tie_reqd = true;

                  // Compute concrete strength required if shear stress forces strut and tie at CSS
                  GET_IFACE(IShearCapacity,pShearCapacity);
                  GDRCONFIG config = this->GetGirderConfiguration(); // current design

                  SHEARCAPACITYDETAILS scd;
                  pShearCapacity->GetShearCapacityDetails( pgsTypes::StrengthII, pgsTypes::BridgeSite3, poi, config, &scd );

                  Float64 fcreq = fabs(scd.vfc * config.Fc / 0.18); // LRFD 5.8.3.2

                  FcRequiredForStrutTieStress = max(FcRequiredForStrutTieStress, fcreq);
               }
            }
            else
            {
               ATLASSERT(0); // this should never happen
               continue;
            }

            LOG(poi.GetDistFromStart()<<_T(", ")<<avs_val<<_T(", ")<<avs_II);

            avs_val = max( avs_val, avs_II );
         }
         else
         {
            // Str I is only load case
            LOG(poi.GetDistFromStart()<<_T(", ")<<avs_val);
         }

         if (avs_val<SPACING_TOL) // weed out noise
            avs_val = 0.0;
      }

      m_VertShearAvsDemandAtPois.push_back(avs_val);

      i++;
   }

   ShearDesignOutcome sd = sdSuccess;

   if (was_strut_tie_reqd)
   {
      // Strut and tie required. Save required f'c and set return code
      m_RequiredFcForShearStress = FcRequiredForStrutTieStress; // will need this to compute a realistic f'c
      sd = sdDesignFailedFromShearStress;
   }

   // Process demand curve for symmetry and maximizing at girder ends
   ProcessAvsDemand(m_VertShearAvsDemandAtPois, m_VertShearAvsDemandAtX);

   return sd;
}

void pgsShearDesignTool::ProcessAvsDemand(std::vector<Float64>& rDemandAtPois, mathPwLinearFunction2dUsingPoints& rDemandAtLocations)
{
   IndexType numpois = m_DesignPois.size();

   // If needed, mirror results and compute max envelop of mirrors about mid-span 
   if (m_bDoDesignFromScratch || m_bIsCurrentStirrupLayoutSymmetrical)
   {
      // Use math function to perform mirroring
      mathPwLinearFunction2dUsingPoints mirror_avs;
      IndexType idx=0;
      std::vector<pgsPointOfInterest>::const_iterator i( m_DesignPois.begin());
      while (i != m_DesignPois.end())
      {
         const pgsPointOfInterest& poi = *i;
         Float64 x = poi.GetDistFromStart();
         Float64 y  = rDemandAtPois[idx];
         mirror_avs.AddPoint(gpPoint2d(x,y));
         idx++;
         i++;
      }
      
      // mathPwLinearFunction2dUsingPoints can throw, and it's probably not the end of the world
      // if if does. Don't let it crash program.
      IndexType spn2_idx=0; // store index at mid-span;
      try
      {
      // Mirror about location of mid-span measured from girder start
      Float64 spn2 = (m_StartConnectionLength+m_GirderLength-m_EndConnectionLength)/2.0; 
      mirror_avs.MirrorAboutY(spn2);

         // Mirrored function needs to lie at least within the same X range as the original function
         // Force this by extending range out to support locations
         Float64 range_start = m_StartConnectionLength;
         Float64 range_end   = m_GirderLength - m_EndConnectionLength;
      mirror_avs.ResetOuterRange(math1dRange(range_start,math1dRange::Bound,range_end,math1dRange::Bound));

      // Since we have mirrored, we aren't gauranteed to have x values in same locations as POI's.
      // Use function2d class to get x,y locations for mirror
      idx=0;
      for ( i = m_DesignPois.begin(); i != m_DesignPois.end(); i++ )
      {
         const pgsPointOfInterest& poi = *i;

         Float64 x = poi.GetDistFromStart();
         Float64& ry  = rDemandAtPois[idx]; // grab reference for reassignment below
         Float64 mry = mirror_avs.Evaluate(x);

         Float64 maxy = max(ry, mry);

         ry = maxy;

         if (spn2_idx==0 && x>spn2)
         {
            spn2_idx = idx;
         }

         idx++;
         }
      }
      catch(...)
      {
         ATLASSERT(0); // This really should never happen. Somehow our function got out
                       // of bounds? This is a programming error.
         spn2_idx = rDemandAtPois.size()/2; // reasonable assumption for mid-span
      }

      // Last step is to process the avs curve to make sure that values always increase from
      // mid-span towards the ends of girder.
      // First the left end
      bool first = true;
      Float64 lastval;
      for(idx = spn2_idx+1; idx > 0; idx--)
      {
         if (first)
         {
            lastval = rDemandAtPois[idx-1];
            first = false;
         }
         else
         {
            Float64& rcurval = rDemandAtPois[idx-1]; // reference
            if( rcurval < lastval)
            {
               rcurval = lastval;
            }
            else
            {
               lastval = rcurval;
            }
         }
      }

      // Right end
      first = true;
      for(idx=spn2_idx; idx<numpois; idx++)
      {
         if (first)
         {
            lastval = rDemandAtPois[idx];
            first = false;
         }
         else
         {
            Float64& rcurval = rDemandAtPois[idx];
            if( rcurval < lastval)
            {
               rcurval = lastval;
            }
            else
            {
               lastval = rcurval;
            }
         }
      }

      // Dump Log of final values
#if defined ENABLE_LOGGING
      LOG(_T("----------------------------------------------------"));
      LOG(_T("Mirrored generic Av/S values at ")<<numpois<<_T(" pois"));
      LOG(_T("Location, mirror_avs, processed_avs"));
      idx=0;
      for ( i = m_DesignPois.begin(); i != m_DesignPois.end(); i++ )
      {
         const pgsPointOfInterest& poi = *i;

         Float64 x = poi.GetDistFromStart();
         Float64 ry  = rDemandAtPois[idx];
         Float64 mry = mirror_avs.Evaluate(x);

         LOG( x << _T(", ") << mry <<_T(", ") << ry);
         idx++;
      }
#endif
   }

   // Store Av/s at locations
   rDemandAtLocations.Clear();
   IndexType pidx = 0;
   std::vector<pgsPointOfInterest>::const_iterator i( m_DesignPois.begin());
   while (i != m_DesignPois.end())
   {
      const pgsPointOfInterest& poi = *i;

      // Stretch demand at supports to ends of girder
      Float64 x;
      if (pidx==0)
      {
         x = 0.0;
      }
      else if (pidx==numpois-1)
      {
         x = m_GirderLength;
      }
      else
      {
         x = poi.GetDistFromStart();
      }

      Float64 y  = rDemandAtPois[pidx];

      rDemandAtLocations.AddPoint(gpPoint2d(x,y));
      pidx++;
      i++;
   }
}

Float64 pgsShearDesignTool::GetVerticalAvsDemand(IndexType PoiIdx)
{
   ATLASSERT(PoiIdx < m_VertShearAvsDemandAtPois.size() && PoiIdx != INVALID_INDEX);
   return m_VertShearAvsDemandAtPois[PoiIdx];
}

Float64 pgsShearDesignTool::GetVerticalAvsDemand(Float64 distFromStart)
{
   try
   {
      return m_VertShearAvsDemandAtX.Evaluate(distFromStart);
   }
   catch (mathXEvalError& e)
   {
      std::wstring msg;
      e.GetErrorMessage(&msg);
      ATLASSERT(0); // This should never happen, but catch here to make debugging easier
      return 0.0;
   }
}

void pgsShearDesignTool::ValidateHorizontalAvsDemand()
{
   GET_IFACE(IShearCapacity,pShearCap);

   IndexType numpois = m_DesignPois.size();
   ATLASSERT(numpois>0);

   // Build envelope of strengthI and strengthII
   LOG(_T("----------------------------------------------------"));
   LOG(_T("Raw Horizontal Av/S values at ")<<numpois<<_T(" pois"));
   LOG(_T("Location, avsStrengthI, avsStrengthII"));

   m_HorizShearAvsDemandAtPois.clear();
   m_HorizShearAvsDemandAtPois.reserve(numpois);

   // Av/s demand details are stored in spec check results
   std::vector<pgsPointOfInterest>::const_iterator i( m_DesignPois.begin());
   while ( i != m_DesignPois.end())
   {
      const pgsPointOfInterest& poi = *i;
      Float64 location = poi.GetDistFromStart();

      Float64 avs_val=0.0;

      // We don't design for demands outside of the CSS's
      if (location >= m_LeftCSS && location <= m_RightCSS)
      {
         pgsStirrupCheckAtPoisArtifactKey keyI(pgsTypes::BridgeSite3, pgsTypes::StrengthI, location);

         const pgsStirrupCheckAtPoisArtifact* pStrI_artf = m_StirrupCheckArtifact.GetStirrupCheckAtPoisArtifact(keyI);
         if(pStrI_artf!=NULL)
         {
            const pgsHorizontalShearArtifact* phorizart = pStrI_artf->GetHorizontalShearArtifact();

            avs_val = phorizart->GetAvOverSReqd();
         }
         else
         {
            ATLASSERT(0); // this should never happen
            continue;
         }
         
         if (m_bIsPermit)
         {
            // Envelope str I & II
            Float64 avs_II;
            pgsStirrupCheckAtPoisArtifactKey keyII(pgsTypes::BridgeSite3, pgsTypes::StrengthII, location);

            const pgsStirrupCheckAtPoisArtifact* pStrII_artf = m_StirrupCheckArtifact.GetStirrupCheckAtPoisArtifact(keyII);
            if(pStrII_artf!=NULL)
            {
               const pgsHorizontalShearArtifact* phorizart = pStrII_artf->GetHorizontalShearArtifact();

               avs_II = phorizart->GetAvOverSReqd();
            }
            else
            {
               ATLASSERT(0); // this should never happen
               continue;
            }

            LOG(poi.GetDistFromStart()<<_T(", ")<<avs_val<<_T(", ")<<avs_II);

            avs_val = max( avs_val, avs_II );
         }
         else
         {
            // Str I is only load case
            LOG(poi.GetDistFromStart()<<_T(", ")<<avs_val);
         }

         if (avs_val<SPACING_TOL) // weed out noise
            avs_val = 0.0;
      }

      m_HorizShearAvsDemandAtPois.push_back(avs_val);

      i++;
   }

   // Process Av/s curve for symmetry and increasing value toward ends
   ProcessAvsDemand(m_HorizShearAvsDemandAtPois, m_HorizShearAvsDemandAtX);
}

Float64 pgsShearDesignTool::GetHorizontalAvsDemand(IndexType PoiIdx)
{
   ATLASSERT(PoiIdx < m_HorizShearAvsDemandAtPois.size() && PoiIdx != INVALID_INDEX);
   return m_HorizShearAvsDemandAtPois[PoiIdx];
}

Float64 pgsShearDesignTool::GetHorizontalAvsDemand(Float64 distFromStart)
{
   try
   {
      return m_HorizShearAvsDemandAtX.Evaluate(distFromStart);
   }
   catch (mathXEvalError& e)
   {
      std::wstring msg;
      e.GetErrorMessage(&msg);
      ATLASSERT(0); // This should never happen, but catch here to make debugging easier
      return 0.0;
   }
}

Float64 pgsShearDesignTool::GetVerticalAvsMaxInRange(Float64 leftBound, Float64 rightBound)
{
   try
   {
      Float64 fmin, fmax;
      m_VertShearAvsDemandAtX.GetMaximumsInRange(math1dRange(leftBound,math1dRange::Bound,rightBound,math1dRange::Bound), &fmin, &fmax);
      return fmax;
   }
   catch (mathXEvalError& e)
   {
      std::wstring msg;
      e.GetErrorMessage(&msg);
      ATLASSERT(0); // This should never happen, but catch here to make debugging easier
      return 0.0;
   }
}

Float64 pgsShearDesignTool::GetHorizontalAvsMaxInRange(Float64 leftBound, Float64 rightBound)
{
   // Tweak boundaries slightly so we don't bleed into next range
   leftBound  += SPACING_TOL;
   rightBound -= SPACING_TOL;

   try
   {
      Float64 fmin, fmax;
      m_HorizShearAvsDemandAtX.GetMaximumsInRange(math1dRange(leftBound,math1dRange::Bound,rightBound,math1dRange::Bound), &fmin, &fmax);
      return fmax;
   }
   catch (mathXEvalError& e)
   {
      std::wstring msg;
      e.GetErrorMessage(&msg);
      ATLASSERT(0); // This should never happen, but catch here to make debugging easier
      return 0.0;
   }
}

pgsShearDesignTool::ShearDesignOutcome pgsShearDesignTool::DesignStirrups(Float64 leftCSS, Float64 rightCSS)
{
   m_LeftCSS  = leftCSS;
   m_RightCSS = rightCSS;

   // Compute and cache some needed values
   ShearDesignOutcome st = Validate();

   // It is possible that we failed above due to shear stress requirement on f'c and f'c was changed.
   // If this happened, we must continue through rest of design and preserve the design outcome
   if (st == sdFail)
   {
      return st; 
   }

   if (DoDesignFromScratch())
   {
      // Design primary stirrup layout from blank slate
      if( !LayoutPrimaryStirrupZones() )
      {
         st = sdFail;
      }
   }
   else
   {
      // Base primary stirrup layout on current shear data
      if( !ModifyPreExistingStirrupDesign() )
      {
         st = sdFail;
      }
   }

   if(st!=sdFail && !this->GetExtendBarsIntoDeck())
   {
      // Primary bars not used for horizontal shear
      // Design additional horizontal shear bars
      if(!DetailHorizontalInterfaceShear())
      {
         st = sdFail;
      }
   }

   if (st!=sdFail)
   {
      // Splitting
      if(!DetailAdditionalSplitting())
      {
         st = sdFail;
      }
   }

   if (st!=sdFail)
   {
      // Confinement
      if(!DetailAdditionalConfinement())
      {
         st = sdFail;
      }
   }

   // We are done with stirrup design
   // Set design artifact data whether successful or not
   m_pArtifact->SetNumberOfStirrupZonesDesigned( m_ShearData.ShearZones.size() );
   m_pArtifact->SetShearData(m_ShearData);

   if (st!=sdFail)
   {
      // Long Reinforcement for shar
      ShearDesignOutcome stlocal = DesignLongReinfShear();

      if (stlocal!=sdSuccess)
      {
         st = stlocal; 
      }
   }

   return st;
}

Float64 pgsShearDesignTool::GetRequiredAsForLongReinfShear() const
{
   return m_LongReinfShearAs;
}

Float64 pgsShearDesignTool::GetFcRequiredForShearStress() const
{
   return m_RequiredFcForShearStress;
}

bool pgsShearDesignTool::LayoutPrimaryStirrupZones()
{
   m_ShearData.ShearZones.clear(); // clear out any existing data

   // From-scratch layouts are always symmetrical
   m_ShearData.bAreZonesSymmetrical = true;

   long nzones_designed(0);
   Float64 curr_zone_start(0.0);
   Float64 L_zone_min(0.0); // minimum length of current zone
   Float64 S_max_next;      // max spacing in next created zone

   // Loop over all shear pois for design for half girder
   IndexType numpois = m_DesignPois.size();

   Float64 gl2 = m_GirderLength/2.0;

   // bar size, nlegs are established at first poi, and the same for entire design
   matRebar::Size bar_size;
   Float64 nlegs;
   Float64 Av; // for bar/legs combo

   // State control variables for loop
   bool create_zone(false); // create data for zone
   bool close_zone(false);  // add zone to collection
   bool done(false);

   CShearZoneData zone_data; // data for current zone

   Float64 prev_location(0.0); // location of previous poi
   IndexType ipoi(0);
   while ( !done )
   {
      Float64 S_reqd;
      Float64 zone_len;

      Float64 location = m_DesignPois[ipoi].GetDistFromStart();
      if (location >= gl2) 
      {
         // We are past left end of mid-girder - close and quit
         close_zone = true;
         done = true;
      }
      else
      {
         // Compute zone data
         Float64 max_spac =   ComputeMaxStirrupSpacing(ipoi);
         Float64 avs_demand = GetVerticalAvsDemand(ipoi);

         // Use primary bars for horizontal shear if needed.
         bool horiz_controlled = false;
         if (GetExtendBarsIntoDeck())
         {
            Float64 havs_demand = this->GetHorizontalAvsDemand(ipoi);
            if(havs_demand > avs_demand)
            {
               avs_demand = havs_demand;
               horiz_controlled = true;
            }
         }

         if (ipoi==0)
         {
            // First POI - select stirrup size and nlegs to be used for entire girder
            // Base size and legs on vertical demand
            if ( !GetBarSizeSpacingForAvs(avs_demand, max_spac, &bar_size, &nlegs, &Av, &S_reqd) )
            {
               // There is no bar/legs combination that can satisfy demand - we have failed.
               if(horiz_controlled)
               {
                  m_pArtifact->SetOutcome(pgsDesignArtifact::TooManyStirrupsReqdForHorizontalInterfaceShear);
               }
               else
               {
                  m_pArtifact->SetOutcome(pgsDesignArtifact::TooManyStirrupsReqd);
               }
               return false;
            }

            // We have a bar size and nlegs - start design of first zone
            curr_zone_start = 0.0;

            Uint32 num_spacings4zone;
            Float64 min_zone_len;
            this->GetMinZoneLength(&num_spacings4zone, &min_zone_len);
            L_zone_min = max(S_reqd*num_spacings4zone, min_zone_len);

            // Store zone data that doesn't change from first zone
            zone_data.VertBarSize = bar_size;
            zone_data.nVertBars = nlegs;

            zone_data.nHorzInterfaceBars = GetExtendBarsIntoDeck() ? nlegs : 0.0;

            // This first zone can be a splitting zone if needed
            if(GetDoPrimaryBarsProvideSplittingCapacity() && DoDesignForSplitting())
            {
               Float64 avs_split = GetAvsReqdForSplitting();
               if (avs_split > avs_demand)
               {
                  // Use same bar size for splitting as selected above for vertical shear demand. 
                  // We may not be able to make splitting requirement with primary zone only, 
                  // so use tightest spacing available if this is the case
                  bool st = GetBarSpacingForAvs(avs_split, max_spac, bar_size, Av, &S_reqd);
                  if (!st)
                  {
                     S_reqd = m_AvailableBarSpacings.front();
                  }

                  // Create splitting zone
                  L_zone_min = Max3(L_zone_min, m_StartSplittingZl, m_EndSplittingZl);
               }
            }

            S_max_next = ComputeMaxNextSpacing(S_reqd);

            create_zone = true; // store creation data for this zone. won't save yet if not splitting zone
         }
         else
         {
            // After first poi. We might be able to start a new zone if location is in new zone
            if (location > curr_zone_start)
            {
               // Get bar spacing for current stirrups
               bool st = GetBarSpacingForAvs(avs_demand, max_spac, bar_size, Av, &S_reqd);
               ATLASSERT(st); // should never fail since av/s demand decreases to mid-span

               // Don't allow spacing to increase too quickly. Feather if necessary.
               if (S_reqd > S_max_next)
               {
                  S_reqd = S_max_next;
               }

               // Should we close existing zone and start new zone based on this spacing?
               if ( S_reqd > zone_data.BarSpacing )
               {
                  zone_len = prev_location - curr_zone_start;

                  // Make zone length a multiple of required spacing
                  zone_len = CeilOff(zone_len, zone_data.BarSpacing); 

                  // FUDGE: Artificiallly increase zone length by two bar spacings. This is done
                  //        because POI's for design are not exactly located at zone termination
                  //        locations.
                  zone_len += 2.0 * zone_data.BarSpacing;

                  // Is the zone long enough?
                  if (zone_len >= L_zone_min)
                  {
                     close_zone = true;
                     create_zone = true;
                  }
               }
            }
         }
      }

      // Terminate loop if current spacing is max possible.
      if (ipoi != 0 && zone_data.BarSpacing >= m_SMaxMax)
      {
         // This will be our last zone
         close_zone = true;
         done = true;
      }

      if (close_zone)
      {
         // Round zone length so it will look good in stirrup dialog grids
         GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
         zone_len = ::ConvertFromSysUnits(zone_len,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
         zone_len = RoundOff(zone_len,0.0001); // three decimal places
         zone_len = ::ConvertToSysUnits(zone_len,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);

         // Store current zone data in zone collection
         zone_data.ZoneNum = ++nzones_designed;

         zone_data.ZoneLength = done ? 0.0 : zone_len; // last zone has infinite length

         m_ShearData.ShearZones.push_back(zone_data);

         // information for next zone - update state data
         if (!done)
         {
            curr_zone_start += zone_len;
         }

         close_zone = false;
      }

      if (create_zone)
      {
         // Store spacing data for new zone
         zone_data.BarSpacing = S_reqd;

         // Minimum length for this zone
         Uint32 num_spacings4zone;
         Float64 min_zone_len;
         this->GetMinZoneLength(&num_spacings4zone, &min_zone_len);
         L_zone_min = max(zone_data.BarSpacing*num_spacings4zone, min_zone_len);

         // Max bar spacing for next zone after this
         S_max_next = ComputeMaxNextSpacing( zone_data.BarSpacing );

         // Add confinement bar if warranted
         if ( DoDesignForConfinement() && GetBarsActAsConfinement() && IsLocationInConfinementZone(curr_zone_start) )
         {
            zone_data.ConfinementBarSize = bar_size;
         }
         else
         {
            zone_data.ConfinementBarSize = matRebar::bsNone;
         }

         create_zone = false;
      }

      ipoi++;

      prev_location = location;

      if (ipoi==numpois) // safety check should never happen since we are only looking at half girder
      {
         ATLASSERT(0);
         done = true;
      }
   } // poi loop

   if (m_ShearData.ShearZones.empty())
   {
      ATLASSERT(0); // We should always end up with at least one shear zone, 
                    // but if not put in a default zone so we don't crash later
      m_ShearData.ShearZones.push_back( CShearZoneData() );
   }

   return true; // Success
}

bool  pgsShearDesignTool::ModifyPreExistingStirrupDesign()
{
   ATLASSERT(!m_ShearData.ShearZones.empty()); // We should have shear zones before we get here
   GET_IFACE(IBridgeMaterial,pMaterial);

   // Some needed values
   matRebar::Grade barGrade;
   matRebar::Type barType;
   pMaterial->GetTransverseRebarMaterial(m_Span,m_Girder,barType,barGrade);
   lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
   ATLASSERT(pool != NULL);

   // Design stirrups from left-right for both symmetrical and non-symmetrical cases

   // Stirrup zone iterator can move from left-right, or right-left depending on type
   // Use iterator class to walk along girder and keep track of correct directional information
   LeftRightStirrupZoneIter lr_zone_iter(m_ShearData.ShearZones, m_GirderLength);
   if(!DesignPreExistingStirrups(lr_zone_iter, m_LeftCSS, barGrade, barType, pool))
   {
      return false;
   }

   // Design right end of girder only for non-symmetric cases
   if (!m_bIsCurrentStirrupLayoutSymmetrical)
   {
      RightLeftStirrupZoneIter rl_zone_iter(m_ShearData.ShearZones, m_GirderLength);
      if(!DesignPreExistingStirrups(rl_zone_iter, m_RightCSS, barGrade, barType, pool))
      {
         return false;
      }
   }

   // Expand stirrup zone lengths so stirrup spacings fit exactly. Only for symmetrical cases
   if (m_bIsCurrentStirrupLayoutSymmetrical)
   {
      ExpandStirrupZoneLengths(m_ShearData.ShearZones);
   }

   return true;
}

bool pgsShearDesignTool::DesignPreExistingStirrups(StirrupZoneIter& rIter, Float64 locCSS,  matRebar::Grade barGrade, matRebar::Type barType, lrfdRebarPool* pool)
{
   // Loop over shear zones using iterator and increase Av/S if need be
   Float64 S_max_next(Float64_Max); // spacing in first zone not limited by pre-zones

   // Capture zone containing CSS - Need this to make sure av/s does not decrease toward end of beam
   IndexType idx(0), idxCSS(INVALID_INDEX);
   Float64 zoneEnd(0.0);
   CShearZoneData szdAtCSS;

   for (rIter.First(); !rIter.IsDone(); rIter.Next())
   {
      StirrupZoneItem sz_item  = rIter.CurrentItem();

      CShearZoneData& rzdata(*sz_item.pShearZoneData);

      // Check if CSS is in this zone and save zone data at CSS if so
      zoneEnd += rzdata.ZoneLength;
      if (zoneEnd>=locCSS && idxCSS==INVALID_INDEX)
      {
         idxCSS = idx;
         szdAtCSS = rzdata;
      }

      // Av/s capacity in this zone
      Float64 av;
      Float64 avs_cap;
      if(rzdata.VertBarSize!=matRebar::bsNone && rzdata.BarSpacing>0.0)
      {
         const matRebar* pRebar = pool->GetRebar(barType,barGrade,rzdata.VertBarSize);
         av = pRebar->GetNominalArea() * rzdata.nVertBars;
         avs_cap = av/rzdata.BarSpacing;
      }
      else
      {
         av = 0.0;
         avs_cap = 0.0;
      }

      // Get max av/s demand in this zone
      Float64 avs_demand = GetVerticalAvsMaxInRange(sz_item.leftEnd ,sz_item.rightEnd);
      if (GetExtendBarsIntoDeck())
      {
         // Design for horizontal shear if neccessary
         Float64 havs_demand = GetHorizontalAvsMaxInRange(sz_item.leftEnd ,sz_item.rightEnd);
         if (havs_demand > avs_demand)
         {
            avs_demand = havs_demand;
         }
      }

      // max stirrup spacing in zone
      Float64 max_spacing = ComputeMaxStirrupSpacing(sz_item.startLocation);

      // check demand
      if (avs_demand > avs_cap)
      {
         // We need to increase capacity. First try bars of the same size/nlegs
         Float64 new_spacing;
         if ( rzdata.VertBarSize!=matRebar::bsNone && 
              GetBarSpacingForAvs(avs_demand, S_max_next, rzdata.VertBarSize, av, &new_spacing) )
         {
            rzdata.BarSpacing = min(new_spacing, max_spacing); // kept bar size, changed spacing
         }
         else
         {
            // Existing bar size won't work. Try others
            matRebar::Size new_size;
            Float64 new_nlegs;
            Float64 new_av;
            if (GetBarSizeSpacingForAvs(avs_demand, S_max_next, &new_size, &new_nlegs, &new_av, &new_spacing) )
            {
               rzdata.VertBarSize = new_size;
               rzdata.nVertBars   = new_nlegs;
               rzdata.BarSpacing  = min(new_spacing, max_spacing);

               rzdata.nHorzInterfaceBars = GetExtendBarsIntoDeck() ? rzdata.nVertBars : 0.0;
            }
            else
            {
               // There is no bar/legs combination that can satisfy demand - we have failed.
               m_pArtifact->SetOutcome(pgsDesignArtifact::TooManyStirrupsReqd);
               return false;
            }
         }

         // Set flag that this zone was redesigned
         rzdata.bWasDesigned = true;

      }

      // make sure we've met spacing requirements
      if(rzdata.BarSpacing > max_spacing)
      {
         rzdata.BarSpacing = max_spacing;

         rzdata.bWasDesigned = true;
      }

      // Extend vertical bars into deck if design option is selected
      if (GetExtendBarsIntoDeck() && rzdata.nHorzInterfaceBars < rzdata.nVertBars)
      {
         rzdata.nHorzInterfaceBars = rzdata.nVertBars;
      }

      // max spacing for next zone (assumes bar size does not change)
      S_max_next = ComputeMaxNextSpacing(rzdata.BarSpacing);

      idx++;
   }

   // Last, we need to make sure that av/s does not decrease from CSS to CL bearing
   if(idxCSS!=INVALID_INDEX && idxCSS>0)
   {
      // We have captured shear zone data at css, and it is not the first zone.
      // Make sure previous zones have at least the same av/s
      if(szdAtCSS.VertBarSize!=matRebar::bsNone && szdAtCSS.BarSpacing>0.0)
      {
         const matRebar* pRebar = pool->GetRebar(barType,barGrade,szdAtCSS.VertBarSize);
         Float64 av = pRebar->GetNominalArea() * szdAtCSS.nVertBars;
         Float64 avsAtCSS = av/szdAtCSS.BarSpacing;

         // Loop over zones up to CSS zone
         idx = 0;
         rIter.First();
         while(idx < idxCSS)
         {
            if (rIter.IsDone())
            {
               ATLASSERT(0); // should never happen
               break;
            }

            StirrupZoneItem sz_item  = rIter.CurrentItem();
            CShearZoneData& rzdata(*sz_item.pShearZoneData);

            // Av/s capacity in this zone
            Float64 avs_cap(0);
            if(rzdata.VertBarSize!=matRebar::bsNone && rzdata.BarSpacing>0.0)
            {
               const matRebar* pRebar = pool->GetRebar(barType,barGrade,rzdata.VertBarSize);
               av = pRebar->GetNominalArea() * rzdata.nVertBars;
               avs_cap = av/rzdata.BarSpacing;
            }

            if (avs_cap < avsAtCSS)
            {
               // Capacity in this zone is less than at CSS. No good - just make same as at CSS
               rzdata.VertBarSize = szdAtCSS.VertBarSize;
               rzdata.BarSpacing  = szdAtCSS.BarSpacing;
               rzdata.nVertBars   = szdAtCSS.nVertBars;

               rzdata.bWasDesigned = true;
            }

            idx++;
            rIter.Next();
         }
      }
   }

   return true;
}

void pgsShearDesignTool::ExpandStirrupZoneLengths(CShearData::ShearZoneVec& shearZones)
{
   CShearData::ShearZoneIterator it = shearZones.begin();
   CShearData::ShearZoneIterator ite = shearZones.end();
   if (ite!=it)
   {
      ite--; // We don't modify last zone
   }

   while(it!=ite)
   {
      CShearZoneData& rzdata(*it);

      // Only tweak zones that we designed
      if (rzdata.bWasDesigned)
      {
         if (!IsZero(rzdata.BarSpacing))
         {
            Float64 rns = rzdata.ZoneLength / rzdata.BarSpacing; // real number of spaces
            Int32   ins = (Int32)rns;                            // chopped integer number of spaces
            Float64 rmdr = rns - ins;                            // remainder

            // Determine if zone length is an integral number of spaces within tolerance
            if ( !(IsZero(rmdr, SPACING_TOL) || IsEqual(1.0, rmdr, SPACING_TOL)) )
            {
               // Bar spacing doesn't fit. 
               // Round zone spacing up to next increment.
               rzdata.ZoneLength = rzdata.BarSpacing * (ins+1);
            }
         }
         else
         {
            ATLASSERT(0); // should always have positive bar spacing
         }
      }

      it++;
   }
}

bool pgsShearDesignTool::DetailHorizontalInterfaceShear()
{
   ATLASSERT(!GetExtendBarsIntoDeck());

   // See if we have existing horizontal shears in zone, otherwise copy vertical shear zone spacing
   if (!GetIsCompositeDeck())
   {
      // no bars if no deck
      m_ShearData.HorizontalInterfaceZones.clear();
      return true;
   }
   else if (DoDesignFromScratch() || m_ShearData.HorizontalInterfaceZones.size() < 2)
   {
      m_ShearData.HorizontalInterfaceZones.clear();
   }

   if(m_ShearData.HorizontalInterfaceZones.empty())
   {
      // Fill with empty zones
      CShearData::ShearZoneConstIterator it(m_ShearData.ShearZones.begin());
      while ( it!=m_ShearData.ShearZones.end())
      {
         CHorizontalInterfaceZoneData zdata;
         zdata.ZoneNum = it->ZoneNum;
         zdata.ZoneLength = it->ZoneLength;

         m_ShearData.HorizontalInterfaceZones.push_back(zdata);
         it++;
      }
   }

   GET_IFACE(IBridgeMaterial,pMaterial);
   matRebar::Grade barGrade;
   matRebar::Type barType;
   pMaterial->GetTransverseRebarMaterial(m_Span,m_Girder,barType,barGrade);

   lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
   ATLASSERT(pool != NULL);

   // Loop over zones and design each. To simplify life, we DO NOT abide by vertical stirrup
   // zone spacing and size jump rules here
   Float64 zone_max = m_ShearData.bAreZonesSymmetrical ? m_GirderLength/2.0 : m_GirderLength;

   GET_IFACE(IBridge,pBridge);
   Float64 girder_length = pBridge->GetGirderLength(m_Span,m_Girder);
   pgsPointOfInterest poiStart(m_Span,m_Girder,0.0);
   pgsPointOfInterest poiEnd(m_Span,m_Girder,girder_length);
   GET_IFACE(IInterfaceShearRequirements,pInterfaceShear);
   Float64 max_spacing = min(pInterfaceShear->GetMaxShearConnectorSpacing(poiStart),pInterfaceShear->GetMaxShearConnectorSpacing(poiEnd));

   Float64 zone_start(0.0);
   bool bdone(false);
   CShearData::HorizontalInterfaceZoneIterator ith(m_ShearData.HorizontalInterfaceZones.begin());
   while(!bdone)
   {
      Float64 zone_end = zone_start + ith->ZoneLength;
      CShearData::HorizontalInterfaceZoneIterator ittest(ith);
      if (zone_end >= zone_max || ++ittest==m_ShearData.HorizontalInterfaceZones.end())
      {
         // we've run of the end of the girder here
         zone_end = zone_max;
         bdone = true;
      }

      Float64 avs_demand = this->GetHorizontalAvsMaxInRange(zone_start, zone_end);

      Float64 avs_provided(0.0), av_onebar(0.0);
      if (ith->BarSize!=matRebar::bsNone)
      {
         const matRebar* pRebar = pool->GetRebar(barType,barGrade,ith->BarSize);
         av_onebar = pRebar->GetNominalArea();
         avs_provided = av_onebar * ith->nBars / ith->BarSpacing;
      }

      if (avs_demand > avs_provided)
      {
         // max stirrup spacing in zone
         Float64 start_location = zone_start < m_GirderLength/2.0 ? zone_start : zone_end;

         // We need to increase capacity. First try bars of the same size/nlegs
         Float64 new_spacing;
         if ( ith->BarSize!=matRebar::bsNone && 
              GetBarSpacingForAvs(avs_demand, max_spacing, ith->BarSize, av_onebar, &new_spacing) )
         {
            ith->BarSpacing = new_spacing; // kept bar size, changed spacing
         }
         else
         {
            // Existing bar size won't work. Try others
            matRebar::Size new_size;
            Float64 new_nlegs;
            Float64 new_av;
            if (GetBarSizeSpacingForAvs(avs_demand, max_spacing, &new_size, &new_nlegs, &new_av, &new_spacing) )
            {
               ith->BarSize     = new_size;
               ith->nBars       = new_nlegs;
               ith->BarSpacing  = new_spacing;
            }
            else
            {
               // There is no bar/legs combination that can satisfy demand - we have failed.
               m_pArtifact->SetOutcome(pgsDesignArtifact::TooManyStirrupsReqd);
               return false;
            }
         }
      }
      else if (avs_demand==0.0 && avs_provided ==0.0)
      {
         // No demand, but we must provide minimum bars/spacing
         avs_demand = 1.0e-4; // tiny demand should give min bar size from our list of choices

         matRebar::Size new_size;
         Float64 new_nlegs;
         Float64 new_av;
         Float64 new_spacing;
         if (GetBarSizeSpacingForAvs(avs_demand, max_spacing, &new_size, &new_nlegs, &new_av, &new_spacing) )
         {
            ith->BarSize     = new_size;
            ith->nBars       = new_nlegs;
            ith->BarSpacing  = new_spacing;
         }
      }


      zone_start = zone_end;
      ith++;
   }

   if (DoDesignFromScratch())
   {
      // See if we can can collapse any duplicate zones for a cleaner design
      bool did_coll=false;
      CShearData::HorizontalInterfaceZoneIterator ith(m_ShearData.HorizontalInterfaceZones.begin());
      CShearData::HorizontalInterfaceZoneIterator itend(m_ShearData.HorizontalInterfaceZones.end());

      CShearData::HorizontalInterfaceZoneVec localHorizZones;

      Uint32  ZNum(0);
      while(ith != itend)
      {
         CHorizontalInterfaceZoneData zdat = *ith;
         if (ZNum==0)
         {
            zdat.ZoneNum = ZNum++;
            localHorizZones.push_back(zdat);
         }
         else
         {
            CHorizontalInterfaceZoneData& rback = localHorizZones.back();
            if (rback.BarSize==zdat.BarSize && rback.BarSpacing==zdat.BarSpacing && rback.nBars==zdat.nBars)
            {
               rback.ZoneLength += ith->ZoneLength;
               did_coll = true; // we collapsed at least one zone
            }
            else
            {
               zdat.ZoneNum = ZNum++;
               localHorizZones.push_back(zdat);
            }
         }

         ith++;
      }

      if (did_coll)
      {
         // copy collapsed zone data to our design
         m_ShearData.HorizontalInterfaceZones = localHorizZones;
      }
   }
   
   return true;
}

bool pgsShearDesignTool::DetailAdditionalSplitting()
{
   if (!DoDesignForSplitting())
   {

      return true;
   }
   else
   {
      pgsSplittingZoneArtifact* pSplittingArtifact = m_StirrupCheckArtifact.GetSplittingZoneArtifact();
      if (pSplittingArtifact->GetIsApplicable())
      {
         // Only design splitting if it is applicable
         GET_IFACE(IBridgeMaterial,pMaterial);
         matRebar::Grade barGrade;
         matRebar::Type barType;
         pMaterial->GetTransverseRebarMaterial(m_Span,m_Girder,barType,barGrade);

         lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
         ATLASSERT(pool != NULL);

         // Determine Av/s required for splitting
         Float64 avs_req = GetAvsReqdForSplitting();

         if (GetDoPrimaryBarsProvideSplittingCapacity())
         {
            // See how much capacity we get from primary bars and subtract from req'd
            Float64 avs_primary = Float64_Max;

            // First look at left (start) end. Use stirrup zone iterator
            LeftRightStirrupZoneIter lr_zone_iter(m_ShearData.ShearZones, m_GirderLength);
            bool bdone = false;
            for (lr_zone_iter.First(); !lr_zone_iter.IsDone() && !bdone; lr_zone_iter.Next())
            {
               StirrupZoneItem sz_item  = lr_zone_iter.CurrentItem();

               if(sz_item.pShearZoneData->VertBarSize != matRebar::bsNone)
               {
                  const matRebar* pRebar = pool->GetRebar(barType,barGrade, sz_item.pShearZoneData->VertBarSize );
                  Float64 avs = pRebar->GetNominalArea() * sz_item.pShearZoneData->nVertBars / sz_item.pShearZoneData->BarSpacing;

                  avs_primary = min(avs_primary, avs);
               }

               if (sz_item.rightEnd >= m_StartSplittingZl)
               {
                  bdone = true; // out of splitting zone - time to quit
               }
            }

            // If not symmetrical, look at right end
            if (!m_ShearData.bAreZonesSymmetrical)
            {
               Float64 right_zoneloc = m_GirderLength - m_EndSplittingZl;
               RightLeftStirrupZoneIter rl_zone_iter(m_ShearData.ShearZones, m_GirderLength);
               bool bdone = false;
               for (rl_zone_iter.First(); !rl_zone_iter.IsDone() && !bdone; rl_zone_iter.Next())
               {
                  StirrupZoneItem sz_item  = rl_zone_iter.CurrentItem();

                  if(sz_item.pShearZoneData->VertBarSize != matRebar::bsNone)
                  {
                     const matRebar* pRebar = pool->GetRebar(barType,barGrade, sz_item.pShearZoneData->VertBarSize );
                     Float64 avs = pRebar->GetNominalArea() * sz_item.pShearZoneData->nVertBars / sz_item.pShearZoneData->BarSpacing;

                     avs_primary = min(avs_primary, avs);
                  }

                  if (sz_item.leftEnd <= right_zoneloc)
                  {
                     bdone = true; // out of splitting zone - time to quit
                  }
               }
            }

            if (avs_primary != Float64_Max)
            {
               avs_req = avs_req - avs_primary; // subtract portion from primary bars
            }
         }

         if (avs_req > 0.0)
         {
            // Additional splitting reinforcement is required
            Float64 splitting_zone_len = max(m_StartSplittingZl, m_EndSplittingZl);

            // See if existing layout does the job. If so, keep it.
            bool use_current(false);
            if(!DoDesignFromScratch())
            {
               if (m_ShearData.SplittingBarSize!=matRebar::bsNone)
               {
                  Float64 szone_length = min(m_ShearData.SplittingZoneLength, splitting_zone_len);

                  const matRebar* pRebar = pool->GetRebar(barType, barGrade, m_ShearData.SplittingBarSize);
                  Float64 sarea = pRebar->GetNominalArea() * m_ShearData.nSplittingBars;

                  if (m_ShearData.SplittingBarSpacing > 0.0)
                  {
                     Float64 cur_av = sarea / m_ShearData.SplittingBarSpacing;

                     use_current = cur_av >= avs_req;
                  }
               }
            }

            if (!use_current)
            {
               // Design new additional splitting bars
               Float64 max_spacing = splitting_zone_len; // don't allow spacing to exceed zone length

               matRebar::Size new_size;
               Float64 new_nlegs;
               Float64 new_av;
               Float64 new_spacing;
               if (GetBarSizeSpacingForAvs(avs_req, splitting_zone_len, &new_size, &new_nlegs, &new_av, &new_spacing) )
               {
                  m_ShearData.SplittingZoneLength = CeilOff(splitting_zone_len-SPACING_TOL, new_spacing); // zone length is multiple of spacing
                  m_ShearData.SplittingBarSize    = new_size;
                  m_ShearData.SplittingBarSpacing = new_spacing;
                  m_ShearData.nSplittingBars      = new_nlegs;
               }
               else
               {
                  // There is no bar/legs combination that can satisfy demand - we have failed.
                  ATLASSERT(0); // suspicious if we ever get here
                  m_pArtifact->SetOutcome(pgsDesignArtifact::TooManyStirrupsReqdForSplitting);
                  return false;
               }
            }
         }
         else
         {
            // No additional splitting needed. Zero out any existing values
            m_ShearData.SplittingZoneLength = 0.0;
            m_ShearData.SplittingBarSize = matRebar::bsNone;
            m_ShearData.SplittingBarSpacing = 0.0;
         }
      }

      return true;
   }
}

bool pgsShearDesignTool::DetailAdditionalConfinement()
{
   if (!DoDesignForConfinement())
   {
      return true;
   }
   else
   {
      const pgsConfinementArtifact& rConfinementArtifact = m_StirrupCheckArtifact.GetConfinementArtifact();
      if (rConfinementArtifact.IsApplicable())
      {
         if (!GetBarsActAsConfinement() || !DoDesignFromScratch())
         {
            // Confinement design may be provided based on existing layout
            GET_IFACE(IBridgeMaterial,pMaterial);
            matRebar::Grade barGrade;
            matRebar::Type barType;
            pMaterial->GetTransverseRebarMaterial(m_Span,m_Girder,barType,barGrade);
            lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
            ATLASSERT(pool != NULL);

            Float64 max_spac = rConfinementArtifact.GetSMax();
            const matRebar* pBar = rConfinementArtifact.GetMinBar();
            Float64 abar_reqd = pBar->GetNominalArea();
            Float64 min_start_zl = rConfinementArtifact.GetStartRequiredZoneLength();

            bool need_additional = false;

            // First look at left (start) end. Use stirrup zone iterator
            LeftRightStirrupZoneIter lr_zone_iter(m_ShearData.ShearZones, m_GirderLength);
            bool bdone = false;
            for (lr_zone_iter.First(); !lr_zone_iter.IsDone() && !bdone; lr_zone_iter.Next())
            {
               StirrupZoneItem sz_item  = lr_zone_iter.CurrentItem();

               if(sz_item.pShearZoneData->ConfinementBarSize == matRebar::bsNone)
               {
                  need_additional = true;
               }
               else if (sz_item.pShearZoneData->BarSpacing > max_spac)
               {
                  need_additional = true;
               }
               else
               {
                  const matRebar* pRebar = pool->GetRebar(barType,barGrade, sz_item.pShearZoneData->VertBarSize );
                  Float64 av = pRebar->GetNominalArea();
                  if (av < abar_reqd)
                  {
                     need_additional = true;
                  }
               }

               if (sz_item.rightEnd >= min_start_zl)
               {
                  bdone = true; // out of conf zone - time to quit
               }
            }

            // If not symmetrical, look at right end
            if (!m_ShearData.bAreZonesSymmetrical)
            {
               Float64 right_zoneloc = m_GirderLength - rConfinementArtifact.GetEndRequiredZoneLength();
               RightLeftStirrupZoneIter rl_zone_iter(m_ShearData.ShearZones, m_GirderLength);
               bool bdone = false;
               for (rl_zone_iter.First(); !rl_zone_iter.IsDone() && !bdone; rl_zone_iter.Next())
               {
                  StirrupZoneItem sz_item  = rl_zone_iter.CurrentItem();

                  if(sz_item.pShearZoneData->ConfinementBarSize == matRebar::bsNone)
                  {
                     need_additional = true;
                  }
                  else if (sz_item.pShearZoneData->BarSpacing > max_spac)
                  {
                     need_additional = true;
                  }
                  else
                  {
                     const matRebar* pRebar = pool->GetRebar(barType,barGrade, sz_item.pShearZoneData->VertBarSize );
                     Float64 av = pRebar->GetNominalArea();
                     if (av < abar_reqd)
                     {
                        need_additional = true;
                     }
                  }

                  if (sz_item.leftEnd <= right_zoneloc)
                  {
                     bdone = true; // out of confinement zone - time to quit
                  }
               }
            }
            
            // Create additional rebar for confinement if needed
            if (need_additional)
            {
               Float64 zone_len = max(min_start_zl, rConfinementArtifact.GetEndRequiredZoneLength());

               // See if there is existing, and if it does the job
               bool use_current(false);
               if (m_ShearData.ConfinementBarSize != matRebar::bsNone)
               {
                  const matRebar* pRebar = pool->GetRebar(barType,barGrade, m_ShearData.ConfinementBarSize );
                  Float64 av = pRebar->GetNominalArea();
                  if (av >= abar_reqd)
                  {
                     if(m_ShearData.ConfinementZoneLength >= zone_len)
                     {
                        if(m_ShearData.ConfinementBarSpacing <= max_spac)
                        {
                           use_current = true;
                        }
                     }
                  }
               }

               if (!use_current)
               {
                  // Try to get smallest bar from available that will do the job. Otherwize use spec size
                  matRebar::Size minSize; 
                  if ( GetMinAvailableBarSize(pBar->GetSize(), barGrade, barType, pool, &minSize) )
                  {
                     m_ShearData.ConfinementBarSize = minSize;
                  }
                  else
                  {
                     m_ShearData.ConfinementBarSize = pBar->GetSize();
                  }

                  m_ShearData.ConfinementZoneLength = CeilOff(zone_len, max_spac); // make zone length a multiple of spacing
                  m_ShearData.ConfinementBarSpacing = max_spac;
               }
            }
         }
      }
   }

   return true;
}

pgsShearDesignTool::ShearDesignOutcome pgsShearDesignTool::DesignLongReinfShear()
{
   // The method here is to perform a check on the current design and use check
   // results to compute if additional long reinf is needed
   GDRCONFIG config = this->GetGirderConfiguration(); // current design

   GET_IFACE(IShearCapacity, pShearCapacity);
   GET_IFACE(IBridge, pBridge);

   const pgsTypes::LimitState limit_states[2]={pgsTypes::StrengthI, pgsTypes::StrengthII};
   Int32 nls = m_bIsPermit ? 2 : 1;

   // Loop over pois between faces of support and CSS
   Float64 startSl = m_StartConnectionLength + pBridge->GetGirderStartSupportWidth(m_Span,m_Girder)/2.0;
   Float64 endSl = m_GirderLength - m_EndConnectionLength - pBridge->GetGirderEndSupportWidth(m_Span,m_Girder)/2.0;

   // We will use #5 bars if using rebar 
   // Compute min development length for this bar size
   Float64 rbfy=1.0;
   Float64 tensile_development_length=0.0;
   if(m_LongShearCapacityIncreaseMethod == GirderLibraryEntry::isAddingRebar)
   {
      GET_IFACE(ILongitudinalRebar,pLongRebar);
      CLongitudinalRebarData lrd = pLongRebar->GetLongitudinalRebarData(m_Span, m_Girder);
      const matRebar* pRebar = lrfdRebarPool::GetInstance()->GetRebar(lrd.BarType,lrd.BarGrade,matRebar::bs5); // #5
      rbfy = pRebar->GetYieldStrength();
      tensile_development_length = lrfdRebar::GetTensileDevelopmentLength( *pRebar, m_pArtifact->GetConcreteStrength() );
      ATLASSERT(tensile_development_length>0.0);
   }

   Float64 As=0.0;
   for(Int32 ils=0; ils<nls; ils++)
   {
      std::vector<pgsPointOfInterest>::const_iterator i( m_DesignPois.begin());
      while ( i != m_DesignPois.end())
      {
         const pgsPointOfInterest& poi = *i;
         Float64 location = poi.GetDistFromStart();

         if ( location>=startSl && location <=endSl )
         {
            SHEARCAPACITYDETAILS scd;
            pShearCapacity->GetShearCapacityDetails( limit_states[ils], pgsTypes::BridgeSite3, poi, config, &scd);

            // We only design for the positive moment case. Users are on their own for negative moment (slab reinforcement)
            if(scd.bTensionBottom)
            {
            // Longitudinal steel check
            pgsLongReinfShearArtifact l_artifact;
            m_pLongReinfShearChecker->CheckLongReinfShear(poi, pgsTypes::BridgeSite3, limit_states[ils], scd, &config, &l_artifact);

            if (!l_artifact.Passed())
            {
               Float64 demand = l_artifact.GetDemandForce();
               Float64 capacity = l_artifact.GetCapacityForce();

               // compute area of rebar or strands required to remedy
               if(m_LongShearCapacityIncreaseMethod == GirderLibraryEntry::isAddingRebar)
               {
                  // Must adjust for development. We are using #5 bars per above
                  Float64 dev_fac(1.0);
                  Float64 min_dist_from_ends = min(location, m_GirderLength-location);
                  if ( min_dist_from_ends <= 0.0 )
                  {
                     // Can't develop rebar
                     m_pArtifact->SetOutcome(pgsDesignArtifact::NoDevelopmentLengthForLongReinfShear);
                     return sdFail;
                  }
                  else if (min_dist_from_ends < tensile_development_length)
                  {
                     dev_fac = min_dist_from_ends / tensile_development_length;
                  }

                  Float64 as = (demand-capacity)/rbfy;
                  if (dev_fac>0.0)
                  {
                     as /= dev_fac;
                  }
                  else
                  {
                     ATLASSERT(0);
                  }

                  As = max(As, as);
               }
               else if(m_LongShearCapacityIncreaseMethod == GirderLibraryEntry::isAddingStrands)
               {
                  Float64 min_dist_from_ends = min(location, m_GirderLength-location);
                  if ( min_dist_from_ends <= 0.0 )
                  {
                     // Can't develop strands
                     m_pArtifact->SetOutcome(pgsDesignArtifact::NoStrandDevelopmentLengthForLongReinfShear);
                     return sdFail;
                  }


                  Float64 fps = l_artifact.GetFps();
                  if (fps==0.0)
                  {
                     fps = ConvertToSysUnits(170.0, unitMeasure::KSI); // No strands exist - just take a shot at a reasonable final
                  }

                  Float64 as = (demand-capacity)/fps;
                  As = max(As, as);
               }
               else
               {
                  ATLASSERT(0);
                  }
               }
            }
         }

         i++;
      }
   }

   if (As > 0.0)
   {
      if (m_bIsLongShearCapacityIncreaseMethodProblem)
      {
         // Can't increase rebar because project criteria won't use it
         m_pArtifact->SetOutcome(pgsDesignArtifact::ConflictWithLongReinforcementShearSpec);
         return sdFail;
      }
      else
      {
         m_LongReinfShearAs = As;
         return (m_LongShearCapacityIncreaseMethod == GirderLibraryEntry::isAddingRebar) ? sdRestartWithAdditionalLongRebar : sdRestartWithAdditionalStrands;
      }
   }
   else
   {
      return sdSuccess;
   }
}

Float64 pgsShearDesignTool::ComputeMaxStirrupSpacing(IndexType PoiIdx)
{
   // Get max allowable spacing from spec check, then check if we are in confinement zone
   const pgsPointOfInterest& poi = m_DesignPois[PoiIdx];
   Float64 location = poi.GetDistFromStart();

   Float64 spec_spacing=Float64_Max;

   // If we are inside of CSS, use location of CSS for stirrup detailing. This gets rid
   // of wierdness happening close to supports - like small negative moment affecting dv
   Float64 detail_loc = location;
   if (location < m_LeftCSS)
   {
      detail_loc = m_LeftCSS;
   }
   else if (location > m_RightCSS)
   {
      detail_loc = m_RightCSS;
   }

   pgsStirrupCheckAtPoisArtifactKey keyI(pgsTypes::BridgeSite3, pgsTypes::StrengthI, detail_loc);
   const pgsStirrupCheckAtPoisArtifact* pStrI_artf = m_StirrupCheckArtifact.GetStirrupCheckAtPoisArtifact(keyI);
   if(pStrI_artf!=NULL)
   {
      const pgsStirrupDetailArtifact* pDet = pStrI_artf->GetStirrupDetailArtifact();
      spec_spacing = pDet->GetSMax();
   }
   else
   {
      ATLASSERT(0); // this should never happen
   }

   if (m_bIsPermit)
   {
      pgsStirrupCheckAtPoisArtifactKey keyII(pgsTypes::BridgeSite3, pgsTypes::StrengthII, detail_loc);
      const pgsStirrupCheckAtPoisArtifact* pStrII_artf = m_StirrupCheckArtifact.GetStirrupCheckAtPoisArtifact(keyII);
      if(pStrII_artf!=NULL)
      {
         const pgsStirrupDetailArtifact* pDet = pStrII_artf->GetStirrupDetailArtifact();
         Float64 spacingII = pDet->GetSMax();

         spec_spacing = min(spec_spacing, spacingII);
      }
      else
      {
         ATLASSERT(0); // this should never happen
      }
   }

   Float64 spacing = spec_spacing;

   // Confinement - use real location
   if (DoDesignForConfinement() && GetBarsActAsConfinement() && IsLocationInConfinementZone(location) )
   {
      // We are in a confinement zone and use primary bars for confinement
      const pgsConfinementArtifact& rConfine = m_StirrupCheckArtifact.GetConfinementArtifact();

      Float64 confine_spacing = rConfine.GetSMax();

      spacing = min(spec_spacing, confine_spacing);
   }

   // Spacing can never exceed max max
   spacing = min(spacing, m_SMaxMax);

   return spacing;
}

Float64 pgsShearDesignTool::ComputeMaxStirrupSpacing(Float64 location)
{
   IndexType poi_idx = GetPoiIdxForLocation(location);

   return ComputeMaxStirrupSpacing(poi_idx);
}

IndexType pgsShearDesignTool::GetPoiIdxForLocation(Float64 location)
{
   // Get index of POI just inboard of location
   IndexType poi_idx;
   if (location <= m_GirderLength/2.0)
   {
      poi_idx = 0;
      for(std::vector<pgsPointOfInterest>::const_iterator it = m_DesignPois.begin(); it!= m_DesignPois.end(); it++)
      {
         const pgsPointOfInterest& rpoi(*it);
         if(rpoi.GetDistFromStart() > location)
         {
            break;
         }

         poi_idx++;
      }
   }
   else
   {
      poi_idx = m_DesignPois.size()-1;
      for(std::vector<pgsPointOfInterest>::const_reverse_iterator it = m_DesignPois.rbegin(); it!= m_DesignPois.rend(); it++)
      {
         const pgsPointOfInterest& rpoi(*it);
         if(rpoi.GetDistFromStart() < location)
         {
            break;
         }

         poi_idx--;
      }
   }

   return poi_idx;
}

Float64 pgsShearDesignTool::GetMinStirrupSpacing(matRebar::Size size)
{
   GET_IFACE(ITransverseReinforcementSpec,pTransverseReinforcementSpec);
   GET_IFACE(IBridgeMaterial,pMaterial);
   matRebar::Grade barGrade;
   matRebar::Type barType;
   pMaterial->GetTransverseRebarMaterial(m_Span,m_Girder,barType,barGrade);

   lrfdRebarPool* prp = lrfdRebarPool::GetInstance();
   const matRebar* pRebar = prp->GetRebar(barType,barGrade,size);

   Float64 db = pRebar->GetNominalDimension();
   Float64 as = pMaterial->GetMaxAggrSizeGdr(m_Span,m_Girder);
   Float64 s_min = pTransverseReinforcementSpec->GetMinStirrupSpacing(as, db);

   return s_min;
}

bool pgsShearDesignTool::GetBarSizeSpacingForAvs(Float64 avsDemand, Float64 maxSpacing, 
                                                 matRebar::Size* pSize, Float64* pNLegs, Float64* pAv, Float64* pSpacing)
{
   // Cycle through available bars/legs (already in order of user precedence)
   ATLASSERT(!m_BarLegCollection.empty());
   ATLASSERT(!m_AvailableBarSpacings.empty());

   for(BarLegCollectionIterator it=m_BarLegCollection.begin(); it!=m_BarLegCollection.end(); it++)
   {
      const BarLegCombo& barlegs = *it;
      Float64 Av = barlegs.m_Av;
      ATLASSERT(Av>0.0);

      Float64 min_spacing = GetMinStirrupSpacing( barlegs.m_Size );

      // Find largest available spacing from user list that meets requirements
      for(std::vector<Float64>::const_reverse_iterator sit = m_AvailableBarSpacings.rbegin(); sit!=m_AvailableBarSpacings.rend(); sit++)
      {
         Float64 spac = *sit;

         // If we fit in spacing requirements, 
         if (spac+SPACING_TOL>=min_spacing && spac<=maxSpacing+SPACING_TOL)
         {
            // and we satisify avs demand,
            if (Av/spac >= avsDemand)
            {
               // then we are the best available candidate.
               *pSize    = barlegs.m_Size;
               *pNLegs   = barlegs.m_Legs;
               *pAv      = Av;
               *pSpacing = spac;
               return true;
            }
         }
      }
   }

   *pSize    = matRebar::bsNone;
   return false;
}

bool pgsShearDesignTool::GetMinAvailableBarSize(matRebar::Size minSize, matRebar::Grade barGrade, matRebar::Type barType, lrfdRebarPool* pool, matRebar::Size* pSize)
{
   // This should be made more efficient if it's called frequently
   const matRebar* pRebar = pool->GetRebar(barType,barGrade, minSize);
   Float64 minArea = pRebar->GetNominalArea();

   // Need a sorted by area list of available bars
   struct SizeArea
   {
      matRebar::Size Size;
      Float64 Area;

      const bool operator < (const SizeArea& rArea)
      {
         return Area < rArea.Area;
      }
   };

   std::vector<SizeArea> availableSizes;
   for(BarLegCollectionIterator it=m_BarLegCollection.begin(); it!=m_BarLegCollection.end(); it++)
   {
      SizeArea sa;
      sa.Size = it->m_Size;

      pRebar = pool->GetRebar(barType,barGrade, sa.Size);
      sa.Area = pRebar->GetNominalArea();

      availableSizes.push_back(sa);
   }

   std::sort(availableSizes.begin(), availableSizes.end());

   // Have sorted list, now we can find a bar that fits our needs
   for (std::vector<SizeArea>::iterator ita=availableSizes.begin(); ita!=availableSizes.end(); ita++)
   {
      if(ita->Area >= minArea)
      {
         *pSize = ita->Size;
         return true;
      }
   }

   // No Luck
   *pSize = matRebar::bsNone;
   return false;
}

bool pgsShearDesignTool::GetBarSpacingForAvs(Float64 avsDemand, Float64 maxSpacing, matRebar::Size Size, Float64 Av, Float64* pSpacing)
{
   Float64 spacing(0.0);

   Float64 min_spacing = GetMinStirrupSpacing( Size );

   // Available spacings as defined by user
   for(std::vector<Float64>::const_reverse_iterator sit = m_AvailableBarSpacings.rbegin(); sit!=m_AvailableBarSpacings.rend(); sit++)
   {
      Float64 spac = *sit;

      // If we fit in spacing requirements, 
      if (spac+SPACING_TOL>=min_spacing && spac<=maxSpacing+SPACING_TOL)
      {
         // and we satisify avs demand,
         if (Av/spac >= avsDemand)
         {
            *pSpacing = spac;
            return true;
         }
      }
   }

   *pSpacing  = 0.0;
   return false; // no spacing works
}

Float64 pgsShearDesignTool::ComputeMaxNextSpacing(Float64 currentSpacing)
{
   // Get a candidate spacing from required criteria, then get nearest spacing from our list
   Float64 candidate = Min3(currentSpacing + m_MaxSpacingChangeInZone, 
                            currentSpacing * (1 + m_MaxShearCapacityChangeInZone),
                            m_SMaxMax);

   return GetDesignSpacingFromList(candidate);
}

Float64 pgsShearDesignTool::GetDesignSpacingFromList(Float64 spacing)
{
   if (spacing < m_AvailableBarSpacings.front())
   {
      // Test spacing is tighter than any we have. just return spacing
      return spacing;
   }
   else
   {
      // Get largest spacing in list that is <= our test value
      for (std::vector<Float64>::reverse_iterator rit = m_AvailableBarSpacings.rbegin(); rit!=m_AvailableBarSpacings.rend(); rit++)
      {
         Float64 spac = *rit;
         if (spac <= spacing+SPACING_TOL)
         {
            return spac; // success
         }
      }
   }
   ATLASSERT(0); // wtf?
   return 0.0;
}

bool pgsShearDesignTool::IsLocationInConfinementZone(Float64 distFromStart)
{
   return distFromStart < m_StartConfinementZl+SPACING_TOL || 
          m_GirderLength-distFromStart < m_EndConfinementZl+SPACING_TOL;
}

Float64 pgsShearDesignTool::GetAvsReqdForSplitting() const
{
   ATLASSERT(DoDesignForSplitting());

   const pgsSplittingZoneArtifact* pSplittingArtifact = m_StirrupCheckArtifact.GetSplittingZoneArtifact();
   Float64 avs_req_start = pSplittingArtifact->GetStartSplittingForce() / (pSplittingArtifact->GetStartFs() * m_StartSplittingZl);
   Float64 avs_req_end   = pSplittingArtifact->GetEndSplittingForce()   / (pSplittingArtifact->GetEndFs()   * m_EndSplittingZl);

   Float64 avs_req = max(avs_req_start, avs_req_end);
   return avs_req;
}
