///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#include "stdafx.h"
#include "resource.h"
#include <Graphing\AnalysisResultsGraphBuilder.h>
#include <Graphing\DrawBeamTool.h>
#include "AnalysisResultsGraphController.h"
#include "AnalysisResultsGraphDefinition.h"

#include <PGSuperColors.h>

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\PhysicalConverter.h>
#include <PgsExt\SegmentArtifact.h>

#include <IFace\Intervals.h>
#include <IFace\DocumentType.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\RatingSpecification.h>
#include <IFace\Allowables.h>
#include <IFace\MomentCapacity.h>
#include <IFace\ShearCapacity.h>
#include <IFace\Artifact.h>

#include <EAF\EAFGraphView.h>

#include <PgsExt\ClosurePourData.h>

#include <MFCTools\MFCTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


COLORREF truckColors[] = 
{
   SLATEGREY,
   DARKSLATEGREY,
   MIDNIGHTBLUE,
   CORNFLOWERBLUE,
   SLATEBLUE,
   SALMON1,
   ORANGE1,
   DARKORANGE1,
   ORANGERED1,
   DEEPPINK1,
   HOTPINK1,
   MAGENTA1,
   PLUM1,
   PURPLE1
};

long numTruckColors = sizeof(truckColors)/sizeof(COLORREF);

// create a dummy unit conversion tool to pacify the graph constructor
static unitmgtLengthData DUMMY(unitMeasure::Meter);
static LengthTool    DUMMY_TOOL(DUMMY);

// Pen styles for stresses at top and bottom of girder
#define PS_STRESS_TOP     PS_SOLID
#define PS_STRESS_BOTTOM  PS_DASH

BEGIN_MESSAGE_MAP(CAnalysisResultsGraphBuilder, CEAFGraphBuilderBase)
END_MESSAGE_MAP()


CAnalysisResultsGraphBuilder::CAnalysisResultsGraphBuilder() :
CGirderGraphBuilderBase()
{
   m_pGraphDefinitions = new CAnalysisResultsGraphDefinitions;

   SetName(_T("Analysis Results"));
}

CAnalysisResultsGraphBuilder::CAnalysisResultsGraphBuilder(const CAnalysisResultsGraphBuilder& other) :
CGirderGraphBuilderBase(other)
{
   m_pGraphDefinitions = new CAnalysisResultsGraphDefinitions;
}

CAnalysisResultsGraphBuilder::~CAnalysisResultsGraphBuilder()
{
   if ( m_pGraphDefinitions != NULL )
   {
      delete m_pGraphDefinitions;
      m_pGraphDefinitions = NULL;
   }
}

void CAnalysisResultsGraphBuilder::DumpLBAM()
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);
   pProgress->UpdateMessage(_T("Dumping Analysis Models"));

   GET_IFACE(IProductForces,pProductForces);
   pProductForces->DumpAnalysisModels( m_pGraphController->GetGirder() );

   AfxMessageBox(_T("Analysis Model Dump Complete"),MB_OK);
}

CGraphBuilder* CAnalysisResultsGraphBuilder::Clone()
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CAnalysisResultsGraphBuilder(*this);
}

std::vector<std::pair<CString,IDType>> CAnalysisResultsGraphBuilder::GetLoadCaseNames(IntervalIndexType intervalIdx,ActionType actionType)
{
   return m_pGraphDefinitions->GetLoadCaseNames(intervalIdx,actionType);
}

std::vector<IntervalIndexType> CAnalysisResultsGraphBuilder::AddTSRemovalIntervals(IntervalIndexType loadingIntervalIdx,const std::vector<IntervalIndexType>& vIntervals,const std::vector<IntervalIndexType>& vTSRIntervals)
{
   // given an interval when a loading occors, a vector of intervals to make results available, and a secondary vector of intervals
   // merge all the secondary intervals that occur at or after the loading interval into the primary vector
   std::vector<IntervalIndexType> intervals(vIntervals);

   // search the temporary support removal intervals for the first interval
   // that occurs at or after the loading interval
   std::vector<IntervalIndexType>::const_iterator iter(vTSRIntervals.begin());
   std::vector<IntervalIndexType>::const_iterator end(vTSRIntervals.end());
   for ( ; iter != end; iter++ )
   {
      IntervalIndexType intervalIdx = *iter;
      if (loadingIntervalIdx <= intervalIdx )
         break; // found the first interval that occurs at or after the loading interval
   }

   // insert the remaining intervals into the vector
   intervals.insert(intervals.end(),iter,end);

   // eliminate duplicates
   std::sort(intervals.begin(),intervals.end());
   intervals.erase( std::unique(intervals.begin(),intervals.end()), intervals.end() );
   return intervals;
}

void CAnalysisResultsGraphBuilder::UpdateGraphDefinitions()
{
#pragma Reminder("UPDATE: need better color scheme")

   m_pGraphDefinitions->Clear();

   IDType graphID = 0;

   GroupIndexType grpIdx  = m_pGraphController->GetGirderGroup();
   GirderIndexType gdrIdx = m_pGraphController->GetGirder();
   CGirderKey girderKey(grpIdx,gdrIdx);

   GET_IFACE(IProductLoads,pProductLoads);

   // determine if there are temporary strands or pedestrian load for any of the girders
   // for this group
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GroupIndexType startGroupIdx = (grpIdx == ALL_GROUPS ? 0 : grpIdx);
   GroupIndexType endGroupIdx   = (grpIdx == ALL_GROUPS ? pBridge->GetGirderGroupCount()-1 : startGroupIdx);
   bool bTempStrand = false;
   bool bPedLoading = false;
   bool bSidewalk   = false;
   bool bShearKey   = false;
   for ( GroupIndexType groupIdx = startGroupIdx; groupIdx <= endGroupIdx; groupIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(groupIdx);
      GirderIndexType girderIdx = min(gdrIdx,nGirders-1);

      CGirderKey thisGirderKey(groupIdx,girderIdx);

      bPedLoading |= pProductLoads->HasPedestrianLoad(thisGirderKey);
      bSidewalk   |= pProductLoads->HasSidewalkLoad(thisGirderKey);
      bShearKey   |= pProductLoads->HasShearKeyLoad(thisGirderKey);

      SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(thisGirderKey,segIdx);

         StrandIndexType NtMax = pStrandGeom->GetMaxStrands(segmentKey,pgsTypes::Temporary);
         bTempStrand |= ( 0 < NtMax );
      } // next segIdx
   } // next groupIdx

