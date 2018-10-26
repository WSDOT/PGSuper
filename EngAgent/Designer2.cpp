///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
#include <IFace\PrecastIGirderDetailsSpec.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\GirderHandling.h>
#include <IFace\ResistanceFactors.h>

#include "Designer2.h"
#include "PsForceEng.h"
#include "GirderHandlingChecker.h"
#include <DesignConfigUtil.h>

#include "StatusItems.h"
#include <PgsExt\StatusItem.h>


#include "..\PGSuperException.h"

#include <Units\SysUnits.h>

#include <Lrfd\Rebar.h>
#include <algorithm>

#include <MathEx.h>

#include <PgsExt\BridgeDescription.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt\LoadFactors.h>
#include <PgsExt\GirderLabel.h>

#include <PsgLib\GirderLibraryEntry.h>
#include <PsgLib\SpecLibraryEntry.h>
#include <PsgLib\ConcreteLibraryEntry.h>


#if defined _DEBUG
#include <IFace\GirderHandlingPointOfInterest.h>
#endif // _DEBUG

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MIN_SPAN_DEPTH_RATIO 4

/****************************************************************************
CLASS
   pgsDesigner2
****************************************************************************/


// Allowable Stress Check Tasks
const pgsDesigner2::ALLOWSTRESSCHECKTASK g_AllowStressTask[] =
{
   {pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension},
   {pgsTypes::CastingYard,pgsTypes::ServiceI,  pgsTypes::Compression},
   {pgsTypes::CastingYard,pgsTypes::ServiceI,  pgsTypes::Tension},
   {pgsTypes::BridgeSite1,pgsTypes::ServiceI,  pgsTypes::Compression},
   {pgsTypes::BridgeSite1,pgsTypes::ServiceI,  pgsTypes::Tension},
   {pgsTypes::BridgeSite2,pgsTypes::ServiceI,  pgsTypes::Compression},
   {pgsTypes::BridgeSite3,pgsTypes::ServiceI,  pgsTypes::Compression},
   {pgsTypes::BridgeSite3,pgsTypes::ServiceIA, pgsTypes::Compression},
   {pgsTypes::BridgeSite3,pgsTypes::FatigueI, pgsTypes::Compression},
//   {pgsTypes::GirderPlacement,pgsTypes::ServiceI, pgsTypes::Tension},
//   {pgsTypes::GirderPlacement,pgsTypes::ServiceI, pgsTypes::Compression},
   {pgsTypes::TemporaryStrandRemoval,pgsTypes::ServiceI, pgsTypes::Tension},
   {pgsTypes::TemporaryStrandRemoval,pgsTypes::ServiceI, pgsTypes::Compression}
};

const Int16 g_cAllowStressTasks = sizeof(g_AllowStressTask)/sizeof(pgsDesigner2::ALLOWSTRESSCHECKTASK);

#if defined ENABLE_LOGGING
const std::_tstring g_Stage[] =
{
   std::_tstring(_T("CastingYard")),
   std::_tstring(_T("BridgeSite1")),
   std::_tstring(_T("BridgeSite2")),
   std::_tstring(_T("BridgeSite3")),
   std::_tstring(_T("Hauling")),
   std::_tstring(_T("Lifting")),
   std::_tstring(_T("Temporary Strand Removal")),
   std::_tstring(_T("Girder Placement"))
};

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
   if (sl==pgsTypes::BottomGirder)
      return _T(" Bottom of Girder");
   else
      return _T(" Top of Girder");
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

//// function to give envelope of two integer vectors (for debond levels)
//inline std::vector<DebondLevelType> EnvelopeDebondLevels(const std::vector<DebondLevelType>& rvec1, const std::vector<DebondLevelType>& rvec2)
//{
//   std::vector<DebondLevelType> result;
//   std::vector<DebondLevelType>::const_iterator r1it( rvec1.begin() );
//   std::vector<DebondLevelType>::const_iterator r2it( rvec2.begin() );
//
//   while (r1it!=rvec1.end() || r2it!=rvec2.end())
//   {
//      if (r1it!=rvec1.end() && r2it!=rvec2.end())
//      {
//         // both values at location
//         DebondLevelType v1 = *r1it;
//         DebondLevelType v2 = *r2it;
//         DebondLevelType imax = max(v1, v2);
//
//         result.push_back(imax);
//
//         r1it++;
//         r2it++;
//      }
//      else if (r1it!=rvec1.end())
//      {
//         // only r1
//         Int16 v1 = *r1it;
//         result.push_back(v1);
//
//         r1it++;
//      }
//      else if (r2it!=rvec2.end())
//      {
//         // only r2
//         Int16 v2 = *r2it;
//         result.push_back(v2);
//
//         r2it++;
//      }
//      else
//         ATLASSERT(0);
//   }
//
//   return result;
//}

static void GetConfinementZoneLengths(SpanIndexType span,GirderIndexType gdr, IGirder* pGdr, Float64 gdrLength, 
                                      Float64* pStartLength, Float64* pEndLength)
{
   // NOTE: This d is defined differently than in 5.10.10.2 of the 2nd 
   //       edition of the spec. We think what they really meant to say 
   //       was d = the overall depth of the precast member.
   // Get height at appropriate end of girder
   Float64 d = pGdr->GetHeight( pgsPointOfInterest(span,gdr, 0.0) );
   *pStartLength = 1.5*d;

   d = pGdr->GetHeight( pgsPointOfInterest(span,gdr, gdrLength) );
   *pEndLength = 1.5*d;
}

////////////////////////// PUBLIC     ///////////////////////////////////////



//======================== LIFECYCLE  =======================================
pgsDesigner2::pgsDesigner2():
m_StrandDesignTool(LOGGER),
m_ShearDesignTool(LOGGER)
{
   m_bShippingDesignWithEqualCantilevers = false;
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

void pgsDesigner2::GetHaunchDetails(SpanIndexType span,GirderIndexType gdr,HAUNCHDETAILS* pHaunchDetails)
{
   GDRCONFIG dummy_config;
   GetHaunchDetails(span,gdr,false,dummy_config,pHaunchDetails);
}

void pgsDesigner2::GetHaunchDetails(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG& config,HAUNCHDETAILS* pHaunchDetails)
{
   GetHaunchDetails(span,gdr,true,config,pHaunchDetails);
}

void pgsDesigner2::GetHaunchDetails(SpanIndexType span,GirderIndexType gdr,bool bUseConfig,const GDRCONFIG& config,HAUNCHDETAILS* pHaunchDetails)
{
   GET_IFACE(ICamber,pCamber);
   GET_IFACE(IPointOfInterest,pIPOI);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IRoadwayData,pRoadway);
   GET_IFACE(IRoadway,pAlignment);
   GET_IFACE(IGirder,pGdr);
   GET_IFACE(IEAFStatusCenter,pStatusCenter);

   GET_IFACE(ILibrary, pLib );
   GET_IFACE(ISpecification, pSpec );
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   pHaunchDetails->Haunch.clear();

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   Float64 fillet = pDeck->Fillet;

   std::vector<pgsPointOfInterest> vPoi( pIPOI->GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_TENTH_POINTS,POIFIND_OR) );
   ATLASSERT( vPoi.size() == 11 ); // 0.0L - 1.0L

   //
   // Profile Effects and Girder Orientation Effects
   //

   // slope of the girder in the plane of the girder
   Float64 girder_slope = pBridge->GetGirderSlope(span,gdr);
   pgsPointOfInterest& firstPoi = vPoi[0];

   // get station and offset of first poi
   Float64 station,offset;
   pBridge->GetStationAndOffset(firstPoi,&station,&offset);
   offset = IsZero(offset) ? 0 : offset;

   // the girder reference line passes through the deck at this station and offset
   Float64 Y_girder_ref_line_left_bearing = pAlignment->GetElevation(station,offset);

   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,gdr);

   MatingSurfaceIndexType nMatingSurfaces = pGdr->GetNumberOfMatingSurfaces(span,gdr);

   Float64 girder_orientation = pGdr->GetOrientation(span,gdr);

   Float64 max_tslab_and_fillet = 0;

   Float64 max_actual_haunch_depth_diff = 0; // maximum difference between "A" and the actual haunch depth at any section

   // determine the minumum and maximum difference in elevation between the
   // roadway surface and the top of the girder.... measured directly above 
   // the top of the girder
   Float64 diff_min =  100000;
   Float64 diff_max = -100000;
   Float64 max_reqd_haunch_depth = -100000;
   Float64 min_reqd_haunch_depth =  100000;
   std::vector<pgsPointOfInterest>::iterator iter( vPoi.begin() );
   std::vector<pgsPointOfInterest>::iterator iterEnd( vPoi.end() );
   for ( ; iter != iterEnd; iter++ )
   {
      pgsPointOfInterest& poi = *iter;

      Float64 slab_offset = (bUseConfig ? pBridge->GetSlabOffset(poi,config) : pBridge->GetSlabOffset(poi));

      Float64 tSlab = pBridge->GetGrossSlabDepth( poi );

      Float64 camber_effect = -999;
      if ( bUseConfig )
         camber_effect = pCamber->GetExcessCamber(poi, config, CREEP_MAXTIME );
      else
         camber_effect = pCamber->GetExcessCamber(poi, CREEP_MAXTIME );

      Float64 top_width = pGdr->GetTopWidth(poi);

      // top of girder elevation, including camber effects
      Float64 elev_top_girder = (bUseConfig ? pGdr->GetTopGirderElevation(poi,config,INVALID_INDEX) : pGdr->GetTopGirderElevation(poi,INVALID_INDEX) );

      // get station and normal offset for this poi
      Float64 x,z;
      pBridge->GetStationAndOffset(poi,&x,&z);
      z = IsZero(z) ? 0 : z;

      // top of girder elevation (ignoring camber effects)
      Float64 yc = pGdr->GetTopGirderReferenceChordElevation(poi);

      // top of alignment elevation above girder
      Float64 ya = pAlignment->GetElevation(x,z);

      // profile effect
      Float64 section_profile_effect = ya - yc;
      diff_min = _cpp_min(diff_min,section_profile_effect);
      diff_max = _cpp_max(diff_max,section_profile_effect);

      // girder orientation effect
      Float64 crown_slope = 0;
      if ( nMatingSurfaces == 1 )
      {
         // single top flange situation
         crown_slope = pAlignment->GetCrownSlope(x,z);

         if ( 0 < z )
            crown_slope *= -1;
      }
      else
      {
         // multiple mating surfaces (like a U-beam)
         // If there is a pivot point in the profile grade between the exterior mating surfaces
         // it is unclear which crown slope to use... to work around this, we will use the 
         // slope of the line connecting the two exterior mating surfaces
         ATLASSERT( 2 <= nMatingSurfaces );

         Float64 left_mating_surface_offset  = pGdr->GetMatingSurfaceLocation(poi,0);
         Float64 right_mating_surface_offset = pGdr->GetMatingSurfaceLocation(poi,nMatingSurfaces-1);

         Float64 ya_left  = pAlignment->GetElevation(x,z+left_mating_surface_offset);
         Float64 ya_right = pAlignment->GetElevation(x,z+right_mating_surface_offset);

         crown_slope = (ya_left - ya_right)/(right_mating_surface_offset - left_mating_surface_offset);
      }

      Float64 section_girder_orientation_effect = (top_width/2)*(fabs(crown_slope - girder_orientation)/(sqrt(1+girder_orientation*girder_orientation)));

      SECTIONHAUNCH haunch;
      haunch.PointOfInterest = poi;
      haunch.CamberEffect = camber_effect;
      haunch.CrownSlope = crown_slope;
      haunch.GirderOrientation = girder_orientation;
      haunch.ElevAlignment = ya;
      haunch.ElevGirder = yc;
      haunch.Fillet = fillet;
      haunch.GirderOrientationEffect = section_girder_orientation_effect;
      haunch.Offset = z;
      haunch.ProfileEffect = -section_profile_effect;
      haunch.Station = x;
      haunch.tSlab = tSlab;
      haunch.Wtop = top_width;
      haunch.ElevTopGirder = elev_top_girder;
      haunch.ActualHaunchDepth = haunch.ElevAlignment - haunch.ElevTopGirder;

      haunch.RequiredHaunchDepth = -(ya - yc - tSlab - fillet - section_girder_orientation_effect - camber_effect);

      max_reqd_haunch_depth = _cpp_max(max_reqd_haunch_depth,haunch.RequiredHaunchDepth);
      min_reqd_haunch_depth = _cpp_min(min_reqd_haunch_depth,haunch.RequiredHaunchDepth);

      pHaunchDetails->Haunch.push_back(haunch);

      max_tslab_and_fillet = _cpp_max(max_tslab_and_fillet,tSlab + fillet);
      
      max_actual_haunch_depth_diff = _cpp_max(max_actual_haunch_depth_diff, haunch.ActualHaunchDepth - slab_offset);
   }

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

   // Check against minimum haunch
   // This could happen if there was little camber, little cross slope, and a large crown
   if ( max_reqd_haunch_depth < max_tslab_and_fillet )
      max_reqd_haunch_depth = max_tslab_and_fillet;

   // record controlling values
   pHaunchDetails->RequiredSlabOffset      = max_reqd_haunch_depth;

   // this is the maximum difference in the haunch depth between the end of the girder and
   // any other point along the girder... if this too big, stirrups may need to be adjusted
   pHaunchDetails->HaunchDiff = max_actual_haunch_depth_diff;
}

pgsGirderArtifact pgsDesigner2::Check(SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(ILiveLoads,pLiveLoads);
   GET_IFACE(IEAFStatusCenter,pStatusCenter);

   pgsGirderArtifact gdr_artifact(span,gdr);

   CheckStrandStresses(span,gdr,gdr_artifact.GetStrandStressArtifact());

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(span,gdr,pgsTypes::Temporary);
   StrandIndexType Nt    = pStrandGeom->GetNumStrands(span,gdr,pgsTypes::Temporary);

   for (Int16 i = 0; i < g_cAllowStressTasks; i++ )
   {
      // skip the temporary strand removal check if the girder can't or doesn't have temporary strands
      if ( (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims && g_AllowStressTask[i].ls == pgsTypes::FatigueI) || 
           (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion()&& g_AllowStressTask[i].ls == pgsTypes::ServiceIA)
           )
      {
         // if before LRFD 2009 and Fatigue I 
         // - OR -
         // LRFD 2009 and later and Service IA
         //
         // ... don't evaluate this case
         continue;
      }

      if ( g_AllowStressTask[i].stage == pgsTypes::TemporaryStrandRemoval && (NtMax == 0 || Nt == 0) )
         continue;

      CheckGirderStresses(span,gdr,g_AllowStressTask[i],&gdr_artifact);
   }

// NOTE
// No longer designing/checking for ultimate moment in temporary construction state
// per e-mail from Bijan Khaleghi, dated 4/28/1999.  See project log.
//   CheckMomentCapacity(span,gdr,pgsTypes::BridgeSite1,pgsTypes::StrengthI,&gdr_artifact);

   if (!pLiveLoads->IsLiveLoadDefined(pgsTypes::lltDesign))
   {
      std::_tstring strMsg(_T("Live load are not defined."));
      pgsLiveLoadStatusItem* pStatusItem = new pgsLiveLoadStatusItem(m_StatusGroupID,m_scidLiveLoad,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   CheckMomentCapacity(span,gdr,pgsTypes::BridgeSite3,pgsTypes::StrengthI,&gdr_artifact);

   pgsStirrupCheckArtifact* pstirrup_artifact= gdr_artifact.GetStirrupCheckArtifact();

   // Get the basic shear poi's
   GET_IFACE(IPointOfInterest, pIPoi);
   std::vector<pgsPointOfInterest> shear_pois( pIPoi->GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_SHEAR | POI_TABULAR) );

   CheckShear(span,gdr,shear_pois,pgsTypes::StrengthI,NULL, pstirrup_artifact);

   // check for strength II only if permit vehicle is defined
   if (pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit))
   {
      CheckMomentCapacity(span,gdr,pgsTypes::BridgeSite3,pgsTypes::StrengthII,&gdr_artifact);
      CheckShear(span,gdr,shear_pois,pgsTypes::StrengthII,NULL,pstirrup_artifact);
   }

   CheckGirderDetailing(span,gdr,&gdr_artifact);

   CheckStrandSlope(span,gdr,gdr_artifact.GetStrandSlopeArtifact());
   CheckHoldDownForce(span,gdr,gdr_artifact.GetHoldDownForceArtifact());

   CheckConstructability(span,gdr,gdr_artifact.GetConstructabilityArtifact());
   CheckLiveLoadDeflection(span,gdr,gdr_artifact.GetDeflectionCheckArtifact());

   CheckDebonding(span,gdr,pgsTypes::Straight,gdr_artifact.GetDebondArtifact(pgsTypes::Straight));
   CheckDebonding(span,gdr,pgsTypes::Harped,gdr_artifact.GetDebondArtifact(pgsTypes::Harped));
   CheckDebonding(span,gdr,pgsTypes::Temporary,gdr_artifact.GetDebondArtifact(pgsTypes::Temporary));

   pgsGirderHandlingChecker handling_checker(m_pBroker,m_StatusGroupID);

   GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   if (pGirderLiftingSpecCriteria->IsLiftingCheckEnabled())
   {
      pgsLiftingCheckArtifact* pLiftingCheckArtifact = new(pgsLiftingCheckArtifact);

      handling_checker.CheckLifting(span,gdr,pLiftingCheckArtifact);
      gdr_artifact.SetLiftingCheckArtifact(pLiftingCheckArtifact);
   }

   GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   if (pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
   {
      pgsHaulingCheckArtifact* pHaulingCheckArtifact = new(pgsHaulingCheckArtifact);

      handling_checker.CheckHauling(span,gdr,pHaulingCheckArtifact);
      gdr_artifact.SetHaulingCheckArtifact(pHaulingCheckArtifact);
   }

   return gdr_artifact;
}

void CheckProgress(IProgress* pProgress)
{
   if ( pProgress->Continue() != S_OK )
   {
      //LOG(_T("*#*#*#*#* DESIGN CANCELLED BY USER *#*#*#*#*"));
      throw pgsDesignArtifact::DesignCancelled;
   }
}

#define CHECK_PROGRESS CheckProgress(pProgress)

pgsDesignArtifact pgsDesigner2::Design(SpanIndexType span,GirderIndexType gdr,arDesignOptions options)
{
   LOG(_T("************************************************************"));
   LOG(_T("Beginning design for span ") << LABEL_SPAN(span) << _T(" girder ") << LABEL_GIRDER(gdr));
#if defined ENABLE_LOGGING
   sysTime startTime;
#endif

   GET_IFACE(ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress,0,PW_ALL | PW_NOGAUGE); // progress window has a cancel button

   std::_tostringstream os;
   os << _T("Designing Span ") << LABEL_SPAN(span) << _T(" Girder ") << LABEL_GIRDER(gdr) << std::ends;
   pProgress->UpdateMessage(os.str().c_str());

   // Initialize the design artifact
   pgsDesignArtifact artifact(span,gdr);
   artifact.SetDesignOptions(options);

   // if not designing for lifting, set the lift values in the artifact to the
   // current values
   if ( !options.doDesignLifting )
   {
      GET_IFACE(IGirderLifting,pGirderLifting);
      Float64 Loh = pGirderLifting->GetLeftLiftingLoopLocation(span,gdr);
      Float64 Roh = pGirderLifting->GetRightLiftingLoopLocation(span,gdr);
      artifact.SetLiftingLocations(Loh,Roh);
   }

   if ( !options.doDesignHauling )
   {
      GET_IFACE(IGirderHauling,pGirderHauling);
      Float64 Loh = pGirderHauling->GetTrailingOverhang(span,gdr);
      Float64 Roh = pGirderHauling->GetLeadingOverhang(span,gdr);
      artifact.SetTruckSupportLocations(Loh,Roh);
   }

   // Copy current longitudinal rebar data to the artifact. 
   // This algorithm will only add more rebar to existing, and only
   // for the longitudinal reinf for shear condition
   GET_IFACE(IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);
   artifact.SetLongitudinalRebarData(girderData.LongitudinalRebarData);

   GET_IFACE(IGirder,pGdr);
   GET_IFACE(IBridge,pBridge);
   Float64 gird_length = pBridge->GetGirderLength(span, gdr);

   // Use strand design tool to control proportioning of strands
   m_StrandDesignTool.Initialize(m_pBroker, m_StatusGroupID, &artifact);

   Float64 startConfinementZl, endConfinementZl;
   GetConfinementZoneLengths(span, gdr, pGdr, gird_length, &startConfinementZl, &endConfinementZl);

   // Use shear design tool to control stirrup design
   m_ShearDesignTool.Initialize(m_pBroker, this, m_StatusGroupID, &artifact, startConfinementZl, endConfinementZl,
                                bPermit, options.doDesignStirrupLayout==slLayoutStirrups);

   // clear outcome codes
   m_DesignerOutcome.Reset();

   // don't do anything if nothing is asked
   if (options.doDesignForFlexure==dtNoDesign && 
       !options.doDesignForShear)
   {
      artifact.SetOutcome(pgsDesignArtifact::NoDesignRequested);
      return artifact;
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
      os2 << _T("Design Iteration ")<<cIter+1<<_T(" for Span ") << LABEL_SPAN(span) << _T(" Girder ") << LABEL_GIRDER(gdr) << std::ends;

      pProgress->UpdateMessage(os2.str().c_str());

      if (options.doDesignForFlexure!=dtNoDesign)
      {
         // reset outcomes
         bool keep_prop = false;
         if (m_DesignerOutcome.DidConcreteChange())
         {
            LOG(_T("Concrete changed on last iteration. Reset min slab offset to zero"));
            m_StrandDesignTool.SetMinimumSlabOffset(0.0);
         }

         keep_prop = m_DesignerOutcome.DidRetainStrandProportioning();
         m_DesignerOutcome.Reset();
         if (keep_prop)
         {
            LOG(_T("Retaining strand proportioning from last iteration"));
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::RetainStrandProportioning);
         }

         // get design back on track with user preferences
         m_StrandDesignTool.RestoreDefaults(m_DesignerOutcome.DidRetainStrandProportioning());

         // Design strands and concrete strengths in mid-zone
         DesignMidZone(cIter == 0 ? false : true, options,pProgress);

         if (m_DesignerOutcome.WasDesignAborted())
            return artifact;

         CHECK_PROGRESS;

         LOG(_T(""));
         LOG(_T("BEGINNING DESIGN OF END-ZONES"));
         LOG(_T(""));

         m_StrandDesignTool.DumpDesignParameters();

         // Design end zones
         DesignEndZone(cIter==0, options, artifact, pProgress);

         if ( m_DesignerOutcome.WasDesignAborted() )
         {
            return artifact;
         }
         else if ( m_DesignerOutcome.DidConcreteChange() )
         {
            LOG(_T("End Zone Design changed concrete strength - Restart"));
            LOG(_T("=================================================="));
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
            return artifact;
         else if  (  m_DesignerOutcome.DidConcreteChange())
            continue;

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
         RefineDesignForUltimateMoment(pgsTypes::BridgeSite3, pgsTypes::StrengthI,pProgress);

         CHECK_PROGRESS;

         if ( m_DesignerOutcome.WasDesignAborted() )
         {
            return artifact;
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
            RefineDesignForUltimateMoment(pgsTypes::BridgeSite3, pgsTypes::StrengthII,pProgress);

            CHECK_PROGRESS;
   
            if ( m_DesignerOutcome.WasDesignAborted() )
            {
               return artifact;
            }
            else if  (  m_DesignerOutcome.GetOutcome(pgsDesignCodes::ChangedForUltimate) )
            {
               LOG(_T("Ultimate moment controlled - restart design"));
               LOG(_T("==========================================="));
               continue;
            }
         }

         if (options.doDesignSlabOffset)
         {
            pProgress->UpdateMessage(_T("Designing Slab Offset Outer Loop"));

            LOG(_T("Starting Slab Offset design in outer loop"));
            Float64 old_offset_start = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart );
            Float64 old_offset_end   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd );

            DesignSlabOffset( pProgress );

            CHECK_PROGRESS;

            if (  m_DesignerOutcome.WasDesignAborted() )
            {
               return artifact;
            }
            else if ( m_DesignerOutcome.GetOutcome(pgsDesignCodes::SlabOffsetChanged) )
            {
               Float64 new_offset_start = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
               Float64 new_offset_end   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);

               new_offset_start = RoundSlabOffset(new_offset_start);
               new_offset_end   = RoundSlabOffset(new_offset_end);

               LOG(_T("Slab Offset changed in outer loop. Set a new minimum of (Start) ") << ::ConvertFromSysUnits(new_offset_start,unitMeasure::Inch)<< _T("in and (End) ") << ::ConvertFromSysUnits(new_offset_end,unitMeasure::Inch) << _T(" - restart design"));
               LOG(_T("========================================================================="));
               m_StrandDesignTool.SetMinimumSlabOffset( min(new_offset_start,new_offset_end));
               m_StrandDesignTool.SetSlabOffset(pgsTypes::metStart,new_offset_start);
               m_StrandDesignTool.SetSlabOffset(pgsTypes::metEnd, new_offset_end);
               continue;
            }
            else
            {
               m_StrandDesignTool.SetSlabOffset(pgsTypes::metStart,old_offset_start);  // restore to original value that passed all spec checks
               m_StrandDesignTool.SetSlabOffset(pgsTypes::metEnd,  old_offset_end);   // restore to original value that passed all spec checks
               LOG(_T("Slab Offset design Successful in outer loop. Current value is (Start) ") <<::ConvertFromSysUnits( m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart),unitMeasure::Inch)<<_T("in and (End) ")<<::ConvertFromSysUnits( m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd),unitMeasure::Inch) << _T(" in"));
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

      if (options.doDesignForShear)
      {
         //
         // Refine stirrup design
         // 
         pProgress->UpdateMessage(_T("Designing Shear Stirrups"));

         DesignShear(&artifact, options.doDesignStirrupLayout==slLayoutStirrups, options.doDesignForFlexure!=dtNoDesign);

         if ( m_DesignerOutcome.WasDesignAborted() )
            return artifact;
         else if  (  m_DesignerOutcome.DidGirderChange())
            continue;
      }

      // we've succussfully completed all the design steps
      // we are DONE!
      bDone = true;
   } while ( cIter < nIterMax && !bDone );

   if ( !bDone ) //&& cIter >= nIterMax )
   {
      LOG(_T("Maximum number of iteratations was exceeded - aborting design ") << cIter);
      artifact.SetOutcome(pgsDesignArtifact::MaxIterExceeded);
      return artifact;
   }

   if (artifact.GetDesignOptions().doDesignSlabOffset)
   {
      LOG(_T("Final Slab Offset before rounding (Start) ") << ::ConvertFromSysUnits( artifact.GetSlabOffset(pgsTypes::metStart),unitMeasure::Inch) << _T(" in and (End) ") << ::ConvertFromSysUnits( artifact.GetSlabOffset(pgsTypes::metEnd),unitMeasure::Inch) << _T(" in"));
      Float64 start_offset = RoundSlabOffset(artifact.GetSlabOffset(pgsTypes::metStart));
      Float64 end_offset   = RoundSlabOffset(artifact.GetSlabOffset(pgsTypes::metEnd));
      artifact.SetSlabOffset(pgsTypes::metStart,start_offset);
      artifact.SetSlabOffset(pgsTypes::metEnd, end_offset);
      LOG(_T("After rounding (Start) ") << ::ConvertFromSysUnits(start_offset,unitMeasure::Inch) << _T(" in and (End) ") << ::ConvertFromSysUnits(end_offset,unitMeasure::Inch) << _T(" in"));
   }


   m_StrandDesignTool.DumpDesignParameters();

   pProgress->UpdateMessage(_T("Design Complete"));
   LOG(_T("Design Complete for span ") << LABEL_SPAN(span) << _T(" girder ") << LABEL_GIRDER(gdr));
   LOG(_T("************************************************************"));
#if defined ENABLE_LOGGING
   sysTime endTime;
   long duration = endTime.Seconds() - startTime.Seconds();
   long min = duration / 60;
   long sec = duration - min*60;
   LOG(_T("Design: ") << min << _T("m:") << sec << _T("s"));
#endif

   // set controlling data for concrete strengths
   artifact.SetReleaseDesignState(m_StrandDesignTool.GetReleaseConcreteDesignState());
   artifact.SetFinalDesignState(m_StrandDesignTool.GetFinalConcreteDesignState());
   artifact.SetOutcome(pgsDesignArtifact::Success);

   return artifact;
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

void pgsDesigner2::CheckStrandStresses(SpanIndexType span,GirderIndexType gdr,pgsStrandStressArtifact* pArtifact)
{
#pragma Reminder("UPDATE: Allowable stress in PT temp strands needs to be checked")
   GET_IFACE(IAllowableStrandStress,pAllow);
   GET_IFACE(IPrestressForce, pPsForce);
   GET_IFACE(IPointOfInterest,pIPOI);

   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);

   std::_tostringstream os;
   os << _T("Checking strand stresses for Span ")
      << LABEL_SPAN(span) << _T(" Girder ")
      << LABEL_GIRDER(gdr) << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );
   
   std::vector<pgsPointOfInterest> vPOI;
   pgsPointOfInterest poi;

   vPOI = pIPOI->GetPointsOfInterest(span,gdr,pgsTypes::CastingYard,POI_MIDSPAN);
   poi = *vPOI.begin();

   pArtifact->SetPointOfInterest(poi);

   GET_IFACE(IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(poi.GetSpan(),poi.GetGirder());

   std::vector<pgsTypes::StrandType> strandTypes;
   if ( girderData.NumPermStrandsType == NPS_TOTAL_NUMBER )
   {
      strandTypes.push_back(pgsTypes::Permanent);
   }
   else
   {
      strandTypes.push_back(pgsTypes::Straight);
      strandTypes.push_back(pgsTypes::Harped);
   }

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType Nt = pStrandGeom->GetNumStrands(poi.GetSpan(),poi.GetGirder(),pgsTypes::Temporary);
   if ( 0 < Nt )
   {
      strandTypes.push_back(pgsTypes::Temporary);
   }

   std::vector<pgsTypes::StrandType>::iterator standTypeIter(strandTypes.begin());
   std::vector<pgsTypes::StrandType>::iterator standTypeIterEnd(strandTypes.end());
   for ( ; standTypeIter != standTypeIterEnd; standTypeIter++ )
   {
      pgsTypes::StrandType strandType = *standTypeIter;

      if ( pAllow->CheckStressAtJacking() )
         pArtifact->SetCheckAtJacking( strandType, pPsForce->GetStrandStress(poi,strandType,pgsTypes::Jacking), pAllow->GetAllowableAtJacking(span,gdr,strandType) );

      if ( pAllow->CheckStressBeforeXfer() )
         pArtifact->SetCheckBeforeXfer( strandType, pPsForce->GetStrandStress(poi,strandType,pgsTypes::BeforeXfer), pAllow->GetAllowableBeforeXfer(span,gdr,strandType) );

      if ( pAllow->CheckStressAfterXfer() )
         pArtifact->SetCheckAfterXfer( strandType, pPsForce->GetStrandStress(poi,strandType,pgsTypes::AfterXfer), pAllow->GetAllowableAfterXfer(span,gdr,strandType) );

      if ( pAllow->CheckStressAfterLosses() && strandType != pgsTypes::Temporary )
         pArtifact->SetCheckAfterLosses( strandType, pPsForce->GetStrandStress(poi,strandType,pgsTypes::AfterLossesWithLiveLoad), pAllow->GetAllowableAfterLosses(span,gdr,strandType) );
   }
}

