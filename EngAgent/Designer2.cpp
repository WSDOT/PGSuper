///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
#include <IFace\DisplayUnits.h>
#include <IFace\GirderHandling.h>

#include "Designer2.h"
#include "PsForceEng.h"
#include "GirderHandlingChecker.h"


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
const std::string g_Stage[] =
{
   std::string("CastingYard"),
   std::string("BridgeSite1"),
   std::string("BridgeSite2"),
   std::string("BridgeSite3"),
   std::string("Hauling"),
   std::string("Lifting"),
   std::string("Temporary Strand Removal"),
   std::string("Girder Placement")
};

const std::string g_LimitState[] =
{
   std::string("ServiceI"),
   std::string("ServiceIA"),
   std::string("ServiceIII"),
   std::string("StrengthI"),
   std::string("StrengthII"),
   std::string("FatigueI")
};

const std::string g_Type[] = 
{
   std::string("Tension"),
   std::string("Compression")
};

inline std::string StrTopBot(pgsTypes::StressLocation sl)
{
   if (sl==pgsTypes::BottomGirder)
      return " Bottom of Girder";
   else
      return " Top of Girder";
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

// function to give envelope of two integer vectors (for debond levels)
inline std::vector<Int16> EnvelopIntVec(const std::vector<Int16>& rvec1, const std::vector<Int16>& rvec2)
{
   std::vector<Int16> result;
   std::vector<Int16>::const_iterator r1it = rvec1.begin();
   std::vector<Int16>::const_iterator r2it = rvec2.begin();

   while (r1it!=rvec1.end() || r2it!=rvec2.end())
   {
      if (r1it!=rvec1.end() && r2it!=rvec2.end())
      {
         // both values at location
         Int16 v1 = *r1it;
         Int16 v2 = *r2it;
         Int16 imax = max(v1, v2);

         result.push_back(imax);

         r1it++;
         r2it++;
      }
      else if (r1it!=rvec1.end())
      {
         // only r1
         Int16 v1 = *r1it;
         result.push_back(v1);

         r1it++;
      }
      else if (r2it!=rvec2.end())
      {
         // only r2
         Int16 v2 = *r2it;
         result.push_back(v2);

         r2it++;
      }
      else
         ATLASSERT(0);
   }

   return result;
}

////////////////////////// PUBLIC     ///////////////////////////////////////



//======================== LIFECYCLE  =======================================
pgsDesigner2::pgsDesigner2():
m_StrandDesignTool(LOGGER)
{
   m_bShippingDesignWithEqualCantilevers = false;
   m_bShippingDesignIgnoreConfigurationLimits = false;

   CREATE_LOGFILE("Designer");
}

pgsDesigner2::pgsDesigner2(const pgsDesigner2& rOther):
m_StrandDesignTool(LOGGER)
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

void pgsDesigner2::SetAgentID(long agentID)
{
   m_AgentID = agentID;
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
   GET_IFACE(IStatusCenter,pStatusCenter);

   GET_IFACE(ILibrary, pLib );
   GET_IFACE(ISpecification, pSpec );
   std::string spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   pHaunchDetails->Haunch.clear();

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   Float64 fillet = pDeck->Fillet;

   std::set<pgsTypes::Stage> stages;
   stages.insert(pgsTypes::CastingYard);
   stages.insert(pgsTypes::BridgeSite3);
   std::vector<pgsPointOfInterest> vPoi = pIPOI->GetPointsOfInterest(stages,span,gdr,POI_ALL,POIFIND_OR);

   //
   // Profile Effects and Girder Orientation Effects
   //

   // slope of the girder in the plane of the girder
   double girder_slope = pBridge->GetGirderSlope(span,gdr);
   pgsPointOfInterest& firstPoi = vPoi[0];

   // get station and offset of first poi
   double station,offset;
   pBridge->GetStationAndOffset(firstPoi,&station,&offset);
   offset = IsZero(offset) ? 0 : offset;

   // the girder reference line passes through the deck at this station and offset
   double Y_girder_ref_line_left_bearing = pAlignment->GetElevation(station,offset);

   double end_size = pBridge->GetGirderStartConnectionLength(span,gdr);

   Uint32 nMatingSurfaces = pGdr->GetNumberOfMatingSurfaces(span,gdr);

   double girder_orientation = pGdr->GetOrientation(span,gdr);

   double max_tslab_and_fillet = 0;

   double max_actual_haunch_depth_diff = 0; // maximum difference between "A" and the actual haunch depth at any section

   // determine the minumum and maximum difference in elevation between the
   // roadway surface and the top of the girder.... measured directly above 
   // the top of the girder
   double diff_min =  100000;
   double diff_max = -100000;
   double max_reqd_haunch_depth = -100000;
   double min_reqd_haunch_depth =  100000;
   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      pgsPointOfInterest& poi = *iter;

      double slab_offset = (bUseConfig ? pBridge->GetSlabOffset(poi,config) : pBridge->GetSlabOffset(poi));

      double tSlab = pBridge->GetGrossSlabDepth( poi );

      double camber_effect = -999;
      if ( bUseConfig )
         camber_effect = pCamber->GetExcessCamber(poi, config, CREEP_MAXTIME );
      else
         camber_effect = pCamber->GetExcessCamber(poi, CREEP_MAXTIME );

      double top_width = pGdr->GetTopWidth(poi);

      // top of girder elevation, including camber effects
      double elev_top_girder = (bUseConfig ? pGdr->GetTopGirderElevation(poi,config,INVALID_INDEX) : pGdr->GetTopGirderElevation(poi,INVALID_INDEX) );

      // get station and normal offset for this poi
      double x,z;
      pBridge->GetStationAndOffset(poi,&x,&z);
      z = IsZero(z) ? 0 : z;

      // top of girder elevation (ignoring camber effects)
      double yc = pGdr->GetTopGirderReferenceChordElevation(poi);

      // top of alignment elevation above girder
      double ya = pAlignment->GetElevation(x,z);

      // profile effect
      double section_profile_effect = ya - yc;
      diff_min = _cpp_min(diff_min,section_profile_effect);
      diff_max = _cpp_max(diff_max,section_profile_effect);

      // girder orientation effect
      double crown_slope = 0;
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

         double left_mating_surface_offset  = pGdr->GetMatingSurfaceLocation(poi,0);
         double right_mating_surface_offset = pGdr->GetMatingSurfaceLocation(poi,nMatingSurfaces-1);

         double ya_left  = pAlignment->GetElevation(x,z+left_mating_surface_offset);
         double ya_right = pAlignment->GetElevation(x,z+right_mating_surface_offset);

         crown_slope = (ya_left - ya_right)/(right_mating_surface_offset - left_mating_surface_offset);
      }

      double section_girder_orientation_effect = (top_width/2)*(fabs(crown_slope - girder_orientation)/(sqrt(1+girder_orientation*girder_orientation)));

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
   double profile_effect = 0;
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
   GET_IFACE(IStatusCenter,pStatusCenter);

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
      std::string strMsg("Live load are not defined.");
      pgsLiveLoadStatusItem* pStatusItem = new pgsLiveLoadStatusItem(m_AgentID,122,strMsg.c_str());
      pStatusCenter->Add(pStatusItem);
   }

   CheckMomentCapacity(span,gdr,pgsTypes::BridgeSite3,pgsTypes::StrengthI,&gdr_artifact);
   CheckShear(span,gdr,pgsTypes::BridgeSite3,pgsTypes::StrengthI,&gdr_artifact);

   // check for strength II only if permit vehicle is defined
   if (pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit))
   {
      CheckMomentCapacity(span,gdr,pgsTypes::BridgeSite3,pgsTypes::StrengthII,&gdr_artifact);
      CheckShear(span,gdr,pgsTypes::BridgeSite3,pgsTypes::StrengthII,&gdr_artifact);
   }

   CheckSplittingZone(span,gdr,&gdr_artifact);

   CheckGirderDetailing(span,gdr,&gdr_artifact);

   CheckStrandSlope(span,gdr,gdr_artifact.GetStrandSlopeArtifact());
   CheckHoldDownForce(span,gdr,gdr_artifact.GetHoldDownForceArtifact());

   CheckConstructability(span,gdr,gdr_artifact.GetConstructabilityArtifact());
   CheckLiveLoadDeflection(span,gdr,gdr_artifact.GetDeflectionCheckArtifact());

   CheckDebonding(span,gdr,pgsTypes::Straight,gdr_artifact.GetDebondArtifact(pgsTypes::Straight));
   CheckDebonding(span,gdr,pgsTypes::Harped,gdr_artifact.GetDebondArtifact(pgsTypes::Harped));
   CheckDebonding(span,gdr,pgsTypes::Temporary,gdr_artifact.GetDebondArtifact(pgsTypes::Temporary));

   pgsGirderHandlingChecker handling_checker(m_pBroker,m_AgentID);

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

#define CHECK_PROGRESS \
if ( pProgress->Continue() != S_OK ) \
{ \
   artifact.SetOutcome(pgsDesignArtifact::DesignCancelled); \
   LOG("*#*#*#*#* DESIGN CANCELLED BY USER *#*#*#*#*"); \
   return artifact; \
}

#define CHECK_PROGRESS_RET \
if ( pProgress->Continue() != S_OK ) \
{ \
   artifact.SetOutcome(pgsDesignArtifact::DesignCancelled); \
   LOG("*#*#*#*#* DESIGN CANCELLED BY USER *#*#*#*#*"); \
   m_DesignerOutcome.AbortDesign(); \
   return; \
}


pgsDesignArtifact pgsDesigner2::Design(SpanIndexType span,GirderIndexType gdr,arDesignOptions options)
{
   LOG("************************************************************");
   LOG("Beginning design for span " << LABEL_SPAN(span) << " girder " << LABEL_GIRDER(gdr));
#if defined ENABLE_LOGGING
   sysTime startTime;
#endif

   GET_IFACE(ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   GET_IFACE(IProgress,pProgress);
   pgsAutoProgress ap(pProgress);

   std::ostringstream os;
   os << "Designing Span " << LABEL_SPAN(span) << " Girder " << LABEL_GIRDER(gdr) << std::ends;
   pProgress->UpdateMessage(os.str().c_str());

   // Initialize the design artifact
   pgsDesignArtifact artifact(span,gdr);
   artifact.SetDesignOptions(options);

   // if not designing for lifting, set the lift values in the artifact to the
   // current values
   if ( !options.doDesignLifting )
   {
      GET_IFACE(IGirderLifting,pGirderLifting);
      double Loh = pGirderLifting->GetLeftLiftingLoopLocation(span,gdr);
      double Roh = pGirderLifting->GetRightLiftingLoopLocation(span,gdr);
      artifact.SetLiftingLocations(Loh,Roh);
   }

   if ( !options.doDesignHauling )
   {
      GET_IFACE(IGirderHauling,pGirderHauling);
      double Loh = pGirderHauling->GetTrailingOverhang(span,gdr);
      double Roh = pGirderHauling->GetLeadingOverhang(span,gdr);
      artifact.SetTruckSupportLocations(Loh,Roh);
   }

   // use strand design tool to control proportioning of strands
   m_StrandDesignTool.Initialize(m_pBroker, m_AgentID, &artifact);

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
      LOG("");
      LOG("Iteration Number # " << cIter );

      pProgress->UpdateMessage(os.str().c_str());

      if (options.doDesignForFlexure!=dtNoDesign)
      {
         // reset outcomes
         bool keep_prop = false;
         if (m_DesignerOutcome.DidConcreteChange())
         {
            LOG("Concrete changed on last iteration. Reset min slab offset to zero");
            m_StrandDesignTool.SetMinimumSlabOffset(0.0);
         }

         keep_prop = m_DesignerOutcome.DidRetainStrandProportioning();
         m_DesignerOutcome.Reset();
         if (keep_prop)
         {
            LOG("Retaining strand proportioning from last iteration");
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::RetainStrandProportioning);
         }

         // get design back on track with user preferences
         m_StrandDesignTool.RestoreDefaults(m_DesignerOutcome.DidRetainStrandProportioning());

         // Design strands and concrete strengths in mid-zone
         DesignMidZone(cIter == 0 ? false : true, options,pProgress);

         if (m_DesignerOutcome.WasDesignAborted())
            return artifact;

         CHECK_PROGRESS;

         LOG("");
         LOG("BEGINNING DESIGN OF END-ZONES");
         LOG("");

         m_StrandDesignTool.DumpDesignParameters();

         // Design end zones
         DesignEndZone(cIter==0, options, artifact, pProgress);

         if ( m_DesignerOutcome.WasDesignAborted() )
         {
            return artifact;
         }
         else if ( m_DesignerOutcome.DidConcreteChange() )
         {
            LOG("End Zone Design changed concrete strength - Restart");
            LOG("==================================================");
            continue;
         }

         LOG("");
         LOG("BEGINNING DESIGN REFINEMENT");
         LOG("");

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
         LOG("Refining the design for ultimate moment capacity");

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

         LOG("Strength I Limit State");
         RefineDesignForUltimateMoment(pgsTypes::BridgeSite3, pgsTypes::StrengthI,pProgress);

         CHECK_PROGRESS;

         if ( m_DesignerOutcome.WasDesignAborted() )
         {
            return artifact;
         }
         else if  (  m_DesignerOutcome.GetOutcome(pgsDesignCodes::ChangedForUltimate) )
         {
            LOG("Ultimate moment controlled - restart design");
            LOG("===========================================");
            continue;
         }
         
         if ( bPermit )
         {
            LOG("Strength II Limit State");
            RefineDesignForUltimateMoment(pgsTypes::BridgeSite3, pgsTypes::StrengthII,pProgress);

            CHECK_PROGRESS;
   
            if ( m_DesignerOutcome.WasDesignAborted() )
            {
               return artifact;
            }
            else if  (  m_DesignerOutcome.GetOutcome(pgsDesignCodes::ChangedForUltimate) )
            {
               LOG("Ultimate moment controlled - restart design");
               LOG("===========================================");
               continue;
            }
         }

         if (options.doDesignSlabOffset)
         {
            LOG("Starting Slab Offset design in outer loop");
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

               LOG("Slab Offset changed in outer loop. Set a new minimum of (Start) " << ::ConvertFromSysUnits(new_offset_start,unitMeasure::Inch)<< "in and (End) " << ::ConvertFromSysUnits(new_offset_end,unitMeasure::Inch) << " - restart design");
               LOG("=========================================================================");
               m_StrandDesignTool.SetMinimumSlabOffset( min(new_offset_start,new_offset_end));
               m_StrandDesignTool.SetSlabOffset(pgsTypes::metStart,new_offset_start);
               m_StrandDesignTool.SetSlabOffset(pgsTypes::metEnd, new_offset_end);
               continue;
            }
            else
            {
               m_StrandDesignTool.SetSlabOffset(pgsTypes::metStart,old_offset_start);  // restore to original value that passed all spec checks
               m_StrandDesignTool.SetSlabOffset(pgsTypes::metEnd,  old_offset_end);   // restore to original value that passed all spec checks
               LOG("Slab Offset design Successful in outer loop. Current value is (Start) " <<::ConvertFromSysUnits( m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart),unitMeasure::Inch)<<"in and (End) "<<::ConvertFromSysUnits( m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd),unitMeasure::Inch) << " in");
               LOG("===========================================");
            }
         }
         else
         {
            LOG("Skipping Outer Slab Offset Design due to user input");
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
         LOG("A slight adjustment was made during flexural design. Clear settings for shear design (if applicable)");
         m_DesignerOutcome.Reset();
      }

      if (options.doDesignForShear)
      {
         //
         // Refine stirrup design
         // 

         LOG("Starting Stirrup Design");
         LOG("Strength I Limit State");
         RefineDesignForStirrups(pgsTypes::BridgeSite3,
                                 pgsTypes::StrengthI,
                                 &artifact);

         if ( m_DesignerOutcome.WasDesignAborted() )
            return artifact;
         else if  (  m_DesignerOutcome.DidGirderChange())
            continue;

         if ( bPermit )
         {
            LOG("Strength II Limit State");
            RefineDesignForStirrups(pgsTypes::BridgeSite3,
                                    pgsTypes::StrengthII,
                                    &artifact);

            if ( m_DesignerOutcome.WasDesignAborted() )
               return artifact;
            else if  (  m_DesignerOutcome.DidGirderChange())
               continue;
         }
      }

      // we've succussfully completed all the design steps
      // we are DONE!
      bDone = true;
   } while ( cIter < nIterMax && !bDone );

   if ( !bDone ) //&& cIter >= nIterMax )
   {
      LOG("Maximum number of iteratations was exceeded - aborting design " << cIter);
      artifact.SetOutcome(pgsDesignArtifact::MaxIterExceeded);
      return artifact;
   }

   if (artifact.GetDesignOptions().doDesignSlabOffset)
   {
      LOG("Final Slab Offset before rounding (Start) " << ::ConvertFromSysUnits( artifact.GetSlabOffset(pgsTypes::metStart),unitMeasure::Inch) << " in and (End) " << ::ConvertFromSysUnits( artifact.GetSlabOffset(pgsTypes::metEnd),unitMeasure::Inch) << " in");
      Float64 start_offset = RoundSlabOffset(artifact.GetSlabOffset(pgsTypes::metStart));
      Float64 end_offset   = RoundSlabOffset(artifact.GetSlabOffset(pgsTypes::metEnd));
      artifact.SetSlabOffset(pgsTypes::metStart,start_offset);
      artifact.SetSlabOffset(pgsTypes::metEnd, end_offset);
      LOG("After rounding (Start) " << ::ConvertFromSysUnits(start_offset,unitMeasure::Inch) << " in and (End) " << ::ConvertFromSysUnits(end_offset,unitMeasure::Inch) << " in");
   }


   m_StrandDesignTool.DumpDesignParameters();

   LOG("Design Complete for span " << LABEL_SPAN(span) << " girder " << LABEL_GIRDER(gdr));
   LOG("************************************************************");
