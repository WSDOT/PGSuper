///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// TestAgentImp.cpp : Implementation of CTestAgentImp
#include "stdafx.h"
#include "TestAgent_i.h"
#include "TestAgent.h"
#include "TestAgentImp.h"

#include <IFace\File.h>
#include <IFace\VersionInfo.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>
#include <IFace\MomentCapacity.h>
#include <IFace\ShearCapacity.h>
#include <IFace\Artifact.h>
#include <IFace\Constructability.h>
#include <IFace\GirderHandlingPointOfInterest.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\StatusCenter.h>

#include <psgLib\ConnectionLibraryEntry.h>
#include <psgLib\SpecLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\DesignArtifact.h>
#include <PgsExt\LiftingCheckArtifact.h>
#include <PgsExt\HaulingCheckArtifact.h>
#include <PgsExt\AutoProgress.h>
#include <PgsExt\GirderLabel.h>

#include <Units\Units.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define QUITE(_x_) (IsZero(_x_) ? 0 : _x_)

/////////////////////////////////////////////////////////////////////////////
// CTestAgentImp

/////////////////////////////////////////////////////////////////////////////
// IAgent
//
STDMETHODIMP CTestAgentImp::SetBroker(IBroker* pBroker)
{
   AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP CTestAgentImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   pBrokerInit->RegInterface( IID_ITest1250,     this );

   return S_OK;
};

STDMETHODIMP CTestAgentImp::Init()
{
   AGENT_INIT;

   return S_OK;
}

STDMETHODIMP CTestAgentImp::Init2()
{
   return S_OK;
}

STDMETHODIMP CTestAgentImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_TestAgent;
   return S_OK;
}

STDMETHODIMP CTestAgentImp::Reset()
{
   return S_OK;
}

STDMETHODIMP CTestAgentImp::ShutDown()
{
   AGENT_CLEAR_INTERFACE_CACHE;
   return S_OK;
}

// ITest1250
bool CTestAgentImp::RunTest(long type,
                            const std::string& outputFileName,
                            const std::string poiFileName)
{
   // use run unit tests with numeric labeling
   pgsGirderLabel::UseAlphaLabel(false);

   // turn off diagnostics
   DIAG_WARNPOPUP(FALSE);

   // open poi file and results file
   std::ofstream resf;
   resf.open(outputFileName.c_str());

   std::ofstream poif;
   poif.open(poiFileName.c_str());
   if (!resf || !poif)
      return false;

   // create progress window
   GET_IFACE(IProgress,pProgress);
   pgsAutoProgress ap(pProgress);

   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();

   for (SpanIndexType span = 0; span < nSpans; span++)
   {
      // run tests
      switch (type)
      {
      case 2:
         if( RunDistFactorTest(resf, poif,span,0))
            return RunDistFactorTest(resf, poif,span,1);
         break;
      case 4:
         if (RunCrossSectionTest(resf, poif, span, 0) )
            return RunCrossSectionTest(resf, poif, span, 1);
         else
            return false; 
         break;
      case 6:
         if (RunDeadLoadActionTest(resf, poif, span, 0))
            return RunDeadLoadActionTest(resf, poif, span, 1);
         else
            return false;
         break;
      case 7:
         if (RunHL93Test(resf, poif, span,0))
            return RunHL93Test(resf, poif, span,1);
         else
            return false;
         break;
      case 8:
         if (RunCombinedLoadActionTest(resf, poif, span, 0))
            return RunCombinedLoadActionTest(resf, poif, span, 1);
         else
            return false;
         break;
      case 15:
      case 18:
         if (RunPrestressedISectionTest(resf, poif, span, 0))
            return RunPrestressedISectionTest(resf, poif, span, 1);
         else
            return false;
         break;
      case 50:
         return RunHandlingTest(resf, poif, span);
         break;
      case 499:
         if ( RunCamberTest(resf, poif, span, 0) )
            return RunCamberTest(resf,poif,span,1);
         break;
      case 500:
         if ( RunFabOptimizationTest(resf, poif, span, 0) )
            return RunFabOptimizationTest(resf, poif, span, 1);
         break;
      case 777:
         if ( RunDesignTest(resf, poif, span, 0) )
            return RunDesignTest(resf, poif, span, 1);
         break;

      case RUN_REGRESSION:

         VERIFY( RunDistFactorTest(resf, poif, span, 0) );
         VERIFY( RunDistFactorTest(resf, poif, span, 1) );
         VERIFY( RunCrossSectionTest(resf, poif,  span,0) );
         VERIFY( RunCrossSectionTest(resf, poif,  span,1) );
         VERIFY( RunDeadLoadActionTest(resf, poif, span, 0) );
         VERIFY( RunDeadLoadActionTest(resf, poif,  span,1) );
         VERIFY( RunHL93Test(resf, poif, span, 0) );
         VERIFY( RunHL93Test(resf, poif, span, 1) );
         VERIFY( RunCombinedLoadActionTest(resf, poif, span, 0) );
         VERIFY( RunCombinedLoadActionTest(resf, poif, span, 1) );
         VERIFY( RunPrestressedISectionTest(resf, poif, span, 0) );
         VERIFY( RunPrestressedISectionTest(resf, poif, span, 1) );
         VERIFY( RunHandlingTest(resf, poif, span));
         VERIFY( RunWsdotGirderScheduleTest(resf, poif, span, 0) );
         VERIFY( RunWsdotGirderScheduleTest(resf, poif, span, 1) );

         if (type==RUN_REGRESSION) // only do design for regression - cad test should have already run design
         {
            VERIFY( RunDesignTest(resf, poif, span, 0) );
            VERIFY( RunDesignTest(resf, poif, span, 1) );
         }

         VERIFY( RunCamberTest(resf,poif,span,0) );
         VERIFY( RunCamberTest(resf,poif,span,1) );

         VERIFY( RunFabOptimizationTest(resf, poif, span, 0) );
         VERIFY( RunFabOptimizationTest(resf, poif, span, 1) );

         return true;
         break;
      }
   }

   return false;
}

bool CTestAgentImp::RunTestEx(long type, const std::vector<SpanGirderHashType>& girderList,
                            const std::string& outputFileName,
                            const std::string poiFileName)
{
   // turn off diagnostics
   DIAG_WARNPOPUP(FALSE);

   // open poi file and results file
   std::ofstream resf;
   resf.open(outputFileName.c_str());

   std::ofstream poif;
   poif.open(poiFileName.c_str());
   if (!resf || !poif)
      return false;

   // create progress window
   GET_IFACE(IProgress,pProgress);
   pgsAutoProgress ap(pProgress);

   GET_IFACE(IBridge,pBridge);
   SpanIndexType nspans = pBridge->GetSpanCount();

   for(std::vector<SpanGirderHashType>::const_iterator it=girderList.begin(); it!=girderList.end(); it++)
   {
      SpanGirderHashType key = *it;
      SpanIndexType span;
	  GirderIndexType girder;
      UnhashSpanGirder(key, &span, &girder);

      // run tests
      switch (type)
      {
      case 2:
         if (!RunDistFactorTest(resf, poif, span, girder))
            return false;
         break;
      case 4:
         if (!RunCrossSectionTest(resf, poif, span, girder) )
            return false; 
         break;
      case 6:
         if (!RunDeadLoadActionTest(resf, poif, span, girder))
            return false;
         break;
      case 7:
         if (!RunHL93Test(resf, poif, span,girder))
            return false;
         break;
      case 8:
         if (!RunCombinedLoadActionTest(resf, poif, span, girder))
            return false;
         break;
      case 15:
      case 18:
         if (!RunPrestressedISectionTest(resf, poif, span, girder))
            return false;
         break;
      case 50:
         return RunHandlingTest(resf, poif, span);
         break;
      case 499:
         if ( !RunCamberTest(resf, poif, span, girder) )
            return false;
         break;
      case 500:
         if ( !RunFabOptimizationTest(resf, poif, span, girder) )
            return false;
         break;
      case 777:
         if ( !RunDesignTest(resf, poif, span, girder) )
            return false;
         break;

      case RUN_REGRESSION:
      case RUN_CADTEST:

         if (!RunDistFactorTest(resf, poif, span, girder) )
            return false;
         if (!RunCrossSectionTest(resf, poif,  span, girder) )
            return false;
         if (!RunDeadLoadActionTest(resf, poif, span, girder) )
            return false;
         if (!RunHL93Test(resf, poif, span, girder) )
            return false;
         if (!RunCombinedLoadActionTest(resf, poif, span, girder) )
            return false;
         if (!RunPrestressedISectionTest(resf, poif, span, girder) )
            return false;
         if (!RunHandlingTest(resf, poif, span))
            return false;
         if (!RunWsdotGirderScheduleTest(resf, poif, span, girder) )
            return false;

         if (type==RUN_REGRESSION) // only do design for regression - cad test should have already run design if requested
         {
            if (!RunDesignTest(resf, poif, span, girder) )
               return false;
         }

         if (!RunCamberTest(resf,poif,span, girder) )
            return false;
         if (!RunFabOptimizationTest(resf, poif, span, girder) )
            return false;
      }
   }

   return true;
}

std::string CTestAgentImp::GetBridgeID()
{
   static std::string strID;
   static bool bDone = false;

   if ( bDone )
      return strID;
   else
   {
      GET_IFACE(IFile,pFile);
      std::string strPath = pFile->GetFilePath();

      // Filename is in the form Regxxx.pgs
      std::string::size_type pos = strPath.find(".pgs");
      int len = strPath.length();
      if ( pos == std::string::npos )
         return 0; // "Reg" was not found

      strID = strPath.substr(pos-3,3); // return the "xxx" number

      bDone = true;
      return strID;
   }
}

std::string CTestAgentImp::GetProcessID()
{
   // the process ID is going to be the PGSuper version number
   GET_IFACE(IVersionInfo,pVI);
   std::string strVersion = pVI->GetVersion();
   return strVersion;
}

bool CTestAgentImp::RunDistFactorTest(std::ofstream& resultsFile, std::ofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE( ILiveLoadDistributionFactors, pDf );

   std::string pid      = GetProcessID();
   std::string bridgeId = GetBridgeID();

   // write to poi file
   poiFile<<" 1, "<< bridgeId<< ", 3, 1, 0.0000, 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0"<<std::endl;

   bool bResult1 = pDf->Run1250Tests(span,gdr,pgsTypes::StrengthI,pid.c_str(),bridgeId.c_str(),resultsFile,poiFile);

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      bResult1 = pDf->Run1250Tests(span,gdr,pgsTypes::FatigueI,pid.c_str(),bridgeId.c_str(),resultsFile,poiFile);
   }

   return bResult1;
}