void pgsDesigner2::CheckGirderStresses(SpanIndexType span,GirderIndexType gdr,ALLOWSTRESSCHECKTASK task,pgsGirderArtifact* pGdrArtifact)
{
   USES_CONVERSION;

   GET_IFACE(IPointOfInterest, pIPoi);
   GET_IFACE(IPrestressStresses, pPrestressStresses);
   GET_IFACE(ILimitStateForces, pLimitStateForces);
   GET_IFACE(IAllowableConcreteStress, pAllowable );
   GET_IFACE(IGirder,pGirder);
   GET_IFACE(ISectProp2,pSectProp2);
   GET_IFACE(IBridgeMaterial,pMaterial);
   GET_IFACE(ISpecification,pSpec);
   GET_IFACE(IContinuity,pContinuity);

   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(span,gdr,task.stage,POI_FLEXURESTRESS | POI_TABULAR) );

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   bool bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   Float64 Es, fy, fu;
   pMaterial->GetLongitudinalRebarProperties(span,gdr,&Es,&fy,&fu);
   Float64 fs = 0.5*fy;
   Float64 fsMax = (bUnitsSI ? ::ConvertToSysUnits(206.0,unitMeasure::MPa) : ::ConvertToSysUnits(30.0,unitMeasure::KSI) );
   if ( fsMax < fs )
       fs = fsMax;


   Float64 AsMax = 0;

   BridgeAnalysisType batTop, batBottom;
   GetBridgeAnalysisType(gdr,task,batTop,batBottom);

   GET_IFACE(IStageMap,pStageMap);
   GET_IFACE(IProgress, pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Checking Girder Stresses"));

   std::vector<pgsPointOfInterest>::iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++)
   {
      const pgsPointOfInterest& poi = *poiIter;

      pgsFlexuralStressArtifactKey key(task.stage,task.ls,task.type,poi.GetDistFromStart());
      pgsFlexuralStressArtifact artifact;

      // get girder stress due to prestressing
      Float64 fTopPrestress, fBotPrestress;
      fTopPrestress = pPrestressStresses->GetStress(task.stage,poi,pgsTypes::TopGirder);
      fBotPrestress = pPrestressStresses->GetStress(task.stage,poi,pgsTypes::BottomGirder);

      // get girder stress due to external loads (top)
      Float64 fTopLimitStateMin, fTopLimitStateMax;
      pLimitStateForces->GetStress(task.ls,task.stage,poi,pgsTypes::TopGirder,false,batTop,&fTopLimitStateMin,&fTopLimitStateMax);
      Float64 fTopLimitState = (task.type == pgsTypes::Compression ? fTopLimitStateMin : fTopLimitStateMax );

      // get girder stress due to external loads (bottom)
      Float64 fBotLimitStateMin, fBotLimitStateMax;
      pLimitStateForces->GetStress(task.ls,task.stage,poi,pgsTypes::BottomGirder,false,batBottom,&fBotLimitStateMin,&fBotLimitStateMax);
      Float64 fBotLimitState = (task.type == pgsTypes::Compression ? fBotLimitStateMin : fBotLimitStateMax );

      // get allowable stress
      Float64 fAllowComp, fAllowTens;
      if ( task.ls != pgsTypes::ServiceIII )
         fAllowComp = pAllowable->GetAllowableStress(poi,task.stage,task.ls,pgsTypes::Compression);

      if ( task.stage != pgsTypes::BridgeSite2 )
      	fAllowTens = pAllowable->GetAllowableStress(poi,task.stage,task.ls,pgsTypes::Tension);

      Float64 fAllowable = (task.type == pgsTypes::Compression ? fAllowComp : fAllowTens);

      Float64 k;
      if (task.ls == pgsTypes::ServiceIA || task.ls == pgsTypes::FatigueI )
         k = 0.5; // Use half prestress stress if service IA (See Tbl 5.9.4.2.1-1)
      else
         k = 1.0;
      
      Float64 fTop = fTopLimitState + k*fTopPrestress;
      Float64 fBot = fBotLimitState + k*fBotPrestress;
      artifact.SetDemand( fTop, fBot );
      artifact.SetCapacity(fAllowable,task.type);
      artifact.SetExternalEffects(fTopLimitState,fBotLimitState);
      artifact.SetPrestressEffects(fTopPrestress,fBotPrestress);

      // determine what concrete strength (if any) would work for this section. 
      // what concrete strength is required to satisify the allowable stress criteria
      // if there isn't a strength that works, use a value of -1
      if ( task.type == pgsTypes::Compression )
      {
         Float64 c = pAllowable->GetAllowableCompressiveStressCoefficient(task.stage,task.ls);
         Float64 fc_reqd = (IsZero(c) ? 0 : _cpp_min(fTop,fBot)/-c);
         
         if ( fc_reqd < 0 ) // the minimum stress is tensile so compression isn't an issue
            fc_reqd = 0;

         artifact.SetRequiredConcreteStrength(fc_reqd);
      }
      else
      {
         Float64 t;
         bool bCheckMax;
         Float64 fmax;

         pAllowable->GetAllowableTensionStressCoefficient(task.stage,task.ls,&t,&bCheckMax,&fmax);

         // if this is bridge site 3, only look at the bottom stress (stress in the precompressed tensile zone)
         // otherwise, take the controlling tension
         Float64 f = (task.stage == pgsTypes::BridgeSite3 ? fBot : _cpp_max(fTop,fBot));

         Float64 fc_reqd;
         if (f>0.0)
         {
            fc_reqd = (IsZero(t) ? 0 : BinarySign(f)*pow(f/t,2));
         }
         else
         {
            // the maximum stress is compressive so tension isn't an issue
            fc_reqd = 0;
         }

         if ( bCheckMax &&                  // allowable stress is limited -AND-
              (0 < fc_reqd) &&              // there is a concrete strength that might work -AND-
              (pow(fmax/t,2) < fc_reqd) )   // that strength will exceed the max limit on allowable
         {
            // then that concrete strength wont really work afterall
            if ( task.stage == pgsTypes::CastingYard )
            {
               // unless we are in the casting yard, then we can add some additional rebar
               // and go to a higher limit
               Float64 talt = pAllowable->GetCastingYardAllowableTensionStressCoefficientWithRebar();
               fc_reqd = pow(f/talt,2);
            }
            else
            {
               // too bad... this isn't going to work
               fc_reqd = -1;
            }
         }
         artifact.SetRequiredConcreteStrength(fc_reqd);
      }

      // Determine mild steel requirement for alternative tensile stress (casting yard only)
      if ( task.stage == pgsTypes::CastingYard )
      {
         Float64 Yna = -1;
         Float64 Area = 0;
         Float64 H = pGirder->GetHeight(poi);

         Float64 T = 0;
         if ( fTop <= 0 && fBot <= 0 )
         {
             // compression over entire cross section
            T = 0;
         }
         else if ( 0 <= fTop && 0 <= fBot )
         {
             // tension over entire cross section
             Area = pSectProp2->GetAg(pgsTypes::CastingYard,poi);
             Float64 fAvg = (fTop + fBot)/2;
             T = fAvg * Area;

             ATLASSERT( T != 0 );
         }
         else
         {
            ATLASSERT( BinarySign(fBot) != BinarySign(fTop) );

            // Location of neutral axis from Bottom of Girder
            Yna = (IsZero(fBot) ? 0 : H - (fTop*H/(fTop-fBot)) );

            ATLASSERT( 0 <= Yna );

            CComPtr<IShape> shape;
            pSectProp2->GetGirderShape(poi,false,&shape);

            CComQIPtr<IXYPosition> position(shape);
            CComPtr<IPoint2d> bc;
            position->get_LocatorPoint(lpBottomCenter,&bc);
            Float64 Y;
            bc->get_Y(&Y);

            CComPtr<ILine2d> line;
            line.CoCreateInstance(CLSID_Line2d);
            CComPtr<IPoint2d> p1, p2;
            p1.CoCreateInstance(CLSID_Point2d);
            p2.CoCreateInstance(CLSID_Point2d);
            p1->Move(-10000,Y+Yna);
            p2->Move( 10000,Y+Yna);

            Float64 fAvg;

            // line clips away left hand side
            if ( 0 <= fTop && fBot <= 0 )
            {
                // Tension top, compression bottom
                // line needs to go right to left
               line->ThroughPoints(p2,p1);

               fAvg = fTop / 2;
            }
            else if ( fTop <= 0 && 0 <= fBot )
            {
                // Compression Top, Tension Bottom
                // line needs to go left to right
               line->ThroughPoints(p1,p2);

               fAvg = fBot / 2;
            }

            CComPtr<IShape> clipped_shape;
            shape->ClipWithLine(line,&clipped_shape);

            if ( clipped_shape )
            {
               CComPtr<IShapeProperties> props;
               clipped_shape->get_ShapeProperties(&props);

               props->get_Area(&Area);
            }
            else
            {
               Area = 0;
            }

            T = fAvg * Area;

            ATLASSERT( T != 0 );
         }

         Float64 As = T/fs;
         ATLASSERT( 0 <= As );

         artifact.IsAlternativeTensileStressApplicable(true);
         artifact.SetAlternativeTensileStressParameters(Yna,Area,T,As,pAllowable->GetCastingYardWithMildRebarAllowableStress(span,gdr));
         AsMax = _cpp_max(As,AsMax);
      }

      artifact.SetKey(key);

      pGdrArtifact->AddFlexuralStressArtifact(key,artifact);
   }

   if ( task.stage == pgsTypes::CastingYard && task.type == pgsTypes::Tension )
   {
       pGdrArtifact->SetCastingYardMildRebarRequirement(AsMax);
       pGdrArtifact->SetCastingYardCapacityWithMildRebar(pAllowable->GetCastingYardWithMildRebarAllowableStress(span,gdr));
   }

}

pgsFlexuralCapacityArtifact pgsDesigner2::CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,pgsTypes::Stage stage,pgsTypes::LimitState ls,const GDRCONFIG& config,bool bPositiveMoment)
{
   GET_IFACE(IMomentCapacity, pMomentCapacity);

   MOMENTCAPACITYDETAILS mcd;
   pMomentCapacity->GetMomentCapacityDetails( stage, poi, config, bPositiveMoment,&mcd );

   MINMOMENTCAPDETAILS mmcd;
   pMomentCapacity->GetMinMomentCapacityDetails(stage, poi, config, bPositiveMoment, &mmcd);

   return CreateFlexuralCapacityArtifact(poi,stage,ls,bPositiveMoment,mcd,mmcd);
}

pgsFlexuralCapacityArtifact pgsDesigner2::CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,pgsTypes::Stage stage,pgsTypes::LimitState ls,bool bPositiveMoment)
{
   GET_IFACE(IMomentCapacity, pMomentCapacity);

   MOMENTCAPACITYDETAILS mcd;
   pMomentCapacity->GetMomentCapacityDetails( stage, poi, bPositiveMoment, &mcd );

   MINMOMENTCAPDETAILS mmcd;
   pMomentCapacity->GetMinMomentCapacityDetails(stage, poi, bPositiveMoment, &mmcd);

   return CreateFlexuralCapacityArtifact(poi,stage,ls,bPositiveMoment,mcd,mmcd);
}

pgsFlexuralCapacityArtifact pgsDesigner2::CreateFlexuralCapacityArtifact(const pgsPointOfInterest& poi,pgsTypes::Stage stage,pgsTypes::LimitState ls,bool bPositiveMoment,const MOMENTCAPACITYDETAILS& mcd,const MINMOMENTCAPDETAILS& mmcd)
{
   GET_IFACE(ILimitStateForces, pLimitStateForces);
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification, pSpec);

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool c_over_de = ( pSpec->GetMomentCapacityMethod() == LRFD_METHOD && pSpecEntry->GetSpecificationType() < lrfdVersionMgr::ThirdEditionWith2006Interims );
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   pgsFlexuralCapacityArtifact artifact;

   Float64 Mu;
   if ( bPositiveMoment )
   {
      Float64 MuMin, MuMax;
      if ( analysisType == pgsTypes::Envelope )
      {
         Float64 min,max;
         pLimitStateForces->GetMoment(ls,stage,poi,MaxSimpleContinuousEnvelope,&min,&max);
         MuMax = max;

         pLimitStateForces->GetMoment(ls,stage,poi,MinSimpleContinuousEnvelope,&min,&max);
         MuMin = min;
      }
      else
      {
         pLimitStateForces->GetMoment(ls,stage,poi,analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan,&MuMin,&MuMax);
      }

      Mu = MuMax;
   }
   else
   {
      if ( analysisType == pgsTypes::Envelope )
         Mu = pLimitStateForces->GetSlabDesignMoment(ls,poi,MinSimpleContinuousEnvelope);
      else
         Mu = pLimitStateForces->GetSlabDesignMoment(ls,poi,analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
   }

   artifact.SetCapacity( mcd.Phi * mcd.Mn );
   artifact.SetDemand( Mu );
   artifact.SetMinCapacity( mmcd.MrMin );

   // When capacity is zero, there is no reinforcing ratio.
   // We need to simulate some numbers so everything works.
   // Also simulate numbers if this is 2006 LRFD or later... c/de has been removed from the LRFD spec
   Float64 c_de;
   if ( c_over_de && !IsZero(mcd.de) )
   {
      c_de = mcd.c/mcd.de;
   }
   else
   {
      c_de = 0.0;
   }

   artifact.SetMaxReinforcementRatio( c_de );
   artifact.SetMaxReinforcementRatioLimit(0.42);  // 5.7.3.3.1

   return artifact;
}

pgsStirrupCheckAtPoisArtifact pgsDesigner2::CreateStirrupCheckAtPoisArtifact(const pgsPointOfInterest& poi,pgsTypes::Stage stage,pgsTypes::LimitState ls, Float64 vu,
                                                                            Float64 fcSlab,Float64 fcGdr, Float64 fy, bool checkConfinement,const GDRCONFIG* pConfig)
{
   CHECK(stage==pgsTypes::BridgeSite3);
   CHECK(ls==pgsTypes::StrengthI || ls == pgsTypes::StrengthII);

   GET_IFACE(IEAFStatusCenter,pStatusCenter);
   GET_IFACE(IBridge,pBridge);

   // throw an exception if span length is too short
   if (IsDeepSection( poi ))
   {
      Int32 reason = XREASON_AGENTVALIDATIONFAILURE;
      std::_tostringstream os;
      os << _T("Cannot perform shear check - Span-to-Depth ratio is less than ")<< MIN_SPAN_DEPTH_RATIO <<_T(" for Span ")<< LABEL_SPAN(poi.GetSpan()) << _T(" Girder ") << LABEL_GIRDER(poi.GetGirder()) << _T(" (See LRFD 5.8.1.1)");

      pgsBridgeDescriptionStatusItem* pStatusItem = new pgsBridgeDescriptionStatusItem(m_StatusGroupID,m_scidBridgeDescriptionError,0,os.str().c_str());
      pStatusCenter->Add(pStatusItem);

      os << std::endl << _T("See Status Center for Details");
      THROW_UNWIND(os.str().c_str(),reason);
   }

   GET_IFACE(IShearCapacity, pShearCapacity);

   SHEARCAPACITYDETAILS scd;
   if(pConfig!=NULL)
   {
      pShearCapacity->GetShearCapacityDetails( ls, stage, poi, *pConfig, &scd );
   }
   else
   {
      pShearCapacity->GetShearCapacityDetails( ls, stage, poi, &scd );
   }

   // vertical shear
   pgsVerticalShearArtifact v_artifact;
   CheckStirrupRequirement( poi, scd, &v_artifact );
   CheckUltimateShearCapacity( poi, scd, vu, pConfig, &v_artifact );

   // horizontal shear
   pgsHorizontalShearArtifact h_artifact;
   if ( pBridge->IsCompositeDeck() )
      CheckHorizontalShear(poi,vu,fcSlab,fcGdr,fy, pConfig,&h_artifact);

   // stirrup detail check
   const STIRRUPCONFIG* pStirrupConfig = (pConfig==NULL) ? NULL : &(pConfig->StirrupConfig);

   pgsStirrupDetailArtifact d_artifact;
   CheckFullStirrupDetailing(poi,v_artifact,scd,vu,fcGdr,fy,pStirrupConfig,&d_artifact);

   // longitudinal steel check
   pgsLongReinfShearArtifact l_artifact;
   CheckLongReinfShear(poi,stage,ls,scd,pConfig,&l_artifact);

   // create the artifact and return it
   pgsStirrupCheckAtPoisArtifact artifact;
   artifact.SetVerticalShearArtifact(v_artifact);
   artifact.SetHorizontalShearArtifact(h_artifact);
   artifact.SetStirrupDetailArtifact(d_artifact);
   artifact.SetLongReinfShearArtifact(l_artifact);

   return artifact;
}

bool pgsDesigner2::IsDeepSection( const pgsPointOfInterest& poi)
{
   // LRFD 5.8.1.1
   // Assume that the point of zero shear is at mid-span (L/2).
   // This assumption is true for uniform load which generally occur for this type of structure.
   // From 5.8.1.1, the beam is considered a deep beem if the distance from the point of zero shear
   // to the face of support is less than 2d.
   //
   // L/2 < 2d 
   // Re-arrange
   // L/d < 4
   GET_IFACE(IBridge,pBridge);
   Float64 span_length = pBridge->GetSpanLength(poi.GetSpan(), poi.GetGirder());
   GET_IFACE(IGirder,pGdr);
   Float64 beam_depth = pGdr->GetHeight(poi);

   Float64 ratio = span_length/beam_depth;
   return ( ratio < Float64(MIN_SPAN_DEPTH_RATIO));
}

void pgsDesigner2::CheckStirrupRequirement( const pgsPointOfInterest& poi, const SHEARCAPACITYDETAILS& scd, pgsVerticalShearArtifact* pArtifact )
{
   pArtifact->SetAreStirrupsReqd(scd.bStirrupsReqd);
   pArtifact->SetAreStirrupsProvided(scd.Av > 0.0);
}

void pgsDesigner2::CheckUltimateShearCapacity( const pgsPointOfInterest& poi, const SHEARCAPACITYDETAILS& scd, Float64 vu, const GDRCONFIG* pConfig, pgsVerticalShearArtifact* pArtifact )
{
   bool bCheck = true;
   if ( m_bSkipShearCheckBeforeLeftCS && poi.GetDistFromStart() < m_LeftCS.GetDistFromStart() )
      bCheck = false;

   if ( m_bSkipShearCheckAfterRightCS && m_RightCS.GetDistFromStart() < poi.GetDistFromStart() )
      bCheck = false;

   if ( bCheck )
   {
      pArtifact->IsApplicable(true);
      pArtifact->SetCapacity( scd.pVn );
      pArtifact->SetDemand( vu );

   }
   else
   {
      // strength check is not applicable for this poi
      pArtifact->IsApplicable(false);

      // note if strut and tie analysis is required
      pArtifact->IsStrutAndTieRequired(pgsTypes::metStart,m_bLeftCS_StrutAndTieRequired);
      pArtifact->IsStrutAndTieRequired(pgsTypes::metEnd,  m_bRightCS_StrutAndTieRequired);

      GET_IFACE(IStirrupGeometry,pStirrupGeom);
      SpanIndexType span  = poi.GetSpan();
      GirderIndexType gdr = poi.GetGirder();

      // the shear reinforcement must be at least as much as at the critical section
      // See LRFD C5.8.3.2 (since the stress in the stirrups doesn't change between
      // the support and the critical section, there should be at least as much 
      // reinforcement between the end and the CS as there is at the CS)
      Float64 AvS_provided = scd.Av/scd.S;
      Float64 AvS_at_CS;
      if ( !m_bLeftCS_StrutAndTieRequired && poi.GetDistFromStart() < m_LeftCS.GetDistFromStart() )
      {
         Float64 s;
         matRebar::Size size;
         Float64 abar, nl;
         if (pConfig!=NULL)
         {
            // Use design config
            GET_IFACE(IBridge, pBridge);
            Float64 gdr_length = pBridge->GetGirderLength(span,gdr);
            Float64 location = poi.GetDistFromStart();
            Float64 lft_supp_loc = pBridge->GetGirderStartConnectionLength(span,gdr);
            Float64 rgt_sup_loc = gdr_length - pBridge->GetGirderEndConnectionLength(span,gdr);

            AvS_at_CS = GetPrimaryStirrupAvs(pConfig->StirrupConfig, getVerticalStirrup, m_LeftCS.GetDistFromStart(), gdr_length, 
                                             lft_supp_loc, rgt_sup_loc, &size, &abar, &nl, &s);
         }
         else
         {
            // Use current bridge data
            AvS_at_CS = pStirrupGeom->GetVertStirrupAvs(m_LeftCS, &size, &abar, &nl, &s);
         }

         pArtifact->SetEndSpacing(pgsTypes::metStart,AvS_provided,AvS_at_CS);
      }
      else if ( !m_bRightCS_StrutAndTieRequired && m_RightCS.GetDistFromStart() < poi.GetDistFromStart() )
      {
         Float64 s;
         matRebar::Size size;
         Float64 abar, nl;
         if (pConfig!=NULL)
         {
            // Use design config
            GET_IFACE(IBridge, pBridge);
            Float64 gdr_length = pBridge->GetGirderLength(span,gdr);
            Float64 location = poi.GetDistFromStart();
            Float64 lft_supp_loc = pBridge->GetGirderStartConnectionLength(span,gdr);
            Float64 rgt_sup_loc = gdr_length - pBridge->GetGirderEndConnectionLength(span,gdr);

            AvS_at_CS = GetPrimaryStirrupAvs(pConfig->StirrupConfig, getVerticalStirrup, m_RightCS.GetDistFromStart(), gdr_length, 
                                             lft_supp_loc, rgt_sup_loc, &size, &abar, &nl, &s);
         }
         else
         {
            AvS_at_CS = pStirrupGeom->GetVertStirrupAvs(m_RightCS, &size, &abar, &nl, &s);
         }

         pArtifact->SetEndSpacing(pgsTypes::metEnd,AvS_provided,AvS_at_CS);
      }
   }

   pArtifact->SetAvOverSReqd( scd.AvOverS_Reqd ); // leave a nugget for shear design algorithm
}

void pgsDesigner2::CheckHorizontalShear(const pgsPointOfInterest& poi, 
                                       Float64 vu, 
                                       Float64 fcSlab,Float64 fcGdr, Float64 fy,
                                       const GDRCONFIG* pConfig,
                                       pgsHorizontalShearArtifact* pArtifact )
{
   GET_IFACE(ISectProp2,pSectProp2);
   GET_IFACE(IGirder,pGdr);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeMaterialEx,pMaterial);
   GET_IFACE(IProductForces,pProductForces);
   GET_IFACE(IStrandGeometry,pStrandGeom);

   // determine shear demand
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Float64 Vuh;

   if ( pSpecEntry->GetShearFlowMethod() == sfmClassical )
   {
      Float64 Qslab = pSectProp2->GetQSlab(poi); // Note: A possible problem here - QSlab is slightly dependent on fcGdr
      ATLASSERT(Qslab!=0);

      Float64 Ic  = pSectProp2->GetIx(pgsTypes::BridgeSite3,poi,fcGdr);
      Vuh  = vu*Qslab/Ic;

      pArtifact->SetI( Ic );
      pArtifact->SetQ( Qslab );
   }
   else
   {
      Float64 nEffStrands;
      Float64 ecc;
      if (pConfig!=NULL)
      {
         ecc = pStrandGeom->GetEccentricity(poi,*pConfig,false,&nEffStrands); // based on non-composite cg
      }
      else
      {
         ecc = pStrandGeom->GetEccentricity(poi,false,&nEffStrands); // based on non-composite cg
      }
      Float64 Yt = pSectProp2->GetYtGirder(pgsTypes::BridgeSite1,poi,fcGdr); // non-composite girder
      Float64 tSlab = pBridge->GetStructuralSlabDepth(poi);

      Float64 dv = ecc + Yt + tSlab/2;

      Vuh  = vu / dv;

      pArtifact->SetDv( dv );
   }

   pArtifact->SetVu( vu );
   pArtifact->SetDemand(Vuh);

   // normal force on top of girder flange
   Float64 Pc = GetNormalFrictionForce(poi);
   pArtifact->SetNormalCompressionForce(Pc);

   // Area of shear transfer
   Float64 Acv = pGdr->GetShearInterfaceWidth( poi );
   pArtifact->SetAcv(Acv);

   // Take minimum concrete strength at interface
   Float64 fc = min(fcSlab,fcGdr);
   pArtifact->SetFc( fc );

   // area of reinforcement crossing shear plane
   // girder stirrups
   if (pConfig!=NULL)
   {
      // Use design config
      SpanIndexType span  = poi.GetSpan();
      GirderIndexType gdr = poi.GetGirder();

      GET_IFACE(IBridge, pBridge);
      Float64 gdr_length = pBridge->GetGirderLength(span,gdr);
      Float64 location = poi.GetDistFromStart();
      Float64 lft_supp_loc = pBridge->GetGirderStartConnectionLength(span,gdr);
      Float64 rgt_sup_loc = gdr_length - pBridge->GetGirderEndConnectionLength(span,gdr);

      Float64 Sg;
      matRebar::Size size;
      Float64 abar, nPrimaryLegs;
      Float64 Avs = GetPrimaryStirrupAvs(pConfig->StirrupConfig, getHorizShearStirrup, poi.GetDistFromStart(),
                                 gdr_length, lft_supp_loc,rgt_sup_loc,
                                 &size, &abar, &nPrimaryLegs, &Sg);

      pArtifact->SetAvfGirder(abar*nPrimaryLegs);
      pArtifact->SetSGirder(Sg);

      // additional hi stirrups
      Float64 nAdditionalLegs;
      Float64 Avftf = GetAdditionalHorizInterfaceAvs(pConfig->StirrupConfig, poi.GetDistFromStart(),
                                 gdr_length, lft_supp_loc,rgt_sup_loc,
                                 &size, &abar, &nAdditionalLegs, &Sg);

      pArtifact->SetAvfAdditional(abar*nAdditionalLegs);
      pArtifact->SetSAdditional(Sg);

      // legs per stirrup
      Float64 num_legs = nPrimaryLegs + nAdditionalLegs;

      pArtifact->SetNumLegs(num_legs);
   }
   else
   {
      // Use current girder model data
      GET_IFACE(IStirrupGeometry, pStirrupGeometry);
      Float64 Sg;
      matRebar::Size size;
      Float64 abar, nl;
      Float64 Avs = pStirrupGeometry->GetPrimaryHorizInterfaceAvs(poi, &size, &abar, &nl, &Sg);

      pArtifact->SetAvfGirder(abar*nl);
      pArtifact->SetSGirder(Sg);

      // addtional hi stirrups
      Float64 Avftf = pStirrupGeometry->GetAdditionalHorizInterfaceAvs(poi, &size, &abar, &nl, &Sg);

      pArtifact->SetAvfAdditional(abar*nl);
      pArtifact->SetSAdditional(Sg);

      // legs per stirrup
      Float64 num_legs = pStirrupGeometry->GetPrimaryHorizInterfaceBarCount(poi);

      num_legs += pStirrupGeometry->GetAdditionalHorizInterfaceBarCount(poi);

      pArtifact->SetNumLegs(num_legs);
   }

   // friction and cohesion factors
   bool is_roughened = pBridge->AreGirderTopFlangesRoughened(poi.GetSpan(),poi.GetGirder());
   lrfdConcreteUtil::DensityType girderDensityType = (lrfdConcreteUtil::DensityType)pMaterial->GetGdrConcreteType(poi.GetSpan(),poi.GetGirder());
   lrfdConcreteUtil::DensityType slabDensityType   = (lrfdConcreteUtil::DensityType)pMaterial->GetSlabConcreteType();
   Float64 c  = lrfdConcreteUtil::ShearCohesionFactor(is_roughened,girderDensityType,slabDensityType);
   Float64 u  = lrfdConcreteUtil::ShearFrictionFactor(is_roughened);
   Float64 K1 = lrfdConcreteUtil::HorizShearK1(is_roughened);
   Float64 K2 = lrfdConcreteUtil::HorizShearK2(is_roughened,girderDensityType,slabDensityType);

   pArtifact->SetCohesionFactor(c);
   pArtifact->SetFrictionFactor(u);
   pArtifact->SetK1(K1);
   pArtifact->SetK2(K2);

   // nominal shear capacities 5.8.4.1-2,3
   Float64 Vn1, Vn2, Vn3;
   lrfdConcreteUtil::HorizontalShearResistances(c, u, K1, K2, Acv, pArtifact->GetAvOverS(), Pc, fc, fy,
                                                &Vn1, &Vn2, &Vn3);
   pArtifact->SetVn(Vn1, Vn2, Vn3);

   pgsTypes::ConcreteType concType = pMaterial->GetGdrConcreteType(poi.GetSpan(),poi.GetGirder());
   GET_IFACE(IResistanceFactors,pResistanceFactors);
   Float64 PhiRC,PhiPS,PhiC;
   pResistanceFactors->GetFlexureResistanceFactors(concType,&PhiPS,&PhiRC,&PhiC);
   Float64 phi = min(PhiRC,PhiPS); // use min of PS and RC (see LRFD 5.8.4.1)
   pArtifact->SetPhi(phi);

   // Minimum steel check 5.8.4.1-4
   // This sucker has changed for every spec so far.

   Float64 bv = pGdr->GetShearInterfaceWidth( poi );
   pArtifact->SetBv(bv);

   Float64 sall = lrfdConcreteUtil::MaxStirrupSpacingForHoriz();
   pArtifact->SetSall(sall);

   pArtifact->SetFy(fy);

   Float64 avfmin = lrfdConcreteUtil::AvfOverSMin(bv,fy,Vuh,phi,c,u,Pc);
   pArtifact->SetAvOverSMin(avfmin);

   Uint16 min_num_legs = lrfdConcreteUtil::MinLegsForBv(bv);
   pArtifact->SetNumLegsReqd(min_num_legs);

   // Determine average shear strength
   Float64 Vnmin = min(Vn1, min(Vn2, Vn3));
   Float64 Vsavg = Vnmin/Acv;
   pArtifact->SetVsAvg(Vsavg);

   // Shear strength so that equation 5.8.4.1-4 is not applicable
   Float64 vs_limit = lrfdConcreteUtil::LowerLimitOfShearStrength(is_roughened);
   pArtifact->SetVsLimit(vs_limit);

   // Get Av/S required for design algorithm
   Float64 avs_reqd = lrfdConcreteUtil::AvfRequiredForHoriz(Vuh, phi, avfmin, c, u, K1, K2,
                                                            bv, Acv, pArtifact->GetAvOverS(), Pc, fc, fy);
   pArtifact->SetAvOverSReqd(avs_reqd);
}

Float64 pgsDesigner2::GetNormalFrictionForce(const pgsPointOfInterest& poi)
{
   GET_IFACE(ISectProp2,pSectProp2);
   GET_IFACE(IGirder,pGdr);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeMaterial,pMaterial);
   GET_IFACE(IProductForces,pProductForces);

   SpanIndexType spanIdx = poi.GetSpan();
   GirderIndexType gdrIdx  = poi.GetGirder();

   // permanent compressive force between slab and girder top
   // If the slab is CIP, use the tributary area.
   // If the slab is SIP, use only the area of cast slab that is NOT over
   // the deck panels.
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(spanIdx);

   Float64 wslab = 0; // weight of slab on shear interface

   // slab load
   Float64 slab_unit_weight = pMaterial->GetWgtDensitySlab() * unitSysUnitsMgr::GetGravitationalAcceleration();

   if ( pDeck->DeckType == pgsTypes::sdtCompositeCIP )
   {
      // Cast in place slab
      // conservative not to use sacrificial material so we will use just the structural slab depth
      // also, ignore the weight of the slab haunch as it may or may not be there depending on 
      // camber variation and other construction uncertainties
      Float64 slab_depth      = pBridge->GetStructuralSlabDepth(poi);
      Float64 trib_slab_width = pSectProp2->GetTributaryFlangeWidth(poi);
      
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
      Float64 slab_depth      = pBridge->GetStructuralSlabDepth(poi);
      Float64 top_flange_width = pGdr->GetTopFlangeWidth(poi);
      Float64 panel_support = pDeck->PanelSupport;

      MatingSurfaceIndexType nMatingSurfaces = pGdr->GetNumberOfMatingSurfaces(spanIdx,gdrIdx);
      Float64 wMating = 0; // sum of mating surface widths... less deck panel support width
      for ( MatingSurfaceIndexType i = 0; i < nMatingSurfaces; i++ )
      {
         if ( pBridge->IsExteriorGirder(spanIdx,gdrIdx) && 
              ((gdrIdx == 0 && i == 0) || // Left exterior girder
               (gdrIdx == pSpan->GetGirderCount()-1 && i == nMatingSurfaces-1))  // right exterior girder
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
      if ( pBridge->IsExteriorGirder(spanIdx,gdrIdx) )
      {
         Float64 slab_overhang;

         if ( poi.GetGirder() == 0 )
            slab_overhang = pBridge->GetLeftSlabOverhang(spanIdx,poi.GetDistFromStart()); 
         else
            slab_overhang = pBridge->GetRightSlabOverhang(spanIdx,poi.GetDistFromStart());

         Float64 top_width = pGdr->GetTopWidth(poi); // total width of the top of the girder

         Float64 woverhang = (slab_overhang - top_width/2)*slab_depth*slab_unit_weight;

         wslab += woverhang;

      }
   }


   Float64 comp_force = wslab;
   return comp_force;
}

void pgsDesigner2::CheckFullStirrupDetailing(const pgsPointOfInterest& poi, 
                                            const pgsVerticalShearArtifact& vertArtifact,
                                            const SHEARCAPACITYDETAILS& scd,
                                            const Float64 vu,
                                            Float64 fcGdr, Float64 fy,
                                            const STIRRUPCONFIG* pConfig,
                                            pgsStirrupDetailArtifact* pArtifact )
{
   // need bv and dv
   GET_IFACE(IShearCapacity, pShearCapacity);
   Float64 bv = scd.bv;
   Float64 dv = scd.dv;
   pArtifact->SetBv(bv);
   pArtifact->SetDv(dv);

   SpanIndexType span  = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   // av/s and fy rebar
   Float64 s;
   matRebar::Size size;
   Float64 abar, nl;
   Float64 Avfs;
   if (pConfig!=NULL)
   {
      GET_IFACE(IBridge, pBridge);
      Float64 gdr_length = pBridge->GetGirderLength(span,gdr);
      Float64 location = poi.GetDistFromStart();
      Float64 lft_supp_loc = pBridge->GetGirderStartConnectionLength(span,gdr);
      Float64 rgt_sup_loc = gdr_length - pBridge->GetGirderEndConnectionLength(span,gdr);

      Avfs = GetPrimaryStirrupAvs(*pConfig, getVerticalStirrup, location, gdr_length, 
                                  lft_supp_loc, rgt_sup_loc, &size, &abar, &nl, &s);
   }
   else
   {
      GET_IFACE(IStirrupGeometry, pStirrupGeometry);
      Avfs = pStirrupGeometry->GetVertStirrupAvs(poi, &size, &abar, &nl, &s);
   }

   pArtifact->SetBarSize(size);
   pArtifact->SetAvs(Avfs);
   pArtifact->SetS(s);

   // see if we even need to have stirrups
   bool is_app = vertArtifact.GetAreStirrupsReqd();
   pArtifact->SetApplicability(is_app);

   // Minimum transverse reinforcement 5.8.2.5
   // Set to zero if not applicable
   Float64 avs_min = 0.0;
   if (is_app)
   {
      avs_min = GetAvsOverMin(poi,scd);
   }
   pArtifact->SetAvsMin(avs_min);

   // applied shear force
   pArtifact->SetVu(vu);

   // max bar spacing
   Float64 s_max;
   Float64 s_under, s_over;
   GET_IFACE(ITransverseReinforcementSpec,pTransverseReinforcementSpec);
   pTransverseReinforcementSpec->GetMaxStirrupSpacing(&s_under, &s_over);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter1999 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2000Interims ? true : false );

   Float64 x = (bAfter1999 ? 0.125 : 0.100);

   Float64 vu_limit = x * fcGdr * bv * dv; // 5.8.2.7
   pArtifact->SetVuLimit(vu_limit);
   if (vu < vu_limit)
   {
      s_max = min(0.8*dv, s_under);  // 5.8.2.7-1
   }
   else
   {
      s_max = min(0.4*dv, s_over);  // 5.8.2.7-1
   }
   pArtifact->SetSMax(s_max);

   // min bar spacing
   Float64 s_min = 0.0;
   if ( size != matRebar::bsNone )
   {
      GET_IFACE(IBridgeMaterial,pMaterial);

      matRebar::Type type;
      matRebar::Grade grade;
      pMaterial->GetTransverseRebarMaterial(span, gdr, type, grade);

      lrfdRebarPool* prp = lrfdRebarPool::GetInstance();
      const matRebar* pRebar = prp->GetRebar(type,grade,size);

      Float64 db = pRebar->GetNominalDimension();
      Float64 as = pMaterial->GetMaxAggrSizeGdr(span, gdr);
      s_min = pTransverseReinforcementSpec->GetMinStirrupSpacing(as, db);
   }
   pArtifact->SetSMin(s_min);
}