#pragma Reminder("UPDATE: using dummy segment")
   // Get intervals for reporting
   GET_IFACE(IIntervals,pIntervals);

   // spec check intervals
   std::vector<IntervalIndexType> vSpecCheckIntervals(pIntervals->GetSpecCheckIntervals(CGirderKey(0,0)));

   // initial intervals
   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(CSegmentKey(0,0,0));
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(CSegmentKey(0,0,0));
   IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetFirstErectedSegmentInterval();
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
   std::vector<IntervalIndexType> vInitialIntervals;
   vInitialIntervals.push_back(releaseIntervalIdx);
   vInitialIntervals.push_back(storageIntervalIdx);
   vInitialIntervals.push_back(erectSegmentIntervalIdx);

   std::vector<IntervalIndexType> vDeckAndDiaphragmIntervals;
   vDeckAndDiaphragmIntervals.push_back(castDeckIntervalIdx);

   std::vector<IntervalIndexType> vRailingSystemIntervals;
   vRailingSystemIntervals.push_back(railingSystemIntervalIdx);

   std::vector<IntervalIndexType> vOverlayIntervals;
   vOverlayIntervals.push_back(overlayIntervalIdx);

   // all intervals
   std::vector<IntervalIndexType> vAllIntervals;
   for ( IntervalIndexType intervalIdx = releaseIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      vAllIntervals.push_back(intervalIdx);
   }

   // bridge site intervals
   std::vector<IntervalIndexType> vBridgeSiteIntervals;
   for ( IntervalIndexType intervalIdx = erectSegmentIntervalIdx+1; intervalIdx < nIntervals; intervalIdx++ )
   {
      vBridgeSiteIntervals.push_back(intervalIdx);
   }

   // intervals only after live load is applied
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   std::vector<IntervalIndexType> vLiveLoadIntervals;
   for ( IntervalIndexType intervalIdx = liveLoadIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      vLiveLoadIntervals.push_back(intervalIdx);
   }

   std::vector<IntervalIndexType> vPTIntervals, vAllPTIntervals;
   GET_IFACE(ITendonGeometry,pTendonGeometry);
   IntervalIndexType firstPTIntervalIdx = nIntervals;
   for ( GroupIndexType groupIdx = startGroupIdx; groupIdx <= endGroupIdx; groupIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(groupIdx);
      GirderIndexType girderIdx = min(gdrIdx,nGirders-1);

      CGirderKey thisGirderKey(groupIdx,girderIdx);

      DuctIndexType nDucts = pTendonGeometry->GetDuctCount(thisGirderKey);
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         IntervalIndexType ptIntervalIdx = pIntervals->GetStressTendonInterval(thisGirderKey,ductIdx);
         vPTIntervals.push_back(ptIntervalIdx);
         firstPTIntervalIdx = min(firstPTIntervalIdx,ptIntervalIdx);
      }
   }
   std::sort(vPTIntervals.begin(),vPTIntervals.end());
   for ( IntervalIndexType intervalIdx = firstPTIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      vAllPTIntervals.push_back(intervalIdx);
   }

   std::vector<IntervalIndexType> vTempSupportRemovalIntervals;
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   SupportIndexType nTS = pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      const CTemporarySupportData* pTS = pBridgeDesc->GetTemporarySupport(tsIdx);
      SupportIDType tsID = pTS->GetID();
      IntervalIndexType tsrIntervalIdx = pIntervals->GetTemporarySupportRemovalInterval(tsID);
      vTempSupportRemovalIntervals.push_back(tsrIntervalIdx);
   }

   //////////////////////////////////////////////////////////////////////////
   // Product Load Cases
   //////////////////////////////////////////////////////////////////////////

   // girder self-weight
   std::vector<IntervalIndexType> intervals( AddTSRemovalIntervals(erectSegmentIntervalIdx,vInitialIntervals,vTempSupportRemovalIntervals) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Girder"), pftGirder, intervals, ACTIONS_ALL,BROWN));


   intervals.clear();
   intervals = AddTSRemovalIntervals(castDeckIntervalIdx,vDeckAndDiaphragmIntervals,vTempSupportRemovalIntervals);
   GET_IFACE(IUserDefinedLoadData,pUserLoads);
   if ( !IsZero(pUserLoads->GetConstructionLoad()) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Construction"), pftConstruction,  intervals, ACTIONS_ALL,INDIANRED) );
   }

   // slab dead load
   CAnalysisResultsGraphDefinition slabGraphDef(graphID++, _T("Slab"),   pftSlab,    intervals, ACTIONS_ALL, SALMON);
   slabGraphDef.AddIntervals(vTempSupportRemovalIntervals);
   m_pGraphDefinitions->AddGraphDefinition(slabGraphDef);

   CAnalysisResultsGraphDefinition haunchGraphDef(graphID++, _T("Haunch"), pftSlabPad, intervals, ACTIONS_ALL, SALMON);
   haunchGraphDef.AddIntervals(vTempSupportRemovalIntervals);
   m_pGraphDefinitions->AddGraphDefinition(haunchGraphDef);

   if ( pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP )
   {
      CAnalysisResultsGraphDefinition slabPanelGraphDef(graphID++, _T("Slab Panels"), pftSlabPanel, intervals, ACTIONS_ALL, RED4);
      slabPanelGraphDef.AddIntervals(vTempSupportRemovalIntervals);
      m_pGraphDefinitions->AddGraphDefinition( slabPanelGraphDef );
   }

   CAnalysisResultsGraphDefinition diaphragmGraphDef(graphID++, _T("Diaphragm"), pftDiaphragm, intervals, ACTIONS_ALL, ORANGE);
   diaphragmGraphDef.AddIntervals(vTempSupportRemovalIntervals);
   m_pGraphDefinitions->AddGraphDefinition( diaphragmGraphDef );

   if (bShearKey)
   {
      CAnalysisResultsGraphDefinition shearKeyGraphDef(graphID++, _T("Shear Key"), pftShearKey, intervals, ACTIONS_ALL, DARKSEAGREEN);
      shearKeyGraphDef.AddIntervals(vTempSupportRemovalIntervals);
      m_pGraphDefinitions->AddGraphDefinition(shearKeyGraphDef);
   }

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Creep"),      pftCreep,      vAllIntervals, ACTIONS_ALL, TAN) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Shrinkage"),  pftShrinkage,  vAllIntervals, ACTIONS_ALL, PLUM) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Relaxation"), pftRelaxation, vAllIntervals, ACTIONS_ALL, SKYBLUE1) );
   }

   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Total Post Tensioning"),   pftTotalPostTensioning,   vPTIntervals, ACTIONS_ALL,DARKSEAGREEN) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Primary Post Tensioning"), pftPrimaryPostTensioning, vPTIntervals, ACTIONS_ALL,BLUE) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Secondary Effects"),       pftSecondaryEffects,      vPTIntervals, ACTIONS_ALL,VIOLET) );


   intervals.clear();
   intervals = AddTSRemovalIntervals(railingSystemIntervalIdx,vRailingSystemIntervals,vTempSupportRemovalIntervals);
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Traffic Barrier"), pftTrafficBarrier, intervals, ACTIONS_ALL,TAN) );

   if ( bSidewalk )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Sidewalk"),pftSidewalk, intervals, ACTIONS_ALL,PERU) );
   }


   intervals.clear();
   intervals = AddTSRemovalIntervals(overlayIntervalIdx,vOverlayIntervals,vTempSupportRemovalIntervals);
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Overlay"), pftOverlay, intervals, ACTIONS_ALL,VIOLET) );

   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Prestress"), graphPrestress, vAllIntervals, DODGERBLUE) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Post Tension"), graphPostTension, vAllPTIntervals, DODGERBLUE) );

   // User Defined Static Loads
