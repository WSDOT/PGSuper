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
#include <PgsExt\PgsExt.h>
#include <IFace\Bridge.h>
#include <IFace\Alignment.h>
#include <IFace\DistributionFactors.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\StatusCenter.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Allowables.h>
#include <IFace\PrestressForce.h>
#include <IFace\MomentCapacity.h>
#include <IFace\ShearCapacity.h>
#include <IFace\Constructability.h>
#include <IFace\TransverseReinforcementSpec.h>
#include <IFace\SplittingChecks.h>
#include <IFace\PrecastIGirderDetailsSpec.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\GirderHandling.h>
#include <IFace\ResistanceFactors.h>
#include <IFace\InterfaceShearRequirements.h>
#include <IFace\Intervals.h>

#include <IFace\DocumentType.h>

#include "Designer2.h"
#include "PsForceEng.h"
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

#include <Lrfd\Rebar.h>
#include <algorithm>
#include <iterator>

#include <MathEx.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\LoadFactors.h>
#include <PgsExt\GirderLabel.h>

#include <PsgLib\GirderLibraryEntry.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\ConcreteLibraryEntry.h>

#if defined _DEBUG
#include <IFace\PointOfInterest.h>
#endif // _DEBUG

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define MIN_SPAN_DEPTH_RATIO 4

#define WITHOUT_REBAR 0
#define WITH_REBAR    1
#define TOP           0
#define BOT           1

static Float64 gs_60KSI = WBFL::Units::ConvertToSysUnits(60.0,WBFL::Units::Measure::KSI);
static Float64 gs_rowToler = WBFL::Units::ConvertToSysUnits(0.25,WBFL::Units::Measure::Inch); // strands are in same row if this tolerance

// Exception-safe utility class for reverting event holding and lldf roa during design
class AutoDesign
{
public:
   AutoDesign(IEvents* pEvents, ILiveLoads* pLiveLoads):
      m_pEvents(pEvents),
      m_pLiveLoads(pLiveLoads)
   {
      pEvents->HoldEvents();
      m_OldRoa = pLiveLoads->GetLldfRangeOfApplicabilityAction();
      if (m_OldRoa == roaEnforce)
      {
         pLiveLoads->SetLldfRangeOfApplicabilityAction(roaIgnore);
      }
   }

   ~AutoDesign()
   {
      m_pLiveLoads->SetLldfRangeOfApplicabilityAction(m_OldRoa);
      m_pEvents->CancelPendingEvents();
   }

private:
   IEvents* m_pEvents; // weak reference to interface
   ILiveLoads* m_pLiveLoads; // weak reference
   LldfRangeOfApplicabilityAction m_OldRoa;
};

bool CanDesign(pgsTypes::StrandDefinitionType type)
{
   // we can only design for these strand definition types
   // Techincally, we should not design for sdtDirectionSelection, however the designer does work. The "gotcha" is that the
   // strand defintion type gets changed to sdtStraightHarped when it should not... design should not change the strand definition type
   // but since sdtDirectSelection has been a valid choice for a long time and it does work, we'll let it go
   return (type == pgsTypes::sdtTotal || type == pgsTypes::sdtStraightHarped || type == pgsTypes::sdtDirectSelection) ? true : false;
}

/****************************************************************************
CLASS
   pgsDesigner2
****************************************************************************/

#if defined ENABLE_LOGGING
const std::_tstring g_LimitState[] =
{
   std::_tstring(_T("ServiceI")),
   std::_tstring(_T("ServiceIA")),
   std::_tstring(_T("ServiceIII")),
   std::_tstring(_T("StrengthI")),
   std::_tstring(_T("StrengthII")),
   std::_tstring(_T("FatigueI"))
};

const std::_tstring g_Type[] = 
{
   std::_tstring(_T("Tension")),
   std::_tstring(_T("Compression"))
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

void GetConfinementZoneLengths(const CSegmentKey& segmentKey, IGirder* pGdr, Float64 gdrLength, 
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
   MatchPoiOffSegment(IPointOfInterest* pIPointOfInterest) : m_pIPointOfInterest(pIPointOfInterest) {}
   bool operator()(const pgsPointOfInterest& poi) const
   {
      return m_pIPointOfInterest->IsOffSegment(poi);
   }

   IPointOfInterest* m_pIPointOfInterest;
};

////////////////////////// PUBLIC     ///////////////////////////////////////



//======================== LIFECYCLE  =======================================
pgsDesigner2::pgsDesigner2():
m_StrandDesignTool(LOGGER),
m_ShearDesignTool(LOGGER)
{
   m_bShippingDesignIgnoreConfigurationLimits = false;

#if defined _WIN64
   CREATE_LOGFILE("Designer_x64");
#else
   CREATE_LOGFILE("Designer");
#endif

}

pgsDesigner2::pgsDesigner2(const pgsDesigner2& rOther):
m_StrandDesignTool(LOGGER),
m_ShearDesignTool(LOGGER)
{
   MakeCopy(rOther);
}

pgsDesigner2::~pgsDesigner2()
{
   CLOSE_LOGFILE;
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

//======================== OPERATIONS =======================================
void pgsDesigner2::SetBroker(IBroker* pBroker)
{
   m_pBroker = pBroker;

}

void pgsDesigner2::SetStatusGroupID(StatusGroupIDType statusGroupID)
{
   m_StatusGroupID = statusGroupID;

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   m_scidLiveLoad = pStatusCenter->RegisterCallback( new pgsLiveLoadStatusCallback(m_pBroker) );
   m_scidBridgeDescriptionError = pStatusCenter->RegisterCallback( new pgsBridgeDescriptionStatusCallback(m_pBroker,eafTypes::statusError));
}

// Function we need to reuse
static Float64 GetSectionGirderOrientationEffect(const pgsPointOfInterest& poi, Float64 x, Float64 z, MatingSurfaceIndexType nMatingSurfaces,
                                          Float64 topWidth, Float64 girderTopSlope,
                                          IRoadway* pAlignment, IBridge* pBridge, IGirder* pGdr,
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

      *pCrownSlope = (ya_left - ya_right)/ Wtf;

      Float64 ya = pAlignment->GetElevation(x,z);
      if ( (ya_left < ya && ya_right < ya) || (ya < ya_left && ya < ya_right) )
      {
         pivot_crown = ya - (ya_left+ya_right)/2;
      }

      CComPtr<IPoint2dCollection> matingSurfaceProfile;
      bool bHasMSProfile = (pGdr->GetMatingSurfaceProfile(poi, 0, true, &matingSurfaceProfile) == false || matingSurfaceProfile == nullptr) ? false : true;
      if (bHasMSProfile)
      {
         CollectionIndexType nPoints;
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

      *pCrownSlope = (ya_left - ya_right)/(right_mating_surface_offset - left_mating_surface_offset);

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
   GET_IFACE(ICamber,pCamber);
   GET_IFACE(IPointOfInterest,pPoi);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IRoadway,pAlignment);
   GET_IFACE(IGirder,pGdr);
   GET_IFACE_NOCHECK(IBridgeDescription,pIBridgeDesc);

   GET_IFACE(ILibrary, pLib );
   GET_IFACE(ISpecification, pSpec );
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
      topFlangeShape = std::make_unique<WBFL::Math::PolynomialFunction>(GenerateParabola(poi_left.GetDistFromStart(), poi_right.GetDistFromStart(), sign*tft));
   }
   else
   {
      // top flange is straight
      topFlangeShape = std::make_unique<WBFL::Math::ZeroFunction>();
   }

   // the amount of top flange thickening at the start CL Bearing
   Float64 tftCLBrg = topFlangeShape->Evaluate(clBrgPoi.GetDistFromStart());

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType gceInterval = pIntervals->GetGeometryControlInterval();

   Float64 overlay_depth = pBridge->GetOverlayDepth(gceInterval);

   GET_IFACE(IDeformedGirderGeometry,pDeformedGirderGeometry);

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
      Float64 camber_effect = pCamber->GetExcessCamberEx(poi, CREEP_MAXTIME, &D, &C, pConfig );
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
      // for this reason, we subtrack off the elevation adjustment
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
      profile_effect = -diff_min; // raise haunch to accomodate
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
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IRoadway,pAlignment);
   GET_IFACE(IGirder,pGdr);

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

   std::map<CSegmentKey,const pgsHaulingAnalysisArtifact*>::iterator iter(m_HaulingAnalysisArtifacts.begin());
   std::map<CSegmentKey,const pgsHaulingAnalysisArtifact*>::iterator end(m_HaulingAnalysisArtifacts.end());
   for ( ; iter != end; iter++ )
   {
      const pgsHaulingAnalysisArtifact* pHaulingArtifact = iter->second;
      delete pHaulingArtifact;
      pHaulingArtifact = nullptr;
   }
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

const WBFL::Stability::LiftingCheckArtifact* pgsDesigner2::GetLiftingCheckArtifact(const CSegmentKey& segmentKey) const
{
   auto found = m_LiftingCheckArtifacts.find(segmentKey);
   if ( found != m_LiftingCheckArtifacts.end() )
   {
      return &(found->second);
   }

   return nullptr;
}

const pgsHaulingAnalysisArtifact* pgsDesigner2::GetHaulingAnalysisArtifact(const CSegmentKey& segmentKey) const
{
   auto found = m_HaulingAnalysisArtifacts.find(segmentKey);
   if ( found != m_HaulingAnalysisArtifacts.end() )
   {
      return found->second;
   }

   return nullptr;
}

const WBFL::Stability::LiftingCheckArtifact* pgsDesigner2::CheckLifting(const CSegmentKey& segmentKey) const
{
   // if we already have the artifact, return it
   const WBFL::Stability::LiftingCheckArtifact* pLiftingArtifact = GetLiftingCheckArtifact(segmentKey);
   if ( pLiftingArtifact )
   {
      return pLiftingArtifact;
   }

   // Nope... need to compute it
   WBFL::Stability::LiftingCheckArtifact liftingArtifact;
   pgsGirderLiftingChecker lifting_checker(m_pBroker,m_StatusGroupID);
   lifting_checker.CheckLifting(segmentKey,&liftingArtifact);

   m_LiftingCheckArtifacts.insert(std::make_pair(segmentKey,liftingArtifact));

   pLiftingArtifact = GetLiftingCheckArtifact(segmentKey);
   ATLASSERT(pLiftingArtifact != nullptr);
   return pLiftingArtifact;
}

const pgsHaulingAnalysisArtifact* pgsDesigner2::CheckHauling(const CSegmentKey& segmentKey) const
{
   return CheckHauling(segmentKey,LOGGER);
}

const pgsHaulingAnalysisArtifact* pgsDesigner2::CheckHauling(const CSegmentKey& segmentKey, SHARED_LOGFILE LOGFILE) const
{
   // if we already have the artifact, return it
   const pgsHaulingAnalysisArtifact* pHaulingArtifact = GetHaulingAnalysisArtifact(segmentKey);
   if ( pHaulingArtifact )
   {
      return pHaulingArtifact;
   }

   // Nope... need to compute it

   // Use factory function to create correct hauling checker
   pgsGirderHandlingChecker checker_factory(m_pBroker,m_StatusGroupID);
   std::unique_ptr<pgsGirderHaulingChecker> hauling_checker( checker_factory.CreateGirderHaulingChecker() );

   pHaulingArtifact = hauling_checker->CheckHauling(segmentKey,LOGFILE);
         
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

   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE(ILiveLoads,         pLiveLoads);
   GET_IFACE(IBridge,            pBridge);
   GET_IFACE(IPointOfInterest,   pPoi);

   std::shared_ptr<pgsGirderArtifact> pGdrArtifact(new pgsGirderArtifact(girderKey));

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   // warning if live load isn't defined... this would be a highly unusual case
   if (!pLiveLoads->IsLiveLoadDefined(pgsTypes::lltDesign))
   {
      std::_tstring strMsg(_T("Live load is not defined."));
      pgsLiveLoadStatusItem* pStatusItem = new pgsLiveLoadStatusItem(m_StatusGroupID,m_scidLiveLoad,strMsg.c_str());
      GET_IFACE(IEAFStatusCenter,   pStatusCenter);
      pStatusCenter->Add(pStatusItem);
   }

   // going to need this inside the loop
   GET_IFACE(ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   GET_IFACE(ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);

   GET_IFACE(IIntervals,pIntervals);
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

      GET_IFACE(IStressCheck, pStressCheck);
      std::vector<StressCheckTask> vStressCheckTasks = pStressCheck->GetStressCheckTasks(segmentKey);
      for (const auto& task : vStressCheckTasks)
      {
         bool bClosureJoints = (segIdx < nSegments - 1 && pIntervals->GetCompositeClosureJointInterval(segmentKey) <= task.intervalIdx ? true : false);

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

         CheckSegmentStresses(segmentKey, vPoi, task, pSegmentArtifact);
      } // next stress check task

      // These checks are independent of interval, so only check them once

      pProgress->UpdateMessage(_T("Checking segment details"));
      CheckSegmentDetailing(segmentKey,pSegmentArtifact);

      // Check Lifting
      if ( pSegmentLiftingSpecCriteria->IsLiftingAnalysisEnabled() )
      {
         pProgress->UpdateMessage(_T("Checking lifting"));
         const WBFL::Stability::LiftingCheckArtifact* pLiftingArtifact = CheckLifting(segmentKey);
         pSegmentArtifact->SetLiftingCheckArtifact(pLiftingArtifact);
      }

      // Check Hauling
      if ( pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled() )
      {
         pProgress->UpdateMessage(_T("Checking hauling"));
         const pgsHaulingAnalysisArtifact* pHaulingAnalysisArtifact = CheckHauling(segmentKey);
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

   pProgress->UpdateMessage(_T("Checking constructibility"));
   CheckConstructability(girderKey,pGdrArtifact->GetConstructabilityArtifact());

   // Check ultimate capacity
   pProgress->UpdateMessage(_T("Checking moment capacity"));
   CheckMomentCapacity(lastIntervalIdx,pgsTypes::StrengthI,pGdrArtifact.get());

   pProgress->UpdateMessage(_T("Checking shear capacity"));
   CheckShear(lastIntervalIdx,pgsTypes::StrengthI,pGdrArtifact.get());

   GET_IFACE(ILimitStateForces,pLimitStateForces);
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

   // add the artifact to the cache
   m_CheckArtifacts.insert( std::make_pair(girderKey,pGdrArtifact) );

   pTheGdrArtifact = GetGirderArtifact(girderKey);// get the artifact from the cache.... this is what we want to return
   ATLASSERT(pTheGdrArtifact != nullptr); 
   return pTheGdrArtifact;
}

void CheckProgress(IProgress* pProgress)
{
   if ( pProgress->Continue() != S_OK )
   {
      //LOG(_T("*#*#*#*#* DESIGN CANCELLED BY USER *#*#*#*#*"));
      throw pgsSegmentDesignArtifact::DesignCancelled;
   }
}

void pgsDesigner2::ConfigureStressCheckTasks(const CSegmentKey& segmentKey) const
{
   // Configure the stress check tasks
   GET_IFACE(IStressCheck, pStressCheck);
   m_StressCheckTasks = pStressCheck->GetStressCheckTasks(segmentKey,true/*design*/);
}

#define CHECK_PROGRESS CheckProgress(pProgress)
pgsGirderDesignArtifact pgsDesigner2::Design(const CGirderKey& girderKey,const std::vector<arDesignOptions>& desOptionsColl) const
{
   // The design artifact
   ASSERT_GIRDER_KEY(girderKey);
   pgsGirderDesignArtifact artifact(girderKey);

   GET_IFACE(IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   ATLASSERT(nSegments == 1); // Design only works for prestressed girders (PGSuper) so there should only be one segment per girder

   // We don't design for time-step analysis
   GET_IFACE(ILossParameters,pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      // we don't design for time-step method so just return the empty artifact
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         pgsSegmentDesignArtifact segArtifact(CSegmentKey(girderKey,segIdx));
         segArtifact.SetOutcome(pgsSegmentDesignArtifact::DesignNotSupported_Losses);
         artifact.AddSegmentDesignArtifact(segIdx,segArtifact);
      }
      return artifact;
   }

   GET_IFACE(IMaterials, pMaterials);
   if (IsUHPC(pMaterials->GetSegmentConcreteType(CSegmentKey(girderKey, 0))))
   {
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
   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);
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
   GET_IFACE(IEvents,pEvents);
   GET_IFACE(ILiveLoads,pLiveLoads);
   AutoDesign myAutoDes(pEvents, pLiveLoads);

   try 
   {
      std::vector<arDesignOptions>::const_iterator designOptionIter(desOptionsColl.begin());
      std::vector<arDesignOptions>::const_iterator designOptionIterEnd(desOptionsColl.end());
      for ( ; designOptionIter != designOptionIterEnd; designOptionIter++ )
      {
         const arDesignOptions& options = *designOptionIter;

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
         }

         if ( bSuccess || designOptionIter == designOptionIterEnd-1 )
         {
            // if the design succeeded or if we are out of design options, we are done
            break;
         }
      }
   }
   catch (pgsSegmentDesignArtifact::Outcome outcome)
   {
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
   
   GET_IFACE(ILosses, pLosses);
   pLosses->ClearDesignLosses();

   return artifact;
}

void pgsDesigner2::DoDesign(const CGirderKey& girderKey,const arDesignOptions& options, pgsGirderDesignArtifact& girderDesignArtifact) const
{
#if defined _DEBUG
   GET_IFACE(IDocumentType,pDocType);
   ATLASSERT(pDocType->IsPGSuperDocument());
#endif

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount()-1;

   GET_IFACE(ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress,0,PW_ALL | PW_NOGAUGE); // progress window has a cancel button

   GET_IFACE_NOCHECK(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData*    pGroup      = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const CSplicedGirderData*  pGirder     = pGroup->GetGirder(girderKey.girderIndex);

   GET_IFACE_NOCHECK(ISpecification, pSpec);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE_NOCHECK(IGirder, pGdr);
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

      LOG(_T("************************************************************"));
      LOG(_T("Beginning design for span ") << LABEL_SPAN(spanIdx) << _T(" girder ") << LABEL_GIRDER(gdrIdx));

      const CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
      if ( !CanDesign(pSegment->Strands.GetStrandDefinitionType()) )
      {
         LOG(_T("-----------------"));
         LOG(_T("Cannot design with the current strand definition type"));

         artifact.SetOutcome(pgsSegmentDesignArtifact::DesignNotSupported_Strands);
         girderDesignArtifact.AddSegmentDesignArtifact(segIdx, artifact);
         continue; // process next segment
      }

#if defined ENABLE_LOGGING
      WBFL::System::Time startTime;
#endif

      ConfigureStressCheckTasks(segmentKey);

      std::_tostringstream os;
      os << _T("Designing Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << std::ends;
      pProgress->UpdateMessage(os.str().c_str());

      // initialize temporary top strand usage type
      // we prefer pretensioned TTS so set it to that
      // however if there is precamber, TTS must be post-tensioned

      artifact.SetTemporaryStrandUsage(IsZero(pGdr->GetPrecamber(segmentKey)) ? pgsTypes::ttsPretensioned : pgsTypes::ttsPTBeforeLifting);


      // initialize lifting and hauling to current values
      GET_IFACE(ISegmentLifting,pSegmentLifting);
      Float64 Loh = pSegmentLifting->GetLeftLiftingLoopLocation(segmentKey);
      Float64 Roh = pSegmentLifting->GetRightLiftingLoopLocation(segmentKey);
      artifact.SetLiftingLocations(Loh,Roh);

      GET_IFACE(ISegmentHauling,pSegmentHauling);
      Loh = pSegmentHauling->GetTrailingOverhang(segmentKey);
      Roh = pSegmentHauling->GetLeadingOverhang(segmentKey);
      artifact.SetTruckSupportLocations(Loh,Roh);
      artifact.SetHaulTruck(pSegmentHauling->GetHaulTruck(segmentKey));

      // Copy current longitudinal rebar data to the artifact. 
      // This algorithm will only add more rebar to existing, and only
      // for the longitudinal reinf for shear condition
      artifact.SetLongitudinalRebarData( pSegment->LongitudinalRebarData );


      Float64 segment_length = pBridge->GetSegmentLength(segmentKey);

      // Use strand design tool to control proportioning of strands
      m_StrandDesignTool.Initialize(m_pBroker, m_StatusGroupID, &artifact);

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
         artifact.SetOutcome(pgsSegmentDesignArtifact::NoDesignRequested);
         girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
         continue; // process next segment
      }

      Int16 cIter = -1;
      Int16 nIterMax = 30;
      bool bDone = false;
      do
      {
         CHECK_PROGRESS;

         cIter++;
         LOG(_T(""));
         LOG(_T("Design Iteration Number # ") << cIter );
         std::_tostringstream os2;
         os2 << _T("Design Iteration ")<<cIter+1<<_T(" for Span ") << LABEL_SPAN(spanIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx) << std::ends;

         pProgress->UpdateMessage(os2.str().c_str());

         if (options.doDesignForFlexure!=dtNoDesign)
         {
            // reset outcomes
            bool keep_prop = false;
            if (m_DesignerOutcome.DidConcreteChange() && m_StrandDesignTool.IsDesignSlabOffset())
            {
               LOG(_T("Concrete changed on last iteration. Reset min slab offset to zero"));
               m_StrandDesignTool.SetMinimumSlabOffset(0.0);
            }

            bool just_added_raised_strands = m_DesignerOutcome.DidRaiseStraightStrands();

            keep_prop = m_DesignerOutcome.DidRetainStrandProportioning();
            m_DesignerOutcome.Reset();
            if (keep_prop)
            {
               LOG(_T("Retaining strand proportioning from last iteration"));
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::RetainStrandProportioning);
            }

            // get design back on track with user preferences
            m_StrandDesignTool.RestoreDefaults(m_DesignerOutcome.DidRetainStrandProportioning(), just_added_raised_strands);

            // Design strands and concrete strengths in mid-zone
            DesignMidZone(cIter == 0 ? false : true, options,pProgress);

            if (m_DesignerOutcome.WasDesignAborted())
            {
               girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
               return; 
            }
            else if( m_DesignerOutcome.DidRaiseStraightStrands() )
            {
               LOG(_T("Raised Straight strands were added - Restarting algorithm"));
               continue;
            }

            CHECK_PROGRESS;

            LOG(_T(""));
            LOG(_T("BEGINNING DESIGN OF END-ZONES"));
            LOG(_T(""));

            m_StrandDesignTool.DumpDesignParameters();

            // Design end zones
            DesignEndZone(cIter < 2, options, artifact, pProgress);

            if ( m_DesignerOutcome.WasDesignAborted() )
            {
               girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
               return;
            }
            else if ( m_DesignerOutcome.DidConcreteChange() )
            {
               LOG(_T("End Zone Design changed concrete strength - Restart"));
               LOG(_T("=================================================="));
               continue;
            }
            else if( m_DesignerOutcome.DidRaiseStraightStrands() )
            {
               LOG(_T("Raised Straight strands were added - Restarting algorithm"));
               continue;
            }

            LOG(_T(""));
            LOG(_T("BEGINNING DESIGN REFINEMENT"));
            LOG(_T(""));

            pProgress->UpdateMessage(_T("Stress Design Refinement"));

            CHECK_PROGRESS;

            m_StrandDesignTool.DumpDesignParameters();

            // Refine design based on allowable stress criteria
            // Add and harp strands to satisfy stress criteria
            RefineDesignForAllowableStress(pProgress);

            CHECK_PROGRESS;

            if ( m_DesignerOutcome.WasDesignAborted() )
            {
               girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
               return;
            }
            else if  (  m_DesignerOutcome.DidConcreteChange())
            {
               continue;
            }

            m_StrandDesignTool.DumpDesignParameters();

            //
            // Refine Design for Ultimate Strength
            //
            LOG(_T("Refining the design for ultimate moment capacity"));
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

            LOG(_T("Strength I Limit State"));
            
            LOG_EXECUTION_TIME( RefineDesignForUltimateMoment(lastIntervalIdx, pgsTypes::StrengthI,pProgress) );

            CHECK_PROGRESS;

            if ( m_DesignerOutcome.WasDesignAborted() )
            {
               girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
               return;
            }
            else if  (  m_DesignerOutcome.GetOutcome(pgsDesignCodes::ChangedForUltimate) )
            {
               LOG(_T("Ultimate moment controlled - restart design"));
               LOG(_T("==========================================="));
               continue;
            }
            
            if ( bPermit )
            {
               pProgress->UpdateMessage(_T("Designing for Strength II Ultimate Moment"));
               LOG(_T("Strength II Limit State"));
               RefineDesignForUltimateMoment(lastIntervalIdx, pgsTypes::StrengthII,pProgress);

               CHECK_PROGRESS;
      
               if ( m_DesignerOutcome.WasDesignAborted() )
               {
                  girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
                  return;
               }
               else if  (  m_DesignerOutcome.GetOutcome(pgsDesignCodes::ChangedForUltimate) )
               {
                  LOG(_T("Ultimate moment controlled - restart design"));
                  LOG(_T("==========================================="));
                  continue;
               }
            }

            if (options.doDesignSlabOffset != sodPreserveHaunch)
            {
               pProgress->UpdateMessage(_T("Designing Slab Offset Outer Loop"));

               LOG(_T("Starting Slab Offset design in outer loop"));
               Float64 old_offset_start = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart );
               Float64 old_offset_end   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd );

               DesignSlabOffset( pProgress );

               CHECK_PROGRESS;

               if (  m_DesignerOutcome.WasDesignAborted() )
               {
                  girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
                  return;
               }
               else if ( m_DesignerOutcome.GetOutcome(pgsDesignCodes::SlabOffsetChanged) )
               {
                  Float64 new_offset_start = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
                  Float64 new_offset_end   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);

                  new_offset_start = RoundSlabOffsetValue(pSpec, new_offset_start);
                  new_offset_end   = RoundSlabOffsetValue(pSpec, new_offset_end);

                  LOG(_T("Slab Offset changed in outer loop. Set a new minimum of (Start) ") << WBFL::Units::ConvertFromSysUnits(new_offset_start,WBFL::Units::Measure::Inch)<< _T(" in and (End) ") << WBFL::Units::ConvertFromSysUnits(new_offset_end,WBFL::Units::Measure::Inch) << _T(" in - restart design"));
                  LOG(_T("========================================================================="));
                  m_StrandDesignTool.SetMinimumSlabOffset( Min(new_offset_start,new_offset_end));
                  m_StrandDesignTool.SetSlabOffset(pgsTypes::metStart,new_offset_start);
                  m_StrandDesignTool.SetSlabOffset(pgsTypes::metEnd, new_offset_end);
                  continue;
               }
               else
               {
                  m_StrandDesignTool.SetSlabOffset(pgsTypes::metStart,old_offset_start);  // restore to original value that passed all spec checks
                  m_StrandDesignTool.SetSlabOffset(pgsTypes::metEnd,  old_offset_end);   // restore to original value that passed all spec checks
                  LOG(_T("Slab Offset design Successful in outer loop. Current value is (Start) ") <<WBFL::Units::ConvertFromSysUnits( m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart),WBFL::Units::Measure::Inch)<<_T("in and (End) ")<<WBFL::Units::ConvertFromSysUnits( m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd),WBFL::Units::Measure::Inch) << _T(" in"));
                  LOG(_T("==========================================="));
               }
            }
            else
            {
               LOG(_T("Skipping Outer Slab Offset Design due to user input"));
            }
         }
         else
         {
            // flexure design was not done. Still need to fill current values in artifact.
            m_StrandDesignTool.FillArtifactWithFlexureValues();
         }

         // if we got here, we are in good shape, however strands may have been adjusted slightly.
         // let's clean the slate
         if (m_DesignerOutcome.DidGirderChange())
         {
            ATLASSERT(!m_DesignerOutcome.DidConcreteChange());
            LOG(_T("A slight adjustment was made during flexural design. Clear settings for shear design (if applicable)"));
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
               girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
               return;
            }
            else if ( m_DesignerOutcome.DidGirderChange() )
            {
               continue;
            }
         }

         // we've succussfully completed all the design steps
         // we are DONE!
         bDone = true;
      } while ( cIter < nIterMax && !bDone );

      if ( !bDone ) //&& cIter >= nIterMax )
      {
         LOG(_T("Maximum number of iteratations was exceeded - aborting design ") << cIter);
         artifact.SetOutcome(pgsSegmentDesignArtifact::MaxIterExceeded);
         girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
         return;
      }

      if (artifact.GetDesignOptions().doDesignSlabOffset != sodPreserveHaunch)
      {
         LOG(_T("Final Slab Offset before rounding (Start) ") << WBFL::Units::ConvertFromSysUnits( artifact.GetSlabOffset(pgsTypes::metStart),WBFL::Units::Measure::Inch) << _T(" in and (End) ") << WBFL::Units::ConvertFromSysUnits( artifact.GetSlabOffset(pgsTypes::metEnd),WBFL::Units::Measure::Inch) << _T(" in"));
         Float64 start_offset = RoundSlabOffsetValue(pSpec, artifact.GetSlabOffset(pgsTypes::metStart));
         Float64 end_offset   = RoundSlabOffsetValue(pSpec, artifact.GetSlabOffset(pgsTypes::metEnd));
         artifact.SetSlabOffset(pgsTypes::metStart,start_offset);
         artifact.SetSlabOffset(pgsTypes::metEnd, end_offset);
         LOG(_T("After rounding (Start) ") << WBFL::Units::ConvertFromSysUnits(start_offset,WBFL::Units::Measure::Inch) << _T(" in and (End) ") << WBFL::Units::ConvertFromSysUnits(end_offset,WBFL::Units::Measure::Inch) << _T(" in"));
      }


      m_StrandDesignTool.DumpDesignParameters();

      pProgress->UpdateMessage(_T("Design Complete"));
      LOG(_T("Design Complete for span ") << LABEL_SPAN(spanIdx) << _T(" girder ") << LABEL_GIRDER(gdrIdx));
      LOG(_T("************************************************************"));
   #if defined ENABLE_LOGGING
      WBFL::System::Time endTime;
      long duration = endTime.Seconds() - startTime.Seconds();
      long min = duration / 60;
      long sec = duration - min*60;
      LOG(_T("Design: ") << min << _T("m:") << sec << _T("s"));
   #endif

      // set controlling data for concrete strengths
      artifact.SetReleaseDesignState(m_StrandDesignTool.GetReleaseConcreteDesignState());
      artifact.SetFinalDesignState(m_StrandDesignTool.GetFinalConcreteDesignState());

      // One last possible hitch(s) here: If we needed to use a higher allowable release strength, it means
      // that we assumed that there is adequate longitudinal rebar in the model to back this assumption. 
      // We need to check this, and if there was not, the design failed; with caveats.
      bool needsAdditionalRebar(false);
      GDRCONFIG config = artifact.GetSegmentConfiguration();
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
            needsAdditionalRebar = true;

            artifact.SetOutcome(pgsSegmentDesignArtifact::SuccessButLongitudinalBarsNeeded4FlexuralTensionHauling);
            artifact.AddDesignNote(pgsSegmentDesignArtifact::dnLongitudinalBarsNeeded4FlexuralTensionHauling);
         }
      }

      // Raised strand designs use direct fill order - if no strands were raised, revert to simplified design
      if (options.doDesignForFlexure!=dtNoDesign)
      {
         m_StrandDesignTool.SimplifyDesignFillOrder(&artifact);
      }

      if (!needsAdditionalRebar)
      {
         artifact.SetOutcome(pgsSegmentDesignArtifact::Success);
      }

      girderDesignArtifact.AddSegmentDesignArtifact(segIdx,artifact);
   } // next segment
}