Float64 pgsDesigner2::GetAvsOverMin(const pgsPointOfInterest& poi,const SHEARCAPACITYDETAILS& scd)
{
   const unitLength* pLengthUnit;
   const unitStress* pStressUnit;
   const unitAreaPerLength* pAvsUnit;
   Float64 K;
   Float64 Kfct;
   if ( lrfdVersionMgr::GetUnits() == lrfdVersionMgr::US )
   {
      pLengthUnit = &unitMeasure::Inch;
      pStressUnit = &unitMeasure::KSI;
      pAvsUnit    = &unitMeasure::Inch2PerInch;
      K = 0.0316;
      Kfct = 4.7;
   }
   else
   {
      pLengthUnit = &unitMeasure::Millimeter;
      pStressUnit = &unitMeasure::MPa;
      pAvsUnit    = &unitMeasure::Millimeter2PerMillimeter;
      K = 0.083;
      Kfct = 1.8;
   }

   Float64 bv = ::ConvertFromSysUnits(scd.bv,*pLengthUnit);
   Float64 fc = ::ConvertFromSysUnits(scd.fc,*pStressUnit);
   Float64 fy = ::ConvertFromSysUnits(scd.fy,*pStressUnit);
   Float64 fct= ::ConvertFromSysUnits(scd.fct,*pStressUnit);
   Float64 avs = K*bv/fy;

   switch( scd.ConcreteType )
   {
   case pgsTypes::Normal:
      avs *= sqrt(fc);
      break;

   case pgsTypes::AllLightweight:
      if ( scd.bHasFct )
         avs *= min(Kfct*fct,sqrt(fc));
      else
         avs *= 0.75*sqrt(fc);
      break;

   case pgsTypes::SandLightweight:
      if ( scd.bHasFct )
         avs *= min(Kfct*fct,sqrt(fc));
      else
         avs *= 0.85*sqrt(fc);
      break;

   default:
      ATLASSERT(false); // is there a new concrete type?
      avs *= sqrt(fc); 
      break;
   }

   avs = ::ConvertToSysUnits(avs,*pAvsUnit);

   return avs;
}

void pgsDesigner2::CheckLongReinfShear(const pgsPointOfInterest& poi, 
                                      pgsTypes::Stage stage,
                                      pgsTypes::LimitState ls,
                                      const SHEARCAPACITYDETAILS& scd,
                                      const GDRCONFIG* pConfig,
                                      pgsLongReinfShearArtifact* pArtifact )
{
   SpanIndexType span = poi.GetSpan();
   GirderIndexType gdr  = poi.GetGirder();

   GET_IFACE(IBridge,pBridge);
   Float64 start_end_size = pBridge->GetGirderStartConnectionLength(span,gdr);
   Float64 end_end_size   = pBridge->GetGirderEndConnectionLength(span,gdr);
   Float64 length         = pBridge->GetGirderLength(span,gdr);

   // check to see if we are outside of the faces of support.... If so, then this check doesn't apply
   GET_IFACE(IPointOfInterest, pIPoi);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(span,gdr,stage,POI_FACEOFSUPPORT) );
   ATLASSERT(vPoi.size() == 2);
   pgsPointOfInterest leftFaceOfSupport(vPoi.front());
   pgsPointOfInterest rightFaceOfSupport(vPoi.back());

   if ( poi.GetDistFromStart() < leftFaceOfSupport.GetDistFromStart() || rightFaceOfSupport.GetDistFromStart() < poi.GetDistFromStart() )
   {
      pArtifact->SetApplicability(false);
      return;
   }

   // the check is applicable

   pArtifact->SetApplicability(true);
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   // Longitudinal steel
   Float64 fy = scd.fy; 
   pArtifact->SetFy(fy);

   Float64 as = 0;
   if ( pSpecEntry->IncludeRebarForShear() )
   {
      // TRICKY: Rebar data from config is not used here. This is only called from the design loop
      //         once (no iterations), so all we need is the current bridge data
      GET_IFACE(ILongRebarGeometry,pRebarGeometry);

      if ( scd.bTensionBottom )
      {
         as = pRebarGeometry->GetAsBottomHalf(poi,false); // not adjusted for lack of development
         Float64 as2 = pRebarGeometry->GetAsBottomHalf(poi,true); // adjusted for lack of development
         if ( !IsZero(as) )
            fy *= (as2/as); // reduce effectiveness of bar for lack of development
         else
            fy = 0; // no strand, no development... reduce effectiveness to 0

         pArtifact->SetFy(fy);
      }
      else
      {
         as = pRebarGeometry->GetAsTopHalf(poi,false); // not adjusted for lack of development
         Float64 as2 = pRebarGeometry->GetAsTopHalf(poi,true); // adjusted for lack of development
         if ( !IsZero(as) )
            fy *= (as2/as); // reduce effectiveness of bar for lack of development
         else
            fy = 0; // no strand, no development... reduce effectiveness to 0

         pArtifact->SetFy(fy);
      }
   }
   pArtifact->SetAs(as);

   // prestress
   GET_IFACE(IStrandGeometry,pStrandGeom);

   // area of prestress on flexural tension side
   // NOTE: fps (see below) from the moment capacity analysis already accounts for a reduction
   //       in strand effectiveness based on lack of development. DO NOT ADJUST THE AREA OF PRESTRESS
   //       HERE TO ACCOUNT FOR THE SAME TIME...
   Float64 aps;
   if(pConfig!=NULL)
   {
      aps = (scd.bTensionBottom ? pStrandGeom->GetApsBottomHalf(poi,*pConfig,dlaNone) : pStrandGeom->GetApsTopHalf(poi,*pConfig,dlaNone));
   }
   else
   {
      aps = (scd.bTensionBottom ? pStrandGeom->GetApsBottomHalf(poi,dlaNone) : pStrandGeom->GetApsTopHalf(poi,dlaNone));
   }

   // get prestress level at ultimate
   GET_IFACE(IMomentCapacity,pMomentCap);
   MOMENTCAPACITYDETAILS mcd;
   if(pConfig!=NULL)
   {
      pMomentCap->GetMomentCapacityDetails(stage,poi,*pConfig,scd.bTensionBottom,&mcd);
   }
   else
   {
      pMomentCap->GetMomentCapacityDetails(stage,poi,scd.bTensionBottom,&mcd);
   }

   Float64 fps = mcd.fps;
   
   pArtifact->SetAps(aps);
   pArtifact->SetFps(fps);

   // set up demands... if this section is before/after the critical section, use the values at the critical section
   // see C5.8.3.5

   // Critical section
   bool bBeforeLeftCS = poi.GetDistFromStart()     < m_LeftCS.GetDistFromStart();
   bool bAfterRightCS = m_RightCS.GetDistFromStart() < poi.GetDistFromStart();

   Float64 vu = scd.Vu;
   Float64 vs = scd.Vs;
   Float64 vp = scd.Vp;
   Float64 theta = scd.Theta;
   if ( bBeforeLeftCS || bAfterRightCS )
   {
      GET_IFACE(IShearCapacity,pShearCapacity);
      SHEARCAPACITYDETAILS scd2;
      if(pConfig!=NULL)
      {
         pShearCapacity->GetShearCapacityDetails(ls,stage,(bBeforeLeftCS ? m_LeftCS : m_RightCS),*pConfig,&scd2);
      }
      else
      {
         pShearCapacity->GetShearCapacityDetails(ls,stage,(bBeforeLeftCS ? m_LeftCS : m_RightCS),&scd2);
      }
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
      phi_axial = 1.0;
   else
      phi_axial = 0.75; // does not consider seismic zones
   pArtifact->SetNu(nu);
   pArtifact->SetAxialPhi(phi_axial);

   // shear demand 
   Float64 phi_shear = scd.Phi;

   if ( lrfdVersionMgr::SecondEditionWith2000Interims <= lrfdVersionMgr::GetVersion() )
   {
       if ( vu/phi_shear < vs )
           vs = vu/phi_shear;
   }

   pArtifact->SetVu(vu);
   pArtifact->SetVs(vs);
   pArtifact->SetVp(vp);
   pArtifact->SetShearPhi(phi_shear);
   pArtifact->SetTheta(theta);

   // calculate required longitudinal reinforcement
   bool bDummy;
   bool bNegMomentAtStart;
   bool bNegMomentAtEnd;
   pBridge->IsContinuousAtPier(span,&bDummy,&bNegMomentAtStart);
   pBridge->IsContinuousAtPier(span+1,&bNegMomentAtEnd,&bDummy);

   Float64 demand;
   Uint16 equation = 999; // dummy value

   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      if ( (InRange(start_end_size,poi.GetDistFromStart(),m_LeftCS.GetDistFromStart()) && !bNegMomentAtStart) ||
           (InRange(m_RightCS.GetDistFromStart(),poi.GetDistFromStart(),length-end_end_size) && !bNegMomentAtEnd) )
      {
          // Equation 5.8.3.5-2
         demand = 0.5*nu/phi_axial +
                  (vu/phi_shear - 0.5*vs - vp)/tan(theta);
         equation = 2;
      }
      else
      {
        // Equation 5.8.3.5-1
        demand = fabs(mu)/(dv*phi_flexure) + 0.5*nu/phi_axial +
                 (fabs(vu/phi_shear - vp) - 0.5*vs)/tan(theta);
        equation = 1;
      }
   }
   else
   {
      if (// Spec is 2003 or earlier AND poi is at one of the points of support 
           (lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::SecondEditionWith2003Interims && 
           ( (IsEqual(poi.GetDistFromStart(),start_end_size) && !bNegMomentAtStart ) || 
             (IsEqual(poi.GetDistFromStart(),length-end_end_size))) && !bNegMomentAtEnd )
            ||
           // Spec is 2004 AND poi is between point of support and critical section for shear
           (lrfdVersionMgr::SecondEditionWith2003Interims < lrfdVersionMgr::GetVersion() && lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::ThirdEdition2004 && 
              ( (InRange(start_end_size,poi.GetDistFromStart(),m_LeftCS.GetDistFromStart()) && !bNegMomentAtStart) || 
                (InRange(m_RightCS.GetDistFromStart(),poi.GetDistFromStart(),length-end_end_size)) && !bNegMomentAtEnd) )
         )
      {
          // Equation 5.8.3.5-2
         demand = 0.5*nu/phi_axial +
                  (vu/phi_shear - 0.5*vs - vp)/tan(theta);
         equation = 2;
      }
      else
      {
          // Equation 5.8.3.5-1
         demand = mu/(dv*phi_flexure) + 0.5*nu/phi_axial +
                  (vu/phi_shear - 0.5*vs - vp)/tan(theta);
         equation = 1;
      }
   }

   if ( equation == 1 )
   {
      // if equation 1 is used, this requirement will be satisfied if Mr >= Mu
      MOMENTCAPACITYDETAILS mcd;
      GET_IFACE(IMomentCapacity,pMomentCapacity);
      if(pConfig!=NULL)
      {
         pMomentCapacity->GetMomentCapacityDetails(pgsTypes::BridgeSite3,poi,*pConfig,scd.bTensionBottom,&mcd);
      }
      else
      {
         pMomentCapacity->GetMomentCapacityDetails(pgsTypes::BridgeSite3,poi,scd.bTensionBottom,&mcd);
      }

      Float64 Mr = mcd.Phi*mcd.Mn;
      pArtifact->SetMr(Mr);
   }

   Float64 capacity = as*fy + aps*fps;

   pArtifact->SetEquation(equation);
   pArtifact->SetDemandForce(demand);
   pArtifact->SetCapacityForce(capacity);
}

void pgsDesigner2::CheckConfinement(SpanIndexType span, GirderIndexType gdr, const GDRCONFIG* pConfig, pgsConfinementArtifact* pArtifact )
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IStrandGeometry,pStrandGeometry);
   GET_IFACE(IGirder,pGdr);
   GET_IFACE(IBridgeMaterial,pMaterial);

   Float64 gird_length  = pBridge->GetGirderLength(span, gdr);

   // If we are in here, confinement check is applicable
   pArtifact->SetApplicability(true);

   // Get spec constraints
   GET_IFACE(ITransverseReinforcementSpec,pTransverseReinforcementSpec);
   matRebar::Size szmin = pTransverseReinforcementSpec->GetMinConfinmentBarSize();
   Float64 smax = pTransverseReinforcementSpec->GetMaxConfinmentBarSpacing();

   matRebar::Grade grade;
   matRebar::Type type;
   pMaterial->GetTransverseRebarMaterial(span,gdr,type,grade);

   pArtifact->SetMinBar(lrfdRebarPool::GetInstance()->GetRebar(type,grade,szmin));
   pArtifact->SetSMax(smax);

   // Use utility function to get confinement zone lengths at girder ends
   Float64 reqdStartZl, reqdEndZl;
   GetConfinementZoneLengths(span, gdr, pGdr, gird_length, &reqdStartZl, &reqdEndZl);

   pArtifact->SetStartRequiredZoneLength(reqdStartZl);
   pArtifact->SetEndRequiredZoneLength(reqdEndZl);

   // get and set provided stirrup configuration at start and ends
   GET_IFACE(IStirrupGeometry, pStirrupGeometry);

   matRebar::Size start_rbsiz, end_rbsiz;
   Float64 start_zl, end_zl;
   Float64 start_s, end_s;
   if (pConfig)
   {
      GetConfinementInfoFromStirrupConfig(pConfig->StirrupConfig, reqdStartZl, &start_rbsiz, &start_zl, &start_s,
                                                   reqdEndZl, &end_rbsiz, &end_zl, &end_s);
   }
   else
   {
      pStirrupGeometry->GetStartConfinementBarInfo(span, gdr, reqdStartZl, &start_rbsiz, &start_zl, &start_s);
      pStirrupGeometry->GetEndConfinementBarInfo(span, gdr, reqdEndZl, &end_rbsiz, &end_zl, &end_s);
   }

   pArtifact->SetStartS(start_s);
   pArtifact->SetStartProvidedZoneLength(start_zl);
   pArtifact->SetStartBar(lrfdRebarPool::GetInstance()->GetRebar(type,grade,start_rbsiz));

   pArtifact->SetEndS(end_s);
   pArtifact->SetEndProvidedZoneLength(end_zl);
   pArtifact->SetEndBar(lrfdRebarPool::GetInstance()->GetRebar(type,grade,end_rbsiz));
}

void pgsDesigner2::CheckMomentCapacity(SpanIndexType span,GirderIndexType gdr,pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsGirderArtifact* pGdrArtifact)
{
   GET_IFACE(IPointOfInterest, pIPoi);

   std::vector<pgsPointOfInterest> vPoi;

   if ( stage == pgsTypes::BridgeSite1 )
   {
      vPoi = pIPoi->GetPointsOfInterest(span,gdr,stage,POI_FLEXURECAPACITY);
   }
   else
   {
      PoiAttributeType attrib = POI_FLEXURECAPACITY | POI_SHEAR | POI_CRITSECTSHEAR1 | POI_CRITSECTSHEAR2;
      vPoi = pIPoi->GetPointsOfInterest(span, gdr, stage, attrib, POIFIND_OR );
   }

   PierIndexType prev_pier = span;
   PierIndexType next_pier = prev_pier+1;
   GET_IFACE(IBridge,pBridge);
   std::vector<pgsPointOfInterest>::iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++)
   {
      const pgsPointOfInterest& poi = *poiIter;
      pgsFlexuralCapacityArtifactKey key(stage,ls,poi.GetDistFromStart());

      // we always do positive moment
      pgsFlexuralCapacityArtifact pm_artifact = CreateFlexuralCapacityArtifact(poi,stage,ls,true);


      // negative moment is a different story. there must be a negative moment connection
      // at one end of the girder
      pgsFlexuralCapacityArtifact nm_artifact;
      bool bComputeNegativeMomentCapacity = pBridge->ProcessNegativeMoments(span);

      if ( stage == pgsTypes::BridgeSite3 && bComputeNegativeMomentCapacity )
         nm_artifact = CreateFlexuralCapacityArtifact(poi,stage,ls,false);

      pGdrArtifact->AddFlexuralCapacityArtifact(key,pm_artifact,nm_artifact);
   }
}

void pgsDesigner2::InitShearCheck(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls,const GDRCONFIG* pConfig)
{
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // get the reactions and determine if we can skip POIs outside of the critical sections
   PierIndexType prev_pier = PierIndexType(span);
   PierIndexType next_pier = prev_pier+1;

   GET_IFACE(ILimitStateForces,pForces);
   Float64 Rmin, Rmax;
   if ( analysisType == pgsTypes::Envelope )
   {
      Float64 min,max;

      pForces->GetReaction(ls,pgsTypes::BridgeSite3,prev_pier,gdr,MaxSimpleContinuousEnvelope,true,&min,&max);
      Rmax = max;

      // we don't need the min reaction
      //pForces->GetReaction(ls,pgsTypes::BridgeSite3,prev_pier,gdr,MinSimpleContinuousEnvelope,&min,&max);
      //Rmin = min;
   }
   else
   {
      pForces->GetReaction(ls,pgsTypes::BridgeSite3,prev_pier,gdr,analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan,true,&Rmin,&Rmax);
   }

   m_bSkipShearCheckBeforeLeftCS = (0 < Rmax ? true : false);

   if ( analysisType == pgsTypes::Envelope )
   {
      Float64 min,max;

      pForces->GetReaction(ls,pgsTypes::BridgeSite3,next_pier,gdr,MaxSimpleContinuousEnvelope,true,&min,&max);
      Rmax = max;

      // we don't need the min reaction
      //pForces->GetReaction(ls,pgsTypes::BridgeSite3,next_pier,gdr,MinSimpleContinuousEnvelope,&min,&max);
      //Rmin = min;
   }
   else
   {
      pForces->GetReaction(ls,pgsTypes::BridgeSite3,next_pier,gdr,analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan,true,&Rmin,&Rmax);
   }

   m_bSkipShearCheckAfterRightCS = (0 < Rmax ? true : false);

   // cache CS locations as they are very expensive to get
   GET_IFACE(IPointOfInterest,pPOI);
   if(pConfig!=NULL)
   {
      pPOI->GetCriticalSection(ls,span,gdr,*pConfig,&m_LeftCS,&m_RightCS);
   }
   else
   {
      pPOI->GetCriticalSection(ls,span,gdr,&m_LeftCS,&m_RightCS);
   }

   // DETERMINE IF vu <= 0.18f'c at each POI... set a boolean flag that indicates if strut and tie analysis is required
   // LRFD 5.8.3.2
   GET_IFACE(IBridge,pBridge);
   bool bIntegral, bIntegralLeft, bIntegralRight; // bIntergral is a dummy variable for the side of the pier we don't care about
   pBridge->IsIntegralAtPier(prev_pier,&bIntegral,&bIntegralLeft);    // right side of left pier (start of span)
   pBridge->IsIntegralAtPier(next_pier,&bIntegralRight,&bIntegral); // left side of right pier (end of span)

   // NOTE: scd.vfc is v/f'c. Since v is divided by f'c, 0.18f'c divided by f'c is simply 0.18
   GET_IFACE(IShearCapacity,pShearCapacity);
   SHEARCAPACITYDETAILS scd;
   if(pConfig!=NULL)
   {
      pShearCapacity->GetShearCapacityDetails( ls, pgsTypes::BridgeSite3, m_LeftCS, *pConfig, &scd );
   }
   else
   {
      pShearCapacity->GetShearCapacityDetails( ls, pgsTypes::BridgeSite3, m_LeftCS, &scd );
   }

   m_bLeftCS_StrutAndTieRequired = ( 0.18 < scd.vfc && !bIntegralLeft );

   if(pConfig!=NULL)
   {
      pShearCapacity->GetShearCapacityDetails( ls, pgsTypes::BridgeSite3, m_RightCS, *pConfig, &scd );
   }
   else
   {
      pShearCapacity->GetShearCapacityDetails( ls, pgsTypes::BridgeSite3, m_RightCS, &scd );
   }

   m_bRightCS_StrutAndTieRequired = ( 0.18 < scd.vfc && !bIntegralRight );
}

void pgsDesigner2::CheckShear(SpanIndexType span,GirderIndexType gdr,const std::vector<pgsPointOfInterest>& VPoi,
                              pgsTypes::LimitState ls,const GDRCONFIG* pConfig,pgsStirrupCheckArtifact* pStirrupArtifact)
{
   pgsTypes::Stage stage = pgsTypes::BridgeSite3;
   InitShearCheck(span,gdr,ls,pConfig); // sets up some class member variables used for checking
                                        // this span and girder

   CHECK(pStirrupArtifact);
   GET_IFACE(IBridgeMaterial,pMaterial);
   Float64 fc_slab = pMaterial->GetFcSlab();

   Float64 fc_girder;
   if(pConfig!=NULL)
   {
      fc_girder = pConfig->Fc;
   }
   else
   {
      fc_girder = pMaterial->GetFcGdr(span,gdr);
   }

   pStirrupArtifact->SetFc(fc_girder);
   
   Float64 Es, fy, fu;
   pMaterial->GetTransverseRebarProperties(span,gdr,&Es,&fy,&fu);
   pStirrupArtifact->SetFy(fy);

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // Confinement check
   GET_IFACE(ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool check_confinement = pSpecEntry->IsConfinementCheckEnabled() && ls==pgsTypes::StrengthI; // only need to check confinement once

   pgsConfinementArtifact c_artifact;
   if (check_confinement)
   {
      CheckConfinement(span, gdr, pConfig, &c_artifact);
      pStirrupArtifact->SetConfinementArtifact(c_artifact);
   }

   // Splitting zone check
   CheckSplittingZone(span,gdr,pConfig,pStirrupArtifact);

   // poi-based shear check
   GET_IFACE(ILimitStateForces, pLimitStateForces);

   // loop over pois
   std::vector<pgsPointOfInterest>::const_iterator poiIter(VPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(VPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++)
   {
      const pgsPointOfInterest& poi = *poiIter;
      sysSectionValue Vmin, Vmax;

      if ( analysisType == pgsTypes::Envelope )
      {
         sysSectionValue min,max;
         pLimitStateForces->GetShear(ls,stage,poi,MaxSimpleContinuousEnvelope,&min,&max);
         Vmax = max;

         pLimitStateForces->GetShear(ls,stage,poi,MinSimpleContinuousEnvelope,&min,&max);
         Vmin = min;
      }
      else
      {
         pLimitStateForces->GetShear(ls,stage,poi,analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan,&Vmin,&Vmax);
      }

      // Take max absolute value for demand
      Float64 Vu = Max4(abs(Vmin.Left()),abs(Vmax.Left()),abs(Vmin.Right()),abs(Vmax.Right()));

      pgsStirrupCheckAtPoisArtifact artifact = CreateStirrupCheckAtPoisArtifact(poi,stage,ls,Vu,fc_slab,fc_girder,fy,check_confinement,pConfig);

      pgsStirrupCheckAtPoisArtifactKey key(stage,ls,poi.GetDistFromStart());
      pStirrupArtifact->AddStirrupCheckAtPoisArtifact(key,artifact);
   }
}

void pgsDesigner2::CheckSplittingZone(SpanIndexType span,GirderIndexType gdr,const GDRCONFIG* pConfig,pgsStirrupCheckArtifact* pStirrupArtifact)
{
   // don't need to do anything if disabled
   GET_IFACE(ISpecification,pSpec);
   GET_IFACE(ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if (!pSpecEntry->IsSplittingCheckEnabled())
      return;

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IStirrupGeometry, pStirrupGeometry);
   GET_IFACE(IBridgeMaterial,pMat);
   GET_IFACE(IGirder,pGdr);
   GET_IFACE(ITransverseReinforcementSpec,pTransverseReinforcementSpec);
   GET_IFACE(ILosses,pLosses);
   GET_IFACE(IPrestressForce,pPrestressForce);

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);
   std::_tostringstream os;
   os << _T("Checking splitting requirements for Span ")
      << LABEL_SPAN(span) << _T(" Girder ")
      << LABEL_GIRDER(gdr) << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );


   // get POI at point of prestress transfer
   // this is where the prestress force is fully transfered
   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(span,gdr,pgsTypes::CastingYard,POI_PSXFER) );
   ATLASSERT(vPOI.size() != 0);
   pgsPointOfInterest start_poi( vPOI.front() );
   pgsPointOfInterest end_poi( vPOI.back() );

   pgsSplittingZoneArtifact* pArtifact = pStirrupArtifact->GetSplittingZoneArtifact();

   pArtifact->SetIsApplicable(true);

   Float64 start_h = pGdr->GetSplittingZoneHeight( start_poi );
   pArtifact->SetStartH(start_h);

   Float64 end_h = pGdr->GetSplittingZoneHeight( end_poi );
   pArtifact->SetEndH(end_h);

   // basically this is h/4 except that the 4 is a parametric value
   pArtifact->SetSplittingZoneLengthFactor(pTransverseReinforcementSpec->GetSplittingZoneLengthFactor());

   Float64 start_zl = pTransverseReinforcementSpec->GetSplittingZoneLength(start_h);
   pArtifact->SetStartSplittingZoneLength(start_zl);

   Float64 end_zl = pTransverseReinforcementSpec->GetSplittingZoneLength(end_h);
   pArtifact->SetEndSplittingZoneLength(end_zl);

   // Get the splitting force parameters (the artifact actually computes the splitting force)
   Float64 start_Fpj,  end_Fpj;
   Float64 start_dFpT, end_dFpT;
   if (pConfig!=NULL)
   {
      start_Fpj = pPrestressForce->GetStrandStress(start_poi,pgsTypes::Permanent,*pConfig,pgsTypes::Jacking);
      start_dFpT = pLosses->GetAfterXferLosses(start_poi,pgsTypes::Permanent,*pConfig);

      end_Fpj = pPrestressForce->GetStrandStress(end_poi,pgsTypes::Permanent,*pConfig,pgsTypes::Jacking);
      end_dFpT = pLosses->GetAfterXferLosses(end_poi,pgsTypes::Permanent,*pConfig);
   }
   else
   {
      start_Fpj = pPrestressForce->GetStrandStress(start_poi,pgsTypes::Permanent,pgsTypes::Jacking);
      start_dFpT = pLosses->GetAfterXferLosses(start_poi,pgsTypes::Permanent);

      end_Fpj = pPrestressForce->GetStrandStress(end_poi,pgsTypes::Permanent,pgsTypes::Jacking);
      end_dFpT = pLosses->GetAfterXferLosses(end_poi,pgsTypes::Permanent);
   }

   StrandIndexType Ns, Nh, Nt;
   StrandIndexType Nsd, Nhd, Ntd;
   if (pConfig!=NULL)
   {
      Ns = pConfig->Nstrands[pgsTypes::Straight];
      Nh = pConfig->Nstrands[pgsTypes::Harped];
      Nt = pConfig->Nstrands[pgsTypes::Temporary];

      Nsd = pConfig->Debond[pgsTypes::Straight].size();
      Nhd = pConfig->Debond[pgsTypes::Harped].size();
      Ntd = pConfig->Debond[pgsTypes::Temporary].size();
   }
   else
   {
      GET_IFACE(IStrandGeometry,pStrandGeometry);
      Ns = pStrandGeometry->GetNumStrands(span,gdr,pgsTypes::Straight);
      Nh = pStrandGeometry->GetNumStrands(span,gdr,pgsTypes::Harped);
      Nt = pStrandGeometry->GetNumStrands(span,gdr,pgsTypes::Temporary);

      Nsd = pStrandGeometry->GetNumDebondedStrands(span,gdr,pgsTypes::Straight);
      Nhd = pStrandGeometry->GetNumDebondedStrands(span,gdr,pgsTypes::Harped);
      Ntd = pStrandGeometry->GetNumDebondedStrands(span,gdr,pgsTypes::Temporary);
   }

   // if the temporary strands aren't pretensioned, then they aren't in the section
   // when Splitting is checked!!!
   if (pConfig!=NULL)
   {
      if ( pConfig->TempStrandUsage != pgsTypes::ttsPretensioned )
      {
         Nt  = 0;
         Ntd = 0;
      }
   }
   else
   {
      GET_IFACE(IGirderData,pGirderData);
      CGirderData girderData = pGirderData->GetGirderData(span,gdr);
      if ( girderData.TempStrandUsage != pgsTypes::ttsPretensioned )
      {
         Nt  = 0;
         Ntd = 0;
      }
   }

   StrandIndexType nDebonded = Nsd + Nhd + Ntd;
   Float64 Aps = (Ns - nDebonded)*pMat->GetStrand(span,gdr,pgsTypes::Straight)->GetNominalArea();
   Aps += Nh*pMat->GetStrand(span,gdr,pgsTypes::Harped)->GetNominalArea();
   Aps += Nt*pMat->GetStrand(span,gdr,pgsTypes::Temporary)->GetNominalArea();

   pArtifact->SetStartAps(Aps);
   pArtifact->SetStartFpj(start_Fpj);
   pArtifact->SetStartLossesAfterTransfer(start_dFpT);

   pArtifact->SetEndAps(Aps);
   pArtifact->SetEndFpj(end_Fpj);
   pArtifact->SetEndLossesAfterTransfer(end_dFpT);

   // Compute Splitting resistance
   Float64 Es, fy, fu;
   pMat->GetTransverseRebarProperties(span,gdr,&Es,&fy,&fu);
   Float64 fs = pTransverseReinforcementSpec->GetMaxSplittingStress(fy);
   pArtifact->SetStartFs(fs);
   pArtifact->SetEndFs(fs);

   Float64 girder_length = pBridge->GetGirderLength(span,gdr);

   Float64 start_Avs;
   Float64 end_Avs;
   if (pConfig!=NULL)
   {
      matRebar::Type barType;
      matRebar::Grade barGrade;
      pMat->GetTransverseRebarMaterial(span, gdr, barType, barGrade);
      GetSplittingAvFromStirrupConfig(pConfig->StirrupConfig, barType, barGrade, girder_length,
                                                  start_zl, &start_Avs, end_zl, &end_Avs);
   }
   else
   {
      start_Avs = pStirrupGeometry->GetSplittingAv(span,gdr,0.0,start_zl);
      end_Avs   = pStirrupGeometry->GetSplittingAv(span,gdr,girder_length-end_zl,girder_length);
   }

   pgsTypes::SplittingDirection splittingDirection = pGdr->GetSplittingDirection(span,gdr);
   pArtifact->SetSplittingDirection(splittingDirection);

   pArtifact->SetStartAvs(start_Avs);
   pArtifact->SetEndAvs(end_Avs);

   Float64 start_Pr = fs*start_Avs;
   pArtifact->SetStartSplittingResistance(start_Pr);

   Float64 end_Pr = fs*end_Avs;
   pArtifact->SetEndSplittingResistance(end_Pr);
}