#if defined ENABLE_LOGGING
   sysTime endTime;
   long duration = endTime.Seconds() - startTime.Seconds();
   long min = duration / 60;
   long sec = duration - min*60;
   LOG("Design: " << min << "m:" << sec << "s");
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
   m_AgentID = rOther.m_AgentID;
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
   pgsAutoProgress ap(pProgress);

   std::ostringstream os;
   os << "Checking strand stresses for Span "
      << LABEL_SPAN(span) << " Girder "
      << LABEL_GIRDER(gdr) << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );
   
   std::vector<pgsPointOfInterest> vPOI;
   pgsPointOfInterest poi;

   vPOI = pIPOI->GetPointsOfInterest(pgsTypes::CastingYard,span,gdr,POI_MIDSPAN);
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
   long Nt = pStrandGeom->GetNumStrands(poi.GetSpan(),poi.GetGirder(),pgsTypes::Temporary);
   if ( 0 < Nt )
   {
      strandTypes.push_back(pgsTypes::Temporary);
   }

   std::vector<pgsTypes::StrandType>::iterator iter;
   for ( iter = strandTypes.begin(); iter != strandTypes.end(); iter++ )
   {
      pgsTypes::StrandType strandType = *iter;

      if ( pAllow->CheckStressAtJacking() )
         pArtifact->SetCheckAtJacking( strandType, pPsForce->GetStrandStress(poi,strandType,pgsTypes::Jacking), pAllow->GetAllowableAtJacking(span,gdr) );

      if ( pAllow->CheckStressBeforeXfer() )
         pArtifact->SetCheckBeforeXfer( strandType, pPsForce->GetStrandStress(poi,strandType,pgsTypes::BeforeXfer), pAllow->GetAllowableBeforeXfer(span,gdr) );

      if ( pAllow->CheckStressAfterXfer() )
         pArtifact->SetCheckAfterXfer( strandType, pPsForce->GetStrandStress(poi,strandType,pgsTypes::AfterXfer), pAllow->GetAllowableAfterXfer(span,gdr) );

      //vPOI = pIPOI->GetPointsOfInterest(pgsTypes::BridgeSite1,span,gdr,POI_MIDSPAN);
      //poi = *vPOI.begin();

      if ( pAllow->CheckStressAfterLosses() && strandType != pgsTypes::Temporary )
         pArtifact->SetCheckAfterLosses( strandType, pPsForce->GetStrandStress(poi,strandType,pgsTypes::AfterLosses), pAllow->GetAllowableAfterLosses(span,gdr) );
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

   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(task.stage,span,gdr,POI_FLEXURESTRESS | POI_TABULAR);
   std::vector<pgsPointOfInterest>::iterator iter;

   GET_IFACE(IProjectSettings,pProjectSettings);
   bool bUnitsSI = (pProjectSettings->GetUnitsMode() == pgsTypes::umSI ? true : false);

   double Es, fy;
   pMaterial->GetLongitudinalRebarProperties(span,gdr,&Es,&fy);
   Float64 fs = 0.5*fy;
   Float64 fsMax = (bUnitsSI ? ::ConvertToSysUnits(206.0,unitMeasure::MPa) : ::ConvertToSysUnits(30.0,unitMeasure::KSI) );
   if ( fsMax < fs )
       fs = fsMax;

   pgsTypes::AnalysisType analysisType = pgsTypes::Simple;
   if ( pContinuity->IsContinuityFullyEffective(gdr) )
      analysisType = pSpec->GetAnalysisType();

   Float64 AsMax = 0;

   BridgeAnalysisType bat;
   if ( analysisType == pgsTypes::Simple )
      bat = SimpleSpan;
   else if ( analysisType == pgsTypes::Continuous )
      bat = ContinuousSpan;
   else
      bat = MaxSimpleContinuousEnvelope;

   GET_IFACE(IStageMap,pStageMap);
   GET_IFACE(IProgress, pProgress);
   GET_IFACE(IDisplayUnits,pDispUnits);
   const unitmgtLengthData& length = pDispUnits->GetSpanLengthUnit();
   pgsAutoProgress ap(pProgress);

   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++)
   {
      const pgsPointOfInterest& poi = *iter;

      std::string strStage = OLE2A(pStageMap->GetStageName(task.stage));
      std::string strType  = (task.type == pgsTypes::Tension ? "Tension" : "Compression");
      std::string strLimitState = OLE2A(pStageMap->GetLimitStateName(task.ls));
      std::ostringstream os;
      os << "Computing " << strStage << " " << strLimitState << " " << strType << " stresses for Span "
      << LABEL_SPAN(poi.GetSpan()) << " Girder "
      << LABEL_GIRDER(poi.GetGirder()) << " at " 
      << std::setw(length.Width)
      << std::setprecision(length.Precision) 
      << ::ConvertFromSysUnits(poi.GetDistFromStart(),length.UnitOfMeasure) << " " 
      << length.UnitOfMeasure.UnitTag() << " from start of girder" << std::ends;
      pProgress->UpdateMessage( os.str().c_str() );
      
      pgsFlexuralStressArtifactKey key(task.stage,task.ls,task.type,poi.GetDistFromStart());
      pgsFlexuralStressArtifact artifact;

      // get girder stress due to prestressing
      Float64 fTopPrestress, fBotPrestress;
      fTopPrestress = pPrestressStresses->GetStress(task.stage,poi,pgsTypes::TopGirder);
      fBotPrestress = pPrestressStresses->GetStress(task.stage,poi,pgsTypes::BottomGirder);

      // get girder stress due to external loads (top)
      Float64 fTopLimitStateMin, fTopLimitStateMax;
      pLimitStateForces->GetStress(task.ls,task.stage,poi,pgsTypes::TopGirder,false,bat,&fTopLimitStateMin,&fTopLimitStateMax);
      Float64 fTopLimitState = (task.type == pgsTypes::Compression ? fTopLimitStateMin : fTopLimitStateMax );

      // get girder stress due to external loads (bottom)
      Float64 fBotLimitStateMin, fBotLimitStateMax;
      pLimitStateForces->GetStress(task.ls,task.stage,poi,pgsTypes::BottomGirder,false,bat,&fBotLimitStateMin,&fBotLimitStateMax);
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
         double c = pAllowable->GetAllowableCompressiveStressCoefficient(task.stage,task.ls);
         double fc_reqd = (IsZero(c) ? 0 : _cpp_min(fTop,fBot)/-c);
         
         if ( fc_reqd < 0 ) // the minimum stress is tensile so compression isn't an issue
            fc_reqd = 0;

         artifact.SetRequiredConcreteStrength(fc_reqd);
      }
      else
      {
         double t;
         bool bCheckMax;
         double fmax;

         pAllowable->GetAllowableTensionStressCoefficient(task.stage,task.ls,&t,&bCheckMax,&fmax);

         // if this is bridge site 3, only look at the bottom stress (stress in the precompressed tensile zone)
         // otherwise, take the controlling tension
         double f = (task.stage == pgsTypes::BridgeSite3 ? fBot : _cpp_max(fTop,fBot));

         double fc_reqd;
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
               double talt = pAllowable->GetCastingYardAllowableTensionStressCoefficientWithRebar();
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
            double Y;
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

            CComPtr<IShapeProperties> props;
            clipped_shape->get_ShapeProperties(&props);

            props->get_Area(&Area);

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

   Float64 MuMin, MuMax;
   if ( analysisType == pgsTypes::Envelope )
   {
      double min,max;
      pLimitStateForces->GetMoment(ls,stage,poi,MaxSimpleContinuousEnvelope,&min,&max);
      MuMax = max;

      pLimitStateForces->GetMoment(ls,stage,poi,MinSimpleContinuousEnvelope,&min,&max);
      MuMin = min;
   }
   else
   {
      pLimitStateForces->GetMoment(ls,stage,poi,analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan,&MuMin,&MuMax);
   }

   artifact.SetCapacity( mcd.Phi * mcd.Mn );
   artifact.SetDemand( bPositiveMoment ? MuMax : MuMin );
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

pgsStirrupCheckAtPoisArtifact pgsDesigner2::CreateStirrupCheckAtPoisArtifact(const pgsPointOfInterest& poi,pgsTypes::Stage stage,pgsTypes::LimitState ls, const sysSectionValue& vu,
                                                                            Float64 fcSlab,Float64 fcGdr, Float64 fy)
{
   CHECK(stage==pgsTypes::BridgeSite3);
   CHECK(ls==pgsTypes::StrengthI || ls == pgsTypes::StrengthII);

   GET_IFACE(IStatusCenter,pStatusCenter);
   GET_IFACE(IBridge,pBridge);

   // throw an exception if span length is too short
   if (IsDeepSection( poi ))
   {
      Int32 reason = XREASON_AGENTVALIDATIONFAILURE;
      std::ostringstream os;
      os << "Cannot perform shear check - Span-to-Depth ratio is less than "<< MIN_SPAN_DEPTH_RATIO <<" for Span "<< poi.GetSpan()+1 << " Girder "<< LABEL_GIRDER(poi.GetGirder());

      pgsBridgeDescriptionStatusItem* pStatusItem = new pgsBridgeDescriptionStatusItem(m_AgentID,109,0,os.str().c_str());
      pStatusCenter->Add(pStatusItem);

      os << std::endl << "See Status Center for Details";
      THROW_UNWIND(os.str().c_str(),reason);
   }

   GET_IFACE(IShearCapacity, pShearCapacity);

   SHEARCAPACITYDETAILS scd;
   pShearCapacity->GetShearCapacityDetails( ls, stage, poi, &scd );

   // vertical shear
   pgsVerticalShearArtifact v_artifact;
   CheckStirrupRequirement( poi, scd, &v_artifact );
   CheckUltimateShearCapacity( poi, scd, vu, &v_artifact );

   // horizontal shear
   pgsHorizontalShearArtifact h_artifact;
   if ( pBridge->IsCompositeDeck() )
      CheckHorizontalShear(poi,vu,fcSlab,fcGdr,fy,&h_artifact);

   // stirrup detail check
   pgsStirrupDetailArtifact d_artifact;
   CheckFullStirrupDetailing(poi,v_artifact,scd,vu,fcGdr,fy,&d_artifact);

   // longitudinal steel check
   pgsLongReinfShearArtifact l_artifact;
   CheckLongReinfShear(poi,stage,ls,scd,&l_artifact);
   
   // create the artifact and return it
   pgsStirrupCheckAtPoisArtifact artifact;
   artifact.SetVerticalShearArtifact(v_artifact);
   artifact.SetHorizontalShearArtifact(h_artifact);
   artifact.SetStirrupDetailArtifact(d_artifact);
   artifact.SetLongReinfShearArtifact(l_artifact);

   return artifact;
}

pgsStirrupCheckAtZonesArtifact pgsDesigner2::CreateStirrupCheckAtZonesArtifact(SpanIndexType span,GirderIndexType gdr,Uint32 zoneNum, bool checkConfinement)
{
   // create the artifact and return it
   pgsStirrupCheckAtZonesArtifact artifact;

   if (checkConfinement)
   {
      // confinement
      pgsConfinementArtifact c_artifact;
      CheckConfinement(span,gdr,zoneNum,&c_artifact);

      artifact.SetConfinementArtifact(c_artifact);
   }

   return artifact;
}

bool pgsDesigner2::IsDeepSection( const pgsPointOfInterest& poi)
{
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

void pgsDesigner2::CheckUltimateShearCapacity( const pgsPointOfInterest& poi, const SHEARCAPACITYDETAILS& scd, const sysSectionValue& vu, pgsVerticalShearArtifact* pArtifact )
{
   bool bCheck = true;
   if ( m_bSkipShearCheckBeforeLeftCS && poi.GetDistFromStart() < m_LeftCS )
      bCheck = false;

   if ( m_bSkipShearCheckAfterRightCS && m_RightCS < poi.GetDistFromStart() )
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
      bool bSTRequired = (poi.GetDistFromStart() < m_LeftCS ? m_bLeftCS_StrutAndTieRequired : m_bRightCS_StrutAndTieRequired);
      pArtifact->IsStrutAndTieRequired(bSTRequired);

      if ( !bSTRequired )
      {
         GET_IFACE(IStirrupGeometry,pStirrupGeom);

         // the shear reinforcement must be at least as much as at the critical section
         Float64 AvS_provided = scd.Av/scd.S;
         Float64 AvS_at_CS;
         if ( poi.GetDistFromStart() < m_LeftCS )
         {
            pgsPointOfInterest poiCS(poi);
            poiCS.SetDistFromStart(m_LeftCS);
            Uint32 nl  = pStirrupGeom->GetVertStirrupBarCount(poi);
            Float64 Av = pStirrupGeom->GetVertStirrupBarArea(poi)*nl;
            Float64 S  = pStirrupGeom->GetS(poi);
            AvS_at_CS = Av/S;
         }
         else if ( m_RightCS < poi.GetDistFromStart() )
         {
            pgsPointOfInterest poiCS(poi);
            poiCS.SetDistFromStart(m_RightCS);
            Uint32 nl  = pStirrupGeom->GetVertStirrupBarCount(poi);
            Float64 Av = pStirrupGeom->GetVertStirrupBarArea(poi)*nl;
            Float64 S  = pStirrupGeom->GetS(poi);
            AvS_at_CS = Av/S;
         }

         pArtifact->SetEndSpacing(AvS_provided,AvS_at_CS);
      }
   }
}

void pgsDesigner2::CheckHorizontalShear(const pgsPointOfInterest& poi, 
                                       const sysSectionValue& vu, 
                                       Float64 fcSlab,Float64 fcGdr, Float64 fy,
                                       pgsHorizontalShearArtifact* pArtifact )
{
   GET_IFACE(ISectProp2,pSectProp2);
   GET_IFACE(IGirder,pGdr);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeMaterial,pMaterial);
   GET_IFACE(IProductForces,pProductForces);
   GET_IFACE(IStrandGeometry,pStrandGeom);

   // determine shear demand
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   sysSectionValue Vuh;

   if ( pSpecEntry->GetShearFlowMethod() == sfmClassical )
   {
      Float64 Qslab = pSectProp2->GetQSlab(poi);
      ATLASSERT(Qslab!=0);

      Float64 Ic  = pSectProp2->GetIx(pgsTypes::BridgeSite3,poi);
      Vuh.Left()  = vu.Left() *Qslab/Ic;
      Vuh.Right() = vu.Right()*Qslab/Ic;

      pArtifact->SetI( Ic );
      pArtifact->SetQ( Qslab );
   }
   else
   {
      Float64 nEffStrands;
      Float64 ecc = pStrandGeom->GetEccentricity(poi,false,&nEffStrands); // based on non-composite cg
      Float64 Yt = pSectProp2-> GetYtGirder(pgsTypes::BridgeSite1,poi,fcGdr); // non-composite girder
      Float64 tSlab = pBridge->GetStructuralSlabDepth(poi);

      Float64 dv = ecc + Yt + tSlab/2;

      Vuh.Left()  = vu.Left() / dv;
      Vuh.Right() = vu.Right()/ dv;

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
   GET_IFACE(IStirrupGeometry, pStirrupGeometry);
   Float64 Avfg=0.0;
   Float64 Sg=0.0;
   bool bDoStirrupsEngageDeck = pStirrupGeometry->DoStirrupsEngageDeck(poi.GetSpan(),poi.GetGirder());
   if (bDoStirrupsEngageDeck)
   {
      Uint32 nl = pStirrupGeometry->GetVertStirrupBarCount(poi);
      Avfg = pStirrupGeometry->GetVertStirrupBarArea(poi)*nl; 
      Sg  = pStirrupGeometry->GetS(poi);
   }
   pArtifact->SetAvfGirder(Avfg);
   pArtifact->SetSGirder(Sg);

   // top flange stirrups
   Float64 Avftf=0.0;
   Float64 Stf=0.0;
   const Uint16 nlegs_topflange = 2; // always for top flange stirrups
   Avftf = pStirrupGeometry->GetTopFlangeBarArea(poi)*nlegs_topflange; 
   Stf  = pStirrupGeometry->GetTopFlangeS(poi);

   pArtifact->SetAvfTopFlange(Avftf);
   pArtifact->SetSTopFlange(Stf);

   // legs per stirrup
   Uint32 num_legs = 0;

   if ( bDoStirrupsEngageDeck )
      num_legs += pStirrupGeometry->GetVertStirrupBarCount(poi);

   if (Avftf>0.0)
      num_legs += nlegs_topflange;

   pArtifact->SetNumLegs(num_legs);

   // friction and cohesion factors
   bool is_roughened = pBridge->AreGirderTopFlangesRoughened();
   Float64 c = lrfdConcreteUtil::ShearCohesionFactor(is_roughened);
   Float64 u = lrfdConcreteUtil::ShearFrictionFactor(is_roughened);
   Float64 K1 = lrfdConcreteUtil::HorizShearK1(is_roughened);
   Float64 K2 = lrfdConcreteUtil::HorizShearK2(is_roughened);

   pArtifact->SetCohesionFactor(c);
   pArtifact->SetFrictionFactor(u);
   pArtifact->SetK1(K1);
   pArtifact->SetK2(K2);

   // nominal shear capacities 5.8.4.1-2,3
   Float64 Vn1, Vn2, Vn3;
   lrfdConcreteUtil::HorizontalShearResistances(c, u, K1, K2, Acv, pArtifact->GetAvOverS(), Pc, fc, fy,
                                                &Vn1, &Vn2, &Vn3);
   pArtifact->SetVn(Vn1, Vn2, Vn3);

   Float64 phi = 0.9;
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
      Float64 slab_depth      = pBridge->GetGrossSlabDepth(poi);
      Float64 trib_slab_width = pSectProp2->GetTributaryFlangeWidth(poi);
      Float64 tot_depth = pSectProp2->GetDistTopSlabToTopGirder(poi);
      Float64 top_flange_width = pGdr->GetTopFlangeWidth(poi);
      
      // conservative not to use sacrificial material
      Float64 slab_volume   = trib_slab_width  * (slab_depth - pDeck->SacrificialDepth);
      Float64 haunch_volume = top_flange_width * (tot_depth - slab_depth);
      
      wslab = (slab_volume + haunch_volume) * slab_unit_weight;
   }
   else
   {
      // SIP Deck Panels
      Float64 slab_depth      = pBridge->GetGrossSlabDepth(poi);
      Float64 top_flange_width = pGdr->GetTopFlangeWidth(poi);
      Float64 panel_support = pDeck->PanelSupport;
      Float64 tot_depth = pSectProp2->GetDistTopSlabToTopGirder(poi);

      Uint32 nMatingSurfaces = pGdr->GetNumberOfMatingSurfaces(spanIdx,gdrIdx);
      Float64 wMating = 0; // sum of mating surface widths... less deck panel support width
      for ( Uint16 i = 0; i < nMatingSurfaces; i++ )
      {
         if ( pBridge->IsExteriorGirder(spanIdx,gdrIdx) && 
              ((gdrIdx == 0 && i == 0) || // Left exterior girder
               (gdrIdx == pSpan->GetGirderCount()-1 && i == nMatingSurfaces-1))  // right exterior girder
            )
            wMating += pGdr->GetMatingSurfaceWidth(poi,i)/2 - panel_support;
         else
            wMating += pGdr->GetMatingSurfaceWidth(poi,i) - 2*panel_support;
      }

      wslab = wMating*(tot_depth - pDeck->SacrificialDepth)*slab_unit_weight;

      // If exterior, add weight of cast overhang
      if ( pBridge->IsExteriorGirder(spanIdx,gdrIdx) )
      {
         Float64 slab_overhang;

         if ( poi.GetGirder() == 0 )
            slab_overhang = pBridge->GetLeftSlabOverhang(spanIdx,poi.GetDistFromStart()); 
         else
            slab_overhang = pBridge->GetRightSlabOverhang(spanIdx,poi.GetDistFromStart());

         Float64 top_width = pGdr->GetTopWidth(poi); // total width of the top of the girder

         Float64 woverhang = (slab_overhang - top_width/2)*(slab_depth - pDeck->SacrificialDepth)*slab_unit_weight;

         wslab += woverhang;

      }
   }


   Float64 comp_force = wslab;
   return comp_force;
}

void pgsDesigner2::CheckFullStirrupDetailing(const pgsPointOfInterest& poi, 
                                            const pgsVerticalShearArtifact& vertArtifact,
                                            const SHEARCAPACITYDETAILS& scd,
                                            const sysSectionValue& vu,
                                            Float64 fcGdr, Float64 fy,
                                            pgsStirrupDetailArtifact* pArtifact )
{
   // need bv and dv
   GET_IFACE(IShearCapacity, pShearCapacity);
   Float64 bv = scd.bv;
   Float64 dv = scd.dv;
   pArtifact->SetBv(bv);
   pArtifact->SetDv(dv);

   // av/s and fy rebar
   Float64 Avfs=0.0;
   Float64 s = 0.0;
   GET_IFACE(IStirrupGeometry, pStirrupGeometry);
   GET_IFACE(IBridgeMaterial,pMaterial);
   BarSizeType size = pStirrupGeometry->GetVertStirrupBarSize(poi);
   if ( size != 0 )
   {
      Uint32 nl = pStirrupGeometry->GetVertStirrupBarCount(poi);
      Float64 Av = pStirrupGeometry->GetVertStirrupBarArea(poi)*nl; 
      s  = pStirrupGeometry->GetS(poi);
      if (s!=0.0)
         Avfs = Av/s;
      else
         CHECKX(0,"S should not be zero - GUI should be checking this");
   }

   pArtifact->SetBarSize(size);
   pArtifact->SetAvs(Avfs);
   pArtifact->SetS(s);

   // see if we even need to have stirrups
   bool is_app = vertArtifact.GetAreStirrupsReqd();
   pArtifact->SetApplicability(is_app);

   // Minimum transverse reinforcement 5.8.2.5
   // Set to zero if not applicable
   GET_IFACE(ITransverseReinforcementSpec,pTransverseReinforcementSpec);
   Float64 avs_min = 0.0;
   if (is_app)
   {
      avs_min = pTransverseReinforcementSpec->GetAvOverSMin(fcGdr, bv, fy);
   }
   pArtifact->SetAvsMin(avs_min);

   Float64 vumax = max(fabs(vu.Left()),fabs(vu.Right()));
   // applied shear force
   pArtifact->SetVu(vumax);

   // max bar spacing
   Float64 s_max;
   Float64 s_under, s_over;
   pTransverseReinforcementSpec->GetMaxStirrupSpacing(&s_under, &s_over);

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   bool bAfter1999 = ( pSpecEntry->GetSpecificationType() >= lrfdVersionMgr::SecondEditionWith2000Interims ? true : false );

   Float64 x = (bAfter1999 ? 0.125 : 0.100);

   Float64 vu_limit = x * fcGdr * bv * dv; // 5.8.2.7
   pArtifact->SetVuLimit(vu_limit);
   if (vumax < vu_limit)
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
   if ( size != 0 )
   {
      Float64 db = pStirrupGeometry->GetVertStirrupBarNominalDiameter(poi);
      Float64 as = pMaterial->GetMaxAggrSizeGdr(poi.GetSpan(),poi.GetGirder());
      s_min = pTransverseReinforcementSpec->GetMinStirrupSpacing(as, db);
   }
   pArtifact->SetSMin(s_min);
}



void pgsDesigner2::CheckLongReinfShear(const pgsPointOfInterest& poi, 
                                      pgsTypes::Stage stage,
                                      pgsTypes::LimitState ls,
                                      const SHEARCAPACITYDETAILS& scd,
                                      pgsLongReinfShearArtifact* pArtifact )
{
   SpanIndexType span = poi.GetSpan();
   GirderIndexType gdr  = poi.GetGirder();

   // check to see if we are outside of the faces of support.... If so, then this check doesn't apply
   GET_IFACE(IPointOfInterest, pIPoi);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(stage,span,gdr,POI_FACEOFSUPPORT);
   ATLASSERT(vPoi.size() == 2);
   pgsPointOfInterest leftFaceOfSupport  = vPoi[0];
   pgsPointOfInterest rightFaceOfSupport = vPoi[1];

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

   // longitudinal steel
   Float64 fy = scd.fy; 
   pArtifact->SetFy(fy);

   Float64 as = 0;
   if ( pSpecEntry->IncludeRebarForShear() )
   {
      GET_IFACE(ILongRebarGeometry,pRebarGeometry);

      if ( scd.bTensionBottom )
         as = pRebarGeometry->GetAsBottomHalf(poi,true);
      else
         as = pRebarGeometry->GetAsTopHalf(poi,true);
   }
   pArtifact->SetAs(as);

   // prestress
   GET_IFACE(IStrandGeometry,pStrandGeom);

   // area of prestress on flexural tension side... adjusted for development length
   Float64 aps = (scd.bTensionBottom ? pStrandGeom->GetApsBottomHalf(poi,true) : pStrandGeom->GetApsTopHalf(poi,true));

   // get maximum available prestress
   GET_IFACE(IPrestressForce,pPSForce);
   STRANDDEVLENGTHDETAILS details = pPSForce->GetDevLengthDetails(poi,false); // not debonded
   Float64 fps = details.fps;
   
   pArtifact->SetAps(aps);
   pArtifact->SetFps(fps);

   // set up demands... if this section is before/after the critical section, use the values at the critical section
   // see C5.8.3.5

   // Get poi's at critical section
   pgsPointOfInterest LeftCS;
   pgsPointOfInterest RightCS;
   pIPoi->GetCriticalSection(ls,span,gdr,&LeftCS,&RightCS);

   bool bBeforeLeftCS = poi.GetDistFromStart()     < LeftCS.GetDistFromStart();
   bool bAfterRightCS = RightCS.GetDistFromStart() < poi.GetDistFromStart();

   Float64 vu = scd.Vu;
   Float64 vs = scd.Vs;
   Float64 vp = scd.Vp;
   Float64 theta = scd.Theta;
   if ( bBeforeLeftCS || bAfterRightCS )
   {
      GET_IFACE(IShearCapacity,pShearCapacity);
      SHEARCAPACITYDETAILS scd2;
      pShearCapacity->GetShearCapacityDetails(ls,stage,(bBeforeLeftCS ? LeftCS : RightCS),&scd2);
      vu = scd2.Vu;
      vs = scd2.Vs;
      vp = scd2.Vp;
      theta = scd2.Theta;
   }


   // flexure demand
   Float64 mu = scd.Mu;
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

   GET_IFACE(IBridge,pBridge);
   Float64 start_end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
   Float64 end_end_size   = pBridge->GetGirderEndConnectionLength(poi.GetSpan(),poi.GetGirder());
   Float64 length         = pBridge->GetGirderLength(poi.GetSpan(),poi.GetGirder());

   GET_IFACE(IPointOfInterest,pPOI);
   pgsPointOfInterest cs_start, cs_end;
   pPOI->GetCriticalSection(ls,poi.GetSpan(),poi.GetGirder(),&cs_start,&cs_end);


   Float64 demand;
   Uint16 equation = 999; // dummy value

   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   {
      if (InRange(start_end_size,poi.GetDistFromStart(),cs_start.GetDistFromStart()) || InRange(cs_end.GetDistFromStart(),poi.GetDistFromStart(),length-end_end_size) )
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
           (lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::SecondEditionWith2003Interims && (IsEqual(poi.GetDistFromStart(),start_end_size) || IsEqual(poi.GetDistFromStart(),length-end_end_size)))
            ||
           // Spec is 2004 AND poi is between point of support and critical section for shear
           (lrfdVersionMgr::SecondEditionWith2003Interims < lrfdVersionMgr::GetVersion() && lrfdVersionMgr::GetVersion() <= lrfdVersionMgr::ThirdEdition2004 && 
              (InRange(start_end_size,poi.GetDistFromStart(),cs_start.GetDistFromStart()) || InRange(cs_end.GetDistFromStart(),poi.GetDistFromStart(),length-end_end_size)) )
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
      pMomentCapacity->GetMomentCapacityDetails(pgsTypes::BridgeSite3,poi,scd.bTensionBottom,&mcd);

      double Mr = mcd.Phi*mcd.Mn;
      pArtifact->SetMr(Mr);
   }

   Float64 capacity = as*fy + aps*fps;

   pArtifact->SetEquation(equation);
   pArtifact->SetDemandForce(demand);
   pArtifact->SetCapacityForce(capacity);
}

void pgsDesigner2::CheckConfinement( SpanIndexType span,GirderIndexType gdr, Uint32 zoneNum, pgsConfinementArtifact* pArtifact )
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IStrandGeometry,pStrandGeometry);
   GET_IFACE(IStirrupGeometry, pStirrupGeometry);
   GET_IFACE(IGirder,pGdr);

   // determine if we even need to check this
   Float64 gird_length  = pBridge->GetGirderLength(span, gdr);
   Float64 gird_length2 = gird_length/2.;

   // NOTE: This d is defined differently than in 5.10.10.2 of the 2nd 
   //       edition of the spec. We think what they really meant to say 
   //       was d = the overall depth of the precast member.
#pragma Reminder("UPDATE: This needs to be done at each end of a girder")
   Float64 d = pGdr->GetHeight( pgsPointOfInterest(span, gdr,0.00) ); // assuming start end only
   Float64 zl = 1.5*d;
   pArtifact->SetApplicableZoneLength(zl);

   Float64 zone_start = pStirrupGeometry->GetZoneStart(span,gdr,zoneNum);
   Float64 zone_end   = pStirrupGeometry->GetZoneEnd(span,gdr,zoneNum);
   pArtifact->SetZoneEnd(zone_end);

   // mirror zones to check for applicability
   if (zone_end > gird_length2)
      zone_end = gird_length-zone_end;

   if (zone_start > gird_length2)
      zone_start = gird_length-zone_start;

   // tolerance check to 1mm so we don't get hosed by numerics
   Float64 tol = ::ConvertToSysUnits(1.0,unitMeasure::Millimeter);
   bool is_app = zl+tol>zone_start || zl+tol>zone_end;
   pArtifact->SetApplicability(is_app);

   // determine current stirrup config
   BarSizeType rbsiz=0;
   Float64 s =0.0;
   if ( pStirrupGeometry->IsConfinementZone(span,gdr,zoneNum) )
   {
      s  = pStirrupGeometry->GetS(span,gdr,zoneNum);
      rbsiz = pStirrupGeometry->GetConfinementBarSize(span,gdr);
      CHECKX(s!=0,"S should not be zero - GUI should be checking this");
   }
   pArtifact->SetS(s);
   pArtifact->SetBarSize(rbsiz);

   // get spec constraints
   BarSizeType szmin = 0;
   Float64 smax = 0.0;
   if (is_app)
   {
      GET_IFACE(ITransverseReinforcementSpec,pTransverseReinforcementSpec);
      szmin = pTransverseReinforcementSpec->GetMinConfinmentBarSize();
      smax  = pTransverseReinforcementSpec->GetMaxConfinmentBarSpacing();
   }
   pArtifact->SetMinBarSize(szmin);
   pArtifact->SetSMax(smax);
}

void pgsDesigner2::CheckMomentCapacity(SpanIndexType span,GirderIndexType gdr,pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsGirderArtifact* pGdrArtifact)
{
   GET_IFACE(IPointOfInterest, pIPoi);

   std::vector<pgsPointOfInterest> vPoi;
   std::vector<pgsPointOfInterest>::iterator iter;

   if ( stage == pgsTypes::BridgeSite1 )
   {
      vPoi = pIPoi->GetPointsOfInterest(stage,span,gdr,POI_FLEXURECAPACITY);
   }
   else
   {
      PoiAttributeType attrib = POI_FLEXURECAPACITY | POI_SHEAR;
      attrib |= (ls == pgsTypes::StrengthI ? POI_CRITSECTSHEAR1 : POI_CRITSECTSHEAR2);
      vPoi = pIPoi->GetPointsOfInterest(stage, span, gdr, attrib, POIFIND_OR );
   }

   PierIndexType prev_pier = span;
   PierIndexType next_pier = prev_pier+1;
   GET_IFACE(IBridge,pBridge);
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++)
   {
      const pgsPointOfInterest& poi = *iter;
      pgsFlexuralCapacityArtifactKey key(stage,ls,poi.GetDistFromStart());

      // we always do positive moment
      pgsFlexuralCapacityArtifact pm_artifact = CreateFlexuralCapacityArtifact(poi,stage,ls,true);


      // negative moment is a different story. there must be a negative moment connection
      // at one end of the girder
      pgsFlexuralCapacityArtifact nm_artifact;
      bool bComputeNegativeMomentCapacity = false;

      GET_IFACE(ISpecification,pSpec);
      pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

      if ( stage == pgsTypes::BridgeSite3 && (analysisType == pgsTypes::Continuous || analysisType == pgsTypes::Envelope) )
      {
         bool bContinuousAtPrevPier,bContinuousAtNextPier,bValue;
         pBridge->IsContinuousAtPier(prev_pier,&bValue,&bContinuousAtPrevPier);
         pBridge->IsContinuousAtPier(next_pier,&bContinuousAtNextPier,&bValue);

         bool bIntegralAtPrevPier,bIntegralAtNextPier;
         pBridge->IsIntegralAtPier(prev_pier,&bValue,&bIntegralAtPrevPier);
         pBridge->IsIntegralAtPier(next_pier,&bIntegralAtNextPier,&bValue);

         bComputeNegativeMomentCapacity = ( bContinuousAtPrevPier || bContinuousAtNextPier || bIntegralAtPrevPier || bIntegralAtNextPier );
      }

      if ( bComputeNegativeMomentCapacity )
         nm_artifact = CreateFlexuralCapacityArtifact(poi,stage,ls,false);

      pGdrArtifact->AddFlexuralCapacityArtifact(key,pm_artifact,nm_artifact);
   }
}