pgsEccEnvelope pgsDesigner2::GetEccentricityEnvelope(const pgsPointOfInterest& poi,const GDRCONFIG& config) const
{
   pgsEccEnvelope envData;

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IAllowableConcreteStress, pAllowable );
   GET_IFACE(ILimitStateForces,pLimitStateForces);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(IPretensionForce,pPrestressForce);
   GET_IFACE(ISectionProperties,pSectProps);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary);
   StrandIndexType Nt    = config.PrestressConfig.GetStrandCount(pgsTypes::Temporary);

   std::vector<StressCheckTask>::iterator iter(m_StressCheckTasks.begin());
   std::vector<StressCheckTask>::iterator end(m_StressCheckTasks.end());
   for ( ; iter != end; iter++ )
   {
      StressCheckTask& task = *iter;
      if ( !pAllowable->IsStressCheckApplicable(segmentKey,task) )
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
      Float64 fAllowable(0.0);
      if(task.stressType == pgsTypes::Compression)
      {
         ATLASSERT(task.limitState != pgsTypes::ServiceIII);
         fAllowable = pAllowable->GetSegmentAllowableCompressionStress(poi,task,fcgdr);
      }
      else // tension
      {
#if defined _DEBUG
         if ( liveLoadIntervalIdx <= task.intervalIdx && task.limitState == pgsTypes::ServiceI)
         {
            ATLASSERT(pAllowable->CheckFinalDeadLoadTensionStress());
         }
#endif
         fAllowable = pAllowable->GetSegmentAllowableTensionStress(poi,task,fcgdr,false/*bWithBondedReinforcement*/);
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
         Float64 Pperm = pPrestressForce->GetPrestressForce(poi,pgsTypes::Permanent,task.intervalIdx,pgsTypes::End,pgsTypes::tltMinimum,&config);
         Float64 Ptemp = pPrestressForce->GetPrestressForce(poi,pgsTypes::Temporary,task.intervalIdx,pgsTypes::End,pgsTypes::tltMinimum,&config);
         Pps = Pperm + Ptemp;
      }

      Float64 k = pLoadFactors->GetDCMax(task.limitState);
      Pps *= k;

      // Section props - bare girder
      Float64 Ag  = pSectProps->GetAg(releaseIntervalIdx, poi);
      Float64 Stg = pSectProps->GetS(releaseIntervalIdx, poi, pgsTypes::TopGirder);
      Float64 Sbg = pSectProps->GetS(releaseIntervalIdx, poi, pgsTypes::BottomGirder);

      // Upper and lower bound ecc's
      Float64 ub_ecc, lb_ecc;
      if(task.stressType == pgsTypes::Compression)
      {
         // lb ecc will be for top stress, ub ecc for bottom stress
         lb_ecc = (-Pps/Ag - fAllowable + fTopMinExt)*Stg/Pps;
         ub_ecc = (-Pps/Ag - fAllowable + fBotMinExt)*Sbg/Pps;
      }
      else
      {
         // Opposite for tension
         ub_ecc = (-Pps/Ag - fAllowable + fTopMaxExt)*Stg/Pps;
         lb_ecc = (-Pps/Ag - fAllowable + fBotMaxExt)*Sbg/Pps;
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

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IDuctLimits, pDuctLimits);
   GET_IFACE(IPointOfInterest, pPoi);
   GET_IFACE_NOCHECK(IGirder,pGirder);
   GET_IFACE_NOCHECK(IIntervals, pIntervals); // only used if there are tendons

   // check segment tendons
   GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeometry);
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
         Float64 duct_diameter = (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::NinthEdition2020 ? pSegmentTendonGeometry->GetOutsideDiameter(segmentKey,ductIdx) : pSegmentTendonGeometry->GetNominalDiameter(segmentKey, ductIdx));

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

   GET_IFACE(IGirderTendonGeometry,pGirderTendonGeometry);
   DuctIndexType nDucts = pGirderTendonGeometry->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressGirderTendonInterval(girderKey,ductIdx);
      Float64 Apt = pGirderTendonGeometry->GetGirderTendonArea(girderKey,stressTendonIntervalIdx,ductIdx);
      Float64 Aduct = pGirderTendonGeometry->GetInsideDuctArea(girderKey,ductIdx);

      // starting with 9th edition, the duct diameter limit and the duct reduction for shear is based on nominal duct diameter
      Float64 duct_diameter = (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::NinthEdition2020 ? pGirderTendonGeometry->GetOutsideDiameter(girderKey, ductIdx) : pGirderTendonGeometry->GetNominalDiameter(girderKey, ductIdx));

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

   GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeometry);
   SegmentIndexType nMaxSegmentDucts = pSegmentTendonGeometry->GetMaxDuctCount(girderKey);

   GET_IFACE(IGirderTendonGeometry,pGirderTendonGeometry);
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(girderKey);
   if (nMaxSegmentDucts+nGirderDucts == 0 )
   {
      return;
   }

   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

   GET_IFACE_NOCHECK(IPointOfInterest, pPoi);
   GET_IFACE(IPosttensionForce,pPTForce);
   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IAllowableTendonStress,pAllowables);

   IntervalIndexType finalIntervalIdx = pIntervals->GetIntervalCount()-1;

   // Check segment tendons
   GET_IFACE(IBridge, pBridge);
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
         if (pAllowables->CheckTendonStressAtJacking())
         {
            artifact.SetCheckAtJacking(pAllowables->GetSegmentTendonAllowableAtJacking(segmentKey), pSegmentTendonGeometry->GetFpj(segmentKey, ductIdx));
         }
         else
         {
            artifact.SetCheckPriorToSeating(pAllowables->GetSegmentTendonAllowablePriorToSeating(segmentKey), fpbtMax);
         }

         artifact.SetCheckAtAnchoragesAfterSeating(pAllowables->GetSegmentTendonAllowableAfterAnchorSetAtAnchorage(segmentKey), fanchorMax);
         artifact.SetCheckAfterSeating(pAllowables->GetSegmentTendonAllowableAfterAnchorSet(segmentKey), fseatMax);
         artifact.SetCheckAfterLosses(pAllowables->GetSegmentTendonAllowableAfterLosses(segmentKey), fpeMax);
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
      GET_IFACE(IPointOfInterest,pPoi);
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
      if ( pAllowables->CheckTendonStressAtJacking() )
      {
         artifact.SetCheckAtJacking(pAllowables->GetGirderTendonAllowableAtJacking(girderKey),pGirderTendonGeometry->GetFpj(girderKey,ductIdx));
      }
      else
      {
         artifact.SetCheckPriorToSeating(pAllowables->GetGirderTendonAllowablePriorToSeating(girderKey),fpbtMax);
      }

      artifact.SetCheckAtAnchoragesAfterSeating(pAllowables->GetGirderTendonAllowableAfterAnchorSetAtAnchorage(girderKey),fanchorMax);
      artifact.SetCheckAfterSeating(pAllowables->GetGirderTendonAllowableAfterAnchorSet(girderKey),fseatMax);
      artifact.SetCheckAfterLosses(pAllowables->GetGirderTendonAllowableAfterLosses(girderKey),fpeMax);
      pGirderArtifact->SetTendonStressArtifact(ductIdx,artifact);
   }
}

void pgsDesigner2::CheckStrandStresses(const CSegmentKey& segmentKey,pgsStrandStressArtifact* pArtifact) const
{
   GET_IFACE(IAllowableStrandStress,pAllow);
   GET_IFACE(IPretensionForce, pPsForce);
   GET_IFACE(IPointOfInterest,pPoi);

   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

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

   GET_IFACE(ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   std::vector<pgsTypes::StrandType> strandTypes{ pgsTypes::Straight, pgsTypes::Harped };

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);
   if ( 0 < Nt )
   {
      ATLASSERT(Nt != INVALID_INDEX);
      strandTypes.push_back(pgsTypes::Temporary);
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType jackIntervalIdx = pIntervals->GetStressStrandInterval(segmentKey);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   std::vector<pgsTypes::StrandType>::iterator standTypeIter(strandTypes.begin());
   std::vector<pgsTypes::StrandType>::iterator standTypeIterEnd(strandTypes.end());
   for ( ; standTypeIter != standTypeIterEnd; standTypeIter++ )
   {
      pgsTypes::StrandType strandType = *standTypeIter;

      if ( pAllow->CheckStressAtJacking() )
      {
         pArtifact->SetCheckAtJacking( strandType, pPsForce->GetEffectivePrestress(mid_span_poi,strandType,jackIntervalIdx,pgsTypes::Start), pAllow->GetAllowableAtJacking(segmentKey,strandType) );
      }

      if ( pAllow->CheckStressBeforeXfer() )
      {
         pArtifact->SetCheckBeforeXfer( strandType, pPsForce->GetEffectivePrestress(mid_span_poi,strandType,jackIntervalIdx,pgsTypes::End/*pgsTypes::BeforeXfer*/), pAllow->GetAllowableBeforeXfer(segmentKey,strandType) );
      }

      if ( pAllow->CheckStressAfterXfer() )
      {
         pArtifact->SetCheckAfterXfer( strandType, pPsForce->GetEffectivePrestress(mid_span_poi,strandType,releaseIntervalIdx,pgsTypes::Start/*pgsTypes::AfterXfer*/), pAllow->GetAllowableAfterXfer(segmentKey,strandType) );
      }

      if ( pAllow->CheckStressAfterLosses() && strandType != pgsTypes::Temporary )
      {
         // LRFD 5.9.2.2 is a Service I check
         Float64 fpe = pPsForce->GetEffectivePrestressWithLiveLoad(mid_span_poi,strandType,pgsTypes::ServiceI,true/*include elastic gains*/, false/*don't apply elastic gain reduction factor*/);
         pArtifact->SetCheckAfterLosses( strandType, fpe, pAllow->GetAllowableAfterLosses(segmentKey,strandType) );
      }
   }
}

void pgsDesigner2::CheckSegmentStresses(const CSegmentKey& segmentKey,const PoiList& vPoi,const StressCheckTask& task,pgsSegmentArtifact* pSegmentArtifact) const
{
   USES_CONVERSION;

   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Checking Girder Stresses"));

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType releaseIntervalIdx              = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx             = pIntervals->GetFirstCastDeckInterval();
   IntervalIndexType tsRemovalIntervalIdx            = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx             = pIntervals->GetLiveLoadInterval();
   IntervalIndexType compositeClosureIntervalIdx     = pIntervals->GetCompositeClosureJointInterval(segmentKey);
   IntervalIndexType lastIntervalIdx                 = pIntervals->GetIntervalCount()-1;

   bool bSISpec = lrfdVersionMgr::GetVersion() == lrfdVersionMgr::SI ? true : false;

   GET_IFACE(IPretensionStresses,       pPretensionStresses);
   GET_IFACE(ILimitStateForces,         pLimitStateForces);
   GET_IFACE(IPrecompressedTensileZone, pPrecompressedTensileZone);
   GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeometry);
   GET_IFACE(IGirderTendonGeometry, pGirderTendonGeometry);

   GET_IFACE(ILoadFactors,              pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();

   // these interfaces only get used if the task type is tension. however we need them
   // inside the POI loop and don't want to get them every time.
   GET_IFACE_NOCHECK(IBridge,            pBridge);
   GET_IFACE_NOCHECK(IGirder,            pGirder);
   GET_IFACE_NOCHECK(ISectionProperties, pSectProps);
   GET_IFACE_NOCHECK(IShapes,            pShapes);
   GET_IFACE_NOCHECK(IMaterials,         pMaterials);
   GET_IFACE_NOCHECK(ILongRebarGeometry, pRebarGeom);
   GET_IFACE_NOCHECK(IPointOfInterest,   pPoi);

   GET_IFACE_NOCHECK(IAllowableConcreteStress,  pAllowable );
   GET_IFACE_NOCHECK(IProductForces,     pProductForces); // only used for spliced girders


   GET_IFACE(ILossParameters, pLossParams);
   bool bTimeStepAnalysis = (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP ? true : false);

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

   // for time-step analysis, pretension stresses are taken from the release interval, otherwise the are taken from the task interval
   // we do this because in time-step analysis girder stresses include the effect of creep, shrinkage, and relaxation. we don't
   // want to include CR, SH, and RE again by computing the stress in the girder due to prestressing with an effective prestress
   // force that includes CR, SH, and RE.
   IntervalIndexType pretensionIntervalIdx = (bTimeStepAnalysis ? releaseIntervalIdx : task.intervalIdx);

   GET_IFACE(IDocumentType, pDocType);
   bool bCheckTemporaryStresses = false;
   if (pDocType->IsPGSuperDocument())
   {
      bCheckTemporaryStresses = pAllowable->CheckTemporaryStresses();
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
	                     artifact.IsApplicable( topStressLocation, !bIsInPTZ[TOP] );
	                     artifact.IsApplicable( botStressLocation, !bIsInPTZ[BOT] );
	                  }
	                  else if ( bCheckTemporaryStresses )
	                  {
	                     if ( bIsStressingInterval )
	                     {
	                        // During a stressing activity, tension stress checks are only performed in areas
	                        // other that the precompressed tensile zone
	                        ATLASSERT(task.intervalIdx == tsRemovalIntervalIdx);
	                        artifact.IsApplicable( topStressLocation, !bIsInPTZ[TOP] );
	                        artifact.IsApplicable( botStressLocation, !bIsInPTZ[BOT] );
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
	                  artifact.IsApplicable( topStressLocation, 
	                     compositeDeckIntervalIdx <= task.intervalIdx && // composite deck
	                     bIsDeckPrecompressed // deck is precompressed
	                     && (
	                           ( (bIsSegmentTendonStressingInterval || bIsGirderTendonStressingInterval) && !bIsInPTZ[TOP]) || // this is a tendon stressing interval and stress location is NOT in the PTZ -OR-
	                           (!(bIsSegmentTendonStressingInterval || bIsGirderTendonStressingInterval) &&  bIsInPTZ[TOP]) // this is NOT a tendon stressing interval and the stress location is in the PTZ
	                         ));
	
	                  artifact.IsApplicable( botStressLocation, 
	                     compositeDeckIntervalIdx <= task.intervalIdx && // composite deck
	                     bIsDeckPrecompressed // deck is precompressed
	                     && (
	                           ( (bIsSegmentTendonStressingInterval || bIsGirderTendonStressingInterval) && !bIsInPTZ[BOT]) || // this is a tendon stressing interval and stress location is NOT in the PTZ -OR-
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
                  (task.limitState == pgsTypes::ServiceI && pAllowable->CheckFinalDeadLoadTensionStress()) ||
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
	         pPretensionStresses->GetStress(pretensionIntervalIdx,poi,topStressLocation,botStressLocation,task.bIncludeLiveLoad,limitState, INVALID_INDEX/*controlling live load*/,&fPretension[TOP],&fPretension[BOT]);
	
	         // get segment stress due to external loads
	         std::array<Float64,2> fLimitStateMin{ 0,0 }, fLimitStateMax{ 0,0 };
	         pLimitStateForces->GetStress(task.intervalIdx,limitState,poi,batTop,false/*exclude prestress*/,topStressLocation,&fLimitStateMin[TOP],&fLimitStateMax[TOP]);
	         pLimitStateForces->GetStress(task.intervalIdx,limitState,poi,botStressLocation == pgsTypes::BottomGirder ? batBottom : batTop,false/*exclude prestress*/,botStressLocation,&fLimitStateMin[BOT],&fLimitStateMax[BOT]);

            if (liveLoadIntervalIdx <= task.intervalIdx && !task.bIncludeLiveLoad)
            {
               // the task interval is at or after the live load interval and this task does not include live load

               GET_IFACE(ILiveLoads, pLiveLoads);

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

               GET_IFACE(IUserDefinedLoadData, pUserLoads);
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
	
	         Float64 k;
	         if (limitState == pgsTypes::ServiceIA || limitState == pgsTypes::FatigueI)
	         {
	            k = 0.5; // Use half prestress stress if service IA  (See Tbl 5.9.4.2.1-1 2008 or before) or Fatigue I (LRFD 5.5.3.1 2009)
	         }
	         else
	         {
	            k = 1.0;
	         }
	         
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

            artifact.SetDemand(             topStressLocation, f[TOP] );
	         artifact.SetExternalEffects(    topStressLocation, fLimitState[TOP]);
	         artifact.SetPretensionEffects(  topStressLocation, fPretension[TOP]);
	
	         artifact.SetDemand(             botStressLocation, f[BOT] );
	         artifact.SetExternalEffects(    botStressLocation, fLimitState[BOT]);
	         artifact.SetPretensionEffects(  botStressLocation, fPretension[BOT]);
	
            // sets the allowable stress limit in the artifact and computes and stores
            // the required concrete strength to satisfy the stress limit
	         ComputeConcreteStrength(artifact,topStressLocation,task);
	         ComputeConcreteStrength(artifact,botStressLocation,task);
	
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
	
	            std::array<std::array<Float64,2>,2> fAllowable;
	            fAllowable[TOP][WITHOUT_REBAR] = artifact.GetCapacity(topStressLocation);
	            fAllowable[BOT][WITHOUT_REBAR] = artifact.GetCapacity(botStressLocation);
	            fAllowable[TOP][WITH_REBAR]    = pAllowable->GetAllowableTensionStress(poi,topStressLocation,task,true/*with rebar*/,bIsInPTZ[TOP]);
	            fAllowable[BOT][WITH_REBAR]    = pAllowable->GetAllowableTensionStress(poi,botStressLocation,task,true/*with rebar*/,bIsInPTZ[BOT]);
	
	            Float64 fTopAllowable = fAllowable[TOP][WITHOUT_REBAR];
	            Float64 fBotAllowable = fAllowable[BOT][WITHOUT_REBAR];
	
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
	
               if (pAllowable->HasAllowableTensionWithRebarOption(task.intervalIdx, bIsInPTZ[TOP], !bIsInClosure, thisSegmentKey))
	            {
	               if (i == 0 /*girder stresses*/ && bIsInClosure && bIsInPTZ[TOP])
	               {
	                  // the bar stress is not limited to 30 ksi [see LRFD Tables 5.9.2.3.1 a and b (pre2017: 5.9.4.1.2-1 and -2)]
	                  // in the precompressed tensile zone for closure joints
	                  altTensionRequirements.bLimitBarStress = false;
	               }
	
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
	               artifact.SetAlternativeTensileStressRequirements(topStressLocation, altTensionRequirements, fAllowable[TOP][WITH_REBAR], bBiaxialStresses);
	            }
	
	
	            if ( pAllowable->HasAllowableTensionWithRebarOption(task.intervalIdx,bIsInPTZ[BOT],!bIsInClosure,thisSegmentKey) )
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
	               artifact.SetAlternativeTensileStressRequirements(botStressLocation, altTensionRequirements, fAllowable[BOT][WITH_REBAR], bBiaxialStresses);
	            }
	
	            artifact.SetCapacity(topStressLocation,fTopAllowable);
	            artifact.SetCapacity(botStressLocation,fBotAllowable);
	
	            //
	            // Get the controlling stress
	            //
	
	            // parameters for tension with rebar
	            std::array<Float64,2> talt;
	            std::array<bool,2> bCheckMax;
	            std::array<Float64,2> fmax;
	            pAllowable->GetAllowableTensionStressCoefficient(poi,topStressLocation,task,true/*with rebar*/, bIsInPTZ[TOP],&talt[TOP],&bCheckMax[TOP],&fmax[TOP]);
	            pAllowable->GetAllowableTensionStressCoefficient(poi,botStressLocation,task,true/*with rebar*/, bIsInPTZ[BOT],&talt[BOT],&bCheckMax[BOT],&fmax[BOT]);
	
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
	               ATLASSERT(bCheckMax[face] == false); // alternate stress doesn't use limiting value
	               fc_reqd = pow(f/(lambda*talt[face]),2);
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

   if (bIsApplicable)
   {
      const auto& poi(artifact.GetPointOfInterest());
      bool bIsInPTZ = false;
      GET_IFACE(IAllowableConcreteStress, pAllowable);
      if (task.stressType == pgsTypes::Compression)
      {
         Float64 fAllowable = pAllowable->GetAllowableCompressionStress(poi, stressLocation, task);
         artifact.SetCapacity(stressLocation, fAllowable);
      }
      else
      {
         bIsInPTZ = artifact.IsInPrecompressedTensileZone(stressLocation);
         Float64 fAllowable = pAllowable->GetAllowableTensionStress(poi, stressLocation, task, false/*without rebar*/, bIsInPTZ); // this accounts for UHPC and returns the correct tension stress limit
         artifact.SetCapacity(stressLocation, fAllowable);
      }

      Float64 fc_reqd = pAllowable->GetRequiredConcreteStrength(poi, stressLocation, artifact.GetDemand(stressLocation), task, bIsInPTZ);
      artifact.SetRequiredConcreteStrength(task.stressType, stressLocation, fc_reqd);
   }
}

void pgsDesigner2::CheckSegmentStressesAtRelease(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig,pgsTypes::StressType type, pgsSegmentArtifact* pSegmentArtifact) const
{
   USES_CONVERSION;

   GET_IFACE(IPointOfInterest,         pPoi);
   GET_IFACE(IPretensionStresses,      pPretensionStresses);
   GET_IFACE(IProductForces,           pProductForces);
   GET_IFACE(ILimitStateForces,        pLimitStateForces);
   GET_IFACE(IAllowableConcreteStress, pAllowable );
   GET_IFACE(IGirder,                  pGirder);
   GET_IFACE(ISectionProperties,       pSectProps);
   GET_IFACE(IShapes,                  pShapes);
   GET_IFACE(IMaterials,               pMaterials);
   GET_IFACE(ILongRebarGeometry,       pRebarGeom);
   GET_IFACE(IIntervals,               pIntervals);
   GET_IFACE(IPrecompressedTensileZone, pPrecompressedTensileZone);

   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   // we only work in the casting yard
   StressCheckTask task;
   task.intervalIdx = releaseIntervalIdx;
   task.limitState  = pgsTypes::ServiceI;
   task.stressType  = type;

   pgsTypes::BridgeAnalysisType batTop, batBottom;
   GetBridgeAnalysisType(segmentKey.girderIndex,task,batTop,batBottom);

   bool bSISpec = lrfdVersionMgr::GetVersion() == lrfdVersionMgr::SI ? true : false;

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

      Float64 fAllowableWithoutRebar(0.0), fAllowableWithRebar(0.0);
      Float64 c(0.0);
      Float64 t(0.0), talt(0.0);
      bool bCheckMax(false);
      Float64 ftmax(0.0);
      if (task.stressType == pgsTypes::Compression)
      {
         // always applicable in compression
         artifact.IsApplicable(pgsTypes::TopGirder,    true);
         artifact.IsApplicable(pgsTypes::BottomGirder, true);

         c = pAllowable->GetSegmentAllowableCompressionStressCoefficient(poi,task);

         fAllowableWithoutRebar  = pAllowable->GetSegmentAllowableCompressionStress(poi, task, fci);
         fAllowableWithRebar = fAllowableWithoutRebar;
      }
      else
      {
         // tension stress check only applicable in areas other that the precompressed tensile zone
         artifact.IsApplicable(pgsTypes::TopGirder,   !bIsInPTZ[pgsTypes::TopGirder]);
         artifact.IsApplicable(pgsTypes::BottomGirder,!bIsInPTZ[pgsTypes::BottomGirder]);

         pAllowable->GetAllowableTensionStressCoefficient(poi, pgsTypes::TopGirder,task,false/*without rebar*/,false,&t,&bCheckMax,&ftmax);

         bool bDummy;
         Float64 fDummy;
         pAllowable->GetAllowableTensionStressCoefficient(poi, pgsTypes::TopGirder,task,true/*with rebar*/,false,&talt,&bDummy,&fDummy);

         fAllowableWithoutRebar = pAllowable->GetSegmentAllowableTensionStress(poi, task, fci,false/*without rebar*/);
         fAllowableWithRebar    = pAllowable->GetSegmentAllowableTensionStress(poi, task, fci,true/*with rebar*/);
      }

      // get segment stress due to prestressing
      Float64 fTopPretension, fBotPretension;
      if (pConfig == nullptr)
      {
         pPretensionStresses->GetStress(task.intervalIdx,poi,pgsTypes::TopGirder,pgsTypes::BottomGirder,task.bIncludeLiveLoad,task.limitState,INVALID_INDEX,&fTopPretension,&fBotPretension);
      }
      else
      {
         pPretensionStresses->GetDesignStress(task.intervalIdx, poi, pgsTypes::TopGirder, pgsTypes::BottomGirder, *pConfig, task.bIncludeLiveLoad, task.limitState, &fTopPretension, &fBotPretension);
      }

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
      Float64 fAllowable(0.0);
      if(task.stressType == pgsTypes::Compression)
      {
         fAllowable = fAllowableWithoutRebar;

         // req'd strength
         Float64 fc_reqd = (IsZero(c) ? -99999 : Min(fTop,fBot)/-c);
         
         if ( fc_reqd < 0 ) // the minimum stress is tensile so compression isn't an issue
         {
            fc_reqd = 0;
         }

         if ( MinIndex(fTop,fBot) == 0 )
         {
            artifact.SetRequiredConcreteStrength(task.stressType,pgsTypes::TopGirder,fc_reqd);
         }
         else
         {
            artifact.SetRequiredConcreteStrength(task.stressType, pgsTypes::BottomGirder,fc_reqd);
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
         artifact.SetAlternativeTensileStressRequirements(pgsTypes::BottomGirder, altTensionRequirements, fAllowableWithRebar, bBiaxialStresses);

         if (altTensionRequirements.AsRequired <= altTensionRequirements.AsProvided)
         {
            // if AsRequired < 0 (eg, -1), the entire section is in compression
            fAllowable = (altTensionRequirements.AsRequired < 0 ? fAllowableWithoutRebar : fAllowableWithRebar);
         }
         else
         {
            fAllowable = fAllowableWithoutRebar;
         }

         // Compute required concrete strength
         // Take the controlling tension
         Float64 f =  Max(fTop,fBot);

         Float64 fci_reqd = 0;
         if (0.0 < f)
         {
#pragma Reminder("Re-implement this with a general call to the SpecAgent to get required concrete stress")
            // ALSO SEE StrandDesitnTool.cpp - it basically has the same code and it should be calling the SpecAgent too
            // We want to call the SpecAgent so there is a single location to get this information.
            if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::PCI_UHPC)
            {
               ATLASSERT(false); // not designing for UHPC yet

               const auto& pConcrete = pMaterials->GetSegmentConcrete(segmentKey);
               const lrfdLRFDConcreteBase* pLRFDConcrete = dynamic_cast<const lrfdLRFDConcreteBase*>(pConcrete.get());
               Float64 f_fc = pLRFDConcrete->GetFirstCrackingStrength();
               Float64 fc_28 = (pConfig == nullptr ? pMaterials->GetSegmentFc28(segmentKey) : pConfig->fc);

               IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

               // the maximum stress is compressive so tension isn't an issue
               // or this is UHPC and allowable stress isn't a function of f'c
               //fc_reqd = 0;

               // the general form of the tension stress limit at release is (2/3)(f_fc)*sqrt(f'ci/f'c)
               // this can be solved for f'ci or f'c as needed
               if (haulingIntervalIdx <= task.intervalIdx)
               {
                  fci_reqd = 0;
               }
               else
               {
                  fci_reqd = pow(1.5 * f / f_fc, 2) * fc_28;
               }
            }
            else if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
            {
               ATLASSERT(false); // not designing for UHPC yet
               //Float64 gamma_u = pAllowable->GetAllowableUHPCTensionStressLimitCoefficient(segmentKey);
               //fci_reqd = f / gamma_u;
               fci_reqd = 0;
            }
            else
            {
               // Is adequate rebar available to use the higher limit?
               if (altTensionRequirements.bIsAdequateRebar )
               {
                  // We have additional rebar and can go to a higher limit
                  fci_reqd = pow(f/(lambda*talt),2);
               }
               else
               {
                  fci_reqd = (IsZero(t) ? 0 : BinarySign(f)*pow(f/(lambda*t),2));
                  if ( bCheckMax &&                  // allowable stress is limited -AND-
                       (0 < fci_reqd) &&              // there is a concrete strength that might work -AND-
                       (pow(ftmax/(lambda*t),2) < fci_reqd) )   // that strength will exceed the max limit on allowable
                  {
                     // too bad... this isn't going to work
                     fci_reqd = NO_AVAILABLE_CONCRETE_STRENGTH;
                  }
               }
            }
         }

         if ( MaxIndex(fTop,fBot) == 0 )
         {
            artifact.SetRequiredConcreteStrength(task.stressType, pgsTypes::TopGirder,fci_reqd);
         }
         else
         {
            artifact.SetRequiredConcreteStrength(task.stressType, pgsTypes::BottomGirder,fci_reqd);
         }
      }

      artifact.SetCapacity(pgsTypes::TopGirder,   fAllowable);
      artifact.SetCapacity(pgsTypes::BottomGirder,fAllowable);

      // Stow our artifact
      pSegmentArtifact->AddFlexuralStressArtifact(artifact);
   } // next poi
}

void pgsDesigner2::CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const GDRCONFIG& config,bool bPositiveMoment,pgsFlexuralCapacityArtifact* pArtifact) const
{
   GET_IFACE(IMomentCapacity, pMomentCapacity);

   const MOMENTCAPACITYDETAILS* pmcd = pMomentCapacity->GetMomentCapacityDetails( intervalIdx, poi, bPositiveMoment, &config );

   MINMOMENTCAPDETAILS mmcd;
   pMomentCapacity->GetMinMomentCapacityDetails(intervalIdx, poi, config, bPositiveMoment, &mmcd);

   CreateFlexuralCapacityArtifact(poi,intervalIdx,limitState,bPositiveMoment,pmcd,&mmcd,true/*designing*/,pArtifact);
}

void pgsDesigner2::CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,bool bPositiveMoment,pgsFlexuralCapacityArtifact* pArtifact) const
{
   GET_IFACE(IMomentCapacity, pMomentCapacity);

   const MOMENTCAPACITYDETAILS* pmcd = pMomentCapacity->GetMomentCapacityDetails( intervalIdx, poi, bPositiveMoment );

   const MINMOMENTCAPDETAILS* pmmcd = pMomentCapacity->GetMinMomentCapacityDetails(intervalIdx, poi, bPositiveMoment);

   CreateFlexuralCapacityArtifact(poi,intervalIdx,limitState,bPositiveMoment,pmcd,pmmcd,false/*checking*/,pArtifact);
}

void pgsDesigner2::CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,bool bPositiveMoment,const MOMENTCAPACITYDETAILS* pmcd,const MINMOMENTCAPDETAILS* pmmcd,bool bDesign,pgsFlexuralCapacityArtifact* pArtifact) const
{
   GET_IFACE(ILimitStateForces, pLimitStateForces);
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification, pSpec);

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool c_over_de = ( pSpec->GetMomentCapacityMethod() == LRFD_METHOD && pSpecEntry->GetSpecificationType() < lrfdVersionMgr::ThirdEditionWith2006Interims );
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

      if ( bDesign && m_StrandDesignTool.IsDesignSlabOffset())
      {
         // Mu is based on the current input values. Since we are doing design, the "A" dimension
         // is likely different than the input value. This changes the slab and slab pad moment.
         // Add the moment adjustments to Mu here.
         Float64 fcgdr = m_StrandDesignTool.GetConcreteStrength();

         const GDRCONFIG& config = m_StrandDesignTool.GetSegmentConfiguration();

         GET_IFACE(IProductForces,pProductForces);
         Float64 dMslab     = pProductForces->GetDesignSlabMomentAdjustment(poi,&config);
         Float64 dMslab_pad = pProductForces->GetDesignSlabPadMomentAdjustment(poi,&config);

         GET_IFACE(ILoadFactors,pLF);
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
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   ATLASSERT(liveLoadIntervalIdx <= intervalIdx);
   ATLASSERT(limitState==pgsTypes::StrengthI || limitState == pgsTypes::StrengthII);
#endif

   GET_IFACE(IBridge,pBridge);

   // throw an exception if span length is too short
   if (IsDeepSection( poi ))
   {
      GET_IFACE(IPointOfInterest,pPoi);
      CSpanKey spanKey;
      Float64 Xspan;
      pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

      Int32 reason = XREASON_AGENTVALIDATIONFAILURE;
      std::_tostringstream os;
      os << _T("Cannot perform shear check. The Span-to-Depth ratio is less than ")<< MIN_SPAN_DEPTH_RATIO <<_T(" for Span ")
         << LABEL_SPAN(spanKey.spanIndex) << _T(" Girder ")<< LABEL_GIRDER(spanKey.girderIndex)
         << _T(" (See LRFD ") << LrfdCw8th(_T("5.8.1.1"),_T("5.7.1.1"))<<_T(")");

      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      pgsBridgeDescriptionStatusItem* pStatusItem = new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionError,pgsBridgeDescriptionStatusItem::General,os.str().c_str());
      pStatusCenter->Add(pStatusItem);

      os << std::endl << _T("See Status Center for Details");
      THROW_UNWIND(os.str().c_str(),reason);
   }

   GET_IFACE(IShearCapacity, pShearCapacity);

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

   GET_IFACE(IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

   GET_IFACE(IBridge,pBridge);
   Float64 span_length = pBridge->GetSpanLength(spanKey);

   GET_IFACE(IGirder,pGdr);
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

      GET_IFACE(IShearCapacity, pShearCapacity);
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

   GET_IFACE(IGirder,pGdr);
   GET_IFACE(IMaterials,pMaterial);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount() - 1;

   // determine shear demand
   GET_IFACE(IInterfaceShearRequirements,pInterfaceShear);

   Float64 Vuh;

   if ( pInterfaceShear->GetShearFlowMethod() == pgsTypes::sfmClassical )
   {
      GET_IFACE(ISectionProperties,pSectProp);

      Float64 Qslab, Ic;
      if ( pConfig == nullptr )
      {
         Qslab = pSectProp->GetQSlab(intervalIdx, poi);
         Ic  = pSectProp->GetIxx(intervalIdx,poi);
      }
      else
      {
         Qslab = pSectProp->GetQSlab(intervalIdx, poi, pConfig->fc28);
         Ic  = pSectProp->GetIxx(intervalIdx,poi,pConfig->fc28);
      }

      ATLASSERT(0 < Qslab);

      Vuh  = vu*Qslab/Ic;

      pArtifact->SetI( Ic );
      pArtifact->SetQ( Qslab );
   }
   else
   {
      // dv is the distance between the centroid of the compression force, taken to be at the mid-height of the deck and the centroid of the tension steel.
      // since the steel on the tension side varies because of harped strand position, estimate by considering straight strands only
      GET_IFACE(IMomentCapacity,pMomentCap);
      const MOMENTCAPACITYDETAILS* pmcd = pMomentCap->GetMomentCapacityDetails(intervalIdx, poi, true/*positive moment*/, pConfig);

  
      GET_IFACE(IBridge,pBridge);
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
   GET_IFACE(ILoadFactors, pILoadFactors);
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
   lrfdConcreteUtil::InterfaceShearParameters(is_roughened, (WBFL::Materials::ConcreteType)girderConcType, (WBFL::Materials::ConcreteType)slabConcType, &c, &u, &K1, &K2);

   pArtifact->SetCohesionFactor(c);
   pArtifact->SetFrictionFactor(u);
   pArtifact->SetK1(K1);
   pArtifact->SetK2(K2);

   // nominal shear capacities 5.7.4.1-2,3 (pre2017: 5.8.4.1)
   if ( lrfdVersionMgr:: FourthEdition2007 <= lrfdVersionMgr::GetVersion() && gs_60KSI < fy)
   {
      // 60 ksi limit was added in 4th Edition 2007
      fy = gs_60KSI;
      pArtifact->WasFyLimited(true);
   }

   pArtifact->SetFy(fy);

   Float64 Vn1, Vn2, Vn3;
   lrfdConcreteUtil::InterfaceShearResistances(c, u, K1, K2, bvi_details.bvi, pArtifact->GetAvOverS(), gamma_dc*Pc, fc, fy, &Vn1, &Vn2, &Vn3);
   pArtifact->SetVn(Vn1, Vn2, Vn3);

   GET_IFACE(IResistanceFactors,pResistanceFactors);
   GET_IFACE(IPointOfInterest,pPoi);
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

   lrfdConcreteUtil::HsAvfOverSMinType avfmin = lrfdConcreteUtil::AvfOverSMin(bvi_details.bvi,fy,Vuh,phi,c,u,Pc);
   pArtifact->SetAvOverSMin_5_7_4_2_1(avfmin.res5_7_4_2_1);
   pArtifact->SetAvOverSMin_5_7_4_1_3(avfmin.res5_7_4_2_3);
   pArtifact->SetAvOverSMin(avfmin.AvfOverSMin);

   Uint16 min_num_legs = lrfdConcreteUtil::MinLegsForBv(bvi_details.bvi);
   pArtifact->SetNumLegsReqd(min_num_legs);

   // Determine average shear stress.
   // Average shear stress. Note: This value is vni prior to 2007 and vui afterwards 
   Float64 Vsavg;
   if ( lrfdVersionMgr::FourthEdition2007 <= lrfdVersionMgr::GetVersion() )
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
   Float64 vs_limit = lrfdConcreteUtil::LowerLimitOfShearStrength(is_roughened,do_all_stirrups_engage_deck);
   pArtifact->SetVsLimit(vs_limit);

   // Get Av/S required for design algorithm
   Float64 avs_reqd = lrfdConcreteUtil::AvfRequiredForHoriz(Vuh, phi, avfmin.AvfOverSMin, c, u, K1, K2,
      bvi_details.bvi, bvi_details.bvi, pArtifact->GetAvOverS(), Pc, fc, fy);
   pArtifact->SetAvOverSReqd(avs_reqd);
}

void pgsDesigner2::ComputeHorizAvs(const pgsPointOfInterest& poi,bool* pIsRoughened, bool* pDoAllStirrupsEngageDeck, 
                                   const GDRCONFIG* pConfig, pgsHorizontalShearArtifact* pArtifact) const
{
   if (pConfig == nullptr)
   {
      // Use current girder model data
      GET_IFACE(IBridge,pBridge);
      GET_IFACE(IStirrupGeometry, pStirrupGeometry);
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

      GET_IFACE(IBridge, pBridge);
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

   GET_IFACE(IPointOfInterest, pPoi);
   IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(deckCastingRegionIdx);

   // permanent compressive force between slab and girder top
   // If the slab is CIP, use the tributary area.
   // If the slab is SIP, use only the area of cast slab that is NOT over
   // the deck panels.
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);

   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   if (!pSpecEntry->UseDeckWeightForPermanentNetCompressiveForce())
   {
      return 0;
   }

   GET_IFACE(IBridge, pBridge);
   GET_IFACE(IMaterials, pMaterial);

   // slab load
   Float64 wslab = 0; // weight of slab on shear interface
   Float64 slab_unit_weight = pMaterial->GetDeckWeightDensity(deckCastingRegionIdx,castDeckIntervalIdx) * WBFL::Units::System::GetGravitationalAcceleration();

   if ( pDeck->GetDeckType() == pgsTypes::sdtCompositeCIP )
   {
      // Cast in place slab
      // conservative not to use sacrificial material so we will use just the structural slab depth
      // also, ignore the weight of the slab haunch as it may or may not be there depending on 
      // camber variation and other construction uncertainties
      GET_IFACE(ISectionProperties,pSectProp);
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
      GET_IFACE(IGirder,pGdr);
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

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter1999 = ( lrfdVersionMgr::SecondEditionWith2000Interims <= pSpecEntry->GetSpecificationType() ? true : false );

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
      GET_IFACE(IStirrupGeometry, pStirrupGeometry);
      Avfs = pStirrupGeometry->GetVertStirrupAvs(poi, &size, &abar, &nl, &s);
   }
   else
   {
      GET_IFACE(IBridge, pBridge);
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

   GET_IFACE_NOCHECK(ITransverseReinforcementSpec, pTransverseReinforcementSpec);// not used for UHPC

   CClosureKey closureKey;
   GET_IFACE(IPointOfInterest, pPoi);
   bool bIsInClosure = pPoi->IsInClosureJoint(poi, &closureKey);

   GET_IFACE(IMaterials, pMaterials);
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
         Float64 vu = lrfdShear::ComputeShearStress(Vu, scd.Vp, scd.Phi, scd.bv, scd.dv);
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

         lrfdRebarPool* prp = lrfdRebarPool::GetInstance();
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
   if ( lrfdVersionMgr::GetUnits() == lrfdVersionMgr::US )
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

   GET_IFACE(IMaterials,pMaterials);
   Float64 lambda = pMaterials->GetSegmentLambda(poi.GetSegmentKey());
   avs *= lambda;

   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::SeventhEditionWith2016Interims )
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
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   pArtifact->SetApplicability(true);

   // 9th edition added a requirement that ApsFps > AsFy
   // This requirement was developed primarily for simple span pretensioned girders. This limit will not be checked
   // in negative moment regions where the tension tie is the deck rebar, unless there are PT tendons providing
   // the tension tie
   GET_IFACE(IGirderTendonGeometry, pGirderTendonGeometry);
   auto nDucts = pGirderTendonGeometry->GetDuctCount(segmentKey);
   pArtifact->PretensionForceMustExceedBarForce((scd.bTensionBottom || 0 < nDucts) && lrfdVersionMgr::NinthEdition2020 <= pSpecEntry->GetSpecificationType() ? true : false);

   // Longitudinal steel
   GET_IFACE(IMaterials, pMaterials);
   Float64 Es, fy, fu;
   pMaterials->GetSegmentTransverseRebarProperties(segmentKey, &Es, &fy, &fu);
   pArtifact->SetFy(fy);
   pArtifact->SetEs(Es);
   ATLASSERT(IsEqual(fy, scd.fy));

   Float64 as = 0;
   if (pSpecEntry->IncludeRebarForShear())
   {
      // TRICKY: Rebar data from config is not used here. This is only called from the design loop
      //         once (no iterations), so all we need is the current bridge data
      GET_IFACE(ILongRebarGeometry, pRebarGeometry);

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
   GET_IFACE(IStrandGeometry, pStrandGeom);
   Float64 aps = (scd.bTensionBottom ? pStrandGeom->GetApsBottomHalf(poi, dlaNone, pConfig) : pStrandGeom->GetApsTopHalf(poi, dlaNone, pConfig));
   
   Float64 aptSegment,aptGirder;
   if ( pConfig == nullptr)
   {
      GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeometry);
      aptSegment = (scd.bTensionBottom ? pSegmentTendonGeometry->GetSegmentAptBottomHalf(poi) : pSegmentTendonGeometry->GetSegmentAptTopHalf(poi));

      GET_IFACE(IGirderTendonGeometry, pGirderTendonGeometry);
      aptGirder = (scd.bTensionBottom ? pGirderTendonGeometry->GetGirderAptBottomHalf(poi) : pGirderTendonGeometry->GetGirderAptTopHalf(poi));
   }
   else
   {
      aptSegment = 0; // no pt for design (design is only for PGSuper)
      aptGirder  = 0; // no pt for design (design is only for PGSuper)
   }

   // get prestress level at ultimate
   GET_IFACE(IMomentCapacity,pMomentCap);
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
      GET_IFACE(IShearCapacity,pShearCapacity);
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

   if ( lrfdVersionMgr::SecondEditionWith2000Interims <= lrfdVersionMgr::GetVersion() )
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

      GET_IFACE(IBridge,pBridge);

      bool bContinuousLeft, bContinuousRight;
      pBridge->IsContinuousAtPier(pierIdx,&bContinuousLeft,&bContinuousRight);
      bContinuous = (face == pgsTypes::Ahead ? bContinuousRight : bContinuousLeft);
   }

   Float64 demand;
   Uint16 equation = 999; // dummy value

   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
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
           (lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::SecondEditionWith2003Interims && 
            (poi.HasAttribute(POI_FACEOFSUPPORT) && !bContinuous) )
            ||
           // Spec is 2004 AND poi is in a critical section zone
           (lrfdVersionMgr::SecondEditionWith2003Interims < lrfdVersionMgr::GetVersion() && lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::ThirdEdition2004 && 
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
      GET_IFACE(IMomentCapacity,pMomentCapacity);
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

      GET_IFACE(IAllowableConcreteStress, pAllowables);
      Float64 gamma_u = pAllowables->GetAllowableUHPCTensionStressLimitCoefficient(segmentKey);
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
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IGirder,pGdr);
   GET_IFACE(IMaterials,pMaterial);

   Float64 segment_length  = pBridge->GetSegmentLength(segmentKey);

   // If we are in here, confinement check is applicable
   pArtifact->SetApplicability(true);

   // Get spec constraints
   GET_IFACE(ITransverseReinforcementSpec,pTransverseReinforcementSpec);
   WBFL::Materials::Rebar::Size szmin = pTransverseReinforcementSpec->GetMinConfinmentBarSize();
   Float64 smax = pTransverseReinforcementSpec->GetMaxConfinmentBarSpacing();

   WBFL::Materials::Rebar::Grade grade;
   WBFL::Materials::Rebar::Type type;
   pMaterial->GetSegmentTransverseRebarMaterial(segmentKey,&type,&grade);

   pArtifact->SetMinBar(lrfdRebarPool::GetInstance()->GetRebar(type,grade,szmin));
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
      GET_IFACE(IStirrupGeometry, pStirrupGeometry);
      pStirrupGeometry->GetStartConfinementBarInfo(segmentKey, reqdStartZl, &start_rbsiz, &start_zl, &start_s);
      pStirrupGeometry->GetEndConfinementBarInfo(segmentKey, reqdEndZl, &end_rbsiz, &end_zl, &end_s);
   }

   pArtifact->SetStartS(start_s);
   pArtifact->SetStartProvidedZoneLength(start_zl);
   pArtifact->SetStartBar(lrfdRebarPool::GetInstance()->GetRebar(type,grade,start_rbsiz));

   pArtifact->SetEndS(end_s);
   pArtifact->SetEndProvidedZoneLength(end_zl);
   pArtifact->SetEndBar(lrfdRebarPool::GetInstance()->GetRebar(type,grade,end_rbsiz));
}

