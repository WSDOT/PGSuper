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

// TestAgentImp.cpp : Implementation of CTestAgentImp
#include "stdafx.h"
#include "TestAgent_i.h"
#include "TestAgent.h"
#include "TestAgentImp.h"

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
#include <IFace\RatingSpecification.h>
#include <EAF\EAFUIIntegration.h>

#include <psgLib\ConnectionLibraryEntry.h>
#include <psgLib\SpecLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>

#include <PgsExt\GirderArtifact.h>
#include <PgsExt\DesignArtifact.h>
#include <PgsExt\LiftingCheckArtifact.h>
#include <PgsExt\HaulingCheckArtifact.h>
#include <PgsExt\RatingArtifact.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\PierData.h>

#include <Units\Units.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define QUITE(_x_) (IsZero(_x_,0.00005) ? 0 : _x_)

/////////////////////////////////////////////////////////////////////////////
// CTestAgentImp
int GetBarSize(matRebar::Size size)
{
   switch(size)
   {
   case matRebar::bs3: return 3;
   case matRebar::bs4: return 4;
   case matRebar::bs5: return 5;
   case matRebar::bs6: return 6;
   case matRebar::bs7: return 7;
   case matRebar::bs8: return 8;
   case matRebar::bs9: return 9;
   case matRebar::bs10:return 10;
   case matRebar::bs11:return 11;
   case matRebar::bs14:return 14;
   case matRebar::bs18:return 18;
   }

   return -1;
}

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
                            const std::_tstring& outputFileName,
                            const std::_tstring poiFileName)
{
   // use run unit tests with numeric labeling
   pgsAutoLabel auto_label;

   // turn off diagnostics
   DIAG_WARNPOPUP(FALSE);

   // open poi file and results file
   std::_tofstream resf;
   resf.open(outputFileName.c_str());

   std::_tofstream poif;
   poif.open(poiFileName.c_str());
   if (!resf || !poif)
      return false;

   // create progress window
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

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
      case 501:
         if ( span == 0 )
         {
            if ( RunLoadRatingTest(resf, poif, 0) )
               return RunLoadRatingTest(resf, poif, 1);
            break;
         }
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

         if ( span == 0 )
         {
            VERIFY( RunLoadRatingTest(resf, poif, 0) );
            VERIFY( RunLoadRatingTest(resf, poif, 1) );
         }

         return true;
         break;
      }
   }

   return false;
}

bool CTestAgentImp::RunTestEx(long type, const std::vector<SpanGirderHashType>& girderList,
                            const std::_tstring& outputFileName,
                            const std::_tstring poiFileName)
{
   pgsAutoLabel auto_label;

   // turn off diagnostics
   DIAG_WARNPOPUP(FALSE);

   // open poi file and results file
   std::_tofstream resf;
   resf.open(outputFileName.c_str());

   std::_tofstream poif;
   poif.open(poiFileName.c_str());
   if (!resf || !poif)
      return false;

   // create progress window
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

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
      case 501:
         if ( !RunLoadRatingTest(resf, poif, girder) )
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

         if (!RunLoadRatingTest(resf, poif, girder) )
            return false;
      }
   }

   return true;
}

std::_tstring CTestAgentImp::GetBridgeID()
{
   static std::_tstring strID;
   static bool bDone = false;

   if ( bDone )
      return strID;
   else
   {
      GET_IFACE(IEAFDocument,pDocumnet);
      std::_tstring strPath = pDocumnet->GetFilePath();

      // Filename is in the form Regxxx.pgs
      std::_tstring::size_type pos = strPath.find(_T(".pgs"));
      std::_tstring::size_type len = strPath.length();
      if ( pos == std::_tstring::npos )
         return 0; // "Reg" was not found

      strID = strPath.substr(pos-3,3); // return the "xxx" number

      bDone = true;
      return strID;
   }
}

std::_tstring CTestAgentImp::GetProcessID()
{
   // the process ID is going to be the PGSuper version number
   GET_IFACE(IVersionInfo,pVI);
   std::_tstring strVersion = pVI->GetVersion(true);
   return strVersion;
}

bool CTestAgentImp::RunDistFactorTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE( ILiveLoadDistributionFactors, pDf );

   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   // write to poi file
   poiFile<<_T(" 1, ")<< bridgeId<< _T(", 3, 1, 0.0000, 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0")<<std::endl;

   bool bResult1 = pDf->Run1250Tests(span,gdr,pgsTypes::StrengthI,pid.c_str(),bridgeId.c_str(),resultsFile,poiFile);

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      bResult1 = pDf->Run1250Tests(span,gdr,pgsTypes::FatigueI,pid.c_str(),bridgeId.c_str(),resultsFile,poiFile);
   }

   return bResult1;
}

bool CTestAgentImp::RunHL93Test(std::_tofstream& resultsFile, std::_tofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE( IProductForces, pForce);
   GET_IFACE( ISpecification,     pSpec);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   BridgeAnalysisType bat;

   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   PoiAttributeType attrib = POI_TABULAR | POI_FLEXURESTRESS;
   std::vector<pgsPointOfInterest> vPoi;

   vPoi = pIPoi->GetPointsOfInterest( span,gdr,pgsTypes::BridgeSite3,attrib);

   CollectionIndexType npoi = vPoi.size();
   for (CollectionIndexType i=0; i<npoi; i++)
   {
      pgsPointOfInterest& rpoi = vPoi[i];
      CollectionIndexType locn = i+1;
      Float64 loc = ::ConvertFromSysUnits(rpoi.GetDistFromStart(), unitMeasure::Millimeter);

      // write to poi file
      poiFile<<locn<<_T(", ")<< bridgeId<< _T(", 7, 1, ")<<loc<<_T(", 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0")<<std::endl;

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

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 32090, ")<<loc<<_T(", ")<< QUITE(pm) <<_T(", 7, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 32091, ")<<loc<<_T(", ")<< QUITE(nm) <<_T(", 7, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 32092, ")<<loc<<_T(", ")<< QUITE(ps) <<_T(", 7, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 32093, ")<<loc<<_T(", ")<< QUITE(ns) <<_T(", 7, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 32094, ")<<loc<<_T(", ")<< QUITE(-nd) <<_T(", 7, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 32095, ")<<loc<<_T(", ")<< QUITE(-pd) <<_T(", 7, ")<<gdr<<std::endl;

      // deflection truck with full width stiffness
      pForce->GetDeflLiveLoadDisplacement( IProductForces::DesignTruckAlone, rpoi, &nd, &pd );
      pd = ::ConvertFromSysUnits(pd, unitMeasure::Millimeter);
      nd = ::ConvertFromSysUnits(nd, unitMeasure::Millimeter);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 32200, ")<<loc<<_T(", ")<< QUITE(-nd) <<_T(", 7, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 32201, ")<<loc<<_T(", ")<< QUITE(-pd) <<_T(", 7, ")<<gdr<<std::endl;

   }
   return true;
}

bool CTestAgentImp::RunCrossSectionTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE( ISectProp2, pSp2 );
   GET_IFACE( IGirder, pGdr);

   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   // write to poi file
   poiFile<<_T(" 1, ")<< bridgeId<< _T(", 3, 1, 0.0000, 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0")<<std::endl;

   pgsPointOfInterest poi(span,gdr,0.0);

   // exterior girder
   // bare girder
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25000, 0.0, ")<< QUITE(::ConvertFromSysUnits(pSp2->GetAg(pgsTypes::CastingYard,poi),  unitMeasure::Millimeter2)) <<_T(", 4, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25001, 0.0, ")<< QUITE(::ConvertFromSysUnits(pSp2->GetVolume(span,gdr), unitMeasure::Millimeter3)) <<_T(", 4, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25002, 0.0, ")<< QUITE(::ConvertFromSysUnits(pSp2->GetSurfaceArea(span,gdr), unitMeasure::Millimeter2)) <<_T(", 4, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25004, 0.0, ")<< QUITE(::ConvertFromSysUnits(pSp2->GetIx(pgsTypes::CastingYard,poi), unitMeasure::Millimeter4)) <<_T(", 4, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25005, 0.0, ")<< QUITE(::ConvertFromSysUnits(pSp2->GetYb(pgsTypes::CastingYard,poi), unitMeasure::Millimeter)) << _T(", 4, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25006, 0.0, ")<< QUITE(::ConvertFromSysUnits(pSp2->GetSb(pgsTypes::CastingYard,poi), unitMeasure::Millimeter3)) <<_T(", 4, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25007, 0.0, ")<< QUITE(::ConvertFromSysUnits(pSp2->GetStGirder(pgsTypes::CastingYard,poi), unitMeasure::Millimeter3)) <<_T(", 4, ")<<gdr<<std::endl;

   // composite girder
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25008, 0.0, ")<< QUITE(::ConvertFromSysUnits(pSp2->GetIx(pgsTypes::BridgeSite2,poi), unitMeasure::Millimeter4)) << _T(", 4, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25009, 0.0, ")<< QUITE(::ConvertFromSysUnits(pSp2->GetYb(pgsTypes::BridgeSite2,poi), unitMeasure::Millimeter)) <<  _T(", 4, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25010, 0.0, ")<< QUITE(::ConvertFromSysUnits(pSp2->GetSb(pgsTypes::BridgeSite2,poi), unitMeasure::Millimeter3)) << _T(", 4, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25011, 0.0, ")<< QUITE(::ConvertFromSysUnits(pSp2->GetStGirder(pgsTypes::BridgeSite2,poi), unitMeasure::Millimeter3)) <<_T(", 4, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25012, 0.0, ")<< QUITE(::ConvertFromSysUnits(pSp2->GetSt(pgsTypes::BridgeSite2,poi), unitMeasure::Millimeter3)) << _T(", 4, ")<<gdr<<std::endl;

   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25031, 0.0, ")<< QUITE(::ConvertFromSysUnits(pSp2->GetAcBottomHalf(poi),unitMeasure::Millimeter2)) <<_T(", 4, 1")<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25033, 0.0, ")<< QUITE(::ConvertFromSysUnits(pGdr->GetHeight(poi), unitMeasure::Millimeter)) <<_T(", 4, 1")<<std::endl;

   return true;
}

