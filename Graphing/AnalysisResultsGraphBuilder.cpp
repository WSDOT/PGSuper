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

#include "stdafx.h"
#include "resource.h"
#include <Graphing\AnalysisResultsGraphBuilder.h>
#include "AnalysisResultsGraphController.h"
#include "AnalysisResultsGraphDefinition.h"
#include "AnalysisResultsGraphViewControllerImp.h"
#include "..\Documentation\PGSuper.hh"

#include "GraphColor.h"

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <UnitMgt\UnitValueNumericalFormatTools.h>
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
#include <IFace\PrestressForce.h>

#include <EAF\EAFGraphView.h>
#include <EAF\EAFDocument.h>

#include <PgsExt\ClosureJointData.h>

#include <MFCTools\MFCTools.h>

#include <WBFLSTL.h>

#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// create a dummy unit conversion tool to pacify the graph constructor
static unitmgtLengthData DUMMY(unitMeasure::Meter);
static LengthTool        DUMMY_TOOL(DUMMY);

// Pen styles for stresses at top and bottom of girder
#define PS_STRESS_TOP_GIRDER     PS_SOLID
#define PS_STRESS_BOTTOM_GIRDER  PS_DASH
#define PS_STRESS_TOP_DECK       PS_DOT
#define PS_STRESS_BOTTOM_DECK    PS_DASHDOT

BEGIN_MESSAGE_MAP(CAnalysisResultsGraphBuilder, CGirderGraphBuilderBase)
END_MESSAGE_MAP()


CAnalysisResultsGraphBuilder::CAnalysisResultsGraphBuilder() :
CGirderGraphBuilderBase(),
m_pGraphColor(new grGraphColor),
m_pGraphDefinitions(new CAnalysisResultsGraphDefinitions)
{
   Init();
}

CAnalysisResultsGraphBuilder::CAnalysisResultsGraphBuilder(const CAnalysisResultsGraphBuilder& other) :
CGirderGraphBuilderBase(other),
m_pGraphColor(new grGraphColor),
m_pGraphDefinitions(new CAnalysisResultsGraphDefinitions)
{
   Init();
}

CAnalysisResultsGraphBuilder::~CAnalysisResultsGraphBuilder()
{
}

void CAnalysisResultsGraphBuilder::Init()
{
   SetName(_T("Analysis Results"));

   InitDocumentation(EAFGetDocument()->GetDocumentationSetName(),IDH_ANALYSIS_RESULTS);

   m_Graph.SetGridPenStyle(GRAPH_GRID_PEN_STYLE, GRAPH_GRID_PEN_WEIGHT, GRAPH_GRID_COLOR);
   m_Graph.SetClientAreaColor(GRAPH_BACKGROUND);
}

BOOL CAnalysisResultsGraphBuilder::CreateGraphController(CWnd* pParent,UINT nID)
{
   // update the graph definitions before creating the controller. the graph controller
   // uses the graph definitions to initialize the options within its controls
   UpdateGraphDefinitions();

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   ATLASSERT(m_pGraphController != nullptr);
   if ( !m_pGraphController->Create(pParent,IDD_ANALYSISRESULTS_GRAPH_CONTROLLER, CBRS_LEFT, nID) )
   {
      TRACE0("Failed to create control bar\n");
      return FALSE; // failed to create
   }

   return TRUE;
}

void CAnalysisResultsGraphBuilder::CreateViewController(IEAFViewController** ppController)
{
   CComPtr<IEAFViewController> stdController;
   __super::CreateViewController(&stdController);

   CComObject<CAnalysisResultsGraphViewController>* pController;
   CComObject<CAnalysisResultsGraphViewController>::CreateInstance(&pController);
   pController->Init((CAnalysisResultsGraphController*)m_pGraphController, stdController);

   (*ppController) = pController;
   (*ppController)->AddRef();
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

CGraphBuilder* CAnalysisResultsGraphBuilder::Clone() const
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CAnalysisResultsGraphBuilder(*this);
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
      {
         break; // found the first interval that occurs at or after the loading interval
      }
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
      GirderIndexType girderIdx = Min(gdrIdx,nGirders-1);

      CGirderKey thisGirderKey(groupIdx,girderIdx);

      bPedLoading |= pProductLoads->HasPedestrianLoad(thisGirderKey);
      bSidewalk   |= pProductLoads->HasSidewalkLoad(thisGirderKey);
      bShearKey   |= pProductLoads->HasShearKeyLoad(thisGirderKey);

      SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey segmentKey(thisGirderKey,segIdx);

         StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary);
         bTempStrand |= ( 0 < Nt );
      } // next segIdx
   } // next groupIdx

   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   bool bLongitudinalJoint = pBridgeDesc->HasStructuralLongitudinalJoints();

   // Get intervals for reporting
   GET_IFACE(IIntervals,pIntervals);

   // spec check intervals
   std::vector<IntervalIndexType> vSpecCheckIntervals(pIntervals->GetSpecCheckIntervals(girderKey));

   // initial intervals
   IntervalIndexType nIntervals               = pIntervals->GetIntervalCount();
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetFirstPrestressReleaseInterval(girderKey);
   IntervalIndexType storageIntervalIdx       = pIntervals->GetFirstStorageInterval(girderKey);
   IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();

   std::vector<IntervalIndexType> vInitialIntervals;
   vInitialIntervals.push_back(releaseIntervalIdx);
   vInitialIntervals.push_back(storageIntervalIdx);
   vInitialIntervals.push_back(erectSegmentIntervalIdx);

   std::vector<IntervalIndexType> vDeckAndDiaphragmIntervals;
   vDeckAndDiaphragmIntervals.push_back(castDeckIntervalIdx);

   std::vector<IntervalIndexType> vRailingSystemIntervals;
   for ( IntervalIndexType rintervalIdx = railingSystemIntervalIdx; rintervalIdx < nIntervals; rintervalIdx++ )
   {
      vRailingSystemIntervals.push_back(rintervalIdx);
   }

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

   // intervals only after load rating live load is applied
   IntervalIndexType loadRatingIntervalIdx = pIntervals->GetLoadRatingInterval();
   std::vector<IntervalIndexType> vLoadRatingIntervals;
   for ( IntervalIndexType intervalIdx = loadRatingIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      vLoadRatingIntervals.push_back(intervalIdx);
   }

   std::vector<IntervalIndexType> vPTIntervals, vAllPTIntervals;
   GET_IFACE(ITendonGeometry,pTendonGeometry);
   IntervalIndexType firstPTIntervalIdx = nIntervals;
   for ( GroupIndexType groupIdx = startGroupIdx; groupIdx <= endGroupIdx; groupIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(groupIdx);
      GirderIndexType girderIdx = Min(gdrIdx,nGirders-1);

      CGirderKey thisGirderKey(groupIdx,girderIdx);

      DuctIndexType nDucts = pTendonGeometry->GetDuctCount(thisGirderKey);
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         IntervalIndexType ptIntervalIdx = pIntervals->GetStressTendonInterval(thisGirderKey,ductIdx);
         vPTIntervals.push_back(ptIntervalIdx);
         firstPTIntervalIdx = Min(firstPTIntervalIdx,ptIntervalIdx);
      }
   }

   std::sort(vPTIntervals.begin(),vPTIntervals.end());
   for ( IntervalIndexType intervalIdx = firstPTIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      vAllPTIntervals.push_back(intervalIdx);
   }

   std::vector<IntervalIndexType> vTempSupportRemovalIntervals;
   SupportIndexType nTS = pBridgeDesc->GetTemporarySupportCount();
   for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
   {
      IntervalIndexType tsrIntervalIdx = pIntervals->GetTemporarySupportRemovalInterval(tsIdx);
      vTempSupportRemovalIntervals.push_back(tsrIntervalIdx);
   }

   //////////////////////////////////////////////////////////////////////////
   // Product Load Cases
   //////////////////////////////////////////////////////////////////////////

   // girder self-weight
   std::vector<IntervalIndexType> intervals( AddTSRemovalIntervals(erectSegmentIntervalIdx,vInitialIntervals,vTempSupportRemovalIntervals) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftGirder), pgsTypes::pftGirder, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY));
   
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftPretension), pgsTypes::pftPretension, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY));

   intervals.clear();
   intervals = AddTSRemovalIntervals(castDeckIntervalIdx,vDeckAndDiaphragmIntervals,vTempSupportRemovalIntervals);
   GET_IFACE(IUserDefinedLoadData,pUserLoads);
   if ( !IsZero(pUserLoads->GetConstructionLoad()) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftConstruction), pgsTypes::pftConstruction,  vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );
   }

   CAnalysisResultsGraphDefinition diaphragmGraphDef(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftDiaphragm), pgsTypes::pftDiaphragm, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY);
   diaphragmGraphDef.AddIntervals(vTempSupportRemovalIntervals);
   m_pGraphDefinitions->AddGraphDefinition(diaphragmGraphDef);

   // slab dead load
   if (pBridge->GetDeckType() != pgsTypes::sdtNone)
   {
      CAnalysisResultsGraphDefinition slabGraphDef(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftSlab), pgsTypes::pftSlab, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY);
      slabGraphDef.AddIntervals(vTempSupportRemovalIntervals);
      m_pGraphDefinitions->AddGraphDefinition(slabGraphDef);

      CAnalysisResultsGraphDefinition haunchGraphDef(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftSlabPad), pgsTypes::pftSlabPad, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY);
      haunchGraphDef.AddIntervals(vTempSupportRemovalIntervals);
      m_pGraphDefinitions->AddGraphDefinition(haunchGraphDef);
   }

   if ( pBridge->GetDeckType() == pgsTypes::sdtCompositeSIP )
   {
      CAnalysisResultsGraphDefinition slabPanelGraphDef(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftSlabPanel), pgsTypes::pftSlabPanel, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY);
      slabPanelGraphDef.AddIntervals(vTempSupportRemovalIntervals);
      m_pGraphDefinitions->AddGraphDefinition( slabPanelGraphDef );
   }

   if (bShearKey)
   {
      CAnalysisResultsGraphDefinition shearKeyGraphDef(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftShearKey), pgsTypes::pftShearKey, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY);
      shearKeyGraphDef.AddIntervals(vTempSupportRemovalIntervals);
      m_pGraphDefinitions->AddGraphDefinition(shearKeyGraphDef);
   }

   if (bLongitudinalJoint)
   {
      CAnalysisResultsGraphDefinition shearKeyGraphDef(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftLongitudinalJoint), pgsTypes::pftLongitudinalJoint, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY);
      shearKeyGraphDef.AddIntervals(vTempSupportRemovalIntervals);
      m_pGraphDefinitions->AddGraphDefinition(shearKeyGraphDef);
   }

   // Deck shrinkage is different animal
   GET_IFACE(ILosses, pLosses);
   if( pLosses->IsDeckShrinkageApplicable() )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Deck Shrinkage"),   graphDeckShrinkageStress ,   vRailingSystemIntervals) );
   }

   GET_IFACE(IDocumentType,pDocType);
   if ( pDocType->IsPGSpliceDocument() )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftPostTensioning), pgsTypes::pftPostTensioning, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftSecondaryEffects),      pgsTypes::pftSecondaryEffects,      vAllIntervals, ACTIONS_FORCE_STRESS | ACTIONS_X_DEFLECTION_ONLY) );
   }


   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftTrafficBarrier), pgsTypes::pftTrafficBarrier, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );

   if ( bSidewalk )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftSidewalk), pgsTypes::pftSidewalk, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );
   }

   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftOverlay), pgsTypes::pftOverlay, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );


   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftCreep),      pgsTypes::pftCreep,      vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftShrinkage),  pgsTypes::pftShrinkage,  vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftRelaxation), pgsTypes::pftRelaxation, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );
   }

   // User Defined Static Loads
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftUserDC),   pgsTypes::pftUserDC,   vAllIntervals,  ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftUserDW),   pgsTypes::pftUserDW,   vAllIntervals,  ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetProductLoadName(pgsTypes::pftUserLLIM), pgsTypes::pftUserLLIM, vAllIntervals,  ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );


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
   {
      vLiveLoadTypes.push_back(pgsTypes::lltPermit);
   }

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

      case pgsTypes::lltLegalRating_Emergency:
         strBase = _T("Legal Rating (Emergency)");
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
      case pgsTypes::lltLegalRating_Emergency:
      case pgsTypes::lltPermitRating_Routine:
      case pgsTypes::lltPermitRating_Special:
         action = ACTIONS_ALL;
         break;

      case pgsTypes::lltPermit:
         action = ACTIONS_FORCE_DEFLECTION;
         break;

      default:
         ATLASSERT(false);
      }

      VehicleIndexType vehicleIdx = 0;
      std::vector<std::_tstring>::iterator iter(strLLNames.begin());
      std::vector<std::_tstring>::iterator end(strLLNames.end());
      for ( ; iter != end; iter++, vehicleIdx++ )
      {
         std::_tstring& strName( *iter );

         // skip the dummy live load
         if ( strName == NO_LIVE_LOAD_DEFINED )
         {
            continue;
         }

         std::_tstring strLLName( strBase + _T(" - ") + strName );

         CAnalysisResultsGraphDefinition def(graphID++,
                                             strLLName,
                                             llType,
                                             vehicleIdx,
                                             vLiveLoadIntervals,
                                             action);

         m_pGraphDefinitions->AddGraphDefinition(def);
      }

      std::_tstring strLLName( strBase + _T(" - LL+IM") );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, strLLName, llType,  INVALID_INDEX, vLiveLoadIntervals,  ACTIONS_ALL) );
   }


   std::vector<pgsTypes::LiveLoadType> vLoadRatingTypes;
   GET_IFACE(IRatingSpecification,pRatingSpec);
   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
   {
      vLoadRatingTypes.push_back(pgsTypes::lltLegalRating_Routine);
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
   {
      vLoadRatingTypes.push_back(pgsTypes::lltLegalRating_Special);
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
   {
      vLoadRatingTypes.push_back(pgsTypes::lltLegalRating_Emergency);
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
   {
      vLoadRatingTypes.push_back(pgsTypes::lltPermitRating_Routine);
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
   {
      vLoadRatingTypes.push_back(pgsTypes::lltPermitRating_Special);
   }

   for ( iter = vLoadRatingTypes.begin(); iter != vLoadRatingTypes.end(); iter++ )
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

      case pgsTypes::lltLegalRating_Emergency:
         strBase = _T("Legal Rating (Emergency)");
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
      case pgsTypes::lltLegalRating_Emergency:
      case pgsTypes::lltPermitRating_Routine:
      case pgsTypes::lltPermitRating_Special:
         action = ACTIONS_ALL;
         break;

      case pgsTypes::lltPermit:
         action = ACTIONS_FORCE_DEFLECTION;
         break;

      default:
         ATLASSERT(false);
      }

      VehicleIndexType vehicleIdx = 0;
      std::vector<std::_tstring>::iterator iter(strLLNames.begin());
      std::vector<std::_tstring>::iterator end(strLLNames.end());
      for ( ; iter != end; iter++, vehicleIdx++ )
      {
         std::_tstring& strName( *iter );

         // skip the dummy live load
         if ( strName == NO_LIVE_LOAD_DEFINED )
         {
            continue;
         }

         std::_tstring strLLName( strBase + _T(" - ") + strName );

         CAnalysisResultsGraphDefinition def(graphID++,
                                             strLLName,
                                             llType,
                                             vehicleIdx,
                                             vLoadRatingIntervals,
                                             action);

         m_pGraphDefinitions->AddGraphDefinition(def);
      }

      std::_tstring strLLName( strBase + _T(" - LL+IM") );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, strLLName, llType,  INVALID_INDEX, vLoadRatingIntervals,  ACTIONS_ALL) );
   }

   // Combined Results
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetLoadCombinationName(lcDC), lcDC, vAllIntervals,  ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetLoadCombinationName(lcDW), lcDW, vAllIntervals,  ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );

   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetLoadCombinationName(lcCR), lcCR, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetLoadCombinationName(lcSH), lcSH, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetLoadCombinationName(lcRE), lcRE, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, pProductLoads->GetLoadCombinationName(lcPS), lcPS, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );
   }

   if ( bPedLoading )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("PL"),   pgsTypes::lltPedestrian, vLiveLoadIntervals,  ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );
   }

   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Design)"), pgsTypes::lltDesign, vLiveLoadIntervals, ACTIONS_ALL) );

   if (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Fatigue)"), pgsTypes::lltFatigue, vLiveLoadIntervals, ACTIONS_ALL) );
   }

   if (bPermit)
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Permit)"), pgsTypes::lltPermit, vLiveLoadIntervals, ACTIONS_ALL) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Legal Rating, Routine)"), pgsTypes::lltLegalRating_Routine, vLoadRatingIntervals, ACTIONS_ALL) );
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Legal Rating, Special)"), pgsTypes::lltLegalRating_Special, vLoadRatingIntervals, ACTIONS_ALL));
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Legal Rating, Emergency)"), pgsTypes::lltLegalRating_Emergency, vLoadRatingIntervals, ACTIONS_ALL));
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Permit Rating, Routine)"), pgsTypes::lltPermitRating_Routine, vLoadRatingIntervals, ACTIONS_ALL) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("LL+IM (Permit Rating, Special)"), pgsTypes::lltPermitRating_Special, vLoadRatingIntervals, ACTIONS_ALL) );
   }

   // Limit States and Capacities
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service I (Design)"), pgsTypes::ServiceI, vAllIntervals, ACTIONS_ALL | ACTIONS_X_DEFLECTION_ONLY) );
   
   if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service IA (Design)"), pgsTypes::ServiceIA, vLiveLoadIntervals, ACTIONS_STRESS_ONLY) );
   }

   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III (Design)"),          pgsTypes::ServiceIII,               vLiveLoadIntervals,  ACTIONS_STRESS_ONLY) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Design)"),           pgsTypes::StrengthI,                vLiveLoadIntervals,  ACTIONS_MOMENT_SHEAR) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity (Design)"),  pgsTypes::StrengthI, graphCapacity, vLiveLoadIntervals,  ACTIONS_SHEAR_ONLY) );
   
   if (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Fatigue I"), pgsTypes::FatigueI, vLiveLoadIntervals, ACTIONS_STRESS_ONLY) );
   }

   GET_IFACE(ILimitStateForces,pLimitStateForces);
   bool bStrII = pLimitStateForces->IsStrengthIIApplicable(CSegmentKey(girderKey.groupIndex,girderKey.girderIndex,0));

   if ( bStrII )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II (Permit)"),          pgsTypes::StrengthII,               vLiveLoadIntervals,  ACTIONS_MOMENT_SHEAR) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II Capacity (Permit)"), pgsTypes::StrengthII,graphCapacity, vLiveLoadIntervals,  ACTIONS_SHEAR_ONLY) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Design Rating, Inventory)"),           pgsTypes::StrengthI_Inventory,                 vLoadRatingIntervals,  ACTIONS_MOMENT_SHEAR) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity (Design Rating, Inventory)"),  pgsTypes::StrengthI_Inventory, graphCapacity,  vLoadRatingIntervals,  ACTIONS_SHEAR_ONLY) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Operating) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Design Rating, Operating)"),           pgsTypes::StrengthI_Operating,                 vLoadRatingIntervals,  ACTIONS_MOMENT_SHEAR) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity (Design Rating, Operating)"),  pgsTypes::StrengthI_Operating, graphCapacity,  vLoadRatingIntervals,  ACTIONS_SHEAR_ONLY) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Legal Rating, Routine)"),           pgsTypes::StrengthI_LegalRoutine,                 vLoadRatingIntervals,  ACTIONS_MOMENT_SHEAR) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity (Legal Rating, Routine)"),  pgsTypes::StrengthI_LegalRoutine, graphCapacity,  vLoadRatingIntervals,  ACTIONS_SHEAR_ONLY) );
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Legal Rating, Special)"), pgsTypes::StrengthI_LegalSpecial, vLoadRatingIntervals, ACTIONS_MOMENT_SHEAR));
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity, (Legal Rating, Special)"), pgsTypes::StrengthI_LegalSpecial, graphCapacity, vLoadRatingIntervals, ACTIONS_SHEAR_ONLY));
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I (Legal Rating, Emergency)"), pgsTypes::StrengthI_LegalEmergency, vLoadRatingIntervals, ACTIONS_MOMENT_SHEAR));
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength I Capacity, (Legal Rating, Emergency)"), pgsTypes::StrengthI_LegalEmergency, graphCapacity, vLoadRatingIntervals, ACTIONS_SHEAR_ONLY));
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Routine) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II (Routine Permit Rating)"),           pgsTypes::StrengthII_PermitRoutine,                 vLoadRatingIntervals,  ACTIONS_MOMENT_SHEAR) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II Capacity (Routine Permit Rating)"),  pgsTypes::StrengthII_PermitRoutine, graphCapacity,  vLoadRatingIntervals,  ACTIONS_SHEAR_ONLY) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrPermit_Special) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II (Special Permit Rating)"),           pgsTypes::StrengthII_PermitSpecial,                 vLoadRatingIntervals,  ACTIONS_MOMENT_SHEAR) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Strength II Capacity (Special Permit Rating)"),  pgsTypes::StrengthII_PermitSpecial, graphCapacity,  vLoadRatingIntervals,  ACTIONS_SHEAR_ONLY) );
   }

   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Moment Capacity"),      pgsTypes::StrengthI, graphCapacity,    vLiveLoadIntervals,  ACTIONS_MOMENT_ONLY) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Min Moment Capacity"),  pgsTypes::StrengthI, graphMinCapacity, vLiveLoadIntervals,  ACTIONS_MOMENT_ONLY) );

   // Demand and Allowable
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service I Demand (Design)"),     pgsTypes::ServiceI,  graphDemand,    vAllIntervals,ACTIONS_STRESS_ONLY | ACTIONS_DEFLECTION_ONLY | ACTIONS_X_DEFLECTION_ONLY) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service I Limit (Design)"),  pgsTypes::ServiceI,  graphAllowable, vSpecCheckIntervals) );
   
   if (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service IA Demand (Design)"),    pgsTypes::ServiceIA, graphDemand,    vLiveLoadIntervals) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service IA Limit (Design)"), pgsTypes::ServiceIA, graphAllowable, vLiveLoadIntervals) );
   }

   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Design)"),   pgsTypes::ServiceIII,graphDemand,    vLiveLoadIntervals) );
   m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Limit (Design)"),pgsTypes::ServiceIII,graphAllowable, vLiveLoadIntervals) );
   
   if (lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Fatigue I Demand"),    pgsTypes::FatigueI, graphDemand,    vLiveLoadIntervals) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Fatigue I Limit"), pgsTypes::FatigueI, graphAllowable, vLiveLoadIntervals) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrDesign_Inventory) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Design Rating, Inventory)"),   pgsTypes::ServiceIII_Inventory,graphDemand,    vLoadRatingIntervals) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Limit (Design Rating, Inventory)"),pgsTypes::ServiceIII_Inventory,graphAllowable, vLoadRatingIntervals) );
   }

   if ( pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Routine) )
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Legal Rating, Routine)"),   pgsTypes::ServiceIII_LegalRoutine,graphDemand,    vLoadRatingIntervals) );
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Limit (Legal Rating, Routine)"),pgsTypes::ServiceIII_LegalRoutine,graphAllowable, vLoadRatingIntervals) );
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Special))
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Legal Rating, Special)"), pgsTypes::ServiceIII_LegalSpecial, graphDemand, vLoadRatingIntervals));
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Limit (Legal Rating, Special)"), pgsTypes::ServiceIII_LegalSpecial, graphAllowable, vLoadRatingIntervals));
   }

   if (pRatingSpec->IsRatingEnabled(pgsTypes::lrLegal_Emergency))
   {
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Demand (Legal Rating, Emergency)"), pgsTypes::ServiceIII_LegalEmergency, graphDemand, vLoadRatingIntervals));
      m_pGraphDefinitions->AddGraphDefinition(CAnalysisResultsGraphDefinition(graphID++, _T("Service III Limit (Legal Rating, Emergency)"), pgsTypes::ServiceIII_LegalEmergency, graphAllowable, vLoadRatingIntervals));
   }
}