void pgsDesigner2::CheckGirderDetailing(SpanIndexType span,GirderIndexType gdr,pgsGirderArtifact* pGdrArtifact)
{
   // 5.14.1.2.2
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);
   std::_tostringstream os;
   os << _T("Checking Girder Detailing requirements for Span ")
      << LABEL_SPAN(span) << _T(" Girder ")
      << LABEL_GIRDER(gdr) << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );

   pgsPrecastIGirderDetailingArtifact* pArtifact = pGdrArtifact->GetPrecastIGirderDetailingArtifact();

   // get min girder dimensions from spec
   GET_IFACE(IPrecastIGirderDetailsSpec,pPrecastIGirderDetailsSpec);
   pArtifact->SetMinTopFlangeThickness(pPrecastIGirderDetailsSpec->GetMinTopFlangeThickness());
   pArtifact->SetMinWebThickness(pPrecastIGirderDetailsSpec->GetMinWebThickness());
   pArtifact->SetMinBottomFlangeThickness(pPrecastIGirderDetailsSpec->GetMinBottomFlangeThickness());

   // get dimensions from bridge model
   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPoi( pPOI->GetPointsOfInterest(span,gdr,pgsTypes::CastingYard,POI_ALL,POIFIND_OR) );

   Float64 minTopFlange = DBL_MAX;
   Float64 minBotFlange = DBL_MAX;
   Float64 minWeb       = DBL_MAX;

   GET_IFACE(IGirder,pGdr);
   FlangeIndexType nTopFlanges = pGdr->GetNumberOfTopFlanges(span,gdr);
   WebIndexType nWebs = pGdr->GetNumberOfWebs(span,gdr);
   FlangeIndexType nBotFlanges = pGdr->GetNumberOfBottomFlanges(span,gdr);

   if ( nTopFlanges != 0 || nWebs != 0 || nBotFlanges != 0 )
   {
      std::vector<pgsPointOfInterest>::iterator poiIter(vPoi.begin());
      std::vector<pgsPointOfInterest>::iterator poiIterEnd(vPoi.end());
      for ( ; poiIter != poiIterEnd; poiIter++)
      {
         pgsPointOfInterest& poi = *poiIter;
         minBotFlange = min(minBotFlange,pGdr->GetMinBottomFlangeThickness(poi));
         minWeb       = min(minWeb,      pGdr->GetMinWebWidth(poi));
         minTopFlange = min(minTopFlange,pGdr->GetMinTopFlangeThickness(poi));
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

   if (  0 == nWebs )
   {
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

void pgsDesigner2::CheckStrandSlope(SpanIndexType span,GirderIndexType gdr,pgsStrandSlopeArtifact* pArtifact)
{
   GET_IFACE(ISpecification,pSpec);
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(IBridgeMaterial,pMat);
   GET_IFACE(IStrandGeometry,pStrGeom);

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);
   std::_tostringstream os;
   os << _T("Checking Strand Slope requirements for Span ")
      << LABEL_SPAN(span) << _T(" Girder ")
      << LABEL_GIRDER(gdr) << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   bool bCheck, bDesign;
   Float64 s50, s60, s70;
   pSpecEntry->GetMaxStrandSlope(&bCheck,&bDesign,&s50,&s60,&s70);
   pArtifact->IsApplicable( bCheck );

   const matPsStrand* pStrand = pMat->GetStrand(span,gdr,pgsTypes::Permanent);
   Float64 capacity;
   Float64 demand;

   if ( pStrand->GetSize() == matPsStrand::D1778  )
      capacity = s70;
   else if ( pStrand->GetSize() == matPsStrand::D1524 )
      capacity = s60;
   else
      capacity = s50;

   demand = pStrGeom->GetMaxStrandSlope( pgsPointOfInterest(span,gdr,0.00) );

   pArtifact->SetCapacity( capacity );
   pArtifact->SetDemand( demand );
}

void pgsDesigner2::CheckHoldDownForce(SpanIndexType span,GirderIndexType gdr,pgsHoldDownForceArtifact* pArtifact)
{
   GET_IFACE(ISpecification,pSpec);
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(IBridgeMaterial,pMat);
   GET_IFACE(IPrestressForce,pPrestressForce);

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);
   std::_tostringstream os;
   os << _T("Checking Hold Down Force requirements for Span ")
      << LABEL_SPAN(span) << _T(" Girder ")
      << LABEL_GIRDER(gdr) << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   bool bCheck, bDesign;
   Float64 maxHoldDownForce;
   pSpecEntry->GetHoldDownForce(&bCheck,&bDesign,&maxHoldDownForce);
   pArtifact->IsApplicable( bCheck );

   Float64 demand;

   demand = pPrestressForce->GetHoldDownForce(span,gdr);

   pArtifact->SetCapacity( maxHoldDownForce );
   pArtifact->SetDemand( demand );

}

void pgsDesigner2::CheckLiveLoadDeflection(SpanIndexType span,GirderIndexType gdr,pgsDeflectionCheckArtifact* pArtifact)
{
   GET_IFACE(ILibrary, pLib );
   GET_IFACE(ISpecification, pSpec );
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if (pSpecEntry->GetDoEvaluateLLDeflection())
   {
      GET_IFACE(IProgress,pProgress);
      CEAFAutoProgress ap(pProgress);
      std::_tostringstream os;
      os << _T("Checking Live Load Deflection requirements for Span ")
         << LABEL_SPAN(span) << _T(" Girder ")
         << LABEL_GIRDER(gdr) << std::ends;
      pProgress->UpdateMessage( os.str().c_str() );

      pArtifact->IsApplicable(true);

      // get max allowable deflection
      GET_IFACE(IBridge,pBridge);
      Float64 L = pBridge->GetSpanLength( span, gdr );
      Float64 ratio = pSpecEntry->GetLLDeflectionLimit();
      ASSERT(ratio>0);
      Float64 capacity = L/ratio;

      pArtifact->SetAllowableSpanRatio(ratio);
      pArtifact->SetCapacity(capacity);

      // find maximum live load deflection along girder
      GET_IFACE(IPointOfInterest,pIPoi);
      std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( span, gdr, pgsTypes::BridgeSite1, POI_FLEXURESTRESS | POI_TABULAR ) );

      GET_IFACE(IProductForces,pForces);
   
      // determine 
      Float64 min_defl =  10000;
      Float64 max_defl = -10000;
      std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
      for ( ; poiIter != poiIterEnd; poiIter++ )
      {
         const pgsPointOfInterest& poi = *poiIter;

         Float64 min, max;
         pForces->GetDeflLiveLoadDisplacement( IProductForces::DeflectionLiveLoadEnvelope, poi, &min, &max );

         min_defl = min(min_defl, min);
         max_defl = max(max_defl, max);
      }

      pArtifact->SetDemand(min_defl,max_defl);

   }
   else
   {
      pArtifact->IsApplicable(false);
   }

}

void pgsDesigner2::CheckConstructability(SpanIndexType span,GirderIndexType gdr,pgsConstructabilityArtifact* pArtifact)
{
   GET_IFACE(ILibrary, pLib );
   GET_IFACE(ISpecification, pSpec );
   GET_IFACE(IBridge,pBridge);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   ///////////////////////////////////////////////////////////////
   //
   // Check _T("A") Dimension
   //
   ///////////////////////////////////////////////////////////////

   // if there is no deck, slab offset is not applicable
   if (!pSpecEntry->IsSlabOffsetCheckEnabled() || pBridge->GetDeckType() == pgsTypes::sdtNone)
   {
      pArtifact->SetSlabOffsetApplicability(false);
   }
   else
   {
      GET_IFACE(IProgress,pProgress);
      CEAFAutoProgress ap(pProgress);
      std::_tostringstream os;
      os << _T("Checking Constructability requirements for Span ")
         << LABEL_SPAN(span) << _T(" Girder ")
         << LABEL_GIRDER(gdr) << std::ends;
      pProgress->UpdateMessage( os.str().c_str() );

      pArtifact->SetSlabOffsetApplicability(true);

      GET_IFACE(IGirderHaunch,pGdrHaunch);

      Float64 A_Provided;
      Float64 A_Required;

      A_Provided = min(pBridge->GetSlabOffset(span,gdr,pgsTypes::metStart),pBridge->GetSlabOffset(span,gdr,pgsTypes::metEnd));
      A_Required = pGdrHaunch->GetRequiredSlabOffset(span,gdr);

      pArtifact->SetRequiredSlabOffset( A_Required );
      pArtifact->SetProvidedSlabOffset( A_Provided );

      GET_IFACE(IStirrupGeometry, pStirrupGeometry);
      bool bDoStirrupsEngageDeck = pStirrupGeometry->DoStirrupsEngageDeck(span,gdr);

      HAUNCHDETAILS haunch_details;
      pGdrHaunch->GetHaunchDetails(span,gdr,&haunch_details);
      pArtifact->CheckStirrupLength( bDoStirrupsEngageDeck && 0.0 < haunch_details.HaunchDiff );
   }

   ///////////////////////////////////////////////////////////////
   //
   // Check Global Stability of Girder
   //
   ///////////////////////////////////////////////////////////////
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   if ( pBridgeDesc->GetGirderOrientation() == pgsTypes::Plumb )
   {
      pArtifact->SetGlobalGirderStabilityApplicability(false); // don't worry about this with a plumb girder
   }
   else
   {
      GET_IFACE(IGirder,pGirder);
      GET_IFACE(ISectProp2,pSectProp);
      Float64 orientation = fabs(pGirder->GetOrientation(span,gdr));
      pArtifact->SetGlobalGirderStabilityApplicability(true);

      // check stability at start of girder
      pgsPointOfInterest poi1(span,gdr,0.00); 
      Float64 Wbottom1 = pGirder->GetBottomWidth(poi1);
      Float64 Ybottom1 = pSectProp->GetYb(pgsTypes::CastingYard,poi1);

      pArtifact->SetGlobalGirderStabilityParameters(Wbottom1,Ybottom1,orientation);
      Float64 incline1 = pArtifact->GetMaxGirderIncline();

      // check stability at end of girder
      GET_IFACE(IBridge,pBridge);
      Float64 girder_length = pBridge->GetGirderLength(span,gdr);
      Float64 end_offset = pBridge->GetGirderEndConnectionLength(span,gdr);
      pgsPointOfInterest poi2(span,gdr,girder_length - end_offset); 
      Float64 Wbottom2 = pGirder->GetBottomWidth(poi2);
      Float64 Ybottom2 = pSectProp->GetYb(pgsTypes::CastingYard,poi2);

      pArtifact->SetGlobalGirderStabilityParameters(Wbottom2,Ybottom2,orientation);
      Float64 incline2 = pArtifact->GetMaxGirderIncline();

      if ( incline1 < incline2 )
      {
         // start of girder is the worst case
         pArtifact->SetGlobalGirderStabilityParameters(Wbottom1,Ybottom1,orientation);
      }
   }


}

void pgsDesigner2::CheckDebonding(SpanIndexType span,GirderIndexType gdr,pgsTypes::StrandType strandType,pgsDebondArtifact* pArtifact)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IStrandGeometry,pStrandGeometry);
   GET_IFACE(IDebondLimits,pDebondLimits);

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);
   std::_tostringstream os;
   os << _T("Checking Debonding requirements for Span ")
      << LABEL_SPAN(span) << _T(" Girder ")
      << LABEL_GIRDER(gdr) << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );

   Float64 maxFra = pDebondLimits->GetMaxDebondedStrands(span,gdr);
   pArtifact->AddMaxDebondStrandsAtSection(pDebondLimits->GetMaxNumDebondedStrandsPerSection(span,gdr),
                                           pDebondLimits->GetMaxDebondedStrandsPerSection(span,gdr));

   Float64 maxFraPerRow = pDebondLimits->GetMaxDebondedStrandsPerRow(span,gdr);

   // Total number of debonded strands
   StrandIndexType nStrands  = pStrandGeometry->GetNumStrands(span,gdr,strandType);
   StrandIndexType nDebonded = pStrandGeometry->GetNumDebondedStrands(span,gdr,strandType);
   Float64 fra = (nStrands == 0 ? 0 : (Float64)nDebonded/(Float64)nStrands);

   pArtifact->SetMaxFraDebondedStrands(maxFra);
   pArtifact->SetFraDebondedStrands(fra);
   pArtifact->SetNumDebondedStrands(nDebonded);

   // Number of debonded strands in row
   RowIndexType nRows = pStrandGeometry->GetNumRowsWithStrand(span,gdr,strandType);
   for ( RowIndexType row = 0; row < nRows; row++ )
   {
      StrandIndexType nStrandsInRow = pStrandGeometry->GetNumStrandInRow(span,gdr,row,strandType);
      StrandIndexType nDebondStrandsInRow = pStrandGeometry->GetNumDebondedStrandsInRow(span,gdr,row,strandType);
      fra = (nStrandsInRow == 0 ? 0 : (Float64)nDebondStrandsInRow/(Float64)nStrandsInRow);
      bool bExteriorStrandDebonded = pStrandGeometry->IsExteriorStrandDebondedInRow(span,gdr,row,strandType);
      pArtifact->AddNumStrandsInRow(nStrandsInRow);
      pArtifact->AddNumDebondedStrandsInRow(nDebondStrandsInRow);
      pArtifact->AddFraDebondedStrandsInRow(fra);
      pArtifact->AddMaxFraDebondedStrandsInRow(maxFraPerRow);
      pArtifact->AddIsExteriorStrandDebondedInRow(bExteriorStrandDebonded);
   }

   // Number of debonded strands at a section and section lengths
   Float64 L = pBridge->GetGirderLength( span, gdr );
   Float64 L2 = L/2.0;

   Float64 lmin_section = Float64_Max;
   Float64 lmax_debond_length = 0.0;

   // left end
   Float64 prev_location = 0.0;
   SectionIndexType nSections = pStrandGeometry->GetNumDebondSections(span,gdr,IStrandGeometry::geLeftEnd,strandType);
   for ( SectionIndexType sectionIdx = 0; sectionIdx < nSections; sectionIdx++ )
   {
      StrandIndexType nDebondedStrands = pStrandGeometry->GetNumDebondedStrandsAtSection(span,gdr,IStrandGeometry::geLeftEnd,sectionIdx,strandType);
      Float64 fraDebondedStrands = (nStrands == 0 ? 0 : (Float64)nDebondedStrands/(Float64)nStrands);
      Float64 location = pStrandGeometry->GetDebondSection(span,gdr,IStrandGeometry::geLeftEnd,sectionIdx,strandType);
      pArtifact->AddDebondSection(location,nDebondedStrands,fraDebondedStrands);

      if ( location < 0 || L2 < location )
      {
         ATLASSERT(0);
         continue; // bond occurs after mid-girder... skip this one
      }

      Float64 section_len = location - prev_location;
      lmin_section = min(lmin_section, section_len);
         
      lmax_debond_length = max(lmax_debond_length, location);

      prev_location = location;
   }

   // right end
   nSections = pStrandGeometry->GetNumDebondSections(span,gdr,IStrandGeometry::geRightEnd,strandType);
   for ( SectionIndexType sectionIdx = 0; sectionIdx < nSections; sectionIdx++ )
   {
      StrandIndexType nDebondedStrands = pStrandGeometry->GetNumDebondedStrandsAtSection(span,gdr,IStrandGeometry::geRightEnd,sectionIdx,strandType);
      Float64 fraDebondedStrands = (nStrands == 0 ? 0 : (Float64)nDebondedStrands/(Float64)nStrands);
      Float64 location = pStrandGeometry->GetDebondSection(span,gdr,IStrandGeometry::geRightEnd,sectionIdx,strandType);
      pArtifact->AddDebondSection(location,nDebondedStrands,fraDebondedStrands);

      if ( location < L2 || L < location )
      {
         ATLASSERT(0);
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
         section_len = location - prev_location;
      }

      lmin_section = min(lmin_section, section_len);


      Float64 debond_length = L - location;
      lmax_debond_length = max(lmax_debond_length, debond_length);

      prev_location = location;
   }

   Float64 dll;
   pgsTypes::DebondLengthControl control;
   pDebondLimits->GetMaxDebondLength(span, gdr, &dll, &control);

   pArtifact->SetMaxDebondLength(lmax_debond_length);
   pArtifact->SetDebondLengthLimit(dll, control);

   Float64 dds = pDebondLimits->GetMinDebondSectionDistance(span,gdr);

   if (lmin_section == Float64_Max)
      lmin_section = 0.0;

   pArtifact->SetMinDebondSectionSpacing(lmin_section);
   pArtifact->SetDebondSectionSpacingLimit(dds);
}

void pgsDesigner2::DesignEndZone(bool firstPass, arDesignOptions options, pgsDesignArtifact& artifact, IProgress* pProgress)
{
   // At this point we either have harping or debonding maximized in the end-zones
   // The concrete strength for lifting will control over this case
   // If we are designing for lifting don't figure the concrete strength here
   if (!options.doDesignLifting )
   {
      DesignEndZoneReleaseStrength(pProgress);
      if (  m_DesignerOutcome.WasDesignAborted() )
      {
         return;
      }
      else if (m_DesignerOutcome.DidConcreteChange() )
      {
         return; // concrete strength changed, we will want to redo strands 
      }
   }

   if (m_StrandDesignTool.GetFlexuralDesignType() == dtDesignForHarping)
   {
      DesignEndZoneHarping(options, artifact, pProgress);
   }
   else
   {
      DesignEndZoneDebonding(firstPass, options, artifact, pProgress);
   }
}