bool CTestAgentImp::RunDeadLoadActionTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
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

   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   pgsTypes::Stage girderLoadStage = pLoads->GetGirderDeadLoadStage(span, gdr);

   PoiAttributeType attrib = POI_TABULAR | POI_FLEXURESTRESS;
   std::vector<pgsPointOfInterest> vPoi;

   vPoi = pIPoi->GetPointsOfInterest( span,gdr,pgsTypes::BridgeSite3,attrib);

   CollectionIndexType npoi = vPoi.size();
   for (CollectionIndexType i=0; i<npoi; i++)
   {
      pgsPointOfInterest& poi = vPoi[i];
      CollectionIndexType locn = i+1;
      Float64 loc = ::ConvertFromSysUnits(poi.GetDistFromStart(), unitMeasure::Millimeter);

      // write to poi file
      poiFile<<locn<<_T(", ")<< bridgeId<< _T(", 7, 1, ")<<loc<<_T(", 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0")<<std::endl;

      // girder 
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30000, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( girderLoadStage, pftGirder, poi, bat ), unitMeasure::NewtonMillimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30001, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetShear( girderLoadStage, pftGirder, poi, bat ).Left(), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30002, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( girderLoadStage, pftGirder, poi, bat ), unitMeasure::Millimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30200, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( girderLoadStage, pftGirder, span, gdr, bat) , unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // diaphragm
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30009, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite1, pftDiaphragm,poi, bat ), unitMeasure::NewtonMillimeter)) << _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30010, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite1, pftDiaphragm, poi, bat ).Left(), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30011, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite1, pftDiaphragm, poi, bat ), unitMeasure::Millimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30209, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite1, pftDiaphragm, span, gdr, bat), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // ShearKey
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30070, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite1, pftShearKey,poi, bat ), unitMeasure::NewtonMillimeter)) << _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30071, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite1, pftShearKey, poi, bat ).Left(), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30072, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite1, pftShearKey, poi, bat ), unitMeasure::Millimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30270, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite1, pftShearKey, span, gdr, bat), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
       
      // slab + slab pad
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30012, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite1, pftSlab,poi, bat )         + pForce->GetMoment( pgsTypes::BridgeSite1, pftSlabPad,poi, bat ),         unitMeasure::NewtonMillimeter)) << _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30013, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite1, pftSlab, poi, bat ).Left()  + pForce->GetShear( pgsTypes::BridgeSite1, pftSlabPad, poi, bat ).Left(),  unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30014, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite1, pftSlab, poi, bat )  + pForce->GetDisplacement( pgsTypes::BridgeSite1, pftSlabPad, poi, bat ),  unitMeasure::Millimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30013, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite1, pftSlab, span, gdr, bat) + pForce->GetReaction( pgsTypes::BridgeSite1, pftSlabPad, span, gdr, bat), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // DC - BSS1
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30036, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForces->GetMoment( lcDC, pgsTypes::BridgeSite1, poi, ctCummulative, bat ), unitMeasure::NewtonMillimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30037, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForces->GetShear( lcDC, pgsTypes::BridgeSite1, poi, ctCummulative, bat ).Left(), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30038, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForces->GetDisplacement( lcDC, pgsTypes::BridgeSite1, poi, ctCummulative, bat ), unitMeasure::Millimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30236, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForces->GetReaction( lcDC, pgsTypes::BridgeSite1, span, gdr, ctCummulative, bat ), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // DW - BSS1
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30039, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForces->GetMoment( lcDW, pgsTypes::BridgeSite1, poi, ctCummulative, bat ), unitMeasure::NewtonMillimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30040, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForces->GetShear( lcDW, pgsTypes::BridgeSite1, poi, ctCummulative, bat ).Left(), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30041, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForces->GetDisplacement( lcDW, pgsTypes::BridgeSite1, poi, ctCummulative, bat ), unitMeasure::Millimeter)) <<_T(", 1, ")<<gdr<<std::endl;

      // overlay
      pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30042, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( overlay_stage, pftOverlay,poi, bat ), unitMeasure::NewtonMillimeter)) << _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30043, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetShear( overlay_stage, pftOverlay, poi, bat ).Left(), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30044, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( overlay_stage, pftOverlay, poi, bat ), unitMeasure::Millimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30242, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( overlay_stage, pftOverlay, span, gdr, bat), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // barrier
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30045, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite2, pftTrafficBarrier,poi, bat ), unitMeasure::NewtonMillimeter)) << _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30046, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite2, pftTrafficBarrier, poi, bat ).Left(), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30047, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite2, pftTrafficBarrier, poi, bat ), unitMeasure::Millimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30245, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite2, pftTrafficBarrier, span, gdr, bat ), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // sidewalk
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30048, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite2, pftSidewalk,poi, bat ), unitMeasure::NewtonMillimeter)) << _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30049, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite2, pftSidewalk, poi, bat ).Left(), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30050, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite2, pftSidewalk, poi, bat ), unitMeasure::Millimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30248, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite2, pftSidewalk, span, gdr, bat ), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // DC - BSS3
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30057, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForces->GetMoment( lcDC, pgsTypes::BridgeSite3, poi, ctCummulative, bat ), unitMeasure::NewtonMillimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30058, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForces->GetShear( lcDC, pgsTypes::BridgeSite3, poi, ctCummulative, bat ).Left(), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30059, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForces->GetDisplacement( lcDC, pgsTypes::BridgeSite3, poi, ctCummulative, bat ), unitMeasure::Millimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30257, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForces->GetReaction( lcDC, pgsTypes::BridgeSite3, span, gdr, ctCummulative, bat ), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // DW - BSS3
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30060, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForces->GetMoment( lcDW, pgsTypes::BridgeSite3, poi, ctCummulative, bat ), unitMeasure::NewtonMillimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30061, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForces->GetShear( lcDW, pgsTypes::BridgeSite3, poi, ctCummulative, bat ).Left(), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30062, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForces->GetDisplacement( lcDW, pgsTypes::BridgeSite3, poi, ctCummulative, bat ), unitMeasure::Millimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30260, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForces->GetReaction( lcDW, pgsTypes::BridgeSite3, span, gdr, ctCummulative, bat ), unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // user loads - Moment
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122100, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite1, pftUserDW, poi, bat ), unitMeasure::NewtonMillimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122101, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite1, pftUserDC, poi, bat ), unitMeasure::NewtonMillimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122102, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite2, pftUserDW, poi, bat ), unitMeasure::NewtonMillimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122103, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite2, pftUserDC, poi, bat ), unitMeasure::NewtonMillimeter)) <<_T(", 1, ")<<gdr<<std::endl;

      // user loads - Shear
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122104, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite1, pftUserDW, poi, bat ).Left(), unitMeasure::Newton)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122105, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite1, pftUserDC, poi, bat ).Left(), unitMeasure::Newton)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122106, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite2, pftUserDW, poi, bat ).Left(), unitMeasure::Newton)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122107, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite2, pftUserDC, poi, bat ).Left(), unitMeasure::Newton)) <<_T(", 1, ")<<gdr<<std::endl;

      // user loads - Displacement
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122108, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite1, pftUserDW, poi, bat ), unitMeasure::Millimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122109, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite1, pftUserDC, poi, bat ), unitMeasure::Millimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122110, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite2, pftUserDW, poi, bat ), unitMeasure::Millimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122111, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite2, pftUserDC, poi, bat ), unitMeasure::Millimeter)) <<_T(", 1, ")<<gdr<<std::endl;

      // user loads - Reaction
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122112, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite1, pftUserDW, span, gdr, bat ), unitMeasure::Newton)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122113, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite1, pftUserDC, span, gdr, bat ), unitMeasure::Newton)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122114, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite2, pftUserDW, span, gdr, bat ), unitMeasure::Newton)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122115, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite2, pftUserDC, span, gdr, bat ), unitMeasure::Newton)) <<_T(", 1, ")<<gdr<<std::endl;

      // user live load
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122116, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetMoment( pgsTypes::BridgeSite1, pftUserLLIM, poi, bat ), unitMeasure::NewtonMillimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122117, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetShear( pgsTypes::BridgeSite1, pftUserLLIM, poi, bat ).Left(), unitMeasure::Newton)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122118, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetDisplacement( pgsTypes::BridgeSite1, pftUserLLIM, poi, bat ), unitMeasure::Millimeter)) <<_T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122119, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pForce->GetReaction( pgsTypes::BridgeSite1, pftUserLLIM, span, gdr, bat ), unitMeasure::Newton)) <<_T(", 1, ")<<gdr<<std::endl;

   }

   // Girder bearing reactions
   GET_IFACE(IBearingDesign,pBearingDesign);
   bool bleft, bright;
   if(pBearingDesign->AreBearingReactionsAvailable(span,gdr,&bleft,&bright))
   {
      Float64 lftReact, rgtReact;
      // girder
      pBearingDesign->GetBearingProductReaction(girderLoadStage, pftGirder, span, gdr, ctCummulative, bat, &lftReact, &rgtReact);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165000, 0.000, ")<< QUITE(::ConvertFromSysUnits( lftReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165001,-1.000, ")<< QUITE(::ConvertFromSysUnits( rgtReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // diaphragm
      pBearingDesign->GetBearingProductReaction(pgsTypes::BridgeSite1, pftDiaphragm, span, gdr, ctCummulative, bat, &lftReact, &rgtReact);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165002, 0.000, ")<< QUITE(::ConvertFromSysUnits( lftReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165003,-1.000, ")<< QUITE(::ConvertFromSysUnits( rgtReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // ShearKey
      pBearingDesign->GetBearingProductReaction(pgsTypes::BridgeSite1, pftShearKey, span, gdr, ctCummulative, bat, &lftReact, &rgtReact);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165004, 0.000, ")<< QUITE(::ConvertFromSysUnits( lftReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165005,-1.000, ")<< QUITE(::ConvertFromSysUnits( rgtReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // slab
      pBearingDesign->GetBearingProductReaction(pgsTypes::BridgeSite1, pftSlab, span, gdr, ctCummulative, bat, &lftReact, &rgtReact);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165006, 0.000, ")<< QUITE(::ConvertFromSysUnits( lftReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165007,-1.000, ")<< QUITE(::ConvertFromSysUnits( rgtReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // DC - BSS1
      pBearingDesign->GetBearingCombinedReaction(lcDC, pgsTypes::BridgeSite1, span, gdr, ctCummulative, bat, &lftReact, &rgtReact); 
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165008, 0.000, ")<< QUITE(::ConvertFromSysUnits( lftReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165009,-1.000, ")<< QUITE(::ConvertFromSysUnits( rgtReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // DW - BSS1
      pBearingDesign->GetBearingCombinedReaction(lcDW, pgsTypes::BridgeSite1, span, gdr, ctCummulative, bat, &lftReact, &rgtReact); 
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165010, 0.000, ")<< QUITE(::ConvertFromSysUnits( lftReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165011,-1.000, ")<< QUITE(::ConvertFromSysUnits( rgtReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // overlay
      pgsTypes::Stage overlay_stage = pBridge->IsFutureOverlay() ? pgsTypes::BridgeSite3 : pgsTypes::BridgeSite2;

      pBearingDesign->GetBearingProductReaction(overlay_stage, pftOverlay, span, gdr, ctCummulative, bat, &lftReact, &rgtReact);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165012, 0.000, ")<< QUITE(::ConvertFromSysUnits( lftReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165013,-1.000, ")<< QUITE(::ConvertFromSysUnits( rgtReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // barrier
      pBearingDesign->GetBearingProductReaction(pgsTypes::BridgeSite2, pftTrafficBarrier, span, gdr, ctCummulative, bat, &lftReact, &rgtReact);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165014, 0.000, ")<< QUITE(::ConvertFromSysUnits( lftReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165015,-1.000, ")<< QUITE(::ConvertFromSysUnits( rgtReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // sidewalk
      pBearingDesign->GetBearingProductReaction(pgsTypes::BridgeSite3, pftSidewalk, span, gdr, ctCummulative, bat, &lftReact, &rgtReact);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165016, 0.000, ")<< QUITE(::ConvertFromSysUnits( lftReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165017,-1.000, ")<< QUITE(::ConvertFromSysUnits( rgtReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // DC - BSS3
      pBearingDesign->GetBearingCombinedReaction(lcDC, pgsTypes::BridgeSite3, span, gdr, ctCummulative, bat, &lftReact, &rgtReact); 
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165018, 0.000, ")<< QUITE(::ConvertFromSysUnits( lftReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165019,-1.000, ")<< QUITE(::ConvertFromSysUnits( rgtReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // DW - BSS3
      pBearingDesign->GetBearingCombinedReaction(lcDW, pgsTypes::BridgeSite3, span, gdr, ctCummulative, bat, &lftReact, &rgtReact); 
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165020, 0.000, ")<< QUITE(::ConvertFromSysUnits( lftReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165021,-1.000, ")<< QUITE(::ConvertFromSysUnits( rgtReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // user loads
      pBearingDesign->GetBearingProductReaction(pgsTypes::BridgeSite3, pftUserDW, span, gdr, ctCummulative, bat, &lftReact, &rgtReact);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165022, 0.000, ")<< QUITE(::ConvertFromSysUnits( lftReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165023,-1.000, ")<< QUITE(::ConvertFromSysUnits( rgtReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;

      // user live load
      pBearingDesign->GetBearingProductReaction(pgsTypes::BridgeSite3, pftUserLLIM, span, gdr, ctCummulative, bat, &lftReact, &rgtReact);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165024, 0.000, ")<< QUITE(::ConvertFromSysUnits( lftReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165025,-1.000, ")<< QUITE(::ConvertFromSysUnits( rgtReact, unitMeasure::Newton)) <<    _T(", 1, ")<<gdr<<std::endl;
   }

   return true;
}

bool CTestAgentImp::RunCombinedLoadActionTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   GET_IFACE( IPointOfInterest,   pIPoi);
   GET_IFACE( ILimitStateForces,  pLsForces);
   GET_IFACE( ICombinedForces,    pCombinedForces);
   GET_IFACE( ISpecification,     pSpec);
   GET_IFACE( IBearingDesign,     pBearingDesign);

   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   PoiAttributeType attrib = POI_TABULAR | POI_FLEXURESTRESS;
   std::vector<pgsPointOfInterest> vPoi;

   vPoi = pIPoi->GetPointsOfInterest( span,gdr,pgsTypes::BridgeSite3,attrib);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   BridgeAnalysisType bat;

   Float64 min, max, dummy;

   CollectionIndexType npoi = vPoi.size();
   for (CollectionIndexType i=0; i<npoi; i++)
   {
      pgsPointOfInterest& poi = vPoi[i];
      CollectionIndexType locn = i+1;
      Float64 loc = ::ConvertFromSysUnits(poi.GetDistFromStart(), unitMeasure::Millimeter);

      // write to poi file
      poiFile<<locn<<_T(", ")<< bridgeId<< _T(", 7, 1, ")<<loc<<_T(", 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0")<<std::endl;

      // Strength I 
      if ( analysisType == pgsTypes::Envelope )
      {
         pLsForces->GetMoment( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &dummy, &max );
         pLsForces->GetMoment( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &min, &dummy );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34000, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::NewtonMillimeter)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34001, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::NewtonMillimeter)) <<_T(", 8, ")<<gdr<<std::endl;

         sysSectionValue smin, smax, svDummy;
         pLsForces->GetShear( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &svDummy, &smax );
         pLsForces->GetShear( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &smin, &svDummy );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34002, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(smax.Left(), unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34003, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(smin.Left(), unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

         pLsForces->GetDisplacement( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &dummy, &max );
         pLsForces->GetDisplacement( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &min, &dummy );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34004, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Millimeter)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34005, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Millimeter)) <<_T(", 8, ")<<gdr<<std::endl;

         // Service I
         pLsForces->GetMoment( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &dummy, &max );
         pLsForces->GetMoment( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &min, &dummy );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34020, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::NewtonMillimeter)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34021, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::NewtonMillimeter)) <<_T(", 8, ")<<gdr<<std::endl;

         pLsForces->GetShear( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &svDummy, &smax );
         pLsForces->GetShear( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &smin, &svDummy );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34022, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(smax.Left(), unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34023, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(smin.Left(), unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

         pLsForces->GetDisplacement( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &dummy, &max );
         pLsForces->GetDisplacement( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &min, &dummy );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34024, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Millimeter)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34025, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Millimeter)) <<_T(", 8, ")<<gdr<<std::endl;

         // Service III
         pLsForces->GetMoment( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &dummy, &max );
         pLsForces->GetMoment( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &min, &dummy );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34032, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::NewtonMillimeter)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34033, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::NewtonMillimeter)) <<_T(", 8, ")<<gdr<<std::endl;

         pLsForces->GetShear( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &svDummy, &smax );
         pLsForces->GetShear( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &smin, &svDummy );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34034, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(smax.Left(), unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34035, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(smin.Left(), unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

         pLsForces->GetDisplacement( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, MaxSimpleContinuousEnvelope, &dummy, &max );
         pLsForces->GetDisplacement( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, MinSimpleContinuousEnvelope, &min, &dummy );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34036, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Millimeter)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34037, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Millimeter)) <<_T(", 8, ")<<gdr<<std::endl;
      }
      else
      {
         bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
         pLsForces->GetMoment( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, bat, &min, &max );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34000, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::NewtonMillimeter)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34001, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::NewtonMillimeter)) <<_T(", 8, ")<<gdr<<std::endl;

         sysSectionValue smin, smax;
         pLsForces->GetShear( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, bat, &smin, &smax );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34002, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(smax.Left(), unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34003, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(smin.Left(), unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

         pLsForces->GetDisplacement( pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi, bat, &min, &max );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34004, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Millimeter)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34005, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Millimeter)) <<_T(", 8, ")<<gdr<<std::endl;

         // Service I
         pLsForces->GetMoment( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, bat, &min, &max );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34020, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::NewtonMillimeter)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34021, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::NewtonMillimeter)) <<_T(", 8, ")<<gdr<<std::endl;

         pLsForces->GetShear( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, bat, &smin, &smax );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34022, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(smax.Left(), unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34023, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(smin.Left(), unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

         pLsForces->GetDisplacement( pgsTypes::ServiceI, pgsTypes::BridgeSite3, poi, bat, &min, &max );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34024, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Millimeter)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34025, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Millimeter)) <<_T(", 8, ")<<gdr<<std::endl;

         // Service III
         pLsForces->GetMoment( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, bat, &min, &max );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34032, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::NewtonMillimeter)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34033, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::NewtonMillimeter)) <<_T(", 8, ")<<gdr<<std::endl;

         pLsForces->GetShear( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, bat, &smin, &smax );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34034, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(smax.Left(), unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34035, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(smin.Left(), unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

         pLsForces->GetDisplacement( pgsTypes::ServiceIII, pgsTypes::BridgeSite3, poi, bat, &min, &max );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34036, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Millimeter)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34037, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Millimeter)) <<_T(", 8, ")<<gdr<<std::endl;
      }
   }

   // Reactions
   Float64 lftloc = ::ConvertFromSysUnits(vPoi.front().GetDistFromStart(), unitMeasure::Millimeter);
   Float64 rgtloc = ::ConvertFromSysUnits(vPoi.back().GetDistFromStart(), unitMeasure::Millimeter);

   if ( analysisType == pgsTypes::Envelope )
   {
      // left end
      pLsForces->GetReaction(pgsTypes::StrengthI, pgsTypes::BridgeSite3, span, gdr, MaxSimpleContinuousEnvelope, true, &dummy, &max );
      pLsForces->GetReaction(pgsTypes::StrengthI, pgsTypes::BridgeSite3, span, gdr, MinSimpleContinuousEnvelope, true,  &min, &dummy );
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34040, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34041, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

      // right end
      pLsForces->GetReaction(pgsTypes::StrengthI, pgsTypes::BridgeSite3, span+1, gdr, MaxSimpleContinuousEnvelope, true, &dummy, &max );
      pLsForces->GetReaction(pgsTypes::StrengthI, pgsTypes::BridgeSite3, span+1, gdr, MinSimpleContinuousEnvelope, true,  &min, &dummy );
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34040, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34041, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

      pLsForces->GetReaction(pgsTypes::ServiceI, pgsTypes::BridgeSite3, span, gdr, MaxSimpleContinuousEnvelope, true, &dummy, &max );
      pLsForces->GetReaction(pgsTypes::ServiceI, pgsTypes::BridgeSite3, span, gdr, MinSimpleContinuousEnvelope, true,  &min, &dummy );
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34042, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34043, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

      pLsForces->GetReaction(pgsTypes::ServiceI, pgsTypes::BridgeSite3, span+1, gdr, MaxSimpleContinuousEnvelope, true, &dummy, &max );
      pLsForces->GetReaction(pgsTypes::ServiceI, pgsTypes::BridgeSite3, span+1, gdr, MinSimpleContinuousEnvelope, true,  &min, &dummy );
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34042, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34043, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

      pCombinedForces->GetCombinedLiveLoadReaction(pgsTypes::lltDesign, pgsTypes::BridgeSite3, span, gdr, MaxSimpleContinuousEnvelope, false, &dummy, &max);
      pCombinedForces->GetCombinedLiveLoadReaction(pgsTypes::lltDesign, pgsTypes::BridgeSite3, span, gdr, MinSimpleContinuousEnvelope, false, &min, &dummy);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34050, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34051, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

      pCombinedForces->GetCombinedLiveLoadReaction(pgsTypes::lltDesign, pgsTypes::BridgeSite3, span+1, gdr, MaxSimpleContinuousEnvelope, false, &dummy, &max);
      pCombinedForces->GetCombinedLiveLoadReaction(pgsTypes::lltDesign, pgsTypes::BridgeSite3, span+1, gdr, MinSimpleContinuousEnvelope, false, &min, &dummy);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34050, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34051, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

      bool isLeft, isRight;
      pBearingDesign->AreBearingReactionsAvailable(span, gdr, &isLeft, &isRight);
      if (isLeft || isRight)
      {
         Float64 leftVal, rightVal;
         pBearingDesign->GetBearingLimitStateReaction(pgsTypes::StrengthI, pgsTypes::BridgeSite3, span, gdr, MaxSimpleContinuousEnvelope, true, &dummy, &leftVal, &dummy, &rightVal);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34044, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(leftVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34044, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rightVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

         pBearingDesign->GetBearingLimitStateReaction(pgsTypes::StrengthI, pgsTypes::BridgeSite3, span, gdr, MinSimpleContinuousEnvelope, true, &leftVal, &dummy, &rightVal, &dummy);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34045, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(leftVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34045, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rightVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

         pBearingDesign->GetBearingLimitStateReaction(pgsTypes::ServiceI, pgsTypes::BridgeSite3, span, gdr, MaxSimpleContinuousEnvelope, true, &dummy, &leftVal, &dummy, &rightVal);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34046, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(leftVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34046, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rightVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

         pBearingDesign->GetBearingLimitStateReaction(pgsTypes::ServiceI, pgsTypes::BridgeSite3, span, gdr, MinSimpleContinuousEnvelope, true, &leftVal, &dummy, &rightVal, &dummy);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34047, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(leftVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34047, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rightVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

         pBearingDesign->GetBearingLiveLoadReaction(pgsTypes::lltDesign, pgsTypes::BridgeSite3, span, gdr, MaxSimpleContinuousEnvelope, true, true, &dummy, &leftVal, &dummy, &dummy, &dummy, &rightVal, &dummy, &dummy);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34050, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(leftVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34050, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rightVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

         pBearingDesign->GetBearingLiveLoadReaction(pgsTypes::lltDesign, pgsTypes::BridgeSite3, span, gdr, MinSimpleContinuousEnvelope, true, true, &leftVal, &dummy, &dummy, &dummy, &rightVal, &dummy, &dummy, &dummy);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34051, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(leftVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34051, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rightVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
      }
   }
   else
   {
      bat = (analysisType == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
      // left end
      pLsForces->GetReaction(pgsTypes::StrengthI, pgsTypes::BridgeSite3, span, gdr, bat, true, &min, &max );
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34040, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34041, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

      // right end
      pLsForces->GetReaction(pgsTypes::StrengthI, pgsTypes::BridgeSite3, span+1, gdr, bat, true, &min, &max );
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34040, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34041, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

      pLsForces->GetReaction(pgsTypes::ServiceI, pgsTypes::BridgeSite3, span, gdr, bat, true, &min, &max );
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34042, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34043, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

      pLsForces->GetReaction(pgsTypes::ServiceI, pgsTypes::BridgeSite3, span+1, gdr, bat, true, &min, &max );
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34042, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34043, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

      bool isLeft, isRight;
      pBearingDesign->AreBearingReactionsAvailable(span, gdr, &isLeft, &isRight);
      if (isLeft || isRight)
      {
         Float64 leftMinVal, rightMinVal, leftMaxVal, rightMaxVal;
         pBearingDesign->GetBearingLimitStateReaction(pgsTypes::StrengthI, pgsTypes::BridgeSite3, span, gdr, bat, true, &leftMinVal, &leftMaxVal, &rightMinVal, &rightMaxVal);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34044, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(leftMaxVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34044, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rightMaxVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34045, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(leftMinVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34045, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rightMinVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

         pBearingDesign->GetBearingLimitStateReaction(pgsTypes::ServiceI, pgsTypes::BridgeSite3, span, gdr, bat, true, &leftMinVal, &leftMaxVal, &rightMinVal, &rightMaxVal);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34046, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(leftMaxVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34046, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rightMaxVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34047, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(leftMinVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34047, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rightMinVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;

         pBearingDesign->GetBearingLiveLoadReaction(pgsTypes::lltDesign, pgsTypes::BridgeSite3, span, gdr, bat, true, true, &leftMinVal, &leftMaxVal, &dummy, &dummy, &rightMinVal, &rightMaxVal, &dummy, &dummy);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34050, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(leftMaxVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34050, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rightMaxVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34051, ")<<lftloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(leftMinVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34051, ")<<rgtloc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rightMinVal, unitMeasure::Newton)) <<_T(", 8, ")<<gdr<<std::endl;
      }
   }


   return true;
}

bool CTestAgentImp::RunPrestressedISectionTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
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

   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   // first dump out non-poi related values
   // losses
   Float64 loc = 0.0;

   const pgsGirderArtifact* gdrArtifact = pIArtifact->GetArtifact(span,gdr);
   const pgsStirrupCheckArtifact* pstirrup_artifact= gdrArtifact->GetStirrupCheckArtifact();
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100200 , ")<<loc<<_T(", ")<<(int)(gdrArtifact->Passed()?1:0) <<_T(", 15, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100201 , ")<<loc<<_T(", ")<<(int)(pstirrup_artifact->Passed()?1:0)<<_T(", 15, ")<<gdr<<std::endl;

   Float64 min, max;
   pShearCapacity->GetCriticalSection(pgsTypes::StrengthI,span,gdr,&min,&max);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50052, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Millimeter)) <<_T(", 15, ")<<gdr<<std::endl;

   const pgsConstructabilityArtifact* pConstruct =  gdrArtifact->GetConstructabilityArtifact();
   if ( pConstruct->IsSlabOffsetApplicable() )
   {
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122005, ")<<loc<<_T(", ")<<(int)(pConstruct->Passed?1:0)<<_T(", 15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122014, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pConstruct->GetRequiredSlabOffset(), unitMeasure::Millimeter)) <<_T(", 2, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122015, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pConstruct->GetProvidedSlabOffset(), unitMeasure::Millimeter)) <<_T(", 2, ")<<gdr<<std::endl;
   }

   const pgsDeflectionCheckArtifact* pDefl = gdrArtifact->GetDeflectionCheckArtifact();
   pDefl->GetDemand(&min,&max);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122006, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min, unitMeasure::Millimeter)) <<_T(", 2, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122007, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pDefl->GetCapacity(), unitMeasure::Millimeter)) <<_T(", 2, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122008, ")<<loc<<_T(", ")<<(int)(pDefl->Passed()?1:0)<<_T(", 15, ")<<gdr<<std::endl;

   const pgsSplittingZoneArtifact* pBurst = gdrArtifact->GetStirrupCheckArtifact()->GetSplittingZoneArtifact();
   if (pBurst->GetIsApplicable())
   {
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122010, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pBurst->GetStartSplittingZoneLength(), unitMeasure::Millimeter)) <<_T(", 2, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122011, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pBurst->GetStartSplittingForce(), unitMeasure::Newton)) <<_T(", 2, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122012, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pBurst->GetStartSplittingResistance(), unitMeasure::Newton)) <<_T(", 2, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122013, ")<<loc<<_T(", ")<<(int)(pBurst->Passed()?1:0)<<_T(", 15, ")<<gdr<<std::endl;
   }

   const pgsHoldDownForceArtifact* pHoldDownForce = gdrArtifact->GetHoldDownForceArtifact();
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122029, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pHoldDownForce->GetDemand(), unitMeasure::Newton)) <<_T(", 2, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122030, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pHoldDownForce->GetCapacity(), unitMeasure::Newton)) <<_T(", 2, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122031, ")<<loc<<_T(", ")<<(int)(pHoldDownForce->Passed()?1:0)<<_T(", 15, ")<<gdr<<std::endl;

   const pgsPrecastIGirderDetailingArtifact* pBeamDetails = gdrArtifact->GetPrecastIGirderDetailingArtifact();
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122032, ")<<loc<<_T(", ")<<(int)(pBeamDetails->Passed()?1:0)<<_T(", 15, ")<<gdr<<std::endl;

   const pgsStrandSlopeArtifact* pSlope = gdrArtifact->GetStrandSlopeArtifact();
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122033, ")<<loc<<_T(", ")<< QUITE(pSlope->GetDemand()) <<_T(", 2, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122034, ")<<loc<<_T(", ")<< QUITE(pSlope->GetCapacity()) <<_T(", 2, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122035, ")<<loc<<_T(", ")<<(int)(pSlope->Passed()?1:0)<<_T(", 15, ")<<gdr<<std::endl;

   const pgsStrandStressArtifact* sStrand = gdrArtifact->GetStrandStressArtifact();
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122036, ")<<loc<<_T(", ")<<(int)(sStrand->Passed()?1:0)<<_T(", 15, ")<<gdr<<std::endl;

   // next poi-related values
   const pgsStirrupCheckAtPoisArtifact* psArtifact = 0;
   const pgsLongReinfShearArtifact* pArtifact      = 0;
   const pgsStirrupDetailArtifact* pSDArtifact     = 0;
   const pgsHorizontalShearArtifact* pAHsrtifact   = 0;

   PoiAttributeType attrib = POI_TABULAR; //POI_FLEXURESTRESS;// | POI_FLEXURECAPACITY | POI_SHEAR;
   std::vector<pgsPointOfInterest> vPoi;
   vPoi = pIPoi->GetPointsOfInterest( span,gdr,pgsTypes::BridgeSite3,attrib);
   for (std::vector<pgsPointOfInterest>::iterator it=vPoi.begin(); it!=vPoi.end(); it++)
   {
      const pgsPointOfInterest& poi = *it;

      Float64 loc = ::ConvertFromSysUnits(poi.GetDistFromStart(), unitMeasure::Millimeter);

      // write to poi file
      poiFile<<_T(" 1, ")<< bridgeId<< _T(", 3, 1, ")<<loc<<_T(", 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0")<<std::endl;

      //resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50006, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetFinal(poi,pgsTypes::Permanent), unitMeasure::MPa)) <<_T(",15, ")<<gdr<<std::endl;
      //resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50007, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetElasticShortening(poi,pgsTypes::Permanent), unitMeasure::MPa)) <<_T(", 2, 15, ")<<gdr<<std::endl;

      // The 12-50 test id's for prestress losses are totally inadequate for the current LRFD. New tests are implemented
      // that are more _T("generic"). 550xx are for permanent strands and 551xx are for temporary strands
//      LOSSDETAILS losses = pLosses->GetLossDetails(poi);
//      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50008, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(losses.RefinedLosses.ShrinkageLosses(), unitMeasure::MPa)) <<         _T(",15, ")<<gdr<<std::endl;
//      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50009, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(losses.RefinedLosses.CreepLosses(), unitMeasure::MPa)) <<              _T(",15, ")<<gdr<<std::endl;
//      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50010, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(losses.RefinedLosses.RelaxationLossesAtXfer(), unitMeasure::MPa)) <<   _T(",15, ")<<gdr<<std::endl;
//      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50011, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(losses.RefinedLosses.RelaxationLossesAfterXfer(), unitMeasure::MPa)) <<_T(",15, ")<<gdr<<std::endl;
//      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50012, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(losses.RefinedLosses.ElasticShortening().GetFcgp(), unitMeasure::MPa)) <<                 _T(",15, ")<<gdr<<std::endl;
//      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50013, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(losses.RefinedLosses.GetEci(), unitMeasure::MPa)) <<                  _T(",15, ")<<gdr<<std::endl;
//
//      // DUMMY VALUE... Fix Test!!!
//      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50014, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(losses.RefinedLosses.GetDeltaFcdp(), unitMeasure::MPa)) <<             _T(",15, ")<<gdr<<std::endl;

// loss in permanent strands
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55001, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetElasticShortening(poi,pgsTypes::Permanent),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55002, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetBeforeXferLosses(poi,pgsTypes::Permanent),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55003, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetAfterXferLosses(poi,pgsTypes::Permanent),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55004, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetLiftingLosses(poi,pgsTypes::Permanent),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55005, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetShippingLosses(poi,pgsTypes::Permanent),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55006, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Permanent),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55007, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetBeforeTemporaryStrandRemovalLosses(poi,pgsTypes::Permanent),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55008, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetAfterTemporaryStrandRemovalLosses(poi,pgsTypes::Permanent),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55009, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetDeckPlacementLosses(poi,pgsTypes::Permanent),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55009a,")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetSIDLLosses(poi,pgsTypes::Permanent),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55010, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetFinal(poi,pgsTypes::Permanent),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
// loss in temporary strands
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55101, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetElasticShortening(poi,pgsTypes::Temporary),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55102, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetBeforeXferLosses(poi,pgsTypes::Temporary),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55103, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetAfterXferLosses(poi,pgsTypes::Temporary),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55104, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetLiftingLosses(poi,pgsTypes::Temporary),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55105, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetShippingLosses(poi,pgsTypes::Temporary),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55106, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetAfterTemporaryStrandInstallationLosses(poi,pgsTypes::Temporary),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55107, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetBeforeTemporaryStrandRemovalLosses(poi,pgsTypes::Temporary),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55108, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetAfterTemporaryStrandRemovalLosses(poi,pgsTypes::Temporary),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55109, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetDeckPlacementLosses(poi,pgsTypes::Temporary),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55109a,")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetSIDLLosses(poi,pgsTypes::Temporary),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 55110, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pLosses->GetFinal(poi,pgsTypes::Temporary),unitMeasure::MPa)) << _T(",15, ")<<gdr<<std::endl;

      // eff flange width
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50001, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSp2->GetEffectiveFlangeWidth(poi), unitMeasure::Millimeter)) <<_T(", 2, ")<<gdr<<std::endl;

      if ( poi.IsFlexureStress(pgsTypes::BridgeSite3) )
      {
         // force and stress in prestressing strands
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50002, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::AfterXfer), unitMeasure::MPa)) <<         _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50003, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pPrestressForce->GetStrandForce(poi,pgsTypes::Permanent,pgsTypes::AfterXfer), unitMeasure::Newton)) <<       _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50004, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::AfterLosses), unitMeasure::MPa)) <<  _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50005, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pPrestressForce->GetStrandForce(poi,pgsTypes::Permanent,pgsTypes::AfterLosses), unitMeasure::Newton)) <<_T(",15, ")<<gdr<<std::endl;

         // stresses due to external loads
         // casting yards
         pLsForces->GetStress(pgsTypes::ServiceI, pgsTypes::CastingYard,poi,pgsTypes::TopGirder, true, SimpleSpan, &min,&max);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50018, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<_T(",15, ")<<gdr<<std::endl;
         pLsForces->GetStress(pgsTypes::ServiceI, pgsTypes::CastingYard,poi,pgsTypes::BottomGirder,true, SimpleSpan,&min,&max);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50019, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<_T(",15, ")<<gdr<<std::endl;

         // bridge site 2
         pLsForces->GetStress(pgsTypes::ServiceI, pgsTypes::BridgeSite2,poi,pgsTypes::TopGirder, true, SimpleSpan,&min,&max);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50020, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<_T(",15, ")<<gdr<<std::endl;

         // bridge site 3
         pLsForces->GetStress(pgsTypes::ServiceI, pgsTypes::BridgeSite3,poi,pgsTypes::TopGirder,true, SimpleSpan,&min,&max);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50021, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<_T(",15, ")<<gdr<<std::endl;
         pLsForces->GetStress(pgsTypes::ServiceI, pgsTypes::BridgeSite3,poi,pgsTypes::BottomGirder,true, SimpleSpan,&min,&max);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50022, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max , unitMeasure::MPa)) <<_T(",15, ")<<gdr<<std::endl;
         pLsForces->GetStress(pgsTypes::ServiceI, pgsTypes::BridgeSite3,poi,pgsTypes::TopSlab,true, SimpleSpan,&min,&max);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50023, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<_T(",15, ")<<gdr<<std::endl;

         pLsForces->GetStress(pgsTypes::ServiceIII, pgsTypes::BridgeSite3,poi,pgsTypes::TopGirder,true, SimpleSpan,&min,&max);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50024, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<_T(",15, ")<<gdr<<std::endl;
         pLsForces->GetStress(pgsTypes::ServiceIII, pgsTypes::BridgeSite3,poi,pgsTypes::BottomGirder,true, SimpleSpan,&min,&max);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50025, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(max , unitMeasure::MPa)) <<_T(",15, ")<<gdr<<std::endl;
         pLsForces->GetStress(pgsTypes::ServiceIII, pgsTypes::BridgeSite3,poi,pgsTypes::TopSlab,true, SimpleSpan,&min,&max);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50026, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<_T(",15, ")<<gdr<<std::endl;

         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
         {
            pLsForces->GetStress(pgsTypes::ServiceIA, pgsTypes::BridgeSite3,poi,pgsTypes::TopSlab,true, SimpleSpan,&min,&max);
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50027, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<_T(",15, ")<<gdr<<std::endl;
         }
         else
         {
            pLsForces->GetStress(pgsTypes::FatigueI, pgsTypes::BridgeSite3,poi,pgsTypes::TopSlab,true, SimpleSpan,&min,&max);
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50027, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(min , unitMeasure::MPa)) <<_T(",15, ")<<gdr<<std::endl;
         }

         // stresses due to prestress
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 56018, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pPrestressStresses->GetStress(pgsTypes::CastingYard,poi,pgsTypes::TopGirder), unitMeasure::MPa)) <<         _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 56019, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pPrestressStresses->GetStress(pgsTypes::CastingYard,poi,pgsTypes::BottomGirder), unitMeasure::MPa)) <<         _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 56020, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pPrestressStresses->GetStress(pgsTypes::BridgeSite1,poi,pgsTypes::TopGirder), unitMeasure::MPa)) <<         _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 56021, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pPrestressStresses->GetStress(pgsTypes::BridgeSite1,poi,pgsTypes::BottomGirder), unitMeasure::MPa)) <<         _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 56022, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pPrestressStresses->GetStress(pgsTypes::BridgeSite2,poi,pgsTypes::TopGirder), unitMeasure::MPa)) <<         _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 56023, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pPrestressStresses->GetStress(pgsTypes::BridgeSite2,poi,pgsTypes::BottomGirder), unitMeasure::MPa)) <<         _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 56024, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pPrestressStresses->GetStress(pgsTypes::BridgeSite3,poi,pgsTypes::TopGirder), unitMeasure::MPa)) <<         _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 56025, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pPrestressStresses->GetStress(pgsTypes::BridgeSite3,poi,pgsTypes::BottomGirder), unitMeasure::MPa)) <<         _T(",15, ")<<gdr<<std::endl;
      }

      if ( poi.IsFlexureCapacity(pgsTypes::BridgeSite3) || poi.IsShear(pgsTypes::BridgeSite3) )
      {
         // positive moment capacity
         CRACKINGMOMENTDETAILS cmd;
         pMomentCapacity->GetCrackingMomentDetails(pgsTypes::BridgeSite3,poi,true,&cmd);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50028, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(cmd.Mcr , unitMeasure::NewtonMillimeter)) <<_T(",15, ")<<gdr<<std::endl;

         MINMOMENTCAPDETAILS mmcd;
         pMomentCapacity->GetMinMomentCapacityDetails(pgsTypes::BridgeSite3,poi,true,&mmcd);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50029, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(mmcd.Mr , unitMeasure::NewtonMillimeter)) <<_T(",15, ")<<gdr<<std::endl;

         MOMENTCAPACITYDETAILS mcd;
         pMomentCapacity->GetMomentCapacityDetails(pgsTypes::BridgeSite3,poi,true,&mcd);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50030, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(mcd.Mn , unitMeasure::NewtonMillimeter)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50035, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(mcd.c, unitMeasure::Millimeter)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50036, ")<<loc<<_T(", ")<< QUITE(IsZero(mcd.de) ? 0 : mcd.c/mcd.de) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50039, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(mcd.de, unitMeasure::Millimeter)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50040, ")<<loc<<_T(", ")<<QUITE(IsZero(mmcd.Mcr) ? 0 : mmcd.Mr/mmcd.Mcr)<<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50041, ")<<loc<<_T(", ")<<QUITE(IsZero(mmcd.Mu) ? 0 : mmcd.Mr/mmcd.Mu)<< _T(",15, ")<<gdr<<std::endl;

         const pgsFlexuralCapacityArtifact* pCompositeCap;
         pCompositeCap = gdrArtifact->GetPositiveMomentFlexuralCapacityArtifact(pgsFlexuralCapacityArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,poi.GetDistFromStart()));
         if ( pCompositeCap )
         {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122016, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pCompositeCap->GetDemand() , unitMeasure::NewtonMillimeter)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122017, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pCompositeCap->GetCapacity() , unitMeasure::NewtonMillimeter)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122018, ")<<loc<<_T(", ")<<(int)(pCompositeCap->Passed()?1:0)<<_T(", 15, ")<<gdr<<std::endl;
         }

         // negative moment capacity
         pMomentCapacity->GetCrackingMomentDetails(pgsTypes::BridgeSite3,poi,false,&cmd);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50128, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(cmd.Mcr , unitMeasure::NewtonMillimeter)) <<_T(",15, ")<<gdr<<std::endl;

         pMomentCapacity->GetMinMomentCapacityDetails(pgsTypes::BridgeSite3,poi,false,&mmcd);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50129, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(mmcd.Mr , unitMeasure::NewtonMillimeter)) <<_T(",15, ")<<gdr<<std::endl;

         pMomentCapacity->GetMomentCapacityDetails(pgsTypes::BridgeSite3,poi,false,&mcd);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50130, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(mcd.Mn , unitMeasure::NewtonMillimeter)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50135, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(mcd.c, unitMeasure::Millimeter)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50136, ")<<loc<<_T(", ")<< QUITE(IsZero(mcd.de) ? 0 : mcd.c/mcd.de) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50139, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(mcd.de, unitMeasure::Millimeter)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50140, ")<<loc<<_T(", ")<<QUITE(IsZero(mmcd.Mcr) ? 0 : mmcd.Mr/mmcd.Mcr)<<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50141, ")<<loc<<_T(", ")<<QUITE(IsZero(mmcd.Mu) ? 0 : mmcd.Mr/mmcd.Mu)<< _T(",15, ")<<gdr<<std::endl;

         pCompositeCap = gdrArtifact->GetNegativeMomentFlexuralCapacityArtifact(pgsFlexuralCapacityArtifactKey(pgsTypes::BridgeSite3,pgsTypes::StrengthI,poi.GetDistFromStart()));
         if ( pCompositeCap )
         {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122116, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pCompositeCap->GetDemand() , unitMeasure::NewtonMillimeter)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122117, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pCompositeCap->GetCapacity() , unitMeasure::NewtonMillimeter)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122118, ")<<loc<<_T(", ")<<(int)(pCompositeCap->Passed()?1:0)<<_T(", 15, ")<<gdr<<std::endl;
         }


         // shear capacity
         SHEARCAPACITYDETAILS scd;
         pShearCapacity->GetShearCapacityDetails(pgsTypes::StrengthI, pgsTypes::BridgeSite3, poi,&scd);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50031, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(scd.Aps , unitMeasure::Millimeter2)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50032, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(scd.As , unitMeasure::Millimeter2)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50033, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pPrestressForce->GetStrandStress(poi,pgsTypes::Permanent,pgsTypes::BeforeTemporaryStrandRemoval), unitMeasure::MPa)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50038, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(scd.As,unitMeasure::Millimeter2)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50042, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(scd.pVn , unitMeasure::Newton)) <<    _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50043, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(scd.Vn , unitMeasure::Newton)) <<     _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50044, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(scd.Vc , unitMeasure::Newton)) <<     _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50045, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(scd.Vs , unitMeasure::Newton)) <<     _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50046, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(scd.Vp , unitMeasure::Newton)) <<     _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50047, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(scd.bv, unitMeasure::Millimeter)) <<  _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50048, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(scd.Aps, unitMeasure::Millimeter2)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50049, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(scd.As, unitMeasure::Millimeter2)) << _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50050, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(scd.Ac, unitMeasure::Millimeter2)) << _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50051, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(scd.dv, unitMeasure::Millimeter)) <<  _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50053, ")<<loc<<_T(", ")<< QUITE(scd.Beta) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50054, ")<<loc<<_T(", ")<< QUITE(scd.ex) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50055, ")<<loc<<_T(", ")<< QUITE(scd.Fe) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50056, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(scd.Theta, unitMeasure::Degree)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50057, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(scd.fpo, unitMeasure::MPa)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50058, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(scd.fpe, unitMeasure::MPa)) <<_T(",15, ")<<gdr<<std::endl;

         psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(pgsTypes::BridgeSite3, pgsTypes::StrengthI,poi.GetDistFromStart()) );
         ATLASSERT(psArtifact != NULL);
         if ( psArtifact )
         {
         pArtifact = psArtifact->GetLongReinfShearArtifact();

         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50061, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pArtifact->GetDemandForce(), unitMeasure::Newton)) <<  _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50062, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pArtifact->GetCapacityForce(), unitMeasure::Newton)) <<_T(",15, ")<<gdr<<std::endl;

         pSDArtifact = psArtifact->GetStirrupDetailArtifact();
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50063, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSDArtifact->GetSMax(), unitMeasure::Millimeter)) <<   _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50064, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSDArtifact->GetAvsMin(), unitMeasure::Millimeter2)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50065, ")<<loc<<_T(", ")<< QUITE(scd.Vn/scd.Vu) <<_T(", 15 ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100205, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSDArtifact->GetS(), unitMeasure::Millimeter)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100206, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSDArtifact->GetVuLimit(), unitMeasure::Newton)) <<_T(",15, ")<<gdr<<std::endl;
         }

         const pgsStirrupCheckAtPoisArtifact* pspArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifact( pgsStirrupCheckAtPoisArtifactKey(pgsTypes::BridgeSite3, pgsTypes::StrengthI,poi.GetDistFromStart()) );
         if ( pspArtifact )
         {
         pAHsrtifact = pspArtifact->GetHorizontalShearArtifact();
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50067, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pAHsrtifact->GetCapacity()*pAHsrtifact->GetPhi(), unitMeasure::Newton)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50068, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pAHsrtifact->GetCapacity(), unitMeasure::Newton)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100207, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pAHsrtifact->GetDemand(), unitMeasure::Newton)) <<_T(",15, ")<<gdr<<std::endl;
 

         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50069, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pAHsrtifact->GetAcv(), unitMeasure::Millimeter2)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50070, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pAHsrtifact->GetAvOverS(), unitMeasure::Millimeter2)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50071, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pAHsrtifact->GetNormalCompressionForce(), unitMeasure::Newton)) <<_T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100208, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pAHsrtifact->GetSMax(), unitMeasure::Millimeter)) <<   _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100209, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pAHsrtifact->GetAvOverSReqd(), unitMeasure::Millimeter)) <<   _T(",15, ")<<gdr<<std::endl;
         }

         if ( psArtifact )
         {
         const pgsVerticalShearArtifact* pVertical = psArtifact->GetVerticalShearArtifact();
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100210, ")<<loc<<_T(", ")<<(int)(pVertical->IsStrutAndTieRequired(pgsTypes::metStart)?1:0)<<_T(", 15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100211, ")<<loc<<_T(", ")<<(int)(pVertical->IsStrutAndTieRequired(pgsTypes::metEnd)?1:0)<<_T(", 15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100212, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pVertical->GetAvOverSReqd(), unitMeasure::Millimeter)) <<   _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100213, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pVertical->GetDemand(), unitMeasure::Newton)) <<  _T(",15, ")<<gdr<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100214, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pVertical->GetCapacity(), unitMeasure::Newton)) <<  _T(",15, ")<<gdr<<std::endl;
         }
      }

      // pass/fail cases