bool CTestAgentImp::RunHL93Test(std::ofstream& resultsFile, std::ofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE( IProductForces, pForce);
   GET_IFACE( ISpecification,     pSpec);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   BridgeAnalysisType bat;

   std::string pid      = GetProcessID();
   std::string bridgeId = GetBridgeID();

   PoiAttributeType attrib = POI_TABULAR | POI_FLEXURESTRESS;
   std::vector<pgsPointOfInterest> vPoi;

   vPoi = pIPoi->GetPointsOfInterest(pgsTypes::BridgeSite3, span,gdr,attrib);

   Uint32 npoi = vPoi.size();
   for (Uint32 i=0; i<npoi; i++)
   {
      pgsPointOfInterest& rpoi = vPoi[i];
      Uint32 locn = i+1;
      Float64 loc = ::ConvertFromSysUnits(rpoi.GetDistFromStart(), unitMeasure::Millimeter);

      // write to poi file
      poiFile<<locn<<", "<< bridgeId<< ", 7, 1, "<<loc<<", 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0"<<std::endl;

      Float64         pm, nm;
      sysSectionValue sps, sns;
      Float64         pd, nd;
      // get live load results

      if ( analysisType == pgsTypes::Envelope )
      {
         Float64 dummy;
         sysSectionValue svDummy;

         pForce->GetLiveLoadMoment(pgsTypes::lltDesign, pgsTypes::BridgeSite3, rpoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &pm);
         pForce->GetLiveLoadMoment(pgsTypes::lltDesign, pgsTypes::BridgeSite3, rpoi, MinSimpleContinuousEnvelope, true, false, &nm, &dummy);

         pForce->GetLiveLoadShear(pgsTypes::lltDesign, pgsTypes::BridgeSite3,  rpoi, MaxSimpleContinuousEnvelope, true, false, &svDummy, &sps);
         pForce->GetLiveLoadShear(pgsTypes::lltDesign, pgsTypes::BridgeSite3,  rpoi, MinSimpleContinuousEnvelope, true, false, &sns, &svDummy);

         pForce->GetLiveLoadDisplacement(pgsTypes::lltDesign, pgsTypes::BridgeSite3,rpoi, MaxSimpleContinuousEnvelope, true, false, &dummy, &pd);
         pForce->GetLiveLoadDisplacement(pgsTypes::lltDesign, pgsTypes::BridgeSite3,rpoi, MinSimpleContinuousEnvelope, true, false, &nd, &dummy);
      }
      else
      {
         bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
         pForce->GetLiveLoadMoment(pgsTypes::lltDesign, pgsTypes::BridgeSite3, rpoi, bat, true, false, &nm, &pm);
         pForce->GetLiveLoadShear(pgsTypes::lltDesign, pgsTypes::BridgeSite3,  rpoi, bat, true, false, &sns, &sps);
         pForce->GetLiveLoadDisplacement(pgsTypes::lltDesign, pgsTypes::BridgeSite3,rpoi, bat, true, false, &nd, &pd);
      }

      // unit conversions
      pm = ::ConvertFromSysUnits(pm, unitMeasure::NewtonMillimeter);
      nm = ::ConvertFromSysUnits(nm, unitMeasure::NewtonMillimeter);
      Float64 ps = ::ConvertFromSysUnits(max(sps.Left(),sps.Right()), unitMeasure::Newton);
      Float64 ns = ::ConvertFromSysUnits(min(sns.Left(),sns.Right()), unitMeasure::Newton);
      pd = ::ConvertFromSysUnits(pd, unitMeasure::Millimeter);
      nd = ::ConvertFromSysUnits(nd, unitMeasure::Millimeter);

      resultsFile<<bridgeId<<", "<<pid<<", 32090, "<<loc<<", "<< QUITE(pm) <<", 7, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 32091, "<<loc<<", "<< QUITE(nm) <<", 7, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 32092, "<<loc<<", "<< QUITE(ps) <<", 7, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 32093, "<<loc<<", "<< QUITE(ns) <<", 7, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 32094, "<<loc<<", "<< QUITE(-nd) <<", 7, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 32095, "<<loc<<", "<< QUITE(-pd) <<", 7, "<<gdr<<std::endl;

      // deflection truck with full width stiffness
      pForce->GetDeflLiveLoadDisplacement( IProductForces::DesignTruckAlone, rpoi, &nd, &pd );
      pd = ::ConvertFromSysUnits(pd, unitMeasure::Millimeter);
      nd = ::ConvertFromSysUnits(nd, unitMeasure::Millimeter);
      resultsFile<<bridgeId<<", "<<pid<<", 32200, "<<loc<<", "<< QUITE(-nd) <<", 7, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 32201, "<<loc<<", "<< QUITE(-pd) <<", 7, "<<gdr<<std::endl;

   }
   return true;
}

bool CTestAgentImp::RunCrossSectionTest(std::ofstream& resultsFile, std::ofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE( ISectProp2, pSp2 );
   GET_IFACE( IGirder, pGdr);

   std::string pid      = GetProcessID();
   std::string bridgeId = GetBridgeID();

   // write to poi file
   poiFile<<" 1, "<< bridgeId<< ", 3, 1, 0.0000, 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0"<<std::endl;

   pgsPointOfInterest poi(span,gdr,0.0);

   // exterior girder
   // bare girder
   resultsFile<<bridgeId<<", "<<pid<<", 25000, 0.0, "<< QUITE(::ConvertFromSysUnits(pSp2->GetAg(pgsTypes::CastingYard,poi),  unitMeasure::Millimeter2)) <<", 4, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 25001, 0.0, "<< QUITE(::ConvertFromSysUnits(pSp2->GetVolume(span,gdr), unitMeasure::Millimeter3)) <<", 4, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 25002, 0.0, "<< QUITE(::ConvertFromSysUnits(pSp2->GetSurfaceArea(span,gdr), unitMeasure::Millimeter2)) <<", 4, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 25004, 0.0, "<< QUITE(::ConvertFromSysUnits(pSp2->GetIx(pgsTypes::CastingYard,poi), unitMeasure::Millimeter4)) <<", 4, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 25005, 0.0, "<< QUITE(::ConvertFromSysUnits(pSp2->GetYb(pgsTypes::CastingYard,poi), unitMeasure::Millimeter)) << ", 4, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 25006, 0.0, "<< QUITE(::ConvertFromSysUnits(pSp2->GetSb(pgsTypes::CastingYard,poi), unitMeasure::Millimeter3)) <<", 4, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 25007, 0.0, "<< QUITE(::ConvertFromSysUnits(pSp2->GetStGirder(pgsTypes::CastingYard,poi), unitMeasure::Millimeter3)) <<", 4, "<<gdr<<std::endl;

   // composite girder
   resultsFile<<bridgeId<<", "<<pid<<", 25008, 0.0, "<< QUITE(::ConvertFromSysUnits(pSp2->GetIx(pgsTypes::BridgeSite2,poi), unitMeasure::Millimeter4)) << ", 4, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 25009, 0.0, "<< QUITE(::ConvertFromSysUnits(pSp2->GetYb(pgsTypes::BridgeSite2,poi), unitMeasure::Millimeter)) <<  ", 4, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 25010, 0.0, "<< QUITE(::ConvertFromSysUnits(pSp2->GetSb(pgsTypes::BridgeSite2,poi), unitMeasure::Millimeter3)) << ", 4, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 25011, 0.0, "<< QUITE(::ConvertFromSysUnits(pSp2->GetStGirder(pgsTypes::BridgeSite2,poi), unitMeasure::Millimeter3)) <<", 4, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 25012, 0.0, "<< QUITE(::ConvertFromSysUnits(pSp2->GetSt(pgsTypes::BridgeSite2,poi), unitMeasure::Millimeter3)) << ", 4, "<<gdr<<std::endl;

   resultsFile<<bridgeId<<", "<<pid<<", 25031, 0.0, "<< QUITE(::ConvertFromSysUnits(pSp2->GetAcBottomHalf(poi),unitMeasure::Millimeter2)) <<", 4, 1"<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 25033, 0.0, "<< QUITE(::ConvertFromSysUnits(pGdr->GetHeight(poi), unitMeasure::Millimeter)) <<", 4, 1"<<std::endl;

   return true;
}