void pgsDesigner2::CheckMomentCapacity(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsGirderArtifact* pGirderArtifact) const
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   // Get points of interest for evaluation
   PoiList vPoi; // POIs for both positive and negative moment
   PoiList vNMPoi; // additional POIs for negative moment only

   GET_IFACE(IPointOfInterest, pPoi);
   GET_IFACE(IBridge,pBridge);
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
   GET_IFACE(IPointOfInterest,pPOI);
   PoiList vPoi;
   pPOI->GetPointsOfInterest(segmentKey, POI_FACEOFSUPPORT, &vPoi);

   // get the piers that go with the face of supports
   std::vector<std::pair<const CPierData2*,pgsTypes::PierFaceType>> vPiers;
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
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

   GET_IFACE_NOCHECK(IBridge,pBridge); // there are cases (drop in span) where there aren't any face of supports and this never gets used

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
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IShearCapacity,pShearCapacity);

#if defined _DEBUG
   // Checking shear should only be occurring at the final condition.... that is, only in intervals
   // after the live load is applied
   GET_IFACE(IIntervals,pIntervals);
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
      GET_IFACE(IPointOfInterest,pPoi);
      pPoi->GetCriticalSections(limitState, segmentKey,&vCSPoi);
      std::vector<CRITSECTDETAILS> vCS = pShearCapacity->GetCriticalSectionDetails(limitState,segmentKey);

      if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEdition2004)
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

      if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::ThirdEdition2004)
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
      GET_IFACE(IPointOfInterest, pPoi);
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
      GET_IFACE_NOCHECK(IIntervals, pIntervals);
      GET_IFACE(IBridge, pBridge);
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
      // PoiIsOusideOfBearings does the filtering and it keeps POIs that are at the closure joint (and this is what we want)
      Float64 segmentSpanLength = pBridge->GetSegmentSpanLength(segmentKey);
      Float64 endDist = pBridge->GetSegmentStartEndDistance(segmentKey);
      std::remove_copy_if(pois.begin(), pois.end(), std::back_inserter(vPoi), PoiIsOutsideOfBearings(segmentKey, endDist, endDist + segmentSpanLength));
   }
}

void pgsDesigner2::CheckShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsGirderArtifact* pGirderArtifact) const
{
   const CGirderKey& girderKey(pGirderArtifact->GetGirderKey());

   GET_IFACE(IBridge,pBridge);
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
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   ATLASSERT(liveLoadIntervalIdx <= intervalIdx);
#endif

   InitShearCheck(segmentKey,intervalIdx,limitState,pConfig); // sets up some class member variables used for checking this segment

   // InitShearCheck causes the critical section for shear POI to be created...
   // Get the POI here so the CS poi are in the list
   PoiList vPoi;
   GetShearPointsOfInterest(bDesign, segmentKey, limitState, intervalIdx, vPoi);

   ATLASSERT(pStirrupArtifact != nullptr);
   GET_IFACE(IMaterials,pMaterials);
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

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // Confinement check
   GET_IFACE(ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bCheckConfinement = pSpecEntry->IsConfinementCheckEnabled() && limitState==pgsTypes::StrengthI; // only need to check confinement once

   pgsConfinementCheckArtifact c_artifact;
   if (bCheckConfinement)
   {
      CheckConfinement(segmentKey, pConfig, &c_artifact);
      pStirrupArtifact->SetConfinementArtifact(c_artifact);
   }

   // Splitting zone check
   pStirrupArtifact->SetSplittingCheckArtifact(CheckSplittingZone(segmentKey,pConfig));

   // poi-based shear check
   GET_IFACE(ILimitStateForces, pLimitStateForces);
   GET_IFACE(IPointOfInterest, pPoi);

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
   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Checking splitting requirements"));

   GET_IFACE(ISplittingChecks,pSplittingChecks);
   return pSplittingChecks->CheckSplitting(segmentKey, pConfig);
}

void pgsDesigner2::CheckSegmentDetailing(const CSegmentKey& segmentKey,pgsSegmentArtifact* pGdrArtifact) const
{
   // 5.12.3.2.2 (pre2017: 5.14.1.2.2)
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage( _T("Checking segment detailing") );

   pgsPrecastIGirderDetailingArtifact* pArtifact = pGdrArtifact->GetPrecastIGirderDetailingArtifact();

   // get min girder dimensions from spec
   GET_IFACE(IPrecastIGirderDetailsSpec,pPrecastIGirderDetailsSpec);
   pArtifact->SetMinTopFlangeThickness(pPrecastIGirderDetailsSpec->GetMinTopFlangeThickness());
   pArtifact->SetMinWebThickness(pPrecastIGirderDetailsSpec->GetMinWebThickness());
   pArtifact->SetMinBottomFlangeThickness(pPrecastIGirderDetailsSpec->GetMinBottomFlangeThickness());

   // get dimensions from bridge model
   GET_IFACE(IPointOfInterest,pPOI);
   PoiList vPoi;
   pPOI->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT, &vPoi);

   Float64 minTopFlange = DBL_MAX;
   Float64 minBotFlange = DBL_MAX;
   Float64 minWeb       = DBL_MAX;

   GET_IFACE(IGirder,pGdr);
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

   GET_IFACE_NOCHECK(IMaterials, pMaterials);
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
   GET_IFACE(IStrandGeometry,pStrGeom);
   StrandIndexType nStrands = pStrGeom->GetStrandCount(segmentKey,pgsTypes::Harped);
   if ( nStrands == 0 )
   {
      pArtifact->IsApplicable(false);
      return;
   }

   GET_IFACE(ISpecification,pSpec);
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(IMaterials,pMaterial);

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage( _T("Checking strand slope requirements") );

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   bool bCheck, bDesign;
   Float64 s50, s60, s70;
   pSpecEntry->GetMaxStrandSlope(&bCheck,&bDesign,&s50,&s60,&s70);
   pArtifact->IsApplicable( bCheck );

   // we are looking for strand diameter for harped strand slope so use Harped here
   const auto* pStrand = pMaterial->GetStrandMaterial(segmentKey,pgsTypes::Harped);
   Float64 capacity;
   Float64 demand;

   if ( pStrand->GetSize() == WBFL::Materials::PsStrand::Size::D1778  )
   {
      capacity = s70;
   }
   else if ( pStrand->GetSize() == WBFL::Materials::PsStrand::Size::D1524 )
   {
      capacity = s60;
   }
   else
   {
      capacity = s50;
   }

   demand = pStrGeom->GetMaxStrandSlope( segmentKey ); // +/- value
   demand = fabs(demand); // capacity is always positive so use absolute value of demand

   pArtifact->SetCapacity( capacity );
   pArtifact->SetDemand( demand );
}

void pgsDesigner2::CheckHoldDownForce(const CSegmentKey& segmentKey,pgsHoldDownForceArtifact* pArtifact) const
{
   GET_IFACE(ISpecification,pSpec);
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(IPretensionForce,pPrestressForce);

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Checking hold down force requirements"));

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   bool bCheck, bDesign;
   int holdDownForceType;
   Float64 maxHoldDownForce, friction;
   pSpecEntry->GetHoldDownForce(&bCheck,&bDesign,&holdDownForceType,&maxHoldDownForce,&friction);
   pArtifact->IsApplicable( bCheck );

   Float64 demand = pPrestressForce->GetHoldDownForce(segmentKey,holdDownForceType == HOLD_DOWN_TOTAL);

   pArtifact->SetCapacity( maxHoldDownForce );
   pArtifact->SetDemand( demand );
}