#pragma Reminder("UPDATE: user defined load intervals")
   // use intervals when user defined loads are applied plus temporary support removal intervals
   // that occur after the first user defined load is applied
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("User DC"),        pftUserDC,   vBridgeSiteIntervals,  ACTIONS_ALL,CORAL) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("User DW"),        pftUserDW,   vBridgeSiteIntervals,  ACTIONS_ALL,GOLD) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("User Live Load"), pftUserLLIM, vLiveLoadIntervals,    ACTIONS_ALL,MAROON) );


   ////////////////////////////////////////////////////////
   // Live Load 
   ////////////////////////////////////////////////////////

   // Individual Truck Responses
   GET_IFACE(ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);

   std::vector<pgsTypes::LiveLoadType> vLiveLoadTypes;
   vLiveLoadTypes.push_back(pgsTypes::lltDesign);
   vLiveLoadTypes.push_back(pgsTypes::lltFatigue);

   if ( bPermit )
      vLiveLoadTypes.push_back(pgsTypes::lltPermit);

   GET_IFACE(IRatingSpecification,pRatingSpec);
   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
      vLiveLoadTypes.push_back(pgsTypes::lltLegalRating_Routine);

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
      vLiveLoadTypes.push_back(pgsTypes::lltLegalRating_Special);

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
      vLiveLoadTypes.push_back(pgsTypes::lltPermitRating_Routine);

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
      vLiveLoadTypes.push_back(pgsTypes::lltPermitRating_Special);

   std::vector<pgsTypes::LiveLoadType>::iterator iter;
   for ( iter = vLiveLoadTypes.begin(); iter != vLiveLoadTypes.end(); iter++ )
   {
      pgsTypes::LiveLoadType llType = *iter;

      std::_tstring strBase;
      switch(llType)
      {
      case pgsTypes::lltDesign:
         strBase = _T("Design");
         break;

      case pgsTypes::lltFatigue:
         strBase = _T("Fatigue");
         break;

      case pgsTypes::lltPermit:
         strBase = _T("Permit");
         break;

      case pgsTypes::lltLegalRating_Routine:
         strBase = _T("Legal Rating (Routine)");
         break;

      case pgsTypes::lltLegalRating_Special:
         strBase = _T("Legal Rating (Special)");
         break;

      case pgsTypes::lltPermitRating_Routine:
         strBase = _T("Permit Rating (Routine)");
         break;

      case pgsTypes::lltPermitRating_Special:
         strBase = _T("Permit Rating (Special)");
         break;

      default:
         ATLASSERT(false); // should never get here
      }

      std::vector<std::_tstring> strLLNames( pProductLoads->GetVehicleNames(llType,girderKey) );
      long colorIdx = 0;
      int action;
      switch(llType)
      {
      case pgsTypes::lltDesign:
      case pgsTypes::lltFatigue:
      case pgsTypes::lltLegalRating_Routine:
      case pgsTypes::lltLegalRating_Special:
      case pgsTypes::lltPermitRating_Routine:
      case pgsTypes::lltPermitRating_Special:
         action = ACTIONS_ALL;
         break;

      case pgsTypes::lltPermit:
         action = ACTIONS_FORCE_DISPLACEMENT;
         break;

      default:
         ATLASSERT(false);
      }

      VehicleIndexType vehicleIndex = 0;
      std::vector<std::_tstring>::iterator iter(strLLNames.begin());
      std::vector<std::_tstring>::iterator end(strLLNames.end());
      for ( ; iter != end; iter++, vehicleIndex++ )
      {
         std::_tstring& strName( *iter );

         // skip the dummy live load
         if ( strName == _T("No Live Load Defined") )
            continue;

         std::_tstring strLLName( strBase + _T(" - ") + strName );

         CAnalysisResultsGraphDefinition def(graphID++,
                                             CString(strLLName.c_str()),
                                             llType,
                                             vehicleIndex,
                                             vLiveLoadIntervals,
                                             action,
                                             truckColors[colorIdx++]);

         m_pGraphDefinitions->AddGraphDefinition(def);

         if ( numTruckColors <= colorIdx )
            colorIdx = 0;
      }

      std::_tstring strLLName( strBase + _T(" - LL+IM") );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, CString(strLLName.c_str()), llType,  -1, vLiveLoadIntervals,  ACTIONS_ALL, MAGENTA) );
   }

   // Combined Results
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("DC"), lcDC, vAllIntervals,         ACTIONS_ALL, RED) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("DW"), lcDW, vBridgeSiteIntervals,  ACTIONS_ALL, GOLDENROD) );

   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("CR"), lcCR, vAllIntervals,         ACTIONS_ALL, BROWN) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("SR"), lcSH, vAllIntervals,         ACTIONS_ALL, MAGENTA) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("PS"), lcPS, vAllIntervals,         ACTIONS_ALL, NAVY) );
   }

   if ( bPedLoading )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("PL"),   pgsTypes::lltPedestrian, vLiveLoadIntervals,  ACTIONS_ALL, FIREBRICK) );
   }

   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Design)"), pgsTypes::lltDesign, vLiveLoadIntervals, ACTIONS_ALL, MAGENTA) );

   if (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Fatigue)"), pgsTypes::lltFatigue, vLiveLoadIntervals, ACTIONS_ALL, PURPLE) );
   }

   if (bPermit)
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Permit)"), pgsTypes::lltPermit, vLiveLoadIntervals, ACTIONS_FORCE_DISPLACEMENT, MEDIUMPURPLE) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Legal Rating, Routine)"), pgsTypes::lltLegalRating_Routine, vLiveLoadIntervals, ACTIONS_ALL, HOTPINK) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Legal Rating, Special)"), pgsTypes::lltLegalRating_Special, vLiveLoadIntervals, ACTIONS_ALL, DARKSEAGREEN4) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Permit Rating, Routine)"), pgsTypes::lltPermitRating_Routine, vLiveLoadIntervals, ACTIONS_ALL, TOMATO) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Permit Rating, Special)"), pgsTypes::lltPermitRating_Special, vLiveLoadIntervals, ACTIONS_ALL, DARKORANGE) );
   }

   // Limit States and Capacities
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service I (Design)"), pgsTypes::ServiceI, vAllIntervals, ACTIONS_STRESS_ONLY | ACTIONS_DISPLACEMENT_ONLY, SLATEBLUE) );
   
   if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service IA (Design)"), pgsTypes::ServiceIA, vLiveLoadIntervals, ACTIONS_STRESS_ONLY, NAVY) );
   }

   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III (Design)"),          pgsTypes::ServiceIII,               vLiveLoadIntervals,  ACTIONS_STRESS_ONLY,  ROYALBLUE) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Design)"),           pgsTypes::StrengthI,                vLiveLoadIntervals,  ACTIONS_MOMENT_SHEAR, CADETBLUE) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity (Design)"),  pgsTypes::StrengthI, graphCapacity, vLiveLoadIntervals,  ACTIONS_SHEAR_ONLY,   CADETBLUE) );
   
   if (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Fatigue I"), pgsTypes::FatigueI, vLiveLoadIntervals, ACTIONS_STRESS_ONLY, CHOCOLATE) );
   }

   GET_IFACE(ILimitStateForces,pLimitStateForces);
   bool bStrII = pLimitStateForces->IsStrengthIIApplicable(CSegmentKey(girderKey.groupIndex,girderKey.girderIndex,0));

   if ( bStrII )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II (Permit)"),          pgsTypes::StrengthII,               vLiveLoadIntervals,  ACTIONS_MOMENT_SHEAR, BROWN) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II Capacity (Permit)"), pgsTypes::StrengthII,graphCapacity, vLiveLoadIntervals,  ACTIONS_SHEAR_ONLY,   BROWN) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Design Rating, Inventory)"),           pgsTypes::StrengthI_Inventory,                 vLiveLoadIntervals,  ACTIONS_MOMENT_SHEAR, DEEPPINK4) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity (Design Rating, Inventory)"),  pgsTypes::StrengthI_Inventory, graphCapacity,  vLiveLoadIntervals,  ACTIONS_SHEAR_ONLY,   DEEPPINK4) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Design Rating, Operating)"),           pgsTypes::StrengthI_Operating,                 vLiveLoadIntervals,  ACTIONS_MOMENT_SHEAR, THISTLE) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity (Design Rating, Operating)"),  pgsTypes::StrengthI_Operating, graphCapacity,  vLiveLoadIntervals,  ACTIONS_SHEAR_ONLY,   THISTLE) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Legal Rating, Routine)"),           pgsTypes::StrengthI_LegalRoutine,                 vLiveLoadIntervals,  ACTIONS_MOMENT_SHEAR, SLATEBLUE) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity (Legal Rating, Routine)"),  pgsTypes::StrengthI_LegalRoutine, graphCapacity,  vLiveLoadIntervals,  ACTIONS_SHEAR_ONLY,   SLATEBLUE) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Legal Rating, Special)"),            pgsTypes::StrengthI_LegalSpecial,                vLiveLoadIntervals,  ACTIONS_MOMENT_SHEAR, DARKORCHID) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity, (Legal Rating, Special)"),  pgsTypes::StrengthI_LegalSpecial, graphCapacity, vLiveLoadIntervals,  ACTIONS_SHEAR_ONLY,   DARKORCHID) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II (Routine Permit Rating)"),           pgsTypes::StrengthII_PermitRoutine,                 vLiveLoadIntervals,  ACTIONS_MOMENT_SHEAR, MEDIUMSLATEBLUE) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II Capacity (Routine Permit Rating)"),  pgsTypes::StrengthII_PermitRoutine, graphCapacity,  vLiveLoadIntervals,  ACTIONS_SHEAR_ONLY,   MEDIUMSLATEBLUE) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II (Special Permit Rating)"),           pgsTypes::StrengthII_PermitSpecial,                 vLiveLoadIntervals,  ACTIONS_MOMENT_SHEAR, DARKSLATEGRAY) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II Capacity (Special Permit Rating)"),  pgsTypes::StrengthII_PermitSpecial, graphCapacity,  vLiveLoadIntervals,  ACTIONS_SHEAR_ONLY,   DARKSLATEGRAY) );
   }

   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Moment Capacity"),      pgsTypes::StrengthI, graphCapacity,    vLiveLoadIntervals,  ACTIONS_MOMENT_ONLY,  CHOCOLATE) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Min Moment Capacity"),  pgsTypes::StrengthI, graphMinCapacity, vLiveLoadIntervals,  ACTIONS_MOMENT_ONLY,  SADDLEBROWN) );

   // Demand and Allowable
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service I Demand (Design)"),     pgsTypes::ServiceI,  graphDemand,    vAllIntervals, RGB(139, 26, 26)) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service I Allowable (Design)"),  pgsTypes::ServiceI,  graphAllowable, vSpecCheckIntervals, RGB(139, 26, 26)) );
   
   if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service IA Demand (Design)"),    pgsTypes::ServiceIA, graphDemand,    vLiveLoadIntervals, RGB(255, 69,  0)) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service IA Allowable (Design)"), pgsTypes::ServiceIA, graphAllowable, vLiveLoadIntervals, RGB(255, 69,  0)) );
   }

   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Design)"),   pgsTypes::ServiceIII,graphDemand,    vLiveLoadIntervals, RGB(205, 16,118)) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Allowable (Design)"),pgsTypes::ServiceIII,graphAllowable, vLiveLoadIntervals, RGB(205, 16,118)) );
   
   if (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Fatigue I Demand"),    pgsTypes::FatigueI, graphDemand,    vLiveLoadIntervals, RGB(255, 69,  0)) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Fatigue I Allowable"), pgsTypes::FatigueI, graphAllowable, vLiveLoadIntervals, RGB(255, 69,  0)) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Design Rating, Inventory)"),   pgsTypes::ServiceIII_Inventory,graphDemand,    vLiveLoadIntervals, RGB(118,16,205)) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Allowable (Design Rating, Inventory)"),pgsTypes::ServiceIII_Inventory,graphAllowable, vLiveLoadIntervals, RGB(118,16,205)) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Legal Rating, Routine)"),   pgsTypes::ServiceIII_LegalRoutine,graphDemand,    vLiveLoadIntervals, RGB( 16,205,118)) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Allowable (Legal Rating, Routine)"),pgsTypes::ServiceIII_LegalRoutine,graphAllowable, vLiveLoadIntervals, RGB( 16,205,118)) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Legal Rating, Special)"),   pgsTypes::ServiceIII_LegalSpecial,graphDemand,     vLiveLoadIntervals, RGB(205, 16,118)) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Allowable (Legal Rating, Special)"),pgsTypes::ServiceIII_LegalSpecial,graphAllowable,  vLiveLoadIntervals, RGB(205, 16,118)) );
   }
}

CGirderGraphControllerBase* CAnalysisResultsGraphBuilder::CreateGraphController()
{
   return new CAnalysisResultsGraphController;
}