void pgsDesigner2::InitShearCheck(SpanIndexType span,GirderIndexType gdr,pgsTypes::LimitState ls)
{
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // get the reactions and determine if we can skip POIs outside of the critical sections
   PierIndexType prev_pier = PierIndexType(span);
   PierIndexType next_pier = prev_pier+1;

   GET_IFACE(ILimitStateForces,pForces);
   double Rmin, Rmax;
   if ( analysisType == pgsTypes::Envelope )
   {
      double min,max;

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
      double min,max;

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

   pgsPointOfInterest leftCS, rightCS;
   GET_IFACE(IPointOfInterest,pPOI);
   pPOI->GetCriticalSection(ls,span,gdr,&leftCS,&rightCS);
   m_LeftCS  = leftCS.GetDistFromStart();
   m_RightCS = rightCS.GetDistFromStart();

   // DETERMINE IF vu <= 0.18f'c at each POI... set a boolean flag that indicates if strut and tie analysis is required
   // LRFD 5.8.3.2
   GET_IFACE(IBridge,pBridge);
   bool bIntegral, bIntegralLeft, bIntegralRight; // bIntergral is a dummy variable for the side of the pier we don't care about
   pBridge->IsIntegralAtPier(prev_pier,&bIntegral,&bIntegralLeft);    // right side of left pier (start of span)
   pBridge->IsIntegralAtPier(next_pier,&bIntegralRight,&bIntegral); // left side of right pier (end of span)

   // NOTE: scd.vfc is v/f'c. Since v is divided by f'c, 0.18f'c divided by f'c is simply 0.18
   GET_IFACE(IShearCapacity,pShearCapacity);
   SHEARCAPACITYDETAILS scd;
   pShearCapacity->GetShearCapacityDetails( ls, pgsTypes::BridgeSite3, leftCS, &scd );
   m_bLeftCS_StrutAndTieRequired = ( 0.18 < scd.vfc && !bIntegralLeft );

   pShearCapacity->GetShearCapacityDetails( ls, pgsTypes::BridgeSite3, rightCS, &scd );
   m_bRightCS_StrutAndTieRequired = ( 0.18 < scd.vfc && !bIntegralRight );
}

void pgsDesigner2::CheckShear(SpanIndexType span,GirderIndexType gdr,pgsTypes::Stage stage,pgsTypes::LimitState ls,pgsGirderArtifact* pGdrArtifact)
{
   ATLASSERT( stage == pgsTypes::BridgeSite3);
   InitShearCheck(span,gdr,ls); // sets up some class member variables used for checking
                                // this span and girder

   pgsStirrupCheckArtifact* pstirrup_artifact= pGdrArtifact->GetStirrupCheckArtifact();
   CHECK(pstirrup_artifact);
   GET_IFACE(IBridgeMaterial,pMaterial);
   Float64 fc_slab = pMaterial->GetFcSlab();
   Float64 fc_girder = pMaterial->GetFcGdr(span,gdr);
   pstirrup_artifact->SetFc(fc_girder);
   
   double Es, fy;
   pMaterial->GetTransverseRebarProperties(span,gdr,&Es,&fy);
   pstirrup_artifact->SetFy(fy);

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   // poi-based shear check
   // Get the basic poi's
   GET_IFACE(IPointOfInterest, pIPoi);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(stage,span,gdr,POI_SHEAR | POI_TABULAR);

   GET_IFACE(ILimitStateForces, pLimitStateForces);

   // loop over pois
   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++)
   {
      const pgsPointOfInterest& poi = *iter;
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

      sysSectionValue Vu = Max4(abs(Vmin.Left()),abs(Vmax.Left()),abs(Vmin.Right()),abs(Vmax.Right()));

      pgsStirrupCheckAtPoisArtifact artifact = CreateStirrupCheckAtPoisArtifact(poi,stage,ls,Vu,fc_slab,fc_girder,fy);

      pgsStirrupCheckAtPoisArtifactKey key(stage,ls,poi.GetDistFromStart());
      pstirrup_artifact->AddStirrupCheckAtPoisArtifact(key,artifact);
   }

   GET_IFACE(ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   bool check_confinement = pSpecEntry->IsAnchorageCheckEnabled();

   // zone-based shear check
   GET_IFACE(IStirrupGeometry, pStirrupGeometry);
   Uint32 nZones = pStirrupGeometry->GetNumZones(span,gdr);
   for (Uint32 zoneIdx = 0; zoneIdx < nZones; zoneIdx++)
   {
      pgsStirrupCheckAtZonesArtifactKey key(zoneIdx);
      pgsStirrupCheckAtZonesArtifact artifact = CreateStirrupCheckAtZonesArtifact(span, gdr, zoneIdx,check_confinement);
      pstirrup_artifact->AddStirrupCheckAtZonesArtifact(key,artifact);
   }
}

void pgsDesigner2::CheckSplittingZone(SpanIndexType span,GirderIndexType gdr,pgsGirderArtifact* pGdrArtifact)
{
   // don't need to do anything if disabled
   GET_IFACE(ISpecification,pSpec);
   GET_IFACE(ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if (!pSpecEntry->IsAnchorageCheckEnabled())
      return;

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IStrandGeometry,pStrandGeometry);
   GET_IFACE(IStirrupGeometry, pStirrupGeometry);
   GET_IFACE(IBridgeMaterial,pMat);
   GET_IFACE(IGirder,pGdr);
   GET_IFACE(ITransverseReinforcementSpec,pTransverseReinforcementSpec);
   GET_IFACE(ILosses,pLosses);
   GET_IFACE(IPrestressForce,pPrestressForce);

   GET_IFACE(IProgress,pProgress);
   pgsAutoProgress ap(pProgress);
   std::ostringstream os;
   os << "Checking splitting requirements for Span "
      << LABEL_SPAN(span) << " Girder "
      << LABEL_GIRDER(gdr) << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );


   // get POI at point of prestress transfer
   // this is where the prestress force is fully transfered
   GET_IFACE(IPointOfInterest,pPOI);
   std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(pgsTypes::CastingYard,span,gdr,POI_PSXFER);
   ATLASSERT(vPOI.size() != 0);
   pgsPointOfInterest poi = vPOI[0];

   pgsSplittingZoneArtifact* pArtifact = pGdrArtifact->GetSplittingZoneArtifact();

   pArtifact->SetIsApplicable(true);

   Float64 h = pGdr->GetSplittingZoneHeight( poi );
   pArtifact->SetH(h);

   // basically this is h/4 except that the 4 is a parametric value
   pArtifact->SetSplittingZoneLengthFactor(pTransverseReinforcementSpec->GetSplittingZoneLengthFactor());
   Float64 zl = pTransverseReinforcementSpec->GetSplittingZoneLength(h);
   pArtifact->SetSplittingZoneLength(zl);

   // Get the splitting force parameters (the artifact actually computes the splitting force)
   Float64 Fpj = pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::Jacking);
   Float64 dFpT = pLosses->GetAfterXferLosses(poi,pgsTypes::Permanent);

   StrandIndexType Ns, Nh, Nt;
   Ns = pStrandGeometry->GetNumStrands(span,gdr,pgsTypes::Straight);
   Nh = pStrandGeometry->GetNumStrands(span,gdr,pgsTypes::Harped);
   Nt = pStrandGeometry->GetNumStrands(span,gdr,pgsTypes::Temporary);

   StrandIndexType Nsd, Nhd, Ntd;
   Nsd = pStrandGeometry->GetNumDebondedStrands(span,gdr,pgsTypes::Straight);
   Nhd = pStrandGeometry->GetNumDebondedStrands(span,gdr,pgsTypes::Harped);
   Ntd = pStrandGeometry->GetNumDebondedStrands(span,gdr,pgsTypes::Temporary);

   // if the temporary strands aren't pretensioned, then they aren't in the section
   // when Splitting is checked!!!
   GET_IFACE(IGirderData,pGirderData);
   CGirderData girderData = pGirderData->GetGirderData(span,gdr);
   if ( girderData.TempStrandUsage != pgsTypes::ttsPretensioned )
   {
      Nt  = 0;
      Ntd = 0;
   }

   const matPsStrand* pStrand = pMat->GetStrand(span,gdr);
   Float64 aps = pStrand->GetNominalArea();


   StrandIndexType nStrands  = Ns  + Nh  + Nt;
   StrandIndexType nDebonded = Nsd + Nhd + Ntd;
   Float64 Aps = aps*(nStrands-nDebonded);


   pArtifact->SetAps(Aps);
   pArtifact->SetFpj(Fpj);
   pArtifact->SetLossesAfterTransfer(dFpT);

   // Compute Splitting resistance
   Float64 Es, fy;
   pMat->GetTransverseRebarProperties(span,gdr,&Es,&fy);
   Float64 fs = pTransverseReinforcementSpec->GetMaxSplittingStress(fy);
   pArtifact->SetFs(fs);

   Float64 AvsVert, AvsHorz;
   pStirrupGeometry->GetAv(span,gdr,0.0,zl,&AvsVert,&AvsHorz);

   pgsTypes::SplittingDirection splittingDirection = pGdr->GetSplittingDirection(span,gdr);
   pArtifact->SetSplittingDirection(splittingDirection);

   Float64 Avs;
   if ( splittingDirection == pgsTypes::sdVertical )
      Avs = AvsVert;
   else
      Avs = AvsHorz;

   pArtifact->SetAvs(Avs);

   Float64 Pr = fs*Avs;
   pArtifact->SetSplittingResistance(Pr);
}