void pgsDesigner2::CheckPlantHandlingWeightLimit(const CSegmentKey& segmentKey, pgsPlantHandlingWeightArtifact* pArtifact) const
{
   GET_IFACE(ISpecification, pSpec);
   GET_IFACE(ILibrary, pLib);

   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Checking plant handling weight requirements"));

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());
   bool bCheck;
   Float64 limit;
   pSpecEntry->GetPlantHandlingWeightLimit(&bCheck, &limit);

   GET_IFACE(ISectionProperties, pSectProps);
   Float64 Wg = pSectProps->GetSegmentWeight(segmentKey);

   pArtifact->IsApplicable(bCheck);
   pArtifact->SetWeight(Wg);
   pArtifact->SetWeightLimit(limit);
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

   GET_IFACE(ILibrary, pLib );
   GET_IFACE(ISpecification, pSpec );
   pgsTypes::BridgeAnalysisType bat = (pSpec->GetAnalysisType() == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);

   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   GET_IFACE(IBridge,pBridge);
   // determine spans that are involved in this check
   SpanIndexType startSpanIdx;
   SpanIndexType endSpanIdx;
   pBridge->GetGirderGroupSpans(girderKey.groupIndex,&startSpanIdx,&endSpanIdx);

   // Get the POIs for this girder
   GET_IFACE(IPointOfInterest,pPoi);
   GET_IFACE(IProductForces,pForces);
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      PoiList vPoi;
      pPoi->GetPointsOfInterest(CSpanKey(spanIdx, girderKey.girderIndex), POI_ERECTED_SEGMENT, &vPoi);

      pgsDeflectionCheckArtifact artifact(spanIdx);

      if (pSpecEntry->GetDoEvaluateLLDeflection())
      {
         GET_IFACE(IProgress,pProgress);
         CEAFAutoProgress ap(pProgress);
         pProgress->UpdateMessage( _T("Checking live load deflection requirements") );

         artifact.IsApplicable(true);

         // get max allowable deflection
         Float64 L = pBridge->GetSpanLength(spanIdx,girderKey.girderIndex); // span length for this girder (cl-brg to cl-brg length)
         Float64 ratio = pSpecEntry->GetLLDeflectionLimit();
         ATLASSERT(0 < ratio);
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
   GET_IFACE(ILibrary, pLib);
   GET_IFACE(ISpecification, pSpec);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(spec_name.c_str());
   bool bCheckInclindedGirder = pSpecEntry->CheckGirderInclination();

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
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

      GET_IFACE(ISegmentLiftingPointsOfInterest, pLiftingPoi);

      GET_IFACE(IGirder, pGirder);
      const WBFL::Stability::Girder* pStabilityModel = pGirder->GetSegmentLiftingStabilityModel(segmentKey);
      const WBFL::Stability::LiftingStabilityProblem* pStabilityProblem = pGirder->GetSegmentLiftingStabilityProblem(segmentKey,config, pLiftingPoi);

      WBFL::Stability::StabilityEngineer engineer;
      WBFL::Stability::LiftingResults liftingResults = engineer.AnalyzeLifting(pStabilityModel, pStabilityProblem);

      Float64 zo = liftingResults.Zo[+WBFL::Stability::ImpactDirection::NoImpact];

      GET_IFACE(IPointOfInterest, pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | POI_ERECTED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 2);

      GET_IFACE(IIntervals, pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

      GET_IFACE(ISectionProperties, pSectProp);
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

      Float64 FS = pSpecEntry->GetGirderInclinationFactorOfSafety();

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

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE_NOCHECK(IProductLoads,pProdLoads);

   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(spec_name.c_str());

   // min fillet is zero if girders are adjacently spaced.
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::SupportedBeamSpacing spacingType = pBridgeDesc->GetGirderSpacingType();
   bool isAdjacentSpacing = IsAdjacentSpacing(spacingType);

   GET_IFACE_NOCHECK(IGirderHaunch,pGdrHaunch);
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(girderKey.girderIndex);
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();

   // we need to know if the stirrups engage the deck along the length of the girder
   // below we loop over all segments and do evaluation... we need to know stirrup engaguement
   // before entering the loop.... figure it out here
   GET_IFACE(IStirrupGeometry,pStirrupGeometry);
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
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Checking constructability requirements"));

   bool isHaunchCheck = pSpecEntry->IsSlabOffsetCheckEnabled();
   pgsTypes::HaunchInputDepthType haunchInputType = pBridge->GetHaunchInputDepthType();

#pragma Reminder("Assumes constant slab thickness throughout bridge")
   Float64 tSlab = pBridge->GetGrossSlabDepth(pgsPointOfInterest(CSegmentKey(0,0,0),0.0));

   // Constructibility check is for all segments in a girder
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

            // For no-deck bridges, check only at geometry control interval. This will need to be redefined when no-deck girders are added to pgsplice
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType geomCtrlInterval = pIntervals->GetGeometryControlInterval();
            artifact.SetFinishedElevationControllingInterval(geomCtrlInterval);

            Float64 tolerance = pSpecEntry->GetFinishedElevationTolerance();
            artifact.SetFinishedElevationTolerance(tolerance);

            GET_IFACE(IPointOfInterest,pPoi);
            GET_IFACE(IRoadway,pAlignment);
            GET_IFACE(IDeformedGirderGeometry,pDeformedGirderGeometry);
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

            Float64 tolerance = pSpecEntry->GetFinishedElevationTolerance();
            artifact.SetFinishedElevationTolerance(tolerance);

            GET_IFACE(IPointOfInterest,pPoi);
            GET_IFACE(IRoadway,pAlignment);
            GET_IFACE(IIntervals,pIntervals);
            GET_IFACE(IDeformedGirderGeometry,pDeformedGirderGeometry);
            GET_IFACE_NOCHECK(ISectionProperties,pSectProps);

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

            // Finished elevation and minimum haunch depth checs
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

            Float64 tolerance = pSpecEntry->GetHaunchLoadCamberTolerance();
            artifact.SetHaunchGeometryTolerance(tolerance);

            Float64 assumedExcessCamber = pBridge->GetAssumedExcessCamber(segmentKey.groupIndex,segmentKey.girderIndex);
            artifact.SetAssumedExcessCamber(assumedExcessCamber);

            GET_IFACE(IGirderHaunch,pGdrHaunch);
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
         GET_IFACE(IGirder,pIGirder);
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

      if (pSpecEntry->CheckBottomFlangeClearance() && ::IsGirderSpacing(spacingType))
      {
         artifact.SetBottomFlangeClearanceApplicability(true);

         GET_IFACE(IPointOfInterest,pPoi);

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

         Float64 Cmin = pSpecEntry->GetMinBottomFlangeClearance();
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
         GET_IFACE_NOCHECK(ISectionProperties,pSectProps);
         GET_IFACE(IPointOfInterest,pPoi);

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
   GET_IFACE(IStrandGeometry, pStrandGeometry);
   GET_IFACE(IBridgeDescription, pIBridgeDesc);

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

   GET_IFACE(ISegmentData, pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);
   CComPtr<IIndexArray> arrayPermStrandIndex;
   if (IsGridBasedStrandModel(pStrands->GetStrandDefinitionType()))
   {
      pStrandGeometry->ComputePermanentStrandIndices(segmentKey, strand_type, &arrayPermStrandIndex);
   }

   GET_IFACE(IBridge, pBridge);
   GET_IFACE(IDebondLimits, pDebondLimits);
   GET_IFACE(IProgress, pProgress);
   GET_IFACE(IPointOfInterest, pPoi);
   GET_IFACE(IGirder, pGirder);

   CEAFAutoProgress ap(pProgress);
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


   StrandIndexType nMax10orLess, nMax;
   bool bCheckMaxPerSection;
   Float64 fraMaxPerSection;
   pDebondLimits->GetMaxDebondedStrandsPerSection(segmentKey, &nMax10orLess, &nMax, &bCheckMaxPerSection, &fraMaxPerSection);
   pArtifact->AddMaxDebondStrandsAtSection(nDebonded <= 10 ? nMax10orLess : nMax, bCheckMaxPerSection, fraMaxPerSection);

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
         if (pArtifact->GetSection() == pgsDebondArtifact::K && lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::NinthEdition2020)
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
         ATLASSERT(lrfdVersionMgr::NinthEdition2020 <= lrfdVersionMgr::GetVersion());
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

   lmin_section[pgsTypes::metStart] = lmin_section[pgsTypes::metStart] == Float64_Max ? 0 : lmin_section[pgsTypes::metStart];
   lmin_section[pgsTypes::metEnd] = lmin_section[pgsTypes::metEnd] == Float64_Max ? 0 : lmin_section[pgsTypes::metEnd];
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
      GET_IFACE(IMaterials, pMaterials);
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

void pgsDesigner2::CheckPrincipalTensionStressInWebs(const CSegmentKey& segmentKey, pgsPrincipalTensionStressArtifact* pArtifact) const
{
   // First determine of this check is applicable... 

   if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::EighthEdition2017)
   {
      // this requirement was added in LRFD 8th Edition, 2017... so any spec before this
      // the requirement is not applicable
      pArtifact->SetApplicablity(pgsPrincipalTensionStressArtifact::Specification);
      return;
   }

   // This is always applicable if there is post-tensioning
   // If there isn't post-tensioning, it is only applicable if fc28 > 10 ksi
   GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeometry);
   DuctIndexType nSegmentDucts = pSegmentTendonGeometry->GetDuctCount(segmentKey);

   GET_IFACE(IGirderTendonGeometry, pGirderTendonGeometry);
   DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(segmentKey);

   DuctIndexType nDucts = nSegmentDucts + nGirderDucts;

   if (nDucts == 0)
   {
      // no post-tensioning, check fc
      GET_IFACE(IMaterials, pMaterials);
      if (IsUHPC(pMaterials->GetSegmentConcreteType(segmentKey)))
      {
         pArtifact->SetApplicablity(pgsPrincipalTensionStressArtifact::Applicable);
      }
      else
      {
         Float64 fc = pMaterials->GetSegmentFc28(segmentKey);

         // threshold f'c for performing principal stress check
         GET_IFACE(IAllowableConcreteStress, pAllowable);
         Float64 principalTensileStressFcThreshold = pAllowable->GetPrincipalTensileStressFcThreshold();

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

   GET_IFACE(IIntervals, pIntervals);
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
   GET_IFACE(IMaterials, pMaterials);
   if (pMaterials->GetSegmentConcreteType(segmentKey) == pgsTypes::UHPC)
   {
      // Reinforcement fatigue must be checked for UHPC per GS 1.5.3 using the procedures of LRFD 5.5.3.1
      pArtifact->IsApplicable(true);

      GET_IFACE(ILoadFactors, pILoadFactors);
      const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
      Float64 gamma = pLoadFactors->GetLLIMMax(pgsTypes::FatigueI);

      GET_IFACE(IIntervals, pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

      GET_IFACE(IPointOfInterest, pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT | POI_5L, &vPoi);
      ATLASSERT(vPoi.size() == 1);
      const pgsPointOfInterest& poi(vPoi.front());

      GET_IFACE(IProductForces, pProductForces);
      pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);
      Float64 Mmin, Mmax;
      pProductForces->GetLiveLoadMoment(liveLoadIntervalIdx, pgsTypes::lltFatigue, poi, bat, true/*bIncludeImpact*/, true/*bIncludeLLDF*/, &Mmin, &Mmax);

      GET_IFACE(ISectionProperties, pSectProps);
      Float64 I = pSectProps->GetIxx(liveLoadIntervalIdx, poi);

      GET_IFACE(IStrandGeometry, pStrandGeom);
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

      GET_IFACE(IMaterials, pMaterials);
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

void pgsDesigner2::GetPrincipalWebStressPointsOfInterest(const CSegmentKey & rSegmentKey, IntervalIndexType intervalIdx, PoiList * pPoiList) const
{
   std::vector<CSegmentKey> segmentKeys;
   if (rSegmentKey.segmentIndex == ALL_SEGMENTS)
   {
      CGirderKey gdrKey(rSegmentKey);

      GET_IFACE(IBridge,pBridge);
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
      GET_IFACE(IPointOfInterest, pPoi);
      for (const auto& poiRef : vPois)
      {
         if (!pPoi->IsInCriticalSectionZone(poiRef.get(), pgsTypes::StrengthI))
         {
            pPoiList->emplace_back(poiRef);
         }
      }
   }
}

void pgsDesigner2::DesignEndZone(bool firstPass, const arDesignOptions& options, pgsSegmentDesignArtifact& artifact, IProgress* pProgress) const
{
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
            Float64 fc_max = m_StrandDesignTool.GetMaximumConcreteStrength();
            Float64 fci_min = m_StrandDesignTool.GetMinimumReleaseStrength();
            LOG(_T("We failed to attain release in the early design stages. Let's throw a Hail Mary and set f'c to max  = ")<< WBFL::Units::ConvertFromSysUnits(fc_max, WBFL::Units::Measure::KSI) << _T(" KSI and f'ci to min = ")<< WBFL::Units::ConvertFromSysUnits(fci_min, WBFL::Units::Measure::KSI));

            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(artifact.GetSegmentKey());
            IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;
            m_StrandDesignTool.UpdateReleaseStrength(fci_min, ConcSuccess, StressCheckTask(releaseIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension), pgsTypes::TopGirder);
            m_StrandDesignTool.UpdateConcreteStrength(fc_max,StressCheckTask(lastIntervalIdx, pgsTypes::ServiceIII, pgsTypes::Tension), pgsTypes::BottomGirder);

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

   if ( m_StrandDesignTool.IsDesignHarping() )
   {
      DesignEndZoneHarping(options, artifact, pProgress);
   }
   else
   {
      DesignEndZoneDebonding(firstPass, options, artifact, pProgress);
   }
}

void pgsDesigner2::DesignEndZoneDebonding(bool firstPass, const arDesignOptions& options, pgsSegmentDesignArtifact& artifact, IProgress* pProgress) const
{
   LOG(_T("Entering DesignEndZoneDebonding"));

   // Refine end-zone design. Lifting will always trump the simple release condition because of the
   // shorter span length.
   // Refine design for lifting. Outcome is lifting loop location and required release strength
   // If temporary strands are required, this design refinement will be incomplete. We will move on to
   // design for hauling because it will typically control the temporary strand requirements. Then,
   // we will return to design for lifting.

   // Compute and layout debonding prior to hauling design
   std::vector<DebondLevelType> debond_demand;

   m_StrandDesignTool.DumpDesignParameters();

   if (options.doDesignLifting && m_StrandDesignTool.IsDesignDebonding())
   {
      LOG(_T("*** Initial Lifting Design for Debond Section"));
      DesignForLiftingDebonding(options.doDesignHauling, pProgress);

      if ( m_DesignerOutcome.WasDesignAborted() )
      {
         // attempt to add raised strands to help lifting - might be a no go
         if ( m_StrandDesignTool.AddRaisedStraightStrands() )
         {
            m_DesignerOutcome.Reset();
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::RaisedStraightStrands);
            LOG(_T("Added Raised Straight Strands to control lifting stresses - Restart design with new strand configuration"));
            return;
         }
         else
         {
            LOG(_T("Initial Lifting Debond Design failed"));
            LOG(_T("============================="));
            return;
         }
      }
      else if (m_DesignerOutcome.DidConcreteChange() )
      {
         LOG(_T("Concrete strength changed for initial lifting design - restart"));
         LOG(_T("=============================================================="));
         return; // concrete strength changed, we will want to redo strands 
      }
   }
   else
   {
      LOG(_T(""));
      LOG(_T("*** Design debonding and release strength for Simple Release Condition at endzone"));
      debond_demand = DesignEndZoneReleaseDebonding(pProgress);

      CHECK_PROGRESS;

      if ( m_DesignerOutcome.WasDesignAborted() )
      {
         LOG(_T("Failed to design Debonding for release - Abort"));
         LOG(_T("=================================================="));
         ATLASSERT(false);
         return;
      }
      else if (m_DesignerOutcome.DidFinalConcreteStrengthChange() )
      {
         LOG(_T("Final Concrete strength changed for end zone release - restart"));
         LOG(_T("=============================================================="));
         return; // concrete strength changed, we will want to redo strands 
      }
   }

   m_StrandDesignTool.DumpDesignParameters();

   if (!debond_demand.empty())
   {
      // Layout debonding prior to hauling design
      LOG(_T("Release/Lifting demand = ")<<DumpIntVector(debond_demand));

      bool succ = m_StrandDesignTool.LayoutDebonding( debond_demand );

      if (!succ)
      {
         LOG(_T("Failed to layout Debonding - Abort"));
         LOG(_T("=================================================="));
         m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::DebondDesignFailed);
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
         LOG(_T("Failed Initial Shipping Design - Abort"));
         LOG(_T("========================================"));
         return;
      }
      else if ( m_DesignerOutcome.DidFinalConcreteStrengthChange() )
      {
         // No use going further - number of strands will change for design
         LOG(_T("Final Concrete strength changed for shipping design - restart"));
         LOG(_T("=============================================================="));
         return; 
      }
      else if( m_DesignerOutcome.DidRaiseStraightStrands() )
      {
         LOG(_T("Raised Straight strands were added after DesignForShipping - restart"));
         LOG(_T("===================================================================="));
         return;
      }

      // The only way hauling design can affect liting/release is if temporary strands 
      // were added. Update release strength if this is the case.
      if ( m_DesignerOutcome.GetOutcome(pgsDesignCodes::LiftingRedesignAfterShipping) ||
           0 < m_StrandDesignTool.GetNt() )
      {
         if (options.doDesignLifting && m_StrandDesignTool.IsDesignDebonding())
         {
            LOG(_T("*** Secondary Lifting Design after Shipping."));
            std::vector<DebondLevelType> debond_demand_lifting;
            debond_demand_lifting = DesignForLiftingDebonding(false,pProgress);

            // Only layout debonding if first pass through lifting design could not
            if (m_DesignerOutcome.GetOutcome(pgsDesignCodes::LiftingRedesignAfterShipping) && !debond_demand_lifting.empty())
            {
               LOG(_T("Release/Lifting demand = ")<<DumpIntVector(debond_demand_lifting));

               bool succ = m_StrandDesignTool.LayoutDebonding( debond_demand_lifting );
               if (!succ)
               {
                  LOG(_T("Failed to layout Debonding - Abort"));
                  LOG(_T("=================================================="));
                  m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::DebondDesignFailed);
                  m_DesignerOutcome.AbortDesign();
                  ATLASSERT(false);
                  return;
               }
            }
         }
         else
         {
            LOG(_T("*** Secondary Design of release condition (strength only) after Shipping."));
            DesignEndZoneReleaseStrength(pProgress);
         }

         if ( m_DesignerOutcome.WasDesignAborted() )
         {
            LOG(_T("Second Pass Lifting/Release Debond Design failed - Abort"));
            LOG(_T("========================================================"));
            return;
         }
         else if ( m_DesignerOutcome.DidConcreteChange() )
         {
            LOG(_T("Lifting/Release Design changed concrete strength - Restart"));
            LOG(_T("=========================================================="));
            return;
         }
      }
   }
   else
   {
      LOG(_T(""));
      LOG(_T("Skipping Hauling design"));
   }

   LOG(_T("Exiting DesignEndZoneDebonding"));
}

void pgsDesigner2::DesignEndZoneHarping(arDesignOptions options, pgsSegmentDesignArtifact& artifact, IProgress* pProgress) const
{
   LOG(_T("Entering DesignEndZoneHarping"));

   // Refine end-zone design. Lifting will always trump the simple release condition because of the
   // shorter span length.
   // Refine design for lifting. Outcome is lifting loop location and required release strength
   // If temporary strands are required, this design refinement will be incomplete. We will move on to
   // design for hauling because it will typically control the temporary strand requirements. Then,
   // we will return to design for lifting.

   pgsDesignCodes lifting_design_outcome;

   if (options.doDesignLifting)
   {
      LOG(_T("*** Start Lifting design."));

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
      LOG(_T(""));
      LOG(_T("*** Skipping Lifting design."));

      // lifting will control over simple release, so it's either/or here
      LOG(_T(""));
      LOG(_T("*** Adjust harping height/angle at endzones"));
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

   m_StrandDesignTool.DumpDesignParameters();

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
      LOG(_T(""));
      LOG(_T("Skipping Hauling design"));
   }

   if ( options.doDesignLifting )
   {
      // design for lifting to get the lifting configuration and required
      // release strength for lifting with temporary strands
      LOG(_T("Design for Lifting after Shipping"));
      LOG(_T("================================="));
      DesignForLiftingHarping(options,false,pProgress);

      CHECK_PROGRESS;

      if ( m_DesignerOutcome.WasDesignAborted() )
      {
         LOG(_T("Lifting Design aborted"));
         return;
      }
      else if ( m_DesignerOutcome.DidConcreteChange() )
      {
         LOG(_T("Lifting Design changed concrete strength - Restart"));
         LOG(_T("=================================================="));
         return;
      }
   }

   LOG(_T("Exiting DesignEndZoneHarping"));
}

void pgsDesigner2::DesignMidZone(bool bUseCurrentStrands, const arDesignOptions& options,IProgress* pProgress) const
{
   if ( bUseCurrentStrands )
   {
      m_StrandDesignTool.DumpDesignParameters();
   }

   Int16 cIter = 0;
   Int16 nFutileAttempts=0;
   Int16 nIterMax = 40;
   Int16 nIterEarlyStage = (options.doDesignSlabOffset != sodPreserveHaunch) ? 10 : 5; // Early design stage - NOTE: DO NOT change this value unless you run all design tests VERY SENSITIVE!!!
   StrandIndexType Ns, Nh, Nt;
   Float64 fc, fci, start_slab_offset(0), end_slab_offset(0);

   LOG(_T(""));
   LOG(_T("UPDATE INITIAL DESIGN PARAMETERS IN MID-ZONE"));
   LOG(_T(""));
   LOG(_T("Determine initial design parameters by iterating until # Strands, f'c, f'ci, and Slab offset all converge"));
   LOG(_T(""));

   m_StrandDesignTool.DumpDesignParameters();

   bool bConverged = false;
   do 
   {
      CHECK_PROGRESS;

      m_DesignerOutcome.Reset();

      Ns = m_StrandDesignTool.GetNs();
      Nh = m_StrandDesignTool.GetNh();
      Nt = m_StrandDesignTool.GetNt();
      fc = m_StrandDesignTool.GetConcreteStrength();
      ConcStrengthResultType str_result;
      fci = m_StrandDesignTool.GetReleaseStrength(&str_result);
      if (options.doDesignSlabOffset != sodPreserveHaunch)
      {
      start_slab_offset = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
      end_slab_offset   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);
      }

      LOG(_T(""));
      LOG(_T("Initial Design Parameters Trial # ") << cIter);

      if (1 < cIter)
      {
         std::_tostringstream os2;
         os2 << _T("Initial Strand Design Trial ")<<cIter << std::ends;
         pProgress->UpdateMessage(os2.str().c_str());
      }

      DesignMidZoneInitialStrands(bUseCurrentStrands ? true : (cIter == 0 ? false : true), pProgress);

      if ( m_DesignerOutcome.WasDesignAborted() )
      {
         if ( 0 < m_StrandDesignTool.GetMaxPermanentStrands() && cIter <= nIterEarlyStage && nFutileAttempts < 2)
         {
            // Could be that release strength controls instead of final. Give it a chance.
            LOG(_T("Initial Design Trial # ") << cIter <<_T(" Failed - try to increase release strength to reduce losses"));
            DesignMidZoneAtRelease(options, pProgress);

            if( m_DesignerOutcome.DidRaiseStraightStrands() )
            {
               LOG(_T("Raised Straight strands were added by DesignMidZoneAtRelease in initial throws"));
               return;
            }

            // the purpose of calling DesignMidZoneAtRelease is to adjust the initial release strength
            // if it is too low. The new value is also and Initial Strength... re-initialize the
            // Fci controller with the new _T("Initial") strength
            Float64 newFci = m_StrandDesignTool.GetReleaseStrength(&str_result);
            if ( !IsEqual(fci,newFci) )
            {
               m_StrandDesignTool.InitReleaseStrength( newFci, m_StrandDesignTool.GetReleaseConcreteDesignState().Interval() );
            }

            // Since it aborted, we know that the initial number of strands was bad. The only good info we have is concrete strength
            if (nFutileAttempts == 0)
            {
               m_StrandDesignTool.GuessInitialStrands();
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
         LOG(_T("Did not converge in early stage of iterations = ")<<nIterEarlyStage<<_T(" take a stab at end zone release"));
         LOG(_T("======================================================================================"));
         if ( m_StrandDesignTool.IsDesignDebonding() )
         {
            // For debond design, set debonding to maximum allowed for the current number of strands.
            // This should result in a minimum possible release strength. 
            std::vector<DebondLevelType> max_debonding = m_StrandDesignTool.GetMaxPhysicalDebonding();

            m_StrandDesignTool.LayoutDebonding( max_debonding );
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
            LOG(_T("Added Raised Straight Strands early in DesignMidZone - Restart design with new strand configuration"));
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
            LOG(_T("Concrete Strength Changed - restart design"));
            LOG(_T("======================================="));
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
            LOG(_T("Concrete Strength Changed - restart design"));
            LOG(_T("======================================="));
            continue; // back to the start of the loop
         }
         else if( m_DesignerOutcome.DidRaiseStraightStrands() )
         {
            LOG(_T("Raised Straight strands were added by DesignMidZoneAtRelease in secondary pass"));
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
         Float64 AdiffStart = start_slab_offset - m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
         Float64 AdiffEnd = end_slab_offset - m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);
         Aconverged = (0.0 <= AdiffStart && AdiffStart <= WBFL::Units::ConvertToSysUnits(0.5,WBFL::Units::Measure::Inch)) &&
            (0.0 <= AdiffEnd && AdiffEnd <= WBFL::Units::ConvertToSysUnits(0.5,WBFL::Units::Measure::Inch));
      }
      else
      {
         LOG(_T("Skipping Slab Offset Design due to user input"));
         Aconverged = true; // we did not touch A
      }

      m_StrandDesignTool.DumpDesignParameters();


      LOG(_T("End of trial ")<<cIter);
      LOG(_T("======================================================================")<<cIter);
      LOG(_T("Ns: ")<< (Ns==m_StrandDesignTool.GetNs() ? _T("Converged"):_T("Did not Converge")) );
      LOG(_T("Nh: ")<< (Nh==m_StrandDesignTool.GetNh() ? _T("Converged"):_T("Did not Converge")) );
      LOG(_T("Nt: ")<< (Nt==m_StrandDesignTool.GetNt() ? _T("Converged"):_T("Did not Converge")) );
      LOG(_T("f'c: ")<< (IsEqual(fc,m_StrandDesignTool.GetConcreteStrength()) ? _T("Converged"):_T("Did not Converge")) );
      LOG(_T("f'ci: ")<< (IsEqual(fci,m_StrandDesignTool.GetReleaseStrength()) ? _T("Converged"):_T("Did not Converge")) );
      if (options.doDesignSlabOffset != sodPreserveHaunch)
      {
         LOG(_T("Slab Offset:") << (Aconverged ? _T("Converged") : _T("Did not Converge")));
      }
      LOG(_T("======================================================================")<<cIter);

      if ( Ns == m_StrandDesignTool.GetNs()     &&
           Nh == m_StrandDesignTool.GetNh()       &&
           Nt == m_StrandDesignTool.GetNt()         &&
           IsEqual(fc,m_StrandDesignTool.GetConcreteStrength()) &&
           IsEqual(fci,m_StrandDesignTool.GetReleaseStrength()) &&
           Aconverged
         )
      {
         bConverged = true;
      }
      else
      {
         LOG(_T("# strands, f'c, f'ci, and slab offset have not converged"));
      }

   } while (cIter++ < nIterMax && !bConverged );

   if ( nIterMax <= cIter )
   {
      LOG(_T("Maximum number of iterations was exceeded - aborting design ") << cIter);
      m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::MaxIterExceeded);
      m_DesignerOutcome.AbortDesign();

      // Check with RDP... if cIter >= nIterMax set the design outcome to abort
      // however, we this if-block exits, the design outcome is reset (see below)
      ATLASSERT(false); 
      return;
   }

   LOG(_T("===================================================="));
   LOG(_T("# strands, f'c, f'ci, and slab offset have Converged. Reset outcome and continue"));
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

void pgsDesigner2::DesignMidZoneFinalConcrete(IProgress* pProgress) const
{
   // Note that the name of this function is a bit of a misnomer since most of the 
   // limit states here look in end-zone locations, and the main work of mid-zone stress
   // design has already been done by the initial strands design.
   // The real purpose of this function is to ensure that the decisions made during the
   // mid-zone design allow a viable end-zone design further on.
   // At this point harped strands are lifted to their highest point at girder ends, or
   // debonding is maximal. If we can't find a concrete strength here, there is no end-zone design.
   LOG(_T(""));
   LOG(_T("DesignMidZoneFinalConcrete:: Computing required concrete strength"));

   const CSegmentKey& segmentKey = m_StrandDesignTool.GetSegmentKey();

   Float64 fc_current = m_StrandDesignTool.GetConcreteStrength();

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType noncompositeIntervalIdx = pIntervals->GetLastNoncompositeInterval();
   IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();
   IntervalIndexType lastIntervalIdx          = pIntervals->GetIntervalCount()-1;

   // Maximize stresses at pois for their config
   std::vector<ConcreteStrengthParameters> vConcreteStrengthParameters;
   vConcreteStrengthParameters.push_back(ConcreteStrengthParameters(pgsTypes::ServiceI,_T("Service I final with live load"),lastIntervalIdx,true,pgsTypes::Compression,pgsTypes::BottomGirder,POI_HARPINGPOINT|POI_PSXFER));
   vConcreteStrengthParameters.push_back(ConcreteStrengthParameters(pgsTypes::ServiceI, _T("Service I final without live load"), lastIntervalIdx, false, pgsTypes::Compression, pgsTypes::BottomGirder, POI_HARPINGPOINT | POI_PSXFER));
   vConcreteStrengthParameters.push_back(ConcreteStrengthParameters(pgsTypes::ServiceI, _T("Service I final without live load"), lastIntervalIdx, false, pgsTypes::Compression, pgsTypes::TopGirder, (POI_SPAN | POI_5L)));
   vConcreteStrengthParameters.push_back(ConcreteStrengthParameters(lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? pgsTypes::ServiceIA : pgsTypes::FatigueI,lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? _T("Service IA") : _T("Fatigue I"),lastIntervalIdx,true,pgsTypes::Compression,pgsTypes::BottomGirder,POI_HARPINGPOINT|POI_PSXFER));
   vConcreteStrengthParameters.push_back(ConcreteStrengthParameters(pgsTypes::ServiceIII,_T("Service III"),lastIntervalIdx,true,pgsTypes::Tension,pgsTypes::BottomGirder,POI_HARPINGPOINT|(POI_SPAN | POI_5L)));

   GET_IFACE(IAllowableConcreteStress,pAllowable);
   if ( pAllowable->CheckTemporaryStresses() )
   {
      vConcreteStrengthParameters.push_back(ConcreteStrengthParameters(pgsTypes::ServiceI,_T("Service I non-composite girder"),noncompositeIntervalIdx,true,pgsTypes::Compression,pgsTypes::TopGirder,(POI_SPAN | POI_5L)));
   }


   if ( pAllowable->CheckFinalDeadLoadTensionStress() )
   {
      vConcreteStrengthParameters.push_back(ConcreteStrengthParameters(pgsTypes::ServiceI,_T("Service I final without live load"),lastIntervalIdx,false,pgsTypes::Tension,pgsTypes::BottomGirder,POI_HARPINGPOINT|(POI_SPAN | POI_5L)));
   }

   GET_IFACE(ILimitStateForces,pForces);
   GET_IFACE(IPretensionStresses,pPrestress);
   const GDRCONFIG& config = m_StrandDesignTool.GetSegmentConfiguration();


   for (auto& concParams : vConcreteStrengthParameters)
   {
      // Get Points of Interest at the expected
      PoiList vPOI;
      m_StrandDesignTool.GetDesignPoi(concParams.task.intervalIdx, concParams.find_type, &vPOI);
      ATLASSERT(!vPOI.empty());

      LOG(_T("Checking for ") << concParams.strLimitState << StrTopBot(concParams.stress_location) << (concParams.task.stressType==pgsTypes::Tension?_T(" Tension"):_T(" Compression")) );

      pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Envelope ? pgsTypes::MaxSimpleContinuousEnvelope : (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan));
      for(const pgsPointOfInterest& poi : vPOI)
      {
         CHECK_PROGRESS;

         Float64 min,max;
         pForces->GetDesignStress(concParams.task,poi,concParams.stress_location,&config,bat,&min,&max);

         LOG(_T("     max = ") << WBFL::Units::ConvertFromSysUnits(max,WBFL::Units::Measure::KSI) << _T(" ksi, min = ") << WBFL::Units::ConvertFromSysUnits(min,WBFL::Units::Measure::KSI) << _T(" ksi, at ")<< WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart(), WBFL::Units::Measure::Feet) << _T(" ft") );

         // save max stress and corresponding prestress stress
         if (concParams.task.stressType == pgsTypes::Tension)
         {
            if (concParams.fmax < max)
            {
               concParams.fmax = max;
               concParams.fbpre = pPrestress->GetDesignStress(concParams.task.intervalIdx,poi,concParams.stress_location,config,concParams.task.bIncludeLiveLoad, concParams.task.limitState);

               concParams.poi = poi;
            }
         }
         else
         {
            // compression
            if (min < concParams.fmax)
            {
               concParams.fmax = min;
               concParams.fbpre = pPrestress->GetDesignStress(concParams.task.intervalIdx,poi,concParams.stress_location,config,concParams.task.bIncludeLiveLoad, concParams.task.limitState);

               concParams.poi = poi;
            }
         }
      }
   }

   GET_IFACE(ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

   for( auto & concParams : vConcreteStrengthParameters)
   {
      Float64 k = pLoadFactors->GetDCMax(concParams.task.limitState);

      LOG(_T("Stress Demand (") << concParams.strLimitState << StrTopBot(concParams.stress_location) << _T(" fmax = ") << WBFL::Units::ConvertFromSysUnits(concParams.fmax,WBFL::Units::Measure::KSI) << _T(" ksi, fbpre = ") << WBFL::Units::ConvertFromSysUnits(concParams.fbpre,WBFL::Units::Measure::KSI) << _T(" ksi, ftotal = ") << WBFL::Units::ConvertFromSysUnits(concParams.fmax + k*concParams.fbpre,WBFL::Units::Measure::KSI) << _T(" ksi, at ")<< WBFL::Units::ConvertFromSysUnits(concParams.poi.GetDistFromStart(), WBFL::Units::Measure::Feet) << _T(" ft") );

      concParams.fmax += k*concParams.fbpre;


      Float64 fc_reqd;
      ConcStrengthResultType success = m_StrandDesignTool.ComputeRequiredConcreteStrength(concParams.fmax,concParams.task,&fc_reqd);
      if ( ConcFailed == success )
      {
         LOG(_T("Error calling ComputeRequiredConcreteStrength in  DesignMidZoneFinalConcrete"));
         ATLASSERT(false);
      }
      else
      {
         m_StrandDesignTool.UpdateConcreteStrength(fc_reqd,concParams.task,concParams.stress_location);
      }
      LOG(_T(""));
   }

   LOG(_T("Exiting DesignMidZoneFinalConcrete"));
}

void pgsDesigner2::DesignMidZoneAtRelease(const arDesignOptions& options, IProgress* pProgress) const
{

   LOG(_T(""));
   LOG(_T("Designing Mid-Zone at Release"));

   const CSegmentKey& segmentKey = m_StrandDesignTool.GetSegmentKey();

   GET_IFACE(ILimitStateForces,pForces);
   GET_IFACE(IPretensionStresses,pPrestress);
   GET_IFACE(IProductForces,pProdForces);

   pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(m_StrandDesignTool.GetSegmentKey());
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

   GDRCONFIG config = m_StrandDesignTool.GetSegmentConfiguration();

   // Get Points of Interest in mid-zone
   PoiList vPOI;
   m_StrandDesignTool.GetDesignPoi(releaseIntervalIdx, POI_5L | POI_RELEASED_SEGMENT, &vPOI);
   PoiList vPOI1;
   m_StrandDesignTool.GetDesignPoi(releaseIntervalIdx, POI_HARPINGPOINT, &vPOI1);

   GET_IFACE(IPointOfInterest, pPoi);
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

      Float64  fBotPretension;
      fBotPretension = pPrestress->GetDesignStress(releaseIntervalIdx, poi, pgsTypes::BottomGirder, config, false, pgsTypes::ServiceI);

      min += fBotPretension;

      // save max'd stress and corresponding poi
      if (min < fbot)
      {
         fbot = min;
         bot_poi = poi;
      }
   }

   LOG(_T("Controlling Stress Demand at Release , bottom, compression = ") << WBFL::Units::ConvertFromSysUnits(fbot,WBFL::Units::Measure::KSI) << _T(" KSI at ")<< WBFL::Units::ConvertFromSysUnits(bot_poi.GetDistFromStart(), WBFL::Units::Measure::Feet) << _T(" ft") );

   ConcStrengthResultType release_result;
   Float64 fc  = m_StrandDesignTool.GetConcreteStrength();
   Float64 fci = m_StrandDesignTool.GetReleaseStrength(&release_result);
   LOG(_T("current f'c  = ") << WBFL::Units::ConvertFromSysUnits(fc,WBFL::Units::Measure::KSI) << _T(" KSI") );
   LOG(_T("current f'ci = ") << WBFL::Units::ConvertFromSysUnits(fci,WBFL::Units::Measure::KSI) << _T(" KSI") );

   Float64 fc_comp;
   ConcStrengthResultType success = m_StrandDesignTool.ComputeRequiredConcreteStrength(fbot,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression),&fc_comp);
   if ( success==ConcFailed )
   {
      if ( m_StrandDesignTool.AddRaisedStraightStrands() )
      {
         // Attempt to add raised straight strands if this is an option. Very small chance that it will work 
         // for this case, but...
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::RaisedStraightStrands);
         LOG(_T("Added Raised Straight Strands to control mid-zone compression - Restart design with new strand configuration"));
         return;
      }
      else
      {
         LOG(_T("Could not find adequate release strength to control mid-zone compression - Design Abort") );
         m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::ReleaseStrength);
         m_DesignerOutcome.AbortDesign();
         return;
      }
   }

   LOG(_T("Required Release Strength = ") << WBFL::Units::ConvertFromSysUnits(fc_comp,WBFL::Units::Measure::KSI) << _T(" KSI") );

   // only update if we are increasing release strength - we are downstream here and a decrease is not desired
   if (fci < fc_comp)
   {
      bool bFciUpdated = m_StrandDesignTool.UpdateReleaseStrength(fc_comp, success, StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI, pgsTypes::Compression), pgsTypes::BottomGirder);
      if ( bFciUpdated )
      {
         fci = m_StrandDesignTool.GetReleaseStrength(&release_result);
         LOG(_T("Release Strength Increased to ")  << WBFL::Units::ConvertFromSysUnits(fci, WBFL::Units::Measure::KSI) << _T(" KSI"));
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciIncreased);

         config = m_StrandDesignTool.GetSegmentConfiguration();
      }

      // We can continue if we only increase f'ci, but must restart if final was increased
      Float64 fc_new  = m_StrandDesignTool.GetConcreteStrength();
      if ( !IsEqual(fc,fc_new) )
      {
         LOG(_T("Final Strength Also Increased to ")  << WBFL::Units::ConvertFromSysUnits(fc_new, WBFL::Units::Measure::KSI) << _T(" KSI"));
         LOG(_T("Restart Design loop"));
         LOG(_T("==================="));
         m_DesignerOutcome.SetOutcome(fc < fc_new ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);
         return;
      }
   }
   else
   {
      LOG(_T("New release strength is less than current, no need to update"));
   }

   // Now that we've passed bottom compression, look at top tension.
   // for this, we will try to adjust harped strands...
   GET_IFACE(IAllowableConcreteStress, pAllowable );
   // allowable tension is constant across girder, a dummy poi works in this case
   // so we don't have to lookup the allowable every time through the loop below
   pgsPointOfInterest dummyPOI(segmentKey,0.0);
   Float64 allowable_tension = pAllowable->GetSegmentAllowableTensionStress(dummyPOI,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension),fci,release_result==ConcSuccessWithRebar?true:false);
   LOG(_T("Allowable tensile stress after Release     = ") << WBFL::Units::ConvertFromSysUnits(allowable_tension,WBFL::Units::Measure::KSI) << _T(" KSI") );

   bat = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   Float64 ftop = -Float64_Max;
   Float64 fetop, fptop;
   pgsPointOfInterest top_poi;

   for( const pgsPointOfInterest& poi : vPOI)
   {
      CHECK_PROGRESS;

      Float64 mine, maxe;
      pForces->GetStress(releaseIntervalIdx,pgsTypes::ServiceI,poi,bat,false,pgsTypes::TopGirder,&mine,&maxe);

      Float64 fTopPretension = pPrestress->GetDesignStress(releaseIntervalIdx, poi, pgsTypes::TopGirder, config, false, pgsTypes::ServiceI);

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

   LOG(_T("Controlling Stress Demand at Release, Top, Tension = ") << WBFL::Units::ConvertFromSysUnits(ftop,WBFL::Units::Measure::KSI) << _T(" KSI at ")<< WBFL::Units::ConvertFromSysUnits(top_poi.GetDistFromStart(), WBFL::Units::Measure::Feet) << _T(" ft") );

   if (allowable_tension < ftop)
   {
      LOG(_T("Tension limit exceeded - see what we can do"));

      if (m_StrandDesignTool.IsDesignHarping())
      {
         LOG(_T("Attempt to adjust harped strands"));
         Float64 pps = m_StrandDesignTool.GetPrestressForceMidZone(releaseIntervalIdx,top_poi);

         // Compute eccentricity required to control top tension
         GET_IFACE(ISectionProperties,pSectProp);
         Float64 Ag  = pSectProp->GetAg(releaseIntervalIdx,top_poi);
         Float64 Stg = pSectProp->GetS(releaseIntervalIdx,top_poi,pgsTypes::TopGirder);
         LOG(_T("Ag  = ") << WBFL::Units::ConvertFromSysUnits(Ag, WBFL::Units::Measure::Inch2) << _T(" in^2"));
         LOG(_T("Stg = ") << WBFL::Units::ConvertFromSysUnits(Stg,WBFL::Units::Measure::Inch3) << _T(" in^3"));

         Float64 ecc_target = ComputeTopTensionEccentricity( pps, allowable_tension, fetop, Ag, Stg);
         LOG(_T("Eccentricity Required to control Top Tension   = ") << WBFL::Units::ConvertFromSysUnits(ecc_target, WBFL::Units::Measure::Inch) << _T(" in"));

         // See if eccentricity can be adjusted and keep Final ServiceIII stresses under control
         Float64 min_ecc = m_StrandDesignTool.GetMinimumFinalMidZoneEccentricity();
         LOG(_T("Min eccentricity for bottom tension at BridgeSite3   = ") << WBFL::Units::ConvertFromSysUnits(min_ecc, WBFL::Units::Measure::Inch) << _T(" in"));

        StrandIndexType Nh = m_StrandDesignTool.GetNh();

         GET_IFACE(IStrandGeometry,pStrandGeom);
         Float64 offset_inc = m_StrandDesignTool.GetHarpedHpOffsetIncrement(pStrandGeom);
         if (0 < Nh && 0.0 <= offset_inc && !options.doForceHarpedStrandsStraight )
         {
            LOG(_T("Attempt to adjust by raising harped bundles at harping points"));

            Float64 off_reqd = m_StrandDesignTool.ComputeHpOffsetForEccentricity(top_poi, ecc_target,releaseIntervalIdx);
            LOG(_T("Harped Hp offset required to achieve controlling Eccentricity   = ") << WBFL::Units::ConvertFromSysUnits(off_reqd, WBFL::Units::Measure::Inch) << _T(" in"));

            // round to increment
            off_reqd = CeilOff(off_reqd, offset_inc);
            LOG(_T("Hp Offset Rounded to increment of ")<<WBFL::Units::ConvertFromSysUnits(offset_inc, WBFL::Units::Measure::Inch) << _T(" in = ") << WBFL::Units::ConvertFromSysUnits(off_reqd, WBFL::Units::Measure::Inch) << _T(" in"));

            // offset could push us out of ServiceIII bounds
            Float64 min_off = m_StrandDesignTool.ComputeHpOffsetForEccentricity(top_poi, min_ecc, lastIntervalIdx);
            LOG(_T("Offset Required to Create Min Eccentricity Required Final Bottom Tension   = ") << WBFL::Units::ConvertFromSysUnits(min_off, WBFL::Units::Measure::Inch) << _T(" in"));
            if (off_reqd <= min_off)
            {
               // Attempt to set our offset, this may be lowered to the highest allowed location 
               // if it is out of bounds
               m_StrandDesignTool.SetHarpStrandOffsetHp(pgsTypes::metStart,off_reqd);
               m_StrandDesignTool.SetHarpStrandOffsetHp(pgsTypes::metEnd,  off_reqd);
               LOG(_T("New casting yard eccentricity is ") << WBFL::Units::ConvertFromSysUnits( m_StrandDesignTool.ComputeEccentricity(top_poi,releaseIntervalIdx), WBFL::Units::Measure::Inch) << _T(" in"));
               LOG(_T("New final eccentricity is ") << WBFL::Units::ConvertFromSysUnits( m_StrandDesignTool.ComputeEccentricity(top_poi,lastIntervalIdx), WBFL::Units::Measure::Inch) << _T(" in"));
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);

               // make sure the job was complete
               Float64 new_off = m_StrandDesignTool.GetHarpStrandOffsetHp(pgsTypes::metStart);
               if (new_off == off_reqd)
               {
                  // Seems like a miracle with all of the conditions around here, but we succeeded
                  LOG(_T("Strands at HP offset set successfully - Continue Onward"));
                  return;
               }
               else
               {
                  // our offset attempt ran into physical constraints or hold down overload. 
                  LOG(_T("Offset at HP not fully completed. Perhaps a change in strength can finish the job?"));
               }
            }
            else
            {
               // so close, but offset failed. fallback is to increase concrete strength
               LOG(_T("Offset Eccentricity has pushed us out of Service allowable zone - Set as high as possible and hope more concrete strength will fix problem"));
               off_reqd = FloorOff(min_off,offset_inc);
               LOG(_T("Hp Offset Rounded to increment of ")<<WBFL::Units::ConvertFromSysUnits(offset_inc, WBFL::Units::Measure::Inch) << _T(" in = ") << WBFL::Units::ConvertFromSysUnits(off_reqd, WBFL::Units::Measure::Inch) << _T(" in"));

               m_StrandDesignTool.SetHarpStrandOffsetHp(pgsTypes::metStart,off_reqd);
               m_StrandDesignTool.SetHarpStrandOffsetHp(pgsTypes::metEnd,  off_reqd);
               LOG(_T("New casting yard eccentricity is ") << WBFL::Units::ConvertFromSysUnits( m_StrandDesignTool.ComputeEccentricity(top_poi,releaseIntervalIdx), WBFL::Units::Measure::Inch) << _T(" in"));
               LOG(_T("New final eccentricity is ") << WBFL::Units::ConvertFromSysUnits( m_StrandDesignTool.ComputeEccentricity(top_poi,lastIntervalIdx), WBFL::Units::Measure::Inch) << _T(" in"));
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
            }
         }
         else
         {
            // TxDOT - non-standard adjustment (Texas Two-Step)
            LOG(_T("Attempt to trade straight strands for harped to relieve top tension - TxDOT non-standard adjustment"));

            StrandIndexType nh_reqd, ns_reqd;
            if (m_StrandDesignTool.ComputeAddHarpedForMidZoneReleaseEccentricity(top_poi, ecc_target, min_ecc, &ns_reqd, &nh_reqd))
            {
               // number of straight/harped were changed. Set them
               LOG(_T("Number of Straight/Harped were changed from ")<<m_StrandDesignTool.GetNs()<<_T("/")<<Nh<<_T(" to ")<<ns_reqd<<_T("/")<<nh_reqd);
               m_StrandDesignTool.SetNumStraightHarped(ns_reqd, nh_reqd);

               LOG(_T("Strands at HP Release set successfully - Continue Onward"));
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
               return;
            }
            else
            {
               LOG(_T("Attempt to trade straight strands for harped to relieve top tension failed."));
            }
         }
      }
      else
      {
         LOG(_T("This is a debond or straight strand design. Adjusting strands in mid-zone is not a remedy"));
      }

      // If we are here,
      LOG(_T("Only option left is to try to increase release strength to control top tension"));

      Float64 fci_reqd;
      ConcStrengthResultType success = m_StrandDesignTool.ComputeRequiredConcreteStrength(ftop,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension),&fci_reqd);
      if ( success != ConcFailed )
      {
         Float64 fci_old = m_StrandDesignTool.GetReleaseStrength();
         LOG(_T("Successfully Increased Release Strength for Release , Top, Tension psxfer  = ") << WBFL::Units::ConvertFromSysUnits(fci_reqd,WBFL::Units::Measure::KSI) << _T(" KSI") );
         m_StrandDesignTool.UpdateReleaseStrength(fci_reqd,success,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension),pgsTypes::TopGirder);
         m_DesignerOutcome.SetOutcome(fci_old<fci_reqd ? pgsDesignCodes::FciIncreased : pgsDesignCodes::FciDecreased);

         Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
         if ( !IsEqual(fc,fc_new) )
         {
            LOG(_T("However, Final Was Also Increased to ") << WBFL::Units::ConvertFromSysUnits(fc_new,WBFL::Units::Measure::KSI) << _T(" KSI") );
            LOG(_T("Restart design with new strengths"));
            m_DesignerOutcome.SetOutcome(fc < fc_new ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);
         }
      }
      else if ( m_StrandDesignTool.AddRaisedStraightStrands() )
      {
         // Attempt to add raised straight strands if this is an option. We could abort if the attempt
         // fails, but give bump 500 a chance if we go down in smoke.
         // If we are here, outer algorithm will restart.
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::RaisedStraightStrands);
         LOG(_T("Added Raised Straight Strands - Restart design with new strand configuration"));
      }
      else
      {
         // Last resort, increase strengths by 500 psi and restart
         bool bSuccess = m_StrandDesignTool.Bump500(StressCheckTask(releaseIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension), pgsTypes::TopGirder);
         if (bSuccess)
         {
            LOG(_T("Just threw a Hail Mary - Restart design with 500 psi higher concrete strength"));
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciIncreased);
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcIncreased);
         }
         else
         {
            LOG(_T("Concrete Strength Cannot be adjusted"));
            m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::ReleaseStrength);
            m_DesignerOutcome.AbortDesign();
         }
      }
   }  // ftop>allowable_tension
}