BOOL CAnalysisResultsGraphBuilder::InitGraphController(CWnd* pParent,UINT nID)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   ATLASSERT(m_pGraphController != NULL);

   if ( !m_pGraphController->Create(pParent,IDD_ANALYSISRESULTS_GRAPH_CONTROLLER, CBRS_LEFT, nID) )
      return FALSE;

   UpdateGraphDefinitions();
   ((CAnalysisResultsGraphController*)m_pGraphController)->FillLoadCaseList();

   return TRUE;
}

bool CAnalysisResultsGraphBuilder::UpdateNow()
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   pProgress->UpdateMessage(_T("Building Graph"));

   CWaitCursor wait;

   CGirderGraphBuilderBase::UpdateNow();

   // Update graph properties
   GroupIndexType    grpIdx      = m_pGraphController->GetGirderGroup();
   GirderIndexType   gdrIdx      = m_pGraphController->GetGirder();
   IntervalIndexType intervalIdx = m_pGraphController->GetInterval();
   ActionType        actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();

   UpdateYAxisUnits(actionType);
   UpdateXAxisTitle(intervalIdx);
   UpdateGraphTitle(grpIdx,gdrIdx,intervalIdx,actionType);

   // get data to graph
   UpdateGraphData(grpIdx,gdrIdx,intervalIdx,actionType);

   return true;
}