std::vector<std::pair<std::_tstring,IDType>> CAnalysisResultsGraphBuilder::GetLoadings(IntervalIndexType intervalIdx,ActionType actionType)
{
   return m_pGraphDefinitions->GetLoadings(intervalIdx,actionType);
}

GraphType CAnalysisResultsGraphBuilder::GetGraphType(IDType graphID)
{
   CAnalysisResultsGraphDefinition& graphDef = m_pGraphDefinitions->GetGraphDefinition(graphID);
   return graphDef.m_GraphType;
}

CGirderGraphControllerBase* CAnalysisResultsGraphBuilder::CreateGraphController()
{
   return new CAnalysisResultsGraphController;
}

bool CAnalysisResultsGraphBuilder::UpdateNow()
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress,0);

   pProgress->UpdateMessage(_T("Building Graph"));

   CWaitCursor wait;

   CGirderGraphBuilderBase::UpdateNow();

   // Update graph properties
   UpdateYAxisUnits();
   UpdateXAxisTitle();
   UpdateGraphTitle();

   // we want a tighter zero tolerance for rotations
   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   Float64 tolerance = m_ZeroToleranceY;
   if ( actionType == actionRotation )
   {
      m_ZeroToleranceY = 1e-07;
   }

   UpdateGraphData();

   m_ZeroToleranceY = tolerance;

   return true;
}