void pgsDesigner2::DesignSlabOffset(IProgress* pProgress) const
{
   GET_IFACE_NOCHECK(ISpecification,pSpec);
   GET_IFACE(IBridge,pBridge);
   if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
   {
      LOG(_T(""));
      LOG(_T("Skipping A-dimension design because there is no deck"));
      // no deck
      return;
   }

   const CSegmentKey& segmentKey = m_StrandDesignTool.GetSegmentKey();

   Float64 AorigStart = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
   Float64 AorigEnd   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);

   Float64 assumedExcessCamberOrig = m_StrandDesignTool.GetAssumedExcessCamber();

   // Iterate on _T("A") dimension and initial number of prestressing strands
   // Use a relaxed tolerance on _T("A") dimension.
   Int16 cIter = 0;
   Int16 nIterMax = 20;
   bool bDone = false;

   // Iterate until we come up with an _T("A") dimension and some strands
   // that are consistent for the current values of f'c and f'ci
   LOG(_T(""));
   LOG(_T("Computing A-dimension requirement"));
   LOG(_T("A-dim Current (Start)   = ") << WBFL::Units::ConvertFromSysUnits(AorigStart, WBFL::Units::Measure::Inch) << _T(" in") );
   LOG(_T("A-dim Current (End)     = ") << WBFL::Units::ConvertFromSysUnits(AorigEnd,   WBFL::Units::Measure::Inch) << _T(" in") );
   if (m_StrandDesignTool.IsDesignExcessCamber())
   {
      LOG(_T("AssumedExcessCamber Current    = ") << WBFL::Units::ConvertFromSysUnits(assumedExcessCamberOrig,   WBFL::Units::Measure::Inch) << _T(" in") );
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
      LOG(os2.str().c_str());

      Float64 AoldStart = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
      Float64 AoldEnd   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);

      Float64 assumedExcessCamberOld = m_StrandDesignTool.GetAssumedExcessCamber();

      // Make a guess at the "A" dimension using this initial strand configuration
      SLABOFFSETDETAILS slab_offset_details;
      GDRCONFIG config = m_StrandDesignTool.GetSegmentConfiguration();
      config.SlabOffset[pgsTypes::metStart] = AoldStart;
      config.SlabOffset[pgsTypes::metEnd]   = AoldEnd;
      config.AssumedExcessCamber = assumedExcessCamberOld;
      GetSlabOffsetDetails(segmentKey,&config,&slab_offset_details);

      IndexType idx = slab_offset_details.SlabOffset.size()/2;
      ATLASSERT(slab_offset_details.SlabOffset[idx].PointOfInterest.IsMidSpan(POI_ERECTED_SEGMENT));
      LOG(_T("Girder Orientation Effect = ") << WBFL::Units::ConvertFromSysUnits(slab_offset_details.SlabOffset[idx].GirderOrientationEffect, WBFL::Units::Measure::Inch) << _T(" in"));
      LOG(_T("Profile Effect = ") << WBFL::Units::ConvertFromSysUnits(slab_offset_details.SlabOffset[idx].ProfileEffect, WBFL::Units::Measure::Inch) << _T(" in"));
      LOG(_T("D = ") << WBFL::Units::ConvertFromSysUnits(slab_offset_details.SlabOffset[idx].D, WBFL::Units::Measure::Inch) << _T(" in"));
      LOG(_T("C = ") << WBFL::Units::ConvertFromSysUnits(slab_offset_details.SlabOffset[idx].C, WBFL::Units::Measure::Inch) << _T(" in"));
      LOG(_T("Camber Effect = ") << WBFL::Units::ConvertFromSysUnits(slab_offset_details.SlabOffset[idx].CamberEffect, WBFL::Units::Measure::Inch) << _T(" in"));
      LOG(_T("A-dim Calculated (raw) = ") << WBFL::Units::ConvertFromSysUnits(slab_offset_details.RequiredMaxSlabOffsetRaw, WBFL::Units::Measure::Inch) << _T(" in"));

      Float64 Anew = slab_offset_details.RequiredMaxSlabOffsetRaw;

      Float64 Amin = m_StrandDesignTool.GetMinimumSlabOffset();
      if (Anew < Amin)
      {
         LOG(_T("Calculated A-dim is less than minimum. Using minimum = ") << WBFL::Units::ConvertFromSysUnits(Amin, WBFL::Units::Measure::Inch) << _T(" in"));
         Anew = Amin;
      }

      if ( IsZero( AoldStart - Anew, tolerance ) && IsZero( AoldEnd - Anew, tolerance ))
      {
         Float64 a;
         a = RoundSlabOffsetValue(pSpec, Max(AoldStart, AoldEnd, Anew) );
         m_StrandDesignTool.SetSlabOffset( pgsTypes::metStart, a );
         m_StrandDesignTool.SetSlabOffset( pgsTypes::metEnd,   a );
         LOG(_T("A-dim camber converged."));

         bDone = true;
      }
      else
      {
         m_StrandDesignTool.SetSlabOffset( pgsTypes::metStart, Anew );
         m_StrandDesignTool.SetSlabOffset( pgsTypes::metEnd,   Anew );
      }

      if (m_StrandDesignTool.IsDesignExcessCamber())
      {
         Float64 ctoler = m_StrandDesignTool.GetAssumedExcessCamberTolerance();
         Float64 computed_camber = slab_offset_details.SlabOffset.at(idx).CamberEffect;
         LOG(_T("Excess Camber Computed = ") << WBFL::Units::ConvertFromSysUnits(computed_camber, WBFL::Units::Measure::Inch) << _T(" in"));
         if (IsZero(assumedExcessCamberOld - computed_camber, ctoler))
         {
            Float64 c;
            c = RoundOff(computed_camber, ctoler);
            LOG(_T("Excess camber converged."));
            m_StrandDesignTool.SetAssumedExcessCamber(c);

            bDone &= true;
         }
         else
         {
            m_StrandDesignTool.SetAssumedExcessCamber(computed_camber);
            LOG(_T("Excess camber does not match within tolerance."));
            bDone = false;
         }
      }

   } while ( !bDone && cIter++ < nIterMax);

   if ( nIterMax < cIter )
   {
      LOG(_T("Maximum number of iterations was exceeded - aborting Slab offset design ") << cIter);
      m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::MaxIterExceeded);
      m_DesignerOutcome.AbortDesign();
   }

   if (bDone)
   {
      Float64 AnewStart = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
      Float64 AnewEnd   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);

      // don't let the new A be much larger than the old, or lots less
      if (  ( AorigStart < (AnewStart - tolerance) || (AnewStart + 2.0*tolerance) < AorigStart ) ||
            ( AorigEnd   < (AnewEnd   - tolerance) || (AnewEnd   + 2.0*tolerance) < AorigEnd   ) )
      {
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::SlabOffsetChanged);
      }
   }
}