bool CTestAgentImp::RunDeadLoadActionTest(std::ofstream& resultsFile, std::ofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE( IPointOfInterest,   pIPoi);
   GET_IFACE( IProductLoads,      pLoads);
   GET_IFACE( IProductForces,     pForce);
   GET_IFACE( ICombinedForces,    pForces);
   GET_IFACE( ISpecification,     pSpec);
   GET_IFACE( IStrandGeometry,    pStrandGeom);
   GET_IFACE( IBridge,            pBridge);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   BridgeAnalysisType bat = (analysisType == pgsTypes::Simple     ? SimpleSpan : 
                             analysisType == pgsTypes::Continuous ? ContinuousSpan : MaxSimpleContinuousEnvelope);

   std::string pid      = GetProcessID();
   std::string bridgeId = GetBridgeID();

   PoiAttributeType attrib = POI_TABULAR | POI_FLEXURESTRESS;
   std::vector<pgsPointOfInterest> vPoi;

   vPoi = pIPoi->GetPointsOfInterest(pgsTypes::BridgeSite3, span,gdr,attrib);

   Uint32 npoi = vPoi.size();
   for (Uint32 i=0; i<npoi; i++)
   {
      pgsPointOfInterest& poi = vPoi[i];
      Uint32 locn = i+1;
      Float64 loc = ::ConvertFromSysUnits(poi.GetDistFromStart(), unitMeasure::Millimeter);

      // write to poi file
      poiFile<<locn<<", "<< bridgeId<< ", 7, 1, "<<loc<<", 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0"<<std::endl;

      // girder 
      pgsTypes::Stage girderLoadStage = pLoads->GetGirderDeadLoadStage(poi.GetSpan(),poi.GetGirder());

      resultsFile<<bridgeId<<", "<<pid<<", 30000, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( girderLoadStage, pftGirder, poi, bat ), unitMeasure::NewtonMillimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30001, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetShear( girderLoadStage, pftGirder, poi, bat ).Left(), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30002, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( girderLoadStage, pftGirder, poi, bat ), unitMeasure::Millimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30200, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( girderLoadStage, pftGirder, span, gdr, bat) , unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;

      // diaphragm
      resultsFile<<bridgeId<<", "<<pid<<", 30009, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite1, pftDiaphragm,poi, bat ), unitMeasure::NewtonMillimeter)) << ", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30010, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite1, pftDiaphragm, poi, bat ).Left(), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30011, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite1, pftDiaphragm, poi, bat ), unitMeasure::Millimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30209, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite1, pftDiaphragm, span, gdr, bat), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;

      // ShearKey
      resultsFile<<bridgeId<<", "<<pid<<", 30070, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite1, pftShearKey,poi, bat ), unitMeasure::NewtonMillimeter)) << ", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30071, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite1, pftShearKey, poi, bat ).Left(), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30072, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite1, pftShearKey, poi, bat ), unitMeasure::Millimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30270, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite1, pftShearKey, span, gdr, bat), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;
       
      // slab
      resultsFile<<bridgeId<<", "<<pid<<", 30012, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite1, pftSlab,poi, bat ), unitMeasure::NewtonMillimeter)) << ", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30013, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite1, pftSlab, poi, bat ).Left(), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30014, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite1, pftSlab, poi, bat ), unitMeasure::Millimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30013, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite1, pftSlab, span, gdr, bat), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;

      // DC - BSS1
      resultsFile<<bridgeId<<", "<<pid<<", 30036, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForces->GetMoment( lcDC, pgsTypes::BridgeSite1, poi, ctCummulative, bat ), unitMeasure::NewtonMillimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30037, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForces->GetShear( lcDC, pgsTypes::BridgeSite1, poi, ctCummulative, bat ).Left(), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30038, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForces->GetDisplacement( lcDC, pgsTypes::BridgeSite1, poi, ctCummulative, bat ), unitMeasure::Millimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30236, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForces->GetReaction( lcDC, pgsTypes::BridgeSite1, span, gdr, ctCummulative, bat ), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;

      // DW - BSS1
      resultsFile<<bridgeId<<", "<<pid<<", 30039, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForces->GetMoment( lcDW, pgsTypes::BridgeSite1, poi, ctCummulative, bat ), unitMeasure::NewtonMillimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30040, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForces->GetShear( lcDW, pgsTypes::BridgeSite1, poi, ctCummulative, bat ).Left(), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30041, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForces->GetDisplacement( lcDW, pgsTypes::BridgeSite1, poi, ctCummulative, bat ), unitMeasure::Millimeter)) <<", 1, "<<gdr<<std::endl;

      // overlay
      pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;

      resultsFile<<bridgeId<<", "<<pid<<", 30042, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( overlay_stage, pftOverlay,poi, bat ), unitMeasure::NewtonMillimeter)) << ", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30043, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetShear( overlay_stage, pftOverlay, poi, bat ).Left(), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30044, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( overlay_stage, pftOverlay, poi, bat ), unitMeasure::Millimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30242, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( overlay_stage, pftOverlay, span, gdr, bat), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;

      // barrier
      resultsFile<<bridgeId<<", "<<pid<<", 30045, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite2, pftTrafficBarrier,poi, bat ), unitMeasure::NewtonMillimeter)) << ", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30046, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite2, pftTrafficBarrier, poi, bat ).Left(), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30047, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite2, pftTrafficBarrier, poi, bat ), unitMeasure::Millimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30245, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite2, pftTrafficBarrier, span, gdr, bat ), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;

      // sidewalk
      resultsFile<<bridgeId<<", "<<pid<<", 30048, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite2, pftSidewalk,poi, bat ), unitMeasure::NewtonMillimeter)) << ", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30049, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite2, pftSidewalk, poi, bat ).Left(), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30050, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite2, pftSidewalk, poi, bat ), unitMeasure::Millimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30248, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite2, pftSidewalk, span, gdr, bat ), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;

      // DC - BSS3
      resultsFile<<bridgeId<<", "<<pid<<", 30057, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForces->GetMoment( lcDC, pgsTypes::BridgeSite3, poi, ctCummulative, bat ), unitMeasure::NewtonMillimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30058, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForces->GetShear( lcDC, pgsTypes::BridgeSite3, poi, ctCummulative, bat ).Left(), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30059, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForces->GetDisplacement( lcDC, pgsTypes::BridgeSite3, poi, ctCummulative, bat ), unitMeasure::Millimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30257, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForces->GetReaction( lcDC, pgsTypes::BridgeSite3, span, gdr, ctCummulative, bat ), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;

      // DW - BSS3
      resultsFile<<bridgeId<<", "<<pid<<", 30060, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForces->GetMoment( lcDW, pgsTypes::BridgeSite3, poi, ctCummulative, bat ), unitMeasure::NewtonMillimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30061, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForces->GetShear( lcDW, pgsTypes::BridgeSite3, poi, ctCummulative, bat ).Left(), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30062, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForces->GetDisplacement( lcDW, pgsTypes::BridgeSite3, poi, ctCummulative, bat ), unitMeasure::Millimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 30260, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForces->GetReaction( lcDW, pgsTypes::BridgeSite3, span, gdr, ctCummulative, bat ), unitMeasure::Newton)) <<    ", 1, "<<gdr<<std::endl;

      // user loads - Moment
      resultsFile<<bridgeId<<", "<<pid<<", 122100, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite1, pftUserDW, poi, bat ), unitMeasure::NewtonMillimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122101, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite1, pftUserDC, poi, bat ), unitMeasure::NewtonMillimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122102, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite2, pftUserDW, poi, bat ), unitMeasure::NewtonMillimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122103, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite2, pftUserDC, poi, bat ), unitMeasure::NewtonMillimeter)) <<", 1, "<<gdr<<std::endl;

      // user loads - Shear
      resultsFile<<bridgeId<<", "<<pid<<", 122104, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite1, pftUserDW, poi, bat ).Left(), unitMeasure::Newton)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122105, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite1, pftUserDC, poi, bat ).Left(), unitMeasure::Newton)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122106, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite2, pftUserDW, poi, bat ).Left(), unitMeasure::Newton)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122107, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite2, pftUserDC, poi, bat ).Left(), unitMeasure::Newton)) <<", 1, "<<gdr<<std::endl;

      // user loads - Displacement
      resultsFile<<bridgeId<<", "<<pid<<", 122108, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite1, pftUserDW, poi, bat ), unitMeasure::Millimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122109, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite1, pftUserDC, poi, bat ), unitMeasure::Millimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122110, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite2, pftUserDW, poi, bat ), unitMeasure::Millimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122111, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite2, pftUserDC, poi, bat ), unitMeasure::Millimeter)) <<", 1, "<<gdr<<std::endl;

      // user loads - Reaction
      resultsFile<<bridgeId<<", "<<pid<<", 122112, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite1, pftUserDW, span, gdr, bat ), unitMeasure::Newton)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122113, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite1, pftUserDC, span, gdr, bat ), unitMeasure::Newton)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122114, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite2, pftUserDW, span, gdr, bat ), unitMeasure::Newton)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122115, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite2, pftUserDC, span, gdr, bat ), unitMeasure::Newton)) <<", 1, "<<gdr<<std::endl;

      // user live load
      resultsFile<<bridgeId<<", "<<pid<<", 122116, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite1, pftUserLLIM, poi, bat ), unitMeasure::NewtonMillimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122117, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite1, pftUserLLIM, poi, bat ).Left(), unitMeasure::Newton)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122118, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite1, pftUserLLIM, poi, bat ), unitMeasure::Millimeter)) <<", 1, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122119, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite1, pftUserLLIM, span, gdr, bat ), unitMeasure::Newton)) <<", 1, "<<gdr<<std::endl;

   }
   return true;
}