void CAnalysisResultsGraphBuilder::UpdateYAxisUnits()
{
   delete m_pYFormat;

   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   switch(actionType)
   {
   case actionMoment:
      {
      const unitmgtMomentData& momentUnit = pDisplayUnits->GetMomentUnit();
      m_pYFormat = new MomentTool(momentUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Moment (") + ((MomentTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   case actionShear:
      {
      const unitmgtForceData& shearUnit = pDisplayUnits->GetShearUnit();
      m_pYFormat = new ShearTool(shearUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Shear (") + ((ShearTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   case actionAxial:
      {
      const unitmgtForceData& axialUnit = pDisplayUnits->GetGeneralForceUnit();
      m_pYFormat = new ForceTool(axialUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Axial (") + ((ForceTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   case actionDeflection:
   case actionXDeflection:
   {
      const unitmgtLengthData& deflectionUnit = pDisplayUnits->GetDeflectionUnit();
      m_pYFormat = new DeflectionTool(deflectionUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Deflection (") + ((DeflectionTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   case actionRotation:
      {
      const unitmgtAngleData& rotationUnit = pDisplayUnits->GetRadAngleUnit();
      m_pYFormat = new RotationTool(rotationUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Rotation (") + ((RotationTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   case actionStress:
      {
      const unitmgtStressData& stressUnit = pDisplayUnits->GetStressUnit();
      m_pYFormat = new StressTool(stressUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Stress (") + ((StressTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   case actionReaction:
      {
      const unitmgtForceData& shearUnit = pDisplayUnits->GetShearUnit();
      m_pYFormat = new ShearTool(shearUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Reaction (") + ((ShearTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   default:
      ASSERT(0); 
   }
}

void CAnalysisResultsGraphBuilder::UpdateXAxisTitle()
{
   CAnalysisResultsGraphController* pMyGraphController = (CAnalysisResultsGraphController*)m_pGraphController;
   std::vector<IntervalIndexType> vIntervals(pMyGraphController->GetSelectedIntervals());
   IntervalIndexType intervalIdx = INVALID_INDEX;
   if ( 0 < vIntervals.size() )
   {
      intervalIdx = vIntervals.back();
   }

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(CSegmentKey(0,0,0));
   if ( intervalIdx == releaseIntervalIdx )
   {
      m_Graph.SetXAxisTitle(std::_tstring(_T("Distance From Left End of Girder (")+m_pXFormat->UnitTag()+_T(")")).c_str());
   }
   else
   {
      m_Graph.SetXAxisTitle(std::_tstring(_T("Distance From CL Bearing at Left End of Girder (")+m_pXFormat->UnitTag()+_T(")")).c_str());
   }
}

void CAnalysisResultsGraphBuilder::UpdateGraphTitle()
{
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();
   CString strCombo = (resultsType == rtIncremental ? _T("Incremental") : _T("Cumulative"));

   ActionType actionType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   CString strAction;
   switch(actionType)
   {
   case actionAxial:
      strAction = _T("Axial");
      break;
   case actionShear:
      strAction = _T("Shear");
      break;
   case actionMoment:
      strAction = _T("Moment");
      break;
   case actionDeflection:
      strAction = _T("Deflection");
      break;
   case actionXDeflection:
      strAction = _T("Deflection X");
      break;
   case actionRotation:
      strAction = _T("Rotation");
      break;
   case actionStress:
      strAction = _T("Stress");
      break;
   case actionReaction:
      strAction = _T("Reaction");
      break;
   default:
      ASSERT(0);
   }

   CAnalysisResultsGraphController* pMyGraphController = (CAnalysisResultsGraphController*)m_pGraphController;
   std::vector<IntervalIndexType> vIntervals(pMyGraphController->GetSelectedIntervals());

   GroupIndexType  grpIdx = m_pGraphController->GetGirderGroup();
   GirderIndexType gdrIdx = m_pGraphController->GetGirder();

   CGirderKey girderKey(grpIdx == ALL_GROUPS ? 0 : grpIdx,gdrIdx);

   if ( ((CAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode() == CAnalysisResultsGraphController::Loading)
   {
      // Plotting by loading
      IntervalIndexType intervalIdx = vIntervals.back();

      GET_IFACE(IIntervals,pIntervals);
      CString strInterval( pIntervals->GetDescription(intervalIdx) );

      CString strGraphTitle;
      if ( grpIdx == ALL_GROUPS )
      {
         strGraphTitle.Format(_T("Girder Line %s - Interval %d: %s - %s %s"),LABEL_GIRDER(gdrIdx),LABEL_INTERVAL(intervalIdx),strInterval,strCombo,strAction);
      }
      else
      {
         strGraphTitle.Format(_T("%s - Interval %d: %s - %s %s"),GIRDER_LABEL(CGirderKey(grpIdx,gdrIdx)),LABEL_INTERVAL(intervalIdx),strInterval,strCombo,strAction);
      }
      
      m_Graph.SetTitle(strGraphTitle);
   }
   else
   {
      // Plotting by interval
      IDType graphID = ((CAnalysisResultsGraphController*)m_pGraphController)->SelectedGraphIndexToGraphID(0);
      const CAnalysisResultsGraphDefinition& graphDef = m_pGraphDefinitions->GetGraphDefinition(graphID);

      CString strGraphTitle;
      if ( grpIdx == ALL_GROUPS )
      {
         strGraphTitle.Format(_T("Girder Line %s - %s - %s %s"),LABEL_GIRDER(gdrIdx),graphDef.m_Name.c_str(),strCombo,strAction);
      }
      else
      {
         strGraphTitle.Format(_T("%s - %s - %s %s"),GIRDER_LABEL(CGirderKey(grpIdx,gdrIdx)),graphDef.m_Name.c_str(),strCombo,strAction);
      }

      m_Graph.SetTitle(strGraphTitle);
   }

   GET_IFACE(IDocumentType,pDocType);
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
         strSubtitle = _T("Envelope of simple span and simple made continuous");
         break;
      }

      m_Graph.SetSubtitle(strSubtitle);
   }
}

COLORREF CAnalysisResultsGraphBuilder::GetGraphColor(IndexType graphIdx,IntervalIndexType intervalIdx)
{
   COLORREF c;
   if ( ((CAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode() == CAnalysisResultsGraphController::Loading)
   {
      c = m_pGraphColor->GetColor(graphIdx);
   }
   else
   {
      c = m_pGraphColor->GetColor(intervalIdx);
   }

   return c;
}

CString CAnalysisResultsGraphBuilder::GetDataLabel(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx)
{
   CString strDataLabel;

   if ( ((CAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode() == CAnalysisResultsGraphController::Loading)
   {
      std::set<IndexType>::iterator found(m_UsedDataLabels.find(graphIdx));
      if ( found == m_UsedDataLabels.end() )
      {
         strDataLabel.Format(_T("%s"),graphDef.m_Name.c_str());
         m_UsedDataLabels.insert(graphIdx);
      }
   }
   else
   {
      std::set<IndexType>::iterator found(m_UsedDataLabels.find(intervalIdx));
      if ( found == m_UsedDataLabels.end() )
      {
         strDataLabel.Format(_T("Interval %d"),LABEL_INTERVAL(intervalIdx));
         m_UsedDataLabels.insert(intervalIdx);
      }
   }

   return strDataLabel;
}

void CAnalysisResultsGraphBuilder::UpdateGraphData()
{
   // clear graph
   m_Graph.ClearData();
   m_UsedDataLabels.clear();

   GroupIndexType  grpIdx = m_pGraphController->GetGirderGroup();
   GirderIndexType gdrIdx = m_pGraphController->GetGirder();

   CGirderKey girderKey(grpIdx == ALL_GROUPS ? 0 : grpIdx,gdrIdx);

   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();

   std::vector<IntervalIndexType> vIntervals = ((CAnalysisResultsGraphController*)m_pGraphController)->GetSelectedIntervals();
   if ( 0 == vIntervals.size() )
   {
      // if there aren't any intervals to plot, there is nothing to plot
      return;
   }

   // Get the X locations for the graph
   GET_IFACE(IPointOfInterest,pIPoi);

   // If the segments are simple span elements, we want to draw a graph for each segment individually.
   // Determine if the segments are simple spans during the intervals being graphed
   GET_IFACE(IIntervals,pIntervals);
   bool bSimpleSpanSegments = true;
   GET_IFACE(IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (grpIdx == ALL_GROUPS ? 0 : grpIdx);
   GroupIndexType endGroupIdx   = (grpIdx == ALL_GROUPS ? nGroups-1 : Min(nGroups-1,startGroupIdx));

   std::vector<IntervalIndexType>::iterator intervalIter(vIntervals.begin());
   std::vector<IntervalIndexType>::iterator intervalIterEnd(vIntervals.end());
   for ( ; intervalIter != intervalIterEnd; intervalIter++ )
   {
      IntervalIndexType intervalIdx = *intervalIter;

      for ( GroupIndexType groupIdx = startGroupIdx; groupIdx <= endGroupIdx; groupIdx++ )
      {
         GirderIndexType nGirders = pBridge->GetGirderCount(groupIdx);
         GirderIndexType girderIdx = Min(gdrIdx,nGirders-1);
         CGirderKey thisGirderKey(groupIdx,girderIdx);
         SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
         if ( 1 < nSegments )
         {
            for ( SegmentIndexType segIdx = 0; segIdx < nSegments-1; segIdx++ )
            {
               CClosureKey closureKey(thisGirderKey,segIdx);
               if ( pIntervals->GetCompositeClosureJointInterval(closureKey) <= intervalIdx )
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
         if ( pIntervals->GetCompositeDeckInterval() <= intervalIdx && grpIdx == ALL_GROUPS )
         {
            bSimpleSpanSegments = false;
         }
      }
   }

   // Determine the interval when the first segment is erected for the girder groups
   // that are being plotted
   IntervalIndexType firstSegmentErectionIntervalIdx = INVALID_INDEX;
   for ( GroupIndexType groupIdx = startGroupIdx; groupIdx <= endGroupIdx; groupIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(groupIdx);
      GirderIndexType girderIdx = Min(gdrIdx,nGirders-1);
      CGirderKey thisGirderKey(groupIdx,girderIdx);
      firstSegmentErectionIntervalIdx = Min(firstSegmentErectionIntervalIdx,pIntervals->GetFirstSegmentErectionInterval(thisGirderKey));
   }

   IntervalIndexType firstPlottingIntervalIdx = vIntervals.front();
   IntervalIndexType lastPlottingIntervalIdx  = vIntervals.back();

   IndexType nGraphs = ((CAnalysisResultsGraphController*)m_pGraphController)->GetGraphCount();
   m_pGraphColor->SetGraphCount(nGraphs);

   CAnalysisResultsGraphController::GraphModeType graphMode = ((CAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode();

   IndexType nSelectedGraphs = ((CAnalysisResultsGraphController*)m_pGraphController)->GetSelectedGraphCount();

   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   for ( GroupIndexType groupIdx = startGroupIdx; groupIdx <= endGroupIdx; groupIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(groupIdx);
      GirderIndexType girderIdx = Min(gdrIdx,nGirders-1);
      CGirderKey thisGirderKey(groupIdx,girderIdx);

      m_GroupOffset = -ComputeShift(thisGirderKey); // shift is negative, we want positive value... change sign

      SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);

      SegmentIndexType endSegmentIdx = (bSimpleSpanSegments ? nSegments-1 : 0);
      for ( SegmentIndexType segIdx = 0; segIdx <= endSegmentIdx; segIdx++ )
      {
         CSegmentKey segmentKey(groupIdx,girderIdx,segIdx);
         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
         IntervalIndexType segmentErectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

         if ( firstSegmentErectionIntervalIdx <= firstPlottingIntervalIdx && // results are for erected segments
              lastPlottingIntervalIdx < segmentErectionIntervalIdx ) // current segment is not yet erected
         {
            continue;
         }
         
         PoiList vPoi;
         pIPoi->GetPointsOfInterest(CSegmentKey(groupIdx, girderIdx, bSimpleSpanSegments ? segIdx : ALL_SEGMENTS), &vPoi);

         if ( bSimpleSpanSegments )
         {
            // these POI are between segments so they don't apply
            pIPoi->RemovePointsOfInterest(vPoi,POI_CLOSURE);
            pIPoi->RemovePointsOfInterest(vPoi,POI_BOUNDARY_PIER);
         }

         // Map POI coordinates to X-values for the graph
         std::vector<Float64> xVals;
         Shift(grpIdx == ALL_GROUPS ? false : true);
         GetXValues(vPoi,&xVals);

         for ( IndexType graphIdx = 0; graphIdx < nSelectedGraphs; graphIdx++ )
         {
            IntervalIndexType intervalIdx = INVALID_INDEX;
            if ( graphMode == CAnalysisResultsGraphController::Loading)
            {
               ATLASSERT(vIntervals.size() == 1);
               intervalIdx = vIntervals.back();
            }
            else
            {
               // if plotting by interval, the graph index is the index into the selected intervals
               intervalIdx = vIntervals[graphIdx];
            }

            IDType graphID = ((CAnalysisResultsGraphController*)m_pGraphController)->SelectedGraphIndexToGraphID(graphIdx);
            const CAnalysisResultsGraphDefinition& graphDef = m_pGraphDefinitions->GetGraphDefinition(graphID);

            IndexType selectedGraphIdx = m_pGraphDefinitions->GetGraphIndex(graphID);

            switch( graphDef.m_GraphType )
            {
            case graphProduct:
               if ( actionType == actionReaction )
               {
                  ProductReactionGraph(selectedGraphIdx,graphDef,intervalIdx,thisGirderKey,bSimpleSpanSegments ? segIdx : ALL_SEGMENTS);
               }
               else
               {
                  //if ( actionType == actionShear && liveLoadIntervalIdx <= intervalIdx )
                  //{
                  //   ProductLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi2,xVals2,true);
                  //}
                  ProductLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi,xVals,false);
               }
               break;

            case graphCombined:
               if ( actionType == actionReaction )
               {
                  CombinedReactionGraph(selectedGraphIdx,graphDef,intervalIdx,thisGirderKey,bSimpleSpanSegments ? segIdx : ALL_SEGMENTS);
               }
               else
               {
                  //if ( actionType == actionShear && liveLoadIntervalIdx <= intervalIdx )
                  //{
                  //   CombinedLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi2,xVals2,true);
                  //}
                  CombinedLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi,xVals,false);
               }
               break;

            case graphAllowable:
               if(intervalIdx == releaseIntervalIdx && actionType == actionStress)
               {
                  // Casting yard stress is its own animal
                  CyStressCapacityGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi,xVals);
                  break; // only break if we ploatted the allowables... otherwise drop through
               }
            case graphDemand:
            case graphLimitState:
            case graphCapacity:
            case graphMinCapacity:
               if ( actionType == actionReaction )
               {
                  LimitStateReactionGraph(selectedGraphIdx,graphDef,intervalIdx,thisGirderKey,bSimpleSpanSegments ? segIdx : ALL_SEGMENTS);
               }
               else
               {
                  //if ( actionType == actionShear && liveLoadIntervalIdx <= intervalIdx )
                  //{
                  //   LimitStateLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi2,xVals2,true);
                  //}
                  LimitStateLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi,xVals,false);
               }
               break;

            case graphLiveLoad:
               if ( actionType == actionReaction )
               {
                  LiveLoadReactionGraph(graphIdx,graphDef,intervalIdx,thisGirderKey);
               }
               else
               {
                  //if ( actionType == actionShear && liveLoadIntervalIdx <= intervalIdx )
                  //{
                  //   LiveLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi2,xVals2,true);
                  //}
                  LiveLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi,xVals,false);
               }
               break;

            case graphVehicularLiveLoad:
               if ( actionType == actionReaction )
               {
                  VehicularLiveLoadReactionGraph(graphIdx,graphDef,intervalIdx,thisGirderKey);
               }
               else
               {
                  //if ( actionType == actionShear && liveLoadIntervalIdx <= intervalIdx )
                  //{
                  //   VehicularLiveLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi2,xVals2,true);
                  //}
                  VehicularLiveLoadGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi,xVals,false);
               }
               break;

            case graphDeckShrinkageStress:
               {
                  ATLASSERT(actionType == actionStress);
                  DeckShrinkageStressGraph(selectedGraphIdx,graphDef,intervalIdx,vPoi,xVals);
                  break; 
               }

            default:
               ASSERT(false); // should never get here
            } // end switch-case
         } // next graph
      } // next segment
   } // next group
}

void CAnalysisResultsGraphBuilder::InitializeGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,ActionType actionType,IntervalIndexType intervalIdx,bool bIsFinalShear,IndexType* pDataSeriesID,pgsTypes::BridgeAnalysisType* pBAT,pgsTypes::StressLocation* pStressLocation,IndexType* pAnalysisTypeCount)
{
   CGirderKey girderKey(m_pGraphController->GetGirderKey());
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      girderKey.groupIndex = 0;
   }

   CString strDataLabel(GetDataLabel(graphIdx,graphDef,intervalIdx));

   COLORREF c(GetGraphColor(graphIdx,intervalIdx));
   int penWeight = GRAPH_PEN_WEIGHT;

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   if (actionType == actionShear )
   {
      GET_IFACE(IProductForces,pProductForces);
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

      int penStyle = (/*liveLoadIntervalIdx <= intervalIdx && actionType == actionShear && !bIsFinalShear ? PS_DOT :*/ PS_SOLID);

      if ( analysisType == pgsTypes::Envelope )
      {
         *pAnalysisTypeCount = 2;
         pDataSeriesID[0] = m_Graph.CreateDataSeries(strDataLabel, penStyle, penWeight, c);
         pDataSeriesID[1] = m_Graph.CreateDataSeries(_T(""),       penStyle, penWeight, c);

         pBAT[0] = pProductForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);
         pBAT[1] = pProductForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
         ATLASSERT(pBAT[0] == pgsTypes::MinSimpleContinuousEnvelope);
         ATLASSERT(pBAT[1] == pgsTypes::MaxSimpleContinuousEnvelope);
      }
      else
      {
         *pAnalysisTypeCount = 1;
         pDataSeriesID[0] = m_Graph.CreateDataSeries(strDataLabel, penStyle, penWeight, c);

         pBAT[0] = pProductForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
         ATLASSERT(pBAT[0] == (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan));
      }
   }
   else if ( actionType == actionAxial || 
             actionType == actionMoment ||
             actionType == actionDeflection ||
             actionType == actionXDeflection ||
             actionType == actionRotation ||
             actionType == actionReaction)
   {
      // For moments and deflections
      GET_IFACE(IProductForces,pProductForces);
      if ( analysisType == pgsTypes::Envelope )
      {
         *pAnalysisTypeCount = 2;
         pDataSeriesID[0] = m_Graph.CreateDataSeries(strDataLabel, PS_SOLID, penWeight, c);
         pDataSeriesID[1] = m_Graph.CreateDataSeries(_T(""),       PS_SOLID, penWeight, c);

         pBAT[0] = pProductForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);
         pBAT[1] = pProductForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);

         ATLASSERT(pBAT[0] == pgsTypes::MinSimpleContinuousEnvelope);
         ATLASSERT(pBAT[1] == pgsTypes::MaxSimpleContinuousEnvelope);
      }
      else
      {
         *pAnalysisTypeCount = 1;
         pDataSeriesID[0] = m_Graph.CreateDataSeries(strDataLabel, PS_SOLID, penWeight, c);

         pBAT[0] = pProductForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
         ATLASSERT(pBAT[0] == (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan));
      }
   }
   else if (actionType == actionStress)
   {
      // for stresses
      CString strStressLabel[4] = { _T(" - Bottom Girder"), _T(" - Top Girder"), _T(" - Bottom Deck"), _T(" - Top Deck") };
      int penStyle[4] = {PS_STRESS_BOTTOM_GIRDER,PS_STRESS_TOP_GIRDER,PS_STRESS_BOTTOM_DECK,PS_STRESS_TOP_DECK};
      pgsTypes::BridgeAnalysisType bat[4] = { pgsTypes::MaxSimpleContinuousEnvelope, pgsTypes::MinSimpleContinuousEnvelope, pgsTypes::MinSimpleContinuousEnvelope, pgsTypes::MinSimpleContinuousEnvelope};

      *pAnalysisTypeCount = 0;
      for ( int i = 0; i < 4; i++ )
      {
         pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;
      
         bool bPlotStresses = ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(stressLocation);

         if ( bPlotStresses )
         {
            CString strStressDataLabel(strDataLabel);
            if ( !strStressDataLabel.IsEmpty() )
            {
               strStressDataLabel += strStressLabel[stressLocation];
            }

            pDataSeriesID[*pAnalysisTypeCount] = m_Graph.CreateDataSeries(strStressDataLabel, penStyle[stressLocation],    penWeight, c);
            pStressLocation[*pAnalysisTypeCount] = stressLocation;

            if ( analysisType == pgsTypes::Envelope )
            {
               pBAT[*pAnalysisTypeCount] = bat[stressLocation];
            }
            else
            {
               pBAT[*pAnalysisTypeCount] = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
            }

            (*pAnalysisTypeCount)++;
         }
      } // next stress location
   }
   else
   {
      ATLASSERT(false); // is there a new action type?
   }
}

void CAnalysisResultsGraphBuilder::ProductLoadGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear)
{
   pgsTypes::ProductForceType pfType(graphDef.m_LoadType.ProductLoadType);

   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();

   // Product forces
   GET_IFACE(IProductForces2,pForces);

   IndexType data_series_id[4];
   pgsTypes::BridgeAnalysisType bat[4];
   pgsTypes::StressLocation stressLocation[4];
   IndexType nAnalysisTypes;
   InitializeGraph(graphIdx,graphDef,actionType,intervalIdx,bIsFinalShear,data_series_id,bat,stressLocation,&nAnalysisTypes);

   for ( IndexType analysisIdx = 0; analysisIdx < nAnalysisTypes; analysisIdx++ )
   {
      switch(actionType)
      {
      case actionAxial:
         {
            std::vector<Float64> forces( pForces->GetAxial( intervalIdx, pfType, vPoi, bat[analysisIdx], resultsType) );
            AddGraphPoints(data_series_id[analysisIdx], xVals, forces);
            break;
         }
      case actionShear:
         {
            std::vector<sysSectionValue> shears( pForces->GetShear( intervalIdx, pfType, vPoi, bat[analysisIdx], resultsType) );
            AddGraphPoints(data_series_id[analysisIdx], xVals, shears);
            break;
         }
      case actionMoment:
         {
            std::vector<Float64> moments( pForces->GetMoment( intervalIdx, pfType, vPoi, bat[analysisIdx], resultsType) );
            AddGraphPoints(data_series_id[analysisIdx], xVals, moments);
            break;
         }
      case actionDeflection:
      {
         bool bIncludeElevationAdjustment = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeElevationAdjustment();
         bool bIncludePrecamber = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludePrecamber();
         std::vector<Float64> deflections(pForces->GetDeflection(intervalIdx, pfType, vPoi, bat[analysisIdx], resultsType, bIncludeElevationAdjustment, bIncludePrecamber));
         AddGraphPoints(data_series_id[analysisIdx], xVals, deflections);
         break;
      }
      case actionXDeflection:
      {
         std::vector<Float64> deflections(pForces->GetXDeflection(intervalIdx, pfType, vPoi, bat[analysisIdx], resultsType));
         AddGraphPoints(data_series_id[analysisIdx], xVals, deflections);
         break;
      }
      case actionRotation:
         {
            bool bIncludeSlopeAdjustment = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeElevationAdjustment();
            bool bIncludePrecamber = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludePrecamber();
            std::vector<Float64> rotations( pForces->GetRotation( intervalIdx, pfType, vPoi, bat[analysisIdx], resultsType, bIncludeSlopeAdjustment, bIncludePrecamber) );
            AddGraphPoints(data_series_id[analysisIdx], xVals, rotations);
            break;
         }
      case actionStress:
         {
            pgsTypes::StressLocation topLocation = (IsGirderStressLocation(stressLocation[analysisIdx]) ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
            pgsTypes::StressLocation botLocation = (IsGirderStressLocation(stressLocation[analysisIdx]) ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);

            std::vector<Float64> fTop, fBot;
            pForces->GetStress( intervalIdx, pfType, vPoi, bat[analysisIdx], resultsType, topLocation, botLocation, &fTop, &fBot);

            if ( IsTopStressLocation(stressLocation[analysisIdx]) )
            {
               AddGraphPoints(data_series_id[analysisIdx], xVals, fTop);
            }
            else
            {
               AddGraphPoints(data_series_id[analysisIdx], xVals, fBot);
            }
            break;
         }
      default:
         ATLASSERT(false);
      }
   } // next analysis type
}

void CAnalysisResultsGraphBuilder::CombinedLoadGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear)
{
   // Combined forces
   GET_IFACE(ICombinedForces2,pForces);
   LoadingCombinationType combination_type(graphDef.m_LoadType.CombinedLoadType);

   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();
   
   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   IndexType data_series_id[4];
   pgsTypes::BridgeAnalysisType bat[4];
   pgsTypes::StressLocation stressLocation[4];
   IndexType nAnalysisTypes;
   InitializeGraph(graphIdx,graphDef,actionType,intervalIdx,bIsFinalShear,data_series_id,bat,stressLocation,&nAnalysisTypes);

   for ( IndexType analysisIdx = 0; analysisIdx < nAnalysisTypes; analysisIdx++ )
   {
      switch(actionType)
      {
      case actionAxial:
         {
            std::vector<Float64> forces = pForces->GetAxial( intervalIdx, combination_type, vPoi, bat[analysisIdx], resultsType );
            AddGraphPoints(data_series_id[analysisIdx], xVals, forces);
            break;
         }

      case actionShear:
         {
            std::vector<sysSectionValue> shear = pForces->GetShear( intervalIdx, combination_type, vPoi, bat[analysisIdx], resultsType );
            AddGraphPoints(data_series_id[analysisIdx], xVals, shear);
            break;
         }

      case actionMoment:
         {
            std::vector<Float64> moments = pForces->GetMoment( intervalIdx, combination_type, vPoi, bat[analysisIdx], resultsType );
            AddGraphPoints(data_series_id[analysisIdx], xVals, moments);
            break;
         }

      case actionDeflection:
         {
            bool bIncludeElevationAdjustment = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeElevationAdjustment();
            bool bIncludePrecamber = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludePrecamber();
            std::vector<Float64> displ = pForces->GetDeflection( intervalIdx, combination_type, vPoi, bat[analysisIdx], resultsType, bIncludeElevationAdjustment, bIncludePrecamber );
            AddGraphPoints(data_series_id[analysisIdx], xVals, displ);
            break;
         }

      case actionXDeflection:
      {
         std::vector<Float64> displ = pForces->GetXDeflection(intervalIdx, combination_type, vPoi, bat[analysisIdx], resultsType);
         AddGraphPoints(data_series_id[analysisIdx], xVals, displ);
         break;
      }

      case actionRotation:
         {
            bool bIncludeSlopeAdjustment = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeElevationAdjustment();
            bool bIncludePrecamber = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludePrecamber();
            std::vector<Float64> rotations = pForces->GetRotation( intervalIdx, combination_type, vPoi, bat[analysisIdx], resultsType, bIncludeSlopeAdjustment, bIncludePrecamber );
            AddGraphPoints(data_series_id[analysisIdx], xVals, rotations);
            break;
         }

      case actionStress:
         {
            pgsTypes::StressLocation topLocation = (IsGirderStressLocation(stressLocation[analysisIdx]) ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
            pgsTypes::StressLocation botLocation = (IsGirderStressLocation(stressLocation[analysisIdx]) ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);

            std::vector<Float64> fTop, fBot;
            pForces->GetStress( intervalIdx, combination_type, vPoi, bat[analysisIdx], resultsType, topLocation, botLocation, &fTop, &fBot );

            if ( IsTopStressLocation(stressLocation[analysisIdx]) )
            {
               AddGraphPoints(data_series_id[analysisIdx], xVals, fTop);
            }
            else
            {
               AddGraphPoints(data_series_id[analysisIdx], xVals, fBot);
            }
         break;
         }

      default:
         ATLASSERT(false);
      }
   }
}

void CAnalysisResultsGraphBuilder::LimitStateLoadGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear)
{
   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();

   CGirderKey girderKey(m_pGraphController->GetGirderKey());
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      girderKey.groupIndex = 0;
   }

   pgsTypes::LimitState limitState(graphDef.m_LoadType.LimitStateType);
   GraphType graphType(graphDef.m_GraphType);

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   bool bProcessNegativeMoments = false;
   if ( graphDef.m_GraphType == graphCapacity || graphDef.m_GraphType == graphMinCapacity )
   {
      GET_IFACE(IBridge,pBridge);
      SpanIndexType startSpanIdx, endSpanIdx;
      pBridge->GetGirderGroupSpans(girderKey.groupIndex,&startSpanIdx,&endSpanIdx);
      for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
      {
         if ( pBridge->ProcessNegativeMoments(spanIdx) )
         {
            bProcessNegativeMoments = true;
            break;
         }
      }
   }

   // Limit state forces
   COLORREF c(GetGraphColor(graphIdx,intervalIdx));
   int penWeight = (graphType == graphAllowable || graphType == graphCapacity ? 3 : 2);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();

   CString strDataLabel(GetDataLabel(graphIdx,graphDef,intervalIdx));

   // data series for moment, shears and deflections
   IndexType max_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,penWeight,c);
   IndexType min_data_series = m_Graph.CreateDataSeries(_T(""),      PS_SOLID,penWeight,c);

   // data series for allowable stresses and capacity
   IndexType max_girder_capacity_series = m_Graph.CreateDataSeries(strDataLabel+(strDataLabel.IsEmpty() ? _T("") : _T(" - Girder")),PS_SOLID,3,c);
   IndexType min_girder_capacity_series = m_Graph.CreateDataSeries(_T(""),PS_SOLID,penWeight,c);
   IndexType max_deck_capacity_series = m_Graph.CreateDataSeries(strDataLabel+(strDataLabel.IsEmpty() ? _T("") : _T(" - Deck")),PS_DOT,3,c);
   IndexType min_deck_capacity_series = m_Graph.CreateDataSeries(_T(""),PS_DOT,penWeight,c);

   // data series for stresses
   IndexType stress_max[4], stress_min[4];
   stress_max[pgsTypes::TopGirder]    = m_Graph.CreateDataSeries(strDataLabel + (strDataLabel.IsEmpty() ? _T("") : _T(" - Top Girder")),   PS_STRESS_TOP_GIRDER,   penWeight,c);
   stress_min[pgsTypes::TopGirder]    = m_Graph.CreateDataSeries(_T(""),                             PS_STRESS_TOP_GIRDER,   penWeight,c);
   stress_max[pgsTypes::BottomGirder] = m_Graph.CreateDataSeries(strDataLabel + (strDataLabel.IsEmpty() ? _T("") : _T(" - Bottom Girder")),PS_STRESS_BOTTOM_GIRDER,penWeight,c);
   stress_min[pgsTypes::BottomGirder] = m_Graph.CreateDataSeries(_T(""),                             PS_STRESS_BOTTOM_GIRDER,penWeight,c);

   stress_max[pgsTypes::TopDeck]    = m_Graph.CreateDataSeries(strDataLabel + (strDataLabel.IsEmpty() ? _T("") : _T(" - Top Deck")),   PS_STRESS_TOP_DECK,   penWeight,c);
   stress_min[pgsTypes::TopDeck]    = m_Graph.CreateDataSeries(_T(""),                           PS_STRESS_TOP_DECK,   penWeight,c);
   stress_max[pgsTypes::BottomDeck] = m_Graph.CreateDataSeries(strDataLabel + (strDataLabel.IsEmpty() ? _T("") : _T(" - Bottom Deck")),PS_STRESS_BOTTOM_DECK,penWeight,c);
   stress_min[pgsTypes::BottomDeck] = m_Graph.CreateDataSeries(_T(""),                           PS_STRESS_BOTTOM_DECK,penWeight,c);

   switch(actionType)
   {
   case actionAxial:
      {
         GET_IFACE(ILimitStateForces2,pForces);
         if ( analysisType == pgsTypes::Envelope )
         {
            std::vector<Float64> mmax, mmin;
            pForces->GetAxial( intervalIdx, limitState, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &mmin, &mmax );
            AddGraphPoints(max_data_series, xVals, mmax);

            pForces->GetAxial( intervalIdx, limitState, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &mmin, &mmax );
            AddGraphPoints(min_data_series, xVals, mmin);
         }
         else
         {
            std::vector<Float64> mmax, mmin;
            pForces->GetAxial( intervalIdx, limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &mmin, &mmax );
            AddGraphPoints(max_data_series, xVals, mmax);
            AddGraphPoints(min_data_series, xVals, mmin);
         }
      break;
      }
   case actionShear:
      {
         if ( graphType == graphCapacity )
         {
            GET_IFACE(IShearCapacity,pCapacity);
            std::vector<Float64> pVn = pCapacity->GetShearCapacity(limitState, intervalIdx, vPoi);
            AddGraphPoints(max_girder_capacity_series, xVals,  pVn);

            std::vector<Float64>::iterator iter;
            std::vector<Float64> nVn;
            for ( iter = pVn.begin(); iter != pVn.end(); iter++ )
            {
               nVn.push_back(-1*(*iter));
            }
            AddGraphPoints(min_girder_capacity_series, xVals, nVn);
         }
         else
         {
            GET_IFACE(ILimitStateForces2,pForces);
            if ( analysisType == pgsTypes::Envelope )
            {
               std::vector<sysSectionValue> shearMin, shearMax;
               pForces->GetShear( intervalIdx, limitState, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &shearMin, &shearMax );
               AddGraphPoints(min_data_series, xVals, shearMin);

               pForces->GetShear( intervalIdx, limitState, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &shearMin, &shearMax );
               AddGraphPoints(max_data_series, xVals, shearMax);
            }
            else
            {
               std::vector<sysSectionValue> shearMin, shearMax;
               pForces->GetShear( intervalIdx, limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &shearMin, &shearMax );
               AddGraphPoints(min_data_series, xVals, shearMin);
               AddGraphPoints(max_data_series, xVals, shearMax);
            }
         }
      break;
      }
   case actionMoment:
      {
         if ( graphType == graphCapacity )
         {
            GET_IFACE(IMomentCapacity,pCapacity);
            std::vector<Float64> pMn = pCapacity->GetMomentCapacity(intervalIdx,vPoi,true);
            AddGraphPoints(max_girder_capacity_series, xVals, pMn);

            if ( bProcessNegativeMoments )
            {
               std::vector<Float64> nMn = pCapacity->GetMomentCapacity(intervalIdx,vPoi,false);
               AddGraphPoints(min_girder_capacity_series, xVals, nMn);
            }
         }
         else if ( graphType == graphMinCapacity )
         {
            GET_IFACE(IMomentCapacity,pCapacity);
            std::vector<Float64> pMrMin = pCapacity->GetMinMomentCapacity(intervalIdx,vPoi,true);
            AddGraphPoints(max_girder_capacity_series, xVals, pMrMin);

            if ( bProcessNegativeMoments )
            {
               std::vector<Float64> nMrMin = pCapacity->GetMinMomentCapacity(intervalIdx,vPoi,false);
               AddGraphPoints(min_girder_capacity_series, xVals, nMrMin);
            }
         }
         else
         {
            GET_IFACE(ILimitStateForces2,pForces);
            if ( analysisType == pgsTypes::Envelope )
            {
               std::vector<Float64> mmax, mmin;
               pForces->GetMoment( intervalIdx, limitState, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &mmin, &mmax );
               AddGraphPoints(max_data_series, xVals, mmax);
               
               if ( liveLoadIntervalIdx <= intervalIdx && IsStrengthLimitState(limitState) )
               {
                  mmin = pForces->GetSlabDesignMoment(limitState,vPoi, pgsTypes::MinSimpleContinuousEnvelope );
               }
               else
               {
                  pForces->GetMoment( intervalIdx, limitState, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &mmin, &mmax );
               }

               AddGraphPoints(min_data_series, xVals, mmin);
            }
            else
            {
               std::vector<Float64> mmax, mmin;
               pForces->GetMoment( intervalIdx, limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &mmin, &mmax );
               AddGraphPoints(max_data_series, xVals, mmax);

               if ( liveLoadIntervalIdx <= intervalIdx && IsStrengthLimitState(limitState) )
               {
                  mmin = pForces->GetSlabDesignMoment(limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan );
               }

               AddGraphPoints(min_data_series, xVals, mmin);
            }
         }
      break;
      }
   case actionDeflection:
      {
        GET_IFACE(ILimitStateForces2,pForces);
         bool bIncPrestress = (graphType == graphDemand ? true : false);
         bool bIncludeLiveLoad = false;
         bool bIncludeElevationAdjustment = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeElevationAdjustment();
         bool bIncludePrecamber = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludePrecamber();
         if ( analysisType == pgsTypes::Envelope )
         {
            std::vector<Float64> dispmn, dispmx;
            pForces->GetDeflection( intervalIdx, limitState, vPoi, pgsTypes::MinSimpleContinuousEnvelope, bIncPrestress, bIncludeLiveLoad, bIncludeElevationAdjustment, bIncludePrecamber, &dispmn, &dispmx);
            AddGraphPoints(min_data_series, xVals, dispmn);

            pForces->GetDeflection( intervalIdx, limitState, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, bIncPrestress, bIncludeLiveLoad, bIncludeElevationAdjustment, bIncludePrecamber, &dispmn, &dispmx);
            AddGraphPoints(max_data_series, xVals, dispmx);
         }
         else
         {
            std::vector<Float64> dispmn, dispmx;
            pForces->GetDeflection( intervalIdx, limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncPrestress, bIncludeLiveLoad, bIncludeElevationAdjustment, bIncludePrecamber, &dispmn, &dispmx);
            AddGraphPoints(min_data_series, xVals, dispmn);
            AddGraphPoints(max_data_series, xVals, dispmx);
         }
      break;
      }
   case actionXDeflection:
      {
      GET_IFACE(ILimitStateForces2, pForces);
      bool bIncPrestress = (graphType == graphDemand ? true : false);
      if (analysisType == pgsTypes::Envelope)
      {
         std::vector<Float64> dispmn, dispmx;
         pForces->GetXDeflection(intervalIdx, limitState, vPoi, pgsTypes::MinSimpleContinuousEnvelope, bIncPrestress, &dispmn, &dispmx);
         AddGraphPoints(min_data_series, xVals, dispmn);

         pForces->GetXDeflection(intervalIdx, limitState, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, bIncPrestress, &dispmn, &dispmx);
         AddGraphPoints(max_data_series, xVals, dispmx);
      }
      else
      {
         std::vector<Float64> dispmn, dispmx;
         pForces->GetXDeflection(intervalIdx, limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncPrestress, &dispmn, &dispmx);
         AddGraphPoints(min_data_series, xVals, dispmn);
         AddGraphPoints(max_data_series, xVals, dispmx);
      }
      break;
   }
   case actionRotation:
      {
         GET_IFACE(ILimitStateForces2,pForces);
         bool bIncPrestress = (graphType == graphDemand ? true : false);
         bool bIncludeLiveLoad = false;
         bool bIncludeSlopeAdjustment = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludeElevationAdjustment();
         bool bIncludePrecamber = ((CAnalysisResultsGraphController*)m_pGraphController)->IncludePrecamber();
         if ( analysisType == pgsTypes::Envelope )
         {
            std::vector<Float64> minRotation, maxRotation;
            pForces->GetRotation( intervalIdx, limitState, vPoi, pgsTypes::MinSimpleContinuousEnvelope, bIncPrestress, bIncludeLiveLoad, bIncludeSlopeAdjustment, bIncludePrecamber, &minRotation, &maxRotation);
            AddGraphPoints(min_data_series, xVals, minRotation);

            pForces->GetRotation( intervalIdx, limitState, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, bIncPrestress, bIncludeLiveLoad, bIncludeSlopeAdjustment, bIncludePrecamber, &minRotation, &maxRotation);
            AddGraphPoints(max_data_series, xVals, maxRotation);
         }
         else
         {
            std::vector<Float64> minRotation, maxRotation;
            pForces->GetRotation( intervalIdx, limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncPrestress, bIncludeLiveLoad, bIncludeSlopeAdjustment, bIncludePrecamber, &minRotation, &maxRotation);
            AddGraphPoints(min_data_series, xVals, minRotation);
            AddGraphPoints(max_data_series, xVals, maxRotation);
         }
      break;
      }
   case actionStress:
      {
         if ( graphType == graphAllowable )
         {
            GET_IFACE(IAllowableConcreteStress,pAllowable);

            if ( ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(pgsTypes::TopGirder) ||
                 ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(pgsTypes::BottomGirder) )
            {
               if ( pAllowable->IsStressCheckApplicable(girderKey,intervalIdx,limitState,pgsTypes::Tension) )
               {
                  std::vector<Float64> t(pAllowable->GetGirderAllowableTensionStress(vPoi,intervalIdx,limitState,false/*without rebar*/,false/*not in PTZ*/));
                  AddGraphPoints(min_girder_capacity_series, xVals, t);
                  m_Graph.SetDataLabel(min_girder_capacity_series,strDataLabel + (strDataLabel.IsEmpty() ? _T("") : _T(" - Girder")));
               }

               if ( pAllowable->IsStressCheckApplicable(girderKey,intervalIdx,limitState,pgsTypes::Compression) )
               {
                  std::vector<Float64> c( pAllowable->GetGirderAllowableCompressionStress(vPoi,intervalIdx,limitState) );
                  AddGraphPoints(max_girder_capacity_series, xVals, c);
               }
            }

            if ( ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(pgsTypes::TopDeck) ||
                 ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(pgsTypes::BottomDeck) )
            {
               if ( pAllowable->IsStressCheckApplicable(girderKey,intervalIdx,limitState,pgsTypes::Tension) )
               {
                  std::vector<Float64> t(pAllowable->GetDeckAllowableTensionStress(vPoi,intervalIdx,limitState,false/*without rebar*/));
                  AddGraphPoints(min_deck_capacity_series, xVals, t);
                  m_Graph.SetDataLabel(min_deck_capacity_series,strDataLabel + (strDataLabel.IsEmpty() ? _T("") : _T(" - Deck")));
               }

               if ( pAllowable->IsStressCheckApplicable(girderKey,intervalIdx,limitState,pgsTypes::Compression) )
               {
                  std::vector<Float64> c( pAllowable->GetDeckAllowableCompressionStress(vPoi,intervalIdx,limitState) );
                  AddGraphPoints(max_deck_capacity_series, xVals, c);
               }
            }
         }
         else
         {
            GET_IFACE(ILimitStateForces2,pForces);
            bool bIncPrestress = (graphType == graphDemand ? true : false);
            std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;

            for ( int i = 0; i < 4; i++ )
            {
               pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;

               bool bPlotStresses = ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(stressLocation);
               if ( bPlotStresses )
               {
                  pgsTypes::StressLocation topLocation = (IsGirderStressLocation(stressLocation) ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
                  pgsTypes::StressLocation botLocation = (IsGirderStressLocation(stressLocation) ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);
                  if ( analysisType == pgsTypes::Envelope )
                  {
                     std::vector<Float64> dummy;
                     pForces->GetStress( intervalIdx, limitState, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, bIncPrestress, topLocation, &dummy,   &fTopMax);
                     pForces->GetStress( intervalIdx, limitState, vPoi, pgsTypes::MinSimpleContinuousEnvelope, bIncPrestress, topLocation, &fTopMin, &dummy);
                     pForces->GetStress( intervalIdx, limitState, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, bIncPrestress, botLocation, &dummy,   &fBotMax);
                     pForces->GetStress( intervalIdx, limitState, vPoi, pgsTypes::MinSimpleContinuousEnvelope, bIncPrestress, botLocation, &fBotMin, &dummy);
                  }
                  else
                  {
                     pForces->GetStress( intervalIdx, limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncPrestress, topLocation, &fTopMin, &fTopMax);
                     pForces->GetStress( intervalIdx, limitState, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncPrestress, botLocation, &fBotMin, &fBotMax);
                  }

                  if ( IsTopStressLocation(stressLocation) )
                  {
                     AddGraphPoints(stress_min[stressLocation], xVals, fTopMin);
                     AddGraphPoints(stress_max[stressLocation], xVals, fTopMax);
                  }
                  else
                  {
                     AddGraphPoints(stress_min[stressLocation], xVals, fBotMin);
                     AddGraphPoints(stress_max[stressLocation], xVals, fBotMax);
                  }
               }
            } // next stress location
         }
      break;
      }

   default:
         ATLASSERT(false);
   }
}

void CAnalysisResultsGraphBuilder::LiveLoadGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear)
{
   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();

   CGirderKey girderKey(m_pGraphController->GetGirderKey());
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      girderKey.groupIndex = 0;
   }

   // Live Load
   pgsTypes::LiveLoadType llType( graphDef.m_LoadType.LiveLoadType );

   COLORREF c(GetGraphColor(graphIdx,intervalIdx));

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   VehicleIndexType vehicleIdx(graphDef.m_VehicleIndex);
   ATLASSERT(vehicleIdx == INVALID_INDEX);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   if ( intervalIdx < liveLoadIntervalIdx )
   {
      return;
   }

   GET_IFACE(ICombinedForces2,pForces);

   CString strDataLabel(GetDataLabel(graphIdx,graphDef,intervalIdx));
   if ( ((CAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode() == CAnalysisResultsGraphController::Loading && !strDataLabel.IsEmpty() )
   {
      strDataLabel += _T(" (per girder)");
   }

   int penWeight = GRAPH_PEN_WEIGHT;

   // data series for moment, shears and deflections
   IndexType min_data_series;
   IndexType max_data_series;
   if (actionType == actionShear )
   {
      //if (actionType == actionShear && !bIsFinalShear)
      //{
      //   strDataLabel = _T("");
      //}

      int penStyle = (/*liveLoadIntervalIdx <= intervalIdx && actionType == actionShear && !bIsFinalShear ? PS_DOT :*/ PS_SOLID);

      min_data_series = m_Graph.CreateDataSeries(strDataLabel, penStyle, penWeight, c);
      max_data_series = m_Graph.CreateDataSeries(_T(""),       penStyle, penWeight, c);
   }
   else
   {
      min_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,penWeight,c);
      max_data_series = m_Graph.CreateDataSeries(_T(""),PS_SOLID,penWeight,c);
   }

   // data series for stresses
   IndexType stress_max[4], stress_min[4];
   stress_max[pgsTypes::TopGirder]    = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top Girder"),   PS_STRESS_TOP_GIRDER,   penWeight,c);
   stress_min[pgsTypes::TopGirder]    = m_Graph.CreateDataSeries(_T(""),                             PS_STRESS_TOP_GIRDER,   penWeight,c);
   stress_max[pgsTypes::BottomGirder] = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom Girder"),PS_STRESS_BOTTOM_GIRDER,penWeight,c);
   stress_min[pgsTypes::BottomGirder] = m_Graph.CreateDataSeries(_T(""),                             PS_STRESS_BOTTOM_GIRDER,penWeight,c);

   stress_max[pgsTypes::TopDeck]    = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top Deck"),   PS_STRESS_TOP_DECK,   penWeight,c);
   stress_min[pgsTypes::TopDeck]    = m_Graph.CreateDataSeries(_T(""),                           PS_STRESS_TOP_DECK,   penWeight,c);
   stress_max[pgsTypes::BottomDeck] = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom Deck"),PS_STRESS_BOTTOM_DECK,penWeight,c);
   stress_min[pgsTypes::BottomDeck] = m_Graph.CreateDataSeries(_T(""),                           PS_STRESS_BOTTOM_DECK,penWeight,c);

   switch(actionType)
   {
   case actionAxial:
      {
         std::vector<Float64> Pmin, Pmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            pForces->GetCombinedLiveLoadAxial(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &Pmin, &Pmax);
            AddGraphPoints(min_data_series, xVals, Pmin);

            pForces->GetCombinedLiveLoadAxial(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &Pmin, &Pmax);
            AddGraphPoints(max_data_series, xVals, Pmax);
         }
         else
         {
            pForces->GetCombinedLiveLoadAxial(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &Pmin, &Pmax);
            AddGraphPoints(min_data_series, xVals, Pmin);
            AddGraphPoints(max_data_series, xVals, Pmax);
         }
      break;
      }

   case actionShear:
      {
         std::vector<sysSectionValue> Vmin, Vmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            pForces->GetCombinedLiveLoadShear(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, &Vmin, &Vmax);
            AddGraphPoints(min_data_series, xVals, Vmin);

            pForces->GetCombinedLiveLoadShear(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, &Vmin, &Vmax);
            AddGraphPoints(max_data_series, xVals, Vmax);
         }
         else
         {
            pForces->GetCombinedLiveLoadShear(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, &Vmin, &Vmax);
            AddGraphPoints(min_data_series, xVals, Vmin);
            AddGraphPoints(max_data_series, xVals, Vmax);
         }
      break;
      }

   case actionMoment:
      {
         std::vector<Float64> Mmin, Mmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            pForces->GetCombinedLiveLoadMoment(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &Mmin, &Mmax);
            AddGraphPoints(min_data_series, xVals, Mmin);

            pForces->GetCombinedLiveLoadMoment(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &Mmin, &Mmax);
            AddGraphPoints(max_data_series, xVals, Mmax);
         }
         else
         {
            pForces->GetCombinedLiveLoadMoment(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &Mmin, &Mmax);
            AddGraphPoints(min_data_series, xVals, Mmin);
            AddGraphPoints(max_data_series, xVals, Mmax);
         }
      break;
      }

   case actionDeflection:
   {
      std::vector<Float64> Dmin, Dmax;
      if (analysisType == pgsTypes::Envelope)
      {
         pForces->GetCombinedLiveLoadDeflection(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &Dmin, &Dmax);
         AddGraphPoints(min_data_series, xVals, Dmin);

         pForces->GetCombinedLiveLoadDeflection(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &Dmin, &Dmax);
         AddGraphPoints(max_data_series, xVals, Dmax);
      }
      else
      {
         pForces->GetCombinedLiveLoadDeflection(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &Dmin, &Dmax);
         AddGraphPoints(min_data_series, xVals, Dmin);
         AddGraphPoints(max_data_series, xVals, Dmax);
      }
      break;
   }

   case actionXDeflection:
   {
      // there are no lateral live load deflections
      std::vector<Float64> Dmin, Dmax;
      Dmin.resize(vPoi.size(), 0.0);
      Dmax.resize(vPoi.size(), 0.0);
      AddGraphPoints(min_data_series, xVals, Dmin);
      AddGraphPoints(max_data_series, xVals, Dmax);
      break;
   }

   case actionRotation:
      {
         std::vector<Float64> Rmin, Rmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            pForces->GetCombinedLiveLoadRotation(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, &Rmin, &Rmax);
            AddGraphPoints(min_data_series, xVals, Rmin);

            pForces->GetCombinedLiveLoadRotation(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, &Rmin, &Rmax);
            AddGraphPoints(max_data_series, xVals, Rmax);
         }
         else
         {
            pForces->GetCombinedLiveLoadRotation(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, &Rmin, &Rmax);
            AddGraphPoints(min_data_series, xVals, Rmin);
            AddGraphPoints(max_data_series, xVals, Rmax);
         }
      break;
      }

   case actionStress:
      {
         std::vector<Float64> fTopMin,fTopMax,fBotMin,fBotMax;
         for ( int i = 0; i < 4; i++ )
         {
            pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;

            bool bPlotStresses = ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(stressLocation);
            if ( bPlotStresses )
            {
               pgsTypes::StressLocation topLocation = (IsGirderStressLocation(stressLocation) ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
               pgsTypes::StressLocation botLocation = (IsGirderStressLocation(stressLocation) ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);
               if ( analysisType == pgsTypes::Envelope )
               {
                  pForces->GetCombinedLiveLoadStress(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, topLocation, botLocation, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
               }
               else
               {
                  pForces->GetCombinedLiveLoadStress(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, topLocation, botLocation, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
               }

               if ( IsTopStressLocation(stressLocation) )
               {
                  AddGraphPoints(stress_min[stressLocation], xVals, fTopMin);
                  AddGraphPoints(stress_max[stressLocation], xVals, fTopMax);
               }
               else
               {
                  AddGraphPoints(stress_min[stressLocation], xVals, fBotMin);
                  AddGraphPoints(stress_max[stressLocation], xVals, fBotMax);
               }
            } // if bPlotStresses
         } // next stress location
      break;
      }
      default:
         ATLASSERT(false);
   }
}

void CAnalysisResultsGraphBuilder::VehicularLiveLoadGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals,bool bIsFinalShear)
{
   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();

   CGirderKey girderKey(m_pGraphController->GetGirderKey());
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      girderKey.groupIndex = 0;
   }

   // Live Load
   COLORREF c(GetGraphColor(graphIdx,intervalIdx));
   int penWeight = GRAPH_PEN_WEIGHT;

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   pgsTypes::LiveLoadType llType(graphDef.m_LoadType.LiveLoadType);
   VehicleIndexType vehicleIdx(graphDef.m_VehicleIndex);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   if ( intervalIdx < liveLoadIntervalIdx )
   {
      return;
   }

   GET_IFACE(IProductForces2,pForces);

   CString strDataLabel(GetDataLabel(graphIdx,graphDef,intervalIdx));
   if ( ((CAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode() == CAnalysisResultsGraphController::Loading && !strDataLabel.IsEmpty() )
   {
      strDataLabel += _T(" (per lane)");
   }

   // data series for moment, shears and deflections
   IndexType min_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,penWeight,c);
   IndexType max_data_series = m_Graph.CreateDataSeries(_T(""),PS_SOLID,penWeight,c);
   if (actionType == actionShear )
   {
      //if (actionType == actionShear && !bIsFinalShear)
      //{
      //   strDataLabel = _T("");
      //}

      int penStyle = (/*liveLoadIntervalIdx <= intervalIdx && actionType == actionShear && !bIsFinalShear ? PS_DOT :*/ PS_SOLID);

      min_data_series = m_Graph.CreateDataSeries(strDataLabel, penStyle, penWeight, c);
      max_data_series = m_Graph.CreateDataSeries(_T(""), penStyle, penWeight, c);
   }

   // data series for stresses
   IndexType stress_max[4], stress_min[4];
   stress_max[pgsTypes::TopGirder]    = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top Girder"),   PS_STRESS_TOP_GIRDER,   penWeight,c);
   stress_min[pgsTypes::TopGirder]    = m_Graph.CreateDataSeries(_T(""),                             PS_STRESS_TOP_GIRDER,   penWeight,c);
   stress_max[pgsTypes::BottomGirder] = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom Girder"),PS_STRESS_BOTTOM_GIRDER,penWeight,c);
   stress_min[pgsTypes::BottomGirder] = m_Graph.CreateDataSeries(_T(""),                             PS_STRESS_BOTTOM_GIRDER,penWeight,c);

   stress_max[pgsTypes::TopDeck]    = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top Deck"),   PS_STRESS_TOP_DECK,   penWeight,c);
   stress_min[pgsTypes::TopDeck]    = m_Graph.CreateDataSeries(_T(""),                           PS_STRESS_TOP_DECK,   penWeight,c);
   stress_max[pgsTypes::BottomDeck] = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom Deck"),PS_STRESS_BOTTOM_DECK,penWeight,c);
   stress_min[pgsTypes::BottomDeck] = m_Graph.CreateDataSeries(_T(""),                           PS_STRESS_BOTTOM_DECK,penWeight,c);

   switch(actionType)
   {
   case actionAxial:
      {
         std::vector<Float64> Pmin, Pmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadAxial(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Pmin, &Pmax);
            }
            else
            {
               pForces->GetLiveLoadAxial(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Pmin, &Pmax);
            }

            AddGraphPoints(min_data_series, xVals, Pmin);

            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadAxial(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Pmin, &Pmax);
            }
            else
            {
               pForces->GetLiveLoadAxial(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Pmin, &Pmax);
            }

            AddGraphPoints(max_data_series, xVals, Pmax);
         }
         else
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadAxial(intervalIdx, llType, vehicleIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Pmin, &Pmax, nullptr, nullptr);
            }
            else
            {
               pForces->GetLiveLoadAxial(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Pmin, &Pmax, nullptr, nullptr);
            }

            AddGraphPoints(min_data_series, xVals, Pmin);
            AddGraphPoints(max_data_series, xVals, Pmax);
         }
      break;
      }

   case actionShear:
      {
         std::vector<sysSectionValue> Vmin, Vmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadShear(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Vmin, &Vmax);
            }
            else
            {
               pForces->GetLiveLoadShear(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Vmin, &Vmax);
            }
 
            AddGraphPoints(min_data_series, xVals, Vmin);

            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadShear(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Vmin, &Vmax);
            }
            else
            {
               pForces->GetLiveLoadShear(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Vmin, &Vmax);
            }

            AddGraphPoints(max_data_series, xVals, Vmax);
         }
         else
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadShear(intervalIdx, llType, vehicleIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Vmin, &Vmax);
            }
            else
            {
               pForces->GetLiveLoadShear(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Vmin, &Vmax);
            }

            AddGraphPoints(min_data_series, xVals, Vmin);
            AddGraphPoints(max_data_series, xVals, Vmax);
         }
      break;
      }

   case actionMoment:
      {
         std::vector<Float64> Mmin, Mmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadMoment(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Mmin, &Mmax);
            }
            else
            {
               pForces->GetLiveLoadMoment(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Mmin, &Mmax);
            }

            AddGraphPoints(min_data_series, xVals, Mmin);

            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadMoment(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Mmin, &Mmax);
            }
            else
            {
               pForces->GetLiveLoadMoment(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Mmin, &Mmax);
            }

            AddGraphPoints(max_data_series, xVals, Mmax);
         }
         else
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadMoment(intervalIdx, llType, vehicleIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Mmin, &Mmax, nullptr, nullptr);
            }
            else
            {
               pForces->GetLiveLoadMoment(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Mmin, &Mmax, nullptr, nullptr);
            }

            AddGraphPoints(min_data_series, xVals, Mmin);
            AddGraphPoints(max_data_series, xVals, Mmax);
         }
      break;
      }

   case actionDeflection:
      {
         std::vector<Float64> Dmin, Dmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadDeflection(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Dmin, &Dmax);
            }
            else
            {
               pForces->GetLiveLoadDeflection(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Dmin, &Dmax);
            }

            AddGraphPoints(min_data_series, xVals, Dmin);

            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadDeflection(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Dmin, &Dmax);
            }
            else
            {
               pForces->GetLiveLoadDeflection(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Dmin, &Dmax);
            }

            AddGraphPoints(max_data_series, xVals, Dmax);
         }
         else
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadDeflection(intervalIdx, llType, vehicleIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Dmin, &Dmax);
            }
            else
            {
               pForces->GetLiveLoadDeflection(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Dmin, &Dmax);
            }

            AddGraphPoints(min_data_series, xVals, Dmin);
            AddGraphPoints(max_data_series, xVals, Dmax);
         }
      break;
      }


   case actionXDeflection:
   {
      // there are no lateral live load deflections
      std::vector<Float64> Dmin, Dmax;
      Dmin.resize(vPoi.size(), 0.0);
      Dmax.resize(vPoi.size(), 0.0);
      AddGraphPoints(min_data_series, xVals, Dmin);
      AddGraphPoints(max_data_series, xVals, Dmax);
      break;
   }

   case actionRotation:
      {
         std::vector<Float64> Rmin, Rmax;
         if ( analysisType == pgsTypes::Envelope )
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadRotation(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Rmin, &Rmax);
            }
            else
            {
               pForces->GetLiveLoadRotation(intervalIdx, llType, vPoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Rmin, &Rmax);
            }

            AddGraphPoints(min_data_series, xVals, Rmin);

            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadRotation(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Rmin, &Rmax);
            }
            else
            {
               pForces->GetLiveLoadRotation(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Rmin, &Rmax);
            }

            AddGraphPoints(max_data_series, xVals, Rmax);
         }
         else
         {
            if ( vehicleIdx != INVALID_INDEX )
            {
               pForces->GetVehicularLiveLoadRotation(intervalIdx, llType, vehicleIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Rmin, &Rmax);
            }
            else
            {
               pForces->GetLiveLoadRotation(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Rmin, &Rmax);
            }

            AddGraphPoints(min_data_series, xVals, Rmin);
            AddGraphPoints(max_data_series, xVals, Rmax);
         }
      break;
      }

   case actionStress:
      {
         std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;

         for ( int i = 0; i < 4; i++ )
         {
            pgsTypes::StressLocation stressLocation = (pgsTypes::StressLocation)i;

            bool bPlotStresses = ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(stressLocation);
            if ( bPlotStresses )
            {
               pgsTypes::StressLocation topLocation = (IsGirderStressLocation(stressLocation) ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
               pgsTypes::StressLocation botLocation = (IsGirderStressLocation(stressLocation) ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);

               if ( analysisType == pgsTypes::Envelope )
               {
                  if ( vehicleIdx != INVALID_INDEX )
                  {
                     pForces->GetVehicularLiveLoadStress(intervalIdx, llType, vehicleIdx, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, topLocation, botLocation, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
                  }
                  else
                  {
                     pForces->GetLiveLoadStress(intervalIdx, llType, vPoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, topLocation, botLocation, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
                  }
               }
               else
               {
                  if ( vehicleIdx != INVALID_INDEX )
                  {
                     pForces->GetVehicularLiveLoadStress(intervalIdx, llType, vehicleIdx, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, topLocation, botLocation, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
                  }
                  else
                  {
                     pForces->GetLiveLoadStress(intervalIdx, llType, vPoi, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, topLocation, botLocation, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
                  }
               }

               if ( IsTopStressLocation(stressLocation) )
               {
                  AddGraphPoints(stress_min[stressLocation], xVals, fTopMin);
                  AddGraphPoints(stress_max[stressLocation], xVals, fTopMax);
               }
               else
               {
                  AddGraphPoints(stress_min[stressLocation], xVals, fBotMin);
                  AddGraphPoints(stress_max[stressLocation], xVals, fBotMax);
               }
            } // end if bPlotStresses
         } // next stress location
      break;
      }
      default:
         ATLASSERT(false);
   } // end switch
}

void CAnalysisResultsGraphBuilder::GetSegmentXValues(const CGirderKey& girderKey,SegmentIndexType segIdx,IntervalIndexType intervalIdx,std::vector<CSegmentKey>* pSegments,std::vector<Float64>* pLeftXVals,std::vector<Float64>* pRightXVals)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IPointOfInterest,pIPoi);

   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   // get the segments
   if ( segIdx == ALL_SEGMENTS )
   {
      for ( SegmentIndexType i = 0; i < nSegments; i++ )
      {
         pSegments->push_back(CSegmentKey(girderKey,i));
      }
   }
   else
   {
      pSegments->push_back(CSegmentKey(girderKey,segIdx));
   }

   // get the left and right support points
   std::vector<CSegmentKey>::iterator segmentKeyIter(pSegments->begin());
   std::vector<CSegmentKey>::iterator segmentKeyIterEnd(pSegments->end());
   for ( ; segmentKeyIter != segmentKeyIterEnd; segmentKeyIter++ )
   {
      const CSegmentKey& segmentKey = *segmentKeyIter;

      PoiAttributeType poiReference;
      if (intervalIdx == pIntervals->GetPrestressReleaseInterval(segmentKey))
      {
         poiReference = POI_RELEASED_SEGMENT;
      }
      else if (intervalIdx == pIntervals->GetLiftSegmentInterval(segmentKey))
      {
         poiReference = POI_LIFT_SEGMENT;
      }
      else if (intervalIdx == pIntervals->GetStorageInterval(segmentKey))
      {
         poiReference = POI_STORAGE_SEGMENT;
      }
      else if (intervalIdx == pIntervals->GetHaulSegmentInterval(segmentKey))
      {
         poiReference = POI_HAUL_SEGMENT;
      }
      else
      {
         poiReference = POI_ERECTED_SEGMENT;
      }

      PoiList vPoi;
      pIPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | poiReference, &vPoi);
      ATLASSERT(vPoi.size() == 2);
      
      Float64 Xg = pIPoi->ConvertPoiToGirderCoordinate(pgsPointOfInterest(segmentKey,0.0));
      pLeftXVals->push_back(Xg); // want to start plotting at the start face of the segment

      Xg = pIPoi->ConvertPoiToGirderCoordinate(vPoi.front());
      pLeftXVals->push_back(Xg);

      Xg = pIPoi->ConvertPoiToGirderCoordinate(vPoi.back());
      pRightXVals->push_back(Xg);

      Float64 Ls = pBridge->GetSegmentLength(segmentKey);
      Xg = pIPoi->ConvertPoiToGirderCoordinate(pgsPointOfInterest(segmentKey,Ls));
      pRightXVals->push_back(Xg); // end graph at end face of segment
   }
}