void CAnalysisResultsGraphBuilder::UpdateYAxisUnits(ActionType actionType)
{
   delete m_pYFormat;

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   switch(actionType)
   {
   case actionMoment:
      {
      const unitmgtMomentData& momentUnit = pDisplayUnits->GetMomentUnit();
      m_pYFormat = new MomentTool(momentUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Moment (") + ((MomentTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle);
      break;
      }
   case actionShear:
      {
      const unitmgtForceData& shearUnit = pDisplayUnits->GetShearUnit();
      m_pYFormat = new ShearTool(shearUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Shear (") + ((ShearTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle);
      break;
      }
   case actionDisplacement:
      {
      const unitmgtLengthData& displacementUnit = pDisplayUnits->GetDisplacementUnit();
      m_pYFormat = new DisplacementTool(displacementUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Displacement (") + ((DisplacementTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle);
      break;
      }
   case actionStress:
      {
      const unitmgtStressData& stressUnit = pDisplayUnits->GetStressUnit();
      m_pYFormat = new StressTool(stressUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Stress (") + ((StressTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle);
      break;
      }
   default:
      ASSERT(0); 
   }
}

void CAnalysisResultsGraphBuilder::UpdateXAxisTitle(IntervalIndexType intervalIdx)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(CSegmentKey(0,0,0));
   if ( intervalIdx == releaseIntervalIdx )
   {
      m_Graph.SetXAxisTitle(_T("Distance From Left End of Girder (")+m_pXFormat->UnitTag()+_T(")"));
   }
   else
   {
      m_Graph.SetXAxisTitle(_T("Distance From CL Bearing at Left End of Girder (")+m_pXFormat->UnitTag()+_T(")"));
   }
}

void CAnalysisResultsGraphBuilder::UpdateGraphTitle(GroupIndexType grpIdx,GirderIndexType gdrIdx,IntervalIndexType intervalIdx,ActionType actionType)
{
   CString strAction;
   switch(actionType)
   {
   case actionMoment:
      strAction = _T("Moments");
      break;
   case actionShear:
      strAction = _T("Shears");
      break;
   case actionDisplacement:
      strAction = _T("Deflections");
      break;
   case actionStress:
      strAction = _T("Stresses");
      break;
   default:
      ASSERT(0);
   }

   GET_IFACE(IDocumentType,pDocType);

   GET_IFACE(IIntervals,pIntervals);
   CString strInterval( pIntervals->GetDescription(intervalIdx) );

   CString strGraphTitle;
   if ( grpIdx == ALL_GROUPS )
   {
      strGraphTitle.Format(_T("Girder Line %s - Interval %d: %s - %s"),LABEL_GIRDER(gdrIdx),LABEL_INTERVAL(intervalIdx),strInterval,strAction);
   }
   else
   {
      if ( pDocType->IsPGSuperDocument() )
      {
         strGraphTitle.Format(_T("Span %d Girder %s - Interval %d: %s - %s"),LABEL_SPAN(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_INTERVAL(intervalIdx),strInterval,strAction);
      }
      else
      {
         strGraphTitle.Format(_T("Group %d Girder %s - Interval %d: %s - %s"),LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),LABEL_INTERVAL(intervalIdx),strInterval,strAction);
      }
   }
   
   m_Graph.SetTitle(std::_tstring(strGraphTitle));

   if ( pDocType->IsPGSuperDocument() )
   {
      CString strSubtitle;
      switch ( GetAnalysisType() )
      {
      case pgsTypes::Simple:
         strSubtitle = _T("Simple span");
         break;

      case pgsTypes::Continuous:
         strSubtitle = _T("Simple made continuous");
         break;

      case pgsTypes::Envelope:
         strSubtitle = _T("Envelope of Simple span and simple made continuous");
         break;
      }

      m_Graph.SetSubtitle(std::_tstring(strSubtitle));
   }
}

void CAnalysisResultsGraphBuilder::UpdateGraphData(GroupIndexType grpIdx,GirderIndexType gdrIdx,IntervalIndexType intervalIdx,ActionType actionType)
{
   // clear graph
   m_Graph.ClearData();

   // Get the points of interest we need.
   GET_IFACE(IPointOfInterest,pIPoi);
   CSegmentKey segmentKey(grpIdx,gdrIdx,ALL_SEGMENTS);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( segmentKey ) );

   // The vector of POIs contains all the POIs for the girder... if this is before the
   // segments are made continuous, we don't want the POIs at the closure pours or the CL Piers.
   // Evaluate the girder and the interval to determine if the segments are still simple spans.
   GET_IFACE(IIntervals,pIntervals);
   bool bSimpleSpanSegments = true;
   GET_IFACE(IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (grpIdx == ALL_GROUPS ? 0 : grpIdx);
   GroupIndexType endGroupIdx = (grpIdx == ALL_GROUPS ? nGroups-1 : min(nGroups-1,startGroupIdx+1));
   for ( GroupIndexType groupIdx = startGroupIdx; groupIdx <= endGroupIdx; groupIdx++ )
   {
      SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(groupIdx,gdrIdx));
      if ( 1 < nSegments )
      {
         for ( SegmentIndexType segIdx = 0; segIdx < nSegments-1; segIdx++ )
         {
            CClosureKey closureKey(groupIdx,gdrIdx,segIdx);
            if ( pIntervals->GetCompositeClosurePourInterval(closureKey) <= intervalIdx )
            {
               bSimpleSpanSegments = false;
               break;
            }
         }
      }
   }

   // if we got this far either the interval is before any continuity has occured
   // or the girder groups have one segment each which means this is a precast girder
   // bridge.... if this is a conventional precast girder bridge, check to see if the
   // deck is composite with the girders (in which case we can assume continuity has occured)
   if ( bSimpleSpanSegments )
   {
      if ( pIntervals->GetCompositeDeckInterval() <= intervalIdx )
         bSimpleSpanSegments = false;
   }

   if ( bSimpleSpanSegments )
   {
      pIPoi->RemovePointsOfInterest(vPoi,POI_CLOSURE);
      pIPoi->RemovePointsOfInterest(vPoi,POI_PIER);
   }

   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(CSegmentKey(grpIdx,gdrIdx,0));


   // Map POI coordinates to X-values for the graph
   std::vector<Float64> xVals;
   GetXValues(vPoi,xVals);

   std::vector<pgsPointOfInterest> vPoi2;
   std::vector<Float64> xVals2(xVals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   if ( actionType == actionShear && liveLoadIntervalIdx <= intervalIdx )
   {
      GET_IFACE(IShearCapacity,pShearCapacity);

      std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
      std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
      for ( ; iter != end; iter++ )
      {
         pgsPointOfInterest& poi = *iter;
         ZoneIndexType csZoneIdx = pShearCapacity->GetCriticalSectionZoneIndex(pgsTypes::StrengthI,poi);
         if ( csZoneIdx != INVALID_INDEX )
         {
#if defined _DEBUG
            Float64 start,end;
            pShearCapacity->GetCriticalSectionZoneBoundary(pgsTypes::StrengthI,poi.GetSegmentKey(),csZoneIdx,&start,&end);
            Float64 Xg = pIPoi->ConvertPoiToGirderCoordinate(poi);
            ATLASSERT( ::InRange(start,Xg,end) );
#endif

            std::vector<pgsPointOfInterest> vCSPoi(pIPoi->GetCriticalSections(pgsTypes::StrengthI,poi.GetSegmentKey()));

            //if ( poi.IsTenthPoint(POI_ERECTED_SEGMENT) == 1 )
            //{
            //   // POI is at the CL Bearing at the start of the segment... there is a jump is shear
            //   // so we want both the regular shear and the critical section shear here
            //   Float64 X = xVals2.at(vPoi2.size());
            //   xVals2.insert(xVals2.begin()+vPoi2.size(),X);
            //   vPoi2.push_back(poi);
            //   vPoi2.push_back(vCSPoi[csZoneIdx]);
            //}
            //else if ( poi.IsTenthPoint(POI_ERECTED_SEGMENT) == 11 )
            //{
            //   Float64 X = xVals2.at(vPoi2.size());
            //   xVals2.insert(xVals2.begin()+vPoi2.size(),X);
            //   vPoi2.push_back(vCSPoi[csZoneIdx]);
            //   vPoi2.push_back(poi);
            //}
            //else
            {
               vPoi2.push_back(vCSPoi[csZoneIdx]);
            }
         }
         else
         {
            vPoi2.push_back(poi);
         }
      }
   }


   IndexType nGraphs = m_pGraphController->GetGraphCount();
   for ( IndexType graphIdx = 0; graphIdx < nGraphs; graphIdx++ )
   {
      IDType graphID = ((CAnalysisResultsGraphController*)m_pGraphController)->SelectedGraphIndexToGraphID(graphIdx);
      const CAnalysisResultsGraphDefinition& graphDef = m_pGraphDefinitions->GetGraphDefinition(graphID);
      switch( graphDef.m_GraphType )
      {
      case graphCombined:
         CombinedLoadGraph(graphDef,intervalIdx,actionType,vPoi,xVals);
         if ( actionType == actionShear && liveLoadIntervalIdx <= intervalIdx )
            CombinedLoadGraph(graphDef,intervalIdx,actionType,vPoi2,xVals2,true);
         break;

      case graphLiveLoad:
         LiveLoadGraph(graphDef,intervalIdx,actionType,vPoi,xVals);
         //if ( actionType == actionShear && liveLoadIntervalIdx <= intervalIdx )
         //   LiveLoadGraph(graphDef,intervalIdx,actionType,vPoi2,xVals2,true);
         break;

      case graphVehicularLiveLoad:
         VehicularLiveLoadGraph(graphDef,intervalIdx,actionType,vPoi,xVals);
         //if ( actionType == actionShear && liveLoadIntervalIdx <= intervalIdx )
         //   VehicularLiveLoadGraph(graphDef,intervalIdx,actionType,vPoi2,xVals2,true);
         break;

      case graphProduct:
         ProductLoadGraph(graphDef,intervalIdx,actionType,vPoi,xVals);
         if ( actionType == actionShear && liveLoadIntervalIdx <= intervalIdx )
            ProductLoadGraph(graphDef,intervalIdx,actionType,vPoi2,xVals2,true);
         break;

      case graphPrestress:
         PrestressLoadGraph(graphDef,intervalIdx,actionType,vPoi,xVals);
         break;

      case graphPostTension:
         PostTensionLoadGraph(graphDef,intervalIdx,actionType,vPoi,xVals);
         break;

      case graphAllowable:
         if(intervalIdx == releaseIntervalIdx && actionType == actionStress)
         {
            // Casting yard stress is its own animal
            CyStressCapacityGraph(graphDef,intervalIdx,actionType,vPoi,xVals);
            break;
         }

      case graphLimitState:
      case graphDemand:
      case graphCapacity:
      case graphMinCapacity:
         LimitStateLoadGraph(graphDef,intervalIdx,actionType,vPoi,xVals);
         //if ( actionType == actionShear && liveLoadIntervalIdx <= intervalIdx )
         //   LimitStateLoadGraph(graphDef,intervalIdx,actionType,vPoi2,xVals2,true);
         break;

      default:
         ASSERT(false); // should never get here
      }
   }
 }

void CAnalysisResultsGraphBuilder::InitializeGraph(const CAnalysisResultsGraphDefinition& graphDef,ActionType actionType,IntervalIndexType intervalIdx,bool bIsFinalShear,IndexType* pDataSeriesID,pgsTypes::BridgeAnalysisType* pBAT,IndexType* pAnalysisTypeCount)
{
   CString strDataLabel(graphDef.m_Name);
   COLORREF c(graphDef.m_Color);

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   if (actionType == actionShear )
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      if (actionType == actionShear && !bIsFinalShear)
         strDataLabel = _T("");
      int penStyle = (liveLoadIntervalIdx <= intervalIdx && actionType == actionShear && !bIsFinalShear ? PS_DOT : PS_SOLID);

      if ( analysisType == pgsTypes::Envelope )
      {
         *pAnalysisTypeCount = 2;
         pDataSeriesID[0] = m_Graph.CreateDataSeries(strDataLabel, penStyle, 1, c);
         pDataSeriesID[1] = m_Graph.CreateDataSeries(_T(""), penStyle, 1, c);

         pBAT[0] = pgsTypes::MinSimpleContinuousEnvelope;
         pBAT[1] = pgsTypes::MaxSimpleContinuousEnvelope;
      }
      else
      {
         *pAnalysisTypeCount = 1;
         pDataSeriesID[0] = m_Graph.CreateDataSeries(strDataLabel, penStyle, 1, c);

         pBAT[0] = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
      }
   }
   else if ( actionType == actionMoment ||
             actionType == actionDisplacement )
   {
      // For moments and deflections
      if ( analysisType == pgsTypes::Envelope )
      {
         *pAnalysisTypeCount = 2;
         pDataSeriesID[0] = m_Graph.CreateDataSeries(strDataLabel, PS_SOLID, 1, c);
         pDataSeriesID[1] = m_Graph.CreateDataSeries(_T(""), PS_SOLID, 1, c);

         pBAT[0] = pgsTypes::MinSimpleContinuousEnvelope;
         pBAT[1] = pgsTypes::MaxSimpleContinuousEnvelope;
      }
      else
      {
         *pAnalysisTypeCount = 1;
         pDataSeriesID[0] = m_Graph.CreateDataSeries(strDataLabel, PS_SOLID, 1, c);

         pBAT[0] = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
      }
   }
   else if (actionType == actionStress)
   {
      // for stresses

      if ( analysisType == pgsTypes::Envelope )
      {
         *pAnalysisTypeCount = 1;
         pDataSeriesID[0] = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top"),    PS_STRESS_TOP,    1, c);
         pDataSeriesID[1] = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom"), PS_STRESS_BOTTOM, 1, c);

         pBAT[0] = pgsTypes::MaxSimpleContinuousEnvelope;
      }
      else
      {
         *pAnalysisTypeCount = 2;
         pDataSeriesID[0] = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top"),    PS_STRESS_TOP,    1, c);
         pDataSeriesID[1] = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom"), PS_STRESS_BOTTOM, 1, c);

         pBAT[0] = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
         pBAT[1] = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
      }
   }
   else
   {
      ATLASSERT(false); // is there a new action type?
   }
}

void CAnalysisResultsGraphBuilder::CombinedLoadGraph(const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,ActionType action,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear)
{
   // Combined forces
   GET_IFACE(IPointOfInterest,pIPOI);
   GET_IFACE(ICombinedForces2,pForces);
   LoadingCombination combination_type(graphDef.m_LoadType.CombinedLoadType);

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   IndexType data_series_id[4];
   pgsTypes::BridgeAnalysisType bat[4];
   IndexType nAnalysisTypes;
   InitializeGraph(graphDef,action,intervalIdx,bIsFinalShear,data_series_id,bat,&nAnalysisTypes);

   for ( IndexType analysisIdx = 0; analysisIdx < nAnalysisTypes; analysisIdx++ )
   {
      switch(action)
      {
      case actionMoment:
         {
         std::vector<Float64> moments = pForces->GetMoment( combination_type, intervalIdx, vPoi, ctCummulative, bat[analysisIdx] );
         AddGraphPoints(data_series_id[analysisIdx], xVals, moments);
         break;
         }
      case actionShear:
         {
         std::vector<sysSectionValue> shear = pForces->GetShear( combination_type, intervalIdx, vPoi, ctCummulative, bat[analysisIdx] );
         AddGraphPoints(data_series_id[analysisIdx], xVals, shear);
         break;
         }
      case actionDisplacement:
         {
         std::vector<Float64> displ = pForces->GetDisplacement( combination_type, intervalIdx, vPoi, ctCummulative, bat[analysisIdx] );
         AddGraphPoints(data_series_id[analysisIdx], xVals, displ);
         break;
         }
      case actionStress:
         {
         std::vector<Float64> fTop, fBot;
         pForces->GetStress( combination_type, intervalIdx, vPoi, ctCummulative, bat[analysisIdx], &fTop, &fBot );
         AddGraphPoints(data_series_id[2*analysisIdx],   xVals, fTop);
         AddGraphPoints(data_series_id[2*analysisIdx+1], xVals, fBot);
         break;
         }
      }
   }
}

void CAnalysisResultsGraphBuilder::LiveLoadGraph(const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,ActionType actionType,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear)
{
   // Live Load
   GET_IFACE(ICombinedForces2,pForces);
   GET_IFACE(IPointOfInterest,pIPOI);

   pgsTypes::LiveLoadType llType( graphDef.m_LoadType.LiveLoadType );

   CString strDataLabel(graphDef.m_Name);
   strDataLabel += _T(" (per girder)");

   COLORREF c(graphDef.m_Color);

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   VehicleIndexType vehicleIndex(graphDef.m_VehicleIndex);
   ATLASSERT(vehicleIndex == INVALID_INDEX);

   // data series for moment, shears and deflections
   IndexType min_data_series;
   IndexType max_data_series;
   if (actionType == actionShear )
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      if (actionType == actionShear && !bIsFinalShear)
         strDataLabel = _T("");
      int penStyle = (liveLoadIntervalIdx <= intervalIdx && actionType == actionShear && !bIsFinalShear ? PS_DOT : PS_SOLID);

      min_data_series = m_Graph.CreateDataSeries(strDataLabel, penStyle, 1, c);
      max_data_series = m_Graph.CreateDataSeries(_T(""), penStyle, 1, c);
   }
   else
   {
      min_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,1,c);
      max_data_series = m_Graph.CreateDataSeries(_T(""),PS_SOLID,1,c);
   }

   // data series for stresses
   IndexType stress_top_max = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top"),   PS_STRESS_TOP,   1,c);
   IndexType stress_top_min = m_Graph.CreateDataSeries(_T(""),                      PS_STRESS_TOP,   1,c);
   IndexType stress_bot_max = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom"),PS_STRESS_BOTTOM,1,c);
   IndexType stress_bot_min = m_Graph.CreateDataSeries(_T(""),                      PS_STRESS_BOTTOM,1,c);

   switch(actionType)
   {
   case actionMoment:
      {
         std::vector<Float64> Mmin, Mmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            pForces->GetCombinedLiveLoadMoment(llType, intervalIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &Mmin, &Mmax);
            AddGraphPoints(min_data_series, xVals, Mmin);

            pForces->GetCombinedLiveLoadMoment(llType, intervalIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &Mmin, &Mmax);
            AddGraphPoints(max_data_series, xVals, Mmax);
         }
         else
         {
            pForces->GetCombinedLiveLoadMoment(llType, intervalIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &Mmin, &Mmax);
            AddGraphPoints(min_data_series, xVals, Mmin);
            AddGraphPoints(max_data_series, xVals, Mmax);
         }
      break;

      }
   case actionShear:
      {
         std::vector<sysSectionValue> Vmin, Vmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            pForces->GetCombinedLiveLoadShear(llType, intervalIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, &Vmin, &Vmax);
            AddGraphPoints(min_data_series, xVals, Vmin);

            pForces->GetCombinedLiveLoadShear(llType, intervalIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, &Vmin, &Vmax);
            AddGraphPoints(max_data_series, xVals, Vmax);
         }
         else
         {
            pForces->GetCombinedLiveLoadShear(llType, intervalIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, &Vmin, &Vmax);
            AddGraphPoints(min_data_series, xVals, Vmin);
            AddGraphPoints(max_data_series, xVals, Vmax);
         }
      break;
      }
   case actionDisplacement:
      {
         std::vector<Float64> Dmin, Dmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            pForces->GetCombinedLiveLoadDisplacement(llType, intervalIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &Dmin, &Dmax);
            AddGraphPoints(min_data_series, xVals, Dmin);

            pForces->GetCombinedLiveLoadDisplacement(llType, intervalIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &Dmin, &Dmax);
            AddGraphPoints(max_data_series, xVals, Dmax);
         }
         else
         {
            pForces->GetCombinedLiveLoadDisplacement(llType, intervalIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &Dmin, &Dmax);
            AddGraphPoints(min_data_series, xVals, Dmin);
            AddGraphPoints(max_data_series, xVals, Dmax);
         }
      break;
      }
   case actionStress:
      {
         std::vector<Float64> fTopMin,fTopMax,fBotMin,fBotMax;
         if ( analysisType == pgsTypes::Envelope )
         {
            pForces->GetCombinedLiveLoadStress(llType, intervalIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
            AddGraphPoints(stress_top_min, xVals, fTopMin);
            AddGraphPoints(stress_bot_min, xVals, fBotMin);
            AddGraphPoints(stress_top_max, xVals, fTopMax);
            AddGraphPoints(stress_bot_max, xVals, fBotMax);
         }
         else
         {
            pForces->GetCombinedLiveLoadStress(llType, intervalIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
            AddGraphPoints(stress_top_min, xVals, fTopMin);
            AddGraphPoints(stress_bot_min, xVals, fBotMin);
            AddGraphPoints(stress_top_max, xVals, fTopMax);
            AddGraphPoints(stress_bot_max, xVals, fBotMax);
         }
      break;
      }
   }
}

void CAnalysisResultsGraphBuilder::VehicularLiveLoadGraph(const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,ActionType actionType,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear)
{
   // Live Load
   GET_IFACE(IProductForces2,pForces);
   GET_IFACE(IPointOfInterest,pIPOI);

   CString strDataLabel(graphDef.m_Name);
   strDataLabel += _T(" (per lane)");

   COLORREF c(graphDef.m_Color);

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   pgsTypes::LiveLoadType llType(graphDef.m_LoadType.LiveLoadType);
   VehicleIndexType vehicleIndex(graphDef.m_VehicleIndex);

   // data series for moment, shears and deflections
   IndexType min_data_series;
   IndexType max_data_series;
   if (actionType == actionShear )
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      if (actionType == actionShear && !bIsFinalShear)
         strDataLabel = _T("");
      int penStyle = (liveLoadIntervalIdx <= intervalIdx && actionType == actionShear && !bIsFinalShear ? PS_DOT : PS_SOLID);

      min_data_series = m_Graph.CreateDataSeries(strDataLabel, penStyle, 1, c);
      max_data_series = m_Graph.CreateDataSeries(_T(""), penStyle, 1, c);
   }
   else
   {
      min_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,1,c);
      max_data_series = m_Graph.CreateDataSeries(_T(""),PS_SOLID,1,c);
   }

   // data series for stresses
   IndexType stress_top_max = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top"),   PS_STRESS_TOP,   1,c);
   IndexType stress_top_min = m_Graph.CreateDataSeries(_T(""),                      PS_STRESS_TOP,   1,c);
   IndexType stress_bot_max = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom"),PS_STRESS_BOTTOM,1,c);
   IndexType stress_bot_min = m_Graph.CreateDataSeries(_T(""),                      PS_STRESS_BOTTOM,1,c);

   switch(actionType)
   {
   case actionMoment:
      {
         std::vector<Float64> Mmin, Mmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadMoment(llType, vehicleIndex, intervalIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Mmin, &Mmax);
            else
               pForces->GetLiveLoadMoment(llType, intervalIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Mmin, &Mmax);

            AddGraphPoints(min_data_series, xVals, Mmin);

            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadMoment(llType, vehicleIndex, intervalIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Mmin, &Mmax);
            else
               pForces->GetLiveLoadMoment(llType, intervalIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Mmin, &Mmax);

            AddGraphPoints(max_data_series, xVals, Mmax);
         }
         else
         {
            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadMoment(llType, vehicleIndex, intervalIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Mmin, &Mmax, NULL, NULL);
            else
               pForces->GetLiveLoadMoment(llType, intervalIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Mmin, &Mmax, NULL, NULL);

            AddGraphPoints(min_data_series, xVals, Mmin);
            AddGraphPoints(max_data_series, xVals, Mmax);
         }
      break;

      }
   case actionShear:
      {
         std::vector<sysSectionValue> Vmin, Vmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadShear(llType, vehicleIndex, intervalIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Vmin, &Vmax);
            else
               pForces->GetLiveLoadShear(llType, intervalIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Vmin, &Vmax);
 
            AddGraphPoints(min_data_series, xVals, Vmin);

            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadShear(llType, vehicleIndex, intervalIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Vmin, &Vmax);
            else
               pForces->GetLiveLoadShear(llType, intervalIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Vmin, &Vmax);

            AddGraphPoints(max_data_series, xVals, Vmax);
         }
         else
         {
            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadShear(llType, vehicleIndex, intervalIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Vmin, &Vmax);
            else
               pForces->GetLiveLoadShear(llType, intervalIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Vmin, &Vmax);

            AddGraphPoints(min_data_series, xVals, Vmin);
            AddGraphPoints(max_data_series, xVals, Vmax);
         }
      break;
      }
   case actionDisplacement:
      {
         std::vector<Float64> Dmin, Dmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadDisplacement(llType, vehicleIndex, intervalIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Dmin, &Dmax);
            else
               pForces->GetLiveLoadDisplacement(llType, intervalIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Dmin, &Dmax);

            AddGraphPoints(min_data_series, xVals, Dmin);

            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadDisplacement(llType, vehicleIndex, intervalIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Dmin, &Dmax);
            else
               pForces->GetLiveLoadDisplacement(llType, intervalIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Dmin, &Dmax);

            AddGraphPoints(max_data_series, xVals, Dmax);
         }
         else
         {
            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadDisplacement(llType, vehicleIndex, intervalIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Dmin, &Dmax);
            else
               pForces->GetLiveLoadDisplacement(llType, intervalIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Dmin, &Dmax);

            AddGraphPoints(min_data_series, xVals, Dmin);
            AddGraphPoints(max_data_series, xVals, Dmax);
         }
      break;
      }
   case actionStress:
      {
         std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;
         if ( analysisType == pgsTypes::Envelope )
         {
            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadStress(llType, vehicleIndex, intervalIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
            else
               pForces->GetLiveLoadStress(llType, intervalIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &fTopMin, &fTopMax, &fBotMin, &fBotMax );

            AddGraphPoints(stress_top_min, xVals, fTopMin);
            AddGraphPoints(stress_bot_min, xVals, fBotMin);
            AddGraphPoints(stress_top_max, xVals, fTopMax);
            AddGraphPoints(stress_bot_max, xVals, fBotMax);
         }
         else
         {
            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadStress(llType, vehicleIndex, intervalIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
            else
               pForces->GetLiveLoadStress(llType, intervalIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &fTopMin, &fTopMax, &fBotMin, &fBotMax );

            AddGraphPoints(stress_top_min, xVals, fTopMin);
            AddGraphPoints(stress_bot_min, xVals, fBotMin);
            AddGraphPoints(stress_top_max, xVals, fTopMax);
            AddGraphPoints(stress_bot_max, xVals, fBotMax);
         }
      break;
      }
   }
}