/*      if (pArtifact->IsApplicable())*/
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100202, ")<<loc<<_T(", ")<<(int)(pArtifact->Passed()?1:0)<<_T(", 15, ")<<gdr<<std::endl;

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100203, ")<<loc<<_T(", ")<<(int)(pSDArtifact->Passed()?1:0)<<_T(", 15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100204, ")<<loc<<_T(", ")<<(int)(pAHsrtifact->Passed()?1:0)<<_T(", 15, ")<<gdr<<std::endl;

      if ( poi.IsFlexureStress(pgsTypes::BridgeSite1) && poi.IsFlexureStress(pgsTypes::BridgeSite2) && poi.IsFlexureStress(pgsTypes::BridgeSite3) )
      {
         const pgsFlexuralStressArtifact* pStresses;
         pStresses = gdrArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite1,pgsTypes::ServiceI,pgsTypes::Tension,poi.GetDistFromStart()));
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122019, ")<<loc<<_T(", ")<<(int)(pStresses->Passed(pgsFlexuralStressArtifact::WithoutRebar)?1:0)<<_T(", 15, ")<<gdr<<std::endl;
         pStresses = gdrArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite3,pgsTypes::ServiceIII,pgsTypes::Tension,poi.GetDistFromStart()));
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122023, ")<<loc<<_T(", ")<<(int)(pStresses->TopPassed(pgsFlexuralStressArtifact::WithoutRebar)?1:0)<<_T(", 15, ")<<gdr<<std::endl;
         pStresses = gdrArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite1,pgsTypes::ServiceI,pgsTypes::Compression,poi.GetDistFromStart()));
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122024, ")<<loc<<_T(", ")<<(int)(pStresses->Passed(pgsFlexuralStressArtifact::WithoutRebar)?1:0)<<_T(", 15, ")<<gdr<<std::endl;
         pStresses = gdrArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite2,pgsTypes::ServiceI,pgsTypes::Compression,poi.GetDistFromStart()));
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122025, ")<<loc<<_T(", ")<<(int)(pStresses->Passed(pgsFlexuralStressArtifact::WithoutRebar)?1:0)<<_T(", 15, ")<<gdr<<std::endl;
         pStresses = gdrArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite3,pgsTypes::ServiceI,pgsTypes::Compression,poi.GetDistFromStart()));
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122026, ")<<loc<<_T(", ")<<(int)(pStresses->Passed(pgsFlexuralStressArtifact::WithoutRebar)?1:0)<<_T(", 15, ")<<gdr<<std::endl;
         if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
         {
            pStresses = gdrArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite3,pgsTypes::ServiceIA,pgsTypes::Compression,poi.GetDistFromStart()));
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122027, ")<<loc<<_T(", ")<<(int)(pStresses->Passed(pgsFlexuralStressArtifact::WithoutRebar)?1:0)<<_T(", 15, ")<<gdr<<std::endl;
         }
         else
         {
            pStresses = gdrArtifact->GetFlexuralStressArtifact(pgsFlexuralStressArtifactKey(pgsTypes::BridgeSite3,pgsTypes::FatigueI,pgsTypes::Compression,poi.GetDistFromStart()));
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122027, ")<<loc<<_T(", ")<<(int)(pStresses->Passed(pgsFlexuralStressArtifact::WithoutRebar)?1:0)<<_T(", 15, ")<<gdr<<std::endl;
         }
      }
   }

   // confinement
   const pgsConfinementArtifact& rconf = pstirrup_artifact->GetConfinementArtifact();
   if (rconf.IsApplicable())
   {
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100220, ")<<loc<<_T(", ")<<(int)(rconf.Passed()?1:0)<<_T(", 15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100221, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rconf.GetSMax(), unitMeasure::Millimeter)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100222, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rconf.GetStartProvidedZoneLength(), unitMeasure::Millimeter)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100223, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rconf.GetStartRequiredZoneLength(), unitMeasure::Millimeter)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100224, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rconf.GetStartS(), unitMeasure::Millimeter)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100225, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rconf.GetEndProvidedZoneLength(), unitMeasure::Millimeter)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100226, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rconf.GetEndRequiredZoneLength(), unitMeasure::Millimeter)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100227, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(rconf.GetEndS(), unitMeasure::Millimeter)) <<   _T(",15, ")<<gdr<<std::endl;
   }

   // splitting / bursting
   const pgsSplittingZoneArtifact* pSplit = pstirrup_artifact->GetSplittingZoneArtifact();
   if(pSplit->GetIsApplicable())
   {
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100230, ")<<loc<<_T(", ")<<(int)(pSplit->Passed()?1:0)<<_T(", 15, ")<<gdr<<std::endl;

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100231, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetStartAps(), unitMeasure::Millimeter2)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100232, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetStartAvs(), unitMeasure::Millimeter)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100233, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetStartFpj(), unitMeasure::Newton)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100234, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetStartFs(), unitMeasure::MPa)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100235, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetStartH(), unitMeasure::Millimeter)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100236, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetStartLossesAfterTransfer(), unitMeasure::MPa)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100237, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetStartSplittingForce(), unitMeasure::Newton)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100238, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetStartSplittingResistance(), unitMeasure::Newton)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100239, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetStartSplittingZoneLength(), unitMeasure::Millimeter)) <<   _T(",15, ")<<gdr<<std::endl;

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100241, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetEndAps(), unitMeasure::Millimeter2)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100242, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetEndAvs(), unitMeasure::Millimeter)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100243, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetEndFpj(), unitMeasure::Newton)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100244, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetEndFs(), unitMeasure::MPa)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100245, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetEndH(), unitMeasure::Millimeter)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100246, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetEndLossesAfterTransfer(), unitMeasure::MPa)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100247, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetEndSplittingForce(), unitMeasure::Newton)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100248, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetEndSplittingResistance(), unitMeasure::Newton)) <<   _T(",15, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100249, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSplit->GetEndSplittingZoneLength(), unitMeasure::Millimeter)) <<   _T(",15, ")<<gdr<<std::endl;
   }

   return true;
}