void CAnalysisResultsGraphBuilder::GetSupportXValues(const CGirderKey& girderKey,bool bIncludeTemporarySupports,std::vector<Float64>* pXVals,std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>* pSupports)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IPointOfInterest,pIPoi);
   std::map<Float64,std::pair<SupportIndexType,pgsTypes::SupportType>> supportMap;
   
   pXVals->push_back(0.0); // want to start the graph at the left end of the girder

   // get pier locations
   PierIndexType startPierIdx, endPierIdx;
   pBridge->GetGirderGroupPiers(girderKey.groupIndex,&startPierIdx,&endPierIdx);
   for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
   {
      Float64 Xgp;
      VERIFY( pBridge->GetPierLocation(girderKey,pierIdx,&Xgp) );
      Float64 Xg = pIPoi->ConvertGirderPathCoordinateToGirderCoordinate(girderKey,Xgp);

      // if the pier is an abutment, adjust the location so that the reactions get plotted
      // at the centerline of bearing instead of CL pier
      if ( pBridge->IsAbutment(pierIdx) )
      {
         if ( pierIdx == 0 )
         {
            Float64 brgOffset = pBridge->GetSegmentStartBearingOffset(CSegmentKey(girderKey,0));
            Xg += brgOffset;
         }
         else
         {
            SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);
            Float64 brgOffset = pBridge->GetSegmentEndBearingOffset(CSegmentKey(girderKey,nSegments-1));
            Xg -= brgOffset;
         }
      }
      supportMap.insert(std::make_pair(Xg,std::make_pair(pierIdx,pgsTypes::stPier)));
   }

   // get temporary support locations
   if ( bIncludeTemporarySupports )
   {
      SpanIndexType startSpanIdx = (SpanIndexType)startPierIdx;
      SpanIndexType endSpanIdx = (SpanIndexType)(endPierIdx-1);

      SupportIndexType nTS = pBridge->GetTemporarySupportCount();
      for ( SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++ )
      {
         if ( pBridge->GetTemporarySupportType(tsIdx) == pgsTypes::ErectionTower )
         {
            SpanIndexType spanIdx;
            Float64 Xspan;
            pBridge->GetTemporarySupportLocation(tsIdx,girderKey.girderIndex,&spanIdx,&Xspan);
            if ( startSpanIdx <= spanIdx && spanIdx <= endSpanIdx )
            {
               Float64 Xgp = pBridge->GetTemporarySupportLocation(tsIdx,girderKey.girderIndex);
               Float64 Xg = pIPoi->ConvertGirderPathCoordinateToGirderCoordinate(girderKey,Xgp);
               supportMap.insert(std::make_pair(Xg,std::make_pair(tsIdx,pgsTypes::stTemporary)));
            }
         }
      }
   }

   // setup the location and support vectors
   std::map<Float64,std::pair<SupportIndexType,pgsTypes::SupportType>>::iterator supportMapIter(supportMap.begin());
   std::map<Float64,std::pair<SupportIndexType,pgsTypes::SupportType>>::iterator supportMapIterEnd(supportMap.end());
   for ( ; supportMapIter != supportMapIterEnd; supportMapIter++ )
   {
      pXVals->push_back(supportMapIter->first);
      pSupports->push_back(supportMapIter->second);
   }

   Float64 Lg = pBridge->GetGirderLength(girderKey);
   pXVals->push_back(Lg); // last point is at the end of the girder
}