void CAnalysisResultsGraphBuilder::ProductLoadGraph(const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,ActionType actionType,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear)
{
   ProductForceType prod_type(graphDef.m_LoadType.ProductLoadType);
   
   // Product forces
   GET_IFACE(IPointOfInterest,pIPOI);   
   GET_IFACE(IProductForces2,pForces);

   IndexType data_series_id[4];
   pgsTypes::BridgeAnalysisType bat[4];
   IndexType nAnalysisTypes;
   InitializeGraph(graphDef,actionType,intervalIdx,bIsFinalShear,data_series_id,bat,&nAnalysisTypes);

   for ( IndexType analysisIdx = 0; analysisIdx < nAnalysisTypes; analysisIdx++ )
   {
      switch(actionType)
      {
      case actionMoment:
         {
            std::vector<Float64> moments( pForces->GetMoment( intervalIdx, prod_type, vPoi, bat[analysisIdx]) );
            AddGraphPoints(data_series_id[analysisIdx], xVals, moments);
            break;
         }
      case actionShear:
         {
            std::vector<sysSectionValue> shears( pForces->GetShear( intervalIdx, prod_type, vPoi, bat[analysisIdx]) );
            AddGraphPoints(data_series_id[analysisIdx], xVals, shears);
            break;
         }
      case actionDisplacement:
         {
            std::vector<Float64> displacements( pForces->GetDisplacement( intervalIdx, prod_type, vPoi, bat[analysisIdx]) );
            AddGraphPoints(data_series_id[analysisIdx], xVals, displacements);
            break;
         }
      case actionStress:
         {
            std::vector<Float64> fTop, fBot;
            pForces->GetStress( intervalIdx, prod_type, vPoi, bat[analysisIdx], &fTop, &fBot);
            AddGraphPoints(data_series_id[2*analysisIdx], xVals, fTop);
            AddGraphPoints(data_series_id[2*analysisIdx+1], xVals, fBot);
            break;
         }
      }
   }
}