bool CTestAgentImp::RunHandlingTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, SpanIndexType span)
{
   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   // write to poi file
   poiFile<<_T(" 1, ")<< bridgeId<< _T(", 3, 1, 0.0000, 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0")<<std::endl;

   GirderIndexType gdr  = 0;

   // lifting
   GET_IFACE(IArtifact,pArtifacts);
   const pgsGirderArtifact* pArtifact = pArtifacts->GetArtifact(span,gdr);
   const pgsLiftingCheckArtifact* pLiftArtifact = pArtifact->GetLiftingCheckArtifact();
   if (pLiftArtifact != NULL)
   {
      if (!pLiftArtifact->IsGirderStable())
      {
         resultsFile<<_T("Girder is unstable for lifting")<<std::endl;
         return false;
      }

      GET_IFACE(IGirderLiftingPointsOfInterest,pGirderLiftingPointsOfInterest);
      std::vector<pgsPointOfInterest> poi_vec;
      poi_vec = pGirderLiftingPointsOfInterest->GetLiftingPointsOfInterest(span,gdr,POI_MIDSPAN);
      CHECK(poi_vec.size()==1);
      pgsPointOfInterest& poi = poi_vec[0];

      Float64 loc = poi.GetDistFromStart();

      pgsLiftingStressCheckArtifact stressArtifact = pLiftArtifact->GetLiftingStressCheckArtifact(poi.GetDistFromStart());

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100001, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(stressArtifact.GetMaximumConcreteTensileStress() , unitMeasure::MPa)) <<_T(", 50, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100002, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(stressArtifact.GetMaximumConcreteCompressiveStress() , unitMeasure::MPa)) <<_T(", 50, ")<<gdr<<std::endl;

      pgsLiftingCrackingCheckArtifact crackArtifact =  pLiftArtifact->GetLiftingCrackingCheckArtifact(poi.GetDistFromStart());

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100003, ")<<loc<<_T(", ")<<crackArtifact.GetFsCracking()<<_T(", 50, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100004, ")<<loc<<_T(", ")<<pLiftArtifact->GetFsFailure()<<_T(", 50, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100009, ")<<loc<<_T(", ")<<(int)(pLiftArtifact->Passed()?1:0)<<_T(", 50, ")<<gdr<<std::endl;
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

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100005, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(hStress.GetMaximumConcreteTensileStress() , unitMeasure::MPa)) <<_T(", 50, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100006, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(hStress.GetMaximumConcreteCompressiveStress() , unitMeasure::MPa)) <<_T(", 50, ")<<gdr<<std::endl;

      pgsHaulingCrackingCheckArtifact hCrack =  pHaulArtifact->GetHaulingCrackingCheckArtifact(poi.GetDistFromStart());

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100007, ")<<loc<<_T(", ")<< hCrack.GetFsCracking()<<_T(", 50, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100008, ")<<loc<<_T(", ")<< pHaulArtifact->GetFsRollover()<<_T(", 50, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100010, ")<<loc<<_T(", ")<<(int)(pHaulArtifact->Passed()?1:0)<<_T(", 50, ")<<gdr<<std::endl;
   }

   return true;
}