void CAnalysisResultsGraphBuilder::ProductReactionGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const CGirderKey& girderKey,SegmentIndexType segIdx)
{
   pgsTypes::ProductForceType pfType(graphDef.m_LoadType.ProductLoadType);

   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();
   ATLASSERT(actionType == actionReaction);

   GET_IFACE(IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType firstSegmentErectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   
   // get global X values of the reactions
   std::vector<Float64> leftXVals,rightXVals;
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   std::vector<CSegmentKey> segmentKeys;

   if ( intervalIdx < firstSegmentErectionIntervalIdx )
   {
      // reactions are begin requested before any of the segments are erected
      // so we are going to use IReaction::GetSegmentReaction. We need the
      // segments to get reactions for and the location of the support points
      GetSegmentXValues(girderKey,segIdx,intervalIdx,&segmentKeys,&leftXVals,&rightXVals);
      std::transform(leftXVals.cbegin(), leftXVals.cend(), leftXVals.begin(), [&](const auto& value) {return value + m_GroupOffset;});
      std::transform(rightXVals.cbegin(),rightXVals.cend(),rightXVals.begin(),[&](const auto& value) {return value + m_GroupOffset;});
   }
   else
   {
      // segments have been erected so we are getting reactions from the bridge model
      // IReactions::GetReactions

      // Get locations of piers and temporary supports. 
      GetSupportXValues(girderKey,true,&leftXVals,&vSupports);
      std::transform(leftXVals.cbegin(),leftXVals.cend(),leftXVals.begin(),[&](const auto& value) {return value + m_GroupOffset;});
   }

   GET_IFACE(IReactions,pReactions);

   IndexType data_series_id[4];
   pgsTypes::BridgeAnalysisType bat[4];
   pgsTypes::StressLocation stressLocation[4];
   IndexType nAnalysisTypes;
   InitializeGraph(graphIdx,graphDef,actionType,intervalIdx,false,data_series_id,bat,stressLocation,&nAnalysisTypes);

   for ( IndexType analysisIdx = 0; analysisIdx < nAnalysisTypes; analysisIdx++ )
   {
      if ( intervalIdx < firstSegmentErectionIntervalIdx )
      {
         std::vector<Float64> Rleft, Rright;
         pReactions->GetSegmentReactions(segmentKeys,intervalIdx,pfType,bat[analysisIdx],resultsType,&Rleft,&Rright);

         // put values of 0.0 at the locations that correspond to the face of the member locations 
         // in leftXVals and rightXVals
         int i = 0;
         std::vector<CSegmentKey>::iterator segKeyIter(segmentKeys.begin());
         std::vector<CSegmentKey>::iterator segKeyIterEnd(segmentKeys.end());
         for ( ; segKeyIter != segKeyIterEnd; segKeyIter++, i+=2 )
         {
            Rleft.insert(Rleft.begin()+i,0.0);
            Rright.insert(Rright.begin()+i+1,0.0);
         }

         std::vector<Float64>::iterator leftXValIter(leftXVals.begin());
         std::vector<Float64>::iterator leftXValIterEnd(leftXVals.end());
         std::vector<Float64>::iterator rightXValIter(rightXVals.begin());
         std::vector<Float64>::iterator RleftIter(Rleft.begin());
         std::vector<Float64>::iterator RrightIter(Rright.begin());
         for ( ; leftXValIter != leftXValIterEnd; leftXValIter++, rightXValIter++, RleftIter++, RrightIter++ )
         {
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,*RleftIter);
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);

            AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,0.0);
            AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,*RrightIter);
            AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,0.0);
         }
      }
      else
      {
         std::vector<REACTION> reactions;
         reactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,pfType,bat[analysisIdx],resultsType);
         reactions.insert(reactions.begin(),REACTION()); // matches first point in leftXVals
         reactions.push_back(REACTION()); // matches last point in leftXVals
         std::vector<Float64>::iterator leftXValIter(leftXVals.begin());
         std::vector<Float64>::iterator leftXValIterEnd(leftXVals.end());
         std::vector<REACTION>::iterator reactionIter(reactions.begin());
         for ( ; leftXValIter != leftXValIterEnd; leftXValIter++, reactionIter++ )
         {
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,(*reactionIter).Fy);
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);
         }
      }
   } // next analysis type
}