bool CTestAgentImp::RunCombinedLoadActionTest(std::ofstream& resultsFile, std::ofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE( IPointOfInterest,   pIPoi);
   GET_IFACE( ILimitStateForces,  pLsForces);
   GET_IFACE( ISpecification,     pSpec);

   std::string pid      = GetProcessID();
   std::string bridgeId = GetBridgeID();

   PoiAttributeType attrib = POI_TABULAR | POI_FLEXURESTRESS;
   std::vector<pgsPointOfInterest> vPoi;

   vPoi = pIPoi->GetPointsOfInterest(pgsTypes::BridgeSite3, span,gdr,attrib);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   BridgeAnalysisType bat;

   Uint32 npoi = vPoi.size();
   for (Uint32 i=0; i<npoi; i++)
   {
      pgsPointOfInterest& poi = vPoi[i];
      Uint32 locn = i+1;
      Float64 loc = ::ConvertFromSysUnits(poi.GetDistFromStart(), unitMeasure::Millimeter);

      // write to poi file
      poiFile<<locn<<", "<< bridgeId<< ", 7, 1, "<<loc<<", 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0"<<std::endl;

      // Strength I 
      Float64 min, max, dummy;
      if ( analysisType == pgsTypes::Envelope )
      {
         pLsForces->GetMoment( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &dummy, &max );
         pLsForces->GetMoment( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &min, &dummy );
         resultsFile<<bridgeId<<", "<<pid<<", 34000, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(max, unitMeasure::NewtonMillimeter)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34001, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min, unitMeasure::NewtonMillimeter)) <<", 8, "<<gdr<<std::endl;

         sysSectionValue smin, smax, svDummy;
         pLsForces->GetShear( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &svDummy, &smax );
         pLsForces->GetShear( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &smin, &svDummy );
         resultsFile<<bridgeId<<", "<<pid<<", 34002, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(smax.Left(), unitMeasure::Newton)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34003, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(smin.Left(), unitMeasure::Newton)) <<", 8, "<<gdr<<std::endl;

         pLsForces->GetDisplacement( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &dummy, &max );
         pLsForces->GetDisplacement( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &min, &dummy );
         resultsFile<<bridgeId<<", "<<pid<<", 34004, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Millimeter)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34005, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Millimeter)) <<", 8, "<<gdr<<std::endl;

         // Service I
         pLsForces->GetMoment( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &dummy, &max );
         pLsForces->GetMoment( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &min, &dummy );
         resultsFile<<bridgeId<<", "<<pid<<", 34020, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(max, unitMeasure::NewtonMillimeter)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34021, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min, unitMeasure::NewtonMillimeter)) <<", 8, "<<gdr<<std::endl;

         pLsForces->GetShear( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &svDummy, &smax );
         pLsForces->GetShear( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &smin, &svDummy );
         resultsFile<<bridgeId<<", "<<pid<<", 34022, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(smax.Left(), unitMeasure::Newton)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34023, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(smin.Left(), unitMeasure::Newton)) <<", 8, "<<gdr<<std::endl;

         pLsForces->GetDisplacement( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &dummy, &max );
         pLsForces->GetDisplacement( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &min, &dummy );
         resultsFile<<bridgeId<<", "<<pid<<", 34024, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Millimeter)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34025, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Millimeter)) <<", 8, "<<gdr<<std::endl;

         // Service III
         pLsForces->GetMoment( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &dummy, &max );
         pLsForces->GetMoment( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &min, &dummy );
         resultsFile<<bridgeId<<", "<<pid<<", 34032, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(max, unitMeasure::NewtonMillimeter)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34033, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min, unitMeasure::NewtonMillimeter)) <<", 8, "<<gdr<<std::endl;

         pLsForces->GetShear( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &svDummy, &smax );
         pLsForces->GetShear( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &smin, &svDummy );
         resultsFile<<bridgeId<<", "<<pid<<", 34034, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(smax.Left(), unitMeasure::Newton)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34035, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(smin.Left(), unitMeasure::Newton)) <<", 8, "<<gdr<<std::endl;

         pLsForces->GetDisplacement( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &dummy, &max );
         pLsForces->GetDisplacement( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &min, &dummy );
         resultsFile<<bridgeId<<", "<<pid<<", 34036, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Millimeter)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34037, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Millimeter)) <<", 8, "<<gdr<<std::endl;
      }
      else
      {
         bat = (pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
         pLsForces->GetMoment( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, bat, &min, &max );
         resultsFile<<bridgeId<<", "<<pid<<", 34000, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(max, unitMeasure::NewtonMillimeter)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34001, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min, unitMeasure::NewtonMillimeter)) <<", 8, "<<gdr<<std::endl;

         sysSectionValue smin, smax;
         pLsForces->GetShear( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, bat, &smin, &smax );
         resultsFile<<bridgeId<<", "<<pid<<", 34002, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(smax.Left(), unitMeasure::Newton)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34003, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(smin.Left(), unitMeasure::Newton)) <<", 8, "<<gdr<<std::endl;

         pLsForces->GetDisplacement( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, bat, &min, &max );
         resultsFile<<bridgeId<<", "<<pid<<", 34004, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Millimeter)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34005, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Millimeter)) <<", 8, "<<gdr<<std::endl;

         // Service I
         pLsForces->GetMoment( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, bat, &min, &max );
         resultsFile<<bridgeId<<", "<<pid<<", 34020, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(max, unitMeasure::NewtonMillimeter)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34021, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min, unitMeasure::NewtonMillimeter)) <<", 8, "<<gdr<<std::endl;

         pLsForces->GetShear( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, bat, &smin, &smax );
         resultsFile<<bridgeId<<", "<<pid<<", 34022, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(smax.Left(), unitMeasure::Newton)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34023, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(smin.Left(), unitMeasure::Newton)) <<", 8, "<<gdr<<std::endl;

         pLsForces->GetDisplacement( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, bat, &min, &max );
         resultsFile<<bridgeId<<", "<<pid<<", 34024, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Millimeter)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34025, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Millimeter)) <<", 8, "<<gdr<<std::endl;

         // Service III
         pLsForces->GetMoment( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, bat, &min, &max );
         resultsFile<<bridgeId<<", "<<pid<<", 34032, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(max, unitMeasure::NewtonMillimeter)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34033, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min, unitMeasure::NewtonMillimeter)) <<", 8, "<<gdr<<std::endl;

         pLsForces->GetShear( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, bat, &smin, &smax );
         resultsFile<<bridgeId<<", "<<pid<<", 34034, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(smax.Left(), unitMeasure::Newton)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34035, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(smin.Left(), unitMeasure::Newton)) <<", 8, "<<gdr<<std::endl;

         pLsForces->GetDisplacement( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, bat, &min, &max );
         resultsFile<<bridgeId<<", "<<pid<<", 34036, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Millimeter)) <<", 8, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 34037, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Millimeter)) <<", 8, "<<gdr<<std::endl;
      }
   }
   return true;
}

bool CTestAgentImp::RunPrestressedISectionTest(std::ofstream& resultsFile, std::ofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE( IPointOfInterest,   pIPoi);
   GET_IFACE( ISectProp2, pSp2 );
   GET_IFACE( IPrestressForce, pPrestressForce );
   GET_IFACE( IPrestressStresses, pPrestressStresses );
   GET_IFACE( ILosses, pLosses );
   GET_IFACE( ILimitStateForces,pLsForces);
   GET_IFACE( IMomentCapacity,pMomentCapacity);
   GET_IFACE( IShearCapacity,pShearCapacity);
   GET_IFACE( IBridge, pBridge);
   GET_IFACE( IArtifact,pIArtifact);

   std::string pid      = GetProcessID();
   std::string bridgeId = GetBridgeID();

   // first dump out non-poi related values
   // losses
   Float64 loc = 0.0;

   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,gdr);
   const pgsStirrupCheckArtifact* pstirrup_artifact= gdrArtifact->GetStirrupCheckArtifact();
   resultsFile<<bridgeId<<", "<<pid<<", 100200 , "<<loc<<", "<<(int)(gdrArtifact->Passed()?1:0) <<", 15, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 100201 , "<<loc<<", "<<(int)(pstirrup_artifact->Passed()?1:0)<<", 15, "<<gdr<<std::endl;

   Float64 min, max;
   pShearCapacity->GetCriticalSection(pgsTypes::StrengthI,span,gdr,&min,&max);
   resultsFile<<bridgeId<<", "<<pid<<", 50052, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Millimeter)) <<", 15, "<<gdr<<std::endl;

   const pgsConstructabilityArtifact* pConstruct =  gdrArtifact->GetConstructabilityArtifact();
   if ( pConstruct->IsSlabOffsetApplicable() )
   {
      resultsFile<<bridgeId<<", "<<pid<<", 122005, "<<loc<<", "<<(int)(pConstruct->Passed?1:0)<<", 15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122014, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pConstruct->GetRequiredSlabOffset(), unitMeasure::Millimeter)) <<", 2, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122015, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pConstruct->GetProvidedSlabOffset(), unitMeasure::Millimeter)) <<", 2, "<<gdr<<std::endl;
   }

   const pgsDeflectionCheckArtifact* pDefl = gdrArtifact->GetDeflectionCheckArtifact();
   pDefl->GetDemand(&min,&max);
   resultsFile<<bridgeId<<", "<<pid<<", 122006, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Millimeter)) <<", 2, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 122007, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pDefl->GetCapacity(), unitMeasure::Millimeter)) <<", 2, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 122008, "<<loc<<", "<<(int)(pDefl->Passed()?1:0)<<", 15, "<<gdr<<std::endl;

   const pgsSplittingZoneArtifact* pBurst = gdrArtifact->GetSplittingZoneArtifact();
   if (pBurst->GetIsApplicable())
   {
      resultsFile<<bridgeId<<", "<<pid<<", 122010, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pBurst->GetSplittingZoneLength(), unitMeasure::Millimeter)) <<", 2, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122011, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pBurst->GetSplittingForce(), unitMeasure::Newton)) <<", 2, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122012, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pBurst->GetSplittingResistance(), unitMeasure::Newton)) <<", 2, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 122013, "<<loc<<", "<<(int)(pBurst->Passed()?1:0)<<", 15, "<<gdr<<std::endl;
   }

   const pgsHoldDownForceArtifact* pHoldDownForce = gdrArtifact->GetHoldDownForceArtifact();
   resultsFile<<bridgeId<<", "<<pid<<", 122029, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pHoldDownForce->GetDemand(), unitMeasure::Newton)) <<", 2, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 122030, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pHoldDownForce->GetCapacity(), unitMeasure::Newton)) <<", 2, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 122031, "<<loc<<", "<<(int)(pHoldDownForce->Passed()?1:0)<<", 15, "<<gdr<<std::endl;

   const pgsPrecastIGirderDetailingArtifact* pBeamDetails = gdrArtifact->GetPrecastIGirderDetailingArtifact();
   resultsFile<<bridgeId<<", "<<pid<<", 122032, "<<loc<<", "<<(int)(pBeamDetails->Passed()?1:0)<<", 15, "<<gdr<<std::endl;

   const pgsStrandSlopeArtifact* pSlope = gdrArtifact->GetStrandSlopeArtifact();
   resultsFile<<bridgeId<<", "<<pid<<", 122033, "<<loc<<", "<< QUITE(pSlope->GetDemand()) <<", 2, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 122034, "<<loc<<", "<< QUITE(pSlope->GetCapacity()) <<", 2, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 122035, "<<loc<<", "<<(int)(pSlope->Passed()?1:0)<<", 15, "<<gdr<<std::endl;

   const pgsStrandStressArtifact* sStrand = gdrArtifact->GetStrandStressArtifact();
   resultsFile<<bridgeId<<", "<<pid<<", 122036, "<<loc<<", "<<(int)(sStrand->Passed()?1:0)<<", 15, "<<gdr<<std::endl;

   // next poi-related values
   const pgsStirrupCheckAtPoisArtifact* psArtifact = 0;
   const pgsLongReinfShearArtifact* pArtifact      = 0;
   const pgsStirrupDetailArtifact* pSDArtifact     = 0;
   const pgsHorizontalShearArtifact* pAHsrtifact   = 0;

   PoiAttributeType attrib = POI_TABULAR; //POI_FLEXURESTRESS;// | POI_FLEXURECAPACITY | POI_SHEAR;
   std::vector<pgsPointOfInterest> vPoi;
   vPoi = pIPoi->GetPointsOfInterest(pgsTypes::BridgeSite3, span,gdr,attrib);
   for (std::vector<pgsPointOfInterest>::iterator it=vPoi.begin(); it!=vPoi.end(); it++)
   {
      const pgsPointOfInterest& poi = *it;

      Float64 loc = ::ConvertFromSysUnits(poi.GetDistFromStart(), unitMeasure::Millimeter);

      // write to poi file
      poiFile<<" 1, "<< bridgeId<< ", 3, 1, "<<loc<<", 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0"<<std::endl;

      resultsFile<<bridgeId<<", "<<pid<<", 50006, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetFinal(poi,pgsTypes::Permanent), unitMeasure::MPa)) <<",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50007, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetElasticShortening(poi,pgsTypes::Permanent), unitMeasure::MPa)) <<", 2, 15"<<gdr<<std::endl;

      // The 12-50 test id's for prestress losses are totally inadequate for the current LRFD. New tests are implemented
      // that are more "generic". 550xx are for permanent strands and 551xx are for temporary strands