void pgsDesigner2::DesignMidZoneInitialStrands(bool bUseCurrentStrands, IProgress* pProgress) const
{
   // Figure out the number of strands required to make the prestressing
   // work at the bottom centerline of the span at ServiceIII limit state,
   // using the current values for "A", f'c, and f'ci.

   // The only way to continue to the next step from this function is to have adequate concrete
   // strength and the minimum number of strands for tension to control at mid-span

   LOG(_T("** DesignMidZoneInitialStrands"));
   LOG(_T("Computing initial prestressing requirements for Service in Mid-Zone"));

   const CSegmentKey& segmentKey = m_StrandDesignTool.GetSegmentKey();

   GET_IFACE(IIntervals, pIntervals);
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
   GET_IFACE(IBridge, pBridge);

   GET_IFACE(ISegmentData, pSegmentData);
   const CGirderMaterial* pGirderMaterial = pSegmentData->GetSegmentMaterial(segmentKey);

   // Get controlling Point of Interest at mid zone
   pgsPointOfInterest poi = GetControllingFinalMidZonePoi(segmentKey);

   GET_IFACE(IPointOfInterest, pPoi);
   IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);

   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(deckCastingRegionIdx);

   Float64 fcgdr = m_StrandDesignTool.GetConcreteStrength();

   // Get the section properties of the girder
   GET_IFACE(ISectionProperties, pSectProp);
   Float64 Ag = pSectProp->GetAg(releaseIntervalIdx, poi);
   Float64 Stg = pSectProp->GetS(releaseIntervalIdx, poi, pgsTypes::TopGirder);
   Float64 Sbg = pSectProp->GetS(releaseIntervalIdx, poi, pgsTypes::BottomGirder);
   LOG(_T("Ag  = ") << WBFL::Units::ConvertFromSysUnits(Ag, WBFL::Units::Measure::Inch2) << _T(" in^2"));
   LOG(_T("Stg = ") << WBFL::Units::ConvertFromSysUnits(Stg, WBFL::Units::Measure::Inch3) << _T(" in^3"));
   LOG(_T("Sbg = ") << WBFL::Units::ConvertFromSysUnits(Sbg, WBFL::Units::Measure::Inch3) << _T(" in^3"));

   LOG(_T("Stcg = ") << WBFL::Units::ConvertFromSysUnits(pSectProp->GetS(lastIntervalIdx, poi, pgsTypes::TopGirder), WBFL::Units::Measure::Inch3) << _T(" in^3"));
   LOG(_T("Sbcg = ") << WBFL::Units::ConvertFromSysUnits(pSectProp->GetS(lastIntervalIdx, poi, pgsTypes::BottomGirder), WBFL::Units::Measure::Inch3) << _T(" in^3"));

   LOG(_T("Stcg_adjusted = ") << WBFL::Units::ConvertFromSysUnits(pSectProp->GetS(lastIntervalIdx, poi, pgsTypes::TopGirder, fcgdr), WBFL::Units::Measure::Inch3) << _T(" in^3"));
   LOG(_T("Sbcg_adjusted = ") << WBFL::Units::ConvertFromSysUnits(pSectProp->GetS(lastIntervalIdx, poi, pgsTypes::BottomGirder, fcgdr), WBFL::Units::Measure::Inch3) << _T(" in^3"));

   GET_IFACE(IProductForces, pProductForces);
   pgsTypes::BridgeAnalysisType bat = pProductForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   PierIndexType startPierIdx, endPierIdx;
   pBridge->GetGirderGroupPiers(segmentKey.groupIndex, &startPierIdx, &endPierIdx);
   ATLASSERT(endPierIdx == startPierIdx + 1);

   if (m_StrandDesignTool.IsDesignSlabOffset())
   {
   LOG(_T(""));
      LOG(_T("Bridge A dimension  (Start) = ") << WBFL::Units::ConvertFromSysUnits(pBridge->GetSlabOffset(segmentKey,pgsTypes::metStart),WBFL::Units::Measure::Inch) << _T(" in"));
      LOG(_T("Bridge A dimension  (End)   = ") << WBFL::Units::ConvertFromSysUnits(pBridge->GetSlabOffset(segmentKey,pgsTypes::metEnd),WBFL::Units::Measure::Inch) << _T(" in"));
      LOG(_T("Current A dimension (Start) = ") << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart),WBFL::Units::Measure::Inch) << _T(" in"));
      LOG(_T("Current A dimension (End)   = ") << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd),WBFL::Units::Measure::Inch) << _T(" in"));
   }
   LOG(_T(""));
   LOG(_T("M girder      = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(erectSegmentIntervalIdx, pgsTypes::pftGirder, poi, bat, rtCumulative), WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
   LOG(_T("M diaphragm   = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(castDiaphragmIntervalIdx, pgsTypes::pftDiaphragm, poi, bat, rtIncremental), WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
   if (castLongitudinalJointIntervalIdx != INVALID_INDEX)
   {
      LOG(_T("M longitudinal joint = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(castLongitudinalJointIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, bat, rtIncremental), WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
   }

   if (castShearKeyIntervalIdx != INVALID_INDEX)
   {
      LOG(_T("M shear key   = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(castShearKeyIntervalIdx, pgsTypes::pftShearKey, poi, bat, rtIncremental), WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
   }

   LOG(_T("M construction= ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(constructionLoadIntervalIdx, pgsTypes::pftConstruction, poi, bat, rtIncremental), WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
   
   if (castDeckIntervalIdx != INVALID_INDEX)
   {
      LOG(_T("M slab        = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlab, poi, bat, rtIncremental), WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
      LOG(_T("dM slab       = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetDesignSlabMomentAdjustment(poi, &m_StrandDesignTool.GetSegmentConfiguration()), WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
      LOG(_T("M slab pad    = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlabPad, poi, bat, rtIncremental), WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
      LOG(_T("dM slab pad   = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetDesignSlabPadMomentAdjustment(poi, &m_StrandDesignTool.GetSegmentConfiguration()), WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
      LOG(_T("M panel       = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlabPanel, poi, bat, rtIncremental), WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
   }
   LOG(_T("M user dc (1) = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(noncompositeUserLoadIntervalIdx,pgsTypes::pftUserDC,poi,bat, rtIncremental),WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
   LOG(_T("M user dw (1) = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(noncompositeUserLoadIntervalIdx,pgsTypes::pftUserDW,poi,bat, rtIncremental),WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
   LOG(_T("M barrier     = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(railingSystemIntervalIdx,pgsTypes::pftTrafficBarrier,poi,bat, rtIncremental),WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
   LOG(_T("M sidewalk    = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(railingSystemIntervalIdx,pgsTypes::pftSidewalk      ,poi,bat, rtIncremental),WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
   LOG(_T("M user dc (2) = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(compositeUserLoadIntervalIdx,pgsTypes::pftUserDC,poi,bat, rtIncremental),WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
   LOG(_T("M user dw (2) = ") << WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(compositeUserLoadIntervalIdx,pgsTypes::pftUserDW,poi,bat, rtIncremental),WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
   LOG(_T("M overlay     = ") << (overlayIntervalIdx==INVALID_INDEX ? 0.0 : WBFL::Units::ConvertFromSysUnits(pProductForces->GetMoment(overlayIntervalIdx,pgsTypes::pftOverlay,poi,bat, rtIncremental),WBFL::Units::Measure::KipFeet)) << _T(" k-ft"));

#if defined ENABLE_LOGGING
   Float64 Mllmax, Mllmin;
   pProductForces->GetLiveLoadMoment(lastIntervalIdx,pgsTypes::lltDesign,poi,bat,true,false,&Mllmin,&Mllmax);
   LOG(_T("M ll+im min   = ") << WBFL::Units::ConvertFromSysUnits(Mllmin,WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
   LOG(_T("M ll+im max   = ") << WBFL::Units::ConvertFromSysUnits(Mllmax,WBFL::Units::Measure::KipFeet) << _T(" k-ft"));

   Float64 fc_lldf = fcgdr;
   if ( pGirderMaterial->Concrete.bUserEc )
   {
      fc_lldf = lrfdConcreteUtil::FcFromEc( (WBFL::Materials::ConcreteType)(pGirderMaterial->Concrete.Type), pGirderMaterial->Concrete.Ec, pGirderMaterial->Concrete.StrengthDensity );
   }

   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);
   Float64 gV, gpM, gnM;
   pLLDF->GetDistributionFactors(poi,pgsTypes::StrengthI,fc_lldf,&gpM,&gnM,&gV);
   LOG(_T("LLDF = ") << gpM);
   LOG(_T(""));
#endif

   // Initial potential controlling design cases during service
   GET_IFACE(IAllowableConcreteStress,pAllowStress);
   GET_IFACE(ILimitStateForces,pForces);
   std::vector<InitialDesignParameters> vInitialDesignParameters;
   // In the past, we looked at these cases, but that was a mistake. The design strategy is to determine the number of strands required to satisfy the
   // tension limits. We never manipulate number of strands to satisfy compression. Compression limits are satisfied by changing f'ci/f'c
   // The following 3 lines are commented out because we don't want to look at the compression cases but left here as a reminder of what we don't want to do.
   //vInitialDesignParameters.push_back(InitialDesignParameters(lastIntervalIdx, true /*with live load*/,  pgsTypes::ServiceI, _T("Service I"), pgsTypes::TopGirder, _T("Top"), pgsTypes::Compression)); // 0.6f'c
   //vInitialDesignParameters.push_back(InitialDesignParameters(lastIntervalIdx, false /*without live load*/, pgsTypes::ServiceI, _T("Service I"), pgsTypes::TopGirder, _T("Top"), pgsTypes::Compression)); // 0.45f'c
   //vInitialDesignParameters.push_back(InitialDesignParameters(lastIntervalIdx, true /*with live load*/,  lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? pgsTypes::ServiceIA : pgsTypes::FatigueI,lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? _T("Service IA") : _T("Fatigue I"),pgsTypes::TopGirder,_T("Top"),pgsTypes::Compression));
   vInitialDesignParameters.push_back(InitialDesignParameters(lastIntervalIdx, true /*with live load*/,  pgsTypes::ServiceIII,_T("Service III"),pgsTypes::BottomGirder,_T("Bottom"),pgsTypes::Tension));
   if ( pAllowStress->CheckFinalDeadLoadTensionStress() )
   {
      vInitialDesignParameters.push_back(InitialDesignParameters(lastIntervalIdx,false /*without live load*/,pgsTypes::ServiceI,_T("Service I"),pgsTypes::BottomGirder,_T("Bottom"),pgsTypes::Tension));
   }
   
   GET_IFACE(ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

   const GDRCONFIG& config = m_StrandDesignTool.GetSegmentConfiguration();

   for(auto& designParams : vInitialDesignParameters)
   {
      LOG(_T(""));
      pForces->GetDesignStress(designParams.task,poi,designParams.stress_location,&config,bat,&designParams.fmin,&designParams.fmax);

      Float64 f_demand = ( designParams.task.stressType == pgsTypes::Compression ) ? designParams.fmin : designParams.fmax;
      LOG(_T("Stress Demand (") << pIntervals->GetDescription(designParams.task.intervalIdx) << _T(", ") << designParams.strLimitState << _T(", ") << designParams.strStressLocation << _T(", mid-span) = ") << WBFL::Units::ConvertFromSysUnits(f_demand,WBFL::Units::Measure::KSI) << _T(" KSI") );


      // Get allowable stress 
      ATLASSERT(designParams.task.stressType == pgsTypes::Tension);
      designParams.fAllow = pAllowStress->GetSegmentAllowableTensionStress(poi,designParams.task,m_StrandDesignTool.GetConcreteStrength(),false);
      LOG(_T("Allowable stress (") << designParams.strLimitState << _T(") = ") << WBFL::Units::ConvertFromSysUnits(designParams.fAllow,WBFL::Units::Measure::KSI)  << _T(" KSI"));

      // Compute required stress due to prestressing
      Float64 k = pLoadFactors->GetDCMax(designParams.task.limitState);
      designParams.fpre = IsZero(k) ? 0 : (designParams.fAllow - f_demand)/k;

      LOG(_T("Reqd stress due to prestressing (") << designParams.strLimitState << _T(") = ") << WBFL::Units::ConvertFromSysUnits(designParams.fpre,WBFL::Units::Measure::KSI) << _T(" KSI") );
   }

   // Guess the number of strands if first time through. otherwise use previous guess
   if ( bUseCurrentStrands )
   {
      // Not the first time through. 
      // We could be here because concrete strength increased and because of that, we may need less strands.
      // The design algorithm can overshoot np because eccentricity typically reduces with increased strands.
      // So, reduce to the next available if possible.
      StrandIndexType np = m_StrandDesignTool.GetNumPermanentStrands();

      StrandIndexType npmin = Max((StrandIndexType)3, m_StrandDesignTool.GetMinimumPermanentStrands());

      if (npmin < np)
      {
         np = m_StrandDesignTool.GetPreviousNumPermanentStrands(np);
         LOG(_T(""));
         LOG(_T("Reducing num permanent strands from ") << m_StrandDesignTool.GetNumPermanentStrands() << _T(" to ") << np);
         ATLASSERT(0 < np);
         m_StrandDesignTool.SetNumPermanentStrands(np);
      }
   }
   else
   {
      // uses minimal number of strands
      m_StrandDesignTool.GuessInitialStrands();
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

      LOG(_T(""));
      LOG(_T("Strand Configuration Trial # ") << cIter);

      LOG(_T("Reset end-zone strands maximize harping or debonding effect"));
      if (!m_StrandDesignTool.ResetEndZoneStrandConfig())
      {
         LOG(_T("ERROR - Could not reset end-zone offsets to maximize differential"));
         // this error is not very descriptive, but it probably means that there is no way for the strands to fit 
         // within offset bounds. This should have been caught in the library
         m_DesignerOutcome.AbortDesign();
         m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::TooManyStrandsReqd);
         ATLASSERT(false);
         return;
      }

      LOG(_T("Guess at number of strands -> Ns = ") << m_StrandDesignTool.GetNs() << _T(" Nh = ") << m_StrandDesignTool.GetNh() << _T(" Nt = ") << m_StrandDesignTool.GetNt());
      m_StrandDesignTool.DumpDesignParameters();

      // Compute prestress force required to achieve fpre to satisfy the tension limits
      Float64 fNreqd = -Float64_Max;
      Float64 ecc = 0;
      const InitialDesignParameters* pControllingParams = nullptr;
      for( auto& designParams : vInitialDesignParameters)
      {
         Float64 thisEcc = m_StrandDesignTool.ComputeEccentricity(poi, designParams.task.intervalIdx);
         LOG(_T("Eccentricity at mid-span = ") << WBFL::Units::ConvertFromSysUnits(thisEcc, WBFL::Units::Measure::Inch) << _T(" in"));

         LOG(_T(""));
         LOG(_T("Determine required prestressing for ") << designParams.strLimitState);

         LOG(_T("Required prestress force, P = fpre / [1/Ag + ecc/S]"));
         Float64 S = (designParams.stress_location == pgsTypes::TopGirder ? Stg : Sbg);
         designParams.Preqd = designParams.fpre / (1.0 / Ag + thisEcc / S);
         LOG(_T("Required prestress force (") << designParams.strLimitState << _T(") = ") << WBFL::Units::ConvertFromSysUnits(designParams.fpre, WBFL::Units::Measure::KSI) << _T("/[ 1/") << WBFL::Units::ConvertFromSysUnits(Ag, WBFL::Units::Measure::Inch2) << _T(" + ") << WBFL::Units::ConvertFromSysUnits(thisEcc, WBFL::Units::Measure::Inch) << _T("/") << WBFL::Units::ConvertFromSysUnits(S, WBFL::Units::Measure::Inch3) << _T("] = ") << WBFL::Units::ConvertFromSysUnits(-designParams.Preqd, WBFL::Units::Measure::Kip) << _T(" Kip"));
         m_StrandDesignTool.ComputePermanentStrandsRequiredForPrestressForce(poi, &designParams);
         LOG(_T("Required number of strands = ") << designParams.fN << _T(" (") << designParams.Np << _T(")"));
         if (fNreqd < designParams.fN)
         {
            fNreqd = designParams.fN;
            pControllingParams = &designParams;
            ecc = thisEcc;
         }

         LOG(_T(""));
      }

      LOG(_T("Required prestress force = ") << WBFL::Units::ConvertFromSysUnits(-pControllingParams->Preqd, WBFL::Units::Measure::Kip) << _T(" Kip"));
      LOG(_T(""));

      Np = 0.0 < pControllingParams->fN ? pControllingParams->Np : 0; // Np is unsigned - don't let negative conversion cause problems

      // see if we can match Np and ecc
      if ( Np == INVALID_INDEX )
      {
         StrandIndexType npmax = m_StrandDesignTool.GetMaxPermanentStrands();
         if (m_StrandDesignTool.GetNumPermanentStrands()==npmax)
         {
            LOG(_T("**** TOO MANY STRANDS REQUIRED **** - already tried max= ")<<npmax);

            // OK, This is a final gasp - we have maxed out strands, now see if we can get a reasonable concrete strength
            //     to relieve tension before puking
            LOG(_T("Hail Mary - See if reasonable concrete strength can satisfy tension limit"));
            const GDRCONFIG& config = m_StrandDesignTool.GetSegmentConfiguration();
            GET_IFACE(IPretensionStresses,pPsStress);
            Float64 fBotPre = pPsStress->GetDesignStress(pControllingParams->task.intervalIdx, poi, pControllingParams->stress_location, config, pControllingParams->task.bIncludeLiveLoad, pControllingParams->task.limitState);
            Float64 k = pLoadFactors->GetDCMax(pControllingParams->task.limitState);
            Float64 f_allow_required = pControllingParams->fmax+k*fBotPre;
            LOG(_T("Required allowable = fb ") << pControllingParams->strLimitState << _T(" + fb Prestress = ") << WBFL::Units::ConvertFromSysUnits(pControllingParams->fmax,WBFL::Units::Measure::KSI) << _T(" + ") << WBFL::Units::ConvertFromSysUnits(fBotPre,WBFL::Units::Measure::KSI) << _T(" = ") << WBFL::Units::ConvertFromSysUnits(f_allow_required,WBFL::Units::Measure::KSI) << _T(" KSI"));
            Float64 fc_rqd;
            if ( ConcFailed != m_StrandDesignTool.ComputeRequiredConcreteStrength(f_allow_required, pControllingParams->task,&fc_rqd) )
            {
               // Use user-defined practical upper limit here
               Float64 max_girder_fc = m_StrandDesignTool.GetMaximumConcreteStrength();
               LOG(_T("User-defined upper limit for final girder concrete = ") << WBFL::Units::ConvertFromSysUnits(max_girder_fc,WBFL::Units::Measure::KSI) << _T(" KSI. Computed required strength = ")<< WBFL::Units::ConvertFromSysUnits(fc_rqd,WBFL::Units::Measure::KSI) << _T(" KSI"));

               if (fc_rqd <= max_girder_fc)
               {
                  Float64 fc_old = m_StrandDesignTool.GetConcreteStrength();

                  bool bFcUpdated = m_StrandDesignTool.UpdateConcreteStrength(fc_rqd,StressCheckTask(lastIntervalIdx,pgsTypes::ServiceIII,pgsTypes::Tension),pgsTypes::BottomGirder);
                  if ( bFcUpdated )
                  {
                     Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
                     m_DesignerOutcome.SetOutcome(fc_old < fc_new ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);

                     // Tricky: Use concrete growth relationship for this case:
                     // Many times the reason we are not converging here is high initial losses due to a low f'ci
                     // Don't allow f'ci to be more than 2ksi smaller than final (TxDOT research supports this value)
                     Float64 fc_curr = m_StrandDesignTool.GetConcreteStrength();
                     ConcStrengthResultType strength_result;
                     Float64 fci_curr = m_StrandDesignTool.GetReleaseStrength(&strength_result);
                     Float64 fc_2k = WBFL::Units::ConvertToSysUnits(2.0,WBFL::Units::Measure::KSI); // add one to protect lt
                     if (fc_curr-fci_curr > fc_2k)
                     {
                        Float64 fci_max  = m_StrandDesignTool.GetMaximumReleaseStrength();
                        Float64 fci = Min(fci_max, fci_curr+fc_2k);
                        LOG(_T("  Release strength was more than 2 ksi smaller than final, bump release as well"));
                        bool didchg = m_StrandDesignTool.UpdateReleaseStrength(fci, strength_result, pControllingParams->task, pControllingParams->stress_location);
                        if (didchg)
                        {
                           m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciIncreased);
                        }
                     }

                     LOG(_T("** Hail Mary to increase final concrete for tension succeeded - restart design"));
                     return;
                  }
               }
            }

            LOG(_T("Hail Mary - FAILED!! There is no way to satisfy tension limit unless outer loop can fix this problem"));
            m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::TooManyStrandsReqd);
            m_DesignerOutcome.AbortDesign();
            return;
         }
         else
         {
            LOG(_T("**** TOO MANY STRANDS REQUIRED ****, but let's try the max before we give up: ")<<npmax);
            Np = npmax;
         }
      }

      StrandIndexType np_min = m_StrandDesignTool.GetMinimumPermanentStrands();
      if (Np < np_min)
      {
         Np = np_min;
         LOG(_T("Number of strands computed is less than minimum set for Ultimate Moment. Setting to ")<<Np);
      }

      Np_old = m_StrandDesignTool.GetNumPermanentStrands();

      // Controller - can change number of strands if we are bifurcating
      StrandIndexType npchg;
      StrandDesignController::strUpdateResult updateResult = designController.DoUpdate(Np, Np_old, &npchg );
      LOG(_T("** StrandDesignController update result = ")<< updateResult <<_T(" Np = ")<<Np <<_T(" Npchg = ")<<npchg);
      Np = npchg;

      // set number of permanent strands
      if (m_StrandDesignTool.SetNumPermanentStrands(Np))
      {
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
      }
      else
      {
         LOG(_T("Error trying to set permanent strands - Abort Design"));
         m_DesignerOutcome.AbortDesign();
         return;
      }

      LOG(_T("Np = ") << Np_old << _T(" NpGuess = ") << Np);
      LOG(_T("NsGuess = ") << m_StrandDesignTool.GetNs());
      LOG(_T("NhGuess = ") << m_StrandDesignTool.GetNh());
      LOG(_T("NtGuess = ") << m_StrandDesignTool.GetNt());
      LOG(_T("** End of strand configuration trial # ") << cIter <<_T(", Tension controlled"));

      if (updateResult == StrandDesignController::struConverged)
      {
         // solution has converged - compute and save the minimum eccentricity that we can have with
         // Np and our allowable. This will be used later to limit strand adjustments in mid-zone
         // We know that Service III controlled because we are here:
         Float64 pps = m_StrandDesignTool.GetPrestressForceMidZone(pControllingParams->task.intervalIdx,poi);
         Float64 ecc_min = ComputeBottomCompressionEccentricity( pps, pControllingParams->fAllow, pControllingParams->fmax, Ag, Sbg);
         LOG(_T("Minimum eccentricity Required to control Bottom Tension  = ") << WBFL::Units::ConvertFromSysUnits(ecc_min, WBFL::Units::Measure::Inch) << _T(" in"));
         LOG(_T("Actual current eccentricity   = ") << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool.ComputeEccentricity(poi, pControllingParams->task.intervalIdx), WBFL::Units::Measure::Inch) << _T(" in"));
         m_StrandDesignTool.SetMinimumFinalMidZoneEccentricity(ecc_min);
         break;
      }
      else if (updateResult == StrandDesignController::struUpdateFailed)
      {
         LOG(_T("** Strand controller update failed - Number of strands could not be found"));
         m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::TooManyStrandsReqd);
         m_DesignerOutcome.AbortDesign();
         return;
      }


      cIter++;
   } while ( cIter < maxIter );

   if ( maxIter <= cIter )
   {
      LOG(_T("Maximum number of iterations was exceeded - aborting design ") << cIter);
      m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::MaxIterExceeded);
      m_DesignerOutcome.AbortDesign();
   }

   LOG(cIter << _T(" iterations were used"));

   LOG(_T(""));
   LOG(_T("Preliminary Design"));
   LOG(_T("Ns = ") << m_StrandDesignTool.GetNs() << _T(" PjS = ") << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool.GetPjackStraightStrands(),WBFL::Units::Measure::Kip) << _T(" Kip"));
   LOG(_T("Nh = ") << m_StrandDesignTool.GetNh() << _T(" PjH = ") << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool.GetPjackHarpedStrands(),WBFL::Units::Measure::Kip) << _T(" Kip"));
   LOG(_T("Nt = ") << m_StrandDesignTool.GetNt() << _T(" PjT = ") << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool.GetPjackTempStrands(),WBFL::Units::Measure::Kip) << _T(" Kip"));
   LOG(_T("** Preliminary Design Complete"));
   LOG(_T("==========================="));
   // Done
}

pgsPointOfInterest pgsDesigner2::GetControllingFinalMidZonePoi(const CSegmentKey& segmentKey) const
{
   // find location in mid-zone with max stress due to Service III tension
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ISpecification,pSpec);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   Float64 gl = pBridge->GetSegmentLength(segmentKey);
   Float64 lhp, rhp;
   m_StrandDesignTool.GetMidZoneBoundaries(&lhp, &rhp);

   Float64 left_limit = lhp;
   Float64 rgt_limit  = rhp;
   if ( IsEqual(lhp,rhp) )
   {
      left_limit = 0.4*gl;
      rgt_limit  = 0.6*gl;
   }

   const GDRCONFIG& config = m_StrandDesignTool.GetSegmentConfiguration();

   GET_IFACE(ILimitStateForces,pForces);
   PoiList vPoi;
   m_StrandDesignTool.GetDesignPoi(lastIntervalIdx, POI_ERECTED_SEGMENT, &vPoi);
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

   LOG(_T("Found controlling mid-zone final poi at ")<< WBFL::Units::ConvertFromSysUnits(max_poi.GetDistFromStart(),WBFL::Units::Measure::Feet) << _T(" ft") );

   ATLASSERT(found);
   return max_poi;
}

void pgsDesigner2::DesignEndZoneReleaseStrength(IProgress* pProgress) const
{
   const CSegmentKey& segmentKey = m_StrandDesignTool.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   LOG(_T(""));
   LOG(_T("Computing Release requirements at End-Zone - Assumes that harped strands have been raised to highest location or debonding is maximized before entering"));

   Float64 fc  = m_StrandDesignTool.GetConcreteStrength();
   Float64 fci = m_StrandDesignTool.GetReleaseStrength();
   LOG(_T("current f'c  = ") << WBFL::Units::ConvertFromSysUnits(fc,WBFL::Units::Measure::KSI) << _T(" KSI") );
   LOG(_T("current f'ci = ") << WBFL::Units::ConvertFromSysUnits(fci,WBFL::Units::Measure::KSI) << _T(" KSI") );

   const GDRCONFIG& config = m_StrandDesignTool.GetSegmentConfiguration();

   GET_IFACE(ILimitStateForces,pForces);
   GET_IFACE(IPretensionStresses, pPrestress);
   GET_IFACE(IProductForces,pProdForces);

   pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   PoiList vPOI;
   m_StrandDesignTool.GetDesignPoi(releaseIntervalIdx, POI_PSXFER, &vPOI);
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

      Float64 fTopPretension, fBotPretension;
      pPrestress->GetDesignStress(releaseIntervalIdx, poi, pgsTypes::TopGirder, pgsTypes::BottomGirder, config, false, pgsTypes::ServiceI, &fTopPretension, &fBotPretension);

      Float64 max = maxe + fTopPretension;
      Float64 min = mine + fBotPretension;

      // save max'd stress and corresponding poi
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

   LOG(_T("Controlling Stress at Release , top, tension psxfer  = ")           << WBFL::Units::ConvertFromSysUnits(ftop,WBFL::Units::Measure::KSI) << _T(" KSI at ")<<WBFL::Units::ConvertFromSysUnits(top_poi.GetDistFromStart(),WBFL::Units::Measure::Feet) << _T(" ft") );
   LOG(_T("Controlling Stress at Release , bottom, compression psxfer = ")     << WBFL::Units::ConvertFromSysUnits(fbot,WBFL::Units::Measure::KSI) << _T(" KSI at ")<<WBFL::Units::ConvertFromSysUnits(bot_poi.GetDistFromStart(),WBFL::Units::Measure::Feet) << _T(" ft"));
   LOG(_T("External Stress Demand at Release , top, tension psxfer  = ")       << WBFL::Units::ConvertFromSysUnits(fetop,WBFL::Units::Measure::KSI) << _T(" KSI") );
   LOG(_T("External Stress Demand at Release , bottom, compression psxfer = ") << WBFL::Units::ConvertFromSysUnits(febot,WBFL::Units::Measure::KSI) << _T(" KSI") );

   // First crack is to design concrete release strength for harped strands raised to top.
   // No use going further if we can't
   LOG(_T("Try Designing EndZone Release Strength at Initial Condition") );
   DesignConcreteRelease(ftop, fbot);
}

void pgsDesigner2::DesignEndZoneHarpingAdjustment(const arDesignOptions& options, IProgress* pProgress) const
{
   // This function attempts to adjust harping at the ends of the girder to either minimize the number of harped strands,
   // or lower the harped strands in order to maximize constructibility.
   const CSegmentKey& segmentKey = m_StrandDesignTool.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   LOG(_T("*** Refine harped design adjustments at end zone"));
   LOG(_T("Computing adjustment requirements at End-Zone - Assumes that harped strands have been raised to highest location before entering"));

   GDRCONFIG config = m_StrandDesignTool.GetSegmentConfiguration();

   // Get eccentricity requirements for release
   pgsPointOfInterest top_poi, bot_poi;
   Float64 ecc_tens, ecc_comp;
   Float64 fe_top, fe_bot;
   LOG(_T("** Compute allowable eccentricity for Release...") );
   GetControllingHarpedEccentricity(releaseIntervalIdx, config, &top_poi, &bot_poi, &ecc_tens, &ecc_comp, &fe_top, &fe_bot, pProgress);

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nh = m_StrandDesignTool.GetNh();

   if (m_StrandDesignTool.GetOriginalStrandFillType() == ftMinimizeHarping)
   {
      // try to trade harped to straight and, if necessary, lower strands to achieve eccentricity.
      // This is WSDOT's method, and we only look at release conditions here
      if (::IsLE(ecc_tens, ecc_comp))
      {
         LOG(_T("Tension Controls")); 
      }
      else
      {
         LOG(_T("Compression Controls"));
      }

      Float64 ecc_control = ecc_tens < ecc_comp ? ecc_tens : ecc_comp;
      const pgsPointOfInterest& poi_control = ecc_tens < ecc_comp ? top_poi : bot_poi;

      StrandIndexType Ns = m_StrandDesignTool.GetNs();
      StrandIndexType nh_reqd, ns_reqd;

      LOG(_T("Try to raise end eccentricity by trading harped to straight and lowering ends"));
      if (m_StrandDesignTool.ComputeMinHarpedForEndZoneEccentricity(poi_control, ecc_control, releaseIntervalIdx, &ns_reqd, &nh_reqd))
      {
         // number of straight/harped were changed. Set them
         LOG(_T("Number of Straight/Harped were changed from ")<<Ns<<_T("/")<<Nh<<_T(" to ")<<ns_reqd<<_T("/")<<nh_reqd);
         m_StrandDesignTool.SetNumStraightHarped(ns_reqd, nh_reqd);

         m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::RetainStrandProportioning);
      }
   }
   else
   {
      // See if we can adjust end harped strands downward and do it if we can
      Float64 offset_inc = m_StrandDesignTool.GetHarpedEndOffsetIncrement(pStrandGeom);

      if (0.0 <= offset_inc && 0 < Nh && !options.doForceHarpedStrandsStraight )
      {
         LOG(_T("Harped strands can be adjusted downward at ends - See how low can we go...") );
         // The older version of this algorithm only adjusted for release. Later (Oct 2019), we realized that the adjustment must also 
         // consider the Bridge Site 1 (wet slab) condition so we don't lower the strands
         // too far and cause the final concrete strength to be too high

         // Get eccentricity requirements for BSS1 if considered
         IntervalIndexType deckCastingIntervalIdx = pIntervals->GetFirstCastDeckInterval();

         GET_IFACE(IAllowableConcreteStress,pAllowable);
         if (pAllowable->CheckTemporaryStresses() && deckCastingIntervalIdx != INVALID_INDEX)
         {
            LOG(_T("** Need to compare allowable eccentricity for BSS2...") );

            pgsPointOfInterest bss1_top_poi, bss1_bot_poi;
            Float64 bss1_ecc_tens, bss1_ecc_comp;
            Float64 bss1_fe_top, bss1_fe_bot;
            GetControllingHarpedEccentricity(deckCastingIntervalIdx, config, &bss1_top_poi, &bss1_bot_poi, &bss1_ecc_tens, &bss1_ecc_comp, &bss1_fe_top, &bss1_fe_bot, pProgress);

            // bss1 only considers bottom compression
            if (bss1_ecc_comp < ecc_comp)
            {
               LOG(_T("BSS1 eccentricity controls. We can only lower strands so far without increasing final strength requirements") );
               bot_poi  = bss1_bot_poi;
               ecc_comp = bss1_ecc_comp;
               fe_bot   = bss1_fe_bot;
            }
         }
         else
         {
            LOG(_T("Don't need to consider BSS1 according to spec entry. Just use release requirements") );
            // ...already computed above
         }

         // compute harped offset required to achieve this ecc
         Float64 off_reqd;

         // smallest ecc controls
         if( ::IsLE(ecc_tens,ecc_comp))
         {
            LOG(_T("Tension Controls, ecc = ") << WBFL::Units::ConvertFromSysUnits(ecc_tens, WBFL::Units::Measure::Inch) << _T(" in"));
            off_reqd = m_StrandDesignTool.ComputeEndOffsetForEccentricity(top_poi, ecc_tens);
         }
         else
         {
            LOG(_T("Compression Controls, ecc = ") << WBFL::Units::ConvertFromSysUnits(ecc_comp, WBFL::Units::Measure::Inch) << _T(" in"));
            off_reqd = m_StrandDesignTool.ComputeEndOffsetForEccentricity(bot_poi, ecc_comp);
         }

         LOG(_T("Harped End offset required to achieve controlling Eccentricity (raw)   = ") << WBFL::Units::ConvertFromSysUnits(off_reqd, WBFL::Units::Measure::Inch) << _T(" in"));
         // round to increment
         off_reqd = CeilOff(off_reqd, offset_inc);
         LOG(_T("Harped End offset required to achieve controlling Eccentricity (rounded)  = ") << WBFL::Units::ConvertFromSysUnits(off_reqd, WBFL::Units::Measure::Inch) << _T(" in"));

         // Attempt to set our offset, this may be lowered to the highest allowed location 
         // if it is out of bounds
         m_StrandDesignTool.SetHarpStrandOffsetEnd(pgsTypes::metStart,off_reqd);
         m_StrandDesignTool.SetHarpStrandOffsetEnd(pgsTypes::metEnd,  off_reqd);

         m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
      }
      else
      {
         LOG((0 < Nh ? _T("Cannot adjust harped strands due to user input"):_T("There are no harped strands to adjust")));
      }
   }

   CHECK_PROGRESS;

   config = m_StrandDesignTool.GetSegmentConfiguration();

   LOG(_T("New eccentricity is ") << WBFL::Units::ConvertFromSysUnits( pStrandGeom->GetEccentricity(releaseIntervalIdx,ecc_tens<ecc_comp?top_poi:bot_poi, true, &config).Y(), WBFL::Units::Measure::Inch) << _T(" in"));

   GET_IFACE(IPretensionStresses, pPrestress);

   Float64 fTopPs, fBotPs;
   fTopPs = pPrestress->GetDesignStress(releaseIntervalIdx,top_poi,pgsTypes::TopGirder,config,false, pgsTypes::ServiceI);
   fBotPs = pPrestress->GetDesignStress(releaseIntervalIdx,bot_poi,pgsTypes::BottomGirder,config,false, pgsTypes::ServiceI);

   Float64 ftop = fe_top + fTopPs;
   Float64 fbot = fe_bot + fBotPs;

   LOG(_T("After Adjustment, Controlling Stress at Release , Top, Tension        = ") << WBFL::Units::ConvertFromSysUnits(ftop,WBFL::Units::Measure::KSI) << _T(" KSI") );
   LOG(_T("After Adjustment, Controlling Stress at Release , Bottom, Compression = ") << WBFL::Units::ConvertFromSysUnits(fbot,WBFL::Units::Measure::KSI) << _T(" KSI") );

   // Recompute required release strength
   DesignConcreteRelease(ftop, fbot);

   // Done
}

void pgsDesigner2::GetControllingHarpedEccentricity(IntervalIndexType interval, const GDRCONFIG& config, 
                                                    pgsPointOfInterest* pTopPoi,pgsPointOfInterest* pBotPoi, 
                                                    Float64* pEccTens, Float64* pEccComp, Float64* pFeTop, Float64* pFeBot, 
                                                    IProgress* pProgress) const
{
   GET_IFACE(ILimitStateForces,pForces);
   GET_IFACE(IPretensionStresses, pPrestress);
   PoiList vPOI;
   m_StrandDesignTool.GetDesignPoi(interval, POI_PSXFER, &vPOI);
   ATLASSERT(!vPOI.empty());

   GET_IFACE(IProductForces,pProdForces);
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

      Float64 fTopPretension, fBotPretension;
      fTopPretension = pPrestress->GetDesignStress(interval, poi, pgsTypes::TopGirder, config, false, pgsTypes::ServiceI);
      fBotPretension = pPrestress->GetDesignStress(interval, poi, pgsTypes::BottomGirder, config, false, pgsTypes::ServiceI);

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

   GET_IFACE(IIntervals,pIntervals);
   LOG(_T("Controlling Stress at ") << pIntervals->GetDescription(interval) << _T(", top, tension psxfer  = ") << WBFL::Units::ConvertFromSysUnits(ftop,WBFL::Units::Measure::KSI) << _T(" KSI") );
   LOG(_T("Controlling Stress at ") << pIntervals->GetDescription(interval) << _T(" , bottom, compression psxfer = ") << WBFL::Units::ConvertFromSysUnits(fbot,WBFL::Units::Measure::KSI) << _T(" KSI") );
   LOG(_T("External Stress Demand at ") << pIntervals->GetDescription(interval) << _T(" , top, tension psxfer  = ") << WBFL::Units::ConvertFromSysUnits(*pFeTop,WBFL::Units::Measure::KSI) << _T(" KSI") );
   LOG(_T("External Stress Demand at ") << pIntervals->GetDescription(interval) << _T(" , bottom, compression psxfer = ") << WBFL::Units::ConvertFromSysUnits(*pFeBot,WBFL::Units::Measure::KSI) << _T(" KSI") );

   // Get the section properties of the girder
   GET_IFACE(ISectionProperties,pSectProp);
   Float64 Ag  = pSectProp->GetAg(interval,vPOI[0]);
   Float64 Stg = pSectProp->GetS(interval,vPOI[0],pgsTypes::TopGirder);
   Float64 Sbg = pSectProp->GetS(interval,vPOI[0],pgsTypes::BottomGirder);
   LOG(_T("Ag  = ") << WBFL::Units::ConvertFromSysUnits(Ag, WBFL::Units::Measure::Inch2) << _T(" in^2"));
   LOG(_T("Stg = ") << WBFL::Units::ConvertFromSysUnits(Stg,WBFL::Units::Measure::Inch3) << _T(" in^3"));
   LOG(_T("Sbg = ") << WBFL::Units::ConvertFromSysUnits(Sbg,WBFL::Units::Measure::Inch3) << _T(" in^3"));

   // compute eccentricity to control top tension
   const CSegmentKey& segmentKey = m_StrandDesignTool.GetSegmentKey();

   GET_IFACE(IAllowableConcreteStress,pAllowable);

   Float64 fc;
   Float64 allowable_tension;
   Float64 allowable_compression;
   if (pIntervals->GetPrestressReleaseInterval(segmentKey) == interval)
   {
      ConcStrengthResultType conc_res;
      fc = m_StrandDesignTool.GetReleaseStrength(&conc_res);
      LOG(_T("current f'ci  = ") << WBFL::Units::ConvertFromSysUnits(fc, WBFL::Units::Measure::KSI) << _T(" KSI "));

      allowable_tension     = pAllowable->GetSegmentAllowableTensionStress(    vPOI[0],StressCheckTask(interval,pgsTypes::ServiceI,pgsTypes::Tension),fc,conc_res==ConcSuccessWithRebar?true:false);
      allowable_compression = pAllowable->GetSegmentAllowableCompressionStress(vPOI[0],StressCheckTask(interval,pgsTypes::ServiceI,pgsTypes::Compression),fc);
      LOG(_T("Allowable tensile stress     = ") << WBFL::Units::ConvertFromSysUnits(allowable_tension,WBFL::Units::Measure::KSI) << _T(" KSI") );
      LOG(_T("Allowable compressive stress = ") << WBFL::Units::ConvertFromSysUnits(allowable_compression,WBFL::Units::Measure::KSI) << _T(" KSI") );
   }
   else
   {
      fc = m_StrandDesignTool.GetConcreteStrength();
      LOG(_T("current f'c  = ") << WBFL::Units::ConvertFromSysUnits(fc, WBFL::Units::Measure::KSI) << _T(" KSI "));

      allowable_tension     = pAllowable->GetSegmentAllowableTensionStress(    vPOI[0],StressCheckTask(interval,pgsTypes::ServiceI,pgsTypes::Tension),fc,false);
      allowable_compression = pAllowable->GetSegmentAllowableCompressionStress(vPOI[0],StressCheckTask(interval,pgsTypes::ServiceI,pgsTypes::Compression),fc);
      LOG(_T("Allowable tensile stress     = ") << WBFL::Units::ConvertFromSysUnits(allowable_tension,WBFL::Units::Measure::KSI) << _T(" KSI") );
      LOG(_T("Allowable compressive stress = ") << WBFL::Units::ConvertFromSysUnits(allowable_compression,WBFL::Units::Measure::KSI) << _T(" KSI") );
   }

   // ecc's required to control stresses
   Float64 top_pps  = m_StrandDesignTool.GetPrestressForceAtLifting(config,*pTopPoi);
   LOG(_T("Total Prestress Force for top location: P  = ") << WBFL::Units::ConvertFromSysUnits(top_pps, WBFL::Units::Measure::Kip) << _T(" kip"));

   *pEccTens = ComputeTopTensionEccentricity( top_pps, allowable_tension, *pFeTop, Ag, Stg);
   LOG(_T("Eccentricity Required to control Top Tension   = ") << WBFL::Units::ConvertFromSysUnits(*pEccTens, WBFL::Units::Measure::Inch) << _T(" in"));

   // ecc to control bottom compression
   Float64 bot_pps  = m_StrandDesignTool.GetPrestressForceAtLifting(config,*pBotPoi);
   LOG(_T("Total Prestress Force for bottom location: P  = ") << WBFL::Units::ConvertFromSysUnits(bot_pps, WBFL::Units::Measure::Kip) << _T(" kip"));

   *pEccComp = ComputeBottomCompressionEccentricity( bot_pps, allowable_compression, *pFeBot, Ag, Sbg);
   LOG(_T("Eccentricity Required to control Bottom Compression   = ") << WBFL::Units::ConvertFromSysUnits(*pEccComp, WBFL::Units::Measure::Inch) << _T(" in"));
}

bool pgsDesigner2::CheckLiftingStressDesign(const CSegmentKey& segmentKey,const GDRCONFIG& config) const
{
   WBFL::Stability::LiftingCheckArtifact artifact;

   HANDLINGCONFIG lift_config;
   lift_config.bIgnoreGirderConfig = false;
   lift_config.GdrConfig = config;
   lift_config.LeftOverhang = m_StrandDesignTool.GetLeftLiftingLocation();
   lift_config.RightOverhang = m_StrandDesignTool.GetRightLiftingLocation();

   pgsGirderLiftingChecker checker(m_pBroker,m_StatusGroupID);
   ISegmentLiftingDesignPointsOfInterest* pPoiLd = dynamic_cast<ISegmentLiftingDesignPointsOfInterest*>(&m_StrandDesignTool);

   checker.AnalyzeLifting(segmentKey,lift_config,pPoiLd,&artifact);

   return artifact.PassedStressCheck();
}

std::vector<DebondLevelType> pgsDesigner2::DesignEndZoneReleaseDebonding(IProgress* pProgress,bool bAbortOnFail) const
{
   const CSegmentKey& segmentKey = m_StrandDesignTool.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   LOG(_T("Refine Debonded design by computing debond demand levels for release condition at End-Zone"));

   // We also get into this function for fully debonded designs, no use debonding if so
   if ( !m_StrandDesignTool.IsDesignDebonding() )
   {
      LOG(_T("Fully bonded design - no need to compute debond levels "));
      std::vector<DebondLevelType> levels;
      levels.assign((long)0,0);
      return levels;
   }

   // compute eccentricity to control top tension
   Float64 fc  = m_StrandDesignTool.GetConcreteStrength();
   ConcStrengthResultType rebar_reqd;
   Float64 fci = m_StrandDesignTool.GetReleaseStrength(&rebar_reqd);
   LOG(_T("current f'c  = ") << WBFL::Units::ConvertFromSysUnits(fc,WBFL::Units::Measure::KSI) << _T(" KSI "));
   LOG(_T("current f'ci = ") << WBFL::Units::ConvertFromSysUnits(fci,WBFL::Units::Measure::KSI) << _T(" KSI") );

   GET_IFACE(IPointOfInterest,pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT, &vPoi);
   ASSERT( vPoi.size() == 1 );
   pgsPointOfInterest midPOI(vPoi.front());

   GET_IFACE(IAllowableConcreteStress,pAllowable);
   Float64 allowable_tension     = pAllowable->GetSegmentAllowableTensionStress(    midPOI,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension),fci,rebar_reqd==ConcSuccessWithRebar?true:false);
   Float64 allowable_compression = pAllowable->GetSegmentAllowableCompressionStress(midPOI,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression),fci);
   LOG(_T("Allowable tensile stress after Release     = ") << WBFL::Units::ConvertFromSysUnits(allowable_tension,WBFL::Units::Measure::KSI) << _T(" KSI")<<(rebar_reqd==ConcSuccessWithRebar ? _T(" min rebar was required for this strength"):_T(""))  );
   LOG(_T("Allowable compressive stress after Release = ") << WBFL::Units::ConvertFromSysUnits(allowable_compression,WBFL::Units::Measure::KSI) << _T(" KSI") );

   // We want to compute total debond demand, so bond all strands
   GDRCONFIG config = m_StrandDesignTool.GetSegmentConfiguration();
   config.PrestressConfig.Debond[pgsTypes::Straight].clear();

   StrandIndexType nperm = config.PrestressConfig.GetStrandCount(pgsTypes::Permanent);
   StrandIndexType ntemp = config.PrestressConfig.GetStrandCount(pgsTypes::Temporary);


   GET_IFACE(ILimitStateForces,pForces);
   GET_IFACE(IPretensionStresses, pPrestress);
   PoiList vPOI;
   m_StrandDesignTool.GetDesignPoiEndZone(releaseIntervalIdx, &vPOI);
   ATLASSERT(!vPOI.empty());

   GET_IFACE(IProductForces,pProdForces);
   pgsTypes::BridgeAnalysisType bat = pProdForces->GetBridgeAnalysisType(pgsTypes::Maximize);

   // Build stress demand
   GET_IFACE(IPretensionForce,pPrestressForce);
   std::vector<pgsStrandDesignTool::StressDemand> stress_demands;
   stress_demands.reserve(vPOI.size());

   for ( const pgsPointOfInterest& poi : vPOI)
   {
      Float64 fTopAppl,fBotAppl,bogus;
      pForces->GetStress(releaseIntervalIdx,pgsTypes::ServiceI,poi,bat,false,pgsTypes::TopGirder,   &bogus,&fTopAppl);
      pForces->GetStress(releaseIntervalIdx,pgsTypes::ServiceI,poi,bat,false,pgsTypes::BottomGirder,&fBotAppl,&bogus);

      Float64 fTopPretension, fBotPretension;
      fTopPretension = pPrestress->GetDesignStress(releaseIntervalIdx,poi,pgsTypes::TopGirder,config,false, pgsTypes::ServiceI);
      fBotPretension = pPrestress->GetDesignStress(releaseIntervalIdx,poi,pgsTypes::BottomGirder,config,false, pgsTypes::ServiceI);

      // demand stress with fully bonded straight strands
      Float64 fTop = fTopAppl + fTopPretension;
      Float64 fBot = fBotAppl + fBotPretension;

      Float64 strand_force = pPrestressForce->GetPrestressForcePerStrand(poi, pgsTypes::Permanent, releaseIntervalIdx, pgsTypes::End, &config );

      LOG(_T("Computing stresses at ")   <<WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart(),WBFL::Units::Measure::Feet) << _T(" ft"));
      LOG(_T("Applied Top stress    = ") << WBFL::Units::ConvertFromSysUnits(fTopAppl,WBFL::Units::Measure::KSI) << _T(" ksi. Prestress stress = ")<< WBFL::Units::ConvertFromSysUnits(fTopPretension,WBFL::Units::Measure::KSI) << _T(" ksi. Total stress = ")<< WBFL::Units::ConvertFromSysUnits(fTop,WBFL::Units::Measure::KSI) << _T(" ksi"));
      LOG(_T("Applied Bottom stress = ") << WBFL::Units::ConvertFromSysUnits(fBotAppl,WBFL::Units::Measure::KSI) << _T(" ksi. Prestress stress = ")<< WBFL::Units::ConvertFromSysUnits(fBotPretension,WBFL::Units::Measure::KSI) << _T(" ksi. Total stress = ")<< WBFL::Units::ConvertFromSysUnits(fBot,WBFL::Units::Measure::KSI) << _T(" ksi"));
      LOG(_T("Force per strand = ") << WBFL::Units::ConvertFromSysUnits(strand_force, WBFL::Units::Measure::Kip) << _T(" kip"));

      pgsStrandDesignTool::StressDemand demand;
      demand.m_Poi          = poi;
      demand.m_TopStress    = fTop;
      demand.m_BottomStress = fBot;
      demand.m_PrestressForcePerStrand = strand_force;

      stress_demands.push_back(demand);
   }

   // compute debond levels at each section from demand
   GET_IFACE(IStrandGeometry, pStrandGeom);
   auto cg = pStrandGeom->GetStrandCG(releaseIntervalIdx, midPOI, true, &config);
   std::vector<DebondLevelType> debond_levels;
   debond_levels = m_StrandDesignTool.ComputeDebondsForDemand(stress_demands, config, cg.Y(), releaseIntervalIdx, allowable_tension, allowable_compression);

   if (  debond_levels.empty() && bAbortOnFail )
   {
      ATLASSERT(false);
      LOG(_T("Debonding failed, this should not happen?"));

      m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::DebondDesignFailed);
      m_DesignerOutcome.AbortDesign();
   }

   return debond_levels;
}


void pgsDesigner2::DesignConcreteRelease(Float64 ftop, Float64 fbot) const
{
   const CSegmentKey& segmentKey = m_StrandDesignTool.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   LOG(_T("Entering DesignConcreteRelease"));
   LOG(_T("Total Stress at bottom = ") << WBFL::Units::ConvertFromSysUnits(fbot,WBFL::Units::Measure::KSI) << _T(" KSI") );
   LOG(_T("Total Stress at top    = ") << WBFL::Units::ConvertFromSysUnits(ftop,WBFL::Units::Measure::KSI) << _T(" KSI") );

   Float64 fci = m_StrandDesignTool.GetReleaseStrength();
   Float64 fc_old = m_StrandDesignTool.GetConcreteStrength();

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

      LOG(_T("F'ci to control tension at release is = ") << WBFL::Units::ConvertFromSysUnits(fc_tens,WBFL::Units::Measure::KSI) << _T(" KSI") );

      ConcStrengthResultType tens_success = m_StrandDesignTool.ComputeRequiredConcreteStrength(ftens,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension),&fc_tens);
      if ( ConcFailed == tens_success )
      {
         // Attempt to remedy by adding raised straight strands
         if ( m_StrandDesignTool.AddRaisedStraightStrands() )
         {
            // Attempt to add raised straight strands if this is an option. We could abort if the attempt
            // fails, but give bump 500 a chance if we go down in smoke.
            // If we are here, outer algorithm will restart.
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::RaisedStraightStrands);
            LOG(_T("Added Raised Straight Strands - Restart design with new strand configuration"));
            return;
         }
         else
         {
            LOG(_T("Could not find adequate release strength to control tension - Design Abort") );
            m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::ReleaseStrength);
            m_DesignerOutcome.AbortDesign();
            return;
         }
      }
      else
      {
         Float64 fci_old = m_StrandDesignTool.GetReleaseStrength();

         bool bFciUpdated = m_StrandDesignTool.UpdateReleaseStrength(fc_tens, tens_success, StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI, pgsTypes::Tension), tens_location);
         if ( bFciUpdated )
         {
            Float64 fci_new = m_StrandDesignTool.GetReleaseStrength();

            LOG(_T("Release Strength For tension Changed to ")  << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool.GetReleaseStrength(), WBFL::Units::Measure::KSI) << _T(" KSI"));
            m_DesignerOutcome.SetOutcome(fci_new> fci_old ? pgsDesignCodes::FciIncreased : pgsDesignCodes::FciDecreased);

            Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
            if ( !IsEqual(fc_new,fc_old) )
            {
               LOG(_T("Final Strength Also Increased to ")  << WBFL::Units::ConvertFromSysUnits(fc_new, WBFL::Units::Measure::KSI) << _T(" KSI"));
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

      LOG(_T("F'ci to control compression at release is = ") << WBFL::Units::ConvertFromSysUnits(fc_comp,WBFL::Units::Measure::KSI) << _T(" KSI") );

      ConcStrengthResultType success = m_StrandDesignTool.ComputeRequiredConcreteStrength(fcomp,StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression),&fc_comp);
      if ( ConcFailed == success )
      {
         if ( comp_location == pgsTypes::BottomGirder && m_StrandDesignTool.AddRaisedStraightStrands() )
         {
            // Attempt to add raised straight strands if this is an option. Slim chance for compression controlled
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::RaisedStraightStrands);
            LOG(_T("Added Raised Straight Strands for bottom compression - Restart design with new strand configuration"));
            return;
         }
         else
         {
            LOG(_T("Could not find adequate release strength to control compression - Design Abort") );
            m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::ReleaseStrength);
            m_DesignerOutcome.AbortDesign();
            return;
         }
      }
      else
      {
         Float64 fci_old = m_StrandDesignTool.GetReleaseStrength();
         bool bFciUpdated = m_StrandDesignTool.UpdateReleaseStrength(fc_comp, success, StressCheckTask(releaseIntervalIdx,pgsTypes::ServiceI, pgsTypes::Compression), comp_location);
         if ( bFciUpdated )
         {
           Float64 fci_new = m_StrandDesignTool.GetReleaseStrength();

            LOG(_T("Release Strength For compression Increased to ")  << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool.GetReleaseStrength(), WBFL::Units::Measure::KSI) << _T(" KSI"));
            m_DesignerOutcome.SetOutcome(fci_new> fci_old ? pgsDesignCodes::FciIncreased : pgsDesignCodes::FciDecreased);

            Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
            if (fc_new!=fc_old)
            {
               LOG(_T("Final Strength Also Increased to ")  << WBFL::Units::ConvertFromSysUnits(fc_new, WBFL::Units::Measure::KSI) << _T(" KSI"));
               m_DesignerOutcome.SetOutcome(fc_new> fc_old ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);
            }
         }
      }

   }

   LOG(_T("Exiting DesignConcreteRelease"));
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

void pgsDesigner2::DesignForLiftingHarping(const arDesignOptions& options, bool bProportioningStrands,IProgress* pProgress) const
{
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


   LOG(_T(""));
   LOG(_T("DESIGNING FOR LIFTING"));
   LOG(_T(""));
   m_StrandDesignTool.DumpDesignParameters();

   // get some initial data to make function calls a little easier to read
   const CSegmentKey& segmentKey = m_StrandDesignTool.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   pgsGirderLiftingChecker checker(m_pBroker,m_StatusGroupID); // this guy can do the stability design!

   GDRCONFIG config = m_StrandDesignTool.GetSegmentConfiguration();
   if ( bProportioningStrands )
   {
      // if this is the first design for lifting, look at the lifting without temporary strands case
      // to get the optimum strand configuration
      LOG(_T("Phase 1 Lifting Design - Design for Lifting without Temporary Strands"));
      LOG(_T("Determine straight/harped strands proportions"));
      LOG(_T(""));
      LOG(_T("Removing temporary strands for lifting analysis"));
      config.PrestressConfig.ClearStrandFill(pgsTypes::Temporary);
   }
#if defined ENABLE_LOGGING
   else
   {
      LOG(_T("Phase 2 Lifting Design - Design for Lifting with Temporary Strands"));
      LOG(_T("Determine lifting locations and release strength requirements"));
   }
#endif

   // Do a stability based design for lifting. this will locate the lift point locations required
   // for stability
   // Designer manages it's own POIs
   ISegmentLiftingDesignPointsOfInterest* pPoiLd = dynamic_cast<ISegmentLiftingDesignPointsOfInterest*>(&m_StrandDesignTool);

   HANDLINGCONFIG liftConfig;
   liftConfig.bIgnoreGirderConfig = false;
   liftConfig.GdrConfig = config;
   WBFL::Stability::LiftingCheckArtifact artifact;
   const WBFL::Stability::LiftingStabilityProblem* pStabilityProblem;
   pgsDesignCodes::OutcomeType result = checker.DesignLifting(segmentKey,liftConfig,pPoiLd,&artifact,&pStabilityProblem,LOGGER);
   SectionFinder::pStabilityProblem = pStabilityProblem;

#if defined ENABLE_LOGGING
   LOG(_T("-- Dump of Lifting Artifact After Design --"));
   DumpLiftingArtifact(pStabilityProblem,artifact,LOGGER);
   LOG(_T("-- End Dump of Lifting Artifact --"));
#endif

   m_StrandDesignTool.SetLiftingLocations(liftConfig.LeftOverhang,liftConfig.RightOverhang);

   CHECK_PROGRESS;

   m_DesignerOutcome.SetOutcome(result);
   if ( m_DesignerOutcome.WasDesignAborted() )
   {
      return;
   }

   const WBFL::Stability::LiftingResults& liftingResults = artifact.GetLiftingResults();

   // Check to see if the girder is stable for lifting
   GET_IFACE(ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   Float64 FScr    = liftingResults.FScrMin;
   Float64 FScrMin = pSegmentLiftingSpecCriteria->GetLiftingCrackingFs();
   LOG(_T("FScr = ") << FScr);
   LOG(_T(""));
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
         LOG(_T("Cannot find a pick point to satisfy FScr"));
         LOG(_T("Temporary strands required"));
         LOG(_T("Move on to Shipping Design"));
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
         LOG(_T("Cannot find a pick point to satisfy FScr"));
         LOG(_T("Additional temporary strands required"));
         if ( m_StrandDesignTool.AddTempStrands() )
         {
            LOG(_T("Temporary strands added"));
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::LiftingConfigChanged);
         }
         else
         {
            // couldn't add temporary strands (girder probably doesn't support them or there isn't any room)
            LOG(_T("Tweaking straight/harped strand proportion")); // we are going to loose the design optimization, but it is better to get a design
            if ( m_StrandDesignTool.SwapStraightForHarped() )
            {
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::LiftingConfigChanged);
            }
            else
            {
               ATLASSERT(false); // need to add temporary strands
               m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::GirderLiftingStability);
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

      GET_IFACE(ISectionProperties,pSectProp);
      GET_IFACE(IPointOfInterest, pPoi);

      // Adjust the proportions of the straight and harped strands such the stress at the harp point
      // is matched by the stress at the lift point or the point of prestress transfer (which ever controls)
      
      ATLASSERT( m_StrandDesignTool.IsDesignHarping() );

      LOG(_T("--------------------------------------------------------------------------------------------------------------------"));
      LOG(_T("Attempt to reduce and lower harped strands for lifting condition. Use lifting points, or transfer lengths as controlling locations"));

      // get controlling stress at xfer/lift point
      Float64 fbot, bot_loc, ftop, top_loc;
      GetEndZoneMinMaxRawStresses(segmentKey,liftingResults,liftConfig,&ftop, &fbot, &top_loc, &bot_loc);
      LOG(_T("Max applied top stress at lifting point or transfer location    = ") << WBFL::Units::ConvertFromSysUnits(ftop,WBFL::Units::Measure::KSI) << _T(" KSI at ")<< WBFL::Units::ConvertFromSysUnits(top_loc,WBFL::Units::Measure::Feet) << _T(" ft"));
      LOG(_T("Max applied bottom stress at lifting point or transfer location = ") << WBFL::Units::ConvertFromSysUnits(fbot,WBFL::Units::Measure::KSI) << _T(" KSI at ")<< WBFL::Units::ConvertFromSysUnits(bot_loc,WBFL::Units::Measure::Feet) << _T(" ft"));
      
      // get top and bottom stresses at harp points
      PoiList vPoi;
      m_StrandDesignTool.GetPointsOfInterest(segmentKey, POI_HARPINGPOINT, &vPoi);
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

      LOG(_T("Computing eccentricity required to make stress at lift/xfer point approx equal to stress at hp"));
      // POIs for the current design
      pgsPointOfInterest tpoi(m_StrandDesignTool.GetPointOfInterest(segmentKey,top_loc));
      pgsPointOfInterest bpoi(m_StrandDesignTool.GetPointOfInterest(segmentKey,bot_loc));
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
      LOG(_T("Agt = ") << WBFL::Units::ConvertFromSysUnits(Agt, WBFL::Units::Measure::Inch2) << _T(" in^2"));
      LOG(_T("Agb = ") << WBFL::Units::ConvertFromSysUnits(Agb, WBFL::Units::Measure::Inch2) << _T(" in^2"));
      LOG(_T("Stg = ") << WBFL::Units::ConvertFromSysUnits(Stg, WBFL::Units::Measure::Inch3) << _T(" in^3"));
      LOG(_T("Sbg = ") << WBFL::Units::ConvertFromSysUnits(Sbg, WBFL::Units::Measure::Inch3) << _T(" in^3"));

      Float64 P_for_top = m_StrandDesignTool.GetPrestressForceAtLifting(config,tpoi);
      Float64 P_for_bot;
      if ( IsEqual(top_loc,bot_loc) )
      {
         P_for_bot = P_for_top;
      }
      else
      {
         P_for_bot = m_StrandDesignTool.GetPrestressForceAtLifting(config,bpoi);
      }

      
      LOG(_T("Total Prestress Force for top location: P     = ") << WBFL::Units::ConvertFromSysUnits(P_for_top, WBFL::Units::Measure::Kip) << _T(" kip"));

      // ecc's required to match stresses at harp point
      Float64 ecc_tens = compute_required_eccentricity(P_for_top,Agt,Stg,ftop,fHpMax);
      LOG(_T("Eccentricity Required to control Top Tension  = ") << WBFL::Units::ConvertFromSysUnits(ecc_tens, WBFL::Units::Measure::Inch) << _T(" in"));
      LOG(_T("Total Prestress Force for bottom location: P          = ") << WBFL::Units::ConvertFromSysUnits(P_for_bot, WBFL::Units::Measure::Kip) << _T(" kip"));

      // Note that the _T("exact") way to do this would be to iterate on eccentricity because prestress force is dependent on strand
      // slope, which is dependent on end strand locations. But, so far, no problems????
      Float64 ecc_comp = compute_required_eccentricity(P_for_bot,Agb,Sbg,fbot,fHpMin);
      LOG(_T("Eccentricity Required to control Bottom Compression   = ") << WBFL::Units::ConvertFromSysUnits(ecc_comp, WBFL::Units::Measure::Inch) << _T(" in"));

#if defined ENABLE_LOGGING
      if( ::IsLE(ecc_tens,ecc_comp))
      {
         LOG(_T("Tension Controls")); 
      }
      else
      {
         LOG(_T("Compression Controls"));
      }
#endif

      // try to trade harped to straight to achieve required eccentricity
      Float64 required_eccentricity = Min(ecc_tens,ecc_comp);
      const pgsPointOfInterest& poi_control = ecc_tens < ecc_comp ? tpoi : bpoi;

      StrandIndexType Ns = m_StrandDesignTool.GetNs();
      StrandIndexType Nh = m_StrandDesignTool.GetNh();

      StrandIndexType nh_reqd, ns_reqd;

      // At this point, it is assumed that end strands are raised as high as possible
      // See if our target is lower (bigger) than the current.
      Float64 curr_ecc = m_StrandDesignTool.ComputeEccentricity(poi_control,liftSegmentIntervalIdx);
      LOG(_T("Eccentricity for current number of strands = ")<< WBFL::Units::ConvertFromSysUnits(curr_ecc, WBFL::Units::Measure::Inch) << _T(" in"));
      if (curr_ecc <= required_eccentricity) // greater means the CG of prestress force must be lower in the section
      {
         if (m_StrandDesignTool.GetOriginalStrandFillType() == ftMinimizeHarping)
         {
            LOG(_T("Try to increase end eccentricity by trading harped to straight"));
            if (m_StrandDesignTool.ComputeMinHarpedForEndZoneEccentricity(poi_control, required_eccentricity, liftSegmentIntervalIdx, &ns_reqd, &nh_reqd))
            {
               // number of straight/harped were changed. Set them
               LOG(_T("Number of Straight/Harped were changed from ")<<Ns<<_T("/")<<Nh<<_T(" to ")<<ns_reqd<<_T("/")<<nh_reqd);
               m_StrandDesignTool.SetNumStraightHarped(ns_reqd, nh_reqd);

               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::RetainStrandProportioning);
            }
            else
            {
               // Can't adjust strands so adjust concrete strength - this is what phase 2 does
               bProportioningStrands = false; // causes phase 2 design below
            }
         }
         else
         {
            // See if we can lower end pattern
            GET_IFACE(IStrandGeometry,pStrandGeom);
            Float64 offset_inc = m_StrandDesignTool.GetHarpedEndOffsetIncrement(pStrandGeom);
            if ( 0.0 <= offset_inc && !options.doForceHarpedStrandsStraight)
            {
               LOG(_T("Try to raise end eccentricity by lowering harped strands at ends"));
               Float64 off_reqd = m_StrandDesignTool.ComputeEndOffsetForEccentricity(poi_control, required_eccentricity);

               // round to increment
               LOG(_T("Harped End offset required to achieve controlling Eccentricity (raw)   = ") << WBFL::Units::ConvertFromSysUnits(off_reqd, WBFL::Units::Measure::Inch) << _T(" in"));
               off_reqd = CeilOff(off_reqd, offset_inc);
               LOG(_T("Harped End offset required to achieve controlling Eccentricity (rounded)  = ") << WBFL::Units::ConvertFromSysUnits(off_reqd, WBFL::Units::Measure::Inch) << _T(" in"));

               // Attempt to set our offset, this may be lowered to the highest allowed location 
               // if it is out of bounds
               m_StrandDesignTool.SetHarpStrandOffsetEnd(pgsTypes::metStart,off_reqd);
               m_StrandDesignTool.SetHarpStrandOffsetEnd(pgsTypes::metEnd,  off_reqd);

               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
               LOG(_T("New Eccentricity  = ") << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool.ComputeEccentricity(poi_control,liftSegmentIntervalIdx), WBFL::Units::Measure::Inch) << _T(" in"));
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

      // The lifting points required for stability have already been determined above. Now we have to
      // find the release strength required to satisfy the allowable stress requirements

      // get the current value of fc in case it changes. f'c will change if the required f'ci > f'c
      // f'c will be made equal to f'ci
      Float64 fc_old = m_StrandDesignTool.GetConcreteStrength();


      // go to the artifact to get the required release strength to satisfy the compression and
      // tension criteria
      Float64 fci_comp = artifact.RequiredFcCompression();
      Float64 fci_tens = artifact.RequiredFcTensionWithoutRebar();
      Float64 fci_tens_wrebar = artifact.RequiredFcTensionWithRebar();

      // if there isn't a concrete strength that will make the tension limits work,
      // get the heck outta here!
      if ( fci_tens < 0 && fci_tens_wrebar < 0)
      {
         // there isn't a concrete strength that will work (because of tension limit)
         LOG(_T("There is no concrete strength that will work for lifting after shipping design - Tension controls - FAILED"));
         m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::GirderLiftingConcreteStrength);
         m_DesignerOutcome.AbortDesign();
         return; // bye
      }

      // we've got viable concrete strengths
      LOG(_T("Lifting Results : New f'ci (unrounded) comp = ") << WBFL::Units::ConvertFromSysUnits(fci_comp,WBFL::Units::Measure::KSI) << _T(" ksi, tension = ") << WBFL::Units::ConvertFromSysUnits(fci_tens,WBFL::Units::Measure::KSI) << _T(" ksi") << _T(" Pick Point = ") << WBFL::Units::ConvertFromSysUnits(liftConfig.LeftOverhang,WBFL::Units::Measure::Feet) << _T(" ft"));

      ConcStrengthResultType rebar_reqd = (fci_tens<0) ? ConcSuccessWithRebar : ConcSuccess;

      // get the controlling value
      Float64 fci_required = Max(fci_tens,fci_tens_wrebar,fci_comp);

      // get the maximum allowable f'ci
      Float64 fci_max = m_StrandDesignTool.GetMaximumReleaseStrength();
      if( fci_max < fci_required)
      {
         // required strength is greater than max...
         // sometimes, if we are right at the limit the max value will work... give it a try

         LOG(_T("f'ci max = ") << WBFL::Units::ConvertFromSysUnits(fci_max,WBFL::Units::Measure::KSI) << _T(" KSI"));
         LOG(_T("f'ci cannot be greater than max. See if we can use max for one last attempt"));

         Float64 fci_curr = m_StrandDesignTool.GetReleaseStrength();

         if ( !IsEqual(fci_curr,fci_max) )
         {
            LOG(_T("Set to max for one more attempt"));
            fci_tens = Min(fci_tens, fci_max);
            fci_comp = Min(fci_comp, fci_max);
         }
         else
         {
            LOG(_T("Fci max already used.There is no concrete strength that will work for lifting after shipping design - time to abort"));
            m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::GirderLiftingConcreteStrength);
            m_DesignerOutcome.AbortDesign();
            return;
         }
      }

      // Set the concrete strength. Set it once for tension and once for compression. The controlling value will stick.
      Float64 fci_old = m_StrandDesignTool.GetReleaseStrength();

      bool bFciTensionUpdated     = m_StrandDesignTool.UpdateReleaseStrength(fci_tens,rebar_reqd,StressCheckTask(liftSegmentIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension),pgsTypes::TopGirder);
      bool bFciCompressionUpdated = m_StrandDesignTool.UpdateReleaseStrength(fci_comp,rebar_reqd, StressCheckTask(liftSegmentIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression),pgsTypes::BottomGirder);

      if ( bFciTensionUpdated || bFciCompressionUpdated )
      {
         Float64 fci_new = m_StrandDesignTool.GetReleaseStrength();
         LOG(_T("f'ci has been updated"));
         m_DesignerOutcome.SetOutcome(fci_new> fci_old ? pgsDesignCodes::FciIncreased : pgsDesignCodes::FciDecreased);
      }

      // check to see if f'c was changed also
      Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
      if ( !IsEqual(fc_old,fc_new) )
      {
         LOG(_T("However, Final Was Also Increased to ") << WBFL::Units::ConvertFromSysUnits(fc_new,WBFL::Units::Measure::KSI) << _T(" KSI") );
         LOG(_T("Restart design with new strengths"));
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
   GET_IFACE(IPretensionForce,pPrestressForce);
   Float64 XferLength = Max(pPrestressForce->GetTransferLength(segmentKey, pgsTypes::Straight, pgsTypes::tltMinimum), 
                            pPrestressForce->GetTransferLength(segmentKey, pgsTypes::Harped,pgsTypes::tltMinimum));

   GET_IFACE(IBridge,pBridge);
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

std::vector<DebondLevelType> pgsDesigner2::DesignForLiftingDebonding(bool bProportioningStrands, IProgress* pProgress) const
{
   // If designConcrete is true, we want to set the release strength for our real design. If not,
   // the goal is to simply come up with a debonding layout that will work for the strength we compute
   // below. This layout will be used for the fabrication option when temporary strands are not used.

   pProgress->UpdateMessage(_T("Lifting Design for Debonded Girders"));
   ATLASSERT(m_StrandDesignTool.IsDesignDebonding());

   std::vector<DebondLevelType> debond_demand;

   m_StrandDesignTool.DumpDesignParameters();

   const CSegmentKey& segmentKey = m_StrandDesignTool.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftSegmentIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

   pgsGirderLiftingChecker checker(m_pBroker,m_StatusGroupID);
   GDRCONFIG config = m_StrandDesignTool.GetSegmentConfiguration();

   if ( bProportioningStrands )
   {
      // if this is the first design for lifting, look at the lifting without temporary strands case
      // to get the optimum strand configuration
      LOG(_T("Phase 1 Lifting Design - Design for Lifting without Temporary Strands"));
      LOG(_T("Determine debond strand layout"));
      LOG(_T(""));
      LOG(_T("Removing temporary strands for lifting analysis"));
      config.PrestressConfig.ClearStrandFill(pgsTypes::Temporary);
   }
#if defined ENABLE_LOGGING
   else
   {
      LOG(_T("Phase 2 Lifting Design - Design for Lifting with Temporary Strands"));
      LOG(_T("Determine lifting locations and release strength requirements"));
   }
#endif

   // Designer manages it's own POIs
   ISegmentLiftingDesignPointsOfInterest* pPoiLd = dynamic_cast<ISegmentLiftingDesignPointsOfInterest*>(&m_StrandDesignTool);

   HANDLINGCONFIG liftConfig;
   liftConfig.bIgnoreGirderConfig = false;
   liftConfig.GdrConfig = config;
   WBFL::Stability::LiftingCheckArtifact artifact;
   const WBFL::Stability::LiftingStabilityProblem* pStabilityProblem;
   pgsDesignCodes::OutcomeType result = checker.DesignLifting(segmentKey,liftConfig,pPoiLd,&artifact,&pStabilityProblem,LOGGER);
   SectionFinder::pStabilityProblem = pStabilityProblem; // this is the design problem we will be searching ... set it here and it will get used in multiple calls below

#if defined _DEBUG
   LOG(_T("-- Dump of Lifting Artifact After Design --"));
   DumpLiftingArtifact(pStabilityProblem,artifact,LOGGER);
   LOG(_T("-- End Dump of Lifting Artifact --"));
#endif

   CHECK_PROGRESS;

   m_DesignerOutcome.SetOutcome(result);
   if ( m_DesignerOutcome.WasDesignAborted() )
   {
      return debond_demand;
   }

   // Set the location required for stability
   m_StrandDesignTool.SetLiftingLocations(liftConfig.LeftOverhang,liftConfig.RightOverhang);

   m_DesignerOutcome.SetOutcome(pgsDesignCodes::LiftingConfigChanged);

   const WBFL::Stability::LiftingResults& liftingResults = artifact.GetLiftingResults();

   // Check to see if the girder is stable for lifting
   GET_IFACE(ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   Float64 FScr    = liftingResults.FScrMin;
   Float64 FScrMin = pSegmentLiftingSpecCriteria->GetLiftingCrackingFs();
   LOG(_T("FScr = ") << FScr);
   LOG(_T(""));
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
      LOG(_T("Cannot find a pick point to satisfy FScr"));
      if (bProportioningStrands)
      {
         LOG(_T("Temporary strands required"));
         LOG(_T("Move on to Shipping Design"));
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::LiftingRedesignAfterShipping);
      }
      else
      {
         // Hauling design didn't help - crap out
         LOG(_T("Unstable for lifting and any temporary strands added for hauling did not help - Design Failed") );
         m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::GirderLiftingStability);
         m_DesignerOutcome.AbortDesign();
      }

      return debond_demand;
   }

   Float64 fc_old  = m_StrandDesignTool.GetConcreteStrength();
   Float64 fci_old = m_StrandDesignTool.GetReleaseStrength();

   Float64 fci_max = m_StrandDesignTool.GetMaximumReleaseStrength();

   // Get required release strength from artifact
   Float64 fci_comp = artifact.RequiredFcCompression();
   Float64 fci_tens = artifact.RequiredFcTensionWithoutRebar();
   Float64 fci_tens_wrebar = artifact.RequiredFcTensionWithRebar();

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

   LOG(_T("Required Lifting Release Strength from artifact : f'ci (unrounded) tens = ") << WBFL::Units::ConvertFromSysUnits(fci_tens,WBFL::Units::Measure::KSI) << _T(" KSI, compression = ") << WBFL::Units::ConvertFromSysUnits(fci_comp,WBFL::Units::Measure::KSI) << _T(" KSI, Pick Point = ") << WBFL::Units::ConvertFromSysUnits(liftConfig.LeftOverhang,WBFL::Units::Measure::Feet) << _T(" ft"));
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

      LOG(_T("fci_reqd = ") << WBFL::Units::ConvertFromSysUnits(fci_reqd,WBFL::Units::Measure::KSI) << _T(" KSI") );

      if (fci_old < fci_reqd)
      {
         LOG(_T("fci_reqd is greater than current - will need to revisit lifting design after shipping for stress purposes") << WBFL::Units::ConvertFromSysUnits(fci_reqd,WBFL::Units::Measure::KSI) << _T(" KSI") );
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
         Float64 fci_curr = m_StrandDesignTool.GetReleaseStrength();
         if ( IsEqual(fci_max,fci_curr) )
         {
            LOG(_T("Release strength required for lifting is greater than our current max, and we have already tried max for design - Design Failed") );
            m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::GirderLiftingConcreteStrength);
            m_DesignerOutcome.AbortDesign();
            return debond_demand;
         }
         else
         {
            LOG(_T("Strength required for lifting is greater than our current max of ") << WBFL::Units::ConvertFromSysUnits(fci_max,WBFL::Units::Measure::KSI) << _T(" KSI - Try using max for one more go-around") );
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
      Float64 fci_old = m_StrandDesignTool.GetReleaseStrength();
      bFciUpdated |= m_StrandDesignTool.UpdateReleaseStrength(fci_tens,rebar_reqd, StressCheckTask(liftSegmentIntervalIdx,pgsTypes::ServiceI,pgsTypes::Tension),pgsTypes::TopGirder);
      bFciUpdated |= m_StrandDesignTool.UpdateReleaseStrength(fci_comp,rebar_reqd, StressCheckTask(liftSegmentIntervalIdx,pgsTypes::ServiceI,pgsTypes::Compression),pgsTypes::BottomGirder);
      if (bFciUpdated)
      {
         Float64 fci_new = m_StrandDesignTool.GetReleaseStrength();
         m_DesignerOutcome.SetOutcome(fci_new> fci_old ? pgsDesignCodes::FciIncreased : pgsDesignCodes::FciDecreased);

         Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
         if ( !IsEqual(fc_old,fc_new) )
         {
            LOG(_T("However, Final Was Also Increased to ") << WBFL::Units::ConvertFromSysUnits(fc_new,WBFL::Units::Measure::KSI) << _T(" KSI") );
            LOG(_T("May need to Restart design with new strengths"));
            m_DesignerOutcome.SetOutcome(fc_new> fc_old ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);
            return debond_demand;
         }
         else
         {
            LOG(_T("Release strength increased for lifting - design continues..."));
         }
      }

      fci_reqd =  m_StrandDesignTool.GetReleaseStrength();

      // Now that we have an established concrete strength, we can use it to design our debond layout

      HANDLINGCONFIG lift_config;
      lift_config.bIgnoreGirderConfig = false;
      lift_config.GdrConfig = m_StrandDesignTool.GetSegmentConfiguration();

      lift_config.GdrConfig.fci = fci_reqd;
      lift_config.LeftOverhang = m_StrandDesignTool.GetLeftLiftingLocation();
      lift_config.RightOverhang = m_StrandDesignTool.GetRightLiftingLocation();

      return DesignDebondingForLifting(lift_config, pProgress);
   }
}


std::vector<DebondLevelType> pgsDesigner2::DesignDebondingForLifting(HANDLINGCONFIG& liftConfig, IProgress* pProgress) const
{
   pProgress->UpdateMessage(_T("Designing initial debonding for Lifting"));
   ATLASSERT(m_StrandDesignTool.IsDesignDebonding());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liftingIntervalIdx = pIntervals->GetLiftSegmentInterval(liftConfig.GdrConfig.SegmentKey);

   // set up our vector to return debond levels at each section
   SectionIndexType max_db_sections = m_StrandDesignTool.GetMaxNumberOfDebondSections();
   std::vector<DebondLevelType> lifting_debond_levels;
   lifting_debond_levels.assign(max_db_sections,0);

   LOG(_T(""));
   LOG(_T("Detailed Debond Design for Lifting"));
   LOG(_T(""));
   m_StrandDesignTool.DumpDesignParameters();

   {
      const CSegmentKey& segmentKey = m_StrandDesignTool.GetSegmentKey();

      Float64 fc  = liftConfig.GdrConfig.fc;
      Float64 fci = liftConfig.GdrConfig.fci;
      LOG(_T("current f'c  = ") << WBFL::Units::ConvertFromSysUnits(fc,WBFL::Units::Measure::KSI) << _T(" KSI "));
      LOG(_T("current f'ci = ") << WBFL::Units::ConvertFromSysUnits(fci,WBFL::Units::Measure::KSI) << _T(" KSI") );

      GET_IFACE(ISegmentLiftingSpecCriteria,pLiftingCrit);
      Float64 allowable_tension = pLiftingCrit->GetLiftingAllowableTensileConcreteStressEx(segmentKey,fci,true);
      Float64 allowable_global_compression = pLiftingCrit->GetLiftingAllowableGlobalCompressiveConcreteStressEx(segmentKey, fci);
      Float64 allowable_peak_compression = pLiftingCrit->GetLiftingAllowablePeakCompressiveConcreteStressEx(segmentKey, fci);
      LOG(_T("Allowable tensile stress after Release     = ") << WBFL::Units::ConvertFromSysUnits(allowable_tension,WBFL::Units::Measure::KSI) << _T(" KSI - min rebar was required for this strength"));
      LOG(_T("Allowable global compressive stress after Release = ") << WBFL::Units::ConvertFromSysUnits(allowable_global_compression, WBFL::Units::Measure::KSI) << _T(" KSI"));
      LOG(_T("Allowable peak compressive stress after Release = ") << WBFL::Units::ConvertFromSysUnits(allowable_peak_compression, WBFL::Units::Measure::KSI) << _T(" KSI"));

      // This is an analysis to determine stresses that must be reduced by debonding
      LOG(_T("Debond levels measured from fully bonded section"));
      liftConfig.GdrConfig.PrestressConfig.Debond[pgsTypes::Straight].clear();

      WBFL::Stability::LiftingCheckArtifact artifact;

      pgsGirderLiftingChecker checker(m_pBroker,m_StatusGroupID);
      // Designer manages it's own POIs
      ISegmentLiftingDesignPointsOfInterest* pPoiLd = dynamic_cast<ISegmentLiftingDesignPointsOfInterest*>(&m_StrandDesignTool);
      checker.AnalyzeLifting(segmentKey,liftConfig,pPoiLd,&artifact);

      StrandIndexType nperm = liftConfig.GdrConfig.PrestressConfig.GetStrandCount(pgsTypes::Permanent);
      StrandIndexType ntemp =  liftConfig.GdrConfig.PrestressConfig.GetStrandCount(pgsTypes::Temporary);

      // Need total number of strands and cg of total strand group. 
      GET_IFACE(IPointOfInterest,pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_LIFT_SEGMENT, &vPoi);
      ASSERT( vPoi.size() == 1 );
      pgsPointOfInterest midPOI(vPoi.front());

      Float64 force_per_strand = 0.0;

      // only want stresses in end zones
      Float64 rgt_end, lft_end;
      m_StrandDesignTool.GetMidZoneBoundaries(&lft_end, &rgt_end);

      // we'll pick strand force at location just past transfer length
      Float64 xfer_length = m_StrandDesignTool.GetTransferLength(pgsTypes::Permanent);

      // Build stress demand
      std::vector<pgsStrandDesignTool::StressDemand> stress_demands;
      LOG(_T("--- Compute lifting stresses for debonding --- nperm = ")<<nperm);
      GET_IFACE(IGirder,pGirder);
      const WBFL::Stability::ILiftingStabilityProblem* pStabilityProblem = pGirder->GetSegmentLiftingStabilityProblem(segmentKey,liftConfig,pPoiLd);
      const WBFL::Stability::LiftingResults& results = artifact.GetLiftingResults();
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

            LOG(_T("At ")<< WBFL::Units::ConvertFromSysUnits(poi_loc,WBFL::Units::Measure::Feet)<<_T(" ft, Ftop = ")<< WBFL::Units::ConvertFromSysUnits(fTop,WBFL::Units::Measure::KSI) << _T(" ksi Fbot = ")<< WBFL::Units::ConvertFromSysUnits(fBot,WBFL::Units::Measure::KSI) << _T(" ksi") );
            LOG(_T("Average force per strand = ") << WBFL::Units::ConvertFromSysUnits(Fpe/(nperm+ntemp),WBFL::Units::Measure::Kip) << _T(" kip"));

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
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liftingIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);

      GET_IFACE(IStrandGeometry, pStrandGeom);
      auto cg = pStrandGeom->GetStrandCG(liftingIntervalIdx, midPOI, true, &liftConfig.GdrConfig);
      lifting_debond_levels = m_StrandDesignTool.ComputeDebondsForDemand(stress_demands, liftConfig.GdrConfig, cg.Y(), liftingIntervalIdx, allowable_tension, allowable_global_compression);

      if ( lifting_debond_levels.empty() )
      {
         ATLASSERT(false);
         LOG(_T("Debonding failed, this should not happen?"));

         m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::DebondDesignFailed);
         m_DesignerOutcome.AbortDesign();
      }
   }

   return lifting_debond_levels;
}

void pgsDesigner2::DesignForShipping(IProgress* pProgress) const
{
   pProgress->UpdateMessage(_T("Designing for Shipping"));

   LOG(_T(""));
   LOG(_T("DESIGNING FOR SHIPPING"));
   LOG(_T(""));

   m_StrandDesignTool.DumpDesignParameters();

   Float64 fc_current = m_StrandDesignTool.GetConcreteStrength();

   const CSegmentKey& segmentKey = m_StrandDesignTool.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType haulSegmentIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);

   // Use factory to create appropriate hauling checker
   pgsGirderHandlingChecker checker_factory(m_pBroker,m_StatusGroupID);
   std::unique_ptr<pgsGirderHaulingChecker> hauling_checker( checker_factory.CreateGirderHaulingChecker() );

   bool bResult = false;

   HANDLINGCONFIG haulConfig;
   haulConfig.bIgnoreGirderConfig = false;
   haulConfig.GdrConfig = m_StrandDesignTool.GetSegmentConfiguration();

   ISegmentHaulingDesignPointsOfInterest* pPoiLd = dynamic_cast<ISegmentHaulingDesignPointsOfInterest*>(&m_StrandDesignTool);

   std::unique_ptr<pgsHaulingAnalysisArtifact> artifact ( hauling_checker->DesignHauling(segmentKey,haulConfig,m_bShippingDesignIgnoreConfigurationLimits,pPoiLd,&bResult,LOGGER));

   if (bResult == false && m_bShippingDesignIgnoreConfigurationLimits == false)
   {
      // Designer could not find a valid configuration.
      LOG(_T("Failed to satisfy shipping requirements - shipping configuration limitations may be preventing a suitable solution from being found. Ignore limitations and try again"));
      m_bShippingDesignIgnoreConfigurationLimits = true; // ignore configuration limitations and try again
      artifact.reset(hauling_checker->DesignHauling(segmentKey, haulConfig, m_bShippingDesignIgnoreConfigurationLimits, pPoiLd, &bResult, LOGGER));
   }

   // We've got a good shipping configuration - the only thing to worry about now is stresses
   
   // capture the results of the design
   m_StrandDesignTool.SetTruckSupportLocations(haulConfig.LeftOverhang,haulConfig.RightOverhang);
   // We now have bunk point locations to ensure stability
   m_DesignerOutcome.SetOutcome(pgsDesignCodes::HaulingConfigChanged);
   if ( haulConfig.pHaulTruckEntry )
   {
      m_StrandDesignTool.SetHaulTruck( haulConfig.pHaulTruckEntry->GetName().c_str() );
   }
   else
   {
      m_StrandDesignTool.SetHaulTruck( _T("Unknown") );
   }

   CHECK_PROGRESS;

#if defined _DEBUG
   LOG(_T("-- Dump of Hauling Artifact After Design --"));
   if (artifact.get() != nullptr)
   {
      artifact->Dump(LOGGER);
   }
   LOG(_T("-- End Dump of Hauling Artifact --"));
#endif

   bool bPassedStressChecks = artifact->PassedStressCheck(pgsTypes::CrownSlope) && artifact->PassedStressCheck(pgsTypes::Superelevation);
   LOG(_T("Design ") << (bPassedStressChecks ? _T("did") : _T("did not")) << _T(" pass stress checks"));

   if (bResult && bPassedStressChecks)
   {
      m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::Success);
      return;
   }

   Float64 fc_max = m_StrandDesignTool.GetMaximumConcreteStrength();

   // Get required release strength from artifact
   Float64 fc_comp1(0.0), fc_comp2(0.0), fc_tens(0.0), fc_tens_wrebar1(0.0), fc_tens_wrebar2(0.0);
   artifact->GetRequiredConcreteStrength(pgsTypes::CrownSlope, &fc_comp1, &fc_tens, &fc_tens_wrebar1);
   artifact->GetRequiredConcreteStrength(pgsTypes::Superelevation, &fc_comp2, &fc_tens, &fc_tens_wrebar2);

   Float64 fc_comp = Max(fc_comp1, fc_comp2);
   fc_tens = Max(fc_tens_wrebar1, fc_tens_wrebar2); // Hauling design always uses higher allowable limit (lower f'c)

   LOG(_T("f'c (unrounded) required for shipping; tension = ") << WBFL::Units::ConvertFromSysUnits(fc_tens,WBFL::Units::Measure::KSI) << _T(" KSI, compression = ") << WBFL::Units::ConvertFromSysUnits(fc_comp,WBFL::Units::Measure::KSI) << _T(" KSI"));

   CHECK_PROGRESS;

   if (fc_tens <= fc_max && // tension is less than max strength - AND -
       fc_comp <= fc_max && // compression is less than max strength - AND -
       fc_tens <= fc_comp // strength is controlled by compression... adjust the concrete strength (if controlled by tension, add temporary strands)
      )
   {
      LOG(_T("Required concrete strength does not exceed maximum. Attempting to increase concrete strength"));
      // NOTE: Using bogus stress location
      Float64 fc_old = m_StrandDesignTool.GetConcreteStrength();

      bool bFcUpdated = m_StrandDesignTool.UpdateConcreteStrength(fc_tens, StressCheckTask(haulSegmentIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension), pgsTypes::TopGirder);
      bFcUpdated |= m_StrandDesignTool.UpdateConcreteStrength(fc_comp, StressCheckTask(haulSegmentIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression), pgsTypes::BottomGirder);
      if (bFcUpdated)
      {
         LOG(_T("Concrete strength was increased for shipping - Restart"));
         Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
         m_DesignerOutcome.SetOutcome(fc_old < fc_new ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);
         return;
      }
   }

   CHECK_PROGRESS;

   // there isn't a concrete strength that will work (because of tension limit)


   // Add temporary strands and try again.
   LOG(_T("There is no concrete strength that will work for shipping... Adding temporary strands"));
   if (m_StrandDesignTool.AddTempStrands())
   {
      LOG(_T("Temporary strands added. Restart design"));
      m_DesignerOutcome.SetOutcome(pgsDesignCodes::TemporaryStrandsChanged);
      return;
   }
   else
   {
      LOG(_T("Could not add temporary strands - attempt to increase concrete strength"));
      Float64 fc_old = m_StrandDesignTool.GetConcreteStrength();
      bool bFcUpdated = m_StrandDesignTool.UpdateConcreteStrength(fc_tens, StressCheckTask(haulSegmentIntervalIdx, pgsTypes::ServiceI, pgsTypes::Tension), pgsTypes::TopGirder);
      bFcUpdated |= m_StrandDesignTool.UpdateConcreteStrength(fc_comp, StressCheckTask(haulSegmentIntervalIdx, pgsTypes::ServiceI, pgsTypes::Compression), pgsTypes::BottomGirder);
      if (bFcUpdated)
      {
         LOG(_T("Concrete strength was increased for shipping - Restart"));
         Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
         m_DesignerOutcome.SetOutcome(fc_old < fc_new ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);
         return;
      }

      m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::GirderShippingConcreteStrength);
      //m_DesignerOutcome.AbortDesign();
      return;
   }

   LOG(_T("Shipping Results : f'c (unrounded) tens = ") << WBFL::Units::ConvertFromSysUnits(fc_tens, WBFL::Units::Measure::KSI) << _T(" KSI, Comp = ")
      << WBFL::Units::ConvertFromSysUnits(fc_comp, WBFL::Units::Measure::KSI) << _T("KSI, Left Bunk Point = ")
      << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool.GetTrailingOverhang(), WBFL::Units::Measure::Feet) << _T(" ft")
      << _T("    Right Bunk Point = ") << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool.GetLeadingOverhang(), WBFL::Units::Measure::Feet) << _T(" ft"));

   LOG(_T("Shipping Design Complete - Continue design") );
}

bool pgsDesigner2::CheckShippingStressDesign(const CSegmentKey& segmentKey,const GDRCONFIG& config) const
{
   HANDLINGCONFIG ship_config;
   ship_config.bIgnoreGirderConfig = false;
   ship_config.GdrConfig = config;
   ship_config.LeftOverhang = m_StrandDesignTool.GetLeadingOverhang();
   ship_config.RightOverhang = m_StrandDesignTool.GetTrailingOverhang();

   ISegmentHaulingDesignPointsOfInterest* pPoiLd = dynamic_cast<ISegmentHaulingDesignPointsOfInterest*>(&m_StrandDesignTool);

   // Use factory to create appropriate hauling checker
   pgsGirderHandlingChecker checker_factory(m_pBroker,m_StatusGroupID);
   std::unique_ptr<pgsGirderHaulingChecker> hauling_checker( checker_factory.CreateGirderHaulingChecker() );

   std::unique_ptr<pgsHaulingAnalysisArtifact> artifact( hauling_checker->AnalyzeHauling(segmentKey,ship_config,pPoiLd) );

   return artifact->PassedStressCheck(pgsTypes::CrownSlope) && artifact->PassedStressCheck(pgsTypes::Superelevation);
}

void pgsDesigner2::RefineDesignForAllowableStress(IProgress* pProgress) const
{
   const CSegmentKey& segmentKey = m_StrandDesignTool.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);

#if defined ENABLE_LOGGING
   IntervalIndexType liveLoadIntervalIdx  = pIntervals->GetLiveLoadInterval();
#endif

   ATLASSERT(!m_DesignerOutcome.DidConcreteChange()); // if this flag is set going in, we will get false positive

   GET_IFACE(IAllowableConcreteStress,pAllowable);

   // Our only option is to increase concrete strength, so let loop finish unless we fail.
   for(const auto& task : m_StressCheckTasks)
   {
      if ( !pAllowable->IsStressCheckApplicable(segmentKey,task) )
      {
         // stress check isn't applicable so move on to the next one
         continue;
      }

      if ( task.intervalIdx == tsRemovalIntervalIdx && m_StrandDesignTool.GetNt() == 0 )
      {
         // if there aren't any temporary strands, don't refine design for temporary strand removal
         continue;
      }

#if defined ENABLE_LOGGING
      LOG(_T(""));
      if ( task.bIncludeLiveLoad )
      {
         LOG(_T("*** Refining design for Interval ") << LABEL_INTERVAL(task.intervalIdx) << _T(", ") << pIntervals->GetDescription(task.intervalIdx) << _T(" ") << g_LimitState[task.limitState] << _T(" ") << g_Type[task.stressType] );
      }
      else
      {
         if ( liveLoadIntervalIdx <= task.intervalIdx )
         {
            LOG(_T("*** Refining design for Interval ") << LABEL_INTERVAL(task.intervalIdx) << _T(", ") << pIntervals->GetDescription(task.intervalIdx) << _T(" (without live load) ") << g_LimitState[task.limitState] << _T(" ") << g_Type[task.stressType] );
         }
         else
         {
            LOG(_T("*** Refining design for Interval ") << LABEL_INTERVAL(task.intervalIdx) << _T(", ") << pIntervals->GetDescription(task.intervalIdx) << _T(" ") << g_LimitState[task.limitState] << _T(" ") << g_Type[task.stressType] );
         }
      }
#endif // ENABLE_LOGGING

      RefineDesignForAllowableStress(task,pProgress);

      CHECK_PROGRESS;
      if (m_DesignerOutcome.WasDesignAborted() )
      {
         return;
      }
      else if (m_DesignerOutcome.DidConcreteChange())
      {
         LOG(_T("An allowable stress check failed - Restart design with new concrete strength"));
         LOG(_T("============================================================================"));
         return;
      }
   }

   LOG(_T("**** Successfully completed allowable stress design"));
}

void pgsDesigner2::RefineDesignForAllowableStress(const StressCheckTask& task,IProgress* pProgress) const
{
   const CSegmentKey& segmentKey = m_StrandDesignTool.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
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
   GET_IFACE(ILossParameters, pLossParams);
   ATLASSERT(pLossParams->GetLossMethod() != pgsTypes::TIME_STEP);

   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   ATLASSERT(task.intervalIdx != railingSystemIntervalIdx);
#endif

   IntervalIndexType intervalIdx = task.intervalIdx;
   if (intervalIdx == releaseIntervalIdx && 0 < m_StrandDesignTool.GetNt())
   {
      // if there are TTS, then use the interval after release, instead of release
      // to account for TTS PT immediately after release
      intervalIdx++;
   }

   Float64 fcgdr;
   const GDRCONFIG& config = m_StrandDesignTool.GetSegmentConfiguration();
   if ( task.intervalIdx == releaseIntervalIdx )
   {
      fcgdr = config.fci;
   }
   else
   {
      fcgdr = config.fc;
   }

   GET_IFACE(IAllowableConcreteStress,pAllowable);
   GET_IFACE(ILimitStateForces,pLimitStateForces);
   GET_IFACE(IPretensionStresses,pPsStress);

   LOG(_T(""));
   LOG(_T("Begin Design Refinement Iterations"));
   m_StrandDesignTool.DumpDesignParameters();

   Float64 start_end_size = 0.0;
   if ( releaseIntervalIdx < intervalIdx )
   {
      GET_IFACE(IBridge,pBridge);
      start_end_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   }

   //
   // Get the allowable stresses
   //
   Float64 fAllow;
   pgsPointOfInterest dummyPOI(segmentKey,0.0);
   if ( task.stressType == pgsTypes::Compression )
   {
      fAllow = pAllowable->GetSegmentAllowableCompressionStress(dummyPOI,task,fcgdr);
   }
   else
   {
      bool bWithBondedReinforcement = false;
      if ( intervalIdx == releaseIntervalIdx )
      {
         bWithBondedReinforcement = m_StrandDesignTool.DoesReleaseRequireAdditionalRebar();
      }
      fAllow = pAllowable->GetSegmentAllowableTensionStress(dummyPOI,task,fcgdr,bWithBondedReinforcement);
   }
   LOG(_T("Allowable stress = ") << WBFL::Units::ConvertFromSysUnits(fAllow,WBFL::Units::Measure::KSI) << _T(" KSI"));

   bool adj_strength = false; // true if we need to increase strength
   Float64 fControl = task.stressType == pgsTypes::Tension ? -Float64_Max :  Float64_Max;  // controlling stress for all pois
   pgsTypes::StressLocation stress_location;

   pgsTypes::BridgeAnalysisType batTop, batBottom;
   GetBridgeAnalysisType(segmentKey.girderIndex,task,batTop,batBottom);

   PoiList vPoi;
   if ( m_StrandDesignTool.IsDesignHarping() )
   {
      PoiAttributeType refAttrib = (intervalIdx < erectSegmentIntervalIdx ? POI_RELEASED_SEGMENT : POI_ERECTED_SEGMENT);
      m_StrandDesignTool.GetDesignPoi(intervalIdx, refAttrib | POI_5L, &vPoi);
   
      PoiList morePoi;
      m_StrandDesignTool.GetDesignPoi(intervalIdx, POI_PSXFER | POI_CONCLOAD | POI_HARPINGPOINT, &morePoi);
      vPoi.insert(vPoi.end(),morePoi.begin(),morePoi.end());
   }
   else
   {
      PoiAttributeType refAttrib = (intervalIdx < erectSegmentIntervalIdx ? POI_RELEASED_SEGMENT : POI_ERECTED_SEGMENT);
      m_StrandDesignTool.GetDesignPoi(intervalIdx,refAttrib,&vPoi);

      PoiList morePoi;
      m_StrandDesignTool.GetDesignPoi(intervalIdx, POI_H | POI_PSXFER | POI_DEBOND,&morePoi);
      vPoi.insert(vPoi.end(),morePoi.begin(),morePoi.end());
   }

   GET_IFACE(IPointOfInterest, pPoi);
   pPoi->SortPoiList(&vPoi); // sort and remove duplicates
   ATLASSERT(0 < vPoi.size());

   for(const pgsPointOfInterest& poi : vPoi)
   {
      CHECK_PROGRESS;

      LOG(_T("Designing at ") << WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart() - start_end_size,WBFL::Units::Measure::Feet) << _T(" ft") << _T("(POI ID ") << poi.GetID() << _T(")"));

      //
      // Get the stresses due to externally applied loads
      //
      Float64 fTopMinExt, fTopMaxExt;
      Float64 fBotMinExt, fBotMaxExt;
      pLimitStateForces->GetDesignStress(task,poi,pgsTypes::TopGirder,   &config,batTop,   &fTopMinExt,&fTopMaxExt);
      pLimitStateForces->GetDesignStress(task,poi,pgsTypes::BottomGirder,&config,batBottom,&fBotMinExt,&fBotMaxExt);

      LOG(_T("Max External Stress  :: Top = ") << WBFL::Units::ConvertFromSysUnits(fTopMaxExt,WBFL::Units::Measure::KSI) << _T(" KSI") << _T("    Bot = ") << WBFL::Units::ConvertFromSysUnits(fBotMaxExt,WBFL::Units::Measure::KSI) << _T(" KSI"));
      LOG(_T("Min External Stress  :: Top = ") << WBFL::Units::ConvertFromSysUnits(fTopMinExt,WBFL::Units::Measure::KSI) << _T(" KSI") << _T("    Bot = ") << WBFL::Units::ConvertFromSysUnits(fBotMinExt,WBFL::Units::Measure::KSI) << _T(" KSI"));

      //
      // Get the stresses due to prestressing (adjust for losses)
      //
      Float64 fTopPre = pPsStress->GetDesignStress(intervalIdx,poi,pgsTypes::TopGirder,config, task.bIncludeLiveLoad, task.limitState);
      Float64 fBotPre = pPsStress->GetDesignStress(intervalIdx,poi,pgsTypes::BottomGirder,config, task.bIncludeLiveLoad, task.limitState);
      LOG(_T("Prestress Stress     :: Top = ") << WBFL::Units::ConvertFromSysUnits(fTopPre,WBFL::Units::Measure::KSI) << _T(" KSI") << _T("    Bot = ") << WBFL::Units::ConvertFromSysUnits(fBotPre,WBFL::Units::Measure::KSI) << _T(" KSI"));

      //
      // Compute the resultant stresses on the section
      //
      GET_IFACE(ILoadFactors,pLF);
      const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();
      Float64 k = pLoadFactors->GetDCMax(task.limitState);

      Float64 fTopMin, fTopMax;
      Float64 fBotMin, fBotMax;

      fTopMin = fTopMinExt + k*fTopPre;
      fTopMax = fTopMaxExt + k*fTopPre;
      fBotMin = fBotMinExt + k*fBotPre;
      fBotMax = fBotMaxExt + k*fBotPre;

      LOG(_T("Max Resultant Stress :: Top = ") << WBFL::Units::ConvertFromSysUnits(fTopMax,WBFL::Units::Measure::KSI) << _T(" KSI") << _T("    Bot = ") << WBFL::Units::ConvertFromSysUnits(fBotMax,WBFL::Units::Measure::KSI) << _T(" KSI"));
      LOG(_T("Min Resultant Stress :: Top = ") << WBFL::Units::ConvertFromSysUnits(fTopMin,WBFL::Units::Measure::KSI) << _T(" KSI") << _T("    Bot = ") << WBFL::Units::ConvertFromSysUnits(fBotMin,WBFL::Units::Measure::KSI) << _T(" KSI"));

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
            if ( fAllow < fTopMax && !IsEqual(fAllow,fTopMax) )
            {
               LOG(_T("** Failed in tension at top of girder"));
               fControl = Max(fControl, fTopMax);
               stress_location = pgsTypes::TopGirder;
               adj_strength = true;
            }
         }
         else
         {
            // tension bottom controlling
            if ( fAllow < fBotMax && !IsEqual(fAllow,fBotMax)  )
            {
               // tensile zone (bottom of girder)
               LOG(_T("** Failed in tension at bottom of girder"));
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
            if ( fBotMin < fAllow && !IsEqual(fBotMin,fAllow,0.001) )
            {
               LOG( _T("** Failed in compression at the bottom of the girder") );

               fControl = Min(fControl, fBotMin);
               stress_location = pgsTypes::BottomGirder;
               adj_strength = true;
            }
         }
         else
         {
            // compression top controlling
            if ( fTopMin < fAllow && !IsEqual(fTopMin,fAllow,0.001) )
            {
               LOG( _T("** Failed in compression at the top of the girder") );

               fControl = Min(fControl, fTopMin);
               stress_location = pgsTypes::TopGirder;
               adj_strength = true;
            }
         }
         break;

      default:
         ATLASSERT(false); // should never get here
      } // end of switch on type

   }  // Next poi

   if ( adj_strength )
   {
      LOG(_T("** Need to increase concrete strength. Controlling stress is ")<<WBFL::Units::ConvertFromSysUnits(fControl,WBFL::Units::Measure::KSI) << _T(" KSI"));

      // Try the next highest concrete strength
      Float64 fc_reqd;
      ConcStrengthResultType result = m_StrandDesignTool.ComputeRequiredConcreteStrength(fControl,task,&fc_reqd);

      if ( ConcFailed == result )
      {
         // could not find a concrete strength that would work
         m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::StressExceedsConcreteStrength);
         m_DesignerOutcome.AbortDesign();
         return;
      }

      if( intervalIdx == releaseIntervalIdx )
      {
         Float64 fci_old = m_StrandDesignTool.GetReleaseStrength();

         if (m_StrandDesignTool.UpdateReleaseStrength(fc_reqd,result, task,stress_location))
         {
            Float64 fci_new = m_StrandDesignTool.GetReleaseStrength();
            LOG(_T("Release Strength For tension Changed to ")  << WBFL::Units::ConvertFromSysUnits(m_StrandDesignTool.GetReleaseStrength(), WBFL::Units::Measure::KSI) << _T(" KSI"));
            m_DesignerOutcome.SetOutcome(fci_new> fci_old ? pgsDesignCodes::FciIncreased : pgsDesignCodes::FciDecreased);
         }
      }
      else
      {
         Float64 fc_old = m_StrandDesignTool.GetConcreteStrength();
         if (m_StrandDesignTool.UpdateConcreteStrength(fc_reqd,task,stress_location))
         {
            Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
            m_DesignerOutcome.SetOutcome(fc_new> fc_old ? pgsDesignCodes::FcIncreased : pgsDesignCodes::FcDecreased);
         }
      }

      LOG(_T(""));

   }
}

