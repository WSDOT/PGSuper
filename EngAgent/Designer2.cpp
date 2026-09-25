///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2026  Washington State Department of Transportation
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
#include "EngAgent.h"
#include <PgsExt\PgsExt.h>
#include <IFace\Bridge.h>
#include <IFace\Alignment.h>
#include <IFace\DistributionFactors.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <EAF/EAFStatusCenter.h>
#include <EAF/EAFUIIntegration.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace/Limits.h>
#include <IFace\PrestressForce.h>
#include <IFace\MomentCapacity.h>
#include <IFace\ShearCapacity.h>
#include <IFace\Constructability.h>
#include <IFace\TransverseReinforcementSpec.h>
#include <IFace\SplittingChecks.h>
#include <IFace\HorizontalTensionTieChecks.h>
#include <IFace\PrecastIGirderDetailsSpec.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\GirderHandling.h>
#include <IFace\ResistanceFactors.h>
#include <IFace\InterfaceShearRequirements.h>
#include <IFace\Intervals.h>

#include <IFace\DocumentType.h>

#include <EAF/AutoProgress.h>

#include "Designer2.h"
#include "PsForceEng.h"
#include "HorizTieForceEng.h"
#include "GirderHandlingChecker.h"
#include "GirderLiftingChecker.h"
#include "PrincipalWebStressEngineer.h"
#include <PgsExt\DesignConfigUtil.h>
#include <PgsExt\StabilityAnalysisPoint.h>
#include <PgsExt\EngUtil.h>

#include <WBFLGenericBridgeTools\AlternativeTensileStressCalculator.h>

#include "StatusItems.h"
#include <PgsExt\StatusItem.h>


#include <PGSuperException.h>

#include <Units\Convert.h>

#include <LRFD\Rebar.h>
#include <algorithm>
#include <iterator>

#include <MathEx.h>

#include <PsgLib\BridgeDescription2.h>
#include <PsgLib\LoadFactors.h>
#include <PsgLib\GirderLabel.h>

#include <PsgLib\GirderLibraryEntry.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\ConcreteLibraryEntry.h>

#include <psgLib/SpecificationCriteria.h>
#include <psgLib/InterfaceShearCriteria.h>
#include <psgLib/ShearCapacityCriteria.h>
#include <psgLib/HaunchCriteria.h>
#include <psgLib/BottomFlangeClearanceCriteria.h>
#include <psgLib/LiveLoadDeflectionCriteria.h>
#include <psgLib/GirderInclinationCriteria.h>
#include <psgLib/PlantHandlingCriteria.h>

#if defined _DEBUG
#include <IFace\PointOfInterest.h>
#endif // _DEBUG



#define MIN_SPAN_DEPTH_RATIO 4

#define WITHOUT_REBAR 0
#define WITH_REBAR    1
#define TOP           0
#define BOT           1

static Float64 gs_60KSI = WBFL::Units::ConvertToSysUnits(60.0,WBFL::Units::Measure::KSI);
static Float64 gs_rowToler = WBFL::Units::ConvertToSysUnits(0.25,WBFL::Units::Measure::Inch); // strands are in same row if this tolerance

// Exception-safe utility class for reverting event holding and LLDF ROA during design
class AutoDesign
{
public:
   AutoDesign(std::shared_ptr<IEvents> pEvents, std::shared_ptr<ILiveLoads> pLiveLoads):
      m_pEvents(pEvents),
      m_pLiveLoads(pLiveLoads)
   {
      pEvents->HoldEvents();
      m_OldRoa = pLiveLoads->GetRangeOfApplicabilityAction();
      if (m_OldRoa == WBFL::LRFD::RangeOfApplicabilityAction::Enforce)
      {
         pLiveLoads->SetRangeOfApplicabilityAction(WBFL::LRFD::RangeOfApplicabilityAction::Ignore);
      }
   }

   ~AutoDesign()
   {
      m_pLiveLoads->SetRangeOfApplicabilityAction(m_OldRoa);
      m_pEvents->CancelPendingEvents();
   }

private:
   std::shared_ptr<IEvents> m_pEvents;
   std::shared_ptr<ILiveLoads> m_pLiveLoads;
   WBFL::LRFD::RangeOfApplicabilityAction m_OldRoa;
};

bool CanDesign(pgsTypes::StrandDefinitionType type)
{
   // we can only design for these strand definition types
   // Technically, we should not design for sdtDirectionSelection, however the designer does work. The "gotcha" is that the
   // strand defintion type gets changed to sdtStraightHarped when it should not... design should not change the strand definition type
   // but since sdtDirectSelection has been a valid choice for a long time and it does work, we'll let it go
   return (type == pgsTypes::sdtTotal || type == pgsTypes::sdtStraightHarped || type == pgsTypes::sdtDirectSelection) ? true : false;
}

/****************************************************************************
CLASS
   pgsDesigner2
****************************************************************************/

#if defined ENABLE_DESIGN_LOGGING
const std::_tstring g_LimitState[] =
{
   std::_tstring(_T("ServiceI")),
   std::_tstring(_T("ServiceIA")),
   std::_tstring(_T("ServiceIII")),
   std::_tstring(_T("StrengthI")),
   std::_tstring(_T("StrengthII")),
   std::_tstring(_T("FatigueI"))
};

// order must match pgsTypes::StressType { Compression, Tension } (PGSuperTypes.h) - this was previously
// {Tension, Compression}, backwards, silently mislabeling every Tension/Compression DLOG() header in
// Designer_x64.log. Purely cosmetic (task.stressType == pgsTypes::Compression comparisons elsewhere in
// this file use the enum directly and were never affected), but very misleading when reading the log.
const std::_tstring g_Type[] =
{
   std::_tstring(_T("Compression")),
   std::_tstring(_T("Tension"))
};

inline std::_tstring StrTopBot(pgsTypes::StressLocation sl)
{
   return (sl==pgsTypes::BottomGirder ? _T(" Bottom of Girder") : _T(" Top of Girder"));
}
#endif


// utilities dealing with proportioning harped and straight strands
// ecc to control top tension stress
inline Float64 ComputeTopTensionEccentricity( Float64 Pps, Float64 allTens, Float64 Fexternal, Float64 Ag, Float64 Stg)
{
   return (-Pps/Ag - allTens + Fexternal)*Stg/Pps;
}

// ecc to control bottom compression
inline Float64 ComputeBottomCompressionEccentricity( Float64 Pps, Float64 allComp, Float64 Fexternal, Float64 Ag, Float64 Sbg)
{
   return (-Pps/Ag - allComp + Fexternal)*Sbg/Pps;
}

inline Float64 compute_required_eccentricity(Float64 P,Float64 A,Float64 S,Float64 fDL,Float64 fHP)
{
   return -(fHP - fDL + P/A)*(S/P);
}

void GetConfinementZoneLengths(const CSegmentKey& segmentKey, std::shared_ptr<IGirder> pGdr, Float64 gdrLength, 
                                      Float64* pZoneFactor, Float64* pStartd, Float64* pEndd,
                                      Float64* pStartLength, Float64* pEndLength)
{
   // NOTE: This d is defined differently than in 5.10.10.2 of the 2nd 
   //       edition of the spec (after 2017, 5.9.4.4.2). We think what they really meant to say 
   //       was d = the overall depth of the precast member.
   // Get height at appropriate end of girder
   *pZoneFactor = 1.5;
   *pStartd = pGdr->GetHeight( pgsPointOfInterest(segmentKey, 0.0) );
   *pStartLength = 1.5 * (*pStartd);

   *pEndd = pGdr->GetHeight( pgsPointOfInterest(segmentKey, gdrLength) );
   *pEndLength = 1.5 * (*pEndd);
}

class MatchPoiOffSegment
{
public:
   MatchPoiOffSegment(std::shared_ptr<IPointOfInterest> pIPointOfInterest) : m_pIPointOfInterest(pIPointOfInterest) {}
   bool operator()(const pgsPointOfInterest& poi) const
   {
      return m_pIPointOfInterest->IsOffSegment(poi);
   }

   std::shared_ptr<IPointOfInterest> m_pIPointOfInterest;
};

////////////////////////// PUBLIC     ///////////////////////////////////////



//======================== LIFECYCLE  =======================================
pgsDesigner2::pgsDesigner2(std::weak_ptr<WBFL::EAF::Broker> pBroker, StatusGroupIDType statusGroupID) :
   m_pBroker(pBroker), m_StatusGroupID(statusGroupID), m_ShearDesignTool(DESIGN_LOGGER)
{
   m_StrandDesignTool = std::make_shared<pgsStrandDesignTool>(DESIGN_LOGGER);
   m_bShippingDesignIgnoreConfigurationLimits = false;

   GET_IFACE2(GetBroker(), IEAFStatusCenter, pStatusCenter);
   m_scidLiveLoad = pStatusCenter->RegisterCallback(std::make_shared<pgsLiveLoadStatusCallback>());
   m_scidBridgeDescriptionError = pStatusCenter->RegisterCallback(std::make_shared<pgsBridgeDescriptionStatusCallback>(WBFL::EAF::StatusSeverityType::Error));

   // The designer log is opened when a design starts (see OpenDesignLog), when the project name is known
}

pgsDesigner2::pgsDesigner2(const pgsDesigner2& rOther):
m_ShearDesignTool(DESIGN_LOGGER)
{
   m_StrandDesignTool = std::make_shared<pgsStrandDesignTool>(DESIGN_LOGGER);
   MakeCopy(rOther);
}

pgsDesigner2::~pgsDesigner2()
{
   m_Log.Close();
}

//======================== OPERATORS  =======================================
pgsDesigner2& pgsDesigner2::operator= (const pgsDesigner2& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

// Function we need to reuse
static Float64 GetSectionGirderOrientationEffect(const pgsPointOfInterest& poi, Float64 x, Float64 z, MatingSurfaceIndexType nMatingSurfaces,
                                          Float64 topWidth, Float64 girderTopSlope,
                                          std::shared_ptr<IRoadway> pAlignment, std::shared_ptr<IBridge> pBridge, std::shared_ptr<IGirder> pGdr,
                                          Float64* pCrownSlope)
{
   // for complex girder/roadway configurations, the actual girder orientation effect is the vertical distance between the
   // roadway surface and the mating surface profile at the CL web where the "A" dimension is measured.
   // this calculation isn't that sophisticated

   // girder orientation effect
   *pCrownSlope = 0;
   Float64 pivot_crown = 0; // accounts for the pivot point being over a girder
   if ( nMatingSurfaces == 1 )
   {
      // single top flange situation
      // to account for the case when the pivot point is over the girder, compute an
      // average crown slope based on the elevation at the flange tips
      Float64 Wtf = pGdr->GetTopWidth(poi);

      Float64 ya_left  = pAlignment->GetElevation(x,z - Wtf /2);
      Float64 ya_right = pAlignment->GetElevation(x,z + Wtf /2);

      *pCrownSlope = (ya_right - ya_left)/ Wtf;

      Float64 ya = pAlignment->GetElevation(x,z);
      if ( (ya_left < ya && ya_right < ya) || (ya < ya_left && ya < ya_right) )
      {
         pivot_crown = ya - (ya_left+ya_right)/2;
      }

      CComPtr<IPoint2dCollection> matingSurfaceProfile;
      bool bHasMSProfile = (pGdr->GetMatingSurfaceProfile(poi, 0, true, &matingSurfaceProfile) == false || matingSurfaceProfile == nullptr) ? false : true;
      if (bHasMSProfile)
      {
         IndexType nPoints;
         matingSurfaceProfile->get_Count(&nPoints);
         if (nPoints == 3)
         {
            // the top surface has a crown point in it (this is probably a deck bulb tee girder)
            CComPtr<IPoint2d> pnt1, pnt2, pnt3;
            matingSurfaceProfile->get_Item(0, &pnt1);
            matingSurfaceProfile->get_Item(1, &pnt2);
            matingSurfaceProfile->get_Item(2, &pnt3);

            // this is kind of a hack and doesn't work perfectly, but will cover the common case of the
            // roadway cross section crown point on the CL of the girder

            // if the change horizontal distance between the roadway surface points is equal to the
            // horizontal distance between the mating surface points, and the vertical elevation changes
            // are also the same, the mating surface has a profile that is parallel to the roadway surface
            // so pivot_crown should be zero
            Float64 x1, y1;
            pnt1->Location(&x1, &y1);

            Float64 x2, y2;
            pnt2->Location(&x2, &y2);

            Float64 x3, y3;
            pnt3->Location(&x3, &y3);

            Float64 dxl1 = x2 - x1;
            Float64 dyl1 = y2 - y1;
            Float64 dxl2 = Wtf / 2;
            Float64 dyl2 = ya - ya_left;

            Float64 dxr1 = x3 - x2;
            Float64 dyr1 = y3 - y2;
            Float64 dxr2 = Wtf / 2;
            Float64 dyr2 = ya_right - ya;

            if (IsEqual(dxl1, dxl2) && IsEqual(dyl1, dyl2) && IsEqual(dxr1, dxr2) && IsEqual(dyr1, dyr2))
            {
               pivot_crown = 0;
            }
         }
      }
   }
   else
   {
      // multiple mating surfaces (like a U-beam)
      // If there is a pivot point in the profile grade between the exterior mating surfaces
      // it is unclear which crown slope to use... to work around this, we will use the 
      // slope of the line connecting the two exterior mating surfaces
      ATLASSERT( 2 <= nMatingSurfaces );

      // this is at CL mating surface... we need out to out
      Float64 left_mating_surface_offset  = pGdr->GetMatingSurfaceLocation(poi,0);
      Float64 right_mating_surface_offset = pGdr->GetMatingSurfaceLocation(poi,nMatingSurfaces-1);

      // width of mating surface
      Float64 left_mating_surface_width  = pGdr->GetMatingSurfaceWidth(poi,0);
      Float64 right_mating_surface_width = pGdr->GetMatingSurfaceWidth(poi,nMatingSurfaces-1);

      // add half the width to get the offset to the outside edge of the top of the section
      left_mating_surface_offset  += ::BinarySign(left_mating_surface_offset) * left_mating_surface_width/2;
      right_mating_surface_offset += ::BinarySign(right_mating_surface_offset)* right_mating_surface_width/2;

      Float64 ya_left  = pAlignment->GetElevation(x,z+left_mating_surface_offset);
      Float64 ya_right = pAlignment->GetElevation(x,z+right_mating_surface_offset);

      *pCrownSlope = (ya_right - ya_left)/(right_mating_surface_offset - left_mating_surface_offset);

      Float64 ya = pAlignment->GetElevation(x,z);
      if ( (ya_left < ya && ya_right < ya) || (ya < ya_left && ya < ya_right) )
      {
         pivot_crown = ya - (ya_left+ya_right)/2;
      }
   }

   Float64 section_girder_orientation_effect = pivot_crown + (topWidth/2)*(fabs(*pCrownSlope - girderTopSlope)/(sqrt(1+ girderTopSlope*girderTopSlope)));

   return section_girder_orientation_effect;
}

void pgsDesigner2::GetSlabOffsetDetails(const CSegmentKey& segmentKey,const GDRCONFIG* pConfig,SLABOFFSETDETAILS* pSlabOffsetDetails) const
{
   GET_IFACE2(GetBroker(),ICamber,pCamber);
   GET_IFACE2(GetBroker(),IPointOfInterest,pPoi);
   GET_IFACE2(GetBroker(),IBridge,pBridge);
   GET_IFACE2(GetBroker(),IRoadway,pAlignment);
   GET_IFACE2(GetBroker(),IGirder,pGdr);
   GET_IFACE2_NOCHECK(GetBroker(),IBridgeDescription,pIBridgeDesc);

   GET_IFACE2(GetBroker(),ILibrary, pLib );
   GET_IFACE2(GetBroker(),ISpecification, pSpec );
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   pSlabOffsetDetails->SlabOffset.clear();
   pSlabOffsetDetails->SlabOffset.reserve(11);

   // Slab offset is measured at the CL Bearing of segments in the erected state
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT | POI_TENTH_POINTS, &vPoi);
   ATLASSERT(11 == vPoi.size());

   const pgsPointOfInterest& clBrgPoi = vPoi.front();

   PoiList vEndPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_START_FACE | POI_END_FACE, &vEndPoi);
   ATLASSERT(vEndPoi.size() == 2);
   const pgsPointOfInterest& poi_left(vEndPoi.front());
   const pgsPointOfInterest& poi_right(vEndPoi.back());

   //
   // Profile Effects and Girder Orientation Effects
   //

   // get station and offset at CL Bearing poi
   Float64 station,offset;
   pBridge->GetStationAndOffset(clBrgPoi,&station,&offset);
   offset = IsZero(offset) ? 0 : offset;

   // the profile chord reference line passes through the deck at this station and offset
   Float64 Y_girder_ref_line_left_bearing = pAlignment->GetElevation(station,offset);

   MatingSurfaceIndexType nMatingSurfaces = pGdr->GetMatingSurfaceCount(segmentKey);

   Float64 girder_top_slope = pGdr->GetTransverseTopFlangeSlope(segmentKey);

   Float64 max_tslab_and_fillet = 0;

   std::unique_ptr<WBFL::Math::Function> topFlangeShape; // function that models the longitudinal top flange shape
   pgsTypes::TopFlangeThickeningType tftType = pGdr->GetTopFlangeThickeningType(segmentKey);
   Float64 tft = pGdr->GetTopFlangeThickening(segmentKey);
   if (tftType != pgsTypes::tftNone && !IsZero(tft))
   {
      // this is non-zero thickening of the top flange. assume its longitudinal shape to be a parabola

      Float64 sign = (tftType == pgsTypes::tftEnds ? -1 : 1); // thickening at the ends, it is a concave parabola, otherwise a convex parabola

      // there is an imposed camber and/or top flange thickening. use its shape, excluding natural camber, for the top of the girder
      // create the parabola
      topFlangeShape = std::make_unique<WBFL::Math::PolynomialFunction>(WBFL::Math::GenerateParabola(poi_left.GetDistFromStart(), poi_right.GetDistFromStart(), sign*tft));
   }
   else
   {
      // top flange is straight
      topFlangeShape = std::make_unique<WBFL::Math::ZeroFunction>();
   }

   // the amount of top flange thickening at the start CL Bearing
   Float64 tftCLBrg = topFlangeShape->Evaluate(clBrgPoi.GetDistFromStart());

   GET_IFACE2(GetBroker(),IIntervals, pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType gceInterval = pIntervals->GetGeometryControlInterval();

   Float64 overlay_depth = pBridge->GetOverlayDepth(gceInterval);

   GET_IFACE2(GetBroker(),IDeformedGirderGeometry,pDeformedGirderGeometry);

   // determine the minumum and maximum difference in elevation between the
   // roadway surface and the top of the segment.... measured directly above 
   // the top of the segment
   Float64 diff_min =  DBL_MAX;
   Float64 diff_max = -DBL_MAX;
   Float64 min_haunch =  DBL_MAX;
   Float64 max_haunch = -DBL_MAX;
   Float64 max_reqd_slab_offset = -DBL_MAX;
   Float64 min_reqd_slab_offset =  DBL_MAX;
   for( const pgsPointOfInterest& poi : vPoi)
   {
      Float64 tSlab = pBridge->GetGrossSlabDepth( poi );

      Float64 fillet = pIBridgeDesc->GetFillet();

      Float64 D, C;
      Float64 camber_effect = pCamber->GetExcessCamberEx(poi, pgsTypes::CreepTime::Max, &D, &C, pConfig );
      ATLASSERT(IsEqual(camber_effect,D-C));

      Float64 top_flange_shape_effect = topFlangeShape->Evaluate(poi.GetDistFromStart()) - tftCLBrg;

      Float64 top_width = pGdr->GetTopWidth(poi);

      // top of girder elevation, including camber effects
      Float64 elev_top_girder = pDeformedGirderGeometry->GetTopGirderElevation(poi,pConfig);

      // get station and normal offset for this poi
      Float64 station, offset;
      pBridge->GetStationAndOffset(poi,&station,&offset);
      offset = IsZero(offset) ? 0 : offset;

      // top of girder elevation (ignoring camber effects)
      Float64 yc = pGdr->GetProfileChordElevation(poi);

      // top of alignment elevation above girder
      Float64 ya = pAlignment->GetElevation(station,offset);

      // profile effect
      Float64 section_profile_effect = yc - ya;
      diff_min = Min(diff_min,-section_profile_effect);
      diff_max = Max(diff_max,-section_profile_effect);

      // girder orientation effect
      Float64 crown_slope;
      Float64 section_girder_orientation_effect = ::GetSectionGirderOrientationEffect(poi, station, offset, nMatingSurfaces, top_width, girder_top_slope,
                                                                                      pAlignment, pBridge, pGdr, 
                                                                                      &crown_slope);

      Float64 elev_adj = pBridge->GetElevationAdjustment(erectionIntervalIdx, poi);

      SLAB_OFFSET_AT_SECTION slab_offset;
      slab_offset.PointOfInterest = poi;
      slab_offset.Station = station;
      slab_offset.Offset = offset;
      slab_offset.ElevGirderChord = yc;
      slab_offset.ElevAlignment = ya;
      slab_offset.ProfileEffect = section_profile_effect;
      slab_offset.D = D;
      slab_offset.C = C;
      slab_offset.CamberEffect = camber_effect;
      slab_offset.CrownSlope = crown_slope;
      slab_offset.GirderTopSlope = girder_top_slope;
      slab_offset.Fillet = fillet;
      slab_offset.GirderOrientationEffect = section_girder_orientation_effect;
      slab_offset.TopFlangeShapeEffect = top_flange_shape_effect;
      slab_offset.tSlab = tSlab;
      slab_offset.Wtop = top_width;
      slab_offset.ElevTopGirder = elev_top_girder;
      slab_offset.TopSlabToTopGirder = slab_offset.ElevAlignment - slab_offset.ElevTopGirder - overlay_depth;
      slab_offset.ElevAdjustment = elev_adj;

      slab_offset.RequiredSlabOffsetRaw = tSlab + fillet + section_profile_effect + section_girder_orientation_effect + camber_effect + top_flange_shape_effect;
      // the required slab offset at this section is measured relative to a horizontal line at the start of the segment
      // it should be measured relative to a line that is basically parallel to the girder
      // for this reason, we subtract off the elevation adjustment
      slab_offset.RequiredSlabOffsetRaw -= elev_adj;

      max_reqd_slab_offset = Max(max_reqd_slab_offset, slab_offset.RequiredSlabOffsetRaw);
      min_reqd_slab_offset = Min(min_reqd_slab_offset, slab_offset.RequiredSlabOffsetRaw);

      pSlabOffsetDetails->SlabOffset.push_back(slab_offset);

      max_tslab_and_fillet = Max(max_tslab_and_fillet,tSlab + fillet);

      // store min and max haunch depths
      Float64 haunch_depth = slab_offset.TopSlabToTopGirder - tSlab;
      min_haunch = Min(min_haunch, haunch_depth);
      max_haunch = Max(max_haunch, haunch_depth);
   } // next POI

   // profile effect
   Float64 profile_effect = 0;
   if ( diff_min < 0 ) // there is a sag in the profile
   {
      profile_effect = -diff_min; // raise haunch to accommodate
   }
   else
   {
      profile_effect = -diff_max; // there is a crown in the profile.... lower the haunch
   }

   // Check against minimum slab offset
   // This could happen if there was little camber, little cross slope, and a large crown
   if ( max_reqd_slab_offset < max_tslab_and_fillet )
   {
      max_reqd_slab_offset = max_tslab_and_fillet;
   }

   // record controlling values
   pSlabOffsetDetails->RequiredMaxSlabOffsetRaw = max_reqd_slab_offset;

   pSlabOffsetDetails->RequiredMaxSlabOffsetRounded  = RoundSlabOffsetValue(pSpec, max_reqd_slab_offset);

   // this is the maximum difference in the haunch depth along the girder...
   // if this too big, stirrups may need to be adjusted
   pSlabOffsetDetails->HaunchDiff = max_haunch - min_haunch;
}

Float64 pgsDesigner2::GetSectionGirderOrientationEffect(const pgsPointOfInterest& poi) const
{
   GET_IFACE2(GetBroker(),IBridge,pBridge);
   GET_IFACE2(GetBroker(),IRoadway,pAlignment);
   GET_IFACE2(GetBroker(),IGirder,pGdr);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   MatingSurfaceIndexType nMatingSurfaces = pGdr->GetMatingSurfaceCount(segmentKey);

   Float64 girder_orientation = pGdr->GetOrientation(segmentKey);

   Float64 top_width = pGdr->GetTopWidth(poi);

   // get station and normal offset for this poi
   Float64 x,z;
   pBridge->GetStationAndOffset(poi,&x,&z);
   z = IsZero(z) ? 0 : z;

   // girder orientation effect
   Float64 crown_slope;
   Float64 section_girder_orientation_effect = ::GetSectionGirderOrientationEffect(poi, x, z, nMatingSurfaces, top_width, girder_orientation,
                                                                                   pAlignment, pBridge, pGdr, 
                                                                                   &crown_slope);
   return section_girder_orientation_effect;
}

void pgsDesigner2::ClearArtifacts()
{
   m_CheckArtifacts.clear();
   m_LiftingCheckArtifacts.clear();
   m_HaulingAnalysisArtifacts.clear();
}

const pgsGirderArtifact* pgsDesigner2::GetGirderArtifact(const CGirderKey& girderKey) const
{
   auto found = m_CheckArtifacts.find(girderKey);
   if ( found != m_CheckArtifacts.cend() )
   {
      const auto& pArtifact = found->second;
      return pArtifact.get();
   }

   return nullptr;
}

std::shared_ptr<const WBFL::Stability::LiftingCheckArtifact> pgsDesigner2::GetLiftingCheckArtifact(const CSegmentKey& segmentKey) const
{
   auto found = m_LiftingCheckArtifacts.find(segmentKey);
   if ( found != m_LiftingCheckArtifacts.end() )
   {
      return (found->second);
   }

   return nullptr;
}

std::shared_ptr<const pgsHaulingAnalysisArtifact> pgsDesigner2::GetHaulingAnalysisArtifact(const CSegmentKey& segmentKey) const
{
   auto found = m_HaulingAnalysisArtifacts.find(segmentKey);
   if ( found != m_HaulingAnalysisArtifacts.end() )
   {
      return found->second;
   }

   return nullptr;
}

std::shared_ptr<const WBFL::Stability::LiftingCheckArtifact> pgsDesigner2::CheckLifting(const CSegmentKey& segmentKey) const
{
   // if we already have the artifact, return it
   auto pLiftingArtifact = GetLiftingCheckArtifact(segmentKey);
   if ( pLiftingArtifact )
   {
      return pLiftingArtifact;
   }

   // Nope... need to compute it
   pgsGirderLiftingChecker lifting_checker(m_pBroker,m_StatusGroupID);
   auto lifting_artifact = lifting_checker.CheckLifting(segmentKey);

   m_LiftingCheckArtifacts.insert(std::make_pair(segmentKey,lifting_artifact));

   return lifting_artifact;
}

std::shared_ptr<const pgsHaulingAnalysisArtifact> pgsDesigner2::CheckHauling(const CSegmentKey& segmentKey) const
{
   return CheckHauling(segmentKey,DESIGN_LOGGER);
}

std::shared_ptr<const pgsHaulingAnalysisArtifact> pgsDesigner2::CheckHauling(const CSegmentKey& segmentKey, DESIGN_SHARED_LOGFILE DESIGN_LOGFILE) const
{
   // if we already have the artifact, return it
   auto pHaulingArtifact = GetHaulingAnalysisArtifact(segmentKey);
   if ( pHaulingArtifact )
   {
      return pHaulingArtifact;
   }

   // Nope... need to compute it

   // Use factory function to create correct hauling checker
   pgsGirderHandlingChecker checker_factory(m_pBroker,m_StatusGroupID);
   std::unique_ptr<pgsGirderHaulingChecker> hauling_checker( checker_factory.CreateGirderHaulingChecker() );

   pHaulingArtifact = hauling_checker->CheckHauling(segmentKey,DESIGN_LOGFILE);
         
   m_HaulingAnalysisArtifacts.insert(std::make_pair(segmentKey,pHaulingArtifact));

   pHaulingArtifact = GetHaulingAnalysisArtifact(segmentKey);
   ATLASSERT(pHaulingArtifact != nullptr);
   return pHaulingArtifact;
}

const pgsGirderArtifact* pgsDesigner2::Check(const CGirderKey& girderKey) const
{
   // if we already have the artifact, return it
   const pgsGirderArtifact* pTheGdrArtifact = GetGirderArtifact(girderKey);
   if ( pTheGdrArtifact )
   {
      return pTheGdrArtifact;
   }

   // Nope... create the artifact

   USES_CONVERSION;

   // must be checking a specific girder
   ASSERT_GIRDER_KEY(girderKey);

   GET_IFACE2(GetBroker(),IEAFProgress, pProgress);
   WBFL::EAF::AutoProgress ap(pProgress);

   GET_IFACE2(GetBroker(),ILiveLoads,         pLiveLoads);
   GET_IFACE2(GetBroker(),IBridge,            pBridge);
   GET_IFACE2(GetBroker(),IPointOfInterest,   pPoi);

   std::shared_ptr<pgsGirderArtifact> pGdrArtifact(new pgsGirderArtifact(girderKey));

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   // warning if live load isn't defined... this would be a highly unusual case
   if (!pLiveLoads->IsLiveLoadDefined(pgsTypes::lltDesign))
   {
      std::_tstring strMsg(_T("Live load is not defined."));
      GET_IFACE2(GetBroker(),IEAFStatusCenter,   pStatusCenter);
      pStatusCenter->Add(std::make_shared<pgsLiveLoadStatusItem>(m_StatusGroupID, m_scidLiveLoad, strMsg.c_str()));
   }

   // going to need this inside the loop
   GET_IFACE2(GetBroker(),ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   GET_IFACE2(GetBroker(),ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount()-1;

   // we want to switch between using POI's for individual segments to using the POI's of the span
   // once all the segments are connected together
   IntervalIndexType lastCompositeCJIntervalIdx = pIntervals->GetLastCompositeClosureJointInterval(girderKey);
   if ( lastCompositeCJIntervalIdx == INVALID_INDEX )
   {
      // this happens if there aren't any closure joints... we can consider
      // analysis to be on a span basis once the last deck casting is composite
      lastCompositeCJIntervalIdx = pIntervals->GetLastCompositeDeckInterval();
   }

   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey,segIdx);

      // get the POI that will be used for spec checking
      PoiList releasePois;
      pPoi->GetPointsOfInterest(segmentKey, POI_RELEASED_SEGMENT, &releasePois);
      ATLASSERT(releasePois.size() == 11);

      // make sure release includes the end faces (usually will, but not if release supports aren't at ends of segment)
      PoiList endFacePois;
      pPoi->GetPointsOfInterest(segmentKey, POI_START_FACE | POI_END_FACE, &endFacePois);
      ATLASSERT(endFacePois.size() == 2);
      releasePois.insert(releasePois.begin(),endFacePois.front());
      releasePois.insert(releasePois.end(),endFacePois.back());
      
      PoiList storagePois;
      pPoi->GetPointsOfInterest(segmentKey, POI_STORAGE_SEGMENT, &storagePois);
      ATLASSERT(storagePois.size() == 11);

      PoiList erectedPois;
      pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT, &erectedPois);

      PoiList spanPois;
      pPoi->GetPointsOfInterest(segmentKey, POI_SPAN, &spanPois);

      // get all some special POI related to flexure
      PoiList vOtherPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_HARPINGPOINT | POI_DIAPHRAGM | POI_CONCLOAD | POI_PSXFER | POI_DEBOND | POI_SECTCHANGE_LEFTFACE | POI_SECTCHANGE_RIGHTFACE, &vOtherPoi, POIFIND_OR);
      releasePois.insert(releasePois.end(),vOtherPoi.begin(),vOtherPoi.end());
      storagePois.insert(storagePois.end(),vOtherPoi.begin(),vOtherPoi.end());
      erectedPois.insert(erectedPois.end(),vOtherPoi.begin(),vOtherPoi.end());
      spanPois.insert(spanPois.end(),vOtherPoi.begin(),vOtherPoi.end());

      // sort and remove duplicates
      pPoi->SortPoiList(&releasePois);
      pPoi->SortPoiList(&storagePois);
      pPoi->SortPoiList(&erectedPois);
      pPoi->SortPoiList(&spanPois);

      pgsSegmentArtifact* pSegmentArtifact = pGdrArtifact->GetSegmentArtifact(segIdx);
      ATLASSERT( segmentKey == pSegmentArtifact->GetSegmentKey() );

      IntervalIndexType storageIntervalIdx  = pIntervals->GetStorageInterval(segmentKey);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

      GET_IFACE2(GetBroker(),IStressCheck, pStressCheck);
      std::vector<StressCheckTask> vStressCheckTasks = pStressCheck->GetStressCheckTasks(segmentKey);
      for (const auto& task : vStressCheckTasks)
      {
         bool bClosureJoints(false);
         if (nSegments > 1)
         {
            // Closure joint is always associated with segment to its left. Last segment must use previous seg to get correct CJ info
            if (segIdx == nSegments - 1) // last segment
            {
               bClosureJoints = pIntervals->GetCompositeClosureJointInterval( CSegmentKey(girderKey,segIdx-1) ) <= task.intervalIdx ? true : false;
            }
            else
            {
               bClosureJoints = pIntervals->GetCompositeClosureJointInterval(segmentKey) <= task.intervalIdx ? true : false;
            }
         }

         // POIs to spec check
         PoiList vPoi;
         if (task.intervalIdx < storageIntervalIdx)
         {
            vPoi = releasePois;
         }
         else if (storageIntervalIdx <= task.intervalIdx && task.intervalIdx < erectionIntervalIdx)
         {
            vPoi = storagePois;
         }
         else
         {
            // after the segment is erected, don't spec check locations that are outside of the CL Bearings
            // (these are the POI in the little end cantilevers)
            Float64 segmentSpanLength = pBridge->GetSegmentSpanLength(segmentKey);
            Float64 startEndDist = pBridge->GetSegmentStartEndDistance(segmentKey);
            bool bStartCantilever, bEndCantilever;
            pBridge->ModelCantilevers(segmentKey, &bStartCantilever, &bEndCantilever);

            if (bClosureJoints)
            {
               // this is a multi-segment girder and the closure joints have become composite forming
               // a continuous girder 
               if (segIdx == 0)
               {
                  // if this is the first segment, always model the cantilever at the end
                  bEndCantilever = true;
               }
               else if (segIdx == nSegments - 1)
               {
                  // if this is the last segment, always model the cantilever at the start
                  bStartCantilever = true;
               }
               else
               {
                  // this is an intermediate segment... model cantilevers at both ends
                  bStartCantilever = true;
                  bEndCantilever = true;
               }
            }

            Float64 start = (bStartCantilever ? 0 : startEndDist);
            Float64 end = (bEndCantilever ? pBridge->GetSegmentLength(segmentKey) : startEndDist + segmentSpanLength);

            if (task.intervalIdx < lastCompositeCJIntervalIdx)
            {
               std::remove_copy_if(erectedPois.begin(), erectedPois.end(), std::back_inserter(vPoi), PoiIsOutsideOfBearings(segmentKey, start, end));
            }
            else
            {
               std::remove_copy_if(spanPois.begin(), spanPois.end(), std::back_inserter(vPoi), PoiIsOutsideOfBearings(segmentKey, start, end));
            }
         }


         // if closures can take any load, add them to the list of poi
         if (bClosureJoints)
         {
            PoiList vCJPoi;
            pPoi->GetPointsOfInterest(segmentKey, POI_CLOSURE, &vCJPoi);
            pPoi->MergePoiLists(vPoi, vCJPoi, &vPoi);
         }

         std::_tostringstream os;
         os << _T("Checking ") << GetStressTypeString(task.stressType) << _T(" stress for ") << GetLimitStateString(task.limitState) << _T(" for Interval ") << LABEL_INTERVAL(task.intervalIdx) << _T(": ") << pIntervals->GetDescription(task.intervalIdx) << std::endl;
         pProgress->UpdateMessage(os.str().c_str());

         // TEMPORARY - for comparing against the design-time RefineDesignForAllowableStress numbers
         // in Designer_x64.log while tracking down the reg021 girder-B compression discrepancy. Remove
         // once that's resolved.
         DLOG(_T(""));
         DLOG(_T("*** Final Check for Interval ") << LABEL_INTERVAL(task.intervalIdx) << _T(", ") << pIntervals->GetDescription(task.intervalIdx) << _T(" ") << g_LimitState[task.limitState] << _T(" ") << g_Type[task.stressType]);

         CheckSegmentStresses(segmentKey, vPoi, task, pSegmentArtifact);
      } // next stress check task

      // These checks are independent of interval, so only check them once

      pProgress->UpdateMessage(_T("Checking segment details"));
      CheckSegmentDetailing(segmentKey,pSegmentArtifact);

      // Check Lifting
      if ( pSegmentLiftingSpecCriteria->IsLiftingAnalysisEnabled() )
      {
         pProgress->UpdateMessage(_T("Checking lifting"));
         auto pLiftingArtifact = CheckLifting(segmentKey);
         pSegmentArtifact->SetLiftingCheckArtifact(pLiftingArtifact);
      }

      // Check Hauling
      if ( pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled() )
      {
         pProgress->UpdateMessage(_T("Checking hauling"));
         auto pHaulingAnalysisArtifact = CheckHauling(segmentKey);
         pSegmentArtifact->SetHaulingAnalysisArtifact(pHaulingAnalysisArtifact);
      }

      pProgress->UpdateMessage(_T("Checking strand stresses"));
      CheckStrandStresses(segmentKey,pSegmentArtifact->GetStrandStressArtifact());

      pProgress->UpdateMessage(_T("Checking strand slope and hold down force"));
      CheckStrandSlope(   segmentKey, pSegmentArtifact->GetStrandSlopeArtifact()   );
      CheckHoldDownForce( segmentKey, pSegmentArtifact->GetHoldDownForceArtifact() );
      CheckPlantHandlingWeightLimit(segmentKey, pSegmentArtifact->GetPlantHandlingWeightArtifact());

      pProgress->UpdateMessage(_T("Checking handling stability"));
      CheckSegmentStability(segmentKey,pSegmentArtifact->GetSegmentStabilityArtifact());

      pProgress->UpdateMessage(_T("Checking debonding"));
      CheckDebonding(segmentKey, pSegmentArtifact->GetDebondArtifact());

      pProgress->UpdateMessage(_T("Checking principal tension stress in webs"));
      CheckPrincipalTensionStressInWebs(segmentKey, pSegmentArtifact->GetPrincipalTensionStressArtifact());

      pProgress->UpdateMessage(_T("Checking reinforcement fatigue"));
      CheckReinforcementFatigue(segmentKey, pSegmentArtifact->GetReinforcementFatigueArtifact());
   } // next segment

   pProgress->UpdateMessage(_T("Checking constructability"));
   CheckConstructability(girderKey,pGdrArtifact->GetConstructabilityArtifact());

   // Check ultimate capacity
   pProgress->UpdateMessage(_T("Checking moment capacity"));
   CheckMomentCapacity(lastIntervalIdx,pgsTypes::StrengthI,pGdrArtifact.get());

   pProgress->UpdateMessage(_T("Checking shear capacity"));
   CheckShear(lastIntervalIdx,pgsTypes::StrengthI,pGdrArtifact.get());

   GET_IFACE2(GetBroker(),ILimitStateForces,pLimitStateForces);
   if(pLimitStateForces->IsStrengthIIApplicable(girderKey))
   {
      CheckMomentCapacity(lastIntervalIdx,pgsTypes::StrengthII,pGdrArtifact.get());
      CheckShear(lastIntervalIdx,pgsTypes::StrengthII,pGdrArtifact.get());
   }

   // check live load deflection
   pProgress->UpdateMessage(_T("Checking live load deflection"));
   CheckLiveLoadDeflection(girderKey,pGdrArtifact.get());

   // check tendon stresses
   pProgress->UpdateMessage(_T("Checking tendon stresses"));
   CheckTendonStresses(girderKey,pGdrArtifact.get());
   CheckTendonDetailing(girderKey,pGdrArtifact.get());

   pProgress->UpdateMessage(_T("Checking minimum deck reinforcement for tensile stress"));
   CheckMinimumDeckReinforcement(girderKey, pGdrArtifact.get());

   pProgress->UpdateMessage(_T("Checking horizontal transverse tension tie reinforcement"));
   CheckHorizontalTensionTie(girderKey, pGdrArtifact.get());

   // add the artifact to the cache
   m_CheckArtifacts.insert( std::make_pair(girderKey,pGdrArtifact) );

   pTheGdrArtifact = GetGirderArtifact(girderKey);// get the artifact from the cache.... this is what we want to return
   ATLASSERT(pTheGdrArtifact != nullptr); 
   return pTheGdrArtifact;
}

void CheckProgress(std::shared_ptr<IEAFProgress> pProgress)
{
   if ( pProgress->Continue() != S_OK )
   {
      //DLOG(_T("*#*#*#*#* DESIGN CANCELLED BY USER *#*#*#*#*"));
      throw pgsSegmentDesignArtifact::DesignCancelled;
   }
}

void pgsDesigner2::ConfigureStressCheckTasks(const CSegmentKey& segmentKey) const
{
   // Configure the stress check tasks
   GET_IFACE2(GetBroker(),IStressCheck, pStressCheck);
   m_StressCheckTasks = pStressCheck->GetStressCheckTasks(segmentKey,true/*design*/);
}

#define CHECK_PROGRESS CheckProgress(pProgress)
void pgsDesigner2::OpenDesignLog() const
{
#if defined ENABLE_DESIGN_LOGGING
   if (m_Log.IsOpen() || !pgsDesignLog::IsRequested())
   {
      return;
   }

   std::_tstring strProjectTitle, strProjectFolder;
   GET_IFACE2_NOCHECK(GetBroker(), IEAFDocument, pDocument);
   if (pDocument)
   {
      strProjectTitle = pDocument->GetFileTitle();
      strProjectFolder = pDocument->GetFileRoot();
   }

   if (m_Log.Open(pgsDesignLog::GetLogFilePath(strProjectTitle.c_str(), strProjectFolder.c_str())))
   {
      pgsDesignLog::WriteLegend(m_Log);
      DLOG(_T("Project: ") << (pDocument ? std::_tstring(pDocument->GetFilePath()) : std::_tstring(_T("unknown"))));
      DLOG(_T("Log file: ") << m_Log.GetFilePath());
   }
#endif
}

pgsGirderDesignArtifact pgsDesigner2::Design(const CGirderKey& girderKey,const std::vector<arDesignOptions>& desOptionsColl) const
{
   OpenDesignLog();

   // The design artifact
   ASSERT_GIRDER_KEY(girderKey);
   pgsGirderDesignArtifact artifact(girderKey);

   GET_IFACE2(GetBroker(),IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   ATLASSERT(nSegments == 1); // Design only works for prestressed girders (PGSuper) so there should only be one segment per girder

   // We don't design for time-step analysis
   GET_IFACE2(GetBroker(),ILossParameters,pLossParams);
   if ( pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      LOG_ABORT(_T("Design of Span ") << LABEL_SPAN(girderKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(girderKey.girderIndex) << _T(" not performed: design is not supported for time-step losses"));
      // we don't design for time-step method so just return the empty artifact
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         pgsSegmentDesignArtifact segArtifact(CSegmentKey(girderKey,segIdx));
         segArtifact.SetOutcome(pgsSegmentDesignArtifact::DesignNotSupported_Losses);
         artifact.AddSegmentDesignArtifact(segIdx,segArtifact);
      }
      return artifact;
   }

   GET_IFACE2(GetBroker(),IMaterials, pMaterials);
   if (IsUHPC(pMaterials->GetSegmentConcreteType(CSegmentKey(girderKey, 0))))
   {
      LOG_ABORT(_T("Design of Span ") << LABEL_SPAN(girderKey.groupIndex) << _T(" Girder ") << LABEL_GIRDER(girderKey.girderIndex) << _T(" not performed: design is not supported for UHPC"));
      // we don't design for UHPC so just return the empty artifact
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         pgsSegmentDesignArtifact segArtifact(CSegmentKey(girderKey, segIdx));
         segArtifact.SetOutcome(pgsSegmentDesignArtifact::DesignNotSupported_Material);
         artifact.AddSegmentDesignArtifact(segIdx, segArtifact);
      }
      return artifact;
   }

   // on occasion, the LLDF range of applicability is violated and it halts the design.
   // this usually happens with f'c is at its max value which impacts n and Kg.
   // this is undesirable.
   // to get around this, set the LLDF ROA action to ignore the ROA.. then reset it to the
   // current value when design is done.

   // But first, test the ROA and don't even try to design if there is a problem. A CXUnwind will get thrown from pLLDF->TestRangeOfApplicability if there is a problem
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GET_IFACE2(GetBroker(),ILiveLoadDistributionFactors,pLLDF);
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirdersThisGroup = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = Min(girderKey.girderIndex, nGirdersThisGroup - 1);

      SpanIndexType startSpanIdx, endSpanIdx;
      pBridge->GetGirderGroupSpans(grpIdx, &startSpanIdx, &endSpanIdx);
      for (SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++)
      {
         CSpanKey spanKey(spanIdx, gdrIdx);
         pLLDF->TestRangeOfApplicability(spanKey);
      }
   }

   // we don't want events to fire so we'll hold events and then cancel any pending events
   // when design is done. Use auto class so we do it exception safely.
   GET_IFACE2(GetBroker(),IEvents,pEvents);
   GET_IFACE2(GetBroker(),ILiveLoads,pLiveLoads);
   AutoDesign myAutoDes(pEvents, pLiveLoads);

   try 
   {
      std::vector<arDesignOptions>::const_iterator designOptionIter(desOptionsColl.begin());
      std::vector<arDesignOptions>::const_iterator designOptionIterEnd(desOptionsColl.end());
      for ( ; designOptionIter != designOptionIterEnd; designOptionIter++ )
      {
         const arDesignOptions& options = *designOptionIter;

         // Design is attempted with each set of design options until one succeeds (e.g. harped, then debonded)
         DESIGN_LOG_SCOPE(_T("Design strategy ") << (designOptionIter - desOptionsColl.begin()) + 1 << _T(" of ") << desOptionsColl.size() << _T(": ") << pgsDesignLog::DescribeDesignOptions(options));

         DoDesign(girderKey,options,artifact);

         bool bSuccess = true;
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            pgsSegmentDesignArtifact* pSegmentDesignArtifact = artifact.GetSegmentDesignArtifact(segIdx);
            if ( pSegmentDesignArtifact->GetOutcome() != pgsSegmentDesignArtifact::Success )
            {
               bSuccess = false;
               pSegmentDesignArtifact->AddFailedDesign(options);
            }
            DESIGN_LOG_SCOPE_RESULT(_T("-> ") << pgsDesignLog::OutcomeName(pSegmentDesignArtifact->GetOutcome()));
         }

         if ( bSuccess || designOptionIter == designOptionIterEnd-1 )
         {
            // if the design succeeded or if we are out of design options, we are done
            break;
         }

         DLOG(_T("Strategy did not succeed - trying the next design strategy"));
      }
   }
   catch (pgsSegmentDesignArtifact::Outcome outcome)
   {
      LOG_ABORT(_T("Design ended by exception with outcome ") << pgsDesignLog::OutcomeName(outcome));
      if (outcome == pgsSegmentDesignArtifact::DesignCancelled )
      {
         // Design was cancelled... put a dummy artifact for all segments
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
         {
            CSegmentKey segmentKey(girderKey,segIdx);
            pgsSegmentDesignArtifact segmentArtifact(segmentKey);
            artifact.AddSegmentDesignArtifact(segIdx,segmentArtifact);
         }
      }

      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         pgsSegmentDesignArtifact* pSegmentDesignArtifact = artifact.GetSegmentDesignArtifact(segIdx);
         pSegmentDesignArtifact->SetOutcome(outcome);
      }
   }
   
   GET_IFACE2(GetBroker(),ILosses, pLosses);
   pLosses->ClearDesignLosses();

   return artifact;
}

void pgsDesigner2::DoDesign(const CGirderKey& girderKey,const arDesignOptions& options, pgsGirderDesignArtifact& girderDesignArtifact) const
{
#if defined _DEBUG
   GET_IFACE2(GetBroker(),IDocumentType,pDocType);
   ATLASSERT(pDocType->IsPGSuperDocument());
#endif

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount()-1;

   GET_IFACE2(GetBroker(),ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   GET_IFACE2(GetBroker(),IEAFProgress,pProgress);
   WBFL::EAF::AutoProgress ap(pProgress,0,PW_ALL | PW_NOGAUGE); // progress window has a cancel button

   GET_IFACE2_NOCHECK(GetBroker(),IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData*    pGroup      = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const CSplicedGirderData*  pGirder     = pGroup->GetGirder(girderKey.girderIndex);

   GET_IFACE2_NOCHECK(GetBroker(),ISpecification, pSpec);
   GET_IFACE2(GetBroker(),IBridge,pBridge);
   GET_IFACE2_NOCHECK(GetBroker(),IGirder, pGdr);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   // Design each segment in the girder
   // (for now, there should only be one segment)
   ATLASSERT(nSegments == 1);
   for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      CSegmentKey segmentKey(girderKey, segIdx);
      pgsSegmentDesignArtifact artifact(segmentKey);
      artifact.SetDesignOptions(options);

      SpanIndexType spanIdx = girderKey.groupIndex;
      GirderIndexType gdrIdx = girderKey.girderIndex;

      DESIGN_LOG_SET_ITERATION(INVALID_INDEX);
      DESIGN_LOG_SCOPE(_T("Design Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx));

      const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
      if ( !CanDesign(pSegment->Strands.GetStrandDefinitionType()) )
      {
         LOG_ABORT(_T("Cannot design with the current strand definition type"));
         DESIGN_LOG_SCOPE_RESULT(_T("-> ") << pgsDesignLog::OutcomeName(pgsSegmentDesignArtifact::DesignNotSupported_Strands));

         artifact.SetOutcome(pgsSegmentDesignArtifact::DesignNotSupported_Strands);
         girderDesignArtifact.AddSegmentDesignArtifact(segIdx, artifact);
         continue; // process next segment
      }

      ConfigureStressCheckTasks(segmentKey);

      std::_tostringstream os;
      os << _T("Designing Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << std::ends;
      pProgress->UpdateMessage(os.str().c_str());

      // initialize temporary top strand usage type
      // we prefer pretensioned TTS so set it to that
      // however if there is precamber, TTS must be post-tensioned

      artifact.SetTemporaryStrandUsage(IsZero(pGdr->GetPrecamber(segmentKey)) ? pgsTypes::ttsPretensioned : pgsTypes::ttsPTBeforeLifting);


      // initialize lifting and hauling to current values
      GET_IFACE2(GetBroker(),ISegmentLifting,pSegmentLifting);
      Float64 Loh = pSegmentLifting->GetLeftLiftingLoopLocation(segmentKey);
      Float64 Roh = pSegmentLifting->GetRightLiftingLoopLocation(segmentKey);
      artifact.SetLiftingLocations(Loh,Roh);

      GET_IFACE2(GetBroker(),ISegmentHauling,pSegmentHauling);
      Loh = pSegmentHauling->GetTrailingOverhang(segmentKey);
      Roh = pSegmentHauling->GetLeadingOverhang(segmentKey);
      artifact.SetTruckSupportLocations(Loh,Roh);
      artifact.SetHaulTruck(pSegmentHauling->GetHaulTruck(segmentKey));

      // Copy current longitudinal rebar data to the artifact. 
      // This algorithm will only add more rebar to existing, and only
      // for the longitudinal reinforcement for shear condition
      artifact.SetLongitudinalRebarData( pSegment->LongitudinalRebarData );


      Float64 segment_length = pBridge->GetSegmentLength(segmentKey);

      DLOG(_T("Girder type = ") << pGirder->GetGirderName() << _T(", segment length = ") << pgsDesignLog::ft(segment_length) << _T(" ft")
          << _T(", temporary strand usage = ") << (artifact.GetTemporaryStrandUsage() == pgsTypes::ttsPretensioned ? _T("pretensioned") : _T("post-tensioned before lifting")));

      // Use strand design tool to control proportioning of strands
      m_StrandDesignTool->Initialize(m_pBroker, m_StatusGroupID, &artifact);

      Float64 zoneFactor, startd, endd;
      Float64 startConfinementZl, endConfinementZl;
      GetConfinementZoneLengths(segmentKey, pGdr, segment_length, &zoneFactor, &startd, &endd, &startConfinementZl, &endConfinementZl);

      // Use shear design tool to control stirrup design
      m_ShearDesignTool.Initialize(m_pBroker, this, m_StatusGroupID, &artifact, startConfinementZl, endConfinementZl, bPermit, options.doDesignForShear == sdtLayoutStirrups);

      // clear outcome codes
      m_DesignerOutcome.Reset();

      // don't do anything if nothing is asked
      if (options.doDesignForFlexure==dtNoDesign && options.doDesignForShear == sdtNoDesign)
      {
         DLOG(_T("Neither flexure nor shear design was requested - nothing to do"));
         DESIGN_LOG_SCOPE_RESULT(_T("-> ") << pgsDesignLog::OutcomeName(pgsSegmentDesignArtifact::NoDesignRequested));
         artifact.SetOutcome(pgsSegmentDesignArtifact::NoDesignRequested);
         girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
         continue; // process next segment
      }

      Int16 cIter = -1;
      Int16 nIterMax = 30;
      bool bDone = false;
      // (f'c, f'ci) seen at the start of each outer iteration - detects the case below where design
      // cycles between two or more marginally-different requirements (typically because a pick-point or
      // POI search is itself unstable near a rounding boundary) instead of converging. The
      // exponential-backoff logic in ConcreteStrengthController::DoUpdate exists to damp exactly this
      // kind of thrash, but isn't guaranteed to catch every case it can arise in.
      std::vector<std::pair<Float64,Float64>> fcHistory;

#if defined ENABLE_DESIGN_LOGGING
      // The design summary is written when this segment's design ends for any reason (success, abort,
      // max iterations, or exception). It is the first thing to read when diagnosing a design.
      std::vector<std::_tstring> restartHistory;
      auto logDesignSummary = [&]()
      {
         if (!pgsDesignLog::IsEnabled())
         {
            return;
         }

         DESIGN_LOG_SET_ITERATION(INVALID_INDEX);
         DLOG(_T("DESIGN SUMMARY - Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx));
         if (0 < std::uncaught_exceptions())
         {
            DLOG(_T("   Outcome     : design ended by exception (cancelled by user, or an error)"));
         }
         else
         {
            DLOG(_T("   Outcome     : ") << pgsDesignLog::OutcomeName(artifact.GetOutcome()) << (m_DesignerOutcome.WasDesignAborted() ? _T(" (design aborted)") : _T("")));
         }
         DLOG(_T("   Options     : ") << pgsDesignLog::DescribeDesignOptions(options));
         DLOG(_T("   Iterations  : ") << cIter + 1 << _T(" outer iteration(s), maximum is ") << nIterMax + 1);
         DLOG(_T("   Restarts    : ") << restartHistory.size() << (restartHistory.empty() ? _T("") : _T(" (iteration: reason [designer outcome flags])")));
         for (const auto& restart : restartHistory)
         {
            DLOG(_T("      ") << restart);
         }
         DLOG(_T("   Final state : ") << m_StrandDesignTool->GetDesignStateSummary());
      };
      struct SummaryOnExit
      {
         std::function<void()> f;
         ~SummaryOnExit() { try { f(); } catch (...) {} }
      } summaryOnExit{ logDesignSummary };
#endif

      // Logs the reason the outer design loop is restarting and records it for the design summary.
      // Must be used just before a "continue" of the outer loop.
#if defined ENABLE_DESIGN_LOGGING
#define LOG_OUTER_RESTART(_x_) \
      if (pgsDesignLog::IsEnabled()) { std::_tostringstream _reason_; _reason_ << _x_; /* not named _os_, which DESIGN_LOG_SCOPE_RESULT uses internally */ \
        LOG_RESTART(_reason_.str() << _T(" [") << m_DesignerOutcome.ToString() << _T("]")); \
        DESIGN_LOG_SCOPE_RESULT(_T("-> RESTART: ") << _reason_.str() << _T(". End state: ") << m_StrandDesignTool->GetDesignStateSummary()); \
        std::_tostringstream _osh_; _osh_ << _T("i") << std::setw(2) << std::setfill(_T('0')) << cIter << _T(": ") << _reason_.str() << _T(" [") << m_DesignerOutcome.ToString() << _T("]"); \
        restartHistory.push_back(_osh_.str()); }

      // Logs that a design step aborted the design. Must be used just before returning from the outer loop.
#define LOG_OUTER_ABORT(_step_) \
      { LOG_ABORT(_step_ << _T(" aborted the design. Outcome = ") << pgsDesignLog::OutcomeName(artifact.GetOutcome()) << _T(" [") << m_StrandDesignTool->GetDesignStateSummary() << _T("]")); \
        DESIGN_LOG_SCOPE_RESULT(_T("-> ABORT: ") << _step_ << _T(", ") << pgsDesignLog::OutcomeName(artifact.GetOutcome())); }
#else
#define LOG_OUTER_RESTART(_x_)
#define LOG_OUTER_ABORT(_step_)
#endif

      do
      {
         CHECK_PROGRESS;

         cIter++;
         DESIGN_LOG_SET_ITERATION(cIter);
         DESIGN_LOG_SCOPE(_T("Outer design iteration ") << cIter);
         DLOG(_T("Start state: ") << m_StrandDesignTool->GetDesignStateSummary());
         std::_tostringstream os2;
         os2 << _T("Design Iteration ")<<cIter+1<<_T(" for Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << std::ends;

         pProgress->UpdateMessage(os2.str().c_str());

         if (options.doDesignForFlexure != dtNoDesign)
         {
            // Only treat this as evidence about concrete-strength cycling if the PREVIOUS iteration's
            // restart was actually triggered by a concrete strength change. Plenty of ordinary restarts
            // (slab offset converging, raised strands added, etc.) leave f'c/f'ci untouched and quite
            // normally recur across iterations while something unrelated iterates - that's not a cycle,
            // and must not be recorded as one, or every such restart would look like the first half of
            // an oscillation. m_DesignerOutcome still reflects the prior iteration here; it isn't reset
            // until just below.
            if (m_DesignerOutcome.DidConcreteChange())
            {
               Float64 fc_now  = m_StrandDesignTool->GetConcreteStrength();
               Float64 fci_now = m_StrandDesignTool->GetReleaseStrength();
               auto cycleStart = std::find(fcHistory.begin(), fcHistory.end(), std::make_pair(fc_now, fci_now));
               if (cycleStart != fcHistory.end())
               {
                  // Rather than burn the rest of the iteration budget and abort the whole design over a
                  // cycle, lock concrete strength at the highest f'c and f'ci seen anywhere in the cycle.
                  // That's safe for every check that contributed to it - more strength only ever helps a
                  // stress check pass, never hurts - and guarantees this loop terminates from here (either
                  // converging normally, or failing on some other, genuinely separate issue). Locking
                  // still permits later, genuinely higher requirements from unrelated checks to raise it
                  // further (see ConcreteStrengthController::DoUpdate's fciLocked case) - it only stops
                  // it from being pulled back down into the cycle again.
                  Float64 fc_safe = fc_now, fci_safe = fci_now;
                  for (auto it = cycleStart; it != fcHistory.end(); ++it)
                  {
                     fc_safe  = Max(fc_safe,  it->first);
                     fci_safe = Max(fci_safe, it->second);
                  }
                  DLOG(_T("Concrete strength cycling between previously-seen values without converging - locking at the safe (highest) f'c = ")
                     << WBFL::Units::ConvertFromSysUnits(fc_safe,WBFL::Units::Measure::KSI) << _T(" ksi, f'ci = ")
                     << WBFL::Units::ConvertFromSysUnits(fci_safe,WBFL::Units::Measure::KSI) << _T(" ksi"));
                  m_StrandDesignTool->LockConcreteStrengthAt(fc_safe, fci_safe);
                  fcHistory.clear();
               }
               else
               {
                  fcHistory.emplace_back(fc_now, fci_now);
               }
            }
         }

         if (options.doDesignForFlexure!=dtNoDesign)
         {
            // reset outcomes
            bool keep_prop = false;
            if (m_DesignerOutcome.DidConcreteChange() && m_StrandDesignTool->IsDesignSlabOffset())
            {
               DLOG(_T("Concrete changed on last iteration. Reset min slab offset to zero"));
               m_StrandDesignTool->SetMinimumSlabOffset(0.0);
            }

            bool just_added_raised_strands = m_DesignerOutcome.DidRaiseStraightStrands();

            keep_prop = m_DesignerOutcome.DidRetainStrandProportioning();
            m_DesignerOutcome.Reset();
            if (keep_prop)
            {
               DLOG(_T("Retaining strand proportioning from last iteration"));
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::RetainStrandProportioning);
            }

            // get design back on track with user preferences
            m_StrandDesignTool->RestoreDefaults(m_DesignerOutcome.DidRetainStrandProportioning(), just_added_raised_strands);

            // Design strands and concrete strengths in mid-zone
            DesignMidZone(cIter == 0 ? false : true, options,pProgress);

            if (m_DesignerOutcome.WasDesignAborted())
            {
               LOG_OUTER_ABORT(_T("Mid-zone design"));
               girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
               return;
            }
            else if( m_DesignerOutcome.DidRaiseStraightStrands() )
            {
               LOG_OUTER_RESTART(_T("Mid-zone design added raised straight strands"));
               continue;
            }

            CHECK_PROGRESS;

            m_StrandDesignTool->DumpDesignParameters(_T("before end-zone design"));

            // Design end zones
            DesignEndZone(cIter < 2, options, artifact, pProgress);

            if ( m_DesignerOutcome.WasDesignAborted() )
            {
               LOG_OUTER_ABORT(_T("End-zone design"));
               girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
               return;
            }
            else if ( m_DesignerOutcome.DidConcreteChange() )
            {
               LOG_OUTER_RESTART(_T("End-zone design changed concrete strength"));
               continue;
            }
            else if( m_DesignerOutcome.DidRaiseStraightStrands() )
            {
               LOG_OUTER_RESTART(_T("End-zone design added raised straight strands"));
               continue;
            }
            else if ( m_DesignerOutcome.GetOutcome(pgsDesignCodes::TemporaryStrandsChanged) )
            {
               // DesignForShipping added temporary strands because no concrete strength could
               // satisfy the hauling tension limit, and asked for a restart. Honor it: the hauling
               // design that motivated them ran without them, and the mid-zone strand count was
               // sized before they existed, so both have to be redone with the new configuration.
               // This terminates - Nt only increases (RestoreDefaults does not reset it and
               // AddTempStrands fails at the girder maximum) and nIterMax bounds the outer loop.
               LOG_OUTER_RESTART(_T("Shipping design added temporary strands"));
               continue;
            }

            pProgress->UpdateMessage(_T("Stress Design Refinement"));

            CHECK_PROGRESS;

            m_StrandDesignTool->DumpDesignParameters(_T("before allowable stress refinement"));

            // Refine design based on allowable stress criteria
            // Add and harp strands to satisfy stress criteria
            RefineDesignForAllowableStress(pProgress);

            CHECK_PROGRESS;

            if ( m_DesignerOutcome.WasDesignAborted() )
            {
               LOG_OUTER_ABORT(_T("Allowable stress refinement"));
               girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
               return;
            }
            else if  (  m_DesignerOutcome.DidConcreteChange())
            {
               LOG_OUTER_RESTART(_T("Allowable stress refinement changed concrete strength"));
               continue;
            }

            //
            // Refine Design for Ultimate Strength
            //
            pProgress->UpdateMessage(_T("Designing for Ultimate Moment"));

   // NOTE
   // No longer designing/checking for ultimate moment in temporary construction state
   // per e-mail from Bijan Khaleghi, dated 4/28/1999.  See project log.
   //         retval = RefineDesignForUltimateMoment(pgsTypes::BridgeSite1,
   //                                                pgsTypes::StrengthI,
   //                                                &artifact);
   //         if ( retval == DESIGN_RESTART )
   //            continue;
   //         if ( retval == DESIGN_ABORT )
   //            return artifact;

            RefineDesignForUltimateMoment(lastIntervalIdx, pgsTypes::StrengthI,pProgress);

            CHECK_PROGRESS;

            if ( m_DesignerOutcome.WasDesignAborted() )
            {
               LOG_OUTER_ABORT(_T("Strength I ultimate moment refinement"));
               girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
               return;
            }
            else if  (  m_DesignerOutcome.GetOutcome(pgsDesignCodes::ChangedForUltimate) )
            {
               LOG_OUTER_RESTART(_T("Strength I ultimate moment changed the design"));
               continue;
            }

            if ( bPermit )
            {
               pProgress->UpdateMessage(_T("Designing for Strength II Ultimate Moment"));
               RefineDesignForUltimateMoment(lastIntervalIdx, pgsTypes::StrengthII,pProgress);

               CHECK_PROGRESS;

               if ( m_DesignerOutcome.WasDesignAborted() )
               {
                  LOG_OUTER_ABORT(_T("Strength II ultimate moment refinement"));
                  girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
                  return;
               }
               else if  (  m_DesignerOutcome.GetOutcome(pgsDesignCodes::ChangedForUltimate) )
               {
                  LOG_OUTER_RESTART(_T("Strength II ultimate moment changed the design"));
                  continue;
               }
            }

            if (options.doDesignSlabOffset != sodPreserveHaunch)
            {
               pProgress->UpdateMessage(_T("Designing Slab Offset Outer Loop"));

               Float64 old_offset_start = m_StrandDesignTool->GetSlabOffset(pgsTypes::metStart );
               Float64 old_offset_end   = m_StrandDesignTool->GetSlabOffset(pgsTypes::metEnd );

               DesignSlabOffset( pProgress );

               CHECK_PROGRESS;

               if (  m_DesignerOutcome.WasDesignAborted() )
               {
                  LOG_OUTER_ABORT(_T("Slab offset design"));
                  girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
                  return;
               }
               else if ( m_DesignerOutcome.GetOutcome(pgsDesignCodes::SlabOffsetChanged) )
               {
                  Float64 new_offset_start = m_StrandDesignTool->GetSlabOffset(pgsTypes::metStart);
                  Float64 new_offset_end   = m_StrandDesignTool->GetSlabOffset(pgsTypes::metEnd);

                  new_offset_start = RoundSlabOffsetValue(pSpec, new_offset_start);
                  new_offset_end   = RoundSlabOffsetValue(pSpec, new_offset_end);

                  m_StrandDesignTool->SetMinimumSlabOffset( Min(new_offset_start,new_offset_end));
                  m_StrandDesignTool->SetSlabOffset(pgsTypes::metStart,new_offset_start);
                  m_StrandDesignTool->SetSlabOffset(pgsTypes::metEnd, new_offset_end);
                  LOG_OUTER_RESTART(_T("Slab offset changed to ") << pgsDesignLog::in(new_offset_start) << _T(" in (start), ") << pgsDesignLog::in(new_offset_end) << _T(" in (end); new minimum set"));
                  continue;
               }
               else
               {
                  m_StrandDesignTool->SetSlabOffset(pgsTypes::metStart,old_offset_start);  // restore to original value that passed all spec checks
                  m_StrandDesignTool->SetSlabOffset(pgsTypes::metEnd,  old_offset_end);   // restore to original value that passed all spec checks
                  LOG_OK(_T("Slab offset design converged: ") << pgsDesignLog::in(m_StrandDesignTool->GetSlabOffset(pgsTypes::metStart)) << _T(" in (start), ") << pgsDesignLog::in(m_StrandDesignTool->GetSlabOffset(pgsTypes::metEnd)) << _T(" in (end)"));
               }
            }
            else
            {
               DLOG(_T("Slab offset design skipped (user input: preserve haunch)"));
            }
         }
         else
         {
            // flexure design was not done. Still need to fill current values in artifact.
            m_StrandDesignTool->FillArtifactWithFlexureValues();
         }

         // if we got here, we are in good shape, however strands may have been adjusted slightly.
         // let's clean the slate
         if (m_DesignerOutcome.DidGirderChange())
         {
            ATLASSERT(!m_DesignerOutcome.DidConcreteChange());
            DLOG(_T("A slight adjustment was made during flexural design. Clear settings for shear design (if applicable)"));
            m_DesignerOutcome.Reset();
         }

         if (options.doDesignForShear != sdtNoDesign)
         {
            //
            // Refine stirrup design
            // 
            pProgress->UpdateMessage(_T("Designing Shear Stirrups"));

            DesignShear(&artifact, options.doDesignForShear == sdtLayoutStirrups, options.doDesignForFlexure!=dtNoDesign);

            if ( m_DesignerOutcome.WasDesignAborted() )
            {
               LOG_OUTER_ABORT(_T("Shear design"));
               girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
               return;
            }
            else if ( m_DesignerOutcome.DidGirderChange() )
            {
               LOG_OUTER_RESTART(_T("Shear design changed the girder (concrete strength or strands)"));
               continue;
            }
         }

         // we've successfully completed all the design steps
         // we are DONE!
         DESIGN_LOG_SCOPE_RESULT(_T("-> all design steps passed. End state: ") << m_StrandDesignTool->GetDesignStateSummary());
         bDone = true;
      } while ( cIter < nIterMax && !bDone );

      DESIGN_LOG_SET_ITERATION(INVALID_INDEX);

      if ( !bDone ) //&& cIter >= nIterMax )
      {
         LOG_ABORT(_T("Maximum number of outer design iterations (") << nIterMax + 1 << _T(") was exceeded without converging. See the restart list in the DESIGN SUMMARY for what kept changing."));
         artifact.SetOutcome(pgsSegmentDesignArtifact::MaxIterExceeded);
         girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
         return;
      }

      if (artifact.GetDesignOptions().doDesignSlabOffset != sodPreserveHaunch)
      {
         Float64 start_offset = RoundSlabOffsetValue(pSpec, artifact.GetSlabOffset(pgsTypes::metStart));
         Float64 end_offset   = RoundSlabOffsetValue(pSpec, artifact.GetSlabOffset(pgsTypes::metEnd));
         DLOG(_T("Final slab offset rounded from ") << pgsDesignLog::in(artifact.GetSlabOffset(pgsTypes::metStart)) << _T("/") << pgsDesignLog::in(artifact.GetSlabOffset(pgsTypes::metEnd))
             << _T(" in to ") << pgsDesignLog::in(start_offset) << _T("/") << pgsDesignLog::in(end_offset) << _T(" in (start/end)"));
         artifact.SetSlabOffset(pgsTypes::metStart,start_offset);
         artifact.SetSlabOffset(pgsTypes::metEnd, end_offset);
      }

      m_StrandDesignTool->DumpDesignParameters(_T("at end of design"));

      pProgress->UpdateMessage(_T("Design Complete"));

      // set controlling data for concrete strengths
      artifact.SetReleaseDesignState(m_StrandDesignTool->GetReleaseConcreteDesignState());
      artifact.SetFinalDesignState(m_StrandDesignTool->GetFinalConcreteDesignState());

      // One last possible hitch(s) here: If we needed to use a higher allowable release strength, it means
      // that we assumed that there is adequate longitudinal rebar in the model to back this assumption. 
      // We need to check this, and if there was not, the design failed; with caveats.
      bool needsAdditionalRebar(false);
      GDRCONFIG config = artifact.GetSegmentConfiguration();

      // The iterative design check (RefineDesignForAllowableStress) only evaluates a sparse set of
      // critical-section POIs for speed. That's normally sufficient, but for some harped-strand
      // geometries the true peak service-limit-state stress falls between those points (see
      // CheckFinalConcreteStrengthAgainstFullPoiGrid). Catch that here with one bounded, full-POI-grid
      // re-check, and bump the final concrete strength once if it finds a shortfall.
      if (options.doDesignForFlexure != dtNoDesign)
      {
         Float64 fc_bumped = CheckFinalConcreteStrengthAgainstFullPoiGrid(segmentKey, config);
         if (0 < fc_bumped)
         {
            LOG_ACTION(_T("Final full-POI stress check raised f'c from ") << pgsDesignLog::ksi(config.fc) << _T(" to ") << pgsDesignLog::ksi(fc_bumped) << _T(" ksi"));
            artifact.SetConcreteStrength(fc_bumped);
            config.fc = fc_bumped;
         }
      }

      if (options.doDesignForFlexure != dtNoDesign &&
         artifact.GetReleaseDesignState().GetRequiredAdditionalRebar())
      {
         // Need to run a flexural spec check in casting yard to validate design

         // Check tension in the casting yard
         pgsSegmentArtifact cySegmentArtifact(segmentKey);
         CheckSegmentStressesAtRelease(segmentKey, &config, pgsTypes::Tension, &cySegmentArtifact);

         bool cytPassed = cySegmentArtifact.DidSegmentFlexuralStressesPass();
         if (!cytPassed)
         {
            LOG_WARN(_T("Release tension check failed without the assumed bonded longitudinal rebar - outcome is success but longitudinal bars are needed (casting yard)"));
            needsAdditionalRebar = true;

            artifact.SetOutcome(pgsSegmentDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionCy);
            artifact.AddDesignNote(pgsSegmentDesignArtifact::dnLongitudinalBarsNeeded4FlexuralTensionCy);
         }
      }

      // Another possible case is hauling since the design algorithm always uses the higher final strength
      if (!needsAdditionalRebar && options.doDesignLifting)
      {
         if ( !CheckLiftingStressDesign(segmentKey,config) )
         {
            LOG_WARN(_T("Lifting stress check requires bonded longitudinal rebar - outcome is success but longitudinal bars are needed (lifting)"));
            needsAdditionalRebar = true;

            artifact.SetOutcome(pgsSegmentDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionLifting);
            artifact.AddDesignNote(pgsSegmentDesignArtifact::dnLongitudinalBarsNeeded4FlexuralTensionLifting);
         }
      }

      // Another possible case is hauling since the design algorithm always uses the higher final strength
      if (!needsAdditionalRebar && options.doDesignHauling)
      {
         if ( !CheckShippingStressDesign(segmentKey, config) )
         {
            LOG_WARN(_T("Hauling stress check requires bonded longitudinal rebar - outcome is success but longitudinal bars are needed (hauling)"));
            needsAdditionalRebar = true;

            artifact.SetOutcome(pgsSegmentDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionHauling);
            artifact.AddDesignNote(pgsSegmentDesignArtifact::dnLongitudinalBarsNeeded4FlexuralTensionHauling);
         }
      }

      // Raised strand designs use direct fill order - if no strands were raised, revert to simplified design
      if (options.doDesignForFlexure!=dtNoDesign)
      {
         m_StrandDesignTool->SimplifyDesignFillOrder(&artifact);
      }

      if (!needsAdditionalRebar)
      {
         artifact.SetOutcome(pgsSegmentDesignArtifact::Success);
      }

      girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
   } // next segment
}

#undef LOG_OUTER_RESTART
#undef LOG_OUTER_ABORT

pgsEccEnvelope pgsDesigner2::GetEccentricityEnvelope(const pgsPointOfInterest& poi,const GDRCONFIG& config) const
{
   pgsEccEnvelope envData;

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE2(GetBroker(),IConcreteStressLimits, pLimits );
   GET_IFACE2(GetBroker(),ILimitStateForces,pLimitStateForces);
   GET_IFACE2(GetBroker(),IStrandGeometry,pStrandGeom);
   GET_IFACE2(GetBroker(),IPretensionForce,pPrestressForce);
   GET_IFACE2(GetBroker(),ISectionProperties,pSectProps);
   GET_IFACE2(GetBroker(),IBridge,pBridge);
   GET_IFACE2(GetBroker(),ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary);
   StrandIndexType Nt    = config.PrestressConfig.GetStrandCount(pgsTypes::Temporary);

   std::vector<StressCheckTask>::iterator iter(m_StressCheckTasks.begin());
   std::vector<StressCheckTask>::iterator end(m_StressCheckTasks.end());
   for ( ; iter != end; iter++ )
   {
      StressCheckTask& task = *iter;
      if ( !pLimits->IsConcreteStressLimitApplicable(segmentKey,task) )
      {
         // this stress check is not applicable... continue to the next task
         continue;
      }

      Float64 fcgdr;
      if ( task.intervalIdx == releaseIntervalIdx )
      {
         fcgdr = config.fci;
      }
      else
      {
         fcgdr = config.fc;
      }

      // get allowable stress
      Float64 fLimit(0.0);
      if(task.stressType == pgsTypes::Compression)
      {
         ATLASSERT(task.limitState != pgsTypes::ServiceIII);
         fLimit = pLimits->GetSegmentConcreteCompressionStressLimit(poi,task,fcgdr);
      }
      else // tension
      {
#if defined _DEBUG
         if ( liveLoadIntervalIdx <= task.intervalIdx && task.limitState == pgsTypes::ServiceI)
         {
            ATLASSERT(pLimits->CheckFinalDeadLoadTensionStress());
         }
#endif
         fLimit = pLimits->GetSegmentConcreteTensionStressLimit(poi,task,fcgdr,false/*bWithBondedReinforcement*/);
      }

      pgsTypes::BridgeAnalysisType batTop, batBottom;
      GetBridgeAnalysisType(segmentKey.girderIndex,task,batTop,batBottom);

      //
      // Get the stresses due to externally applied loads
      //
      Float64 fTopMinExt, fTopMaxExt;
      Float64 fBotMinExt, fBotMaxExt;
      pLimitStateForces->GetDesignStress(task,poi,pgsTypes::TopGirder,   &config, batTop,   &fTopMinExt,&fTopMaxExt);
      pLimitStateForces->GetDesignStress(task,poi,pgsTypes::BottomGirder,&config, batBottom,&fBotMinExt,&fBotMaxExt);

      Float64 Pps;
      if ( liveLoadIntervalIdx <= task.intervalIdx )
      {
         Float64 Pperm = pPrestressForce->GetPrestressForceWithLiveLoad(poi,pgsTypes::Permanent,task.limitState,INVALID_INDEX/*controlling live load*/,&config);
         Float64 Ptemp = pPrestressForce->GetPrestressForceWithLiveLoad(poi,pgsTypes::Temporary,task.limitState, INVALID_INDEX/*controlling live load*/,&config);
         Pps = Pperm + Ptemp;
      }
      else
      {
         Float64 Pperm = pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,task.intervalIdx,pgsTypes::End,pgsTypes::TransferLengthType::Minimum,&config);
         Float64 Ptemp = pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,task.intervalIdx,pgsTypes::End,pgsTypes::TransferLengthType::Minimum,&config);
         Pps = Pperm + Ptemp;
      }

      Float64 k = pLoadFactors->GetDCMax(task.limitState);
      Pps *= k;

      // Section props - bare girder
      Float64 Ag  = pSectProps->GetAg(releaseIntervalIdx, poi);
      Float64 Stg = pSectProps->GetS(releaseIntervalIdx, poi, pgsTypes::TopGirder);
      Float64 Sbg = pSectProps->GetS(releaseIntervalIdx, poi, pgsTypes::BottomGirder);

      // Upper and lower bound eccentricities
      Float64 ub_ecc, lb_ecc;
      if(task.stressType == pgsTypes::Compression)
      {
         // lb ecc will be for top stress, ub ecc for bottom stress
         lb_ecc = (-Pps/Ag - fLimit + fTopMinExt)*Stg/Pps;
         ub_ecc = (-Pps/Ag - fLimit + fBotMinExt)*Sbg/Pps;
      }
      else
      {
         // Opposite for tension
         ub_ecc = (-Pps/Ag - fLimit + fTopMaxExt)*Stg/Pps;
         lb_ecc = (-Pps/Ag - fLimit + fBotMaxExt)*Sbg/Pps;
      }

      // Compare and store controlling values in data object
      envData.SaveControllingUpperBound(ub_ecc, task.stressType, task.intervalIdx, task.limitState);
      envData.SaveControllingLowerBound(lb_ecc, task.stressType, task.intervalIdx, task.limitState);
   }

   return envData;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void pgsDesigner2::MakeCopy(const pgsDesigner2& rOther)
{
   // Add copy code here...
   m_pBroker = rOther.m_pBroker;
   m_StatusGroupID = rOther.m_StatusGroupID;
}

void pgsDesigner2::MakeAssignment(const pgsDesigner2& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================

void pgsDesigner2::CheckTendonDetailing(const CGirderKey& girderKey,pgsGirderArtifact* pGirderArtifact) const
{
   // Check LRFD 5.4.6.2 - Size of Ducts

   ASSERT_GIRDER_KEY(girderKey);

   GET_IFACE2(GetBroker(),IBridge,pBridge);
   GET_IFACE2(GetBroker(),IDuctLimits, pDuctLimits);
   GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);
   GET_IFACE2_NOCHECK(GetBroker(),IGirder,pGirder);
   GET_IFACE2_NOCHECK(GetBroker(),IIntervals, pIntervals); // only used if there are tendons

   // check segment tendons
   GET_IFACE2(GetBroker(),ISegmentTendonGeometry, pSegmentTendonGeometry);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);

      auto* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);

      Float64 Kmax = pDuctLimits->GetSegmentTendonAreaLimit(segmentKey);
      Float64 Tmax = pDuctLimits->GetSegmentTendonDuctSizeLimit(segmentKey);
      Float64 Rmin = pDuctLimits->GetSegmentTendonRadiusOfCurvatureLimit(segmentKey);

      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_ERECTED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 1);
      const pgsPointOfInterest& poi(vPoi.front());

      IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);
      DuctIndexType nDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);
      for (DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++)
      {
         Float64 Apt = pSegmentTendonGeometry->GetSegmentTendonArea(segmentKey, stressTendonIntervalIdx, ductIdx);
         Float64 Aduct = pSegmentTendonGeometry->GetInsideDuctArea(segmentKey, ductIdx);

         // starting with 9th edition, the duct diameter limit and the duct reduction for shear is based on nominal duct diameter
         Float64 duct_diameter = (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::NinthEdition2020 ? pSegmentTendonGeometry->GetOutsideDiameter(segmentKey,ductIdx) : pSegmentTendonGeometry->GetNominalDiameter(segmentKey, ductIdx));

         Float64 r = pSegmentTendonGeometry->GetMinimumRadiusOfCurvature(segmentKey, ductIdx);

         Float64 tWebMin = pGirder->GetWebThicknessAtDuct(poi, ductIdx);

         pgsDuctSizeArtifact artifact;
         artifact.SetDuctArea(Apt, Aduct, Kmax);
         artifact.SetDuctSize(duct_diameter, tWebMin, Tmax);
         artifact.SetRadiusOfCurvature(r, Rmin);

         pSegmentArtifact->SetDuctSizeArtifact(ductIdx, artifact);
      }
   }

   // check girder tendons

   PoiList vPoi;
   pPoi->GetPointsOfInterest(CSegmentKey(girderKey, ALL_SEGMENTS), POI_5L | POI_ERECTED_SEGMENT, &vPoi);
   ATLASSERT(vPoi.size() == nSegments);

   // Determine maximum duct area
   Float64 Kmax = pDuctLimits->GetGirderTendonAreaLimit(girderKey);
   Float64 Tmax = pDuctLimits->GetGirderTendonDuctSizeLimit(girderKey);
   Float64 Rmin = pDuctLimits->GetGirderTendonRadiusOfCurvatureLimit(girderKey);

   GET_IFACE2(GetBroker(),IGirderTendonGeometry,pGirderTendonGeometry);
   DuctIndexType nDucts = pGirderTendonGeometry->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressGirderTendonInterval(girderKey,ductIdx);
      Float64 Apt = pGirderTendonGeometry->GetGirderTendonArea(girderKey,stressTendonIntervalIdx,ductIdx);
      Float64 Aduct = pGirderTendonGeometry->GetInsideDuctArea(girderKey,ductIdx);

      // starting with 9th edition, the duct diameter limit and the duct reduction for shear is based on nominal duct diameter
      Float64 duct_diameter = (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::NinthEdition2020 ? pGirderTendonGeometry->GetOutsideDiameter(girderKey, ductIdx) : pGirderTendonGeometry->GetNominalDiameter(girderKey, ductIdx));

      Float64 r = pGirderTendonGeometry->GetMinimumRadiusOfCurvature(girderKey,ductIdx);

      Float64 tWebMin = DBL_MAX;
      for ( const pgsPointOfInterest& poi : vPoi)
      {
         Float64 minWebWidth = pGirder->GetWebThicknessAtDuct(poi,ductIdx);
         tWebMin = Min(tWebMin,minWebWidth);
      }

      pgsDuctSizeArtifact artifact;
      artifact.SetDuctArea(Apt,Aduct,Kmax);
      artifact.SetDuctSize(duct_diameter,tWebMin,Tmax);
      artifact.SetRadiusOfCurvature(r,Rmin);

      pGirderArtifact->SetDuctSizeArtifact(ductIdx,artifact);
   }
}

void pgsDesigner2::CheckTendonStresses(const CGirderKey& girderKey,pgsGirderArtifact* pGirderArtifact) const
{
   // Check LRFD 5.9.2.2
   ASSERT_GIRDER_KEY(girderKey);

   GET_IFACE2(GetBroker(),ISegmentTendonGeometry, pSegmentTendonGeometry);
   SegmentIndexType nMaxSegmentDucts = pSegmentTendonGeometry->GetMaxDuctCount(girderKey);

   GET_IFACE2(GetBroker(),IGirderTendonGeometry,pGirderTendonGeometry);
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);
   if (nMaxSegmentDucts+nGirderDucts == 0 )
   {
      return;
   }

   GET_IFACE2(GetBroker(),IEAFProgress, pProgress);
   WBFL::EAF::AutoProgress ap(pProgress);

   GET_IFACE2_NOCHECK(GetBroker(),IPointOfInterest, pPoi);
   GET_IFACE2(GetBroker(),IPosttensionForce,pPTForce);
   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   GET_IFACE2(GetBroker(),ITendonStressLimit,pLimits);

   IntervalIndexType finalIntervalIdx = pIntervals->GetIntervalCount()-1;

   // Check segment tendons
   GET_IFACE2(GetBroker(),IBridge, pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey, segIdx);

      auto* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);

      IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressSegmentTendonInterval(segmentKey);
      DuctIndexType nSegmentDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);
      for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
      {
         std::_tostringstream os;
         os << _T("Checking tendon stresses for Group ") << LABEL_GROUP(segmentKey.groupIndex)
            << _T(" Girder ") << LABEL_GIRDER(segmentKey.girderIndex)
            << _T(" Segment ") << LABEL_SEGMENT(segmentKey.segmentIndex)
            << _T(" Duct ") << LABEL_DUCT(ductIdx)
            << std::ends;
         pProgress->UpdateMessage(os.str().c_str());


         pgsTypes::JackingEndType jackingEnd = pSegmentTendonGeometry->GetJackingEnd(segmentKey, ductIdx);

         Float64 fpbtMax = -DBL_MAX;
         Float64 fseatMax = -DBL_MAX;
         Float64 fanchorMax = -DBL_MAX;
         Float64 fpeMax = -DBL_MAX;
         
         PoiList vPoi;
         pPoi->GetPointsOfInterest(segmentKey, &vPoi);
         for (const pgsPointOfInterest& poi : vPoi)
         {
            Float64 fpbt = pPTForce->GetSegmentTendonStress(poi, stressTendonIntervalIdx, pgsTypes::Start, ductIdx);
            fpbtMax = Max(fpbtMax, fpbt);

            Float64 fseat = pPTForce->GetSegmentTendonStress(poi, stressTendonIntervalIdx, pgsTypes::End, ductIdx);
            fseatMax = Max(fseatMax, fseat);

            Float64 fpe = pPTForce->GetSegmentTendonStress(poi, finalIntervalIdx, pgsTypes::End, ductIdx);
            fpeMax = Max(fpeMax, fpe);
         }

         if (jackingEnd == pgsTypes::jeStart)
         {
            fanchorMax = pPTForce->GetSegmentTendonStress(vPoi.front(), stressTendonIntervalIdx, pgsTypes::End, ductIdx);
         }
         else if (jackingEnd == pgsTypes::jeEnd)
         {
            fanchorMax = pPTForce->GetSegmentTendonStress(vPoi.back(), stressTendonIntervalIdx, pgsTypes::End, ductIdx);
         }
         else
         {
            fanchorMax = Max(pPTForce->GetSegmentTendonStress(vPoi.front(), stressTendonIntervalIdx, pgsTypes::End, ductIdx), pPTForce->GetSegmentTendonStress(vPoi.back(), stressTendonIntervalIdx, pgsTypes::End, ductIdx));
         }

         pgsTendonStressArtifact artifact;
         if (pLimits->CheckTendonStressAtJacking())
         {
            artifact.SetCheckAtJacking(pLimits->GetSegmentTendonStressLimitAtJacking(segmentKey), pSegmentTendonGeometry->GetFpj(segmentKey, ductIdx));
         }
         else
         {
            artifact.SetCheckPriorToSeating(pLimits->GetSegmentTendonStressLimitPriorToSeating(segmentKey), fpbtMax);
         }

         artifact.SetCheckAtAnchoragesAfterSeating(pLimits->GetSegmentTendonStressLimitAfterAnchorSetAtAnchorage(segmentKey), fanchorMax);
         artifact.SetCheckAfterSeating(pLimits->GetSegmentTendonStressLimitAfterAnchorSet(segmentKey), fseatMax);
         artifact.SetCheckAfterLosses(pLimits->GetSegmentTendonStressLimitAfterLosses(segmentKey), fpeMax);
         pSegmentArtifact->SetTendonStressArtifact(ductIdx, artifact);
      }
   }

   // Check girder tendons
   for ( DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++ )
   {
      std::_tostringstream os;
      os << _T("Checking tendon stresses for Group ") << LABEL_GROUP(girderKey.groupIndex) 
         << _T(" Girder ") << LABEL_GIRDER(girderKey.girderIndex) 
         << _T(" Duct ") << LABEL_DUCT(ductIdx)
         << std::ends;
      pProgress->UpdateMessage( os.str().c_str() );

      IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressGirderTendonInterval(girderKey,ductIdx);

      pgsTypes::JackingEndType jackingEnd = pGirderTendonGeometry->GetJackingEnd(girderKey,ductIdx);

      Float64 fpbtMax    = -DBL_MAX;
      Float64 fseatMax   = -DBL_MAX;
      Float64 fanchorMax = -DBL_MAX;
      Float64 fpeMax     = -DBL_MAX;
      GET_IFACE2(GetBroker(),IPointOfInterest,pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(CSegmentKey(girderKey.groupIndex, girderKey.girderIndex, ALL_SEGMENTS), &vPoi);
      for ( const pgsPointOfInterest& poi : vPoi)
      {
         Float64 fpbt = pPTForce->GetGirderTendonStress(poi,stressTendonIntervalIdx,pgsTypes::Start,ductIdx);
         fpbtMax = Max(fpbtMax,fpbt);

         Float64 fseat = pPTForce->GetGirderTendonStress(poi,stressTendonIntervalIdx,pgsTypes::End,ductIdx);
         fseatMax = Max(fseatMax,fseat);

         Float64 fpe = pPTForce->GetGirderTendonStress(poi,finalIntervalIdx,pgsTypes::End,ductIdx);
         fpeMax = Max(fpeMax,fpe);
      }

      const pgsPointOfInterest* ppoiStart;
      const pgsPointOfInterest* ppoiEnd;
      pPoi->GetDuctRange(girderKey, ductIdx, &ppoiStart, &ppoiEnd);

      if ( jackingEnd == pgsTypes::jeStart )
      {
         fanchorMax = pPTForce->GetGirderTendonStress(*ppoiStart,stressTendonIntervalIdx,pgsTypes::End,ductIdx);
      }
      else if ( jackingEnd == pgsTypes::jeEnd )
      {
         fanchorMax = pPTForce->GetGirderTendonStress(*ppoiEnd,stressTendonIntervalIdx,pgsTypes::End,ductIdx);
      }
      else
      {
         fanchorMax = Max(pPTForce->GetGirderTendonStress(*ppoiStart,stressTendonIntervalIdx,pgsTypes::End,ductIdx),pPTForce->GetGirderTendonStress(*ppoiEnd,stressTendonIntervalIdx,pgsTypes::End,ductIdx));
      }

      pgsTendonStressArtifact artifact;
      if ( pLimits->CheckTendonStressAtJacking() )
      {
         artifact.SetCheckAtJacking(pLimits->GetGirderTendonStressLimitAtJacking(girderKey),pGirderTendonGeometry->GetFpj(girderKey,ductIdx));
      }
      else
      {
         artifact.SetCheckPriorToSeating(pLimits->GetGirderTendonStressLimitPriorToSeating(girderKey),fpbtMax);
      }

      artifact.SetCheckAtAnchoragesAfterSeating(pLimits->GetGirderTendonStressLimitAfterAnchorSetAtAnchorage(girderKey),fanchorMax);
      artifact.SetCheckAfterSeating(pLimits->GetGirderTendonStressLimitAfterAnchorSet(girderKey),fseatMax);
      artifact.SetCheckAfterLosses(pLimits->GetGirderTendonStressLimitAfterLosses(girderKey),fpeMax);
      pGirderArtifact->SetTendonStressArtifact(ductIdx,artifact);
   }
}

void pgsDesigner2::CheckStrandStresses(const CSegmentKey& segmentKey,pgsStrandStressArtifact* pArtifact) const
{
   GET_IFACE2(GetBroker(),IStrandStressLimit,pAllow);
   GET_IFACE2(GetBroker(),IPretensionForce, pPsForce);
   GET_IFACE2(GetBroker(),IPointOfInterest,pPoi);

   GET_IFACE2(GetBroker(),IEAFProgress, pProgress);
   WBFL::EAF::AutoProgress ap(pProgress);

   std::_tostringstream os;
   os << _T("Checking strand stresses for Group ") << LABEL_GROUP(segmentKey.groupIndex) 
      << _T(" Girder ") << LABEL_GIRDER(segmentKey.girderIndex) 
      << _T(" Segment ") << LABEL_SEGMENT(segmentKey.segmentIndex) 
      << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );
   
   PoiList vPOI;
   pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT, &vPOI);
   ATLASSERT(vPOI.size() == 1);
   const pgsPointOfInterest& mid_span_poi(vPOI.front());

   pArtifact->SetPointOfInterest(mid_span_poi);

   ATLASSERT(segmentKey == mid_span_poi.GetSegmentKey());

   GET_IFACE2(GetBroker(),ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   std::vector<pgsTypes::StrandType> strandTypes{ pgsTypes::Straight, pgsTypes::Harped };

   GET_IFACE2(GetBroker(),IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);
   if ( 0 < Nt )
   {
      ATLASSERT(Nt != INVALID_INDEX);
      strandTypes.push_back(pgsTypes::Temporary);
   }

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType jackIntervalIdx = pIntervals->GetStressStrandInterval(segmentKey);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   std::vector<pgsTypes::StrandType>::iterator standTypeIter(strandTypes.begin());
   std::vector<pgsTypes::StrandType>::iterator standTypeIterEnd(strandTypes.end());
   for ( ; standTypeIter != standTypeIterEnd; standTypeIter++ )
   {
      pgsTypes::StrandType strandType = *standTypeIter;

      if ( pAllow->CheckStrandStressAtJacking() )
      {
         pArtifact->SetCheckAtJacking( strandType, pPsForce->GetEffectivePrestress(mid_span_poi,strandType,jackIntervalIdx,pgsTypes::Start), pAllow->GetStrandStressLimitAtJacking(segmentKey,strandType) );
      }

      if ( pAllow->CheckStrandStressBeforeXfer() )
      {
         pArtifact->SetCheckBeforeXfer( strandType, pPsForce->GetEffectivePrestress(mid_span_poi,strandType,jackIntervalIdx,pgsTypes::End/*pgsTypes::BeforeXfer*/), pAllow->GetStrandStressLimitBeforeXfer(segmentKey,strandType) );
      }

      if ( pAllow->CheckStrandStressAfterXfer() )
      {
         pArtifact->SetCheckAfterXfer( strandType, pPsForce->GetEffectivePrestress(mid_span_poi,strandType,releaseIntervalIdx,pgsTypes::Start/*pgsTypes::AfterXfer*/), pAllow->GetStrandStressLimitAfterXfer(segmentKey,strandType) );
      }

      if ( pAllow->CheckStrandStressAfterLosses() && strandType != pgsTypes::Temporary )
      {
         // LRFD 5.9.2.2 is a Service I check
         Float64 fpe = pPsForce->GetEffectivePrestressWithLiveLoad(mid_span_poi,strandType,pgsTypes::ServiceI,true/*include elastic gains*/, false/*don't apply elastic gain reduction factor*/);
         pArtifact->SetCheckAfterLosses( strandType, fpe, pAllow->GetStrandStressLimitAfterLosses(segmentKey,strandType) );
      }
   }
}

void pgsDesigner2::CheckSegmentStresses(const CSegmentKey& segmentKey,const PoiList& vPoi,const StressCheckTask& task,pgsSegmentArtifact* pSegmentArtifact) const
{
   USES_CONVERSION;

   GET_IFACE2(GetBroker(),IEAFProgress, pProgress);
   WBFL::EAF::AutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Checking Girder Stresses"));

   GET_IFACE2(GetBroker(),IIntervals, pIntervals);
   IntervalIndexType releaseIntervalIdx              = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx             = pIntervals->GetFirstCastDeckInterval();
   IntervalIndexType tsRemovalIntervalIdx            = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx             = pIntervals->GetLiveLoadInterval();
   IntervalIndexType compositeClosureIntervalIdx     = pIntervals->GetCompositeClosureJointInterval(segmentKey);
   IntervalIndexType lastIntervalIdx                 = pIntervals->GetIntervalCount()-1;

   bool bSISpec = WBFL::LRFD::BDSManager::GetUnits() == WBFL::LRFD::BDSManager::Units::SI ? true : false;

   GET_IFACE2(GetBroker(),IPretensionStresses,       pPretensionStresses);
   GET_IFACE2(GetBroker(),ILimitStateForces,         pLimitStateForces);
   GET_IFACE2(GetBroker(),IPrecompressedTensileZone, pPrecompressedTensileZone);
   GET_IFACE2(GetBroker(),ISegmentTendonGeometry, pSegmentTendonGeometry);
   GET_IFACE2(GetBroker(),IGirderTendonGeometry, pGirderTendonGeometry);

   GET_IFACE2(GetBroker(),ILoadFactors,              pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();

   // these interfaces only get used if the task type is tension. however we need them
   // inside the POI loop and don't want to get them every time.
   GET_IFACE2_NOCHECK(GetBroker(),IBridge,            pBridge);
   GET_IFACE2_NOCHECK(GetBroker(),IGirder,            pGirder);
   GET_IFACE2_NOCHECK(GetBroker(),ISectionProperties, pSectProps);
   GET_IFACE2_NOCHECK(GetBroker(),IShapes,            pShapes);
   GET_IFACE2_NOCHECK(GetBroker(),IMaterials,         pMaterials);
   GET_IFACE2_NOCHECK(GetBroker(),ILongRebarGeometry, pRebarGeom);
   GET_IFACE2_NOCHECK(GetBroker(),IPointOfInterest,   pPoi);

   GET_IFACE2_NOCHECK(GetBroker(),IConcreteStressLimits,  pLimits );
   GET_IFACE2_NOCHECK(GetBroker(),IProductForces,     pProductForces); // only used for spliced girders


   GET_IFACE2(GetBroker(),ILossParameters, pLossParams);
   bool bTimeStepAnalysis = (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP ? true : false);

   int nElementsToCheck = (bTimeStepAnalysis ? 2 : 1); // check girder and deck for timestep, otherwise only check girder

   pgsTypes::BridgeAnalysisType batTop, batBottom;
   GetBridgeAnalysisType(segmentKey.girderIndex,task,batTop,batBottom);

   bool bIsDeckPrecompressed = pPrecompressedTensileZone->IsDeckPrecompressed(segmentKey);

   bool bIsStressingInterval = pIntervals->IsStressingInterval(segmentKey, task.intervalIdx);
   bool bIsSegmentTendonStressingInterval = pIntervals->IsSegmentTendonStressingInterval(segmentKey, task.intervalIdx); // not used yet... need to look at this method more closely
   bool bIsGirderTendonStressingInterval = pIntervals->IsGirderTendonStressingInterval(segmentKey, task.intervalIdx);

   SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);
   PoiList poiList;
   pPoi->GetPointsOfInterest(segmentKey, POI_RELEASED_SEGMENT | POI_5L, &poiList);
   ATLASSERT(poiList.size() == 1);
   const pgsPointOfInterest& midsegment_poi(poiList.front());
   ATLASSERT(midsegment_poi.IsMidSpan(POI_RELEASED_SEGMENT));

   // For time-step analysis, pretension stresses are taken from the interval when the prestress is applied or removed (temporary strands),
   // otherwise the are taken from the task interval.
   // We do this because in time-step analysis girder stresses include the effect of creep, shrinkage, and relaxation. We don't
   // want to include CR, SH, and RE again by computing the stress in the girder due to prestressing with an effective prestress
   // force that includes CR, SH, and RE.
   IntervalIndexType pretensionIntervalIdx = (bTimeStepAnalysis ? 
      releaseIntervalIdx <= task.intervalIdx && task.intervalIdx < tsRemovalIntervalIdx ? releaseIntervalIdx : tsRemovalIntervalIdx 
      : task.intervalIdx);

   GET_IFACE2(GetBroker(),IDocumentType, pDocType);
   bool bCheckTemporaryStresses = false;
   if (pDocType->IsPGSuperDocument())
   {
      bCheckTemporaryStresses = pLimits->CheckTemporaryStresses();
      if (task.intervalIdx != tsRemovalIntervalIdx && task.intervalIdx != castDeckIntervalIdx)
      {
         // if this is not one of the temporary condition intervals, don't check temporary stresses
         bCheckTemporaryStresses = false;
      }
   }

   DuctIndexType nSegmentDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(segmentKey);

   for(const pgsPointOfInterest& poi : vPoi)
   {
      ATLASSERT(poi.GetSegmentKey() == segmentKey);

      IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
      IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);

      pgsFlexuralStressArtifact artifact(poi,task);

      DLOG(_T("Checking at ") << WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart(),WBFL::Units::Measure::Feet) << _T(" ft") << _T(" (POI ID ") << poi.GetID() << _T(")"));

      if(releaseIntervalIdx <= task.intervalIdx)
      {
	      for ( int i = 0; i < nElementsToCheck; i++ )
	      {
	         pgsTypes::StressLocation topStressLocation = (i == 0 ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
	         pgsTypes::StressLocation botStressLocation = (i == 0 ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);
	
	         std::array<bool,2> bIsInPTZ;
	         pPrecompressedTensileZone->IsInPrecompressedTensileZone(poi,task.limitState,topStressLocation,botStressLocation,&bIsInPTZ[TOP],&bIsInPTZ[BOT]);
	         
	         artifact.IsInPrecompressedTensileZone(topStressLocation,bIsInPTZ[TOP]);
	         artifact.IsInPrecompressedTensileZone(botStressLocation,bIsInPTZ[BOT]);
	
	         Float64 lambda;
	         if ( i == 0 )
	         {
	            lambda = pMaterials->GetSegmentLambda(segmentKey);
	         }
	         else
	         {
	            lambda = pMaterials->GetDeckLambda();
	         }
	
	         //
	         // Determine applicability of the stress check
	         //
	         if ( i == 0 /*girder stresses*/ )
	         {
	            ATLASSERT(IsGirderStressLocation(topStressLocation));
	            ATLASSERT(IsGirderStressLocation(botStressLocation));
	
	            // Stress check in Girder Segment or Closure Joint (not in deck)
	            CClosureKey closureKey;
	            if ( pPoi->IsInClosureJoint(poi,&closureKey) )
	            {
	               // Stress check in Closure Joint
	
	               // Top and bottom stress check is always applicable at a closure joint
	               // after it is made composite, otherwise closure joint can't take load
	               artifact.IsApplicable(topStressLocation,compositeClosureIntervalIdx <= task.intervalIdx ? true : false);
	               artifact.IsApplicable(botStressLocation,compositeClosureIntervalIdx <= task.intervalIdx ? true : false);
	            }
	            else
	            {
	               // Stress check in Girder Segment
	               if ( task.stressType == pgsTypes::Compression )
	               {
	                  // Compression in the girder segment... always check
	                  artifact.IsApplicable(topStressLocation, true);
	                  artifact.IsApplicable(botStressLocation, true);
	               }
	               else
	               {
	                  // Tension in the girder segment
	                  ATLASSERT(task.stressType == pgsTypes::Tension);
	                  if ( task.intervalIdx == releaseIntervalIdx || bIsSegmentTendonStressingInterval || bIsGirderTendonStressingInterval )
	                  {
	                     // During a stressing activity, tension stress checks are only performed in areas
	                     // other that the precompressed tensile zone
                         //
                         // Starting with the 10th edition, the tension stress check is always performed
	                     artifact.IsApplicable( topStressLocation, WBFL::LRFD::BDSManager::Edition::TenthEdition2024 <= WBFL::LRFD::BDSManager::GetEdition() ? true : !bIsInPTZ[TOP] );
	                     artifact.IsApplicable( botStressLocation, WBFL::LRFD::BDSManager::Edition::TenthEdition2024 <= WBFL::LRFD::BDSManager::GetEdition() ? true : !bIsInPTZ[BOT] );
                     }
	                  else if ( bCheckTemporaryStresses )
	                  {
	                     if ( bIsStressingInterval )
	                     {
	                        // During a stressing activity, tension stress checks are only performed in areas
	                        // other that the precompressed tensile zone
                            //
                            // Starting with the 10th edition, the tension stress check is always performed
	                        ATLASSERT(task.intervalIdx == tsRemovalIntervalIdx);
	                        artifact.IsApplicable( topStressLocation, WBFL::LRFD::BDSManager::Edition::TenthEdition2024 <= WBFL::LRFD::BDSManager::GetEdition() ? true : !bIsInPTZ[TOP] );
	                        artifact.IsApplicable( botStressLocation, WBFL::LRFD::BDSManager::Edition::TenthEdition2024 <= WBFL::LRFD::BDSManager::GetEdition() ? true : !bIsInPTZ[BOT] );
	                     }
	                     else
	                     {
	                        // This is a non-stressing interval so tension stress checks are only performed
	                        // in the precompressed tensile zone
	                        ATLASSERT(task.intervalIdx == castDeckIntervalIdx);
	                        artifact.IsApplicable( topStressLocation, bIsInPTZ[TOP] );
	                        artifact.IsApplicable( botStressLocation, bIsInPTZ[BOT] );
	                     }
	                  }
	                  else
	                  {
	                     // This is a non-stressing interval so tension stress checks are only performed
	                     // in the precompressed tensile zone
	                     artifact.IsApplicable( topStressLocation, bIsInPTZ[TOP] );
	                     artifact.IsApplicable( botStressLocation, bIsInPTZ[BOT] );
	                  }
	               }
	            }
	         }
	         else
	         {
	            // Stress checks in Deck
	            ATLASSERT(IsDeckStressLocation(topStressLocation));
	            ATLASSERT(IsDeckStressLocation(botStressLocation));
	
	            if ( task.stressType == pgsTypes::Compression )
	            {
	               // compression in the deck
	               // stress checks are only applicable if the deck is composite and if it 
	               // has been precompressed due to PT being applied after it is composite
	               artifact.IsApplicable(topStressLocation, compositeDeckIntervalIdx <= task.intervalIdx && bIsDeckPrecompressed ? true : false);
	               artifact.IsApplicable(botStressLocation, compositeDeckIntervalIdx <= task.intervalIdx && bIsDeckPrecompressed ? true : false);
	            }
	            else
	            {
	               // tension in the deck
	               ATLASSERT(task.stressType == pgsTypes::Tension);
	               if ( task.intervalIdx == releaseIntervalIdx )
	               {
	                  // stress checks are never applicable in the deck at pretension release
	                  artifact.IsApplicable( topStressLocation, false);
	                  artifact.IsApplicable( botStressLocation, false);
	               }
	               else
	               {
	                  // deck is checked for tension if the deck is composite in this interval, the deck
	                  // has been precompressed due to PT applied after the deck is composite, and
	                  // the stress location is not in the precompressed tensile zone during a PT-stressing interval
	                  // or in the precompressed tensile zone in a non-PT-stressing interval
                      //
                      // Starting with the 10th edition, the tension stress check is always performed in tendon stressing intervals
	                  artifact.IsApplicable( topStressLocation, 
	                     compositeDeckIntervalIdx <= task.intervalIdx && // composite deck
	                     bIsDeckPrecompressed // deck is precompressed
	                     && (
	                           ( (bIsSegmentTendonStressingInterval || bIsGirderTendonStressingInterval) && WBFL::LRFD::BDSManager::Edition::TenthEdition2024 <= WBFL::LRFD::BDSManager::GetEdition() ? true : !bIsInPTZ[TOP]) || // this is a tendon stressing interval and stress location is NOT in the PTZ -OR-
	                           (!(bIsSegmentTendonStressingInterval || bIsGirderTendonStressingInterval) &&  bIsInPTZ[TOP]) // this is NOT a tendon stressing interval and the stress location is in the PTZ
	                         ));
	
	                  artifact.IsApplicable( botStressLocation, 
	                     compositeDeckIntervalIdx <= task.intervalIdx && // composite deck
	                     bIsDeckPrecompressed // deck is precompressed
	                     && (
	                           ( (bIsSegmentTendonStressingInterval || bIsGirderTendonStressingInterval) && WBFL::LRFD::BDSManager::Edition::TenthEdition2024 <= WBFL::LRFD::BDSManager::GetEdition() ? true : !bIsInPTZ[BOT]) || // this is a tendon stressing interval and stress location is NOT in the PTZ -OR-
	                           (!(bIsSegmentTendonStressingInterval || bIsGirderTendonStressingInterval) &&  bIsInPTZ[BOT]) // this is NOT a tendon stressing interval and the stress location is in the PTZ
	                         ));
	
	               }
	            }
	         }
	
	         // Special Case... 
	         // After bridge is open to traffic, we only check tension under three conditions
            // 1) Service III limit state
            // 2) Service I limit state and CheckFinalDeadLoadTensionStress() is true
            // 3) It is a UHPC Segment and its the Fatigue I limit state
            // otherwise, only compression is checked for the "Effective Prestress + Permanent Loads only case" (LRFD 5.9.2.3.2 (pre2017: 5.9.4.2)).
	         if (liveLoadIntervalIdx <= task.intervalIdx && task.stressType == pgsTypes::Tension )
	         {
               bool bIsApplicable = (
                  IsServiceIIILimitState(task.limitState) || 
                  (task.limitState == pgsTypes::ServiceI && pLimits->CheckFinalDeadLoadTensionStress()) ||
                  (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC && task.limitState == pgsTypes::FatigueI) ) ? true : false;
	            artifact.IsApplicable(topStressLocation, bIsInPTZ[TOP] ? bIsApplicable : false);
	            artifact.IsApplicable(botStressLocation, bIsInPTZ[BOT] ? bIsApplicable : false);
	         }
	
	         //
	         // Do the stress check
	         //
	
	         // NOTE, don't return here if stress check is not applicable. We want to capture the stress information
	         // at this POI for reporting purposes

            // NOTE: UHPC has a tension stress check for the fatigue limit state but it uses the Service I load combination. See GS 1.5.2.3.
            // The original UHPC Structural Design Guidance (SDG) presented this as a Fatigue I limit state check using the Service I limit
            // state combination. After AASHTO T-10 developed the GS, the requirements were moved to GS 1.5.2.3 and it no longer specifically
            // talks about the Fatigue I limit state, however it is a stress limit for cyclic loads. This is effectively a fatigue check.
            // This is totally different than anything we've seen before.  The task has been set up with the FatigueI limit state and Tension stress.
            // When this task occurs, we want to get the concrete stresses using the ServiceI limit state. For this reason, we create a local
            // limitState variable and assign it the task's limit state. If this is the UHPC Fatigue Tension check, we set the local limitState variable
            // to ServiceI. All the calls below use the local limitState variable instead of task.limitState.
            pgsTypes::LimitState limitState = task.limitState;
            if (task.limitState == pgsTypes::FatigueI && task.stressType == pgsTypes::Tension)
            {
               limitState = pgsTypes::ServiceI;
               ATLASSERT(pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC);
            }
	
	         // get segment stress due to prestressing
	         std::array<Float64,2> fPretension{ 0,0 };
            std::tie(fPretension[TOP],fPretension[BOT]) = pPretensionStresses->GetStress(pretensionIntervalIdx, poi, topStressLocation, botStressLocation, task.bIncludeLiveLoad, limitState, INVALID_INDEX/*controlling live load*/);

            DLOG(_T("Prestress Stress     :: Top = ") << WBFL::Units::ConvertFromSysUnits(fPretension[TOP],WBFL::Units::Measure::KSI) << _T(" ksi") << _T("    Bot = ") << WBFL::Units::ConvertFromSysUnits(fPretension[BOT],WBFL::Units::Measure::KSI) << _T(" ksi"));

	         // get segment stress due to external loads
	         std::array<Float64,2> fLimitStateMin{ 0,0 }, fLimitStateMax{ 0,0 };
	         pLimitStateForces->GetStress(task.intervalIdx,limitState,poi,batTop,false/*exclude prestress*/,topStressLocation,&fLimitStateMin[TOP],&fLimitStateMax[TOP]);
	         pLimitStateForces->GetStress(task.intervalIdx,limitState,poi,botStressLocation == pgsTypes::BottomGirder ? batBottom : batTop,false/*exclude prestress*/,botStressLocation,&fLimitStateMin[BOT],&fLimitStateMax[BOT]);

            if (liveLoadIntervalIdx <= task.intervalIdx && !task.bIncludeLiveLoad)
            {
               // the task interval is at or after the live load interval and this task does not include live load

               GET_IFACE2(GetBroker(),ILiveLoads, pLiveLoads);

               std::array<Float64, 2> LLIM_Min_to_remove{ 0,0 };
               std::array<Float64, 2> LLIM_Max_to_remove{ 0,0 };

               pgsTypes::LiveLoadType llType = pgsTypes::lltDesign;

               if (pLiveLoads->IsLiveLoadDefined(llType))
               {
                  // remove vehicular live load, including live load factor
                  Float64 gLLIMmin, gLLIMmax;
                  pLoadFactors->GetLLIM(task.limitState, &gLLIMmin, &gLLIMmax);

                  std::array<Float64, 2> fLLIMmin, fLLIMmax;
                  pProductForces->GetLiveLoadStress(task.intervalIdx, pgsTypes::lltDesign, poi, batTop, true/*include impact*/, true/*include LLDF*/, topStressLocation, topStressLocation, &fLLIMmin[TOP], &fLLIMmax[TOP], &fLLIMmin[BOT], &fLLIMmax[BOT]);
                  LLIM_Min_to_remove[TOP] += gLLIMmin * fLLIMmin[TOP];
                  LLIM_Max_to_remove[TOP] += gLLIMmax * fLLIMmax[TOP];

                  pProductForces->GetLiveLoadStress(task.intervalIdx, pgsTypes::lltDesign, poi, botStressLocation == pgsTypes::BottomGirder ? batBottom : batTop, true/*include impact*/, true/*include LLDF*/, botStressLocation, botStressLocation, & fLLIMmin[TOP], & fLLIMmax[TOP], & fLLIMmin[BOT], & fLLIMmax[BOT]);
                  LLIM_Min_to_remove[BOT] += gLLIMmin * fLLIMmin[BOT];
                  LLIM_Max_to_remove[BOT] += gLLIMmax * fLLIMmax[BOT];
               }

               GET_IFACE2(GetBroker(),IUserDefinedLoadData, pUserLoads);
               if (pUserLoads->HasUserLLIM(segmentKey))
               {
                  std::array<Float64, 2> fUserLLIM;
                  pProductForces->GetStress(task.intervalIdx, pgsTypes::pftUserLLIM, poi, batTop, rtCumulative, topStressLocation, botStressLocation, &fUserLLIM[TOP], &fUserLLIM[BOT]);
                  LLIM_Min_to_remove[TOP] += fUserLLIM[TOP];
                  LLIM_Max_to_remove[TOP] += fUserLLIM[TOP];

                  pProductForces->GetStress(task.intervalIdx, pgsTypes::pftUserLLIM, poi, botStressLocation == pgsTypes::BottomGirder ? batBottom : batTop, rtCumulative, topStressLocation, botStressLocation, &fUserLLIM[TOP], &fUserLLIM[BOT]);
                  LLIM_Min_to_remove[BOT] += fUserLLIM[BOT];
                  LLIM_Max_to_remove[BOT] += fUserLLIM[BOT];
               }

               ILiveLoads::PedestrianLoadApplicationType pedLoadType = pLiveLoads->GetPedestrianLoadApplication(llType);

               if (pedLoadType != ILiveLoads::PedDontApply)
               {
                  std::array<Float64, 2> PL_Min, PL_Max;

                  pProductForces->GetLiveLoadStress(task.intervalIdx, pgsTypes::lltPedestrian, poi, batTop, true/*include impact*/, true/*include LLDF*/, topStressLocation, topStressLocation, &PL_Min[TOP], &PL_Max[TOP], &PL_Min[BOT], &PL_Max[BOT]);

                  if (pLiveLoads->GetPedestrianLoadApplication(llType) == ILiveLoads::PedEnvelopeWithVehicular)
                  {
                     // PL is enveloped with LLIM so we want to remove the what that has the most extreme value
                     // Vehicular live load is stored in LLIM_Min/Max_to_remove
                     LLIM_Min_to_remove[TOP] = Min(LLIM_Min_to_remove[TOP], PL_Min[TOP]);
                     LLIM_Max_to_remove[TOP] = Max(LLIM_Max_to_remove[TOP], PL_Max[TOP]);
                  }
                  else
                  {
                     // PL is concurrent with vehicular LLIM so we want to add PL to LLIM for removal
                     ATLASSERT(pLiveLoads->GetPedestrianLoadApplication(llType) == ILiveLoads::PedConcurrentWithVehicular);
                     LLIM_Min_to_remove[TOP] += PL_Min[TOP];
                     LLIM_Max_to_remove[TOP] += PL_Max[TOP];
                  }

                  pProductForces->GetLiveLoadStress(task.intervalIdx, pgsTypes::lltPedestrian, poi, botStressLocation == pgsTypes::BottomGirder ? batBottom : batTop, true/*include impact*/, true/*include LLDF*/, botStressLocation, botStressLocation, & PL_Min[TOP], & PL_Max[TOP], & PL_Min[BOT], & PL_Max[BOT]);
                  if (pLiveLoads->GetPedestrianLoadApplication(llType) == ILiveLoads::PedEnvelopeWithVehicular)
                  {
                     // PL is enveloped with LLIM so we want to remove the what that has the most extreme value
                     // Vehicular live load is stored in LLIM_Min/Max_to_remove
                     LLIM_Min_to_remove[BOT] = Min(LLIM_Min_to_remove[BOT], PL_Min[BOT]);
                     LLIM_Max_to_remove[BOT] = Max(LLIM_Max_to_remove[BOT], PL_Max[BOT]);
                  }
                  else
                  {
                     // PL is concurrent with vehicular LLIM so we want to add PL to LLIM for removal
                     ATLASSERT(pLiveLoads->GetPedestrianLoadApplication(llType) == ILiveLoads::PedConcurrentWithVehicular);
                     LLIM_Min_to_remove[BOT] += PL_Min[BOT];
                     LLIM_Max_to_remove[BOT] += PL_Max[BOT];
                  }
               }

               // remove the live loads from the limit state results
               fLimitStateMin[TOP] -= LLIM_Min_to_remove[TOP];
               fLimitStateMin[BOT] -= LLIM_Min_to_remove[BOT];

               fLimitStateMax[TOP] -= LLIM_Max_to_remove[TOP];
               fLimitStateMax[BOT] -= LLIM_Max_to_remove[BOT];
            }
	         
	         std::array<Float64,2> fLimitState{ 0,0 };
	         fLimitState[TOP] = (task.stressType == pgsTypes::Compression ? fLimitStateMin[TOP] : fLimitStateMax[TOP] );
	         fLimitState[BOT] = (task.stressType == pgsTypes::Compression ? fLimitStateMin[BOT] : fLimitStateMax[BOT] );

            DLOG(_T("External Stress      :: Top = ") << WBFL::Units::ConvertFromSysUnits(fLimitState[TOP],WBFL::Units::Measure::KSI) << _T(" ksi") << _T("    Bot = ") << WBFL::Units::ConvertFromSysUnits(fLimitState[BOT],WBFL::Units::Measure::KSI) << _T(" ksi"));

	         // Use the DC load factor for the applicable limit state (e.g. 0.5 for Service IA per Tbl 5.9.4.2.1-1
	         // 2008 or before, or Fatigue I per LRFD 5.5.3.1 2009) rather than hard-coding it, so this respects
	         // load factors the user has customized in the Load Factors library (see LoadFactorsDlg) and stays
	         // consistent with the k-factor used elsewhere in the designer (e.g. RefineDesignForAllowableStress).
	         Float64 k = pLoadFactors->GetDCMax(limitState);

	         std::array<Float64,2> f{ 0,0 };
	         f[TOP] = fLimitState[TOP] + k*fPretension[TOP];
	         f[BOT] = fLimitState[BOT] + k*fPretension[BOT];
	
            // get segment stress due to post-tensioning
            if ( 0 < nSegmentDucts+nGirderDucts )
	         {
	#pragma Reminder("UPDATE: Stress due to PT may need to include live load")
	            // NOTE: in the call to pProductForces->GetStress for pftPostTensioning, it doesn't matter which bridge analysis type (bat) that we use because
	            // spliced girders (the only place we have PT) are always continuous so top/bot min/max BAT are always the same.
	            std::array<Float64,2> fPosttension;
	            pProductForces->GetStress(task.intervalIdx,pgsTypes::pftPostTensioning,poi,batTop,rtCumulative,topStressLocation,botStressLocation,&fPosttension[TOP],&fPosttension[BOT]);
	            f[TOP] += k*fPosttension[TOP];
	            f[BOT] += k*fPosttension[BOT];
	            artifact.SetPosttensionEffects( topStressLocation, fPosttension[TOP]);
	            artifact.SetPosttensionEffects( botStressLocation, fPosttension[BOT]);
	         }
	
	         f[TOP] = (IsZero(f[TOP]) ? 0 : f[TOP]);
	         f[BOT] = (IsZero(f[BOT]) ? 0 : f[BOT]);

            DLOG(_T("Resultant Stress     :: Top = ") << WBFL::Units::ConvertFromSysUnits(f[TOP],WBFL::Units::Measure::KSI) << _T(" ksi") << _T("    Bot = ") << WBFL::Units::ConvertFromSysUnits(f[BOT],WBFL::Units::Measure::KSI) << _T(" ksi"));

            artifact.SetDemand(             topStressLocation, f[TOP] );
	         artifact.SetExternalEffects(    topStressLocation, fLimitState[TOP]);
	         artifact.SetPretensionEffects(  topStressLocation, fPretension[TOP]);
	
	         artifact.SetDemand(             botStressLocation, f[BOT] );
	         artifact.SetExternalEffects(    botStressLocation, fLimitState[BOT]);
	         artifact.SetPretensionEffects(  botStressLocation, fPretension[BOT]);
	
            // sets the allowable stress limit in the artifact and computes and stores
            // the required concrete strength to satisfy the stress limit
#pragma Reminder("Computation of required concrete strength below and later in this function has duplicate logic with IAllowableConcreteStress::ComputeRequiredConcreteStrength(). Much of this was fixed in mantis 1334, but not here due to complexity. Consider consolidating this.")
	         ComputeConcreteStrength(artifact,topStressLocation,task);
	         ComputeConcreteStrength(artifact,botStressLocation,task);

            DLOG(_T("Allowable Stress     :: Top = ") << WBFL::Units::ConvertFromSysUnits(artifact.GetCapacity(topStressLocation),WBFL::Units::Measure::KSI) << _T(" ksi") << _T("    Bot = ") << WBFL::Units::ConvertFromSysUnits(artifact.GetCapacity(botStressLocation),WBFL::Units::Measure::KSI) << _T(" ksi"));

	         // compute the "with rebar" allowable tensile stress
            //

            CClosureKey closureKey;
            bool bIsInClosure = pPoi->IsInClosureJoint(poi, &closureKey);
            WBFL::Materials::ConcreteType concreteType;
            if (i == 0)
            {
               if (bIsInClosure)
               {
                  concreteType = (WBFL::Materials::ConcreteType)pMaterials->GetClosureJointConcreteType(closureKey);
               }
               else
               {
                  concreteType = (WBFL::Materials::ConcreteType)pMaterials->GetSegmentConcreteType(segmentKey);
               }
            }
            else
            {
               concreteType = (WBFL::Materials::ConcreteType)pMaterials->GetDeckConcreteType();
            }

            // Skip this case for UHPC since UHPC doesn't have a "with rebar" stress limit
            if ( task.stressType == pgsTypes::Tension && !IsUHPC(concreteType))
	         {
	            bool bIsTopApplicable = artifact.IsApplicable(topStressLocation); 
	            bool bIsBotApplicable = artifact.IsApplicable(botStressLocation);
	
	            if ( !bIsTopApplicable && !bIsBotApplicable )
	            {
	               // neither top and bottom are applicable for allowable stress checks... 
	               continue;
	            }
	
	            Float64 fTop = artifact.GetDemand(topStressLocation);
	            Float64 fBot = artifact.GetDemand(botStressLocation);
	
	            std::array<bool,2> IsAdequateRebar{false,false};
	
	            std::array<bool,2> bIsInPTZ{artifact.IsInPrecompressedTensileZone(topStressLocation),artifact.IsInPrecompressedTensileZone(botStressLocation)};
	
	            std::array<std::array<Float64,2>,2> fLimit;
	            fLimit[TOP][WITHOUT_REBAR] = artifact.GetCapacity(topStressLocation);
	            fLimit[BOT][WITHOUT_REBAR] = artifact.GetCapacity(botStressLocation);
	            fLimit[TOP][WITH_REBAR]    = pLimits->GetConcreteTensionStressLimit(poi,topStressLocation,task,true/*with rebar*/,bIsInPTZ[TOP]);
	            fLimit[BOT][WITH_REBAR]    = pLimits->GetConcreteTensionStressLimit(poi,botStressLocation,task,true/*with rebar*/,bIsInPTZ[BOT]);
	
	            Float64 fTopLimits = fLimit[TOP][WITHOUT_REBAR];
	            Float64 fBotAllowable = fLimit[BOT][WITHOUT_REBAR];
	
	            // Use calculator object to deal with allowable tensile stresses if there is adequate rebar
	            Float64 fsMax = (bSISpec ? WBFL::Units::ConvertToSysUnits(206.0,WBFL::Units::Measure::MPa) : WBFL::Units::ConvertToSysUnits(30.0,WBFL::Units::Measure::KSI) );
	
	            gbtAlternativeTensileStressRequirements altTensionRequirements;

               Float64 Es, fu; // rebar parameters that we aren't using but get anyway
	            if (i == 0)
	            {
	               if (bIsInClosure)
	               {
	                  altTensionRequirements.concreteType = (WBFL::Materials::ConcreteType)pMaterials->GetClosureJointConcreteType(closureKey);
	                  altTensionRequirements.bHasFct = pMaterials->DoesClosureJointConcreteHaveAggSplittingStrength(closureKey);
	                  altTensionRequirements.Fct = altTensionRequirements.bHasFct ? pMaterials->GetClosureJointConcreteAggSplittingStrength(closureKey) : 0.0;
	                  altTensionRequirements.fc = pMaterials->GetClosureJointFc(closureKey, task.intervalIdx);
	                  altTensionRequirements.density = pMaterials->GetClosureJointStrengthDensity(closureKey);
	                  pMaterials->GetClosureJointLongitudinalRebarProperties(closureKey, &Es, &altTensionRequirements.fy, &fu);
	               }
	               else
	               {
	                  altTensionRequirements.concreteType = (WBFL::Materials::ConcreteType)pMaterials->GetSegmentConcreteType(segmentKey);
	                  altTensionRequirements.bHasFct = pMaterials->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
	                  altTensionRequirements.Fct = altTensionRequirements.bHasFct ? pMaterials->GetSegmentConcreteAggSplittingStrength(segmentKey) : 0.0;
	                  altTensionRequirements.fc = pMaterials->GetSegmentFc(segmentKey, task.intervalIdx);
	                  altTensionRequirements.density = pMaterials->GetSegmentStrengthDensity(segmentKey);
	                  pMaterials->GetSegmentLongitudinalRebarProperties(segmentKey, &Es, &altTensionRequirements.fy, &fu);
	               }
	            }
	            else
	            {
	               altTensionRequirements.concreteType = (WBFL::Materials::ConcreteType)pMaterials->GetDeckConcreteType();
	               altTensionRequirements.bHasFct = pMaterials->DoesDeckConcreteHaveAggSplittingStrength();
	               altTensionRequirements.Fct = altTensionRequirements.bHasFct ? pMaterials->GetDeckConcreteAggSplittingStrength() : 0.0;
	               altTensionRequirements.fc = pMaterials->GetDeckFc(deckCastingRegionIdx,task.intervalIdx);
	               altTensionRequirements.density = pMaterials->GetDeckStrengthDensity();
	               pMaterials->GetDeckRebarProperties(&Es, &altTensionRequirements.fy, &fu);
	            }
	            altTensionRequirements.fsMax = fsMax;
	            altTensionRequirements.bLimitBarStress = true; // limit bar stress to fsMax
	
	
	            CSegmentKey thisSegmentKey = segmentKey;
	            if ( bIsInClosure )
	            {
	               thisSegmentKey = closureKey;
	            }
	
               if (pLimits->HasConcreteTensionStressLimitWithRebarOption(task.intervalIdx, bIsInPTZ[TOP], !bIsInClosure, thisSegmentKey))
	            {
	               if (i == 0 /*girder stresses*/ && bIsInClosure && bIsInPTZ[TOP])
	               {
	                  // the bar stress is not limited to 30 ksi [see LRFD Tables 5.9.2.3.1 a and b (pre2017: 5.9.4.1.2-1 and -2)]
	                  // in the precompressed tensile zone for closure joints
	                  altTensionRequirements.bLimitBarStress = false;
	               }

                  altTensionRequirements.MaxCoverToUseHigherTensionStressLimit = pLimits->GetMaxCoverToUseHigherTensionStressLimit();
	
                  CComPtr<IShape> shape;
                  pShapes->GetSegmentShape(task.intervalIdx, poi, false, pgsTypes::scCentroid, &shape);
                  altTensionRequirements.shape = shape;

                  CComPtr<IRebarSection> rebarSection;
	               pRebarGeom->GetRebars(poi, &rebarSection);
	               altTensionRequirements.rebarSection = rebarSection;
                  altTensionRequirements.bAdjustForDevelopmentLength = true;
                  if (pRebarGeom->IsAnchored(poi))
                  {
                     IntervalIndexType anchoringIntervalIdx = INVALID_INDEX;
                     if (bIsInClosure)
                     {
                        // poi is in a closure joint.... it can only anchor when it is composite
                        anchoringIntervalIdx = pIntervals->GetCompositeClosureJointInterval(closureKey);
                     }
                     else
                     {
                        if (poi <= midsegment_poi)
                        {
                           // poi is closer to the left end of the segment than the right end
                           if (segmentKey.segmentIndex == 0 || nSegments == 1)
                           {
                              // this is the first segment (or the only segment) so bars can only anchor into
                              // diaphragms. get the interval when the diaphragm has enough strength to anchor
                              anchoringIntervalIdx = pIntervals->GetCompositeIntermediateDiaphragmsInterval();
                           }
                           else
                           {
                              // the closure joint at the left end of the segment anchors the bars
                              CClosureKey prevClosureKey(segmentKey.groupIndex, segmentKey.girderIndex, segmentKey.segmentIndex - 1);
                              anchoringIntervalIdx = pIntervals->GetCompositeClosureJointInterval(prevClosureKey);
                           }
                        }
                        else
                        {
                           // poi is closer to the right end of the segment than the left end
                           if (segmentKey.segmentIndex == nSegments - 1 || nSegments == 1)
                           {
                              // this is the last segment (or the only segment) so bars can only anchor into
                              // diaphragms. get the interval when the diaphragm has enough strength to anchor
                              anchoringIntervalIdx = pIntervals->GetCompositeIntermediateDiaphragmsInterval();
                           }
                           else
                           {
                              // the closure joint at the right end of the segment anchors the bars
                              CClosureKey nextClosureKey(segmentKey);
                              anchoringIntervalIdx = pIntervals->GetCompositeClosureJointInterval(nextClosureKey);
                           }
                        }
                     }

                     if (anchoringIntervalIdx <= task.intervalIdx)
                     {
                        // the bars are anchored so they can be considered developed at all locations
                        altTensionRequirements.bAdjustForDevelopmentLength = false;
                     }
                  }

                  Float64 Ytg = pSectProps->GetY(task.intervalIdx, poi, pgsTypes::TopGirder);
                  altTensionRequirements.Ytg = Ytg;
	
	               Float64 Ca, Cbx, Cby;
	               IndexType controllingTopStressPointIdx;
	               pSectProps->GetStressCoefficients(task.intervalIdx, poi, pgsTypes::TopGirder, nullptr, &Ca, &Cbx, &Cby, &controllingTopStressPointIdx);
	               ATLASSERT(controllingTopStressPointIdx != INVALID_INDEX);
	               auto vTopStressPoints = pSectProps->GetStressPoints(task.intervalIdx, poi, pgsTypes::TopGirder);
	
	               IndexType controllingBottomStressPointIdx;
	               pSectProps->GetStressCoefficients(task.intervalIdx, poi, pgsTypes::BottomGirder, nullptr, &Ca, &Cbx, &Cby, &controllingBottomStressPointIdx);
	               ATLASSERT(controllingBottomStressPointIdx != INVALID_INDEX);
	               auto vBottomStressPoints = pSectProps->GetStressPoints(task.intervalIdx, poi, pgsTypes::BottomGirder);

	               bool bBiaxialStresses = (vTopStressPoints.size() == 1 && vBottomStressPoints.size() == 1 ? false : true);
	
	               if (vTopStressPoints.size() == 1)
	               {
	                  // one stress points means we have a symmetric section and the top center point is the stress point
	                  // make two stress points by spreading them apart in the X direction
	                  Float64 W = pGirder->GetTopWidth(poi);
	                  auto pntTop = vTopStressPoints.front();
	                  altTensionRequirements.pntTopLeft.Move(pntTop.X() - W/2, pntTop.Y(), fTop);
	                  altTensionRequirements.pntTopRight.Move(pntTop.X() + W/2, pntTop.Y(), fTop);
	               }
	               else
	               {
	                  ATLASSERT(2 <= vTopStressPoints.size());
	                  IndexType otherIdx = (controllingTopStressPointIdx == 0 ? 1 : 0); // index of a different stress point
	                  auto pntTop = vTopStressPoints[controllingTopStressPointIdx]; // location of controlling stress point (this is where fTop occurs)
                     auto pntTop2 = vTopStressPoints[otherIdx]; // location of a different stress point
	                  // stress at a point (x,y)
	                  // let D = (IxxIyy - Ixy^2)
	                  // f = [(MyIxx + MxIxy)x - (MxIyy + MyIxy)y]/D
	                  // My = 0 (we only have gravity and prestress forces), therefore
	                  // f = [(MxIxy)x - (MxIyy)y]/D
	                  // Solve for Mx
	                  // Mx = (D*f)/(Ixy*x - Iyy*y)
	                  // stress at other point (X,Y), f2 = [(MxIxy)X - (MxIyy)Y]/D
	                  // substitute for Mx
	                  // f2 = f(Ixy*X - Iyy*Y)/(Ixy*x - Iyy*y)
	                  Float64 Iyy = pSectProps->GetIyy(task.intervalIdx, poi);
	                  Float64 Ixy = pSectProps->GetIxy(task.intervalIdx, poi);
	                  Float64 fTop2 = fTop*(Ixy*pntTop2.X() - Iyy*pntTop2.Y()) / (Ixy*pntTop.X() - Iyy*pntTop.Y());
	                  altTensionRequirements.pntTopLeft.Move(pntTop.X(), pntTop.Y(), fTop);
	                  altTensionRequirements.pntTopRight.Move(pntTop2.X(), pntTop2.Y(), fTop2);
	               }
	
	               if (vBottomStressPoints.size() == 1)
	               {
	                  Float64 W = pGirder->GetBottomWidth(poi);
                     auto pntBottom = vBottomStressPoints.front();
	                  altTensionRequirements.pntBottomLeft.Move(pntBottom.X() - W/2, pntBottom.Y(), fBot);
	                  altTensionRequirements.pntBottomRight.Move(pntBottom.X() + W/2, pntBottom.Y(), fBot);
	               }
	               else
	               {
	                  ATLASSERT(2 <= vBottomStressPoints.size());
	                  IndexType otherIdx = (controllingTopStressPointIdx == 0 ? 1 : 0); // index of a different stress point
                     auto pntBot = vBottomStressPoints[controllingTopStressPointIdx]; // location of controlling stress point (this is where fTop occurs)
                     auto pntBot2 = vBottomStressPoints[otherIdx]; // location of a different stress point
	                  // stress at a point (x,y)
	                  // let D = (IxxIyy - Ixy^2)
	                  // f = [(MyIxx + MxIxy)x - (MxIyy + MyIxy)y]/D
	                  // My = 0 (we only have gravity and prestress forces), therefore
	                  // f = [(MxIxy)x - (MxIyy)y]/D
	                  // Solve for Mx
	                  // Mx = (D*f)/(Ixy*x - Iyy*y)
	                  // stress at other point (X,Y), f2 = [(MxIxy)X - (MxIyy)Y]/D
	                  // substitute for Mx
	                  // f2 = f(Ixy*X - Iyy*Y)/(Ixy*x - Iyy*y)
	                  Float64 Iyy = pSectProps->GetIyy(task.intervalIdx, poi);
	                  Float64 Ixy = pSectProps->GetIxy(task.intervalIdx, poi);
	                  Float64 fBot2 = fBot*(Ixy*pntBot2.X() - Iyy*pntBot2.Y()) / (Ixy*pntBot.X() - Iyy*pntBot.Y());
	                  altTensionRequirements.pntBottomLeft.Move(pntBot.X(), pntBot.Y(), fBot);
	                  altTensionRequirements.pntBottomRight.Move(pntBot2.X(), pntBot2.Y(), fBot2);
	               }
	
	               gbtComputeAlternativeStressRequirements(&altTensionRequirements);
	               IsAdequateRebar[TOP] = altTensionRequirements.bIsAdequateRebar;
	               artifact.SetAlternativeTensileStressRequirements(topStressLocation, altTensionRequirements, fLimit[TOP][WITH_REBAR], bBiaxialStresses);
	            }
	
	
	            if ( pLimits->HasConcreteTensionStressLimitWithRebarOption(task.intervalIdx,bIsInPTZ[BOT],!bIsInClosure,thisSegmentKey) )
	            {
	               if ( i == 0 /*girder stresses*/ && bIsInClosure && bIsInPTZ[BOT] )
	               {
	                  // the bar stress is not limited to 30 ksi [see LRFD Tables 5.9.2.3.1 a and b (pre2017: 5.9.4.1.2-1 and -2)]
	                  // in the precompressed tensile zone for closure joints
	                  altTensionRequirements.bLimitBarStress = false;
	               }
	
	
	
	               CComPtr<IShape> shape;
	               pShapes->GetSegmentShape(task.intervalIdx, poi, false, pgsTypes::scCentroid, &shape);
	               CComPtr<IRebarSection> rebarSection;
	               pRebarGeom->GetRebars(poi, &rebarSection);
	
	               altTensionRequirements.shape = shape;
	               altTensionRequirements.rebarSection = rebarSection;
	
	               Float64 Ca, Cbx, Cby;
	               IndexType controllingTopStressPointIdx;
	               pSectProps->GetStressCoefficients(task.intervalIdx, poi, pgsTypes::TopGirder, nullptr, &Ca, &Cbx, &Cby, &controllingTopStressPointIdx);
	               ATLASSERT(controllingTopStressPointIdx != INVALID_INDEX);
                  auto vTopStressPoints = pSectProps->GetStressPoints(task.intervalIdx, poi, pgsTypes::TopGirder);
	
	               IndexType controllingBottomStressPointIdx;
	               pSectProps->GetStressCoefficients(task.intervalIdx, poi, pgsTypes::BottomGirder, nullptr, &Ca, &Cbx, &Cby, &controllingBottomStressPointIdx);
	               ATLASSERT(controllingBottomStressPointIdx != INVALID_INDEX);
                  auto vBottomStressPoints = pSectProps->GetStressPoints(task.intervalIdx, poi, pgsTypes::BottomGirder);
	
	
	               bool bBiaxialStresses = (vTopStressPoints.size() == 1 && vBottomStressPoints.size() == 1 ? false : true);
	
	               if (vTopStressPoints.size() == 1)
	               {
	                  // one stress points means we have a symmetric section and the top center point is the stress point
	                  // make two stress points by spreading them apart in the X direction
	                  Float64 W = pGirder->GetTopWidth(poi);
                     auto pntTop = vTopStressPoints.front();
	                  altTensionRequirements.pntTopLeft.Move(pntTop.X() - W/2, pntTop.Y(), fTop);
	                  altTensionRequirements.pntTopRight.Move(pntTop.X() + W/2, pntTop.Y(), fTop);
	               }
	               else
	               {
	                  ATLASSERT(2 <= vTopStressPoints.size());
	                  IndexType otherIdx = (controllingTopStressPointIdx == 0 ? 1 : 0); // index of a different stress point
                     auto pntTop = vTopStressPoints[controllingTopStressPointIdx]; // location of controlling stress point (this is where fTop occurs)
                     auto pntTop2 = vTopStressPoints[otherIdx]; // location of a different stress point
	                                                                  // stress at a point (x,y)
	                                                                  // let D = (IxxIyy - Ixy^2)
	                                                                  // f = [(MyIxx + MxIxy)x - (MxIyy + MyIxy)y]/D
	                                                                  // My = 0 (we only have gravity and prestress forces), therefore
	                                                                  // f = [(MxIxy)x - (MxIyy)y]/D
	                                                                  // Solve for Mx
	                                                                  // Mx = (D*f)/(Ixy*x - Iyy*y)
	                                                                  // stress at other point (X,Y), f2 = [(MxIxy)X - (MxIyy)Y]/D
	                                                                  // substitute for Mx
	                                                                  // f2 = f(Ixy*X - Iyy*Y)/(Ixy*x - Iyy*y)
	                  Float64 Iyy = pSectProps->GetIyy(task.intervalIdx, poi);
	                  Float64 Ixy = pSectProps->GetIxy(task.intervalIdx, poi);
	                  Float64 fTop2 = fTop*(Ixy*pntTop2.X() - Iyy*pntTop2.Y()) / (Ixy*pntTop.X() - Iyy*pntTop.Y());
	                  altTensionRequirements.pntTopLeft.Move(pntTop.X(), pntTop.Y(), fTop);
	                  altTensionRequirements.pntTopRight.Move(pntTop2.X(), pntTop2.Y(), fTop2);
	               }
	
	               if (vBottomStressPoints.size() == 1)
	               {
	                  Float64 W = pGirder->GetBottomWidth(poi);
                     auto pntBottom = vBottomStressPoints.front();
	                  altTensionRequirements.pntBottomLeft.Move(pntBottom.X() - W/2, pntBottom.Y(), fBot);
	                  altTensionRequirements.pntBottomRight.Move(pntBottom.X() + W/2, pntBottom.Y(), fBot);
	               }
	               else
	               {
	                  ATLASSERT(2 <= vBottomStressPoints.size());
	                  IndexType otherIdx = (controllingTopStressPointIdx == 0 ? 1 : 0); // index of a different stress point
                     auto pntBot = vBottomStressPoints[controllingTopStressPointIdx]; // location of controlling stress point (this is where fTop occurs)
                     auto pntBot2 = vBottomStressPoints[otherIdx]; // location of a different stress point
	                                                                     // stress at a point (x,y)
	                                                                     // let D = (IxxIyy - Ixy^2)
	                                                                     // f = [(MyIxx + MxIxy)x - (MxIyy + MyIxy)y]/D
	                                                                     // My = 0 (we only have gravity and prestress forces), therefore
	                                                                     // f = [(MxIxy)x - (MxIyy)y]/D
	                                                                     // Solve for Mx
	                                                                     // Mx = (D*f)/(Ixy*x - Iyy*y)
	                                                                     // stress at other point (X,Y), f2 = [(MxIxy)X - (MxIyy)Y]/D
	                                                                     // substitute for Mx
	                                                                     // f2 = f(Ixy*X - Iyy*Y)/(Ixy*x - Iyy*y)
	                  Float64 Iyy = pSectProps->GetIyy(task.intervalIdx, poi);
	                  Float64 Ixy = pSectProps->GetIxy(task.intervalIdx, poi);
	                  Float64 fBot2 = fBot*(Ixy*pntBot2.X() - Iyy*pntBot2.Y()) / (Ixy*pntBot.X() - Iyy*pntBot.Y());
	                  altTensionRequirements.pntBottomLeft.Move(pntBot.X(), pntBot.Y(), fBot);
	                  altTensionRequirements.pntBottomRight.Move(pntBot2.X(), pntBot2.Y(), fBot2);
	               }
	
	               gbtComputeAlternativeStressRequirements(&altTensionRequirements);
	               IsAdequateRebar[BOT] = altTensionRequirements.bIsAdequateRebar;
	               artifact.SetAlternativeTensileStressRequirements(botStressLocation, altTensionRequirements, fLimit[BOT][WITH_REBAR], bBiaxialStresses);
	            }
	
	            artifact.SetCapacity(topStressLocation,fTopLimits);
	            artifact.SetCapacity(botStressLocation,fBotAllowable);
	
	            //
	            // Get the controlling stress
	            //
	
	            // parameters for tension with rebar
               std::array<TensionStressLimit, 2> tension_stress_limit;
	            tension_stress_limit[TOP] = pLimits->GetConcreteTensionStressLimitParameters(poi,topStressLocation,task,true/*with rebar*/, bIsInPTZ[TOP]);
               tension_stress_limit[BOT] = pLimits->GetConcreteTensionStressLimitParameters(poi,botStressLocation,task,true/*with rebar*/, bIsInPTZ[BOT]);
	
	            Float64 f;
	            IndexType face;
	            if ( bIsTopApplicable && bIsBotApplicable )
	            {
	               face = MaxIndex(fTop,fBot);
	               f = Max(fTop,fBot);
	            }
	            else if ( bIsTopApplicable && !bIsBotApplicable )
	            {
	               face = TOP;
	               f = fTop;
	            }
	            else if ( !bIsTopApplicable && bIsBotApplicable )
	            {
	               face = BOT;
	               f = fBot;
	            }
	            else
	            {
	               ATLASSERT(false); // why are neither applicable
	               // there are legit cases of this... need to deal with them
	               face = MaxIndex(fTop,fBot);
	               f = Max(fTop,fBot);
	            }
	
	            // Compute concrete strength required to satisfy stress limit when there is adequate reinforcement to use the secondary tension stress limit
               // This method is needed because ComputeConcreteStrength above only considers the first limit.
	            if (0.0 < f && IsAdequateRebar[face])
	            {
	               // stress is tensile and there is adequate reinforcement to use the 
	               // alternative limit... compute the required strength here... otherwise
	               // don't change anything because the "without rebar" case governs and it
	               // is done
	               Float64 fc_reqd;
	               ATLASSERT(tension_stress_limit[face].bHasMaxValue == false); // alternate stress doesn't use limiting value
	               fc_reqd = pow(f/(lambda*tension_stress_limit[face].Coefficient),2);
	               artifact.SetRequiredConcreteStrength(pgsTypes::Tension,face == TOP ? topStressLocation : botStressLocation,fc_reqd);
	            }
	         } // if is tension
	      } // next section
      } // if segment exists


      pSegmentArtifact->AddFlexuralStressArtifact(artifact);
   } // next poi
}

void pgsDesigner2::ComputeConcreteStrength(pgsFlexuralStressArtifact& artifact,pgsTypes::StressLocation stressLocation,const StressCheckTask& task) const
{
   bool bIsApplicable = artifact.IsApplicable(stressLocation);

   DLOG(_T("ComputeConcreteStrength :: stressLocation = ") << (int)stressLocation << _T(" task.stressType = ") << g_Type[task.stressType] << _T(" bIsApplicable = ") << bIsApplicable);

   if (bIsApplicable)
   {
      const auto& poi(artifact.GetPointOfInterest());
      bool bIsInPTZ = false;
      GET_IFACE2(GetBroker(),IConcreteStressLimits, pLimits);
      if (task.stressType == pgsTypes::Compression)
      {
         Float64 fLimit = pLimits->GetConcreteCompressionStressLimit(poi, stressLocation, task);
         DLOG(_T("   GetConcreteCompressionStressLimit = ") << WBFL::Units::ConvertFromSysUnits(fLimit,WBFL::Units::Measure::KSI) << _T(" ksi"));
         artifact.SetCapacity(stressLocation, fLimit);
      }
      else
      {
         bIsInPTZ = artifact.IsInPrecompressedTensileZone(stressLocation);
         Float64 fLimit = pLimits->GetConcreteTensionStressLimit(poi, stressLocation, task, false/*without rebar*/, bIsInPTZ); // this accounts for UHPC and returns the correct tension stress limit
         DLOG(_T("   GetConcreteTensionStressLimit = ") << WBFL::Units::ConvertFromSysUnits(fLimit,WBFL::Units::Measure::KSI) << _T(" ksi") << _T(" bIsInPTZ = ") << bIsInPTZ);
         artifact.SetCapacity(stressLocation, fLimit);
      }

      Float64 fc_reqd = pLimits->ComputeRequiredConcreteStrength(poi, stressLocation, artifact.GetDemand(stressLocation), task, false/*inadequate rebar*/, bIsInPTZ);
      artifact.SetRequiredConcreteStrength(task.stressType, stressLocation, fc_reqd);

      DLOG(_T("   fc_reqd = ") << WBFL::Units::ConvertFromSysUnits(fc_reqd,WBFL::Units::Measure::KSI) << _T(" ksi"));
   }
}

void pgsDesigner2::CheckSegmentStressesAtRelease(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig,pgsTypes::StressType type, pgsSegmentArtifact* pSegmentArtifact) const
{
   USES_CONVERSION;

   GET_IFACE2(GetBroker(),IPointOfInterest,         pPoi);
   GET_IFACE2(GetBroker(),IPretensionStresses,      pPretensionStresses);
   GET_IFACE2(GetBroker(),IProductForces,           pProductForces);
   GET_IFACE2(GetBroker(),ILimitStateForces,        pLimitStateForces);
   GET_IFACE2(GetBroker(),IConcreteStressLimits, pLimits );
   GET_IFACE2(GetBroker(),IGirder,                  pGirder);
   GET_IFACE2(GetBroker(),ISectionProperties,       pSectProps);
   GET_IFACE2(GetBroker(),IShapes,                  pShapes);
   GET_IFACE2(GetBroker(),IMaterials,               pMaterials);
   GET_IFACE2(GetBroker(),ILongRebarGeometry,       pRebarGeom);
   GET_IFACE2(GetBroker(),IIntervals,               pIntervals);
   GET_IFACE2(GetBroker(),IPrecompressedTensileZone, pPrecompressedTensileZone);

   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   // we only work in the casting yard
   StressCheckTask task;
   task.intervalIdx = releaseIntervalIdx;
   task.limitState  = pgsTypes::ServiceI;
   task.stressType  = type;

   pgsTypes::BridgeAnalysisType batTop, batBottom;
   GetBridgeAnalysisType(segmentKey.girderIndex,task,batTop,batBottom);

   bool bSISpec = WBFL::LRFD::BDSManager::GetUnits() == WBFL::LRFD::BDSManager::Units::SI ? true : false;

   gbtAlternativeTensileStressRequirements altTensionRequirements;
   altTensionRequirements.concreteType = (WBFL::Materials::ConcreteType)pMaterials->GetSegmentConcreteType(segmentKey);
   altTensionRequirements.density = pMaterials->GetSegmentStrengthDensity(segmentKey);
   altTensionRequirements.bHasFct = pMaterials->DoesSegmentConcreteHaveAggSplittingStrength(segmentKey);
   altTensionRequirements.Fct = altTensionRequirements.bHasFct ? pMaterials->GetSegmentConcreteAggSplittingStrength(segmentKey) : 0.0;
   Float64 fci;
   if ( pConfig == nullptr )
   {
      fci = pMaterials->GetSegmentDesignFc(segmentKey,releaseIntervalIdx);
   }
   else
   {
      fci = pConfig->fci;
   }
   altTensionRequirements.fc = fci;

   Float64 lambda = pMaterials->GetSegmentLambda(segmentKey);

   // Use calculator object to deal with casting yard higher allowable stress
   Float64 fsMax = (bSISpec ? WBFL::Units::ConvertToSysUnits(206.0,WBFL::Units::Measure::MPa) : WBFL::Units::ConvertToSysUnits(30.0,WBFL::Units::Measure::KSI) );
   
   altTensionRequirements.fsMax = fsMax;
   altTensionRequirements.bLimitBarStress = true; // limit bar stress to fsMax

   Float64 Es, fu;
   pMaterials->GetSegmentLongitudinalRebarProperties(segmentKey, &Es, &altTensionRequirements.fy, &fu);
   
   // Don't check closure joint POI at release
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_RELEASED_SEGMENT, &vPoi);
   pPoi->RemovePointsOfInterest(vPoi,POI_CLOSURE);


   for(const pgsPointOfInterest& poi : vPoi)
   {
      pgsFlexuralStressArtifact artifact(poi,task);

      std::array<bool,2> bIsInPTZ; // access with pgsTypes::StressLocation constant
      pPrecompressedTensileZone->IsInPrecompressedTensileZone(poi,task.limitState,pgsTypes::TopGirder, pgsTypes::BottomGirder, pConfig,&bIsInPTZ[pgsTypes::TopGirder],&bIsInPTZ[pgsTypes::BottomGirder]);
      artifact.IsInPrecompressedTensileZone(pgsTypes::TopGirder,   bIsInPTZ[pgsTypes::TopGirder]);
      artifact.IsInPrecompressedTensileZone(pgsTypes::BottomGirder,bIsInPTZ[pgsTypes::BottomGirder]);

      Float64 fLimitWithoutRebar(0.0), fLimitWithRebar(0.0);
      if (task.stressType == pgsTypes::Compression)
      {
         // always applicable in compression
         artifact.IsApplicable(pgsTypes::TopGirder,    true);
         artifact.IsApplicable(pgsTypes::BottomGirder, true);

         fLimitWithoutRebar  = pLimits->GetSegmentConcreteCompressionStressLimit(poi, task, fci);
         fLimitWithRebar = fLimitWithoutRebar;
      }
      else
      {
         // tension stress check only applicable in areas other that the precompressed tensile zone
         artifact.IsApplicable(pgsTypes::TopGirder,   !bIsInPTZ[pgsTypes::TopGirder]);
         artifact.IsApplicable(pgsTypes::BottomGirder,!bIsInPTZ[pgsTypes::BottomGirder]);

         fLimitWithoutRebar = pLimits->GetSegmentConcreteTensionStressLimit(poi, task, fci,false/*without rebar*/);
         fLimitWithRebar    = pLimits->GetSegmentConcreteTensionStressLimit(poi, task, fci,true/*with rebar*/);
      }

      // get segment stress due to prestressing
      auto [fTopPretension, fBotPretension] = pPretensionStresses->GetStress(task.intervalIdx, poi, pgsTypes::TopGirder, pgsTypes::BottomGirder, task.bIncludeLiveLoad, task.limitState, INVALID_INDEX, pConfig);

      // get segment stress due to post-tensioning
      Float64 fTopPosttension, fBotPosttension, fDummy;
      pProductForces->GetStress(task.intervalIdx,pgsTypes::pftPostTensioning,poi,batTop,   rtCumulative,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTopPosttension,&fDummy);
      pProductForces->GetStress(task.intervalIdx,pgsTypes::pftPostTensioning,poi,batBottom,rtCumulative,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fDummy,         &fBotPosttension);

      // get girder stress due to external loads (top)
      Float64 fTopLimitStateMin, fTopLimitStateMax;
      pLimitStateForces->GetStress(task.intervalIdx,task.limitState,poi,batTop,false,pgsTypes::TopGirder,&fTopLimitStateMin,&fTopLimitStateMax);
      Float64 fTopLimitState = (task.stressType == pgsTypes::Compression ? fTopLimitStateMin : fTopLimitStateMax );

      // get girder stress due to external loads (bottom)
      Float64 fBotLimitStateMin, fBotLimitStateMax;
      pLimitStateForces->GetStress(task.intervalIdx,task.limitState,poi,batBottom,false,pgsTypes::BottomGirder,&fBotLimitStateMin,&fBotLimitStateMax);
      Float64 fBotLimitState = (task.stressType == pgsTypes::Compression ? fBotLimitStateMin : fBotLimitStateMax );

      Float64 fTop = fTopLimitState + fTopPretension + fTopPosttension;
      Float64 fBot = fBotLimitState + fBotPretension + fBotPosttension;

      fTop = (IsZero(fTop) ? 0 : fTop);
      fBot = (IsZero(fBot) ? 0 : fBot);

      artifact.SetDemand(             pgsTypes::TopGirder,    fTop );
      artifact.SetDemand(             pgsTypes::BottomGirder, fBot );
      artifact.SetExternalEffects(    pgsTypes::TopGirder,    fTopLimitState);
      artifact.SetExternalEffects(    pgsTypes::BottomGirder, fBotLimitState);
      artifact.SetPretensionEffects(  pgsTypes::TopGirder,    fTopPretension);
      artifact.SetPretensionEffects(  pgsTypes::BottomGirder, fBotPretension);
      artifact.SetPosttensionEffects( pgsTypes::TopGirder,    fTopPosttension);
      artifact.SetPosttensionEffects( pgsTypes::BottomGirder, fBotPosttension);

      // Compute allowable stress and required concrete strengths
      Float64 fLimit(0.0);
      if(task.stressType == pgsTypes::Compression)
      {
         fLimit = fLimitWithoutRebar;

         // required strength
         Float64 fc_reqd_top = pLimits->ComputeRequiredConcreteStrength(poi,pgsTypes::TopGirder,fTop,task,false/*without rebar*/,bIsInPTZ[pgsTypes::TopGirder]);
         Float64 fc_reqd_bot = pLimits->ComputeRequiredConcreteStrength(poi, pgsTypes::BottomGirder, fBot, task, false/*without rebar*/, bIsInPTZ[pgsTypes::BottomGirder]);

         if (fc_reqd_top > fc_reqd_bot)
         {
            artifact.SetRequiredConcreteStrength(task.stressType,pgsTypes::TopGirder,fc_reqd_top);
         }
         else
         {
            artifact.SetRequiredConcreteStrength(task.stressType, pgsTypes::BottomGirder,fc_reqd_bot);
         }
      }
      else // tension
      {
         CComPtr<IShape> shape;
         pShapes->GetSegmentShape(task.intervalIdx, poi, false, pgsTypes::scCentroid, &shape);
         altTensionRequirements.shape = shape;

         CComPtr<IRebarSection> rebarSection;
         pRebarGeom->GetRebars(poi, &rebarSection);
         altTensionRequirements.rebarSection = rebarSection;

         altTensionRequirements.MaxCoverToUseHigherTensionStressLimit = pLimits->GetMaxCoverToUseHigherTensionStressLimit();

         altTensionRequirements.bAdjustForDevelopmentLength = true; // anchorage of rebar never helps development at release

         Float64 Ca, Cbx, Cby;
         IndexType controllingTopStressPointIdx;
         pSectProps->GetStressCoefficients(task.intervalIdx, poi, pgsTypes::TopGirder, nullptr, &Ca, &Cbx, &Cby, &controllingTopStressPointIdx);
         ATLASSERT(controllingTopStressPointIdx != INVALID_INDEX);
         auto vTopStressPoints = pSectProps->GetStressPoints(task.intervalIdx, poi, pgsTypes::TopGirder);

         IndexType controllingBottomStressPointIdx;
         pSectProps->GetStressCoefficients(task.intervalIdx, poi, pgsTypes::BottomGirder, nullptr, &Ca, &Cbx, &Cby, &controllingBottomStressPointIdx);
         ATLASSERT(controllingBottomStressPointIdx != INVALID_INDEX);
         auto vBottomStressPoints = pSectProps->GetStressPoints(task.intervalIdx, poi, pgsTypes::BottomGirder);

         bool bBiaxialStresses = (vTopStressPoints.size() == 1 && vBottomStressPoints.size() == 1 ? false : true);

         if (vTopStressPoints.size() == 1)
         {
            // one stress points means we have a symmetric section and the top center point is the stress point
            // make two stress points by spreading them apart in the X direction
            Float64 W = pGirder->GetTopWidth(poi);
            auto pntTop = vTopStressPoints.front();
            altTensionRequirements.pntTopLeft.Move(pntTop.X() - W/2, pntTop.Y(), fTop);
            altTensionRequirements.pntTopRight.Move(pntTop.X() + W/2, pntTop.Y(), fTop);
         }
         else
         {
            ATLASSERT(2 <= vTopStressPoints.size());
            IndexType otherIdx = (controllingTopStressPointIdx == 0 ? 1 : 0); // index of a different stress point
            auto pntTop = vTopStressPoints[controllingTopStressPointIdx]; // location of controlling stress point (this is where fTop occurs)
            auto pntTop2 = vTopStressPoints[otherIdx]; // location of a different stress point
                                                            // stress at a point (x,y)
                                                            // let D = (IxxIyy - Ixy^2)
                                                            // f = [(MyIxx + MxIxy)x - (MxIyy + MyIxy)y]/D
                                                            // My = 0 (we only have gravity and prestress forces), therefore
                                                            // f = [(MxIxy)x - (MxIyy)y]/D
                                                            // Solve for Mx
                                                            // Mx = (D*f)/(Ixy*x - Iyy*y)
                                                            // stress at other point (X,Y), f2 = [(MxIxy)X - (MxIyy)Y]/D
                                                            // substitute for Mx
                                                            // f2 = f(Ixy*X - Iyy*Y)/(Ixy*x - Iyy*y)
            Float64 Iyy = pSectProps->GetIyy(task.intervalIdx, poi);
            Float64 Ixy = pSectProps->GetIxy(task.intervalIdx, poi);
            Float64 fTop2 = fTop*(Ixy*pntTop2.X() - Iyy*pntTop2.Y()) / (Ixy*pntTop.X() - Iyy*pntTop.Y());
            altTensionRequirements.pntTopLeft.Move(pntTop.X(), pntTop.Y(), fTop);
            altTensionRequirements.pntTopRight.Move(pntTop2.X(), pntTop2.Y(), fTop2);
         }

         if (vBottomStressPoints.size() == 1)
         {
            Float64 W = pGirder->GetBottomWidth(poi);
            auto pntBottom = vBottomStressPoints.front();
            altTensionRequirements.pntBottomLeft.Move(pntBottom.X() - W/2, pntBottom.Y(), fBot);
            altTensionRequirements.pntBottomRight.Move(pntBottom.X() + W/2, pntBottom.Y(), fBot);
         }
         else
         {
            ATLASSERT(2 <= vBottomStressPoints.size());
            IndexType otherIdx = (controllingTopStressPointIdx == 0 ? 1 : 0); // index of a different stress point
            auto pntBot = vBottomStressPoints[controllingTopStressPointIdx]; // location of controlling stress point (this is where fTop occurs)
            auto pntBot2 = vBottomStressPoints[otherIdx]; // location of a different stress point
                                                               // stress at a point (x,y)
                                                               // let D = (IxxIyy - Ixy^2)
                                                               // f = [(MyIxx + MxIxy)x - (MxIyy + MyIxy)y]/D
                                                               // My = 0 (we only have gravity and prestress forces), therefore
                                                               // f = [(MxIxy)x - (MxIyy)y]/D
                                                               // Solve for Mx
                                                               // Mx = (D*f)/(Ixy*x - Iyy*y)
                                                               // stress at other point (X,Y), f2 = [(MxIxy)X - (MxIyy)Y]/D
                                                               // substitute for Mx
                                                               // f2 = f(Ixy*X - Iyy*Y)/(Ixy*x - Iyy*y)
            Float64 Iyy = pSectProps->GetIyy(task.intervalIdx, poi);
            Float64 Ixy = pSectProps->GetIxy(task.intervalIdx, poi);
            Float64 fBot2 = fBot*(Ixy*pntBot2.X() - Iyy*pntBot2.Y()) / (Ixy*pntBot.X() - Iyy*pntBot.Y());
            altTensionRequirements.pntBottomLeft.Move(pntBot.X(), pntBot.Y(), fBot);
            altTensionRequirements.pntBottomRight.Move(pntBot2.X(), pntBot2.Y(), fBot2);
         }

         altTensionRequirements.Ytg = pSectProps->GetY(task.intervalIdx, poi, pgsTypes::TopGirder);
         gbtComputeAlternativeStressRequirements(&altTensionRequirements);
         artifact.SetAlternativeTensileStressRequirements(pgsTypes::BottomGirder, altTensionRequirements, fLimitWithRebar, bBiaxialStresses);

         if (altTensionRequirements.AsRequired <= altTensionRequirements.AsProvided)
         {
            // if AsRequired < 0 (eg, -1), the entire section is in compression
            fLimit = (altTensionRequirements.AsRequired < 0 ? fLimitWithoutRebar : fLimitWithRebar);
         }
         else
         {
            fLimit = fLimitWithoutRebar;
         }

         // Compute required concrete strength
         // 
         Float64 fc_reqd_top = pLimits->ComputeRequiredConcreteStrength(poi,pgsTypes::TopGirder,fTop,task,altTensionRequirements.bIsAdequateRebar,bIsInPTZ[pgsTypes::TopGirder]);
         Float64 fc_reqd_bot = pLimits->ComputeRequiredConcreteStrength(poi, pgsTypes::BottomGirder, fTop, task, altTensionRequirements.bIsAdequateRebar, bIsInPTZ[pgsTypes::BottomGirder]);

         if (fc_reqd_top > fc_reqd_bot)
         {
            artifact.SetRequiredConcreteStrength(task.stressType,pgsTypes::TopGirder,fc_reqd_top);
         }
         else
         {
            artifact.SetRequiredConcreteStrength(task.stressType,pgsTypes::BottomGirder,fc_reqd_bot);
         }
      }

      artifact.SetCapacity(pgsTypes::TopGirder,   fLimit);
      artifact.SetCapacity(pgsTypes::BottomGirder,fLimit);

      // Stow our artifact
      pSegmentArtifact->AddFlexuralStressArtifact(artifact);
   } // next poi
}

void pgsDesigner2::CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const GDRCONFIG& config,bool bPositiveMoment,pgsFlexuralCapacityArtifact* pArtifact) const
{
   GET_IFACE2(GetBroker(),IMomentCapacity, pMomentCapacity);

   const MOMENTCAPACITYDETAILS* pmcd = pMomentCapacity->GetMomentCapacityDetails( intervalIdx, poi, bPositiveMoment, &config );

   MINMOMENTCAPDETAILS mmcd;
   pMomentCapacity->GetMinMomentCapacityDetails(intervalIdx, poi, config, bPositiveMoment, &mmcd);

   CreateFlexuralCapacityArtifact(poi,intervalIdx,limitState,bPositiveMoment,pmcd,&mmcd,true/*designing*/,pArtifact);
}

void pgsDesigner2::CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,bool bPositiveMoment,pgsFlexuralCapacityArtifact* pArtifact) const
{
   GET_IFACE2(GetBroker(),IMomentCapacity, pMomentCapacity);

   const MOMENTCAPACITYDETAILS* pmcd = pMomentCapacity->GetMomentCapacityDetails( intervalIdx, poi, bPositiveMoment );

   const MINMOMENTCAPDETAILS* pmmcd = pMomentCapacity->GetMinMomentCapacityDetails(intervalIdx, poi, bPositiveMoment);

   CreateFlexuralCapacityArtifact(poi,intervalIdx,limitState,bPositiveMoment,pmcd,pmmcd,false/*checking*/,pArtifact);
}

void pgsDesigner2::CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,bool bPositiveMoment,const MOMENTCAPACITYDETAILS* pmcd,const MINMOMENTCAPDETAILS* pmmcd,bool bDesign,pgsFlexuralCapacityArtifact* pArtifact) const
{
   GET_IFACE2(GetBroker(),ILimitStateForces, pLimitStateForces);
   GET_IFACE2(GetBroker(),ILibrary,pLib);
   GET_IFACE2(GetBroker(),ISpecification, pSpec);

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool c_over_de = ( pSpec->GetMomentCapacityMethod() == LRFD_METHOD && pSpecEntry->GetSpecificationCriteria().GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2006Interims );
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   pArtifact->SetPointOfInterest(poi);

   Float64 Mu;
   if ( bPositiveMoment )
   {
      Float64 MuMin, MuMax;
      if ( analysisType == pgsTypes::Envelope )
      {
         Float64 min,max;
         pLimitStateForces->GetMoment(intervalIdx,limitState,poi,pgsTypes::MaxSimpleContinuousEnvelope,&min,&max);
         MuMax = max;

         pLimitStateForces->GetMoment(intervalIdx,limitState,poi,pgsTypes::MinSimpleContinuousEnvelope,&min,&max);
         MuMin = min;
      }
      else
      {
         pLimitStateForces->GetMoment(intervalIdx,limitState,poi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,&MuMin,&MuMax);
      }

      Mu = MuMax;

      if ( bDesign && m_StrandDesignTool->IsDesignSlabOffset())
      {
         // Mu is based on the current input values. Since we are doing design, the "A" dimension
         // is likely different than the input value. This changes the slab and slab pad moment.
         // Add the moment adjustments to Mu here.
         Float64 fcgdr = m_StrandDesignTool->GetConcreteStrength();

         const GDRCONFIG& config = m_StrandDesignTool->GetSegmentConfiguration();

         GET_IFACE2(GetBroker(),IProductForces,pProductForces);
         Float64 dMslab     = pProductForces->GetDesignSlabMomentAdjustment(poi,&config);
         Float64 dMslab_pad = pProductForces->GetDesignSlabPadMomentAdjustment(poi,&config);

         GET_IFACE2(GetBroker(),ILoadFactors,pLF);
         const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();
         Float64 k = pLoadFactors->GetDCMax(limitState);
         
         Mu += k*(dMslab + dMslab_pad);
      }
   }
   else
   {
      if ( analysisType == pgsTypes::Envelope )
      {
         Mu = pLimitStateForces->GetSlabDesignMoment(limitState,poi,pgsTypes::MinSimpleContinuousEnvelope);
      }
      else
      {
         Mu = pLimitStateForces->GetSlabDesignMoment(limitState,poi,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
      }
   }

   pArtifact->SetCapacity( pmcd->Phi * pmcd->Mn );
   pArtifact->SetDemand( Mu );
   pArtifact->SetMinCapacity( pmmcd->MrMin );

   // When capacity is zero, there is no reinforcing ratio.
   // We need to simulate some numbers so everything works.
   // Also simulate numbers if this is 2006 LRFD or later... c/de has been removed from the LRFD spec
   Float64 c_de;
   if ( c_over_de && !IsZero(pmcd->de) )
   {
      c_de = pmcd->c/pmcd->de;
   }
   else
   {
      c_de = 0.0;
   }

   pArtifact->SetMaxReinforcementRatio( c_de );
   pArtifact->SetMaxReinforcementRatioLimit(0.42);  // 5.7.3.3.1 (removed from spec 2005)
}

void pgsDesigner2::CreateStirrupCheckAtPoisArtifact(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState, Float64 vu,
                                                    Float64 fcSlab,Float64 fcGdr, Float64 fy, bool checkConfinement,const GDRCONFIG* pConfig,
                                                    pgsStirrupCheckAtPoisArtifact* pArtifact) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

#if defined _DEBUG
   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   ATLASSERT(liveLoadIntervalIdx <= intervalIdx);
   ATLASSERT(limitState==pgsTypes::StrengthI || limitState == pgsTypes::StrengthII);
#endif

   GET_IFACE2(GetBroker(),IBridge,pBridge);

   // throw an exception if span length is too short
   if (IsDeepSection( poi ))
   {
      GET_IFACE2(GetBroker(),IPointOfInterest,pPoi);
      CSpanKey spanKey;
      Float64 Xspan;
      pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

      Int32 reason = XREASON_AGENTVALIDATIONFAILURE;
      std::_tostringstream os;
      os << _T("Cannot perform shear check. The Span-to-Depth ratio is less than ")<< MIN_SPAN_DEPTH_RATIO <<_T(" for Span ")
         << LABEL_SPAN(spanKey.spanIndex) << _T(" Girder ")<< LABEL_GIRDER(spanKey.girderIndex)
         << _T(" (See LRFD ") << WBFL::LRFD::LrfdCw8th(_T("5.8.1.1"),_T("5.7.1.1"))<<_T(")");

      GET_IFACE2(GetBroker(),IEAFStatusCenter,pStatusCenter);
      pStatusCenter->Add(std::make_shared<pgsBridgeDescriptionStatusItem>(m_StatusGroupID, m_scidBridgeDescriptionError, pgsBridgeDescriptionStatusItem::General, os.str().c_str()));

      os << std::endl << _T("See Status Center for Details");
      THROW_UNWIND(os.str().c_str(),reason);
   }

   GET_IFACE2(GetBroker(),IShearCapacity, pShearCapacity);

   SHEARCAPACITYDETAILS scd;
   pShearCapacity->GetShearCapacityDetails( limitState, intervalIdx, poi, pConfig, &scd );

   // vertical shear
   pgsVerticalShearArtifact v_artifact;
   CheckStirrupRequirement( poi, scd, &v_artifact );
   CheckUltimateShearCapacity( limitState, intervalIdx, poi, scd, vu, pConfig, &v_artifact );

   // horizontal shear
   pgsHorizontalShearArtifact h_artifact;
   h_artifact.SetApplicability(false);
   if ( pBridge->IsCompositeDeck() )
   {
      h_artifact.SetApplicability(true);
      CheckHorizontalShear(limitState, poi,vu,fcSlab,fcGdr,fy, pConfig,&h_artifact);
   }

   // stirrup detail check
   const STIRRUPCONFIG* pStirrupConfig = (pConfig==nullptr) ? nullptr : &(pConfig->StirrupConfig);

   pgsStirrupDetailArtifact d_artifact;
   CheckFullStirrupDetailing(poi,v_artifact,scd,vu,fcGdr,fy,pStirrupConfig,&d_artifact);

   // longitudinal steel check
   pgsLongReinfShearArtifact l_artifact;
   CheckLongReinfShear(poi,intervalIdx,limitState,scd,pConfig,&l_artifact);

   // populate the artifact
   pArtifact->SetPointOfInterest(poi);
   pArtifact->SetVerticalShearArtifact(v_artifact);
   pArtifact->SetHorizontalShearArtifact(h_artifact);
   pArtifact->SetStirrupDetailArtifact(d_artifact);
   pArtifact->SetLongReinfShearArtifact(l_artifact);
}

bool pgsDesigner2::IsDeepSection( const pgsPointOfInterest& poi) const
{
   // LRFD 5.7.1.1 (pre2017: 5.8.1.1)
   // Assume that the point of zero shear is at mid-span (L/2).
   // This assumption is true for uniform load which generally occur for this type of structure.
   // From 5.7.1.1, the beam is considered a deep beam if the distance from the point of zero shear
   // to the face of support is less than 2d.
   //
   // L/2 < 2d 
   // Re-arrange
   // L/d < 4

   GET_IFACE2(GetBroker(),IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

   GET_IFACE2(GetBroker(),IBridge,pBridge);
   Float64 span_length = pBridge->GetSpanLength(spanKey);

   GET_IFACE2(GetBroker(),IGirder,pGdr);
   Float64 beam_depth = pGdr->GetHeight(poi);

   Float64 ratio = span_length/beam_depth;
   return ( ratio < Float64(MIN_SPAN_DEPTH_RATIO));
}

ZoneIndexType pgsDesigner2::GetCriticalSectionZone(const pgsPointOfInterest& poi,bool bIncludeCS) const
{
   Float64 Xpoi = poi.GetDistFromStart();

   auto iter(m_CriticalSections.cbegin());
   auto end(m_CriticalSections.cend());
   for ( ; iter != end; iter++ )
   {
      const CRITSECTDETAILS& csDetails(iter->first);
      const pgsPointOfInterest& csPoi = csDetails.GetPointOfInterest();
      const CSegmentKey& csSegmentKey = csPoi.GetSegmentKey();

      if ( csSegmentKey == poi.GetSegmentKey() && ::InRange(csDetails.Start,Xpoi,csDetails.End) )
      {
         // poi is in the critical section zone
         if ( !bIncludeCS && csPoi.AtSamePlace(poi) )
         {
            // we want to exclude the actual critical section and the poi is at the same place as the critical section
            // return now with INVALID_INDEX since there is no reason to keep going through the loop
            return INVALID_INDEX;
         }

         // we found the critical section zone that contains our poi
         return (ZoneIndexType)(iter - m_CriticalSections.begin());
      }
   }

   return INVALID_INDEX;
}

ZoneIndexType pgsDesigner2::GetSupportZoneIndex(const pgsPointOfInterest& poi) const
{
   // Determines if a POI is in a support zone
   // Support zones are between end of girder and FOS at end of girder and
   // between CL Brg and FOS at intermediate supports.

   // In previous versions of PGSuper, the Face of Support was considered to be
   // outside of the support zone.
   if (poi.HasAttribute(POI_FACEOFSUPPORT) )
   {
      return INVALID_INDEX; // face of support is not considered to be in the support zone
   }

   Float64 x = poi.GetDistFromStart();

   auto iter(m_SupportZones.cbegin());
   auto end(m_SupportZones.cend());
   for ( ; iter != end; iter++ )
   {
      const SUPPORTZONE& supportZone = *iter;
      if ( ::InRange(supportZone.Start,x,supportZone.End) )
      {
         return (ZoneIndexType)std::distance(m_SupportZones.cbegin(),iter);
      }
   }

   return INVALID_INDEX;
}

void pgsDesigner2::CheckStirrupRequirement( const pgsPointOfInterest& poi, const SHEARCAPACITYDETAILS& scd, pgsVerticalShearArtifact* pArtifact ) const
{
   pArtifact->SetAreStirrupsReqd(scd.bStirrupsReqd);
   pArtifact->SetAreStirrupsProvided(0.0 < scd.Av);
}

void pgsDesigner2::CheckUltimateShearCapacity( pgsTypes::LimitState limitState,IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const SHEARCAPACITYDETAILS& scd, Float64 vu, const GDRCONFIG* pConfig, pgsVerticalShearArtifact* pArtifact ) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   Float64 poi_loc = poi.GetDistFromStart();

   ZoneIndexType csZoneIdx = GetCriticalSectionZone(poi);

   if ( csZoneIdx == INVALID_INDEX)
   {
      // poi is not in a critical section zone.. strength check is applicable
      pArtifact->IsApplicable(true);
      pArtifact->SetCapacity( scd.pVn );
      pArtifact->SetDemand( scd.Vu );

      // Make strut and tie check at CS
      if ( poi.HasAttribute(POI_CRITSECTSHEAR1) || poi.HasAttribute(POI_CRITSECTSHEAR2) )
      {
         ZoneIndexType csZoneIdx2 = GetCriticalSectionZone(poi,true);
         ATLASSERT(csZoneIdx2 != INVALID_INDEX);
         bool bStrutAndTieRequired = m_CriticalSections[csZoneIdx2].second;
         pArtifact->IsStrutAndTieRequired(bStrutAndTieRequired);
         pArtifact->IsApplicable(true);
      }
   }
   else
   {
      ATLASSERT(csZoneIdx != INVALID_INDEX); // we are in a CS zone so we better have a zone index

      // strength check is not applicable for this poi
      pArtifact->IsApplicable(false);

      const pgsPointOfInterest& csPoi(m_CriticalSections[csZoneIdx].first.GetPointOfInterest());

      // the shear reinforcement must be at least as much as the required reinforcement at section critical section
      // See LRFD C5.7.3.2 (pre2017: 5.8.3.2) (since the stress in the stirrups doesn't change between
      // the support and the critical section, there should be at least as much 
      // reinforcement between the end and the CS as there is at the CS)
      Float64 AvS_provided = (0.0 < scd.S ? scd.Av/scd.S : 0.0);
      Float64 AvS_required_at_CS;

      GET_IFACE2(GetBroker(),IShearCapacity, pShearCapacity);
      SHEARCAPACITYDETAILS shearCapacityDetailsAtCS;
      pShearCapacity->GetRawShearCapacityDetails(limitState, intervalIdx, csPoi, pConfig, &shearCapacityDetailsAtCS);
      AvS_required_at_CS = shearCapacityDetailsAtCS.AvOverS_Reqd;

      pArtifact->SetEndSpacing(AvS_provided,AvS_required_at_CS);
   }

   pArtifact->SetAvOverSReqd( scd.AvOverS_Reqd ); // leave a nugget for shear design algorithm
}

void pgsDesigner2::CheckHorizontalShear(pgsTypes::LimitState limitState, const pgsPointOfInterest& poi,
                                       Float64 vu, 
                                       Float64 fcSlab,Float64 fcGdr, Float64 fy,
                                       const GDRCONFIG* pConfig,
                                       pgsHorizontalShearArtifact* pArtifact ) const
{
   // NOTE: At one time (before BridgeLink:PGSuper version 5.0) horizontal interface shear was only check in regions
   // outside of the critical sections for shear. This was not correct. Horizontal interface shear checks are applicable
   // at all sections. 
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE2(GetBroker(),IGirder,pGdr);
   GET_IFACE2(GetBroker(),IMaterials,pMaterial);

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount() - 1;

   // determine shear demand
   GET_IFACE2(GetBroker(),IInterfaceShearRequirements,pInterfaceShear);

   Float64 Vuh;

   if ( pInterfaceShear->GetShearFlowMethod() == pgsTypes::sfmClassical )
   {
      GET_IFACE2(GetBroker(),ISectionProperties,pSectProps);

      auto Qslab = pSectProps->GetQSlab(intervalIdx, poi, pConfig);
      auto Ic = pSectProps->GetIxx(intervalIdx, poi, pConfig);
      ATLASSERT(0 < Qslab);

      Vuh  = vu*Qslab/Ic;

      pArtifact->SetI( Ic );
      pArtifact->SetQ( Qslab );
   }
   else
   {
      // dv is the distance between the centroid of the compression force, taken to be at the mid-height of the deck and the centroid of the tension steel.
      // since the steel on the tension side varies because of harped strand position, estimate by considering straight strands only
      GET_IFACE2(GetBroker(),IMomentCapacity,pMomentCap);
      const MOMENTCAPACITYDETAILS* pmcd = pMomentCap->GetMomentCapacityDetails(intervalIdx, poi, true/*positive moment*/, pConfig);

  
      GET_IFACE2(GetBroker(),IBridge,pBridge);
      Float64 tSlab = pBridge->GetStructuralSlabDepth(poi);

      Float64 dv = pmcd->de_shear - tSlab/2;

      Vuh  = vu / dv;

      pArtifact->SetDv( dv );
   }

   pArtifact->SetVu( vu );
   pArtifact->SetDemand(Vuh);

   // normal force on top of girder flange
   Float64 Pc = GetNormalFrictionForce(poi);
   pArtifact->SetNormalCompressionForce(Pc);

   Float64 gamma_dc;
   GET_IFACE2(GetBroker(),ILoadFactors, pILoadFactors);
   const auto* pLoadFactors = pILoadFactors->GetLoadFactors();
   if (IsRatingLimitState(limitState))
   {
      // MBE Table 6A.4.2.2-1 lists gamma_dc as 1.25 while the LRFD BDS
      // give a min and max value (1.25 and 0.90). The load rating
      // load factors are based on the MBE and have only a single value.
      // We don't want to artificially amplify the clamping force so
      // use the min gamma_dc based on the design strength limit state
      pgsTypes::LimitState ls = (IsStrengthILimitState(limitState) ? pgsTypes::StrengthI : pgsTypes::StrengthII);
      gamma_dc = pLoadFactors->GetDCMin(ls);
   }
   else
   {
      gamma_dc = pLoadFactors->GetDCMin(limitState);
   }
   pArtifact->SetNormalCompressionForceLoadFactor(gamma_dc);


   // Interface shear width (bvi = Acv per unit length)
   InterfaceShearWidthDetails bvi_details = pGdr->GetInterfaceShearWidthDetails(poi);
   pArtifact->SetInterfaceShearWidthDetails(bvi_details);

   // Take minimum concrete strength at interface
   Float64 fc = Min(fcSlab,fcGdr);
   pArtifact->SetFc( fc );

   // area of reinforcement crossing shear plane
   // girder stirrups
   bool is_roughened;
   bool do_all_stirrups_engage_deck;

   ComputeHorizAvs(poi, &is_roughened, &do_all_stirrups_engage_deck, pConfig,  pArtifact);

   pArtifact->SetIsTopFlangeRoughened(is_roughened);
   pArtifact->SetDoAllPrimaryStirrupsEngageDeck(do_all_stirrups_engage_deck);

   // friction and cohesion factors
   pgsTypes::ConcreteType girderConcType = pMaterial->GetSegmentConcreteType(segmentKey);
   pgsTypes::ConcreteType slabConcType = pMaterial->GetDeckConcreteType();
   Float64 c, u, K1, K2;
   WBFL::LRFD::ConcreteUtil::InterfaceShearParameters(is_roughened, (WBFL::Materials::ConcreteType)girderConcType, (WBFL::Materials::ConcreteType)slabConcType, &c, &u, &K1, &K2);

   pArtifact->SetCohesionFactor(c);
   pArtifact->SetFrictionFactor(u);
   pArtifact->SetK1(K1);
   pArtifact->SetK2(K2);

   // nominal shear capacities 5.7.4.1-2,3 (pre2017: 5.8.4.1)
   if ( WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= WBFL::LRFD::BDSManager::GetEdition() && gs_60KSI < fy)
   {
      // 60 ksi limit was added in 4th Edition 2007
      fy = gs_60KSI;
      pArtifact->WasFyLimited(true);
   }

   pArtifact->SetFy(fy);

   Float64 Vn1, Vn2, Vn3;
   WBFL::LRFD::ConcreteUtil::InterfaceShearResistances(c, u, K1, K2, bvi_details.bvi, pArtifact->GetAvOverS(), gamma_dc*Pc, fc, fy, &Vn1, &Vn2, &Vn3);
   pArtifact->SetVn(Vn1, Vn2, Vn3);

   GET_IFACE2(GetBroker(),IResistanceFactors,pResistanceFactors);
   GET_IFACE2(GetBroker(),IPointOfInterest,pPoi);
   Float64 phiGirder;
   CClosureKey closureKey;
   if ( pPoi->IsInClosureJoint(poi,&closureKey) )
   {
      pgsTypes::ConcreteType cjConcType = pMaterial->GetClosureJointConcreteType(closureKey);
      phiGirder = pResistanceFactors->GetClosureJointShearResistanceFactor(cjConcType);
   }
   else
   {
      phiGirder = pResistanceFactors->GetShearResistanceFactor(poi, girderConcType);
   }

   Float64 phiSlab   = pResistanceFactors->GetShearResistanceFactor(false, slabConcType);
   Float64 phi       = Min(phiGirder,phiSlab); // use minimum [see LRFD 5.7.4.1 (pre2017: 5.8.4.1)]
   pArtifact->SetPhi(phi);

   // Minimum steel check 5.7.4.1-4
   // This sucker has changed for every spec so far.


   Float64 sMax = pInterfaceShear->GetMaxShearConnectorSpacing(poi);
   pArtifact->SetSmax(sMax);

   WBFL::LRFD::ConcreteUtil::HsAvfOverSMinType avfmin = WBFL::LRFD::ConcreteUtil::AvfOverSMin(bvi_details.bvi,fy,Vuh,phi,c,u,Pc);
   pArtifact->SetAvOverSMin_5_7_4_2_1(avfmin.res5_7_4_2_1);
   pArtifact->SetAvOverSMin_5_7_4_1_3(avfmin.res5_7_4_2_3);
   pArtifact->SetAvOverSMin(avfmin.AvfOverSMin);

   Uint16 min_num_legs = WBFL::LRFD::ConcreteUtil::MinLegsForBv(bvi_details.bvi);
   pArtifact->SetNumLegsReqd(min_num_legs);

   // Determine average shear stress.
   // Average shear stress. Note: This value is vni prior to 2007 and vui afterwards 
   Float64 Vsavg;
   if ( WBFL::LRFD::BDSManager::Edition::FourthEdition2007 <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      Float64 vui = IsZero(bvi_details.bvi) ? 0.0 : Vuh/ bvi_details.bvi;
      Vsavg = vui;
   }
   else
   {
      Float64 Vnmin = Min(Vn1, Vn2, Vn3);
      Vsavg = IsZero(bvi_details.bvi) ? 0.0 : Vnmin/ bvi_details.bvi;
   }

   pArtifact->SetVsAvg(Vsavg);

   // Shear strength so that equation 5.7.4.3-4 (pre2017: 5.8.4.1-4) is not applicable
   Float64 vs_limit = WBFL::LRFD::ConcreteUtil::LowerLimitOfShearStrength(is_roughened,do_all_stirrups_engage_deck);
   pArtifact->SetVsLimit(vs_limit);

   // Get Av/S required for design algorithm
   Float64 avs_reqd = WBFL::LRFD::ConcreteUtil::AvfRequiredForHoriz(Vuh, phi, avfmin.AvfOverSMin, c, u, K1, K2,
      bvi_details.bvi, bvi_details.bvi, pArtifact->GetAvOverS(), Pc, fc, fy);
   pArtifact->SetAvOverSReqd(avs_reqd);
}

void pgsDesigner2::ComputeHorizAvs(const pgsPointOfInterest& poi,bool* pIsRoughened, bool* pDoAllStirrupsEngageDeck, 
                                   const GDRCONFIG* pConfig, pgsHorizontalShearArtifact* pArtifact) const
{
   if (pConfig == nullptr)
   {
      // Use current girder model data
      GET_IFACE2(GetBroker(),IBridge,pBridge);
      GET_IFACE2(GetBroker(),IStirrupGeometry, pStirrupGeometry);
      *pIsRoughened = pBridge->AreGirderTopFlangesRoughened(poi.GetSegmentKey());
      *pDoAllStirrupsEngageDeck = pStirrupGeometry->DoAllPrimaryStirrupsEngageDeck(poi.GetSegmentKey());

      Float64 Sg;
      WBFL::Materials::Rebar::Size size;
      Float64 abar, nl;
      Float64 Avs = pStirrupGeometry->GetPrimaryHorizInterfaceAvs(poi, &size, &abar, &nl, &Sg);

      pArtifact->SetAvfGirder(abar*nl);
      pArtifact->SetSGirder(Sg);

      // additional interface shear stirrups
      Float64 Avftf = pStirrupGeometry->GetAdditionalHorizInterfaceAvs(poi, &size, &abar, &nl, &Sg);

      pArtifact->SetAvfAdditional(abar*nl);
      pArtifact->SetSAdditional(Sg);

      // legs per stirrup
      Float64 num_legs = pStirrupGeometry->GetPrimaryHorizInterfaceBarCount(poi);

      num_legs += pStirrupGeometry->GetAdditionalHorizInterfaceBarCount(poi);

      pArtifact->SetNumLegs(num_legs);
   }
   else
   {
      // Use design config
      const CSegmentKey& segmentKey(poi.GetSegmentKey());

      *pIsRoughened = pConfig->StirrupConfig.bIsRoughenedSurface;
      *pDoAllStirrupsEngageDeck = DoAllStirrupsEngageDeck(pConfig->StirrupConfig);

      GET_IFACE2(GetBroker(),IBridge, pBridge);
      Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
      Float64 location = poi.GetDistFromStart();
      Float64 lft_supp_loc = pBridge->GetSegmentStartEndDistance(segmentKey);
      Float64 rgt_sup_loc = segment_length - pBridge->GetSegmentEndEndDistance(segmentKey);

      Float64 Sg;
      WBFL::Materials::Rebar::Size size;
      Float64 abar, nPrimaryLegs;
      Float64 Avs = GetPrimaryStirrupAvs(pConfig->StirrupConfig, getHorizShearStirrup, poi.GetDistFromStart(),
                                 segment_length, lft_supp_loc,rgt_sup_loc,
                                 &size, &abar, &nPrimaryLegs, &Sg);

      pArtifact->SetAvfGirder(abar*nPrimaryLegs);
      pArtifact->SetSGirder(Sg);

      // additional hi stirrups
      Float64 nAdditionalLegs;
      Float64 Avftf = GetAdditionalHorizInterfaceAvs(pConfig->StirrupConfig, poi.GetDistFromStart(),
                                 segment_length, lft_supp_loc,rgt_sup_loc,
                                 &size, &abar, &nAdditionalLegs, &Sg);

      pArtifact->SetAvfAdditional(abar*nAdditionalLegs);
      pArtifact->SetSAdditional(Sg);

      // legs per stirrup
      Float64 num_legs = nPrimaryLegs + nAdditionalLegs;

      pArtifact->SetNumLegs(num_legs);
   }
}

Float64 pgsDesigner2::GetNormalFrictionForce(const pgsPointOfInterest& poi) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);
   IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);

   GET_IFACE2(GetBroker(),IIntervals, pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(deckCastingRegionIdx);

   // permanent compressive force between slab and girder top
   // If the slab is CIP, use the tributary area.
   // If the slab is SIP, use only the area of cast slab that is NOT over
   // the deck panels.
   GET_IFACE2(GetBroker(),IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);

   GET_IFACE2(GetBroker(),ILibrary, pLib);
   GET_IFACE2(GetBroker(),ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   const auto& interface_shear_criteria = pSpecEntry->GetInterfaceShearCriteria();
   if (!interface_shear_criteria.bUseDeckWeightForPc)
   {
      return 0;
   }

   GET_IFACE2(GetBroker(),IBridge, pBridge);
   GET_IFACE2(GetBroker(),IMaterials, pMaterial);

   // slab load
   Float64 wslab = 0; // weight of slab on shear interface
   Float64 slab_unit_weight = pMaterial->GetDeckWeightDensity(deckCastingRegionIdx,castDeckIntervalIdx) * WBFL::Units::System::GetGravitationalAcceleration();

   if ( pDeck->GetDeckType() == pgsTypes::sdtCompositeCIP )
   {
      // Cast in place slab
      // conservative not to use sacrificial material so we will use just the structural slab depth
      // also, ignore the weight of the slab haunch as it may or may not be there depending on 
      // camber variation and other construction uncertainties
      GET_IFACE2(GetBroker(),ISectionProperties,pSectProp);
      Float64 slab_depth      = pBridge->GetStructuralSlabDepth(poi);
      Float64 trib_slab_width = pSectProp->GetTributaryFlangeWidth(poi);
      
      Float64 slab_volume = trib_slab_width*slab_depth;
      
      wslab = slab_volume * slab_unit_weight;
   }
   else
   {
      // SIP Deck Panels
      // For slab panels, the weight of the cast deck is mostly carried by the slab panels and does
      // not contribute to the compression force on the shear interface. The only slab dead load that
      // causes compression on the shear interface is weight of the slab between the
      // panels that bears on the top flange of the girder
      GET_IFACE2(GetBroker(),IGirder,pGdr);
      Float64 slab_depth       = pBridge->GetStructuralSlabDepth(poi);
      Float64 top_flange_width = pGdr->GetTopFlangeWidth(poi);
      Float64 panel_support    = pDeck->PanelSupport;

      MatingSurfaceIndexType nMatingSurfaces = pGdr->GetMatingSurfaceCount(segmentKey);
      Float64 wMating = 0; // sum of mating surface widths... less deck panel support width
      for ( MatingSurfaceIndexType i = 0; i < nMatingSurfaces; i++ )
      {
         if ( pBridge->IsExteriorGirder(segmentKey) && 
              ((segmentKey.girderIndex == 0 && i == 0) || // Left exterior girder
               (segmentKey.girderIndex == pGroup->GetGirderCount()-1 && i == nMatingSurfaces-1))  // right exterior girder
            )
         {
            wMating += pGdr->GetMatingSurfaceWidth(poi,i)/2 - panel_support;
         }
         else
         {
            wMating += pGdr->GetMatingSurfaceWidth(poi,i) - 2*panel_support;
         }
      }

      wslab = wMating*slab_depth*slab_unit_weight;

      // If exterior, add weight of cast overhang
      if ( pBridge->IsExteriorGirder(segmentKey) )
      {
         Float64 slab_overhang;

         Float64 station,offset;
         pBridge->GetStationAndOffset(poi,&station,&offset);
         Float64 start_station = pBridge->GetPierStation(0);
         Float64 Xb = station - start_station;

         if ( segmentKey.girderIndex == 0 )
         {
            slab_overhang = pBridge->GetLeftSlabOverhang(Xb); 
         }
         else
         {
            slab_overhang = pBridge->GetRightSlabOverhang(Xb);
         }

         Float64 top_width = pGdr->GetTopWidth(poi); // total width of the top of the girder

         Float64 woverhang = (slab_overhang - top_width/2)*slab_depth*slab_unit_weight;

         wslab += woverhang;
      }
   }

   return wslab;
}

void pgsDesigner2::CheckFullStirrupDetailing(const pgsPointOfInterest& poi, 
                                            const pgsVerticalShearArtifact& vertArtifact,
                                            const SHEARCAPACITYDETAILS& scd,
                                            const Float64 Vu,
                                            Float64 fcGdr, Float64 fy,
                                            const STIRRUPCONFIG* pConfig,
                                            pgsStirrupDetailArtifact* pArtifact ) const
{

   GET_IFACE2(GetBroker(),ILibrary,pLib);
   GET_IFACE2(GetBroker(),ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter1999 = ( WBFL::LRFD::BDSManager::Edition::SecondEditionWith2000Interims <= pSpecEntry->GetSpecificationCriteria().GetEdition() ? true : false );

   pArtifact->SetAfter1999(bAfter1999);

   // need bv and dv
   Float64 bv = scd.bv;
   Float64 dv = scd.dv;
   pArtifact->SetBv(bv);
   pArtifact->SetDv(dv);

   pArtifact->SetFc(fcGdr);
   pArtifact->SetFy(fy);

   // need theta for UHPC (GS 1.7.2.6)
   pArtifact->SetTheta(scd.Theta);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // av/s and fy rebar
   Float64 s;
   WBFL::Materials::Rebar::Size size;
   Float64 abar, nl;
   Float64 Avfs;
   if (pConfig == nullptr)
   {
      GET_IFACE2(GetBroker(),IStirrupGeometry, pStirrupGeometry);
      Avfs = pStirrupGeometry->GetVertStirrupAvs(poi, &size, &abar, &nl, &s);
   }
   else
   {
      GET_IFACE2(GetBroker(),IBridge, pBridge);
      Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
      Float64 location = poi.GetDistFromStart();
      Float64 lft_supp_loc = pBridge->GetSegmentStartBearingOffset(segmentKey);
      Float64 rgt_sup_loc = segment_length - pBridge->GetSegmentEndBearingOffset(segmentKey);

      Avfs = GetPrimaryStirrupAvs(*pConfig, getVerticalStirrup, location, segment_length, 
                                  lft_supp_loc, rgt_sup_loc, &size, &abar, &nl, &s);
   }

   pArtifact->SetBarSize(size);
   pArtifact->SetAvs(Avfs);
   pArtifact->SetS(s);

   // see if we even need to have stirrups
   bool bAreStirrupsRequired = vertArtifact.GetAreStirrupsReqd();
   pArtifact->SetApplicability(bAreStirrupsRequired ? true : false);

   // Set flag to indicate if we are in end region (outside of css) for reporting 
   pArtifact->SetIsInCriticalSectionZone(vertArtifact.IsInCriticalSectionZone());

   // Minimum transverse reinforcement 5.7.2.5 (pre2017: 5.8.2.5)
   // Set to zero if not applicable
   Float64 avs_min = 0.0;
   if (bAreStirrupsRequired)
   {
      avs_min = GetAvsMin(poi,scd);
   }
   pArtifact->SetAvsMin(avs_min);

   GET_IFACE2_NOCHECK(GetBroker(),ITransverseReinforcementSpec, pTransverseReinforcementSpec);// not used for UHPC

   CClosureKey closureKey;
   GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);
   bool bIsInClosure = pPoi->IsInClosureJoint(poi, &closureKey);

   GET_IFACE2(GetBroker(),IMaterials, pMaterials);
   if ((bIsInClosure && pMaterials->GetClosureJointConcreteType(closureKey) == pgsTypes::UHPC)
      || 
      pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
   {
      ATLASSERT(!bIsInClosure); // closures can't be UHPC yet
      Float64 theta = WBFL::Units::ConvertFromSysUnits(scd.Theta, WBFL::Units::Measure::Radian); // must be in radian
      Float64 cot_theta = 1 / tan(theta);
      dv = scd.controlling_uhpc_dv; // using dv per GS 1.7.2.8
      pArtifact->SetDv(dv);
      Float64 Smax = 0.25 * dv * cot_theta;
      Float64 SmaxLimit = WBFL::Units::ConvertToSysUnits(24.0, WBFL::Units::Measure::Inch); // 24 in limit per GS 1.7.2.6

      pArtifact->SetSMax(std::min<Float64>(Smax,SmaxLimit));
   }
   else
   {
      // max bar spacing
      Float64 s_max;
      Float64 s_under, s_over;
      pTransverseReinforcementSpec->GetMaxStirrupSpacing(dv, &s_under, &s_over);

      if (bAfter1999)
      {
         // applied shear stress
         Float64 vu = WBFL::LRFD::Shear::ComputeShearStress(Vu, scd.Vp, scd.Phi, scd.bv, scd.dv);
         pArtifact->Setvu(vu);

         Float64 vu_limit = 0.125 * fcGdr; // 5.7.2.6 (pre2017: 5.8.2.7)
         pArtifact->SetvuLimit(vu_limit);
         if (vu < vu_limit)
         {
            s_max = s_under;
         }
         else
         {
            s_max = s_over;
         }
         pArtifact->SetSMax(s_max);
      }
      else
      {
         // applied shear force
         pArtifact->SetVu(Vu);

         Float64 Vu_limit = 0.1 * fcGdr * bv * dv; // 5.7.2.6 (pre2017: 5.8.2.7)
         pArtifact->SetVuLimit(Vu_limit);
         if (Vu < Vu_limit)
         {
            s_max = s_under;
         }
         else
         {
            s_max = s_over;
         }
         pArtifact->SetSMax(s_max);
      }
   }

   // min bar spacing
   Float64 s_min = 0.0;
   if ((bIsInClosure && pMaterials->GetClosureJointConcreteType(closureKey) == pgsTypes::UHPC)
      ||
      pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
   {
      // UHPC GS 1.10.3
      ATLASSERT(!bIsInClosure); // closures can't be UHPC yet
      Float64 fiber_length = (bIsInClosure ? pMaterials->GetClosureJointConcreteFiberLength(closureKey) : pMaterials->GetSegmentConcreteFiberLength(segmentKey));
      Float64 limit = WBFL::Units::ConvertToSysUnits(0.75, WBFL::Units::Measure::Inch);
      s_min = Max(1.5 * fiber_length, limit);
   }
   else
   {
      if (size != WBFL::Materials::Rebar::Size::bsNone)
      {
         WBFL::Materials::Rebar::Type type;
         WBFL::Materials::Rebar::Grade grade;

         if (bIsInClosure)
         {
            pMaterials->GetClosureJointTransverseRebarMaterial(closureKey, &type, &grade);
         }
         else
         {
            pMaterials->GetSegmentTransverseRebarMaterial(segmentKey, &type, &grade);
         }

         const auto* prp = WBFL::LRFD::RebarPool::GetInstance();
         const auto* pRebar = prp->GetRebar(type, grade, size);

         Float64 db = pRebar->GetNominalDimension();
         Float64 as;
         if (bIsInClosure)
         {
            as = pMaterials->GetClosureJointMaxAggrSize(closureKey);
         }
         else
         {
            as = pMaterials->GetSegmentMaxAggrSize(segmentKey);
         }

         s_min = pTransverseReinforcementSpec->GetMinStirrupSpacing(as, db);
      }
   }
   pArtifact->SetSMin(s_min);
}

Float64 pgsDesigner2::GetAvsMin(const pgsPointOfInterest& poi,const SHEARCAPACITYDETAILS& scd) const
{
   const WBFL::Units::Length* pLengthUnit;
   const WBFL::Units::Stress* pStressUnit;
   const WBFL::Units::AreaPerLength* pAvsUnit;
   Float64 K;
   Float64 Kfct;
   if ( WBFL::LRFD::BDSManager::GetUnits() == WBFL::LRFD::BDSManager::Units::US )
   {
      pLengthUnit = &WBFL::Units::Measure::Inch;
      pStressUnit = &WBFL::Units::Measure::KSI;
      pAvsUnit    = &WBFL::Units::Measure::Inch2PerInch;
      K = 0.0316;
      Kfct = 4.7;
   }
   else
   {
      pLengthUnit = &WBFL::Units::Measure::Millimeter;
      pStressUnit = &WBFL::Units::Measure::MPa;
      pAvsUnit    = &WBFL::Units::Measure::Millimeter2PerMillimeter;
      K = 0.083;
      Kfct = 1.8;
   }

   Float64 bv = WBFL::Units::ConvertFromSysUnits(scd.bv,*pLengthUnit);
   Float64 fc = WBFL::Units::ConvertFromSysUnits(scd.fc,*pStressUnit);
   Float64 fy = WBFL::Units::ConvertFromSysUnits(scd.fy,*pStressUnit);
   Float64 fct= WBFL::Units::ConvertFromSysUnits(scd.fct,*pStressUnit);
   Float64 avs = K*bv/fy;

   GET_IFACE2(GetBroker(),IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetSegmentLambda(poi.GetSegmentKey());
   avs *= lambda;

   if ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::SeventhEditionWith2016Interims )
   {
      switch( scd.ConcreteType )
      {
      case pgsTypes::Normal:
         avs *= sqrt(fc);
         break;

      case pgsTypes::AllLightweight:
         if ( scd.bHasFct )
         {
            avs *= Min(Kfct*fct,sqrt(fc));
         }
         else
         {
            avs *= 0.75*sqrt(fc);
         }
         break;

      case pgsTypes::SandLightweight:
         if ( scd.bHasFct )
         {
            avs *= Min(Kfct*fct,sqrt(fc));
         }
         else
         {
            avs *= 0.85*sqrt(fc);
         }
         break;

      case pgsTypes::PCI_UHPC: // drop through
      case pgsTypes::UHPC: // drop through
      default:
         ATLASSERT(false); // is there a new concrete type? - shouldn't get here with UHPC
         avs *= sqrt(fc); 
         break;
      }
   }
   else
   {
      if (IsUHPC(scd.ConcreteType))
      {
         avs = 0.0; // there isn't a minimum Av/S for UHPC, stirrups not required - see PCI GS E.7.2.2 and AASHTO UHPC GS 1.7.2.5
      }
      else
      {
         avs *= sqrt(fc);
      }
   }

   avs = WBFL::Units::ConvertToSysUnits(avs,*pAvsUnit);

   return avs;
}

void pgsDesigner2::CheckLongReinfShear(const pgsPointOfInterest& poi, 
                                      IntervalIndexType intervalIdx,
                                      pgsTypes::LimitState limitState,
                                      const SHEARCAPACITYDETAILS& scd,
                                      const GDRCONFIG* pConfig,
                                      pgsLongReinfShearArtifact* pArtifact ) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   ZoneIndexType supportZoneIdx = GetSupportZoneIndex(poi);
   if ( supportZoneIdx != INVALID_INDEX )
   {
      // POI is between the CL pier and the FOS... this check is not applicable
      pArtifact->SetApplicability(false);
      return;
   }
   

   // the check is applicable
   GET_IFACE2(GetBroker(),ILibrary,pLib);
   GET_IFACE2(GetBroker(),ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& shear_capacity_criteria = pSpecEntry->GetShearCapacityCriteria();

   pArtifact->SetApplicability(true);

   // 9th edition added a requirement that ApsFps > AsFy
   // This requirement was developed primarily for simple span pretensioned girders. This limit will not be checked
   // in negative moment regions where the tension tie is the deck rebar, unless there are PT tendons providing
   // the tension tie
   GET_IFACE2(GetBroker(),IGirderTendonGeometry, pGirderTendonGeometry);
   auto nDucts = pGirderTendonGeometry->GetDuctCount(segmentKey);
   pArtifact->PretensionForceMustExceedBarForce((scd.bTensionBottom || 0 < nDucts) && WBFL::LRFD::BDSManager::Edition::NinthEdition2020 <= pSpecEntry->GetSpecificationCriteria().GetEdition() ? true : false);

   // Longitudinal steel
   GET_IFACE2(GetBroker(),IMaterials, pMaterials);
   Float64 Es, fy, fu;
   pMaterials->GetSegmentTransverseRebarProperties(segmentKey, &Es, &fy, &fu);
   pArtifact->SetFy(fy);
   pArtifact->SetEs(Es);
   ATLASSERT(IsEqual(fy, scd.fy));

   Float64 as = 0;
   if (shear_capacity_criteria.bIncludeRebar)
   {
      // TRICKY: Rebar data from config is not used here. This is only called from the design loop
      //         once (no iterations), so all we need is the current bridge data
      GET_IFACE2(GetBroker(),ILongRebarGeometry, pRebarGeometry);

      if (scd.bTensionBottom)
      {
         as = pRebarGeometry->GetAsBottomHalf(poi, false); // not adjusted for lack of development
         Float64 as2 = pRebarGeometry->GetAsBottomHalf(poi, true); // adjusted for lack of development
         if (!IsZero(as))
         {
            fy *= (as2 / as); // reduce effectiveness of bar for lack of development
         }
         else
         {
            fy = 0; // no strand, no development... reduce effectiveness to 0
         }

         pArtifact->SetFy(fy);
      }
      else
      {
         as = pRebarGeometry->GetAsTopHalf(poi, false); // not adjusted for lack of development
         Float64 as2 = pRebarGeometry->GetAsTopHalf(poi, true); // adjusted for lack of development
         if (!IsZero(as))
         {
            fy *= (as2 / as); // reduce effectiveness of bar for lack of development
         }
         else
         {
            fy = 0; // no strand, no development... reduce effectiveness to 0
         }

         pArtifact->SetFy(fy);
      }
   }
   pArtifact->SetAs(as);

   // prestress

   // area of prestress on flexural tension side
   // NOTE: fps (see below) from the moment capacity analysis already accounts for a reduction
   //       in strand effectiveness based on lack of development. DO NOT ADJUST THE AREA OF PRESTRESS
   //       HERE TO ACCOUNT FOR THE SAME TIME...
   GET_IFACE2(GetBroker(),IStrandGeometry, pStrandGeom);
   Float64 aps = (scd.bTensionBottom ? pStrandGeom->GetApsBottomHalf(poi, dlaNone, pConfig) : pStrandGeom->GetApsTopHalf(poi, dlaNone, pConfig));
   
   Float64 aptSegment,aptGirder;
   if ( pConfig == nullptr)
   {
      GET_IFACE2(GetBroker(),ISegmentTendonGeometry, pSegmentTendonGeometry);
      aptSegment = (scd.bTensionBottom ? pSegmentTendonGeometry->GetSegmentAptBottomHalf(poi) : pSegmentTendonGeometry->GetSegmentAptTopHalf(poi));

      GET_IFACE2(GetBroker(),IGirderTendonGeometry, pGirderTendonGeometry);
      aptGirder = (scd.bTensionBottom ? pGirderTendonGeometry->GetGirderAptBottomHalf(poi) : pGirderTendonGeometry->GetGirderAptTopHalf(poi));
   }
   else
   {
      aptSegment = 0; // no pt for design (design is only for PGSuper)
      aptGirder  = 0; // no pt for design (design is only for PGSuper)
   }

   // get prestress level at ultimate
   GET_IFACE2(GetBroker(),IMomentCapacity,pMomentCap);
   const MOMENTCAPACITYDETAILS* pmcd = pMomentCap->GetMomentCapacityDetails(intervalIdx, poi, scd.bTensionBottom, pConfig);

   Float64 fps = pmcd->fps_avg;
   pArtifact->SetAps(aps);
   pArtifact->SetFps(fps);

   Float64 fptSegment = pmcd->fpt_avg_segment;
   pArtifact->SetAptSegment(aptSegment);
   pArtifact->SetFptSegment(fptSegment);

   Float64 fptGirder = pmcd->fpt_avg_girder;
   pArtifact->SetAptGirder(aptGirder);
   pArtifact->SetFptGirder(fptGirder);

   // set up demands... if this section is in a critical section zone, use the values at the critical section
   // see C5.7.3.5 (pre2017: C5.8.3.5)

   // Critical section
   ZoneIndexType csZoneIdx = GetCriticalSectionZone(poi);
   bool bInCriticalSectionZone = (csZoneIdx == INVALID_INDEX ? false : true);

   Float64 vu = scd.Vu;
   Float64 vs = scd.Vs;
   Float64 vp = scd.Vp;
   Float64 theta = scd.Theta;
   if ( bInCriticalSectionZone )
   {
      // we are in a critical section zone
      const pgsPointOfInterest& csPoi(m_CriticalSections[csZoneIdx].first.GetPointOfInterest());
      GET_IFACE2(GetBroker(),IShearCapacity,pShearCapacity);
      SHEARCAPACITYDETAILS scd2;
      pShearCapacity->GetShearCapacityDetails(limitState,intervalIdx,csPoi,pConfig,&scd2);

      vu = scd2.Vu;
      vs = scd2.Vs;
      vp = scd2.Vp;
      theta = scd2.Theta;
   }


   // flexure demand
   Float64 mu = scd.RealMu;
   Float64 dv = scd.dv;
   ATLASSERT(dv!=0.0);
   Float64 phi_flexure = scd.PhiMu;
   pArtifact->SetMu(mu);
   pArtifact->SetDv(dv);
   pArtifact->SetFlexuralPhi(phi_flexure);

   // axial demand
   Float64 nu = scd.Nu;
   Float64 phi_axial;
   if (0.0 < nu)
   {
      phi_axial = 1.0;
   }
   else
   {
      phi_axial = 0.75; // does not consider seismic zones
   }

   pArtifact->SetNu(nu);
   pArtifact->SetAxialPhi(phi_axial);

   // shear demand 
   Float64 phi_shear = scd.Phi;

   if ( WBFL::LRFD::BDSManager::Edition::SecondEditionWith2000Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
       if ( vu/phi_shear < vs )
       {
           vs = vu/phi_shear;
       }
   }

   pArtifact->SetVu(vu);
   pArtifact->SetVs(vs);
   pArtifact->SetVp(vp);
   pArtifact->SetShearPhi(phi_shear);
   pArtifact->SetTheta(theta);

   // calculate required longitudinal reinforcement
   bool bContinuous = false;
   if ( bInCriticalSectionZone )
   {
      PierIndexType pierIdx = m_CriticalSections[csZoneIdx].first.PierIdx;
      pgsTypes::PierFaceType face = m_CriticalSections[csZoneIdx].first.PierFace;

      GET_IFACE2(GetBroker(),IBridge,pBridge);

      bool bContinuousLeft, bContinuousRight;
      pBridge->IsContinuousAtPier(pierIdx,&bContinuousLeft,&bContinuousRight);
      bContinuous = (face == pgsTypes::Ahead ? bContinuousRight : bContinuousLeft);
   }

   Float64 demand;
   Uint16 equation = 999; // dummy value

   if ( WBFL::LRFD::BDSManager::Edition::ThirdEditionWith2005Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      if ( bInCriticalSectionZone && !bContinuous )
      {
          // Equation 5.7.3.5-2 (pre2017: 5.8.3.5-2)
         demand = 0.5*nu/phi_axial +  (vu/phi_shear - 0.5*vs - vp)/tan(theta);
         equation = 2;
      }
      else
      {
        // Equation 5.7.3.5-1 (pre2017: 5.8.3.5-1)
        demand = fabs(mu)/(dv*phi_flexure) + 0.5*nu/phi_axial + (fabs(vu/phi_shear - vp) - 0.5*vs)/tan(theta);
        equation = 1;
      }
   }
   else
   {
      if (// Spec is 2003 or earlier AND poi is at one of the points of support 
           (WBFL::LRFD::BDSManager::GetEdition() <= WBFL::LRFD::BDSManager::Edition::SecondEditionWith2003Interims && 
            (poi.HasAttribute(POI_FACEOFSUPPORT) && !bContinuous) )
            ||
           // Spec is 2004 AND poi is in a critical section zone
           (WBFL::LRFD::BDSManager::Edition::SecondEditionWith2003Interims < WBFL::LRFD::BDSManager::GetEdition() && WBFL::LRFD::BDSManager::GetEdition() <= WBFL::LRFD::BDSManager::Edition::ThirdEdition2004 && 
              ( bInCriticalSectionZone && !bContinuous ))
         )
      {
          // Equation 5.7.3.5-2 (pre2017: 5.8.3.5-2)
         demand = 0.5*nu/phi_axial + (vu/phi_shear - 0.5*vs - vp)/tan(theta);
         equation = 2;
      }
      else
      {
          // Equation 5.7.3.5-1 (pre2017: 5.8.3.5-1)
         demand = mu/(dv*phi_flexure) + 0.5*nu/phi_axial + (vu/phi_shear - 0.5*vs - vp)/tan(theta);
         equation = 1;
      }
   }

   if ( equation == 1 )
   {
      // if equation 1 is used, this requirement will be satisfied if Mr >= Mu
      GET_IFACE2(GetBroker(),IMomentCapacity,pMomentCapacity);
      const MOMENTCAPACITYDETAILS* pmcd = pMomentCapacity->GetMomentCapacityDetails(intervalIdx, poi, scd.bTensionBottom, pConfig);

      Float64 Mr = pmcd->Phi * pmcd->Mn;
      pArtifact->SetMr(Mr);
   }

   if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
   {
      ATLASSERT(pConfig == nullptr); // pConfig is for design, no UHPC design(?)
      pArtifact->IsUHPC(true);

      Float64 et_loc = pMaterials->GetSegmentConcreteCrackLocalizationStrain(segmentKey);
      pArtifact->SetCrackLocalizationStrain(et_loc);

      GET_IFACE2(GetBroker(),IConcreteStressLimits, pLimits);
      Float64 gamma_u = pLimits->GetUHPCTensionStressLimitCoefficient(segmentKey);
      pArtifact->SetFiberOrientationReductionFactor(gamma_u);

      Float64 ft_cr = pMaterials->GetSegmentConcreteDesignEffectiveCrackingStrength(segmentKey);
      pArtifact->SetDesignEffectiveConcreteStrength(ft_cr);

      pArtifact->SetAct(scd.Ac);
   }

   pArtifact->SetEquation(equation);
   pArtifact->SetDemandForce(demand);
}

void pgsDesigner2::CheckConfinement(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig, pgsConfinementCheckArtifact* pArtifact ) const
{
   GET_IFACE2(GetBroker(),IBridge,pBridge);
   GET_IFACE2(GetBroker(),IGirder,pGdr);
   GET_IFACE2(GetBroker(),IMaterials,pMaterial);

   Float64 segment_length  = pBridge->GetSegmentLength(segmentKey);

   // If we are in here, confinement check is applicable
   pArtifact->SetApplicability(true);

   // Get spec constraints
   GET_IFACE2(GetBroker(),ITransverseReinforcementSpec,pTransverseReinforcementSpec);
   WBFL::Materials::Rebar::Size szmin = pTransverseReinforcementSpec->GetMinConfinementBarSize();
   Float64 smax = pTransverseReinforcementSpec->GetMaxConfinementBarSpacing();

   WBFL::Materials::Rebar::Grade grade;
   WBFL::Materials::Rebar::Type type;
   pMaterial->GetSegmentTransverseRebarMaterial(segmentKey,&type,&grade);

   pArtifact->SetMinBar(WBFL::LRFD::RebarPool::GetInstance()->GetRebar(type,grade,szmin));
   pArtifact->SetSMax(smax);

   // Use utility function to get confinement zone lengths at girder ends
   Float64 zoneFactor, startd, endd;
   Float64 reqdStartZl, reqdEndZl;
   GetConfinementZoneLengths(segmentKey, pGdr, segment_length, &zoneFactor, &startd, &endd, &reqdStartZl, &reqdEndZl);

   pArtifact->SetZoneLengthFactor(zoneFactor);

   pArtifact->SetStartRequiredZoneLength(reqdStartZl);
   pArtifact->SetStartd(startd);

   pArtifact->SetEndRequiredZoneLength(reqdEndZl);
   pArtifact->SetEndd(endd);

   // get and set provided stirrup configuration at start and ends
   WBFL::Materials::Rebar::Size start_rbsiz, end_rbsiz;
   Float64 start_zl, end_zl;
   Float64 start_s, end_s;
   if (pConfig)
   {
      GetConfinementInfoFromStirrupConfig(pConfig->StirrupConfig, reqdStartZl, &start_rbsiz, &start_zl, &start_s,
                                                   reqdEndZl, &end_rbsiz, &end_zl, &end_s);
   }
   else
   {
      GET_IFACE2(GetBroker(),IStirrupGeometry, pStirrupGeometry);
      pStirrupGeometry->GetStartConfinementBarInfo(segmentKey, reqdStartZl, &start_rbsiz, &start_zl, &start_s);
      pStirrupGeometry->GetEndConfinementBarInfo(segmentKey, reqdEndZl, &end_rbsiz, &end_zl, &end_s);
   }

   pArtifact->SetStartS(start_s);
   pArtifact->SetStartProvidedZoneLength(start_zl);
   pArtifact->SetStartBar(WBFL::LRFD::RebarPool::GetInstance()->GetRebar(type,grade,start_rbsiz));

   pArtifact->SetEndS(end_s);
   pArtifact->SetEndProvidedZoneLength(end_zl);
   pArtifact->SetEndBar(WBFL::LRFD::RebarPool::GetInstance()->GetRebar(type,grade,end_rbsiz));
}

void pgsDesigner2::CheckMomentCapacity(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsGirderArtifact* pGirderArtifact) const
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   // Get points of interest for evaluation
   PoiList vPoi; // POIs for both positive and negative moment
   PoiList vNMPoi; // additional POIs for negative moment only

   GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);
   GET_IFACE2(GetBroker(),IBridge,pBridge);
   SpanIndexType startSpanIdx, endSpanIdx;
   pBridge->GetGirderGroupSpans(girderKey.groupIndex,&startSpanIdx,&endSpanIdx);
   bool bComputeNegativeMomentCapacity = false;
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      if ( pBridge->ProcessNegativeMoments(spanIdx) )
      {
         bComputeNegativeMomentCapacity = true;
      }

      PoiList vPoiThisSpan;
      pPoi->GetPointsOfInterest(CSpanKey(spanIdx, girderKey.girderIndex), POI_SPAN | POI_TENTH_POINTS, &vPoiThisSpan);
      vPoi.insert(vPoi.end(),vPoiThisSpan.begin(),vPoiThisSpan.end());

      PoiList vNMPoiThisSpan;
      pPoi->GetPointsOfInterest(CSpanKey(spanIdx, girderKey.girderIndex), POI_FACEOFSUPPORT | POI_DECKBARCUTOFF, &vNMPoiThisSpan, POIFIND_OR);
      vNMPoi.insert(vNMPoi.end(), vNMPoiThisSpan.begin(), vNMPoiThisSpan.end());
   }
   pPoi->SortPoiList(&vPoi);
   pPoi->SortPoiList(&vNMPoi);

   // NOTE: This would be a good place for multi-threading... instead of computing +Mn and -Mn for each POI,
   // send the vector of POI to the computer engine for +Mn and -Mn so they can be computed at the same time!
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      // we always do positive moment
      pgsFlexuralCapacityArtifact pmArtifact(true);
      CreateFlexuralCapacityArtifact(poi,intervalIdx,limitState,true,&pmArtifact);
      pGirderArtifact->AddPositiveMomentFlexuralCapacityArtifact(intervalIdx, limitState, pmArtifact);

      // negative moment is a different story. there must be a negative moment connection
      // at one end of the span
      if ( liveLoadIntervalIdx <= intervalIdx && bComputeNegativeMomentCapacity )
      {
         pgsFlexuralCapacityArtifact nmArtifact(false);
         nmArtifact.SetPointOfInterest(poi);
         CreateFlexuralCapacityArtifact(poi,intervalIdx,limitState,false,&nmArtifact);
         pGirderArtifact->AddNegativeMomentFlexuralCapacityArtifact(intervalIdx, limitState, nmArtifact);
      }
   }

   if (liveLoadIntervalIdx <= intervalIdx && bComputeNegativeMomentCapacity)
   {
      for (const pgsPointOfInterest& poi : vNMPoi)
      {
         pgsFlexuralCapacityArtifact nmArtifact(false);
         nmArtifact.SetPointOfInterest(poi);
         CreateFlexuralCapacityArtifact(poi, intervalIdx, limitState, false, &nmArtifact);
         pGirderArtifact->AddNegativeMomentFlexuralCapacityArtifact(intervalIdx, limitState, nmArtifact);
      }
   }
}

void pgsDesigner2::InitSupportZones(const CSegmentKey& segmentKey) const
{
   // cache support zone locations are they are expensive to get
   m_SupportZones.clear();

   // get the face of support poi for this segment
   GET_IFACE2(GetBroker(),IPointOfInterest,pPOI);
   PoiList vPoi;
   pPOI->GetPointsOfInterest(segmentKey, POI_FACEOFSUPPORT, &vPoi);

   // get the piers that go with the face of supports
   std::vector<std::pair<const CPierData2*,pgsTypes::PierFaceType>> vPiers;
   GET_IFACE2(GetBroker(),IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);

   // Pier at start of segment ?
   const CPierData2* pPier;
   const CTemporarySupportData* pTS;
   pSegment->GetSupport(pgsTypes::metStart,&pPier,&pTS);
   if ( pPier )
   {
      vPiers.emplace_back(pPier,pgsTypes::Ahead);
   }

   // piers between the ends of the segment
   const CSpanData2* pStartSpan = pSegment->GetSpan(pgsTypes::metStart);
   const CSpanData2* pEndSpan   = pSegment->GetSpan(pgsTypes::metEnd);
   pPier = pStartSpan->GetNextPier();
   while ( pPier != pEndSpan->GetNextPier() )
   {
      vPiers.emplace_back(pPier,pgsTypes::Back); // left FOS
      vPiers.emplace_back(pPier,pgsTypes::Ahead); // right FOS

      pPier = pPier->GetNextSpan()->GetNextPier();
   }

   // Pier at end of segment ?
   pSegment->GetSupport(pgsTypes::metEnd,&pPier,&pTS);
   if ( pPier )
   {
      vPiers.emplace_back(pPier,pgsTypes::Back);
   }

   // should be one pier for each poi
   ATLASSERT(vPoi.size() == vPiers.size());

   GET_IFACE2_NOCHECK(GetBroker(),IBridge,pBridge); // there are cases (drop in span) where there aren't any face of supports and this never gets used

   auto fosIter(vPoi.begin());
   auto fosEnd(vPoi.end());
   std::vector<std::pair<const CPierData2*,pgsTypes::PierFaceType>>::iterator pierIter(vPiers.begin());
   for ( ; fosIter != fosEnd; fosIter++, pierIter++ )
   {
      const pgsPointOfInterest& poiFaceOfSupport(*fosIter);

      // need to get pier index that goes with this FOS
      const CPierData2* pPier = pierIter->first;
      pgsTypes::PierFaceType face = pierIter->second;
      PierIndexType pierIdx = pPier->GetIndex();

      // location of CL pier from start of segment
      Float64 XclBrg;
      pBridge->GetPierLocation(pierIdx,segmentKey,&XclBrg);

      Float64 start, end;
      if ( face == pgsTypes::Ahead )
      {
         // FOS is on ahead side of pier so zone goes from XclBrg to FOS location
         start = XclBrg;
         end   = poiFaceOfSupport.GetDistFromStart();
      }
      else
      {
         // FOS is on back side of pier so zone goes from FOS to XclBrg
         start = poiFaceOfSupport.GetDistFromStart();
         end = XclBrg;
      }

      SUPPORTZONE supportZone;
      supportZone.Start = start;
      supportZone.End = end;
      supportZone.PierIdx = pierIdx;
      supportZone.PierFace = face;

      m_SupportZones.push_back(supportZone);
   }
}

void pgsDesigner2::InitShearCheck(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const GDRCONFIG* pConfig) const
{
   GET_IFACE2(GetBroker(),ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   GET_IFACE2(GetBroker(),IBridge,pBridge);
   GET_IFACE2(GetBroker(),IShearCapacity,pShearCapacity);

#if defined _DEBUG
   // Checking shear should only be occurring at the final condition.... that is, only in intervals
   // after the live load is applied
   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   ATLASSERT(liveLoadIntervalIdx <= intervalIdx);
#endif

   InitSupportZones(segmentKey);

   // cache CS locations as they are very expensive to get
   // First try to get them from our list of POIs
   m_CriticalSections.clear();
   // Critical sections not in the POI list - we need to compute them - this is really expensive,
   // and likely for load rating cases only
   PoiList vCSPoi;
   if( pConfig == nullptr)
   {
      GET_IFACE2(GetBroker(),IPointOfInterest,pPoi);
      pPoi->GetCriticalSections(limitState, segmentKey,&vCSPoi);
      std::vector<CRITSECTDETAILS> vCS = pShearCapacity->GetCriticalSectionDetails(limitState,segmentKey);

      if (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEdition2004)
      {
         // if the LRFD is before 2004, critical section for shear was a function of loading.... we end up with many critical section POIs but
         // only a few (usually 2) critical section details. Match the details to the POIs and throw out the other POIs. LRFD 2004 and later only depend on Mu
         // so the number of CS POIs and CS details should always match.
         vCSPoi.erase(
            std::remove_if(vCSPoi.begin(), vCSPoi.end(), [&vCS](const pgsPointOfInterest& poi)
         {
            return std::find_if(vCS.begin(), vCS.end(), [&poi](const auto& csDetails) {return csDetails.GetPointOfInterest().AtSamePlace(poi);}) == vCS.cend();
         }),
            vCSPoi.end());
      }

      ATLASSERT(vCSPoi.size() == vCS.size());
      std::vector<CRITSECTDETAILS>::iterator iter(vCS.begin());
      std::vector<CRITSECTDETAILS>::iterator end(vCS.end());
      auto poiIter(vCSPoi.begin());
      for ( ; iter != end; iter++, poiIter++ )
      {
         CRITSECTDETAILS& csDetails(*iter);
         if ( csDetails.bAtFaceOfSupport )
         {
            csDetails.poiFaceOfSupport = *poiIter;
         }
         else
         {
            csDetails.pCriticalSection->Poi = *poiIter;
         }
#if defined _DEBUG
         const pgsPointOfInterest& csPoi = csDetails.GetPointOfInterest();
         ATLASSERT(csPoi.GetID() != INVALID_ID);
         ATLASSERT(csPoi.GetSegmentKey() == poiIter->get().GetSegmentKey());
         ATLASSERT(IsEqual(csPoi.GetDistFromStart(),poiIter->get().GetDistFromStart()));
#endif
         m_CriticalSections.emplace_back(csDetails,false);
      }
   }
   else
   {
      //std::vector<pgsPointOfInterest> vCSPoi(pPoi->GetCriticalSections(limitState,segmentKey,*pConfig)); // these POIs don't have IDs assigned (they are temporary POI, not part of the real bridge)
      vCSPoi = m_ShearDesignTool.GetCriticalSections(); // these POIs have IDs assigned. They are temporary POIs held in the shear design tool. We want to use the ones with IDs so results at critical sections are cached (namely Mn)

      const std::vector<CRITSECTDETAILS>& vCS = pShearCapacity->GetCriticalSectionDetails(limitState,segmentKey,pConfig);

      if (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::ThirdEdition2004)
      {
         // if the LRFD is before 2004, critical section for shear was a function of loading.... we end up with many critical section POIs but
         // only a few (usually 2) critical section details. Match the details to the POIs and throw out the other POIs. LRFD 2004 and later only depend on Mu
         // so the number of CS POIs and CS details should always match.
         vCSPoi.erase(
            std::remove_if(vCSPoi.begin(), vCSPoi.end(), [&vCS](auto& poi)
         {
            return std::find_if(vCS.begin(), vCS.end(), [&poi](const auto& csDetails) {return csDetails.GetPointOfInterest().AtSamePlace(poi);}) == vCS.cend();
         }),
            vCSPoi.end());
      }

      ATLASSERT(vCSPoi.size() == vCS.size());

      // Assigned the POIs with IDs to the details because we don't want to work with POIs without valid IDs
      auto iter(vCS.begin());
      auto end(vCS.end());
      auto poiIter(vCSPoi.begin());
      for ( ; iter != end; iter++, poiIter++ )
      {
         CRITSECTDETAILS csDetails(*iter);

         ATLASSERT(csDetails.GetPointOfInterest().GetSegmentKey() == poiIter->get().GetSegmentKey());
         ATLASSERT(IsEqual(csDetails.GetPointOfInterest().GetDistFromStart(),poiIter->get().GetDistFromStart()));
         ATLASSERT(poiIter->get().GetID() != INVALID_INDEX);

         csDetails.SetPointOfInterest(*poiIter);

         m_CriticalSections.emplace_back(csDetails,false);
      }
   }

   std::vector<std::pair<CRITSECTDETAILS,bool>>::iterator csIter(m_CriticalSections.begin());
   std::vector<std::pair<CRITSECTDETAILS,bool>>::iterator csIterEnd(m_CriticalSections.end());
   for ( ; csIter != csIterEnd; csIter++ )
   {
      CRITSECTDETAILS& csDetails(csIter->first);
      const pgsPointOfInterest& csPoi = csDetails.GetPointOfInterest();
      ATLASSERT(csPoi.GetID() != INVALID_ID);

      // DETERMINE IF vu <= 0.18f'c at each POI... set a boolean flag that indicates if strut and tie analysis is required
      // LRFD 5.7.3.2 (pre2017: 5.8.3.2)
      PierIndexType pierIdx = csDetails.PierIdx;
      pgsTypes::PierFaceType pierFace = csDetails.PierFace;

      bool bIntegralLeft, bIntegralRight;
      pBridge->IsIntegralAtPier(pierIdx,&bIntegralLeft,&bIntegralRight);
      bool bIntegral = (pierFace == pgsTypes::Back ? bIntegralLeft : bIntegralRight);

      SHEARCAPACITYDETAILS scd;
      pShearCapacity->GetShearCapacityDetails( limitState, intervalIdx, csPoi, pConfig, &scd );

      // NOTE: scd.vfc is v/f'c. Since v is divided by f'c, 0.18f'c divided by f'c is simply 0.18
      csIter->second = (0.18 < scd.vufc && !bIntegral);
   }
}

void pgsDesigner2::GetShearPointsOfInterest(bool bDesign,const CSegmentKey& segmentKey,pgsTypes::LimitState limitState,IntervalIndexType intervalIdx,PoiList& vPoi) const
{
   if (bDesign)
   {
      vPoi = m_ShearDesignTool.GetDesignPoi();
   }
   else
   {
      GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);
      PoiList pois;
      pPoi->GetPointsOfInterest(segmentKey, POI_SPAN, &pois);

      PoiList csPoi;
      pPoi->GetCriticalSections(limitState, segmentKey, &csPoi); // this gets all CS for the girderline
      // only keep CS poi on this segment
      for (const auto& poi : csPoi)
      {
         if (poi.get().GetSegmentKey() == segmentKey)
         {
            pois.emplace_back(poi);
         }
      }

      PoiList morePoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_FACEOFSUPPORT | POI_HARPINGPOINT | POI_STIRRUP_ZONE | POI_CONCLOAD | POI_DIAPHRAGM | POI_DECKBARCUTOFF | POI_BARCUTOFF | POI_BARDEVELOP | POI_DEBOND, &morePoi, POIFIND_OR);
      pois.insert(std::end(pois), std::begin(morePoi), std::end(morePoi));

      // if closures can take any load, add it to the list of poi
      GET_IFACE2_NOCHECK(GetBroker(),IIntervals, pIntervals);
      GET_IFACE2(GetBroker(),IBridge, pBridge);
      SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);
      if (segmentKey.segmentIndex < nSegments - 1 && pIntervals->GetCompositeClosureJointInterval(segmentKey) <= intervalIdx)
      {
         PoiList vCJPoi;
         pPoi->GetPointsOfInterest(segmentKey, POI_CLOSURE, &vCJPoi);
         pois.insert(std::end(pois), std::begin(vCJPoi), std::end(vCJPoi));
      }


      // these poi are for the WSDOT summary report. They are traditional location for reporting shear checks
      morePoi.clear();
      pPoi->GetPointsOfInterest(segmentKey, POI_H | POI_15H, &morePoi, POIFIND_OR);
      pois.insert(pois.end(), morePoi.begin(), morePoi.end());

      pPoi->SortPoiList(&pois); // sort and remove duplicates

      // remove all POI from the container that are outside of the CL Bearings...
      // PoiIsOutsideOfBearings does the filtering and it keeps POIs that are at the closure joint (and this is what we want)
      Float64 segmentSpanLength = pBridge->GetSegmentSpanLength(segmentKey);
      Float64 endDist = pBridge->GetSegmentStartEndDistance(segmentKey);
      std::remove_copy_if(pois.begin(), pois.end(), std::back_inserter(vPoi), PoiIsOutsideOfBearings(segmentKey, endDist, endDist + segmentSpanLength));
   }
}

void pgsDesigner2::CheckShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsGirderArtifact* pGirderArtifact) const
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   GET_IFACE2(GetBroker(),IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
   {
      pgsSegmentArtifact* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      pgsStirrupCheckArtifact* pStirrupArtifact = pSegmentArtifact->GetStirrupCheckArtifact();
      CheckShear(false,pSegmentArtifact->GetSegmentKey(),intervalIdx,limitState,nullptr,pStirrupArtifact);
   }
}

void pgsDesigner2::CheckShear(bool bDesign,const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,
                              pgsTypes::LimitState limitState,const GDRCONFIG* pConfig,pgsStirrupCheckArtifact* pStirrupArtifact) const
{
#if defined _DEBUG
   // Checking shear should only be occurring at the final condition.... that is, only in intervals
   // after the live load is applied
   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   ATLASSERT(liveLoadIntervalIdx <= intervalIdx);
#endif

   InitShearCheck(segmentKey,intervalIdx,limitState,pConfig); // sets up some class member variables used for checking this segment

   // InitShearCheck causes the critical section for shear POI to be created...
   // Get the POI here so the CS poi are in the list
   PoiList vPoi;
   GetShearPointsOfInterest(bDesign, segmentKey, limitState, intervalIdx, vPoi);

   ATLASSERT(pStirrupArtifact != nullptr);
   GET_IFACE2(GetBroker(),IMaterials,pMaterials);
   Float64 fc_slab = pMaterials->GetDeckDesignFc(intervalIdx);

   Float64 fc_girder;
   if ( pConfig == nullptr )
   {
      fc_girder = pMaterials->GetSegmentFc28(segmentKey);
   }
   else
   {
      fc_girder = pConfig->fc28;
   }

   Float64 Es, fy, fu;
   pMaterials->GetSegmentTransverseRebarProperties(segmentKey,&Es,&fy,&fu);

   GET_IFACE2(GetBroker(),ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // Confinement check
   GET_IFACE2(GetBroker(),ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bCheckConfinement = pSpecEntry->GetEndZoneCriteria().bCheckConfinement && limitState==pgsTypes::StrengthI; // only need to check confinement once

   pgsConfinementCheckArtifact c_artifact;
   if (bCheckConfinement)
   {
      CheckConfinement(segmentKey, pConfig, &c_artifact);
      pStirrupArtifact->SetConfinementArtifact(c_artifact);
   }

   // Splitting zone check
   pStirrupArtifact->SetSplittingCheckArtifact(CheckSplittingZone(segmentKey,pConfig));


   // poi-based shear check
   GET_IFACE2(GetBroker(),ILimitStateForces, pLimitStateForces);
   GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);

   // loop over pois
   for ( const pgsPointOfInterest& poi : vPoi)
   {
      if ( poi.HasAttribute(POI_BOUNDARY_PIER) && !poi.HasAttribute(POI_CLOSURE))
      {
         //pgsStirrupCheckAtPoisArtifact artifact;
         //pStirrupArtifact->AddStirrupCheckAtPoisArtifact(intervalIdx,limitState,artifact);
         continue;
      }

      // Take demand at CSS if poi is in end region
      pgsPointOfInterest poi_4demand;
      ZoneIndexType csZoneIdx = GetCriticalSectionZone(poi);
      if ( csZoneIdx == INVALID_INDEX )
      {
         // not in a critical section zone
         poi_4demand = poi;
      }
      else
      {
         // in a critical section zone... get the demand at the critical section
         poi_4demand = m_CriticalSections[csZoneIdx].first.GetPointOfInterest();
      }

      WBFL::System::SectionValue Vmin, Vmax;
      if ( analysisType == pgsTypes::Envelope )
      {
         WBFL::System::SectionValue min,max;
         pLimitStateForces->GetShear(intervalIdx,limitState,poi_4demand,pgsTypes::MaxSimpleContinuousEnvelope,&min,&max);
         Vmax = max;

         pLimitStateForces->GetShear(intervalIdx,limitState,poi_4demand,pgsTypes::MinSimpleContinuousEnvelope,&min,&max);
         Vmin = min;
      }
      else
      {
         pLimitStateForces->GetShear(intervalIdx,limitState,poi_4demand,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,&Vmin,&Vmax);
      }

      // Take max absolute value for demand
      Float64 Vu = Max(abs(Vmin.Left()),abs(Vmax.Left()),abs(Vmin.Right()),abs(Vmax.Right()));

      CClosureKey closureKey;
      if ( pPoi->IsInClosureJoint(poi,&closureKey) )
      {
         fc_girder = pMaterials->GetClosureJointFc(closureKey,intervalIdx);
         pMaterials->GetClosureJointTransverseRebarProperties(closureKey,&Es,&fy,&fu);
      }

      pgsStirrupCheckAtPoisArtifact artifact;
      CreateStirrupCheckAtPoisArtifact(poi,intervalIdx,limitState,Vu,fc_slab,fc_girder,fy,bCheckConfinement,pConfig,&artifact);

      pStirrupArtifact->AddStirrupCheckAtPoisArtifact(intervalIdx,limitState,artifact);
   } // next POI
}

std::shared_ptr<pgsSplittingCheckArtifact> pgsDesigner2::CheckSplittingZone(const CSegmentKey& segmentKey,const GDRCONFIG* pConfig) const
{
   GET_IFACE2(GetBroker(),IEAFProgress, pProgress);
   WBFL::EAF::AutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Checking splitting requirements"));

   GET_IFACE2(GetBroker(),ISplittingChecks,pSplittingChecks);
   return pSplittingChecks->CheckSplitting(segmentKey, pConfig);
}

void pgsDesigner2::CheckSegmentDetailing(const CSegmentKey& segmentKey,pgsSegmentArtifact* pGdrArtifact) const
{
   // 5.12.3.2.2 (pre2017: 5.14.1.2.2)
   GET_IFACE2(GetBroker(),IEAFProgress,pProgress);
   WBFL::EAF::AutoProgress ap(pProgress);
   pProgress->UpdateMessage( _T("Checking segment detailing") );

   pgsPrecastIGirderDetailingArtifact* pArtifact = pGdrArtifact->GetPrecastIGirderDetailingArtifact();

   // get min girder dimensions from spec
   GET_IFACE2(GetBroker(),IPrecastIGirderDetailsSpec,pPrecastIGirderDetailsSpec);
   pArtifact->SetMinTopFlangeThickness(pPrecastIGirderDetailsSpec->GetMinTopFlangeThickness());
   pArtifact->SetMinWebThickness(pPrecastIGirderDetailsSpec->GetMinWebThickness());
   pArtifact->SetMinBottomFlangeThickness(pPrecastIGirderDetailsSpec->GetMinBottomFlangeThickness());

   // get dimensions from bridge model
   GET_IFACE2(GetBroker(),IPointOfInterest,pPOI);
   PoiList vPoi;
   pPOI->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT, &vPoi);

   Float64 minTopFlange = DBL_MAX;
   Float64 minBotFlange = DBL_MAX;
   Float64 minWeb       = DBL_MAX;

   GET_IFACE2(GetBroker(),IGirder,pGdr);
   FlangeIndexType nTopFlanges = pGdr->GetTopFlangeCount(segmentKey);
   WebIndexType    nWebs       = pGdr->GetWebCount(segmentKey);
   FlangeIndexType nBotFlanges = pGdr->GetBottomFlangeCount(segmentKey);

   if ( nTopFlanges != 0 || nWebs != 0 || nBotFlanges != 0 )
   {
      for(const pgsPointOfInterest& poi : vPoi)
      {
         minBotFlange = Min(minBotFlange,pGdr->GetMinBottomFlangeThickness(poi));
         minWeb       = Min(minWeb,      pGdr->GetMinWebWidth(poi));
         minTopFlange = Min(minTopFlange,pGdr->GetMinTopFlangeThickness(poi));
      }
   }

   if ( 0 == nTopFlanges )
   {
      pArtifact->SetProvidedTopFlangeThickness(0);
   }
   else
   {
      pArtifact->SetProvidedTopFlangeThickness(minTopFlange);
   }

   GET_IFACE2_NOCHECK(GetBroker(),IMaterials, pMaterials);
   if (  0 == nWebs || IsUHPC(pMaterials->GetSegmentConcreteType(segmentKey)))
   {
      // this is kind of a hack for UHPC
      // UHPC can have much thinner webs than conventional concrete... the LRFD limit doesn't apply
      // we can skip the spec check by saying the web thickness is zero.
      pArtifact->SetProvidedWebThickness(0);
   }
   else
   {
      pArtifact->SetProvidedWebThickness(minWeb);
   }

   if ( 0 == nBotFlanges )
   {
      pArtifact->SetProvidedBottomFlangeThickness(0);
   }
   else
   {
      pArtifact->SetProvidedBottomFlangeThickness(minBotFlange);
   }
}

void pgsDesigner2::CheckStrandSlope(const CSegmentKey& segmentKey,pgsStrandSlopeArtifact* pArtifact) const
{
   GET_IFACE2(GetBroker(),IStrandGeometry,pStrGeom);
   StrandIndexType nStrands = pStrGeom->GetStrandCount(segmentKey,pgsTypes::Harped);
   if ( nStrands == 0 )
   {
      pArtifact->IsApplicable(false);
      return;
   }

   GET_IFACE2(GetBroker(),ISpecification,pSpec);
   GET_IFACE2(GetBroker(),ILibrary,pLib);
   GET_IFACE2(GetBroker(),IMaterials,pMaterial);

   GET_IFACE2(GetBroker(),IEAFProgress,pProgress);
   WBFL::EAF::AutoProgress ap(pProgress);
   pProgress->UpdateMessage( _T("Checking strand slope requirements") );

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   const auto& strand_slope_criteria = pSpecEntry->GetStrandSlopeCriteria();
   pArtifact->IsApplicable( strand_slope_criteria.bCheck );

   // we are looking for strand diameter for harped strand slope so use Harped here
   const auto* pStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Harped);
   auto capacity = strand_slope_criteria.GetStrandSlopeLimit(pStrand->GetSize());
   auto demand = pStrGeom->GetMaxStrandSlope( segmentKey ); // +/- value
   demand = fabs(demand); // capacity is always positive so use absolute value of demand

   pArtifact->SetCapacity( capacity );
   pArtifact->SetDemand( demand );
}

void pgsDesigner2::CheckHoldDownForce(const CSegmentKey& segmentKey,pgsHoldDownForceArtifact* pArtifact) const
{
   GET_IFACE2(GetBroker(),ISpecification,pSpec);
   GET_IFACE2(GetBroker(),ILibrary,pLib);
   GET_IFACE2(GetBroker(),IPretensionForce,pPrestressForce);

   GET_IFACE2(GetBroker(),IEAFProgress,pProgress);
   WBFL::EAF::AutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Checking hold down force requirements"));

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   const auto& hold_down_criteria = pSpecEntry->GetHoldDownCriteria();
   pArtifact->IsApplicable(hold_down_criteria.bCheck );

   Float64 demand = pPrestressForce->GetHoldDownForce(segmentKey, hold_down_criteria.type);

   pArtifact->SetCapacity(hold_down_criteria.force_limit );
   pArtifact->SetDemand( demand );
}

void pgsDesigner2::CheckPlantHandlingWeightLimit(const CSegmentKey& segmentKey, pgsPlantHandlingWeightArtifact* pArtifact) const
{
   GET_IFACE2(GetBroker(),ISpecification, pSpec);
   GET_IFACE2(GetBroker(),ILibrary, pLib);

   GET_IFACE2(GetBroker(),IEAFProgress, pProgress);
   WBFL::EAF::AutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Checking plant handling weight requirements"));

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   const auto& plant_handling_criteria = pSpecEntry->GetPlantHandlingCriteria();

   GET_IFACE2(GetBroker(),ISectionProperties, pSectProps);
   Float64 Wg = pSectProps->GetSegmentWeight(segmentKey);

   pArtifact->IsApplicable(plant_handling_criteria.bCheck);
   pArtifact->SetWeight(Wg);
   pArtifact->SetWeightLimit(plant_handling_criteria.WeightLimit);
}

void pgsDesigner2::CheckLiveLoadDeflection(const CGirderKey& girderKey,pgsGirderArtifact* pGdrArtifact) const
{
   // Girders can go across multiple spans... deflection checks are done by span
   // Example
   //
   //   =====================================================================
   //   ^                      ^                    ^                       ^
   //
   // One continuous spliced girder, going over 3 spans... there will be 3 deflection checks

   GET_IFACE2(GetBroker(),ILibrary, pLib );
   GET_IFACE2(GetBroker(),ISpecification, pSpec );
   pgsTypes::BridgeAnalysisType bat = (pSpec->GetAnalysisType() == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);

   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   GET_IFACE2(GetBroker(),IBridge,pBridge);
   // determine spans that are involved in this check
   SpanIndexType startSpanIdx;
   SpanIndexType endSpanIdx;
   pBridge->GetGirderGroupSpans(girderKey.groupIndex,&startSpanIdx,&endSpanIdx);

   // Get the POIs for this girder
   GET_IFACE2(GetBroker(),IPointOfInterest,pPoi);
   GET_IFACE2(GetBroker(),IProductForces,pForces);
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      PoiList vPoi;
      pPoi->GetPointsOfInterest(CSpanKey(spanIdx, girderKey.girderIndex), POI_ERECTED_SEGMENT, &vPoi);

      pgsDeflectionCheckArtifact artifact(spanIdx);

      const auto& live_load_deflection_criteria = pSpecEntry->GetLiveLoadDeflectionCriteria();
      if (live_load_deflection_criteria.bCheck)
      {
         GET_IFACE2(GetBroker(),IEAFProgress,pProgress);
         WBFL::EAF::AutoProgress ap(pProgress);
         pProgress->UpdateMessage( _T("Checking live load deflection requirements") );

         artifact.IsApplicable(true);

         // get max allowable deflection
         Float64 L = pBridge->GetSpanLength(spanIdx,girderKey.girderIndex); // span length for this girder (cl-brg to cl-brg length)
         Float64 ratio = live_load_deflection_criteria.DeflectionLimit;
         CHECK(0.0 < ratio);
         Float64 capacity = L/ratio;

         artifact.SetAllowableSpanRatio(ratio);
         artifact.SetCapacity(capacity);

         //
         // find maximum live load deflection in each span
         //
      
         Float64 min_defl =  DBL_MAX;
         Float64 max_defl = -DBL_MAX;

         for ( const pgsPointOfInterest& poi : vPoi)
         {
#if defined _DEBUG
            // make sure the poi is actually in this span
            CSpanKey thisSpanKey;
            Float64 Xspan;
            pPoi->ConvertPoiToSpanPoint(poi,&thisSpanKey,&Xspan);
            if ( !poi.HasAttribute(POI_INTERMEDIATE_PIER) )
            {
               ATLASSERT(thisSpanKey.spanIndex == spanIdx);
            }
#endif // _DEBUG

            // Determine if this POI is in the span that is currently being evaluated
            if ( poi.HasAttribute(POI_CRITSECTSHEAR1) || poi.HasAttribute(POI_CRITSECTSHEAR2) )
            {
               // skip if critical section as there aren't deflection results at the critical section
            }
            else
            {
               Float64 min, max;
               pForces->GetDeflLiveLoadDeflection( IProductForces::DeflectionLiveLoadEnvelope, poi, bat, &min, &max );

               min_defl = Min(min_defl, min);
               max_defl = Max(max_defl, max);
            }
         }

         artifact.SetDemand(min_defl,max_defl);
      }
      else
      {
         artifact.IsApplicable(false);
      }

      pGdrArtifact->AddDeflectionCheckArtifact(artifact);
   } // next span
}

void pgsDesigner2::CheckSegmentStability(const CSegmentKey& segmentKey,pgsSegmentStabilityArtifact* pArtifact) const
{
   ///////////////////////////////////////////////////////////////
   //
   // Check Girder Inclination (this really isn't a stability check, but it is related)
   //
   ///////////////////////////////////////////////////////////////
   GET_IFACE2(GetBroker(),ILibrary, pLib);
   GET_IFACE2(GetBroker(),ISpecification, pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(spec_name.c_str());
   bool bCheckInclindedGirder = pSpecEntry->GetGirderInclinationCriteria().bCheck;

   GET_IFACE2(GetBroker(),IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if ( pBridgeDesc->GetGirderOrientation() == pgsTypes::Plumb || !bCheckInclindedGirder )
   {
      pArtifact->SetGlobalGirderStabilityApplicability(false); // don't worry about this with a plumb girder
   }
   else
   {
      // We need to compute zo. The best way to do that is to delegate to the stability engineer.
      // We want zo for no overhangs. Do a dummy lifting stability analysis and get the zo result
      HANDLINGCONFIG config;
      config.bIgnoreGirderConfig = true; // ignore the girder configuration... we want the functions we call to use the handling configuration
      config.LeftOverhang = 0;
      config.RightOverhang = 0;

      GET_IFACE2(GetBroker(),ISegmentLiftingPointsOfInterest, pLiftingPoi);

      GET_IFACE2(GetBroker(),IGirder, pGirder);
      const WBFL::Stability::Girder* pStabilityModel = pGirder->GetSegmentLiftingStabilityModel(segmentKey);
      const WBFL::Stability::LiftingStabilityProblem* pStabilityProblem = pGirder->GetSegmentLiftingStabilityProblem(segmentKey,config, pLiftingPoi);

      WBFL::Stability::StabilityEngineer engineer;
      WBFL::Stability::LiftingResults liftingResults = engineer.AnalyzeLifting(pStabilityModel, pStabilityProblem);

      Float64 zo = liftingResults.Zo[+WBFL::Stability::ImpactDirection::NoImpact];

      GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | POI_ERECTED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 2);

      GET_IFACE2(GetBroker(),IIntervals, pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

      GET_IFACE2(GetBroker(),ISectionProperties, pSectProp);
      const pgsPointOfInterest& poi1(vPoi.front());
      Float64 Wbottom1 = pGirder->GetBottomWidth(poi1);
      Float64 Ybottom1 = pSectProp->GetY(releaseIntervalIdx, poi1, pgsTypes::BottomGirder);

      const pgsPointOfInterest& poi2(vPoi.back());
      Float64 Wbottom2 = pGirder->GetBottomWidth(poi2);
      Float64 Ybottom2 = pSectProp->GetY(releaseIntervalIdx, poi2, pgsTypes::BottomGirder);


      const CPrecastSegmentData* pSegment = pBridgeDesc->GetSegment(segmentKey);
      const CPierData2* pStartPier;
      const CTemporarySupportData* pStartTS;
      pSegment->GetSupport(pgsTypes::metStart, &pStartPier, &pStartTS);
      const CPierData2* pEndPier;
      const CTemporarySupportData* pEndTS;
      pSegment->GetSupport(pgsTypes::metEnd, &pEndPier, &pEndTS);
      Float64 startBrgPadWidth = Wbottom1 - WBFL::Units::ConvertToSysUnits(1.0, WBFL::Units::Measure::Inch); // dummy minimum value
      Float64 endBrgPadWidth = Wbottom2 - WBFL::Units::ConvertToSysUnits(1.0, WBFL::Units::Measure::Inch); // dummy minimum value
      if (pStartPier)
      {
         const CBearingData2* pBearingData = pIBridgeDesc->GetBearingData(pStartPier->GetIndex(), pgsTypes::Ahead, segmentKey.girderIndex);
         ATLASSERT(0 < pBearingData->BearingCount);
         startBrgPadWidth = (pBearingData->BearingCount - 1)*(pBearingData->Spacing) + pBearingData->Width;
      }

      if (pEndPier)
      {
         const CBearingData2* pBearingData = pIBridgeDesc->GetBearingData(pEndPier->GetIndex(), pgsTypes::Back, segmentKey.girderIndex);
         ATLASSERT(0 < pBearingData->BearingCount);
         endBrgPadWidth = (pBearingData->BearingCount - 1)*(pBearingData->Spacing) + pBearingData->Width;
      }

      Float64 FS = pSpecEntry->GetGirderInclinationCriteria().FS;

      Float64 orientation = fabs(pGirder->GetOrientation(segmentKey));
      pArtifact->SetGlobalGirderStabilityApplicability(true);
      pArtifact->SetTargetFactorOfSafety(FS);

      // check stability at start of girder
      pArtifact->SetGlobalGirderStabilityParameters(startBrgPadWidth,Ybottom1,orientation,zo);
      Float64 FS1 = pArtifact->GetFactorOfSafety();

      // check stability at end of girder
      pArtifact->SetGlobalGirderStabilityParameters(endBrgPadWidth,Ybottom2,orientation,zo);
      Float64 FS2 = pArtifact->GetFactorOfSafety();

      if ( FS1 < FS2 )
      {
         // start of girder is the worst case
         pArtifact->SetGlobalGirderStabilityParameters(startBrgPadWidth,Ybottom1,orientation,zo);
      }
   }
}

void pgsDesigner2::CheckConstructability(const CGirderKey& girderKey,pgsConstructabilityArtifact* pArtifact) const
{
   ASSERT_GIRDER_KEY(girderKey);

   GET_IFACE2(GetBroker(),ILibrary,pLib);
   GET_IFACE2(GetBroker(),ISpecification,pSpec);
   GET_IFACE2(GetBroker(),IBridge,pBridge);
   GET_IFACE2_NOCHECK(GetBroker(),IProductLoads,pProdLoads);

   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(spec_name.c_str());
   const auto& slab_offset_criteria = pSpecEntry->GetSlabOffsetCriteria();

   // min fillet is zero if girders are adjacently spaced.
   GET_IFACE2(GetBroker(),IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();
   bool isAdjacentSpacing = IsAdjacentSpacing(spacingType);

   GET_IFACE2_NOCHECK(GetBroker(),IGirderHaunch,pGdrHaunch);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();

   // we need to know if the stirrups engage the deck along the length of the girder
   // below we loop over all segments and do evaluation... we need to know stirrup engagement
   // before entering the loop.... figure it out here
   GET_IFACE2(GetBroker(),IStirrupGeometry,pStirrupGeometry);
   SegmentIndexType nSegments = pGirder->GetSegmentCount();
   bool bDoStirrupsEngageDeck = false;
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      CSegmentKey segmentKey(girderKey,segIdx);
      bDoStirrupsEngageDeck = pStirrupGeometry->DoStirrupsEngageDeck(segmentKey);
      if (bDoStirrupsEngageDeck)
      {
         break; // all we have to do is find one
      }
   }

   ///////////////////////////////////////////////////////////////
   //
   // Check Slab Offset ("A" Dimension)
   //
   ///////////////////////////////////////////////////////////////
   GET_IFACE2(GetBroker(),IEAFProgress,pProgress);
   WBFL::EAF::AutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Checking constructability requirements"));

   bool isHaunchCheck = pSpecEntry->GetSlabOffsetCriteria().bCheck;
   pgsTypes::HaunchInputDepthType haunchInputType = pBridge->GetHaunchInputDepthType();

#pragma Reminder("Assumes constant slab thickness throughout bridge")
   Float64 tSlab = pBridge->GetGrossSlabDepth(pgsPointOfInterest(CSegmentKey(0,0,0),0.0));

   // Constructability check is for all segments in a girder
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      // artifact for each span
      CSegmentKey segmentKey(girderKey,segIdx);
      pgsSegmentConstructabilityArtifact artifact(segmentKey);

      if (!isHaunchCheck)
      {
         artifact.SetSlabOffsetApplicability(false);
         artifact.SetFinishedElevationApplicability(false);
      }
      else
      {
         // If there is no deck, or haunch input is direct; slab offset is not applicable
         if (pBridge->GetDeckType() == pgsTypes::sdtNone)
         {
            artifact.SetSlabOffsetApplicability(false);
            artifact.SetFinishedElevationApplicability(true);

            // For no-deck bridges, check only at geometry control interval. This will need to be redefined when no-deck girders are added to PGSplice
            GET_IFACE2(GetBroker(),IIntervals,pIntervals);
            IntervalIndexType geomCtrlInterval = pIntervals->GetGeometryControlInterval();
            artifact.SetFinishedElevationControllingInterval(geomCtrlInterval);

            Float64 tolerance = slab_offset_criteria.FinishedElevationTolerance;
            artifact.SetFinishedElevationTolerance(tolerance);

            GET_IFACE2(GetBroker(),IPointOfInterest,pPoi);
            GET_IFACE2(GetBroker(),IRoadway,pAlignment);
            GET_IFACE2(GetBroker(),IDeformedGirderGeometry,pDeformedGirderGeometry);
            const int Left = 0;
            const int Center = 1;
            const int Right = 2;

            PoiList vPoi;
            pPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_TENTH_POINTS,&vPoi);
            ATLASSERT(vPoi.size() == 11);

            Float64 maxDiff = 0;
            for (const pgsPointOfInterest& poi : vPoi)
            {
               Float64 station,offset;
               pBridge->GetStationAndOffset(poi,&station,&offset);
               Float64 elev = pAlignment->GetElevation(station,offset);

               std::array<Float64,3> finished_elevation;
               pDeformedGirderGeometry->GetFinishedElevation(poi,nullptr,&finished_elevation[Left],&finished_elevation[Center],&finished_elevation[Right]);

               Float64 diff = fabs(finished_elevation[Center] - elev);

               if (maxDiff < diff)
               {
                  artifact.SetMaxFinishedElevation(station,offset,poi,elev,finished_elevation[Center]);
                  maxDiff = diff;
               }
            }
         }
         else if (pgsTypes::hidHaunchDirectly == haunchInputType || pgsTypes::hidHaunchPlusSlabDirectly == haunchInputType)
         {
            artifact.SetSlabOffsetApplicability(false);
            artifact.SetFinishedElevationApplicability(true);
            artifact.SetMinimumHaunchDepthApplicability(true);

            Float64 tolerance = slab_offset_criteria.FinishedElevationTolerance;
            artifact.SetFinishedElevationTolerance(tolerance);

            GET_IFACE2(GetBroker(),IPointOfInterest,pPoi);
            GET_IFACE2(GetBroker(),IRoadway,pAlignment);
            GET_IFACE2(GetBroker(),IIntervals,pIntervals);
            GET_IFACE2(GetBroker(),IDeformedGirderGeometry,pDeformedGirderGeometry);
            GET_IFACE2_NOCHECK(GetBroker(),ISectionProperties,pSectProps);

            // minimum fillet input requirements are applicable for this case
            if (isAdjacentSpacing)
            {
               // Min fillet is zero for adjacently spaced beams
               artifact.SetRequiredMinimumFillet(0.0);
            }
            else
            {
               Float64 min_fillet = pGirderEntry->GetMinFilletValue();
               artifact.SetRequiredMinimumFillet(min_fillet);
            }

            Float64 fillet = pBridge->GetFillet();
            artifact.SetProvidedFillet(fillet);

            // Finished elevation and minimum haunch depth checks
            artifact.SetMinimumAllowableHaunchDepth(fillet);

            // check at geometry control interval and user-specified intervals
            std::vector<IntervalIndexType> checkIntervals = pIntervals->GetSpecCheckGeometryControlIntervals();

            PoiList vPoi;
            pPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_TENTH_POINTS,&vPoi);
            pPoi->GetPointsOfInterest(segmentKey,POI_SPAN | POI_0L | POI_10L,&vPoi);
            pPoi->GetPointsOfInterest(segmentKey,POI_START_FACE | POI_END_FACE,&vPoi);
            // We can't compute accurate elevations within closure joints. Get rid of any pois off segments
            vPoi.erase(std::remove_if(vPoi.begin(),vPoi.end(),MatchPoiOffSegment(pPoi)),std::end(vPoi));
            pPoi->SortPoiList(&vPoi); // sorts and removes duplicates

            Float64 maxElevDiff = 0;
            Float64 minHaunchDepth = Float32_Max;

            for (auto interval : checkIntervals)
            {
               for (const pgsPointOfInterest& poi : vPoi)
               {
                  Float64 station,offset;
                  pBridge->GetStationAndOffset(poi,&station,&offset);
                  Float64 elev = pAlignment->GetElevation(station,offset);

                  Float64 lftHaunch,ctrHaunch,rgtHaunch;
                  Float64 finished_elevation = pDeformedGirderGeometry->GetFinishedElevation(poi,interval,&lftHaunch,&ctrHaunch,&rgtHaunch);

                  Float64 diff = fabs(finished_elevation - elev);

                  if (maxElevDiff < diff)
                  {
                     artifact.SetMaxFinishedElevation(station,offset,poi,elev,finished_elevation);
                     artifact.SetFinishedElevationControllingInterval(interval);
                     maxElevDiff = diff;
                  }

                  // Check min haunch depth at edges of top flange against fillet requirements
                  Float64 minDepth = min(lftHaunch,rgtHaunch);
                  if (minDepth < minHaunchDepth)
                  {
                     artifact.SetMinimumHaunchDepth(station,offset,poi,minDepth);
                     artifact.SetMinimumHaunchDepthControllingInterval(interval);
                     minHaunchDepth = minDepth;
                  }
               }
            }
         }
         else
         {
            artifact.SetSlabOffsetApplicability(true);
            artifact.SetFinishedElevationApplicability(false);

            //  provided slab offsets
            std::array<Float64,2> slabOffset;
            pBridge->GetSlabOffset(segmentKey,&slabOffset[pgsTypes::metStart],&slabOffset[pgsTypes::metEnd]);

            artifact.SetProvidedSlabOffset(slabOffset[pgsTypes::metStart],slabOffset[pgsTypes::metEnd]);

            // get required slab offset
            Float64 requiredSlabOffset = pGdrHaunch->GetRequiredSlabOffset(segmentKey);
            artifact.SetRequiredSlabOffset(requiredSlabOffset);

            const auto& slab_offset_details = pGdrHaunch->GetSlabOffsetDetails(segmentKey);

            // Get least haunch depth and its location along girder
            Float64 minval(Float64_Max);
            Float64 minloc;
            for (const auto& slab_offset : slab_offset_details.SlabOffset)
            {
               Float64 val = slab_offset.TopSlabToTopGirder - slab_offset.tSlab - slab_offset.GirderOrientationEffect;
               if (val < minval)
               {
                  minval = val;
                  minloc = slab_offset.PointOfInterest.GetDistFromStart();
               }
            }

            artifact.SetLeastHaunchDepth(minloc,minval);

            // minimum fillet requirements
            if (isAdjacentSpacing)
            {
               // Min fillet is zero for adjacently spaced beams
               artifact.SetRequiredMinimumFillet(0.0);
            }
            else
            {
               Float64 min_fillet = pGirderEntry->GetMinFilletValue();
               artifact.SetRequiredMinimumFillet(min_fillet);
            }

            Float64 fillet = pBridge->GetFillet();
            artifact.SetProvidedFillet(fillet);

            // warning tolerance for excessive haunch
            Float64 warn_tol = pGirderEntry->GetExcessiveSlabOffsetWarningTolerance();
            artifact.SetExcessSlabOffsetWarningTolerance(warn_tol);

            // determine if stirrup lengths could be a problem

         // warn of possible stirrup length issue if the difference in haunch depth along the girder is more than half the deck thickness"
            artifact.CheckStirrupLength(bDoStirrupsEngageDeck && tSlab / 2 < fabs(slab_offset_details.HaunchDiff));
         }

         // warning tolerance for excessive haunch
         Float64 warn_tol = pGirderEntry->GetExcessiveSlabOffsetWarningTolerance();
         artifact.SetExcessSlabOffsetWarningTolerance(warn_tol);

         ///////////////////////////////////////////////////////////////
         //
         // Camber Tolerance for Haunch 
         //
         ///////////////////////////////////////////////////////////////
         if (!pSpec->IsAssumedExcessCamberInputEnabled())
         {
            artifact.SetHaunchGeometryCheckApplicability(false);
         }
         else
         {
            artifact.SetHaunchGeometryCheckApplicability(true);

            const auto& haunch_criteria = pSpecEntry->GetHaunchCriteria();
            artifact.SetHaunchGeometryTolerance(haunch_criteria.HaunchLoadCamberTolerance);

            Float64 assumedExcessCamber = pBridge->GetAssumedExcessCamber(segmentKey.groupIndex,segmentKey.girderIndex);
            artifact.SetAssumedExcessCamber(assumedExcessCamber);

            GET_IFACE2(GetBroker(),IGirderHaunch,pGdrHaunch);
            const auto& slab_offset_details = pGdrHaunch->GetSlabOffsetDetails(segmentKey);

            // Need excess camber at mid-span - get details there
            ATLASSERT(std::is_sorted(std::begin(slab_offset_details.SlabOffset),std::end(slab_offset_details.SlabOffset),[](const auto& a,const auto& b) {return a.PointOfInterest < b.PointOfInterest; }));
            // search only the middle of the container
            auto nItems = slab_offset_details.SlabOffset.size();
            auto begin_search = std::begin(slab_offset_details.SlabOffset);
            std::advance(begin_search,nItems / 2 - 1);
            auto end_search = begin_search;
            std::advance(end_search,2);

            auto find_midspan_poi = [](const auto& a) {return a.PointOfInterest.IsMidSpan(POI_ERECTED_SEGMENT); }; // named lamda express for searching

            auto hit(std::find_if(begin_search,end_search,find_midspan_poi));
            if (hit == end_search)
            {
               // not found, search the entire container
               ATLASSERT(false); // it is ok that we get here... the assert is to let us know that the above quicker search
               // didn't work... if we get here a lot, there is probably something wrong with the strategy above
               hit = std::find_if(std::begin(slab_offset_details.SlabOffset),std::end(slab_offset_details.SlabOffset),find_midspan_poi);
            }

            if (hit != std::end(slab_offset_details.SlabOffset))
            {
               Float64 haunch_depth = hit->CamberEffect;
               artifact.SetComputedExcessCamber(haunch_depth);
            }
            else
            {
               ATLASSERT(false); // THIS IS A BIG DEAL!! Can't find mid-span details. Cannot perform check. 
               // Should never happen, but kill check to avoid later crash
               artifact.SetHaunchGeometryCheckApplicability(false);
            }
            // minimum assumed haunch depth
            std::vector<SlabLoad> slab_loads;
            CSegmentKey segKey(girderKey.groupIndex,girderKey.girderIndex,0);
            pProdLoads->GetMainSpanSlabLoad(segKey,&slab_loads);

            Float64 minDepth = Float64_Max;
            for (const auto& sload : slab_loads)
            {
               minDepth = min(minDepth,sload.HaunchDepth);
            }

            minDepth = IsZero(minDepth) ? 0.0 : minDepth; // tolerancing

            artifact.SetAssumedMinimumHaunchDepth(minDepth);
         }
      }
      ///////////////////////////////////////////////////////////////
      // Check Precamber (if applicable)
      ///////////////////////////////////////////////////////////////
      if (pGirderEntry->CanPrecamber())
      {
         GET_IFACE2(GetBroker(),IGirder,pIGirder);
         artifact.SetPrecamberApplicability(true);
         SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
         for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
         {
            CSegmentKey segmentKey(girderKey,segIdx);
            Float64 L = pBridge->GetSegmentLength(segmentKey);
            Float64 N = pGirderEntry->GetPrecamberLimit();
            Float64 limit = IsZero(N) ? 0 : L / N;
            artifact.SetPrecamber(segmentKey,limit,pIGirder->GetPrecamber(segmentKey));
         }
      }

      ///////////////////////////////////////////////////////////////
      //
      // Check Bottom Flange Clearance
      //
      ///////////////////////////////////////////////////////////////
      pgsTypes::SupportedBeamSpacing spacingType = pIBridgeDesc->GetGirderSpacingType();
      const auto& bottom_flange_clearance_criteria = pSpecEntry->GetBottomFlangeClearanceCriteria();
      if (bottom_flange_clearance_criteria.bCheck && ::IsGirderSpacing(spacingType))
      {
         artifact.SetBottomFlangeClearanceApplicability(true);

         GET_IFACE2(GetBroker(),IPointOfInterest,pPoi);

         PoiList vPoi;
         pPoi->GetPointsOfInterest(segmentKey,POI_START_FACE | POI_END_FACE,&vPoi);
         ATLASSERT(vPoi.size() == 2);

         Float64 CleftStart,CrightStart;
         pBridge->GetBottomFlangeClearance(vPoi.front(),&CleftStart,&CrightStart);

         Float64 CleftEnd,CrightEnd;
         pBridge->GetBottomFlangeClearance(vPoi.back(),&CleftEnd,&CrightEnd);

         Float64 Cleft = Min(CleftStart,CleftEnd);
         Float64 Cright = Min(CrightStart,CrightEnd);

         Float64 CthisSegment = 0;
         if (0 < Cleft && 0 < Cright)
         {
            CthisSegment = Min(Cleft,Cright);
         }
         else if (Cleft < 0 && 0 < Cright)
         {
            CthisSegment = Cright;
         }
         else if (0 < Cleft && Cright < 0)
         {
            CthisSegment = Cleft;
         }
         else
         {
            // Cleft and Cright < 0... this is a single girder bridges
            artifact.SetBottomFlangeClearanceApplicability(false); // not applicable
         }

         Float64 Cmin = bottom_flange_clearance_criteria.MinClearance;
         artifact.SetBottomFlangeClearanceParameters(CthisSegment,Cmin);
      }
      else
      {
         artifact.SetBottomFlangeClearanceApplicability(false);
      }

      pArtifact->AddSegmentArtifact(artifact);
   }

   // Along entire girderline
   // Check minimum haunch depth requirements at bearing centerlines if appropriate
   Float64 min_haunch;
   if (!pGirderEntry->GetMinHaunchAtBearingLines(&min_haunch))
   {
      pArtifact->SetHaunchBearingCLApplicability(pgsConstructabilityArtifact::hbcAppNA);
   }
   else
   {

      pArtifact->SetHaunchBearingCLApplicability(isHaunchCheck ?
         pgsConstructabilityArtifact::hbcAppYes :
         pgsConstructabilityArtifact::hbcAppNAPrintOnly);

      pArtifact->SetRequiredHaunchAtBearingCLs(min_haunch);

      if (pgsTypes::hidHaunchDirectly == haunchInputType || pgsTypes::hidHaunchPlusSlabDirectly == haunchInputType)
      {
         GET_IFACE2_NOCHECK(GetBroker(),ISectionProperties,pSectProps);
         GET_IFACE2(GetBroker(),IPointOfInterest,pPoi);

         // Haunch depth at start end bearing
         CSegmentKey startSegmentKey(girderKey,0);

         PoiList vPoi;
         pPoi->GetPointsOfInterest(startSegmentKey,POI_ERECTED_SEGMENT | POI_0L ,&vPoi);
         ATLASSERT(vPoi.size() == 1);
         Float64 haunchstrt = pSectProps->GetStructuralHaunchDepth(vPoi.front(),pgsTypes::hspDetailedDescription);

         CSegmentKey endSegmentKey(girderKey, nSegments-1);
         vPoi.clear();
         pPoi->GetPointsOfInterest(endSegmentKey,POI_ERECTED_SEGMENT | POI_10L,&vPoi);
         ATLASSERT(vPoi.size() == 1);
         Float64 haunchend = pSectProps->GetStructuralHaunchDepth(vPoi.back(),pgsTypes::hspDetailedDescription);

         pArtifact->SetProvidedHaunchAtBearingCLs(haunchstrt, haunchend);
      }
      else
      {
         ATLASSERT(nSegments == 1); // pgsuper
         CSegmentKey segmentKey(girderKey,0);

         Float64 haunchstrt,haunchend;
         pBridge->GetSlabOffset(segmentKey,&haunchstrt,&haunchend);

         haunchstrt -= tSlab;
         haunchend  -= tSlab;
         pArtifact->SetProvidedHaunchAtBearingCLs(haunchstrt,haunchend);
      }
   }
}


void pgsDesigner2::CheckDebonding(const CSegmentKey& segmentKey, pgsDebondArtifact* pArtifact) const
{
   GET_IFACE2(GetBroker(),IStrandGeometry, pStrandGeometry);
   GET_IFACE2(GetBroker(),IBridgeDescription, pIBridgeDesc);

   // Get total number of straight strands below half height. Never include harped strands in count
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   pgsTypes::AdjustableStrandType adjustable_strand_type = pSegment->Strands.GetAdjustableStrandType();
   pgsTypes::StrandType strand_type = adjustable_strand_type == pgsTypes::asHarped ? pgsTypes::Straight : pgsTypes::Permanent;

   StrandIndexType nDebonded = pStrandGeometry->GetNumDebondedStrands(segmentKey, strand_type, pgsTypes::dbetEither);
   pArtifact->SetNumDebondedStrands(nDebonded);

   if (nDebonded == 0)
   {
      return;
   }

   GET_IFACE2(GetBroker(),ISegmentData, pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
   CComPtr<IIndexArray> arrayPermStrandIndex;
   if (IsGridBasedStrandModel(pStrands->GetStrandDefinitionType()))
   {
      pStrandGeometry->ComputePermanentStrandIndices(segmentKey, strand_type, &arrayPermStrandIndex);
   }

   GET_IFACE2(GetBroker(),IBridge, pBridge);
   GET_IFACE2(GetBroker(),IDebondLimits, pDebondLimits);
   GET_IFACE2(GetBroker(),IEAFProgress, pProgress);
   GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);
   GET_IFACE2(GetBroker(),IGirder, pGirder);

   WBFL::EAF::AutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Checking debonding requirements"));

   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_RELEASED_SEGMENT | POI_0L | POI_10L, &vPoi);
   ATLASSERT(vPoi.size() == 2);
   std::array<pgsPointOfInterest, 2> poi{ vPoi.front(),vPoi.back() };

   StrandIndexType nStrands = pStrandGeometry->GetStrandCount(segmentKey, strand_type);
   StrandIndexType nPermStrands = pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Permanent);

   // +--------------+--------+------------------+-------------+
   // | Beam         | # Webs | # Bottom Flanges | Restriction |
   // +--------------+--------+------------------+-------------+
   // | I-Beams      |    1   |        1         |     I       |
   // | U-Beams      |    2   |        1         |     J       |
   // | Voided Slabs |    0   |        0         |     K       |
   // | Double Tee   |    2   |        0         |     K       |
   // +--------------+--------+------------------+-------------+
   WebIndexType nWebs = pGirder->GetWebCount(segmentKey);
   FlangeIndexType nFlanges = pGirder->GetBottomFlangeCount(segmentKey);
   if (nWebs == 1 && nFlanges == 1)
   {
      // single-web flanged sections
      // (I-beams, bulb-tees, and inverted-tees)
      // Restriction I applies
      pArtifact->SetSection(pgsDebondArtifact::I);
   }
   else if (1 < nWebs && nFlanges == 1)
   {
      // multi-web sections having bottom flanges
      // (voided slab, box beams, and U-beams)
      // Restriction J applies
      pArtifact->SetSection(pgsDebondArtifact::J);
   }
   else
   {
      // all other sections
      // Restriction K applies
      ATLASSERT(0 <= nWebs && nFlanges == 0);
      pArtifact->SetSection(pgsDebondArtifact::K);
   }

   GET_IFACE2(GetBroker(), IMaterials, pMaterials);
   const WBFL::Materials::PsStrand* pStrand = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::Straight);

   StrandIndexType nMax10orLess, nMax, nMax07;
   bool bCheckMaxPerSection;
   Float64 fraMaxPerSection;
   pDebondLimits->GetMaxDebondedStrandsPerSection(segmentKey, &nMax10orLess, &nMax, &nMax07, &bCheckMaxPerSection, &fraMaxPerSection);
   pArtifact->AddMaxDebondStrandsAtSection(nDebonded <= 10 ? nMax10orLess : (pStrand->GetSize() == WBFL::Materials::PsStrand::Size::D1778 ? nMax07 : nMax), bCheckMaxPerSection, fraMaxPerSection);

   Float64 maxFraPerRow = pDebondLimits->GetMaxDebondedStrandsPerRow(segmentKey);

   // Only straight strands can be debonded
   // Total number of debonded strands
   pArtifact->CheckMaxFraDebondedStrands(pDebondLimits->CheckMaxDebondedStrands(segmentKey));
   Float64 fraDebonded = (nStrands == 0 ? 0 : (Float64)nDebonded / (Float64)nPermStrands);
   pArtifact->SetFraDebondedStrands(fraDebonded);
   if (pArtifact->CheckMaxFraDebondedStrands())
   {
      Float64 maxFra = pDebondLimits->GetMaxDebondedStrands(segmentKey);
      pArtifact->SetMaxFraDebondedStrands(maxFra);
   }

   // If adjustable strands are straight, we will need to match up rows in order to get a total for each row
   bool bCheckAdjustableStrands(false);
   if (adjustable_strand_type != pgsTypes::asHarped)
   {
      if (0 < pStrandGeometry->GetStrandCount(segmentKey, pgsTypes::Harped))
      {
         bCheckAdjustableStrands = true;
      }
   }

   // Number of debonded strands in row
   RowIndexType nRows = pStrandGeometry->GetNumRowsWithStrand(poi[pgsTypes::metStart], pgsTypes::Straight);
   for (RowIndexType rowIdx = 0; rowIdx < nRows; rowIdx++)
   {
      StrandIndexType nStrandsInRow = pStrandGeometry->GetNumStrandInRow(poi[pgsTypes::metStart], rowIdx, pgsTypes::Straight);

      if (bCheckAdjustableStrands)
      {
         // In order to determine all strands in row - we need to add in any adjustable straight stands that might be in this row.
         Float64 row_elev = pStrandGeometry->GetUnadjustedStrandRowElevation(poi[pgsTypes::metStart], rowIdx, pgsTypes::Straight);

         RowIndexType nAdjustableStrandRows = pStrandGeometry->GetNumRowsWithStrand(poi[pgsTypes::metStart], pgsTypes::Harped);
         for (RowIndexType adjustableStrandRowIdx = 0; adjustableStrandRowIdx < nAdjustableStrandRows; adjustableStrandRowIdx++)
         {
            Float64 adjustable_strand_row_elev = pStrandGeometry->GetUnadjustedStrandRowElevation(poi[pgsTypes::metStart], adjustableStrandRowIdx, pgsTypes::Harped);

            if (IsEqual(adjustable_strand_row_elev, row_elev, gs_rowToler))
            {
               StrandIndexType nAdjustableStrandsInRow = pStrandGeometry->GetNumStrandInRow(poi[pgsTypes::metStart], adjustableStrandRowIdx, pgsTypes::Harped);
               nStrandsInRow += nAdjustableStrandsInRow;
               break;
            }
         }
      }

      StrandIndexType nDebondStrandsInRow = pStrandGeometry->GetNumDebondedStrandsInRow(poi[pgsTypes::metStart], rowIdx, pgsTypes::Straight);
      Float64 fra = (nStrandsInRow == 0 ? 0 : (Float64)nDebondStrandsInRow / (Float64)nStrandsInRow);
      pArtifact->AddNumStrandsInRow(nStrandsInRow);
      pArtifact->AddNumDebondedStrandsInRow(nDebondStrandsInRow);
      pArtifact->AddFraDebondedStrandsInRow(fra);
      pArtifact->AddMaxFraDebondedStrandsInRow(maxFraPerRow);

      // LRFD 9th Edition, 5.9.4.3.3, Restriction I, J, K (exterior strands in row must be debonded)
      if (pDebondLimits->IsExteriorStrandBondingRequiredInRow(segmentKey, pgsTypes::metStart, rowIdx))
      {
         if (pArtifact->GetSection() == pgsDebondArtifact::K && WBFL::LRFD::BDSManager::GetEdition() <= WBFL::LRFD::BDSManager::Edition::NinthEdition2020)
         {
            // this is a multi-web, no flange section - LRFD 9th Edition, 5.9.4.3.3 Requirement K applies. Exterior strands in each web need to be bonded
            if (nWebs == 0)
            {
               // this is a solid slab
               bool bIsExteriorStrandDebonded = pStrandGeometry->IsExteriorStrandDebondedInRow(poi[pgsTypes::metStart], rowIdx, pgsTypes::Straight);
               pArtifact->SetExtriorStrandBondState(rowIdx, bIsExteriorStrandDebonded ? pgsDebondArtifact::Debonded : pgsDebondArtifact::Bonded);
            }
            else
            {
               for (WebIndexType webIdx = 0; webIdx < nWebs; webIdx++)
               {
                  bool bIsExteriorStrandDebonded = pStrandGeometry->IsExteriorWebStrandDebondedInRow(poi[pgsTypes::metStart], webIdx, rowIdx, pgsTypes::Straight);
                  pArtifact->SetExtriorStrandBondState(rowIdx, bIsExteriorStrandDebonded ? pgsDebondArtifact::Debonded : pgsDebondArtifact::Bonded, webIdx);
               }
            }
         }
         else
         {
            bool bIsExteriorStrandDebonded = pStrandGeometry->IsExteriorStrandDebondedInRow(poi[pgsTypes::metStart], rowIdx, pgsTypes::Straight);
            pArtifact->SetExtriorStrandBondState(rowIdx, bIsExteriorStrandDebonded ? pgsDebondArtifact::Debonded : pgsDebondArtifact::Bonded);
         }
      }
      else
      {
         // exterior strands are not required to be bonded in this row
         // this requirement was first introduced in LRFD 9th edition for I-Beams (5.9.4.3.3 Restriction I)
         ATLASSERT(WBFL::LRFD::BDSManager::Edition::NinthEdition2020 <= WBFL::LRFD::BDSManager::GetEdition());
         pArtifact->SetExtriorStrandBondState(rowIdx, pgsDebondArtifact::None);
      }
   }

   // Number of debonded strands at a section and section lengths
   Float64 L = pBridge->GetSegmentLength(segmentKey);
   Float64 L2 = L / 2.0;

   std::array<Float64, 2> lmin_section{ Float64_Max,Float64_Max };
   Float64 lmax_debond_length = 0.0;

   // left end
   StrandIndexType nDebondedEnd = pStrandGeometry->GetNumDebondedStrands(segmentKey, pgsTypes::Straight, pgsTypes::dbetStart);
   Float64 prev_location = 0.0;
   SectionIndexType nSections = pStrandGeometry->GetNumDebondSections(segmentKey, pgsTypes::metStart, pgsTypes::Straight);
   for (SectionIndexType sectionIdx = 0; sectionIdx < nSections; sectionIdx++)
   {
      StrandIndexType nDebondedStrands = pStrandGeometry->GetNumDebondedStrandsAtSection(segmentKey, pgsTypes::metStart, sectionIdx, pgsTypes::Straight);
      Float64 fraDebondedStrands = (nDebondedEnd == 0 ? 0 : (Float64)nDebondedStrands / (Float64)nDebondedEnd);
      Float64 location = pStrandGeometry->GetDebondSection(segmentKey, pgsTypes::metStart, sectionIdx, pgsTypes::Straight);
      pArtifact->AddDebondSection(location, nDebondedStrands, fraDebondedStrands);

      if (location < 0 || L2 < location)
      {
         ATLASSERT(false);
         continue; // bond occurs after mid-girder... skip this one
      }

      Float64 section_len = fabs(location - prev_location);
      lmin_section[pgsTypes::metStart] = Min(lmin_section[pgsTypes::metStart], section_len);

      lmax_debond_length = Max(lmax_debond_length, location);

      prev_location = location;
   }

   // right end
   nDebondedEnd = pStrandGeometry->GetNumDebondedStrands(segmentKey, pgsTypes::Straight, pgsTypes::dbetEnd);
   nSections = pStrandGeometry->GetNumDebondSections(segmentKey, pgsTypes::metEnd, pgsTypes::Straight);
   for (SectionIndexType sectionIdx = 0; sectionIdx < nSections; sectionIdx++)
   {
      StrandIndexType nDebondedStrands = pStrandGeometry->GetNumDebondedStrandsAtSection(segmentKey, pgsTypes::metEnd, sectionIdx, pgsTypes::Straight);
      Float64 fraDebondedStrands = (nDebondedEnd == 0 ? 0 : (Float64)nDebondedStrands / (Float64)nDebondedEnd);
      Float64 location = pStrandGeometry->GetDebondSection(segmentKey, pgsTypes::metEnd, sectionIdx, pgsTypes::Straight);
      pArtifact->AddDebondSection(location, nDebondedStrands, fraDebondedStrands);

      if (location < L2 || L < location)
      {
         ATLASSERT(false);
         continue; // bond occurs after the end of the girder... skip this one
      }

      // on right end, the first section is the right-most working toward mid-girder
      Float64 section_len;
      if (sectionIdx == 0)
      {
         section_len = L - location;
      }
      else
      {
         section_len = fabs(location - prev_location);
      }

      lmin_section[pgsTypes::metEnd] = Min(lmin_section[pgsTypes::metEnd], section_len);


      Float64 debond_length = L - location;
      lmax_debond_length = Max(lmax_debond_length, debond_length);

      prev_location = location;
   }

   pArtifact->SetMinDebondSectionSpacing(Min(lmin_section[pgsTypes::metStart], lmin_section[pgsTypes::metEnd]));

   Float64 dll;
   pgsTypes::DebondLengthControl control;
   pDebondLimits->GetMaxDebondLength(segmentKey, &dll, &control);

   pArtifact->SetMaxDebondLength(lmax_debond_length);
   pArtifact->SetDebondLengthLimit(dll, control);

   // Restriction C
   Float64 dds = pDebondLimits->GetMinDistanceBetweenDebondSections(segmentKey);
   pArtifact->SetDebondSectionSpacingLimit(dds);

   // LRFD 5.9.4.3.3, 9th Edition, Restriction D
   // If one of the built-in strand models are used, strands are forced to be symmetric and satisfy the requirement
   // Symmetry is not enforced for direct strand input and it is not checked
   pArtifact->CheckDebondingSymmetry(pDebondLimits->CheckDebondingSymmetry(segmentKey)); // Is checking symmetry enabled?
   pArtifact->IsDebondingSymmetrical(pSegment->Strands.GetStrandDefinitionType() == pgsTypes::sdtDirectStrandInput ? DEBOND_SYMMETRY_NA : DEBOND_SYMMETRY_TRUE); // the result is either NA or TRUE

   // LRFD 5.9.4.3.3, 9th Edition, Restriction E
   if (pDebondLimits->CheckAdjacentDebonding(segmentKey))
   {
      pArtifact->CheckAdjacentDebonding(true);
      std::vector<RowIndexType> vRowsWithDebonding = pStrandGeometry->GetRowsWithDebonding(segmentKey, pgsTypes::Straight);
      if (1 < vRowsWithDebonding.size())
      {
         // loop over all the rows with debonding, working two rows at a time
         auto iter = vRowsWithDebonding.begin() + 1;
         auto end = vRowsWithDebonding.end();
         for (; iter != end; iter++)
         {
            RowIndexType prevRowIdx = *(iter - 1);  // row i-1
            RowIndexType thisRowIdx = *iter;        // row i

            bool bCompareVertically = true;
            if (prevRowIdx + 1 != thisRowIdx)
            {
               // there is a row with all bonded strands between rows with debonded strands
               // rows with debonding are not adjacent so don't compare vertically
               bCompareVertically = false;
            }


            // create records for each strand, left to right
            // the record is the horizontal position, the debonding state and strand index of the strand at this position on the previous row,
            // and the debonding state and strand index of the strand at this position in this row
            const int debonded = 0; // stand is debonded
            const int bonded = 1; // strand is bonded
            const int none = -1; // there isn't a strand vertically above or below the strand a this position

            for (int i = 0; i < 2; i++)
            {
               pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;

               std::map<Float64, std::tuple<StrandIndexType, int, StrandIndexType, int>> debondRecords; // key = horizontal position, value = strand index and debond state in prev row, strand index and debond state in this row

               // evaluate previous row
               std::vector<StrandIndexType> vStrandsPrevRow = pStrandGeometry->GetStrandsInRow(poi[endType], prevRowIdx, pgsTypes::Straight);
               for (auto strandIdx : vStrandsPrevRow)
               {
                  CComPtr<IPoint2d> pnt;
                  pStrandGeometry->GetStrandPosition(poi[endType], strandIdx, pgsTypes::Straight, &pnt);
                  Float64 x;
                  pnt->get_X(&x);
                  bool bIsDebonded = pStrandGeometry->IsStrandDebonded(poi[endType], strandIdx, pgsTypes::Straight);
                  debondRecords.emplace(x, std::make_tuple(strandIdx, bIsDebonded ? debonded : bonded, INVALID_INDEX, none));
               }

               // evaluate current row
               std::vector<StrandIndexType> vStrandsThisRow = pStrandGeometry->GetStrandsInRow(poi[endType], thisRowIdx, pgsTypes::Straight);
               for (auto strandIdx : vStrandsThisRow)
               {
                  CComPtr<IPoint2d> pnt;
                  pStrandGeometry->GetStrandPosition(poi[endType], strandIdx, pgsTypes::Straight, &pnt);
                  Float64 x;
                  pnt->get_X(&x);
                  bool bIsDebonded = pStrandGeometry->IsStrandDebonded(poi[endType], strandIdx, pgsTypes::Straight);

                  auto found = debondRecords.find(x);
                  if (found != debondRecords.end())
                  {
                     // this is a strand at this position in the previous row... update the strand record
                     std::get<2>(found->second) = strandIdx;
                     std::get<3>(found->second) = bIsDebonded ? debonded : bonded;
                  }
                  else
                  {
                     // this is not a strand at this position in the previous row... create a new strand record
                     debondRecords.emplace(x, std::make_tuple(INVALID_INDEX, none, strandIdx, bIsDebonded ? debonded : bonded));
                  }
               }

               // now that we have the strand records, analyze them to make sure adjacent strands are not debonded
               auto iter = debondRecords.begin();
               std::pair<Float64, std::tuple<StrandIndexType, int, StrandIndexType, int>> prevItem = *iter;
               iter++;
               auto end = debondRecords.end();
               for (; iter != end; iter++)
               {
                  std::pair<Float64, std::tuple<StrandIndexType, int, StrandIndexType, int>> thisItem = *iter;
                  if (bCompareVertically && (std::get<1>(prevItem.second) == debonded && std::get<3>(prevItem.second) == debonded))
                  {
                     // adjacent rows are being compared for vertical adjacency and the strands in both rows are debonded
                     StrandIndexType permStrandIdx1(std::get<0>(prevItem.second)), permStrandIdx2(std::get<2>(prevItem.second));
                     if (arrayPermStrandIndex)
                     {
                        arrayPermStrandIndex->get_Item(std::get<0>(prevItem.second), &permStrandIdx1);
                        arrayPermStrandIndex->get_Item(std::get<2>(prevItem.second), &permStrandIdx2);
                     }
                     pArtifact->AddAdjacentDebondedStrands(endType, permStrandIdx1, permStrandIdx2);
                  }

                  if (std::get<1>(prevItem.second) == debonded && std::get<1>(thisItem.second) == debonded)
                  {
                     // adjacent strands horizontally are debonded
                     StrandIndexType permStrandIdx1(std::get<0>(prevItem.second)), permStrandIdx2(std::get<0>(thisItem.second));
                     if (arrayPermStrandIndex)
                     {
                        arrayPermStrandIndex->get_Item(std::get<0>(prevItem.second), &permStrandIdx1);
                        arrayPermStrandIndex->get_Item(std::get<0>(thisItem.second), &permStrandIdx2);
                     }
                     pArtifact->AddAdjacentDebondedStrands(endType, permStrandIdx1, permStrandIdx2);
                  }

                  prevItem = thisItem;
               }
            }
         }
      }
      else
      {
         // zero or one row have debonding.... if it's zero the loop doesn't do anything
         // if there is one, the we are just checking if horizontally adjacent strands are debonded
         for (int i = 0; i < 2; i++)
         {
            pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
            for (auto rowIdx : vRowsWithDebonding)
            {
               std::vector<StrandIndexType> vStrandsThisRow = pStrandGeometry->GetStrandsInRow(poi[endType], rowIdx, pgsTypes::Straight);

               // sort the indexes based on the x-position of the strand, sorting left to right (need to compare adjacent strands, not adjacent strand indices)
               std::sort(std::begin(vStrandsThisRow), std::end(vStrandsThisRow), [poi, endType, pStrandGeometry](auto strandIdx1, auto strandIdx2) {CComPtr<IPoint2d> pnt1, pnt2;
               pStrandGeometry->GetStrandPosition(poi[endType], strandIdx1, pgsTypes::Straight, &pnt1);
               pStrandGeometry->GetStrandPosition(poi[endType], strandIdx2, pgsTypes::Straight, &pnt2);
               Float64 x1, x2;
               pnt1->get_X(&x1);
               pnt2->get_X(&x2);
               return x1 < x2; }
               );

#if defined _DEBUG
               Float64 X = -Float64_Max; // make sure strand indices represent strands left to right in order
#endif

               StrandIndexType prevStrandIdx = INVALID_INDEX;
               bool bWasPreviousStrandDebonded = false;
               for (auto strandIdx : vStrandsThisRow)
               {
#if defined _DEBUG
                  CComPtr<IPoint2d> pnt;
                  pStrandGeometry->GetStrandPosition(poi[endType], strandIdx, pgsTypes::Straight, &pnt);
                  Float64 x;
                  pnt->get_X(&x);
                  ATLASSERT(X < x); // make sure strands are ordered left to right
                  X = x;
#endif

                  bool bIsDebonded = pStrandGeometry->IsStrandDebonded(poi[endType], strandIdx, pgsTypes::Straight);
                  if (bIsDebonded && bWasPreviousStrandDebonded)
                  {
                     StrandIndexType permStrandIdx1(prevStrandIdx), permStrandIdx2(strandIdx);
                     if (arrayPermStrandIndex)
                     {
                        arrayPermStrandIndex->get_Item(prevStrandIdx, &permStrandIdx1);
                        arrayPermStrandIndex->get_Item(strandIdx, &permStrandIdx2);
                     }
                     pArtifact->AddAdjacentDebondedStrands(endType, permStrandIdx1, permStrandIdx2);
                  }
                  bWasPreviousStrandDebonded = bIsDebonded;
                  prevStrandIdx = strandIdx;
               }
            }
         }
      }
   }
   else
   {
      pArtifact->CheckAdjacentDebonding(false);
   }


   if(pDebondLimits->CheckDebondingInWebWidthProjections(segmentKey))
   {
      pArtifact->CheckDebondingInWebWidthProjection(true); // need to check with Spec/Girder for this
      IndexType nRegions = 0;
      std::vector<RowIndexType> vRowsWithDebonding = pStrandGeometry->GetRowsWithDebonding(segmentKey, pgsTypes::Straight);

      // get strand diameter/radius so we can create a bounding box for a strand point
      GET_IFACE2(GetBroker(),IMaterials, pMaterials);
      const auto* pStrand = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::Straight);
      Float64 d_strand = pStrand->GetNominalDiameter();
      Float64 r_strand = 0.5*d_strand;

      CComPtr<IRect2d> strandRect;
      strandRect.CoCreateInstance(CLSID_Rect2d);
      for (int i = 0; i < 2; i++)
      {
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
         Float64 fra, ratio;
         std::vector<CComPtr<IRect2d>> vRegions = pStrandGeometry->GetWebWidthProjectionsForDebonding(segmentKey, endType, &fra, &ratio);
         if (pArtifact->GetSection() == pgsDebondArtifact::I)
         {
            // only applicable to Requirement I cross sections
            ATLASSERT(IsEqual(fraDebonded, fra));
            pArtifact->SetBottomFlangeToWebWidthRatio(endType, ratio);
         }

         auto n = vRegions.size();
         if (0 < n)
         {
            nRegions += n;

            for (auto rowIdx : vRowsWithDebonding)
            {
               std::vector<StrandIndexType> vStrandsThisRow = pStrandGeometry->GetStrandsInRow(poi[endType], rowIdx, pgsTypes::Straight);
               for (auto strandIdx : vStrandsThisRow)
               {
                  // we are checking to see if there are debonded strands in the web width projection region... no need to check bonded strands
                  if (pStrandGeometry->IsStrandDebonded(poi[endType], strandIdx, pgsTypes::Straight))
                  {
                     // strand is debonded... get its location
                     CComPtr<IPoint2d> pnt;
                     pStrandGeometry->GetStrandPosition(poi[endType], strandIdx, pgsTypes::Straight, &pnt);

                     Float64 x, y;
                     pnt->Location(&x, &y);

                     // the entire strand must be contained within the region... set the strand rectangle so cover
                     // the strand (this is the strand's bounding box)
                     strandRect->SetBounds(x - r_strand, x + r_strand, y - r_strand, y + r_strand);

                     // check if the strand is within on of the regions
                     for (auto rect : vRegions)
                     {
                        VARIANT_BOOL vbResult;
                        rect->ContainsRect(strandRect, &vbResult);
                        if (vbResult == VARIANT_TRUE)
                        {
                           // a debonded strand is in the web width projection region, record it
                           StrandIndexType permStrandIdx(strandIdx);
                           if (arrayPermStrandIndex)
                           {
                              arrayPermStrandIndex->get_Item(strandIdx,&permStrandIdx);
                           }
                           pArtifact->AddDebondedStrandInWebWidthProjection(endType, permStrandIdx);
                           break; // no need to check other regions, break out of the loop
                        }
                     } // next region
                  } // if debonded
               } // next strand
            } // next row
         } // if there are regions
      } // next end

      if (nRegions == 0)
      {
         // if there aren't any web width projection regions, then this check is not applicable
         // (think about double-tee beams... the check may be enabled in the criteria,
         // but double-tee beams fall under Restriction K of 5.9.4.3.3 where the web width projection
         // is not a requirement
         pArtifact->CheckDebondingInWebWidthProjection(false); 
      }
   } // if evaluated
   else
   {
      pArtifact->CheckDebondingInWebWidthProjection(false);
   }

   // NOTE: Restriction I, check for debonding furthest from vertical centerline, is not evaluated
   // NOTE: Restriction J and K, check uniformity of debond spacing, is not evaluated
   // NOTE: Restriction J, check debonding from vertical centerline outward (from notation in Fig C5.9.4.3.3-2), is not evaluated
}

void pgsDesigner2::CheckHorizontalTensionTie(const CGirderKey& girderKey, pgsGirderArtifact* pGdrArtifact) const
{
   GET_IFACE2(GetBroker(), IHorizontalTensionTieChecks, pChecks);
   pChecks->CheckHorizontalTensionTieForce(girderKey, pGdrArtifact);
}

void pgsDesigner2::CheckPrincipalTensionStressInWebs(const CSegmentKey& segmentKey, pgsPrincipalTensionStressArtifact* pArtifact) const
{
   // First determine of this check is applicable... 

   if (WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::EighthEdition2017)
   {
      // this requirement was added in LRFD 8th Edition, 2017... so any spec before this
      // the requirement is not applicable
      pArtifact->SetApplicablity(pgsPrincipalTensionStressArtifact::Specification);
      return;
   }

   // This is always applicable if there is post-tensioning
   // If there isn't post-tensioning, it is only applicable if fc28 > 10 ksi
   GET_IFACE2(GetBroker(),ISegmentTendonGeometry, pSegmentTendonGeometry);
   DuctIndexType nSegmentDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);

   GET_IFACE2(GetBroker(),IGirderTendonGeometry, pGirderTendonGeometry);
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(segmentKey);

   DuctIndexType nDucts = nSegmentDucts + nGirderDucts;

   if (nDucts == 0)
   {
      // no post-tensioning, check fc
      GET_IFACE2(GetBroker(),IMaterials, pMaterials);
      if (IsUHPC(pMaterials->GetSegmentConcreteType(segmentKey)))
      {
         pArtifact->SetApplicablity(pgsPrincipalTensionStressArtifact::Applicable);
      }
      else
      {
         Float64 fc = pMaterials->GetSegmentFc28(segmentKey);

         // threshold f'c for performing principal stress check
         GET_IFACE2(GetBroker(),IConcreteStressLimits, pLimits);
         Float64 principalTensileStressFcThreshold = pLimits->GetPrincipalTensileStressFcThreshold();

         pArtifact->SetApplicablity(principalTensileStressFcThreshold < fc ? pgsPrincipalTensionStressArtifact::Applicable : pgsPrincipalTensionStressArtifact::ConcreteStrength); // no PT so only applicable if fc > 10 ksi
      }
   }
   else
   {
      // this is PT
      pArtifact->SetApplicablity(pgsPrincipalTensionStressArtifact::Applicable);
   }

   if (!pArtifact->IsApplicable())
   {
      // if the check isn't applicable, leave now
      return;

   }

   GET_IFACE2(GetBroker(),IIntervals, pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount() - 1;

   // Get points of interest for the check
   PoiList vPois;
   GetPrincipalWebStressPointsOfInterest(segmentKey, intervalIdx, &vPois);

   pgsPrincipalWebStressEngineer engineer(m_pBroker,m_StatusGroupID);
   engineer.Check(vPois, pArtifact);
}

void pgsDesigner2::CheckReinforcementFatigue(const CSegmentKey& segmentKey, pgsReinforcementFatigueArtifact* pArtifact) const
{
   // As a first implement, we are only check the fatigue stress range in the bottom reinforcement within UHPC girders for
   // positive moment. In the future, we will add this check for top flange reinforcement used to make UHPC deck bulb tee
   // girders continuous for negative moment. The reinforcement only needs to be check if it is in UHPC. Continuity provided
   // by conventional concrete CIP decks with rebar don't need to be checked because the negative moment tension tie is coming
   // from the rebar in the deck.
#pragma Reminder("Add Reinforcement Fatigue check per GS 1.5.3 and LRFD 5.5.3.1 for negative moments continuity in UHPC deck bulb tees, slabs, and other no-deck systems")
   GET_IFACE2(GetBroker(),IMaterials, pMaterials);
   if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
   {
      // Reinforcement fatigue must be checked for UHPC per GS 1.5.3 using the procedures of LRFD 5.5.3.1
      pArtifact->IsApplicable(true);

      GET_IFACE2(GetBroker(),ILoadFactors, pILoadFactors);
      const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
      Float64 gamma = pLoadFactors->GetLLIMMax(pgsTypes::FatigueI);

      GET_IFACE2(GetBroker(),IIntervals, pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

      GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT | POI_5L, &vPoi);
      ATLASSERT(vPoi.size() == 1);
      const pgsPointOfInterest& poi(vPoi.front());

      GET_IFACE2(GetBroker(),IProductForces, pProductForces);
      pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);
      Float64 Mmin, Mmax;
      pProductForces->GetLiveLoadMoment(liveLoadIntervalIdx, pgsTypes::lltFatigue, poi, bat, true/*bIncludeImpact*/, true/*bIncludeLLDF*/, &Mmin, &Mmax);

      GET_IFACE2(GetBroker(),ISectionProperties, pSectProps);
      Float64 I = pSectProps->GetIxx(liveLoadIntervalIdx, poi);

      GET_IFACE2(GetBroker(),IStrandGeometry, pStrandGeom);
      // is there a more efficient way to do this?
      Float64 yps = Float64_Max;
      for (IndexType i = 0; i < 2; i++) // only consider straight and harped, not temporary
      {
         pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
         StrandIndexType nStrands = pStrandGeom->GetStrandCount(segmentKey, strandType);
         for (StrandIndexType strandIdx = 0; strandIdx < nStrands; strandIdx++)
         {
            CComPtr<IPoint2d> pnt;
            pStrandGeom->GetStrandPosition(poi, strandIdx, strandType, &pnt);
            Float64 y;
            pnt->get_Y(&y);
            ATLASSERT(y <= 0.0); // Y value should be negative in girder section coordinates (Y = 0 at top of girder)
            yps = Min(y, yps);
         }
      }

      // yps is measured down from the top of the girder. e is relative to the CG of the composite
      // section. Need to deduct Ytop from yps to get eccentricity.
      Float64 Ytgc = pSectProps->GetY(liveLoadIntervalIdx, poi, pgsTypes::TopGirder);
      Float64 e = Ytgc + yps;

      GET_IFACE2(GetBroker(),IMaterials, pMaterials);
      Float64 Eps = pMaterials->GetStrandMaterial(segmentKey, pgsTypes::Straight)->GetE();
      Float64 Ec = pMaterials->GetSegmentEc28(segmentKey);
      
      // LRFD 5.5.3.3 threshold for prestressing steel
      // Assume radii of curvature is in excess of 30 ft (strands are straight at mid-girder)
      Float64 deltaFth = WBFL::Units::ConvertToSysUnits(18.0, WBFL::Units::Measure::KSI);

      pArtifact->SetLoadFactor(gamma);
      pArtifact->SetFatigueLiveLoadMoment(Mmax);
      pArtifact->SetStrandEccentricity(e);
      pArtifact->SetMomentOfInertia(I);
      pArtifact->SetEps(Eps);
      pArtifact->SetEc(Ec);
      pArtifact->SetFatigueThreshold(deltaFth);
   }
   else
   {
      pArtifact->IsApplicable(false);
   }
}

void pgsDesigner2::CheckMinimumDeckReinforcement(const CGirderKey& girderKey, pgsGirderArtifact* pGirderArtifact) const
{
   ASSERT_GIRDER_KEY(girderKey);

   // Check LRFD 9.7.1.6�Minimum Deck Reinforcement in Negative Moment Region. 
   GET_IFACE2(GetBroker(),ISpecification, pSpec);
   GET_IFACE2_NOCHECK(GetBroker(), IBridge, pBridge);
   GET_IFACE2(GetBroker(), IIntervals, pIntervals);
   GET_IFACE2_NOCHECK(GetBroker(), ILimitStateForces2, pLsForces2);
   GET_IFACE2_NOCHECK(GetBroker(), IPointOfInterest, pPoi);
   GET_IFACE2_NOCHECK(GetBroker(), IMaterials, pMaterials);
   GET_IFACE2_NOCHECK(GetBroker(), ISectionProperties, pSectionProperties);
   GET_IFACE2_NOCHECK(GetBroker(), ILongRebarGeometry, pRebarGeom);
   GET_IFACE2(GetBroker(), IProductForces, pProdForces);

   // Article first appeared in 10th edition
   bool doCheck = pSpec->GetSpecificationType() >= WBFL::LRFD::BDSManager::Edition::TenthEdition2024 && pBridge->IsCompositeDeck();

   // The code below to get the check interval can be found throughout BridgeLink. We might want to consolidate this into a function call.
   auto checkIntervalIdx = pIntervals->GetIntervalCount() - 1;

   pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   // Check deck reinforcement along segments
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
   for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
   {
      auto* pSegmentArtifact = pGirderArtifact->GetSegmentArtifact(segIdx);
      auto* pDeckReinfArtifact = pSegmentArtifact->GetDeckReinforcementCheckArtifact();

      pDeckReinfArtifact->IsApplicable(doCheck);
      if (!doCheck)
      {
         break;
      }
      else
      {
         Float64 phi = 0.9;
         pDeckReinfArtifact->SetPhiFactor(phi);

         GET_IFACE2_NOCHECK(GetBroker(), IGirder, pGirder);

         CSegmentKey segmentKey(girderKey, segIdx);
         PoiList vPois;
         pPoi->GetPointsOfInterest(segmentKey, POI_SPAN, &vPois);
         pPoi->SortPoiList(&vPois);
         // No need to capture jumps here so remove any coincident pois
         vPois.erase(std::unique(std::begin(vPois), std::end(vPois), [](const pgsPointOfInterest& a, const pgsPointOfInterest& b) {return a.AtSamePlace(b);}), std::end(vPois));

         std::vector<Float64> fTopMinServiceI, fTopMaxServiceI;
         pLsForces2->GetStress(checkIntervalIdx, pgsTypes::ServiceI, vPois, bat, false, pgsTypes::TopDeck, &fTopMinServiceI, &fTopMaxServiceI);

         std::size_t poiIdx = 0;
         for (const auto& poi : vPois)
         {
            Float64 deckTensileStress = fTopMaxServiceI[poiIdx];

            IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
            Float64 modRupture = pMaterials->GetDeckFlexureFr(deckCastingRegionIdx, checkIntervalIdx);

            Float64 tribAreaDeck = pSectionProperties->GetTributaryDeckArea(poi);
            Float64 effAreaDeck = pSectionProperties->GetEffectiveDeckArea(poi);
            Float64 areaCIPDeck = max(tribAreaDeck, effAreaDeck);

            Float64 areaReinforcement = pRebarGeom->GetAsTopMat(poi, pgsTypes::drbAll, pgsTypes::drcAll);
            areaReinforcement        += pRebarGeom->GetAsBottomMat(poi, pgsTypes::drbAll, pgsTypes::drcAll);

            bool Passed(true);
            bool isApplicable(false);

            Float64 limit = phi * modRupture; ;
            if (deckTensileStress > limit)
            {
               isApplicable = true;

               Float64 percentRebar = 100.0 * areaReinforcement / areaCIPDeck;
               Passed = percentRebar > 1.0;
            }

            pgsDeckReinforcementCheckAtPoisArtifact poiArtifact(poi.get(), deckTensileStress, areaCIPDeck, areaReinforcement, modRupture, Passed, isApplicable);
            pDeckReinfArtifact->AddDeckReinforcementCheckAtPoisArtifact(poiArtifact);

            poiIdx++;
         }
      }
   }
}

void pgsDesigner2::GetPrincipalWebStressPointsOfInterest(const CSegmentKey & rSegmentKey, IntervalIndexType intervalIdx, PoiList * pPoiList) const
{
   std::vector<CSegmentKey> segmentKeys;
   if (rSegmentKey.segmentIndex == ALL_SEGMENTS)
   {
      CGirderKey gdrKey(rSegmentKey);

      GET_IFACE2(GetBroker(),IBridge,pBridge);
      SegmentIndexType nSegments = pBridge->GetSegmentCount(gdrKey);
      for (SegmentIndexType iseg = 0; iseg < nSegments; iseg++)
      {
         segmentKeys.push_back(CSegmentKey(gdrKey,iseg));
      }
   }
   else
   {
      segmentKeys.push_back(rSegmentKey);
   }

   for (auto& segmentKey : segmentKeys)
   {
      PoiList vPois;
      GetShearPointsOfInterest(false/*not design*/, segmentKey, pgsTypes::StrengthI, intervalIdx, vPois);
      // NOTE: even though this is a ServiceIII check, we need to get the shear POI for StrengthI because
      // shear is a strength limit state check. This principal tension check was added to LRFD in 8th Edition.
      // Prior to 8th edition, the critical section was changed so that it is no longer a function of the
      // limit state. As such, we can safely use the StrengthI limit state value.

      // don't check POIs that are in critical section zones
      GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);
      for (const auto& poiRef : vPois)
      {
         if (!pPoi->IsInCriticalSectionZone(poiRef.get(), pgsTypes::StrengthI))
         {
            pPoiList->emplace_back(poiRef);
         }
      }
   }
}

void pgsDesigner2::DesignEndZone(bool firstPass, const arDesignOptions& options, pgsSegmentDesignArtifact& artifact, std::shared_ptr<IEAFProgress> pProgress) const
{
   DESIGN_LOG_SCOPE(_T("DesignEndZone"));
   // At this point we either have harping or debonding maximized in the end-zones
   // The concrete strength for lifting will control over this case
   // If we are designing for lifting don't figure the concrete strength here
   if (!options.doDesignLifting )
   {
      DesignEndZoneReleaseStrength(pProgress);
      if (  m_DesignerOutcome.WasDesignAborted() )
      {
         if (firstPass)
         {
            // There is a slim chance that we can reduce the number of strands needed by increasing the final 
            // strength. Go for it if we are early in design.
            Float64 fc_max = m_StrandDesignTool->GetMaximumConcreteStrength();
            Float64 fci_min = m_StrandDesignTool->GetMinimumReleaseStrength();
            LOG_ACTION(_T("We failed to attain release in the early design stages. Let's throw a Hail Mary and set f'c to max  = ")<< WBFL::Units::ConvertFromSysUnits(fc_max, WBFL::Units::Measure::KSI) << _T(" ksi and f'ci to min = ")<< WBFL::Units::ConvertFromSysUnits(fci_min, WBFL::Units::Measure::KSI));

            GET_IFACE2(GetBroker(),IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(artifact.GetSegmentKey());
            IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;
            m_StrandDesignTool->UpdateReleaseStrength(fci_min, ConcSuccess, StressCheckTask(releaseIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension), pgsTypes::TopGirder);
            m_StrandDesignTool->UpdateConcreteStrength(fc_max,StressCheckTask(lastIntervalIdx, pgsTypes::ServiceIII, pgsTypes::Tension), pgsTypes::BottomGirder);

            m_DesignerOutcome.Reset();
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciIncreased);
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcIncreased);
            return;
         }
         else
         {
            return;
         }
      }
      else if (m_DesignerOutcome.DidConcreteChange() )
      {
         return; // concrete strength changed, we will want to redo strands 
      }
      else if (m_DesignerOutcome.DidRaiseStraightStrands() )
      {
         return; // restart with raised strands
      }
   }

   if ( m_StrandDesignTool->IsDesignHarping() )
   {
      DesignEndZoneHarping(options, artifact, pProgress);
   }
   else
   {
      DesignEndZoneDebonding(firstPass, options, artifact, pProgress);
   }
}

void pgsDesigner2::DesignEndZoneDebonding(bool firstPass, const arDesignOptions& options, pgsSegmentDesignArtifact& artifact, std::shared_ptr<IEAFProgress> pProgress) const
{
   DESIGN_LOG_SCOPE(_T("DesignEndZoneDebonding"));

   // Refine end-zone design. Lifting will always trump the simple release condition because of the
   // shorter span length.
   // Refine design for lifting. Outcome is lifting loop location and required release strength
   // If temporary strands are required, this design refinement will be incomplete. We will move on to
   // design for hauling because it will typically control the temporary strand requirements. Then,
   // we will return to design for lifting.

   // Compute and layout debonding prior to hauling design
   std::vector<DebondLevelType> debond_demand;

   m_StrandDesignTool->DumpDesignParameters();

   if (options.doDesignLifting && m_StrandDesignTool->IsDesignDebonding())
   {
      DLOG(_T("Initial Lifting Design for Debond Section"));
      DesignForLiftingDebonding(options.doDesignHauling, pProgress);

      if ( m_DesignerOutcome.WasDesignAborted() )
      {
         // attempt to add raised strands to help lifting - might be a no go
         if ( m_StrandDesignTool->AddRaisedStraightStrands() )
         {
            m_DesignerOutcome.Reset();
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::RaisedStraightStrands);
            LOG_ACTION(_T("Added Raised Straight Strands to control lifting stresses - Restart design with new strand configuration"));
            return;
         }
         else
         {
            LOG_FAIL(_T("Initial Lifting Debond Design failed"));
            return;
         }
      }
      else if (m_DesignerOutcome.DidConcreteChange() )
      {
         LOG_ACTION(_T("Concrete strength changed for initial lifting design - restart"));
         return; // concrete strength changed, we will want to redo strands 
      }
   }
   else
   {
      DLOG(_T("Design debonding and release strength for Simple Release Condition at endzone"));
      debond_demand = DesignEndZoneReleaseDebonding(pProgress);

      CHECK_PROGRESS;

      if ( m_DesignerOutcome.WasDesignAborted() )
      {
         LOG_ABORT(_T("Failed to design Debonding for release - Abort"));
         ATLASSERT(false);
         return;
      }
      else if (m_DesignerOutcome.DidFinalConcreteStrengthChange() )
      {
         LOG_ACTION(_T("Final Concrete strength changed for end zone release - restart"));
         return; // concrete strength changed, we will want to redo strands 
      }
   }

   m_StrandDesignTool->DumpDesignParameters();

   if (!debond_demand.empty())
   {
      // Layout debonding prior to hauling design
      DLOG(_T("Release/Lifting demand = ")<<DumpIntVector(debond_demand));

      bool succ = m_StrandDesignTool->LayoutDebonding( debond_demand );

      if (!succ)
      {
         LOG_ABORT(_T("Failed to layout Debonding - Abort"));
         m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::DebondDesignFailed);
         m_DesignerOutcome.AbortDesign();
         ATLASSERT(false);
         return;
      }
   }

   // Initial design for hauling. Outcome is truck support location, required concrete strength, and temporary strands if necessary
   if (options.doDesignHauling)
   {
      DesignForShipping(pProgress);
      
      CHECK_PROGRESS;

      if( m_DesignerOutcome.WasDesignAborted() )
      {
         LOG_ABORT(_T("Failed Initial Shipping Design - Abort"));
         return;
      }
      else if ( m_DesignerOutcome.DidFinalConcreteStrengthChange() )
      {
         // No use going further - number of strands will change for design
         LOG_ACTION(_T("Final Concrete strength changed for shipping design - restart"));
         return; 
      }
      else if( m_DesignerOutcome.DidRaiseStraightStrands() )
      {
         LOG_ACTION(_T("Raised Straight strands were added after DesignForShipping - restart"));
         return;
      }

      // The only way hauling design can affect lifting/release is if temporary strands 
      // were added. Update release strength if this is the case.
      if ( m_DesignerOutcome.GetOutcome(pgsDesignCodes::LiftingRedesignAfterShipping) ||
           0 < m_StrandDesignTool->GetNt() )
      {
         if (options.doDesignLifting && m_StrandDesignTool->IsDesignDebonding())
         {
            DLOG(_T("Secondary Lifting Design after Shipping."));
            std::vector<DebondLevelType> debond_demand_lifting;
            debond_demand_lifting = DesignForLiftingDebonding(false,pProgress);

            // Only layout debonding if first pass through lifting design could not
            if (m_DesignerOutcome.GetOutcome(pgsDesignCodes::LiftingRedesignAfterShipping) && !debond_demand_lifting.empty())
            {
               DLOG(_T("Release/Lifting demand = ")<<DumpIntVector(debond_demand_lifting));

               bool succ = m_StrandDesignTool->LayoutDebonding( debond_demand_lifting );
               if (!succ)
               {
                  LOG_ABORT(_T("Failed to layout Debonding - Abort"));
                  m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::DebondDesignFailed);
                  m_DesignerOutcome.AbortDesign();
                  ATLASSERT(false);
                  return;
               }
            }
         }
         else
         {
            DLOG(_T("Secondary Design of release condition (strength only) after Shipping."));
            DesignEndZoneReleaseStrength(pProgress);
         }

         if ( m_DesignerOutcome.WasDesignAborted() )
         {
            LOG_ABORT(_T("Second Pass Lifting/Release Debond Design failed - Abort"));
            return;
         }
         else if ( m_DesignerOutcome.DidConcreteChange() )
         {
            LOG_ACTION(_T("Lifting/Release Design changed concrete strength - Restart"));
            return;
         }
      }
   }
   else
   {
      DLOG(_T("Skipping Hauling design"));
   }

}

void pgsDesigner2::DesignEndZoneHarping(arDesignOptions options, pgsSegmentDesignArtifact& artifact, std::shared_ptr<IEAFProgress> pProgress) const
{
   DESIGN_LOG_SCOPE(_T("DesignEndZoneHarping"));

   // Refine end-zone design. Lifting will always trump the simple release condition because of the
   // shorter span length.
   // Refine design for lifting. Outcome is lifting loop location and required release strength
   // If temporary strands are required, this design refinement will be incomplete. We will move on to
   // design for hauling because it will typically control the temporary strand requirements. Then,
   // we will return to design for lifting.

   pgsDesignCodes lifting_design_outcome;

   // Captured before phase 1 runs, so it can be compared against the release strength after phase 2
   // completes. Phase 1's harped/straight strand trading falls back to bumping the release strength
   // (see DesignForLiftingHarping) when it can't reach the target eccentricity, using whatever
   // no-temporary-strand configuration is current; phase 2 then makes the real, final determination
   // for the with-temporary-strand configuration. For this segment those two determinations can be
   // stable, reproducible, and genuinely different from each other every time - if so, phase 2 will
   // undo phase 1's bump back to the same value this function started with, and that is not a change
   // worth restarting the whole outer design loop over.
   Float64 fci_on_entry = m_StrandDesignTool->GetReleaseStrength();

   if (options.doDesignLifting)
   {
      DLOG(_T("Start Lifting design."));

      // the goal of this lifting design is to adjust the harped and straight strands
      // into the optimal configuration for fabrication
      DesignForLiftingHarping(options,true,pProgress);
      
      lifting_design_outcome = m_DesignerOutcome;

      if ( m_DesignerOutcome.WasDesignAborted() )
      {
         return;
      }
   }
   else
   {
      DLOG(_T("Skipping Lifting design."));

      // lifting will control over simple release, so it's either/or here
      DLOG(_T("Adjust harping height/angle at endzones"));
      DesignEndZoneHarpingAdjustment(options, pProgress);

      CHECK_PROGRESS;

      if ( m_DesignerOutcome.WasDesignAborted() )
      {
         return;
      }
      else if (m_DesignerOutcome.DidConcreteChange() )
      {
         return; // concrete strength changed, we will want to redo strands 
      }
   }

   m_StrandDesignTool->DumpDesignParameters();

   // Refine design for hauling. Outcome is truck support location, required concrete strength, and temporary strands if necessary
   if (options.doDesignHauling)
   {
      // determine shipping configuration, number of temporary strands,
      // and possibly the final concrete strength
      DesignForShipping(pProgress);
      
      CHECK_PROGRESS;

      if( m_DesignerOutcome.WasDesignAborted() )
      {
         return;
      }
   }
   else
   {
      DLOG(_T("Skipping Hauling design"));
   }

   if ( options.doDesignLifting )
   {
      // design for lifting to get the lifting configuration and required
      // release strength for lifting with temporary strands
      DLOG(_T("Design for Lifting after Shipping"));
      DesignForLiftingHarping(options,false,pProgress);

      CHECK_PROGRESS;

      if ( m_DesignerOutcome.WasDesignAborted() )
      {
         LOG_ABORT(_T("Lifting Design aborted"));
         return;
      }
      else if ( m_DesignerOutcome.DidConcreteChange() )
      {
         if ( !IsEqual(m_StrandDesignTool->GetReleaseStrength(), fci_on_entry) )
         {
            LOG_ACTION(_T("Lifting Design changed concrete strength - Restart"));
            return;
         }
         else
         {
            // Phase 1's fallback bump (if any) and phase 2's final determination netted out to
            // exactly the release strength this function started with - nothing downstream of here
            // was verified against a value that's actually changing, so there's nothing to restart
            // the outer design loop for. Clear the outcome bits so the outer loop doesn't restart
            // for this either - FciIncreased/FciDecreased are only ever read in aggregate via
            // DidConcreteChange()/DidFinalConcreteStrengthChange(), never individually, so clearing
            // just these two is safe.
            DLOG(_T("Lifting Design's release strength change netted out to no change - continuing"));
            m_DesignerOutcome.ClearOutcome(pgsDesignCodes::FciIncreased);
            m_DesignerOutcome.ClearOutcome(pgsDesignCodes::FciDecreased);
         }
      }
   }

}

void pgsDesigner2::DesignMidZone(bool bUseCurrentStrands, const arDesignOptions& options,std::shared_ptr<IEAFProgress> pProgress) const
{
   DESIGN_LOG_SCOPE(_T("DesignMidZone"));
   if ( bUseCurrentStrands )
   {
      m_StrandDesignTool->DumpDesignParameters();
   }

   Int16 cIter = 0;
   Int16 nFutileAttempts=0;
   Int16 nIterMax = 40;
   Int16 nIterEarlyStage = (options.doDesignSlabOffset != sodPreserveHaunch) ? 10 : 5; // Early design stage - NOTE: DO NOT change this value unless you run all design tests VERY SENSITIVE!!!
   StrandIndexType Np, Nt;
   Float64 fc, fci, start_slab_offset(0), end_slab_offset(0);

   DLOG(_T("UPDATE INITIAL DESIGN PARAMETERS IN MID-ZONE"));
   DLOG(_T("Determine initial design parameters by iterating until # Strands, f'c, f'ci, and Slab offset all converge"));

   m_StrandDesignTool->DumpDesignParameters();

   bool bConverged = false;
   do 
   {
      CHECK_PROGRESS;

      m_DesignerOutcome.Reset();

      Np = m_StrandDesignTool->GetNumPermanentStrands();
      Nt = m_StrandDesignTool->GetNt();
      fc = m_StrandDesignTool->GetConcreteStrength();
      ConcStrengthResultType str_result;
      fci = m_StrandDesignTool->GetReleaseStrength(&str_result);
      if (options.doDesignSlabOffset != sodPreserveHaunch)
      {
         start_slab_offset = m_StrandDesignTool->GetSlabOffset(pgsTypes::metStart);
         end_slab_offset   = m_StrandDesignTool->GetSlabOffset(pgsTypes::metEnd);
      }

      DESIGN_LOG_SCOPE(_T("Mid-zone trial ") << cIter << _T(" [") << m_StrandDesignTool->GetDesignStateSummary() << _T("]"));

      if (1 < cIter)
      {
         std::_tostringstream os2;
         os2 << _T("Initial Strand Design Trial ")<<cIter << std::ends;
         pProgress->UpdateMessage(os2.str().c_str());
      }

      DesignMidZoneInitialStrands(bUseCurrentStrands ? true : (cIter == 0 ? false : true), pProgress);

      if ( m_DesignerOutcome.WasDesignAborted() )
      {
         if ( 0 < m_StrandDesignTool->GetMaxPermanentStrands() && cIter <= nIterEarlyStage && nFutileAttempts < 2)
         {
            // Could be that release strength controls instead of final. Give it a chance.
            LOG_FAIL(_T("Initial Design Trial # ") << cIter <<_T(" Failed - try to increase release strength to reduce losses"));
            DesignMidZoneAtRelease(options, pProgress);

            if( m_DesignerOutcome.DidRaiseStraightStrands() )
            {
               DLOG(_T("Raised Straight strands were added by DesignMidZoneAtRelease in initial throws"));
               return;
            }

            // the purpose of calling DesignMidZoneAtRelease is to adjust the initial release strength
            // if it is too low. The new value is also and Initial Strength... re-initialize the
            // Fci controller with the new _T("Initial") strength
            Float64 newFci = m_StrandDesignTool->GetReleaseStrength(&str_result);
            if ( !IsEqual(fci,newFci) )
            {
               m_StrandDesignTool->InitReleaseStrength( newFci, m_StrandDesignTool->GetReleaseConcreteDesignState().Interval() );
            }

            // Since it aborted, we know that the initial number of strands was bad. The only good info we have is concrete strength
            if (nFutileAttempts == 0)
            {
               m_StrandDesignTool->GuessInitialStrands();
            }

            nFutileAttempts++; // not totally futile, but doesn't work often
         }
         else
         {
            return;
         }
      }

      // logic here is a bit tricky. 
      // We want the initial design above to work for a while to dial in the final strength
      // because this will help minimize final strength. However, if it works too long, 
      // Service tension might not be the controlling issue, so let other issues into the mix
      // after five or so iterations.
      if( nIterEarlyStage < cIter )
      {
         // We have tried multiple strand designs and still have not converged.
         // In practice, this may mean that the release strength is way too low, and we
         // are getting excessive losses at Release.
         DLOG(_T("Did not converge in early stage of iterations = ")<<nIterEarlyStage<<_T(" take a stab at end zone release"));
         if ( m_StrandDesignTool->IsDesignDebonding() )
         {
            // For debond design, set debonding to maximum allowed for the current number of strands.
            // This should result in a minimum possible release strength. 
            std::vector<DebondLevelType> max_debonding = m_StrandDesignTool->GetMaxPhysicalDebonding();

            m_StrandDesignTool->LayoutDebonding( max_debonding );
         }

         // find the release strength
         pProgress->UpdateMessage(_T("Computing Concrete Strength At Release"));

         DesignEndZoneReleaseStrength(pProgress);

         if (  m_DesignerOutcome.WasDesignAborted() )
         {
            return;
         }
         else if (  m_DesignerOutcome.DidRaiseStraightStrands() )
         {
            LOG_ACTION(_T("Added Raised Straight Strands early in DesignMidZone - Restart design with new strand configuration"));
            return; // will restart design
         }
         else if ( m_DesignerOutcome.DidConcreteChange() )
         {
            continue; // back to the start of the loop
         }
      }
      else if (cIter<=nIterEarlyStage && m_DesignerOutcome.DidConcreteChange() )
      {
         // give the initial design more chances
         continue; // back to the start of the loop
      }

      // Skip tweaking the concrete strength if we are doing a hauling design
      // Hauling will produce the maximum required strength so we don't need to waste time here
      pProgress->UpdateMessage(_T("Computing Final Concrete Strength"));
      if ( !options.doDesignHauling )
      {
         DesignMidZoneFinalConcrete( pProgress );

         CHECK_PROGRESS;
         if (  m_DesignerOutcome.WasDesignAborted() )
         {
            return;
         }
         else if ( m_DesignerOutcome.DidConcreteChange() )
         {
            LOG_ACTION(_T("Concrete Strength Changed - restart design"));
            continue; // back to the start of the loop
         }

         DesignMidZoneAtRelease( options, pProgress );

         CHECK_PROGRESS;
         if (  m_DesignerOutcome.WasDesignAborted() )
         {
            return;
         }
         else if ( m_DesignerOutcome.DidConcreteChange() )
         {
            LOG_ACTION(_T("Concrete Strength Changed - restart design"));
            continue; // back to the start of the loop
         }
         else if( m_DesignerOutcome.DidRaiseStraightStrands() )
         {
            DLOG(_T("Raised Straight strands were added by DesignMidZoneAtRelease in secondary pass"));
            return;
         }
      }

      bool Aconverged;
      if (options.doDesignSlabOffset != sodPreserveHaunch)
      {
         pProgress->UpdateMessage(_T("Designing Slab Offset - inner loop"));
         DesignSlabOffset( pProgress );

         CHECK_PROGRESS;
         if (  m_DesignerOutcome.WasDesignAborted() )
         {
            return;
         }

         // slab offset must be equal to or slightly larger than calculated. If it is smaller, we might under design.
         Float64 AdiffStart = start_slab_offset - m_StrandDesignTool->GetSlabOffset(pgsTypes::metStart);
         Float64 AdiffEnd = end_slab_offset - m_StrandDesignTool->GetSlabOffset(pgsTypes::metEnd);
         Aconverged = (0.0 <= AdiffStart && AdiffStart <= WBFL::Units::ConvertToSysUnits(0.5,WBFL::Units::Measure::Inch)) &&
            (0.0 <= AdiffEnd && AdiffEnd <= WBFL::Units::ConvertToSysUnits(0.5,WBFL::Units::Measure::Inch));
      }
      else
      {
         DLOG(_T("Skipping Slab Offset Design due to user input"));
         Aconverged = true; // we did not touch A
      }

      m_StrandDesignTool->DumpDesignParameters();

      DLOG(_T("End of trial ")<<cIter);
      DLOG(_T("Np: ")<< (Np==m_StrandDesignTool->GetNumPermanentStrands() ? _T("Converged"):_T("Did not Converge")) );
      DLOG(_T("Nt: ")<< (Nt==m_StrandDesignTool->GetNt() ? _T("Converged"):_T("Did not Converge")) );
      DLOG(_T("f'c: ")<< (IsEqual(fc,m_StrandDesignTool->GetConcreteStrength()) ? _T("Converged"):_T("Did not Converge")) );
      DLOG(_T("f'ci: ")<< (IsEqual(fci,m_StrandDesignTool->GetReleaseStrength()) ? _T("Converged"):_T("Did not Converge")) );
      if (options.doDesignSlabOffset != sodPreserveHaunch)
      {
         DLOG(_T("Slab Offset:") << (Aconverged ? _T("Converged") : _T("Did not Converge")));
      }

      if ( Np == m_StrandDesignTool->GetNumPermanentStrands()     &&
           Nt == m_StrandDesignTool->GetNt()         &&
           IsEqual(fc,m_StrandDesignTool->GetConcreteStrength()) &&
           IsEqual(fci,m_StrandDesignTool->GetReleaseStrength()) &&
           Aconverged
         )
      {
         bConverged = true;
      }
      else
      {
         DLOG(_T("# strands, f'c, f'ci, and slab offset have not converged"));
      }

   } while (cIter++ < nIterMax && !bConverged );

   if ( nIterMax <= cIter )
   {
      LOG_ABORT(_T("Maximum number of iterations was exceeded - aborting design ") << cIter);
      m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::MaxIterExceeded);
      m_DesignerOutcome.AbortDesign();

      // Check with RDP... if cIter >= nIterMax set the design outcome to abort
      // however, we this if-block exits, the design outcome is reset (see below)
      ATLASSERT(false); 
      return;
   }

   DLOG(_T("# strands, f'c, f'ci, and slab offset have Converged. Reset outcome and continue"));
   m_DesignerOutcome.Reset();
}

struct ConcreteStrengthParameters
{
   StressCheckTask task;

   pgsTypes::StressLocation stress_location;
   std::_tstring strLimitState;
   PoiAttributeType find_type;
   Float64 fmax;

   Float64 fbpre;
   pgsPointOfInterest poi; // location of controlling concrete strength (for debugging and logging)

   ConcreteStrengthParameters(pgsTypes::LimitState ls,
                              LPCTSTR lpszLimitState,
                              IntervalIndexType intervalIdx,
                              bool bIncludeLiveLoad,
                              pgsTypes::StressType stressType,
                              pgsTypes::StressLocation stressLocation,
                              PoiAttributeType findType) :
      task(intervalIdx,ls,stressType,bIncludeLiveLoad),
      strLimitState(lpszLimitState),
      stress_location(stressLocation),
      find_type(findType)
   {fmax = (task.stressType == pgsTypes::Tension ? -Float64_Max : Float64_Max);}

};

void pgsDesigner2::DesignMidZoneFinalConcrete(std::shared_ptr<IEAFProgress> pProgress) const
{
   DESIGN_LOG_SCOPE(_T("DesignMidZoneFinalConcrete"));
   // Note that the name of this function is a bit of a misnomer since most of the 
   // limit states here look in end-zone locations, and the main work of mid-zone stress
   // design has already been done by the initial strands design.
   // The real purpose of this function is to ensure that the decisions made during the
   // mid-zone design allow a viable end-zone design further on.
   // At this point harped strands are lifted to their highest point at girder ends, or
   // debonding is maximal. If we can't find a concrete strength here, there is no end-zone design.
   DLOG(_T("DesignMidZoneFinalConcrete:: Computing required concrete strength"));

   const CSegmentKey& segmentKey = m_StrandDesignTool->GetSegmentKey();

   Float64 fc_current = m_StrandDesignTool->GetConcreteStrength();

   GET_IFACE2(GetBroker(),ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType noncompositeIntervalIdx = pIntervals->GetLastNoncompositeInterval();
   IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();
   IntervalIndexType lastIntervalIdx          = pIntervals->GetIntervalCount()-1;

   // Maximize stresses at pois for their config
   std::vector<ConcreteStrengthParameters> vConcreteStrengthParameters;
   vConcreteStrengthParameters.push_back(ConcreteStrengthParameters(pgsTypes::ServiceI,_T("Service I final with live load"),lastIntervalIdx,true,pgsTypes::Compression,pgsTypes::BottomGirder,POI_HARPINGPOINT|POI_PSXFER));
   vConcreteStrengthParameters.push_back(ConcreteStrengthParameters(pgsTypes::ServiceI, _T("Service I final without live load"), lastIntervalIdx, false, pgsTypes::Compression, pgsTypes::BottomGirder, POI_HARPINGPOINT | POI_PSXFER));
   vConcreteStrengthParameters.push_back(ConcreteStrengthParameters(pgsTypes::ServiceI, _T("Service I final without live load"), lastIntervalIdx, false, pgsTypes::Compression, pgsTypes::TopGirder, (POI_SPAN | POI_5L)));
   vConcreteStrengthParameters.push_back(ConcreteStrengthParameters(WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims ? pgsTypes::ServiceIA : pgsTypes::FatigueI,WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims ? _T("Service IA") : _T("Fatigue I"),lastIntervalIdx,true,pgsTypes::Compression,pgsTypes::BottomGirder,POI_HARPINGPOINT|POI_PSXFER));
   vConcreteStrengthParameters.push_back(ConcreteStrengthParameters(pgsTypes::ServiceIII,_T("Service III"),lastIntervalIdx,true,pgsTypes::Tension,pgsTypes::BottomGirder,POI_HARPINGPOINT|(POI_SPAN | POI_5L)));

   GET_IFACE2(GetBroker(),IConcreteStressLimits,pLimits);
   if ( pLimits->CheckTemporaryStresses() )
   {
      vConcreteStrengthParameters.push_back(ConcreteStrengthParameters(pgsTypes::ServiceI,_T("Service I non-composite girder"),noncompositeIntervalIdx,true,pgsTypes::Compression,pgsTypes::TopGirder,(POI_SPAN | POI_5L)));
   }

   if ( pLimits->CheckFinalDeadLoadTensionStress() )
   {
      vConcreteStrengthParameters.push_back(ConcreteStrengthParameters(pgsTypes::ServiceI,_T("Service I final without live load"),lastIntervalIdx,false,pgsTypes::Tension,pgsTypes::BottomGirder,POI_HARPINGPOINT|(POI_SPAN | POI_5L)));
   }

   GET_IFACE2(GetBroker(),ILimitStateForces,pForces);
   GET_IFACE2(GetBroker(),IPretensionStresses,pPrestress);
   const GDRCONFIG& config = m_StrandDesignTool->GetSegmentConfiguration();

   for (auto& concParams : vConcreteStrengthParameters)
   {
      // Get Points of Interest at the expected
      PoiList vPOI;
      m_StrandDesignTool->GetDesignPoi(concParams.task.intervalIdx, concParams.find_type, &vPOI);
      ATLASSERT(!vPOI.empty());

      DLOG(_T("Checking for ") << concParams.strLimitState << StrTopBot(concParams.stress_location) << (concParams.task.stressType==pgsTypes::Tension?_T(" Tension"):_T(" Compression")) );

      pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Envelope ? pgsTypes::MaxSimpleContinuousEnvelope : (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan));
      for(const pgsPointOfInterest& poi : vPOI)
      {
         CHECK_PROGRESS;

         Float64 min,max;
         pForces->GetDesignStress(concParams.task,poi,concParams.stress_location,&config,bat,&min,&max);

         DLOG(_T("     max = ") << WBFL::Units::ConvertFromSysUnits(max,WBFL::Units::Measure::KSI) << _T(" ksi, min = ") << WBFL::Units::ConvertFromSysUnits(min,WBFL::Units::Measure::KSI) << _T(" ksi, at ")<< WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart(), WBFL::Units::Measure::Feet) << _T(" ft") );

         // save max stress and corresponding prestress stress
         if (concParams.task.stressType == pgsTypes::Tension)
         {
            if (concParams.fmax < max)
            {
               concParams.fmax = max;
               concParams.fbpre = pPrestress->GetStress(concParams.task.intervalIdx,poi,concParams.stress_location,concParams.task.bIncludeLiveLoad, concParams.task.limitState, INVALID_INDEX, &config);

               concParams.poi = poi;
            }
         }
         else
         {
            // compression
            if (min < concParams.fmax)
            {
               concParams.fmax = min;
               concParams.fbpre = pPrestress->GetStress(concParams.task.intervalIdx,poi,concParams.stress_location,concParams.task.bIncludeLiveLoad, concParams.task.limitState, INVALID_INDEX, &config);

               concParams.poi = poi;
            }
         }
      }
   }

   GET_IFACE2(GetBroker(),ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

   for( auto & concParams : vConcreteStrengthParameters)
   {
      Float64 k = pLoadFactors->GetDCMax(concParams.task.limitState);

      DLOG(_T("Stress Demand (") << concParams.strLimitState << StrTopBot(concParams.stress_location) << _T(" fmax = ") << WBFL::Units::ConvertFromSysUnits(concParams.fmax,WBFL::Units::Measure::KSI) << _T(" ksi, fbpre = ") << WBFL::Units::ConvertFromSysUnits(concParams.fbpre,WBFL::Units::Measure::KSI) << _T(" ksi, ftotal = ") << WBFL::Units::ConvertFromSysUnits(concParams.fmax + k*concParams.fbpre,WBFL::Units::Measure::KSI) << _T(" ksi, at ")<< WBFL::Units::ConvertFromSysUnits(concParams.poi.GetDistFromStart(), WBFL::Units::Measure::Feet) << _T(" ft") );

      concParams.fmax += k*concParams.fbpre;

      Float64 fc_reqd;
      ConcStrengthResultType success = m_StrandDesignTool->ComputeRequiredConcreteStrength(concParams.fmax,concParams.task,&fc_reqd);
      if ( ConcFailed == success )
      {
         LOG_FAIL(_T("ComputeRequiredConcreteStrength in DesignMidZoneFinalConcrete returned with ConcFailed"));
      }
      else
      {
         m_StrandDesignTool->UpdateConcreteStrength(fc_reqd,concParams.task,concParams.stress_location);
      }
   }

}

void pgsDesigner2::DesignMidZoneAtRelease(const arDesignOptions& options, std::shared_ptr<IEAFProgress> pProgress) const
{
   DESIGN_LOG_SCOPE(_T("DesignMidZoneAtRelease"));

   DLOG(_T("Designing Mid-Zone at Release"));

   const CSegmentKey& segmentKey = m_StrandDesignTool->GetSegmentKey();

   GET_IFACE2(GetBroker(),ILimitStateForces,pForces);
   GET_IFACE2(GetBroker(),IPretensionStresses,pPrestress);
   GET_IFACE2(GetBroker(),IProductForces,pProdForces);

   pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(m_StrandDesignTool->GetSegmentKey());
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

   GDRCONFIG config = m_StrandDesignTool->GetSegmentConfiguration();

   // Get Points of Interest in mid-zone
   PoiList vPOI;
   m_StrandDesignTool->GetDesignPoi(releaseIntervalIdx, POI_5L | POI_RELEASED_SEGMENT, &vPOI);
   PoiList vPOI1;
   m_StrandDesignTool->GetDesignPoi(releaseIntervalIdx, POI_HARPINGPOINT, &vPOI1);

   GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);
   pPoi->MergePoiLists(vPOI, vPOI1,&vPOI);
   ATLASSERT( !vPOI.empty() );

   // let's look at bottom compression first. 
   // If we have to increase final strength, we restart
   Float64 fbot = Float64_Max;
   pgsPointOfInterest bot_poi;

   for(const pgsPointOfInterest& poi : vPOI)
   {
      CHECK_PROGRESS;

      Float64 min, max;
      pForces->GetStress(releaseIntervalIdx,pgsTypes::ServiceI,poi,bat,false,pgsTypes::BottomGirder,&min,&max);

      Float64  fBotPretension = pPrestress->GetStress(releaseIntervalIdx, poi, pgsTypes::BottomGirder, false, pgsTypes::ServiceI, INVALID_INDEX, &config);

      min += fBotPretension;

      // save max'd stress and corresponding poi
      if (min < fbot)
      {
         fbot = min;
         bot_poi = poi;
      }
   }

   DLOG(_T("Controlling Stress Demand at Release , bottom, compression = ") << WBFL::Units::ConvertFromSysUnits(fbot,WBFL::Units::Measure::KSI) << _T(" ksi at ")<< WBFL::Units::ConvertFromSysUnits(bot_poi.GetDistFromStart(), WBFL::Units::Measure::Feet) << _T(" ft") );

   ConcStrengthResultType release_result;
   Float64 fc  = m_StrandDesignTool->GetConcreteStrength();
   Float64 fci = m_StrandDesignTool->GetReleaseStrength(&release_result);
   DLOG(_T("current f'c  = ") << WBFL::Units::ConvertFromSysUnits(fc,WBFL::Units::Measure::KSI) << _T(" ksi") );
   DLOG(_T("current f'ci = ") << WBFL::Units::ConvertFromSysUnits(fci,WBFL::Units::Measure::KSI) << _T(" ksi") );

   Float64 fc_comp;
   ConcStrengthResultType success = m_StrandDesignTool->ComputeRequiredConcreteStrength(fbot,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression),&fc_comp);
   if ( success==ConcFailed )
   {
      if ( m_StrandDesignTool->AddRaisedStraightStrands() )
      {
         // Attempt to add raised straight strands if this is an option. Very small chance that it will work 
         // for this case, but...
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::RaisedStraightStrands);
         LOG_ACTION(_T("Added Raised Straight Strands to control mid-zone compression - Restart design with new strand configuration"));
         return;
      }
      else
      {
         LOG_ABORT(_T("Could not find adequate release strength to control mid-zone compression - Design Abort") );
         m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::ReleaseStrength);
         m_DesignerOutcome.AbortDesign();
         return;
      }
   }

   DLOG(_T("Required Release Strength = ") << WBFL::Units::ConvertFromSysUnits(fc_comp,WBFL::Units::Measure::KSI) << _T(" ksi") );

   // only update if we are increasing release strength - we are downstream here and a decrease is not desired
   if (fci < fc_comp)
   {
      bool bFciUpdated = m_StrandDesignTool->UpdateReleaseStrength(fc_comp, success, StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI, pgsTypes::Compression), pgsTypes::BottomGirder);
      if ( bFciUpdated )
      {
         fci = m_StrandDesignTool->GetReleaseStrength(&release_result);
         DLOG(_T("Release Strength Increased to ")  << WBFL::Units::ConvertFromSysUnits(fci, WBFL::Units::Measure::KSI) << _T(" ksi"));
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciIncreased);

         config = m_StrandDesignTool->GetSegmentConfiguration();
      }

      // We can continue if we only increase f'ci, but must restart if final was increased
      Float64 fc_new  = m_StrandDesignTool->GetConcreteStrength();
      if ( !IsEqual(fc,fc_new) )
      {
         DLOG(_T("Final Strength Also Increased to ")  << WBFL::Units::ConvertFromSysUnits(fc_new, WBFL::Units::Measure::KSI) << _T(" ksi"));
         LOG_ACTION(_T("Restart Design loop"));
         m_DesignerOutcome.SetOutcome(fc < fc_new ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);
         return;
      }
   }
   else
   {
      DLOG(_T("New release strength is less than current, no need to update"));
   }

   // Now that we've passed bottom compression, look at top tension.
   // for this, we will try to adjust harped strands...
   GET_IFACE2(GetBroker(),IConcreteStressLimits, pLimits );
   // allowable tension is constant across girder, a dummy poi works in this case
   // so we don't have to lookup the allowable every time through the loop below
   pgsPointOfInterest dummyPOI(segmentKey,0.0);
   Float64 allowable_tension = pLimits->GetSegmentConcreteTensionStressLimit(dummyPOI,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension),fci,release_result==ConcSuccessWithRebar?true:false);
   DLOG(_T("Allowable tensile stress after Release     = ") << WBFL::Units::ConvertFromSysUnits(allowable_tension,WBFL::Units::Measure::KSI) << _T(" ksi") );

   bat = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   Float64 ftop = -Float64_Max;
   Float64 fetop, fptop;
   pgsPointOfInterest top_poi;

   for( const pgsPointOfInterest& poi : vPOI)
   {
      CHECK_PROGRESS;

      Float64 mine, maxe;
      pForces->GetStress(releaseIntervalIdx,pgsTypes::ServiceI,poi,bat,false,pgsTypes::TopGirder,&mine,&maxe);

      Float64 fTopPretension = pPrestress->GetStress(releaseIntervalIdx, poi, pgsTypes::TopGirder, false, pgsTypes::ServiceI, INVALID_INDEX, &config);

      Float64 max = maxe+fTopPretension;

      // save max'd stress and corresponding poi
      if (ftop < max)
      {
         ftop    = max;
         fetop   = maxe;
         fptop   = fTopPretension;
         top_poi = poi;
      }
   }

   DLOG(_T("Controlling Stress Demand at Release, Top, Tension = ") << WBFL::Units::ConvertFromSysUnits(ftop,WBFL::Units::Measure::KSI) << _T(" ksi at ")<< WBFL::Units::ConvertFromSysUnits(top_poi.GetDistFromStart(), WBFL::Units::Measure::Feet) << _T(" ft") );

   if (allowable_tension < ftop)
   {
      DLOG(_T("Tension limit exceeded - see what we can do"));

      if (m_StrandDesignTool->IsDesignHarping())
      {
         DLOG(_T("Attempt to adjust harped strands"));
         Float64 pps = m_StrandDesignTool->GetPrestressForceMidZone(releaseIntervalIdx,top_poi);

         // Compute eccentricity required to control top tension
         GET_IFACE2(GetBroker(),ISectionProperties,pSectProp);
         Float64 Ag  = pSectProp->GetAg(releaseIntervalIdx,top_poi);
         Float64 Stg = pSectProp->GetS(releaseIntervalIdx,top_poi,pgsTypes::TopGirder);
         DLOG(_T("Ag  = ") << WBFL::Units::ConvertFromSysUnits(Ag, WBFL::Units::Measure::Inch2) << _T(" in^2"));
         DLOG(_T("Stg = ") << WBFL::Units::ConvertFromSysUnits(Stg,WBFL::Units::Measure::Inch3) << _T(" in^3"));

         Float64 ecc_target = ComputeTopTensionEccentricity( pps, allowable_tension, fetop, Ag, Stg);
         DLOG(_T("Eccentricity Required to control Top Tension   = ") << WBFL::Units::ConvertFromSysUnits(ecc_target, WBFL::Units::Measure::Inch) << _T(" in"));

         // See if eccentricity can be adjusted and keep Final ServiceIII stresses under control
         Float64 min_ecc = m_StrandDesignTool->GetMinimumFinalMidZoneEccentricity();
         DLOG(_T("Min eccentricity for bottom tension at BridgeSite3   = ") << WBFL::Units::ConvertFromSysUnits(min_ecc, WBFL::Units::Measure::Inch) << _T(" in"));

        StrandIndexType Nh = m_StrandDesignTool->GetNh();

         GET_IFACE2(GetBroker(),IStrandGeometry,pStrandGeom);
         Float64 offset_inc = m_StrandDesignTool->GetHarpedHpOffsetIncrement();
         if (0 < Nh && 0.0 <= offset_inc && !options.doForceHarpedStrandsStraight )
         {
            DLOG(_T("Attempt to adjust by raising harped bundles at harping points"));

            Float64 off_reqd = m_StrandDesignTool->ComputeHpOffsetForEccentricity(top_poi, ecc_target,releaseIntervalIdx);
            DLOG(_T("Harped Hp offset required to achieve controlling Eccentricity   = ") << WBFL::Units::ConvertFromSysUnits(off_reqd, WBFL::Units::Measure::Inch) << _T(" in"));

            // round to increment
            off_reqd = CeilOff(off_reqd, offset_inc);
            DLOG(_T("Hp Offset Rounded to increment of ")<<WBFL::Units::ConvertFromSysUnits(offset_inc, WBFL::Units::Measure::Inch) << _T(" in = ") << WBFL::Units::ConvertFromSysUnits(off_reqd, WBFL::Units::Measure::Inch) << _T(" in"));

            // offset could push us out of ServiceIII bounds
            Float64 min_off = m_StrandDesignTool->ComputeHpOffsetForEccentricity(top_poi, min_ecc, lastIntervalIdx);
            DLOG(_T("Offset Required to Create Min Eccentricity Required Final Bottom Tension   = ") << WBFL::Units::ConvertFromSysUnits(min_off, WBFL::Units::Measure::Inch) << _T(" in"));
            if (off_reqd <= min_off)
            {
               // Attempt to set our offset, this may be lowered to the highest allowed location 
               // if it is out of bounds
               m_StrandDesignTool->SetHarpStrandOffsetHp(pgsTypes::metStart,off_reqd);
               m_StrandDesignTool->SetHarpStrandOffsetHp(pgsTypes::metEnd,  off_reqd);
               DLOG(_T("New casting yard eccentricity is ") << WBFL::Units::ConvertFromSysUnits( m_StrandDesignTool->ComputeEccentricity(top_poi,releaseIntervalIdx), WBFL::Units::Measure::Inch) << _T(" in"));
               DLOG(_T("New final eccentricity is ") << WBFL::Units::ConvertFromSysUnits( m_StrandDesignTool->ComputeEccentricity(top_poi,lastIntervalIdx), WBFL::Units::Measure::Inch) << _T(" in"));
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);

               // make sure the job was complete
               Float64 new_off = m_StrandDesignTool->GetHarpStrandOffsetHp(pgsTypes::metStart);
               if (new_off == off_reqd)
               {
                  // Seems like a miracle with all of the conditions around here, but we succeeded
                  DLOG(_T("Strands at HP offset set successfully - Continue Onward"));
                  return;
               }
               else
               {
                  // our offset attempt ran into physical constraints or hold down overload. 
                  DLOG(_T("Offset at HP not fully completed. Perhaps a change in strength can finish the job?"));
               }
            }
            else
            {
               // so close, but offset failed. fallback is to increase concrete strength
               DLOG(_T("Offset Eccentricity has pushed us out of Service allowable zone - Set as high as possible and hope more concrete strength will fix problem"));
               off_reqd = FloorOff(min_off,offset_inc);
               DLOG(_T("Hp Offset Rounded to increment of ")<<WBFL::Units::ConvertFromSysUnits(offset_inc, WBFL::Units::Measure::Inch) << _T(" in = ") << WBFL::Units::ConvertFromSysUnits(off_reqd, WBFL::Units::Measure::Inch) << _T(" in"));

               m_StrandDesignTool->SetHarpStrandOffsetHp(pgsTypes::metStart,off_reqd);
               m_StrandDesignTool->SetHarpStrandOffsetHp(pgsTypes::metEnd,  off_reqd);
               DLOG(_T("New casting yard eccentricity is ") << WBFL::Units::ConvertFromSysUnits( m_StrandDesignTool->ComputeEccentricity(top_poi,releaseIntervalIdx), WBFL::Units::Measure::Inch) << _T(" in"));
               DLOG(_T("New final eccentricity is ") << WBFL::Units::ConvertFromSysUnits( m_StrandDesignTool->ComputeEccentricity(top_poi,lastIntervalIdx), WBFL::Units::Measure::Inch) << _T(" in"));
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
            }
         }
         else
         {
            // TxDOT - non-standard adjustment (Texas Two-Step)
            DLOG(_T("Attempt to trade straight strands for harped to relieve top tension - TxDOT non-standard adjustment"));

            StrandIndexType nh_reqd, ns_reqd;
            if (m_StrandDesignTool->ComputeAddHarpedForMidZoneReleaseEccentricity(top_poi, ecc_target, min_ecc, &ns_reqd, &nh_reqd))
            {
               // number of straight/harped were changed. Set them
               DLOG(_T("Number of Straight/Harped were changed from ")<<m_StrandDesignTool->GetNs()<<_T("/")<<Nh<<_T(" to ")<<ns_reqd<<_T("/")<<nh_reqd);
               m_StrandDesignTool->SetNumStraightHarped(ns_reqd, nh_reqd);

               DLOG(_T("Strands at HP Release set successfully - Continue Onward"));
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
               return;
            }
            else
            {
               LOG_FAIL(_T("Attempt to trade straight strands for harped to relieve top tension failed."));
            }
         }
      }
      else
      {
         DLOG(_T("This is a debond or straight strand design. Adjusting strands in mid-zone is not a remedy"));
      }

      // If we are here,
      DLOG(_T("Only option left is to try to increase release strength to control top tension"));

      Float64 fci_reqd;
      ConcStrengthResultType success = m_StrandDesignTool->ComputeRequiredConcreteStrength(ftop,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension),&fci_reqd);
      if ( success != ConcFailed )
      {
         Float64 fci_old = m_StrandDesignTool->GetReleaseStrength();
         DLOG(_T("Successfully Increased Release Strength for Release , Top, Tension psxfer  = ") << WBFL::Units::ConvertFromSysUnits(fci_reqd,WBFL::Units::Measure::KSI) << _T(" ksi") );
         m_StrandDesignTool->UpdateReleaseStrength(fci_reqd,success,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension),pgsTypes::TopGirder);
         m_DesignerOutcome.SetOutcome(fci_old<fci_reqd ? pgsDesignCodes::FciIncreased : pgsDesignCodes::FciDecreased);

         Float64 fc_new = m_StrandDesignTool->GetConcreteStrength();
         if ( !IsEqual(fc,fc_new) )
         {
            DLOG(_T("However, Final Was Also Increased to ") << WBFL::Units::ConvertFromSysUnits(fc_new,WBFL::Units::Measure::KSI) << _T(" ksi") );
            LOG_ACTION(_T("Restart design with new strengths"));
            m_DesignerOutcome.SetOutcome(fc < fc_new ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);
         }
      }
      else if ( m_StrandDesignTool->AddRaisedStraightStrands() )
      {
         // Attempt to add raised straight strands if this is an option. We could abort if the attempt
         // fails, but give bump 500 a chance if we go down in smoke.
         // If we are here, outer algorithm will restart.
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::RaisedStraightStrands);
         LOG_ACTION(_T("Added Raised Straight Strands - Restart design with new strand configuration"));
      }
      else
      {
         // Last resort, increase strengths by 500 psi and restart
         bool bSuccess = m_StrandDesignTool->Bump500(StressCheckTask(releaseIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension), pgsTypes::TopGirder);
         if (bSuccess)
         {
            LOG_ACTION(_T("Just threw a Hail Mary - Restart design with 500 psi higher concrete strength"));
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciIncreased);
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcIncreased);
         }
         else
         {
            DLOG(_T("Concrete Strength Cannot be adjusted"));
            m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::ReleaseStrength);
            m_DesignerOutcome.AbortDesign();
         }
      }
   }  // ftop>allowable_tension
}

void pgsDesigner2::DesignSlabOffset(std::shared_ptr<IEAFProgress> pProgress) const
{
   DESIGN_LOG_SCOPE(_T("DesignSlabOffset"));
   GET_IFACE2_NOCHECK(GetBroker(),ISpecification,pSpec);
   GET_IFACE2(GetBroker(),IBridge,pBridge);
   if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
   {
      DLOG(_T("Skipping A-dimension design because there is no deck"));
      // no deck
      return;
   }

   const CSegmentKey& segmentKey = m_StrandDesignTool->GetSegmentKey();

   Float64 AorigStart = m_StrandDesignTool->GetSlabOffset(pgsTypes::metStart);
   Float64 AorigEnd   = m_StrandDesignTool->GetSlabOffset(pgsTypes::metEnd);

   Float64 assumedExcessCamberOrig = m_StrandDesignTool->GetAssumedExcessCamber();

   // Iterate on _T("A") dimension and initial number of prestressing strands
   // Use a relaxed tolerance on _T("A") dimension.
   Int16 cIter = 0;
   Int16 nIterMax = 20;
   bool bDone = false;

   // Iterate until we come up with an _T("A") dimension and some strands
   // that are consistent for the current values of f'c and f'ci
   DLOG(_T("Computing A-dimension requirement"));
   DLOG(_T("A-dim Current (Start)   = ") << WBFL::Units::ConvertFromSysUnits(AorigStart, WBFL::Units::Measure::Inch) << _T(" in") );
   DLOG(_T("A-dim Current (End)     = ") << WBFL::Units::ConvertFromSysUnits(AorigEnd,   WBFL::Units::Measure::Inch) << _T(" in") );
   if (m_StrandDesignTool->IsDesignExcessCamber())
   {
      DLOG(_T("AssumedExcessCamber Current    = ") << WBFL::Units::ConvertFromSysUnits(assumedExcessCamberOrig,   WBFL::Units::Measure::Inch) << _T(" in") );
   }
   
   // to prevent the design from bouncing back and forth over two "A" dimensions that are 1/4" apart, we are going to use the
   // raw computed "A" requirement and round it after design is complete.
   // use a somewhat tight tolerance to converge of the theoretical "A" dimension
   Float64 tolerance = WBFL::Units::ConvertToSysUnits(0.125, WBFL::Units::Measure::Inch);
   do
   {
      CHECK_PROGRESS;

      std::_tostringstream os2;
      os2 << _T("Slab Offset Design Iteration ")<<cIter+1 << std::ends;
      pProgress->UpdateMessage(os2.str().c_str());
      DLOG(os2.str().c_str());

      Float64 AoldStart = m_StrandDesignTool->GetSlabOffset(pgsTypes::metStart);
      Float64 AoldEnd   = m_StrandDesignTool->GetSlabOffset(pgsTypes::metEnd);

      Float64 assumedExcessCamberOld = m_StrandDesignTool->GetAssumedExcessCamber();

      // Make a guess at the "A" dimension using this initial strand configuration
      SLABOFFSETDETAILS slab_offset_details;
      GDRCONFIG config = m_StrandDesignTool->GetSegmentConfiguration();
      config.SlabOffset[pgsTypes::metStart] = AoldStart;
      config.SlabOffset[pgsTypes::metEnd]   = AoldEnd;
      config.AssumedExcessCamber = assumedExcessCamberOld;
      GetSlabOffsetDetails(segmentKey,&config,&slab_offset_details);

      IndexType idx = slab_offset_details.SlabOffset.size()/2;
      ATLASSERT(slab_offset_details.SlabOffset[idx].PointOfInterest.IsMidSpan(POI_ERECTED_SEGMENT));
      DLOG(_T("Girder Orientation Effect = ") << WBFL::Units::ConvertFromSysUnits(slab_offset_details.SlabOffset[idx].GirderOrientationEffect, WBFL::Units::Measure::Inch) << _T(" in"));
      DLOG(_T("Profile Effect = ") << WBFL::Units::ConvertFromSysUnits(slab_offset_details.SlabOffset[idx].ProfileEffect, WBFL::Units::Measure::Inch) << _T(" in"));
      DLOG(_T("D = ") << WBFL::Units::ConvertFromSysUnits(slab_offset_details.SlabOffset[idx].D, WBFL::Units::Measure::Inch) << _T(" in"));
      DLOG(_T("C = ") << WBFL::Units::ConvertFromSysUnits(slab_offset_details.SlabOffset[idx].C, WBFL::Units::Measure::Inch) << _T(" in"));
      DLOG(_T("Camber Effect = ") << WBFL::Units::ConvertFromSysUnits(slab_offset_details.SlabOffset[idx].CamberEffect, WBFL::Units::Measure::Inch) << _T(" in"));
      DLOG(_T("A-dim Calculated (raw) = ") << WBFL::Units::ConvertFromSysUnits(slab_offset_details.RequiredMaxSlabOffsetRaw, WBFL::Units::Measure::Inch) << _T(" in"));

      Float64 Anew = slab_offset_details.RequiredMaxSlabOffsetRaw;

      Float64 Amin = m_StrandDesignTool->GetMinimumSlabOffset();
      if (Anew < Amin)
      {
         DLOG(_T("Calculated A-dim is less than minimum. Using minimum = ") << WBFL::Units::ConvertFromSysUnits(Amin, WBFL::Units::Measure::Inch) << _T(" in"));
         Anew = Amin;
      }

      if ( IsZero( AoldStart - Anew, tolerance ) && IsZero( AoldEnd - Anew, tolerance ))
      {
         Float64 a;
         a = RoundSlabOffsetValue(pSpec, Max(AoldStart, AoldEnd, Anew) );
         m_StrandDesignTool->SetSlabOffset( pgsTypes::metStart, a );
         m_StrandDesignTool->SetSlabOffset( pgsTypes::metEnd,   a );
         DLOG(_T("A-dim camber converged."));

         bDone = true;
      }
      else
      {
         m_StrandDesignTool->SetSlabOffset( pgsTypes::metStart, Anew );
         m_StrandDesignTool->SetSlabOffset( pgsTypes::metEnd,   Anew );
      }

      if (m_StrandDesignTool->IsDesignExcessCamber())
      {
         // ctoler serves two roles here: it is the acceptance criterion - the assumed camber is
         // good enough when it sits within ctoler of the computed camber, the same test the
         // haunch geometry check applies - and it is the increment the delivered value is
         // rounded to, 1/2 in being the AEC output standard for this number. 
         //
         // Known limitation. Converging on "within tolerance" rather than "closest increment"
         // lets the delivered camber sit almost a full ctoler away from the computed camber,
         // and the haunch geometry spec check spends that same budget. Rounding the computed
         // value instead - what this code did before - held the gap to half an increment and so
         // nearly always passed that check, but it often did not converge.
         //
         // The exposure is worst where the assumed camber does more than set the haunch load.
         // Under HaunchAnalysisSectionPropertiesType == hspDetailedDescription
         // (IsAssumedExcessCamberForSectProps) the haunch depth follows a parabola fitted to the
         // slab offset and the assumed camber, so the camber also sets the composite section
         // properties along the girder. The computed camber then answers to a stiffness feedback
         // on top of the load feedback, which makes the map steeper and less stable - and with
         // transformed section properties the camber this loop computes and the camber the spec
         // check reports diverge further still. Tx54_ParabolicTransformed span 1 girder 2 designs
         // successfully and then failed the excess camber check for exactly this reason: it
         // settles at an assumed 1.5 in against a computed 1.11837 in, spending 0.382 of the
         // 0.5 in budget before the check has looked at it.
         //
         // Note also that m_bIsDesignExcessCamber (StrandDesignTool) keys off
         // IsAssumedExcessCamberForLoad() alone. A project that uses the assumed camber for
         // section properties but not for haunch load never designs the value here at all.
         Float64 ctoler = m_StrandDesignTool->GetAssumedExcessCamberTolerance();
         Float64 computed_camber = slab_offset_details.SlabOffset.at(idx).CamberEffect;
         DLOG(_T("Excess Camber Computed = ") << WBFL::Units::ConvertFromSysUnits(computed_camber, WBFL::Units::Measure::Inch) << _T(" in"));
         if (IsZero(assumedExcessCamberOld - computed_camber, ctoler))
         {
            // The assumed camber satisfies the tolerance criterion - the same criterion the
            // haunch geometry check applies - so it is an acceptable answer. Keep it rather
            // than replacing it with RoundOff(computed_camber): that asks the camber to be a
            // fixed point on the rounding increment, and one does not always exist. When the
            // true fixed point falls near a bin boundary the two adjacent increments map to
            // each other (e.g., assume 1.5 -> compute 1.93 -> store 2.0 -> compute 1.62 -> store 1.5)
            // and the design never settles, even though both values are within tolerance.
            Float64 c = RoundOff(assumedExcessCamberOld, ctoler);
            if (IsEqual(c, assumedExcessCamberOld))
            {
               DLOG(_T("Excess camber converged."));

               bDone &= true;
            }
            else
            {
               // Within tolerance, but not on the rounding increment - this only happens when
               // the design started from an off-increment value. Move onto the increment and
               // verify there.
               m_StrandDesignTool->SetAssumedExcessCamber(c);
               DLOG(_T("Excess camber is within tolerance but not on the rounding increment."));
               bDone = false;
            }
         }
         else
         {
            // Store the computed camber rounded to the increment, not the raw value. Every
            // value the design is verified at is then a value that can be delivered, so the
            // haunch load used for design is the haunch load the final configuration has.
            m_StrandDesignTool->SetAssumedExcessCamber(RoundOff(computed_camber, ctoler));
            DLOG(_T("Excess camber does not match within tolerance."));
            bDone = false;
         }
      }

   } while ( !bDone && cIter++ < nIterMax);

   if ( nIterMax < cIter )
   {
      LOG_ABORT(_T("Maximum number of iterations was exceeded - aborting Slab offset design ") << cIter);
      m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::MaxIterExceeded);
      m_DesignerOutcome.AbortDesign();
   }

   if (bDone)
   {
      Float64 AnewStart = m_StrandDesignTool->GetSlabOffset(pgsTypes::metStart);
      Float64 AnewEnd   = m_StrandDesignTool->GetSlabOffset(pgsTypes::metEnd);

      // don't let the new A be much larger than the old, or lots less
      if (  ( AorigStart < (AnewStart - tolerance) || (AnewStart + 2.0*tolerance) < AorigStart ) ||
            ( AorigEnd   < (AnewEnd   - tolerance) || (AnewEnd   + 2.0*tolerance) < AorigEnd   ) )
      {
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::SlabOffsetChanged);
      }
   }
}

void pgsDesigner2::DesignMidZoneInitialStrands(bool bUseCurrentStrands, std::shared_ptr<IEAFProgress> pProgress) const
{
   DESIGN_LOG_SCOPE(_T("DesignMidZoneInitialStrands"));
   // Figure out the number of strands required to make the prestressing
   // work at the bottom centerline of the span at ServiceIII limit state,
   // using the current values for "A", f'c, and f'ci.

   // The only way to continue to the next step from this function is to have adequate concrete
   // strength and the minimum number of strands for tension to control at mid-span

   DLOG(_T("Computing initial prestressing requirements for Service in Mid-Zone"));

   const CSegmentKey& segmentKey = m_StrandDesignTool->GetSegmentKey();

   GET_IFACE2(GetBroker(),IIntervals, pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType constructionLoadIntervalIdx = pIntervals->GetConstructionLoadInterval();
   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();
   IntervalIndexType castShearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();
   IntervalIndexType castLongitudinalJointIntervalIdx = pIntervals->GetCastLongitudinalJointInterval();
   IntervalIndexType noncompositeUserLoadIntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();
   IntervalIndexType compositeUserLoadIntervalIdx = pIntervals->GetCompositeUserLoadInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

   // Get some information about the girder
   GET_IFACE2(GetBroker(),IBridge, pBridge);

   GET_IFACE2(GetBroker(),ISegmentData, pSegmentData);
   const CGirderMaterial* pGirderMaterial = pSegmentData->GetSegmentMaterial(segmentKey);

   // Get controlling Point of Interest at mid zone
   pgsPointOfInterest poi = GetControllingFinalMidZonePoi(segmentKey);

   GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);
   IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);

   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(deckCastingRegionIdx);

   const auto& config = m_StrandDesignTool->GetSegmentConfiguration();

   // Get the section properties of the girder
   GET_IFACE2(GetBroker(),ISectionProperties, pSectProp);
   Float64 Ag = pSectProp->GetAg(releaseIntervalIdx, poi);
   Float64 Stg = pSectProp->GetS(releaseIntervalIdx, poi, pgsTypes::TopGirder);
   Float64 Sbg = pSectProp->GetS(releaseIntervalIdx, poi, pgsTypes::BottomGirder);
   DLOG(_T("Ag  = ") << WBFL::Units::ConvertFromSysUnits(Ag, WBFL::Units::Measure::Inch2) << _T(" in^2"));
   DLOG(_T("Stg = ") << WBFL::Units::ConvertFromSysUnits(Stg, WBFL::Units::Measure::Inch3) << _T(" in^3"));
   DLOG(_T("Sbg = ") << WBFL::Units::ConvertFromSysUnits(Sbg, WBFL::Units::Measure::Inch3) << _T(" in^3"));

   DLOG(_T("Stcg = ") << WBFL::Units::ConvertFromSysUnits(pSectProp->GetS(lastIntervalIdx, poi, pgsTypes::TopGirder), WBFL::Units::Measure::Inch3) << _T(" in^3"));
   DLOG(_T("Sbcg = ") << WBFL::Units::ConvertFromSysUnits(pSectProp->GetS(lastIntervalIdx, poi, pgsTypes::BottomGirder), WBFL::Units::Measure::Inch3) << _T(" in^3"));

   DLOG(_T("Stcg_adjusted = ") << WBFL::Units::ConvertFromSysUnits(pSectProp->GetS(lastIntervalIdx, poi, pgsTypes::TopGirder, &config), WBFL::Units::Measure::Inch3) << _T(" in^3"));
   DLOG(_T("Sbcg_adjusted = ") << WBFL::Units::ConvertFromSysUnits(pSectProp->GetS(lastIntervalIdx, poi, pgsTypes::BottomGirder, &config), WBFL::Units::Measure::Inch3) << _T(" in^3"));

   GET_IFACE2(GetBroker(),IProductForces, pProductForces);
   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   PierIndexType startPierIdx, endPierIdx;
   pBridge->GetGirderGroupPiers(segmentKey.groupIndex, &startPierIdx, &endPierIdx);
   ATLASSERT(endPierIdx == startPierIdx + 1);

   if (m_StrandDesignTool->IsDesignSlabOffset())
   {
      DLOG(_T("Bridge A dimension  (Start) = ") << WBFL::Units::ConvertFromSysUnits(pBridge->GetSlabOffset(segmentKey,pgsTypes::metStart),WBFL::Units::Measure::Inch) << _T(" in"));
      DLOG(_T("Bridge A dimension  (End)   = ") << WBFL::Units::ConvertFromSysUnits(pBridge->GetSlabOffset(segmentKey,pgsTypes::metEnd),WBFL::Units::Measure::Inch) << _T(" in"));
      DLOG(_T("Current A dimension (Start) = ") << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool->GetSlabOffset(pgsTypes::metStart),WBFL::Units::Measure::Inch) << _T(" in"));
      DLOG(_T("Current A dimension (End)   = ") << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool->GetSlabOffset(pgsTypes::metEnd),WBFL::Units::Measure::Inch) << _T(" in"));
   }
   DLOG(_T("M girder      = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(erectSegmentIntervalIdx, pgsTypes::pftGirder, poi, bat, rtCumulative), WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));
   DLOG(_T("M diaphragm   = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(castDiaphragmIntervalIdx, pgsTypes::pftDiaphragm, poi, bat, rtIncremental), WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));
   if (castLongitudinalJointIntervalIdx != INVALID_INDEX)
   {
      DLOG(_T("M longitudinal joint = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(castLongitudinalJointIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, bat, rtIncremental), WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));
   }

   if (castShearKeyIntervalIdx != INVALID_INDEX)
   {
      DLOG(_T("M shear key   = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(castShearKeyIntervalIdx, pgsTypes::pftShearKey, poi, bat, rtIncremental), WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));
   }

   DLOG(_T("M construction= ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(constructionLoadIntervalIdx, pgsTypes::pftConstruction, poi, bat, rtIncremental), WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));
   
   if (castDeckIntervalIdx != INVALID_INDEX)
   {
      DLOG(_T("M slab        = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlab, poi, bat, rtIncremental), WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));
      DLOG(_T("dM slab       = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetDesignSlabMomentAdjustment(poi, &m_StrandDesignTool->GetSegmentConfiguration()), WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));
      DLOG(_T("M slab pad    = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlabPad, poi, bat, rtIncremental), WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));
      DLOG(_T("dM slab pad   = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetDesignSlabPadMomentAdjustment(poi, &m_StrandDesignTool->GetSegmentConfiguration()), WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));
      DLOG(_T("M panel       = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlabPanel, poi, bat, rtIncremental), WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));
   }
   DLOG(_T("M user dc (1) = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(noncompositeUserLoadIntervalIdx,pgsTypes::pftUserDC,poi,bat, rtIncremental),WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));
   DLOG(_T("M user dw (1) = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(noncompositeUserLoadIntervalIdx,pgsTypes::pftUserDW,poi,bat, rtIncremental),WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));
   DLOG(_T("M barrier     = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(railingSystemIntervalIdx,pgsTypes::pftTrafficBarrier,poi,bat, rtIncremental),WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));
   DLOG(_T("M sidewalk    = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(railingSystemIntervalIdx,pgsTypes::pftSidewalk      ,poi,bat, rtIncremental),WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));
   DLOG(_T("M user dc (2) = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(compositeUserLoadIntervalIdx,pgsTypes::pftUserDC,poi,bat, rtIncremental),WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));
   DLOG(_T("M user dw (2) = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(compositeUserLoadIntervalIdx,pgsTypes::pftUserDW,poi,bat, rtIncremental),WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));
   DLOG(_T("M overlay     = ") << (overlayIntervalIdx==INVALID_INDEX ? 0.0 : WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(overlayIntervalIdx,pgsTypes::pftOverlay,poi,bat, rtIncremental),WBFL::Units::Measure::KipFeet)) << _T(" kip-ft"));

#if defined ENABLE_DESIGN_LOGGING
   if (pgsDesignLog::IsEnabled())
   {
   Float64 Mllmax, Mllmin;
   pProductForces->GetLiveLoadMoment(lastIntervalIdx,pgsTypes::lltDesign,poi,bat,true,false,&Mllmin,&Mllmax);
   DLOG(_T("M ll+im min   = ") << WBFL::Units::ConvertFromSysUnits(Mllmin,WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));
   DLOG(_T("M ll+im max   = ") << WBFL::Units::ConvertFromSysUnits(Mllmax,WBFL::Units::Measure::KipFeet) << _T(" kip-ft"));

   //Float64 fc_lldf = fcgdr;
   //if ( pGirderMaterial->Concrete.bUserEc )
   //{
   //   fc_lldf = WBFL::LRFD::ConcreteUtil::FcFromEc( (WBFL::Materials::ConcreteType)(pGirderMaterial->Concrete.Type), pGirderMaterial->Concrete.Ec, pGirderMaterial->Concrete.StrengthDensity );
   //}

   GET_IFACE2(GetBroker(),ILiveLoadDistributionFactors,pLLDF);
   Float64 gV, gpM, gnM;
   pLLDF->GetDistributionFactors(poi,pgsTypes::StrengthI,&gpM,&gnM,&gV,&config);
   DLOG(_T("LLDF = ") << gpM);
   }
#endif

   // Initial potential controlling design cases during service
   GET_IFACE2(GetBroker(),IConcreteStressLimits,pAllowStress);
   GET_IFACE2(GetBroker(),ILimitStateForces,pForces);
   std::vector<InitialDesignParameters> vInitialDesignParameters;
   // In the past, we looked at these cases, but that was a mistake. The design strategy is to determine the number of strands required to satisfy the
   // tension limits. We never manipulate number of strands to satisfy compression. Compression limits are satisfied by changing f'ci/f'c
   // The following 3 lines are commented out because we don't want to look at the compression cases but left here as a reminder of what we don't want to do.
   //vInitialDesignParameters.push_back(InitialDesignParameters(lastIntervalIdx, true /*with live load*/,  pgsTypes::ServiceI, _T("Service I"), pgsTypes::TopGirder, _T("Top"), pgsTypes::Compression)); // 0.6f'c
   //vInitialDesignParameters.push_back(InitialDesignParameters(lastIntervalIdx, false /*without live load*/, pgsTypes::ServiceI, _T("Service I"), pgsTypes::TopGirder, _T("Top"), pgsTypes::Compression)); // 0.45f'c
   //vInitialDesignParameters.push_back(InitialDesignParameters(lastIntervalIdx, true /*with live load*/,  WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims ? pgsTypes::ServiceIA : pgsTypes::FatigueI,WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims ? _T("Service IA") : _T("Fatigue I"),pgsTypes::TopGirder,_T("Top"),pgsTypes::Compression));
   vInitialDesignParameters.push_back(InitialDesignParameters(lastIntervalIdx, true /*with live load*/,  pgsTypes::ServiceIII,_T("Service III"),pgsTypes::BottomGirder,_T("Bottom"),pgsTypes::Tension));
   if ( pAllowStress->CheckFinalDeadLoadTensionStress() )
   {
      vInitialDesignParameters.push_back(InitialDesignParameters(lastIntervalIdx,false /*without live load*/,pgsTypes::ServiceI,_T("Service I"),pgsTypes::BottomGirder,_T("Bottom"),pgsTypes::Tension));
   }
   
   GET_IFACE2(GetBroker(),ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

   for(auto& designParams : vInitialDesignParameters)
   {
      pForces->GetDesignStress(designParams.task,poi,designParams.stress_location,&config,bat,&designParams.fmin,&designParams.fmax);

      Float64 f_demand = ( designParams.task.stressType == pgsTypes::Compression ) ? designParams.fmin : designParams.fmax;
      DLOG(_T("Stress Demand (") << pIntervals->GetDescription(designParams.task.intervalIdx) << _T(", ") << designParams.strLimitState << _T(", ") << designParams.strStressLocation << _T(", mid-span) = ") << WBFL::Units::ConvertFromSysUnits(f_demand,WBFL::Units::Measure::KSI) << _T(" ksi") );

      // Get allowable stress 
      ATLASSERT(designParams.task.stressType == pgsTypes::Tension);
      designParams.fLimit = pAllowStress->GetSegmentConcreteTensionStressLimit(poi,designParams.task,m_StrandDesignTool->GetConcreteStrength(),false);
      DLOG(_T("Allowable stress (") << designParams.strLimitState << _T(") = ") << WBFL::Units::ConvertFromSysUnits(designParams.fLimit,WBFL::Units::Measure::KSI)  << _T(" ksi"));

      // Compute required stress due to prestressing
      Float64 k = pLoadFactors->GetDCMax(designParams.task.limitState);
      designParams.fpre = IsZero(k) ? 0 : (designParams.fLimit - f_demand)/k;

      DLOG(_T("Reqd stress due to prestressing (") << designParams.strLimitState << _T(") = ") << WBFL::Units::ConvertFromSysUnits(designParams.fpre,WBFL::Units::Measure::KSI) << _T(" ksi") );
   }

   // Guess the number of strands if first time through. otherwise use previous guess
   if ( bUseCurrentStrands )
   {
      // Not the first time through. 
      // We could be here because concrete strength increased and because of that, we may need less strands.
      // The design algorithm can overshoot np because eccentricity typically reduces with increased strands.
      // So, reduce to the next available if possible.
      StrandIndexType np = m_StrandDesignTool->GetNumPermanentStrands();

      StrandIndexType npmin = Max((StrandIndexType)3, m_StrandDesignTool->GetMinimumPermanentStrands());

      if (npmin < np)
      {
         np = m_StrandDesignTool->GetPreviousNumPermanentStrands(np);
         DLOG(_T("Reducing num permanent strands from ") << m_StrandDesignTool->GetNumPermanentStrands() << _T(" to ") << np);
         ATLASSERT(0 < np);
         m_StrandDesignTool->SetNumPermanentStrands(np);
      }
   }
   else
   {
      // uses minimal number of strands
      m_StrandDesignTool->GuessInitialStrands();
   }

   // Safety net
   StrandIndexType Np = INVALID_INDEX, Np_old = INVALID_INDEX;
   Int16 cIter = 0;
   Int16 maxIter = 80;

   // Use controller class to keep design from getting off track
   StrandDesignController designController(m_StrandDesignTool);

   do
   {
      CHECK_PROGRESS;

      DESIGN_LOG_SCOPE(_T("Strand configuration trial ") << cIter);

      DLOG(_T("Reset end-zone strands maximize harping or debonding effect"));
      if (!m_StrandDesignTool->ResetEndZoneStrandConfig())
      {
         LOG_FAIL(_T("ERROR - Could not reset end-zone offsets to maximize differential"));
         // this error is not very descriptive, but it probably means that there is no way for the strands to fit 
         // within offset bounds. This should have been caught in the library
         m_DesignerOutcome.AbortDesign();
         m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::TooManyStrandsReqd);
         ATLASSERT(false);
         return;
      }

      DLOG(_T("Guess at number of strands -> Ns = ") << m_StrandDesignTool->GetNs() << _T(" Nh = ") << m_StrandDesignTool->GetNh() << _T(" Nt = ") << m_StrandDesignTool->GetNt());
      m_StrandDesignTool->DumpDesignParameters();

      // Compute prestress force required to achieve fpre to satisfy the tension limits
      Float64 fNreqd = -Float64_Max;
      Float64 ecc = 0;
      const InitialDesignParameters* pControllingParams = nullptr;
      for( auto& designParams : vInitialDesignParameters)
      {
         Float64 thisEcc = m_StrandDesignTool->ComputeEccentricity(poi, designParams.task.intervalIdx);
         DLOG(_T("Eccentricity at mid-span = ") << WBFL::Units::ConvertFromSysUnits(thisEcc, WBFL::Units::Measure::Inch) << _T(" in"));

         DLOG(_T("Determine required prestressing for ") << designParams.strLimitState);

         DLOG(_T("Required prestress force, P = fpre / [1/Ag + ecc/S]"));
         Float64 S = (designParams.stress_location == pgsTypes::TopGirder ? Stg : Sbg);
         designParams.Preqd = designParams.fpre / (1.0 / Ag + thisEcc / S);
         DLOG(_T("Required prestress force (") << designParams.strLimitState << _T(") = ") << WBFL::Units::ConvertFromSysUnits(designParams.fpre, WBFL::Units::Measure::KSI) << _T("/[ 1/") << WBFL::Units::ConvertFromSysUnits(Ag, WBFL::Units::Measure::Inch2) << _T(" + ") << WBFL::Units::ConvertFromSysUnits(thisEcc, WBFL::Units::Measure::Inch) << _T("/") << WBFL::Units::ConvertFromSysUnits(S, WBFL::Units::Measure::Inch3) << _T("] = ") << WBFL::Units::ConvertFromSysUnits(-designParams.Preqd, WBFL::Units::Measure::Kip) << _T(" kip"));
         m_StrandDesignTool->ComputePermanentStrandsRequiredForPrestressForce(poi, &designParams);
         DLOG(_T("Required number of strands = ") << designParams.fN << _T(" (") << designParams.Np << _T(")"));
         if (fNreqd < designParams.fN)
         {
            fNreqd = designParams.fN;
            pControllingParams = &designParams;
            ecc = thisEcc;
         }

      }

      DLOG(_T("Required prestress force = ") << WBFL::Units::ConvertFromSysUnits(-pControllingParams->Preqd, WBFL::Units::Measure::Kip) << _T(" kip"));

      Np = 0.0 < pControllingParams->fN ? pControllingParams->Np : 0; // Np is unsigned - don't let negative conversion cause problems

      // see if we can match Np and ecc
      if ( Np == INVALID_INDEX )
      {
         StrandIndexType npmax = m_StrandDesignTool->GetMaxPermanentStrands();
         if (m_StrandDesignTool->GetNumPermanentStrands()==npmax)
         {
            LOG_ABORT(_T("TOO MANY STRANDS REQUIRED - already tried max= ")<<npmax);

            // OK, This is a final gasp - we have maxed out strands, now see if we can get a reasonable concrete strength
            //     to relieve tension before puking
            DLOG(_T("Hail Mary - See if reasonable concrete strength can satisfy tension limit"));
            const GDRCONFIG& config = m_StrandDesignTool->GetSegmentConfiguration();
            GET_IFACE2(GetBroker(),IPretensionStresses,pPsStress);
            Float64 fBotPre = pPsStress->GetStress(pControllingParams->task.intervalIdx, poi, pControllingParams->stress_location, pControllingParams->task.bIncludeLiveLoad, pControllingParams->task.limitState, INVALID_INDEX, &config);
            Float64 k = pLoadFactors->GetDCMax(pControllingParams->task.limitState);
            Float64 f_allow_required = pControllingParams->fmax+k*fBotPre;
            DLOG(_T("Required allowable = fb ") << pControllingParams->strLimitState << _T(" + fb Prestress = ") << WBFL::Units::ConvertFromSysUnits(pControllingParams->fmax,WBFL::Units::Measure::KSI) << _T(" + ") << WBFL::Units::ConvertFromSysUnits(fBotPre,WBFL::Units::Measure::KSI) << _T(" = ") << WBFL::Units::ConvertFromSysUnits(f_allow_required,WBFL::Units::Measure::KSI) << _T(" ksi"));
            Float64 fc_rqd;
            if ( ConcFailed != m_StrandDesignTool->ComputeRequiredConcreteStrength(f_allow_required, pControllingParams->task,&fc_rqd) )
            {
               // Use user-defined practical upper limit here
               Float64 max_girder_fc = m_StrandDesignTool->GetMaximumConcreteStrength();
               DLOG(_T("User-defined upper limit for final girder concrete = ") << WBFL::Units::ConvertFromSysUnits(max_girder_fc,WBFL::Units::Measure::KSI) << _T(" ksi. Computed required strength = ")<< WBFL::Units::ConvertFromSysUnits(fc_rqd,WBFL::Units::Measure::KSI) << _T(" ksi"));

               if (fc_rqd <= max_girder_fc)
               {
                  Float64 fc_old = m_StrandDesignTool->GetConcreteStrength();

                  bool bFcUpdated = m_StrandDesignTool->UpdateConcreteStrength(fc_rqd,StressCheckTask(lastIntervalIdx,pgsTypes::ServiceIII,pgsTypes::Tension),pgsTypes::BottomGirder);
                  if ( bFcUpdated )
                  {
                     Float64 fc_new = m_StrandDesignTool->GetConcreteStrength();
                     m_DesignerOutcome.SetOutcome(fc_old < fc_new ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);

                     // Tricky: Use concrete growth relationship for this case:
                     // Many times the reason we are not converging here is high initial losses due to a low f'ci
                     // Don't allow f'ci to be more than 2ksi smaller than final (TxDOT research supports this value)
                     Float64 fc_curr = m_StrandDesignTool->GetConcreteStrength();
                     ConcStrengthResultType strength_result;
                     Float64 fci_curr = m_StrandDesignTool->GetReleaseStrength(&strength_result);
                     Float64 fc_2k = WBFL::Units::ConvertToSysUnits(2.0,WBFL::Units::Measure::KSI); // add one to protect lt
                     if (fc_curr-fci_curr > fc_2k)
                     {
                        Float64 fci_max  = m_StrandDesignTool->GetMaximumReleaseStrength();
                        Float64 fci = Min(fci_max, fci_curr+fc_2k);
                        DLOG(_T("  Release strength was more than 2 ksi smaller than final, bump release as well"));
                        bool didchg = m_StrandDesignTool->UpdateReleaseStrength(fci, strength_result, pControllingParams->task, pControllingParams->stress_location);
                        if (didchg)
                        {
                           m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciIncreased);
                        }
                     }

                     LOG_ACTION(_T("Hail Mary to increase final concrete for tension succeeded - restart design"));
                     return;
                  }
               }
            }

            LOG_ABORT(_T("Hail Mary - FAILED!! There is no way to satisfy tension limit unless outer loop can fix this problem"));
            m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::TooManyStrandsReqd);
            m_DesignerOutcome.AbortDesign();
            return;
         }
         else
         {
            LOG_ACTION(_T("TOO MANY STRANDS REQUIRED, but let's try the max before we give up: ")<<npmax);
            Np = npmax;
         }
      }

      StrandIndexType np_min = m_StrandDesignTool->GetMinimumPermanentStrands();
      if (Np < np_min)
      {
         Np = np_min;
         DLOG(_T("Number of strands computed is less than minimum set for Ultimate Moment. Setting to ")<<Np);
      }

      Np_old = m_StrandDesignTool->GetNumPermanentStrands();

      // Controller - can change number of strands if we are bifurcating
      StrandIndexType npchg;
      StrandDesignController::strUpdateResult updateResult = designController.DoUpdate(Np, Np_old, &npchg );
      DLOG(_T("StrandDesignController update result = ")<< updateResult <<_T(" Np = ")<<Np <<_T(" Npchg = ")<<npchg);
      Np = npchg;

      // set number of permanent strands
      if (m_StrandDesignTool->SetNumPermanentStrands(Np))
      {
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
      }
      else
      {
         LOG_ABORT(_T("Error trying to set permanent strands - Abort Design"));
         m_DesignerOutcome.AbortDesign();
         return;
      }

      DLOG(_T("Np = ") << Np_old << _T(" NpGuess = ") << Np);
      DLOG(_T("NsGuess = ") << m_StrandDesignTool->GetNs());
      DLOG(_T("NhGuess = ") << m_StrandDesignTool->GetNh());
      DLOG(_T("NtGuess = ") << m_StrandDesignTool->GetNt());
      DLOG(_T("End of strand configuration trial # ") << cIter <<_T(", Tension controlled"));

      if (updateResult == StrandDesignController::struConverged)
      {
         // solution has converged - compute and save the minimum eccentricity that we can have with
         // Np and our allowable. This will be used later to limit strand adjustments in mid-zone
         // We know that Service III controlled because we are here:
         Float64 pps = m_StrandDesignTool->GetPrestressForceMidZone(pControllingParams->task.intervalIdx,poi);
         Float64 ecc_min = ComputeBottomCompressionEccentricity( pps, pControllingParams->fLimit, pControllingParams->fmax, Ag, Sbg);
         DLOG(_T("Minimum eccentricity Required to control Bottom Tension  = ") << WBFL::Units::ConvertFromSysUnits(ecc_min, WBFL::Units::Measure::Inch) << _T(" in"));
         DLOG(_T("Actual current eccentricity   = ") << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool->ComputeEccentricity(poi, pControllingParams->task.intervalIdx), WBFL::Units::Measure::Inch) << _T(" in"));
         m_StrandDesignTool->SetMinimumFinalMidZoneEccentricity(ecc_min);
         break;
      }
      else if (updateResult == StrandDesignController::struUpdateFailed)
      {
         LOG_FAIL(_T("Strand controller update failed - Number of strands could not be found"));
         m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::TooManyStrandsReqd);
         m_DesignerOutcome.AbortDesign();
         return;
      }

      cIter++;
   } while ( cIter < maxIter );

   if ( maxIter <= cIter )
   {
      LOG_ABORT(_T("Maximum number of iterations was exceeded - aborting design ") << cIter);
      m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::MaxIterExceeded);
      m_DesignerOutcome.AbortDesign();
   }

   DLOG(cIter << _T(" iterations were used"));

   DLOG(_T("Preliminary Design"));
   DLOG(_T("Ns = ") << m_StrandDesignTool->GetNs() << _T(" PjS = ") << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool->GetPjackStraightStrands(),WBFL::Units::Measure::Kip) << _T(" kip"));
   DLOG(_T("Nh = ") << m_StrandDesignTool->GetNh() << _T(" PjH = ") << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool->GetPjackHarpedStrands(),WBFL::Units::Measure::Kip) << _T(" kip"));
   DLOG(_T("Nt = ") << m_StrandDesignTool->GetNt() << _T(" PjT = ") << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool->GetPjackTempStrands(),WBFL::Units::Measure::Kip) << _T(" kip"));
   DLOG(_T("Preliminary Design Complete"));
   // Done
}

pgsPointOfInterest pgsDesigner2::GetControllingFinalMidZonePoi(const CSegmentKey& segmentKey) const
{
   // find location in mid-zone with max stress due to Service III tension
   GET_IFACE2(GetBroker(),IBridge,pBridge);
   GET_IFACE2(GetBroker(),ISpecification,pSpec);

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   Float64 gl = pBridge->GetSegmentLength(segmentKey);
   Float64 lhp, rhp;
   m_StrandDesignTool->GetMidZoneBoundaries(&lhp, &rhp);

   Float64 left_limit = lhp;
   Float64 rgt_limit  = rhp;
   if ( IsEqual(lhp,rhp) )
   {
      left_limit = 0.4*gl;
      rgt_limit  = 0.6*gl;
   }

   const GDRCONFIG& config = m_StrandDesignTool->GetSegmentConfiguration();

   GET_IFACE2(GetBroker(),ILimitStateForces,pForces);
   PoiList vPoi;
   m_StrandDesignTool->GetDesignPoi(lastIntervalIdx, POI_ERECTED_SEGMENT, &vPoi);
   ATLASSERT(0 < vPoi.size());

   Float64 fmax = -Float64_Max;
   pgsPointOfInterest max_poi;
   bool found=false;
   for( const pgsPointOfInterest& poi : vPoi)
   {
      Float64 Xpoi = poi.GetDistFromStart();

      if ( ::InRange(left_limit,Xpoi,rgt_limit) )
      {
         // poi is in mid-zone
         pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Envelope ? pgsTypes::MaxSimpleContinuousEnvelope : (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan));
         Float64 min,max;
         pForces->GetDesignStress(StressCheckTask(lastIntervalIdx,pgsTypes::ServiceIII,pgsTypes::Tension),poi,pgsTypes::BottomGirder,&config,bat,&min,&max);

         if (fmax < max)
         {
            fmax    = max;
            max_poi = poi;
            found   = true;
         }
      }
   }

   DLOG(_T("Found controlling mid-zone final poi at ")<< WBFL::Units::ConvertFromSysUnits(max_poi.GetDistFromStart(),WBFL::Units::Measure::Feet) << _T(" ft") );

   ATLASSERT(found);
   return max_poi;
}

void pgsDesigner2::DesignEndZoneReleaseStrength(std::shared_ptr<IEAFProgress> pProgress) const
{
   DESIGN_LOG_SCOPE(_T("DesignEndZoneReleaseStrength"));
   const CSegmentKey& segmentKey = m_StrandDesignTool->GetSegmentKey();

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   DLOG(_T("Computing Release requirements at End-Zone - Assumes that harped strands have been raised to highest location or debonding is maximized before entering"));

   Float64 fc  = m_StrandDesignTool->GetConcreteStrength();
   Float64 fci = m_StrandDesignTool->GetReleaseStrength();
   DLOG(_T("current f'c  = ") << WBFL::Units::ConvertFromSysUnits(fc,WBFL::Units::Measure::KSI) << _T(" ksi") );
   DLOG(_T("current f'ci = ") << WBFL::Units::ConvertFromSysUnits(fci,WBFL::Units::Measure::KSI) << _T(" ksi") );

   const GDRCONFIG& config = m_StrandDesignTool->GetSegmentConfiguration();

   GET_IFACE2(GetBroker(),ILimitStateForces,pForces);
   GET_IFACE2(GetBroker(),IPretensionStresses, pPrestress);
   GET_IFACE2(GetBroker(),IProductForces,pProdForces);

   pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   PoiList vPOI;
   m_StrandDesignTool->GetDesignPoi(releaseIntervalIdx, POI_PSXFER, &vPOI);
   ATLASSERT(!vPOI.empty());

   // max top tension and bottom compression stresses at critical locations
   Float64 fbot =  Float64_Max;
   Float64 ftop = -Float64_Max;
   Float64 fetop, febot; 
   Float64 fptop, fpbot; 
   pgsPointOfInterest top_poi, bot_poi;

   for (const pgsPointOfInterest& poi : vPOI)
   {
      CHECK_PROGRESS;

      Float64 mine,maxe,bogus;
      pForces->GetStress(releaseIntervalIdx,pgsTypes::ServiceI,poi,bat,false,pgsTypes::TopGirder,   &bogus,&maxe);
      pForces->GetStress(releaseIntervalIdx,pgsTypes::ServiceI,poi,bat,false,pgsTypes::BottomGirder,&mine,&bogus);

      auto [fTopPretension, fBotPretension] = pPrestress->GetStress(releaseIntervalIdx, poi, pgsTypes::TopGirder, pgsTypes::BottomGirder, false /*no live load*/, pgsTypes::ServiceI, INVALID_INDEX/*controlling live load, if used*/, &config);

      Float64 max = maxe + fTopPretension;
      Float64 min = mine + fBotPretension;

      // save max stress and corresponding poi
      if (ftop < max)
      {
         ftop    = max;
         fetop   = maxe;
         fptop   = fTopPretension;
         top_poi = poi;
      }

      if (min < fbot)
      {
         fbot    = min;
         febot   = mine;
         fpbot   = fBotPretension;
         bot_poi = poi;
      }
   }

   DLOG(_T("Controlling Stress at Release , top, tension psxfer  = ")           << WBFL::Units::ConvertFromSysUnits(ftop,WBFL::Units::Measure::KSI) << _T(" ksi at ")<<WBFL::Units::ConvertFromSysUnits(top_poi.GetDistFromStart(),WBFL::Units::Measure::Feet) << _T(" ft") );
   DLOG(_T("Controlling Stress at Release , bottom, compression psxfer = ")     << WBFL::Units::ConvertFromSysUnits(fbot,WBFL::Units::Measure::KSI) << _T(" ksi at ")<<WBFL::Units::ConvertFromSysUnits(bot_poi.GetDistFromStart(),WBFL::Units::Measure::Feet) << _T(" ft"));
   DLOG(_T("External Stress Demand at Release , top, tension psxfer  = ")       << WBFL::Units::ConvertFromSysUnits(fetop,WBFL::Units::Measure::KSI) << _T(" ksi") );
   DLOG(_T("External Stress Demand at Release , bottom, compression psxfer = ") << WBFL::Units::ConvertFromSysUnits(febot,WBFL::Units::Measure::KSI) << _T(" ksi") );

   // First crack is to design concrete release strength for harped strands raised to top.
   // No use going further if we can't
   DLOG(_T("Try Designing EndZone Release Strength at Initial Condition") );
   DesignConcreteRelease(ftop, fbot);
}

void pgsDesigner2::DesignEndZoneHarpingAdjustment(const arDesignOptions& options, std::shared_ptr<IEAFProgress> pProgress) const
{
   DESIGN_LOG_SCOPE(_T("DesignEndZoneHarpingAdjustment"));
   // This function attempts to adjust harping at the ends of the girder to either minimize the number of harped strands,
   // or lower the harped strands in order to maximize constructibility.
   const CSegmentKey& segmentKey = m_StrandDesignTool->GetSegmentKey();

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   DLOG(_T("Refine harped design adjustments at end zone"));
   DLOG(_T("Computing adjustment requirements at End-Zone - Assumes that harped strands have been raised to highest location before entering"));

   GDRCONFIG config = m_StrandDesignTool->GetSegmentConfiguration();

   // Get eccentricity requirements for release
   pgsPointOfInterest top_poi, bot_poi;
   Float64 ecc_tens, ecc_comp;
   Float64 fe_top, fe_bot;
   DLOG(_T("Compute allowable eccentricity for Release...") );
   GetControllingHarpedEccentricity(releaseIntervalIdx, config, &top_poi, &bot_poi, &ecc_tens, &ecc_comp, &fe_top, &fe_bot, pProgress);

   GET_IFACE2(GetBroker(),IStrandGeometry,pStrandGeom);
   StrandIndexType Nh = m_StrandDesignTool->GetNh();

   if (m_StrandDesignTool->GetOriginalStrandFillType() == ftMinimizeHarping)
   {
      // try to trade harped to straight and, if necessary, lower strands to achieve eccentricity.
      // This is WSDOT's method, and we only look at release conditions here
      if (::IsLE(ecc_tens, ecc_comp))
      {
         DLOG(_T("Tension Controls")); 
      }
      else
      {
         DLOG(_T("Compression Controls"));
      }

      Float64 ecc_control = ecc_tens < ecc_comp ? ecc_tens : ecc_comp;
      const pgsPointOfInterest& poi_control = ecc_tens < ecc_comp ? top_poi : bot_poi;

      StrandIndexType Ns = m_StrandDesignTool->GetNs();
      StrandIndexType nh_reqd, ns_reqd;

      DLOG(_T("Try to raise end eccentricity by trading harped to straight and lowering ends"));
      if (m_StrandDesignTool->ComputeMinHarpedForEndZoneEccentricity(poi_control, ecc_control, releaseIntervalIdx, &ns_reqd, &nh_reqd))
      {
         // number of straight/harped were changed. Set them
         DLOG(_T("Number of Straight/Harped were changed from ")<<Ns<<_T("/")<<Nh<<_T(" to ")<<ns_reqd<<_T("/")<<nh_reqd);
         m_StrandDesignTool->SetNumStraightHarped(ns_reqd, nh_reqd);

         m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::RetainStrandProportioning);
      }
   }
   else
   {
      // See if we can adjust end harped strands downward and do it if we can
      Float64 offset_inc = m_StrandDesignTool->GetHarpedEndOffsetIncrement();

      if (0.0 <= offset_inc && 0 < Nh && !options.doForceHarpedStrandsStraight )
      {
         DLOG(_T("Harped strands can be adjusted downward at ends - See how low can we go...") );
         // The older version of this algorithm only adjusted for release. Later (Oct 2019), we realized that the adjustment must also 
         // consider the Bridge Site 1 (wet slab) condition so we don't lower the strands
         // too far and cause the final concrete strength to be too high

         // Get eccentricity requirements for BSS1 if considered
         IntervalIndexType deckCastingIntervalIdx = pIntervals->GetFirstCastDeckInterval();

         GET_IFACE2(GetBroker(),IConcreteStressLimits,pLimits);
         if (pLimits->CheckTemporaryStresses() && deckCastingIntervalIdx != INVALID_INDEX)
         {
            DLOG(_T("Need to compare allowable eccentricity for BSS2...") );

            pgsPointOfInterest bss1_top_poi, bss1_bot_poi;
            Float64 bss1_ecc_tens, bss1_ecc_comp;
            Float64 bss1_fe_top, bss1_fe_bot;
            GetControllingHarpedEccentricity(deckCastingIntervalIdx, config, &bss1_top_poi, &bss1_bot_poi, &bss1_ecc_tens, &bss1_ecc_comp, &bss1_fe_top, &bss1_fe_bot, pProgress);

            // bss1 only considers bottom compression
            if (bss1_ecc_comp < ecc_comp)
            {
               DLOG(_T("BSS1 eccentricity controls. We can only lower strands so far without increasing final strength requirements") );
               bot_poi  = bss1_bot_poi;
               ecc_comp = bss1_ecc_comp;
               fe_bot   = bss1_fe_bot;
            }
         }
         else
         {
            DLOG(_T("Don't need to consider BSS1 according to spec entry. Just use release requirements") );
            // ...already computed above
         }

         // compute harped offset required to achieve this ecc
         Float64 off_reqd;

         // smallest ecc controls
         if( ::IsLE(ecc_tens,ecc_comp))
         {
            DLOG(_T("Tension Controls, ecc = ") << WBFL::Units::ConvertFromSysUnits(ecc_tens, WBFL::Units::Measure::Inch) << _T(" in"));
            off_reqd = m_StrandDesignTool->ComputeEndOffsetForEccentricity(top_poi, ecc_tens);
         }
         else
         {
            DLOG(_T("Compression Controls, ecc = ") << WBFL::Units::ConvertFromSysUnits(ecc_comp, WBFL::Units::Measure::Inch) << _T(" in"));
            off_reqd = m_StrandDesignTool->ComputeEndOffsetForEccentricity(bot_poi, ecc_comp);
         }

         DLOG(_T("Harped End offset required to achieve controlling Eccentricity (raw)   = ") << WBFL::Units::ConvertFromSysUnits(off_reqd, WBFL::Units::Measure::Inch) << _T(" in"));
         // round to increment
         off_reqd = CeilOff(off_reqd, offset_inc);
         DLOG(_T("Harped End offset required to achieve controlling Eccentricity (rounded)  = ") << WBFL::Units::ConvertFromSysUnits(off_reqd, WBFL::Units::Measure::Inch) << _T(" in"));

         // Attempt to set our offset, this may be lowered to the highest allowed location 
         // if it is out of bounds
         m_StrandDesignTool->SetHarpStrandOffsetEnd(pgsTypes::metStart,off_reqd);
         m_StrandDesignTool->SetHarpStrandOffsetEnd(pgsTypes::metEnd,  off_reqd);

         m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
      }
      else
      {
         DLOG((0 < Nh ? _T("Cannot adjust harped strands due to user input"):_T("There are no harped strands to adjust")));
      }
   }

   CHECK_PROGRESS;

   config = m_StrandDesignTool->GetSegmentConfiguration();

   DLOG(_T("New eccentricity is ") << WBFL::Units::ConvertFromSysUnits( pStrandGeom->GetEccentricity(releaseIntervalIdx,ecc_tens<ecc_comp?top_poi:bot_poi, true, &config).Y(), WBFL::Units::Measure::Inch) << _T(" in"));

   GET_IFACE2(GetBroker(),IPretensionStresses, pPrestress);

   auto [fTopPs, fBotPs] = pPrestress->GetStress(releaseIntervalIdx,top_poi,pgsTypes::TopGirder,pgsTypes::BottomGirder, false, pgsTypes::ServiceI, INVALID_INDEX,&config);

   Float64 ftop = fe_top + fTopPs;
   Float64 fbot = fe_bot + fBotPs;

   DLOG(_T("After Adjustment, Controlling Stress at Release , Top, Tension        = ") << WBFL::Units::ConvertFromSysUnits(ftop,WBFL::Units::Measure::KSI) << _T(" ksi") );
   DLOG(_T("After Adjustment, Controlling Stress at Release , Bottom, Compression = ") << WBFL::Units::ConvertFromSysUnits(fbot,WBFL::Units::Measure::KSI) << _T(" ksi") );

   // Recompute required release strength
   DesignConcreteRelease(ftop, fbot);

   // Done
}

void pgsDesigner2::GetControllingHarpedEccentricity(IntervalIndexType interval, const GDRCONFIG& config, 
                                                    pgsPointOfInterest* pTopPoi,pgsPointOfInterest* pBotPoi, 
                                                    Float64* pEccTens, Float64* pEccComp, Float64* pFeTop, Float64* pFeBot, 
                                                    std::shared_ptr<IEAFProgress> pProgress) const
{
   GET_IFACE2(GetBroker(),ILimitStateForces,pForces);
   GET_IFACE2(GetBroker(),IPretensionStresses, pPrestress);
   PoiList vPOI;
   m_StrandDesignTool->GetDesignPoi(interval, POI_PSXFER, &vPOI);
   ATLASSERT(!vPOI.empty());

   GET_IFACE2(GetBroker(),IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   // max top tension and bottom compression stresses at critical locations
   Float64 fbot =  Float64_Max;
   Float64 ftop = -Float64_Max;
   Float64 fptop, fpbot; 

   for(const pgsPointOfInterest& poi : vPOI)
   {
      CHECK_PROGRESS;

      Float64 mine,maxe,bogus;
      pForces->GetStress(interval,pgsTypes::ServiceI,poi,bat,false,pgsTypes::TopGirder,   &bogus,&maxe);
      pForces->GetStress(interval,pgsTypes::ServiceI,poi,bat,false,pgsTypes::BottomGirder,&mine,&bogus);

      auto [fTopPretension, fBotPretension] = pPrestress->GetStress(interval, poi, pgsTypes::TopGirder, pgsTypes::BottomGirder, false, pgsTypes::ServiceI, INVALID_INDEX, &config);

      Float64 max = maxe + fTopPretension;
      Float64 min = mine + fBotPretension;

      // save max'd stress and corresponding poi
      if (ftop < max )
      {
         ftop    = max;
         *pFeTop   = maxe;
         fptop   = fTopPretension;
         *pTopPoi = poi;
      }

      if (min < fbot)
      {
         fbot    = min;
         *pFeBot   = mine;
         fpbot   = fBotPretension;
         *pBotPoi = poi;
      }
   }

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   DLOG(_T("Controlling Stress at ") << pIntervals->GetDescription(interval) << _T(", top, tension psxfer  = ") << WBFL::Units::ConvertFromSysUnits(ftop,WBFL::Units::Measure::KSI) << _T(" ksi") );
   DLOG(_T("Controlling Stress at ") << pIntervals->GetDescription(interval) << _T(" , bottom, compression psxfer = ") << WBFL::Units::ConvertFromSysUnits(fbot,WBFL::Units::Measure::KSI) << _T(" ksi") );
   DLOG(_T("External Stress Demand at ") << pIntervals->GetDescription(interval) << _T(" , top, tension psxfer  = ") << WBFL::Units::ConvertFromSysUnits(*pFeTop,WBFL::Units::Measure::KSI) << _T(" ksi") );
   DLOG(_T("External Stress Demand at ") << pIntervals->GetDescription(interval) << _T(" , bottom, compression psxfer = ") << WBFL::Units::ConvertFromSysUnits(*pFeBot,WBFL::Units::Measure::KSI) << _T(" ksi") );

   // Get the section properties of the girder
   GET_IFACE2(GetBroker(),ISectionProperties,pSectProp);
   Float64 Ag  = pSectProp->GetAg(interval,vPOI[0]);
   Float64 Stg = pSectProp->GetS(interval,vPOI[0],pgsTypes::TopGirder);
   Float64 Sbg = pSectProp->GetS(interval,vPOI[0],pgsTypes::BottomGirder);
   DLOG(_T("Ag  = ") << WBFL::Units::ConvertFromSysUnits(Ag, WBFL::Units::Measure::Inch2) << _T(" in^2"));
   DLOG(_T("Stg = ") << WBFL::Units::ConvertFromSysUnits(Stg,WBFL::Units::Measure::Inch3) << _T(" in^3"));
   DLOG(_T("Sbg = ") << WBFL::Units::ConvertFromSysUnits(Sbg,WBFL::Units::Measure::Inch3) << _T(" in^3"));

   // compute eccentricity to control top tension
   const CSegmentKey& segmentKey = m_StrandDesignTool->GetSegmentKey();

   GET_IFACE2(GetBroker(),IConcreteStressLimits,pLimits);

   Float64 fc;
   Float64 allowable_tension;
   Float64 allowable_compression;
   if (pIntervals->GetPrestressReleaseInterval(segmentKey) == interval)
   {
      ConcStrengthResultType conc_res;
      fc = m_StrandDesignTool->GetReleaseStrength(&conc_res);
      DLOG(_T("current f'ci  = ") << WBFL::Units::ConvertFromSysUnits(fc, WBFL::Units::Measure::KSI) << _T(" ksi "));

      allowable_tension     = pLimits->GetSegmentConcreteTensionStressLimit(    vPOI[0],StressCheckTask(interval,pgsTypes::ServiceI,pgsTypes::Tension),fc,conc_res==ConcSuccessWithRebar?true:false);
      allowable_compression = pLimits->GetSegmentConcreteCompressionStressLimit(vPOI[0],StressCheckTask(interval,pgsTypes::ServiceI,pgsTypes::Compression),fc);
      DLOG(_T("Allowable tensile stress     = ") << WBFL::Units::ConvertFromSysUnits(allowable_tension,WBFL::Units::Measure::KSI) << _T(" ksi") );
      DLOG(_T("Allowable compressive stress = ") << WBFL::Units::ConvertFromSysUnits(allowable_compression,WBFL::Units::Measure::KSI) << _T(" ksi") );
   }
   else
   {
      fc = m_StrandDesignTool->GetConcreteStrength();
      DLOG(_T("current f'c  = ") << WBFL::Units::ConvertFromSysUnits(fc, WBFL::Units::Measure::KSI) << _T(" ksi "));

      allowable_tension     = pLimits->GetSegmentConcreteTensionStressLimit(    vPOI[0],StressCheckTask(interval,pgsTypes::ServiceI,pgsTypes::Tension),fc,false);
      allowable_compression = pLimits->GetSegmentConcreteCompressionStressLimit(vPOI[0],StressCheckTask(interval,pgsTypes::ServiceI,pgsTypes::Compression),fc);
      DLOG(_T("Allowable tensile stress     = ") << WBFL::Units::ConvertFromSysUnits(allowable_tension,WBFL::Units::Measure::KSI) << _T(" ksi") );
      DLOG(_T("Allowable compressive stress = ") << WBFL::Units::ConvertFromSysUnits(allowable_compression,WBFL::Units::Measure::KSI) << _T(" ksi") );
   }

   // ecc's required to control stresses
   Float64 top_pps  = m_StrandDesignTool->GetPrestressForceAtLifting(config,*pTopPoi);
   DLOG(_T("Total Prestress Force for top location: P  = ") << WBFL::Units::ConvertFromSysUnits(top_pps, WBFL::Units::Measure::Kip) << _T(" kip"));

   *pEccTens = ComputeTopTensionEccentricity( top_pps, allowable_tension, *pFeTop, Ag, Stg);
   DLOG(_T("Eccentricity Required to control Top Tension   = ") << WBFL::Units::ConvertFromSysUnits(*pEccTens, WBFL::Units::Measure::Inch) << _T(" in"));

   // ecc to control bottom compression
   Float64 bot_pps  = m_StrandDesignTool->GetPrestressForceAtLifting(config,*pBotPoi);
   DLOG(_T("Total Prestress Force for bottom location: P  = ") << WBFL::Units::ConvertFromSysUnits(bot_pps, WBFL::Units::Measure::Kip) << _T(" kip"));

   *pEccComp = ComputeBottomCompressionEccentricity( bot_pps, allowable_compression, *pFeBot, Ag, Sbg);
   DLOG(_T("Eccentricity Required to control Bottom Compression   = ") << WBFL::Units::ConvertFromSysUnits(*pEccComp, WBFL::Units::Measure::Inch) << _T(" in"));
}

bool pgsDesigner2::CheckLiftingStressDesign(const CSegmentKey& segmentKey,const GDRCONFIG& config) const
{
   HANDLINGCONFIG lift_config;
   lift_config.bIgnoreGirderConfig = false;
   lift_config.GdrConfig = config;
   lift_config.LeftOverhang = m_StrandDesignTool->GetLeftLiftingLocation();
   lift_config.RightOverhang = m_StrandDesignTool->GetRightLiftingLocation();

   pgsGirderLiftingChecker checker(m_pBroker,m_StatusGroupID);
   auto pPoiLd = std::dynamic_pointer_cast<ISegmentLiftingDesignPointsOfInterest>(m_StrandDesignTool);

   auto artifact = checker.AnalyzeLifting(segmentKey,lift_config,pPoiLd);

   return artifact->PassedStressCheck();
}

std::vector<DebondLevelType> pgsDesigner2::DesignEndZoneReleaseDebonding(std::shared_ptr<IEAFProgress> pProgress,bool bAbortOnFail) const
{
   DESIGN_LOG_SCOPE(_T("DesignEndZoneReleaseDebonding"));
   const CSegmentKey& segmentKey = m_StrandDesignTool->GetSegmentKey();

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   DLOG(_T("Refine Debonded design by computing debond demand levels for release condition at End-Zone"));

   // We also get into this function for fully debonded designs, no use debonding if so
   if ( !m_StrandDesignTool->IsDesignDebonding() )
   {
      DLOG(_T("Fully bonded design - no need to compute debond levels "));
      std::vector<DebondLevelType> levels;
      levels.assign((long)0,0);
      return levels;
   }

   // compute eccentricity to control top tension
   Float64 fc  = m_StrandDesignTool->GetConcreteStrength();
   ConcStrengthResultType rebar_reqd;
   Float64 fci = m_StrandDesignTool->GetReleaseStrength(&rebar_reqd);
   DLOG(_T("current f'c  = ") << WBFL::Units::ConvertFromSysUnits(fc,WBFL::Units::Measure::KSI) << _T(" ksi "));
   DLOG(_T("current f'ci = ") << WBFL::Units::ConvertFromSysUnits(fci,WBFL::Units::Measure::KSI) << _T(" ksi") );

   GET_IFACE2(GetBroker(),IPointOfInterest,pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT, &vPoi);
   ASSERT( vPoi.size() == 1 );
   pgsPointOfInterest midPOI(vPoi.front());

   GET_IFACE2(GetBroker(),IConcreteStressLimits,pLimits);
   Float64 allowable_tension     = pLimits->GetSegmentConcreteTensionStressLimit(    midPOI,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension),fci,rebar_reqd==ConcSuccessWithRebar?true:false);
   Float64 allowable_compression = pLimits->GetSegmentConcreteCompressionStressLimit(midPOI,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression),fci);
   DLOG(_T("Allowable tensile stress after Release     = ") << WBFL::Units::ConvertFromSysUnits(allowable_tension,WBFL::Units::Measure::KSI) << _T(" ksi")<<(rebar_reqd==ConcSuccessWithRebar ? _T(" min rebar was required for this strength"):_T(""))  );
   DLOG(_T("Allowable compressive stress after Release = ") << WBFL::Units::ConvertFromSysUnits(allowable_compression,WBFL::Units::Measure::KSI) << _T(" ksi") );

   // We want to compute total debond demand, so bond all strands
   GDRCONFIG config = m_StrandDesignTool->GetSegmentConfiguration();
   config.PrestressConfig.Debond[pgsTypes::Straight].clear();

   StrandIndexType nperm = config.PrestressConfig.GetStrandCount(pgsTypes::Permanent);
   StrandIndexType ntemp = config.PrestressConfig.GetStrandCount(pgsTypes::Temporary);

   GET_IFACE2(GetBroker(),ILimitStateForces,pForces);
   GET_IFACE2(GetBroker(),IPretensionStresses, pPrestress);
   PoiList vPOI;
   m_StrandDesignTool->GetDesignPoiEndZone(releaseIntervalIdx, &vPOI);
   ATLASSERT(!vPOI.empty());

   GET_IFACE2(GetBroker(),IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   // Build stress demand
   GET_IFACE2(GetBroker(),IPretensionForce,pPrestressForce);
   std::vector<pgsStrandDesignTool::StressDemand> stress_demands;
   stress_demands.reserve(vPOI.size());

   for ( const pgsPointOfInterest& poi : vPOI)
   {
      Float64 fTopAppl,fBotAppl,bogus;
      pForces->GetStress(releaseIntervalIdx,pgsTypes::ServiceI,poi,bat,false,pgsTypes::TopGirder,   &bogus,&fTopAppl);
      pForces->GetStress(releaseIntervalIdx,pgsTypes::ServiceI,poi,bat,false,pgsTypes::BottomGirder,&fBotAppl,&bogus);

      auto [fTopPretension, fBotPretension] = pPrestress->GetStress(releaseIntervalIdx,poi,pgsTypes::TopGirder, pgsTypes::BottomGirder,false, pgsTypes::ServiceI, INVALID_INDEX, &config);

      // demand stress with fully bonded straight strands
      Float64 fTop = fTopAppl + fTopPretension;
      Float64 fBot = fBotAppl + fBotPretension;

      Float64 strand_force = pPrestressForce->GetPrestressForcePerStrand(poi, pgsTypes::Permanent, releaseIntervalIdx, pgsTypes::End, &config );

      DLOG(_T("Computing stresses at ")   <<WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart(),WBFL::Units::Measure::Feet) << _T(" ft"));
      DLOG(_T("Applied Top stress    = ") << WBFL::Units::ConvertFromSysUnits(fTopAppl,WBFL::Units::Measure::KSI) << _T(" ksi. Prestress stress = ")<< WBFL::Units::ConvertFromSysUnits(fTopPretension,WBFL::Units::Measure::KSI) << _T(" ksi. Total stress = ")<< WBFL::Units::ConvertFromSysUnits(fTop,WBFL::Units::Measure::KSI) << _T(" ksi"));
      DLOG(_T("Applied Bottom stress = ") << WBFL::Units::ConvertFromSysUnits(fBotAppl,WBFL::Units::Measure::KSI) << _T(" ksi. Prestress stress = ")<< WBFL::Units::ConvertFromSysUnits(fBotPretension,WBFL::Units::Measure::KSI) << _T(" ksi. Total stress = ")<< WBFL::Units::ConvertFromSysUnits(fBot,WBFL::Units::Measure::KSI) << _T(" ksi"));
      DLOG(_T("Force per strand = ") << WBFL::Units::ConvertFromSysUnits(strand_force, WBFL::Units::Measure::Kip) << _T(" kip"));

      pgsStrandDesignTool::StressDemand demand;
      demand.m_Poi          = poi;
      demand.m_TopStress    = fTop;
      demand.m_BottomStress = fBot;
      demand.m_PrestressForcePerStrand = strand_force;

      stress_demands.push_back(demand);
   }

   // compute debond levels at each section from demand
   GET_IFACE2(GetBroker(),IStrandGeometry, pStrandGeom);
   auto cg = pStrandGeom->GetStrandCG(releaseIntervalIdx, midPOI, true, &config);
   std::vector<DebondLevelType> debond_levels;
   debond_levels = m_StrandDesignTool->ComputeDebondsForDemand(stress_demands, config, cg.Y(), releaseIntervalIdx, allowable_tension, allowable_compression);

   if (  debond_levels.empty() && bAbortOnFail )
   {
      ATLASSERT(false);
      LOG_FAIL(_T("Debonding failed, this should not happen?"));

      m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::DebondDesignFailed);
      m_DesignerOutcome.AbortDesign();
   }

   return debond_levels;
}

void pgsDesigner2::DesignConcreteRelease(Float64 ftop, Float64 fbot) const
{
   DESIGN_LOG_SCOPE(_T("DesignConcreteRelease"));
   const CSegmentKey& segmentKey = m_StrandDesignTool->GetSegmentKey();

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   DLOG(_T("Total Stress at bottom = ") << WBFL::Units::ConvertFromSysUnits(fbot,WBFL::Units::Measure::KSI) << _T(" ksi") );
   DLOG(_T("Total Stress at top    = ") << WBFL::Units::ConvertFromSysUnits(ftop,WBFL::Units::Measure::KSI) << _T(" ksi") );

   Float64 fci = m_StrandDesignTool->GetReleaseStrength();
   Float64 fc_old = m_StrandDesignTool->GetConcreteStrength();

   Float64 fc_tens = fci;
   pgsTypes::StressLocation tens_location;
   if (0.0 < ftop || 0.0 < fbot)
   {
      // have tension stress, determine adequate f'ci
      Float64 ftens;
      if (fbot < ftop)
      {
        ftens = ftop;
        tens_location = pgsTypes::TopGirder;
      }
      else
      {
        ftens = fbot;
        tens_location = pgsTypes::BottomGirder;
      }

      DLOG(_T("F'ci to control tension at release is = ") << WBFL::Units::ConvertFromSysUnits(fc_tens,WBFL::Units::Measure::KSI) << _T(" ksi") );

      ConcStrengthResultType tens_success = m_StrandDesignTool->ComputeRequiredConcreteStrength(ftens,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension),&fc_tens);
      if ( ConcFailed == tens_success )
      {
         // Attempt to remedy by adding raised straight strands
         if ( m_StrandDesignTool->AddRaisedStraightStrands() )
         {
            // Attempt to add raised straight strands if this is an option. We could abort if the attempt
            // fails, but give bump 500 a chance if we go down in smoke.
            // If we are here, outer algorithm will restart.
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::RaisedStraightStrands);
            LOG_ACTION(_T("Added Raised Straight Strands - Restart design with new strand configuration"));
            return;
         }
         else
         {
            LOG_ABORT(_T("Could not find adequate release strength to control tension - Design Abort") );
            m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::ReleaseStrength);
            m_DesignerOutcome.AbortDesign();
            return;
         }
      }
      else
      {
         Float64 fci_old = m_StrandDesignTool->GetReleaseStrength();

         bool bFciUpdated = m_StrandDesignTool->UpdateReleaseStrength(fc_tens, tens_success, StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI, pgsTypes::Tension), tens_location);
         if ( bFciUpdated )
         {
            Float64 fci_new = m_StrandDesignTool->GetReleaseStrength();

            DLOG(_T("Release Strength For tension Changed to ")  << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool->GetReleaseStrength(), WBFL::Units::Measure::KSI) << _T(" ksi"));
            m_DesignerOutcome.SetOutcome(fci_new> fci_old ? pgsDesignCodes::FciIncreased : pgsDesignCodes::FciDecreased);

            Float64 fc_new = m_StrandDesignTool->GetConcreteStrength();
            if ( !IsEqual(fc_new,fc_old) )
            {
               DLOG(_T("Final Strength Also Increased to ")  << WBFL::Units::ConvertFromSysUnits(fc_new, WBFL::Units::Measure::KSI) << _T(" ksi"));
               m_DesignerOutcome.SetOutcome(fc_new> fc_old ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);
            }
         }
      }
   }

   Float64 fc_comp=fci;
   pgsTypes::StressLocation comp_location;
   if (ftop < 0.0 || fbot < 0.0)
   {
      // have compression stress, determine adequate f'ci
      Float64 fcomp;
      if (ftop < fbot)
      {
        fcomp = ftop;
        comp_location = pgsTypes::TopGirder;
      }
      else
      {
        fcomp = fbot;
        comp_location = pgsTypes::BottomGirder;
      }

      DLOG(_T("F'ci to control compression at release is = ") << WBFL::Units::ConvertFromSysUnits(fc_comp,WBFL::Units::Measure::KSI) << _T(" ksi") );

      ConcStrengthResultType success = m_StrandDesignTool->ComputeRequiredConcreteStrength(fcomp,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression),&fc_comp);
      if ( ConcFailed == success )
      {
         if ( comp_location == pgsTypes::BottomGirder && m_StrandDesignTool->AddRaisedStraightStrands() )
         {
            // Attempt to add raised straight strands if this is an option. Slim chance for compression controlled
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::RaisedStraightStrands);
            LOG_ACTION(_T("Added Raised Straight Strands for bottom compression - Restart design with new strand configuration"));
            return;
         }
         else
         {
            LOG_ABORT(_T("Could not find adequate release strength to control compression - Design Abort") );
            m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::ReleaseStrength);
            m_DesignerOutcome.AbortDesign();
            return;
         }
      }
      else
      {
         Float64 fci_old = m_StrandDesignTool->GetReleaseStrength();
         bool bFciUpdated = m_StrandDesignTool->UpdateReleaseStrength(fc_comp, success, StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI, pgsTypes::Compression), comp_location);
         if ( bFciUpdated )
         {
           Float64 fci_new = m_StrandDesignTool->GetReleaseStrength();

            DLOG(_T("Release Strength For compression Increased to ")  << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool->GetReleaseStrength(), WBFL::Units::Measure::KSI) << _T(" ksi"));
            m_DesignerOutcome.SetOutcome(fci_new> fci_old ? pgsDesignCodes::FciIncreased : pgsDesignCodes::FciDecreased);

            Float64 fc_new = m_StrandDesignTool->GetConcreteStrength();
            if (fc_new!=fc_old)
            {
               DLOG(_T("Final Strength Also Increased to ")  << WBFL::Units::ConvertFromSysUnits(fc_new, WBFL::Units::Measure::KSI) << _T(" ksi"));
               m_DesignerOutcome.SetOutcome(fc_new> fc_old ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);
            }
         }
      }

   }

}

class SectionFinder
{
public:
   static const WBFL::Stability::LiftingStabilityProblem* pStabilityProblem;
   static Float64 X;
   static bool Find(const WBFL::Stability::LiftingSectionResult& sectionResult) 
   { 
      const auto& pAnalysisPoint = pStabilityProblem->GetAnalysisPoint(sectionResult.AnalysisPointIndex);
      return IsEqual(SectionFinder::X,pAnalysisPoint->GetLocation()); 
   }
};
Float64 SectionFinder::X = 0;
const WBFL::Stability::LiftingStabilityProblem* SectionFinder::pStabilityProblem = nullptr;

void pgsDesigner2::DesignForLiftingHarping(const arDesignOptions& options, bool bProportioningStrands,std::shared_ptr<IEAFProgress> pProgress) const
{
   DESIGN_LOG_SCOPE(_T("DesignForLiftingHarping"));
   // There are two phases to lifting design. The first phase is to proportion the number of straight
   // and harped strands to obtain a _T("balanced") state of stresses when lifting the girder without
   // temporary strands. The stress at the harp point is basically independent of the number of
   // straight and harped strands (it changes very little with changing proportions). The optimum
   // design occurs when the stress at either the lift point or the point of prestress transfer
   // are approximately equal to the stresses at the harp point.
   //
   // The second phase of lifting design is to determine the lifting loop location and the
   // required release strength. When temporary strands are used, the release strength found
   // in the second phase will be lower then in the first phase.

   pProgress->UpdateMessage(_T("Designing for Lifting"));

   DLOG(_T("DESIGNING FOR LIFTING"));
   m_StrandDesignTool->DumpDesignParameters();

   // get some initial data to make function calls a little easier to read
   const CSegmentKey& segmentKey = m_StrandDesignTool->GetSegmentKey();

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   pgsGirderLiftingChecker checker(m_pBroker,m_StatusGroupID); // this guy can do the stability design!

   // Captured before bProportioningStrands can be reassigned below (see "Can't adjust strands so adjust
   // concrete strength" further down) - true only when we were called to proportion strands (the
   // separate, later "after shipping" call always passes false) and fell through to a concrete-strength
   // fallback because strand-trading couldn't reach the target eccentricity without TTS.
   const bool bWasProportioningStrands = bProportioningStrands;

   GDRCONFIG config = m_StrandDesignTool->GetSegmentConfiguration();
   if ( bProportioningStrands )
   {
      // if this is the first design for lifting, look at the lifting without temporary strands case
      // to get the optimum strand configuration
      DLOG(_T("Phase 1 Lifting Design - Design for Lifting without Temporary Strands"));
      DLOG(_T("Determine straight/harped strands proportions"));
      DLOG(_T("Removing temporary strands for lifting analysis"));
      config.PrestressConfig.ClearStrandFill(pgsTypes::Temporary);
   }
#if defined ENABLE_DESIGN_LOGGING
   else
   {
      DLOG(_T("Phase 2 Lifting Design - Design for Lifting with Temporary Strands"));
      DLOG(_T("Determine lifting locations and release strength requirements"));
   }
#endif

   // Do a stability based design for lifting. this will locate the lift point locations required
   // for stability
   // Designer manages it's own POIs
   auto pPoiLd = std::dynamic_pointer_cast<ISegmentLiftingDesignPointsOfInterest>(m_StrandDesignTool);

   HANDLINGCONFIG liftConfig;
   liftConfig.bIgnoreGirderConfig = false;
   liftConfig.GdrConfig = config;
   const WBFL::Stability::LiftingStabilityProblem* pStabilityProblem;
   auto [result, artifact] = checker.DesignLifting(segmentKey,liftConfig,pPoiLd,&pStabilityProblem,DESIGN_LOGGER);
   SectionFinder::pStabilityProblem = pStabilityProblem;

#if defined ENABLE_DESIGN_LOGGING
   DLOG(_T("-- Dump of Lifting Artifact After Design --"));
   if (pgsDesignLog::IsEnabled())
   {
      DumpLiftingArtifact(pStabilityProblem,artifact,DESIGN_LOGGER);
   }
   DLOG(_T("-- End Dump of Lifting Artifact --"));
#endif

   m_StrandDesignTool->SetLiftingLocations(liftConfig.LeftOverhang,liftConfig.RightOverhang);

   CHECK_PROGRESS;

   m_DesignerOutcome.SetOutcome(result);
   if ( m_DesignerOutcome.WasDesignAborted() )
   {
      return;
   }

   const WBFL::Stability::LiftingResults& liftingResults = artifact->GetLiftingResults();

   // Check to see if the girder is stable for lifting
   GET_IFACE2(GetBroker(),ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   Float64 FScr    = liftingResults.FScrMin;
   Float64 FScrMin = pSegmentLiftingSpecCriteria->GetLiftingCrackingFs();
   DLOG(_T("FScr = ") << FScr);
   if (FScr < FScrMin )
   {
      // the girder is not stable for lifting

      if ( bProportioningStrands )
      {
         // We are in the first phase lifting design. If we get to this point
         // temporary strands are required for lifting stability. We could add them
         // and try again, however shipping usually requires more temporary strands.
         //
         // We will quit designing for lifting here and move on to design for shipping to
         // establish the (most likely maximum) required number of temporary strands.
         //
         // The full design will restart after shipping has added temporary strands so the 
         // next time we enter this function will be for a new phase one design. The straight/harped
         // strands will be proportions considering the temporary strands.

         // Temporary strands are required... 
         DLOG(_T("Cannot find a pick point to satisfy FScr"));
         DLOG(_T("Temporary strands required"));
         DLOG(_T("Move on to Shipping Design"));
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::LiftingRedesignAfterShipping);
      }
      else
      {
         // We are in the second phase lifting design. If we get to this point
         // then the girder is more unstable during lifting than during shipping.
         // This is practically impossible (but could happen if there are strange
         // values used in the shipping stability analysis). More temporary strands
         // are required for lifting than for shipping.
         
         // Try adding temporary strands
         DLOG(_T("Cannot find a pick point to satisfy FScr"));
         DLOG(_T("Additional temporary strands required"));
         if ( m_StrandDesignTool->AddTempStrands() )
         {
            DLOG(_T("Temporary strands added"));
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::LiftingConfigChanged);
         }
         else
         {
            // couldn't add temporary strands (girder probably doesn't support them or there isn't any room)
            DLOG(_T("Tweaking straight/harped strand proportion")); // we are going to loose the design optimization, but it is better to get a design
            if ( m_StrandDesignTool->SwapStraightForHarped() )
            {
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::LiftingConfigChanged);
            }
            else
            {
               ATLASSERT(false); // need to add temporary strands
               m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::GirderLiftingStability);
               m_DesignerOutcome.AbortDesign();
            }
         }
      }
      return;
   }

   // set the location required for stability
   m_DesignerOutcome.SetOutcome(pgsDesignCodes::LiftingConfigChanged);

   if ( bProportioningStrands )
   {
      // Phase 1 design - trade harped for straight until the stress at the lift point or the point
      // of prestress transfer are approximately equal to the stress at the harp point.
      // 
      // Stresses to not have to approximately equal the stresses at the top and bottom of the girder
      // at the harp point. Rather we want a harped/straight strand configuration where the stress
      // at either the top or bottom of girder, at the harp point, are approximately matched.

      // Set the design outcome so that after the shipping design is completed
      // the second phase of lifting design will begin
      m_DesignerOutcome.SetOutcome(pgsDesignCodes::LiftingRedesignAfterShipping);

      GET_IFACE2(GetBroker(),ISectionProperties,pSectProp);
      GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);

      // Adjust the proportions of the straight and harped strands such the stress at the harp point
      // is matched by the stress at the lift point or the point of prestress transfer (which ever controls)
      
      ATLASSERT( m_StrandDesignTool->IsDesignHarping() );

      DLOG(_T("--------------------------------------------------------------------------------------------------------------------"));
      DLOG(_T("Attempt to reduce and lower harped strands for lifting condition. Use lifting points, or transfer lengths as controlling locations"));

      // get controlling stress at xfer/lift point
      Float64 fbot, bot_loc, ftop, top_loc;
      GetEndZoneMinMaxRawStresses(segmentKey,liftingResults,liftConfig,&ftop, &fbot, &top_loc, &bot_loc);
      DLOG(_T("Max applied top stress at lifting point or transfer location    = ") << WBFL::Units::ConvertFromSysUnits(ftop,WBFL::Units::Measure::KSI) << _T(" ksi at ")<< WBFL::Units::ConvertFromSysUnits(top_loc,WBFL::Units::Measure::Feet) << _T(" ft"));
      DLOG(_T("Max applied bottom stress at lifting point or transfer location = ") << WBFL::Units::ConvertFromSysUnits(fbot,WBFL::Units::Measure::KSI) << _T(" ksi at ")<< WBFL::Units::ConvertFromSysUnits(bot_loc,WBFL::Units::Measure::Feet) << _T(" ft"));
      
      // get top and bottom stresses at harp points
      PoiList vPoi;
      m_StrandDesignTool->GetPointsOfInterest(segmentKey, POI_HARPINGPOINT, &vPoi);
      ATLASSERT(0 < vPoi.size());

      std::vector<Float64> fHpTopMin, fHpTopMax, fHpBotMin, fHpBotMax;
      for(const pgsPointOfInterest& poi : vPoi)
      {
         ATLASSERT(poi.GetID() != INVALID_ID);
         ATLASSERT(poi.HasAttribute(POI_HARPINGPOINT));

         SectionFinder::X = poi.GetDistFromStart();
         std::vector<WBFL::Stability::LiftingSectionResult>::const_iterator found = std::find_if(liftingResults.vSectionResults.begin(),liftingResults.vSectionResults.end(),SectionFinder::Find);
         ATLASSERT(found != liftingResults.vSectionResults.end());
         const WBFL::Stability::LiftingSectionResult& sectionResult = *found;
         fHpTopMin.push_back(sectionResult.fMinDirect[+WBFL::Stability::GirderFace::Top]);
         fHpTopMax.push_back(sectionResult.fMaxDirect[+WBFL::Stability::GirderFace::Top]);
         fHpBotMin.push_back(sectionResult.fMinDirect[+WBFL::Stability::GirderFace::Bottom]);
         fHpBotMax.push_back(sectionResult.fMaxDirect[+WBFL::Stability::GirderFace::Bottom]);
      }

      Float64 fTopHpMin = *std::min_element(fHpTopMin.begin(),fHpTopMin.end());
      Float64 fBotHpMin = *std::min_element(fHpBotMin.begin(),fHpBotMin.end());
      Float64 fTopHpMax = *std::max_element(fHpTopMax.begin(),fHpTopMax.end());
      Float64 fBotHpMax = *std::max_element(fHpBotMax.begin(),fHpBotMax.end());
      Float64 fHpMin = Min(fTopHpMin,fBotHpMin);
      Float64 fHpMax = Max(fTopHpMax,fBotHpMax);

      DLOG(_T("Computing eccentricity required to make stress at lift/xfer point approx equal to stress at hp"));
      // POIs for the current design
      pgsPointOfInterest tpoi(m_StrandDesignTool->GetPointOfInterest(segmentKey,top_loc));
      pgsPointOfInterest bpoi(m_StrandDesignTool->GetPointOfInterest(segmentKey,bot_loc));
      ATLASSERT(tpoi.GetID() != INVALID_ID);
      ATLASSERT(bpoi.GetID() != INVALID_ID);

      // POIs to get stuff from the real bridge model
      pgsPointOfInterest tpoi_bridge(pPoi->GetPointOfInterest(segmentKey, top_loc));
      pgsPointOfInterest bpoi_bridge(pPoi->GetPointOfInterest(segmentKey, bot_loc));

      // Get the section properties of the girder
      Float64 Agt = pSectProp->GetAg(liftSegmentIntervalIdx, tpoi_bridge);
      Float64 Agb = pSectProp->GetAg(liftSegmentIntervalIdx, bpoi_bridge);
      Float64 Stg = pSectProp->GetS(liftSegmentIntervalIdx, tpoi_bridge, pgsTypes::TopGirder);
      Float64 Sbg = pSectProp->GetS(liftSegmentIntervalIdx, bpoi_bridge, pgsTypes::BottomGirder);
      DLOG(_T("Agt = ") << WBFL::Units::ConvertFromSysUnits(Agt, WBFL::Units::Measure::Inch2) << _T(" in^2"));
      DLOG(_T("Agb = ") << WBFL::Units::ConvertFromSysUnits(Agb, WBFL::Units::Measure::Inch2) << _T(" in^2"));
      DLOG(_T("Stg = ") << WBFL::Units::ConvertFromSysUnits(Stg, WBFL::Units::Measure::Inch3) << _T(" in^3"));
      DLOG(_T("Sbg = ") << WBFL::Units::ConvertFromSysUnits(Sbg, WBFL::Units::Measure::Inch3) << _T(" in^3"));

      Float64 P_for_top = m_StrandDesignTool->GetPrestressForceAtLifting(config,tpoi);
      Float64 P_for_bot;
      if ( IsEqual(top_loc,bot_loc) )
      {
         P_for_bot = P_for_top;
      }
      else
      {
         P_for_bot = m_StrandDesignTool->GetPrestressForceAtLifting(config,bpoi);
      }

      DLOG(_T("Total Prestress Force for top location: P     = ") << WBFL::Units::ConvertFromSysUnits(P_for_top, WBFL::Units::Measure::Kip) << _T(" kip"));

      // ecc's required to match stresses at harp point
      Float64 ecc_tens = compute_required_eccentricity(P_for_top,Agt,Stg,ftop,fHpMax);
      DLOG(_T("Eccentricity Required to control Top Tension  = ") << WBFL::Units::ConvertFromSysUnits(ecc_tens, WBFL::Units::Measure::Inch) << _T(" in"));
      DLOG(_T("Total Prestress Force for bottom location: P          = ") << WBFL::Units::ConvertFromSysUnits(P_for_bot, WBFL::Units::Measure::Kip) << _T(" kip"));

      // Note that the _T("exact") way to do this would be to iterate on eccentricity because prestress force is dependent on strand
      // slope, which is dependent on end strand locations. But, so far, no problems????
      Float64 ecc_comp = compute_required_eccentricity(P_for_bot,Agb,Sbg,fbot,fHpMin);
      DLOG(_T("Eccentricity Required to control Bottom Compression   = ") << WBFL::Units::ConvertFromSysUnits(ecc_comp, WBFL::Units::Measure::Inch) << _T(" in"));

#if defined ENABLE_DESIGN_LOGGING
      if( ::IsLE(ecc_tens,ecc_comp))
      {
         DLOG(_T("Tension Controls")); 
      }
      else
      {
         DLOG(_T("Compression Controls"));
      }
#endif

      // try to trade harped to straight to achieve required eccentricity
      Float64 required_eccentricity = Min(ecc_tens,ecc_comp);
      const pgsPointOfInterest& poi_control = ecc_tens < ecc_comp ? tpoi : bpoi;

      StrandIndexType Ns = m_StrandDesignTool->GetNs();
      StrandIndexType Nh = m_StrandDesignTool->GetNh();

      StrandIndexType nh_reqd, ns_reqd;

      // At this point, it is assumed that end strands are raised as high as possible
      // See if our target is lower (bigger) than the current.
      Float64 curr_ecc = m_StrandDesignTool->ComputeEccentricity(poi_control,liftSegmentIntervalIdx);
      DLOG(_T("Eccentricity for current number of strands = ")<< WBFL::Units::ConvertFromSysUnits(curr_ecc, WBFL::Units::Measure::Inch) << _T(" in"));
      if (curr_ecc <= required_eccentricity) // greater means the CG of prestress force must be lower in the section
      {
         if (m_StrandDesignTool->GetOriginalStrandFillType() == ftMinimizeHarping)
         {
            DLOG(_T("Try to increase end eccentricity by trading harped to straight"));
            if (m_StrandDesignTool->ComputeMinHarpedForEndZoneEccentricity(poi_control, required_eccentricity, liftSegmentIntervalIdx, &ns_reqd, &nh_reqd)
                && m_StrandDesignTool->SetNumStraightHarped(ns_reqd, nh_reqd))
            {
               // number of straight/harped were changed. Set them
               DLOG(_T("Number of Straight/Harped were changed from ")<<Ns<<_T("/")<<Nh<<_T(" to ")<<ns_reqd<<_T("/")<<nh_reqd);

               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::RetainStrandProportioning);
            }
            else
            {
               // Either no trade could reach the required eccentricity, or reaching it would
               // violate the strand slope limit (SetNumStraightHarped enforces that limit itself,
               // and won't leave the strand count changed if it can't be satisfied) - restore the
               // original count and adjust concrete strength instead, same as phase 2 does.
               m_StrandDesignTool->SetNumStraightHarped(Ns, Nh);
               bProportioningStrands = false; // causes phase 2 design below
            }
         }
         else
         {
            // See if we can lower end pattern
            Float64 offset_inc = m_StrandDesignTool->GetHarpedEndOffsetIncrement();
            if ( 0.0 <= offset_inc && !options.doForceHarpedStrandsStraight)
            {
               DLOG(_T("Try to raise end eccentricity by lowering harped strands at ends"));
               Float64 off_reqd = m_StrandDesignTool->ComputeEndOffsetForEccentricity(poi_control, required_eccentricity);

               // round to increment
               DLOG(_T("Harped End offset required to achieve controlling Eccentricity (raw)   = ") << WBFL::Units::ConvertFromSysUnits(off_reqd, WBFL::Units::Measure::Inch) << _T(" in"));
               off_reqd = CeilOff(off_reqd, offset_inc);
               DLOG(_T("Harped End offset required to achieve controlling Eccentricity (rounded)  = ") << WBFL::Units::ConvertFromSysUnits(off_reqd, WBFL::Units::Measure::Inch) << _T(" in"));

               // Attempt to set our offset, this may be lowered to the highest allowed location 
               // if it is out of bounds
               m_StrandDesignTool->SetHarpStrandOffsetEnd(pgsTypes::metStart,off_reqd);
               m_StrandDesignTool->SetHarpStrandOffsetEnd(pgsTypes::metEnd,  off_reqd);

               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
               DLOG(_T("New Eccentricity  = ") << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool->ComputeEccentricity(poi_control,liftSegmentIntervalIdx), WBFL::Units::Measure::Inch) << _T(" in"));
            }
            else
            {
               // Can't adjust strands so adjust concrete strength - this is what phase 2 does
               bProportioningStrands = false; // causes phase 2 design below
            }
         }
      } // end if - eccentricity

   } // end if - phase 1 design

   if ( !bProportioningStrands )
   {
      // This is phase 2 design - the goal is to determine the lifting location and the required release strength.
      // This is done in phase 2 because the shipping analysis will set the number of required temporary
      // strands. Phase 2 lifting finds the best lifting options when TTS are used.

      // Phase 2's release strength is expected to come out lower than phase 1's (see the comment at
      // the top of this function) - that is a normal outcome here, not the kind of back-and-forth the
      // decrease-history/exponential-backoff logic in ConcreteStrengthController::DoUpdate exists to
      // catch. Forget phase 1's decrease history for these checks so phase 2's authoritative result
      // isn't damped by it.
      m_StrandDesignTool->ClearReleaseStrengthDecreaseHistory(StressCheckTask(liftSegmentIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression), pgsTypes::BottomGirder);
      m_StrandDesignTool->ClearReleaseStrengthDecreaseHistory(StressCheckTask(liftSegmentIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension), pgsTypes::TopGirder);

      // The lifting points required for stability have already been determined above. Now we have to
      // find the release strength required to satisfy the allowable stress requirements

      // get the current value of fc in case it changes. f'c will change if the required f'ci > f'c
      // f'c will be made equal to f'ci
      Float64 fc_old = m_StrandDesignTool->GetConcreteStrength();

      // go to the artifact to get the required release strength to satisfy the compression and
      // tension criteria
      Float64 fci_comp = artifact->RequiredFcCompression();
      Float64 fci_tens = artifact->RequiredFcTensionWithoutRebar();
      Float64 fci_tens_wrebar = artifact->RequiredFcTensionWithRebar();

      // if there isn't a concrete strength that will make the tension limits work,
      // get the heck outta here!
      if ( fci_tens < 0 && fci_tens_wrebar < 0)
      {
         // there isn't a concrete strength that will work (because of tension limit)
         DLOG(_T("There is no concrete strength that will work for lifting after shipping design - Tension controls - FAILED"));
         m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::GirderLiftingConcreteStrength);
         m_DesignerOutcome.AbortDesign();
         return; // bye
      }

      // we've got viable concrete strengths
      DLOG(_T("Lifting Results : New f'ci (unrounded) comp = ") << WBFL::Units::ConvertFromSysUnits(fci_comp,WBFL::Units::Measure::KSI) << _T(" ksi, tension = ") << WBFL::Units::ConvertFromSysUnits(fci_tens,WBFL::Units::Measure::KSI) << _T(" ksi") << _T(" Pick Point = ") << WBFL::Units::ConvertFromSysUnits(liftConfig.LeftOverhang,WBFL::Units::Measure::Feet) << _T(" ft"));

      ConcStrengthResultType rebar_reqd = (fci_tens<0) ? ConcSuccessWithRebar : ConcSuccess;

      // get the controlling value
      Float64 fci_required = Max(fci_tens,fci_tens_wrebar,fci_comp);

      // get the maximum allowable f'ci
      Float64 fci_max = m_StrandDesignTool->GetMaximumReleaseStrength();
      if( fci_max < fci_required)
      {
         // required strength is greater than max...
         // sometimes, if we are right at the limit the max value will work... give it a try

         DLOG(_T("f'ci max = ") << WBFL::Units::ConvertFromSysUnits(fci_max,WBFL::Units::Measure::KSI) << _T(" ksi"));
         DLOG(_T("f'ci cannot be greater than max. See if we can use max for one last attempt"));

         Float64 fci_curr = m_StrandDesignTool->GetReleaseStrength();

         if ( !IsEqual(fci_curr,fci_max) )
         {
            DLOG(_T("Set to max for one more attempt"));
            fci_tens = Min(fci_tens, fci_max);
            fci_comp = Min(fci_comp, fci_max);
         }
         else
         {
            LOG_ABORT(_T("Fci max already used.There is no concrete strength that will work for lifting after shipping design - time to abort"));
            m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::GirderLiftingConcreteStrength);
            m_DesignerOutcome.AbortDesign();
            return;
         }
      }

      // Set the concrete strength. Set it once for tension and once for compression. The controlling value will stick.
      Float64 fci_old = m_StrandDesignTool->GetReleaseStrength();

      // fci_old is the release strength that was in effect when checker.DesignLifting (above) chose
      // liftConfig's pick points. If the stress-based requirement below is lower than that, see
      // whether some pick point - not necessarily the one already chosen - is stable at (or near)
      // that lower value: a lower f'ci lowers the modulus of rupture and hence the cracking moment,
      // so the pick points chosen for a higher f'ci are not guaranteed to still be stable at a lower
      // one, but different (typically wider) pick points very well may be.
      Float64 fci_target = Max(fci_tens, fci_comp);
      if (fci_target < fci_old)
      {
         DLOG(_T("Stress-based release strength of ") << WBFL::Units::ConvertFromSysUnits(fci_target, WBFL::Units::Measure::KSI)
            << _T(" ksi is lower than the ") << WBFL::Units::ConvertFromSysUnits(fci_old, WBFL::Units::Measure::KSI)
            << _T(" ksi the current pick points were chosen against - searching for pick points stable at the lower strength"));

         bool bFoundFeasible = false;
         HANDLINGCONFIG bestConfig;
         Float64 bestFciComp = 0, bestFciTens = 0, bestFciTensWRebar = 0;

         Float64 fciTrial = fci_target;
         for (int i = 0; i < 5; i++)
         {
            HANDLINGCONFIG trialConfig(liftConfig);
            trialConfig.GdrConfig.fci = fciTrial;
            auto [trialResult, trialArtifact] = checker.DesignLifting(segmentKey, trialConfig, pPoiLd, &pStabilityProblem, DESIGN_LOGGER);
            if (trialResult != pgsDesignCodes::LiftingConfigChanged)
            {
               DLOG(_T("No pick point within range is stable at ") << WBFL::Units::ConvertFromSysUnits(fciTrial, WBFL::Units::Measure::KSI) << _T(" ksi - stopping search"));
               break;
            }

            Float64 trialFciComp = trialArtifact->RequiredFcCompression();
            Float64 trialFciTens = trialArtifact->RequiredFcTensionWithoutRebar();
            Float64 trialFciTensWRebar = trialArtifact->RequiredFcTensionWithRebar();
            Float64 trialDemand = Max(trialFciTens, trialFciTensWRebar, trialFciComp);

            DLOG(_T("At ") << WBFL::Units::ConvertFromSysUnits(fciTrial, WBFL::Units::Measure::KSI)
               << _T(" ksi, pick points of ") << WBFL::Units::ConvertFromSysUnits(trialConfig.LeftOverhang, WBFL::Units::Measure::Feet)
               << _T("/") << WBFL::Units::ConvertFromSysUnits(trialConfig.RightOverhang, WBFL::Units::Measure::Feet)
               << _T(" ft are stable; stress demand there is ") << WBFL::Units::ConvertFromSysUnits(trialDemand, WBFL::Units::Measure::KSI) << _T(" ksi"));

            bFoundFeasible = true;
            bestConfig = trialConfig;
            bestFciComp = trialFciComp;
            bestFciTens = trialFciTens;
            bestFciTensWRebar = trialFciTensWRebar;

            if (trialDemand <= fciTrial)
            {
               // self-consistent: the pick points found at this strength don't need any more of it
               break;
            }

            fciTrial = trialDemand; // these pick points want more - try again at that higher strength
         }

         if (bFoundFeasible)
         {
            liftConfig = bestConfig;
            fci_comp = bestFciComp;
            fci_tens = bestFciTens;
            fci_tens_wrebar = bestFciTensWRebar;
            m_StrandDesignTool->SetLiftingLocations(liftConfig.LeftOverhang, liftConfig.RightOverhang);
         }
         // else: no feasible pick point was found below fci_old - fall through with the original
         // fci_comp/fci_tens/liftConfig, exactly as if this search had never run.
      }

      // Phase 1 (proportioning, without TTS) falling back to a concrete-strength fix here because
      // strand-trading couldn't reach the target eccentricity is a genuinely different scenario from
      // the girder's actual, as-lifted condition whenever temporary strands are used - the separate,
      // later "after shipping" call (bProportioningStrands=false) redoes this WITH TTS and is documented
      // to normally come out lower (see the comment above ClearReleaseStrengthDecreaseHistory). Writing
      // Phase 1's without-TTS requirement through to the persistent, cross-iteration
      // ConcreteStrengthController here - just to have Phase 2 supersede it every single outer iteration
      // - is what was defeating that controller's oscillation detection: it cleared the decrease history
      // needed to recognize the repeat, every time, so the exponential backoff never got a chance to
      // fire. Compute the requirement (needed above to decide lifting pick points) but leave persisting
      // it to Phase 2 when Phase 2 is going to run anyway.
      bool bPersistReleaseStrength = !(bWasProportioningStrands && 0 < m_StrandDesignTool->GetNt());

      bool bFciUpdated = false;
      if (!bPersistReleaseStrength)
      {
         DLOG(_T("Phase 1 fallback (without TTS) requires f'ci = ") << WBFL::Units::ConvertFromSysUnits(Max(fci_tens,fci_comp),WBFL::Units::Measure::KSI)
            << _T(" ksi, but temporary strands are used in the final design - leaving this to Phase 2's authoritative with-TTS determination"));
      }
      else if (fci_tens < fci_comp)
      {
         DLOG(_T("Update f'ci based on compression stress"));
         bFciUpdated = m_StrandDesignTool->UpdateReleaseStrength(fci_comp, rebar_reqd, StressCheckTask(liftSegmentIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression), pgsTypes::BottomGirder);
      }
      else
      {
         DLOG(_T("Update f'ci based on tension stress"));
         bFciUpdated = m_StrandDesignTool->UpdateReleaseStrength(fci_tens, rebar_reqd, StressCheckTask(liftSegmentIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension), pgsTypes::TopGirder);
      }

      if ( bFciUpdated )
      {
         Float64 fci_new = m_StrandDesignTool->GetReleaseStrength();
         DLOG(_T("f'ci has been updated"));
         m_DesignerOutcome.SetOutcome(fci_new> fci_old ? pgsDesignCodes::FciIncreased : pgsDesignCodes::FciDecreased);
      }

      // check to see if f'c was changed also
      Float64 fc_new = m_StrandDesignTool->GetConcreteStrength();
      if ( !IsEqual(fc_old,fc_new) )
      {
         DLOG(_T("However, Final Was Also Increased to ") << WBFL::Units::ConvertFromSysUnits(fc_new,WBFL::Units::Measure::KSI) << _T(" ksi") );
         LOG_ACTION(_T("Restart design with new strengths"));
         m_DesignerOutcome.SetOutcome(fc_old < fc_new ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);
      }
   } // end else - phase 2 design

   // always retain the strand proportioning after lifting design
   m_DesignerOutcome.SetOutcome(pgsDesignCodes::RetainStrandProportioning);
}

void pgsDesigner2::GetEndZoneMinMaxRawStresses(const CSegmentKey& segmentKey,const WBFL::Stability::LiftingResults& liftingResults,const HANDLINGCONFIG& liftConfig,Float64* pftop, Float64* pfbot, Float64* ptop_loc,Float64* pbot_loc) const
{
   ATLASSERT(0 < liftingResults.vSectionResults.size());

   // look at lifting locations and transfer lengths
   // Largest of overhang or transfer will control. (from sensitivity study and until proven wrong)
   GET_IFACE2(GetBroker(),IPretensionForce,pPrestressForce);
   Float64 XferLength = Max(pPrestressForce->GetTransferLength(segmentKey, pgsTypes::Straight, pgsTypes::TransferLengthType::Minimum), 
                            pPrestressForce->GetTransferLength(segmentKey, pgsTypes::Harped,pgsTypes::TransferLengthType::Minimum));

   GET_IFACE2(GetBroker(),IBridge,pBridge);
   Float64 Lg = pBridge->GetSegmentLength(segmentKey);

   Float64 left_loc  = Max(XferLength,liftConfig.LeftOverhang);
   Float64 right_loc = Min(Lg - XferLength,Lg - liftConfig.RightOverhang);

   SectionFinder::X = left_loc;
   std::vector<WBFL::Stability::LiftingSectionResult>::const_iterator foundLeft = std::find_if(liftingResults.vSectionResults.begin(),liftingResults.vSectionResults.end(),SectionFinder::Find);
   ATLASSERT(foundLeft != liftingResults.vSectionResults.end());

   SectionFinder::X = right_loc;
   std::vector<WBFL::Stability::LiftingSectionResult>::const_iterator foundRight = std::find_if(liftingResults.vSectionResults.begin(),liftingResults.vSectionResults.end(),SectionFinder::Find);
   ATLASSERT(foundRight != liftingResults.vSectionResults.end());

   const WBFL::Stability::LiftingSectionResult& leftSection  = *foundLeft;
   const WBFL::Stability::LiftingSectionResult& rightSection = *foundRight;

   Float64 fMaxTopLeftEnd = leftSection.fMaxDirect[+WBFL::Stability::GirderFace::Top] - leftSection.fps[+WBFL::Stability::Corner::TopLeft];
   Float64 fMaxTopRightEnd = rightSection.fMaxDirect[+WBFL::Stability::GirderFace::Top] - rightSection.fps[+WBFL::Stability::Corner::TopRight];

   Float64 fMinBottomLeftEnd = leftSection.fMinDirect[+WBFL::Stability::GirderFace::Bottom] - leftSection.fps[+WBFL::Stability::Corner::BottomLeft];
   Float64 fMinBottomRightEnd = rightSection.fMinDirect[+WBFL::Stability::GirderFace::Bottom] - rightSection.fps[+WBFL::Stability::Corner::BottomRight];

   *pftop = Max(fMaxTopLeftEnd,fMaxTopRightEnd);
   *ptop_loc = (MaxIndex(fMaxTopLeftEnd,fMaxTopRightEnd) == 0 ? left_loc : right_loc);

   *pfbot = Min(fMinBottomLeftEnd,fMinBottomRightEnd);
   *pbot_loc = (MinIndex(fMinBottomLeftEnd,fMinBottomRightEnd) == 0 ? left_loc : right_loc);
}

std::vector<DebondLevelType> pgsDesigner2::DesignForLiftingDebonding(bool bProportioningStrands, std::shared_ptr<IEAFProgress> pProgress) const
{
   DESIGN_LOG_SCOPE(_T("DesignForLiftingDebonding"));
   // If designConcrete is true, we want to set the release strength for our real design. If not,
   // the goal is to simply come up with a debonding layout that will work for the strength we compute
   // below. This layout will be used for the fabrication option when temporary strands are not used.

   pProgress->UpdateMessage(_T("Lifting Design for Debonded Girders"));
   ATLASSERT(m_StrandDesignTool->IsDesignDebonding());

   std::vector<DebondLevelType> debond_demand;

   m_StrandDesignTool->DumpDesignParameters();

   const CSegmentKey& segmentKey = m_StrandDesignTool->GetSegmentKey();

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   pgsGirderLiftingChecker checker(m_pBroker,m_StatusGroupID);
   GDRCONFIG config = m_StrandDesignTool->GetSegmentConfiguration();

   if ( bProportioningStrands )
   {
      // if this is the first design for lifting, look at the lifting without temporary strands case
      // to get the optimum strand configuration
      DLOG(_T("Phase 1 Lifting Design - Design for Lifting without Temporary Strands"));
      DLOG(_T("Determine debond strand layout"));
      DLOG(_T("Removing temporary strands for lifting analysis"));
      config.PrestressConfig.ClearStrandFill(pgsTypes::Temporary);
   }
#if defined ENABLE_DESIGN_LOGGING
   else
   {
      DLOG(_T("Phase 2 Lifting Design - Design for Lifting with Temporary Strands"));
      DLOG(_T("Determine lifting locations and release strength requirements"));
   }
#endif

   // Designer manages it's own POIs
   auto pPoiLd = std::dynamic_pointer_cast<ISegmentLiftingDesignPointsOfInterest>(m_StrandDesignTool);

   HANDLINGCONFIG liftConfig;
   liftConfig.bIgnoreGirderConfig = false;
   liftConfig.GdrConfig = config;
   const WBFL::Stability::LiftingStabilityProblem* pStabilityProblem;
   auto [result,artifact] = checker.DesignLifting(segmentKey,liftConfig,pPoiLd,&pStabilityProblem,DESIGN_LOGGER);
   SectionFinder::pStabilityProblem = pStabilityProblem; // this is the design problem we will be searching ... set it here and it will get used in multiple calls below

#if defined ENABLE_DESIGN_LOGGING
   DLOG(_T("-- Dump of Lifting Artifact After Design --"));
   if (pgsDesignLog::IsEnabled())
   {
      DumpLiftingArtifact(pStabilityProblem,artifact,DESIGN_LOGGER);
   }
   DLOG(_T("-- End Dump of Lifting Artifact --"));
#endif

   CHECK_PROGRESS;

   m_DesignerOutcome.SetOutcome(result);
   if ( m_DesignerOutcome.WasDesignAborted() )
   {
      return debond_demand;
   }

   // Set the location required for stability
   m_StrandDesignTool->SetLiftingLocations(liftConfig.LeftOverhang,liftConfig.RightOverhang);

   m_DesignerOutcome.SetOutcome(pgsDesignCodes::LiftingConfigChanged);

   const WBFL::Stability::LiftingResults& liftingResults = artifact->GetLiftingResults();

   // Check to see if the girder is stable for lifting
   GET_IFACE2(GetBroker(),ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   Float64 FScr    = liftingResults.FScrMin;
   Float64 FScrMin = pSegmentLiftingSpecCriteria->GetLiftingCrackingFs();
   DLOG(_T("FScr = ") << FScr);
   if (FScr < FScrMin )
   {
      // The girder cannot be lifted at any concrete strength.
      // If we are in the first phase lifting design (not designing concrete) and we get to this point
      // temporary strands are required for lifting stability. We could add them
      // and try again, however shipping usually requires more temporary strands.
      //
      // We will quit designing for lifting here and move on to design for shipping to
      // establish the (most likely maximum) required number of temporary strands.
      //
      // The full design will restart after shipping has added temporary strands so the 
      // next time we enter this function we will design strength and layout for temp strand design

      // Temporary strands are required... 
      DLOG(_T("Cannot find a pick point to satisfy FScr"));
      if (bProportioningStrands)
      {
         DLOG(_T("Temporary strands required"));
         DLOG(_T("Move on to Shipping Design"));
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::LiftingRedesignAfterShipping);
      }
      else
      {
         // Hauling design didn't help - crap out
         LOG_ABORT(_T("Unstable for lifting and any temporary strands added for hauling did not help - Design Failed") );
         m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::GirderLiftingStability);
         m_DesignerOutcome.AbortDesign();
      }

      return debond_demand;
   }

   Float64 fc_old  = m_StrandDesignTool->GetConcreteStrength();
   Float64 fci_old = m_StrandDesignTool->GetReleaseStrength();

   Float64 fci_max = m_StrandDesignTool->GetMaximumReleaseStrength();

   // Get required release strength from artifact
   Float64 fci_comp = artifact->RequiredFcCompression();
   Float64 fci_tens = artifact->RequiredFcTensionWithoutRebar();
   Float64 fci_tens_wrebar = artifact->RequiredFcTensionWithRebar();

   // Determine if we need to add rebar
   bool minRebarRequired;
   if (fci_max < fci_tens || fci_tens < 0.0)
   {
      fci_tens = fci_tens_wrebar;
      minRebarRequired = true;
   }
   else
   {
      minRebarRequired = false;
   }

   DLOG(_T("Required Lifting Release Strength from artifact : f'ci (unrounded) tens = ") << WBFL::Units::ConvertFromSysUnits(fci_tens,WBFL::Units::Measure::KSI) << _T(" ksi, compression = ") << WBFL::Units::ConvertFromSysUnits(fci_comp,WBFL::Units::Measure::KSI) << _T(" ksi, Pick Point = ") << WBFL::Units::ConvertFromSysUnits(liftConfig.LeftOverhang,WBFL::Units::Measure::Feet) << _T(" ft"));
   ATLASSERT( 0 <= fci_tens ); // This should never happen if FScr is OK

   // Slight changes in losses going from one strength to another can cause convergence problems. Also a strength too tight
   // might cause our debond design to fail.
   // Artificially bump strengths a bit in case were are on the edge
   const Float64 LiftingFudge = 1.02;

   // If we do not set the concrete strength, the reqd value below is for our debond design. Otherwise we will use the designed value
   Float64 fci_reqd; // our required strength

   if (bProportioningStrands)
   {
      // In first pass - see if we can get a debond design with a max'd concrete strength

      // just get the concrete strength we want to use for our debond layout
      fci_reqd = Max(fci_comp, fci_tens);
      fci_reqd = Min(fci_reqd*LiftingFudge, fci_max);

      DLOG(_T("fci_reqd = ") << WBFL::Units::ConvertFromSysUnits(fci_reqd,WBFL::Units::Measure::KSI) << _T(" ksi") );

      if (fci_old < fci_reqd)
      {
         DLOG(_T("fci_reqd is greater than current - will need to revisit lifting design after shipping for stress purposes") << WBFL::Units::ConvertFromSysUnits(fci_reqd,WBFL::Units::Measure::KSI) << _T(" ksi") );
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::LiftingRedesignAfterShipping);
      }

      return debond_demand;
   }
   else
   {
      bool bFciUpdated = false;

      // make sure new f'ci fits in the code limits
      if (fci_max < fci_tens || fci_max < fci_comp)
      {
         // strength needed is more than max allowed. Try setting to max for one more design go-around
         Float64 fci_curr = m_StrandDesignTool->GetReleaseStrength();
         if ( IsEqual(fci_max,fci_curr) )
         {
            LOG_ABORT(_T("Release strength required for lifting is greater than our current max, and we have already tried max for design - Design Failed") );
            m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::GirderLiftingConcreteStrength);
            m_DesignerOutcome.AbortDesign();
            return debond_demand;
         }
         else
         {
            DLOG(_T("Strength required for lifting is greater than our current max of ") << WBFL::Units::ConvertFromSysUnits(fci_max,WBFL::Units::Measure::KSI) << _T(" ksi - Try using max for one more go-around") );
            if (fci_max < fci_comp)
            {
               fci_comp = fci_max;
            }

            if (fci_max < fci_tens)
            {
               fci_tens = fci_max;
            }
         }
      }
      else
      {
         fci_tens *= LiftingFudge;
         fci_comp *= LiftingFudge;

         fci_tens = Min(fci_tens, fci_max);
         fci_comp = Min(fci_comp, fci_max);
      }

      ConcStrengthResultType rebar_reqd = (minRebarRequired ? ConcSuccessWithRebar : ConcSuccess);

      // update both for tension and compression. NOTE: using a dummy stress location here
      Float64 fci_old = m_StrandDesignTool->GetReleaseStrength();
      bFciUpdated |= m_StrandDesignTool->UpdateReleaseStrength(fci_tens,rebar_reqd, StressCheckTask(liftSegmentIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension),pgsTypes::TopGirder);
      bFciUpdated |= m_StrandDesignTool->UpdateReleaseStrength(fci_comp,rebar_reqd, StressCheckTask(liftSegmentIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression),pgsTypes::BottomGirder);
      if (bFciUpdated)
      {
         Float64 fci_new = m_StrandDesignTool->GetReleaseStrength();
         m_DesignerOutcome.SetOutcome(fci_new> fci_old ? pgsDesignCodes::FciIncreased : pgsDesignCodes::FciDecreased);

         Float64 fc_new = m_StrandDesignTool->GetConcreteStrength();
         if ( !IsEqual(fc_old,fc_new) )
         {
            DLOG(_T("However, Final Was Also Increased to ") << WBFL::Units::ConvertFromSysUnits(fc_new,WBFL::Units::Measure::KSI) << _T(" ksi") );
            DLOG(_T("May need to Restart design with new strengths"));
            m_DesignerOutcome.SetOutcome(fc_new> fc_old ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);
            return debond_demand;
         }
         else
         {
            DLOG(_T("Release strength increased for lifting - design continues..."));
         }
      }

      fci_reqd =  m_StrandDesignTool->GetReleaseStrength();

      // Now that we have an established concrete strength, we can use it to design our debond layout

      HANDLINGCONFIG lift_config;
      lift_config.bIgnoreGirderConfig = false;
      lift_config.GdrConfig = m_StrandDesignTool->GetSegmentConfiguration();

      lift_config.GdrConfig.fci = fci_reqd;
      lift_config.LeftOverhang = m_StrandDesignTool->GetLeftLiftingLocation();
      lift_config.RightOverhang = m_StrandDesignTool->GetRightLiftingLocation();

      return DesignDebondingForLifting(lift_config, pProgress);
   }
}

std::vector<DebondLevelType> pgsDesigner2::DesignDebondingForLifting(HANDLINGCONFIG& liftConfig, std::shared_ptr<IEAFProgress> pProgress) const
{
   DESIGN_LOG_SCOPE(_T("DesignDebondingForLifting"));
   pProgress->UpdateMessage(_T("Designing initial debonding for Lifting"));
   ATLASSERT(m_StrandDesignTool->IsDesignDebonding());

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType liftingIntervalIdx = pIntervals->GetLiftSegmentInterval(liftConfig.GdrConfig.SegmentKey);

   // set up our vector to return debond levels at each section
   SectionIndexType max_db_sections = m_StrandDesignTool->GetMaxNumberOfDebondSections();
   std::vector<DebondLevelType> lifting_debond_levels;
   lifting_debond_levels.assign(max_db_sections,0);

   DLOG(_T("Detailed Debond Design for Lifting"));
   m_StrandDesignTool->DumpDesignParameters();

   {
      const CSegmentKey& segmentKey = m_StrandDesignTool->GetSegmentKey();

      Float64 fc  = liftConfig.GdrConfig.fc;
      Float64 fci = liftConfig.GdrConfig.fci;
      DLOG(_T("current f'c  = ") << WBFL::Units::ConvertFromSysUnits(fc,WBFL::Units::Measure::KSI) << _T(" ksi "));
      DLOG(_T("current f'ci = ") << WBFL::Units::ConvertFromSysUnits(fci,WBFL::Units::Measure::KSI) << _T(" ksi") );

      GET_IFACE2(GetBroker(),ISegmentLiftingSpecCriteria,pLiftingCrit);
      Float64 allowable_tension = pLiftingCrit->GetLiftingAllowableTensileConcreteStressEx(segmentKey,fci,true);
      Float64 allowable_global_compression = pLiftingCrit->GetLiftingAllowableGlobalCompressiveConcreteStressEx(segmentKey, fci);
      Float64 allowable_peak_compression = pLiftingCrit->GetLiftingAllowablePeakCompressiveConcreteStressEx(segmentKey, fci);
      DLOG(_T("Allowable tensile stress after Release     = ") << WBFL::Units::ConvertFromSysUnits(allowable_tension,WBFL::Units::Measure::KSI) << _T(" ksi - min rebar was required for this strength"));
      DLOG(_T("Allowable global compressive stress after Release = ") << WBFL::Units::ConvertFromSysUnits(allowable_global_compression, WBFL::Units::Measure::KSI) << _T(" ksi"));
      DLOG(_T("Allowable peak compressive stress after Release = ") << WBFL::Units::ConvertFromSysUnits(allowable_peak_compression, WBFL::Units::Measure::KSI) << _T(" ksi"));

      // This is an analysis to determine stresses that must be reduced by debonding
      DLOG(_T("Debond levels measured from fully bonded section"));
      liftConfig.GdrConfig.PrestressConfig.Debond[pgsTypes::Straight].clear();

      pgsGirderLiftingChecker checker(m_pBroker,m_StatusGroupID);
      // Designer manages it's own POIs
      auto pPoiLd = std::dynamic_pointer_cast<ISegmentLiftingDesignPointsOfInterest>(m_StrandDesignTool);
      auto artifact = checker.AnalyzeLifting(segmentKey,liftConfig,pPoiLd);

      StrandIndexType nperm = liftConfig.GdrConfig.PrestressConfig.GetStrandCount(pgsTypes::Permanent);
      StrandIndexType ntemp =  liftConfig.GdrConfig.PrestressConfig.GetStrandCount(pgsTypes::Temporary);

      // Need total number of strands and cg of total strand group. 
      GET_IFACE2(GetBroker(),IPointOfInterest,pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_LIFT_SEGMENT, &vPoi);
      ASSERT( vPoi.size() == 1 );
      pgsPointOfInterest midPOI(vPoi.front());

      Float64 force_per_strand = 0.0;

      // only want stresses in end zones
      Float64 rgt_end, lft_end;
      m_StrandDesignTool->GetMidZoneBoundaries(&lft_end, &rgt_end);

      // we'll pick strand force at location just past transfer length
      Float64 xfer_length = m_StrandDesignTool->GetTransferLength(pgsTypes::Permanent);

      // Build stress demand
      std::vector<pgsStrandDesignTool::StressDemand> stress_demands;
      DLOG(_T("--- Compute lifting stresses for debonding --- nperm = ")<<nperm);
      GET_IFACE2(GetBroker(),IGirder,pGirder);
      const WBFL::Stability::ILiftingStabilityProblem* pStabilityProblem = pGirder->GetSegmentLiftingStabilityProblem(segmentKey,liftConfig,pPoiLd);
      auto results = artifact->GetLiftingResults();
      stress_demands.reserve(results.vSectionResults.size());
      for( const auto& sectionResult : results.vSectionResults)
      {
         const auto& pAnalysisPoint = pStabilityProblem->GetAnalysisPoint(sectionResult.AnalysisPointIndex);
         Float64 poi_loc = pAnalysisPoint->GetLocation();
         if(poi_loc <= lft_end || rgt_end <= poi_loc)
         {
            // get strand force if we haven't yet
            Float64 FpeStraight, XpsStraight, YpsStraight;
            pStabilityProblem->GetFpe(_T("Straight"), poi_loc ,&FpeStraight,&XpsStraight,&YpsStraight);
            
            Float64 FpeHarped, XpsHarped, YpsHarped;
            pStabilityProblem->GetFpe(_T("Harped"), poi_loc,&FpeHarped,&XpsHarped,&YpsHarped);
            
            Float64 FpeTemporary, XpsTemporary, YpsTemporary;
            pStabilityProblem->GetFpe(_T("Temporary"),poi_loc,&FpeTemporary,&XpsTemporary,&YpsTemporary);
            
            Float64 Fpe = FpeStraight + FpeHarped + FpeTemporary;
            force_per_strand = Fpe / (nperm+ntemp);

            Float64 fTop = sectionResult.fMaxDirect[+WBFL::Stability::GirderFace::Top];
            Float64 fBot = sectionResult.fMinDirect[+WBFL::Stability::GirderFace::Bottom];

            DLOG(_T("At ")<< WBFL::Units::ConvertFromSysUnits(poi_loc,WBFL::Units::Measure::Feet)<<_T(" ft, Ftop = ")<< WBFL::Units::ConvertFromSysUnits(fTop,WBFL::Units::Measure::KSI) << _T(" ksi Fbot = ")<< WBFL::Units::ConvertFromSysUnits(fBot,WBFL::Units::Measure::KSI) << _T(" ksi") );
            DLOG(_T("Average force per strand = ") << WBFL::Units::ConvertFromSysUnits(Fpe/(nperm+ntemp),WBFL::Units::Measure::Kip) << _T(" kip"));

            pgsStrandDesignTool::StressDemand demand;
            ATLASSERT(((const pgsStabilityAnalysisPoint*)(pAnalysisPoint.get()))->GetPointOfInterest().GetSegmentKey() == segmentKey);
            demand.m_Poi = ((const pgsStabilityAnalysisPoint*)(pAnalysisPoint.get()))->GetPointOfInterest();
            demand.m_TopStress = fTop;
            demand.m_BottomStress = fBot;
            demand.m_PrestressForcePerStrand = force_per_strand;

            stress_demands.push_back(demand);
         }
      }

      // compute debond levels at each section from demand
      GET_IFACE2(GetBroker(),IIntervals,pIntervals);
      IntervalIndexType liftingIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

      GET_IFACE2(GetBroker(),IStrandGeometry, pStrandGeom);
      auto cg = pStrandGeom->GetStrandCG(liftingIntervalIdx, midPOI, true, &liftConfig.GdrConfig);
      lifting_debond_levels = m_StrandDesignTool->ComputeDebondsForDemand(stress_demands, liftConfig.GdrConfig, cg.Y(), liftingIntervalIdx, allowable_tension, allowable_global_compression);

      if ( lifting_debond_levels.empty() )
      {
         ATLASSERT(false);
         LOG_FAIL(_T("Debonding failed, this should not happen?"));

         m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::DebondDesignFailed);
         m_DesignerOutcome.AbortDesign();
      }
   }

   return lifting_debond_levels;
}

void pgsDesigner2::DesignForShipping(std::shared_ptr<IEAFProgress> pProgress) const
{
   DESIGN_LOG_SCOPE(_T("DesignForShipping"));
   pProgress->UpdateMessage(_T("Designing for Shipping"));

   DLOG(_T("DESIGNING FOR SHIPPING"));

   m_StrandDesignTool->DumpDesignParameters();

   Float64 fc_current = m_StrandDesignTool->GetConcreteStrength();

   const CSegmentKey& segmentKey = m_StrandDesignTool->GetSegmentKey();

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   // Use factory to create appropriate hauling checker
   pgsGirderHandlingChecker checker_factory(m_pBroker,m_StatusGroupID);
   std::unique_ptr<pgsGirderHaulingChecker> hauling_checker( checker_factory.CreateGirderHaulingChecker() );

   bool bResult = false;

   HANDLINGCONFIG haulConfig;
   haulConfig.bIgnoreGirderConfig = false;
   haulConfig.GdrConfig = m_StrandDesignTool->GetSegmentConfiguration();

   auto pPoiLd = std::dynamic_pointer_cast<ISegmentHaulingDesignPointsOfInterest>(m_StrandDesignTool);

   auto artifact = hauling_checker->DesignHauling(segmentKey,haulConfig,m_bShippingDesignIgnoreConfigurationLimits,pPoiLd,&bResult,DESIGN_LOGGER);

   if (bResult == false && m_bShippingDesignIgnoreConfigurationLimits == false)
   {
      // Designer could not find a valid configuration.
      LOG_FAIL(_T("Failed to satisfy shipping requirements - shipping configuration limitations may be preventing a suitable solution from being found. Ignore limitations and try again"));
      m_bShippingDesignIgnoreConfigurationLimits = true; // ignore configuration limitations and try again
      artifact = hauling_checker->DesignHauling(segmentKey, haulConfig, m_bShippingDesignIgnoreConfigurationLimits, pPoiLd, &bResult, DESIGN_LOGGER);
   }

   // We've got a good shipping configuration - the only thing to worry about now is stresses
   
   // capture the results of the design
   m_StrandDesignTool->SetTruckSupportLocations(haulConfig.LeftOverhang,haulConfig.RightOverhang);
   // We now have bunk point locations to ensure stability
   m_DesignerOutcome.SetOutcome(pgsDesignCodes::HaulingConfigChanged);
   if ( haulConfig.pHaulTruckEntry )
   {
      m_StrandDesignTool->SetHaulTruck( haulConfig.pHaulTruckEntry->GetName().c_str() );
   }
   else
   {
      m_StrandDesignTool->SetHaulTruck( _T("Unknown") );
   }

   CHECK_PROGRESS;

#if defined _DEBUG
   DLOG(_T("-- Dump of Hauling Artifact After Design --"));
   if (artifact.get() != nullptr)
   {
      artifact->Dump(DESIGN_LOGGER);
   }
   DLOG(_T("-- End Dump of Hauling Artifact --"));
#endif

   bool bPassedStressChecks = artifact->PassedStressCheck(WBFL::Stability::HaulingSlope::CrownSlope) && artifact->PassedStressCheck(WBFL::Stability::HaulingSlope::Superelevation);
   DLOG(_T("Design ") << (bPassedStressChecks ? _T("did") : _T("did not")) << _T(" pass stress checks"));

   if (bResult && bPassedStressChecks)
   {
      // Everything already passes at the current concrete strength, but that strength may be a
      // leftover from an earlier iteration's configuration (different overhangs, strand
      // proportions, etc.) that no longer needs to be this high. Check whether a lower value also
      // works for the current configuration; UpdateConcreteStrength only acts on genuine decreases
      // (through its usual controller), so this is a no-op if the current value is already the
      // minimum.
      Float64 fc_max_check = m_StrandDesignTool->GetMaximumConcreteStrength();
      Float64 fc_comp1_check(0.0), fc_comp2_check(0.0), fc_tens_check(0.0), fc_tens_wrebar1_check(0.0), fc_tens_wrebar2_check(0.0);
      artifact->GetRequiredConcreteStrength(WBFL::Stability::HaulingSlope::CrownSlope, &fc_comp1_check, &fc_tens_check, &fc_tens_wrebar1_check);
      artifact->GetRequiredConcreteStrength(WBFL::Stability::HaulingSlope::Superelevation, &fc_comp2_check, &fc_tens_check, &fc_tens_wrebar2_check);
      Float64 fc_comp_check = Max(fc_comp1_check, fc_comp2_check);
      Float64 fc_tens_check_final = Max(fc_tens_wrebar1_check, fc_tens_wrebar2_check);

      if (fc_tens_check_final <= fc_max_check && fc_comp_check <= fc_max_check && fc_tens_check_final <= fc_comp_check)
      {
         Float64 fc_old = m_StrandDesignTool->GetConcreteStrength();
         bool bFcUpdated = m_StrandDesignTool->UpdateConcreteStrength(fc_tens_check_final, StressCheckTask(haulSegmentIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension), pgsTypes::TopGirder);
         bFcUpdated |= m_StrandDesignTool->UpdateConcreteStrength(fc_comp_check, StressCheckTask(haulSegmentIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression), pgsTypes::BottomGirder);
         if (bFcUpdated)
         {
            Float64 fc_new = m_StrandDesignTool->GetConcreteStrength();
            LOG_ACTION(_T("Hauling already passed, but a lower concrete strength also works for the current configuration - Restart"));
            m_DesignerOutcome.SetOutcome(fc_old < fc_new ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);
            return;
         }
      }

      m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::Success);
      return;
   }

   Float64 fc_max = m_StrandDesignTool->GetMaximumConcreteStrength();

   // Get required release strength from artifact
   Float64 fc_comp1(0.0), fc_comp2(0.0), fc_tens(0.0), fc_tens_wrebar1(0.0), fc_tens_wrebar2(0.0);
   artifact->GetRequiredConcreteStrength(WBFL::Stability::HaulingSlope::CrownSlope, &fc_comp1, &fc_tens, &fc_tens_wrebar1);
   artifact->GetRequiredConcreteStrength(WBFL::Stability::HaulingSlope::Superelevation, &fc_comp2, &fc_tens, &fc_tens_wrebar2);

   Float64 fc_comp = Max(fc_comp1, fc_comp2);
   fc_tens = Max(fc_tens_wrebar1, fc_tens_wrebar2); // Hauling design always uses higher allowable limit (lower f'c)

   DLOG(_T("f'c (unrounded) required for shipping; tension = ") << WBFL::Units::ConvertFromSysUnits(fc_tens,WBFL::Units::Measure::KSI) << _T(" ksi, compression = ") << WBFL::Units::ConvertFromSysUnits(fc_comp,WBFL::Units::Measure::KSI) << _T(" ksi"));

   CHECK_PROGRESS;

   if (fc_tens <= fc_max && // tension is less than max strength - AND -
       fc_comp <= fc_max && // compression is less than max strength - AND -
       fc_tens <= fc_comp // strength is controlled by compression... adjust the concrete strength (if controlled by tension, add temporary strands)
      )
   {
      DLOG(_T("Required concrete strength does not exceed maximum. Attempting to increase concrete strength"));
      // NOTE: Using bogus stress location
      Float64 fc_old = m_StrandDesignTool->GetConcreteStrength();

      bool bFcUpdated = m_StrandDesignTool->UpdateConcreteStrength(fc_tens, StressCheckTask(haulSegmentIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension), pgsTypes::TopGirder);
      bFcUpdated |= m_StrandDesignTool->UpdateConcreteStrength(fc_comp, StressCheckTask(haulSegmentIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression), pgsTypes::BottomGirder);
      if (bFcUpdated)
      {
         LOG_ACTION(_T("Concrete strength was increased for shipping - Restart"));
         Float64 fc_new = m_StrandDesignTool->GetConcreteStrength();
         m_DesignerOutcome.SetOutcome(fc_old < fc_new ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);
         return;
      }
   }

   CHECK_PROGRESS;

   if (bPassedStressChecks)
   {
      // The hauling stresses are already satisfied by the current concrete strength. Reaching
      // here means UpdateConcreteStrength returned false because there was nothing to update
      // (the required strength is at or below the current value, or below the minimum) - not
      // because no strength can work. Adding temporary strands cannot improve a stress check
      // that already passes, and because DesignHauling can report failure for configuration
      // reasons that strands do not affect, doing so escalates Nt to the girder maximum on
      // every restart for no benefit.
      DLOG(_T("Hauling stresses are satisfied by the current concrete strength - no temporary strands needed"));
      return;
   }

   // there isn't a concrete strength that will work (because of tension limit)

   // Add temporary strands and try again.
   DLOG(_T("There is no concrete strength that will work for shipping... Adding temporary strands"));
   if (m_StrandDesignTool->AddTempStrands())
   {
      LOG_ACTION(_T("Temporary strands added. Restart design"));
      m_DesignerOutcome.SetOutcome(pgsDesignCodes::TemporaryStrandsChanged);
      return;
   }
   else
   {
      DLOG(_T("Could not add temporary strands - attempt to increase concrete strength"));
      Float64 fc_old = m_StrandDesignTool->GetConcreteStrength();
      bool bFcUpdated = m_StrandDesignTool->UpdateConcreteStrength(fc_tens, StressCheckTask(haulSegmentIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension), pgsTypes::TopGirder);
      bFcUpdated |= m_StrandDesignTool->UpdateConcreteStrength(fc_comp, StressCheckTask(haulSegmentIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression), pgsTypes::BottomGirder);
      if (bFcUpdated)
      {
         LOG_ACTION(_T("Concrete strength was increased for shipping - Restart"));
         Float64 fc_new = m_StrandDesignTool->GetConcreteStrength();
         m_DesignerOutcome.SetOutcome(fc_old < fc_new ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);
         return;
      }

      m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::GirderShippingConcreteStrength);
      //m_DesignerOutcome.AbortDesign();
      return;
   }

   DLOG(_T("Shipping Results : f'c (unrounded) tens = ") << WBFL::Units::ConvertFromSysUnits(fc_tens, WBFL::Units::Measure::KSI) << _T(" ksi, Comp = ")
      << WBFL::Units::ConvertFromSysUnits(fc_comp, WBFL::Units::Measure::KSI) << _T("KSI, Left Bunk Point = ")
      << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool->GetTrailingOverhang(), WBFL::Units::Measure::Feet) << _T(" ft")
      << _T("    Right Bunk Point = ") << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool->GetLeadingOverhang(), WBFL::Units::Measure::Feet) << _T(" ft"));

   DLOG(_T("Shipping Design Complete - Continue design") );
}

bool pgsDesigner2::CheckShippingStressDesign(const CSegmentKey& segmentKey,const GDRCONFIG& config) const
{
   HANDLINGCONFIG ship_config;
   ship_config.bIgnoreGirderConfig = false;
   ship_config.GdrConfig = config;
   ship_config.LeftOverhang = m_StrandDesignTool->GetLeadingOverhang();
   ship_config.RightOverhang = m_StrandDesignTool->GetTrailingOverhang();

   auto pPoiLd = std::dynamic_pointer_cast<ISegmentHaulingDesignPointsOfInterest>(m_StrandDesignTool);

   // Use factory to create appropriate hauling checker
   pgsGirderHandlingChecker checker_factory(m_pBroker,m_StatusGroupID);
   std::unique_ptr<pgsGirderHaulingChecker> hauling_checker( checker_factory.CreateGirderHaulingChecker() );

   auto artifact( hauling_checker->AnalyzeHauling(segmentKey,ship_config,pPoiLd) );

   return artifact->PassedStressCheck(WBFL::Stability::HaulingSlope::CrownSlope) && artifact->PassedStressCheck(WBFL::Stability::HaulingSlope::Superelevation);
}

// Evaluates one stress check task, config-aware (see CheckFinalConcreteStrengthAgainstFullPoiGrid), over
// whatever POI list the caller supplies, and returns the concrete strength required to satisfy it, or 0
// if none is needed. This mirrors RefineDesignForAllowableStress(task,...)'s own math line-for-line -
// same GetDesignStress/GetStress(...,&config) calls, same k-factor, same controlling-stress tracking -
// so it is exactly as correct as the check the design loop already trusts every iteration; the only
// difference is the POI list passed in. It never touches m_StrandDesignTool's ratchet controller or ANY
// other state - it's a pure, read-only "what would be required" query.
Float64 pgsDesigner2::CheckAllowableStressFullPoiGrid(const StressCheckTask& task,const PoiList& vPoi) const
{
   const CSegmentKey& segmentKey = m_StrandDesignTool->GetSegmentKey();
   IntervalIndexType intervalIdx = task.intervalIdx;

   const GDRCONFIG& config = m_StrandDesignTool->GetSegmentConfiguration();
   Float64 fcgdr = config.fc;

   GET_IFACE2(GetBroker(),IConcreteStressLimits,pLimits);
   GET_IFACE2(GetBroker(),ILimitStateForces,pLimitStateForces);
   GET_IFACE2(GetBroker(),IPretensionStresses,pPsStress);

   Float64 fLimit;
   pgsPointOfInterest dummyPOI(segmentKey,0.0);
   if ( task.stressType == pgsTypes::Compression )
   {
      fLimit = pLimits->GetSegmentConcreteCompressionStressLimit(dummyPOI,task,fcgdr);
   }
   else
   {
      fLimit = pLimits->GetSegmentConcreteTensionStressLimit(dummyPOI,task,fcgdr,false/*without rebar*/);
   }

   bool adj_strength = false;
   Float64 fControl = task.stressType == pgsTypes::Tension ? -Float64_Max : Float64_Max;

   pgsTypes::BridgeAnalysisType batTop, batBottom;
   GetBridgeAnalysisType(segmentKey.girderIndex,task,batTop,batBottom);

   GET_IFACE2(GetBroker(),ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();
   Float64 k = pLoadFactors->GetDCMax(task.limitState);

   for (const pgsPointOfInterest& poi : vPoi)
   {
      Float64 fTopMinExt, fTopMaxExt;
      Float64 fBotMinExt, fBotMaxExt;
      pLimitStateForces->GetDesignStress(task,poi,pgsTypes::TopGirder,   &config,batTop,   &fTopMinExt,&fTopMaxExt);
      pLimitStateForces->GetDesignStress(task,poi,pgsTypes::BottomGirder,&config,batBottom,&fBotMinExt,&fBotMaxExt);

      auto [fTopPre, fBotPre] = pPsStress->GetStress(intervalIdx,poi,pgsTypes::TopGirder, pgsTypes::BottomGirder, task.bIncludeLiveLoad, task.limitState, INVALID_INDEX, &config);

      Float64 fTopMin = fTopMinExt + k*fTopPre;
      Float64 fTopMax = fTopMaxExt + k*fTopPre;
      Float64 fBotMin = fBotMinExt + k*fBotPre;
      Float64 fBotMax = fBotMaxExt + k*fBotPre;

      if ( task.stressType == pgsTypes::Tension )
      {
         if ( fLimit < fTopMax && !IsEqual(fLimit,fTopMax) )
         {
            fControl = Max(fControl, fTopMax);
            adj_strength = true;
         }
         if ( fLimit < fBotMax && !IsEqual(fLimit,fBotMax) )
         {
            fControl = Max(fControl, fBotMax);
            adj_strength = true;
         }
      }
      else
      {
         if ( fTopMin < fLimit && !IsEqual(fTopMin,fLimit,0.001) )
         {
            fControl = Min(fControl, fTopMin);
            adj_strength = true;
         }
         if ( fBotMin < fLimit && !IsEqual(fBotMin,fLimit,0.001) )
         {
            fControl = Min(fControl, fBotMin);
            adj_strength = true;
         }
      }
   }

   if ( adj_strength )
   {
      Float64 fc_reqd;
      ConcStrengthResultType result = m_StrandDesignTool->ComputeRequiredConcreteStrength(fControl,task,&fc_reqd);
      if ( result != ConcFailed && 0 < fc_reqd )
      {
         return fc_reqd;
      }
   }

   return 0;
}

Float64 pgsDesigner2::CheckFinalConcreteStrengthAgainstFullPoiGrid(const CSegmentKey& segmentKey,const GDRCONFIG& config) const
{
   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType releaseIntervalIdx      = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType tsRemovalIntervalIdx    = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);

   GET_IFACE2(GetBroker(),IConcreteStressLimits,pLimits);
   GET_IFACE2(GetBroker(),IPointOfInterest,pPoi);

   Float64 fc_reqd_max = 0;

   for (const auto& task : m_StressCheckTasks)
   {
      if (task.intervalIdx < erectSegmentIntervalIdx || task.intervalIdx == releaseIntervalIdx)
      {
         // release/lifting/hauling are governed by f'ci and already independently verified by the
         // caller (CheckSegmentStressesAtRelease, CheckLiftingStressDesign, CheckShippingStressDesign) -
         // this is scoped to final concrete strength (f'c), i.e. the post-erection service checks
         continue;
      }

      if ( !pLimits->IsConcreteStressLimitApplicable(segmentKey,task) )
      {
         continue;
      }

      if ( task.intervalIdx == tsRemovalIntervalIdx && m_StrandDesignTool->GetNt() == 0 )
      {
         continue;
      }

      // Full analysis grid for this task's erection state - same POI set CheckSegmentStresses's caller
      // uses for the real, final spec check, not the sparse critical-section set
      // RefineDesignForAllowableStress uses during iteration.
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT, &vPoi);

      Float64 fc_reqd = CheckAllowableStressFullPoiGrid(task, vPoi);
      fc_reqd_max = Max(fc_reqd_max, fc_reqd);
   }

   if ( 0 < fc_reqd_max && config.fc < fc_reqd_max )
   {
      Float64 fc_max = m_StrandDesignTool->GetMaximumConcreteStrength();
      Float64 fc_new = CeilOff(fc_reqd_max, m_StrandDesignTool->GetConcreteAccuracy());
      if ( fc_new <= fc_max )
      {
         DLOG(_T("Full-POI-grid final check found a shortfall the sparse design-time check missed - f'c required = ")
            << WBFL::Units::ConvertFromSysUnits(fc_reqd_max,WBFL::Units::Measure::KSI) << _T(" ksi, rounded up to ")
            << WBFL::Units::ConvertFromSysUnits(fc_new,WBFL::Units::Measure::KSI) << _T(" ksi (was ")
            << WBFL::Units::ConvertFromSysUnits(config.fc,WBFL::Units::Measure::KSI) << _T(" ksi)"));
         return fc_new;
      }
   }

   return 0;
}

void pgsDesigner2::RefineDesignForAllowableStress(std::shared_ptr<IEAFProgress> pProgress) const
{
   const CSegmentKey& segmentKey = m_StrandDesignTool->GetSegmentKey();

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);

#if defined ENABLE_DESIGN_LOGGING
   IntervalIndexType liveLoadIntervalIdx  = pIntervals->GetLiveLoadInterval();
#endif

   ATLASSERT(!m_DesignerOutcome.DidConcreteChange()); // if this flag is set going in, we will get false positive

   DESIGN_LOG_SCOPE(_T("RefineDesignForAllowableStress"));

   GET_IFACE2(GetBroker(),IConcreteStressLimits,pLimits);

   // Our only option is to increase concrete strength, so let loop finish unless we fail.
   for(const auto& task : m_StressCheckTasks)
   {
      if ( !pLimits->IsConcreteStressLimitApplicable(segmentKey,task) )
      {
         // stress check isn't applicable so move on to the next one
         continue;
      }

      if ( task.intervalIdx == tsRemovalIntervalIdx && m_StrandDesignTool->GetNt() == 0 )
      {
         // if there aren't any temporary strands, don't refine design for temporary strand removal
         continue;
      }

      {
         DESIGN_LOG_SCOPE(_T("Stress check: Interval ") << LABEL_INTERVAL(task.intervalIdx) << _T(" (") << pIntervals->GetDescription(task.intervalIdx) << _T(")")
                          << (!task.bIncludeLiveLoad && liveLoadIntervalIdx <= task.intervalIdx ? _T(" without live load") : _T(""))
                          << _T(", ") << g_LimitState[task.limitState] << _T(" ") << g_Type[task.stressType]);

         RefineDesignForAllowableStress(task,pProgress);

         if (m_DesignerOutcome.WasDesignAborted())
         {
            DESIGN_LOG_SCOPE_RESULT(_T("-> ABORT"));
         }
         else if (m_DesignerOutcome.DidConcreteChange())
         {
            DESIGN_LOG_SCOPE_RESULT(_T("-> FAIL, concrete strength changed"));
         }
         else
         {
            DESIGN_LOG_SCOPE_RESULT(_T("-> OK"));
         }
      }

      CHECK_PROGRESS;
      if (m_DesignerOutcome.WasDesignAborted() )
      {
         DESIGN_LOG_SCOPE_RESULT(_T("-> ABORT"));
         return;
      }
      else if (m_DesignerOutcome.DidConcreteChange())
      {
         DESIGN_LOG_SCOPE_RESULT(_T("-> concrete strength changed, design must restart"));
         return;
      }
   }

   DESIGN_LOG_SCOPE_RESULT(_T("-> OK, all applicable stress checks passed"));
}

void pgsDesigner2::RefineDesignForAllowableStress(const StressCheckTask& task,std::shared_ptr<IEAFProgress> pProgress) const
{
   const CSegmentKey& segmentKey = m_StrandDesignTool->GetSegmentKey();

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liftSegmentIntervalIdx   = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tsRemovalIntervalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType noncompositeIntervalIdx  = pIntervals->GetLastNoncompositeInterval();
   IntervalIndexType lastIntervalIdx          = pIntervals->GetIntervalCount()-1;

#if defined _DEBUG
   // we don't do design for time-step analysis method
   // there were checks for loss method in an earlier version of this method
   // so the assert is used here to make sure the loss method is ok
   GET_IFACE2(GetBroker(),ILossParameters, pLossParams);
   ATLASSERT(pLossParams->GetLossMethod() != PrestressLossCriteria::LossMethodType::TIME_STEP);

   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   ATLASSERT(task.intervalIdx != railingSystemIntervalIdx);
#endif

   IntervalIndexType intervalIdx = task.intervalIdx;
   if (intervalIdx == releaseIntervalIdx && 0 < m_StrandDesignTool->GetNt())
   {
      // if there are TTS, then use the interval after release, instead of release
      // to account for TTS PT immediately after release
      intervalIdx++;
   }

   Float64 fcgdr;
   const GDRCONFIG& config = m_StrandDesignTool->GetSegmentConfiguration();
   if ( task.intervalIdx == releaseIntervalIdx )
   {
      fcgdr = config.fci;
   }
   else
   {
      fcgdr = config.fc;
   }

   GET_IFACE2(GetBroker(),IConcreteStressLimits,pLimits);
   GET_IFACE2(GetBroker(),ILimitStateForces,pLimitStateForces);
   GET_IFACE2(GetBroker(),IPretensionStresses,pPsStress);

   DLOG(_T("Design state: ") << m_StrandDesignTool->GetDesignStateSummary());

   Float64 start_end_size = 0.0;
   if ( releaseIntervalIdx < intervalIdx )
   {
      GET_IFACE2(GetBroker(),IBridge,pBridge);
      start_end_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   }

   //
   // Get the allowable stresses
   //
   Float64 fLimit;
   pgsPointOfInterest dummyPOI(segmentKey,0.0);
   if ( task.stressType == pgsTypes::Compression )
   {
      fLimit = pLimits->GetSegmentConcreteCompressionStressLimit(dummyPOI,task,fcgdr);
   }
   else
   {
      bool bWithBondedReinforcement = false;
      if ( intervalIdx == releaseIntervalIdx )
      {
         bWithBondedReinforcement = m_StrandDesignTool->DoesReleaseRequireAdditionalRebar();
      }
      fLimit = pLimits->GetSegmentConcreteTensionStressLimit(dummyPOI,task,fcgdr,bWithBondedReinforcement);
   }
   DLOG(_T("Stress limit = ") << pgsDesignLog::ksi(fLimit) << _T(" ksi (") << (task.stressType == pgsTypes::Tension ? _T("tension") : _T("compression"))
       << _T(", concrete strength used = ") << pgsDesignLog::ksi(fcgdr) << _T(" ksi)"));

   bool adj_strength = false; // true if we need to increase strength
   Float64 fControl = task.stressType == pgsTypes::Tension ? -Float64_Max :  Float64_Max;  // controlling stress for all pois
   pgsTypes::StressLocation stress_location;

   pgsTypes::BridgeAnalysisType batTop, batBottom;
   GetBridgeAnalysisType(segmentKey.girderIndex,task,batTop,batBottom);

   PoiList vPoi;
   if ( m_StrandDesignTool->IsDesignHarping() )
   {
      PoiAttributeType refAttrib = (intervalIdx < erectSegmentIntervalIdx ? POI_RELEASED_SEGMENT : POI_ERECTED_SEGMENT);
      m_StrandDesignTool->GetDesignPoi(intervalIdx, refAttrib | POI_5L, &vPoi);
   
      PoiList morePoi;
      m_StrandDesignTool->GetDesignPoi(intervalIdx, POI_PSXFER | POI_CONCLOAD | POI_HARPINGPOINT, &morePoi);
      vPoi.insert(vPoi.end(),morePoi.begin(),morePoi.end());
   }
   else
   {
      PoiAttributeType refAttrib = (intervalIdx < erectSegmentIntervalIdx ? POI_RELEASED_SEGMENT : POI_ERECTED_SEGMENT);
      m_StrandDesignTool->GetDesignPoi(intervalIdx,refAttrib,&vPoi);

      PoiList morePoi;
      m_StrandDesignTool->GetDesignPoi(intervalIdx, POI_H | POI_PSXFER | POI_DEBOND,&morePoi);
      vPoi.insert(vPoi.end(),morePoi.begin(),morePoi.end());
   }

   GET_IFACE2(GetBroker(),IPointOfInterest, pPoi);
   pPoi->SortPoiList(&vPoi); // sort and remove duplicates
   ATLASSERT(0 < vPoi.size());

   // One row per POI. "ps" is the stress due to prestress, "res" is the resultant (external + k*ps) as [min, max]
   DLOG(_T("        x (ft)    POI | top: ps      res min   res max | bot: ps      res min   res max | result"));

   for(const pgsPointOfInterest& poi : vPoi)
   {
      CHECK_PROGRESS;

      //
      // Get the stresses due to externally applied loads
      //
      Float64 fTopMinExt, fTopMaxExt;
      Float64 fBotMinExt, fBotMaxExt;
      pLimitStateForces->GetDesignStress(task,poi,pgsTypes::TopGirder,   &config,batTop,   &fTopMinExt,&fTopMaxExt);
      pLimitStateForces->GetDesignStress(task,poi,pgsTypes::BottomGirder,&config,batBottom,&fBotMinExt,&fBotMaxExt);

      LOG_DETAIL(_T("External stress at x = ") << pgsDesignLog::ft(poi.GetDistFromStart()) << _T(" ft: top [") << pgsDesignLog::ksi(fTopMinExt) << _T(", ") << pgsDesignLog::ksi(fTopMaxExt)
                 << _T("] ksi, bottom [") << pgsDesignLog::ksi(fBotMinExt) << _T(", ") << pgsDesignLog::ksi(fBotMaxExt) << _T("] ksi"));

      //
      // Get the stresses due to prestressing (adjust for losses)
      //
      auto [fTopPre, fBotPre] = pPsStress->GetStress(intervalIdx,poi,pgsTypes::TopGirder, pgsTypes::BottomGirder, task.bIncludeLiveLoad, task.limitState, INVALID_INDEX, &config);

      //
      // Compute the resultant stresses on the section
      //
      GET_IFACE2(GetBroker(),ILoadFactors,pLF);
      const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();
      Float64 k = pLoadFactors->GetDCMax(task.limitState);

      Float64 fTopMin, fTopMax;
      Float64 fBotMin, fBotMax;

      fTopMin = fTopMinExt + k*fTopPre;
      fTopMax = fTopMaxExt + k*fTopPre;
      fBotMin = fBotMinExt + k*fBotPre;
      fBotMax = fBotMaxExt + k*fBotPre;

      // the failure (if any) at this POI is appended to the row, so there is one line per POI
      DESIGN_LOG_ONLY(LPCTSTR strPoiResult = _T("OK"));

      //
      // Check the resultant stresses on the section
      //
      switch( task.stressType )
      {
      case pgsTypes::Tension:
         // Only look at tension at the top in the casting yard or at lifting. 
         // Other stages are considered to be after losses, so tension rules only apply out of the precompressed
         // tensile zone (top of girder) 
         if (fBotMax < fTopMax && (intervalIdx == releaseIntervalIdx || intervalIdx == liftSegmentIntervalIdx ||
                                   intervalIdx == tsRemovalIntervalIdx || intervalIdx == noncompositeIntervalIdx))
         {
            // tension top controlling
            if ( fLimit < fTopMax && !IsEqual(fLimit,fTopMax) )
            {
               DESIGN_LOG_ONLY(strPoiResult = _T("FAIL: tension at top exceeds limit"));
               fControl = Max(fControl, fTopMax);
               stress_location = pgsTypes::TopGirder;
               adj_strength = true;
            }
         }
         else
         {
            // tension bottom controlling
            if ( fLimit < fBotMax && !IsEqual(fLimit,fBotMax)  )
            {
               // tensile zone (bottom of girder)
               DESIGN_LOG_ONLY(strPoiResult = _T("FAIL: tension at bottom exceeds limit"));
               fControl = Max(fControl, fBotMax);
               stress_location = pgsTypes::BottomGirder;
               adj_strength = true;
            }
         }
         break;

      case pgsTypes::Compression:
         if ( fBotMin < fTopMin )
         {
            // compression bottom controlling
            if ( fBotMin < fLimit && !IsEqual(fBotMin,fLimit,0.001) )
            {
               DESIGN_LOG_ONLY(strPoiResult = _T("FAIL: compression at bottom exceeds limit"));

               fControl = Min(fControl, fBotMin);
               stress_location = pgsTypes::BottomGirder;
               adj_strength = true;
            }
         }
         else
         {
            // compression top controlling
            if ( fTopMin < fLimit && !IsEqual(fTopMin,fLimit,0.001) )
            {
               DESIGN_LOG_ONLY(strPoiResult = _T("FAIL: compression at top exceeds limit"));

               fControl = Min(fControl, fTopMin);
               stress_location = pgsTypes::TopGirder;
               adj_strength = true;
            }
         }
         break;

      default:
         ATLASSERT(false); // should never get here
      } // end of switch on type

#if defined ENABLE_DESIGN_LOGGING
      if (pgsDesignLog::IsEnabled())
      {
         using namespace pgsDesignLog;
         std::_tostringstream osRow;
         osRow << Fixed(ft(poi.GetDistFromStart()), 3, 14) << std::setw(7) << poi.GetID()
               << _T(" |     ") << Fixed(ksi(fTopPre), 3, 8) << _T("  ") << Fixed(ksi(fTopMin), 3, 8) << _T("  ") << Fixed(ksi(fTopMax), 3, 8)
               << _T(" |     ") << Fixed(ksi(fBotPre), 3, 8) << _T("  ") << Fixed(ksi(fBotMin), 3, 8) << _T("  ") << Fixed(ksi(fBotMax), 3, 8)
               << _T(" | ") << strPoiResult;
         DLOG(osRow.str());
      }
#endif // ENABLE_DESIGN_LOGGING
   }  // Next poi

   if ( adj_strength )
   {
      LOG_FAIL(_T("Stress limit exceeded. Controlling stress = ") << pgsDesignLog::ksi(fControl) << _T(" ksi at ") << (stress_location == pgsTypes::TopGirder ? _T("top") : _T("bottom"))
               << _T(", limit = ") << pgsDesignLog::ksi(fLimit) << _T(" ksi. Need higher ") << (intervalIdx == releaseIntervalIdx ? _T("f'ci") : _T("f'c")));

      // Try the next highest concrete strength
      Float64 fc_reqd;
      ConcStrengthResultType result = m_StrandDesignTool->ComputeRequiredConcreteStrength(fControl,task,&fc_reqd);

      if ( ConcFailed == result )
      {
         // could not find a concrete strength that would work
         LOG_ABORT(_T("No concrete strength can satisfy this stress limit"));
         m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::StressExceedsConcreteStrength);
         m_DesignerOutcome.AbortDesign();
         return;
      }

      if( intervalIdx == releaseIntervalIdx )
      {
         Float64 fci_old = m_StrandDesignTool->GetReleaseStrength();

         if (m_StrandDesignTool->UpdateReleaseStrength(fc_reqd,result, task,stress_location))
         {
            Float64 fci_new = m_StrandDesignTool->GetReleaseStrength();
            m_DesignerOutcome.SetOutcome(fci_new> fci_old ? pgsDesignCodes::FciIncreased : pgsDesignCodes::FciDecreased);
         }
      }
      else
      {
         Float64 fc_old = m_StrandDesignTool->GetConcreteStrength();
         if (m_StrandDesignTool->UpdateConcreteStrength(fc_reqd,task,stress_location))
         {
            Float64 fc_new = m_StrandDesignTool->GetConcreteStrength();
            m_DesignerOutcome.SetOutcome(fc_new> fc_old ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);
         }
      }
   }
}

void pgsDesigner2::RefineDesignForUltimateMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,std::shared_ptr<IEAFProgress> pProgress) const
{
   const CSegmentKey& segmentKey = m_StrandDesignTool->GetSegmentKey();

   PoiList vPoi;
   m_StrandDesignTool->GetDesignPoi(intervalIdx, POI_ERECTED_SEGMENT, &vPoi);

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   
   DESIGN_LOG_SCOPE(_T("RefineDesignForUltimateMoment: Interval ") << LABEL_INTERVAL(intervalIdx) << _T(", ") << g_LimitState[limitState]);
   DESIGN_LOG_SCOPE_RESULT(_T("-> OK"));
   DLOG(_T("Design state: ") << m_StrandDesignTool->GetDesignStateSummary());

#if defined ENABLE_DESIGN_LOGGING
   // Logs the details of the moment capacity calculation. Always logged at a failing POI, otherwise only with detailed logging.
   auto logCapacityDetails = [&](const pgsPointOfInterest& poi, const GDRCONFIG& config)
   {
      using namespace pgsDesignLog;
      GET_IFACE2(GetBroker(),IMomentCapacity, pMomentCapacity);
      const MOMENTCAPACITYDETAILS* pmcd = pMomentCapacity->GetMomentCapacityDetails( intervalIdx, poi, true, &config );

      GET_IFACE2(GetBroker(),ILosses,pILosses);
      Float64 check_loss = pILosses->GetEffectivePrestressLossWithLiveLoad(poi,pgsTypes::Permanent,pgsTypes::ServiceIII, INVALID_INDEX/*controlling live load*/, true/*include elastic effects*/, true/*apply elastic gain reduction*/, &config);

      CRACKINGMOMENTDETAILS cmd;
      pMomentCapacity->GetCrackingMomentDetails(intervalIdx, poi, config, true, &cmd);

      DLOG(_T("   capacity details: fps_avg = ") << ksi(pmcd->fps_avg) << _T(" ksi, fpt_avg (segment/girder) = ") << ksi(pmcd->fpt_avg_segment) << _T("/") << ksi(pmcd->fpt_avg_girder)
          << _T(" ksi, phi = ") << pmcd->Phi << _T(", C = ") << kip(pmcd->C) << _T(" kip, dc = ") << in(pmcd->dc) << _T(" in, de = ") << in(pmcd->de) << _T(" in, dt = ") << in(pmcd->dt)
          << _T(" in, moment arm = ") << in(pmcd->MomentArm) << _T(" in, losses = ") << ksi(check_loss) << _T(" ksi"));
      DLOG(_T("   cracking moment : Mcr = ") << kipft(cmd.Mcr) << _T(" kip-ft, Mdnc = ") << kipft(cmd.Mdnc) << _T(" kip-ft, fcpe = ") << ksi(cmd.fcpe) << _T(" ksi, fr = ") << ksi(cmd.fr)
          << _T(" ksi, Sb = ") << WBFL::Units::ConvertFromSysUnits(cmd.Sb, WBFL::Units::Measure::Inch3) << _T(" in^3, Sbc = ") << WBFL::Units::ConvertFromSysUnits(cmd.Sbc, WBFL::Units::Measure::Inch3)
          << _T(" in^3, Mcr limit = ") << kipft(cmd.McrLimit) << _T(" kip-ft"));
   };

   // Used to detect the refinement loop failing at the same place with the same design state over and over
   std::set<std::_tstring> failureStates;
   IndexType nRestarts = 0;
#endif

   // One row per POI. D/C = Mu/phiMn, c/de = reinforcement ratio (limit in parentheses)
   DLOG(_T("        x (ft)    POI |  phiMn (kip-ft)  Mu (kip-ft)    D/C  phiMn,min (kip-ft) |  c/de (limit)  | result"));

   auto poiIter(vPoi.begin());
   auto poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      CHECK_PROGRESS;

      const pgsPointOfInterest& poi = *poiIter;

      const GDRCONFIG& config = m_StrandDesignTool->GetSegmentConfiguration();

      pgsFlexuralCapacityArtifact cap_artifact(true);
      CreateFlexuralCapacityArtifact(poi,intervalIdx,limitState,config,true,&cap_artifact); // positive moment

#if defined ENABLE_DESIGN_LOGGING
      if (pgsDesignLog::IsEnabled())
      {
         using namespace pgsDesignLog;
         Float64 capacity = cap_artifact.GetCapacity();
         Float64 demand = cap_artifact.GetDemand();
         std::_tostringstream osRow;
         osRow << Fixed(ft(poi.GetDistFromStart()), 3, 14) << std::setw(7) << poi.GetID()
               << _T(" | ") << Fixed(kipft(capacity), 1, 15) << Fixed(kipft(demand), 1, 13) << Fixed(IsZero(capacity) ? 0.0 : demand/capacity, 3, 7) << Fixed(kipft(cap_artifact.GetMinCapacity()), 1, 20)
               << _T(" | ") << Fixed(cap_artifact.GetMaxReinforcementRatio(), 3, 6) << _T(" (") << Fixed(cap_artifact.GetMaxReinforcementRatioLimit(), 3) << _T(")")
               << _T(" | ") << (cap_artifact.Passed() ? _T("OK") : capacity < demand ? _T("FAIL: phiMn < Mu") :
                                cap_artifact.GetMaxReinforcementRatioLimit() < cap_artifact.GetMaxReinforcementRatio() ? _T("FAIL: over reinforced") : _T("FAIL: phiMn < phiMn,min"));
         DLOG(osRow.str());

         if (!cap_artifact.Passed() || IsDetailEnabled())
         {
            logCapacityDetails(poi, config);
         }
      }
#endif // ENABLE_DESIGN_LOGGING

      if ( !cap_artifact.Passed() )
      {
#if defined ENABLE_DESIGN_LOGGING
         // If this POI already failed with exactly the same design state, the refinement is not making progress.
         // This is how an endless loop shows up in the log.
         if (pgsDesignLog::IsEnabled())
         {
            std::_tostringstream osState;
            osState << poi.GetID() << _T(" ") << m_StrandDesignTool->GetDesignStateSummary();
            if (!failureStates.insert(osState.str()).second)
            {
               LOG_WARN(_T("POI ") << poi.GetID() << _T(" failed before with the identical design state (") << m_StrandDesignTool->GetDesignStateSummary()
                        << _T(") - ultimate moment refinement is not making progress and may loop indefinitely (restart ") << nRestarts << _T(")"));
            }
         }
#endif

         // Check Ultimate Capacity
         Float64 capacity = cap_artifact.GetCapacity();
         Float64 demand  = cap_artifact.GetDemand();
         if ( capacity < demand )
         {
            LOG_FAIL(_T("phiMn = ") << pgsDesignLog::kipft(capacity) << _T(" < Mu = ") << pgsDesignLog::kipft(demand) << _T(" kip-ft at x = ") << pgsDesignLog::ft(poi.GetDistFromStart())
                     << _T(" ft (POI ") << poi.GetID() << _T("). Try adding strands"));
            StrandIndexType curr_strands = m_StrandDesignTool->GetNumPermanentStrands();
            StrandIndexType max_strands = m_StrandDesignTool->GetMaxPermanentStrands();

            bool success=false;
            if (max_strands <= curr_strands)
            {
               LOG_FAIL(_T("Already at the maximum number of permanent strands (") << max_strands << _T(") - cannot add more"));
               success = false;
            }
            else
            {
               // Need to add more strands.
               // It is faster to approximate the number of strand required
               // than just bumping to the next number of strands
               
               // approximate capacity per strand 
               // num strands = demand/(capacity per strand) - or -
               // num strands = ( curr_strands/capacity )(demand)
               StrandIndexType new_num = (Uint16)Round( (Float64)curr_strands * demand/capacity);

               // Limit max new to 10% of max possible, so we don't overshoot
               StrandIndexType max_new_num = curr_strands + max_strands/10;
               new_num = Min(new_num, max_new_num);

               new_num = m_StrandDesignTool->GetNextNumPermanentStrands(new_num);

               // Make sure we actually add some strands
               StrandIndexType min_new_num = m_StrandDesignTool->GetNextNumPermanentStrands(curr_strands);
               new_num = Max(new_num, min_new_num);

               if (new_num < max_strands)
               {
                  DLOG(_T("Mu/phiMn = ") << (demand/capacity) << _T(" -> try Np = ") << new_num << _T(" (currently ") << curr_strands << _T(")"));
               }
               else
               {
                  DLOG(_T("Estimated Np (") << new_num << _T(") is at or above the maximum - try the maximum Np = ") << max_strands);
                  new_num = max_strands;
               }

               success = m_StrandDesignTool->SetNumPermanentStrands(new_num);
            }

            if ( !success )
            {
               DLOG(_T("Could not add strands - last resort is to increase concrete strength by 500 psi"));
               // Last resort, increase strengths by 500 psi and restart
               //
               //////////////////////////////////
               // RAB - 11/5/08
               // This is only going to make a difference if this is a non-composite section
               // the better choice is to bump the slab strength. However, this is very
               // broad reaching effects that PGSuper isn't set up to handle yet.
               //
               // Deck concrete parameters, Ec will have to parameterized as well as
               // composite section properties.
               //////////////////////////////////
               bool success = m_StrandDesignTool->Bump500(StressCheckTask(intervalIdx, limitState, pgsTypes::Tension), pgsTypes::BottomGirder);
               if (success)
               {
                  LOG_ACTION(_T("Concrete strength increased by 500 psi for ultimate moment - design must restart"));
                  DESIGN_LOG_SCOPE_RESULT(_T("-> design changed (f'c +500 psi)"));
                  m_DesignerOutcome.SetOutcome(pgsDesignCodes::ChangedForUltimate);
                  m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciIncreased);
                  m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcIncreased);
                  break;
               }
               else
               {
                  LOG_ABORT(_T("Cannot add strands or increase concrete strength to satisfy ultimate moment"));
                  DESIGN_LOG_SCOPE_RESULT(_T("-> ABORT: UltimateMomentCapacity"));
                  m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::UltimateMomentCapacity);
                  m_DesignerOutcome.AbortDesign();
                  return;
               }
            }
            else
            {
               LOG_ACTION(_T("Np changed from ") << curr_strands << _T(" to ") << m_StrandDesignTool->GetNumPermanentStrands() << _T(" for ultimate moment"));
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::ChangedForUltimate);
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);

               // Compute new capacity to see if we are increasing. If not, we need another strategy
               const GDRCONFIG& new_config = m_StrandDesignTool->GetSegmentConfiguration();
               pgsFlexuralCapacityArtifact new_cap_artifact(true);
               CreateFlexuralCapacityArtifact(poi,intervalIdx,limitState,new_config,true,&new_cap_artifact); // positive moment
               Float64 new_capacity = new_cap_artifact.GetCapacity();
               DLOG(_T("phiMn with the added strands = ") << pgsDesignLog::kipft(new_capacity) << _T(" kip-ft (was ") << pgsDesignLog::kipft(capacity) << _T(" kip-ft)"));

               if (new_capacity < capacity)
               {
                  LOG_WARN(_T("Adding strands did not increase phiMn - restoring Np = ") << curr_strands << _T(" and trying to increase concrete strength instead"));
                  success = m_StrandDesignTool->SetNumPermanentStrands(curr_strands);

                  bool success = m_StrandDesignTool->Bump500(StressCheckTask(intervalIdx, limitState, pgsTypes::Tension), pgsTypes::BottomGirder);
                  if (success)
                  {
                     DESIGN_LOG_SCOPE_RESULT(_T("-> design changed (f'c +500 psi)"));
                     m_DesignerOutcome.SetOutcome(pgsDesignCodes::ChangedForUltimate);
                     m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcIncreased);
                     return;
                  }
                  else
                  {
                     LOG_ABORT(_T("Could not increase concrete strength either - outcome set to UltimateMomentCapacity and design flagged as aborted, but the POI scan continues"));
                     m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::UltimateMomentCapacity);
                     m_DesignerOutcome.AbortDesign();
#pragma Reminder("BUG - infinite loop: design is aborted here but there is no return, so the POI scan restarts with the same strands and fails again forever")
                     // KNOWN BUG (infinite loop): AbortDesign() is called but execution falls through to the
                     // POI rescan below. Strands were just restored to curr_strands and f'c could not be raised,
                     // so the design state is unchanged - the same POI fails again, the same strands are added,
                     // capacity again does not increase, and this repeats forever. The DoDesign loop never regains
                     // control, so nIterMax does not stop it. Seen in Designer_x64.log (2026-09-24) as 101+ identical
                     // "Ultimate Flexural Capacity Artifact failed" cycles. The new log reports it as
                     // "[WARN] POI nnn failed before with the identical design state".
                     // Likely fix: return here (the outcome and abort flag are already set). See devdocs/DesignerRefinements.md.
                     // Also note: "poiIter = vPoi.begin(); continue;" below skips vPoi[0] on every rescan
                     // because the for-loop increment runs after continue.
                  }
               }

               DESIGN_LOG_ONLY(nRestarts++);
               DLOG(_T("Rescanning POIs with the new design state (rescan ") << nRestarts << _T(")"));
               poiIter = vPoi.begin();
               continue;
            }
         }

         // Check Maximum Reinforcement
         if ( cap_artifact.GetMaxReinforcementRatioLimit() < cap_artifact.GetMaxReinforcementRatio() )
         {
            // No adjustment to be made. Use a bigger section
            LOG_FAIL(_T("Section is over reinforced (c/de = ") << cap_artifact.GetMaxReinforcementRatio() << _T(" > ") << cap_artifact.GetMaxReinforcementRatioLimit() << _T(") at x = ") << pgsDesignLog::ft(poi.GetDistFromStart()) << _T(" ft - only option is to increase concrete strength by 500 psi"));
            bool bSuccess = m_StrandDesignTool->Bump500(StressCheckTask(intervalIdx, limitState, pgsTypes::Tension), pgsTypes::BottomGirder);
            if (bSuccess)
            {
               LOG_ACTION(_T("Concrete strength increased for over-reinforced section - design must restart"));
               DESIGN_LOG_SCOPE_RESULT(_T("-> design changed (f'c +500 psi, over reinforced)"));
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::ChangedForUltimate);
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciIncreased);
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcIncreased);
               return;
            }
            else
            {
               LOG_ABORT(_T("Section is over reinforced and concrete strength cannot be increased"));
               DESIGN_LOG_SCOPE_RESULT(_T("-> ABORT: OverReinforced"));
               m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::OverReinforced);
               m_DesignerOutcome.AbortDesign();
               return;
            }
         }

         // Check Minimum Reinforcement
         if ( cap_artifact.GetCapacity() < cap_artifact.GetMinCapacity() )
         {
           LOG_FAIL(_T("Minimum reinforcement: phiMn = ") << pgsDesignLog::kipft(cap_artifact.GetCapacity()) << _T(" < phiMn,min = ") << pgsDesignLog::kipft(cap_artifact.GetMinCapacity()) << _T(" kip-ft at x = ") << pgsDesignLog::ft(poi.GetDistFromStart()) << _T(" ft - try adding strands"));

           if ( !m_StrandDesignTool->AddStrands() )
           {
              LOG_ABORT(_T("Cannot add strands to satisfy minimum reinforcement"));
              DESIGN_LOG_SCOPE_RESULT(_T("-> ABORT: UnderReinforced"));
              m_StrandDesignTool->SetOutcome(pgsSegmentDesignArtifact::UnderReinforced);
              m_DesignerOutcome.AbortDesign();
              return;
           }
           else
           {
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::ChangedForUltimate);
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
               DESIGN_LOG_ONLY(nRestarts++);
               DLOG(_T("Rescanning POIs with the new design state (rescan ") << nRestarts << _T(")"));
               poiIter = vPoi.begin();
               continue;
           }
         }
      }
   }

   if (m_DesignerOutcome.GetOutcome(pgsDesignCodes::ChangedForUltimate) )
   {
      // set minimum number of strands for next design iteration
      StrandIndexType min_strands = m_StrandDesignTool->GetNumPermanentStrands();
      DLOG(_T("Minimum number of permanent strands for later iterations set to ") << min_strands << _T(" (controlled by ultimate moment)"));
      DESIGN_LOG_SCOPE_RESULT(_T("-> design changed, Np = ") << min_strands);
      m_StrandDesignTool->SetMinimumPermanentStrands(min_strands);
   }
}

// Stirrup Design
void pgsDesigner2::DesignShear(pgsSegmentDesignArtifact* pArtifact, bool bDoStartFromScratch, bool bDoDesignFlexure) const
{
   DESIGN_LOG_SCOPE(_T("DesignShear"));
   const CSegmentKey& segmentKey = pArtifact->GetSegmentKey();
   ATLASSERT(segmentKey.segmentIndex == 0); // only design with PGSuper and there is only one segment
   const Float64 one_inch = WBFL::Units::ConvertToSysUnits(1.0, WBFL::Units::Measure::Inch); // Very US bias here

   GET_IFACE2(GetBroker(),IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount()-1;

   // We only iterate on shear design if Long Reinf for Shear design runs into the case where
   // stirrup tightening is a remedy.
   m_ShearDesignTool.SetLongShearCapacityRequiresStirrupTightening(false);

   GET_IFACE2(GetBroker(),IMaterials, pMaterials);
   bool bUHPC = IsUHPC(pMaterials->GetSegmentConcreteType(segmentKey));
   ATLASSERT(!bUHPC); // not supporting UHPC design at this time.

   bool bIter = true;
   while(bIter)
   {
      // Initialize shear design tool using flexure design pois
      PoiList vPoi;
      m_StrandDesignTool->GetDesignPoi(intervalIdx, 0, &vPoi);
      m_ShearDesignTool.ResetDesign( vPoi );

      // First step here is to perform a shear spec check. We will use the results later for
      // design if needed
      GET_IFACE2(GetBroker(),IShear,pShear);
      CShearData2 shear_data( *pShear->GetSegmentShearData(segmentKey) );
      if (bDoStartFromScratch)
      {
         // From-scratch stirrup layout - do initial check with minimal stirrups

         // Minimal stirrups are needed so we don't use equations for Beta for less than min stirrup configuration
         CShearData2 default_data; // Use defaults from constructor to create no-stirrup condition
         if (bUHPC)
         {
            // stirrups are not required for UHPC
            shear_data.ShearZones.clear();
         }
         else
         {
            shear_data.ShearZones = default_data.ShearZones;
            shear_data.ShearZones.front().VertBarSize = WBFL::Materials::Rebar::Size::bs5;
            shear_data.ShearZones.front().BarSpacing = 24.0 * one_inch;
            shear_data.ShearZones.front().nVertBars = 2;
         }
         shear_data.HorizontalInterfaceZones = default_data.HorizontalInterfaceZones;
         shear_data.bIsRoughenedSurface = m_ShearDesignTool.GetIsTopFlangeRoughened();
         shear_data.bUsePrimaryForSplitting = m_ShearDesignTool.GetDoPrimaryBarsProvideSplittingCapacity();
      }

      pArtifact->SetShearData(shear_data);

      // Get data needed for check
      GDRCONFIG config = pArtifact->GetSegmentConfiguration();

      // Use check artifact in design tool
      pgsStirrupCheckArtifact* pstirrup_check_artif = m_ShearDesignTool.GetStirrupCheckArtifact();

      // Do the Check
      CheckShear(true, segmentKey, intervalIdx, pgsTypes::StrengthI, &config, pstirrup_check_artif);

      GET_IFACE2(GetBroker(),ILiveLoads,pLiveLoads);
      if (pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit))
      {
         CheckShear(true, segmentKey, intervalIdx, pgsTypes::StrengthII, &config, pstirrup_check_artif);
      }

      if (!bDoStartFromScratch && pstirrup_check_artif->Passed())
      {
         // Performed spec check on existing input stirrup layout and it passed. 
         // No need to do new design
         pArtifact->SetNumberOfStirrupZonesDesigned( shear_data.ShearZones.size() );
         pArtifact->SetShearData(shear_data);
         pArtifact->AddDesignNote(pgsSegmentDesignArtifact::dnExistingShearDesignPassedSpecCheck);
      }
      else
      {
         // We are designing...
         ATLASSERT(m_CriticalSections.size() == 2);
         const pgsPointOfInterest& leftCS(m_CriticalSections.front().first.GetPointOfInterest());
         const pgsPointOfInterest& rightCS(m_CriticalSections.back().first.GetPointOfInterest());
         ATLASSERT(leftCS.GetID() != INVALID_ID);
         ATLASSERT(rightCS.GetID() != INVALID_ID);

         pgsShearDesignTool::ShearDesignOutcome sdo = m_ShearDesignTool.DesignStirrups(leftCS.GetDistFromStart(), rightCS.GetDistFromStart());

         if (sdo == pgsShearDesignTool::sdRestartWithAdditionalLongRebar)
         {
            // Additional rebar is needed for long reinf for shear. Add bars, if possible
            Float64 av_add = m_ShearDesignTool.GetRequiredAsForLongReinfShear();

            WBFL::Materials::Rebar::Grade barGrade;
            WBFL::Materials::Rebar::Type barType;
            pMaterials->GetSegmentTransverseRebarMaterial(segmentKey,&barType,&barGrade);
            const auto* pool = WBFL::LRFD::RebarPool::GetInstance();
            ATLASSERT(pool != nullptr);

            Float64 max_agg_size = pMaterials->GetSegmentMaxAggrSize(segmentKey); // for 1.33 max agg size for bar spacing
            Float64 fiber_length = pMaterials->GetSegmentConcreteFiberLength(segmentKey); // for 1.0 * max fiber length

            GET_IFACE2(GetBroker(),IGirder,pGirder);
            Float64 wFlange = pGirder->GetBottomWidth(pgsPointOfInterest(segmentKey, 0.0));
            Float64 spacing_width = wFlange - 2*one_inch; // this is the c-c width of the two outer-most bars
                                                          // this will equal (nbars-1)*spacing

            Float64 nbars = 0;
            Float64 spacing = 0;
            WBFL::Materials::Rebar::Size barSize;
            bool bBarSpacingOK = false;
            WBFL::Materials::Rebar::Size barSizes[] = {WBFL::Materials::Rebar::Size::bs5,WBFL::Materials::Rebar::Size::bs6,WBFL::Materials::Rebar::Size::bs7};
            int nBarSizes = sizeof(barSizes)/sizeof(WBFL::Materials::Rebar::Size);
            for ( int i = 0; i < nBarSizes; i++ )
            {
               barSize = barSizes[i];
               const auto* pRebar = pool->GetRebar(barType,barGrade,barSize);
               Float64 av_onebar = pRebar->GetNominalArea();
               Float64 db = pRebar->GetNominalDimension();

               // min clear spacing per 5.10.3.1.2 (NOTE: this is really intended for longitudinal bars)
               Float64 min_clear = Max(one_inch,1.33*max_agg_size,db,1.0*fiber_length);
               Float64 min_bar_spacing = min_clear + db;

               nbars = av_add/av_onebar;
               nbars = CeilOff(nbars, 1.0); // round up to next bar increment

               // Make sure spacing fits in girder
               if ( nbars == 1 )
               {
                  spacing = 0;
                  bBarSpacingOK = true;
                  break;
               }
               else
               {
                  Float64 dspacing = spacing_width/(nbars-1);
                  spacing = FloorOff(dspacing, one_inch/4); // try for a reasonable spacing
                  if (spacing == 0.0)
                  {
                     spacing = dspacing; // take any old spacing
                  }

                  if ( min_bar_spacing < spacing )
                  {
                     bBarSpacingOK = true;
                     break; // we have a spacing that works or there is only one bar so spacing is irrelevant
                  }
               }
            }

            if ( !bBarSpacingOK )
            {
               // could not find a bar spacing that works
               pArtifact->SetOutcome(pgsSegmentDesignArtifact::TooManyBarsForLongReinfShear);
               m_DesignerOutcome.AbortDesign();
            }

            // Add row of bars
            CLongitudinalRebarData& rebar_data = pArtifact->GetLongitudinalRebarData();

            CLongitudinalRebarData::RebarRow row;
            row.BarSize = barSize;
            row.Cover = 2.0*one_inch;
            row.Face = pgsTypes::BottomFace;
            row.NumberOfBars = (Int32)nbars;
            row.BarSpacing = spacing;

            rebar_data.RebarRows.push_back(row);

            pArtifact->SetWasLongitudinalRebarForShearDesigned(true);
         }
         else if (sdo == pgsShearDesignTool::sdRestartWithAdditionalStrands)
         {
            // Additional strands are needed for long reinf for shear.
            // We can only make this adjustment if flexure design is turned on (no use in adding strands
            // if concrete strengths can't be adjusted).
            if (!bDoDesignFlexure)
            {
               pArtifact->SetOutcome(pgsSegmentDesignArtifact::StrandsReqdForLongReinfShearAndFlexureTurnedOff);
               m_DesignerOutcome.AbortDesign();
            }
            else
            {
               // We can add strands?
               // Find area of current strands, attempt to add required
               Float64 av_add = m_ShearDesignTool.GetRequiredAsForLongReinfShear();

               GET_IFACE2(GetBroker(),IMaterials,pMaterial);
               Float64 aone_strand = pMaterial->GetStrandMaterial(segmentKey, pgsTypes::Straight)->GetNominalArea(); // assume straight strands are used to make LRS tie

               Float64 nstrands = av_add/aone_strand; // Additional strands needed
               nstrands = CeilOff(nstrands, 1.0);

               StrandIndexType numNp = m_StrandDesignTool->GetNumPermanentStrands();
               StrandIndexType minNp = numNp + (StrandIndexType)nstrands - 1;
               StrandIndexType nextNp = m_StrandDesignTool->GetNextNumPermanentStrands(minNp);

               // Tricky:
               // Experience has shown that adding more than 10% additional strands for LRS is likely to end in failure,
               // or at least a lousy flexural design. If this is the first time, let's try tightening up the stirrup 
               // layout before we do stirrup layout instead. This only works for from-scratch designs
               // This will require another trip through the shear algorithm
               if (1.1*numNp < nextNp && !m_ShearDesignTool.GetLongShearCapacityRequiresStirrupTightening() && bDoStartFromScratch)
               {
                  m_ShearDesignTool.SetLongShearCapacityRequiresStirrupTightening(true); // Tell algorithm to tighten layout next time through
                  pArtifact->AddDesignNote(pgsSegmentDesignArtifact::dnStirrupsTightendedForLongReinfShear); // give user a note
                  continue; // cycle back through shear design
               }

               bool it_worked=true;
               if ( 0 < nextNp)
               {
                  if (m_StrandDesignTool->SetNumPermanentStrands(nextNp))
                  {
                     DLOG(_T("Minimum number of strands set to control long reinf shear = ")<<nextNp);
                     m_StrandDesignTool->SetMinimumPermanentStrands(nextNp);
                  }
                  else
                  {
                     it_worked = false;
                  }
               }
               else
               {
                  it_worked = false;
               }

               if (!it_worked)
               {
                  m_DesignerOutcome.AbortDesign();
                  pArtifact->SetOutcome(pgsSegmentDesignArtifact::TooMuchStrandsForLongReinfShear);
               }
               else
               {
                  m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
                  pArtifact->AddDesignNote(pgsSegmentDesignArtifact::dnStrandsAddedForLongReinfShear); // give user a note
               }
            }
         }
         else if (sdo == pgsShearDesignTool::sdDesignFailedFromShearStress)
         {
            // Strut and tie required - see if we can find a f'c that will work
            Float64 fcreqd = m_ShearDesignTool.GetFcRequiredForShearStress();

            if (fcreqd < m_StrandDesignTool->GetMaximumConcreteStrength())
            {
               m_StrandDesignTool->UpdateConcreteStrengthForShear(fcreqd, intervalIdx, pgsTypes::StrengthI);
               pArtifact->AddDesignNote(pgsSegmentDesignArtifact::dnConcreteStrengthIncreasedForShearStress);
            }
            else
            {
               // We can't increase concrete strength enough. Just issue message
               pArtifact->AddDesignNote(pgsSegmentDesignArtifact::dnShearRequiresStrutAndTie);
            }
         }
         else if (sdo != pgsShearDesignTool::sdSuccess)
         {
            ATLASSERT(false);
            m_DesignerOutcome.AbortDesign();
         }
      }

      // Design is done;
      bIter = false;
   }
}

//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

bool pgsDesigner2::CollapseZoneData(CShearZoneData zoneData[MAX_ZONES], ZoneIndexType numZones) const
{
   // Return true if last two zones have equivalent stirrups
   if (numZones < 2)
   {
      return false;
   }

   if (zoneData[numZones-2].VertBarSize != zoneData[numZones-1].VertBarSize)
   {
      return false;
   }

   if (zoneData[numZones-2].BarSpacing != zoneData[numZones-1].BarSpacing)
   {
      return false;
   }

   // two zones are equivalent - make 1st zone longer
   zoneData[numZones-2].ZoneLength += zoneData[numZones-1].ZoneLength;
   return true;
}

void pgsDesigner2::GetBridgeAnalysisType(GirderIndexType gdr,const StressCheckTask& task,pgsTypes::BridgeAnalysisType& batTop,pgsTypes::BridgeAnalysisType& batBottom) const
{
   // Compression stresses are greatest at the top of the girder using the maximum model in Envelope mode. 
   // Tensile stresses are greatest at the bottom of the girder using the maximum model in Envelope mode. 
   // In all other modes, Min/Max are the same
   GET_IFACE2(GetBroker(),IProductForces,pProdForces);
   batTop    = pProdForces->GetBridgeAnalysisType(task.stressType == pgsTypes::Compression ? pgsTypes::Maximize : pgsTypes::Minimize);
   batBottom = pProdForces->GetBridgeAnalysisType(task.stressType == pgsTypes::Compression ? pgsTypes::Minimize : pgsTypes::Maximize);
}

void pgsDesigner2::DumpLiftingArtifact(const WBFL::Stability::LiftingStabilityProblem* pStabilityProblem,std::shared_ptr<const WBFL::Stability::LiftingCheckArtifact> artifact,WBFL::Debug::LogContext& os) const
{
   os << _T("Dump for WBFL::Stability::LiftingCheckArtifact") << WBFL::Debug::endl;
   os << _T("===================================") << WBFL::Debug::endl;

   os <<_T(" Stress Artifacts")<< WBFL::Debug::endl;
   os << _T("================") << WBFL::Debug::endl;
   const WBFL::Stability::LiftingResults& results = artifact->GetLiftingResults();
   for(const auto& sectionResult : results.vSectionResults)
   {
      const auto& pAnalysisPoint = pStabilityProblem->GetAnalysisPoint(sectionResult.AnalysisPointIndex);
      Float64 loc = pAnalysisPoint->GetLocation();
      os <<_T("At ") << WBFL::Units::ConvertFromSysUnits(loc,WBFL::Units::Measure::Feet) << _T(" ft: ");

      // NOTE: min_stress and max_stress are backwards to match the original log file dump code from pgsLiftingAnalysisArtifact
      Float64 min_stress = Max(sectionResult.fMaxDirect[+WBFL::Stability::GirderFace::Top],sectionResult.fMaxDirect[+WBFL::Stability::GirderFace::Bottom]);
      Float64 max_stress = Min(sectionResult.fMinDirect[+WBFL::Stability::GirderFace::Top],sectionResult.fMinDirect[+WBFL::Stability::GirderFace::Bottom]);
      os<<_T("Total Stress: Min =")<<WBFL::Units::ConvertFromSysUnits(min_stress,WBFL::Units::Measure::KSI)<<_T("ksi, Max=")<<WBFL::Units::ConvertFromSysUnits(max_stress,WBFL::Units::Measure::KSI)<<_T("ksi")<< WBFL::Debug::endl;
   }

   os <<_T(" Cracking Artifacts")<< WBFL::Debug::endl;
   os << _T("==================") << WBFL::Debug::endl;
   // we don't do impact or wind for lifting so these values will work
   WBFL::Stability::ImpactDirection impact = WBFL::Stability::ImpactDirection::NoImpact;
   WBFL::Stability::WindDirection wind = WBFL::Stability::WindDirection::Left;
   for(const auto& sectionResult : results.vSectionResults)
   {
      const auto& pAnalysisPoint = pStabilityProblem->GetAnalysisPoint(sectionResult.AnalysisPointIndex);
      Float64 loc = pAnalysisPoint->GetLocation();
      os <<_T("At ") << WBFL::Units::ConvertFromSysUnits(loc,WBFL::Units::Measure::Feet) << _T(" ft: ");

      WBFL::Stability::Corner corner = sectionResult.MinFScrCorner[+impact][+wind];
      if ( corner == WBFL::Stability::Corner::TopLeft ||
           corner == WBFL::Stability::Corner::TopRight )
      {
         os << _T("Flange=TopFlange");
      }
      else
      {
         os << _T("Flange=BottomFlange");
      }

      Float64 stress = sectionResult.f[+impact][+wind][+corner];
      Float64 fs = sectionResult.FScr[+impact][+wind][+corner];
      os<<_T(" Lateral Stress = ")<<WBFL::Units::ConvertFromSysUnits(stress,WBFL::Units::Measure::KSI)<<_T("ksi, FS =")<<fs<< WBFL::Debug::endl;
   }
}