//      LOSSDETAILS losses = pLosses->GetLossDetails(poi);
//      resultsFile<<bridgeId<<", "<<pid<<", 50008, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(losses.RefinedLosses.ShrinkageLosses(), unitMeasure::MPa)) <<         ",15, "<<gdr<<std::endl;
//      resultsFile<<bridgeId<<", "<<pid<<", 50009, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(losses.RefinedLosses.CreepLosses(), unitMeasure::MPa)) <<              ",15, "<<gdr<<std::endl;
//      resultsFile<<bridgeId<<", "<<pid<<", 50010, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(losses.RefinedLosses.RelaxationLossesAtXfer(), unitMeasure::MPa)) <<   ",15, "<<gdr<<std::endl;
//      resultsFile<<bridgeId<<", "<<pid<<", 50011, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(losses.RefinedLosses.RelaxationLossesAfterXfer(), unitMeasure::MPa)) <<",15, "<<gdr<<std::endl;
//      resultsFile<<bridgeId<<", "<<pid<<", 50012, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(losses.RefinedLosses.ElasticShortening().GetFcgp(), unitMeasure::MPa)) <<                 ",15, "<<gdr<<std::endl;
//      resultsFile<<bridgeId<<", "<<pid<<", 50013, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(losses.RefinedLosses.GetEci(), unitMeasure::MPa)) <<                  ",15, "<<gdr<<std::endl;
//
//      // DUMMY VALUE... Fix Test!!!
//      resultsFile<<bridgeId<<", "<<pid<<", 50014, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(losses.RefinedLosses.GetDeltaFcdp(), unitMeasure::MPa)) <<             ",15, "<<gdr<<std::endl;

// loss in permanent strands
      resultsFile<<bridgeId<<", "<<pid<<", 55001, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetElasticShortening(poi,pgsTypes::Permanent),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55002, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetBeforeXferLosses(poi,pgsTypes::Permanent),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55003, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetAfterXferLosses(poi,pgsTypes::Permanent),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55004, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetLiftingLosses(poi,pgsTypes::Permanent),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55005, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetShippingLosses(poi,pgsTypes::Permanent),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55006, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Permanent),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55007, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetBeforeTemporaryStrandRemovalLosses(poi,pgsTypes::Permanent),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55008, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetAfterTemporaryStrandRemovalLosses(poi,pgsTypes::Permanent),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55009, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetDeckPlacementLosses(poi,pgsTypes::Permanent),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55010, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetFinal(poi,pgsTypes::Permanent),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
// loss in temporary strands
      resultsFile<<bridgeId<<", "<<pid<<", 55101, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetElasticShortening(poi,pgsTypes::Temporary),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55102, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetBeforeXferLosses(poi,pgsTypes::Temporary),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55103, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetAfterXferLosses(poi,pgsTypes::Temporary),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55104, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetLiftingLosses(poi,pgsTypes::Temporary),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55105, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetShippingLosses(poi,pgsTypes::Temporary),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55106, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Temporary),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55107, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetBeforeTemporaryStrandRemovalLosses(poi,pgsTypes::Temporary),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55108, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetAfterTemporaryStrandRemovalLosses(poi,pgsTypes::Temporary),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55109, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetDeckPlacementLosses(poi,pgsTypes::Temporary),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55110, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pLosses->GetFinal(poi,pgsTypes::Temporary),unitMeasure::MPa)) << ",15, "<<gdr<<std::endl;

      // eff flange width
      resultsFile<<bridgeId<<", "<<pid<<", 50001, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pSp2->GetEffectiveFlangeWidth(poi), unitMeasure::Millimeter)) <<", 2, "<<gdr<<std::endl;

      if ( poi.IsFlexureStress() )
      {
         // force and stress in prestressing strands
         resultsFile<<bridgeId<<", "<<pid<<", 50002, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::AfterXfer), unitMeasure::MPa)) <<         ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50003, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pPrestressForce->GetStrandForce(poi,pgsTypes::Permanent,pgsTypes::AfterXfer), unitMeasure::Newton)) <<       ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50004, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::AfterLosses), unitMeasure::MPa)) <<  ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50005, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pPrestressForce->GetStrandForce(poi,pgsTypes::Permanent,pgsTypes::AfterLosses), unitMeasure::Newton)) <<",15, "<<gdr<<std::endl;

         // stresses due to external loads
         // casting yards
         pLsForces->GetStress(pgsTypes::ServiceI, pgsTypes::CastingYard,poi,pgsTypes::TopGirder, true, SimpleSpan, &min,&max);
         resultsFile<<bridgeId<<", "<<pid<<", 50018, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<",15, "<<gdr<<std::endl;
         pLsForces->GetStress(pgsTypes::ServiceI, pgsTypes::CastingYard,poi,pgsTypes::BottomGirder,true, SimpleSpan,&min,&max);
         resultsFile<<bridgeId<<", "<<pid<<", 50019, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<",15, "<<gdr<<std::endl;

         // bridge site 2
         pLsForces->GetStress(pgsTypes::ServiceI, pgsTypes::BridgeSite2,poi,pgsTypes::TopGirder, true, SimpleSpan,&min,&max);
         resultsFile<<bridgeId<<", "<<pid<<", 50020, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<",15, "<<gdr<<std::endl;

         // bridge site 3
         pLsForces->GetStress(pgsTypes::ServiceI, pgsTypes::BridgeSite3,poi,pgsTypes::TopGirder,true, SimpleSpan,&min,&max);
         resultsFile<<bridgeId<<", "<<pid<<", 50021, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<",15, "<<gdr<<std::endl;
         pLsForces->GetStress(pgsTypes::ServiceI, pgsTypes::BridgeSite3,poi,pgsTypes::BottomGirder,true, SimpleSpan,&min,&max);
         resultsFile<<bridgeId<<", "<<pid<<", 50022, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(max , unitMeasure::MPa)) <<",15, "<<gdr<<std::endl;
         pLsForces->GetStress(pgsTypes::ServiceI, pgsTypes::BridgeSite3,poi,pgsTypes::TopSlab,true, SimpleSpan,&min,&max);
         resultsFile<<bridgeId<<", "<<pid<<", 50023, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<",15, "<<gdr<<std::endl;

         pLsForces->GetStress(pgsTypes::ServiceIII, pgsTypes::BridgeSite3,poi,pgsTypes::TopGirder,true, SimpleSpan,&min,&max);
         resultsFile<<bridgeId<<", "<<pid<<", 50024, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<",15, "<<gdr<<std::endl;
         pLsForces->GetStress(pgsTypes::ServiceIII, pgsTypes::BridgeSite3,poi,pgsTypes::BottomGirder,true, SimpleSpan,&min,&max);
         resultsFile<<bridgeId<<", "<<pid<<", 50025, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(max , unitMeasure::MPa)) <<",15, "<<gdr<<std::endl;
         pLsForces->GetStress(pgsTypes::ServiceIII, pgsTypes::BridgeSite3,poi,pgsTypes::TopSlab,true, SimpleSpan,&min,&max);
         resultsFile<<bridgeId<<", "<<pid<<", 50026, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<",15, "<<gdr<<std::endl;

         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
         {
            pLsForces->GetStress(pgsTypes::ServiceIA, pgsTypes::BridgeSite3,poi,pgsTypes::TopSlab,true, SimpleSpan,&min,&max);
            resultsFile<<bridgeId<<", "<<pid<<", 50027, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<",15, "<<gdr<<std::endl;
         }
         else
         {
            pLsForces->GetStress(pgsTypes::FatigueI, pgsTypes::BridgeSite3,poi,pgsTypes::TopSlab,true, SimpleSpan,&min,&max);
            resultsFile<<bridgeId<<", "<<pid<<", 50027, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<",15, "<<gdr<<std::endl;
         }

         // stresses due to prestress
         resultsFile<<bridgeId<<", "<<pid<<", 56018, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pPrestressStresses->GetStress(pgsTypes::CastingYard,poi,pgsTypes::TopGirder), unitMeasure::MPa)) <<         ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 56019, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pPrestressStresses->GetStress(pgsTypes::CastingYard,poi,pgsTypes::BottomGirder), unitMeasure::MPa)) <<         ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 56020, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pPrestressStresses->GetStress(pgsTypes::BridgeSite1,poi,pgsTypes::TopGirder), unitMeasure::MPa)) <<         ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 56021, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pPrestressStresses->GetStress(pgsTypes::BridgeSite1,poi,pgsTypes::BottomGirder), unitMeasure::MPa)) <<         ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 56022, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pPrestressStresses->GetStress(pgsTypes::BridgeSite2,poi,pgsTypes::TopGirder), unitMeasure::MPa)) <<         ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 56023, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pPrestressStresses->GetStress(pgsTypes::BridgeSite2,poi,pgsTypes::BottomGirder), unitMeasure::MPa)) <<         ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 56024, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pPrestressStresses->GetStress(pgsTypes::BridgeSite3,poi,pgsTypes::TopGirder), unitMeasure::MPa)) <<         ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 56025, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pPrestressStresses->GetStress(pgsTypes::BridgeSite3,poi,pgsTypes::BottomGirder), unitMeasure::MPa)) <<         ",15, "<<gdr<<std::endl;
      }

      if ( poi.IsFlexureCapacity() || poi.IsShear() )
      {
         // positive moment capacity
         CRACKINGMOMENTDETAILS cmd;
         pMomentCapacity->GetCrackingMomentDetails(pgsTypes::BridgeSite3,poi,true,&cmd);
         resultsFile<<bridgeId<<", "<<pid<<", 50028, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(cmd.Mcr , unitMeasure::NewtonMillimeter)) <<",15, "<<gdr<<std::endl;

         MINMOMENTCAPDETAILS mmcd;
         pMomentCapacity->GetMinMomentCapacityDetails(pgsTypes::BridgeSite3,poi,true,&mmcd);
         resultsFile<<bridgeId<<", "<<pid<<", 50029, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(mmcd.Mr , unitMeasure::NewtonMillimeter)) <<",15, "<<gdr<<std::endl;

         MOMENTCAPACITYDETAILS mcd;
         pMomentCapacity->GetMomentCapacityDetails(pgsTypes::BridgeSite3,poi,true,&mcd);
         resultsFile<<bridgeId<<", "<<pid<<", 50030, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(mcd.Mn , unitMeasure::NewtonMillimeter)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50035, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(mcd.c, unitMeasure::Millimeter)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50036, "<<loc<<", "<< QUITE(IsZero(mcd.de) ? 0 : mcd.c/mcd.de) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50039, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(mcd.de, unitMeasure::Millimeter)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50040, "<<loc<<", "<<QUITE(IsZero(mmcd.Mcr) ? 0 : mmcd.Mr/mmcd.Mcr)<<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50041, "<<loc<<", "<<QUITE(IsZero(mmcd.Mu) ? 0 : mmcd.Mr/mmcd.Mu)<< ",15, "<<gdr<<std::endl;

         const pgsFlexuralCapacityArtifact* pCompositeCap;
         pCompositeCap = gdrArtifact->GetPositiveMomentFlexuralCapacityArtifact(pgsFlexuralCapacityArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,poi.GetDistFromStart()));
         resultsFile<<bridgeId<<", "<<pid<<", 122016, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pCompositeCap->GetDemand() , unitMeasure::NewtonMillimeter)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 122017, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pCompositeCap->GetCapacity() , unitMeasure::NewtonMillimeter)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 122018, "<<loc<<", "<<(int)(pCompositeCap->Passed()?1:0)<<", 15, "<<gdr<<std::endl;

         // negative moment capacity
         pMomentCapacity->GetCrackingMomentDetails(pgsTypes::BridgeSite3,poi,false,&cmd);
         resultsFile<<bridgeId<<", "<<pid<<", 50128, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(cmd.Mcr , unitMeasure::NewtonMillimeter)) <<",15, "<<gdr<<std::endl;

         pMomentCapacity->GetMinMomentCapacityDetails(pgsTypes::BridgeSite3,poi,false,&mmcd);
         resultsFile<<bridgeId<<", "<<pid<<", 50129, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(mmcd.Mr , unitMeasure::NewtonMillimeter)) <<",15, "<<gdr<<std::endl;

         pMomentCapacity->GetMomentCapacityDetails(pgsTypes::BridgeSite3,poi,false,&mcd);
         resultsFile<<bridgeId<<", "<<pid<<", 50130, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(mcd.Mn , unitMeasure::NewtonMillimeter)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50135, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(mcd.c, unitMeasure::Millimeter)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50136, "<<loc<<", "<< QUITE(IsZero(mcd.de) ? 0 : mcd.c/mcd.de) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50139, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(mcd.de, unitMeasure::Millimeter)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50140, "<<loc<<", "<<QUITE(IsZero(mmcd.Mcr) ? 0 : mmcd.Mr/mmcd.Mcr)<<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50141, "<<loc<<", "<<QUITE(IsZero(mmcd.Mu) ? 0 : mmcd.Mr/mmcd.Mu)<< ",15, "<<gdr<<std::endl;

         pCompositeCap = gdrArtifact->GetNegativeMomentFlexuralCapacityArtifact(pgsFlexuralCapacityArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,poi.GetDistFromStart()));
         resultsFile<<bridgeId<<", "<<pid<<", 122116, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pCompositeCap->GetDemand() , unitMeasure::NewtonMillimeter)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 122117, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pCompositeCap->GetCapacity() , unitMeasure::NewtonMillimeter)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 122118, "<<loc<<", "<<(int)(pCompositeCap->Passed()?1:0)<<", 15, "<<gdr<<std::endl;


         // shear capacity
         SHEARCAPACITYDETAILS scd;
         pShearCapacity->GetShearCapacityDetails(pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi,&scd);
         resultsFile<<bridgeId<<", "<<pid<<", 50031, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(scd.Aps , unitMeasure::Millimeter2)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50032, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(scd.As , unitMeasure::Millimeter2)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50033, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::BeforeTemporaryStrandRemoval), unitMeasure::MPa)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50038, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(scd.As,unitMeasure::Millimeter2)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50042, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(scd.pVn , unitMeasure::Newton)) <<    ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50043, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(scd.Vn , unitMeasure::Newton)) <<     ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50044, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(scd.Vc , unitMeasure::Newton)) <<     ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50045, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(scd.Vs , unitMeasure::Newton)) <<     ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50046, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(scd.Vp , unitMeasure::Newton)) <<     ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50047, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(scd.bv, unitMeasure::Millimeter)) <<  ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50048, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(scd.Aps, unitMeasure::Millimeter2)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50049, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(scd.As, unitMeasure::Millimeter2)) << ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50050, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(scd.Ac, unitMeasure::Millimeter2)) << ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50051, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(scd.dv, unitMeasure::Millimeter)) <<  ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50053, "<<loc<<", "<< QUITE(scd.Beta) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50054, "<<loc<<", "<< QUITE(scd.ex) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50055, "<<loc<<", "<< QUITE(scd.Fe) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50056, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(scd.Theta, unitMeasure::Degree)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50057, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(scd.fpo, unitMeasure::MPa)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50058, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(scd.fpe, unitMeasure::MPa)) <<",15, "<<gdr<<std::endl;

         psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(pgsTypes::BridgeSite3, pgsTypes::StrengthI,poi.GetDistFromStart()) );
         ATLASSERT(psArtifact != NULL);
         pArtifact = psArtifact->GetLongReinfShearArtifact();

         resultsFile<<bridgeId<<", "<<pid<<", 50061, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pArtifact->GetDemandForce(), unitMeasure::Newton)) <<  ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50062, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pArtifact->GetCapacityForce(), unitMeasure::Newton)) <<",15, "<<gdr<<std::endl;

         pSDArtifact = psArtifact->GetStirrupDetailArtifact();
         resultsFile<<bridgeId<<", "<<pid<<", 50063, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pSDArtifact->GetSMax(), unitMeasure::Millimeter)) <<   ",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50064, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pSDArtifact->GetAvsMin(), unitMeasure::Millimeter2)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50065, "<<loc<<", "<< QUITE(scd.Vn/scd.Vu) <<", 15 "<<gdr<<std::endl;

         const pgsStirrupCheckAtPoisArtifact* pspArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(pgsTypes::BridgeSite3, pgsTypes::StrengthI,poi.GetDistFromStart()) );
         pAHsrtifact = pspArtifact->GetHorizontalShearArtifact();
         resultsFile<<bridgeId<<", "<<pid<<", 50067, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pAHsrtifact->GetCapacity()*pAHsrtifact->GetPhi(), unitMeasure::Newton)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50068, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pAHsrtifact->GetCapacity(), unitMeasure::Newton)) <<",15, "<<gdr<<std::endl;

         const pgsVerticalShearArtifact* pVertical = psArtifact->GetVerticalShearArtifact();


         resultsFile<<bridgeId<<", "<<pid<<", 50069, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pAHsrtifact->GetAcv(), unitMeasure::Millimeter2)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50070, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pAHsrtifact->GetAvOverS(), unitMeasure::Millimeter2)) <<",15, "<<gdr<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 50071, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pAHsrtifact->GetNormalCompressionForce(), unitMeasure::Newton)) <<",15, "<<gdr<<std::endl;
      }

      // pass/fail cases