void pgsDesigner2::CheckGirderDetailing(SpanIndexType span,GirderIndexType gdr,pgsGirderArtifact* pGdrArtifact)
{
   // 5.14.1.2.2
   GET_IFACE(IProgress,pProgress);
   pgsAutoProgress ap(pProgress);
   std::ostringstream os;
   os << "Checking Girder Detailing requirements for Span "
      << LABEL_SPAN(span) << " Girder "
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
   std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(pgsTypes::CastingYard,span,gdr,POI_ALL,POIFIND_OR);

   Float64 minTopFlange = DBL_MAX;
   Float64 minBotFlange = DBL_MAX;
   Float64 minWeb       = DBL_MAX;

   GET_IFACE(IGirder,pGdr);
   Float64 nTopFlanges = pGdr->GetNumberOfTopFlanges(span,gdr);
   WebIndexType nWebs = pGdr->GetNumberOfWebs(span,gdr);
   FlangeIndexType nBotFlanges = pGdr->GetNumberOfBottomFlanges(span,gdr);

   if ( nTopFlanges != 0 || nWebs != 0 || nBotFlanges != 0 )
   {
      std::vector<pgsPointOfInterest>::iterator iter;
      for ( iter = vPOI.begin(); iter != vPOI.end(); iter++ )
      {
         pgsPointOfInterest poi = *iter;
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
   pgsAutoProgress ap(pProgress);
   std::ostringstream os;
   os << "Checking Strand Slope requirements for Span "
      << LABEL_SPAN(span) << " Girder "
      << LABEL_GIRDER(gdr) << std::ends;
   pProgress->UpdateMessage( os.str().c_str() );

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   bool bCheck, bDesign;
   Float64 s50, s60, s70;
   pSpecEntry->GetMaxStrandSlope(&bCheck,&bDesign,&s50,&s60,&s70);
   pArtifact->IsApplicable( bCheck );

   const matPsStrand* pStrand = pMat->GetStrand(span,gdr);
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
   pgsAutoProgress ap(pProgress);
   std::ostringstream os;
   os << "Checking Hold Down Force requirements for Span "
      << LABEL_SPAN(span) << " Girder "
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
   std::string spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   if (pSpecEntry->GetDoEvaluateLLDeflection())
   {
      GET_IFACE(IProgress,pProgress);
      pgsAutoProgress ap(pProgress);
      std::ostringstream os;
      os << "Checking Live Load Deflection requirements for Span "
         << LABEL_SPAN(span) << " Girder "
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
      std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( pgsTypes::BridgeSite1, span, gdr, POI_FLEXURESTRESS | POI_TABULAR );

      GET_IFACE(IProductForces,pForces);
   
      // determine 
      Float64 min_defl =  10000;
      Float64 max_defl = -10000;
      std::vector<pgsPointOfInterest>::const_iterator i;
      for ( i = vPoi.begin(); i != vPoi.end(); i++ )
      {
         const pgsPointOfInterest& poi = *i;

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
   std::string spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   ///////////////////////////////////////////////////////////////
   //
   // Check "A" Dimension
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
      pgsAutoProgress ap(pProgress);
      std::ostringstream os;
      os << "Checking Constructability requirements for Span "
         << LABEL_SPAN(span) << " Girder "
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
      pArtifact->CheckStirrupLength( ::ConvertToSysUnits(2.0,unitMeasure::Inch) < fabs(haunch_details.HaunchDiff) &&
                                     bDoStirrupsEngageDeck);
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
   pgsAutoProgress ap(pProgress);
   std::ostringstream os;
   os << "Checking Debonding requirements for Span "
      << LABEL_SPAN(span) << " Girder "
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
   long nSections = pStrandGeometry->GetNumDebondSections(span,gdr,IStrandGeometry::geLeftEnd,strandType);
   for ( Uint16 sectionIdx = 0; sectionIdx < nSections; sectionIdx++ )
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
   for ( Uint16 sectionIdx = 0; sectionIdx < nSections; sectionIdx++ )
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
   LOG("Entering DesignEndZoneDebonding");

   // Refine end-zone design. Lifting will always trump the simple release condition because of the
   // shorter span length.
   // Refine design for lifting. Outcome is lifting loop location and required release strength
   // If temporary strands are required, this design refinement will be incomplete. We will move on to
   // design for hauling because it will typically control the temporary strand requirements. Then,
   // we will return to design for lifting.

   pgsDesignCodes lifting_design_outcome;

   // Compute and layout debonding prior to hauling design
   std::vector<Int16> debond_demand;

   m_StrandDesignTool.DumpDesignParameters();

   if (options.doDesignLifting && m_StrandDesignTool.GetFlexuralDesignType()==dtDesignForDebonding)
   {
      LOG("*** Initial Lifting Design for Debond Section");
      DesignForLiftingDebonding(true,pProgress);

      if ( m_DesignerOutcome.WasDesignAborted() )
      {
         LOG("Initial Lifting Debond Design failed");
         LOG("=============================");
         return;
      }
      else if (m_DesignerOutcome.DidConcreteChange() )
      {
         LOG("Concrete strength changed for initial lifting design - restart");
         LOG("==============================================================");
         return; // concrete strength changed, we will want to redo strands 
      }
   }
   else
   {
      LOG("");
      LOG("*** Design debonding and release strength for Simple Release Condition at endzone");
      debond_demand = DesignEndZoneReleaseDebonding(pProgress);

      CHECK_PROGRESS_RET;

      if ( m_DesignerOutcome.WasDesignAborted() )
      {
         LOG("Failed to design Debonding for release - Abort");
         LOG("==================================================");
         ATLASSERT(0);
         return;
      }
      else if (m_DesignerOutcome.DidFinalConcreteStrengthChange() )
      {
         LOG("Final Concrete strength changed for end zone release - restart");
         LOG("==============================================================");
         return; // concrete strength changed, we will want to redo strands 
      }
   }

   m_StrandDesignTool.DumpDesignParameters();

   if (!debond_demand.empty())
   {
      // Layout debonding prior to hauling design
      LOG("Release/Lifting demand = "<<DumpIntVector(debond_demand));

      bool succ = m_StrandDesignTool.LayoutDebonding( debond_demand );

      if (!succ)
      {
         LOG("Failed to layout Debonding - Abort");
         LOG("==================================================");
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
      
      CHECK_PROGRESS_RET;

      if( m_DesignerOutcome.WasDesignAborted() )
      {
         LOG("Failed Initial Shipping Design - Abort");
         LOG("========================================");
         return;
      }
      else if ( m_DesignerOutcome.DidFinalConcreteStrengthChange() )
      {
         // No use going further - number of strands will change for design
         LOG("Final Concrete strength changed for shipping design - restart");
         LOG("==============================================================");
         return; 
      }

      // The only way hauling design can affect liting/release is if temporary strands 
      // were added. Update release strength if this is the case.
      if ( lifting_design_outcome.GetOutcome(pgsDesignCodes::LiftingRedesignAfterShipping) ||
           m_StrandDesignTool.GetNt() > 0 )
      {
         if (options.doDesignLifting && m_StrandDesignTool.GetFlexuralDesignType()==dtDesignForDebonding)
         {
            LOG("*** Secondary Lifting Design after Shipping.");
            std::vector<Int16> debond_demand_lifting;
            debond_demand_lifting = DesignForLiftingDebonding(false,pProgress);

            // Only layout debonding if first pass through lifting design could not
            if (lifting_design_outcome.GetOutcome(pgsDesignCodes::LiftingRedesignAfterShipping) && !debond_demand_lifting.empty())
            {
               LOG("Release/Lifting demand = "<<DumpIntVector(debond_demand_lifting));

               bool succ = m_StrandDesignTool.LayoutDebonding( debond_demand_lifting );
               if (!succ)
               {
                  LOG("Failed to layout Debonding - Abort");
                  LOG("==================================================");
                  m_StrandDesignTool.SetOutcome(pgsDesignArtifact::DebondDesignFailed);
                  m_DesignerOutcome.AbortDesign();
                  ATLASSERT(0);
                  return;
               }
            }
         }
         else
         {
            LOG("*** Secondary Design of release condition (strength only) after Shipping.");
            DesignEndZoneReleaseStrength(pProgress);
         }

         if ( m_DesignerOutcome.WasDesignAborted() )
         {
            LOG("Second Pass Lifting/Release Debond Design failed - Abort");
            LOG("========================================================");
            return;
         }
         else if ( m_DesignerOutcome.DidConcreteChange() )
         {
            LOG("Lifting/Release Design changed concrete strength - Restart");
            LOG("==========================================================");
            return;
         }
      }
   }
   else
   {
      LOG("");
      LOG("Skipping Hauling design");
   }

   LOG("Exiting DesignEndZoneDebonding");
}

void pgsDesigner2::DesignEndZoneHarping(arDesignOptions options, pgsDesignArtifact& artifact, IProgress* pProgress)
{
   LOG("Entering DesignEndZoneHarping");

   // Refine end-zone design. Lifting will always trump the simple release condition because of the
   // shorter span length.
   // Refine design for lifting. Outcome is lifting loop location and required release strength
   // If temporary strands are required, this design refinement will be incomplete. We will move on to
   // design for hauling because it will typically control the temporary strand requirements. Then,
   // we will return to design for lifting.

   pgsDesignCodes lifting_design_outcome;

   if (options.doDesignLifting)
   {
      LOG("*** Start Lifting design.");

      // the goal of this lifting design is to adjust the harped and straight strands
      // into the optimal configuration for fabrication
      DesignForLiftingHarping(true,pProgress);
      
      lifting_design_outcome = m_DesignerOutcome;

      if ( m_DesignerOutcome.WasDesignAborted() )
      {
         return;
      }
   }
   else
   {
      LOG("");
      LOG("*** Skipping Lifting design.");

      // lifting will control over simple release, so it's either/or here
      LOG("");
      LOG("*** Design for simple release condition at endzone");
      DesignEndZoneReleaseHarping(pProgress);

      CHECK_PROGRESS_RET;

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
      
      CHECK_PROGRESS_RET;

      if( m_DesignerOutcome.WasDesignAborted() )
      {
         return;
      }
   }
   else
   {
      LOG("");
      LOG("Skipping Hauling design");
   }

   if ( options.doDesignLifting )
   {
      // design for lifting to get the lifting configuration and required
      // release strength for lifting with temporary strands
      LOG("Design for Lifting after Shipping");
      LOG("=================================");
      DesignForLiftingHarping(false,pProgress);

      CHECK_PROGRESS_RET;

      if ( m_DesignerOutcome.WasDesignAborted() )
      {
         LOG("Lifting Design aborted");
         return;
      }
      else if ( m_DesignerOutcome.DidConcreteChange() )
      {
         LOG("Lifting Design changed concrete strength - Restart");
         LOG("==================================================");
         return;
      }
   }

   LOG("Exiting DesignEndZoneHarping");
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

   LOG("");
   LOG("UPDATE INITIAL DESIGN PARAMETERS IN MID-ZONE");
   LOG("");
   LOG("Determine initial design parameters by iterating until # Strands, f'c, f'ci, and Slab offset all converge");
   LOG("");

   m_StrandDesignTool.DumpDesignParameters();

   bool bConverged = false;
   do 
   {
      if ( pProgress->Continue() != S_OK )
         return;

      m_DesignerOutcome.Reset();

      Ns = m_StrandDesignTool.GetNs();
      Nh = m_StrandDesignTool.GetNh();
      Nt = m_StrandDesignTool.GetNt();
      fc = m_StrandDesignTool.GetConcreteStrength();
      ConcStrengthResultType str_result;
      fci = m_StrandDesignTool.GetReleaseStrength(&str_result);
      start_slab_offset = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
      end_slab_offset   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);

      LOG("");
      LOG("Initial Design Parameters Trial # " << cIter);

      DesignMidZoneInitialStrands(bUseCurrentStrands ? true : (cIter == 0 ? false : true), pProgress);

      if ( m_DesignerOutcome.WasDesignAborted() )
      {
         if ( m_StrandDesignTool.GetMaxPermanentStrands() > 0 && cIter<=nIterEarlyStage && nFutileAttempts<2)
         {
            // Could be that release strength controls instead of final. Give it a chance.
            LOG("Initial Design Trial # " << cIter <<" Failed - try to increase release strength to reduce losses");
            DesignMidZoneAtRelease(pProgress);

            // the purpose of calling DesignMidZoneAtRelease is to adjust the initial release strength
            // if it is too low. The new value is also and Initial Strength... re-initialize the
            // Fci controller with the new "Initial" strength
            double newFci = m_StrandDesignTool.GetReleaseStrength(&str_result);
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
         LOG("Did not converge in early stage of iterations = "<<nIterEarlyStage<<" take a stab at end zone release");
         LOG("======================================================================================");
         if ( m_StrandDesignTool.GetFlexuralDesignType() == dtDesignForDebonding )
         {
            // for debond design, take a "guess" at the debond configuration based on release before
            // coming up with a release strength. This is analogous to having some of the permanent
            // strands harped in the harped design case

            // the false in this call is to tell the designer not to abort if it can't fit
            // the debonding
            std::vector<Int16> debond_demand = DesignEndZoneReleaseDebonding(pProgress,false);
            if ( debond_demand.empty() )
            {
               // for the given number of strands we can't debond enough... 
               // this is just a guess to get us closer so use the max debonding
               LOG("Can't come up with enough debonding... just use the max possible");
               debond_demand = m_StrandDesignTool.GetMaxPhysicalDebonding();
            }

            m_StrandDesignTool.LayoutDebonding( debond_demand );
         }

         // find the release strength
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
      if ( !options.doDesignHauling )
      {
         DesignMidZoneFinalConcrete( pProgress );

         if (  m_DesignerOutcome.WasDesignAborted() || pProgress->Continue() != S_OK )
         {
            return;
         }
         else if ( m_DesignerOutcome.DidConcreteChange() )
         {
            LOG("Concrete Strength Changed - restart design");
            LOG("=======================================");
            continue; // back to the start of the loop
         }

         DesignMidZoneAtRelease( pProgress );

         if (  m_DesignerOutcome.WasDesignAborted() || pProgress->Continue() != S_OK )
         {
            return;
         }
         else if ( m_DesignerOutcome.DidConcreteChange() )
         {
            LOG("Concrete Strength Changed - restart design");
            LOG("=======================================");
            continue; // back to the start of the loop
         }
      }

      if (options.doDesignSlabOffset)
      {
         DesignSlabOffset( pProgress );

         if (  m_DesignerOutcome.WasDesignAborted() || pProgress->Continue() != S_OK )
         {
            return;
         }
      }
      else
      {
         LOG("Skipping Slab Offset Design due to user input");
      }

      m_StrandDesignTool.DumpDesignParameters();

      // slab offset must be equal to or slightly larger than calculated. If it is smaller, we might under design.
      Float64 AdiffStart = start_slab_offset - m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
      Float64 AdiffEnd   = end_slab_offset   - m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);
      bool Aconverged = (0.0 <= AdiffStart && AdiffStart <= ::ConvertToSysUnits(0.5,unitMeasure::Inch)) &&
                        (0.0 <= AdiffEnd   && AdiffEnd   <= ::ConvertToSysUnits(0.5,unitMeasure::Inch));

      LOG("End of trial "<<cIter);
      LOG("======================================================================"<<cIter);
      LOG("Ns: "<< (Ns==m_StrandDesignTool.GetNs() ? "Converged":"Did not Converge") );
      LOG("Nh: "<< (Nh==m_StrandDesignTool.GetNh() ? "Converged":"Did not Converge") );
      LOG("Nt: "<< (Nt==m_StrandDesignTool.GetNt() ? "Converged":"Did not Converge") );
      LOG("f'c: "<< (IsEqual(fc,m_StrandDesignTool.GetConcreteStrength()) ? "Converged":"Did not Converge") );
      LOG("f'ci: "<< (IsEqual(fci,m_StrandDesignTool.GetReleaseStrength()) ? "Converged":"Did not Converge") );
      LOG("Slab Offset:"<< (Aconverged ? "Converged":"Did not Converge") );
      LOG("======================================================================"<<cIter);

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
         LOG("# strands, f'c, f'ci, and slab offset have not converged");
      }

   } while (cIter++ < nIterMax && !bConverged );

   if ( cIter >= nIterMax )
   {
      LOG("Maximum number of iteratations was exceeded - aborting design " << cIter);
      m_StrandDesignTool.SetOutcome(pgsDesignArtifact::MaxIterExceeded);
      m_DesignerOutcome.AbortDesign();

      // Check with RDP... if cIter >= nIterMax set the design outcome to abort
      // however, we this if-block exits, the design outcome is reset (see below)
      ATLASSERT(false); 
      return;
   }

   LOG("====================================================");
   LOG("# strands, f'c, f'ci, and slab offset have Converged. Reset outcome and continue");
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
   LOG("");
   LOG("DesignMidZoneFinalConcrete:: Computing required concrete strength");

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
   std::string strLimitState[] =                {"Service I (BSS3)",          lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? "Service IA" : "Fatigue I",                "Service III",                "Service I (BSS2)",           "Service I (BSS1)",    "Service I (BSS2)"};
   PoiAttributeType find_type[] =               {POI_HARPINGPOINT|POI_PSXFER, POI_HARPINGPOINT|POI_PSXFER, POI_HARPINGPOINT|POI_MIDSPAN, POI_HARPINGPOINT|POI_PSXFER, POI_MIDSPAN,           POI_MIDSPAN};
   Float64 fmax[] =                             {Float64_Max,                 Float64_Max,                 -Float64_Max,                 Float64_Max,                 Float64_Max,           Float64_Max};

   const int NCases=6;
   Float64 fbpre[NCases];

   GET_IFACE(ILimitStateForces,pForces);
   GET_IFACE(IPrestressStresses,pPrestress);
   GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();

   // store poi and case where max happened (for debugging)
   pgsPointOfInterest maxPoi[NCases];
   Uint32 maxCase[NCases] = {-1,-1,-1,-1};

   int i = 0;
   for ( i = 0; i < NCases; i++ )
   {
      // Get Points of Interest at the expected
      std::vector<pgsPointOfInterest> vPOI = m_StrandDesignTool.GetDesignPoi(stage_type[i],find_type[i]);
      ATLASSERT(!vPOI.empty());

      LOG("Checking for " << strLimitState[i] << StrTopBot(stress_location[i]) << (stress_type[i]==pgsTypes::Tension?" Tension":" Compression") );

      for (std::vector<pgsPointOfInterest>::iterator it=vPOI.begin(); it!=vPOI.end(); it++)
      {
         if ( pProgress->Continue() != S_OK )
            return;

         const pgsPointOfInterest& poi = *it;
         double min,max;
         if ( analysisType == pgsTypes::Envelope )
         {
            pForces->GetDesignStress(limit_state[i],stage_type[i],poi,stress_location[i],fc_current,startSlabOffset,endSlabOffset,MaxSimpleContinuousEnvelope,&min,&max);
         }
         else
         {
            pForces->GetDesignStress(limit_state[i],stage_type[i],poi,stress_location[i],fc_current,startSlabOffset,endSlabOffset,analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan,&min,&max);
         }

         LOG("     max = " << ::ConvertFromSysUnits(max,unitMeasure::KSI) << " ksi, min = " << ::ConvertFromSysUnits(min,unitMeasure::KSI) << " ksi, at "<< ::ConvertFromSysUnits(poi.GetDistFromStart(), unitMeasure::Feet) << " ft" );

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
      double k = pLoadFactors->DCmax[limit_state[i]];

      LOG("Stress Demand (" << strLimitState[i] << StrTopBot(stress_location[i]) << " fmax = " << ::ConvertFromSysUnits(fmax[i],unitMeasure::KSI) << " ksi, fbpre = " << ::ConvertFromSysUnits(fbpre[i],unitMeasure::KSI) << " ksi, ftotal = " << ::ConvertFromSysUnits(fmax[i] + k*fbpre[i],unitMeasure::KSI) << " ksi, at "<< ::ConvertFromSysUnits(maxPoi[i].GetDistFromStart(), unitMeasure::Feet) << " ft" );

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
      LOG("");
   }

   LOG("Exiting DesignMidZoneFinalConcrete");
}

void pgsDesigner2::DesignMidZoneAtRelease(IProgress* pProgress)
{

   LOG("");
   LOG("Designing Mid-Zone at Release");
   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

   GET_IFACE(ILimitStateForces,pForces);
   GET_IFACE(IPointOfInterest,pIPOI);
   GET_IFACE(IPrestressStresses,pPrestress);

   GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();

   // Get Points of Interest in mid-zone
   std::vector<pgsPointOfInterest> vPOI = m_StrandDesignTool.GetDesignPoi(pgsTypes::CastingYard,POI_MIDSPAN|POI_HARPINGPOINT);
   CHECK( !vPOI.empty() );

   // let's look at bottom compression first. 
   // If we have to increase final strength, we restart
   Float64 fbot = Float64_Max;
   pgsPointOfInterest bot_poi;

   std::vector<pgsPointOfInterest>::iterator it;
   for (it=vPOI.begin(); it!=vPOI.end(); it++)
   {
      if ( pProgress->Continue() != S_OK )
         return;

      const pgsPointOfInterest& poi = *it;
      double min, max;
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

   LOG("Controlling Stress Demand at Release , bottom, compression = " << ::ConvertFromSysUnits(fbot,unitMeasure::KSI) << " KSI at "<< ::ConvertFromSysUnits(bot_poi.GetDistFromStart(), unitMeasure::Feet) << " ft" );

   ConcStrengthResultType release_result;
   Float64 fc  = m_StrandDesignTool.GetConcreteStrength();
   Float64 fci = m_StrandDesignTool.GetReleaseStrength(&release_result);
   LOG("current f'c  = " << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << " KSI" );
   LOG("current f'ci = " << ::ConvertFromSysUnits(fci,unitMeasure::KSI) << " KSI" );

   Float64 fc_comp;
   ConcStrengthResultType success = m_StrandDesignTool.ComputeRequiredConcreteStrength(fbot,pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Compression,&fc_comp);
   if ( success==ConcFailed )
   {
      LOG("Could not find adequate release strength to control mid-zone compresssion - Design Abort" );
      m_StrandDesignTool.SetOutcome(pgsDesignArtifact::ReleaseStrength);
      m_DesignerOutcome.AbortDesign();
   }

   LOG("Required Release Strength = " << ::ConvertFromSysUnits(fc_comp,unitMeasure::KSI) << " KSI" );

   // only update if we are increasing release strength - we are downstream here and a decrease is not desired
   if (fc_comp > fci)
   {
      bool bFciUpdated = m_StrandDesignTool.UpdateReleaseStrength(fc_comp, success,pgsTypes::CastingYard,pgsTypes::ServiceI, pgsTypes::Compression, pgsTypes::BottomGirder);
      if ( bFciUpdated )
      {
         fci = m_StrandDesignTool.GetReleaseStrength(&release_result);
         LOG("Release Strength Increased to "  << ::ConvertFromSysUnits(fci, unitMeasure::KSI) << " KSI");
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);

         config = m_StrandDesignTool.GetGirderConfiguration();
      }

      // We can continue if we only increase f'ci, but must restart if final was increased
      Float64 fc_new  = m_StrandDesignTool.GetConcreteStrength();
      if (fc!=fc_new)
      {
         LOG("Final Strength Also Increased to "  << ::ConvertFromSysUnits(fc_new, unitMeasure::KSI) << " KSI");
         LOG("Restart Design loop");
         LOG("===================");
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
         return;
      }
   }
   else
   {
      LOG("New release strength is less than current, no need to update");
   }

   // Now that we've passed bottom compression, look at top tension.
   // for this, we will try to adjust harped strands...
   GET_IFACE(IAllowableConcreteStress, pAllowable );
   Float64 all_tens = pAllowable->GetInitialAllowableTensileStress(fci, release_result==ConcSuccessWithRebar );
   LOG("Allowable tensile stress after Release     = " << ::ConvertFromSysUnits(all_tens,unitMeasure::KSI) << " KSI" );

   Float64 ftop = -Float64_Max;
   Float64 fetop, fptop;
   pgsPointOfInterest top_poi;

   for (it=vPOI.begin(); it!=vPOI.end(); it++)
   {
      if ( pProgress->Continue() != S_OK )
         return;

      const pgsPointOfInterest& poi = *it;
      double mine, maxe;
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

   LOG("Controlling Stress Demand at Release, Top, Tension = " << ::ConvertFromSysUnits(ftop,unitMeasure::KSI) << " KSI at "<< ::ConvertFromSysUnits(top_poi.GetDistFromStart(), unitMeasure::Feet) << " ft" );

   if (ftop>all_tens)
   {
      LOG("Tension limit exceeded - see what we can do");

      if (m_StrandDesignTool.GetFlexuralDesignType()==dtDesignForHarping)
      {
         LOG("Attempt to adjust harped strands");
         Float64 pps = m_StrandDesignTool.GetPrestressForceMz(pgsTypes::CastingYard,top_poi);

         // Compute eccentricity required to control top tension
         GET_IFACE(ISectProp2,pSectProp2);
         Float64 Ag  = pSectProp2->GetAg(pgsTypes::CastingYard,top_poi);
         Float64 Stg = pSectProp2->GetStGirder(pgsTypes::CastingYard,top_poi);
         LOG("Ag  = " << ::ConvertFromSysUnits(Ag, unitMeasure::Inch2) << " in^2");
         LOG("Stg = " << ::ConvertFromSysUnits(Stg,unitMeasure::Inch3) << " in^3");

         Float64 ecc_target = ComputeTopTensionEccentricity( pps, all_tens, fetop, Ag, Stg);
         LOG("Eccentricity Required to control Top Tension   = " << ::ConvertFromSysUnits(ecc_target, unitMeasure::Inch) << " in");

         // See if eccentricity can be adjusted and keep Final ServiceIII stresses under control
         Float64 min_ecc = m_StrandDesignTool.GetMinimumFinalMzEccentricity();
         LOG("Min eccentricity for bottom tension at BridgeSite3   = " << ::ConvertFromSysUnits(min_ecc, unitMeasure::Inch) << " in");

        StrandIndexType Nh = m_StrandDesignTool.GetNh();

         GET_IFACE(IStrandGeometry,pStrandGeom);
         Float64 offset_inc = pStrandGeom->GetHarpedHpOffsetIncrement(span,gdr);
         if (Nh>0 && offset_inc>=0.0 )
         {
            LOG("Attempt to adjust by raising harped bundles at harping points");

            Float64 off_reqd = m_StrandDesignTool.ComputeHpOffsetForEccentricity(top_poi, ecc_target,pgsTypes::CastingYard);
            LOG("Harped Hp offset required to achieve controlling Eccentricity   = " << ::ConvertFromSysUnits(off_reqd, unitMeasure::Inch) << " in");

            // round to increment
            off_reqd = CeilOff(off_reqd, offset_inc);
            LOG("Hp Offset Rounded to increment of "<<::ConvertFromSysUnits(offset_inc, unitMeasure::Inch) << " in = " << ::ConvertFromSysUnits(off_reqd, unitMeasure::Inch) << " in");

            // offset could push us out of ServiceIII bounds
            Float64 min_off = m_StrandDesignTool.ComputeHpOffsetForEccentricity(top_poi, min_ecc, pgsTypes::BridgeSite3);
            LOG("Offset Required to Create Min Eccentricity Required Final Bottom Tension   = " << ::ConvertFromSysUnits(min_off, unitMeasure::Inch) << " in");
            if (off_reqd<=min_off)
            {
               // Attempt to set our offset, this may be lowered to the highest allowed location 
               // if it is out of bounds
               m_StrandDesignTool.SetHarpStrandOffsetHp(off_reqd);
               LOG("New casting yard eccentricity is " << ::ConvertFromSysUnits( m_StrandDesignTool.ComputeEccentricity(top_poi,pgsTypes::CastingYard), unitMeasure::Inch) << " in");
               LOG("New BridgeSite 3 eccentricity is " << ::ConvertFromSysUnits( m_StrandDesignTool.ComputeEccentricity(top_poi,pgsTypes::BridgeSite3), unitMeasure::Inch) << " in");
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);

               // make sure the job was complete
               Float64 new_off = m_StrandDesignTool.GetHarpStrandOffsetHp();
               if (new_off==off_reqd)
               {
                  // Seems like a miracle with all of the conditions around here, but we succeeded
                  LOG("Strands at HP offset set successfully - Continue Onward");
                  return;
               }
               else
               {
                  // our offset attempt ran into physical constraints or hold down overload. 
                  LOG("Offset at HP not fully completed. Perhaps a change in strength can finish the job?");
               }
            }
            else
            {
               // so close, but offset failed. fallback is to increase concrete strength
               LOG("Offset Eccentricity has pushed us out of Service allowable zone - Set as high as possible and hope more concrete strength will fix problem");
               off_reqd = FloorOff(min_off,offset_inc);
               LOG("Hp Offset Rounded to increment of "<<::ConvertFromSysUnits(offset_inc, unitMeasure::Inch) << " in = " << ::ConvertFromSysUnits(off_reqd, unitMeasure::Inch) << " in");

               m_StrandDesignTool.SetHarpStrandOffsetHp(off_reqd);
               LOG("New casting yard eccentricity is " << ::ConvertFromSysUnits( m_StrandDesignTool.ComputeEccentricity(top_poi,pgsTypes::CastingYard), unitMeasure::Inch) << " in");
               LOG("New BridgeSite 3 eccentricity is " << ::ConvertFromSysUnits( m_StrandDesignTool.ComputeEccentricity(top_poi,pgsTypes::BridgeSite3), unitMeasure::Inch) << " in");
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
            }
         }
         else
         {
            // TxDOT - non-standard adjustment (Texas Two-Step)
            LOG("Attempt to trade straight strands for harped to releive top tension - TxDOT non-standard adjustment");

            StrandIndexType nh_reqd, ns_reqd;
            if (m_StrandDesignTool.ComputeAddHarpedForMzReleaseEccentricity(top_poi, ecc_target, min_ecc, &ns_reqd, &nh_reqd))
            {
               // number of straight/harped were changed. Set them
               LOG("Number of Straight/Harped were changed from "<<m_StrandDesignTool.GetNs()<<"/"<<Nh<<" to "<<ns_reqd<<"/"<<nh_reqd);
               m_StrandDesignTool.SetNumStraightHarped(ns_reqd, nh_reqd);

               LOG("Strands at HP Release set successfully - Continue Onward");
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
               return;
            }
            else
            {
               LOG("Attempt to trade straight strands for harped to releive top tension failed.");
            }
         }
      }
      else
      {
         LOG("This is a debond or straight strand design. Adjusting strands in mid-zone is not a remedy");
      }

      // If we are here,
      LOG("Only option left is to try to increase release strength to control top tension");

      Float64 fci_reqd;
      ConcStrengthResultType success = m_StrandDesignTool.ComputeRequiredConcreteStrength(ftop,pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Tension,&fci_reqd);
      if ( success!=ConcFailed )
      {
         LOG("Successfully Increased Release Strength for Release , Top, Tension psxfer  = " << ::ConvertFromSysUnits(fci_reqd,unitMeasure::KSI) << " KSI" );
         m_StrandDesignTool.UpdateReleaseStrength(fci_reqd,success,pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Tension,pgsTypes::TopGirder);
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);

         Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
         if (fc != fc_new)
         {
            LOG("However, Final Was Also Increased to " << ::ConvertFromSysUnits(fc_new,unitMeasure::KSI) << " KSI" );
            LOG("Restart design with new strengths");
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
         }
      }
      else
      {
         // Last resort, increase strengths by 500 psi and restart
         bool bSuccess = m_StrandDesignTool.Bump500(pgsTypes::CastingYard, pgsTypes::ServiceI, pgsTypes::Tension, pgsTypes::TopGirder);
         if (bSuccess)
         {
            LOG("Just threw a Hail Mary - Restart design with 500 psi higher concrete strength");
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
         }
         else
         {
            LOG("Concrete Strength Cannot be adjusted");
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
      LOG("");
      LOG("Skipping A-dimension design because there is no deck");
      // no deck
      return;
   }

   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

   Float64 AorigStart = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
   Float64 AorigEnd   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);

   // Iterate on "A" dimension and initial number of prestressing strands
   // Use a relaxed tolerance on "A" dimension.
   Int16 cIter = 0;
   Int16 nIterMax = 20;
   bool bDone = false;

   GET_IFACE(IBridgeMaterial,pMaterial);

   // Iterate until we come up with an "A" dimension and some strands
   // that are consistent for the current values of f'c and f'ci
   LOG("");
   LOG("Computing A-dimension requirement");
   LOG("A-dim Current (Start)   = " << ::ConvertFromSysUnits(AorigStart, unitMeasure::Inch) << " in" );
   LOG("A-dim Current (End)     = " << ::ConvertFromSysUnits(AorigEnd,   unitMeasure::Inch) << " in" );

   
   // to prevent the design from bouncing back and forth over two "A" dimensions that are 1/4" apart, we are going to use the
   // raw computed "A" requirement and round it after design is complete.
   // use a somewhat tight tolerance to converge of the theoretical "A" dimension
   double tolerance = ::ConvertToSysUnits(0.125,unitMeasure::Inch);
   do
   {
      if ( pProgress->Continue() != S_OK )
         return;

      Float64 AoldStart = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
      Float64 AoldEnd   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);

      // Make a guess at the "A" dimension using this initial strand configuration
      HAUNCHDETAILS haunch_details;
      GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();
      config.SlabOffset[pgsTypes::metStart] = AoldStart;
      config.SlabOffset[pgsTypes::metEnd]   = AoldEnd;
      GetHaunchDetails(span,gdr,config,&haunch_details);

      long idx = haunch_details.Haunch.size()/2;
      LOG("A-dim Calculated        = " << ::ConvertFromSysUnits(haunch_details.RequiredSlabOffset, unitMeasure::Inch) << " in");

      Float64 Anew = haunch_details.RequiredSlabOffset;

      Float64 Amin = m_StrandDesignTool.GetMinimumSlabOffset();
      if (Anew < Amin)
      {
         LOG("Calculated A-dim is less than minimum. Using minimum = " << ::ConvertFromSysUnits(Amin, unitMeasure::Inch) << " in");
         Anew = Amin;
      }

      if ( IsZero( AoldStart - Anew, tolerance ) && IsZero( AoldEnd - Anew, tolerance ))
      {
         double a;
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
   } while ( !bDone && cIter < nIterMax);

   if ( nIterMax < cIter )
   {
      LOG("Maximum number of iteratations was exceeded - aborting design " << cIter);
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

   LOG("");
   LOG("Computing initial prestressing requirements for Service in Mid-Zone");

   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

   // Get some information about the girder
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IStrandGeometry,pStrandGeom);

   GET_IFACE(IGirderData,pGdrData);
   const CGirderMaterial* pGirderMaterial = pGdrData->GetGirderMaterial(span,gdr);

   // Get controlling Point of Interest at mid zone
   pgsPointOfInterest poi = GetControllingFinalMidZonePoi(span,gdr);

   double fcgdr = m_StrandDesignTool.GetConcreteStrength();

   // Get the section properties of the girder
   GET_IFACE(ISectProp2,pSectProp2);
   Float64 Ag  = pSectProp2->GetAg(pgsTypes::CastingYard,poi);
   Float64 Stg = pSectProp2->GetStGirder(pgsTypes::CastingYard,poi);
   Float64 Sbg = pSectProp2->GetSb(pgsTypes::CastingYard,poi);
   LOG("Ag  = " << ::ConvertFromSysUnits(Ag, unitMeasure::Inch2) << " in^2");
   LOG("Stg = " << ::ConvertFromSysUnits(Stg,unitMeasure::Inch3) << " in^3");
   LOG("Sbg = " << ::ConvertFromSysUnits(Sbg,unitMeasure::Inch3) << " in^3");

   LOG("Stcg = " << ::ConvertFromSysUnits(pSectProp2->GetStGirder(pgsTypes::BridgeSite3,poi),unitMeasure::Inch3) << " in^3");
   LOG("Sbcg = " << ::ConvertFromSysUnits(pSectProp2->GetSb(pgsTypes::BridgeSite3,poi),unitMeasure::Inch3) << " in^3");

   LOG("Stcg_adjusted = " << ::ConvertFromSysUnits(pSectProp2->GetStGirder(pgsTypes::BridgeSite3,poi,fcgdr),unitMeasure::Inch3) << " in^3");
   LOG("Sbcg_adjusted = " << ::ConvertFromSysUnits(pSectProp2->GetSb(pgsTypes::BridgeSite3,poi,fcgdr),unitMeasure::Inch3) << " in^3");

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? SimpleSpan : analysisType == pgsTypes::Continuous ? ContinuousSpan : MaxSimpleContinuousEnvelope);

   Float64 startSlabOffset = m_StrandDesignTool.GetSlabOffset(pgsTypes::metStart);
   Float64 endSlabOffset   = m_StrandDesignTool.GetSlabOffset(pgsTypes::metEnd);

   GET_IFACE(IProductForces,pProductForces);
   pgsTypes::Stage girderLoadStage = pProductForces->GetGirderDeadLoadStage(span,gdr);
 
   LOG("");
   LOG("Bridge A dimension  (Start) = " << ::ConvertFromSysUnits(pBridge->GetSlabOffset(span,gdr,pgsTypes::metStart),unitMeasure::Inch) << " in");
   LOG("Bridge A dimension  (End)   = " << ::ConvertFromSysUnits(pBridge->GetSlabOffset(span,gdr,pgsTypes::metEnd),  unitMeasure::Inch) << " in");
   LOG("Current A dimension (Start) = " << ::ConvertFromSysUnits(startSlabOffset,unitMeasure::Inch) << " in");
   LOG("Current A dimension (End)   = " << ::ConvertFromSysUnits(endSlabOffset,  unitMeasure::Inch) << " in");
   LOG("");
   LOG("M girder      = " << ::ConvertFromSysUnits(pProductForces->GetMoment(girderLoadStage,pftGirder,poi,bat),unitMeasure::KipFeet) << " k-ft");
   LOG("M diaphragm   = " << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite1,pftDiaphragm,poi,bat),unitMeasure::KipFeet) << " k-ft");
   LOG("M slab        = " << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite1,pftSlab,poi,bat),unitMeasure::KipFeet) << " k-ft");
   LOG("dM slab       = " << ::ConvertFromSysUnits(pProductForces->GetDesignSlabPadMomentAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi),unitMeasure::KipFeet) << " k-ft");
   LOG("M panel       = " << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite1,pftSlabPanel,poi,bat),unitMeasure::KipFeet) << " k-ft");
   LOG("M user dc (1) = " << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite1,pftUserDC,poi,bat),unitMeasure::KipFeet) << " k-ft");
   LOG("M user dw (1) = " << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite1,pftUserDW,poi,bat),unitMeasure::KipFeet) << " k-ft");
   LOG("M barrier     = " << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite2,pftTrafficBarrier,poi,bat),unitMeasure::KipFeet) << " k-ft");
   LOG("M sidewalk    = " << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite2,pftSidewalk      ,poi,bat),unitMeasure::KipFeet) << " k-ft");
   LOG("M user dc (2) = " << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite2,pftUserDC,poi,bat),unitMeasure::KipFeet) << " k-ft");
   LOG("M user dw (2) = " << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite2,pftUserDW,poi,bat),unitMeasure::KipFeet) << " k-ft");
   LOG("M overlay     = " << ::ConvertFromSysUnits(pProductForces->GetMoment(pgsTypes::BridgeSite3,pftOverlay,poi,bat),unitMeasure::KipFeet) << " k-ft");

   double Mllmax, Mllmin;
   pProductForces->GetLiveLoadMoment(pgsTypes::lltDesign, pgsTypes::BridgeSite3,poi,bat,true,false,&Mllmin,&Mllmax);
   LOG("M ll+im min   = " << ::ConvertFromSysUnits(Mllmin,unitMeasure::KipFeet) << " k-ft");
   LOG("M ll+im max   = " << ::ConvertFromSysUnits(Mllmax,unitMeasure::KipFeet) << " k-ft");

   double fc_lldf = fcgdr;
   if ( pGirderMaterial->bUserEc )
      fc_lldf = lrfdConcreteUtil::FcFromEc( pGirderMaterial->Ec, pGirderMaterial->StrengthDensity );

   GET_IFACE(ILiveLoadDistributionFactors,pLLDF);
   double gV, gpM, gnM;
   pLLDF->GetDistributionFactors(poi,pgsTypes::StrengthI,fc_lldf,&gpM,&gnM,&gV);
   LOG("LLDF = " << gpM);
   LOG("");

   // Get Service III stress at bottom of girder
   GET_IFACE(IAllowableConcreteStress,pAllowStress);
   GET_IFACE(ILimitStateForces,pForces);
   Float64 fmin[3], fmax[3]; // 0 = Service I, 1 = Service IA, 2 = Service III
   Float64 fAllow[3];
   Float64 fpre[3];
   std::string strLimitState[] = { "Service I", lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? "Service IA" : "Fatigue I", "Service III" };
   std::string strStressLocation[] = { "Top", "Top", "Bottom" };
   pgsTypes::LimitState limit_state[] = { pgsTypes::ServiceI, lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? pgsTypes::ServiceIA : pgsTypes::FatigueI, pgsTypes::ServiceIII };
   pgsTypes::StressLocation stress_location[] = { pgsTypes::TopGirder, pgsTypes::TopGirder, pgsTypes::BottomGirder };
   pgsTypes::StressType stress_type[] = { pgsTypes::Compression, pgsTypes::Compression, pgsTypes::Tension };
   
   GET_IFACE(ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

   Float64 fc = m_StrandDesignTool.GetConcreteStrength();

   int i = 0;
   for ( i = 0; i < 3; i++ )
   {
      LOG("");
      if ( analysisType == pgsTypes::Envelope )
      {
         double min,max;
         pForces->GetDesignStress(limit_state[i],pgsTypes::BridgeSite3,poi,stress_location[i],fc,startSlabOffset,endSlabOffset,MaxSimpleContinuousEnvelope,&min,&max);
         fmax[i] = max;
         fmin[i] = min;
      }
      else
      {
         pForces->GetDesignStress(limit_state[i],pgsTypes::BridgeSite3,poi,stress_location[i],fc,startSlabOffset,endSlabOffset,analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan,&fmin[i],&fmax[i]);
      }

      double f_demand = ( stress_type[i] == pgsTypes::Compression ) ? fmin[i] : fmax[i];
      LOG("Stress Demand (" << strLimitState[i] << ", " << strStressLocation[i] << ", mid-span) = " << ::ConvertFromSysUnits(f_demand,unitMeasure::KSI) << " KSI" );


      // Get allowable tensile stress 
      fAllow[i] = pAllowStress->GetAllowableStress(pgsTypes::BridgeSite3,limit_state[i],stress_type[i],m_StrandDesignTool.GetConcreteStrength());
      LOG("Allowable stress (" << strLimitState[i] << ") = " << ::ConvertFromSysUnits(fAllow[i],unitMeasure::KSI)  << " KSI");

      // Compute required stress due to prestressing
      double k = pLoadFactors->DCmax[limit_state[i]];
      fpre[i] = (fAllow[i] - f_demand)/k;

      LOG("Reqd stress due to prestressing (" << strLimitState[i] << ") = " << ::ConvertFromSysUnits(fpre[i],unitMeasure::KSI) << " KSI" );
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
         LOG("Reducing num permanent strands from "<<m_StrandDesignTool.GetNumPermanentStrands()<<" to "<<np);
         ATLASSERT(np>0);
         m_StrandDesignTool.SetNumPermanentStrands(np);
      }
   }

   // Safety net
   StrandIndexType Np=-1, Np_old=-1;
   Int16 cIter = 0;
   Int16 maxIter = 80;

   
   do
   {
      if ( pProgress->Continue() != S_OK )
         return;

      LOG("");
      LOG("Strand Configuration Trial # " << cIter);

      LOG("Reset end-zone strands maximize harping or debonding effect");
      if (!m_StrandDesignTool.ResetEndZoneStrandConfig())
      {
         LOG("ERROR - Could not reset end-zone offsets to maximize differential");
         // this error is not very descriptive, but it probably means that there is no way for the strands to fit 
         // within offset bounds. This should have been caught in the library
         m_DesignerOutcome.AbortDesign();
         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::TooManyStrandsReqd);
         ATLASSERT(0); 
         return;
      }

      LOG("Guess at number of strands -> Ns = " << m_StrandDesignTool.GetNs() << " Nh = " << m_StrandDesignTool.GetNh() << " Nt = " << m_StrandDesignTool.GetNt() );
      m_StrandDesignTool.DumpDesignParameters();

      // Compute initial eccentricity of prestress force using current guess
      Float64 ecc;
      ecc = m_StrandDesignTool.ComputeEccentricity(poi, pgsTypes::BridgeSite3);
      LOG("Eccentricity at mid-span = " << ::ConvertFromSysUnits(ecc,unitMeasure::Inch) << " in");

      // Compute prestress force required to acheive fpre
      Float64 P_reqd[3];
      LOG("Required prestress force, P = fpre / [1/Ag + ecc/S]");
      for ( i = 0; i < 3; i++ )
      {
         Float64 S = (stress_location[i] == pgsTypes::TopGirder ? Stg : Sbg);
         P_reqd[i] = fpre[i]/(1.0/Ag + ecc/S);
         LOG("Required prestress force (" << strLimitState[i] << ") = " << ::ConvertFromSysUnits(fpre[i],unitMeasure::KSI) << "/[ 1/" << ::ConvertFromSysUnits(Ag,unitMeasure::Inch2) << " + " << ::ConvertFromSysUnits(ecc,unitMeasure::Inch) << "/" << ::ConvertFromSysUnits(S,unitMeasure::Inch3) << "] = " << ::ConvertFromSysUnits(-P_reqd[i],unitMeasure::Kip) << " Kip");
      }

      Float64 P = Min3(P_reqd[0],P_reqd[1],P_reqd[2]);
      LOG("Required prestress force = " << ::ConvertFromSysUnits(-P,unitMeasure::Kip) << " Kip");
      LOG("");

      long idx = Min3Index(P_reqd[0],P_reqd[1],P_reqd[2]);
      if ( stress_type[idx] == pgsTypes::Compression )
      {
         LOG("COMPRESSION CONTROLS");

         // stress required at top of girder to make tension control
         double fpre = P_reqd[2]/Ag + P_reqd[2]*ecc/Stg; 
         LOG("Stress due to prestressing required at top of girder to make tension control = P/Ag + Pe/Stg, where P (" << strLimitState[idx] << ") = P (Service III)");
         LOG("Stress due to prestressing required at top of girder to make tension control = " << ::ConvertFromSysUnits(P_reqd[2],unitMeasure::Kip) << "/" << ::ConvertFromSysUnits(Ag,unitMeasure::Inch2) << " + (" << ::ConvertFromSysUnits(P_reqd[2],unitMeasure::Kip) << ")(" << ::ConvertFromSysUnits(ecc,unitMeasure::Inch) << ")/" << ::ConvertFromSysUnits(Stg,unitMeasure::Inch3) << " = " << ::ConvertFromSysUnits(fpre,unitMeasure::KSI) << " KSI");

         double c = pAllowStress->GetAllowableCompressiveStressCoefficient(pgsTypes::BridgeSite3,limit_state[idx]);
         LOG("Compression stress coefficient " << c);

         double k = pLoadFactors->DCmax[limit_state[idx]];
         double fc = (fmin[idx]+k*fpre)/-c;

         LOG("Solve for required concrete strength: f " << strLimitState[idx] << " + (" << k << ")(f prestress) = f allowable = (c)(f'c)");

         LOG("Required concrete strength = [" << ::ConvertFromSysUnits(fmin[idx],unitMeasure::KSI) << " + (" << k << ")(" << ::ConvertFromSysUnits(fpre,unitMeasure::KSI) << ")] / " << -c << " = " << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << " KSI");

         Float64 fc_min = m_StrandDesignTool.GetMinimumConcreteStrength();
         fc = _cpp_max(fc,fc_min);

         LOG("Required concrete strength (adjusting for minimum allowed value) = " << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << " KSI");
         LOG("");

         bool bFcUpdated = m_StrandDesignTool.UpdateConcreteStrength(fc,pgsTypes::BridgeSite3,limit_state[idx],stress_type[idx],stress_location[idx]);

         if (bFcUpdated)
         {
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
            LOG("** End of strand configuration trial # " << cIter <<", Compression controlled, f'c changed");
            return;
         }
         else
         {
            // probably should never get here. intial strand selection too high?
            P = P_reqd[2]; // force tension to control and use service III
            LOG("** Oddball case - compressioned controlled, but did not increase concrete strength. Initial Ns too high?");
            LOG("Find concrete strength required to satisfy tension limit");

            GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();
            GET_IFACE(IPrestressStresses,pPsStress);
            Float64 fBotPre = pPsStress->GetDesignStress(pgsTypes::BridgeSite3,poi,pgsTypes::BottomGirder,config);
            double fc_rq;
            double k = pLoadFactors->DCmax[pgsTypes::ServiceIII];

            double f_allow_required = fmax[2]+k*fBotPre;
            LOG("Required allowable = fb Service III + fb Prestress = " << ::ConvertFromSysUnits(fmax[2],unitMeasure::KSI) << " + " << ::ConvertFromSysUnits(fBotPre,unitMeasure::KSI) << " = " << ::ConvertFromSysUnits(f_allow_required,unitMeasure::KSI) << " KSI");
            if ( ConcFailed != m_StrandDesignTool.ComputeRequiredConcreteStrength(f_allow_required,pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension,&fc_rq) )
            {
               // Practical upper limit here - if we are going above 15ksi, we are wasting time
               double max_girder_fc = m_StrandDesignTool.GetMaximumConcreteStrength();
               LOG("Max upper limit for final girder concrete = " << ::ConvertFromSysUnits(max_girder_fc,unitMeasure::KSI) << " KSI. Computed required strength = "<< ::ConvertFromSysUnits(fc_rq,unitMeasure::KSI) << " KSI");

               if (fc_rq < max_girder_fc)
               {
                  bool bFcUpdated = m_StrandDesignTool.UpdateConcreteStrength(fc_rq,pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension,pgsTypes::BottomGirder);
                  if ( bFcUpdated )
                  {
                     m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
                     LOG("** Oddball Success - End of strand configuration trial # " << cIter <<", Compression controlled, f'c changed");
                     return;
                  }
               }

               // Fell out of possible remedies
               LOG("OddBall Attempt Failed - May not survive this case...");
               m_StrandDesignTool.SetOutcome(pgsDesignArtifact::TooManyStrandsReqd);
               m_DesignerOutcome.AbortDesign();
               return;
            }

         }
      }

      // if we are here, tension is controlling, see if we can match Np and ecc
      Np = m_StrandDesignTool.ComputePermanentStrandsRequiredForPrestressForce(poi,P);

      if ( Np == Uint32_Max )
      {
         StrandIndexType npmax = m_StrandDesignTool.GetMaxPermanentStrands();
         if (m_StrandDesignTool.GetNumPermanentStrands()==npmax)
         {
            LOG("**** TOO MANY STRANDS REQUIRED **** - already tried max= "<<npmax);

            // OK, This is a final gasp - we have maxed out strands, now see if we can get a reasonable concrete strength
            //     to relieve tension before puking
            LOG("Hail Mary - See if reasonable concrete strength can satisfy tension limit");
            GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();
            GET_IFACE(IPrestressStresses,pPsStress);
            Float64 fBotPre = pPsStress->GetDesignStress(pgsTypes::BridgeSite3,poi,pgsTypes::BottomGirder,config);
            double k = pLoadFactors->DCmax[pgsTypes::ServiceIII];
            double f_allow_required = fmax[2]+k*fBotPre;
            LOG("Required allowable = fb Service III + fb Prestress = " << ::ConvertFromSysUnits(fmax[2],unitMeasure::KSI) << " + " << ::ConvertFromSysUnits(fBotPre,unitMeasure::KSI) << " = " << ::ConvertFromSysUnits(f_allow_required,unitMeasure::KSI) << " KSI");
            double fc_rqd;
            if ( ConcFailed != m_StrandDesignTool.ComputeRequiredConcreteStrength(f_allow_required,pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension,&fc_rqd) )
            {
               // Use user-defined practical upper limit here - if we are going for 15ksi, we are wasting time
               GET_IFACE(ILimits,pLimits);
               double max_girder_fc = pLimits->GetMaxGirderFc();
               LOG("User-defined upper limit for final girder concrete = " << ::ConvertFromSysUnits(max_girder_fc,unitMeasure::KSI) << " KSI. Computed required strength = "<< ::ConvertFromSysUnits(fc_rqd,unitMeasure::KSI) << " KSI");

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
                        LOG("  Release strength was more than 2 ksi smaller than final, bump release as well");
                        m_StrandDesignTool.UpdateReleaseStrength(fci_curr+fc_2k, strength_result, pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension,pgsTypes::BottomGirder);
                        m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);
                     }

                     LOG("** Hail Mary to increase final concrete for tension succeeded - restart design");
                     return;
                  }
               }
            }

            LOG("Hail Mary - FAILED!! There is no way to satisfy tension limit unless outer loop can fix this problem");
            m_StrandDesignTool.SetOutcome(pgsDesignArtifact::TooManyStrandsReqd);
            m_DesignerOutcome.AbortDesign();
            return;
         }
         else
         {
            LOG("**** TOO MANY STRANDS REQUIRED ****, but let's try the max before we give up: "<<npmax);
            Np = npmax;
         }
      }

      StrandIndexType np_min = m_StrandDesignTool.GetMinimumPermanentStrands();
      if (Np < np_min)
      {
         Np = np_min;
         LOG("Number of strands computed is less than minimum set for Ultimate Moment. Setting to "<<Np);
      }

      Np_old = m_StrandDesignTool.GetNumPermanentStrands();
      // set number of permanent strands
      if (!m_StrandDesignTool.SetNumPermanentStrands(Np))
      {
         LOG("Error trying to set permanent strands - Abort Design");
         m_DesignerOutcome.AbortDesign();
         return;
      }
      else
      {
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
      }

      LOG("Np = " << Np_old << " NpGuess = " << Np);
      LOG("NsGuess = " << m_StrandDesignTool.GetNs());
      LOG("NhGuess = " << m_StrandDesignTool.GetNh());
      LOG("NtGuess = " << m_StrandDesignTool.GetNt());
      LOG("** End of strand configuration trial # " << cIter <<", Tension controlled");

      if (Np==Np_old)
      {
         // solution has converged - compute and save the minimum eccentricity that we can have with
         // Np and our allowable. This will be used later to limit strand adjustments in mid-zone
         // We know that Service III controlled because w'ere here:
         Float64 pps = m_StrandDesignTool.GetPrestressForceMz(pgsTypes::BridgeSite3,poi);
         Float64 ecc_min = ComputeBottomCompressionEccentricity( pps, fAllow[2], fmax[2], Ag, Sbg);
         LOG("Minimum eccentricity Required to control Bottom Tension  = " << ::ConvertFromSysUnits(ecc_min, unitMeasure::Inch) << " in");
         LOG("Actual current eccentricity   = " << ::ConvertFromSysUnits(m_StrandDesignTool.ComputeEccentricity(poi, pgsTypes::BridgeSite3), unitMeasure::Inch) << " in");
         m_StrandDesignTool.SetMinimumFinalMzEccentricity(ecc_min);
      }

      cIter++;
   } while ( Np!=Np_old && cIter<maxIter );

   if ( cIter >= maxIter )
   {
      LOG("Maximum number of iteratations was exceeded - aborting design " << cIter);
      m_StrandDesignTool.SetOutcome(pgsDesignArtifact::MaxIterExceeded);
      m_DesignerOutcome.AbortDesign();
   }

   LOG(cIter << " iterations were used");

   LOG("");
   LOG("Preliminary Design");
   LOG("Ns = " << m_StrandDesignTool.GetNs() << " PjS = " << ::ConvertFromSysUnits(m_StrandDesignTool.GetPjackStraightStrands(),unitMeasure::Kip) << " Kip");
   LOG("Nh = " << m_StrandDesignTool.GetNh() << " PjH = " << ::ConvertFromSysUnits(m_StrandDesignTool.GetPjackHarpedStrands(),unitMeasure::Kip) << " Kip");
   LOG("Nt = " << m_StrandDesignTool.GetNt() << " PjT = " << ::ConvertFromSysUnits(m_StrandDesignTool.GetPjackTempStrands(),unitMeasure::Kip) << " Kip");
   LOG("** Preliminary Design Complete");
   LOG("===========================");
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
   std::vector<pgsPointOfInterest> vPoi = m_StrandDesignTool.GetDesignPoi(pgsTypes::BridgeSite3,POI_ALLACTIONS);

   Float64 fmax = -Float64_Max;
   pgsPointOfInterest max_poi;
   bool found=false;
   for (std::vector<pgsPointOfInterest>::iterator it=vPoi.begin(); it!=vPoi.end(); it++)
   {
      const pgsPointOfInterest& poi = *it;
      Float64 dfs = poi.GetDistFromStart();

      if (dfs>=left_limit && dfs<=rgt_limit)
      {
         // poi is in mid-zone
         double min,max;
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

   LOG("Found controlling mid-zone final poi at "<< ::ConvertFromSysUnits(max_poi.GetDistFromStart(),unitMeasure::Feet) << " ft" );

   ATLASSERT(found);
   return max_poi;
}

void pgsDesigner2::DesignEndZoneReleaseStrength(IProgress* pProgress)
{
   LOG("");
   LOG("Computing Release requirements at End-Zone - Assumes that harped strands have been raised to highest location or debonding is maximized before entering");
#pragma Reminder("This code needs to be changed if girder is not assumed to rest on ends at release")

   Float64 fc  = m_StrandDesignTool.GetConcreteStrength();
   Float64 fci = m_StrandDesignTool.GetReleaseStrength();
   LOG("current f'c  = " << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << " KSI" );
   LOG("current f'ci = " << ::ConvertFromSysUnits(fci,unitMeasure::KSI) << " KSI" );

   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();
   GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();

   GET_IFACE(ILimitStateForces,pForces);
   GET_IFACE(IPrestressStresses, pPrestress);
   GET_IFACE(IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPOI = m_StrandDesignTool.GetDesignPoi(pgsTypes::CastingYard,POI_PSXFER);
   ATLASSERT(!vPOI.empty());

   // max top tension and bottom compression stresses at critical locations
   Float64 fbot =  Float64_Max;
   Float64 ftop = -Float64_Max;
   Float64 fetop, febot; 
   Float64 fptop, fpbot; 
   pgsPointOfInterest top_poi, bot_poi;

   for (std::vector<pgsPointOfInterest>::iterator it=vPOI.begin(); it!=vPOI.end(); it++)
   {
      if ( pProgress->Continue() != S_OK )
         return;

      const pgsPointOfInterest& poi = *it;
      double mine,maxe,bogus;
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

   LOG("Controlling Stress at Release , top, tension psxfer  = " << ::ConvertFromSysUnits(ftop,unitMeasure::KSI) << " KSI at "<<::ConvertFromSysUnits(top_poi.GetDistFromStart(),unitMeasure::Feet) << " ft" );
   LOG("Controlling Stress at Release , bottom, compression psxfer = " << ::ConvertFromSysUnits(fbot,unitMeasure::KSI) << " KSI at "<<::ConvertFromSysUnits(bot_poi.GetDistFromStart(),unitMeasure::Feet) << " ft");
   LOG("External Stress Demand at Release , top, tension psxfer  = " << ::ConvertFromSysUnits(fetop,unitMeasure::KSI) << " KSI" );
   LOG("External Stress Demand at Release , bottom, compression psxfer = " << ::ConvertFromSysUnits(febot,unitMeasure::KSI) << " KSI" );

   // First crack is to design concrete release strength for harped strands raised to top.
   // No use going further if we can't
   LOG("Try Designing EndZone Release Strength at Initial Condition" );
   DesignConcreteRelease(ftop, fbot);
}

void pgsDesigner2::DesignEndZoneReleaseHarping(IProgress* pProgress)
{
   LOG("Refine harped design for release condition");
   LOG("Computing Release requirements at End-Zone - Assumes that harped strands have been raised to highest location before entering");
#pragma Reminder("This code needs to be changed if girder is not assumed to rest on ends at release")

   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();
   GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();

   GET_IFACE(ILimitStateForces,pForces);
   GET_IFACE(IPrestressStresses, pPrestress);
   GET_IFACE(IPointOfInterest,pIPOI);
   std::vector<pgsPointOfInterest> vPOI = m_StrandDesignTool.GetDesignPoi(pgsTypes::CastingYard,POI_PSXFER);
   ATLASSERT(!vPOI.empty());

   // max top tension and bottom compression stresses at critical locations
   Float64 fbot =  Float64_Max;
   Float64 ftop = -Float64_Max;
   Float64 fetop, febot; 
   Float64 fptop, fpbot; 
   pgsPointOfInterest top_poi, bot_poi;

   for (std::vector<pgsPointOfInterest>::iterator it=vPOI.begin(); it!=vPOI.end(); it++)
   {
      if ( pProgress->Continue() != S_OK )
         return;

      const pgsPointOfInterest& poi = *it;
      double mine,maxe,bogus;
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

   LOG("Controlling Stress at Release , top, tension psxfer  = " << ::ConvertFromSysUnits(ftop,unitMeasure::KSI) << " KSI" );
   LOG("Controlling Stress at Release , bottom, compression psxfer = " << ::ConvertFromSysUnits(fbot,unitMeasure::KSI) << " KSI" );
   LOG("External Stress Demand at Release , top, tension psxfer  = " << ::ConvertFromSysUnits(fetop,unitMeasure::KSI) << " KSI" );
   LOG("External Stress Demand at Release , bottom, compression psxfer = " << ::ConvertFromSysUnits(febot,unitMeasure::KSI) << " KSI" );

   GET_IFACE(IStrandGeometry,pStrandGeom);

   // See if we can adjust end strands downward
   StrandIndexType Nh = m_StrandDesignTool.GetNh();
   Float64 offset_inc = pStrandGeom->GetHarpedEndOffsetIncrement(span,gdr);

   // Get the section properties of the girder
   GET_IFACE(ISectProp2,pSectProp2);
   Float64 Ag  = pSectProp2->GetAg(pgsTypes::CastingYard,vPOI[0]);
   Float64 Stg = pSectProp2->GetStGirder(pgsTypes::CastingYard,vPOI[0]);
   Float64 Sbg = pSectProp2->GetSb(pgsTypes::CastingYard,vPOI[0]);
   LOG("Ag  = " << ::ConvertFromSysUnits(Ag, unitMeasure::Inch2) << " in^2");
   LOG("Stg = " << ::ConvertFromSysUnits(Stg,unitMeasure::Inch3) << " in^3");
   LOG("Sbg = " << ::ConvertFromSysUnits(Sbg,unitMeasure::Inch3) << " in^3");

   // compute eccentricity to control top tension
   Float64 fc  = m_StrandDesignTool.GetConcreteStrength();
   ConcStrengthResultType conc_res;
   Float64 fci = m_StrandDesignTool.GetReleaseStrength(&conc_res);
   LOG("current f'c  = " << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << " KSI "<<(conc_res==ConcSuccessWithRebar ? " min rebar assumed":" "));
   LOG("current f'ci = " << ::ConvertFromSysUnits(fci,unitMeasure::KSI) << " KSI" );

   GET_IFACE(IAllowableConcreteStress,pAllowable);
   Float64 all_tens = pAllowable->GetInitialAllowableTensileStress(fci,conc_res==ConcSuccessWithRebar);
   Float64 all_comp = pAllowable->GetInitialAllowableCompressiveStress(fci);
   LOG("Allowable tensile stress after Release     = " << ::ConvertFromSysUnits(all_tens,unitMeasure::KSI) << " KSI" );
   LOG("Allowable compressive stress after Release = " << ::ConvertFromSysUnits(all_comp,unitMeasure::KSI) << " KSI" );

   // ecc's required to control stresses
   Float64 top_pps  = m_StrandDesignTool.GetPrestressForceAtLifting(config,top_poi);
   LOG("Total Prestress Force for top location: P  = " << ::ConvertFromSysUnits(top_pps, unitMeasure::Kip) << " kip");

   Float64 ecc_tens = ComputeTopTensionEccentricity( top_pps, all_tens, fetop, Ag, Stg);
   LOG("Eccentricity Required to control Top Tension   = " << ::ConvertFromSysUnits(ecc_tens, unitMeasure::Inch) << " in");

   // ecc to control bottom compression
   Float64 bot_pps  = m_StrandDesignTool.GetPrestressForceAtLifting(config,bot_poi);
   LOG("Total Prestress Force for bottom location: P  = " << ::ConvertFromSysUnits(bot_pps, unitMeasure::Kip) << " kip");

   Float64 ecc_comp = ComputeBottomCompressionEccentricity( bot_pps, all_comp, febot, Ag, Sbg);
   LOG("Eccentricity Required to control Bottom Compression   = " << ::ConvertFromSysUnits(ecc_comp, unitMeasure::Inch) << " in");

   if (m_StrandDesignTool.GetOriginalStrandFillType() == ftMinimizeHarping)
   {
      // try to trade harped to straight and, if necessary, lower strands to achieve eccentricity
      if(ecc_tens<ecc_comp)
         LOG("Tension Controls"); 
      else
         LOG("Compression Controls");

      Float64 ecc_control = ecc_tens<ecc_comp ? ecc_tens : ecc_comp;
      const pgsPointOfInterest& poi_control = ecc_tens<ecc_comp ? top_poi : bot_poi;

      StrandIndexType Ns = m_StrandDesignTool.GetNs();
      StrandIndexType nh_reqd, ns_reqd;

      LOG("Try to raise end eccentricity by trading harped to straight and lowering ends");
      if (m_StrandDesignTool.ComputeMinHarpedForEzEccentricity(poi_control, ecc_control, pgsTypes::CastingYard, &ns_reqd, &nh_reqd))
      {
         // number of straight/harped were changed. Set them
         LOG("Number of Straight/Harped were changed from "<<Ns<<"/"<<Nh<<" to "<<ns_reqd<<"/"<<nh_reqd);
         m_StrandDesignTool.SetNumStraightHarped(ns_reqd, nh_reqd);

         m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::RetainStrandProportioning);
      }
   }
   else
   {
      StrandIndexType Nh = m_StrandDesignTool.GetNh();

      if (offset_inc>=0.0 && Nh>0)
      {
         LOG("Harped strands can be adjusted at ends for release - See how low can we go..." );
         // compute harped offset required to achieve this ecc
         Float64 off_reqd;

         // smallest ecc controls
         if (ecc_tens < ecc_comp)
         {
            LOG("Tension Controls");
            off_reqd = m_StrandDesignTool.ComputeEndOffsetForEccentricity(top_poi, ecc_tens);
         }
         else
         {
            LOG("Compression Controls");
            off_reqd = m_StrandDesignTool.ComputeEndOffsetForEccentricity(bot_poi, ecc_comp);
         }

         LOG("Harped End offset required to achieve controlling Eccentricity (raw)   = " << ::ConvertFromSysUnits(off_reqd, unitMeasure::Inch) << " in");
         // round to increment
         off_reqd = CeilOff(off_reqd, offset_inc);
         LOG("Harped End offset required to achieve controlling Eccentricity (rounded)  = " << ::ConvertFromSysUnits(off_reqd, unitMeasure::Inch) << " in");

         // Attempt to set our offset, this may be lowered to the highest allowed location 
         // if it is out of bounds
         m_StrandDesignTool.SetHarpStrandOffsetEnd(off_reqd);

         m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
      }
      else
      {
         LOG((Nh>0 ? "Cannot adjust harped strands due to user input":"There are no harped strands to adjust"));
      }
   }

   if ( pProgress->Continue() != S_OK )
      return;

   config = m_StrandDesignTool.GetGirderConfiguration();

#if defined ENABLE_LOGGING
   Float64 neff;
#endif
   LOG("New eccentricity is " << ::ConvertFromSysUnits( pStrandGeom->GetEccentricity(ecc_tens<ecc_comp?top_poi:bot_poi, config, true, &neff), unitMeasure::Inch) << " in");


   Float64 fTopPs, fBotPs;
   fTopPs = pPrestress->GetDesignStress(pgsTypes::CastingYard,top_poi,pgsTypes::TopGirder,config);
   fBotPs = pPrestress->GetDesignStress(pgsTypes::CastingYard,bot_poi,pgsTypes::BottomGirder,config);

   ftop = fetop + fTopPs;
   fbot = febot + fBotPs;

   LOG("After Adjustment, Controlling Stress at Release , Top, Tension        = " << ::ConvertFromSysUnits(ftop,unitMeasure::KSI) << " KSI" );
   LOG("After Adjustment, Controlling Stress at Release , Bottom, Compression = " << ::ConvertFromSysUnits(fbot,unitMeasure::KSI) << " KSI" );

   // Recompute required release strength
   DesignConcreteRelease(ftop, fbot);

   // Done
}

std::vector<Int16> pgsDesigner2::DesignEndZoneReleaseDebonding(IProgress* pProgress,bool bAbortOnFail)
{
   LOG("Refine Debonded design by computing debond demand levels for release condition at End-Zone");
#pragma Reminder("This code needs to be changed if girder is not assumed to rest on ends at release")

   // We also get into this function for fully debonded designs, no use debonding if so
   if (m_StrandDesignTool.GetFlexuralDesignType() != dtDesignForDebonding)
   {
      LOG("Fully bonded design - no need to compute debond levels ");
      std::vector<Int16> levels;
      levels.assign((long)0,0);
      return levels;
   }

   // compute eccentricity to control top tension
   Float64 fc  = m_StrandDesignTool.GetConcreteStrength();
   ConcStrengthResultType rebar_reqd;
   Float64 fci = m_StrandDesignTool.GetReleaseStrength(&rebar_reqd);
   LOG("current f'c  = " << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << " KSI ");
   LOG("current f'ci = " << ::ConvertFromSysUnits(fci,unitMeasure::KSI) << " KSI" );

   GET_IFACE(IAllowableConcreteStress,pAllowable);
   Float64 all_tens = pAllowable->GetInitialAllowableTensileStress(fci,rebar_reqd==ConcSuccessWithRebar);
   Float64 all_comp = pAllowable->GetInitialAllowableCompressiveStress(fci);
   LOG("Allowable tensile stress after Release     = " << ::ConvertFromSysUnits(all_tens,unitMeasure::KSI) << " KSI"<<(rebar_reqd==ConcSuccessWithRebar ? " min rebar was required for this strength":"")  );
   LOG("Allowable compressive stress after Release = " << ::ConvertFromSysUnits(all_comp,unitMeasure::KSI) << " KSI" );

   // We want to compute total debond demand, so bond all strands
   GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();
   config.Debond[pgsTypes::Straight].clear();

   // losses due to refined method will be most at end of girder - let's grab the first poi past the transfer length from end of girder
   pgsPointOfInterest sample_poi =  m_StrandDesignTool.GetDebondSamplingPOI(pgsTypes::CastingYard);
   LOG("Debond Design sample POI for prestressing force taken at "<<::ConvertFromSysUnits(sample_poi.GetDistFromStart(),unitMeasure::Feet) << " ft");

   GET_IFACE(IPrestressForce,pPrestressForce);
   Float64 strand_force = pPrestressForce->GetPrestressForcePerStrand(sample_poi, config, pgsTypes::Straight, pgsTypes::AfterXfer );

   StrandIndexType nss = config.Nstrands[pgsTypes::Straight];
   LOG("Average force per strand at sampling location = " << ::ConvertFromSysUnits(strand_force,unitMeasure::Kip) << " kip, with "<<nss<<" strands");

   GET_IFACE(ILimitStateForces,pForces);
   GET_IFACE(IPrestressStresses, pPrestress);
   std::vector<pgsPointOfInterest> vPOI = m_StrandDesignTool.GetDesignPoiEndZone(pgsTypes::CastingYard,POI_FLEXURESTRESS);
   ATLASSERT(!vPOI.empty());

   // Build stress demand
   std::vector<pgsStrandDesignTool::StressDemand> stress_demands;
   stress_demands.reserve(vPOI.size());

   for (std::vector<pgsPointOfInterest>::iterator it=vPOI.begin(); it!=vPOI.end(); it++)
   {
      const pgsPointOfInterest& poi = *it;
      double fTopAppl,fBotAppl,bogus;
      pForces->GetStress(pgsTypes::ServiceI,pgsTypes::CastingYard,poi,pgsTypes::TopGirder,   false,SimpleSpan,&bogus,&fTopAppl);
      pForces->GetStress(pgsTypes::ServiceI,pgsTypes::CastingYard,poi,pgsTypes::BottomGirder,false,SimpleSpan,&fBotAppl,&bogus);

      Float64 fTopPrestress, fBotPrestress;
      fTopPrestress = pPrestress->GetDesignStress(pgsTypes::CastingYard,poi,pgsTypes::TopGirder,config);
      fBotPrestress = pPrestress->GetDesignStress(pgsTypes::CastingYard,poi,pgsTypes::BottomGirder,config);

      // demand stress with fully bonded straight strands
      Float64 fTop = fTopAppl + fTopPrestress;
      Float64 fBot = fBotAppl + fBotPrestress;

      LOG("Computing stresses at "<<::ConvertFromSysUnits(poi.GetDistFromStart(),unitMeasure::Feet) << " ft");
      LOG("Applied Top stress = " << ::ConvertFromSysUnits(fTopAppl,unitMeasure::KSI) << " ksi. Prestress stress ="<< ::ConvertFromSysUnits(fTopPrestress,unitMeasure::KSI) << " ksi. Total stress = "<< ::ConvertFromSysUnits(fTop,unitMeasure::KSI) << " ksi");
      LOG("Applied Bottom stress = " << ::ConvertFromSysUnits(fBotAppl,unitMeasure::KSI) << " ksi. Prestress stress ="<< ::ConvertFromSysUnits(fBotPrestress,unitMeasure::KSI) << " ksi. Total stress = "<< ::ConvertFromSysUnits(fBot,unitMeasure::KSI) << " ksi");

      pgsStrandDesignTool::StressDemand demand;
      demand.m_Location = poi.GetDistFromStart();
      demand.m_TopStress = fTop;
      demand.m_BottomStress = fBot;

      stress_demands.push_back(demand);
   }

   // compute debond levels at each section from demand
   std::vector<Int16> debond_levels;
   debond_levels = m_StrandDesignTool.ComputeDebondsForDemand(stress_demands, nss, strand_force, all_tens, all_comp);

   if (  debond_levels.empty() && bAbortOnFail )
   {
      ATLASSERT(0);
      LOG("Debonding failed, this should not happen?");

      m_StrandDesignTool.SetOutcome(pgsDesignArtifact::DebondDesignFailed);
      m_DesignerOutcome.AbortDesign();
   }

   return debond_levels;
}


void pgsDesigner2::DesignConcreteRelease(Float64 ftop, Float64 fbot)
{
   LOG("Entering DesignConcreteRelease");
   LOG("Total Stress at bottom = " << ::ConvertFromSysUnits(fbot,unitMeasure::KSI) << " KSI" );
   LOG("Total Stress at top    = " << ::ConvertFromSysUnits(ftop,unitMeasure::KSI) << " KSI" );

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

      LOG("F'ci to control tension at release is = " << ::ConvertFromSysUnits(fc_tens,unitMeasure::KSI) << " KSI" );

      ConcStrengthResultType tens_success = m_StrandDesignTool.ComputeRequiredConcreteStrength(ftens,pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Tension,&fc_tens);
      if ( ConcFailed==tens_success )
      {
         LOG("Could not find adequate release strength to control tension - Design Abort" );
         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::ReleaseStrength);
         m_DesignerOutcome.AbortDesign();
         return;
      }
      else
      {
         bool bFciUpdated = m_StrandDesignTool.UpdateReleaseStrength(fc_tens, tens_success, pgsTypes::CastingYard,pgsTypes::ServiceI, pgsTypes::Tension, tens_location);
         if ( bFciUpdated )
         {
            LOG("Release Strength For tension Increased to "  << ::ConvertFromSysUnits(m_StrandDesignTool.GetReleaseStrength(), unitMeasure::KSI) << " KSI");
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);

            Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
            if (fc_new!=fc_old)
            {
               LOG("Final Strength Also Increased to "  << ::ConvertFromSysUnits(fc_new, unitMeasure::KSI) << " KSI");
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

      LOG("F'ci to control compression at release is = " << ::ConvertFromSysUnits(fc_comp,unitMeasure::KSI) << " KSI" );

      ConcStrengthResultType success = m_StrandDesignTool.ComputeRequiredConcreteStrength(fcomp,pgsTypes::CastingYard,pgsTypes::ServiceI,pgsTypes::Compression,&fc_comp);
      if ( ConcFailed==success )
      {
         LOG("Could not find adequate release strength to control compression - Design Abort" );
         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::ReleaseStrength);
         m_DesignerOutcome.AbortDesign();
         return;
      }
      else
      {
         bool bFciUpdated = m_StrandDesignTool.UpdateReleaseStrength(fc_comp, success, pgsTypes::CastingYard,pgsTypes::ServiceI, pgsTypes::Compression, comp_location);
         if ( bFciUpdated )
         {
            LOG("Release Strength For compression Increased to "  << ::ConvertFromSysUnits(m_StrandDesignTool.GetReleaseStrength(), unitMeasure::KSI) << " KSI");
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);

            Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
            if (fc_new!=fc_old)
            {
               LOG("Final Strength Also Increased to "  << ::ConvertFromSysUnits(fc_new, unitMeasure::KSI) << " KSI");
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
            }
         }
      }

   }

   LOG("Exiting DesignConcreteRelease");
}

void pgsDesigner2::DesignForLiftingHarping(bool bProportioningStrands,IProgress* pProgress)
{
   // There are two phases to lifting design. The first phase is to proportion the number of straight
   // and harped strands to obtain a "balanced" state of stresses when lifting the girder without
   // temporary strands. The stress at the harp point is basically independent of the number of
   // straight and harped strands (it changes very little with changing proportions). The optimum
   // design occurs when the stress at either the lift point or the point of prestress transfer
   // are approximately equal to the stresses at the harp point.
   //
   // The second phase of lifting design is to determine the lifting loop location and the
   // required release strength. When temporary strands are used, the release strength found
   // in the second phase will be lower then in the first phase.

   pProgress->UpdateMessage("Designing for Lifting");

   LOG("");
   LOG("DESIGNING FOR LIFTING");
   LOG("");
   m_StrandDesignTool.DumpDesignParameters();

   // get some initial data to make function calls a little easier to read
   SpanIndexType span  = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr = m_StrandDesignTool.GetGirder();

   pgsGirderHandlingChecker checker(m_pBroker,m_AgentID); // this guy can do the stability design!

   GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();
   if ( bProportioningStrands )
   {
      // if this is the first design for lifting, look at the lifting without temporary strands case
      // to get the optimum strand configuration
      LOG("Phase 1 Lifting Design - Design for Lifting without Temporary Strands");
      LOG("Determine straight/harped strands proportions");
      LOG("");
      LOG("Removing temporary strands for lifting analysis");
      config.Nstrands[pgsTypes::Temporary] = 0;
   }
#if defined _DEBUG
   else
   {
      LOG("Phase 2 Lifting Design - Design for Lifting with Temporary Strands");
      LOG("Determine lifting locations and release strength requirements");
   }
#endif

   // Do a stability based design for lifting. this will locate the lift point locations required
   // for stability
   pgsLiftingAnalysisArtifact artifact;
   // Designer manages it's own POIs
   IGirderLiftingDesignPointsOfInterest* pPoiLd = dynamic_cast<IGirderLiftingDesignPointsOfInterest*>(&m_StrandDesignTool);
   pgsDesignCodes::OutcomeType result = checker.DesignLifting(span,gdr,config,pPoiLd,&artifact,LOGGER);

#if defined _DEBUG
   LOG("-- Dump of Lifting Artifact After Design --");
   artifact.Dump(LOGGER);
   LOG("-- End Dump of Lifting Artifact --");
#endif

   m_StrandDesignTool.SetLiftingLocations(artifact.GetLeftOverhang(),artifact.GetRightOverhang());

   if ( pProgress->Continue() != S_OK )
      return;

   m_DesignerOutcome.SetOutcome(result);
   if ( m_DesignerOutcome.WasDesignAborted() )
   {
      return;
   }

   // Check to see if the girder is stable for lifting
   GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   Float64 FScr    = artifact.GetMinFsForCracking();
   Float64 FScrMin = pGirderLiftingSpecCriteria->GetLiftingCrackingFs();
   LOG("FScr = " << FScr);
   LOG("");
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
         LOG("Cannot find a pick point to safisfy FScr");
         LOG("Temporary strands required");
         LOG("Move on to Shipping Design");
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::LiftingRedesignAfterShipping);
      }
      else
      {
         // We are in the second phase lifting design. If we get to this point
         // then the girder is more unstable during lifting than during shipping.
         // This is practically impossible (but could happen if there are strange
         // values used in the shipping stablity analysis). More temporary strands
         // are required for lifting than for shipping.
         //
         // At this point, we are going to abort the design. In the future
         // it we should provide some logic to add temporary strands and keep
         // the design going. This would mean a second pass through the shipping design
         // to refine the bunk point locations and final strength

         ATLASSERT(false); // need to add temporary strands
         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::GirderLiftingStability);
         m_DesignerOutcome.AbortDesign();
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

      LOG("--------------------------------------------------------------------------------------------------------------------");
      LOG("Attempt to reduce and lower harped strands for lifting condition. Use lifting points, or transfer lengths as controlling locations");

      // get controlling stress at xfer/lift point
      Float64 fbot, bot_loc, ftop, top_loc;
      artifact.GetEndZoneMinMaxRawStresses(&ftop, &fbot, &top_loc, &bot_loc);
      LOG("Max applied top stress at lifting point or transfer location    = " << ::ConvertFromSysUnits(ftop,unitMeasure::KSI) << " KSI at "<< ::ConvertFromSysUnits(top_loc,unitMeasure::Feet) << " ft");
      LOG("Max applied bottom stress at lifting point or transfer location = " << ::ConvertFromSysUnits(fbot,unitMeasure::KSI) << " KSI at "<< ::ConvertFromSysUnits(bot_loc,unitMeasure::Feet) << " ft");
      
      // get top and bottom stresses at harp points
      GET_IFACE(IStrandGeometry,pStrandGeom);
      Float64 lhp,rhp;
      pStrandGeom->GetHarpingPointLocations(span,gdr,&lhp,&rhp);
      std::vector<double> hpLocs;
      hpLocs.push_back(lhp);
      hpLocs.push_back(rhp);

      std::vector<double> fHpTopMin, fHpTopMax, fHpBotMin, fHpBotMax;
      artifact.GetGirderStress(hpLocs,true, true,fHpTopMin,fHpBotMin);
      artifact.GetGirderStress(hpLocs,false,true,fHpTopMax,fHpBotMax);

      double fTopHpMin = *std::min_element(fHpTopMin.begin(),fHpTopMin.end());
      double fBotHpMin = *std::min_element(fHpBotMin.begin(),fHpBotMin.end());
      double fTopHpMax = *std::max_element(fHpTopMax.begin(),fHpTopMax.end());
      double fBotHpMax = *std::max_element(fHpBotMax.begin(),fHpBotMax.end());
      double fHpMin = _cpp_min(fTopHpMin,fBotHpMin);
      double fHpMax = _cpp_max(fTopHpMax,fBotHpMax);

      LOG("Computing eccentricity required to make stress at lift/xfer point approx equal to stress at hp");

      pgsPointOfInterest tpoi(span,gdr,top_loc);
      pgsPointOfInterest bpoi(span,gdr,bot_loc);

      // Get the section properties of the girder
      Float64 Agt = pSectProp2->GetAg(pgsTypes::CastingYard,tpoi);
      Float64 Agb = pSectProp2->GetAg(pgsTypes::CastingYard,bpoi);
      Float64 Stg = pSectProp2->GetStGirder(pgsTypes::CastingYard, tpoi);
      Float64 Sbg = pSectProp2->GetSb(pgsTypes::CastingYard, bpoi);
      LOG("Agt = " << ::ConvertFromSysUnits(Agt, unitMeasure::Inch2) << " in^2");
      LOG("Agb = " << ::ConvertFromSysUnits(Agb, unitMeasure::Inch2) << " in^2");
      LOG("Stg = " << ::ConvertFromSysUnits(Stg, unitMeasure::Inch3) << " in^3");
      LOG("Sbg = " << ::ConvertFromSysUnits(Sbg, unitMeasure::Inch3) << " in^3");

      Float64 P_for_top = m_StrandDesignTool.GetPrestressForceAtLifting(config,tpoi);
      Float64 P_for_bot;
      if ( IsEqual(top_loc,bot_loc) )
         P_for_bot = P_for_top;
      else
         P_for_bot = m_StrandDesignTool.GetPrestressForceAtLifting(config,bpoi);

      
      LOG("Total Prestress Force for top location: P     = " << ::ConvertFromSysUnits(P_for_top, unitMeasure::Kip) << " kip");

      // ecc's required to match stresses at harp point
      Float64 ecc_tens = compute_required_eccentricity(P_for_top,Agt,Stg,ftop,fHpMax);
      LOG("Eccentricity Required to control Top Tension  = " << ::ConvertFromSysUnits(ecc_tens, unitMeasure::Inch) << " in");
      LOG("Total Prestress Force for bottom location: P          = " << ::ConvertFromSysUnits(P_for_bot, unitMeasure::Kip) << " kip");

      // Note that the "exact" way to do this would be to iterate on eccentricity because prestress force is dependent on strand
      // slope, which is dependent on end strand locations. But, so far, no problems????
      Float64 ecc_comp = compute_required_eccentricity(P_for_bot,Agb,Sbg,fbot,fHpMin);
      LOG("Eccentricity Required to control Bottom Compression   = " << ::ConvertFromSysUnits(ecc_comp, unitMeasure::Inch) << " in");