void pgsDesigner2::DesignEndZoneDebonding(bool firstPass, arDesignOptions options, pgsDesignArtifact& artifact, IProgress* pProgress)
{
   LOG(_T("Entering DesignEndZoneDebonding"));

   // Refine end-zone design. Lifting will always trump the simple release condition because of the
   // shorter span length.
   // Refine design for lifting. Outcome is lifting loop location and required release strength
   // If temporary strands are required, this design refinement will be incomplete. We will move on to
   // design for hauling because it will typically control the temporary strand requirements. Then,
   // we will return to design for lifting.

   pgsDesignCodes lifting_design_outcome;

   // Compute and layout debonding prior to hauling design
   std::vector<DebondLevelType> debond_demand;

   m_StrandDesignTool.DumpDesignParameters();

   if (options.doDesignLifting && m_StrandDesignTool.GetFlexuralDesignType()==dtDesignForDebonding)
   {
      LOG(_T("*** Initial Lifting Design for Debond Section"));
      DesignForLiftingDebonding(true,pProgress);

      if ( m_DesignerOutcome.WasDesignAborted() )
      {
         LOG(_T("Initial Lifting Debond Design failed"));
         LOG(_T("============================="));
         return;
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
         ATLASSERT(0);
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
         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::DebondDesignFailed);
         m_DesignerOutcome.AbortDesign();
         ATLASSERT(0);
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

      // The only way hauling design can affect liting/release is if temporary strands 
      // were added. Update release strength if this is the case.
      if ( lifting_design_outcome.GetOutcome(pgsDesignCodes::LiftingRedesignAfterShipping) ||
           m_StrandDesignTool.GetNt() > 0 )
      {
         if (options.doDesignLifting && m_StrandDesignTool.GetFlexuralDesignType()==dtDesignForDebonding)
         {
            LOG(_T("*** Secondary Lifting Design after Shipping."));
            std::vector<DebondLevelType> debond_demand_lifting;
            debond_demand_lifting = DesignForLiftingDebonding(false,pProgress);

            // Only layout debonding if first pass through lifting design could not
            if (lifting_design_outcome.GetOutcome(pgsDesignCodes::LiftingRedesignAfterShipping) && !debond_demand_lifting.empty())
            {
               LOG(_T("Release/Lifting demand = ")<<DumpIntVector(debond_demand_lifting));

               bool succ = m_StrandDesignTool.LayoutDebonding( debond_demand_lifting );
               if (!succ)
               {
                  LOG(_T("Failed to layout Debonding - Abort"));
                  LOG(_T("=================================================="));
                  m_StrandDesignTool.SetOutcome(pgsDesignArtifact::DebondDesignFailed);
                  m_DesignerOutcome.AbortDesign();
                  ATLASSERT(0);
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

void pgsDesigner2::DesignEndZoneHarping(arDesignOptions options, pgsDesignArtifact& artifact, IProgress* pProgress)
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
      LOG(_T("*** Design for simple release condition at endzone"));
      DesignEndZoneReleaseHarping(options, pProgress);

      CHECK_PROGRESS;

      if ( m_DesignerOutcome.WasDesignAborted() )
         return;
      else if (m_DesignerOutcome.DidConcreteChange() )
         return; // concrete strength changed, we will want to redo strands 
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

void pgsDesigner2::DesignMidZone(bool bUseCurrentStrands, const arDesignOptions& options,IProgress* pProgress)
{
   if ( bUseCurrentStrands )
      m_StrandDesignTool.DumpDesignParameters();

   Int16 cIter = 0;
   Int16 nFutileAttempts=0;
   Int16 nIterMax = 40;
   Int16 nIterEarlyStage = 5; // Early design stage - NOTE: DO NOT change this value unless you run all design tests VERY SENSITIVE!!!
   StrandIndexType Ns, Nh, Nt;
   Float64 fc, fci, start_slab_offset, end_slab_offset;

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
      start_slab_offset = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
      end_slab_offset   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);

      LOG(_T(""));
      LOG(_T("Initial Design Parameters Trial # ") << cIter);

      if (cIter>1)
      {
         std::_tostringstream os2;
         os2 << _T("Initial Strand Design Trial ")<<cIter << std::ends;
         pProgress->UpdateMessage(os2.str().c_str());
      }

      DesignMidZoneInitialStrands(bUseCurrentStrands ? true : (cIter == 0 ? false : true), pProgress);

      if ( m_DesignerOutcome.WasDesignAborted() )
      {
         if ( m_StrandDesignTool.GetMaxPermanentStrands() > 0 && cIter<=nIterEarlyStage && nFutileAttempts<2)
         {
            // Could be that release strength controls instead of final. Give it a chance.
            LOG(_T("Initial Design Trial # ") << cIter <<_T(" Failed - try to increase release strength to reduce losses"));
            DesignMidZoneAtRelease(options, pProgress);

            // the purpose of calling DesignMidZoneAtRelease is to adjust the initial release strength
            // if it is too low. The new value is also and Initial Strength... re-initialize the
            // Fci controller with the new _T("Initial") strength
            Float64 newFci = m_StrandDesignTool.GetReleaseStrength(&str_result);
            if ( !IsEqual(fci,newFci) )
               m_StrandDesignTool.InitReleaseStrength( newFci );

            // Since it aborted, we know that the initial number of strands was bad. The only good info we have is concrete strength
            if (nFutileAttempts==0)
               m_StrandDesignTool.GuessInitialStrands();

            nFutileAttempts++; // not totally futile, but doesn't work often
         }
         else
            return;
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
         if ( m_StrandDesignTool.GetFlexuralDesignType() == dtDesignForDebonding )
         {
            // For debond design, set debonding to maximum allowed for the current number of strands.
            // This should result in a minimum possible release strength. 
            std::vector<DebondLevelType> max_debonding = m_StrandDesignTool.GetMaxPhysicalDebonding();

            m_StrandDesignTool.LayoutDebonding( max_debonding );
         }

         // find the release strength
         pProgress->UpdateMessage(_T("Finding Release Strength"));

         DesignEndZoneReleaseStrength(pProgress);

         if (  m_DesignerOutcome.WasDesignAborted() )
         {
            return;
         }
         else if ( m_DesignerOutcome.DidConcreteChange() )
         {
            continue; // back to the start of the loop
         }
      }
      else if (cIter<=nIterEarlyStage && m_DesignerOutcome.DidConcreteChange() )
      {
         // give the intial design more chances
         continue; // back to the start of the loop
      }

      // Skip tweaking the concrete strength if we are doing a hauling design
      // Hauling will produce the maximum required strength so we don't need to waste time here
      pProgress->UpdateMessage(_T("Finding Final Strength"));
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
      }

      if (options.doDesignSlabOffset)
      {
         pProgress->UpdateMessage(_T("Designing Slab Offset"));
         DesignSlabOffset( pProgress );

         CHECK_PROGRESS;
         if (  m_DesignerOutcome.WasDesignAborted() )
         {
            return;
         }
      }
      else
      {
         LOG(_T("Skipping Slab Offset Design due to user input"));
      }

      m_StrandDesignTool.DumpDesignParameters();

      // slab offset must be equal to or slightly larger than calculated. If it is smaller, we might under design.
      Float64 AdiffStart = start_slab_offset - m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
      Float64 AdiffEnd   = end_slab_offset   - m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);
      bool Aconverged = (0.0 <= AdiffStart && AdiffStart <= ::ConvertToSysUnits(0.5,unitMeasure::Inch)) &&
                        (0.0 <= AdiffEnd   && AdiffEnd   <= ::ConvertToSysUnits(0.5,unitMeasure::Inch));

      LOG(_T("End of trial ")<<cIter);
      LOG(_T("======================================================================")<<cIter);
      LOG(_T("Ns: ")<< (Ns==m_StrandDesignTool.GetNs() ? _T("Converged"):_T("Did not Converge")) );
      LOG(_T("Nh: ")<< (Nh==m_StrandDesignTool.GetNh() ? _T("Converged"):_T("Did not Converge")) );
      LOG(_T("Nt: ")<< (Nt==m_StrandDesignTool.GetNt() ? _T("Converged"):_T("Did not Converge")) );
      LOG(_T("f'c: ")<< (IsEqual(fc,m_StrandDesignTool.GetConcreteStrength()) ? _T("Converged"):_T("Did not Converge")) );
      LOG(_T("f'ci: ")<< (IsEqual(fci,m_StrandDesignTool.GetReleaseStrength()) ? _T("Converged"):_T("Did not Converge")) );
      LOG(_T("Slab Offset:")<< (Aconverged ? _T("Converged"):_T("Did not Converge")) );
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

   if ( cIter >= nIterMax )
   {
      LOG(_T("Maximum number of iteratations was exceeded - aborting design ") << cIter);
      m_StrandDesignTool.SetOutcome(pgsDesignArtifact::MaxIterExceeded);
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

void pgsDesigner2::DesignMidZoneFinalConcrete(IProgress* pProgress)
{
   // Note that the name of this function is a bit of a misnomer since most of the 
   // limit states here look in end-zone locations, and the main work of mid-zone stress
   // design has already been done by the intial strands design.
   // The real purpose of this function is to ensure that the decisions made during the
   // mid-zone design allow a viable end-zone design further on.
   // At this point harped strands are lifted to their highest point at girder ends, or
   // debonding is maximal. If we can't find a concrete strength here, there is no end-zone design.
   LOG(_T(""));
   LOG(_T("DesignMidZoneFinalConcrete:: Computing required concrete strength"));

   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

   Float64 fc_current = m_StrandDesignTool.GetConcreteStrength();

   Float64 startSlabOffset = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
   Float64 endSlabOffset   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // Maximize stresses at pois for their config
   pgsTypes::LimitState limit_state[] =         {pgsTypes::ServiceI,          lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? pgsTypes::ServiceIA : pgsTypes::FatigueI,         pgsTypes::ServiceIII,         pgsTypes::ServiceI,          pgsTypes::ServiceI,    pgsTypes::ServiceI};
   pgsTypes::Stage stage_type[] =               {pgsTypes::BridgeSite3,       pgsTypes::BridgeSite3,       pgsTypes::BridgeSite3,        pgsTypes::BridgeSite2,       pgsTypes::BridgeSite1, pgsTypes::BridgeSite2};
   pgsTypes::StressType stress_type[] =         {pgsTypes::Compression,       pgsTypes::Compression,       pgsTypes::Tension,            pgsTypes::Compression,       pgsTypes::Compression, pgsTypes::Compression};
   pgsTypes::StressLocation stress_location[] = {pgsTypes::BottomGirder,      pgsTypes::BottomGirder,      pgsTypes::BottomGirder,       pgsTypes::BottomGirder,      pgsTypes::TopGirder,   pgsTypes::TopGirder};
   std::_tstring strLimitState[] =                {_T("Service I (BSS3)"),          lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? _T("Service IA") : _T("Fatigue I"),                _T("Service III"),                _T("Service I (BSS2)"),           _T("Service I (BSS1)"),    _T("Service I (BSS2)")};
   PoiAttributeType find_type[] =               {POI_HARPINGPOINT|POI_PSXFER, POI_HARPINGPOINT|POI_PSXFER, POI_HARPINGPOINT|POI_MIDSPAN, POI_HARPINGPOINT|POI_PSXFER, POI_MIDSPAN,           POI_MIDSPAN};
   Float64 fmax[] =                             {Float64_Max,                 Float64_Max,                 -Float64_Max,                 Float64_Max,                 Float64_Max,           Float64_Max};

   const int NCases=6;
   Float64 fbpre[NCases];

   GET_IFACE(ILimitStateForces,pForces);
   GET_IFACE(IPrestressStresses,pPrestress);
   const GDRCONFIG& config = m_StrandDesignTool.GetGirderConfiguration();

   // store poi and case where max happened (for debugging)
   pgsPointOfInterest maxPoi[NCases];
   Uint32 maxCase[NCases] = {-1,-1,-1,-1};

   int i = 0;
   for ( i = 0; i < NCases; i++ )
   {
      // Get Points of Interest at the expected
      std::vector<pgsPointOfInterest> vPOI( m_StrandDesignTool.GetDesignPoi(stage_type[i],find_type[i]) );
      ATLASSERT(!vPOI.empty());

      LOG(_T("Checking for ") << strLimitState[i] << StrTopBot(stress_location[i]) << (stress_type[i]==pgsTypes::Tension?_T(" Tension"):_T(" Compression")) );

      std::vector<pgsPointOfInterest>::iterator poiIter(vPOI.begin());
      std::vector<pgsPointOfInterest>::iterator poiIterEnd(vPOI.end());
      for ( ; poiIter != poiIterEnd; poiIter++)
      {
         CHECK_PROGRESS;

         const pgsPointOfInterest& poi = *poiIter;
         Float64 min,max;
         if ( analysisType == pgsTypes::Envelope )
         {
            pForces->GetDesignStress(limit_state[i],stage_type[i],poi,stress_location[i],fc_current,startSlabOffset,endSlabOffset,MaxSimpleContinuousEnvelope,&min,&max);
         }
         else
         {
            pForces->GetDesignStress(limit_state[i],stage_type[i],poi,stress_location[i],fc_current,startSlabOffset,endSlabOffset,analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan,&min,&max);
         }

         LOG(_T("     max = ") << ::ConvertFromSysUnits(max,unitMeasure::KSI) << _T(" ksi, min = ") << ::ConvertFromSysUnits(min,unitMeasure::KSI) << _T(" ksi, at ")<< ::ConvertFromSysUnits(poi.GetDistFromStart(), unitMeasure::Feet) << _T(" ft") );

         // save max stress and corresponding prestress stress
         if (stress_type[i] == pgsTypes::Tension)
         {
            if (max > fmax[i])
            {
               fmax[i]=max;
               fbpre[i] = pPrestress->GetDesignStress(stage_type[i],poi,stress_location[i],config);

               maxPoi[i] = poi;
               maxCase[i] = i;
            }
         }
         else
         {
            // compression
            if (min < fmax[i])
            {
               fmax[i]=min;
               fbpre[i] = pPrestress->GetDesignStress(stage_type[i],poi,stress_location[i],config);

               maxPoi[i] = poi;
               maxCase[i] = i;
            }
         }
      }
   }

   GET_IFACE(ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

   Float64 fc_reqd[NCases];

   for ( i = 0; i < NCases; i++ )
   {
      Float64 k = pLoadFactors->DCmax[limit_state[i]];

      LOG(_T("Stress Demand (") << strLimitState[i] << StrTopBot(stress_location[i]) << _T(" fmax = ") << ::ConvertFromSysUnits(fmax[i],unitMeasure::KSI) << _T(" ksi, fbpre = ") << ::ConvertFromSysUnits(fbpre[i],unitMeasure::KSI) << _T(" ksi, ftotal = ") << ::ConvertFromSysUnits(fmax[i] + k*fbpre[i],unitMeasure::KSI) << _T(" ksi, at ")<< ::ConvertFromSysUnits(maxPoi[i].GetDistFromStart(), unitMeasure::Feet) << _T(" ft") );

      fmax[i] += k*fbpre[i];


      ConcStrengthResultType success = m_StrandDesignTool.ComputeRequiredConcreteStrength(fmax[i],stage_type[i],limit_state[i],stress_type[i],&fc_reqd[i]);
      if ( ConcFailed==success )
      {
         ATLASSERT(false);
      }
      else
      {
         m_StrandDesignTool.UpdateConcreteStrength(fc_reqd[i],stage_type[i],limit_state[i],stress_type[i],stress_location[i]);
      }
      LOG(_T(""));
   }

   LOG(_T("Exiting DesignMidZoneFinalConcrete"));
}

void pgsDesigner2::DesignMidZoneAtRelease(const arDesignOptions& options, IProgress* pProgress)
{

   LOG(_T(""));
   LOG(_T("Designing Mid-Zone at Release"));
   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

   GET_IFACE(ILimitStateForces,pForces);
   GET_IFACE(IPointOfInterest,pIPOI);
   GET_IFACE(IPrestressStresses,pPrestress);

   GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();

   // Get Points of Interest in mid-zone
   std::vector<pgsPointOfInterest> vPOI( m_StrandDesignTool.GetDesignPoi(pgsTypes::CastingYard,POI_MIDSPAN|POI_HARPINGPOINT) );
   CHECK( !vPOI.empty() );

   // let's look at bottom compression first. 
   // If we have to increase final strength, we restart
   Float64 fbot = Float64_Max;
   pgsPointOfInterest bot_poi;

   std::vector<pgsPointOfInterest>::iterator poiIter(vPOI.begin());
   std::vector<pgsPointOfInterest>::iterator poiIterEnd(vPOI.end());
   for ( ; poiIter != poiIterEnd; poiIter++)
   {
      CHECK_PROGRESS;

      const pgsPointOfInterest& poi = *poiIter;
      Float64 min, max;
      pForces->GetStress(pgsTypes::ServiceI,pgsTypes::CastingYard,poi,pgsTypes::BottomGirder,false,SimpleSpan,&min,&max);

      Float64  fBotPrestress;
      fBotPrestress = pPrestress->GetDesignStress(pgsTypes::CastingYard,poi,pgsTypes::BottomGirder,config);

      min+=fBotPrestress;

      // save max'd stress and corresponding poi
      if (min< fbot)
      {
            fbot=min;
            bot_poi = poi;
      }
   }

   LOG(_T("Controlling Stress Demand at Release , bottom, compression = ") << ::ConvertFromSysUnits(fbot,unitMeasure::KSI) << _T(" KSI at ")<< ::ConvertFromSysUnits(bot_poi.GetDistFromStart(), unitMeasure::Feet) << _T(" ft") );

   ConcStrengthResultType release_result;
   Float64 fc  = m_StrandDesignTool.GetConcreteStrength();
   Float64 fci = m_StrandDesignTool.GetReleaseStrength(&release_result);
   LOG(_T("current f'c  = ") << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << _T(" KSI") );
   LOG(_T("current f'ci = ") << ::ConvertFromSysUnits(fci,unitMeasure::KSI) << _T(" KSI") );

   Float64 fc_comp;
   ConcStrengthResultType success = m_StrandDesignTool.ComputeRequiredConcreteStrength(fbot,pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Compression,&fc_comp);
   if ( success==ConcFailed )
   {
      LOG(_T("Could not find adequate release strength to control mid-zone compresssion - Design Abort") );
      m_StrandDesignTool.SetOutcome(pgsDesignArtifact::ReleaseStrength);
      m_DesignerOutcome.AbortDesign();
   }

   LOG(_T("Required Release Strength = ") << ::ConvertFromSysUnits(fc_comp,unitMeasure::KSI) << _T(" KSI") );

   // only update if we are increasing release strength - we are downstream here and a decrease is not desired
   if (fc_comp > fci)
   {
      bool bFciUpdated = m_StrandDesignTool.UpdateReleaseStrength(fc_comp, success,pgsTypes::CastingYard,pgsTypes::ServiceI, pgsTypes::Compression, pgsTypes::BottomGirder);
      if ( bFciUpdated )
      {
         fci = m_StrandDesignTool.GetReleaseStrength(&release_result);
         LOG(_T("Release Strength Increased to ")  << ::ConvertFromSysUnits(fci, unitMeasure::KSI) << _T(" KSI"));
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);

         config = m_StrandDesignTool.GetGirderConfiguration();
      }

      // We can continue if we only increase f'ci, but must restart if final was increased
      Float64 fc_new  = m_StrandDesignTool.GetConcreteStrength();
      if (fc!=fc_new)
      {
         LOG(_T("Final Strength Also Increased to ")  << ::ConvertFromSysUnits(fc_new, unitMeasure::KSI) << _T(" KSI"));
         LOG(_T("Restart Design loop"));
         LOG(_T("==================="));
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
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
   Float64 all_tens = pAllowable->GetInitialAllowableTensileStress(fci, release_result==ConcSuccessWithRebar );
   LOG(_T("Allowable tensile stress after Release     = ") << ::ConvertFromSysUnits(all_tens,unitMeasure::KSI) << _T(" KSI") );

   Float64 ftop = -Float64_Max;
   Float64 fetop, fptop;
   pgsPointOfInterest top_poi;

   for ( ; poiIter != poiIterEnd; poiIter++)
   {
      CHECK_PROGRESS;

      const pgsPointOfInterest& poi = *poiIter;
      Float64 mine, maxe;
      pForces->GetStress(pgsTypes::ServiceI,pgsTypes::CastingYard,poi,pgsTypes::TopGirder,false,SimpleSpan,&mine,&maxe);

      Float64 fTopPrestress = pPrestress->GetDesignStress(pgsTypes::CastingYard,poi,pgsTypes::TopGirder,config);

      Float64 max = maxe+fTopPrestress;

      // save max'd stress and corresponding poi
      if (max> ftop)
      {
            ftop=max;
            fetop=maxe;
            fptop=fTopPrestress;
            top_poi = poi;
      }
   }

   LOG(_T("Controlling Stress Demand at Release, Top, Tension = ") << ::ConvertFromSysUnits(ftop,unitMeasure::KSI) << _T(" KSI at ")<< ::ConvertFromSysUnits(top_poi.GetDistFromStart(), unitMeasure::Feet) << _T(" ft") );

   if (ftop>all_tens)
   {
      LOG(_T("Tension limit exceeded - see what we can do"));

      if (m_StrandDesignTool.GetFlexuralDesignType()==dtDesignForHarping)
      {
         LOG(_T("Attempt to adjust harped strands"));
         Float64 pps = m_StrandDesignTool.GetPrestressForceMz(pgsTypes::CastingYard,top_poi);

         // Compute eccentricity required to control top tension
         GET_IFACE(ISectProp2,pSectProp2);
         Float64 Ag  = pSectProp2->GetAg(pgsTypes::CastingYard,top_poi);
         Float64 Stg = pSectProp2->GetStGirder(pgsTypes::CastingYard,top_poi);
         LOG(_T("Ag  = ") << ::ConvertFromSysUnits(Ag, unitMeasure::Inch2) << _T(" in^2"));
         LOG(_T("Stg = ") << ::ConvertFromSysUnits(Stg,unitMeasure::Inch3) << _T(" in^3"));

         Float64 ecc_target = ComputeTopTensionEccentricity( pps, all_tens, fetop, Ag, Stg);
         LOG(_T("Eccentricity Required to control Top Tension   = ") << ::ConvertFromSysUnits(ecc_target, unitMeasure::Inch) << _T(" in"));

         // See if eccentricity can be adjusted and keep Final ServiceIII stresses under control
         Float64 min_ecc = m_StrandDesignTool.GetMinimumFinalMzEccentricity();
         LOG(_T("Min eccentricity for bottom tension at BridgeSite3   = ") << ::ConvertFromSysUnits(min_ecc, unitMeasure::Inch) << _T(" in"));

        StrandIndexType Nh = m_StrandDesignTool.GetNh();

         GET_IFACE(IStrandGeometry,pStrandGeom);
         Float64 offset_inc = pStrandGeom->GetHarpedHpOffsetIncrement(span,gdr);
         if (Nh>0 && offset_inc>=0.0 && !options.doForceHarpedStrandsStraight)
         {
            LOG(_T("Attempt to adjust by raising harped bundles at harping points"));

            Float64 off_reqd = m_StrandDesignTool.ComputeHpOffsetForEccentricity(top_poi, ecc_target,pgsTypes::CastingYard);
            LOG(_T("Harped Hp offset required to achieve controlling Eccentricity   = ") << ::ConvertFromSysUnits(off_reqd, unitMeasure::Inch) << _T(" in"));

            // round to increment
            off_reqd = CeilOff(off_reqd, offset_inc);
            LOG(_T("Hp Offset Rounded to increment of ")<<::ConvertFromSysUnits(offset_inc, unitMeasure::Inch) << _T(" in = ") << ::ConvertFromSysUnits(off_reqd, unitMeasure::Inch) << _T(" in"));

            // offset could push us out of ServiceIII bounds
            Float64 min_off = m_StrandDesignTool.ComputeHpOffsetForEccentricity(top_poi, min_ecc, pgsTypes::BridgeSite3);
            LOG(_T("Offset Required to Create Min Eccentricity Required Final Bottom Tension   = ") << ::ConvertFromSysUnits(min_off, unitMeasure::Inch) << _T(" in"));
            if (off_reqd<=min_off)
            {
               // Attempt to set our offset, this may be lowered to the highest allowed location 
               // if it is out of bounds
               m_StrandDesignTool.SetHarpStrandOffsetHp(off_reqd);
               LOG(_T("New casting yard eccentricity is ") << ::ConvertFromSysUnits( m_StrandDesignTool.ComputeEccentricity(top_poi,pgsTypes::CastingYard), unitMeasure::Inch) << _T(" in"));
               LOG(_T("New BridgeSite 3 eccentricity is ") << ::ConvertFromSysUnits( m_StrandDesignTool.ComputeEccentricity(top_poi,pgsTypes::BridgeSite3), unitMeasure::Inch) << _T(" in"));
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);

               // make sure the job was complete
               Float64 new_off = m_StrandDesignTool.GetHarpStrandOffsetHp();
               if (new_off==off_reqd)
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
               LOG(_T("Hp Offset Rounded to increment of ")<<::ConvertFromSysUnits(offset_inc, unitMeasure::Inch) << _T(" in = ") << ::ConvertFromSysUnits(off_reqd, unitMeasure::Inch) << _T(" in"));

               m_StrandDesignTool.SetHarpStrandOffsetHp(off_reqd);
               LOG(_T("New casting yard eccentricity is ") << ::ConvertFromSysUnits( m_StrandDesignTool.ComputeEccentricity(top_poi,pgsTypes::CastingYard), unitMeasure::Inch) << _T(" in"));
               LOG(_T("New BridgeSite 3 eccentricity is ") << ::ConvertFromSysUnits( m_StrandDesignTool.ComputeEccentricity(top_poi,pgsTypes::BridgeSite3), unitMeasure::Inch) << _T(" in"));
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
            }
         }
         else
         {
            // TxDOT - non-standard adjustment (Texas Two-Step)
            LOG(_T("Attempt to trade straight strands for harped to releive top tension - TxDOT non-standard adjustment"));

            StrandIndexType nh_reqd, ns_reqd;
            if (m_StrandDesignTool.ComputeAddHarpedForMzReleaseEccentricity(top_poi, ecc_target, min_ecc, &ns_reqd, &nh_reqd))
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
               LOG(_T("Attempt to trade straight strands for harped to releive top tension failed."));
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
      ConcStrengthResultType success = m_StrandDesignTool.ComputeRequiredConcreteStrength(ftop,pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Tension,&fci_reqd);
      if ( success!=ConcFailed )
      {
         LOG(_T("Successfully Increased Release Strength for Release , Top, Tension psxfer  = ") << ::ConvertFromSysUnits(fci_reqd,unitMeasure::KSI) << _T(" KSI") );
         m_StrandDesignTool.UpdateReleaseStrength(fci_reqd,success,pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Tension,pgsTypes::TopGirder);
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);

         Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
         if (fc != fc_new)
         {
            LOG(_T("However, Final Was Also Increased to ") << ::ConvertFromSysUnits(fc_new,unitMeasure::KSI) << _T(" KSI") );
            LOG(_T("Restart design with new strengths"));
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
         }
      }
      else
      {
         // Last resort, increase strengths by 500 psi and restart
         bool bSuccess = m_StrandDesignTool.Bump500(pgsTypes::CastingYard, pgsTypes::ServiceI, pgsTypes::Tension, pgsTypes::TopGirder);
         if (bSuccess)
         {
            LOG(_T("Just threw a Hail Mary - Restart design with 500 psi higher concrete strength"));
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
         }
         else
         {
            LOG(_T("Concrete Strength Cannot be adjusted"));
            m_StrandDesignTool.SetOutcome(pgsDesignArtifact::ReleaseStrength);
            m_DesignerOutcome.AbortDesign();
         }
      }
   }  // ftop>all_tens
}

void pgsDesigner2::DesignSlabOffset(IProgress* pProgress)
{
   GET_IFACE(IBridge,pBridge);
   if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
   {
      LOG(_T(""));
      LOG(_T("Skipping A-dimension design because there is no deck"));
      // no deck
      return;
   }

   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

   Float64 AorigStart = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
   Float64 AorigEnd   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);

   // Iterate on _T("A") dimension and initial number of prestressing strands
   // Use a relaxed tolerance on _T("A") dimension.
   Int16 cIter = 0;
   Int16 nIterMax = 20;
   bool bDone = false;

   GET_IFACE(IBridgeMaterial,pMaterial);

   // Iterate until we come up with an _T("A") dimension and some strands
   // that are consistent for the current values of f'c and f'ci
   LOG(_T(""));
   LOG(_T("Computing A-dimension requirement"));
   LOG(_T("A-dim Current (Start)   = ") << ::ConvertFromSysUnits(AorigStart, unitMeasure::Inch) << _T(" in") );
   LOG(_T("A-dim Current (End)     = ") << ::ConvertFromSysUnits(AorigEnd,   unitMeasure::Inch) << _T(" in") );

   
   // to prevent the design from bouncing back and forth over two "A" dimensions that are 1/4" apart, we are going to use the
   // raw computed "A" requirement and round it after design is complete.
   // use a somewhat tight tolerance to converge of the theoretical "A" dimension
   Float64 tolerance = ::ConvertToSysUnits(0.125,unitMeasure::Inch);
   do
   {
      CHECK_PROGRESS;

      std::_tostringstream os2;
      os2 << _T("Slab Offset Design Iteration ")<<cIter+1 << std::ends;
      pProgress->UpdateMessage(os2.str().c_str());

      Float64 AoldStart = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
      Float64 AoldEnd   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);

      // Make a guess at the "A" dimension using this initial strand configuration
      HAUNCHDETAILS haunch_details;
      GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();
      config.SlabOffset[pgsTypes::metStart] = AoldStart;
      config.SlabOffset[pgsTypes::metEnd]   = AoldEnd;
      GetHaunchDetails(span,gdr,config,&haunch_details);

      IndexType idx = haunch_details.Haunch.size()/2;
      LOG(_T("A-dim Calculated        = ") << ::ConvertFromSysUnits(haunch_details.RequiredSlabOffset, unitMeasure::Inch) << _T(" in"));

      Float64 Anew = haunch_details.RequiredSlabOffset;

      Float64 Amin = m_StrandDesignTool.GetMinimumSlabOffset();
      if (Anew < Amin)
      {
         LOG(_T("Calculated A-dim is less than minimum. Using minimum = ") << ::ConvertFromSysUnits(Amin, unitMeasure::Inch) << _T(" in"));
         Anew = Amin;
      }

      if ( IsZero( AoldStart - Anew, tolerance ) && IsZero( AoldEnd - Anew, tolerance ))
      {
         Float64 a;
         a = CeilOff(Max3(AoldStart,AoldEnd,Anew),tolerance );
         m_StrandDesignTool.SetSlabOffset( pgsTypes::metStart, a );
         m_StrandDesignTool.SetSlabOffset( pgsTypes::metEnd,   a );

         bDone = true;
      }
      else
      {
         m_StrandDesignTool.SetSlabOffset( pgsTypes::metStart, Anew );
         m_StrandDesignTool.SetSlabOffset( pgsTypes::metEnd,   Anew );
      }
   } while ( !bDone && cIter++ < nIterMax);

   if ( nIterMax < cIter )
   {
      LOG(_T("Maximum number of iteratations was exceeded - aborting design ") << cIter);
      m_StrandDesignTool.SetOutcome(pgsDesignArtifact::MaxIterExceeded);
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

void pgsDesigner2::DesignMidZoneInitialStrands(bool bUseCurrentStrands,IProgress* pProgress)
{
   // Figure out the number of strands required to make the prestressing
   // work at the bottom centerline of the span at ServiceIII limit state,
   // using the current values for "A", f'c, and f'ci.

   // The only way to continue to the next step from this function is to have adequate concrete
   // strength and the minimum number of strands for tension to control at mid-span

   LOG(_T(""));
   LOG(_T("Computing initial prestressing requirements for Service in Mid-Zone"));

   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

   // Get some information about the girder
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IStrandGeometry,pStrandGeom);

   GET_IFACE(IGirderData,pGdrData);
   const CGirderMaterial* pGirderMaterial = pGdrData->GetGirderMaterial(span,gdr);

   // Get controlling Point of Interest at mid zone
   pgsPointOfInterest poi = GetControllingFinalMidZonePoi(span,gdr);

   Float64 fcgdr = m_StrandDesignTool.GetConcreteStrength();

   // Get the section properties of the girder
   GET_IFACE(ISectProp2,pSectProp2);
   Float64 Ag  = pSectProp2->GetAg(pgsTypes::CastingYard,poi);
   Float64 Stg = pSectProp2->GetStGirder(pgsTypes::CastingYard,poi);
   Float64 Sbg = pSectProp2->GetSb(pgsTypes::CastingYard,poi);
   LOG(_T("Ag  = ") << ::ConvertFromSysUnits(Ag, unitMeasure::Inch2) << _T(" in^2"));
   LOG(_T("Stg = ") << ::ConvertFromSysUnits(Stg,unitMeasure::Inch3) << _T(" in^3"));
   LOG(_T("Sbg = ") << ::ConvertFromSysUnits(Sbg,unitMeasure::Inch3) << _T(" in^3"));

   LOG(_T("Stcg = ") << ::ConvertFromSysUnits(pSectProp2->GetStGirder(pgsTypes::BridgeSite3,poi),unitMeasure::Inch3) << _T(" in^3"));
   LOG(_T("Sbcg = ") << ::ConvertFromSysUnits(pSectProp2->GetSb(pgsTypes::BridgeSite3,poi),unitMeasure::Inch3) << _T(" in^3"));

   LOG(_T("Stcg_adjusted = ") << ::ConvertFromSysUnits(pSectProp2->GetStGirder(pgsTypes::BridgeSite3,poi,fcgdr),unitMeasure::Inch3) << _T(" in^3"));
   LOG(_T("Sbcg_adjusted = ") << ::ConvertFromSysUnits(pSectProp2->GetSb(pgsTypes::BridgeSite3,poi,fcgdr),unitMeasure::Inch3) << _T(" in^3"));

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : analysisType == pgsTypes::Continuous ? ContinuousSpan : MaxSimpleContinuousEnvelope);

   Float64 startSlabOffset = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
   Float64 endSlabOffset   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);

   GET_IFACE(IProductLoads,pProductLoads);
   GET_IFACE(IProductForces,pProductForces);
   pgsTypes::Stage girderLoadStage = pProductLoads->GetGirderDeadLoadStage(span,gdr);
 
   LOG(_T(""));
   LOG(_T("Bridge A dimension  (Start) = ") << ::ConvertFromSysUnits(pBridge->GetSlabOffset(span,gdr,pgsTypes::metStart),unitMeasure::Inch) << _T(" in"));
   LOG(_T("Bridge A dimension  (End)   = ") << ::ConvertFromSysUnits(pBridge->GetSlabOffset(span,gdr,pgsTypes::metEnd),  unitMeasure::Inch) << _T(" in"));
   LOG(_T("Current A dimension (Start) = ") << ::ConvertFromSysUnits(startSlabOffset,unitMeasure::Inch) << _T(" in"));
   LOG(_T("Current A dimension (End)   = ") << ::ConvertFromSysUnits(endSlabOffset,  unitMeasure::Inch) << _T(" in"));
   LOG(_T(""));
   LOG(_T("M girder      = ") << ::ConvertFromSysUnits(pProductForces->GetMoment(girderLoadStage,pftGirder,poi,bat),unitMeasure::KipFeet) << _T(" k-ft"));
   LOG(_T("M diaphragm   = ") << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite1,pftDiaphragm,poi,bat),unitMeasure::KipFeet) << _T(" k-ft"));
   LOG(_T("M shear key   = ") << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite1,pftShearKey,poi,bat),unitMeasure::KipFeet) << _T(" k-ft"));
   LOG(_T("M construction= ") << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite1,pftConstruction,poi,bat),unitMeasure::KipFeet) << _T(" k-ft"));
   LOG(_T("M slab        = ") << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite1,pftSlab,poi,bat),unitMeasure::KipFeet) << _T(" k-ft"));
   LOG(_T("M slab pad    = ") << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite1,pftSlabPad,poi,bat),unitMeasure::KipFeet) << _T(" k-ft"));
   LOG(_T("dM slab pad   = ") << ::ConvertFromSysUnits(pProductForces->GetDesignSlabPadMomentAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi),unitMeasure::KipFeet) << _T(" k-ft"));
   LOG(_T("M panel       = ") << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite1,pftSlabPanel,poi,bat),unitMeasure::KipFeet) << _T(" k-ft"));
   LOG(_T("M user dc (1) = ") << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite1,pftUserDC,poi,bat),unitMeasure::KipFeet) << _T(" k-ft"));
   LOG(_T("M user dw (1) = ") << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite1,pftUserDW,poi,bat),unitMeasure::KipFeet) << _T(" k-ft"));
   LOG(_T("M barrier     = ") << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite2,pftTrafficBarrier,poi,bat),unitMeasure::KipFeet) << _T(" k-ft"));
   LOG(_T("M sidewalk    = ") << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite2,pftSidewalk      ,poi,bat),unitMeasure::KipFeet) << _T(" k-ft"));
   LOG(_T("M user dc (2) = ") << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite2,pftUserDC,poi,bat),unitMeasure::KipFeet) << _T(" k-ft"));
   LOG(_T("M user dw (2) = ") << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite2,pftUserDW,poi,bat),unitMeasure::KipFeet) << _T(" k-ft"));
   LOG(_T("M overlay     = ") << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite3,pftOverlay,poi,bat),unitMeasure::KipFeet) << _T(" k-ft"));

   Float64 Mllmax, Mllmin;
   pProductForces->GetLiveLoadMoment(pgsTypes::lltDesign, pgsTypes::BridgeSite3,poi,bat,true,false,&Mllmin,&Mllmax);
   LOG(_T("M ll+im min   = ") << ::ConvertFromSysUnits(Mllmin,unitMeasure::KipFeet) << _T(" k-ft"));
   LOG(_T("M ll+im max   = ") << ::ConvertFromSysUnits(Mllmax,unitMeasure::KipFeet) << _T(" k-ft"));

   Float64 fc_lldf = fcgdr;
   if ( pGirderMaterial->bUserEc )
      fc_lldf = lrfdConcreteUtil::FcFromEc( pGirderMaterial->Ec, pGirderMaterial->StrengthDensity );

   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);
   Float64 gV, gpM, gnM;
   pLLDF->GetDistributionFactors(poi,pgsTypes::StrengthI,fc_lldf,&gpM,&gnM,&gV);
   LOG(_T("LLDF = ") << gpM);
   LOG(_T(""));

   // Get Service III stress at bottom of girder
   GET_IFACE(IAllowableConcreteStress,pAllowStress);
   GET_IFACE(ILimitStateForces,pForces);
   Float64 fmin[3], fmax[3]; // 0 = Service I, 1 = Service IA, 2 = Service III
   Float64 fAllow[3];
   Float64 fpre[3];
   std::_tstring strLimitState[] = { _T("Service I"), lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? _T("Service IA") : _T("Fatigue I"), _T("Service III") };
   std::_tstring strStressLocation[] = { _T("Top"), _T("Top"), _T("Bottom") };
   pgsTypes::LimitState limit_state[] = { pgsTypes::ServiceI, lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? pgsTypes::ServiceIA : pgsTypes::FatigueI, pgsTypes::ServiceIII };
   pgsTypes::StressLocation stress_location[] = { pgsTypes::TopGirder, pgsTypes::TopGirder, pgsTypes::BottomGirder };
   pgsTypes::StressType stress_type[] = { pgsTypes::Compression, pgsTypes::Compression, pgsTypes::Tension };
   
   GET_IFACE(ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

   Float64 fc = m_StrandDesignTool.GetConcreteStrength();

   int i = 0;
   for ( i = 0; i < 3; i++ )
   {
      LOG(_T(""));
      if ( analysisType == pgsTypes::Envelope )
      {
         Float64 min,max;
         pForces->GetDesignStress(limit_state[i],pgsTypes::BridgeSite3,poi,stress_location[i],fc,startSlabOffset,endSlabOffset,MaxSimpleContinuousEnvelope,&min,&max);
         fmax[i] = max;
         fmin[i] = min;
      }
      else
      {
         pForces->GetDesignStress(limit_state[i],pgsTypes::BridgeSite3,poi,stress_location[i],fc,startSlabOffset,endSlabOffset,analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan,&fmin[i],&fmax[i]);
      }

      Float64 f_demand = ( stress_type[i] == pgsTypes::Compression ) ? fmin[i] : fmax[i];
      LOG(_T("Stress Demand (") << strLimitState[i] << _T(", ") << strStressLocation[i] << _T(", mid-span) = ") << ::ConvertFromSysUnits(f_demand,unitMeasure::KSI) << _T(" KSI") );


      // Get allowable tensile stress 
      fAllow[i] = pAllowStress->GetAllowableStress(pgsTypes::BridgeSite3,limit_state[i],stress_type[i],m_StrandDesignTool.GetConcreteStrength());
      LOG(_T("Allowable stress (") << strLimitState[i] << _T(") = ") << ::ConvertFromSysUnits(fAllow[i],unitMeasure::KSI)  << _T(" KSI"));

      // Compute required stress due to prestressing
      Float64 k = pLoadFactors->DCmax[limit_state[i]];
      fpre[i] = (fAllow[i] - f_demand)/k;

      LOG(_T("Reqd stress due to prestressing (") << strLimitState[i] << _T(") = ") << ::ConvertFromSysUnits(fpre[i],unitMeasure::KSI) << _T(" KSI") );
   }

   // Guess the number of strands if first time through. otherwise use previous guess
   if ( !bUseCurrentStrands )
   {
      // uses minimal number of strands
      m_StrandDesignTool.GuessInitialStrands();
   }
   else
   {
      // Not the first time through. 
      // We are probably here because concrete strength increased and because of that, we may need less strands.
      // The design algorithm can overshoot np because eccentricity typically reduces with increased strands.
      // So, reduce to the next available if possible.
      StrandIndexType np = m_StrandDesignTool.GetNumPermanentStrands();

      StrandIndexType npmin = max(3, m_StrandDesignTool.GetMinimumPermanentStrands());

      if (np > npmin)
      {
         np = m_StrandDesignTool.GetPreviousNumPermanentStrands(np);
         LOG(_T("Reducing num permanent strands from ")<<m_StrandDesignTool.GetNumPermanentStrands()<<_T(" to ")<<np);
         ATLASSERT(np>0);
         m_StrandDesignTool.SetNumPermanentStrands(np);
      }
   }

   // Safety net
   StrandIndexType Np = INVALID_INDEX, Np_old = INVALID_INDEX;
   Int16 cIter = 0;
   Int16 maxIter = 80;

   
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
         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::TooManyStrandsReqd);
         ATLASSERT(0); 
         return;
      }

      LOG(_T("Guess at number of strands -> Ns = ") << m_StrandDesignTool.GetNs() << _T(" Nh = ") << m_StrandDesignTool.GetNh() << _T(" Nt = ") << m_StrandDesignTool.GetNt() );
      m_StrandDesignTool.DumpDesignParameters();

      // Compute initial eccentricity of prestress force using current guess
      Float64 ecc;
      ecc = m_StrandDesignTool.ComputeEccentricity(poi, pgsTypes::BridgeSite3);
      LOG(_T("Eccentricity at mid-span = ") << ::ConvertFromSysUnits(ecc,unitMeasure::Inch) << _T(" in"));

      // Compute prestress force required to acheive fpre
      Float64 P_reqd[3];
      LOG(_T("Required prestress force, P = fpre / [1/Ag + ecc/S]"));
      for ( i = 0; i < 3; i++ )
      {
         Float64 S = (stress_location[i] == pgsTypes::TopGirder ? Stg : Sbg);
         P_reqd[i] = fpre[i]/(1.0/Ag + ecc/S);
         LOG(_T("Required prestress force (") << strLimitState[i] << _T(") = ") << ::ConvertFromSysUnits(fpre[i],unitMeasure::KSI) << _T("/[ 1/") << ::ConvertFromSysUnits(Ag,unitMeasure::Inch2) << _T(" + ") << ::ConvertFromSysUnits(ecc,unitMeasure::Inch) << _T("/") << ::ConvertFromSysUnits(S,unitMeasure::Inch3) << _T("] = ") << ::ConvertFromSysUnits(-P_reqd[i],unitMeasure::Kip) << _T(" Kip"));
      }

      Float64 P = Min3(P_reqd[0],P_reqd[1],P_reqd[2]);
      LOG(_T("Required prestress force = ") << ::ConvertFromSysUnits(-P,unitMeasure::Kip) << _T(" Kip"));
      LOG(_T(""));

      long idx = Min3Index(P_reqd[0],P_reqd[1],P_reqd[2]);
      if ( stress_type[idx] == pgsTypes::Compression )
      {
         LOG(_T("COMPRESSION CONTROLS"));

         // stress required at top of girder to make tension control
         Float64 fpre = P_reqd[2]/Ag + P_reqd[2]*ecc/Stg; 
         LOG(_T("Stress due to prestressing required at top of girder to make tension control = P/Ag + Pe/Stg, where P (") << strLimitState[idx] << _T(") = P (Service III)"));
         LOG(_T("Stress due to prestressing required at top of girder to make tension control = ") << ::ConvertFromSysUnits(P_reqd[2],unitMeasure::Kip) << _T("/") << ::ConvertFromSysUnits(Ag,unitMeasure::Inch2) << _T(" + (") << ::ConvertFromSysUnits(P_reqd[2],unitMeasure::Kip) << _T(")(") << ::ConvertFromSysUnits(ecc,unitMeasure::Inch) << _T(")/") << ::ConvertFromSysUnits(Stg,unitMeasure::Inch3) << _T(" = ") << ::ConvertFromSysUnits(fpre,unitMeasure::KSI) << _T(" KSI"));

         Float64 c = pAllowStress->GetAllowableCompressiveStressCoefficient(pgsTypes::BridgeSite3,limit_state[idx]);
         LOG(_T("Compression stress coefficient ") << c);

         Float64 k = pLoadFactors->DCmax[limit_state[idx]];
         Float64 fc = (fmin[idx]+k*fpre)/-c;

         LOG(_T("Solve for required concrete strength: f ") << strLimitState[idx] << _T(" + (") << k << _T(")(f prestress) = f allowable = (c)(f'c)"));

         LOG(_T("Required concrete strength = [") << ::ConvertFromSysUnits(fmin[idx],unitMeasure::KSI) << _T(" + (") << k << _T(")(") << ::ConvertFromSysUnits(fpre,unitMeasure::KSI) << _T(")] / ") << -c << _T(" = ") << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << _T(" KSI"));

         Float64 fc_min = m_StrandDesignTool.GetMinimumConcreteStrength();
         fc = _cpp_max(fc,fc_min);

         LOG(_T("Required concrete strength (adjusting for minimum allowed value) = ") << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << _T(" KSI"));
         LOG(_T(""));

         bool bFcUpdated = m_StrandDesignTool.UpdateConcreteStrength(fc,pgsTypes::BridgeSite3,limit_state[idx],stress_type[idx],stress_location[idx]);

         if (bFcUpdated)
         {
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
            LOG(_T("** End of strand configuration trial # ") << cIter <<_T(", Compression controlled, f'c changed"));
            return;
         }
         else
         {
            // probably should never get here. intial strand selection too high?
            P = P_reqd[2]; // force tension to control and use service III
            LOG(_T("** Oddball case - compressioned controlled, but did not increase concrete strength. Initial Ns too high?"));
            LOG(_T("Find concrete strength required to satisfy tension limit"));

            const GDRCONFIG& config = m_StrandDesignTool.GetGirderConfiguration();
            GET_IFACE(IPrestressStresses,pPsStress);
            Float64 fBotPre = pPsStress->GetDesignStress(pgsTypes::BridgeSite3,poi,pgsTypes::BottomGirder,config);
            Float64 fc_rq;
            Float64 k = pLoadFactors->DCmax[pgsTypes::ServiceIII];

            Float64 f_allow_required = fmax[2]+k*fBotPre;
            LOG(_T("Required allowable = fb Service III + fb Prestress = ") << ::ConvertFromSysUnits(fmax[2],unitMeasure::KSI) << _T(" + ") << ::ConvertFromSysUnits(fBotPre,unitMeasure::KSI) << _T(" = ") << ::ConvertFromSysUnits(f_allow_required,unitMeasure::KSI) << _T(" KSI"));
            if ( ConcFailed != m_StrandDesignTool.ComputeRequiredConcreteStrength(f_allow_required,pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension,&fc_rq) )
            {
               // Practical upper limit here - if we are going above 15ksi, we are wasting time
               Float64 max_girder_fc = m_StrandDesignTool.GetMaximumConcreteStrength();
               LOG(_T("Max upper limit for final girder concrete = ") << ::ConvertFromSysUnits(max_girder_fc,unitMeasure::KSI) << _T(" KSI. Computed required strength = ")<< ::ConvertFromSysUnits(fc_rq,unitMeasure::KSI) << _T(" KSI"));

               if (fc_rq < max_girder_fc)
               {
                  bool bFcUpdated = m_StrandDesignTool.UpdateConcreteStrength(fc_rq,pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension,pgsTypes::BottomGirder);
                  if ( bFcUpdated )
                  {
                     m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
                     LOG(_T("** Oddball Success - End of strand configuration trial # ") << cIter <<_T(", Compression controlled, f'c changed"));
                     return;
                  }
               }

               // Fell out of possible remedies
               LOG(_T("OddBall Attempt Failed - May not survive this case..."));
               m_StrandDesignTool.SetOutcome(pgsDesignArtifact::TooManyStrandsReqd);
               m_DesignerOutcome.AbortDesign();
               return;
            }

         }
      }

      // if we are here, tension is controlling, see if we can match Np and ecc
      Np = m_StrandDesignTool.ComputePermanentStrandsRequiredForPrestressForce(poi,P);

      if ( Np == INVALID_INDEX )
      {
         StrandIndexType npmax = m_StrandDesignTool.GetMaxPermanentStrands();
         if (m_StrandDesignTool.GetNumPermanentStrands()==npmax)
         {
            LOG(_T("**** TOO MANY STRANDS REQUIRED **** - already tried max= ")<<npmax);

            // OK, This is a final gasp - we have maxed out strands, now see if we can get a reasonable concrete strength
            //     to relieve tension before puking
            LOG(_T("Hail Mary - See if reasonable concrete strength can satisfy tension limit"));
            const GDRCONFIG& config = m_StrandDesignTool.GetGirderConfiguration();
            GET_IFACE(IPrestressStresses,pPsStress);
            Float64 fBotPre = pPsStress->GetDesignStress(pgsTypes::BridgeSite3,poi,pgsTypes::BottomGirder,config);
            Float64 k = pLoadFactors->DCmax[pgsTypes::ServiceIII];
            Float64 f_allow_required = fmax[2]+k*fBotPre;
            LOG(_T("Required allowable = fb Service III + fb Prestress = ") << ::ConvertFromSysUnits(fmax[2],unitMeasure::KSI) << _T(" + ") << ::ConvertFromSysUnits(fBotPre,unitMeasure::KSI) << _T(" = ") << ::ConvertFromSysUnits(f_allow_required,unitMeasure::KSI) << _T(" KSI"));
            Float64 fc_rqd;
            if ( ConcFailed != m_StrandDesignTool.ComputeRequiredConcreteStrength(f_allow_required,pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension,&fc_rqd) )
            {
               // Use user-defined practical upper limit here - if we are going for 15ksi, we are wasting time
               GET_IFACE(ILimits2,pLimits);
               Float64 max_girder_fc = pLimits->GetMaxGirderFc(config.ConcType);
               LOG(_T("User-defined upper limit for final girder concrete = ") << ::ConvertFromSysUnits(max_girder_fc,unitMeasure::KSI) << _T(" KSI. Computed required strength = ")<< ::ConvertFromSysUnits(fc_rqd,unitMeasure::KSI) << _T(" KSI"));

               if (fc_rqd <= max_girder_fc)
               {
                  bool bFcUpdated = m_StrandDesignTool.UpdateConcreteStrength(fc_rqd,pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension,pgsTypes::BottomGirder);
                  if ( bFcUpdated )
                  {
                     m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);

                     // Tricky: Use concrete growth relationship for this case:
                     // Many times the reason we are not converging here is high initial losses due to a low f'ci
                     // Don't allow f'ci to be more than 2ksi smaller than final (TxDOT reasearch supports this value)
                     Float64 fc_curr = m_StrandDesignTool.GetConcreteStrength();
                     ConcStrengthResultType strength_result;
                     Float64 fci_curr = m_StrandDesignTool.GetReleaseStrength(&strength_result);
                     Float64 fc_2k = ::ConvertToSysUnits(2.0,unitMeasure::KSI); // add one to protect lt
                     if (fc_curr-fci_curr > fc_2k)
                     {
                        LOG(_T("  Release strength was more than 2 ksi smaller than final, bump release as well"));
                        m_StrandDesignTool.UpdateReleaseStrength(fci_curr+fc_2k, strength_result, pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension,pgsTypes::BottomGirder);
                        m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);
                     }

                     LOG(_T("** Hail Mary to increase final concrete for tension succeeded - restart design"));
                     return;
                  }
               }
            }

            LOG(_T("Hail Mary - FAILED!! There is no way to satisfy tension limit unless outer loop can fix this problem"));
            m_StrandDesignTool.SetOutcome(pgsDesignArtifact::TooManyStrandsReqd);
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
      // set number of permanent strands
      if (!m_StrandDesignTool.SetNumPermanentStrands(Np))
      {
         LOG(_T("Error trying to set permanent strands - Abort Design"));
         m_DesignerOutcome.AbortDesign();
         return;
      }
      else
      {
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
      }

      LOG(_T("Np = ") << Np_old << _T(" NpGuess = ") << Np);
      LOG(_T("NsGuess = ") << m_StrandDesignTool.GetNs());
      LOG(_T("NhGuess = ") << m_StrandDesignTool.GetNh());
      LOG(_T("NtGuess = ") << m_StrandDesignTool.GetNt());
      LOG(_T("** End of strand configuration trial # ") << cIter <<_T(", Tension controlled"));

      if (Np==Np_old)
      {
         // solution has converged - compute and save the minimum eccentricity that we can have with
         // Np and our allowable. This will be used later to limit strand adjustments in mid-zone
         // We know that Service III controlled because w'ere here:
         Float64 pps = m_StrandDesignTool.GetPrestressForceMz(pgsTypes::BridgeSite3,poi);
         Float64 ecc_min = ComputeBottomCompressionEccentricity( pps, fAllow[2], fmax[2], Ag, Sbg);
         LOG(_T("Minimum eccentricity Required to control Bottom Tension  = ") << ::ConvertFromSysUnits(ecc_min, unitMeasure::Inch) << _T(" in"));
         LOG(_T("Actual current eccentricity   = ") << ::ConvertFromSysUnits(m_StrandDesignTool.ComputeEccentricity(poi, pgsTypes::BridgeSite3), unitMeasure::Inch) << _T(" in"));
         m_StrandDesignTool.SetMinimumFinalMzEccentricity(ecc_min);
      }

      cIter++;
   } while ( Np!=Np_old && cIter<maxIter );

   if ( cIter >= maxIter )
   {
      LOG(_T("Maximum number of iteratations was exceeded - aborting design ") << cIter);
      m_StrandDesignTool.SetOutcome(pgsDesignArtifact::MaxIterExceeded);
      m_DesignerOutcome.AbortDesign();
   }

   LOG(cIter << _T(" iterations were used"));

   LOG(_T(""));
   LOG(_T("Preliminary Design"));
   LOG(_T("Ns = ") << m_StrandDesignTool.GetNs() << _T(" PjS = ") << ::ConvertFromSysUnits(m_StrandDesignTool.GetPjackStraightStrands(),unitMeasure::Kip) << _T(" Kip"));
   LOG(_T("Nh = ") << m_StrandDesignTool.GetNh() << _T(" PjH = ") << ::ConvertFromSysUnits(m_StrandDesignTool.GetPjackHarpedStrands(),unitMeasure::Kip) << _T(" Kip"));
   LOG(_T("Nt = ") << m_StrandDesignTool.GetNt() << _T(" PjT = ") << ::ConvertFromSysUnits(m_StrandDesignTool.GetPjackTempStrands(),unitMeasure::Kip) << _T(" Kip"));
   LOG(_T("** Preliminary Design Complete"));
   LOG(_T("==========================="));
   // Done
}