bool CTestAgentImp::RunWsdotGirderScheduleTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   GET_IFACE(IArtifact,pIArtifact);
   const pgsGirderArtifact* pArtifact = pIArtifact->GetArtifact(span,gdr);

   GET_IFACE(IArtifact,pArtifacts);
   const pgsGirderArtifact* pGdrArtifact = pArtifacts->GetArtifact(span,gdr);
   const pgsConstructabilityArtifact* pConstArtifact = pGdrArtifact->GetConstructabilityArtifact();

   GET_IFACE(ICamber,pCamber);
   // create pois at the start of girder and mid-span
   pgsPointOfInterest pois(span,gdr,0.0);

   GET_IFACE(IPointOfInterest, pPointOfInterest );
   std::vector<pgsPointOfInterest> pmid = pPointOfInterest->GetPointsOfInterest( span, gdr, pgsTypes::BridgeSite1,POI_MIDSPAN);
   CHECK(pmid.size()==1);

   Float64 loc = pmid[0].GetDistFromStart();

   GET_IFACE(IBridge, pBridge );
   CComPtr<IAngle> as1;
   pBridge->GetPierSkew(span,&as1);
   Float64 s1;
   as1->get_Value(&s1);
   Float64 t1 = s1 + M_PI/2.0;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123001, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(t1 , unitMeasure::Degree)) <<_T(", 101, ")<<gdr<<std::endl;

   CComPtr<IAngle> as2;
   pBridge->GetPierSkew(span+1,&as2);
   Float64 s2;
   as2->get_Value(&s2);
   Float64 t2 = s2 + M_PI/2.0;

   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123002, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(t2 , unitMeasure::Degree)) <<_T(", 101, ")<<gdr<<std::endl;

   GET_IFACE(ILibrary, pLib );

   std::_tstring start_connection = pBridge->GetRightSidePierConnection(span);
   std::_tstring end_connection = pBridge->GetLeftSidePierConnection(span+1);
   const ConnectionLibraryEntry* pConnEntry = pLib->GetConnectionEntry( start_connection.c_str() );
   Float64 N1 = pConnEntry->GetGirderEndDistance();

   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123003, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(N1, unitMeasure::Millimeter)) <<   _T(", 101, ")<<gdr<<std::endl;

   pConnEntry = pLib->GetConnectionEntry( end_connection.c_str() );
   Float64 N2 = pConnEntry->GetGirderEndDistance();

   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123004, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(N2, unitMeasure::Millimeter)) <<   _T(", 101, ")<<gdr<<std::endl;

   Float64 P1 = N1 / sin(t1);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123005, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(P1, unitMeasure::Millimeter)) <<   _T(", 101, ")<<gdr<<std::endl;

   Float64 P2 = N2 / sin(t2);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123006, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(P2, unitMeasure::Millimeter)) <<   _T(", 101, ")<<gdr<<std::endl;

   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123007, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pBridge->GetGirderLength(span,gdr), unitMeasure::Millimeter)) <<   _T(", 101, ")<<gdr<<std::endl;

   GET_IFACE(IBridgeMaterial, pMaterial);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123008, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pMaterial->GetFcGdr(span,gdr), unitMeasure::MPa)) <<   _T(", 101, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123009, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pMaterial->GetFciGdr(span,gdr), unitMeasure::MPa)) <<   _T(", 101, ")<<gdr<<std::endl;

   GET_IFACE(IStrandGeometry, pStrandGeometry );
   
   StrandIndexType nh = pStrandGeometry->GetNumStrands(span,gdr,pgsTypes::Harped);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123010, ")<<loc<<_T(", ")<<nh<<   _T(", 101, ")<<gdr<<std::endl;

   Float64 hj = pStrandGeometry->GetPjack(span,gdr,pgsTypes::Harped);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123011, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(hj, unitMeasure::Newton)) <<   _T(", 101, ")<<gdr<<std::endl;

   StrandIndexType ns = pStrandGeometry->GetNumStrands(span,gdr,pgsTypes::Straight);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123012, ")<<loc<<_T(", ")<<ns<<   _T(", 101, ")<<gdr<<std::endl;

   Float64 sj = pStrandGeometry->GetPjack(span,gdr,pgsTypes::Straight);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123013, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(sj, unitMeasure::Newton)) <<   _T(", 101, ")<<gdr<<std::endl;

   StrandIndexType nt = pStrandGeometry->GetNumStrands(span,gdr,pgsTypes::Temporary);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123014, ")<<loc<<_T(", ")<<nt<<   _T(", 101, ")<<gdr<<std::endl;

   Float64 tj = pStrandGeometry->GetPjack(span,gdr,pgsTypes::Temporary);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123015, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(tj, unitMeasure::Newton)) <<   _T(", 101, ")<<gdr<<std::endl;

   GET_IFACE(ISectProp2, pSectProp2 );
   Float64 ybg = pSectProp2->GetYb(pgsTypes::CastingYard,pois);
   Float64 nEff;
   Float64 sse = pStrandGeometry->GetSsEccentricity(pois, &nEff);
   if (0 < ns)
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123016, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(ybg-sse, unitMeasure::Millimeter)) <<   _T(", 101, ")<<gdr<<std::endl;

   Float64 hse = pStrandGeometry->GetHsEccentricity(pmid[0], &nEff);
   if (0 < nh)
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123017, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(ybg-hse, unitMeasure::Millimeter)) <<   _T(", 101, ")<<gdr<<std::endl;

   // get location of first harped strand
   if (0 < nh)
   {
      StrandIndexType nns = pStrandGeometry->GetNextNumStrands(span,gdr,pgsTypes::Harped,0);
      ConfigStrandFillVector fillvec = pStrandGeometry->ComputeStrandFill(span, gdr, pgsTypes::Harped, nns);
      GDRCONFIG config = pBridge->GetGirderConfiguration(span,gdr);
      config.PrestressConfig.SetStrandFill(pgsTypes::Harped, fillvec);
      Float64 eh2 = pStrandGeometry->GetHsEccentricity( pmid[0], config.PrestressConfig, &nEff );
      Float64 Fb  = pSectProp2->GetYb(pgsTypes::CastingYard,pois) - eh2;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123018, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(Fb, unitMeasure::Millimeter)) <<   _T(", 101, ")<<gdr<<std::endl;
   }

   Float64 ytg = pSectProp2->GetYtGirder(pgsTypes::CastingYard,pois);
   Float64 hss = pStrandGeometry->GetHsEccentricity(pois, &nEff);
   if (0 < nh)
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123019, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(ytg+hss, unitMeasure::Millimeter)) <<   _T(", 101, ")<<gdr<<std::endl;

   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123020, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pCamber->GetScreedCamber( pmid[0] ), unitMeasure::Millimeter)) <<   _T(", 101, ")<<gdr<<std::endl;

   // get # of days for creep
   GET_IFACE(ISpecification, pSpec );
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   Float64 days =  ::ConvertFromSysUnits(pSpecEntry->GetCreepDuration1Min(), unitMeasure::Day);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 123021, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pSpecEntry->GetCreepDuration1Min(), unitMeasure::Day)) <<   _T(", 101, ")<<gdr<<std::endl;



   return true;
}