void CAnalysisResultsGraphBuilder::PrestressLoadGraph(const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,ActionType action,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals)
{
   // Prestress
   GET_IFACE(IPointOfInterest,pIPOI);
   GET_IFACE(IPretensionStresses,pPrestress);

   CString strDataLabel(graphDef.m_Name);

   COLORREF c(graphDef.m_Color);

   IndexType deflection = m_Graph.CreateDataSeries(strDataLabel,   PS_SOLID,   1,c);
   IndexType stress_top = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top Girder"),   PS_STRESS_TOP,   1,c);
   IndexType stress_bot = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom"),PS_STRESS_BOTTOM,1,c);

   std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   std::vector<Float64>::const_iterator xIter(xVals.begin());
   for ( ; i != end; i++, xIter++ )
   {
      const pgsPointOfInterest& poi = *i;
      Float64 x = *xIter;
      
      switch(action)
      {
      case actionMoment:
      case actionShear:
         ATLASSERT(false); // should never get here
         break;

      case actionDisplacement:
         {
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(poi.GetSegmentKey());
            GET_IFACE(ICamber,pCamber);
            bool bRelativeToBearings = (intervalIdx == releaseIntervalIdx ? false : true);
            Float64 dy = pCamber->GetPrestressDeflection(poi,bRelativeToBearings);
            AddGraphPoint(deflection, x, dy);
         }
         break;

      case actionStress:
         {
            Float64 fTop,fBot;
            fTop = pPrestress->GetStress(intervalIdx,poi,pgsTypes::TopGirder);
            fBot = pPrestress->GetStress(intervalIdx,poi,pgsTypes::BottomGirder);
            AddGraphPoint(stress_top, x, fTop);
            AddGraphPoint(stress_bot, x, fBot);
         break;
         }
      }
   }
}

void CAnalysisResultsGraphBuilder::CyStressCapacityGraph(const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,ActionType action,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals)
{
   pgsTypes::LimitState limitState = pgsTypes::ServiceI;

   CString strDataLabel(graphDef.m_Name);

   COLORREF c(graphDef.m_Color);
   int pen_size = 3;

   // data series for moment, shears and deflections
   IndexType max_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,pen_size,c);
   IndexType min_data_series = m_Graph.CreateDataSeries(_T(""),      PS_SOLID,pen_size,c);

   // Allowable tension in cy is dependent on capacity - must get spec check results
   // First get pois using same request as spec check report
   GET_IFACE(IArtifact,pIArtifact);

   Float64 cap_prev, x_prev; // tension capacity can jump at a location. we must capture this
   bool first(true);
   std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   std::vector<Float64>::const_iterator xIter(xVals.begin());
   for ( ; i != end; i++, xIter++ )
   {
      const pgsPointOfInterest& poi = *i;
      Float64 x = *xIter;

      const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(poi.GetSegmentKey());

      const pgsFlexuralStressArtifact* pMaxArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(intervalIdx,limitState,pgsTypes::Tension,    poi.GetID());
      const pgsFlexuralStressArtifact* pMinArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(intervalIdx,limitState,pgsTypes::Compression,poi.GetID());
      Float64 maxcap, mincap;
      if (pMaxArtifact != NULL)
      {
         // compression is easy
         mincap = pMinArtifact->GetCapacity();
         AddGraphPoint(min_data_series, x, mincap);

         // Tension - we must catch jumps
         // Use a simple rule: Jumps happen at starts/ends of high points
         maxcap = pMaxArtifact->GetCapacity();
         if (!first && !IsEqual(maxcap,cap_prev))
         {
            if (cap_prev < maxcap)
            {
               // We are going up hill. Jump is at this location
               AddGraphPoint(max_data_series, x, cap_prev);
            }
            else
            {
               // We went down hill. Jump was at last location
               AddGraphPoint(max_data_series, x_prev, maxcap);
            }
         }

         AddGraphPoint(max_data_series, x, maxcap);

         cap_prev = maxcap;
         x_prev = x;
      }

      first = false;
   }
}