/*      if (pArtifact->IsApplicable())*/
         resultsFile<<bridgeId<<", "<<pid<<", 100202, "<<loc<<", "<<(int)(pArtifact->Passed()?1:0)<<", 15, "<<gdr<<std::endl;

      resultsFile<<bridgeId<<", "<<pid<<", 100203, "<<loc<<", "<<(int)(pSDArtifact->Passed()?1:0)<<", 15, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 100204, "<<loc<<", "<<(int)(pAHsrtifact->Passed()?1:0)<<", 15, "<<gdr<<std::endl;

      if ( poi.IsFlexureStress() && poi.HasStage(pgsTypes::BridgeSite1) && poi.HasStage(pgsTypes::BridgeSite2) && poi.HasStage(pgsTypes::BridgeSite3) )
      {
         const pgsFlexuralStressArtifact* pStresses;
         pStresses = gdrArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite1,pgsTypes::ServiceI,pgsTypes::Tension,poi.GetDistFromStart()));
         resultsFile<<bridgeId<<", "<<pid<<", 122019, "<<loc<<", "<<(int)(pStresses->Passed(pgsFlexuralStressArtifact::WithoutRebar)?1:0)<<", 15, "<<gdr<<std::endl;
         pStresses = gdrArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension,poi.GetDistFromStart()));
         resultsFile<<bridgeId<<", "<<pid<<", 122023, "<<loc<<", "<<(int)(pStresses->TopPassed(pgsFlexuralStressArtifact::WithoutRebar)?1:0)<<", 15, "<<gdr<<std::endl;
         pStresses = gdrArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite1,pgsTypes::ServiceI,pgsTypes::Compression,poi.GetDistFromStart()));
         resultsFile<<bridgeId<<", "<<pid<<", 122024, "<<loc<<", "<<(int)(pStresses->Passed(pgsFlexuralStressArtifact::WithoutRebar)?1:0)<<", 15, "<<gdr<<std::endl;
         pStresses = gdrArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite2,pgsTypes::ServiceI,pgsTypes::Compression,poi.GetDistFromStart()));
         resultsFile<<bridgeId<<", "<<pid<<", 122025, "<<loc<<", "<<(int)(pStresses->Passed(pgsFlexuralStressArtifact::WithoutRebar)?1:0)<<", 15, "<<gdr<<std::endl;
         pStresses = gdrArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite3,pgsTypes::ServiceI,pgsTypes::Compression,poi.GetDistFromStart()));
         resultsFile<<bridgeId<<", "<<pid<<", 122026, "<<loc<<", "<<(int)(pStresses->Passed(pgsFlexuralStressArtifact::WithoutRebar)?1:0)<<", 15, "<<gdr<<std::endl;
         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
         {
            pStresses = gdrArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite3,pgsTypes::ServiceIA,pgsTypes::Compression,poi.GetDistFromStart()));
            resultsFile<<bridgeId<<", "<<pid<<", 122027, "<<loc<<", "<<(int)(pStresses->Passed(pgsFlexuralStressArtifact::WithoutRebar)?1:0)<<", 15, "<<gdr<<std::endl;
         }
         else
         {
            pStresses = gdrArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite3,pgsTypes::FatigueI,pgsTypes::Compression,poi.GetDistFromStart()));
            resultsFile<<bridgeId<<", "<<pid<<", 122027, "<<loc<<", "<<(int)(pStresses->Passed(pgsFlexuralStressArtifact::WithoutRebar)?1:0)<<", 15, "<<gdr<<std::endl;
         }
      }
   }

   // stirrup check at zones
   GET_IFACE( IStirrupGeometry, pStirrupGeometry );
   Uint32 nZones = pStirrupGeometry->GetNumZones(span,gdr);
   for (Uint32 zoneIdx = 0; zoneIdx < nZones; zoneIdx++)
   {
      const pgsStirrupCheckAtZonesArtifact* psArtifact = pstirrup_artifact->GetStirrupCheckAtZonesArtifact( zoneIdx );
      const pgsConfinementArtifact* pconf = psArtifact->GetConfinementArtifact();
      resultsFile<<bridgeId<<", "<<pid<<", 122004, "<<zoneIdx<<", "<<(int)(pconf->Passed()?1:0)<<", 15, "<<gdr<<std::endl;
   }

   return true;
}