bool CTestAgentImp::RunDesignTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   Float64 loc = 0.0;

   // get design options from spec. Do shear and flexure
   GET_IFACE(ISpecification,pSpecification);
   arDesignOptions des_options = pSpecification->GetDesignOptions(span,gdr);

   des_options.doDesignForShear = true;
   des_options.doDesignStirrupLayout = slLayoutStirrups; // always layout zones from scratch

   GET_IFACE(IArtifact,pIArtifact);
   const pgsDesignArtifact* pArtifact;

   try
   {
      pArtifact = pIArtifact->CreateDesignArtifact( span,gdr, des_options);
   }
   catch(...)
   {
      resultsFile << _T("Design Failed for span ") << span << _T(" girder ") << gdr << std::endl;
      return false;
   }

   if ( pArtifact == NULL )
   {
      resultsFile << _T("Design was cancelled for span ") << span << _T(" girder ") << gdr << std::endl;
      return false;
   }

   if ( pArtifact->GetOutcome() != pgsDesignArtifact::Success )
   {
      resultsFile << _T("Design not successful for span ") << span << _T(" girder ") << gdr << std::endl;
      return false;
   }

   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124001, ")<<loc<<_T(", ")<<pArtifact->GetOutcome()<<   _T(", 102, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124002, ")<<loc<<_T(", ")<<pArtifact->GetNumStraightStrands()<<   _T(", 102, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124003, ")<<loc<<_T(", ")<<pArtifact->GetNumTempStrands()<<   _T(", 102, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124004, ")<<loc<<_T(", ")<<pArtifact->GetNumHarpedStrands()<<   _T(", 102, ")<<gdr<<std::endl;

   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124005, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pArtifact->GetPjackStraightStrands(), unitMeasure::Newton)) <<   _T(", 102, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124006, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pArtifact->GetPjackTempStrands(), unitMeasure::Newton)) <<   _T(", 102, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124007, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pArtifact->GetPjackHarpedStrands(), unitMeasure::Newton)) <<   _T(", 102, ")<<gdr<<std::endl;

   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124008, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pArtifact->GetHarpStrandOffsetHp(), unitMeasure::Millimeter)) <<   _T(", 102, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124009, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pArtifact->GetHarpStrandOffsetEnd(), unitMeasure::Millimeter)) <<   _T(", 102, ")<<gdr<<std::endl;

   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124010, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pArtifact->GetReleaseStrength(), unitMeasure::MPa)) <<   _T(", 102, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124011, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pArtifact->GetConcrete().GetFc(), unitMeasure::MPa)) <<   _T(", 102, ")<<gdr<<std::endl;

   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124012s, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pArtifact->GetSlabOffset(pgsTypes::metStart), unitMeasure::Millimeter)) <<   _T(", 102, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124012e, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pArtifact->GetSlabOffset(pgsTypes::metEnd), unitMeasure::Millimeter)) <<   _T(", 102, ")<<gdr<<std::endl;

#pragma Reminder("UPDATE: Designing for symmetrical lift and haul points")
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124013, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pArtifact->GetLeftLiftingLocation(), unitMeasure::Millimeter)) <<   _T(", 102, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124014, ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(pArtifact->GetLeadingOverhang(), unitMeasure::Millimeter)) <<   _T(", 102, ")<<gdr<<std::endl;

   const CShearData& sd = pArtifact->GetShearData();

   int ncz = -1;
   matRebar::Size czsize(matRebar::bsNone);
   for (CShearData::ShearZoneConstIterator czit = sd.ShearZones.begin(); czit != sd.ShearZones.end(); czit++)
   {
      if (czit->ConfinementBarSize!=matRebar::bsNone)
      {
         czsize = czit->ConfinementBarSize;
         ncz++;
      }
      else
         break;
   }

   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124015, ")<<loc<<_T(", ")<<GetBarSize(czsize)<<   _T(", 102, ")<<gdr<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124016, ")<<loc<<_T(", ")<< ncz <<   _T(", 102, ")<<gdr<<std::endl;

   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124017, ")<<loc<<_T(", ")<<pArtifact->GetNumberOfStirrupZonesDesigned()<<   _T(", 102, ")<<gdr<<std::endl;

   Int32 id = 124018;
   for (CShearData::ShearZoneConstIterator czit = sd.ShearZones.begin(); czit != sd.ShearZones.end(); czit++)
   {
      const CShearZoneData& zd = *czit;

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<<GetBarSize(zd.VertBarSize)<<   _T(", 102, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(zd.BarSpacing, unitMeasure::Millimeter)) <<   _T(", 102, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<< QUITE(::ConvertFromSysUnits(zd.ZoneLength, unitMeasure::Millimeter)) <<   _T(", 102, ")<<gdr<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<< zd.nVertBars <<   _T(", 102, ")<<gdr<<std::endl;
   }

   return true;
}