pgsPointOfInterest pgsDesigner2::GetControllingFinalMidZonePoi(SpanIndexType span,GirderIndexType gdr)
{
   // find location in mid-zone with max stress due to Service III tension
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ISpecification,pSpec);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   Float64 gl = pBridge->GetGirderLength(span,gdr);
   Float64 lhp, rhp;
   m_StrandDesignTool.GetMidZoneBoundaries(&lhp, &rhp);

   Float64 left_limit = lhp;
   Float64 rgt_limit  = rhp;

   Float64 fc = m_StrandDesignTool.GetConcreteStrength();
   Float64 startSlabOffset = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
   Float64 endSlabOffset   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);

   GET_IFACE(ILimitStateForces,pForces);
   GET_IFACE(IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPoi( m_StrandDesignTool.GetDesignPoi(pgsTypes::BridgeSite3,POI_ALLACTIONS) );

   Float64 fmax = -Float64_Max;
   pgsPointOfInterest max_poi;
   bool found=false;
   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++)
   {
      const pgsPointOfInterest& poi = *poiIter;
      Float64 dfs = poi.GetDistFromStart();

      if (dfs>=left_limit && dfs<=rgt_limit)
      {
         // poi is in mid-zone
         Float64 min,max;
         if ( analysisType == pgsTypes::Envelope )
         {
            pForces->GetDesignStress(pgsTypes::ServiceIII,pgsTypes::BridgeSite3,poi,pgsTypes::BottomGirder,fc,startSlabOffset,endSlabOffset,MaxSimpleContinuousEnvelope,&min,&max);
         }
         else
         {
            pForces->GetDesignStress(pgsTypes::ServiceIII,pgsTypes::BridgeSite3,poi,pgsTypes::BottomGirder,fc,startSlabOffset,endSlabOffset,analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan,&min,&max);
         }

         if (max>fmax)
         {
            fmax = max;
            max_poi = poi;
            found = true;
         }
      }
   }

   LOG(_T("Found controlling mid-zone final poi at ")<< ::ConvertFromSysUnits(max_poi.GetDistFromStart(),unitMeasure::Feet) << _T(" ft") );

   ATLASSERT(found);
   return max_poi;
}

void pgsDesigner2::DesignEndZoneReleaseStrength(IProgress* pProgress)
{
   LOG(_T(""));
   LOG(_T("Computing Release requirements at End-Zone - Assumes that harped strands have been raised to highest location or debonding is maximized before entering"));
#pragma Reminder("This code needs to be changed if girder is not assumed to rest on ends at release")

   Float64 fc  = m_StrandDesignTool.GetConcreteStrength();
   Float64 fci = m_StrandDesignTool.GetReleaseStrength();
   LOG(_T("current f'c  = ") << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << _T(" KSI") );
   LOG(_T("current f'ci = ") << ::ConvertFromSysUnits(fci,unitMeasure::KSI) << _T(" KSI") );

   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();
   const GDRCONFIG& config = m_StrandDesignTool.GetGirderConfiguration();

   GET_IFACE(ILimitStateForces,pForces);
   GET_IFACE(IPrestressStresses, pPrestress);
   GET_IFACE(IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPOI( m_StrandDesignTool.GetDesignPoi(pgsTypes::CastingYard,POI_PSXFER) );
   ATLASSERT(!vPOI.empty());

   // max top tension and bottom compression stresses at critical locations
   Float64 fbot =  Float64_Max;
   Float64 ftop = -Float64_Max;
   Float64 fetop, febot; 
   Float64 fptop, fpbot; 
   pgsPointOfInterest top_poi, bot_poi;

   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPOI.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPOI.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      CHECK_PROGRESS;

      const pgsPointOfInterest& poi = *poiIter;
      Float64 mine,maxe,bogus;
      pForces->GetStress(pgsTypes::ServiceI,pgsTypes::CastingYard,poi,pgsTypes::TopGirder,   false,SimpleSpan,&bogus,&maxe);
      pForces->GetStress(pgsTypes::ServiceI,pgsTypes::CastingYard,poi,pgsTypes::BottomGirder,false,SimpleSpan,&mine,&bogus);

      Float64 fTopPrestress, fBotPrestress;
      fTopPrestress = pPrestress->GetDesignStress(pgsTypes::CastingYard,poi,pgsTypes::TopGirder,config);
      fBotPrestress = pPrestress->GetDesignStress(pgsTypes::CastingYard,poi,pgsTypes::BottomGirder,config);

      Float64 max = maxe + fTopPrestress;
      Float64 min = mine + fBotPrestress;

      // save max'd stress and corresponding poi
      if (max > ftop)
      {
            ftop=max;
            fetop = maxe;
            fptop = fTopPrestress;
            top_poi = poi;
      }

      if (min < fbot)
      {
            fbot=min;
            febot = mine;
            fpbot = fBotPrestress;
            bot_poi = poi;
      }
   }

   LOG(_T("Controlling Stress at Release , top, tension psxfer  = ") << ::ConvertFromSysUnits(ftop,unitMeasure::KSI) << _T(" KSI at ")<<::ConvertFromSysUnits(top_poi.GetDistFromStart(),unitMeasure::Feet) << _T(" ft") );
   LOG(_T("Controlling Stress at Release , bottom, compression psxfer = ") << ::ConvertFromSysUnits(fbot,unitMeasure::KSI) << _T(" KSI at ")<<::ConvertFromSysUnits(bot_poi.GetDistFromStart(),unitMeasure::Feet) << _T(" ft"));
   LOG(_T("External Stress Demand at Release , top, tension psxfer  = ") << ::ConvertFromSysUnits(fetop,unitMeasure::KSI) << _T(" KSI") );
   LOG(_T("External Stress Demand at Release , bottom, compression psxfer = ") << ::ConvertFromSysUnits(febot,unitMeasure::KSI) << _T(" KSI") );

   // First crack is to design concrete release strength for harped strands raised to top.
   // No use going further if we can't
   LOG(_T("Try Designing EndZone Release Strength at Initial Condition") );
   DesignConcreteRelease(ftop, fbot);
}

void pgsDesigner2::DesignEndZoneReleaseHarping(const arDesignOptions& options, IProgress* pProgress)
{
   LOG(_T("Refine harped design for release condition"));
   LOG(_T("Computing Release requirements at End-Zone - Assumes that harped strands have been raised to highest location before entering"));
#pragma Reminder("This code needs to be changed if girder is not assumed to rest on ends at release")

   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();
   GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();

   GET_IFACE(ILimitStateForces,pForces);
   GET_IFACE(IPrestressStresses, pPrestress);
   GET_IFACE(IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPOI( m_StrandDesignTool.GetDesignPoi(pgsTypes::CastingYard,POI_PSXFER) );
   ATLASSERT(!vPOI.empty());

   // max top tension and bottom compression stresses at critical locations
   Float64 fbot =  Float64_Max;
   Float64 ftop = -Float64_Max;
   Float64 fetop, febot; 
   Float64 fptop, fpbot; 
   pgsPointOfInterest top_poi, bot_poi;

   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPOI.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPOI.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      CHECK_PROGRESS;

      const pgsPointOfInterest& poi = *poiIter;
      Float64 mine,maxe,bogus;
      pForces->GetStress(pgsTypes::ServiceI,pgsTypes::CastingYard,poi,pgsTypes::TopGirder,   false,SimpleSpan,&bogus,&maxe);
      pForces->GetStress(pgsTypes::ServiceI,pgsTypes::CastingYard,poi,pgsTypes::BottomGirder,false,SimpleSpan,&mine,&bogus);

      Float64 fTopPrestress, fBotPrestress;
      fTopPrestress = pPrestress->GetDesignStress(pgsTypes::CastingYard,poi,pgsTypes::TopGirder,config);
      fBotPrestress = pPrestress->GetDesignStress(pgsTypes::CastingYard,poi,pgsTypes::BottomGirder,config);

      Float64 max = maxe + fTopPrestress;
      Float64 min = mine + fBotPrestress;

      // save max'd stress and corresponding poi
      if (max > ftop)
      {
            ftop=max;
            fetop = maxe;
            fptop = fTopPrestress;
            top_poi = poi;
      }

      if (min < fbot)
      {
            fbot=min;
            febot = mine;
            fpbot = fBotPrestress;
            bot_poi = poi;
      }
   }

   LOG(_T("Controlling Stress at Release , top, tension psxfer  = ") << ::ConvertFromSysUnits(ftop,unitMeasure::KSI) << _T(" KSI") );
   LOG(_T("Controlling Stress at Release , bottom, compression psxfer = ") << ::ConvertFromSysUnits(fbot,unitMeasure::KSI) << _T(" KSI") );
   LOG(_T("External Stress Demand at Release , top, tension psxfer  = ") << ::ConvertFromSysUnits(fetop,unitMeasure::KSI) << _T(" KSI") );
   LOG(_T("External Stress Demand at Release , bottom, compression psxfer = ") << ::ConvertFromSysUnits(febot,unitMeasure::KSI) << _T(" KSI") );

   GET_IFACE(IStrandGeometry,pStrandGeom);

   // See if we can adjust end strands downward
   StrandIndexType Nh = m_StrandDesignTool.GetNh();
   Float64 offset_inc = pStrandGeom->GetHarpedEndOffsetIncrement(span,gdr);

   // Get the section properties of the girder
   GET_IFACE(ISectProp2,pSectProp2);
   Float64 Ag  = pSectProp2->GetAg(pgsTypes::CastingYard,vPOI[0]);
   Float64 Stg = pSectProp2->GetStGirder(pgsTypes::CastingYard,vPOI[0]);
   Float64 Sbg = pSectProp2->GetSb(pgsTypes::CastingYard,vPOI[0]);
   LOG(_T("Ag  = ") << ::ConvertFromSysUnits(Ag, unitMeasure::Inch2) << _T(" in^2"));
   LOG(_T("Stg = ") << ::ConvertFromSysUnits(Stg,unitMeasure::Inch3) << _T(" in^3"));
   LOG(_T("Sbg = ") << ::ConvertFromSysUnits(Sbg,unitMeasure::Inch3) << _T(" in^3"));

   // compute eccentricity to control top tension
   Float64 fc  = m_StrandDesignTool.GetConcreteStrength();
   ConcStrengthResultType conc_res;
   Float64 fci = m_StrandDesignTool.GetReleaseStrength(&conc_res);
   LOG(_T("current f'c  = ") << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << _T(" KSI ")<<(conc_res==ConcSuccessWithRebar ? _T(" min rebar assumed"):_T(" ")));
   LOG(_T("current f'ci = ") << ::ConvertFromSysUnits(fci,unitMeasure::KSI) << _T(" KSI") );

   GET_IFACE(IAllowableConcreteStress,pAllowable);
   Float64 all_tens = pAllowable->GetInitialAllowableTensileStress(fci,conc_res==ConcSuccessWithRebar);
   Float64 all_comp = pAllowable->GetInitialAllowableCompressiveStress(fci);
   LOG(_T("Allowable tensile stress after Release     = ") << ::ConvertFromSysUnits(all_tens,unitMeasure::KSI) << _T(" KSI") );
   LOG(_T("Allowable compressive stress after Release = ") << ::ConvertFromSysUnits(all_comp,unitMeasure::KSI) << _T(" KSI") );

   // ecc's required to control stresses
   Float64 top_pps  = m_StrandDesignTool.GetPrestressForceAtLifting(config,top_poi);
   LOG(_T("Total Prestress Force for top location: P  = ") << ::ConvertFromSysUnits(top_pps, unitMeasure::Kip) << _T(" kip"));

   Float64 ecc_tens = ComputeTopTensionEccentricity( top_pps, all_tens, fetop, Ag, Stg);
   LOG(_T("Eccentricity Required to control Top Tension   = ") << ::ConvertFromSysUnits(ecc_tens, unitMeasure::Inch) << _T(" in"));

   // ecc to control bottom compression
   Float64 bot_pps  = m_StrandDesignTool.GetPrestressForceAtLifting(config,bot_poi);
   LOG(_T("Total Prestress Force for bottom location: P  = ") << ::ConvertFromSysUnits(bot_pps, unitMeasure::Kip) << _T(" kip"));

   Float64 ecc_comp = ComputeBottomCompressionEccentricity( bot_pps, all_comp, febot, Ag, Sbg);
   LOG(_T("Eccentricity Required to control Bottom Compression   = ") << ::ConvertFromSysUnits(ecc_comp, unitMeasure::Inch) << _T(" in"));

   if (m_StrandDesignTool.GetOriginalStrandFillType() == ftMinimizeHarping)
   {
      // try to trade harped to straight and, if necessary, lower strands to achieve eccentricity
      if(ecc_tens<ecc_comp)
         LOG(_T("Tension Controls")); 
      else
         LOG(_T("Compression Controls"));

      Float64 ecc_control = ecc_tens<ecc_comp ? ecc_tens : ecc_comp;
      const pgsPointOfInterest& poi_control = ecc_tens<ecc_comp ? top_poi : bot_poi;

      StrandIndexType Ns = m_StrandDesignTool.GetNs();
      StrandIndexType nh_reqd, ns_reqd;

      LOG(_T("Try to raise end eccentricity by trading harped to straight and lowering ends"));
      if (m_StrandDesignTool.ComputeMinHarpedForEzEccentricity(poi_control, ecc_control, pgsTypes::CastingYard, &ns_reqd, &nh_reqd))
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
      StrandIndexType Nh = m_StrandDesignTool.GetNh();

      if (offset_inc>=0.0 && Nh>0 && !options.doForceHarpedStrandsStraight)
      {
         LOG(_T("Harped strands can be adjusted at ends for release - See how low can we go...") );
         // compute harped offset required to achieve this ecc
         Float64 off_reqd;

         // smallest ecc controls
         if (ecc_tens < ecc_comp)
         {
            LOG(_T("Tension Controls"));
            off_reqd = m_StrandDesignTool.ComputeEndOffsetForEccentricity(top_poi, ecc_tens);
         }
         else
         {
            LOG(_T("Compression Controls"));
            off_reqd = m_StrandDesignTool.ComputeEndOffsetForEccentricity(bot_poi, ecc_comp);
         }

         LOG(_T("Harped End offset required to achieve controlling Eccentricity (raw)   = ") << ::ConvertFromSysUnits(off_reqd, unitMeasure::Inch) << _T(" in"));
         // round to increment
         off_reqd = CeilOff(off_reqd, offset_inc);
         LOG(_T("Harped End offset required to achieve controlling Eccentricity (rounded)  = ") << ::ConvertFromSysUnits(off_reqd, unitMeasure::Inch) << _T(" in"));

         // Attempt to set our offset, this may be lowered to the highest allowed location 
         // if it is out of bounds
         m_StrandDesignTool.SetHarpStrandOffsetEnd(off_reqd);

         m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
      }
      else
      {
         LOG((Nh>0 ? _T("Cannot adjust harped strands due to user input"):_T("There are no harped strands to adjust")));
      }
   }

   CHECK_PROGRESS;

   config = m_StrandDesignTool.GetGirderConfiguration();

#if defined ENABLE_LOGGING
   Float64 neff;
#endif
   LOG(_T("New eccentricity is ") << ::ConvertFromSysUnits( pStrandGeom->GetEccentricity(ecc_tens<ecc_comp?top_poi:bot_poi, config, true, &neff), unitMeasure::Inch) << _T(" in"));


   Float64 fTopPs, fBotPs;
   fTopPs = pPrestress->GetDesignStress(pgsTypes::CastingYard,top_poi,pgsTypes::TopGirder,config);
   fBotPs = pPrestress->GetDesignStress(pgsTypes::CastingYard,bot_poi,pgsTypes::BottomGirder,config);

   ftop = fetop + fTopPs;
   fbot = febot + fBotPs;

   LOG(_T("After Adjustment, Controlling Stress at Release , Top, Tension        = ") << ::ConvertFromSysUnits(ftop,unitMeasure::KSI) << _T(" KSI") );
   LOG(_T("After Adjustment, Controlling Stress at Release , Bottom, Compression = ") << ::ConvertFromSysUnits(fbot,unitMeasure::KSI) << _T(" KSI") );

   // Recompute required release strength
   DesignConcreteRelease(ftop, fbot);

   // Done
}

std::vector<DebondLevelType> pgsDesigner2::DesignEndZoneReleaseDebonding(IProgress* pProgress,bool bAbortOnFail)
{
   LOG(_T("Refine Debonded design by computing debond demand levels for release condition at End-Zone"));
#pragma Reminder("This code needs to be changed if girder is not assumed to rest on ends at release")

   // We also get into this function for fully debonded designs, no use debonding if so
   if (m_StrandDesignTool.GetFlexuralDesignType() != dtDesignForDebonding)
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
   LOG(_T("current f'c  = ") << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << _T(" KSI "));
   LOG(_T("current f'ci = ") << ::ConvertFromSysUnits(fci,unitMeasure::KSI) << _T(" KSI") );

   GET_IFACE(IAllowableConcreteStress,pAllowable);
   Float64 all_tens = pAllowable->GetInitialAllowableTensileStress(fci,rebar_reqd==ConcSuccessWithRebar);
   Float64 all_comp = pAllowable->GetInitialAllowableCompressiveStress(fci);
   LOG(_T("Allowable tensile stress after Release     = ") << ::ConvertFromSysUnits(all_tens,unitMeasure::KSI) << _T(" KSI")<<(rebar_reqd==ConcSuccessWithRebar ? _T(" min rebar was required for this strength"):_T(""))  );
   LOG(_T("Allowable compressive stress after Release = ") << ::ConvertFromSysUnits(all_comp,unitMeasure::KSI) << _T(" KSI") );

   // We want to compute total debond demand, so bond all strands
   GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();
   config.Debond[pgsTypes::Straight].clear();

   // losses due to refined method will be most at end of girder - let's grab the first poi past the transfer length from end of girder
   pgsPointOfInterest sample_poi =  m_StrandDesignTool.GetDebondSamplingPOI(pgsTypes::CastingYard);
   LOG(_T("Debond Design sample POI for prestressing force taken at ")<<::ConvertFromSysUnits(sample_poi.GetDistFromStart(),unitMeasure::Feet) << _T(" ft"));

   GET_IFACE(IPrestressForce,pPrestressForce);
   Float64 strand_force = pPrestressForce->GetPrestressForcePerStrand(sample_poi, config, pgsTypes::Straight, pgsTypes::AfterXfer );

   StrandIndexType nss = config.Nstrands[pgsTypes::Straight];
   LOG(_T("Average force per strand at sampling location = ") << ::ConvertFromSysUnits(strand_force,unitMeasure::Kip) << _T(" kip, with ")<<nss<<_T(" strands"));

   GET_IFACE(ILimitStateForces,pForces);
   GET_IFACE(IPrestressStresses, pPrestress);
   std::vector<pgsPointOfInterest> vPOI( m_StrandDesignTool.GetDesignPoiEndZone(pgsTypes::CastingYard,POI_FLEXURESTRESS) );
   ATLASSERT(!vPOI.empty());

   // Build stress demand
   std::vector<pgsStrandDesignTool::StressDemand> stress_demands;
   stress_demands.reserve(vPOI.size());

   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPOI.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPOI.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi = *poiIter;
      Float64 fTopAppl,fBotAppl,bogus;
      pForces->GetStress(pgsTypes::ServiceI,pgsTypes::CastingYard,poi,pgsTypes::TopGirder,   false,SimpleSpan,&bogus,&fTopAppl);
      pForces->GetStress(pgsTypes::ServiceI,pgsTypes::CastingYard,poi,pgsTypes::BottomGirder,false,SimpleSpan,&fBotAppl,&bogus);

      Float64 fTopPrestress, fBotPrestress;
      fTopPrestress = pPrestress->GetDesignStress(pgsTypes::CastingYard,poi,pgsTypes::TopGirder,config);
      fBotPrestress = pPrestress->GetDesignStress(pgsTypes::CastingYard,poi,pgsTypes::BottomGirder,config);

      // demand stress with fully bonded straight strands
      Float64 fTop = fTopAppl + fTopPrestress;
      Float64 fBot = fBotAppl + fBotPrestress;

      LOG(_T("Computing stresses at ")<<::ConvertFromSysUnits(poi.GetDistFromStart(),unitMeasure::Feet) << _T(" ft"));
      LOG(_T("Applied Top stress = ") << ::ConvertFromSysUnits(fTopAppl,unitMeasure::KSI) << _T(" ksi. Prestress stress =")<< ::ConvertFromSysUnits(fTopPrestress,unitMeasure::KSI) << _T(" ksi. Total stress = ")<< ::ConvertFromSysUnits(fTop,unitMeasure::KSI) << _T(" ksi"));
      LOG(_T("Applied Bottom stress = ") << ::ConvertFromSysUnits(fBotAppl,unitMeasure::KSI) << _T(" ksi. Prestress stress =")<< ::ConvertFromSysUnits(fBotPrestress,unitMeasure::KSI) << _T(" ksi. Total stress = ")<< ::ConvertFromSysUnits(fBot,unitMeasure::KSI) << _T(" ksi"));

      pgsStrandDesignTool::StressDemand demand;
      demand.m_Poi = poi;
      demand.m_TopStress = fTop;
      demand.m_BottomStress = fBot;

      stress_demands.push_back(demand);
   }

   // compute debond levels at each section from demand
   std::vector<DebondLevelType> debond_levels;
   debond_levels = m_StrandDesignTool.ComputeDebondsForDemand(stress_demands, nss, strand_force, all_tens, all_comp);

   if (  debond_levels.empty() && bAbortOnFail )
   {
      ATLASSERT(0);
      LOG(_T("Debonding failed, this should not happen?"));

      m_StrandDesignTool.SetOutcome(pgsDesignArtifact::DebondDesignFailed);
      m_DesignerOutcome.AbortDesign();
   }

   return debond_levels;
}


void pgsDesigner2::DesignConcreteRelease(Float64 ftop, Float64 fbot)
{
   LOG(_T("Entering DesignConcreteRelease"));
   LOG(_T("Total Stress at bottom = ") << ::ConvertFromSysUnits(fbot,unitMeasure::KSI) << _T(" KSI") );
   LOG(_T("Total Stress at top    = ") << ::ConvertFromSysUnits(ftop,unitMeasure::KSI) << _T(" KSI") );

   Float64 fci = m_StrandDesignTool.GetReleaseStrength();
   Float64 fc_old = m_StrandDesignTool.GetConcreteStrength();

   Float64 fc_tens=fci;
   pgsTypes::StressLocation tens_location;
   if (ftop>0.0 || fbot>0.0)
   {
      // have tension stress, determine adequate f'ci
      Float64 ftens;
      if (ftop>fbot)
      {
        ftens = ftop;
        tens_location = pgsTypes::TopGirder;
      }
      else
      {
        ftens = fbot;
        tens_location = pgsTypes::BottomGirder;
      }

      LOG(_T("F'ci to control tension at release is = ") << ::ConvertFromSysUnits(fc_tens,unitMeasure::KSI) << _T(" KSI") );

      ConcStrengthResultType tens_success = m_StrandDesignTool.ComputeRequiredConcreteStrength(ftens,pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Tension,&fc_tens);
      if ( ConcFailed==tens_success )
      {
         LOG(_T("Could not find adequate release strength to control tension - Design Abort") );
         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::ReleaseStrength);
         m_DesignerOutcome.AbortDesign();
         return;
      }
      else
      {
         bool bFciUpdated = m_StrandDesignTool.UpdateReleaseStrength(fc_tens, tens_success, pgsTypes::CastingYard,pgsTypes::ServiceI, pgsTypes::Tension, tens_location);
         if ( bFciUpdated )
         {
            LOG(_T("Release Strength For tension Increased to ")  << ::ConvertFromSysUnits(m_StrandDesignTool.GetReleaseStrength(), unitMeasure::KSI) << _T(" KSI"));
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);

            Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
            if (fc_new!=fc_old)
            {
               LOG(_T("Final Strength Also Increased to ")  << ::ConvertFromSysUnits(fc_new, unitMeasure::KSI) << _T(" KSI"));
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
            }
         }
      }
   }

   Float64 fc_comp=fci;
   pgsTypes::StressLocation comp_location;
   if (ftop<0.0 || fbot<0.0)
   {
      // have compression stress, determine adequate f'ci
      Float64 fcomp;
      if (ftop<fbot)
      {
        fcomp = ftop;
        comp_location = pgsTypes::TopGirder;
      }
      else
      {
        fcomp = fbot;
        comp_location = pgsTypes::BottomGirder;
      }

      LOG(_T("F'ci to control compression at release is = ") << ::ConvertFromSysUnits(fc_comp,unitMeasure::KSI) << _T(" KSI") );

      ConcStrengthResultType success = m_StrandDesignTool.ComputeRequiredConcreteStrength(fcomp,pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Compression,&fc_comp);
      if ( ConcFailed==success )
      {
         LOG(_T("Could not find adequate release strength to control compression - Design Abort") );
         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::ReleaseStrength);
         m_DesignerOutcome.AbortDesign();
         return;
      }
      else
      {
         bool bFciUpdated = m_StrandDesignTool.UpdateReleaseStrength(fc_comp, success, pgsTypes::CastingYard,pgsTypes::ServiceI, pgsTypes::Compression, comp_location);
         if ( bFciUpdated )
         {
            LOG(_T("Release Strength For compression Increased to ")  << ::ConvertFromSysUnits(m_StrandDesignTool.GetReleaseStrength(), unitMeasure::KSI) << _T(" KSI"));
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);

            Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
            if (fc_new!=fc_old)
            {
               LOG(_T("Final Strength Also Increased to ")  << ::ConvertFromSysUnits(fc_new, unitMeasure::KSI) << _T(" KSI"));
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
            }
         }
      }

   }

   LOG(_T("Exiting DesignConcreteRelease"));
}

void pgsDesigner2::DesignForLiftingHarping(const arDesignOptions& options, bool bProportioningStrands,IProgress* pProgress)
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
   SpanIndexType span  = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr = m_StrandDesignTool.GetGirder();

   pgsGirderHandlingChecker checker(m_pBroker,m_StatusGroupID); // this guy can do the stability design!

   GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();
   if ( bProportioningStrands )
   {
      // if this is the first design for lifting, look at the lifting without temporary strands case
      // to get the optimum strand configuration
      LOG(_T("Phase 1 Lifting Design - Design for Lifting without Temporary Strands"));
      LOG(_T("Determine straight/harped strands proportions"));
      LOG(_T(""));
      LOG(_T("Removing temporary strands for lifting analysis"));
      config.Nstrands[pgsTypes::Temporary] = 0;
   }
#if defined _DEBUG
   else
   {
      LOG(_T("Phase 2 Lifting Design - Design for Lifting with Temporary Strands"));
      LOG(_T("Determine lifting locations and release strength requirements"));
   }
#endif

   // Do a stability based design for lifting. this will locate the lift point locations required
   // for stability
   pgsLiftingAnalysisArtifact artifact;
   // Designer manages it's own POIs
   IGirderLiftingDesignPointsOfInterest* pPoiLd = dynamic_cast<IGirderLiftingDesignPointsOfInterest*>(&m_StrandDesignTool);
   pgsDesignCodes::OutcomeType result = checker.DesignLifting(span,gdr,config,pPoiLd,&artifact,LOGGER);

#if defined _DEBUG
   LOG(_T("-- Dump of Lifting Artifact After Design --"));
   artifact.Dump(LOGGER);
   LOG(_T("-- End Dump of Lifting Artifact --"));