bool CTestAgentImp::RunHandlingTest(std::ofstream& resultsFile, std::ofstream& poiFile, SpanIndexType span)
{
   std::string pid      = GetProcessID();
   std::string bridgeId = GetBridgeID();

   // write to poi file
   poiFile<<" 1, "<< bridgeId<< ", 3, 1, 0.0000, 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0"<<std::endl;

   GirderIndexType gdr  = 0;

   // lifting
   GET_IFACE(IArtifact,pArtifacts);
   const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,gdr);
   const pgsLiftingCheckArtifact* pLiftArtifact = pArtifact->GetLiftingCheckArtifact();
   if (pLiftArtifact != NULL)
   {
      if (!pLiftArtifact->IsGirderStable())
      {
         resultsFile<<"Girder is unstable for lifting"<<std::endl;
         return false;
      }

      GET_IFACE(IGirderLiftingPointsOfInterest,pGirderLiftingPointsOfInterest);
      std::vector<pgsPointOfInterest> poi_vec;
      poi_vec = pGirderLiftingPointsOfInterest->GetLiftingPointsOfInterest(span,gdr,POI_MIDSPAN);
      CHECK(poi_vec.size()==1);
      pgsPointOfInterest& poi = poi_vec[0];

      Float64 loc = poi.GetDistFromStart();

      pgsLiftingStressCheckArtifact stressArtifact = pLiftArtifact->GetLiftingStressCheckArtifact(poi.GetDistFromStart());

      resultsFile<<bridgeId<<", "<<pid<<", 100001, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(stressArtifact.GetMaximumConcreteTensileStress() , unitMeasure::MPa)) <<", 50, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 100002, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(stressArtifact.GetMaximumConcreteCompressiveStress() , unitMeasure::MPa)) <<", 50, "<<gdr<<std::endl;

      pgsLiftingCrackingCheckArtifact crackArtifact =  pLiftArtifact->GetLiftingCrackingCheckArtifact(poi.GetDistFromStart());

      resultsFile<<bridgeId<<", "<<pid<<", 100003, "<<loc<<", "<<crackArtifact.GetFsCracking()<<", 50, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 100004, "<<loc<<", "<<pLiftArtifact->GetFsFailure()<<", 50, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 100009, "<<loc<<", "<<(int)(pLiftArtifact->Passed()?1:0)<<", 50, "<<gdr<<std::endl;
   }


   // hauling
   const pgsHaulingCheckArtifact* pHaulArtifact = pArtifact->GetHaulingCheckArtifact();
   if (pHaulArtifact != NULL)
   {
      GET_IFACE(IGirderHaulingPointsOfInterest,pGirderHaulingPointsOfInterest);
      std::vector<pgsPointOfInterest> poi_vec;
      poi_vec = pGirderHaulingPointsOfInterest->GetHaulingPointsOfInterest(span,gdr,POI_MIDSPAN);
      CHECK(poi_vec.size()==1);
      pgsPointOfInterest& poi = poi_vec[0];
      Float64 loc = poi.GetDistFromStart();

      pgsHaulingStressCheckArtifact hStress = pHaulArtifact->GetHaulingStressCheckArtifact(poi.GetDistFromStart());

      resultsFile<<bridgeId<<", "<<pid<<", 100005, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(hStress.GetMaximumConcreteTensileStress() , unitMeasure::MPa)) <<", 50, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 100006, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(hStress.GetMaximumConcreteCompressiveStress() , unitMeasure::MPa)) <<", 50, "<<gdr<<std::endl;

      pgsHaulingCrackingCheckArtifact hCrack =  pHaulArtifact->GetHaulingCrackingCheckArtifact(poi.GetDistFromStart());

      resultsFile<<bridgeId<<", "<<pid<<", 100007, "<<loc<<", "<< hCrack.GetFsCracking()<<", 50, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 100008, "<<loc<<", "<< pHaulArtifact->GetFsRollover()<<", 50, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 100010, "<<loc<<", "<<(int)(pHaulArtifact->Passed()?1:0)<<", 50, "<<gdr<<std::endl;
   }

   return true;
}

bool CTestAgentImp::RunWsdotGirderScheduleTest(std::ofstream& resultsFile, std::ofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   std::string pid      = GetProcessID();
   std::string bridgeId = GetBridgeID();

   GET_IFACE(IArtifact,pIArtifact);
   const pgsGirderArtifact* pArtifact = pIArtifact->GetArtifact(span,gdr);

   GET_IFACE(IArtifact,pArtifacts);
   const pgsGirderArtifact* pGdrArtifact = pArtifacts->GetArtifact(span,gdr);
   const pgsConstructabilityArtifact* pConstArtifact = pGdrArtifact->GetConstructabilityArtifact();

   GET_IFACE(ICamber,pCamber);
   // create pois at the start of girder and mid-span
   pgsPointOfInterest pois(span,gdr,0.0);

   GET_IFACE(IPointOfInterest, pPointOfInterest );
   std::vector<pgsPointOfInterest> pmid = pPointOfInterest->GetPointsOfInterest(pgsTypes::BridgeSite1, span, gdr, POI_MIDSPAN);
   CHECK(pmid.size()==1);

   Float64 loc = pmid[0].GetDistFromStart();

   GET_IFACE(IBridge, pBridge );
   CComPtr<IAngle> as1;
   pBridge->GetPierSkew(span,&as1);
   Float64 s1;
   as1->get_Value(&s1);
   Float64 t1 = s1 + M_PI/2.0;
   resultsFile<<bridgeId<<", "<<pid<<", 123001, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(t1 , unitMeasure::Degree)) <<", 101, "<<gdr<<std::endl;

   CComPtr<IAngle> as2;
   pBridge->GetPierSkew(span+1,&as2);
   Float64 s2;
   as2->get_Value(&s2);
   Float64 t2 = s2 + M_PI/2.0;

   resultsFile<<bridgeId<<", "<<pid<<", 123002, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(t2 , unitMeasure::Degree)) <<", 101, "<<gdr<<std::endl;

   GET_IFACE(ILibrary, pLib );

   std::string start_connection = pBridge->GetRightSidePierConnection(span);
   std::string end_connection = pBridge->GetLeftSidePierConnection(span+1);
   const ConnectionLibraryEntry* pConnEntry = pLib->GetConnectionEntry( start_connection.c_str() );
   Float64 N1 = pConnEntry->GetGirderEndDistance();

   resultsFile<<bridgeId<<", "<<pid<<", 123003, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(N1, unitMeasure::Millimeter)) <<   ", 101, "<<gdr<<std::endl;

   pConnEntry = pLib->GetConnectionEntry( end_connection.c_str() );
   Float64 N2 = pConnEntry->GetGirderEndDistance();

   resultsFile<<bridgeId<<", "<<pid<<", 123004, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(N2, unitMeasure::Millimeter)) <<   ", 101, "<<gdr<<std::endl;

   Float64 P1 = N1 / sin(t1);
   resultsFile<<bridgeId<<", "<<pid<<", 123005, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(P1, unitMeasure::Millimeter)) <<   ", 101, "<<gdr<<std::endl;

   Float64 P2 = N2 / sin(t2);
   resultsFile<<bridgeId<<", "<<pid<<", 123006, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(P2, unitMeasure::Millimeter)) <<   ", 101, "<<gdr<<std::endl;

   resultsFile<<bridgeId<<", "<<pid<<", 123007, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pBridge->GetGirderLength(span,gdr), unitMeasure::Millimeter)) <<   ", 101, "<<gdr<<std::endl;

   GET_IFACE(IBridgeMaterial, pMaterial);
   resultsFile<<bridgeId<<", "<<pid<<", 123008, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pMaterial->GetFcGdr(span,gdr), unitMeasure::MPa)) <<   ", 101, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 123009, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pMaterial->GetFciGdr(span,gdr), unitMeasure::MPa)) <<   ", 101, "<<gdr<<std::endl;

   GET_IFACE(IStrandGeometry, pStrandGeometry );
   
   StrandIndexType nh = pStrandGeometry->GetNumStrands(span,gdr,pgsTypes::Harped);
   resultsFile<<bridgeId<<", "<<pid<<", 123010, "<<loc<<", "<<nh<<   ", 101, "<<gdr<<std::endl;

   Float64 hj = pStrandGeometry->GetPjack(span,gdr,pgsTypes::Harped);
   resultsFile<<bridgeId<<", "<<pid<<", 123011, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(hj, unitMeasure::Newton)) <<   ", 101, "<<gdr<<std::endl;

   StrandIndexType ns = pStrandGeometry->GetNumStrands(span,gdr,pgsTypes::Straight);
   resultsFile<<bridgeId<<", "<<pid<<", 123012, "<<loc<<", "<<ns<<   ", 101, "<<gdr<<std::endl;

   Float64 sj = pStrandGeometry->GetPjack(span,gdr,pgsTypes::Straight);
   resultsFile<<bridgeId<<", "<<pid<<", 123013, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(sj, unitMeasure::Newton)) <<   ", 101, "<<gdr<<std::endl;

   StrandIndexType nt = pStrandGeometry->GetNumStrands(span,gdr,pgsTypes::Temporary);
   resultsFile<<bridgeId<<", "<<pid<<", 123014, "<<loc<<", "<<nt<<   ", 101, "<<gdr<<std::endl;

   Float64 tj = pStrandGeometry->GetPjack(span,gdr,pgsTypes::Temporary);
   resultsFile<<bridgeId<<", "<<pid<<", 123015, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(tj, unitMeasure::Newton)) <<   ", 101, "<<gdr<<std::endl;

   GET_IFACE(ISectProp2, pSectProp2 );
   Float64 ybg = pSectProp2->GetYb(pgsTypes::CastingYard,pois);
   Float64 nEff;
   Float64 sse = pStrandGeometry->GetSsEccentricity(pois, &nEff);
   if (0 < ns)
      resultsFile<<bridgeId<<", "<<pid<<", 123016, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(ybg-sse, unitMeasure::Millimeter)) <<   ", 101, "<<gdr<<std::endl;

   Float64 hse = pStrandGeometry->GetHsEccentricity(pmid[0], &nEff);
   if (0 < nh)
      resultsFile<<bridgeId<<", "<<pid<<", 123017, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(ybg-hse, unitMeasure::Millimeter)) <<   ", 101, "<<gdr<<std::endl;

   // get location of first harped strand
   if (0 < nh)
   {
      StrandIndexType nns = pStrandGeometry->GetNextNumStrands(span,gdr,pgsTypes::Harped,0);
      GDRCONFIG config = pBridge->GetGirderConfiguration(span,gdr);
      config.Nstrands[pgsTypes::Harped] = nns;
      Float64 eh2 = pStrandGeometry->GetHsEccentricity( pmid[0], config, &nEff );
      Float64 Fb  = pSectProp2->GetYb(pgsTypes::CastingYard,pois) - eh2;
      resultsFile<<bridgeId<<", "<<pid<<", 123018, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(Fb, unitMeasure::Millimeter)) <<   ", 101, "<<gdr<<std::endl;
   }

   Float64 ytg = pSectProp2->GetYtGirder(pgsTypes::CastingYard,pois);
   Float64 hss = pStrandGeometry->GetHsEccentricity(pois, &nEff);
   if (0 < nh)
      resultsFile<<bridgeId<<", "<<pid<<", 123019, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(ytg+hss, unitMeasure::Millimeter)) <<   ", 101, "<<gdr<<std::endl;

   resultsFile<<bridgeId<<", "<<pid<<", 123020, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pCamber->GetScreedCamber( pmid[0] ), unitMeasure::Millimeter)) <<   ", 101, "<<gdr<<std::endl;

   // get # of days for creep
   GET_IFACE(ISpecification, pSpec );
   std::string spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   Float64 days =  ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration1Min(), unitMeasure::Day);
   resultsFile<<bridgeId<<", "<<pid<<", 123021, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pSpecEntry->GetCreepDuration1Min(), unitMeasure::Day)) <<   ", 101, "<<gdr<<std::endl;



   return true;
}