void CAnalysisResultsGraphBuilder::PostTensionLoadGraph(const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,ActionType action,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals)
{
   // Post-Tensioning
   GET_IFACE(IPointOfInterest,pIPOI);
   GET_IFACE(IPosttensionStresses,     pPosttensionStresses);

   CString strDataLabel(graphDef.m_Name);

   COLORREF c(graphDef.m_Color);

   IndexType deflection = m_Graph.CreateDataSeries(strDataLabel,   PS_SOLID,   1,c);
   IndexType stress_top = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top"),   PS_STRESS_TOP,   1,c);
   IndexType stress_bot = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom"),PS_STRESS_BOTTOM,1,c);

   std::vector<pgsPointOfInterest>::const_iterator i(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   std::vector<Float64>::const_iterator xIter(xVals.begin());
   for ( ; i != end; i++, xIter++ )
   {
      const pgsPointOfInterest& poi = *i;
      Float64 x = *xIter;
      
      switch(action)
      {
      case actionMoment:
      case actionShear:
      case actionDisplacement:
         ATLASSERT(false); // should never get here
         break;

      //case actionDisplacement:
      //   {
      //      GET_IFACE(IIntervals,pIntervals);
      //      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(poi.GetSegmentKey());
      //      GET_IFACE(ICamber,pCamber);
      //      bool bRelativeToBearings = (intervalIdx == releaseIntervalIdx ? false : true);
      //      Float64 dy = pCamber->GetPrestressDeflection(poi,bRelativeToBearings);
      //      AddGraphPoint(deflection, x, dy);
      //   }
      //   break;

      case actionStress:
         {
            Float64 fTopPosttension, fBotPosttension;
            fTopPosttension = pPosttensionStresses->GetStress(intervalIdx,poi,pgsTypes::TopGirder,   ALL_DUCTS);
            fBotPosttension = pPosttensionStresses->GetStress(intervalIdx,poi,pgsTypes::BottomGirder,ALL_DUCTS);

            AddGraphPoint(stress_top, x, fTopPosttension);
            AddGraphPoint(stress_bot, x, fBotPosttension);
         break;
         }
      }
   }
}
void CAnalysisResultsGraphBuilder::LimitStateLoadGraph(const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,ActionType action,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear)
{
   pgsTypes::LimitState limitState(graphDef.m_LoadType.LimitStateType);
   GraphType graphType(graphDef.m_GraphType);

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   GET_IFACE(IBridge,pBridge);

   // Combined forces
   GET_IFACE(ILimitStateForces2,pForces);

   CString strDataLabel(graphDef.m_Name);

   COLORREF c(graphDef.m_Color);
   int pen_size = (graphType == graphAllowable || graphType == graphCapacity ? 3 : 1);

   // data series for moment, shears and deflections
   IndexType max_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,pen_size,c);
   IndexType min_data_series = m_Graph.CreateDataSeries(_T(""),          PS_SOLID,pen_size,c);

   // data series for stresses
   IndexType stress_top_max = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top"),    PS_STRESS_TOP,   1,c);
   IndexType stress_top_min = m_Graph.CreateDataSeries(_T(""),                       PS_STRESS_TOP,   1,c);
   IndexType stress_bot_max = m_Graph.CreateDataSeries(strDataLabel+_T(" -  Bottom"),PS_STRESS_BOTTOM,1,c);
   IndexType stress_bot_min = m_Graph.CreateDataSeries(_T(""),                       PS_STRESS_BOTTOM,1,c);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   switch(action)
   {
   case actionMoment:
      {
         if ( graphType == graphCapacity )
         {
            GET_IFACE(IMomentCapacity,pCapacity);
            std::vector<Float64> pMn = pCapacity->GetMomentCapacity(intervalIdx,vPoi,true);
            AddGraphPoints(max_data_series, xVals, pMn);

            std::vector<Float64> nMn = pCapacity->GetMomentCapacity(intervalIdx,vPoi,false);
            AddGraphPoints(min_data_series, xVals, nMn);
         }
         else if ( graphType == graphMinCapacity )
         {
            GET_IFACE(IMomentCapacity,pCapacity);
            std::vector<Float64> pMrMin = pCapacity->GetMinMomentCapacity(intervalIdx,vPoi,true);
            AddGraphPoints(max_data_series, xVals, pMrMin);

            std::vector<Float64> nMrMin = pCapacity->GetMinMomentCapacity(intervalIdx,vPoi,false);
            AddGraphPoints(min_data_series, xVals, nMrMin);
         }
         else
         {
            if ( analysisType == pgsTypes::Envelope )
            {
               std::vector<Float64> mmax, mmin;
               pForces->GetMoment( limitState, intervalIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &mmin, &mmax );
               AddGraphPoints(max_data_series, xVals, mmax);
               
               if ( liveLoadIntervalIdx <= intervalIdx && IsStrengthLimitState(limitState) )
                  mmin = pForces->GetSlabDesignMoment(limitState,vPoi, pgsTypes::MinSimpleContinuousEnvelope );
               else
                  pForces->GetMoment( limitState, intervalIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &mmin, &mmax );

               AddGraphPoints(min_data_series, xVals, mmin);
            }
            else
            {
               std::vector<Float64> mmax, mmin;
               pForces->GetMoment( limitState, intervalIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &mmin, &mmax );
               AddGraphPoints(max_data_series, xVals, mmax);

               if ( liveLoadIntervalIdx <= intervalIdx && IsStrengthLimitState(limitState) )
                  mmin = pForces->GetSlabDesignMoment(limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan );

               AddGraphPoints(min_data_series, xVals, mmin);
            }
         }
      break;
      }
   case actionShear:
      {
         if ( graphType == graphCapacity )
         {
            GET_IFACE(IShearCapacity,pCapacity);
            std::vector<Float64> pVn = pCapacity->GetShearCapacity(limitState, intervalIdx, vPoi);
            AddGraphPoints(max_data_series, xVals,  pVn);

            std::vector<Float64>::iterator iter;
            std::vector<Float64> nVn;
            for ( iter = pVn.begin(); iter != pVn.end(); iter++ )
            {
               nVn.push_back(-1*(*iter));
            }
            AddGraphPoints(min_data_series, xVals, nVn);
         }
         else
         {
            if ( analysisType == pgsTypes::Envelope )
            {
               std::vector<sysSectionValue> shearmn, shearmx;
               pForces->GetShear( limitState, intervalIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &shearmn, &shearmx );
               AddGraphPoints(min_data_series, xVals, shearmn);

               pForces->GetShear( limitState, intervalIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &shearmn, &shearmx );
               AddGraphPoints(max_data_series, xVals, shearmx);
            }
            else
            {
               std::vector<sysSectionValue> shearmn, shearmx;
               pForces->GetShear( limitState, intervalIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &shearmn, &shearmx );
               AddGraphPoints(min_data_series, xVals, shearmn);
               AddGraphPoints(max_data_series, xVals, shearmx);
            }
         }
      break;
      }
   case actionDisplacement:
      {
         if ( analysisType == pgsTypes::Envelope )
         {
            std::vector<Float64> dispmn, dispmx;
            pForces->GetDisplacement( limitState, intervalIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &dispmn, &dispmx);
            AddGraphPoints(min_data_series, xVals, dispmn);

            pForces->GetDisplacement( limitState, intervalIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &dispmn, &dispmx);
            AddGraphPoints(max_data_series, xVals, dispmx);
         }
         else
         {
            std::vector<Float64> dispmn, dispmx;
            pForces->GetDisplacement( limitState, intervalIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &dispmn, &dispmx);
            AddGraphPoints(min_data_series, xVals, dispmn);
            AddGraphPoints(max_data_series, xVals, dispmx);
         }
      break;
      }
   case actionStress:
      {
         GET_IFACE(IAllowableConcreteStress,pAllowable);

         if ( graphType == graphAllowable )
         {
            if ( limitState != pgsTypes::ServiceIII )
            {
               std::vector<Float64> c( pAllowable->GetAllowableStress(vPoi,intervalIdx,limitState,pgsTypes::Compression) );
               AddGraphPoints(max_data_series, xVals, c);
            }

            if ( limitState == pgsTypes::ServiceIII || intervalIdx < liveLoadIntervalIdx )
            {
               std::vector<Float64> t(pAllowable->GetAllowableStress(vPoi,intervalIdx,limitState,pgsTypes::Tension));
               AddGraphPoints(min_data_series, xVals, t);
            }
         }
         else
         {
            bool bIncPrestress = (graphType == graphDemand ? true : false);
            std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;

            if ( analysisType == pgsTypes::Envelope )
            {
               pForces->GetStress( limitState, intervalIdx, vPoi, pgsTypes::TopGirder,    bIncPrestress, pgsTypes::MaxSimpleContinuousEnvelope, &fTopMin, &fTopMax);
               pForces->GetStress( limitState, intervalIdx, vPoi, pgsTypes::BottomGirder, bIncPrestress, pgsTypes::MaxSimpleContinuousEnvelope, &fBotMin, &fBotMax);
            }
            else
            {
               pForces->GetStress( limitState, intervalIdx, vPoi, pgsTypes::TopGirder,    bIncPrestress, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &fTopMin, &fTopMax);
               pForces->GetStress( limitState, intervalIdx, vPoi, pgsTypes::BottomGirder, bIncPrestress, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &fBotMin, &fBotMax);
            }

            if ( intervalIdx <= castDeckIntervalIdx )
            {
               AddGraphPoints(stress_top_max, xVals, fTopMax);
               AddGraphPoints(stress_bot_max, xVals, fBotMax);
            }
            else if ( intervalIdx < liveLoadIntervalIdx )
            {
               AddGraphPoints(stress_top_max, xVals, fTopMax);
               AddGraphPoints(stress_bot_min, xVals, fBotMin);
            }
            else
            {
               if ( limitState == pgsTypes::ServiceI || limitState == pgsTypes::ServiceIA || limitState == pgsTypes::FatigueI )
               {
                  AddGraphPoints(stress_top_min, xVals, fTopMin);
                  AddGraphPoints(stress_bot_min, xVals, fBotMin);

                  m_Graph.SetDataLabel(stress_top_min,strDataLabel+_T(" - Top"));
                  m_Graph.SetDataLabel(stress_bot_min,strDataLabel+_T(" - Bottom"));
               }
               else
               {
                  if ( limitState != pgsTypes::ServiceIII)
                     AddGraphPoints(stress_top_max, xVals, fTopMax);

                  AddGraphPoints(stress_bot_max, xVals, fBotMax);
               }
            }
         }
      break;
      }
   }
}

pgsTypes::AnalysisType CAnalysisResultsGraphBuilder::GetAnalysisType()
{
   return ((CAnalysisResultsGraphController*)m_pGraphController)->GetAnalysisType();
}