#if defined _DEBUG
      if( ecc_tens < ecc_comp)
         LOG("Tension Controls"); 
      else
         LOG("Compression Controls");
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
      LOG("Eccentricty for current number of strands = "<< ::ConvertFromSysUnits(curr_ecc, unitMeasure::Inch) << " in");
      if (curr_ecc <= required_eccentricity) // greater means the CG of prestress force must be lower in the section
      {
         if (m_StrandDesignTool.GetOriginalStrandFillType() == ftMinimizeHarping)
         {
            LOG("Try to increase end eccentricity by trading harped to straight");
            if (m_StrandDesignTool.ComputeMinHarpedForEzEccentricity(poi_control, required_eccentricity, pgsTypes::CastingYard, &ns_reqd, &nh_reqd))
            {
               // number of straight/harped were changed. Set them
               LOG("Number of Straight/Harped were changed from "<<Ns<<"/"<<Nh<<" to "<<ns_reqd<<"/"<<nh_reqd);
               m_StrandDesignTool.SetNumStraightHarped(ns_reqd, nh_reqd);

               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::RetainStrandProportioning);
            }
         }
         else
         {
            // See if we can lower end pattern
            Float64 offset_inc = pStrandGeom->GetHarpedEndOffsetIncrement(span,gdr);
            if (offset_inc>=0.0 )
            {
               LOG("Try to raise end eccentricity by lowering harped strands at ends");
               Float64 off_reqd = m_StrandDesignTool.ComputeEndOffsetForEccentricity(poi_control, required_eccentricity);

               // round to increment
               LOG("Harped End offset required to achieve controlling Eccentricity (raw)   = " << ::ConvertFromSysUnits(off_reqd, unitMeasure::Inch) << " in");
               off_reqd = CeilOff(off_reqd, offset_inc);
               LOG("Harped End offset required to achieve controlling Eccentricity (rounded)  = " << ::ConvertFromSysUnits(off_reqd, unitMeasure::Inch) << " in");

               // Attempt to set our offset, this may be lowered to the highest allowed location 
               // if it is out of bounds
               m_StrandDesignTool.SetHarpStrandOffsetEnd(off_reqd);

               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
               LOG("New Eccentricity  = " << ::ConvertFromSysUnits(m_StrandDesignTool.ComputeEccentricity(poi_control,pgsTypes::CastingYard), unitMeasure::Inch) << " in");
            }
         }
      } // end if - eccentricity

   } // end if - phase 1 design
   else
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
         LOG("There is no concrete strength that will work for lifting after shipping design - Tension controls - FAILED");
         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::GirderLiftingConcreteStrength);
         m_DesignerOutcome.AbortDesign();
         return; // bye
      }


      // we've got viable concrete strengths
      LOG("Lifting Results : New f'ci (unrounded) comp = " << ::ConvertFromSysUnits(fci_comp,unitMeasure::KSI) << " ksi, tension = " << ::ConvertFromSysUnits(fci_tens,unitMeasure::KSI) << " ksi" << " Pick Point = " << ::ConvertFromSysUnits(artifact.GetLeftOverhang(),unitMeasure::Feet) << " ft");

      ConcStrengthResultType rebar_reqd = (minRebarRequired) ? ConcSuccessWithRebar : ConcSuccess;

      // get the controlling value
      Float64 fci_required = max(fci_tens,fci_comp);

      // get the maximum allowable f'ci
      Float64 fci_max = m_StrandDesignTool.GetMaximumReleaseStrength();
      if( fci_max < fci_required)
      {
         // required strength is greater than max...
         // sometimes, if we are right at the limit the max value will work... give it a try

         LOG("f'ci max = " << ::ConvertFromSysUnits(fci_max,unitMeasure::KSI) << " KSI");
         LOG("f'ci cannot be greater than max. See if we can use max for one last attempt");

         Float64 fci_curr = m_StrandDesignTool.GetReleaseStrength();

         if (fci_curr != fci_max)
         {
            LOG("Set to max for one more attempt");
            fci_tens = min(fci_tens, fci_max);
            fci_comp = min(fci_comp, fci_max);
         }
         else
         {
            LOG("Fci max already used.There is no concrete strength that will work for lifting after shipping design - time to abort");
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
         LOG("f'ci has been updated");
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);
      }

      // check to see if f'c was changed also
      Float64 fc_new = m_StrandDesignTool.GetConcreteStrength();
      if (fc_old != fc_new)
      {
         LOG("However, Final Was Also Increased to " << ::ConvertFromSysUnits(fc_new,unitMeasure::KSI) << " KSI" );
         LOG("Restart design with new strengths");
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
      }
   } // end else - phase 2 design

   // always retain the strand proportiong after lifting design
   m_DesignerOutcome.SetOutcome(pgsDesignCodes::RetainStrandProportioning);
}