#endif

   m_StrandDesignTool.SetLiftingLocations(artifact.GetLeftOverhang(),artifact.GetRightOverhang());

   CHECK_PROGRESS;

   m_DesignerOutcome.SetOutcome(result);
   if ( m_DesignerOutcome.WasDesignAborted() )
   {
      return;
   }

   // Check to see if the girder is stable for lifting
   GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   Float64 FScr    = artifact.GetMinFsForCracking();
   Float64 FScrMin = pGirderLiftingSpecCriteria->GetLiftingCrackingFs();
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
         // estabilish the (most likely maximum) required number of temporary strands.
         //
         // The full design will restart after shipping has added temporary strands so the 
         // next time we enter this function will be for a new phase one design. The straight/harped
         // strands will be proportions considering the temporary strands.

         // Temporary strands are required... 
         LOG(_T("Cannot find a pick point to safisfy FScr"));
         LOG(_T("Temporary strands required"));
         LOG(_T("Move on to Shipping Design"));
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::LiftingRedesignAfterShipping);
      }
      else
      {
         // We are in the second phase lifting design. If we get to this point
         // then the girder is more unstable during lifting than during shipping.
         // This is practically impossible (but could happen if there are strange
         // values used in the shipping stablity analysis). More temporary strands
         // are required for lifting than for shipping.
         
         // Try adding temporary strands
         LOG(_T("Cannot find a pick point to safisfy FScr"));
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
               m_StrandDesignTool.SetOutcome(pgsDesignArtifact::GirderLiftingStability);
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

      GET_IFACE(ISectProp2,pSectProp2);
      GET_IFACE(IPrestressStresses,pPsStress);
      GET_IFACE(IGirderLiftingSpecCriteria,pLiftingCrit);

      // Adjust the proportions of the straight and harped strands such the stress at the harp point
      // is matched by the stress at the lift point or the point of prestress transfer (which ever controls)
      
      ATLASSERT( m_StrandDesignTool.GetFlexuralDesignType() == dtDesignForHarping );

      LOG(_T("--------------------------------------------------------------------------------------------------------------------"));
      LOG(_T("Attempt to reduce and lower harped strands for lifting condition. Use lifting points, or transfer lengths as controlling locations"));

      // get controlling stress at xfer/lift point
      Float64 fbot, bot_loc, ftop, top_loc;
      artifact.GetEndZoneMinMaxRawStresses(&ftop, &fbot, &top_loc, &bot_loc);
      LOG(_T("Max applied top stress at lifting point or transfer location    = ") << ::ConvertFromSysUnits(ftop,unitMeasure::KSI) << _T(" KSI at ")<< ::ConvertFromSysUnits(top_loc,unitMeasure::Feet) << _T(" ft"));
      LOG(_T("Max applied bottom stress at lifting point or transfer location = ") << ::ConvertFromSysUnits(fbot,unitMeasure::KSI) << _T(" KSI at ")<< ::ConvertFromSysUnits(bot_loc,unitMeasure::Feet) << _T(" ft"));
      
      // get top and bottom stresses at harp points
      GET_IFACE(IStrandGeometry,pStrandGeom);
      Float64 lhp,rhp;
      pStrandGeom->GetHarpingPointLocations(span,gdr,&lhp,&rhp);
      std::vector<Float64> hpLocs;
      hpLocs.push_back(lhp);
      hpLocs.push_back(rhp);

      std::vector<Float64> fHpTopMin, fHpTopMax, fHpBotMin, fHpBotMax;
      artifact.GetGirderStress(hpLocs,true, true,fHpTopMin,fHpBotMin);
      artifact.GetGirderStress(hpLocs,false,true,fHpTopMax,fHpBotMax);

      Float64 fTopHpMin = *std::min_element(fHpTopMin.begin(),fHpTopMin.end());
      Float64 fBotHpMin = *std::min_element(fHpBotMin.begin(),fHpBotMin.end());
      Float64 fTopHpMax = *std::max_element(fHpTopMax.begin(),fHpTopMax.end());
      Float64 fBotHpMax = *std::max_element(fHpBotMax.begin(),fHpBotMax.end());
      Float64 fHpMin = _cpp_min(fTopHpMin,fBotHpMin);
      Float64 fHpMax = _cpp_max(fTopHpMax,fBotHpMax);

      LOG(_T("Computing eccentricity required to make stress at lift/xfer point approx equal to stress at hp"));

      pgsPointOfInterest tpoi(span,gdr,top_loc);
      pgsPointOfInterest bpoi(span,gdr,bot_loc);

      // Get the section properties of the girder
      Float64 Agt = pSectProp2->GetAg(pgsTypes::CastingYard,tpoi);
      Float64 Agb = pSectProp2->GetAg(pgsTypes::CastingYard,bpoi);
      Float64 Stg = pSectProp2->GetStGirder(pgsTypes::CastingYard, tpoi);
      Float64 Sbg = pSectProp2->GetSb(pgsTypes::CastingYard, bpoi);
      LOG(_T("Agt = ") << ::ConvertFromSysUnits(Agt, unitMeasure::Inch2) << _T(" in^2"));
      LOG(_T("Agb = ") << ::ConvertFromSysUnits(Agb, unitMeasure::Inch2) << _T(" in^2"));
      LOG(_T("Stg = ") << ::ConvertFromSysUnits(Stg, unitMeasure::Inch3) << _T(" in^3"));
      LOG(_T("Sbg = ") << ::ConvertFromSysUnits(Sbg, unitMeasure::Inch3) << _T(" in^3"));

      Float64 P_for_top = m_StrandDesignTool.GetPrestressForceAtLifting(config,tpoi);
      Float64 P_for_bot;
      if ( IsEqual(top_loc,bot_loc) )
         P_for_bot = P_for_top;
      else
         P_for_bot = m_StrandDesignTool.GetPrestressForceAtLifting(config,bpoi);

      
      LOG(_T("Total Prestress Force for top location: P     = ") << ::ConvertFromSysUnits(P_for_top, unitMeasure::Kip) << _T(" kip"));

      // ecc's required to match stresses at harp point
      Float64 ecc_tens = compute_required_eccentricity(P_for_top,Agt,Stg,ftop,fHpMax);
      LOG(_T("Eccentricity Required to control Top Tension  = ") << ::ConvertFromSysUnits(ecc_tens, unitMeasure::Inch) << _T(" in"));
      LOG(_T("Total Prestress Force for bottom location: P          = ") << ::ConvertFromSysUnits(P_for_bot, unitMeasure::Kip) << _T(" kip"));

      // Note that the _T("exact") way to do this would be to iterate on eccentricity because prestress force is dependent on strand
      // slope, which is dependent on end strand locations. But, so far, no problems????
      Float64 ecc_comp = compute_required_eccentricity(P_for_bot,Agb,Sbg,fbot,fHpMin);
      LOG(_T("Eccentricity Required to control Bottom Compression   = ") << ::ConvertFromSysUnits(ecc_comp, unitMeasure::Inch) << _T(" in"));

#if defined _DEBUG
      if( ecc_tens < ecc_comp)
         LOG(_T("Tension Controls")); 
      else
         LOG(_T("Compression Controls"));
#endif

      // try to trade harped to straight to achieve required eccentricity
      Float64 required_eccentricity = _cpp_min(ecc_tens,ecc_comp);
      const pgsPointOfInterest& poi_control = ecc_tens < ecc_comp ? tpoi : bpoi;

      StrandIndexType Ns = m_StrandDesignTool.GetNs();
      StrandIndexType Nh = m_StrandDesignTool.GetNh();

      StrandIndexType nh_reqd, ns_reqd;

      // At this point, it is assumed that end strands are raised as high as possible
      // See if our target is lower (bigger) than the current.
      Float64 curr_ecc = m_StrandDesignTool.ComputeEccentricity(poi_control,pgsTypes::CastingYard);
      LOG(_T("Eccentricty for current number of strands = ")<< ::ConvertFromSysUnits(curr_ecc, unitMeasure::Inch) << _T(" in"));
      if (curr_ecc <= required_eccentricity) // greater means the CG of prestress force must be lower in the section
      {
         if (m_StrandDesignTool.GetOriginalStrandFillType() == ftMinimizeHarping)
         {
            LOG(_T("Try to increase end eccentricity by trading harped to straight"));
            if (m_StrandDesignTool.ComputeMinHarpedForEzEccentricity(poi_control, required_eccentricity, pgsTypes::CastingYard, &ns_reqd, &nh_reqd))
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
            Float64 offset_inc = pStrandGeom->GetHarpedEndOffsetIncrement(span,gdr);
            if ( 0.0 <= offset_inc && !options.doForceHarpedStrandsStraight )
            {
               LOG(_T("Try to raise end eccentricity by lowering harped strands at ends"));
               Float64 off_reqd = m_StrandDesignTool.ComputeEndOffsetForEccentricity(poi_control, required_eccentricity);

               // round to increment
               LOG(_T("Harped End offset required to achieve controlling Eccentricity (raw)   = ") << ::ConvertFromSysUnits(off_reqd, unitMeasure::Inch) << _T(" in"));
               off_reqd = CeilOff(off_reqd, offset_inc);
               LOG(_T("Harped End offset required to achieve controlling Eccentricity (rounded)  = ") << ::ConvertFromSysUnits(off_reqd, unitMeasure::Inch) << _T(" in"));

               // Attempt to set our offset, this may be lowered to the highest allowed location 
               // if it is out of bounds
               m_StrandDesignTool.SetHarpStrandOffsetEnd(off_reqd);

               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
               LOG(_T("New Eccentricity  = ") << ::ConvertFromSysUnits(m_StrandDesignTool.ComputeEccentricity(poi_control,pgsTypes::CastingYard), unitMeasure::Inch) << _T(" in"));
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
      Float64 fci_comp, fci_tens;
      bool minRebarRequired;
      artifact.GetRequiredConcreteStrength(&fci_comp,&fci_tens,&minRebarRequired);

      // if there isn't a concrete strength that will make the tension limits work,
      // get the heck outta here!
      if ( fci_tens < 0 )
      {
         // there isn't a concrete strength that will work (because of tension limit)
         LOG(_T("There is no concrete strength that will work for lifting after shipping design - Tension controls - FAILED"));
         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::GirderLiftingConcreteStrength);
         m_DesignerOutcome.AbortDesign();
         return; // bye
      }


      // we've got viable concrete strengths
      LOG(_T("Lifting Results : New f'ci (unrounded) comp = ") << ::ConvertFromSysUnits(fci_comp,unitMeasure::KSI) << _T(" ksi, tension = ") << ::ConvertFromSysUnits(fci_tens,unitMeasure::KSI) << _T(" ksi") << _T(" Pick Point = ") << ::ConvertFromSysUnits(artifact.GetLeftOverhang(),unitMeasure::Feet) << _T(" ft"));

      ConcStrengthResultType rebar_reqd = (minRebarRequired) ? ConcSuccessWithRebar : ConcSuccess;

      // get the controlling value
      Float64 fci_required = max(fci_tens,fci_comp);

      // get the maximum allowable f'ci
      Float64 fci_max = m_StrandDesignTool.GetMaximumReleaseStrength();
      if( fci_max < fci_required)
      {
         // required strength is greater than max...
         // sometimes, if we are right at the limit the max value will work... give it a try

         LOG(_T("f'ci max = ") << ::ConvertFromSysUnits(fci_max,unitMeasure::KSI) << _T(" KSI"));
         LOG(_T("f'ci cannot be greater than max. See if we can use max for one last attempt"));

         Float64 fci_curr = m_StrandDesignTool.GetReleaseStrength();

         if (fci_curr != fci_max)
         {
            LOG(_T("Set to max for one more attempt"));
            fci_tens = min(fci_tens, fci_max);
            fci_comp = min(fci_comp, fci_max);
         }
         else
         {
            LOG(_T("Fci max already used.There is no concrete strength that will work for lifting after shipping design - time to abort"));
            m_StrandDesignTool.SetOutcome(pgsDesignArtifact::GirderLiftingConcreteStrength);
            m_DesignerOutcome.AbortDesign();
            return;
         }
      }

      // Set the concrete strength. Set it once for tension and once for compression. The controlling value will stick.
      bool bFciTensionUpdated     = m_StrandDesignTool.UpdateReleaseStrength(fci_tens,rebar_reqd,pgsTypes::Lifting,pgsTypes::ServiceI,pgsTypes::Tension,pgsTypes::TopGirder);
      bool bFciCompressionUpdated = m_StrandDesignTool.UpdateReleaseStrength(fci_comp,rebar_reqd,pgsTypes::Lifting,pgsTypes::ServiceI,pgsTypes::Compression,pgsTypes::BottomGirder);

      if ( bFciTensionUpdated || bFciCompressionUpdated )
      {
         LOG(_T("f'ci has been updated"));
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);
      }

      // check to see if f'c was changed also
      Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
      if (fc_old != fc_new)
      {
         LOG(_T("However, Final Was Also Increased to ") << ::ConvertFromSysUnits(fc_new,unitMeasure::KSI) << _T(" KSI") );
         LOG(_T("Restart design with new strengths"));
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
      }
   } // end else - phase 2 design

   // always retain the strand proportiong after lifting design
   m_DesignerOutcome.SetOutcome(pgsDesignCodes::RetainStrandProportioning);
}

std::vector<DebondLevelType> pgsDesigner2::DesignForLiftingDebonding(bool bProportioningStrands, IProgress* pProgress)
{
   // If designConcrete is true, we want to set the release strength for our real design. If not,
   // the goal is to simply come up with a debonding layout that will work for the strength we compute
   // below. This layout will be used for the fabrication option when temporary strands are not used.

   pProgress->UpdateMessage(_T("Lifting Design for Debonded Girders"));

   std::vector<DebondLevelType> debond_demand;

   Float64 fci_current = m_StrandDesignTool.GetReleaseStrength();

   m_StrandDesignTool.DumpDesignParameters();

   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

   pgsGirderHandlingChecker checker(m_pBroker,m_StatusGroupID);
   GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();

   if ( bProportioningStrands )
   {
      // if this is the first design for lifting, look at the lifting without temporary strands case
      // to get the optimum strand configuration
      LOG(_T("Phase 1 Lifting Design - Design for Lifting without Temporary Strands"));
      LOG(_T("Determine debond strand layout"));
      LOG(_T(""));
      LOG(_T("Removing temporary strands for lifting analysis"));
      config.Nstrands[pgsTypes::Temporary] = 0;
   }
#if defined _DEBUG
   else
   {
      LOG(_T("Phase 2 Lifting Design - Design for Lifting with Temporary Strands"));
      LOG(_T("Determine lifting locations and release strength requirements"));
   }
#endif

   pgsLiftingAnalysisArtifact artifact;
   // Designer manages it's own POIs
   IGirderLiftingDesignPointsOfInterest* pPoiLd = dynamic_cast<IGirderLiftingDesignPointsOfInterest*>(&m_StrandDesignTool);
   pgsDesignCodes::OutcomeType result = checker.DesignLifting(span,gdr,config,pPoiLd,&artifact,LOGGER);

//#if defined _DEBUG
//   LOG(_T("-- Dump of Lifting Artifact After Design --"));
//   artifact.Dump(LOGGER);
//   LOG(_T("-- End Dump of Lifting Artifact --"));
//#endif

   CHECK_PROGRESS;

   m_DesignerOutcome.SetOutcome(result);
   if ( m_DesignerOutcome.WasDesignAborted() )
   {
      return debond_demand;
   }

   // Set the location required for stability
   m_StrandDesignTool.SetLiftingLocations(artifact.GetLeftOverhang(),artifact.GetRightOverhang());
   m_DesignerOutcome.SetOutcome(pgsDesignCodes::LiftingConfigChanged);

   // Check to see if the girder is stable for lifting
   GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   Float64 FScr    = artifact.GetMinFsForCracking();
   Float64 FScrMin = pGirderLiftingSpecCriteria->GetLiftingCrackingFs();
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
      // estabilish the (most likely maximum) required number of temporary strands.
      //
      // The full design will restart after shipping has added temporary strands so the 
      // next time we enter this function we will design strength and layout for temp strand design

      // Temporary strands are required... 
      LOG(_T("Cannot find a pick point to safisfy FScr"));
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
         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::GirderLiftingStability);
         m_DesignerOutcome.AbortDesign();
      }

      return debond_demand;
   }

   Float64 fc_old = m_StrandDesignTool.GetConcreteStrength();

   // Get required release strength required from artifact
   Float64 fci_comp, fci_tens;
   bool minRebarRequired;
   artifact.GetRequiredConcreteStrength(&fci_comp,&fci_tens,&minRebarRequired);
   LOG(_T("Required Lifting Release Strength from artifact : f'ci (unrounded) tens = ") << ::ConvertFromSysUnits(fci_tens,unitMeasure::KSI) << _T(" KSI, compression = ") << ::ConvertFromSysUnits(fci_comp,unitMeasure::KSI) << _T(" KSI, Pick Point = ") << ::ConvertFromSysUnits(artifact.GetLeftOverhang(),unitMeasure::Feet) << _T(" ft"));
   ATLASSERT( fci_tens >= 0 ); // This should never happen if FScr is OK

   // Slight changes in losses going from one strength to another can cause convergence problems. Also a strength too tight
   // might cause our debond design to fail.
   // Artificially bump strengths a bit in case were are on the edge
   const Float64 LiftingFudge = 1.02;

   // If we do not set the concrete strength, the reqd value below is for our debond design. Otherwise we will use the designed value
   Float64 fci_reqd; // our required strength

   if (!bProportioningStrands)
   {
      bool bFciUpdated=false;


      // make sure new f'ci fits in the code limits
      Float64 fci_max = m_StrandDesignTool.GetMaximumReleaseStrength();
      if (fci_tens>fci_max || fci_comp>fci_max)
      {
         // strength needed is more than max allowed. Try setting to max for one more design go-around
         Float64 fci_curr = m_StrandDesignTool.GetReleaseStrength();
         if (fci_max==fci_curr)
         {
            LOG(_T("Release strength required for lifting is greater than our current max, and we have already tried max for design - Design Failed") );
            m_StrandDesignTool.SetOutcome(pgsDesignArtifact::GirderLiftingConcreteStrength);
            m_DesignerOutcome.AbortDesign();
            return debond_demand;
         }
         else
         {
            LOG(_T("Strength required for lifting is greater than our current max of ") << ::ConvertFromSysUnits(fci_max,unitMeasure::KSI) << _T(" KSI - Try using max for one more go-around") );
            if (fci_comp>fci_max)
               fci_comp = fci_max;

            if (fci_tens>fci_max)
               fci_tens = fci_max;
         }
      }
      else
      {
         fci_tens *= LiftingFudge;
         fci_comp *= LiftingFudge;

         fci_tens = min(fci_tens, fci_max);
         fci_comp = min(fci_comp, fci_max);
      }

      ConcStrengthResultType rebar_reqd = (minRebarRequired) ? ConcSuccessWithRebar : ConcSuccess;

      // update both for tension and compression. NOTE: using a dummy stress location here
      bFciUpdated |= m_StrandDesignTool.UpdateReleaseStrength(fci_tens,rebar_reqd,pgsTypes::Lifting,pgsTypes::ServiceI,pgsTypes::Tension,pgsTypes::TopGirder);
      bFciUpdated |= m_StrandDesignTool.UpdateReleaseStrength(fci_comp,rebar_reqd,pgsTypes::Lifting,pgsTypes::ServiceI,pgsTypes::Compression,pgsTypes::BottomGirder);
      if (bFciUpdated)
      {
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);

         Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
         if (fc_old != fc_new)
         {
            LOG(_T("However, Final Was Also Increased to ") << ::ConvertFromSysUnits(fc_new,unitMeasure::KSI) << _T(" KSI") );
            LOG(_T("May need to Restart design with new strengths"));
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
            return debond_demand;
         }
         else
         {
            LOG(_T("Release strength increased for lifting - design continues..."));
         }
      }

      fci_reqd =  m_StrandDesignTool.GetReleaseStrength();
   }
   else
   {
      // just get the concrete strength we want to use for our debond layout
      fci_reqd = max(fci_comp, fci_tens);
      fci_reqd *= LiftingFudge;
   }

   // Now that we have an established concrete strength, we can use it to design our debond layout

   HANDLINGCONFIG lift_config;
   lift_config.GdrConfig = m_StrandDesignTool.GetGirderConfiguration();

   lift_config.GdrConfig.Fci = fci_reqd;
   lift_config.LeftOverhang = m_StrandDesignTool.GetLeftLiftingLocation();
   lift_config.RightOverhang = m_StrandDesignTool.GetRightLiftingLocation();

   return DesignDebondingForLifting(lift_config, pProgress);
}


std::vector<DebondLevelType> pgsDesigner2::DesignDebondingForLifting(HANDLINGCONFIG& liftConfig, IProgress* pProgress)
{
   pProgress->UpdateMessage(_T("Designing initial debonding for Lifting"));
   ATLASSERT(m_StrandDesignTool.GetFlexuralDesignType() == dtDesignForDebonding);

   // set up our vector to return debond levels at each section
   SectionIndexType max_db_sections = m_StrandDesignTool.GetMaxNumberOfDebondSections();
   std::vector<DebondLevelType> lifting_debond_levels;
   lifting_debond_levels.assign(max_db_sections,0);

   LOG(_T(""));
   LOG(_T("Detailed Debond Design for Lifting"));
   LOG(_T(""));
   m_StrandDesignTool.DumpDesignParameters();

   {
      SpanIndexType   span = m_StrandDesignTool.GetSpan();
      GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

      Float64 fc  = liftConfig.GdrConfig.Fc;
      Float64 fci = liftConfig.GdrConfig.Fci;
      LOG(_T("current f'c  = ") << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << _T(" KSI "));
      LOG(_T("current f'ci = ") << ::ConvertFromSysUnits(fci,unitMeasure::KSI) << _T(" KSI") );

      GET_IFACE(IGirderLiftingSpecCriteria,pLiftingCrit);
      Float64 all_tens = pLiftingCrit->GetLiftingAllowableTensileConcreteStressEx(fci,true);
      Float64 all_comp = pLiftingCrit->GetLiftingAllowableCompressiveConcreteStressEx(fci);
      LOG(_T("Allowable tensile stress after Release     = ") << ::ConvertFromSysUnits(all_tens,unitMeasure::KSI) << _T(" KSI - min rebar was required for this strength"));
      LOG(_T("Allowable compressive stress after Release = ") << ::ConvertFromSysUnits(all_comp,unitMeasure::KSI) << _T(" KSI") );

      // This is an analysis to determine stresses that must be reduced by debonding
      LOG(_T("Debond levels measured from fully bonded section"));
      liftConfig.GdrConfig.Debond[pgsTypes::Straight].clear();

      pgsLiftingAnalysisArtifact artifact;

      pgsGirderHandlingChecker checker(m_pBroker,m_StatusGroupID);
      // Designer manages it's own POIs
      IGirderLiftingDesignPointsOfInterest* pPoiLd = dynamic_cast<IGirderLiftingDesignPointsOfInterest*>(&m_StrandDesignTool);
      checker.AnalyzeLifting(span,gdr,liftConfig,pPoiLd,&artifact);

      StrandIndexType nss = liftConfig.GdrConfig.Nstrands[pgsTypes::Straight];
      StrandIndexType nts = nss + liftConfig.GdrConfig.Nstrands[pgsTypes::Harped] + liftConfig.GdrConfig.Nstrands[pgsTypes::Temporary]; // to get an average force per strand
      Float64 force_per_strand = 0.0;

      // get vector of max applied stresses from artifact
      pgsLiftingAnalysisArtifact::MaxLiftingStressCollection max_applied_stresses;
      artifact.GetMinMaxLiftingStresses(max_applied_stresses);
      ATLASSERT(!max_applied_stresses.empty());

      // only want stresses in end zones
      Float64 rgt_end, lft_end;
      m_StrandDesignTool.GetMidZoneBoundaries(&lft_end, &rgt_end);

      // we'll pick strand force at location just past transfer length
      Float64 xfer_length = m_StrandDesignTool.GetTransferLength(pgsTypes::Permanent);

      // Build stress demand
      std::vector<pgsStrandDesignTool::StressDemand> stress_demands;
      stress_demands.reserve(max_applied_stresses.size());
      LOG(_T("--- Compute lifting stresses for debonding --- nss = ")<<nss);
      pgsLiftingAnalysisArtifact::MaxLiftingStressIterator it(max_applied_stresses.begin());
      pgsLiftingAnalysisArtifact::MaxLiftingStressIterator itEnd(max_applied_stresses.end());
      for ( ; it != itEnd; it++)
      {
         const pgsLiftingAnalysisArtifact::MaxdLiftingStresses& max_stresses = *it;

         Float64 poi_loc = max_stresses.m_LiftingPoi.GetDistFromStart();
         if(poi_loc <= lft_end || poi_loc >= rgt_end)
         {
            // get strand force if we haven't yet
            if (poi_loc >= xfer_length && force_per_strand==0.0)
            {
               force_per_strand = max_stresses.m_PrestressForce / nts;
               LOG(_T("Sample prestress force per strand taken at ")<< ::ConvertFromSysUnits(poi_loc,unitMeasure::Feet)<<_T(" ft, force = ") << ::ConvertFromSysUnits(force_per_strand, unitMeasure::Kip) << _T(" kip"));
            }

            Float64 fTop = max_stresses.m_TopMaxStress;
            Float64 fBot = max_stresses.m_BottomMinStress;

            LOG(_T("At ")<< ::ConvertFromSysUnits(poi_loc,unitMeasure::Feet)<<_T(" ft, Ftop = ")<< ::ConvertFromSysUnits(fTop,unitMeasure::KSI) << _T(" ksi Fbot = ")<< ::ConvertFromSysUnits(fBot,unitMeasure::KSI) << _T(" ksi") );
            LOG(_T("Average force per strand = ") << ::ConvertFromSysUnits(max_stresses.m_PrestressForce / nss,unitMeasure::Kip) << _T(" kip"));

            pgsStrandDesignTool::StressDemand demand;
            demand.m_Poi  = max_stresses.m_LiftingPoi;
            demand.m_TopStress = fTop;
            demand.m_BottomStress = fBot;

            stress_demands.push_back(demand);
         }
      }

      // compute debond levels at each section from demand
      lifting_debond_levels = m_StrandDesignTool.ComputeDebondsForDemand(stress_demands, nss, force_per_strand, all_tens, all_comp);

      if (  lifting_debond_levels.empty() )
      {
         ATLASSERT(0);
         LOG(_T("Debonding failed, this should not happen?"));

         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::DebondDesignFailed);
         m_DesignerOutcome.AbortDesign();
      }
   }

   return lifting_debond_levels;
}



void pgsDesigner2::DesignForShipping(IProgress* pProgress)
{
   pProgress->UpdateMessage(_T("Designing for Shipping"));

   LOG(_T(""));
   LOG(_T("DESIGNING FOR SHIPPING"));
   LOG(_T(""));

   m_StrandDesignTool.DumpDesignParameters();

   Float64 fc_current = m_StrandDesignTool.GetConcreteStrength();

   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

   pgsGirderHandlingChecker checker(m_pBroker,m_StatusGroupID);
   
   pgsHaulingAnalysisArtifact artifact;
   bool bResult = false;
   bool bTemporaryStrandsAdded = false;

   do
   {
      const GDRCONFIG& config = m_StrandDesignTool.GetGirderConfiguration();

      IGirderHaulingDesignPointsOfInterest* pPoiLd = dynamic_cast<IGirderHaulingDesignPointsOfInterest*>(&m_StrandDesignTool);
      bResult = checker.DesignShipping(span,gdr,config,m_bShippingDesignWithEqualCantilevers,m_bShippingDesignIgnoreConfigurationLimits,pPoiLd,&artifact,LOGGER);

      // capture the results of the trial
      m_StrandDesignTool.SetTruckSupportLocations(artifact.GetTrailingOverhang(),artifact.GetLeadingOverhang());
      if (!bResult )
      {
         LOG(_T("Adding temporary strands"));
         if ( !m_StrandDesignTool.AddTempStrands() )
         {
            if ( !m_bShippingDesignWithEqualCantilevers )
            {
               LOG(_T("Could not add temporary strands - go to equal cantilever method and continue"));
               // the design isn't working with unequal cantilevers
               // start again with equal cantilevesr
               m_StrandDesignTool.SetNumTempStrands(0);
               m_bShippingDesignWithEqualCantilevers = true;
               m_bShippingDesignIgnoreConfigurationLimits = true;
               continue;
            }

            LOG(_T("Could not add temporary strands - check to see if we are up against the geometric limits of the shipping configuration"));
            GET_IFACE(IGirderHaulingSpecCriteria,pCriteria);
            Float64 maxDistanceBetweenSupports = pCriteria->GetAllowableDistanceBetweenSupports();
            Float64 maxLeadingOverhang = pCriteria->GetAllowableLeadingOverhang();
            Float64 distBetweenSupportPoints = m_StrandDesignTool.GetGirderLength() - artifact.GetTrailingOverhang() - artifact.GetLeadingOverhang();
            if ( IsEqual(maxLeadingOverhang,artifact.GetLeadingOverhang()) &&
                 IsEqual(maxDistanceBetweenSupports,distBetweenSupportPoints) )
            {
               LOG(_T("Failed to satisfy shipping requirements - shipping configuration prevents a suitable solution from being found"));
               m_StrandDesignTool.SetOutcome(pgsDesignArtifact::GirderShippingConfiguration);
               m_DesignerOutcome.AbortDesign();
               return;
            }

            LOG(_T("Could not add temporary strands - attempt to bump concrete strength by 500psi"));
            bool bSuccess = m_StrandDesignTool.Bump500(pgsTypes::Hauling, pgsTypes::ServiceI, pgsTypes::Tension, pgsTypes::TopGirder);
            if (bSuccess)
            {
               LOG(_T("Concrete strength was increased for shipping - Restart") );
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
               return;
            }
            else
            {
               LOG(_T("Failed to increase concrete strength and could not add temporary strands - abort"));
               m_StrandDesignTool.SetOutcome(pgsDesignArtifact::GirderShippingStability);
               m_DesignerOutcome.AbortDesign();
               return;
            }
         }
         else
         {
            LOG(_T("Temporary strands added. Restart design"));
            bTemporaryStrandsAdded = true;
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::TemporaryStrandsChanged);
            continue; // go back to top of loop and try again
         }
      }
   } while ( !bResult );


   CHECK_PROGRESS;

#if defined _DEBUG
   LOG(_T("-- Dump of Hauling Artifact After Design --"));
   artifact.Dump(LOGGER);
   LOG(_T("-- End Dump of Hauling Artifact --"));
#endif

   // We now have bunk point locations to ensure stability
   m_DesignerOutcome.SetOutcome(pgsDesignCodes::HaulingConfigChanged);
   
   Float64 fc_max = m_StrandDesignTool.GetMaximumConcreteStrength();

   Float64 fc_tens, fc_comp;
   bool minRebarRequired;
   artifact.GetRequiredConcreteStrength(&fc_comp, &fc_tens, &minRebarRequired,fc_max,true);
   LOG(_T("f'c (unrounded) required for shipping; tension = ") << ::ConvertFromSysUnits(fc_tens,unitMeasure::KSI) << _T(" KSI, compression = ") << ::ConvertFromSysUnits(fc_comp,unitMeasure::KSI) << _T(" KSI"));

   if ( fc_tens < 0 )
   {
      // there isn't a concrete strength that will work (because of tension limit)

      // Add temporary strands and try again.
      LOG(_T("There is no concrete strength that will work for shipping - Tension controls... Adding temporary strands"));
      if ( !m_StrandDesignTool.AddTempStrands() )
      {
         LOG(_T("Could not add temporary strands"));
         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::GirderShippingConcreteStrength);
         m_DesignerOutcome.AbortDesign();
         return;
      }
      else
      {
         LOG(_T("Temporary strands added. Restart design"));
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::TemporaryStrandsChanged);
         return;
      }
   }

   LOG(_T("Shipping Results : f'c (unrounded) tens = ") << ::ConvertFromSysUnits(fc_tens,unitMeasure::KSI) << _T(" KSI, Comp = ") << ::ConvertFromSysUnits(fc_comp,unitMeasure::KSI)<<_T("KSI, Left Bunk Point = ") << ::ConvertFromSysUnits(artifact.GetTrailingOverhang(),unitMeasure::Feet) << _T(" ft") << _T("    Right Bunk Point = ") << ::ConvertFromSysUnits(artifact.GetLeadingOverhang(),unitMeasure::Feet) << _T(" ft"));

   CHECK_PROGRESS;

   if (fc_tens>fc_max || fc_comp>fc_max )
   {
      // strength needed is more than max allowed. Try setting to max for one more design go-around
      Float64 fc_curr = m_StrandDesignTool.GetConcreteStrength();
      if (fc_max==fc_curr)
      {
         LOG(_T("Strength required for shipping is greater than our current max, and we have already tried max for design - Design Failed") );
         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::GirderShippingConcreteStrength);
         m_DesignerOutcome.AbortDesign();
         return;
      }
      else
      {
         LOG(_T("Strength required for shipping is greater than our current max of ") << ::ConvertFromSysUnits(fc_max,unitMeasure::KSI) << _T(" KSI - Try using max for one more go-around") );
         if (fc_comp>fc_max)
            fc_comp = fc_max;

         if (fc_tens>fc_max)
            fc_tens = fc_max;
      }
   }

   CHECK_PROGRESS;

   // NOTE: Using bogus stress location
   bool bFcUpdated = m_StrandDesignTool.UpdateConcreteStrength(fc_tens,pgsTypes::Hauling,pgsTypes::ServiceI,pgsTypes::Tension,pgsTypes::TopGirder);
   bFcUpdated |= m_StrandDesignTool.UpdateConcreteStrength(fc_comp,pgsTypes::Hauling,pgsTypes::ServiceI,pgsTypes::Compression,pgsTypes::BottomGirder);
   if ( bFcUpdated )
   {
      LOG(_T("Concrete strength was increased for shipping - Restart") );
      m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
      return;
   }

   if ( bTemporaryStrandsAdded )
   {
      LOG(_T("Temporary strands were added for shipping - Restart") );
      m_DesignerOutcome.SetOutcome(pgsDesignCodes::TemporaryStrandsChanged);
      return;
   }

   LOG(_T("Shipping Design Complete - Continue design") );
}