bool CTestAgentImp::RunCamberTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   GET_IFACE(IPointOfInterest,pPointsOfInterest);
   std::vector<pgsPointOfInterest> poi_vec;
   poi_vec = pPointsOfInterest->GetPointsOfInterest(span,gdr,pgsTypes::BridgeSite3,POI_MIDSPAN);
   CHECK(poi_vec.size()==1);
   pgsPointOfInterest& poi_midspan = poi_vec[0];

   GET_IFACE( ICamber, pCamber );
   Float64 D40  = pCamber->GetDCamberForGirderSchedule(poi_midspan,CREEP_MINTIME);
   Float64 D120 = pCamber->GetDCamberForGirderSchedule(poi_midspan,CREEP_MAXTIME);

   resultsFile << bridgeId << _T(", ") << pid << _T(", 125000, ") << QUITE(::ConvertFromSysUnits(D40,  unitMeasure::Millimeter)) << _T(", ") << gdr << std::endl;
   resultsFile << bridgeId << _T(", ") << pid << _T(", 125001, ") << QUITE(::ConvertFromSysUnits(D120, unitMeasure::Millimeter)) << _T(", ") << gdr << std::endl;

   return true;
}


bool CTestAgentImp::RunFabOptimizationTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, SpanIndexType span,GirderIndexType gdr)
{
   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

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

	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155000, ") << ::ConvertFromSysUnits(details.Fci[NO_TTS],unitMeasure::MPa) << _T(", ") << gdr << std::endl;
	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155001, ") << ::ConvertFromSysUnits(details.L[NO_TTS],unitMeasure::Millimeter) << _T(", ") << gdr << std::endl;

	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155002, ") << ::ConvertFromSysUnits(details.Fci[PS_TTS],unitMeasure::MPa) << _T(", ") << gdr << std::endl;
	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155003, ") << ::ConvertFromSysUnits(details.L[PS_TTS],unitMeasure::Millimeter) << _T(", ") << gdr << std::endl;

	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155004, ") << ::ConvertFromSysUnits(details.Fci[PT_TTS_OPTIONAL],unitMeasure::MPa) << _T(", ") << gdr << std::endl;
	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155005, ") << ::ConvertFromSysUnits(details.L[PT_TTS_OPTIONAL],unitMeasure::Millimeter) << _T(", ") << gdr << std::endl;

	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155006, ") << ::ConvertFromSysUnits(details.Fci[PT_TTS_REQUIRED],unitMeasure::MPa) << _T(", ") << gdr << std::endl;
	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155007, ") << ::ConvertFromSysUnits(details.L[PT_TTS_REQUIRED],unitMeasure::Millimeter) << _T(", ") << gdr << std::endl;

	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155008, ") << ::ConvertFromSysUnits(details.Fci_FormStripping_WithoutTTS,unitMeasure::MPa) << _T(", ") << gdr << std::endl;

	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155009, ") << ::ConvertFromSysUnits(details.Lmin,unitMeasure::Millimeter) << _T(", ") << gdr << std::endl;
	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155010, ") << ::ConvertFromSysUnits(details.Lmax,unitMeasure::Millimeter) << _T(", ") << gdr << std::endl;

	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155011, ") << ::ConvertFromSysUnits(details.LUmin,unitMeasure::Millimeter) << _T(", ") << gdr << std::endl;
	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155012, ") << ::ConvertFromSysUnits(details.LUmax,unitMeasure::Millimeter) << _T(", ") << gdr << std::endl;
	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155013, ") << ::ConvertFromSysUnits(details.LUsum,unitMeasure::Millimeter) << _T(", ") << gdr << std::endl;
   }

   return true;
}