void pgsDesigner2::RefineDesignForUltimateMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,IProgress* pProgress) const
{
   const CSegmentKey& segmentKey = m_StrandDesignTool.GetSegmentKey();

   PoiList vPoi;
   m_StrandDesignTool.GetDesignPoi(intervalIdx, POI_ERECTED_SEGMENT, &vPoi);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   
   GET_IFACE(IBridge,pBridge);
   Float64 start_end_size = (intervalIdx == releaseIntervalIdx)? 0.0 : pBridge->GetSegmentStartBearingOffset(segmentKey);

   m_StrandDesignTool.DumpDesignParameters();

   auto poiIter(vPoi.begin());
   auto poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      CHECK_PROGRESS;

      const pgsPointOfInterest& poi = *poiIter;

      LOG(_T(""));
      LOG(_T("======================================================================================================="));
      LOG(_T("Designing at ") << WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart() - start_end_size,WBFL::Units::Measure::Feet) << _T(" ft"));

      const GDRCONFIG& config = m_StrandDesignTool.GetSegmentConfiguration();

      pgsFlexuralCapacityArtifact cap_artifact(true);
      CreateFlexuralCapacityArtifact(poi,intervalIdx,limitState,config,true,&cap_artifact); // positive moment

      LOG(_T("Capacity (pMn) = ") << WBFL::Units::ConvertFromSysUnits(cap_artifact.GetCapacity(),WBFL::Units::Measure::KipFeet) << _T(" k-ft") << _T("   Demand (Mu) = ") << WBFL::Units::ConvertFromSysUnits(cap_artifact.GetDemand(),WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
      LOG(_T("Max Reinf Ratio (c/de) = ") << cap_artifact.GetMaxReinforcementRatio() << _T("   Max Reinf Ratio Limit = ") << cap_artifact.GetMaxReinforcementRatioLimit());
      LOG(_T("Capacity (pMn) = ") << WBFL::Units::ConvertFromSysUnits(cap_artifact.GetCapacity(),WBFL::Units::Measure::KipFeet) << _T(" k-ft") << _T("   Min Capacity (pMn Min: Lessor of 1.2Mcr and 1.33Mu) = ") << WBFL::Units::ConvertFromSysUnits(cap_artifact.GetMinCapacity(),WBFL::Units::Measure::KipFeet) << _T(" k-ft"));

#if defined ENABLE_LOGGING
      GET_IFACE(IMomentCapacity, pMomentCapacity);

      const MOMENTCAPACITYDETAILS* pmcd = pMomentCapacity->GetMomentCapacityDetails( intervalIdx, poi, true, &config );

      LOG(_T("fps_avg = ") << WBFL::Units::ConvertFromSysUnits( pmcd->fps_avg, WBFL::Units::Measure::KSI) << _T(" KSI") );
      LOG(_T("fpt_avg_segment = ") << WBFL::Units::ConvertFromSysUnits(pmcd->fpt_avg_segment, WBFL::Units::Measure::KSI) << _T(" KSI"));
      LOG(_T("fpt_avg_girder = ") << WBFL::Units::ConvertFromSysUnits(pmcd->fpt_avg_girder, WBFL::Units::Measure::KSI) << _T(" KSI"));
      LOG(_T("phi = ") << pmcd->Phi );
      LOG(_T("C = ") << WBFL::Units::ConvertFromSysUnits( pmcd->C, WBFL::Units::Measure::Kip) << _T(" kip"));
      LOG(_T("dc = ") << WBFL::Units::ConvertFromSysUnits( pmcd->dc, WBFL::Units::Measure::Inch) << _T(" inch"));
      LOG(_T("de = ") << WBFL::Units::ConvertFromSysUnits( pmcd->de, WBFL::Units::Measure::Inch) << _T(" inch"));
      LOG(_T("dt = ") << WBFL::Units::ConvertFromSysUnits( pmcd->dt, WBFL::Units::Measure::Inch) << _T(" inch"));
      LOG(_T("Moment Arm = ") << WBFL::Units::ConvertFromSysUnits( pmcd->MomentArm, WBFL::Units::Measure::Inch) << _T(" inch"));

      GET_IFACE(ILosses,pILosses);
      Float64 check_loss = pILosses->GetEffectivePrestressLossWithLiveLoad(poi,pgsTypes::Permanent,pgsTypes::ServiceIII, INVALID_INDEX/*controlling live load*/, true/*include elastic effects*/, true/*apply elastic gain reduction*/, &config);
      LOG(_T("Losses = ") << WBFL::Units::ConvertFromSysUnits( check_loss, WBFL::Units::Measure::KSI) << _T(" KSI") );

      CRACKINGMOMENTDETAILS cmd;
      pMomentCapacity->GetCrackingMomentDetails(intervalIdx, poi, config, true, &cmd);
      LOG(_T("Mcr = ") << WBFL::Units::ConvertFromSysUnits(cmd.Mcr,WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
      LOG(_T("Mdnc = ")<< WBFL::Units::ConvertFromSysUnits(cmd.Mdnc,WBFL::Units::Measure::KipFeet) << _T(" k-ft"));
      LOG(_T("fcpe = ") << WBFL::Units::ConvertFromSysUnits( cmd.fcpe, WBFL::Units::Measure::KSI) << _T(" KSI") );
      LOG(_T("fr = ") << WBFL::Units::ConvertFromSysUnits( cmd.fr, WBFL::Units::Measure::KSI) << _T(" KSI") );
      LOG(_T("Sb = ") << WBFL::Units::ConvertFromSysUnits( cmd.Sb, WBFL::Units::Measure::Inch3) << _T(" in^3"));
      LOG(_T("Sbc = ") << WBFL::Units::ConvertFromSysUnits( cmd.Sbc, WBFL::Units::Measure::Inch3) << _T(" in^3"));
      LOG(_T("Mcr Limit = ") << WBFL::Units::ConvertFromSysUnits(cmd.McrLimit,WBFL::Units::Measure::KipFeet) << _T(" k-ft"));

#endif // ENABLE_LOGGING

      if ( !cap_artifact.Passed() )
      {
         // Check Ultimate Capacity
         Float64 capacity = cap_artifact.GetCapacity();
         Float64 demand  = cap_artifact.GetDemand();
         if ( capacity < demand )
         {
            LOG(_T("** Ultimate Flexural Capacity Artifact failed at ")<< WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart() , WBFL::Units::Measure::Feet) << _T(" ft. Attempt to add strands"));
            StrandIndexType curr_strands = m_StrandDesignTool.GetNumPermanentStrands();
            StrandIndexType max_strands = m_StrandDesignTool.GetMaxPermanentStrands();

            bool success=false;
            if (max_strands <= curr_strands)
            {
               LOG(_T("Number of strands already max - we can't add any more"));
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

               new_num = m_StrandDesignTool.GetNextNumPermanentStrands(new_num);

               // Make sure we actually add some strands
               StrandIndexType min_new_num = m_StrandDesignTool.GetNextNumPermanentStrands(curr_strands);
               new_num = Max(new_num, min_new_num);

               if (new_num < max_strands)
               {
                  LOG(_T("Used demand/capacity ratio of ")<<(demand/capacity)<<_T(" to get a new number of strands = ")<<new_num);
               }
               else
               {
                  LOG(_T("Use max number of strands to alleviate ultimate moment ")<<new_num);
                  new_num = max_strands;
               }

               success = m_StrandDesignTool.SetNumPermanentStrands(new_num);
            }

            if ( !success )
            {
               LOG(_T("Attempt to add strands failed, Try bumping concrete strength by 500psi"));
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
               bool success = m_StrandDesignTool.Bump500(StressCheckTask(intervalIdx, limitState, pgsTypes::Tension), pgsTypes::BottomGirder);
               if (success)
               {
                  LOG(_T("Just threw a Hail Mary - Restart design with much higher concrete strength"));
                  m_DesignerOutcome.SetOutcome(pgsDesignCodes::ChangedForUltimate);
                  m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciIncreased);
                  m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcIncreased);
                  break;
               }
               else
               {
                  LOG(_T("Concrete Strength Cannot be adjusted"));
                  m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::UltimateMomentCapacity);
                  m_DesignerOutcome.AbortDesign();
                  return;
               }
            }
            else
            {
               LOG(_T("Attempt to add strands succeeded NP = ") << m_StrandDesignTool.GetNumPermanentStrands());
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::ChangedForUltimate);
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);

               // 
               LOG(_T("Compute new capacity to see if we are increasing. If not, we need another strategy"));
               const GDRCONFIG& new_config = m_StrandDesignTool.GetSegmentConfiguration();
               pgsFlexuralCapacityArtifact new_cap_artifact(true);
               CreateFlexuralCapacityArtifact(poi,intervalIdx,limitState,new_config,true,&new_cap_artifact); // positive moment
               Float64 new_capacity = new_cap_artifact.GetCapacity();
               LOG(_T("New Capacity = ") << WBFL::Units::ConvertFromSysUnits(new_capacity,WBFL::Units::Measure::KipFeet) << _T(" k-ft"));

               if (new_capacity < capacity)
               {
                  LOG(_T("We added strands and the capacity did not increase - reduce strands back to original and try bumping concrete strength"));
                  success = m_StrandDesignTool.SetNumPermanentStrands(curr_strands);

                  bool success = m_StrandDesignTool.Bump500(StressCheckTask(intervalIdx, limitState, pgsTypes::Tension), pgsTypes::BottomGirder);
                  if (success)
                  {
                     m_DesignerOutcome.SetOutcome(pgsDesignCodes::ChangedForUltimate);
                     m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcIncreased);
                     return;
                  }
                  else
                  {
                     LOG(_T("Attempt to bump concrete strength failed - we're probably toast at this point, but keep trying to add strands"));
                     m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::UltimateMomentCapacity);
                     m_DesignerOutcome.AbortDesign();
                  }
               }


               poiIter = vPoi.begin();
               continue;
            }
         }

         // Check Maximum Reinforcement
         if ( cap_artifact.GetMaxReinforcementRatioLimit() < cap_artifact.GetMaxReinforcementRatio() )
         {
            // No adjustment to be made. Use a bigger section
            LOG(_T("Capacity Artifact failed for max reinforcement ratio - section over reinforced ")<< WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart() , WBFL::Units::Measure::Feet) << _T(" ft"));
            LOG(_T("All we can do here is attempt to bump concrete strength by 500psi"));
            bool bSuccess = m_StrandDesignTool.Bump500(StressCheckTask(intervalIdx, limitState, pgsTypes::Tension), pgsTypes::BottomGirder);
            if (bSuccess)
            {
               LOG(_T("Concrete strength was increased for section over reinforced case - Restart") );
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::ChangedForUltimate);
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciIncreased);
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcIncreased);
               return;
            }
            else
            {
               LOG(_T("Failed to increase concrete strength, cannot remove strands - Failed due to over reinforcement - abort"));
               m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::OverReinforced);
               m_DesignerOutcome.AbortDesign();
               return;
            }
         }

         // Check Minimum Reinforcement
         if ( cap_artifact.GetCapacity() < cap_artifact.GetMinCapacity() )
         {
           LOG(_T("Min Reinforcement for Flexural Capacity Artifact failed at ")<< WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart() , WBFL::Units::Measure::Feet) << _T(" ft"));

           if ( !m_StrandDesignTool.AddStrands() )
           {
              LOG(_T("Attempt to add strands failed"));
              m_StrandDesignTool.SetOutcome(pgsSegmentDesignArtifact::UnderReinforced);
              m_DesignerOutcome.AbortDesign();
              return;
           }
           else
           {
               LOG(_T("Attempt to add strands succeeded NP = ") << m_StrandDesignTool.GetNumPermanentStrands());
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::ChangedForUltimate);
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
               poiIter = vPoi.begin();
               continue;
           }
         }
      }
   }

   if (m_DesignerOutcome.GetOutcome(pgsDesignCodes::ChangedForUltimate) )
   {
      // set minimum number of strands for next design iteration
      StrandIndexType min_strands = m_StrandDesignTool.GetNumPermanentStrands();
      LOG(_T("Minimum number of strands set to control capacity = ")<<min_strands);
      m_StrandDesignTool.SetMinimumPermanentStrands(min_strands);
   }
}