bool CTestAgentImp::RunDesignTest(std::ofstream& resultsFile, std::ofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   std::string pid      = GetProcessID();
   std::string bridgeId = GetBridgeID();

   Float64 loc = 0.0;

   // get design options from spec. Do shear and flexure
   GET_IFACE(ISpecification,pSpecification);
   arDesignOptions des_options = pSpecification->GetDesignOptions(span,gdr);

   des_options.doDesignForShear = true;

   GET_IFACE(IArtifact,pIArtifact);
   const pgsDesignArtifact* pArtifact;

   try
   {
      pArtifact = pIArtifact->CreateDesignArtifact( span,gdr, des_options);
   }
   catch(...)
   {
      resultsFile << "Design Failed for span " << span << " girder " << gdr << std::endl;
      return false;
   }

   if ( pArtifact->GetOutcome() != pgsDesignArtifact::Success )
   {
      resultsFile << "Design not successful for span " << span << " girder " << gdr << std::endl;
      return false;
   }

   resultsFile<<bridgeId<<", "<<pid<<", 124001, "<<loc<<", "<<pArtifact->GetOutcome()<<   ", 102, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124002, "<<loc<<", "<<pArtifact->GetNumStraightStrands()<<   ", 102, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124003, "<<loc<<", "<<pArtifact->GetNumTempStrands()<<   ", 102, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124004, "<<loc<<", "<<pArtifact->GetNumHarpedStrands()<<   ", 102, "<<gdr<<std::endl;

   resultsFile<<bridgeId<<", "<<pid<<", 124005, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pArtifact->GetPjackStraightStrands(), unitMeasure::Newton)) <<   ", 102, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124006, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pArtifact->GetPjackTempStrands(), unitMeasure::Newton)) <<   ", 102, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124007, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pArtifact->GetPjackHarpedStrands(), unitMeasure::Newton)) <<   ", 102, "<<gdr<<std::endl;

   resultsFile<<bridgeId<<", "<<pid<<", 124008, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pArtifact->GetHarpStrandOffsetHp(), unitMeasure::Millimeter)) <<   ", 102, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124009, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pArtifact->GetHarpStrandOffsetEnd(), unitMeasure::Millimeter)) <<   ", 102, "<<gdr<<std::endl;

   resultsFile<<bridgeId<<", "<<pid<<", 124010, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pArtifact->GetReleaseStrength(), unitMeasure::MPa)) <<   ", 102, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124011, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pArtifact->GetConcrete().GetFc(), unitMeasure::MPa)) <<   ", 102, "<<gdr<<std::endl;

   resultsFile<<bridgeId<<", "<<pid<<", 124012s, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pArtifact->GetSlabOffset(pgsTypes::metStart), unitMeasure::Millimeter)) <<   ", 102, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124012e, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pArtifact->GetSlabOffset(pgsTypes::metEnd), unitMeasure::Millimeter)) <<   ", 102, "<<gdr<<std::endl;

#pragma Reminder("UPDATE: Designing for symmetrical lift and haul points")
   resultsFile<<bridgeId<<", "<<pid<<", 124013, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pArtifact->GetLeftLiftingLocation(), unitMeasure::Millimeter)) <<   ", 102, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124014, "<<loc<<", "<< QUITE(::ConvertFromSysUnits(pArtifact->GetLeadingOverhang(), unitMeasure::Millimeter)) <<   ", 102, "<<gdr<<std::endl;

   resultsFile<<bridgeId<<", "<<pid<<", 124015, "<<loc<<", "<<pArtifact->GetConfinementBarSize()<<   ", 102, "<<gdr<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124016, "<<loc<<", "<<pArtifact->GetLastConfinementZone()<<   ", 102, "<<gdr<<std::endl;

   resultsFile<<bridgeId<<", "<<pid<<", 124017, "<<loc<<", "<<pArtifact->GetNumberOfStirrupZonesDesigned()<<   ", 102, "<<gdr<<std::endl;

   Int32 id = 124018;
   ZoneIndexType nZones = min(4, pArtifact->GetNumberOfStirrupZonesDesigned());
   for (ZoneIndexType zoneIdx = 0; zoneIdx < nZones; zoneIdx++)
   {
      CShearZoneData zd = pArtifact->GetShearZoneData(zoneIdx);

      resultsFile<<bridgeId<<", "<<pid<<", "<<id++<<", "<<loc<<", "<<zd.VertBarSize<<   ", 102, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", "<<id++<<", "<<loc<<", "<< QUITE(::ConvertFromSysUnits(zd.BarSpacing, unitMeasure::Millimeter)) <<   ", 102, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", "<<id++<<", "<<loc<<", "<< QUITE(::ConvertFromSysUnits(zd.ZoneLength, unitMeasure::Millimeter)) <<   ", 102, "<<gdr<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", "<<id++<<", "<<loc<<", "<< zd.nVertBars <<   ", 102, "<<gdr<<std::endl;
   }

   return true;
}

bool CTestAgentImp::RunCamberTest(std::ofstream& resultsFile, std::ofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   std::string pid      = GetProcessID();
   std::string bridgeId = GetBridgeID();

   GET_IFACE(IPointOfInterest,pPointsOfInterest);
   std::vector<pgsPointOfInterest> poi_vec;
   poi_vec = pPointsOfInterest->GetPointsOfInterest(pgsTypes::BridgeSite3,span,gdr,POI_MIDSPAN);
   CHECK(poi_vec.size()==1);
   pgsPointOfInterest& poi_midspan = poi_vec[0];

   GET_IFACE( ICamber, pCamber );
   double D40  = pCamber->GetDCamberForGirderSchedule(poi_midspan,CREEP_MINTIME);;
   double D120 = pCamber->GetDCamberForGirderSchedule(poi_midspan,CREEP_MINTIME);;

   resultsFile << bridgeId << ", " << pid << ", 125000, " << QUITE(::ConvertFromSysUnits(D40,  unitMeasure::Millimeter)) << ", " << gdr << std::endl;
   resultsFile << bridgeId << ", " << pid << ", 125001, " << QUITE(::ConvertFromSysUnits(D120, unitMeasure::Millimeter)) << ", " << gdr << std::endl;

   return true;
}


bool CTestAgentImp::RunFabOptimizationTest(std::ofstream& resultsFile, std::ofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   std::string pid      = GetProcessID();
   std::string bridgeId = GetBridgeID();

   // No use doing this if lifting or hauling is disabled.
   GET_IFACE(IArtifact,pArtifacts);
   const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,gdr);
   const pgsLiftingCheckArtifact* pLiftArtifact = pArtifact->GetLiftingCheckArtifact();
   const pgsHaulingCheckArtifact* pHaulArtifact = pArtifact->GetHaulingCheckArtifact();

   if (pHaulArtifact != NULL && pLiftArtifact != NULL)
   {
	   GET_IFACE(IFabricationOptimization,pFabOp);

	   FABRICATIONOPTIMIZATIONDETAILS details;
	   pFabOp->GetFabricationOptimizationDetails(span,gdr,&details);

	   resultsFile << bridgeId << ", " << pid << ", 155000, " << ::ConvertFromSysUnits(details.Fci[NO_TTS],unitMeasure::MPa) << ", " << gdr << std::endl;
	   resultsFile << bridgeId << ", " << pid << ", 155001, " << ::ConvertFromSysUnits(details.L[NO_TTS],unitMeasure::Millimeter) << ", " << gdr << std::endl;

	   resultsFile << bridgeId << ", " << pid << ", 155002, " << ::ConvertFromSysUnits(details.Fci[PS_TTS],unitMeasure::MPa) << ", " << gdr << std::endl;
	   resultsFile << bridgeId << ", " << pid << ", 155003, " << ::ConvertFromSysUnits(details.L[PS_TTS],unitMeasure::Millimeter) << ", " << gdr << std::endl;

	   resultsFile << bridgeId << ", " << pid << ", 155004, " << ::ConvertFromSysUnits(details.Fci[PT_TTS_OPTIONAL],unitMeasure::MPa) << ", " << gdr << std::endl;
	   resultsFile << bridgeId << ", " << pid << ", 155005, " << ::ConvertFromSysUnits(details.L[PT_TTS_OPTIONAL],unitMeasure::Millimeter) << ", " << gdr << std::endl;

	   resultsFile << bridgeId << ", " << pid << ", 155006, " << ::ConvertFromSysUnits(details.Fci[PT_TTS_REQUIRED],unitMeasure::MPa) << ", " << gdr << std::endl;
	   resultsFile << bridgeId << ", " << pid << ", 155007, " << ::ConvertFromSysUnits(details.L[PT_TTS_REQUIRED],unitMeasure::Millimeter) << ", " << gdr << std::endl;

	   resultsFile << bridgeId << ", " << pid << ", 155008, " << ::ConvertFromSysUnits(details.Fci_FormStripping_WithoutTTS,unitMeasure::MPa) << ", " << gdr << std::endl;

	   resultsFile << bridgeId << ", " << pid << ", 155009, " << ::ConvertFromSysUnits(details.Lmin,unitMeasure::Millimeter) << ", " << gdr << std::endl;
	   resultsFile << bridgeId << ", " << pid << ", 155010, " << ::ConvertFromSysUnits(details.Lmax,unitMeasure::Millimeter) << ", " << gdr << std::endl;

	   resultsFile << bridgeId << ", " << pid << ", 155011, " << ::ConvertFromSysUnits(details.LUmin,unitMeasure::Millimeter) << ", " << gdr << std::endl;
	   resultsFile << bridgeId << ", " << pid << ", 155012, " << ::ConvertFromSysUnits(details.LUmax,unitMeasure::Millimeter) << ", " << gdr << std::endl;
	   resultsFile << bridgeId << ", " << pid << ", 155013, " << ::ConvertFromSysUnits(details.LUsum,unitMeasure::Millimeter) << ", " << gdr << std::endl;
   }

   return true;
}