void CAnalysisResultsGraphBuilder::CombinedReactionGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const CGirderKey& girderKey,SegmentIndexType segIdx)
{
   LoadingCombinationType comboType(graphDef.m_LoadType.CombinedLoadType);

   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();
   ATLASSERT(actionType == actionReaction);

   GET_IFACE(IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType firstSegmentErectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   
   // get global X values of the reactions
   std::vector<Float64> leftXVals,rightXVals;
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   std::vector<CSegmentKey> segmentKeys;

   if ( intervalIdx < firstSegmentErectionIntervalIdx )
   {
      // reactions are begin requested before any of the segments are erected
      // so we are going to use IReaction::GetSegmentReaction. We need the
      // segments to get reactions for and the location of the support points
      GetSegmentXValues(girderKey,segIdx,intervalIdx,&segmentKeys,&leftXVals,&rightXVals);
      std::transform(leftXVals.cbegin(),leftXVals.cend(),leftXVals.begin(),[&](const auto& value) {return value + m_GroupOffset;});
      std::transform(rightXVals.cbegin(),rightXVals.cend(),rightXVals.begin(),[&](const auto& value) {return value + m_GroupOffset;});
   }
   else
   {
      // segments have been erected so we are getting reactions from the bridge model
      // IReactions::GetReactions

      // Get locations of piers and temporary supports. 
      GetSupportXValues(girderKey,true,&leftXVals,&vSupports);
      std::transform(leftXVals.cbegin(),leftXVals.cend(),leftXVals.begin(),[&](const auto& value) {return value + m_GroupOffset;});
   }

   GET_IFACE(IReactions,pReactions);

   IndexType data_series_id[4];
   pgsTypes::BridgeAnalysisType bat[4];
   pgsTypes::StressLocation stressLocation[4];
   IndexType nAnalysisTypes;
   InitializeGraph(graphIdx,graphDef,actionType,intervalIdx,false,data_series_id,bat,stressLocation,&nAnalysisTypes);

   for ( IndexType analysisIdx = 0; analysisIdx < nAnalysisTypes; analysisIdx++ )
   {
      if ( intervalIdx < firstSegmentErectionIntervalIdx )
      {
         std::vector<Float64> Rleft, Rright;
         pReactions->GetSegmentReactions(segmentKeys,intervalIdx,comboType,bat[analysisIdx],resultsType,&Rleft,&Rright);

         // put values of 0.0 at the locations that correspond to the face of the member locations 
         // in leftXVals and rightXVals
         int i = 0;
         std::vector<CSegmentKey>::iterator segKeyIter(segmentKeys.begin());
         std::vector<CSegmentKey>::iterator segKeyIterEnd(segmentKeys.end());
         for ( ; segKeyIter != segKeyIterEnd; segKeyIter++, i+=2 )
         {
            Rleft.insert(Rleft.begin()+i,0.0);
            Rright.insert(Rright.begin()+i+1,0.0);
         }

         std::vector<Float64>::iterator leftXValIter(leftXVals.begin());
         std::vector<Float64>::iterator leftXValIterEnd(leftXVals.end());
         std::vector<Float64>::iterator rightXValIter(rightXVals.begin());
         std::vector<Float64>::iterator RleftIter(Rleft.begin());
         std::vector<Float64>::iterator RrightIter(Rright.begin());
         for ( ; leftXValIter != leftXValIterEnd; leftXValIter++, rightXValIter++, RleftIter++, RrightIter++ )
         {
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,*RleftIter);
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);

            AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,0.0);
            AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,*RrightIter);
            AddGraphPoint(data_series_id[analysisIdx],*rightXValIter,0.0);
         }
      }
      else
      {
         std::vector<REACTION> reactions;
         reactions = pReactions->GetReaction(girderKey,vSupports,intervalIdx,comboType,bat[analysisIdx],resultsType);
         reactions.insert(reactions.begin(),REACTION()); // matches first point in leftXVals
         reactions.push_back(REACTION()); // matches last point in leftXVals
         std::vector<Float64>::iterator leftXValIter(leftXVals.begin());
         std::vector<Float64>::iterator leftXValIterEnd(leftXVals.end());
         std::vector<REACTION>::iterator reactionIter(reactions.begin());
         for ( ; leftXValIter != leftXValIterEnd; leftXValIter++, reactionIter++ )
         {
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,(*reactionIter).Fy);
            AddGraphPoint(data_series_id[analysisIdx],*leftXValIter,0.0);
         }
      }
   } // next analysis type
}