std::vector<Int16> pgsDesigner2::DesignForLiftingDebonding(bool bProportioningStrands, IProgress* pProgress)
{
   // If designConcrete is true, we want to set the release strength for our real design. If not,
   // the goal is to simply come up with a debonding layout that will work for the strength we compute
   // below. This layout will be used for the fabrication option when temporary strands are not used.

   pProgress->UpdateMessage("Lifting Design for Debonded Girders");

   std::vector<Int16> debond_demand;

   Float64 fci_current = m_StrandDesignTool.GetReleaseStrength();

   m_StrandDesignTool.DumpDesignParameters();

   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

   pgsGirderHandlingChecker checker(m_pBroker,m_AgentID);
   GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();

   if ( bProportioningStrands )
   {
      // if this is the first design for lifting, look at the lifting without temporary strands case
      // to get the optimum strand configuration
      LOG("Phase 1 Lifting Design - Design for Lifting without Temporary Strands");
      LOG("Determine debond strand layout");
      LOG("");
      LOG("Removing temporary strands for lifting analysis");
      config.Nstrands[pgsTypes::Temporary] = 0;
   }
#if defined _DEBUG
   else
   {
      LOG("Phase 2 Lifting Design - Design for Lifting with Temporary Strands");
      LOG("Determine lifting locations and release strength requirements");
   }
#endif

   pgsLiftingAnalysisArtifact artifact;
   // Designer manages it's own POIs
   IGirderLiftingDesignPointsOfInterest* pPoiLd = dynamic_cast<IGirderLiftingDesignPointsOfInterest*>(&m_StrandDesignTool);
   pgsDesignCodes::OutcomeType result = checker.DesignLifting(span,gdr,config,pPoiLd,&artifact,LOGGER);

//#if defined _DEBUG
//   LOG("-- Dump of Lifting Artifact After Design --");
//   artifact.Dump(LOGGER);
//   LOG("-- End Dump of Lifting Artifact --");
//#endif

   if ( pProgress->Continue() != S_OK )
      return debond_demand;

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
   LOG("FScr = " << FScr);
   LOG("");
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
      LOG("Cannot find a pick point to safisfy FScr");
      if (bProportioningStrands)
      {
         LOG("Temporary strands required");
         LOG("Move on to Shipping Design");
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::LiftingRedesignAfterShipping);
      }
      else
      {
         // Hauling design didn't help - crap out
         LOG("Unstable for lifting and any temporary strands added for hauling did not help - Design Failed" );
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
   LOG("Required Lifting Release Strength from artifact : f'ci (unrounded) tens = " << ::ConvertFromSysUnits(fci_tens,unitMeasure::KSI) << " KSI, compression = " << ::ConvertFromSysUnits(fci_comp,unitMeasure::KSI) << " KSI, Pick Point = " << ::ConvertFromSysUnits(artifact.GetLeftOverhang(),unitMeasure::Feet) << " ft");
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
            LOG("Release strength required for lifting is greater than our current max, and we have already tried max for design - Design Failed" );
            m_StrandDesignTool.SetOutcome(pgsDesignArtifact::GirderLiftingConcreteStrength);
            m_DesignerOutcome.AbortDesign();
            return debond_demand;
         }
         else
         {
            LOG("Strength required for lifting is greater than our current max of " << ::ConvertFromSysUnits(fci_max,unitMeasure::KSI) << " KSI - Try using max for one more go-around" );
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
            LOG("However, Final Was Also Increased to " << ::ConvertFromSysUnits(fc_new,unitMeasure::KSI) << " KSI" );
            LOG("May need to Restart design with new strengths");
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
            return debond_demand;
         }
         else
         {
            LOG("Release strength increased for lifting - design continues...");
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


std::vector<Int16> pgsDesigner2::DesignDebondingForLifting(HANDLINGCONFIG& liftConfig, IProgress* pProgress)
{
   pProgress->UpdateMessage("Designing initial debonding for Lifting");
   ATLASSERT(m_StrandDesignTool.GetFlexuralDesignType() == dtDesignForDebonding);

   // set up our vector to return debond levels at each section
   SectionIndexType max_db_sections = m_StrandDesignTool.GetMaxNumberOfDebondSections();
   std::vector<DebondLevelType> lifting_debond_levels;
   lifting_debond_levels.assign(max_db_sections,0);

   LOG("");
   LOG("Detailed Debond Design for Lifting");
   LOG("");
   m_StrandDesignTool.DumpDesignParameters();

   {
      SpanIndexType   span = m_StrandDesignTool.GetSpan();
      GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

      Float64 fc  = liftConfig.GdrConfig.Fc;
      Float64 fci = liftConfig.GdrConfig.Fci;
      LOG("current f'c  = " << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << " KSI ");
      LOG("current f'ci = " << ::ConvertFromSysUnits(fci,unitMeasure::KSI) << " KSI" );

      GET_IFACE(IGirderLiftingSpecCriteria,pLiftingCrit);
      Float64 all_tens = pLiftingCrit->GetLiftingAllowableTensileConcreteStressEx(fci,true);
      Float64 all_comp = pLiftingCrit->GetLiftingAllowableCompressiveConcreteStressEx(fci);
      LOG("Allowable tensile stress after Release     = " << ::ConvertFromSysUnits(all_tens,unitMeasure::KSI) << " KSI - min rebar was required for this strength");
      LOG("Allowable compressive stress after Release = " << ::ConvertFromSysUnits(all_comp,unitMeasure::KSI) << " KSI" );

      // This is an analysis to determine stresses that must be reduced by debonding
      LOG("Debond levels measured from fully bonded section");
      liftConfig.GdrConfig.Debond[pgsTypes::Straight].clear();

      pgsLiftingAnalysisArtifact artifact;

      pgsGirderHandlingChecker checker(m_pBroker,m_AgentID);
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
      Float64 xfer_length = m_StrandDesignTool.GetTransferLength();

      // Build stress demand
      std::vector<pgsStrandDesignTool::StressDemand> stress_demands;
      stress_demands.reserve(max_applied_stresses.size());
      LOG("--- Compute lifting stresses for debonding --- nss = "<<nss);
      for (pgsLiftingAnalysisArtifact::MaxLiftingStressIterator it=max_applied_stresses.begin(); it!=max_applied_stresses.end(); it++)
      {
         const pgsLiftingAnalysisArtifact::MaxdLiftingStresses& max_stresses = *it;

         Float64 poi_loc = max_stresses.m_LiftingPoi.GetDistFromStart();
         if(poi_loc <= lft_end || poi_loc >= rgt_end)
         {
            // get strand force if we haven't yet
            if (poi_loc >= xfer_length && force_per_strand==0.0)
            {
               force_per_strand = max_stresses.m_PrestressForce / nts;
               LOG("Sample prestress force per strand taken at "<< ::ConvertFromSysUnits(poi_loc,unitMeasure::Feet)<<" ft, force = " << ::ConvertFromSysUnits(force_per_strand, unitMeasure::Kip) << " kip");
            }

            Float64 fTop = max_stresses.m_TopMaxStress;
            Float64 fBot = max_stresses.m_BottomMinStress;

            LOG("At "<< ::ConvertFromSysUnits(poi_loc,unitMeasure::Feet)<<" ft, Ftop = "<< ::ConvertFromSysUnits(fTop,unitMeasure::KSI) << " ksi Fbot = "<< ::ConvertFromSysUnits(fBot,unitMeasure::KSI) << " ksi" );
            LOG("Average force per strand = " << ::ConvertFromSysUnits(max_stresses.m_PrestressForce / nss,unitMeasure::Kip) << " kip");

            pgsStrandDesignTool::StressDemand demand;
            demand.m_Location  = poi_loc;
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
         LOG("Debonding failed, this should not happen?");

         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::DebondDesignFailed);
         m_DesignerOutcome.AbortDesign();
      }
   }

   return lifting_debond_levels;
}



void pgsDesigner2::DesignForShipping(IProgress* pProgress)
{
   pProgress->UpdateMessage("Designing for Shipping");

   LOG("");
   LOG("DESIGNING FOR SHIPPING");
   LOG("");

   m_StrandDesignTool.DumpDesignParameters();

   Float64 fc_current = m_StrandDesignTool.GetConcreteStrength();

   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

   pgsGirderHandlingChecker checker(m_pBroker,m_AgentID);
   
   pgsHaulingAnalysisArtifact artifact;
   bool bResult = false;
   bool bTemporaryStrandsAdded = false;

   do
   {
      GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();

      IGirderHaulingDesignPointsOfInterest* pPoiLd = dynamic_cast<IGirderHaulingDesignPointsOfInterest*>(&m_StrandDesignTool);
      bResult = checker.DesignShipping(span,gdr,config,m_bShippingDesignWithEqualCantilevers,m_bShippingDesignIgnoreConfigurationLimits,pPoiLd,&artifact,LOGGER);

      // capture the results of the trial
      m_StrandDesignTool.SetTruckSupportLocations(artifact.GetTrailingOverhang(),artifact.GetLeadingOverhang());
      if (!bResult )
      {
         LOG("Adding temporary strands");
         if ( !m_StrandDesignTool.AddTempStrands() )
         {
            if ( !m_bShippingDesignWithEqualCantilevers )
            {
               LOG("Could not add temporary strands - go to equal cantilever method and continue");
               // the design isn't working with unequal cantilevers
               // start again with equal cantilevesr
               m_StrandDesignTool.SetNumTempStrands(0);
               m_bShippingDesignWithEqualCantilevers = true;
               m_bShippingDesignIgnoreConfigurationLimits = true;
               continue;
            }

            LOG("Could not add temporary strands - check to see if we are up against the geometric limits of the shipping configuration");
            GET_IFACE(IGirderHaulingSpecCriteria,pCriteria);
            Float64 maxDistanceBetweenSupports = pCriteria->GetAllowableDistanceBetweenSupports();
            Float64 maxLeadingOverhang = pCriteria->GetAllowableLeadingOverhang();
            Float64 distBetweenSupportPoints = m_StrandDesignTool.GetGirderLength() - artifact.GetTrailingOverhang() - artifact.GetLeadingOverhang();
            if ( IsEqual(maxLeadingOverhang,artifact.GetLeadingOverhang()) &&
                 IsEqual(maxDistanceBetweenSupports,distBetweenSupportPoints) )
            {
               LOG("Failed to satisfy shipping requirements - shipping configuration prevents a suitable solution from being found");
               m_StrandDesignTool.SetOutcome(pgsDesignArtifact::GirderShippingConfiguration);
               m_DesignerOutcome.AbortDesign();
               return;
            }

            LOG("Could not add temporary strands - attempt to bump concrete strength by 500psi");
            bool bSuccess = m_StrandDesignTool.Bump500(pgsTypes::Hauling, pgsTypes::ServiceI, pgsTypes::Tension, pgsTypes::TopGirder);
            if (bSuccess)
            {
               LOG("Concrete strength was increased for shipping - Restart" );
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
               return;
            }
            else
            {
               LOG("Failed to increase concrete strength and could not add temporary strands - abort");
               m_StrandDesignTool.SetOutcome(pgsDesignArtifact::GirderShippingStability);
               m_DesignerOutcome.AbortDesign();
               return;
            }
         }
         else
         {
            LOG("Temporary strands added. Restart design");
            bTemporaryStrandsAdded = true;
            m_DesignerOutcome.SetOutcome(pgsDesignCodes::TemporaryStrandsChanged);
            continue; // go back to top of loop and try again
         }
      }
   } while ( !bResult );


   if ( pProgress->Continue() != S_OK )
      return;

//#if defined _DEBUG
//   LOG("-- Dump of Hauling Artifact After Design --");
//   artifact.Dump(LOGGER);
//   LOG("-- End Dump of Hauling Artifact --");
//#endif

   // We now have bunk point locations to ensure stability
   m_DesignerOutcome.SetOutcome(pgsDesignCodes::HaulingConfigChanged);
   
   Float64 fc_max = m_StrandDesignTool.GetMaximumConcreteStrength();

   Float64 fc_tens, fc_comp;
   bool minRebarRequired;
   artifact.GetRequiredConcreteStrength(&fc_comp, &fc_tens, &minRebarRequired,fc_max,true);
   LOG("f'c (unrounded) required for shipping; tension = " << ::ConvertFromSysUnits(fc_tens,unitMeasure::KSI) << " KSI, compression = " << ::ConvertFromSysUnits(fc_comp,unitMeasure::KSI) << " KSI");

   if ( fc_tens < 0 )
   {
      // there isn't a concrete strength that will work (because of tension limit)

      // Add temporary strands and try again.
      LOG("There is no concrete strength that will work for shipping - Tension controls... Adding temporary strands");
      if ( !m_StrandDesignTool.AddTempStrands() )
      {
         LOG("Could not add temporary strands");
         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::GirderShippingConcreteStrength);
         m_DesignerOutcome.AbortDesign();
         return;
      }
      else
      {
         LOG("Temporary strands added. Restart design");
         m_DesignerOutcome.SetOutcome(pgsDesignCodes::TemporaryStrandsChanged);
         return;
      }
   }

   LOG("Shipping Results : f'c (unrounded) tens = " << ::ConvertFromSysUnits(fc_tens,unitMeasure::KSI) << " KSI, Comp = " << ::ConvertFromSysUnits(fc_comp,unitMeasure::KSI)<<"KSI, Left Bunk Point = " << ::ConvertFromSysUnits(artifact.GetTrailingOverhang(),unitMeasure::Feet) << " ft" << "    Right Bunk Point = " << ::ConvertFromSysUnits(artifact.GetLeadingOverhang(),unitMeasure::Feet) << " ft");

   if ( pProgress->Continue() != S_OK )
      return;

   if (fc_tens>fc_max || fc_comp>fc_max )
   {
      // strength needed is more than max allowed. Try setting to max for one more design go-around
      Float64 fc_curr = m_StrandDesignTool.GetConcreteStrength();
      if (fc_max==fc_curr)
      {
         LOG("Strength required for shipping is greater than our current max, and we have already tried max for design - Design Failed" );
         m_StrandDesignTool.SetOutcome(pgsDesignArtifact::GirderShippingConcreteStrength);
         m_DesignerOutcome.AbortDesign();
         return;
      }
      else
      {
         LOG("Strength required for shipping is greater than our current max of " << ::ConvertFromSysUnits(fc_max,unitMeasure::KSI) << " KSI - Try using max for one more go-around" );
         if (fc_comp>fc_max)
            fc_comp = fc_max;

         if (fc_tens>fc_max)
            fc_tens = fc_max;
      }
   }

   if ( pProgress->Continue() != S_OK )
      return;

   // NOTE: Using bogus stress location
   bool bFcUpdated = m_StrandDesignTool.UpdateConcreteStrength(fc_tens,pgsTypes::Hauling,pgsTypes::ServiceI,pgsTypes::Tension,pgsTypes::TopGirder);
   bFcUpdated |= m_StrandDesignTool.UpdateConcreteStrength(fc_comp,pgsTypes::Hauling,pgsTypes::ServiceI,pgsTypes::Compression,pgsTypes::BottomGirder);
   if ( bFcUpdated )
   {
      LOG("Concrete strength was increased for shipping - Restart" );
      m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
      return;
   }

   if ( bTemporaryStrandsAdded )
   {
      LOG("Temporary strands were added for shipping - Restart" );
      m_DesignerOutcome.SetOutcome(pgsDesignCodes::TemporaryStrandsChanged);
      return;
   }

   LOG("Shipping Design Complete - Continue design" );
}