bool CTestAgentImp::RunLoadRatingTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, GirderIndexType gdr)
{
   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   GET_IFACE(IProductLoads,pProductLoads);
   GET_IFACE(IArtifact,pArtifacts);

   GET_IFACE(IBridge,pBridge);
   bool bNegMoments = pBridge->ProcessNegativeMoments(ALL_SPANS);
   
   GET_IFACE(IRatingSpecification,pRatingSpec);


   for ( int i = 0; i < 6; i++ )
   {
      pgsTypes::LoadRatingType ratingType = (pgsTypes::LoadRatingType)i;
      if ( !pRatingSpec->IsRatingEnabled(ratingType) )
         continue;

      pgsTypes::LiveLoadType llType = ::GetLiveLoadType(ratingType);
      VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
      for ( VehicleIndexType vehIdx = 0; vehIdx < nVehicles; vehIdx++ )
      {
         const pgsRatingArtifact* pArtifact = pArtifacts->GetRatingArtifact(gdr,ratingType,vehIdx);
         if ( pArtifact == NULL )
            continue;

         Float64 RF = pArtifact->GetMomentRatingFactor(true);
	      resultsFile << bridgeId << _T(", ") << pid << _T(", 881001, ") << QUITE(RF) << _T(", ") << gdr << std::endl;

         if ( bNegMoments )
         {
            RF = pArtifact->GetMomentRatingFactor(false);
	         resultsFile << bridgeId << _T(", ") << pid << _T(", 881002, ") << QUITE(RF) << _T(", ") << gdr << std::endl;
         }
         
         if ( pRatingSpec->RateForShear(ratingType) )
         {
            RF = pArtifact->GetShearRatingFactor();
	         resultsFile << bridgeId << _T(", ") << pid << _T(", 881003, ") << QUITE(RF) << _T(", ") << gdr << std::endl;
         }

         if ( pRatingSpec->RateForStress(ratingType) )
         {
            if ( ratingType == pgsTypes::lrPermit_Routine || ratingType == pgsTypes::lrPermit_Special )
            {
               // Service I reinforcement yield check if permit rating
               RF = pArtifact->GetYieldStressRatio(true);
	            resultsFile << bridgeId << _T(", ") << pid << _T(", 881004, ") << QUITE(RF) << _T(", ") << gdr << std::endl;

               if ( bNegMoments )
               {
                  RF = pArtifact->GetYieldStressRatio(false);
	               resultsFile << bridgeId << _T(", ") << pid << _T(", 881005, ") << QUITE(RF) << _T(", ") << gdr << std::endl;
               }
            }
            else
            {
               // Service III flexure if other rating type
               RF = pArtifact->GetStressRatingFactor();
	            resultsFile << bridgeId << _T(", ") << pid << _T(", 881006, ") << QUITE(RF) << _T(", ") << gdr << std::endl;
            }
         }

         RF = pArtifact->GetRatingFactor();
	      resultsFile << bridgeId << _T(", ") << pid << _T(", 881007, ") << QUITE(RF) << _T(", ") << gdr << std::endl;
      }
   }

   return true;
}