void CAnalysisResultsGraphBuilder::LimitStateReactionGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const CGirderKey& girderKey,SegmentIndexType segIdx)
{
   pgsTypes::LimitState limitState(graphDef.m_LoadType.LimitStateType);

   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();
   ATLASSERT(actionType == actionReaction);
   GraphType graphType(graphDef.m_GraphType);

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   GET_IFACE(IBridge,pBridge);
   SegmentIndexType nSegments = pBridge->GetSegmentCount(girderKey);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType firstSegmentErectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   
   // get global X values of the reactions
   std::vector<Float64> leftXVals,rightXVals;
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   std::vector<CSegmentKey> segmentKeys;

   if ( intervalIdx < firstSegmentErectionIntervalIdx )
   {
      // reactions are begin requested before any of the segments are erected
      // so we are going to use IReaction::GetSegmentReaction. We need the
      // segments to get reactions for and the location of the support points
      GetSegmentXValues(girderKey,segIdx,intervalIdx,&segmentKeys,&leftXVals,&rightXVals);
      std::transform(leftXVals.cbegin(),leftXVals.cend(),leftXVals.begin(),[&](const auto& value) {return value + m_GroupOffset;});
      std::transform(rightXVals.cbegin(),rightXVals.cend(),rightXVals.begin(),[&](const auto& value) {return value + m_GroupOffset;});
   }
   else
   {
      // segments have been erected so we are getting reactions from the bridge model
      // IReactions::GetReactions

      // Get locations of piers and temporary supports. 
      GetSupportXValues(girderKey,true,&leftXVals,&vSupports);
      std::transform(leftXVals.cbegin(),leftXVals.cend(),leftXVals.begin(),[&](const auto& value) {return value + m_GroupOffset;});
   }

   GET_IFACE(IReactions,pReactions);

   CString strDataLabel(GetDataLabel(graphIdx,graphDef,intervalIdx));

   COLORREF c(GetGraphColor(graphIdx,intervalIdx));
   int penWeight = (graphType == graphAllowable || graphType == graphCapacity ? 3 : 2);

   // data series for moment, shears and deflections
   IndexType max_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,penWeight,c);
   IndexType min_data_series = m_Graph.CreateDataSeries(_T(""),      PS_SOLID,penWeight,c);

   bool bIncludeImpact = true;

   if ( intervalIdx < firstSegmentErectionIntervalIdx )
   {
      std::vector<Float64> RleftMin, RleftMax, RrightMin, RrightMax;
      if ( analysisType == pgsTypes::Envelope )
      {
         std::vector<Float64> RdummyLeft,RdummyRight;
         pReactions->GetSegmentReactions(segmentKeys,intervalIdx,limitState,pgsTypes::MaxSimpleContinuousEnvelope,&RdummyLeft,&RleftMax,&RdummyRight,&RrightMax);
         pReactions->GetSegmentReactions(segmentKeys,intervalIdx,limitState,pgsTypes::MinSimpleContinuousEnvelope,&RleftMin,&RdummyLeft,&RrightMin,&RdummyRight);
      }
      else
      {
         pReactions->GetSegmentReactions(segmentKeys,intervalIdx,limitState,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,&RleftMin,&RleftMax,&RrightMin,&RrightMax);
      }

      // put values of 0.0 at the locations that correspond to the face of the member locations 
      // in leftXVals and rightXVals
      int i = 0;
      std::vector<CSegmentKey>::iterator segKeyIter(segmentKeys.begin());
      std::vector<CSegmentKey>::iterator segKeyIterEnd(segmentKeys.end());
      for ( ; segKeyIter != segKeyIterEnd; segKeyIter++, i+=2 )
      {
         RleftMin.insert(RleftMin.begin()+i,0.0);
         RleftMax.insert(RleftMax.begin()+i,0.0);
         RrightMin.insert(RrightMin.begin()+i+1,0.0);
         RrightMax.insert(RrightMax.begin()+i+1,0.0);
      }

      std::vector<Float64>::iterator leftXValIter(leftXVals.begin());
      std::vector<Float64>::iterator leftXValIterEnd(leftXVals.end());
      std::vector<Float64>::iterator rightXValIter(rightXVals.begin());
      std::vector<Float64>::iterator RleftMinIter(RleftMin.begin());
      std::vector<Float64>::iterator RleftMaxIter(RleftMax.begin());
      std::vector<Float64>::iterator RrightMinIter(RrightMin.begin());
      std::vector<Float64>::iterator RrightMaxIter(RrightMax.begin());
      for ( ; leftXValIter != leftXValIterEnd; leftXValIter++, rightXValIter++, RleftMinIter++, RleftMaxIter++, RrightMinIter++, RrightMaxIter++ )
      {
         AddGraphPoint(max_data_series,*leftXValIter,0.0);
         AddGraphPoint(max_data_series,*leftXValIter,*RleftMaxIter);
         AddGraphPoint(max_data_series,*leftXValIter,0.0);

         AddGraphPoint(max_data_series,*rightXValIter,0.0);
         AddGraphPoint(max_data_series,*rightXValIter,*RrightMaxIter);
         AddGraphPoint(max_data_series,*rightXValIter,0.0);

         AddGraphPoint(min_data_series,*leftXValIter,0.0);
         AddGraphPoint(min_data_series,*leftXValIter,*RleftMinIter);
         AddGraphPoint(min_data_series,*leftXValIter,0.0);

         AddGraphPoint(min_data_series,*rightXValIter,0.0);
         AddGraphPoint(min_data_series,*rightXValIter,*RrightMinIter);
         AddGraphPoint(min_data_series,*rightXValIter,0.0);
      }
   }
   else
   {
      std::vector<REACTION> Rmin, Rmax, Rdummy;
      if ( analysisType == pgsTypes::Envelope )
      {
         pReactions->GetReaction(girderKey,vSupports,intervalIdx,limitState,pgsTypes::MaxSimpleContinuousEnvelope,bIncludeImpact,&Rdummy,&Rmax);
         pReactions->GetReaction(girderKey,vSupports,intervalIdx,limitState,pgsTypes::MinSimpleContinuousEnvelope,bIncludeImpact,&Rmin,&Rdummy);
      }
      else
      {
         pReactions->GetReaction(girderKey,vSupports,intervalIdx,limitState,analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan,bIncludeImpact,&Rmin,&Rmax);
      }
      Rmin.insert(Rmin.begin(),REACTION()); // matches first point in leftXVals
      Rmin.push_back(REACTION()); // matches last point in leftXVals
      Rmax.insert(Rmax.begin(),REACTION()); // matches first point in leftXVals
      Rmax.push_back(REACTION()); // matches last point in leftXVals
      std::vector<Float64>::iterator leftXValIter(leftXVals.begin());
      std::vector<Float64>::iterator leftXValIterEnd(leftXVals.end());
      std::vector<REACTION>::iterator RminIter(Rmin.begin());
      std::vector<REACTION>::iterator RmaxIter(Rmax.begin());
      for ( ; leftXValIter != leftXValIterEnd; leftXValIter++, RminIter++, RmaxIter++ )
      {
         AddGraphPoint(max_data_series,*leftXValIter,0.0);
         AddGraphPoint(max_data_series,*leftXValIter,(*RmaxIter).Fy);
         AddGraphPoint(max_data_series,*leftXValIter,0.0);

         AddGraphPoint(min_data_series,*leftXValIter,0.0);
         AddGraphPoint(min_data_series,*leftXValIter,(*RminIter).Fy);
         AddGraphPoint(min_data_series,*leftXValIter,0.0);
      }
   }
}