std::vector<Int16> pgsDesigner2::DesignForShippingDebondingFinal(IProgress* pProgress)
{
   pProgress->UpdateMessage("Designing final debonding for Shipping");

   // fine-tune debonding for shipping
   SectionIndexType max_db_sections = m_StrandDesignTool.GetMaxNumberOfDebondSections();

   // set up our vector to return debond levels at each section
   std::vector<Int16> shipping_debond_levels;
   shipping_debond_levels.assign(max_db_sections,0);

   Float64 fci_current = m_StrandDesignTool.GetReleaseStrength();

   LOG("");
   LOG("DESIGNING DEBONDING FOR SHIPPING");
   LOG("");
   m_StrandDesignTool.DumpDesignParameters();

   if (m_StrandDesignTool.GetFlexuralDesignType() == dtDesignForDebonding)
   {
      SpanIndexType   span = m_StrandDesignTool.GetSpan();
      GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

      Float64 fc  = m_StrandDesignTool.GetConcreteStrength();
      ConcStrengthResultType rebar_reqd;
      Float64 fci = m_StrandDesignTool.GetReleaseStrength(&rebar_reqd);
      LOG("current f'c  = " << ::ConvertFromSysUnits(fc,unitMeasure::KSI) << " KSI ");
      LOG("current f'ci = " << ::ConvertFromSysUnits(fci,unitMeasure::KSI) << " KSI" );

      GET_IFACE(IGirderHaulingSpecCriteria,pHaulingCrit);
      Float64 all_tens = pHaulingCrit->GetHaulingAllowableTensileConcreteStressEx(fc,rebar_reqd==ConcSuccessWithRebar);
      Float64 all_comp = pHaulingCrit->GetHaulingAllowableCompressiveConcreteStressEx(fc);
      LOG("Allowable tensile stress at hauling     = " << ::ConvertFromSysUnits(all_tens,unitMeasure::KSI) << " KSI"<<(rebar_reqd==ConcSuccessWithRebar ? " min rebar was required for this strength":"")  );
      LOG("Allowable compressive stress at hauling = " << ::ConvertFromSysUnits(all_comp,unitMeasure::KSI) << " KSI" );

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

      pgsGirderHandlingChecker checker(m_pBroker,m_AgentID);
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
      Float64 xfer_length = m_StrandDesignTool.GetTransferLength();

      // Build stress demand
      std::vector<pgsStrandDesignTool::StressDemand> stress_demands;
      stress_demands.reserve(max_applied_stresses.size());

      LOG("--- Compute hauling stresses for debonding --- nss = "<<nss);
      GET_IFACE(IPrestressStresses, pPrestress);
      for (pgsHaulingAnalysisArtifact::MaxHaulingStressIterator it=max_applied_stresses.begin(); it!=max_applied_stresses.end(); it++)
      {
         if ( pProgress->Continue() != S_OK )
            return shipping_debond_levels;

         const pgsHaulingAnalysisArtifact::MaxdHaulingStresses& max_stresses = *it;

         Float64 poi_loc = max_stresses.m_HaulingPoi.GetDistFromStart();
         if(poi_loc <= lft_end || rgt_end <= poi_loc)
         {
            // get strand force if we haven't yet
            if (xfer_length <= poi_loc && force_per_strand == 0.0)
            {
               force_per_strand = max_stresses.m_PrestressForce / nts;
               LOG("Sample prestress force per strand taken at "<< ::ConvertFromSysUnits(poi_loc,unitMeasure::Feet)<<" ft, force = " << ::ConvertFromSysUnits(force_per_strand, unitMeasure::Kip) << " kip");
            }

            Float64 fTop = max_stresses.m_TopMaxStress;
            Float64 fBot = max_stresses.m_BottomMinStress;

            LOG("At "<< ::ConvertFromSysUnits(poi_loc,unitMeasure::Feet)<<" ft, Ftop = "<< ::ConvertFromSysUnits(fTop,unitMeasure::KSI) << " ksi Fbot = "<< ::ConvertFromSysUnits(fBot,unitMeasure::KSI) << " ksi" );
            LOG("Average force per strand = " << ::ConvertFromSysUnits(max_stresses.m_PrestressForce / nss,unitMeasure::Kip) << " kip");

            pgsStrandDesignTool::StressDemand demand;
            demand.m_Location  = poi_loc;
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
         LOG("Debonding failed, this should not happen?");

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
      if ( task.stage == pgsTypes::TemporaryStrandRemoval && (0 == NtMax || 0 == m_StrandDesignTool.GetNt()) )
         continue; // skip temporary strand removal if this girder doesn't support temporary strands

      LOG("");
      LOG("*** Refining design for " << g_Stage[task.stage] << " " << g_LimitState[task.ls] << " " << g_Type[task.type] );

      RefineDesignForAllowableStress(task,pProgress);

      if (m_DesignerOutcome.WasDesignAborted() || pProgress->Continue() != S_OK )
      {
         return;
      }
      else if (m_DesignerOutcome.DidConcreteChange())
      {
         LOG("An allowable stress check failed - Restart design with new concrete strength");
         LOG("============================================================================");
         return;
      }
   }

   LOG("**** Successfully completed allowable stress design");
}

void pgsDesigner2::RefineDesignForAllowableStress(ALLOWSTRESSCHECKTASK task,IProgress* pProgress)
{
   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

   Float64 fcgdr;
   GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();
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
   GET_IFACE(ISpecification, pSpec);

   LOG("");
   LOG("Begin Design Refinement Iterations");
   m_StrandDesignTool.DumpDesignParameters();

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   Float64 start_end_size = (task.stage==pgsTypes::CastingYard)? 0.0 : pBridge->GetGirderStartConnectionLength(span,gdr);

   //
   // Get the allowable stresses
   //
   Float64 fAllow;
   fAllow = pAllowable->GetAllowableStress(task.stage,task.ls,task.type,fcgdr);
   LOG("Allowable stress = " << ::ConvertFromSysUnits(fAllow,unitMeasure::KSI) << " KSI");

   bool adj_strength = false; // true if we need to increase strength
   Float64 fControl = task.type==pgsTypes::Tension ? -Float64_Max :  Float64_Max;  // controlling stress for all pois
   pgsTypes::StressLocation stress_location;

   BridgeAnalysisType bat;
   if ( analysisType == pgsTypes::Simple )
      bat = SimpleSpan;
   else if ( analysisType == pgsTypes::Continuous )
      bat = ContinuousSpan;
   else
      bat = MaxSimpleContinuousEnvelope;

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

   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = vPoi.begin(); iter < vPoi.end(); iter++ )
   {
      if ( pProgress->Continue() != S_OK )
         return;

      const pgsPointOfInterest& poi = *iter;

      LOG("Designing at " << ::ConvertFromSysUnits(poi.GetDistFromStart() - start_end_size,unitMeasure::Feet) << " ft");

      //
      // Get the stresses due to externally applied loads
      //
      Float64 fTopMinExt, fTopMaxExt;
      Float64 fBotMinExt, fBotMaxExt;
      pLimitStateForces->GetDesignStress(task.ls,task.stage,poi,pgsTypes::TopGirder,   fcgdr,startSlabOffset,endSlabOffset,bat,&fTopMinExt,&fTopMaxExt);
      pLimitStateForces->GetDesignStress(task.ls,task.stage,poi,pgsTypes::BottomGirder,fcgdr,startSlabOffset,endSlabOffset,bat,&fBotMinExt,&fBotMaxExt);

      LOG("Max External Stress  :: Top = " << ::ConvertFromSysUnits(fTopMaxExt,unitMeasure::KSI) << " KSI" << "    Bot = " << ::ConvertFromSysUnits(fBotMaxExt,unitMeasure::KSI) << " KSI");
      LOG("Min External Stress  :: Top = " << ::ConvertFromSysUnits(fTopMinExt,unitMeasure::KSI) << " KSI" << "    Bot = " << ::ConvertFromSysUnits(fBotMinExt,unitMeasure::KSI) << " KSI");

      //
      // Get the stresses due to prestressing (adjust for losses)
      //
      Float64 fTopPre = pPsStress->GetDesignStress(task.stage,poi,pgsTypes::TopGirder,config);
      Float64 fBotPre = pPsStress->GetDesignStress(task.stage,poi,pgsTypes::BottomGirder,config);
      LOG("Prestress Stress     :: Top = " << ::ConvertFromSysUnits(fTopPre,unitMeasure::KSI) << " KSI" << "    Bot = " << ::ConvertFromSysUnits(fBotPre,unitMeasure::KSI) << " KSI");

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

      LOG("Max Resultant Stress :: Top = " << ::ConvertFromSysUnits(fTopMax,unitMeasure::KSI) << " KSI" << "    Bot = " << ::ConvertFromSysUnits(fBotMax,unitMeasure::KSI) << " KSI");
      LOG("Min Resultant Stress :: Top = " << ::ConvertFromSysUnits(fTopMin,unitMeasure::KSI) << " KSI" << "    Bot = " << ::ConvertFromSysUnits(fBotMin,unitMeasure::KSI) << " KSI");

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
               LOG("** Failed in tension at top of girder");
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
               LOG("** Failed in tension at bottom of girder");
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
               LOG( "** Failed in compression at the bottom of the girder" );

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
               LOG( "** Failed in compression at the top of the girder" );

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
      LOG("** Need to increase concrete strength. Controlling stress is "<<::ConvertFromSysUnits(fControl,unitMeasure::KSI) << " KSI");

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

      LOG("");

   }
}


void pgsDesigner2::RefineDesignForUltimateMoment(pgsTypes::Stage stage,pgsTypes::LimitState ls,IProgress* pProgress)
{
   SpanIndexType span = m_StrandDesignTool.GetSpan();
   GirderIndexType gdr  = m_StrandDesignTool.GetGirder();

   std::vector<pgsPointOfInterest> vPoi = m_StrandDesignTool.GetDesignPoi(stage,POI_FLEXURECAPACITY);
   
   GET_IFACE(IBridge,pBridge);
   Float64 start_end_size = (stage==pgsTypes::CastingYard)? 0.0 : pBridge->GetGirderStartConnectionLength(span,gdr);

   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      if ( pProgress->Continue() != S_OK )
         return;

      const pgsPointOfInterest& poi = *iter;

      LOG("");
      LOG("=======================================================================================================");
      LOG("Designing at " << ::ConvertFromSysUnits(poi.GetDistFromStart() - start_end_size,unitMeasure::Feet) << " ft");

      m_StrandDesignTool.DumpDesignParameters();

      GDRCONFIG config = m_StrandDesignTool.GetGirderConfiguration();

      pgsFlexuralCapacityArtifact cap_artifact = CreateFlexuralCapacityArtifact(poi,stage,ls,config,true); // positive moment

      LOG("Capacity (pMn) = " << ::ConvertFromSysUnits(cap_artifact.GetCapacity(),unitMeasure::KipFeet) << " k-ft" << "   Demand (Mu) = " << ::ConvertFromSysUnits(cap_artifact.GetDemand(),unitMeasure::KipFeet) << " k-ft");
      LOG("Max Reinf Ratio (c/de) = " << cap_artifact.GetMaxReinforcementRatio() << "   Max Reinf Ratio Limit = " << cap_artifact.GetMaxReinforcementRatioLimit());
      LOG("Capacity (pMn) = " << ::ConvertFromSysUnits(cap_artifact.GetCapacity(),unitMeasure::KipFeet) << " k-ft" << "   Min Capacity (pMn Min: Lessor of 1.2Mcr and 1.33Mu) = " << ::ConvertFromSysUnits(cap_artifact.GetMinCapacity(),unitMeasure::KipFeet) << " k-ft");

#if defined ENABLE_LOGGING
      GET_IFACE(IMomentCapacity, pMomentCapacity);

      MOMENTCAPACITYDETAILS mcd;
      pMomentCapacity->GetMomentCapacityDetails( stage, poi, config, true, &mcd );

      LOG("fpe = " << ::ConvertFromSysUnits( mcd.fpe, unitMeasure::KSI) << " KSI" );
      LOG("fps = " << ::ConvertFromSysUnits( mcd.fps, unitMeasure::KSI) << " KSI" );
      LOG("e initial = " << mcd.e_initial );
      LOG("phi = " << mcd.Phi );
      LOG("C = " << ::ConvertFromSysUnits( mcd.C, unitMeasure::Kip) << " kip");
      LOG("dc = " << ::ConvertFromSysUnits( mcd.dc, unitMeasure::Inch) << " inch");
      LOG("de = " << ::ConvertFromSysUnits( mcd.de, unitMeasure::Inch) << " inch");
      LOG("dt = " << ::ConvertFromSysUnits( mcd.dt, unitMeasure::Inch) << " inch");
      LOG("Moment Arm = " << ::ConvertFromSysUnits( mcd.MomentArm, unitMeasure::Inch) << " inch");

      GET_IFACE(ILosses,pILosses);
      Float64 check_loss = pILosses->GetFinal(poi,pgsTypes::Permanent,config);
      LOG("Losses = " << ::ConvertFromSysUnits( check_loss, unitMeasure::KSI) << " KSI" );

      CRACKINGMOMENTDETAILS cmd;
      pMomentCapacity->GetCrackingMomentDetails(stage, poi, config, true, &cmd);
      LOG("Mcr = " << ::ConvertFromSysUnits(cmd.Mcr,unitMeasure::KipFeet) << " k-ft");
      LOG("Mdnc = "<< ::ConvertFromSysUnits(cmd.Mdnc,unitMeasure::KipFeet) << " k-ft");
      LOG("fcpe = " << ::ConvertFromSysUnits( cmd.fcpe, unitMeasure::KSI) << " KSI" );
      LOG("fr = " << ::ConvertFromSysUnits( cmd.fr, unitMeasure::KSI) << " KSI" );
      LOG("Sb = " << ::ConvertFromSysUnits( cmd.Sb, unitMeasure::Inch3) << " in^3");
      LOG("Sbc = " << ::ConvertFromSysUnits( cmd.Sbc, unitMeasure::Inch3) << " in^3");
      LOG("Mcr Limit = " << ::ConvertFromSysUnits(cmd.McrLimit,unitMeasure::KipFeet) << " k-ft");

#endif // ENABLE_LOGGING

      if ( !cap_artifact.Passed() )
      {
         // Check Ultimate Capacity
         Float64 capacity = cap_artifact.GetCapacity();
         Float64 demand  = cap_artifact.GetDemand();
         if ( capacity < demand )
         {
            LOG("** Ultimate Flexural Capacity Artifact failed at "<< ::ConvertFromSysUnits(poi.GetDistFromStart() , unitMeasure::Feet) << " ft. Attempt to add strands");
            StrandIndexType curr_strands = m_StrandDesignTool.GetNumPermanentStrands();
            StrandIndexType max_strands = m_StrandDesignTool.GetMaxPermanentStrands();

            bool success=false;
            if (curr_strands >= max_strands)
            {
               LOG("Number of strands already max - we can't add any more");
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
                  LOG("Used demand/capacity ratio of "<<(demand/capacity)<<" to get a new number of strands = "<<new_num);
               }
               else
               {
                  LOG("Use max number of strands to alleviate ultimate moment "<<new_num);
                  new_num = max_strands;
               }

               success = m_StrandDesignTool.SetNumPermanentStrands(new_num);
            }

            if ( !success )
            {
               LOG("Attempt to add strands failed, Try bumping concrete strength by 500psi");
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
                  LOG("Just threw a Hail Mary - Restart design with much higher concrete strength");
                  m_DesignerOutcome.SetOutcome(pgsDesignCodes::ChangedForUltimate);
                  m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);
                  m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
                  break;
               }
               else
               {
                  LOG("Concrete Strength Cannot be adjusted");
                  m_StrandDesignTool.SetOutcome(pgsDesignArtifact::UltimateMomentCapacity);
                  m_DesignerOutcome.AbortDesign();
                  return;
               }
            }
            else
            {
               LOG("Attempt to add strands succeeded NP = " << m_StrandDesignTool.GetNumPermanentStrands());
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::ChangedForUltimate);
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);

               // 
               LOG("Compute new capacity to see if we are increasing. If not, we need another strategy");
               GDRCONFIG new_config = m_StrandDesignTool.GetGirderConfiguration();
               pgsFlexuralCapacityArtifact new_cap_artifact = CreateFlexuralCapacityArtifact(poi,stage,ls,new_config,true); // positive moment
               Float64 new_capacity = new_cap_artifact.GetCapacity();
               LOG("New Capacity = " << ::ConvertFromSysUnits(new_capacity,unitMeasure::KipFeet) << " k-ft");

               if (new_capacity < capacity)
               {
                  LOG("We added strands and the capacity did not increase - reduce strands back to original and try bumping concrete strength");
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
                     LOG("Attempt to bump concrete strength failed - we're probably toast at this point, but keep trying to add strands");
                     m_StrandDesignTool.SetOutcome(pgsDesignArtifact::UltimateMomentCapacity);
                     m_DesignerOutcome.AbortDesign();
                  }
               }


               iter = vPoi.begin();
               continue;
            }
         }

         // Check Maximum Reinforcement
         if ( cap_artifact.GetMaxReinforcementRatio() > cap_artifact.GetMaxReinforcementRatioLimit() )
         {
            // No adjustment to be made. Use a bigger section
            LOG("Capacity Artifact failed for max reinforcement ratio - section overreinforced "<< ::ConvertFromSysUnits(poi.GetDistFromStart() , unitMeasure::Feet) << " ft");
            LOG("All we can do here is attempt to bump concrete strength by 500psi");
            bool bSuccess = m_StrandDesignTool.Bump500(stage, ls, pgsTypes::Tension, pgsTypes::BottomGirder);
            if (bSuccess)
            {
               LOG("Concrete strength was increased for section overreinforced case - Restart" );
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::ChangedForUltimate);
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::FciChanged);
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::FcChanged);
               return;
            }
            else
            {
               LOG("Failed to increase concrete strength, cannot remove strands - Failed due to overreinforcement - abort");
               m_StrandDesignTool.SetOutcome(pgsDesignArtifact::OverReinforced);
               m_DesignerOutcome.AbortDesign();
               return;
            }
         }

         // Check Minimum Reinforcement
         if ( cap_artifact.GetCapacity() < cap_artifact.GetMinCapacity() )
         {
           LOG("Min Reinforcement for Flexural Capacity Artifact failed at "<< ::ConvertFromSysUnits(poi.GetDistFromStart() , unitMeasure::Feet) << " ft");

           if ( !m_StrandDesignTool.AddStrands() )
           {
              LOG("Attempt to add strands failed");
              m_StrandDesignTool.SetOutcome(pgsDesignArtifact::UnderReinforced);
              m_DesignerOutcome.AbortDesign();
              return;
           }
           else
           {
               LOG("Attempt to add strands succeeded NP = " << m_StrandDesignTool.GetNumPermanentStrands());
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::ChangedForUltimate);
               m_DesignerOutcome.SetOutcome(pgsDesignCodes::PermanentStrandsChanged);
               iter = vPoi.begin();
               continue;
           }
         }
      }
   }

   if (m_DesignerOutcome.GetOutcome(pgsDesignCodes::ChangedForUltimate) )
   {
      // set minimum number of strands for next design iteration
      StrandIndexType min_strands = m_StrandDesignTool.GetNumPermanentStrands();
      LOG("Minimum number of strands set to control capacity = "<<min_strands);
      m_StrandDesignTool.SetMinimumPermanentStrands(min_strands);
   }
}


// Stirrup Design

void pgsDesigner2::RefineDesignForStirrups(pgsTypes::Stage stage,
                                           pgsTypes::LimitState ls,
                                           pgsDesignArtifact* pArtifact)
{
   LOG("");
   LOG("**************************************************************");
   LOG("**************************************************************");
   LOG("");
   LOG("Refining Design for Stirrups");
   LOG("");

   ATLASSERT( stage == pgsTypes::BridgeSite3 ); // better be design for this stage

   SpanIndexType span = pArtifact->GetSpan();
   GirderIndexType gdr  = pArtifact->GetGirder();
   GDRCONFIG config = pArtifact->GetGirderConfiguration();

   GET_IFACE(IBridge,pBridge);
   Float64 end_size = pBridge->GetGirderStartConnectionLength( span, gdr );

   // get critical sections for shear (not dependent on stirrups, so we're ok here).
   GET_IFACE(IPointOfInterest,pIPOI);
   pgsPointOfInterest left_cs;
   pgsPointOfInterest right_cs;
   pIPOI->GetCriticalSection(ls,span,gdr,config,&left_cs,&right_cs);

   LOG("Critical sections at " << ::ConvertFromSysUnits(left_cs.GetDistFromStart()-end_size,unitMeasure::Feet) << " ft and " << ::ConvertFromSysUnits(right_cs.GetDistFromStart()-end_size,unitMeasure::Feet) << " ft");

   // first see if it's even worth trying to design for this girder

   // first pass for shear design is to
   // loop over all shear pois and design Av/S at all locations
   // first need a vector to hold Av/S values
   std::vector<pgsPointOfInterest> vPoi = m_StrandDesignTool.GetDesignPoi(stage,POI_SHEAR);

   std::vector<ShearDesignAvs> avs_reqd_vec;

   LOG("Starting CalcAvSAtPois");
   CalcAvSAtPois(stage, ls, span, gdr, pArtifact, left_cs, right_cs, vPoi, &avs_reqd_vec);
   if (m_DesignerOutcome.WasDesignAborted())
      return;

   LOG("Starting DetailStirrupZones");
   DetailStirrupZones(stage, ls, span, gdr, pArtifact, left_cs, right_cs, vPoi, avs_reqd_vec);
   if (m_DesignerOutcome.WasDesignAborted())
      return;
}


void pgsDesigner2::DetailStirrupZones(pgsTypes::Stage stage,
                                       pgsTypes::LimitState ls,
                                       SpanIndexType span,
                                       GirderIndexType gdr,
                                       pgsDesignArtifact* pArtifact,
                                       const pgsPointOfInterest& leftCs,
                                       const pgsPointOfInterest& rightCs,
                                       const std::vector<pgsPointOfInterest>& vPoi, 
                                       const std::vector<ShearDesignAvs>& avsReqdAtPois)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(ITransverseReinforcementSpec,pTransverseReinforcementSpec);
   GET_IFACE(IGirder,pGdr);

   // get some basic information
   Float64 gird_length = pBridge->GetGirderLength(span,gdr);
   Float64 gird_length2 = gird_length/2.0;