// Stirrup Design
void pgsDesigner2::DesignShear(pgsSegmentDesignArtifact* pArtifact, bool bDoStartFromScratch, bool bDoDesignFlexure) const
{
   const CSegmentKey& segmentKey = pArtifact->GetSegmentKey();
   ATLASSERT(segmentKey.segmentIndex == 0); // only design with PGSuper and there is only one segment
   const Float64 one_inch = WBFL::Units::ConvertToSysUnits(1.0, WBFL::Units::Measure::Inch); // Very US bias here

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetIntervalCount()-1;

   // We only iterate on shear design if Long Reinf for Shear design runs into the case where
   // stirrup tightening is a remedy.
   m_ShearDesignTool.SetLongShearCapacityRequiresStirrupTightening(false);

   GET_IFACE(IMaterials, pMaterials);
   bool bUHPC = IsUHPC(pMaterials->GetSegmentConcreteType(segmentKey));
   ATLASSERT(!bUHPC); // not supporting UHPC design at this time.

   bool bIter = true;
   while(bIter)
   {
      // Initialize shear design tool using flexure design pois
      PoiList vPoi;
      m_StrandDesignTool.GetDesignPoi(intervalIdx, 0, &vPoi);
      m_ShearDesignTool.ResetDesign( vPoi );

      // First step here is to perform a shear spec check. We will use the results later for
      // design if needed
      GET_IFACE(IShear,pShear);
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

      GET_IFACE(ILiveLoads,pLiveLoads);
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
            lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
            ATLASSERT(pool != nullptr);

            Float64 max_agg_size = pMaterials->GetSegmentMaxAggrSize(segmentKey); // for 1.33 max agg size for bar spacing
            Float64 fiber_length = pMaterials->GetSegmentConcreteFiberLength(segmentKey); // for 1.0 * max fiber length

            GET_IFACE(IGirder,pGirder);
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

               GET_IFACE(IMaterials,pMaterial);
               Float64 aone_strand = pMaterial->GetStrandMaterial(segmentKey, pgsTypes::Straight)->GetNominalArea(); // assume straight strands are used to make LRS tie

               Float64 nstrands = av_add/aone_strand; // Additional strands needed
               nstrands = CeilOff(nstrands, 1.0);

               StrandIndexType numNp = m_StrandDesignTool.GetNumPermanentStrands();
               StrandIndexType minNp = numNp + (StrandIndexType)nstrands - 1;
               StrandIndexType nextNp = m_StrandDesignTool.GetNextNumPermanentStrands(minNp);

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
                  if (m_StrandDesignTool.SetNumPermanentStrands(nextNp))
                  {
                     LOG(_T("Minimum number of strands set to control long reinf shear = ")<<nextNp);
                     m_StrandDesignTool.SetMinimumPermanentStrands(nextNp);
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

            if (fcreqd < m_StrandDesignTool.GetMaximumConcreteStrength())
            {
               m_StrandDesignTool.UpdateConcreteStrengthForShear(fcreqd, intervalIdx, pgsTypes::StrengthI);
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

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsDesigner2::AssertValid() const
{
   return true;
}

void pgsDesigner2::Dump(WBFL::Debug::LogContext& os) const
{
   os << _T("Dump for pgsDesigner2") << WBFL::Debug::endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsDesigner2::TestMe(WBFL::Debug::Log& rlog)
{
   TESTME_PROLOGUE("pgsDesigner2");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsDesigner2");

   TESTME_EPILOG("Designer");
}
#endif // _UNITTEST

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
   GET_IFACE(IProductForces,pProdForces);
   batTop    = pProdForces->GetBridgeAnalysisType(task.stressType == pgsTypes::Compression ? pgsTypes::Maximize : pgsTypes::Minimize);
   batBottom = pProdForces->GetBridgeAnalysisType(task.stressType == pgsTypes::Compression ? pgsTypes::Minimize : pgsTypes::Maximize);
}

void pgsDesigner2::DumpLiftingArtifact(const WBFL::Stability::LiftingStabilityProblem* pStabilityProblem,const WBFL::Stability::LiftingCheckArtifact& artifact,WBFL::Debug::LogContext& os) const
{
   os << _T("Dump for WBFL::Stability::LiftingCheckArtifact") << WBFL::Debug::endl;
   os << _T("===================================") << WBFL::Debug::endl;

   os <<_T(" Stress Artifacts")<< WBFL::Debug::endl;
   os << _T("================") << WBFL::Debug::endl;
   const WBFL::Stability::LiftingResults& results = artifact.GetLiftingResults();
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