std::vector<DebondLevelType> pgsDesigner2::DesignForShippingDebondingFinal(IProgress* pProgress)
{
   pProgress->UpdateMessage(_T("Designing final debonding for Shipping"));

   // fine-tune debonding for shipping
   SectionIndexType max_db_sections = m_StrandDesignTool.GetMaxNumberOfDebondSections();

   // set up our vector to return debond levels at each section
   std::vector<DebondLevelType> shipping_debond_levels;
   shipping_debond_levels.assign(max_db_sections,0);

   Float64 fci_current = m_StrandDesignTool.GetReleaseStrength();

   LOG(_T(""));
   LOG(_T("DESIGNING DEBONDING FOR SHIPPING"));
   LOG(_T(""));
   m_StrandDesignTool.DumpDesignParameters();

   if (m_StrandDesignTool.GetFlexuralDesignType() == dtDesignForDebonding)
   {
      SpanIndexType   span = m_StrandDesignTool.GetSpan();
      GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

      Float64 fc  = m_StrandDesignTool.GetConcreteStrength();
      ConcStrengthResultType rebar_reqd;
      Float64 fci = m_StrandDesignTool.GetReleaseStrength(&rebar_reqd);
      LOG(_T("current f'c  = ") << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << _T(" KSI "));
      LOG(_T("current f'ci = ") << ::ConvertFromSysUnits(fci,unitMeasure::KSI) << _T(" KSI") );

      GET_IFACE(IGirderHaulingSpecCriteria,pHaulingCrit);
      Float64 all_tens = pHaulingCrit->GetHaulingAllowableTensileConcreteStressEx(fc,rebar_reqd==ConcSuccessWithRebar);
      Float64 all_comp = pHaulingCrit->GetHaulingAllowableCompressiveConcreteStressEx(fc);
      LOG(_T("Allowable tensile stress at hauling     = ") << ::ConvertFromSysUnits(all_tens,unitMeasure::KSI) << _T(" KSI")<<(rebar_reqd==ConcSuccessWithRebar ? _T(" min rebar was required for this strength"):_T(""))  );
      LOG(_T("Allowable compressive stress at hauling = ") << ::ConvertFromSysUnits(all_comp,unitMeasure::KSI) << _T(" KSI") );

      // debond levels must be measured from fully bonded section
      GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();
      config.Debond[pgsTypes::Straight].clear();

      // This is an analysis to determine stresses that must be reduced by debonding
      // maximum debond level is used at this point and we should pass spec check
      pgsHaulingAnalysisArtifact artifact;

      HANDLINGCONFIG ship_config;
      ship_config.GdrConfig = config;
      ship_config.LeftOverhang = m_StrandDesignTool.GetLeadingOverhang();
      ship_config.RightOverhang = m_StrandDesignTool.GetTrailingOverhang();

      pgsGirderHandlingChecker checker(m_pBroker,m_StatusGroupID);
      IGirderHaulingDesignPointsOfInterest* pPoiLd = dynamic_cast<IGirderHaulingDesignPointsOfInterest*>(&m_StrandDesignTool);

      checker.AnalyzeHauling(span,gdr,ship_config,pPoiLd,&artifact);

      StrandIndexType nss = config.Nstrands[pgsTypes::Straight];
      StrandIndexType nts = nss + config.Nstrands[pgsTypes::Harped] + config.Nstrands[pgsTypes::Temporary]; // to get an average force per strand
      Float64 force_per_strand = 0.0;

      // get vector of max stresses from artifact
      pgsHaulingAnalysisArtifact::MaxHaulingStressCollection max_applied_stresses;
      artifact.GetMinMaxHaulingStresses(max_applied_stresses);
      ATLASSERT(!max_applied_stresses.empty());

      // only want stresses in end zones
      Float64 rgt_end, lft_end;
      m_StrandDesignTool.GetMidZoneBoundaries(&lft_end, &rgt_end);

      // we'll pick strand force at location just past transfer length
      Float64 xfer_length = m_StrandDesignTool.GetTransferLength(pgsTypes::Permanent);

      // Build stress demand
      std::vector<pgsStrandDesignTool::StressDemand> stress_demands;
      stress_demands.reserve(max_applied_stresses.size());

      LOG(_T("--- Compute hauling stresses for debonding --- nss = ")<<nss);
      GET_IFACE(IPrestressStresses, pPrestress);
      pgsHaulingAnalysisArtifact::MaxHaulingStressIterator it( max_applied_stresses.begin() );
      pgsHaulingAnalysisArtifact::MaxHaulingStressIterator itEnd( max_applied_stresses.end() );
      for ( ; it != itEnd; it++)
      {
         CHECK_PROGRESS;

         const pgsHaulingAnalysisArtifact::MaxdHaulingStresses& max_stresses = *it;

         Float64 poi_loc = max_stresses.m_HaulingPoi.GetDistFromStart();
         if(poi_loc <= lft_end || rgt_end <= poi_loc)
         {
            // get strand force if we haven't yet
            if (xfer_length <= poi_loc && force_per_strand == 0.0)
            {
               force_per_strand = max_stresses.m_PrestressForce / nts;
               LOG(_T("Sample prestress force per strand taken at ")<< ::ConvertFromSysUnits(poi_loc,unitMeasure::Feet)<<_T(" ft, force = ") << ::ConvertFromSysUnits(force_per_strand, unitMeasure::Kip) << _T(" kip"));
            }

            Float64 fTop = max_stresses.m_TopMaxStress;
            Float64 fBot = max_stresses.m_BottomMinStress;

            LOG(_T("At ")<< ::ConvertFromSysUnits(poi_loc,unitMeasure::Feet)<<_T(" ft, Ftop = ")<< ::ConvertFromSysUnits(fTop,unitMeasure::KSI) << _T(" ksi Fbot = ")<< ::ConvertFromSysUnits(fBot,unitMeasure::KSI) << _T(" ksi") );
            LOG(_T("Average force per strand = ") << ::ConvertFromSysUnits(max_stresses.m_PrestressForce / nss,unitMeasure::Kip) << _T(" kip"));

            pgsStrandDesignTool::StressDemand demand;
            demand.m_Poi  = max_stresses.m_HaulingPoi;
            demand.m_TopStress = fTop;
            demand.m_BottomStress = fBot;

            stress_demands.push_back(demand);
         }
      }

      // compute debond levels at each section from demand
      shipping_debond_levels = m_StrandDesignTool.ComputeDebondsForDemand(stress_demands, nss, force_per_strand, all_tens, all_comp);

      if (  shipping_debond_levels.empty() )
      {
         ATLASSERT(0);
         LOG(_T("Debonding failed, this should not happen?"));

         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::DebondDesignFailed);
         m_DesignerOutcome.AbortDesign();
      }

   }

   return shipping_debond_levels;
}


void pgsDesigner2::RefineDesignForAllowableStress(IProgress* pProgress)
{
   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

   ATLASSERT(!m_DesignerOutcome.DidConcreteChange()); // if this flag is set going in, we will get false positive

   GET_IFACE(IStrandGeometry,pStrandGeom);
   StrandIndexType NtMax = pStrandGeom->GetMaxStrands(span,gdr,pgsTypes::Temporary);

   bool restart = false;
   Int16 i = 0;

   // Our only option is to increase concrete strength, so let loop finish unless we fail.
   while ( i < g_cAllowStressTasks )
   {
      pgsDesigner2::ALLOWSTRESSCHECKTASK task = g_AllowStressTask[i++];

     // skip the temporary strand removal check if the girder can't or doesn't have temporary strands
      if ( (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims && task.ls == pgsTypes::FatigueI) || 
           (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion()&& task.ls == pgsTypes::ServiceIA)
           )
      {
         // if before LRFD 2009 and Fatigue I 
         // - OR -
         // LRFD 2009 and later and Service IA
         //
         // ... don't evaluate this case
         continue;
      }

      if ( task.stage == pgsTypes::TemporaryStrandRemoval && (0 == NtMax || 0 == m_StrandDesignTool.GetNt()) )
         continue; // skip temporary strand removal if this girder doesn't support temporary strands

      LOG(_T(""));
      LOG(_T("*** Refining design for ") << g_Stage[task.stage] << _T(" ") << g_LimitState[task.ls] << _T(" ") << g_Type[task.type] );

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

void pgsDesigner2::RefineDesignForAllowableStress(ALLOWSTRESSCHECKTASK task,IProgress* pProgress)
{
   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

   Float64 fcgdr;
   const GDRCONFIG& config = m_StrandDesignTool.GetGirderConfiguration();
   if ( task.stage == pgsTypes::CastingYard )
   {
      fcgdr = config.Fci;
   }
   else
   {
      fcgdr = config.Fc;
   }

   Float64 startSlabOffset = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
   Float64 endSlabOffset   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IAllowableConcreteStress,pAllowable);
   GET_IFACE(ILimitStateForces,pLimitStateForces);
   GET_IFACE(IPrestressStresses,pPsStress);

   LOG(_T(""));
   LOG(_T("Begin Design Refinement Iterations"));
   m_StrandDesignTool.DumpDesignParameters();

   Float64 start_end_size = (task.stage==pgsTypes::CastingYard)? 0.0 : pBridge->GetGirderStartConnectionLength(span,gdr);

   //
   // Get the allowable stresses
   //
   Float64 fAllow;
   fAllow = pAllowable->GetAllowableStress(task.stage,task.ls,task.type,fcgdr);
   LOG(_T("Allowable stress = ") << ::ConvertFromSysUnits(fAllow,unitMeasure::KSI) << _T(" KSI"));

   bool adj_strength = false; // true if we need to increase strength
   Float64 fControl = task.type==pgsTypes::Tension ? -Float64_Max :  Float64_Max;  // controlling stress for all pois
   pgsTypes::StressLocation stress_location;

   BridgeAnalysisType batTop, batBottom;
   GetBridgeAnalysisType(gdr,task,batTop,batBottom);

   //std::vector<pgsPointOfInterest> vPoi = m_StrandDesignTool.GetDesignPoi(task.stage,POI_FLEXURESTRESS);
   // don't check stresses at all points - it takes too long
   // check it at mid-span, point of prestress transfer (multiple possible control points for debonded girders)
   // points where concentrated loads are applied, and harp points
   std::vector<pgsPointOfInterest> vPoi;
   if (m_StrandDesignTool.GetFlexuralDesignType() == dtDesignForHarping)
   {
      vPoi = m_StrandDesignTool.GetDesignPoi(task.stage,POI_MIDSPAN | POI_PSXFER | POI_CONCLOAD | POI_HARPINGPOINT);
   }
   else
   {
      // debonding needs more points
      vPoi = m_StrandDesignTool.GetDesignPoi(task.stage,POI_FLEXURESTRESS | POI_H);
   }

   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      CHECK_PROGRESS;

      const pgsPointOfInterest& poi = *poiIter;

      LOG(_T("Designing at ") << ::ConvertFromSysUnits(poi.GetDistFromStart() - start_end_size,unitMeasure::Feet) << _T(" ft"));

      //
      // Get the stresses due to externally applied loads
      //
      Float64 fTopMinExt, fTopMaxExt;
      Float64 fBotMinExt, fBotMaxExt;
      pLimitStateForces->GetDesignStress(task.ls,task.stage,poi,pgsTypes::TopGirder,   fcgdr,startSlabOffset,endSlabOffset,batTop,   &fTopMinExt,&fTopMaxExt);
      pLimitStateForces->GetDesignStress(task.ls,task.stage,poi,pgsTypes::BottomGirder,fcgdr,startSlabOffset,endSlabOffset,batBottom,&fBotMinExt,&fBotMaxExt);

      LOG(_T("Max External Stress  :: Top = ") << ::ConvertFromSysUnits(fTopMaxExt,unitMeasure::KSI) << _T(" KSI") << _T("    Bot = ") << ::ConvertFromSysUnits(fBotMaxExt,unitMeasure::KSI) << _T(" KSI"));
      LOG(_T("Min External Stress  :: Top = ") << ::ConvertFromSysUnits(fTopMinExt,unitMeasure::KSI) << _T(" KSI") << _T("    Bot = ") << ::ConvertFromSysUnits(fBotMinExt,unitMeasure::KSI) << _T(" KSI"));

      //
      // Get the stresses due to prestressing (adjust for losses)
      //
      Float64 fTopPre = pPsStress->GetDesignStress(task.stage,poi,pgsTypes::TopGirder,config);
      Float64 fBotPre = pPsStress->GetDesignStress(task.stage,poi,pgsTypes::BottomGirder,config);
      LOG(_T("Prestress Stress     :: Top = ") << ::ConvertFromSysUnits(fTopPre,unitMeasure::KSI) << _T(" KSI") << _T("    Bot = ") << ::ConvertFromSysUnits(fBotPre,unitMeasure::KSI) << _T(" KSI"));

      //
      // Compute the resultant stresses on the section
      //
      GET_IFACE(ILoadFactors,pLF);
      const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();
      Float64 k = pLoadFactors->DCmax[task.ls];

      Float64 fTopMin, fTopMax;
      Float64 fBotMin, fBotMax;

      fTopMin = fTopMinExt + k*fTopPre;
      fTopMax = fTopMaxExt + k*fTopPre;
      fBotMin = fBotMinExt + k*fBotPre;
      fBotMax = fBotMaxExt + k*fBotPre;

      LOG(_T("Max Resultant Stress :: Top = ") << ::ConvertFromSysUnits(fTopMax,unitMeasure::KSI) << _T(" KSI") << _T("    Bot = ") << ::ConvertFromSysUnits(fBotMax,unitMeasure::KSI) << _T(" KSI"));
      LOG(_T("Min Resultant Stress :: Top = ") << ::ConvertFromSysUnits(fTopMin,unitMeasure::KSI) << _T(" KSI") << _T("    Bot = ") << ::ConvertFromSysUnits(fBotMin,unitMeasure::KSI) << _T(" KSI"));

      //
      // Check the resultant stresses on the section
      //
      switch( task.type )
      {
      case pgsTypes::Tension:
         // Only look at tension at the top in the casting yard or at lifting. 
         // Other stages are considered to be after losses, so tension rules only apply out of the precompressed
         // tensile zone (top of girder) 
         if ( fBotMax < fTopMax && (task.stage==pgsTypes::CastingYard || task.stage==pgsTypes::Lifting || 
                                    task.stage==pgsTypes::TemporaryStrandRemoval || task.stage==pgsTypes::BridgeSite1) )
         {
            // tension top controlling
            if ( fAllow < fTopMax && !IsEqual(fAllow,fTopMax) )
            {
               LOG(_T("** Failed in tension at top of girder"));
               fControl = max(fControl, fTopMax);
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
               fControl = max(fControl, fBotMax);
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

               fControl = min(fControl, fBotMin);
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

               fControl = min(fControl, fTopMin);
               stress_location = pgsTypes::TopGirder;
               adj_strength = true;
            }
         }
         break;

      default:
         CHECK(false); // should never get here
      } // end of switch on type

   }  // Next poi

   if ( adj_strength )
   {
      LOG(_T("** Need to increase concrete strength. Controlling stress is ")<<::ConvertFromSysUnits(fControl,unitMeasure::KSI) << _T(" KSI"));

      // Try the next highest concrete strength
      Float64 fc_reqd;
      ConcStrengthResultType success = m_StrandDesignTool.ComputeRequiredConcreteStrength(fControl,task.stage,task.ls,task.type,&fc_reqd);

      if ( ConcFailed==success )
      {
         // could not find a concrete strength that would work
         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::StressExceedsConcreteStrength);
         m_DesignerOutcome.AbortDesign();
         return;
      }

      if( task.stage == pgsTypes::CastingYard )
      {
         if (m_StrandDesignTool.UpdateReleaseStrength(fc_reqd,success, task.stage,task.ls,task.type,stress_location))
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);
      }
      else
      {
         if (m_StrandDesignTool.UpdateConcreteStrength(fc_reqd,task.stage,task.ls,task.type,stress_location))
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
      }

      LOG(_T(""));

   }
}


void pgsDesigner2::RefineDesignForUltimateMoment(pgsTypes::Stage stage,pgsTypes::LimitState ls,IProgress* pProgress)
{
   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

   std::vector<pgsPointOfInterest> vPoi( m_StrandDesignTool.GetDesignPoi(stage,POI_FLEXURECAPACITY) );
   
   GET_IFACE(IBridge,pBridge);
   Float64 start_end_size = (stage==pgsTypes::CastingYard)? 0.0 : pBridge->GetGirderStartConnectionLength(span,gdr);

   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      CHECK_PROGRESS;

      const pgsPointOfInterest& poi = *poiIter;

      LOG(_T(""));
      LOG(_T("======================================================================================================="));
      LOG(_T("Designing at ") << ::ConvertFromSysUnits(poi.GetDistFromStart() - start_end_size,unitMeasure::Feet) << _T(" ft"));

      m_StrandDesignTool.DumpDesignParameters();

      const GDRCONFIG& config = m_StrandDesignTool.GetGirderConfiguration();

      pgsFlexuralCapacityArtifact cap_artifact = CreateFlexuralCapacityArtifact(poi,stage,ls,config,true); // positive moment

      LOG(_T("Capacity (pMn) = ") << ::ConvertFromSysUnits(cap_artifact.GetCapacity(),unitMeasure::KipFeet) << _T(" k-ft") << _T("   Demand (Mu) = ") << ::ConvertFromSysUnits(cap_artifact.GetDemand(),unitMeasure::KipFeet) << _T(" k-ft"));
      LOG(_T("Max Reinf Ratio (c/de) = ") << cap_artifact.GetMaxReinforcementRatio() << _T("   Max Reinf Ratio Limit = ") << cap_artifact.GetMaxReinforcementRatioLimit());
      LOG(_T("Capacity (pMn) = ") << ::ConvertFromSysUnits(cap_artifact.GetCapacity(),unitMeasure::KipFeet) << _T(" k-ft") << _T("   Min Capacity (pMn Min: Lessor of 1.2Mcr and 1.33Mu) = ") << ::ConvertFromSysUnits(cap_artifact.GetMinCapacity(),unitMeasure::KipFeet) << _T(" k-ft"));

#if defined ENABLE_LOGGING
      GET_IFACE(IMomentCapacity, pMomentCapacity);

      MOMENTCAPACITYDETAILS mcd;
      pMomentCapacity->GetMomentCapacityDetails( stage, poi, config, true, &mcd );

      LOG(_T("fpe = ") << ::ConvertFromSysUnits( mcd.fpe, unitMeasure::KSI) << _T(" KSI") );
      LOG(_T("fps = ") << ::ConvertFromSysUnits( mcd.fps, unitMeasure::KSI) << _T(" KSI") );
      LOG(_T("e initial = ") << mcd.e_initial );
      LOG(_T("phi = ") << mcd.Phi );
      LOG(_T("C = ") << ::ConvertFromSysUnits( mcd.C, unitMeasure::Kip) << _T(" kip"));
      LOG(_T("dc = ") << ::ConvertFromSysUnits( mcd.dc, unitMeasure::Inch) << _T(" inch"));
      LOG(_T("de = ") << ::ConvertFromSysUnits( mcd.de, unitMeasure::Inch) << _T(" inch"));
      LOG(_T("dt = ") << ::ConvertFromSysUnits( mcd.dt, unitMeasure::Inch) << _T(" inch"));
      LOG(_T("Moment Arm = ") << ::ConvertFromSysUnits( mcd.MomentArm, unitMeasure::Inch) << _T(" inch"));

      GET_IFACE(ILosses,pILosses);
      Float64 check_loss = pILosses->GetFinal(poi,pgsTypes::Permanent,config);
      LOG(_T("Losses = ") << ::ConvertFromSysUnits( check_loss, unitMeasure::KSI) << _T(" KSI") );

      CRACKINGMOMENTDETAILS cmd;
      pMomentCapacity->GetCrackingMomentDetails(stage, poi, config, true, &cmd);
      LOG(_T("Mcr = ") << ::ConvertFromSysUnits(cmd.Mcr,unitMeasure::KipFeet) << _T(" k-ft"));
      LOG(_T("Mdnc = ")<< ::ConvertFromSysUnits(cmd.Mdnc,unitMeasure::KipFeet) << _T(" k-ft"));
      LOG(_T("fcpe = ") << ::ConvertFromSysUnits( cmd.fcpe, unitMeasure::KSI) << _T(" KSI") );
      LOG(_T("fr = ") << ::ConvertFromSysUnits( cmd.fr, unitMeasure::KSI) << _T(" KSI") );
      LOG(_T("Sb = ") << ::ConvertFromSysUnits( cmd.Sb, unitMeasure::Inch3) << _T(" in^3"));
      LOG(_T("Sbc = ") << ::ConvertFromSysUnits( cmd.Sbc, unitMeasure::Inch3) << _T(" in^3"));
      LOG(_T("Mcr Limit = ") << ::ConvertFromSysUnits(cmd.McrLimit,unitMeasure::KipFeet) << _T(" k-ft"));

#endif // ENABLE_LOGGING

      if ( !cap_artifact.Passed() )
      {
         // Check Ultimate Capacity
         Float64 capacity = cap_artifact.GetCapacity();
         Float64 demand  = cap_artifact.GetDemand();
         if ( capacity < demand )
         {
            LOG(_T("** Ultimate Flexural Capacity Artifact failed at ")<< ::ConvertFromSysUnits(poi.GetDistFromStart() , unitMeasure::Feet) << _T(" ft. Attempt to add strands"));
            StrandIndexType curr_strands = m_StrandDesignTool.GetNumPermanentStrands();
            StrandIndexType max_strands = m_StrandDesignTool.GetMaxPermanentStrands();

            bool success=false;
            if (curr_strands >= max_strands)
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
               new_num = min(new_num, max_new_num);

               new_num = m_StrandDesignTool.GetNextNumPermanentStrands(new_num);

               // Make sure we actually add some strands
               StrandIndexType min_new_num = m_StrandDesignTool.GetNextNumPermanentStrands(curr_strands);
               new_num = max(new_num, min_new_num);

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
               // Deck concrete parameters, Ec will have to parameratized as well as
               // composite section properties.
               //////////////////////////////////
               bool success = m_StrandDesignTool.Bump500(stage, ls, pgsTypes::Tension, pgsTypes::BottomGirder);
               if (success)
               {
                  LOG(_T("Just threw a Hail Mary - Restart design with much higher concrete strength"));
                  m_DesignerOutcome.SetOutcome(pgsDesignCodes::ChangedForUltimate);
                  m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);
                  m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
                  break;
               }
               else
               {
                  LOG(_T("Concrete Strength Cannot be adjusted"));
                  m_StrandDesignTool.SetOutcome(pgsDesignArtifact::UltimateMomentCapacity);
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
               const GDRCONFIG& new_config = m_StrandDesignTool.GetGirderConfiguration();
               pgsFlexuralCapacityArtifact new_cap_artifact = CreateFlexuralCapacityArtifact(poi,stage,ls,new_config,true); // positive moment
               Float64 new_capacity = new_cap_artifact.GetCapacity();
               LOG(_T("New Capacity = ") << ::ConvertFromSysUnits(new_capacity,unitMeasure::KipFeet) << _T(" k-ft"));

               if (new_capacity < capacity)
               {
                  LOG(_T("We added strands and the capacity did not increase - reduce strands back to original and try bumping concrete strength"));
                  success = m_StrandDesignTool.SetNumPermanentStrands(curr_strands);

                  bool success = m_StrandDesignTool.Bump500(stage, ls, pgsTypes::Tension, pgsTypes::BottomGirder);
                  if (success)
                  {
                     m_DesignerOutcome.SetOutcome(pgsDesignCodes::ChangedForUltimate);
                     m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
                     return;
                  }
                  else
                  {
                     LOG(_T("Attempt to bump concrete strength failed - we're probably toast at this point, but keep trying to add strands"));
                     m_StrandDesignTool.SetOutcome(pgsDesignArtifact::UltimateMomentCapacity);
                     m_DesignerOutcome.AbortDesign();
                  }
               }


               poiIter = vPoi.begin();
               continue;
            }
         }

         // Check Maximum Reinforcement
         if ( cap_artifact.GetMaxReinforcementRatio() > cap_artifact.GetMaxReinforcementRatioLimit() )
         {
            // No adjustment to be made. Use a bigger section
            LOG(_T("Capacity Artifact failed for max reinforcement ratio - section overreinforced ")<< ::ConvertFromSysUnits(poi.GetDistFromStart() , unitMeasure::Feet) << _T(" ft"));
            LOG(_T("All we can do here is attempt to bump concrete strength by 500psi"));
            bool bSuccess = m_StrandDesignTool.Bump500(stage, ls, pgsTypes::Tension, pgsTypes::BottomGirder);
            if (bSuccess)
            {
               LOG(_T("Concrete strength was increased for section overreinforced case - Restart") );
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::ChangedForUltimate);
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
               return;
            }
            else
            {
               LOG(_T("Failed to increase concrete strength, cannot remove strands - Failed due to overreinforcement - abort"));
               m_StrandDesignTool.SetOutcome(pgsDesignArtifact::OverReinforced);
               m_DesignerOutcome.AbortDesign();
               return;
            }
         }

         // Check Minimum Reinforcement
         if ( cap_artifact.GetCapacity() < cap_artifact.GetMinCapacity() )
         {
           LOG(_T("Min Reinforcement for Flexural Capacity Artifact failed at ")<< ::ConvertFromSysUnits(poi.GetDistFromStart() , unitMeasure::Feet) << _T(" ft"));

           if ( !m_StrandDesignTool.AddStrands() )
           {
              LOG(_T("Attempt to add strands failed"));
              m_StrandDesignTool.SetOutcome(pgsDesignArtifact::UnderReinforced);
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
void pgsDesigner2::DesignShear(pgsDesignArtifact* pArtifact, bool bDoStartFromScratch, bool bDoDesignFlexure)
{
   SpanIndexType span = pArtifact->GetSpan();
   GirderIndexType gdr  = pArtifact->GetGirder();

   const Float64 one_inch = ::ConvertToSysUnits(1.0, unitMeasure::Inch); // Very US bias here

   // Initialize shear design tool using flexure design pois
   m_ShearDesignTool.ResetDesign( m_StrandDesignTool.GetDesignPoi(pgsTypes::BridgeSite3, POI_ALLACTIONS) );

   // First step here is to perform a shear spec check. We will use the results later for
   // design if needed
   GET_IFACE(IShear,pShear);
   CShearData shear_data = pShear->GetShearData(span, gdr);
   if (bDoStartFromScratch)
   {
      // From-scratch stirrup layout - do initial check with minimal stirrups
      // Minimal stirrups are needed so we don't use equations for Beta for less than min stirrup configuration
      CShearData default_data; // Use defaults from constructor to create no-stirrup condition. Then add minimal stirrups
      shear_data.ShearZones = default_data.ShearZones;
      shear_data.ShearZones.front().VertBarSize = matRebar::bs5;
      shear_data.ShearZones.front().BarSpacing  = 24.0 * one_inch;
      shear_data.ShearZones.front().nVertBars   = 2;
      shear_data.HorizontalInterfaceZones = default_data.HorizontalInterfaceZones;
   }

   pArtifact->SetShearData(shear_data);

   // Get data needed for check
   GDRCONFIG config = pArtifact->GetGirderConfiguration();

   std::vector<pgsPointOfInterest> shear_pois = m_ShearDesignTool.GetDesignPoi();

   // Use check artifact in design tool
   pgsStirrupCheckArtifact* pstirrup_check_artif = m_ShearDesignTool.GetStirrupCheckArtifact();

   // Do the Check
   CheckShear(span, gdr, shear_pois, pgsTypes::StrengthI, &config, pstirrup_check_artif);

   GET_IFACE(ILiveLoads,pLiveLoads);
   if (pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit))
   {
      CheckShear(span, gdr, shear_pois, pgsTypes::StrengthII, &config, pstirrup_check_artif);
   }

   if (!bDoStartFromScratch && pstirrup_check_artif->Passed())
   {
      // Performed spec check on existing input stirrup layout and it passed. 
      // No need to do new design
      pArtifact->SetNumberOfStirrupZonesDesigned( shear_data.ShearZones.size() );
      pArtifact->SetShearData(shear_data);
      pArtifact->AddDesignNote(pgsDesignArtifact::dnExistingShearDesignPassedSpecCheck);
   }
   else
   {
      // We are designing...
      pgsShearDesignTool::ShearDesignOutcome sdo = m_ShearDesignTool.DesignStirrups(m_LeftCS.GetDistFromStart(), m_RightCS.GetDistFromStart());
      if (sdo == pgsShearDesignTool::sdRestartWithAdditionalLongRebar)
      {
         // Additional rebar is needed for long reinf for shear. Add #5 bars, if possible
         Float64 av_add = m_ShearDesignTool.GetRequiredAsForLongReinfShear();

         GET_IFACE(IBridgeMaterial,pMaterial);
         matRebar::Grade barGrade;
         matRebar::Type barType;
         pMaterial->GetTransverseRebarMaterial(span, gdr,barType,barGrade);
         lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
         ATLASSERT(pool != NULL);

         const matRebar* pRebar = pool->GetRebar(barType,barGrade,matRebar::bs5);
         Float64 av_onebar = pRebar->GetNominalArea();

         Float64 nbars = av_add/av_onebar;
         nbars = CeilOff(nbars, 2.0); // round up to next two-bar increment

         // Make sure spacing fits in girder
         GET_IFACE(IGirder,pGirder);
         Float64 wFlange = pGirder->GetBottomWidth(pgsPointOfInterest(span, gdr, 0.0));
         wFlange -= 2*one_inch; // some cover
         Float64 dspacing = wFlange/(nbars-1);
         Float64 spacing = FloorOff(dspacing, one_inch); // try for a reasonable spacing
         if (spacing == 0.0)
         {
            spacing = dspacing; // take any old spacing
         }

         // Add row of bars
         CLongitudinalRebarData& rebar_data = pArtifact->GetLongitudinalRebarData();

         CLongitudinalRebarData::RebarRow row;
         row.BarSize = matRebar::bs5;
         row.Cover = 2.0*one_inch;
         row.Face = pgsTypes::GirderBottom;
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
            pArtifact->SetOutcome(pgsDesignArtifact::StrandsReqdForLongReinfShearAndFlexureTurnedOff);
            m_DesignerOutcome.AbortDesign();
         }
         else
         {
            // We can add strands?
            // Find area of current strands, attempt to add required
            Float64 av_add = m_ShearDesignTool.GetRequiredAsForLongReinfShear();

            GET_IFACE(IBridgeMaterial,pMaterial);
            Float64 aone_strand = pMaterial->GetStrand(span, gdr, pgsTypes::Permanent)->GetNominalArea();

            Float64 nstrands = av_add/aone_strand; // Additional strands needed
            nstrands = CeilOff(nstrands, 1.0);

            StrandIndexType numNp = m_StrandDesignTool.GetNumPermanentStrands();
            StrandIndexType minNp = numNp + (StrandIndexType)nstrands - 1;
            StrandIndexType nextNp = m_StrandDesignTool.GetNextNumPermanentStrands(minNp);
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
               pArtifact->SetOutcome(pgsDesignArtifact::TooMuchStrandsForLongReinfShear);
            }
            else
            {
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
               pArtifact->AddDesignNote(pgsDesignArtifact::dnStrandsAddedForLongReinfShear); // give user a note
            }
         }
      }
      else if (sdo != pgsShearDesignTool::sdSuccess)
      {
         ATLASSERT(0);
         m_DesignerOutcome.AbortDesign();
      }
   }
}


Float64 pgsDesigner2::RoundSlabOffset(Float64 offset)
{
   Float64 newoff;
   // Round to nearest 1/4" (5 mm) per WSDOT BDM
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   if ( IS_SI_UNITS(pDisplayUnits) )
      newoff = CeilOff(offset,::ConvertToSysUnits(5.0,unitMeasure::Millimeter) );
   else
      newoff = CeilOff(offset,::ConvertToSysUnits(0.25,unitMeasure::Inch) );

   return newoff;
}


//======================== ACCESS     =======================================
//======================== INQUERY    =======================================

//======================== DEBUG      =======================================
#if defined _DEBUG
bool pgsDesigner2::AssertValid() const
{
   return true;
}

void pgsDesigner2::Dump(dbgDumpContext& os) const
{
   os << _T("Dump for pgsDesigner2") << endl;
}
#endif // _DEBUG

#if defined _UNITTEST
bool pgsDesigner2::TestMe(dbgLog& rlog)
{
   TESTME_PROLOGUE("pgsDesigner2");

   TEST_NOT_IMPLEMENTED("Unit Tests Not Implemented for pgsDesigner2");

   TESTME_EPILOG("Designer");
}
#endif // _UNITTEST

bool pgsDesigner2::CollapseZoneData(CShearZoneData zoneData[MAX_ZONES], Uint32 numZones)
{
   // Return true if last two zones have equivalent stirrups
   if (numZones<2)
      return false;

   if (zoneData[numZones-2].VertBarSize != zoneData[numZones-1].VertBarSize)
      return false;

   if (zoneData[numZones-2].BarSpacing != zoneData[numZones-1].BarSpacing)
      return false;

   // two zones are equivalent - make 1st zone longer
   zoneData[numZones-2].ZoneLength += zoneData[numZones-1].ZoneLength;
   return true;
}

void pgsDesigner2::GetBridgeAnalysisType(GirderIndexType gdr,const ALLOWSTRESSCHECKTASK& task,BridgeAnalysisType& batTop,BridgeAnalysisType& batBottom)
{
   GET_IFACE(ISpecification, pSpec);
   GET_IFACE(IContinuity,pContinuity);

   pgsTypes::AnalysisType analysisType = pgsTypes::Simple;
   if ( pContinuity->IsContinuityFullyEffective(gdr) )
      analysisType = pSpec->GetAnalysisType();

   if ( analysisType == pgsTypes::Simple )
   {
      batTop    = SimpleSpan;
      batBottom = SimpleSpan;
   }
   else if ( analysisType == pgsTypes::Continuous )
   {
      batTop    = ContinuousSpan;
      batBottom = ContinuousSpan;
   }
   else
   {
      if ( task.type == pgsTypes::Compression )
      {
         batTop    = MaxSimpleContinuousEnvelope;
         batBottom = MinSimpleContinuousEnvelope;
      }
      else
      {
         batTop    = MinSimpleContinuousEnvelope;
         batBottom = MaxSimpleContinuousEnvelope;
      }
   }
}