void CAnalysisResultsGraphBuilder::LiveLoadReactionGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const CGirderKey& girderKey)
{
   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();

   // Live Load
   bool bIncludeImpact = true;

   std::vector<Float64> Xvals;
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;

   GetSupportXValues(girderKey,false/*no temp supports*/,&Xvals,&vSupports);
   std::transform(Xvals.cbegin(),Xvals.cend(),Xvals.begin(),[&](const auto& value) {return value + m_GroupOffset;});
   
   std::vector<PierIndexType> vPiers;
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>::iterator supportIter(vSupports.begin());
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>::iterator supportIterEnd(vSupports.end());
   for ( ; supportIter != supportIterEnd; supportIter++ )
   {
      ATLASSERT(supportIter->second == pgsTypes::stPier);
      vPiers.push_back(supportIter->first);
   }

   COLORREF c(GetGraphColor(graphIdx,intervalIdx));
   int penWeight = GRAPH_PEN_WEIGHT;

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   pgsTypes::LiveLoadType llType(graphDef.m_LoadType.LiveLoadType);
   VehicleIndexType vehicleIdx(graphDef.m_VehicleIndex);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   if ( intervalIdx < liveLoadIntervalIdx )
   {
      return;
   }

   CString strDataLabel(GetDataLabel(graphIdx,graphDef,intervalIdx));
   if ( ((CAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode() == CAnalysisResultsGraphController::Loading && !strDataLabel.IsEmpty() )
   {
      strDataLabel += _T(" (per girder)");
   }

   // data series for moment, shears and deflections
   IndexType min_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,penWeight,c);
   IndexType max_data_series = m_Graph.CreateDataSeries(_T(""),PS_SOLID,penWeight,c);

   GET_IFACE(IReactions,pReactions);

   std::vector<Float64> Rmin, Rmax, Rdummy;

   if ( analysisType == pgsTypes::Envelope )
   {
      pReactions->GetCombinedLiveLoadReaction(intervalIdx,llType,vPiers, girderKey, pgsTypes::MinSimpleContinuousEnvelope, bIncludeImpact, &Rmin, &Rdummy);

      Rmin.insert(Rmin.begin(),0.0);
      Rmin.push_back(0.0);

      pReactions->GetCombinedLiveLoadReaction(intervalIdx,llType,vPiers, girderKey, pgsTypes::MaxSimpleContinuousEnvelope, bIncludeImpact, &Rdummy, &Rmax);

      Rmax.insert(Rmax.begin(),0.0);
      Rmax.push_back(0.0);
   }
   else
   {
      pReactions->GetCombinedLiveLoadReaction(intervalIdx,llType,vPiers, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, bIncludeImpact, &Rmin, &Rmax);

      Rmin.insert(Rmin.begin(),0.0);
      Rmin.push_back(0.0);

      Rmax.insert(Rmax.begin(),0.0);
      Rmax.push_back(0.0);
   }

   std::vector<Float64>::iterator XvalIter(Xvals.begin());
   std::vector<Float64>::iterator XvalIterEnd(Xvals.end());
   std::vector<Float64>::iterator RminIter(Rmin.begin());
   std::vector<Float64>::iterator RmaxIter(Rmax.begin());
   for ( ; XvalIter != XvalIterEnd; XvalIter++, RminIter++, RmaxIter++ )
   {
      AddGraphPoint(max_data_series,*XvalIter,0.0);
      AddGraphPoint(max_data_series,*XvalIter,*RmaxIter);
      AddGraphPoint(max_data_series,*XvalIter,0.0);

      AddGraphPoint(min_data_series,*XvalIter,0.0);
      AddGraphPoint(min_data_series,*XvalIter,*RminIter);
      AddGraphPoint(min_data_series,*XvalIter,0.0);
   }
}

void CAnalysisResultsGraphBuilder::VehicularLiveLoadReactionGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const CGirderKey& girderKey)
{
   ActionType actionType  = ((CAnalysisResultsGraphController*)m_pGraphController)->GetActionType();
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();

   // Live Load
   std::vector<Float64> Xvals;
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;

   GetSupportXValues(girderKey,false/*no temp supports*/,&Xvals,&vSupports);
   std::transform(Xvals.cbegin(),Xvals.cend(),Xvals.begin(),[&](const auto& value) {return value + m_GroupOffset;});
   
   std::vector<PierIndexType> vPiers;
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>::iterator supportIter(vSupports.begin());
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>::iterator supportIterEnd(vSupports.end());
   for ( ; supportIter != supportIterEnd; supportIter++ )
   {
      ATLASSERT(supportIter->second == pgsTypes::stPier);
      vPiers.push_back(supportIter->first);
   }

   COLORREF c(GetGraphColor(graphIdx,intervalIdx));
   int penWeight = GRAPH_PEN_WEIGHT;

   pgsTypes::AnalysisType analysisType = GetAnalysisType();

   pgsTypes::LiveLoadType llType(graphDef.m_LoadType.LiveLoadType);
   VehicleIndexType vehicleIdx(graphDef.m_VehicleIndex);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   if ( intervalIdx < liveLoadIntervalIdx )
   {
      return;
   }


   CString strDataLabel(GetDataLabel(graphIdx,graphDef,intervalIdx));
   if ( ((CAnalysisResultsGraphController*)m_pGraphController)->GetGraphMode() == CAnalysisResultsGraphController::Loading && !strDataLabel.IsEmpty() )
   {
      strDataLabel += _T(" (per lane)");
   }

   // data series for moment, shears and deflections
   IndexType min_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,penWeight,c);
   IndexType max_data_series = m_Graph.CreateDataSeries(_T(""),PS_SOLID,penWeight,c);

   std::vector<REACTION> Rmin, Rmax, Rdummy;
   GET_IFACE(IReactions,pReactions);

   if ( analysisType == pgsTypes::Envelope )
   {
      if ( vehicleIdx != INVALID_INDEX )
      {
         pReactions->GetVehicularLiveLoadReaction(intervalIdx,llType,vehicleIdx, vPiers, girderKey, pgsTypes::MinSimpleContinuousEnvelope, true, false, &Rmin, &Rdummy);
      }
      else
      {
         pReactions->GetLiveLoadReaction(intervalIdx,llType,vPiers, girderKey, pgsTypes::MinSimpleContinuousEnvelope, true, false, pgsTypes::fetFy, &Rmin, &Rdummy);
      }

      Rmin.insert(Rmin.begin(),REACTION());
      Rmin.push_back(REACTION());

      if ( vehicleIdx != INVALID_INDEX )
      {
         pReactions->GetVehicularLiveLoadReaction(intervalIdx,llType,vehicleIdx, vPiers, girderKey, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &Rdummy, &Rmax);
      }
      else
      {
         pReactions->GetLiveLoadReaction(intervalIdx,llType,vPiers, girderKey, pgsTypes::MaxSimpleContinuousEnvelope, true, false, pgsTypes::fetFy, &Rdummy, &Rmax);
      }

      Rmax.insert(Rmax.begin(),REACTION());
      Rmax.push_back(REACTION());
   }
   else
   {
      if ( vehicleIdx != INVALID_INDEX )
      {
         pReactions->GetVehicularLiveLoadReaction(intervalIdx,llType,vehicleIdx, vPiers, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, &Rmin, &Rmax);
      }
      else
      {
         pReactions->GetLiveLoadReaction(intervalIdx,llType,vPiers, girderKey, analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan, true, false, pgsTypes::fetFy, &Rmin, &Rmax);
      }

      Rmin.insert(Rmin.begin(),REACTION());
      Rmin.push_back(REACTION());

      Rmax.insert(Rmax.begin(),REACTION());
      Rmax.push_back(REACTION());
   }

   std::vector<Float64>::iterator XvalIter(Xvals.begin());
   std::vector<Float64>::iterator XvalIterEnd(Xvals.end());
   std::vector<REACTION>::iterator RminIter(Rmin.begin());
   std::vector<REACTION>::iterator RmaxIter(Rmax.begin());
   for ( ; XvalIter != XvalIterEnd; XvalIter++, RminIter++, RmaxIter++ )
   {
      AddGraphPoint(max_data_series,*XvalIter,0.0);
      AddGraphPoint(max_data_series,*XvalIter,(*RmaxIter).Fy);
      AddGraphPoint(max_data_series,*XvalIter,0.0);

      AddGraphPoint(min_data_series,*XvalIter,0.0);
      AddGraphPoint(min_data_series,*XvalIter,(*RminIter).Fy);
      AddGraphPoint(min_data_series,*XvalIter,0.0);
   }
}

void CAnalysisResultsGraphBuilder::CyStressCapacityGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals)
{
   COLORREF c(GetGraphColor(graphIdx,intervalIdx));
   CString strDataLabel(GetDataLabel(graphIdx,graphDef,intervalIdx));
   pgsTypes::LimitState limitState(graphDef.m_LoadType.LimitStateType);

   int penWeight = 3;

   // data series min/max capacity(allowable)
   IndexType max_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,penWeight,c);
   IndexType min_data_series = m_Graph.CreateDataSeries(_T(""),      PS_SOLID,penWeight,c);

   // Allowable tension in cy is dependent on capacity - must get spec check results
   // First get pois using same request as spec check report
   GET_IFACE(IArtifact,pIArtifact);

   Float64 cap_prev = 0;
   Float64 x_prev = xVals.front(); // tension capacity can jump at a location. we must capture this
   bool first(true);
   auto i(vPoi.begin());
   auto end(vPoi.end());
   auto xIter(xVals.begin());
   for ( ; i != end; i++, xIter++ )
   {
      const pgsPointOfInterest& poi = *i;
      Float64 x = *xIter;

      const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(poi.GetSegmentKey());

      const pgsFlexuralStressArtifact* pMaxArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(intervalIdx,limitState,pgsTypes::Tension,    poi.GetID());
      const pgsFlexuralStressArtifact* pMinArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(intervalIdx,limitState,pgsTypes::Compression,poi.GetID());
      Float64 maxcap, mincap;
      if (pMaxArtifact != nullptr)
      {
         // compression is easy
         Float64 fTop = pMinArtifact->GetCapacity(pgsTypes::TopGirder);
         Float64 fBot = pMinArtifact->GetCapacity(pgsTypes::BottomGirder);
         mincap = Min(fTop,fBot);
         AddGraphPoint(min_data_series, x, mincap);

         // Tension - we must catch jumps
         // Use a simple rule: Jumps happen at starts/ends of high points
         if (pMaxArtifact->WasWithRebarAllowableStressUsed(pgsTypes::TopGirder))
         {
            fTop = pMaxArtifact->GetAlternativeAllowableTensileStress(pgsTypes::TopGirder);
         }
         else
         {
            fTop = pMaxArtifact->GetCapacity(pgsTypes::TopGirder);
         }

         if (pMaxArtifact->WasWithRebarAllowableStressUsed(pgsTypes::BottomGirder))
         {
            fBot = pMaxArtifact->GetAlternativeAllowableTensileStress(pgsTypes::BottomGirder);
         }
         else
         {
            fBot = pMaxArtifact->GetCapacity(pgsTypes::BottomGirder);
         }
         maxcap = Max(fTop,fBot);
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

void CAnalysisResultsGraphBuilder::DeckShrinkageStressGraph(IndexType graphIdx,const CAnalysisResultsGraphDefinition& graphDef,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals)
{
   ResultsType resultsType = ((CAnalysisResultsGraphController*)m_pGraphController)->GetResultsType();

   COLORREF c(GetGraphColor(graphIdx,intervalIdx));
   CString strDataLabel(GetDataLabel(graphIdx,graphDef,intervalIdx));

   int penWeight = GRAPH_PEN_WEIGHT;

   bool bPlotTop = ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(pgsTypes::TopGirder);
   bool bPlotBot = ((CAnalysisResultsGraphController*)m_pGraphController)->PlotStresses(pgsTypes::BottomGirder);

   // Deck shrinkage call is not stage dependent - assume deck shrinks after railing in installed
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType dsIntervalIdx = pIntervals->GetCompositeDeckInterval();

   // data series top/bot
   IndexType top_data_series(0), bot_data_series(0);

   if (bPlotTop)
   {
      CString dl = strDataLabel + _T(" - Top");
      top_data_series = m_Graph.CreateDataSeries(dl,PS_STRESS_TOP_GIRDER,penWeight,c);
   }

   if (bPlotBot)
   {
      CString dl = strDataLabel + _T(" - Bottom");
      bot_data_series = m_Graph.CreateDataSeries(dl, PS_STRESS_BOTTOM_GIRDER,penWeight,c);
   }

   GET_IFACE_NOCHECK(IProductForces,pProductForces);

   auto i(vPoi.begin());
   auto end(vPoi.end());
   auto xIter(xVals.begin());
   for ( ; i != end; i++, xIter++ )
   {
      const pgsPointOfInterest& poi = *i;
      Float64 x = *xIter;

      Float64 fTop(0.0), fBot(0.0);
      if(dsIntervalIdx <= intervalIdx)
      {
         pProductForces->GetDeckShrinkageStresses(poi, &fTop, &fBot);
      }

      if(bPlotTop)
      {
         AddGraphPoint(top_data_series, x, fTop);
      }

      if(bPlotBot)
      {
         AddGraphPoint(bot_data_series, x, fBot);
      }
   }
}

pgsTypes::AnalysisType CAnalysisResultsGraphBuilder::GetAnalysisType()
{
   return ((CAnalysisResultsGraphController*)m_pGraphController)->GetAnalysisType();
}

IntervalIndexType CAnalysisResultsGraphBuilder::GetBeamDrawInterval()
{
   CAnalysisResultsGraphController* pMyGraphController = (CAnalysisResultsGraphController*)m_pGraphController;
   std::vector<IntervalIndexType> vIntervals(pMyGraphController->GetSelectedIntervals());
   if ( 0 < vIntervals.size() )
   {
      return vIntervals.back();
   }

   CGirderKey girderKey = pMyGraphController->GetGirderKey();
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType intervalIdx = pIntervals->GetFirstPrestressReleaseInterval(girderKey);
   return intervalIdx;
}

void CAnalysisResultsGraphBuilder::GetSecondaryXValues(const PoiList& vPoi,const std::vector<Float64>& xVals,PoiList* pvPoi,std::vector<Float64>* pXvalues)
{
   *pXvalues = xVals;

   GET_IFACE(IShearCapacity,pShearCapacity);
   GET_IFACE(IPointOfInterest,pPoi);
   const std::vector<CRITSECTDETAILS>& vCSDetails = pShearCapacity->GetCriticalSectionDetails(pgsTypes::StrengthI,vPoi.front().get().GetSegmentKey());

   for (const pgsPointOfInterest& poi : vPoi)
   {
      ZoneIndexType csZoneIdx = pShearCapacity->GetCriticalSectionZoneIndex(pgsTypes::StrengthI,poi);
      if ( csZoneIdx != INVALID_INDEX )
      {
#if defined _DEBUG
         Float64 start,end;
         pShearCapacity->GetCriticalSectionZoneBoundary(pgsTypes::StrengthI,poi.GetSegmentKey(),csZoneIdx,&start,&end);
         Float64 Xg = pPoi->ConvertPoiToGirderCoordinate(poi);
         ATLASSERT( ::InRange(start,Xg,end) );
#endif

         PoiList vCSPoi;
         pPoi->GetCriticalSections(pgsTypes::StrengthI, poi.GetSegmentKey(), &vCSPoi);
         pvPoi->push_back(vCSPoi[csZoneIdx]);
      }
      else
      {
         pvPoi->push_back(poi);
      }
    }
}