#pragma Reminder("Assuming a constant depth girder")
   Float64 h = pGdr->GetHeight( pgsPointOfInterest(span,gdr,0.00) );

   // create an envelope of the horizontal Av/S and vertical Av/s for pois
   LOG("Av/S Envelope - loc, max_avs");
   mathPwLinearFunction2dUsingPoints avs_envelope;
   CHECK(vPoi.size()==avsReqdAtPois.size());
   Uint32 npoi = vPoi.size();
   Uint32 i = 0;
   for(i=0; i<npoi; i++)
   {
      Float64 maxavs = max(avsReqdAtPois[i].VerticalAvS, avsReqdAtPois[i].HorizontalAvS);
      gpPoint2d pnt(vPoi[i].GetDistFromStart(), maxavs);
      avs_envelope.AddPoint(pnt);
      LOG(pnt.X()<<", "<<pnt.Y());
   }

   // get max connection length on girder
   Float64 start_size = pBridge->GetGirderStartConnectionLength( span, gdr );
   Float64 end_size = pBridge->GetGirderEndConnectionLength( span, gdr );
   Float64 lend = max(start_size,end_size);

   // Get Av/S req'd at strategic locations
   // Critical section
   Float64 avs_leftcs  = avs_envelope.Evaluate(leftCs.GetDistFromStart());
   Float64 avs_rightcs = avs_envelope.Evaluate(rightCs.GetDistFromStart());
   Float64 l_cs;   // location of cs
   Float64 avs_cs; // avs at cs
   // locate critical section where max avs occurs
   if (avs_leftcs>avs_rightcs)
   {
      avs_cs = avs_leftcs;
      l_cs = leftCs.GetDistFromStart();
   }
   else
   {
      avs_cs = avs_rightcs;
      l_cs = gird_length-rightCs.GetDistFromStart();
   }

   // Determine avs req'd at 1.5h from support
   Float64 d_1p5 = lend + 1.5*h;
   Float64 avs_left_1p5  = avs_envelope.Evaluate(d_1p5);
   Float64 avs_right_1p5 = avs_envelope.Evaluate(gird_length-d_1p5);
   Float64 avs_1p5 = max(avs_left_1p5,avs_right_1p5);

   // avs req'd for Splitting and confinement
   Float64 avs_burst   = CalcAvsForSplittingZone(span, gdr,*pArtifact);
   Float64 avs_confine = CalcAvsForConfinement(span, gdr);

   // now we can start designing zones
   CShearZoneData zone_data[MAX_ZONES];
   Float64 zone_start=0.0, zone_end=0.0;
   BarSizeType bar_size;
   Float64 bar_spacing, zone_length;

   // confinement steel lives in zones 1 and 2, so it will control spacing requirements
   Float64 s_confine = pTransverseReinforcementSpec->GetMaxConfinmentBarSpacing();
   pArtifact->SetConfinementBarSize(pTransverseReinforcementSpec->GetMinConfinmentBarSize());
   pArtifact->SetLastConfinementZone(1); // always have two zones

   // zone 1 - Splitting zone
   Float64 lbz = pTransverseReinforcementSpec->GetSplittingZoneLength(h);
   zone_end = _cpp_max(lbz,lend);
//   if (lbz<lend || avs_burst<=avs_cs)
//      // Splitting zone ends before support, or does not control into span - extend to support
//      zone_end = lend;
//   else
//      // Splitting zone extends into span
//      zone_end = lbz;

   if (!GetStirrupsForAvs(span, gdr, avs_burst, s_confine, &bar_size, &bar_spacing))
   {
      pArtifact->SetOutcome(pgsDesignArtifact::TooManyStirrupsReqd);
      m_DesignerOutcome.AbortDesign();
      return;
   }

   // make sure bar spacing does not exceed zone length
   zone_length = zone_end-zone_start;
   ATLASSERT(0 < zone_length);
   if (zone_length < bar_spacing)
      bar_spacing = zone_length;

   // save off zone 1 data
   Uint16 num_zones = 1;
   Uint16 zi = 0;

   zone_data[zi].ZoneNum = num_zones;
   zone_data[zi].VertBarSize = bar_size;
   zone_data[zi].ZoneLength = zone_length;
   zone_data[zi].BarSpacing = bar_spacing;
   pArtifact->SetNumberOfStirrupZonesDesigned(num_zones);
   pArtifact->SetShearZoneData(zi, zone_data[zi]);

   // zone 2 - from support (or end of burst zone) to 1.5h
   // (or, if min stirrups req'd, this zone ends at mid-span and is last zone)
   zone_start = zone_end;
   ATLASSERT(zone_start < d_1p5); // Splitting zone should never extend this far
   ATLASSERT(l_cs < d_1p5);      // fire this out of curiousity - probably harmless, but shouldn't happen
   bool zone2_ended_at_cs;
   if (l_cs < d_1p5)
   {
      zone_end = d_1p5;
      zone2_ended_at_cs = false;
   }
   else
   {
      zone_end = l_cs;
      zone2_ended_at_cs = true;
   }

   if (!GetStirrupsForAvs(span, gdr, avs_cs, s_confine, &bar_size, &bar_spacing))
   {
      pArtifact->SetOutcome(pgsDesignArtifact::TooManyStirrupsReqd);
      m_DesignerOutcome.AbortDesign();
      return;
   }

   // figure out if we have a zone 3 - use a min zone length of a meter
   const Float64 MIN_ZONE_LENGTH=::ConvertToSysUnits(1.0,unitMeasure::Meter);
   // return if we've hit mid-girder
   bool is_zone3 = true;
   if (zone_end>=gird_length2-MIN_ZONE_LENGTH)
   {
      zone_end = gird_length2;
      is_zone3 = false;
   }

   // make sure bar spacing does not exceed zone length
   zone_length = zone_end-zone_start;
   if (bar_spacing>zone_length)
      bar_spacing = zone_length;

   // save off zone 2 data
   num_zones =2;
   zi = 1;
   zone_data[zi].ZoneNum = num_zones;
   zone_data[zi].VertBarSize = bar_size;
   zone_data[zi].BarSpacing = bar_spacing;
   zone_data[zi].ZoneLength = zone_length;

   // this zone cannot be collapsed because it is the confinement zone
   pArtifact->SetNumberOfStirrupZonesDesigned(num_zones);
   pArtifact->SetShearZoneData(zi,zone_data[zi]);

   if (!is_zone3)
      return;

   // zone 3 - goes to where max stirrup spacing requirement starts
   // must first determine that location
   // create a function of vu along girder and find intersections where vu = 0.1*bv*dv*f'c
   mathPwLinearFunction2dUsingPoints vu_funct;
   mathPwLinearFunction2dUsingPoints limit_funct;
   for(i=0; i<npoi; i++)
   {
      gpPoint2d vu_pnt(vPoi[i].GetDistFromStart(), avsReqdAtPois[i].Vu);
      vu_funct.AddPoint(vu_pnt);

      gpPoint2d limit_pnt(vPoi[i].GetDistFromStart(), avsReqdAtPois[i].Pt1FcBvDv);
      limit_funct.AddPoint(limit_pnt);
   }
   math1dRange vu_range = vu_funct.GetRange();
   bool is_zone4=true;
   gpPoint2d left_max_loc, right_max_loc;
   Float64 max_spacing_loc;
   // can potentially have two intersections - one on each side of mid-girder
   // both must exist before zone 4 can exist
   math1dRange left_range(vu_range.GetLeftBoundLocation(),math1dRange::Bound,gird_length2,math1dRange::Bound);
   if (vu_funct.Intersect(limit_funct, left_range, &left_max_loc)==1)
   {
      math1dRange right_range(gird_length2,math1dRange::Bound,vu_range.GetRightBoundLocation(),math1dRange::Bound);
      if (vu_funct.Intersect(limit_funct, right_range, &right_max_loc)==1)
      {
         max_spacing_loc = min(left_max_loc.X(),gird_length-right_max_loc.X());
      }
      else
         is_zone4=false;
   }
   else
      is_zone4=false;

   // Stirrup spacing in zone 3
   Float64 s_under_limit, s_over_limit;
   pTransverseReinforcementSpec->GetMaxStirrupSpacing(&s_under_limit,&s_over_limit);

   Float64 s_zone3 = s_over_limit;

   // start and end of zone 3
   zone_start = zone_end;
   if (is_zone4)
   {
      if (max_spacing_loc <= zone_end)
      {
         // it's possible that max stirrup spacing can happen before start of zone 3
         zone_end = gird_length2;
         s_zone3 = s_under_limit;
         is_zone4 = false;
      }
      else if (max_spacing_loc <= gird_length2-MIN_ZONE_LENGTH)
      {
         zone_end = max_spacing_loc;
      }
      else
      {
         zone_end = gird_length2;
         is_zone4 = false;
      }
   }
   else
      zone_end = gird_length2;

   // It is possible that zone 3tarted at the cs, 
   // make sure we use the right avs if this is the case
   Float64 avs_zone3 = avs_1p5;
   if (zone2_ended_at_cs)
      avs_zone3 = avs_cs;

   if (!GetStirrupsForAvs(span, gdr, avs_zone3, s_zone3, &bar_size, &bar_spacing))
   {
      pArtifact->SetOutcome(pgsDesignArtifact::TooManyStirrupsReqd);
      m_DesignerOutcome.AbortDesign();
      return;
   }

   // make sure bar spacing does not exceed zone length
   zone_length = zone_end-zone_start;
   CHECK(zone_length>0.0);

   if (bar_spacing>zone_length)
      bar_spacing = zone_length;

   // save off zone 3 data
   num_zones++;
   zi = num_zones-1;
   zone_data[zi].ZoneNum = num_zones;
   zone_data[zi].VertBarSize = bar_size;
   zone_data[zi].BarSpacing = bar_spacing;
   zone_data[zi].ZoneLength = zone_length;

   if (!CollapseZoneData(zone_data, num_zones))
   {
      pArtifact->SetNumberOfStirrupZonesDesigned(num_zones);
      pArtifact->SetShearZoneData(zi,zone_data[zi]);
   }
   else
      num_zones--;

   if (!is_zone4)
      return;

   // design zone 4
   zone_start = zone_end;
   zone_end   = gird_length2;
   Float64 avs_left_z4  = avs_envelope.Evaluate(zone_start);
   Float64 avs_right_z4 = avs_envelope.Evaluate(gird_length-zone_start);
   Float64 avs_zone4 = max(avs_left_z4,avs_right_z4);

   if (!GetStirrupsForAvs(span, gdr, avs_zone4, s_under_limit, &bar_size, &bar_spacing))
   {
      pArtifact->SetOutcome(pgsDesignArtifact::TooManyStirrupsReqd);
      m_DesignerOutcome.AbortDesign();
      return;
   }

   // make sure bar spacing does not exceed zone length
   zone_length = zone_end-zone_start;
   if (bar_spacing>zone_length)
      bar_spacing = zone_length;

   // save off zone 4 data
   num_zones++;
   zi = num_zones-1;
   zone_data[zi].ZoneNum = num_zones;
   zone_data[zi].VertBarSize = bar_size;
   zone_data[zi].BarSpacing = bar_spacing;
   zone_data[zi].ZoneLength = zone_length;

   if (!CollapseZoneData(zone_data, num_zones))
   {
      pArtifact->SetNumberOfStirrupZonesDesigned(num_zones);
      pArtifact->SetShearZoneData(zi,zone_data[zi]);
   }
}

static Float64 GetBarSpacingForAvs(SpanIndexType span,GirderIndexType gdr, const matRebar* pRebar, Float64 avs, Float64 roundTo, IBroker* pBroker)
{
   // prepare for min bar size check. 5.10.3.1.2
   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);
   Float64 max_aggregate_size = pMaterial->GetMaxAggrSizeGdr(span,gdr);
   ATLASSERT(0 < max_aggregate_size);

   Float64 db = pRebar->GetNominalDimension();

   // min clear spacing
   GET_IFACE2(pBroker,ITransverseReinforcementSpec,pTransverseReinforcementSpec);
   Float64 s_min = pTransverseReinforcementSpec->GetMinStirrupSpacing(max_aggregate_size,db);
   ATLASSERT(0.0 < s_min);
   s_min += db; // c-c spacing

   // nominal spacing
   Float64 ab = pRebar->GetNominalArea();
   CHECK(0.0 < ab);
   Float64 nominal_spacing = (NUM_LEGS*ab)/avs;

   Float64 s = FloorOff(nominal_spacing, roundTo); // round down to nearest "nice number"
   if (s < s_min && s_min < nominal_spacing)  // make sure we didn't round too far
      s = s_min;

   return s;
}


bool pgsDesigner2::GetStirrupsForAvs(SpanIndexType span,GirderIndexType gdr, Float64 avs, Float64 sMax, BarSizeType* pBarSize, Float64 *pSpacing)
{
   ATLASSERT(0.0 <= avs);
   ATLASSERT(0.0 <  sMax);

   const Uint16 NBARSIZES = 3;
   const BarSizeType barSizes[NBARSIZES]={3,4,5}; // first bar is smallest

   // if no demand, return smallest bar size at max spacing
   if (avs == 0.0)
   {
      *pBarSize = barSizes[0];
      *pSpacing = sMax;
      return true;
   }

   GET_IFACE(ISpecification,pSpec);
   GET_IFACE(ILibrary,pLib);

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   lrfdVersionMgr::Units units = pSpecEntry->GetSpecificationUnits();

   // round bar c-c spacing down to nearest 15mm or .5" depending on spec units
   // 
   Float64 round_to;
   if (units==lrfdVersionMgr::SI)
      round_to = ::ConvertToSysUnits(15., unitMeasure::Millimeter);
   else
      round_to = ::ConvertToSysUnits(0.5, unitMeasure::Inch);


   lrfdRebarPool* pool = lrfdRebarPool::GetInstance();
   ATLASSERT(pool != NULL);

   bool smallest_bar = true;
   for (Uint16 barSizeIdx = 0; barSizeIdx < NBARSIZES; barSizeIdx++)
   {
      BarSizeType barsize = barSizes[barSizeIdx];
      const matRebar* pRebar = pool->GetRebar(barsize);
      ATLASSERT(pRebar);
      Float64 s = GetBarSpacingForAvs(span, gdr, pRebar, avs, round_to, this->m_pBroker);

      if (s <= sMax)
      {
         *pBarSize = barsize;
         *pSpacing = s;

         // take a shot at using the next largest bar
         if (barSizeIdx < NBARSIZES-1)
         {
            BarSizeType next_barsize = barSizes[barSizeIdx+1];
            const matRebar* pNextRebar = pool->GetRebar(next_barsize);
            ATLASSERT(pNextRebar);
            Float64 s_next = GetBarSpacingForAvs(span, gdr, pNextRebar, avs, round_to, this->m_pBroker);
            if (s_next <= sMax)
            {
               *pBarSize = next_barsize;
               *pSpacing = s_next;
            }
         }
         return true;
      }

      // if smallest bar exceeds max spacing, use smallest bar at max spac.
      if (smallest_bar)
      {
         if (sMax < s)
         {
            *pBarSize = barsize;
            *pSpacing = sMax;
            return true;
         }
         smallest_bar = false;
      }
   }
   return false;
}

void pgsDesigner2::CalcAvSAtPois(pgsTypes::Stage stage,
                                 pgsTypes::LimitState ls,
                                 SpanIndexType span,
                                 GirderIndexType gdr,
                                 pgsDesignArtifact* pArtifact,
                                 const pgsPointOfInterest& leftCs,
                                 const pgsPointOfInterest& rightCs,
                                 const std::vector<pgsPointOfInterest>& vPoi, 
                                 std::vector<ShearDesignAvs>* avsReqdAtPois)
{
   GET_IFACE(IMomentCapacity,pMomCap);
   GET_IFACE(IShearCapacity,pShearCap);
   GET_IFACE(ISpecification,pSpec);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   LOG("Av/S values at "<<vPoi.size()<<" pois");
   LOG("Location, avsVertical, avsHorizontal");
   std::vector<pgsPointOfInterest>::const_iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      // get demand based on critical section nominal forces - capacity is based on pure nominal
      bool inbetween_support_and_cs;
      pgsPointOfInterest shear_poi;

      if (poi.GetDistFromStart() < leftCs.GetDistFromStart())
      {
         shear_poi = leftCs;
         inbetween_support_and_cs = true;
      }
      else if (poi.GetDistFromStart() > rightCs.GetDistFromStart())
      {
         shear_poi = rightCs;
         inbetween_support_and_cs = true;
      }
      else
      {
         shear_poi = poi;
         inbetween_support_and_cs = false;
      }

      GET_IFACE(ILimitStateForces, pLimitStateForces);
      sysSectionValue mns, mxs;
      if ( analysisType == pgsTypes::Envelope )
      {
         sysSectionValue Vmin,Vmax;
         pLimitStateForces->GetShear(ls,stage,shear_poi,MaxSimpleContinuousEnvelope,&Vmin,&Vmax);
         mxs = Vmax;

         pLimitStateForces->GetShear(ls,stage,shear_poi,MinSimpleContinuousEnvelope,&Vmin,&Vmax);
         mns = Vmin;
      }
      else
      {
         pLimitStateForces->GetShear(ls,stage,shear_poi,analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan,&mns,&mxs);
      }

      ShearDesignAvs sdavs;

      // can use concrete shear capacity of current section because it is independent of stirrups
      SHEARCAPACITYDETAILS scd;
      pShearCap->GetShearCapacityDetails( ls, stage, poi,pArtifact->GetGirderConfiguration(), &scd );

      // spacing calculation values
      sdavs.Pt1FcBvDv = 0.1*scd.fc*scd.bv*scd.dv;

      // shear demand
      Float64 vu = max(max(abs(mns.Left()), abs(mxs.Left())), max(abs(mns.Right()),abs(mxs.Right())));
      sdavs.Vu = vu;

      // do not design in range at girder ends between critical sections and supports
      if (!inbetween_support_and_cs)
      {
         // Design for vertical shear
         DesignForVerticalShear(span, gdr, vu, scd, &(sdavs.VerticalAvS));
         if (m_DesignerOutcome.WasDesignAborted())
         {
            pArtifact->SetOutcome(pgsDesignArtifact::ShearExceedsMaxConcreteStrength);
            return;
         }

         // horizontal shear
         DesignForHorizontalShear(poi, vu, scd, &(sdavs.HorizontalAvS));
         if (m_DesignerOutcome.WasDesignAborted())
         {
            pArtifact->SetOutcome(pgsDesignArtifact::ShearExceedsMaxConcreteStrength);
            return;
         }
      }
      else
      {
         // no stirrup req'd due to horizontal and vertical shears in cs area
         sdavs.VerticalAvS = 0.0;
         sdavs.HorizontalAvS = 0.0;
      }

      // push back successful Av/S calc onto stack
      avsReqdAtPois->push_back(sdavs);

      LOG(poi.GetDistFromStart()<<", "<<sdavs.VerticalAvS <<", "<<sdavs.HorizontalAvS);
   }
}

void pgsDesigner2::DesignForVerticalShear(SpanIndexType span,GirderIndexType gdr,Float64 vu, const SHEARCAPACITYDETAILS& scd, Float64* pAvs)
{
   // Load up some variables from capacity details
   Float64 vp    = scd.Vp;
   Float64 vc    = scd.Vc;
   Float64 phi   = scd.Phi;
   Float64 bv    = scd.bv;
   Float64 dv    = scd.dv;
   Float64 fc    = scd.fc;
   Float64 alpha = scd.Alpha;
   Float64 theta = scd.Theta;
   PRECONDITION(fc>0.0);
   CHECK(alpha!=0.0);

   Float64 fy = scd.fy;
   if ( IsZero(fy) )
   {
      GET_IFACE(IBridgeMaterial,pMaterial);
      Float64 Es;
      pMaterial->GetTransverseRebarProperties(span,gdr,&Es,&fy);
      CHECK(fy!=0.0);
   }

   // shear stress - LRFD C5.8.3.3-1
   Float64 v;
   if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
      v = fabs(vu - phi*vp)/(phi*bv*dv); 
   else
      v = (vu - phi*vp)/(phi*bv*dv); 

   LOG("Shear stress, v="<<v);

   // check to make sure we even have a chance of designing some stirrups
   Float64 vfc = v/fc;
   if ( vfc <= 0.25)
   {
      if (scd.ShearInRange)
      {
         // see if we need shear steel at all
         Float64 vns = 0.5*phi*(vc+vp);
         if (vu<vns)
            *pAvs = 0.0;
         else
         {
            // shear steel is req'd
            Float64 vs = vu/phi - vc - vp;
            Float64 avs = vs/(fy*dv*(1/tan(theta)+1/tan(alpha))*sin(alpha));
            *pAvs=avs;
         }
      }
      else
      {
         LOG("Shear stress exceeds max for section - can not calculate theta - aborting design");
         m_DesignerOutcome.AbortDesign();
         return; // section too small for applied shear
      }
   }
   else
   {
      LOG("Shear stress exceeds max for section - aborting design");
      m_DesignerOutcome.AbortDesign();
      return; // section too small for applied shear
   }
}

void pgsDesigner2::DesignForHorizontalShear(const pgsPointOfInterest& poi, Float64 vu, const SHEARCAPACITYDETAILS& scd, Float64* pAvs)
{
   // no deck, no horizontal shear
   GET_IFACE(IBridge,pBridge);
   if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
   {
      *pAvs = 0.0; 
      return;
   }

   SpanIndexType span = poi.GetSpan();
   GirderIndexType gdr = poi.GetGirder();

   // Load up some variables from capacity details
   Float64 vp    = scd.Vp;
   Float64 vc    = scd.Vc;
   Float64 phi   = scd.Phi;
   Float64 bv    = scd.bv;
   Float64 dv    = scd.dv;
   Float64 alpha = scd.Alpha;
   Float64 theta = scd.Theta;
   PRECONDITION(phi>0.0);
   CHECK(alpha!=0.0);

   Float64 fy = scd.fy;
   if ( IsZero(fy) )
   {
      GET_IFACE(IBridgeMaterial,pMaterial);
      Float64 Es;
      pMaterial->GetTransverseRebarProperties(span,gdr,&Es,&fy);
      CHECK(fy!=0.0);
   }

   // determine shear demand
   GET_IFACE(ISectProp2,pSectProp2);
   GET_IFACE(IBridgeMaterial,pMaterial);
   GET_IFACE(IProductForces,pProductForces);
   GET_IFACE(IGirder, pGdr);

   Float64 Qslab = pSectProp2->GetQSlab(poi);
   PRECONDITION(Qslab!=0);
   Float64 Ic  = pSectProp2->GetIx(pgsTypes::BridgeSite3,poi);
   sysSectionValue Vuh;
   Float64 vuh = vu*Qslab/Ic;

   // normal force on top of girder flange
   Float64 comp_force = GetNormalFrictionForce(poi);
   Float64 comp_force_u = ::ConvertFromSysUnits(comp_force,unitMeasure::Newton);

   // Area of shear transfer
   Float64 Acv = pGdr->GetShearInterfaceWidth( poi );

   Float64 fc_slab = pMaterial->GetFcSlab();
   Float64 fc_girder = pMaterial->GetFcGdr(span,gdr);
   Float64 fc = min(fc_slab,fc_girder);

   // max nominal shear capacity 5.8.4.1-2,3
   Float64 acv_u = ::ConvertFromSysUnits(Acv, unitMeasure::Millimeter2);
   Float64 fc_u  = ::ConvertFromSysUnits(fc, unitMeasure::MPa);
   Float64 vhn_max_u = min( 0.2*fc_u*acv_u, 5.5*acv_u);
   Float64 vhn_max = ::ConvertToSysUnits(vhn_max_u, unitMeasure::Newton);

   // if demand is greater than max capacity, we must bail
   if (vuh <= vhn_max)
   {
      CHECK(fy>0.0);
      Float64 fy_u =  ::ConvertFromSysUnits(fy, unitMeasure::MPa);

      bool is_roughened = pBridge->AreGirderTopFlangesRoughened();
      Float64 c = lrfdConcreteUtil::ShearCohesionFactor(is_roughened);
      Float64 c_u = ::ConvertFromSysUnits(c, unitMeasure::MPa);
      Float64 nu = lrfdConcreteUtil::ShearFrictionFactor(is_roughened);
      CHECK(nu>0.0);

      // demand av/s - 5.8.4.1-1
      Float64 avsd = ((vuh/phi - c*bv)/nu - comp_force)/fy;

      // min av/s
      Float64 bv_u = ::ConvertFromSysUnits(bv, unitMeasure::Millimeter);
      Float64 avsmin_u = 0.35*bv_u/fy_u;
      Float64 avsmin = ::ConvertToSysUnits(avsmin_u, unitMeasure::Millimeter2PerMillimeter);

      Float64 avs=max(avsmin, avsd);
      *pAvs = avs;
   }
   else
   {
      m_DesignerOutcome.AbortDesign();
   }
}

Float64 pgsDesigner2::CalcAvsForSplittingZone(SpanIndexType span,GirderIndexType gdr,const pgsDesignArtifact& rArtifact)
{
   // determine shear demand
   GET_IFACE(IGirder,pGdr);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeMaterial,pMaterial);
   GET_IFACE(IStrandGeometry,pStrandGeometry);
   GET_IFACE(ITransverseReinforcementSpec,pTransverseReinforcementSpec);

   // max stress in rebar
   Float64 Es, fy;
   pMaterial->GetTransverseRebarProperties(span,gdr,&Es,&fy);
   Float64 fs = pTransverseReinforcementSpec->GetMaxSplittingStress(fy);

   // area of strands and pjack
   const matPsStrand* pstrand = pMaterial->GetStrand(span,gdr);
   Float64 astrand = pstrand->GetNominalArea();
   Float64 aps   = astrand*(rArtifact.GetNumHarpedStrands()+rArtifact.GetNumStraightStrands());
   Float64 pjack = rArtifact.GetPjackHarpedStrands() + rArtifact.GetPjackStraightStrands();

   GET_IFACE(ILosses,pLosses);
   pgsPointOfInterest poi(span,gdr,0.00);
   Float64 dFpR0 = pLosses->GetBeforeXferLosses( poi,pgsTypes::Permanent,rArtifact.GetGirderConfiguration());
   Float64 dFpES = pLosses->GetElasticShortening(poi,pgsTypes::Permanent,rArtifact.GetGirderConfiguration());

   // total area of reinforcement located within h/4 from the end of the beam required to resist 4% of the prestress force at transfer
   Float64 avs = 0.04*(pjack - aps*(dFpR0 + dFpES)) / fs;

   return avs;
}

Float64 pgsDesigner2::CalcAvsForConfinement(SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(ITransverseReinforcementSpec,pTransverseReinforcementSpec);

   Float64 avs = pTransverseReinforcementSpec->GetMinConfinmentAvS();

   return avs;
}



Float64 pgsDesigner2::RoundSlabOffset(Float64 offset)
{
   Float64 newoff;
   // Round to nearest 1/4" (5 mm) per WSDOT BDM
   GET_IFACE(IProjectSettings, pProjSettings);
   if ( pProjSettings->GetUnitsMode() == pgsTypes::umSI )
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
   os << "Dump for pgsDesigner2" << endl;
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

