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

// TestAgentImp.cpp : Implementation of CTestAgentImp
#include "stdafx.h"
#include "TestAgent.h"
#include "TestAgentImp.h"

#include <IFace\Alignment.h>
#include <IFace\VersionInfo.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Bridge.h>
#include <IFace\PrestressForce.h>
#include <IFace\MomentCapacity.h>
#include <IFace\ShearCapacity.h>
#include <IFace\Artifact.h>
#include <IFace\Constructability.h>
#include <IFace\PointOfInterest.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <IFace\StatusCenter.h>
#include <IFace\RatingSpecification.h>
#include <EAF\EAFUIIntegration.h>
#include <IFace\Intervals.h>
#include <IFace\PrincipalWebStress.h>
#include <IFace\DocumentType.h>

#include <psgLib\ConnectionLibraryEntry.h>
#include <psgLib\SpecLibraryEntry.h>
#include <PsgLib\GirderLibraryEntry.h>

#include <PgsExt\DeckDescription2.h>
#include <PgsExt\GirderGroupData.h>
#include <PgsExt\SplicedGirderData.h>
#include <PgsExt\PrecastSegmentData.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\GirderArtifact.h>
#include <PgsExt\GirderDesignArtifact.h>
#include <PgsExt\HaulingAnalysisArtifact.h>
#include <PgsExt\RatingArtifact.h>
#include <EAF\EAFAutoProgress.h>
#include <EAF\EAFApp.h>
#include <PgsExt\GirderLabel.h>
#include <PgsExt\Helpers.h>

#include <Units\Units.h>

#include <IFace\TestFileExport.h>
#include "TestFileWriter.h"

#include <algorithm>
#include <MfcTools\XUnwind.h>

#include <System\AutoVariable.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#define QUIET(_x_) (IsZero(_x_,0.00005) ? 0 : _x_)
#define QUIET(_x_) RoundOff(_x_,0.01)
#define DEFLECTION(_x_) RoundOff(_x_,1.0)
#define GIRDER(_girderKey_) _girderKey_.groupIndex << _T(", ") << _girderKey_.girderIndex
#define SEGMENT(_segmentKey_) _segmentKey_.groupIndex << _T(", ") << _segmentKey_.girderIndex <<_T(", ") << _segmentKey_.segmentIndex

/////////////////////////////////////////////////////////////////////////////
// CTestAgentImp
int GetBarSize(WBFL::Materials::Rebar::Size size)
{
   switch(size)
   {
   case WBFL::Materials::Rebar::Size::bs3: return 3;
   case WBFL::Materials::Rebar::Size::bs4: return 4;
   case WBFL::Materials::Rebar::Size::bs5: return 5;
   case WBFL::Materials::Rebar::Size::bs6: return 6;
   case WBFL::Materials::Rebar::Size::bs7: return 7;
   case WBFL::Materials::Rebar::Size::bs8: return 8;
   case WBFL::Materials::Rebar::Size::bs9: return 9;
   case WBFL::Materials::Rebar::Size::bs10:return 10;
   case WBFL::Materials::Rebar::Size::bs11:return 11;
   case WBFL::Materials::Rebar::Size::bs14:return 14;
   case WBFL::Materials::Rebar::Size::bs18:return 18;
   }

   return -1;
}

/////////////////////////////////////////////////////////////////////////////
// IAgent
//
STDMETHODIMP CTestAgentImp::SetBroker(IBroker* pBroker)
{
   EAF_AGENT_SET_BROKER(pBroker);
   return S_OK;
}

STDMETHODIMP CTestAgentImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   pBrokerInit->RegInterface( IID_ITest1250,     this );
   pBrokerInit->RegInterface( IID_ITestFileExport, this );

   return S_OK;
};

STDMETHODIMP CTestAgentImp::Init()
{
   EAF_AGENT_INIT;

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
   EAF_AGENT_CLEAR_INTERFACE_CACHE;
   return S_OK;
}

// ITest1250
bool CTestAgentImp::RunTest(long type,
                            const std::_tstring& outputFileName,
                            const std::_tstring& poiFileName)
{
   WBFL::System::AutoVariable<bool> bIsTesting(&m_bIsTesting, true);

   // use run unit tests with numeric labeling
   pgsAutoGirderLabel auto_label;
   pgsAutoPierLabel auto_plabel;

   // turn off diagnostics
   DIAG_WARNPOPUP(FALSE);

   // open poi file and results file
   std::_tofstream resf;
   resf.open(outputFileName.c_str());

   std::_tofstream poif;
   poif.open(poiFileName.c_str());
   if (!resf || !poif)
   {
      return false;
   }

   // create progress window
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   if (type == RUN_REGRESSION)
   {
      RunAlignmentTest(resf);
   }

   GET_IFACE(IDocumentType, pDocType);
   GET_IFACE(IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,0));

      for ( SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++ )
      {
         CSegmentKey extSegmentKey(grpIdx,0,segIdx);
         CSegmentKey intSegmentKey(grpIdx,nGirders == 1 ? 0 : 1,segIdx);

         // run tests
         switch (type)
         {
         case 2:
            if( RunDistFactorTest(resf, poif, extSegmentKey))
            {
               return RunDistFactorTest(resf, poif, intSegmentKey);
            }
            break;
         case 4:
            if (RunCrossSectionTest(resf, poif, extSegmentKey) )
            {
               return RunCrossSectionTest(resf, poif, intSegmentKey);
            }
            else
            {
               return false; 
            }
            break;
         case 6:
            if (RunDeadLoadActionTest(resf, poif, extSegmentKey))
            {
               return RunDeadLoadActionTest(resf, poif, intSegmentKey);
            }
            else
            {
               return false;
            }
            break;
         case 7:
            if (RunHL93Test(resf, poif, extSegmentKey))
            {
               return RunHL93Test(resf, poif, intSegmentKey);
            }
            else
            {
               return false;
            }
            break;
         case 8:
            if (RunCombinedLoadActionTest(resf, poif, extSegmentKey))
            {
               return RunCombinedLoadActionTest(resf, poif, intSegmentKey);
            }
            else
            {
               return false;
            }
            break;
         case 15:
         case 18:
            if (RunPrestressedISectionTest(resf, poif, extSegmentKey))
            {
               return RunPrestressedISectionTest(resf, poif, intSegmentKey);
            }
            else
            {
               return false;
            }
            break;
         case 50:
            return RunHandlingTest(resf, poif, extSegmentKey);
            break;
         case 55:
            if ( RunGeometryTest(resf, poif, extSegmentKey) )
            {
               return RunGeometryTest(resf, poif, intSegmentKey);
            }
            else
            {
               return false;
            }
            break;
         case 60:
            if ( RunHaunchTest(resf, poif, extSegmentKey) )
            {
               return RunHaunchTest(resf, poif, intSegmentKey);
            }
            else
            {
               return false;
            }
            break;
         case 499:
            if ( RunCamberTest(resf, poif, extSegmentKey) )
            {
               return RunCamberTest(resf,poif,intSegmentKey);
            }
            break;
         case 500:
            if ( RunFabOptimizationTest(resf, poif, extSegmentKey) )
            {
               return RunFabOptimizationTest(resf, poif, intSegmentKey);
            }
            break;
         case 501:
            if ( RunLoadRatingTest(resf, poif, extSegmentKey) )
            {
               return RunLoadRatingTest(resf, poif, intSegmentKey);
            }
            break;
         case 777:
            if ( RunDesignTest(resf, poif, extSegmentKey) )
            {
               return RunDesignTest(resf, poif, intSegmentKey);
            }
            break;

         case RUN_REGRESSION:

            VERIFY( RunGeometryTest(resf, poif, extSegmentKey) );
            VERIFY( RunGeometryTest(resf, poif, intSegmentKey) );
            VERIFY( RunDistFactorTest(resf, poif, extSegmentKey) );
            VERIFY( RunDistFactorTest(resf, poif, intSegmentKey) );
            VERIFY( RunCrossSectionTest(resf, poif,  extSegmentKey) );
            VERIFY( RunCrossSectionTest(resf, poif,  intSegmentKey) );
            VERIFY( RunDeadLoadActionTest(resf, poif, extSegmentKey) );
            VERIFY( RunDeadLoadActionTest(resf, poif,  intSegmentKey) );
            VERIFY( RunHL93Test(resf, poif, extSegmentKey) );
            VERIFY( RunHL93Test(resf, poif, intSegmentKey) );
            VERIFY( RunCombinedLoadActionTest(resf, poif, extSegmentKey) );
            VERIFY( RunCombinedLoadActionTest(resf, poif, intSegmentKey) );
            VERIFY( RunPrestressedISectionTest(resf, poif, extSegmentKey) );
            VERIFY( RunPrestressedISectionTest(resf, poif, intSegmentKey) );
            VERIFY( RunHandlingTest(resf, poif, extSegmentKey));
            VERIFY( RunWsdotGirderScheduleTest(resf, poif, extSegmentKey) );
            VERIFY( RunWsdotGirderScheduleTest(resf, poif, intSegmentKey) );
            VERIFY( RunHaunchTest(resf, poif, extSegmentKey) );
            VERIFY( RunHaunchTest(resf, poif, intSegmentKey) );

            if (type==RUN_REGRESSION && pDocType->IsPGSuperDocument()) // only do design for regression - cad test should have already run design
            { // design only applies to PGSuper documents (not spliced girders)
               VERIFY( RunDesignTest(resf, poif, extSegmentKey) );
               VERIFY( RunDesignTest(resf, poif, intSegmentKey) );
            }

            VERIFY( RunCamberTest(resf,poif,extSegmentKey) );
            VERIFY( RunCamberTest(resf,poif,intSegmentKey) );

            VERIFY( RunFabOptimizationTest(resf, poif, extSegmentKey) );
            VERIFY( RunFabOptimizationTest(resf, poif, intSegmentKey) );

            VERIFY( RunLoadRatingTest(resf, poif, extSegmentKey) );
            VERIFY( RunLoadRatingTest(resf, poif, intSegmentKey) );

            std::_tstring testFileName(outputFileName);
            testFileName.replace(testFileName.begin()+testFileName.find(_T(".")),testFileName.end(),_T(".Test"));
        	   FILE	*fp = nullptr;
            if (_tfopen_s(&fp,testFileName.c_str(), _T("w+")) != 0 || fp == nullptr)
            {
			      AfxMessageBox (_T("Warning: File Cannot be Created."));
			      return S_OK;
		      }

            GET_IFACE(ITestFileExport,pTxDOTExport);
            if ( pTxDOTExport )
            {
               pTxDOTExport->WriteCADDataToFile(fp, m_pBroker, extSegmentKey, true);
               pTxDOTExport->WriteCADDataToFile(fp, m_pBroker, intSegmentKey, true);
            }
		      fclose (fp);

            return true;
            break;
         }
      } // next segment
   } // next group

   return false;
}

bool CTestAgentImp::RunTestEx(long type, const std::vector<SpanGirderHashType>& girderList,
                            const std::_tstring& outputFileName,
                            const std::_tstring& poiFileName)
{
   WBFL::System::AutoVariable<bool> bIsTesting(&m_bIsTesting, true);
   pgsAutoGirderLabel auto_label;
   pgsAutoPierLabel auto_plabel;

   // turn off diagnostics
   DIAG_WARNPOPUP(FALSE);

   // open poi file and results file
   std::_tofstream resf;
   resf.open(outputFileName.c_str());

   std::_tofstream poif;
   poif.open(poiFileName.c_str());
   if (!resf || !poif)
   {
      return false;
   }

   // create progress window
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   if (type == RUN_REGRESSION || type == RUN_CADTEST)
   {
      RunAlignmentTest(resf);
   }

   GET_IFACE(IBridge,pBridge);
   SpanIndexType nspans = pBridge->GetSpanCount();

   for(std::vector<SpanGirderHashType>::const_iterator it=girderList.begin(); it!=girderList.end(); it++)
   {
      SpanGirderHashType key = *it;
      SpanIndexType spanIdx;
      GirderIndexType gdrIdx;
      ::UnhashSpanGirder(key,&spanIdx,&gdrIdx);
      CSegmentKey segmentKey(spanIdx,gdrIdx,0);

      // run tests
      switch (type)
      {
      case 2:
         if (!RunDistFactorTest(resf, poif, segmentKey))
         {
            return false;
         }
         break;
      case 4:
         if (!RunCrossSectionTest(resf, poif, segmentKey) )
         {
            return false; 
         }
         break;
      case 6:
         if (!RunDeadLoadActionTest(resf, poif, segmentKey))
         {
            return false;
         }
         break;
      case 7:
         if (!RunHL93Test(resf, poif, segmentKey))
         {
            return false;
         }
         break;
      case 8:
         if (!RunCombinedLoadActionTest(resf, poif, segmentKey))
         {
            return false;
         }
         break;
      case 15:
      case 18:
         if (!RunPrestressedISectionTest(resf, poif, segmentKey))
         {
            return false;
         }
         break;
      case 50:
         return RunHandlingTest(resf, poif, segmentKey);
         break;
      case 55:
         if ( !RunGeometryTest(resf, poif, segmentKey) )
         {
            return false;
         }
         break;
      case 60:
         if ( !RunHaunchTest(resf, poif, segmentKey) )
         {
            return false;
         }
         break;
      case 499:
         if ( !RunCamberTest(resf, poif, segmentKey) )
         {
            return false;
         }
         break;
      case 500:
         if ( !RunFabOptimizationTest(resf, poif, segmentKey) )
         {
            return false;
         }
         break;
      case 501:
         if ( !RunLoadRatingTest(resf, poif, segmentKey) )
         {
            return false;
         }
         break;
      case 777:
         if ( !RunDesignTest(resf, poif, segmentKey) )
         {
            return false;
         }
         break;
      case RUN_GEOMTEST:
         if ( !RunGeometryTest(resf, poif, segmentKey) )
         {
            return false;
         }
         break;

      case RUN_REGRESSION:
      case RUN_CADTEST:
         if (!RunWsdotGirderScheduleTest(resf, poif, segmentKey) )
         {
            return false;
         }
         if ( !RunGeometryTest(resf, poif, segmentKey) )
         {
            return false;
         }
         if (!RunDistFactorTest(resf, poif, segmentKey) )
         {
            return false;
         }
         if (!RunCrossSectionTest(resf, poif,  segmentKey) )
         {
            return false;
         }
         if (!RunDeadLoadActionTest(resf, poif, segmentKey) )
         {
            return false;
         }
         if (!RunHL93Test(resf, poif, segmentKey) )
         {
            return false;
         }
         if (!RunCombinedLoadActionTest(resf, poif, segmentKey) )
         {
            return false;
         }
         if (!RunPrestressedISectionTest(resf, poif, segmentKey) )
         {
            return false;
         }
         if (!RunHandlingTest(resf, poif, segmentKey))
         {
            return false;
         }
         if ( !RunHaunchTest(resf, poif, segmentKey) )
         {
            return false;
         }

         if (type==RUN_REGRESSION) // only do design for regression - cad test should have already run design if requested
         {
            if (!RunDesignTest(resf, poif, segmentKey) )
            {
               return false;
            }
         }

         if (!RunCamberTest(resf,poif,segmentKey) )
         {
            return false;
         }
         if (!RunFabOptimizationTest(resf, poif, segmentKey) )
         {
            return false;
         }

         if (!RunLoadRatingTest(resf, poif, segmentKey) )
         {
            return false;
         }
      }
   }

   return true;
}


bool CTestAgentImp::IsTesting() const
{
   return m_bIsTesting;
}

/////////////////////////////////////////////////////////////////////////////
// IEAFProcessCommandLine
BOOL CTestAgentImp::ProcessCommandLineOptions(CEAFCommandLineInfo & cmdInfo)
{
   // cmdInfo is the command line information from the application. The application
   // doesn't know about this plug-in at the time the command line parameters are parsed
   //
   // Re-parse the parameters with our own command line information object
   CTestCommandLineInfo txCmdInfo;

   EAFGetApp()->ParseCommandLine(txCmdInfo);
   cmdInfo = txCmdInfo;

   if (txCmdInfo.m_DoTxCadReport && !txCmdInfo.m_bError)
   {
      ProcessTestReport(txCmdInfo);
      return TRUE;
   }

   return (txCmdInfo.m_bError ? TRUE : FALSE);
}
   

/////////////////////////////////////////////////////////////////////////////
// ITestFileExport
int CTestAgentImp::WriteCADDataToFile(FILE *fp, IBroker* pBroker, const CSegmentKey& segmentKey, bool designSucceeded)
{
   return Test_WriteCADDataToFile(fp,pBroker,segmentKey, designSucceeded);
}

int CTestAgentImp::WriteDistributionFactorsToFile(FILE *fp, IBroker* pBroker, const CSegmentKey& segmentKey)
{
   return Test_WriteDistributionFactorsToFile(fp,pBroker,segmentKey);
}


std::_tstring CTestAgentImp::GetBridgeID()
{
   static std::_tstring strID;
   static bool bDone = false;

   if ( bDone )
   {
      return strID;
   }
   else
   {
      GET_IFACE(IEAFDocument,pDocument);
      std::_tstring strPath = pDocument->GetFilePath();

      // Filename is in the form Regxxx.pgs
      std::_tstring::size_type pos = strPath.find(_T(".pgs"));
      if ( pos == std::_tstring::npos )
      {
         pos = strPath.find(_T(".spl"));
      }
      IndexType len = strPath.length();
      if ( pos == std::_tstring::npos )
      {
         return std::_tstring(_T("")); // "pgs" was not found
      }

      strID = strPath.substr(pos-3,3); // return the "xxx" number

      bDone = true;
      return strID;
   }
}

std::_tstring CTestAgentImp::GetProcessID()
{
   //// the process ID is going to be the PGSuper version number
   //GET_IFACE(IVersionInfo,pVI);
   //std::_tstring strVersion = pVI->GetVersion(true);
   //return strVersion;

   // we stopped using the version number as the process ID
   // because we never compared results from one process to another
   // as described in the 12-50 process. Since the move to 
   // GIT VCS, merging files is difficult and it is especially difficult
   // with reg test results that have the version number incoded in it
   //
   // now using a "generic" number for the process id
   return std::_tstring(_T("0.0.0.0"));
}

bool CTestAgentImp::RunHaunchTest(std::_tofstream& resultsFile, std::_tofstream& poiFile,const CSegmentKey& segmentKey)
{
   GET_IFACE(ILibrary, pLib );
   GET_IFACE(ISpecification, pSpec );
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   GET_IFACE(IBridge,pBridge);
   if ( pBridge->GetDeckType() == pgsTypes::sdtNone || !pSpecEntry->IsSlabOffsetCheckEnabled() )
   {
      // No data
      return true;
   }

   // Generate data
   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   GET_IFACE(IGirderHaunch,pGdrHaunch);
  
#pragma Reminder("REVIEW: A DIMENSIONS REGRESSION TESTS")
   // haunch data is by girder. for PGSuper there is only one segment per girder
   // so this is ok, but for spliced girders... we may need to re-think this regression test
   // maybe all regression tests should be by girder and not by segment
   const auto& slab_offset_details = pGdrHaunch->GetSlabOffsetDetails(segmentKey);

   for ( const auto& slab_offset : slab_offset_details.SlabOffset)
   {
      const pgsPointOfInterest& rpoi = slab_offset.PointOfInterest;
      Float64 loc = WBFL::Units::ConvertFromSysUnits(rpoi.GetDistFromStart(), WBFL::Units::Measure::Millimeter);

      Float64 camber = WBFL::Units::ConvertFromSysUnits(slab_offset.CamberEffect, WBFL::Units::Measure::Millimeter);
      Float64 orientation = WBFL::Units::ConvertFromSysUnits(slab_offset.GirderOrientationEffect, WBFL::Units::Measure::Millimeter);
      Float64 profile = WBFL::Units::ConvertFromSysUnits(slab_offset.ProfileEffect, WBFL::Units::Measure::Millimeter);
      Float64 required = WBFL::Units::ConvertFromSysUnits(slab_offset.RequiredSlabOffsetRaw, WBFL::Units::Measure::Millimeter);
      Float64 actual = WBFL::Units::ConvertFromSysUnits(slab_offset.TopSlabToTopGirder, WBFL::Units::Measure::Millimeter);

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 89000, ")<<loc<<_T(", ")<< QUIET(camber) <<_T(", 7, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 89001, ")<<loc<<_T(", ")<< QUIET(orientation) <<_T(", 7, ")<< SEGMENT(segmentKey) <<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 89002, ")<<loc<<_T(", ")<< QUIET(profile) <<_T(", 7, ") << SEGMENT(segmentKey) <<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 89003, ")<<loc<<_T(", ")<< QUIET(required) <<_T(", 7, ") << SEGMENT(segmentKey) <<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 89004, ")<<loc<<_T(", ")<< QUIET(actual) <<_T(", 7, ") << SEGMENT(segmentKey) <<std::endl;
   }

   return true;
}

bool CTestAgentImp::RunGeometryTest(std::_tofstream& resultsFile, std::_tofstream& poiFile,const CSegmentKey& segmentKey)
{
   GET_IFACE(IGirder,pGirder);

   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();


   CComPtr<IPoint2d> pntPier1, pntEnd1, pntBrg1, pntBrg2, pntEnd2, pntPier2;
   pGirder->GetSegmentEndPoints(segmentKey,pgsTypes::pcGlobal,&pntPier1,&pntEnd1,&pntBrg1,&pntBrg2,&pntEnd2,&pntPier2);

   Float64 x,y;
   pntPier1->Location(&x,&y);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88000, ")<<_T("-1")<<_T(", ")<< QUIET(x) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88001, ")<<_T("-1")<<_T(", ")<< QUIET(y) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;

   pntEnd1->Location(&x,&y);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88002, ")<<_T("-1")<<_T(", ")<< QUIET(x) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88003, ")<<_T("-1")<<_T(", ")<< QUIET(y) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;

   pntBrg1->Location(&x,&y);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88004, ")<<_T("-1")<<_T(", ")<< QUIET(x) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88005, ")<<_T("-1")<<_T(", ")<< QUIET(y) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;

   pntBrg2->Location(&x,&y);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88006, ")<<_T("-1")<<_T(", ")<< QUIET(x) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88007, ")<<_T("-1")<<_T(", ")<< QUIET(y) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;

   pntEnd2->Location(&x,&y);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88008, ")<<_T("-1")<<_T(", ")<< QUIET(x) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88009, ")<<_T("-1")<<_T(", ")<< QUIET(y) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;

   pntPier2->Location(&x,&y);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88010, ")<<_T("-1")<<_T(", ")<< QUIET(x) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88011, ")<<_T("-1")<<_T(", ")<< QUIET(y) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;

   GET_IFACE(IBridge,pBridge);
   Float64 L = pBridge->GetSegmentLength(segmentKey);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88100, ")<<_T("-1")<<_T(", ")<< QUIET(L) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;

   L = pBridge->GetSegmentSpanLength(segmentKey);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88101, ")<<_T("-1")<<_T(", ")<< QUIET(L) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;

   L = pBridge->GetSegmentPlanLength(segmentKey);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88102, ")<<_T("-1")<<_T(", ")<< QUIET(L) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;

   // Bearing seat elevations
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   PierIndexType startPierIdx = (SpanIndexType)(pSegment->GetGirder()->GetGirderGroup()->GetPierIndex(pgsTypes::metStart));
   PierIndexType endPierIdx = startPierIdx + 1;

   // Bearing Seat elevation data
   // ============================
   CGirderKey testGirder(segmentKey);
   // Write out all bearings at start end of girder
   std::vector<BearingElevationDetails> startBearings = pBridge->GetBearingElevationDetails(startPierIdx, pgsTypes::Ahead, testGirder.girderIndex, false);
   for (const BearingElevationDetails& beDet : startBearings)
   {
      if (beDet.GirderKey == testGirder)
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88200, ")<<_T("-1")<<_T(", ")<< QUIET(beDet.BrgSeatElevation) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88201, ")<<_T("-1")<<_T(", ")<< QUIET(beDet.Station) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88202, ")<<_T("-1")<<_T(", ")<< QUIET(beDet.Offset) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88203, ")<<_T("-1")<<_T(", ")<< QUIET(beDet.FinishedGradeElevation) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;
      }
   }

   // Write out all bearings at end end of girder
   std::vector<BearingElevationDetails> endBearings = pBridge->GetBearingElevationDetails(endPierIdx, pgsTypes::Back, testGirder.girderIndex, false);
   for (const BearingElevationDetails& beDet : endBearings)
   {
      if (beDet.GirderKey == testGirder)
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88210, ")<<_T("-1")<<_T(", ")<< QUIET(beDet.BrgSeatElevation) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88211, ")<<_T("-1")<<_T(", ")<< QUIET(beDet.Station) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88212, ")<<_T("-1")<<_T(", ")<< QUIET(beDet.Offset) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 88213, ")<<_T("-1")<<_T(", ")<< QUIET(beDet.FinishedGradeElevation) <<_T(", 55, ")<< SEGMENT(segmentKey) <<std::endl;
      }
   }

   // temporary support elevations
   SupportIndexType nTS = pBridge->GetTemporarySupportCount();
   GET_IFACE_NOCHECK(ITempSupport, pTempSupport);
   for (SupportIndexType tsIdx = 0; tsIdx < nTS; tsIdx++)
   {
      std::vector<TEMPORARYSUPPORTELEVATIONDETAILS> vElevDetails = pTempSupport->GetElevationDetails(tsIdx, ALL_GIRDERS);
      for (const auto& tsElev : vElevDetails)
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 88250, ") << _T("-1") << _T(", ") << tsIdx << _T(", ") << tsElev.girderIdx << _T(", ") << QUIET(tsElev.Elevation) << std::endl;
      }
   }

   return true;
}

bool CTestAgentImp::RunDistFactorTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, const CSegmentKey& segmentKey)
{
   // get results for the span where the segment starts
   // this may cause duplicates but it allows use to just do test results by segment
   // rather than have a new class of tests that are by span
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   SpanIndexType spanIdx = (SpanIndexType)(pSegment->GetGirder()->GetGirderGroup()->GetPierIndex(pgsTypes::metStart));
   GirderIndexType gdrIdx = segmentKey.girderIndex;

   CSpanKey spanKey(spanIdx,gdrIdx);

   GET_IFACE( ILiveLoadDistributionFactors, pDf );

   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   // write to poi file
   poiFile<<_T(" 1, ")<< bridgeId<< _T(", 3, 1, 0.0000, 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0")<<std::endl;

   bool bResult1 = pDf->Run1250Tests(spanKey,pgsTypes::StrengthI,pid.c_str(),bridgeId.c_str(),resultsFile,poiFile);

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      bResult1 = pDf->Run1250Tests(spanKey,pgsTypes::FatigueI,pid.c_str(),bridgeId.c_str(),resultsFile,poiFile);
   }

   return bResult1;
}

bool CTestAgentImp::RunHL93Test(std::_tofstream& resultsFile, std::_tofstream& poiFile, const CSegmentKey& segmentKey)
{
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE( IProductForces, pForce);
   GET_IFACE( ISpecification,     pSpec);
   GET_IFACE( IIntervals, pIntervals);

   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);

   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   // This complex method of getting POIs is to match the POIs used in the regression test
   // from previous version of PGSuper (Versions 2.x). By making the vector of POI match
   // the older versions the same regression test results are reported making it easier to
   // compare results.
   PoiList vPoi;
   pIPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT, &vPoi);
   pIPoi->RemovePointsOfInterest(vPoi,POI_15H);
   PoiList vPoi2;
   pIPoi->GetPointsOfInterest(segmentKey, POI_RELEASED_SEGMENT, &vPoi2);
   pIPoi->RemovePointsOfInterest(vPoi2, POI_RELEASED_SEGMENT | POI_0L);
   pIPoi->RemovePointsOfInterest(vPoi2, POI_RELEASED_SEGMENT | POI_10L);
   PoiList vPoi3;
   pIPoi->GetPointsOfInterest(segmentKey, POI_PSXFER | POI_HARPINGPOINT | POI_BARDEVELOP, &vPoi3);
   vPoi.insert(vPoi.end(),vPoi2.begin(),vPoi2.end());
   vPoi.insert(vPoi.end(),vPoi3.begin(),vPoi3.end());
   pIPoi->SortPoiList(&vPoi);

   IndexType npoi = vPoi.size();
   for (IndexType i=0; i<npoi; i++)
   {
      const pgsPointOfInterest& rpoi = vPoi[i];
      IndexType locn = i+1;
      Float64 loc = WBFL::Units::ConvertFromSysUnits(rpoi.GetDistFromStart(), WBFL::Units::Measure::Millimeter);

      // write to poi file
      poiFile<<locn<<_T(", ")<< bridgeId<< _T(", 7, 1, ")<<loc<<_T(", 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0")<<std::endl;

      Float64         pm, nm;
      WBFL::System::SectionValue sps, sns;
      Float64         pd, nd;
      // get live load results

      if ( analysisType == pgsTypes::Envelope )
      {
         Float64 dummy;
         WBFL::System::SectionValue svDummy;

         pForce->GetLiveLoadMoment(liveLoadIntervalIdx, pgsTypes::lltDesign, rpoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &dummy, &pm);
         pForce->GetLiveLoadMoment(liveLoadIntervalIdx, pgsTypes::lltDesign, rpoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &nm, &dummy);

         pForce->GetLiveLoadShear(liveLoadIntervalIdx,  pgsTypes::lltDesign, rpoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &svDummy, &sps);
         pForce->GetLiveLoadShear(liveLoadIntervalIdx,  pgsTypes::lltDesign, rpoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &sns, &svDummy);

         pForce->GetLiveLoadDeflection(liveLoadIntervalIdx, pgsTypes::lltDesign, rpoi, pgsTypes::MaxSimpleContinuousEnvelope, true, false, &dummy, &pd);
         pForce->GetLiveLoadDeflection(liveLoadIntervalIdx, pgsTypes::lltDesign, rpoi, pgsTypes::MinSimpleContinuousEnvelope, true, false, &nd, &dummy);
      }
      else
      {
         pForce->GetLiveLoadMoment(    liveLoadIntervalIdx, pgsTypes::lltDesign, rpoi, bat, true, false, &nm,  &pm);
         pForce->GetLiveLoadShear(     liveLoadIntervalIdx, pgsTypes::lltDesign, rpoi, bat, true, false, &sns, &sps);
         pForce->GetLiveLoadDeflection(liveLoadIntervalIdx, pgsTypes::lltDesign, rpoi, bat, true, false, &nd,  &pd);
      }

      // unit conversions
      pm = WBFL::Units::ConvertFromSysUnits(pm, WBFL::Units::Measure::NewtonMillimeter);
      nm = WBFL::Units::ConvertFromSysUnits(nm, WBFL::Units::Measure::NewtonMillimeter);
      Float64 ps = WBFL::Units::ConvertFromSysUnits(Max(sps.Left(),sps.Right()), WBFL::Units::Measure::Newton);
      Float64 ns = WBFL::Units::ConvertFromSysUnits(Min(sns.Left(),sns.Right()), WBFL::Units::Measure::Newton);
      pd = WBFL::Units::ConvertFromSysUnits(pd, WBFL::Units::Measure::Millimeter);
      nd = WBFL::Units::ConvertFromSysUnits(nd, WBFL::Units::Measure::Millimeter);

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 32090, ")<<loc<<_T(", ")<< QUIET(pm) <<_T(", 7, ")<< SEGMENT(segmentKey) <<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 32091, ")<<loc<<_T(", ")<< QUIET(nm) <<_T(", 7, ")<< SEGMENT(segmentKey) <<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 32092, ")<<loc<<_T(", ")<< QUIET(ps) <<_T(", 7, ")<< SEGMENT(segmentKey) <<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 32093, ")<<loc<<_T(", ")<< QUIET(ns) <<_T(", 7, ")<< SEGMENT(segmentKey) <<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 32094, ")<<loc<<_T(", ")<< DEFLECTION(-nd) <<_T(", 7, ")<< SEGMENT(segmentKey) <<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 32095, ")<<loc<<_T(", ")<< DEFLECTION(-pd) <<_T(", 7, ")<< SEGMENT(segmentKey) <<std::endl;

      // deflection truck with full width stiffness
      pForce->GetDeflLiveLoadDeflection( IProductForces::DesignTruckAlone, rpoi, bat, &nd, &pd );
      pd = WBFL::Units::ConvertFromSysUnits(pd, WBFL::Units::Measure::Millimeter);
      nd = WBFL::Units::ConvertFromSysUnits(nd, WBFL::Units::Measure::Millimeter);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 32200, ")<<loc<<_T(", ")<< DEFLECTION(-nd) <<_T(", 7, ")<< SEGMENT(segmentKey) <<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 32201, ")<<loc<<_T(", ")<< DEFLECTION(-pd) <<_T(", 7, ")<< SEGMENT(segmentKey) <<std::endl;

   }
   return true;
}

bool CTestAgentImp::RunCrossSectionTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, const CSegmentKey& segmentKey)
{
   GET_IFACE( ISectionProperties, pSp2 );
   GET_IFACE( IGirder, pGdr);
   GET_IFACE( IIntervals, pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();

   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   // write to poi file
   poiFile<<_T(" 1, ")<< bridgeId<< _T(", 3, 1, 0.0000, 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0")<<std::endl;

   pgsPointOfInterest poi( segmentKey, 0.0);

   // exterior girder
   // bare girder
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25000, 0.0, ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSp2->GetAg(releaseIntervalIdx,poi),  WBFL::Units::Measure::Millimeter2)) <<_T(", 4, ")<< SEGMENT(segmentKey) <<std::endl;
   Float64 V, S;
   pSp2->GetSegmentVolumeAndSurfaceArea(segmentKey, &V, &S);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25001, 0.0, ")<< QUIET(WBFL::Units::ConvertFromSysUnits(V, WBFL::Units::Measure::Millimeter3)) <<_T(", 4, ")<< SEGMENT(segmentKey) <<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25002, 0.0, ")<< QUIET(WBFL::Units::ConvertFromSysUnits(S, WBFL::Units::Measure::Millimeter2)) <<_T(", 4, ")<< SEGMENT(segmentKey) <<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25004, 0.0, ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSp2->GetIxx(releaseIntervalIdx,poi), WBFL::Units::Measure::Millimeter4)) <<_T(", 4, ")<< SEGMENT(segmentKey) <<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25005, 0.0, ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSp2->GetY(releaseIntervalIdx,poi,pgsTypes::BottomGirder), WBFL::Units::Measure::Millimeter)) << _T(", 4, ")<< SEGMENT(segmentKey) <<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25006, 0.0, ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSp2->GetS(releaseIntervalIdx,poi,pgsTypes::BottomGirder), WBFL::Units::Measure::Millimeter3)) <<_T(", 4, ")<< SEGMENT(segmentKey) <<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25007, 0.0, ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSp2->GetS(releaseIntervalIdx,poi,pgsTypes::TopGirder), WBFL::Units::Measure::Millimeter3)) <<_T(", 4, ")<< SEGMENT(segmentKey) <<std::endl;

   // composite girder
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25008, 0.0, ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSp2->GetIxx(compositeIntervalIdx,poi), WBFL::Units::Measure::Millimeter4)) << _T(", 4, ")<< SEGMENT(segmentKey) <<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25009, 0.0, ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSp2->GetY(compositeIntervalIdx,poi,pgsTypes::BottomGirder), WBFL::Units::Measure::Millimeter)) <<  _T(", 4, ")<< SEGMENT(segmentKey) <<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25010, 0.0, ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSp2->GetS(compositeIntervalIdx,poi,pgsTypes::BottomGirder), WBFL::Units::Measure::Millimeter3)) << _T(", 4, ")<< SEGMENT(segmentKey) <<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25011, 0.0, ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSp2->GetS(compositeIntervalIdx,poi,pgsTypes::TopGirder), WBFL::Units::Measure::Millimeter3)) <<_T(", 4, ")<< SEGMENT(segmentKey) <<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25012, 0.0, ") << QUIET(WBFL::Units::ConvertFromSysUnits(pSp2->GetS(compositeIntervalIdx, poi, pgsTypes::TopDeck), WBFL::Units::Measure::Millimeter3)) << _T(", 4, ") << SEGMENT(segmentKey) << std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25013, 0.0, ") << QUIET(WBFL::Units::ConvertFromSysUnits(pSp2->GetQSlab(compositeIntervalIdx, poi), WBFL::Units::Measure::Millimeter3)) << _T(", 4, ") << SEGMENT(segmentKey) << std::endl;

   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25031, 0.0, ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSp2->GetAcBottomHalf(compositeIntervalIdx,poi),WBFL::Units::Measure::Millimeter2)) <<_T(", 4, 1")<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 25033, 0.0, ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pGdr->GetHeight(poi), WBFL::Units::Measure::Millimeter)) <<_T(", 4, 1")<<std::endl;

   return true;
}

bool CTestAgentImp::RunDeadLoadActionTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, const CSegmentKey& segmentKey)
{
   GET_IFACE( IPointOfInterest,   pIPoi);
   GET_IFACE( IProductForces,     pForce);
   GET_IFACE( IReactions,         pReactions);
   GET_IFACE( ICombinedForces,    pForces);
   GET_IFACE( ISpecification,     pSpec);
   GET_IFACE( IIntervals,         pIntervals);

   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();
   IntervalIndexType noncompositeIntervalIdx  = pIntervals->GetLastNoncompositeInterval();
   IntervalIndexType compositeIntervalIdx     = pIntervals->GetLastCompositeInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType noncompositeUserLoadIntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();
   IntervalIndexType compositeUserLoadIntervalIdx = pIntervals->GetCompositeUserLoadInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple     ? pgsTypes::SimpleSpan : 
                             analysisType == pgsTypes::Continuous ? pgsTypes::ContinuousSpan : pgsTypes::MaxSimpleContinuousEnvelope);

   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   // This complex method of getting POIs is to match the POIs used in the regression test
   // from previous version of PGSuper (Versions 2.x). By making the vector of POI match
   // the older versions the same regression test results are reported making it easier to
   // compare results.
   PoiList vPoi;
   pIPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT, &vPoi);
   pIPoi->RemovePointsOfInterest(vPoi,POI_15H);
   PoiList vPoi2;
   pIPoi->GetPointsOfInterest(segmentKey, POI_RELEASED_SEGMENT, &vPoi2);
   pIPoi->RemovePointsOfInterest(vPoi2, POI_RELEASED_SEGMENT | POI_0L);
   pIPoi->RemovePointsOfInterest(vPoi2, POI_RELEASED_SEGMENT | POI_10L);
   PoiList vPoi3;
   pIPoi->GetPointsOfInterest(segmentKey, POI_PSXFER | POI_HARPINGPOINT | POI_BARDEVELOP, &vPoi3);
   vPoi.insert(vPoi.end(),vPoi2.begin(),vPoi2.end());
   vPoi.insert(vPoi.end(),vPoi3.begin(),vPoi3.end());
   pIPoi->SortPoiList(&vPoi);

   IndexType npoi = vPoi.size();
   for (IndexType i=0; i<npoi; i++)
   {
      const pgsPointOfInterest& poi = vPoi[i];
      CSpanKey spanKey;
      Float64 Xspan;
      pIPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);
      PierIndexType pierIdx(spanKey.spanIndex);

      IndexType deckCastingRegionIdx = pIPoi->GetDeckCastingRegion(poi);

      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval(deckCastingRegionIdx);
      IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);


      IndexType locn = i+1;
      Float64 loc = WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart(), WBFL::Units::Measure::Millimeter);

      // write to poi file
      poiFile<<locn<<_T(", ")<< bridgeId<< _T(", 7, 1, ")<<loc<<_T(", 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0")<<std::endl;

      resultsFile << bridgeId << _T(", ") << pid << _T(", 29998, ") << loc << _T(", ") << DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForce->GetDeflection(releaseIntervalIdx, pgsTypes::pftGirder, poi, bat, rtCumulative, false), WBFL::Units::Measure::Millimeter)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 29999, ") << loc << _T(", ") << DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForce->GetDeflection(releaseIntervalIdx, pgsTypes::pftPretension, poi, bat, rtCumulative, false), WBFL::Units::Measure::Millimeter)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;


      // girder 
      IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30000, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetMoment( erectSegmentIntervalIdx, pgsTypes::pftGirder, poi, bat, rtCumulative ), WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      if ( poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L) )
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30001, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear( erectSegmentIntervalIdx, pgsTypes::pftGirder, poi, bat, rtIncremental ).Right(), WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }
      else
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30001, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear( erectSegmentIntervalIdx, pgsTypes::pftGirder, poi, bat, rtIncremental ).Left(), WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30002, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForce->GetDeflection( erectSegmentIntervalIdx, pgsTypes::pftGirder, poi, bat, rtIncremental, false ), WBFL::Units::Measure::Millimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      
      if (i == 0)
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 30200, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pReactions->GetReaction(segmentKey, pierIdx, pgsTypes::stPier, erectSegmentIntervalIdx, pgsTypes::pftGirder, bat, rtIncremental).Fy, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      }

      // diaphragm
      resultsFile << bridgeId << _T(", ") << pid << _T(", 30009, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetMoment(castDiaphragmIntervalIdx, pgsTypes::pftDiaphragm, poi, bat, rtIncremental), WBFL::Units::Measure::NewtonMillimeter)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      if (poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L))
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 30010, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear(castDiaphragmIntervalIdx, pgsTypes::pftDiaphragm, poi, bat, rtIncremental).Right(), WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      }
      else
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 30010, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear(castDiaphragmIntervalIdx, pgsTypes::pftDiaphragm, poi, bat, rtIncremental).Left(), WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      }

      resultsFile << bridgeId << _T(", ") << pid << _T(", 30011, ") << loc << _T(", ") << DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForce->GetDeflection(castDiaphragmIntervalIdx, pgsTypes::pftDiaphragm, poi, bat, rtIncremental, false), WBFL::Units::Measure::Millimeter)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      if (i == 0)
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 30209, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pReactions->GetReaction(segmentKey, pierIdx, pgsTypes::stPier, castDiaphragmIntervalIdx, pgsTypes::pftDiaphragm, bat, rtIncremental).Fy, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      }

      // ShearKey
      resultsFile << bridgeId << _T(", ") << pid << _T(", 30070, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetMoment(noncompositeIntervalIdx, pgsTypes::pftShearKey, poi, bat, rtIncremental), WBFL::Units::Measure::NewtonMillimeter)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      if (poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L))
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 30071, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear(noncompositeIntervalIdx, pgsTypes::pftShearKey, poi, bat, rtIncremental).Right(), WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      }
      else
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 30071, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear(noncompositeIntervalIdx, pgsTypes::pftShearKey, poi, bat, rtIncremental).Left(), WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      }
      resultsFile << bridgeId << _T(", ") << pid << _T(", 30072, ") << loc << _T(", ") << DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForce->GetDeflection(noncompositeIntervalIdx, pgsTypes::pftShearKey, poi, bat, rtIncremental, false), WBFL::Units::Measure::Millimeter)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      if (i == 0)
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 30270, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pReactions->GetReaction(segmentKey, pierIdx, pgsTypes::stPier, noncompositeIntervalIdx, pgsTypes::pftShearKey, bat, rtIncremental).Fy, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      }
       
      // slab + slab pad
      if (castDeckIntervalIdx == INVALID_INDEX)
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 30012, ") << loc << _T(", 0, 1, ") << SEGMENT(segmentKey) << std::endl;
         if (poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L))
         {
            resultsFile << bridgeId << _T(", ") << pid << _T(", 30013, ") << loc << _T(", 0, 1, ") << SEGMENT(segmentKey) << std::endl;
         }
         else
         {
            resultsFile << bridgeId << _T(", ") << pid << _T(", 30013, ") << loc << _T(", 0, 1, ") << SEGMENT(segmentKey) << std::endl;
         }
         resultsFile << bridgeId << _T(", ") << pid << _T(", 30014, ") << loc << _T(", 0, 1, ") << SEGMENT(segmentKey) << std::endl;
         if (i == 0)
         {
            resultsFile << bridgeId << _T(", ") << pid << _T(", 30013, ") << loc << _T(", 0, 1, ") << SEGMENT(segmentKey) << std::endl;
         }
      }
      else
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 30012, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlab, poi, bat, rtIncremental) + pForce->GetMoment(castDeckIntervalIdx, pgsTypes::pftSlabPad, poi, bat, rtIncremental), WBFL::Units::Measure::NewtonMillimeter)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
         if (poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L))
         {
            resultsFile << bridgeId << _T(", ") << pid << _T(", 30013, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear(castDeckIntervalIdx, pgsTypes::pftSlab, poi, bat, rtIncremental).Right() + pForce->GetShear(castDeckIntervalIdx, pgsTypes::pftSlabPad, poi, bat, rtIncremental).Right(), WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
         }
         else
         {
            resultsFile << bridgeId << _T(", ") << pid << _T(", 30013, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear(castDeckIntervalIdx, pgsTypes::pftSlab, poi, bat, rtIncremental).Left() + pForce->GetShear(castDeckIntervalIdx, pgsTypes::pftSlabPad, poi, bat, rtIncremental).Left(), WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
         }
         resultsFile << bridgeId << _T(", ") << pid << _T(", 30014, ") << loc << _T(", ") << DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForce->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi, bat, rtIncremental, false) + pForce->GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlabPad, poi, bat, rtIncremental, false), WBFL::Units::Measure::Millimeter)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
         if (i == 0)
         {
            resultsFile << bridgeId << _T(", ") << pid << _T(", 30013, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pReactions->GetReaction(segmentKey, pierIdx, pgsTypes::stPier, castDeckIntervalIdx, pgsTypes::pftSlab, bat, rtIncremental).Fy + pReactions->GetReaction(segmentKey, pierIdx, pgsTypes::stPier, castDeckIntervalIdx, pgsTypes::pftSlabPad, bat, rtIncremental).Fy, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
         }
      }

      // DC - BSS1
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30036, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForces->GetMoment( noncompositeIntervalIdx, lcDC, poi, bat, rtCumulative ), WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      if ( poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L) )
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30037, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForces->GetShear(noncompositeIntervalIdx, lcDC, poi, bat, rtCumulative ).Right(), WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }
      else
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30037, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForces->GetShear(noncompositeIntervalIdx, lcDC, poi, bat, rtCumulative ).Left(), WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30038, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForces->GetDeflection(noncompositeIntervalIdx, lcDC, poi, bat, rtCumulative, false), WBFL::Units::Measure::Millimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      if (i == 0)
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 30236, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pReactions->GetReaction(segmentKey, pierIdx, pgsTypes::stPier, noncompositeIntervalIdx, lcDC, bat, rtCumulative).Fy, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      }

      // DW - BSS1
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30039, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForces->GetMoment(noncompositeIntervalIdx, lcDW, poi, bat, rtCumulative ), WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      if ( poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L) )
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30040, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForces->GetShear(noncompositeIntervalIdx, lcDW, poi, bat, rtCumulative ).Right(), WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }
      else
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30040, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForces->GetShear(noncompositeIntervalIdx, lcDW, poi, bat, rtCumulative ).Left(), WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30041, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForces->GetDeflection(noncompositeIntervalIdx, lcDW, poi, bat, rtCumulative, false ), WBFL::Units::Measure::Millimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      if (i == 0)
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 30239, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pReactions->GetReaction(segmentKey, pierIdx, pgsTypes::stPier, noncompositeIntervalIdx, lcDW, bat, rtCumulative).Fy, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      }

      // overlay
      if ( overlayIntervalIdx == INVALID_INDEX )
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30042, ")<<loc<<_T(", ")<< 0.0 << _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
         if ( poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L) )
         {
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30043, ")<<loc<<_T(", ")<< 0.0 <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
         }
         else
         {
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30043, ")<<loc<<_T(", ")<< 0.0 <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
         }

         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30044, ")<<loc<<_T(", ")<< 0.0 <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
         if (i == 0)
         {
            resultsFile << bridgeId << _T(", ") << pid << _T(", 30242, ") << loc << _T(", ") << 0.0 << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
         }
      }
      else
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30042, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetMoment( overlayIntervalIdx, pgsTypes::pftOverlay,poi, bat, rtIncremental ), WBFL::Units::Measure::NewtonMillimeter)) << _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
         if ( poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L) )
         {
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30043, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear( overlayIntervalIdx, pgsTypes::pftOverlay, poi, bat, rtIncremental ).Right(), WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
         }
         else
         {
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30043, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear( overlayIntervalIdx, pgsTypes::pftOverlay, poi, bat, rtIncremental ).Left(), WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
         }

         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30044, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForce->GetDeflection( overlayIntervalIdx, pgsTypes::pftOverlay, poi, bat, rtIncremental, false ), WBFL::Units::Measure::Millimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
         if (i == 0)
         {
            resultsFile << bridgeId << _T(", ") << pid << _T(", 30242, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pReactions->GetReaction(segmentKey, pierIdx, pgsTypes::stPier, overlayIntervalIdx, pgsTypes::pftOverlay, bat, rtIncremental).Fy, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
         }
      }

      // barrier
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30045, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetMoment( railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier,poi, bat, rtIncremental ), WBFL::Units::Measure::NewtonMillimeter)) << _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      if ( poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L) )
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30046, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear( railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, poi, bat, rtIncremental ).Right(), WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }
      else
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30046, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear( railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, poi, bat, rtIncremental ).Left(), WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30047, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForce->GetDeflection( railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, poi, bat, rtIncremental, false ), WBFL::Units::Measure::Millimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      if (i == 0)
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 30245, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pReactions->GetReaction(segmentKey, pierIdx, pgsTypes::stPier, railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, bat, rtIncremental).Fy, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      }

      // sidewalk
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30048, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetMoment( railingSystemIntervalIdx, pgsTypes::pftSidewalk,poi, bat, rtIncremental ), WBFL::Units::Measure::NewtonMillimeter)) << _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      if ( poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L) )
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30049, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear( railingSystemIntervalIdx, pgsTypes::pftSidewalk, poi, bat, rtIncremental ).Right(), WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }
      else
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30049, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear( railingSystemIntervalIdx, pgsTypes::pftSidewalk, poi, bat, rtIncremental ).Left(), WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30050, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForce->GetDeflection( railingSystemIntervalIdx, pgsTypes::pftSidewalk, poi, bat, rtIncremental, false ), WBFL::Units::Measure::Millimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      if (i == 0)
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 30248, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pReactions->GetReaction(segmentKey, pierIdx, pgsTypes::stPier, railingSystemIntervalIdx, pgsTypes::pftSidewalk, bat, rtIncremental).Fy, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      }

      // DC - BSS3
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30057, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForces->GetMoment( liveLoadIntervalIdx, lcDC, poi, bat, rtCumulative), WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      if ( poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L) )
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30058, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForces->GetShear( liveLoadIntervalIdx, lcDC, poi, bat, rtCumulative).Right(), WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }
      else
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30058, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForces->GetShear( liveLoadIntervalIdx, lcDC, poi, bat, rtCumulative).Left(), WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30059, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForces->GetDeflection( liveLoadIntervalIdx, lcDC, poi, bat, rtCumulative, false), WBFL::Units::Measure::Millimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      if (i == 0)
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 30257, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pReactions->GetReaction(segmentKey, pierIdx, pgsTypes::stPier, liveLoadIntervalIdx, lcDC, bat, rtCumulative).Fy, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      }

      // DW - BSS3
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30060, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForces->GetMoment( liveLoadIntervalIdx, lcDW, poi, bat, rtCumulative), WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      if ( poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L) )
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30061, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForces->GetShear( liveLoadIntervalIdx, lcDW, poi, bat, rtCumulative).Right(), WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }
      else
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30061, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForces->GetShear( liveLoadIntervalIdx, lcDW, poi, bat, rtCumulative).Left(), WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 30062, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForces->GetDeflection( liveLoadIntervalIdx, lcDW, poi, bat, rtCumulative, false ), WBFL::Units::Measure::Millimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      if (i == 0)
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 30260, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pReactions->GetReaction(segmentKey, pierIdx, pgsTypes::stPier, liveLoadIntervalIdx, lcDW, bat, rtCumulative).Fy, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      }

      // user loads - Moment
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122100, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetMoment( noncompositeUserLoadIntervalIdx, pgsTypes::pftUserDW, poi, bat, rtIncremental ), WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122101, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetMoment(noncompositeUserLoadIntervalIdx, pgsTypes::pftUserDC, poi, bat, rtIncremental ), WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122102, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetMoment( compositeUserLoadIntervalIdx, pgsTypes::pftUserDW, poi, bat, rtIncremental ), WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122103, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetMoment(compositeUserLoadIntervalIdx, pgsTypes::pftUserDC, poi, bat, rtIncremental ), WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;

      // user loads - Shear
      if ( poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L) )
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122104, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear(noncompositeUserLoadIntervalIdx, pgsTypes::pftUserDW, poi, bat, rtIncremental ).Right(), WBFL::Units::Measure::Newton)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122105, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear(noncompositeUserLoadIntervalIdx, pgsTypes::pftUserDC, poi, bat, rtIncremental ).Right(), WBFL::Units::Measure::Newton)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122106, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear(compositeUserLoadIntervalIdx, pgsTypes::pftUserDW, poi, bat, rtIncremental ).Right(), WBFL::Units::Measure::Newton)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122107, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear(compositeUserLoadIntervalIdx, pgsTypes::pftUserDC, poi, bat, rtIncremental ).Right(), WBFL::Units::Measure::Newton)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }
      else
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122104, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear(noncompositeUserLoadIntervalIdx, pgsTypes::pftUserDW, poi, bat, rtIncremental ).Left(), WBFL::Units::Measure::Newton)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122105, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear(noncompositeUserLoadIntervalIdx, pgsTypes::pftUserDC, poi, bat, rtIncremental ).Left(), WBFL::Units::Measure::Newton)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122106, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear(compositeUserLoadIntervalIdx, pgsTypes::pftUserDW, poi, bat, rtIncremental ).Left(), WBFL::Units::Measure::Newton)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122107, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear(compositeUserLoadIntervalIdx, pgsTypes::pftUserDC, poi, bat, rtIncremental ).Left(), WBFL::Units::Measure::Newton)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }

      // user loads - Deflection
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122108, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForce->GetDeflection(noncompositeUserLoadIntervalIdx, pgsTypes::pftUserDW, poi, bat, rtIncremental, false ), WBFL::Units::Measure::Millimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122109, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForce->GetDeflection(noncompositeUserLoadIntervalIdx, pgsTypes::pftUserDC, poi, bat, rtIncremental, false ), WBFL::Units::Measure::Millimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122110, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForce->GetDeflection(compositeUserLoadIntervalIdx, pgsTypes::pftUserDW, poi, bat, rtIncremental, false ), WBFL::Units::Measure::Millimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122111, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForce->GetDeflection(compositeUserLoadIntervalIdx, pgsTypes::pftUserDC, poi, bat, rtIncremental, false ), WBFL::Units::Measure::Millimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;

      // user loads - Reaction
      if (i == 0)
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 122112, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pReactions->GetReaction(segmentKey, pierIdx, pgsTypes::stPier, noncompositeUserLoadIntervalIdx, pgsTypes::pftUserDW, bat, rtIncremental).Fy, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
         resultsFile << bridgeId << _T(", ") << pid << _T(", 122113, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pReactions->GetReaction(segmentKey, pierIdx, pgsTypes::stPier, noncompositeUserLoadIntervalIdx, pgsTypes::pftUserDC, bat, rtIncremental).Fy, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
         resultsFile << bridgeId << _T(", ") << pid << _T(", 122114, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pReactions->GetReaction(segmentKey, pierIdx, pgsTypes::stPier, compositeUserLoadIntervalIdx, pgsTypes::pftUserDW, bat, rtIncremental).Fy, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
         resultsFile << bridgeId << _T(", ") << pid << _T(", 122115, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pReactions->GetReaction(segmentKey, pierIdx, pgsTypes::stPier, compositeUserLoadIntervalIdx, pgsTypes::pftUserDC, bat, rtIncremental).Fy, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      }

      // user live load
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122116, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetMoment(liveLoadIntervalIdx, pgsTypes::pftUserLLIM, poi, bat, rtIncremental ), WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      if ( poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L) )
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122117, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear(liveLoadIntervalIdx, pgsTypes::pftUserLLIM, poi, bat, rtIncremental ).Right(), WBFL::Units::Measure::Newton)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }
      else
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122117, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pForce->GetShear(liveLoadIntervalIdx, pgsTypes::pftUserLLIM, poi, bat, rtIncremental ).Left(), WBFL::Units::Measure::Newton)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      }
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122118, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(pForce->GetDeflection(liveLoadIntervalIdx, pgsTypes::pftUserLLIM, poi, bat, rtIncremental, false ), WBFL::Units::Measure::Millimeter)) <<_T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      if (i == 0)
      {
         resultsFile << bridgeId << _T(", ") << pid << _T(", 122119, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(pReactions->GetReaction(segmentKey, pierIdx, pgsTypes::stPier, liveLoadIntervalIdx, pgsTypes::pftUserLLIM, bat, rtIncremental).Fy, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;
      }
   }

   // Girder bearing reactions
   GET_IFACE(IBearingDesign,pBearingDesign);
   IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

   // NOTE: These regression test results will not align with those from versions of PGSuper prior to 3.0
   // The concept of reactions has been generalized and the old method of dumping results no longer worked
   GET_IFACE(IBridge,pBridge);
   PierIndexType startPierIdx, endPierIdx;
   pBridge->GetGirderGroupPiers(segmentKey.groupIndex,&startPierIdx,&endPierIdx);
   std::vector<PierIndexType> vPiers = pBearingDesign->GetBearingReactionPiers(liveLoadIntervalIdx,segmentKey);
   for (const auto& pierIdx : vPiers)
   {
      ReactionLocation location;
      location.Face = (pierIdx == startPierIdx ? rftAhead : rftBack);
      if (pBridge->IsInteriorPier(pierIdx))
      {
         location.Face = rftMid;
      }

      location.GirderKey = segmentKey;
      location.PierIdx = pierIdx;


      Float64 R;
      // girder
      R = pBearingDesign->GetBearingProductReaction(erectSegmentIntervalIdx, location, pgsTypes::pftGirder, bat, rtCumulative);
      resultsFile << bridgeId << _T(", ") << pid << _T(", 165000, 0.000, ") << QUIET(WBFL::Units::ConvertFromSysUnits(R, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;

      // diaphragm
      R = pBearingDesign->GetBearingProductReaction(erectSegmentIntervalIdx, location, pgsTypes::pftDiaphragm, bat, rtCumulative);
      resultsFile << bridgeId << _T(", ") << pid << _T(", 165002, 0.000, ") << QUIET(WBFL::Units::ConvertFromSysUnits(R, WBFL::Units::Measure::Newton)) << _T(", 1, ") << SEGMENT(segmentKey) << std::endl;

      // ShearKey
      R = pBearingDesign->GetBearingProductReaction(erectSegmentIntervalIdx, location, pgsTypes::pftShearKey, bat, rtCumulative);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165004, 0.000, ")<< QUIET(WBFL::Units::ConvertFromSysUnits( R, WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;

      // slab
      R = pBearingDesign->GetBearingProductReaction(erectSegmentIntervalIdx, location, pgsTypes::pftSlab, bat, rtCumulative);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165006, 0.000, ")<< QUIET(WBFL::Units::ConvertFromSysUnits( R, WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;

      // DC - BSS1
      R = pBearingDesign->GetBearingCombinedReaction(noncompositeIntervalIdx, location, lcDC, bat, rtCumulative); 
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165008, 0.000, ")<< QUIET(WBFL::Units::ConvertFromSysUnits( R, WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
      
      // DW - BSS1
      R = pBearingDesign->GetBearingCombinedReaction(noncompositeIntervalIdx, location, lcDW, bat, rtCumulative);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165010, 0.000, ")<< QUIET(WBFL::Units::ConvertFromSysUnits( R, WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;

      // overlay
      R = pBearingDesign->GetBearingProductReaction(erectSegmentIntervalIdx, location, pgsTypes::pftOverlay, bat, rtCumulative);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165012, 0.000, ")<< QUIET(WBFL::Units::ConvertFromSysUnits( R, WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;

      // barrier
      R = pBearingDesign->GetBearingProductReaction(erectSegmentIntervalIdx, location, pgsTypes::pftTrafficBarrier, bat, rtCumulative);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165014, 0.000, ")<< QUIET(WBFL::Units::ConvertFromSysUnits( R, WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;

      // sidewalk
      R = pBearingDesign->GetBearingProductReaction(erectSegmentIntervalIdx, location, pgsTypes::pftSidewalk, bat, rtCumulative);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165016, 0.000, ")<< QUIET(WBFL::Units::ConvertFromSysUnits( R, WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;

      // DC - BSS3
      R = pBearingDesign->GetBearingCombinedReaction(liveLoadIntervalIdx, location, lcDC, bat, rtCumulative); 
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165018, 0.000, ")<< QUIET(WBFL::Units::ConvertFromSysUnits( R, WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;

      // DW - BSS3
      R = pBearingDesign->GetBearingCombinedReaction(liveLoadIntervalIdx, location, lcDW, bat, rtCumulative); 
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165020, 0.000, ")<< QUIET(WBFL::Units::ConvertFromSysUnits( R, WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;

      // user loads
      R = pBearingDesign->GetBearingProductReaction(erectSegmentIntervalIdx, location, pgsTypes::pftUserDW, bat, rtCumulative);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165022, 0.000, ")<< QUIET(WBFL::Units::ConvertFromSysUnits( R, WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;

      // user live load
      R = pBearingDesign->GetBearingProductReaction(erectSegmentIntervalIdx, location, pgsTypes::pftUserLLIM, bat, rtCumulative);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 165024, 0.000, ")<< QUIET(WBFL::Units::ConvertFromSysUnits( R, WBFL::Units::Measure::Newton)) <<    _T(", 1, ")<< SEGMENT(segmentKey) <<std::endl;
   }
   return true;
}

bool CTestAgentImp::RunCombinedLoadActionTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, const CSegmentKey& segmentKey)
{
   GET_IFACE( IPointOfInterest,   pIPoi);
   GET_IFACE( ILimitStateForces,  pLsForces);
   GET_IFACE( ISpecification,     pSpec);
   GET_IFACE( IBearingDesign,     pBearingDesign);
   GET_IFACE( IIntervals,         pIntervals);

   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   // This complex method of getting POIs is to match the POIs used in the regression test
   // from previous version of PGSuper (Versions 2.x). By making the vector of POI match
   // the older versions the same regression test results are reported making it easier to
   // compare results.
   PoiList vPoi;
   pIPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT, &vPoi);
   pIPoi->RemovePointsOfInterest(vPoi,POI_15H);
   PoiList vPoi2;
   pIPoi->GetPointsOfInterest(segmentKey, POI_RELEASED_SEGMENT, &vPoi2);
   pIPoi->RemovePointsOfInterest(vPoi2, POI_RELEASED_SEGMENT | POI_0L);
   pIPoi->RemovePointsOfInterest(vPoi2, POI_RELEASED_SEGMENT | POI_10L);
   PoiList vPoi3;
   pIPoi->GetPointsOfInterest(segmentKey, POI_PSXFER | POI_HARPINGPOINT | POI_BARDEVELOP, &vPoi3);
   vPoi.insert(vPoi.end(),vPoi2.begin(),vPoi2.end());
   vPoi.insert(vPoi.end(),vPoi3.begin(),vPoi3.end());
   pIPoi->SortPoiList(&vPoi);

   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   pgsTypes::BridgeAnalysisType bat;

   Float64 min, max, dummy;

   IndexType npoi = vPoi.size();
   for (IndexType i=0; i<npoi; i++)
   {
      const pgsPointOfInterest& poi = vPoi[i];
      IndexType locn = i+1;
      Float64 loc = WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart(), WBFL::Units::Measure::Millimeter);

      // write to poi file
      poiFile<<locn<<_T(", ")<< bridgeId<< _T(", 7, 1, ")<<loc<<_T(", 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0")<<std::endl;

      // Strength I 
      if ( analysisType == pgsTypes::Envelope )
      {
         pLsForces->GetMoment( liveLoadIntervalIdx, pgsTypes::StrengthI, poi, pgsTypes::MaxSimpleContinuousEnvelope, &dummy, &max );
         pLsForces->GetMoment( liveLoadIntervalIdx, pgsTypes::StrengthI, poi, pgsTypes::MinSimpleContinuousEnvelope, &min, &dummy );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34000, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 8, ")<< SEGMENT(segmentKey) <<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34001, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 8, ")<< SEGMENT(segmentKey) <<std::endl;

         WBFL::System::SectionValue smin, smax, svDummy;
         pLsForces->GetShear( liveLoadIntervalIdx, pgsTypes::StrengthI, poi, pgsTypes::MaxSimpleContinuousEnvelope, &svDummy, &smax );
         pLsForces->GetShear( liveLoadIntervalIdx, pgsTypes::StrengthI, poi, pgsTypes::MinSimpleContinuousEnvelope, &smin, &svDummy );
         if ( poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L) )
         {
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34002, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smax.Right(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34003, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smin.Right(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         }
         else
         {
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34002, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smax.Left(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34003, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smin.Left(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         }

         pLsForces->GetDeflection( liveLoadIntervalIdx, pgsTypes::StrengthI, poi, pgsTypes::MaxSimpleContinuousEnvelope, false/*no prestress*/, true, false, false, true, &dummy, &max );
         pLsForces->GetDeflection( liveLoadIntervalIdx, pgsTypes::StrengthI, poi, pgsTypes::MinSimpleContinuousEnvelope, false/*no prestress*/, true, false, false, true, &min, &dummy );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34004, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::Millimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34005, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::Millimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;

         // Service I
         pLsForces->GetMoment( liveLoadIntervalIdx, pgsTypes::ServiceI, poi, pgsTypes::MaxSimpleContinuousEnvelope, &dummy, &max );
         pLsForces->GetMoment( liveLoadIntervalIdx, pgsTypes::ServiceI, poi, pgsTypes::MinSimpleContinuousEnvelope, &min, &dummy );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34020, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34021, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;

         pLsForces->GetShear( liveLoadIntervalIdx, pgsTypes::ServiceI, poi, pgsTypes::MaxSimpleContinuousEnvelope, &svDummy, &smax );
         pLsForces->GetShear( liveLoadIntervalIdx, pgsTypes::ServiceI, poi, pgsTypes::MinSimpleContinuousEnvelope, &smin, &svDummy );
         if ( poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L) )
         {
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34022, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smax.Right(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34023, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smin.Right(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         }
         else
         {
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34022, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smax.Left(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34023, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smin.Left(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         }

         pLsForces->GetDeflection( liveLoadIntervalIdx, pgsTypes::ServiceI, poi, pgsTypes::MaxSimpleContinuousEnvelope, false/*no prestress*/, true, false, false, true, &dummy, &max );
         pLsForces->GetDeflection( liveLoadIntervalIdx, pgsTypes::ServiceI, poi, pgsTypes::MinSimpleContinuousEnvelope, false/*no prestress*/, true, false, false, true, &min, &dummy );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34024, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::Millimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34025, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::Millimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;

         // Service III
         pLsForces->GetMoment( liveLoadIntervalIdx, pgsTypes::ServiceIII, poi, pgsTypes::MaxSimpleContinuousEnvelope, &dummy, &max );
         pLsForces->GetMoment( liveLoadIntervalIdx, pgsTypes::ServiceIII, poi, pgsTypes::MinSimpleContinuousEnvelope, &min, &dummy );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34032, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34033, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;

         pLsForces->GetShear( liveLoadIntervalIdx, pgsTypes::ServiceIII, poi, pgsTypes::MaxSimpleContinuousEnvelope, &svDummy, &smax );
         pLsForces->GetShear( liveLoadIntervalIdx, pgsTypes::ServiceIII, poi, pgsTypes::MinSimpleContinuousEnvelope, &smin, &svDummy );
         if ( poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L) )
         {
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34034, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smax.Right(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34035, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smin.Right(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         }
         else
         {
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34034, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smax.Left(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34035, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smin.Left(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         }

         pLsForces->GetDeflection( liveLoadIntervalIdx, pgsTypes::ServiceIII, poi, pgsTypes::MaxSimpleContinuousEnvelope, false/*no prestress*/, true, false, false, true, &dummy, &max );
         pLsForces->GetDeflection( liveLoadIntervalIdx, pgsTypes::ServiceIII, poi, pgsTypes::MinSimpleContinuousEnvelope, false/*no prestress*/, true, false, false, true, &min, &dummy );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34036, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::Millimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34037, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::Millimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
      }
      else
      {
         bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);
         pLsForces->GetMoment( liveLoadIntervalIdx, pgsTypes::StrengthI, poi, bat, &min, &max );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34000, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34001, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;

         WBFL::System::SectionValue smin, smax;
         pLsForces->GetShear( liveLoadIntervalIdx, pgsTypes::StrengthI, poi, bat, &smin, &smax );
         if ( poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L) )
         {
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34002, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smax.Right(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34003, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smin.Right(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         }
         else
         {
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34002, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smax.Left(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34003, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smin.Left(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         }

         pLsForces->GetDeflection( liveLoadIntervalIdx, pgsTypes::StrengthI, poi, bat, false/*no prestress*/, true, false, false, true, &min, &max );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34004, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::Millimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34005, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::Millimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;

         // Service I
         pLsForces->GetMoment( liveLoadIntervalIdx, pgsTypes::ServiceI, poi, bat, &min, &max );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34020, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34021, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;

         pLsForces->GetShear( liveLoadIntervalIdx, pgsTypes::ServiceI, poi, bat, &smin, &smax );
         if ( poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L) )
         {
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34022, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smax.Right(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34023, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smin.Right(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         }
         else
         {
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34022, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smax.Left(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34023, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smin.Left(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         }

         pLsForces->GetDeflection( liveLoadIntervalIdx, pgsTypes::ServiceI, poi, bat, false/*no prestress*/, true, false, false, true, &min, &max );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34024, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::Millimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34025, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::Millimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;

         // Service III
         pLsForces->GetMoment( liveLoadIntervalIdx, pgsTypes::ServiceIII, poi, bat, &min, &max );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34032, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34033, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::NewtonMillimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;

         pLsForces->GetShear( liveLoadIntervalIdx, pgsTypes::ServiceIII, poi, bat, &smin, &smax );
         if ( poi.HasAttribute(POI_ERECTED_SEGMENT | POI_0L) )
         {
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34034, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smax.Right(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34035, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smin.Right(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         }
         else
         {
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34034, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smax.Left(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34035, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(smin.Left(), WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         }

         pLsForces->GetDeflection( liveLoadIntervalIdx, pgsTypes::ServiceIII, poi, bat, false/*no prestress*/, true, false, false, true, &min, &max );
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34036, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::Millimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34037, ")<<loc<<_T(", ")<< DEFLECTION(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::Millimeter)) <<_T(", 8, ")<<SEGMENT(segmentKey)<<std::endl;
      }
   }

   // Reactions
   Float64 lftloc = WBFL::Units::ConvertFromSysUnits(vPoi.front().get().GetDistFromStart(), WBFL::Units::Measure::Millimeter);
   Float64 rgtloc = WBFL::Units::ConvertFromSysUnits(vPoi.back().get().GetDistFromStart(), WBFL::Units::Measure::Millimeter);

   GET_IFACE(IBridge,pBridge);
   PierIndexType startPierIdx, endPierIdx;
   pBridge->GetGirderGroupPiers(segmentKey.groupIndex,&startPierIdx,&endPierIdx);

   if ( analysisType == pgsTypes::Envelope )
   {
      GET_IFACE( IReactions, pReactions);

      // left end
      pLsForces->GetLSReaction(liveLoadIntervalIdx, pgsTypes::StrengthI, startPierIdx, segmentKey, pgsTypes::MaxSimpleContinuousEnvelope, true, &dummy, &max );
      pLsForces->GetLSReaction(liveLoadIntervalIdx, pgsTypes::StrengthI, startPierIdx, segmentKey, pgsTypes::MinSimpleContinuousEnvelope, true,  &min, &dummy );
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34040, ")<<lftloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34041, ")<<lftloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;

      // right end
      pLsForces->GetLSReaction(liveLoadIntervalIdx, pgsTypes::StrengthI, endPierIdx, segmentKey, pgsTypes::MaxSimpleContinuousEnvelope, true, &dummy, &max );
      pLsForces->GetLSReaction(liveLoadIntervalIdx, pgsTypes::StrengthI, endPierIdx, segmentKey, pgsTypes::MinSimpleContinuousEnvelope, true,  &min, &dummy );
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34040, ")<<rgtloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34041, ")<<rgtloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;

      pLsForces->GetLSReaction(liveLoadIntervalIdx, pgsTypes::ServiceI, startPierIdx, segmentKey, pgsTypes::MaxSimpleContinuousEnvelope, true, &dummy, &max );
      pLsForces->GetLSReaction(liveLoadIntervalIdx, pgsTypes::ServiceI, startPierIdx, segmentKey, pgsTypes::MinSimpleContinuousEnvelope, true,  &min, &dummy );
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34042, ")<<lftloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34043, ")<<lftloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;

      pLsForces->GetLSReaction(liveLoadIntervalIdx, pgsTypes::ServiceI, endPierIdx, segmentKey, pgsTypes::MaxSimpleContinuousEnvelope, true, &dummy, &max );
      pLsForces->GetLSReaction(liveLoadIntervalIdx, pgsTypes::ServiceI, endPierIdx, segmentKey, pgsTypes::MinSimpleContinuousEnvelope, true,  &min, &dummy );
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34042, ")<<rgtloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34043, ")<<rgtloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;

      pReactions->GetCombinedLiveLoadReaction(liveLoadIntervalIdx, pgsTypes::lltDesign, startPierIdx, segmentKey, pgsTypes::MaxSimpleContinuousEnvelope, false, &dummy, &max);
      pReactions->GetCombinedLiveLoadReaction(liveLoadIntervalIdx, pgsTypes::lltDesign, startPierIdx, segmentKey, pgsTypes::MinSimpleContinuousEnvelope, false, &min, &dummy);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34050, ")<<rgtloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34051, ")<<rgtloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;

      pReactions->GetCombinedLiveLoadReaction(liveLoadIntervalIdx, pgsTypes::lltDesign, endPierIdx, segmentKey, pgsTypes::MaxSimpleContinuousEnvelope, false, &dummy, &max);
      pReactions->GetCombinedLiveLoadReaction(liveLoadIntervalIdx, pgsTypes::lltDesign, endPierIdx, segmentKey, pgsTypes::MinSimpleContinuousEnvelope, false, &min, &dummy);
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34050, ")<<rgtloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34051, ")<<rgtloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;

      // NOTE: These regression test results will not align with those from versions of PGSuper prior to 3.0
      // The concept of reactions has been generalized and the old method of dumping results no longer worked
      PierIndexType startPierIdx, endPierIdx;
      pBridge->GetGirderGroupPiers(segmentKey.groupIndex,&startPierIdx,&endPierIdx);
      std::vector<PierIndexType> vPiers = pBearingDesign->GetBearingReactionPiers(liveLoadIntervalIdx,segmentKey);
      for (const auto& pierIdx : vPiers)
      {
         ReactionLocation location;
         location.Face = (pierIdx == startPierIdx ? rftAhead : rftBack);
         if ( pBridge->IsInteriorPier(pierIdx) )
         {
            location.Face = rftMid;
         }
         location.GirderKey = segmentKey;
         location.PierIdx = pierIdx;

         Float64 Val;
         pBearingDesign->GetBearingLimitStateReaction(liveLoadIntervalIdx, location, pgsTypes::StrengthI, pgsTypes::MaxSimpleContinuousEnvelope, true, &dummy, &Val);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34044, ")<<lftloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(Val, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;

         pBearingDesign->GetBearingLimitStateReaction(liveLoadIntervalIdx, location, pgsTypes::StrengthI, pgsTypes::MinSimpleContinuousEnvelope, true, &Val, &dummy);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34045, ")<<lftloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(Val, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;

         pBearingDesign->GetBearingLimitStateReaction(liveLoadIntervalIdx, location, pgsTypes::ServiceI, pgsTypes::MaxSimpleContinuousEnvelope, true, &dummy, &Val);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34046, ")<<lftloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(Val, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;

         pBearingDesign->GetBearingLimitStateReaction(liveLoadIntervalIdx, location, pgsTypes::ServiceI, pgsTypes::MinSimpleContinuousEnvelope, true, &Val, &dummy);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34047, ")<<lftloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(Val, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;

         pBearingDesign->GetBearingLiveLoadReaction(liveLoadIntervalIdx, location, pgsTypes::lltDesign, pgsTypes::MaxSimpleContinuousEnvelope, true, true, &dummy, &Val,  &dummy, &dummy);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34050, ")<<lftloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(Val, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;

         pBearingDesign->GetBearingLiveLoadReaction(liveLoadIntervalIdx, location, pgsTypes::lltDesign, pgsTypes::MinSimpleContinuousEnvelope, true, true, &Val,  &dummy, &dummy, &dummy);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34051, ")<<lftloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(Val, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;
      }
   }
   else
   {
      bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : pgsTypes::ContinuousSpan);

      // left end
      pLsForces->GetLSReaction(liveLoadIntervalIdx, pgsTypes::StrengthI, startPierIdx, segmentKey, bat, true, &min, &max );
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34040, ")<<lftloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34041, ")<<lftloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;

      // right end
      pLsForces->GetLSReaction(liveLoadIntervalIdx, pgsTypes::StrengthI, endPierIdx, segmentKey, bat, true, &min, &max );
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34040, ")<<rgtloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34041, ")<<rgtloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;

      pLsForces->GetLSReaction(liveLoadIntervalIdx, pgsTypes::ServiceI, startPierIdx, segmentKey, bat, true, &min, &max );
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34042, ")<<lftloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34043, ")<<lftloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;

      pLsForces->GetLSReaction(liveLoadIntervalIdx, pgsTypes::ServiceI, endPierIdx, segmentKey, bat, true, &min, &max );
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34042, ")<<rgtloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(max, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34043, ")<<rgtloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;

      // NOTE: These regression test results will not align with those from versions of PGSuper prior to 3.0
      // The concept of reactions has been generalized and the old method of dumping results no longer worked
      PierIndexType startPierIdx, endPierIdx;
      pBridge->GetGirderGroupPiers(segmentKey.groupIndex,&startPierIdx,&endPierIdx);
      std::vector<PierIndexType> vPiers = pBearingDesign->GetBearingReactionPiers(liveLoadIntervalIdx,segmentKey);
      for (const auto& pierIdx : vPiers)
      {
         ReactionLocation location;
         location.Face = (pierIdx == startPierIdx ? rftAhead : rftBack);
         if ( pBridge->IsInteriorPier(pierIdx) )
         {
            location.Face = rftMid;
         }
         location.GirderKey = segmentKey;
         location.PierIdx = pierIdx;

         Float64 minVal, maxVal;
         pBearingDesign->GetBearingLimitStateReaction(liveLoadIntervalIdx, location, pgsTypes::StrengthI, bat, true, &minVal, &maxVal);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34044, ")<<rgtloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(minVal, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34045, ")<<lftloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(maxVal, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;

         pBearingDesign->GetBearingLimitStateReaction(liveLoadIntervalIdx, location,   pgsTypes::ServiceI, bat, true, &minVal, &maxVal);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34046, ")<<rgtloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(minVal, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34047, ")<<lftloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(maxVal, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;

         pBearingDesign->GetBearingLiveLoadReaction(liveLoadIntervalIdx, location,   pgsTypes::lltDesign, bat, true, true, &minVal, &maxVal, &dummy, &dummy);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34050, ")<<rgtloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(minVal, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 34051, ")<<rgtloc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(maxVal, WBFL::Units::Measure::Newton)) <<_T(", 8, ")<<segmentKey.girderIndex<<std::endl;
      }
   }

   return true;
}

bool CTestAgentImp::RunPrestressedISectionTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, const CSegmentKey& segmentKey)
{
   GET_IFACE( IPointOfInterest,    pIPoi);
   GET_IFACE( ISectionProperties,  pSp2               );
   GET_IFACE( IPretensionForce,    pPrestressForce    );
   GET_IFACE( IPretensionStresses, pPrestressStresses );
   GET_IFACE( ILosses, pLosses );
   GET_IFACE( ILimitStateForces,pLsForces);
   GET_IFACE( IMomentCapacity,pMomentCapacity);
   GET_IFACE( IShearCapacity,pShearCapacity);
   GET_IFACE( IArtifact,pIArtifact);
   GET_IFACE( IProductForces, pProdForce);
   GET_IFACE( IIntervals, pIntervals);

   IntervalIndexType stressStrandsIntervalIdx = pIntervals->GetStressStrandInterval(segmentKey);
   IntervalIndexType releaseIntervalIdx       = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liftSegmentIntervalIdx   = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType haulSegmentIntervalIdx   = pIntervals->GetHaulSegmentInterval(segmentKey);
   IntervalIndexType erectSegmentIntervalIdx  = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tsInstallIntervalIdx     = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
   IntervalIndexType tsRemovalIntervalIdx     = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType noncompositeIntervalIdx  = pIntervals->GetLastNoncompositeInterval();
   IntervalIndexType compositeIntervalIdx     = pIntervals->GetLastCompositeInterval();
   IntervalIndexType trafficBarrierIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount() - 1;

   pgsTypes::BridgeAnalysisType bat = pProdForce->GetBridgeAnalysisType(pgsTypes::Maximize);

   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   // first dump out non-poi related values
   
   // losses
   CGirderKey girderKey(segmentKey);
   const pgsGirderArtifact* pGdrArtifact = pIArtifact->GetGirderArtifact(girderKey);

   const pgsSegmentArtifact* pSegmentArtifact = pIArtifact->GetSegmentArtifact(segmentKey);
   const pgsStirrupCheckArtifact* pstirrup_artifact= pSegmentArtifact->GetStirrupCheckArtifact();
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100200 , ")<<-1<<_T(", ")<<(int)(pSegmentArtifact->Passed()?1:0) <<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100201 , ")<<-1<<_T(", ")<<(int)(pstirrup_artifact->Passed()?1:0)<<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;

   std::vector<Float64> vCSLoc(pShearCapacity->GetCriticalSections(pgsTypes::StrengthI,segmentKey));
   for (const auto& Xcs : vCSLoc)
   {
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50052, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(Xcs, WBFL::Units::Measure::Millimeter)) <<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
   }

#pragma Reminder("UPDATE: assuming precast girder bridge") // there could be more than one constructibility check artifact per girder (one for each span)
   SpanIndexType spanIdx = segmentKey.groupIndex;

   const pgsConstructabilityArtifact* pConstr =  pGdrArtifact->GetConstructabilityArtifact();
   const auto& segArtifact = pConstr->GetSegmentArtifact(segmentKey.segmentIndex);
   if ( segArtifact.IsSlabOffsetApplicable() )
   {
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122005, ")<<-1<<_T(", ")<<(int)(segArtifact.Passed()?1:0)<<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      Float64 startA, endA;
      segArtifact.GetProvidedSlabOffset(&startA, &endA);
      if (segArtifact.AreSlabOffsetsSameAtEnds())
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122014, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(segArtifact.GetRequiredSlabOffset(), WBFL::Units::Measure::Millimeter)) <<_T(", 2, ")<<SEGMENT(segmentKey)<<std::endl;
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122015, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(startA, WBFL::Units::Measure::Millimeter)) <<_T(", 2, ")<<SEGMENT(segmentKey)<<std::endl;
      }
      else
      {
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122014, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(segArtifact.GetProvidedFillet(), WBFL::Units::Measure::Millimeter)) <<_T(", 2, ")<<SEGMENT(segmentKey)<<std::endl;
         Float64 location, leastH;
         segArtifact.GetLeastHaunchDepth(&location, &leastH);
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122015, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(leastH, WBFL::Units::Measure::Millimeter)) <<_T(", 2, ")<<SEGMENT(segmentKey)<<std::endl;
      }
   }

   if (segArtifact.IsHaunchGeometryCheckApplicable())
   {
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122040, ")<<-1<<_T(", ")<< (int)segArtifact.HaunchGeometryStatus() <<_T(", 2, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122041, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(segArtifact.GetAssumedExcessCamber(), WBFL::Units::Measure::Millimeter)) <<_T(", 2, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122042, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(segArtifact.GetComputedExcessCamber(), WBFL::Units::Measure::Millimeter)) <<_T(", 2, ")<<SEGMENT(segmentKey)<<std::endl;
   }

#pragma Reminder("UPDATE: assuming precast girder bridge") // there could be more than one deflection check artifact per girder (one for each span)
   const pgsDeflectionCheckArtifact* pDefl = pGdrArtifact->GetDeflectionCheckArtifact(0);
   Float64 min,max;
   pDefl->GetDemand(&min,&max);
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122006, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(min, WBFL::Units::Measure::Millimeter)) <<_T(", 2, ")<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122007, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pDefl->GetCapacity(), WBFL::Units::Measure::Millimeter)) <<_T(", 2, ")<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122008, ")<<-1<<_T(", ")<<(int)(pDefl->Passed()?1:0)<<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;

   const auto pBurst = pSegmentArtifact->GetStirrupCheckArtifact()->GetSplittingCheckArtifact();
   if (pBurst)
   {
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122010, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pBurst->GetSplittingZoneLength(pgsTypes::metStart), WBFL::Units::Measure::Millimeter)) <<_T(", 2, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122011, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pBurst->GetSplittingForce(pgsTypes::metStart), WBFL::Units::Measure::Newton)) <<_T(", 2, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122012, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pBurst->GetSplittingResistance(pgsTypes::metStart), WBFL::Units::Measure::Newton)) <<_T(", 2, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122013, ")<<-1<<_T(", ")<<(int)(pBurst->Passed()?1:0)<<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
   }

   const pgsHoldDownForceArtifact* pHoldDownForce = pSegmentArtifact->GetHoldDownForceArtifact();
   resultsFile<<bridgeId<<", "<<pid<<", 122029, "<<-1<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pHoldDownForce->GetDemand(), WBFL::Units::Measure::Newton)) <<", 2, "<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 122030, "<<-1<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pHoldDownForce->GetCapacity(), WBFL::Units::Measure::Newton)) <<", 2, "<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 122031, "<<-1<<", "<<(int)(pHoldDownForce->Passed()?1:0)<<", 15, "<<SEGMENT(segmentKey)<<std::endl;

   const pgsPrecastIGirderDetailingArtifact* pBeamDetails = pSegmentArtifact->GetPrecastIGirderDetailingArtifact();
   resultsFile<<bridgeId<<", "<<pid<<", 122032, "<<-1<<", "<<(int)(pBeamDetails->Passed()?1:0)<<", 15, "<<SEGMENT(segmentKey)<<std::endl;

   const pgsStrandSlopeArtifact* pSlope = pSegmentArtifact->GetStrandSlopeArtifact();
   resultsFile<<bridgeId<<", "<<pid<<", 122033, "<<-1<<", "<< QUIET(pSlope->GetDemand()) <<", 2, "<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 122034, "<<-1<<", "<< QUIET(pSlope->GetCapacity()) <<", 2, "<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 122035, "<<-1<<", "<<(int)(pSlope->Passed()?1:0)<<", 15, "<<SEGMENT(segmentKey)<<std::endl;

   const pgsStrandStressArtifact* sStrand = pSegmentArtifact->GetStrandStressArtifact();
   resultsFile<<bridgeId<<", "<<pid<<", 122036, "<<-1<<", "<<(int)(sStrand->Passed()?1:0)<<", 15, "<<SEGMENT(segmentKey)<<std::endl;

   // next poi-related values

   // This complex method of getting POIs is to match the POIs used in the regression test
   // from previous version of PGSuper (Versions 2.x). By making the vector of POI match
   // the older versions the same regression test results are reported making it easier to
   // compare results.
   PoiList vPoi;
   pIPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT, &vPoi);
   PoiList vPoi2;
   pIPoi->GetPointsOfInterest(segmentKey, POI_RELEASED_SEGMENT, &vPoi2);
   pIPoi->RemovePointsOfInterest(vPoi2,POI_RELEASED_SEGMENT | POI_0L);
   pIPoi->RemovePointsOfInterest(vPoi2,POI_RELEASED_SEGMENT | POI_10L);
   PoiList vPoi3;
   pIPoi->GetPointsOfInterest(segmentKey, POI_CRITSECTSHEAR1 | POI_PSXFER | POI_HARPINGPOINT | POI_BARDEVELOP | POI_STIRRUP_ZONE | POI_CONCLOAD | POI_DIAPHRAGM, &vPoi3);
   vPoi.insert(vPoi.end(),vPoi2.begin(),vPoi2.end());
   vPoi.insert(vPoi.end(),vPoi3.begin(),vPoi3.end());
   pIPoi->SortPoiList(&vPoi);

   const pgsStirrupCheckAtPoisArtifact* psArtifact = nullptr;
   const pgsLongReinfShearArtifact* pArtifact = nullptr;
   const pgsStirrupDetailArtifact* pSDArtifact = nullptr;
   const pgsHorizontalShearArtifact* pAHsrtifact = nullptr;

   for(const pgsPointOfInterest& poi : vPoi)
   {
       psArtifact = nullptr;
       pArtifact = nullptr;
       pSDArtifact = nullptr;
       pAHsrtifact = nullptr;

       Float64 loc = WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart(), WBFL::Units::Measure::Millimeter);

      // write to poi file
      poiFile<<" 1, "<< bridgeId<< ", 3, 1, "<<loc<<", 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0"<<std::endl;

      //resultsFile<<bridgeId<<", "<<pid<<", 50006, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetFinal(poi,pgsTypes::Permanent), WBFL::Units::Measure::MPa)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      //resultsFile<<bridgeId<<", "<<pid<<", 50007, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetElasticShortening(poi,pgsTypes::Permanent), WBFL::Units::Measure::MPa)) <<", 2, 15, "<<SEGMENT(segmentKey)<<std::endl;

      // The 12-50 test id's for prestress losses are totally inadequate for the current LRFD. New tests are implemented
      // that are more "generic". 550xx are for permanent strands and 551xx are for temporary strands
//      LOSSDETAILS losses = pLosses->GetLossDetails(poi);
//      resultsFile<<bridgeId<<", "<<pid<<", 50008, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(losses.RefinedLosses.ShrinkageLosses(), WBFL::Units::Measure::MPa)) <<         ", 15, "<<SEGMENT(segmentKey)<<std::endl;
//      resultsFile<<bridgeId<<", "<<pid<<", 50009, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(losses.RefinedLosses.CreepLosses(), WBFL::Units::Measure::MPa)) <<              ", 15, "<<SEGMENT(segmentKey)<<std::endl;
//      resultsFile<<bridgeId<<", "<<pid<<", 50010, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(losses.RefinedLosses.RelaxationLossesAtXfer(), WBFL::Units::Measure::MPa)) <<   ", 15, "<<SEGMENT(segmentKey)<<std::endl;
//      resultsFile<<bridgeId<<", "<<pid<<", 50011, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(losses.RefinedLosses.RelaxationLossesAfterXfer(), WBFL::Units::Measure::MPa)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
//      resultsFile<<bridgeId<<", "<<pid<<", 50012, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(losses.RefinedLosses.ElasticShortening().GetFcgp(), WBFL::Units::Measure::MPa)) <<                 ", 15, "<<SEGMENT(segmentKey)<<std::endl;
//      resultsFile<<bridgeId<<", "<<pid<<", 50013, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(losses.RefinedLosses.GetEci(), WBFL::Units::Measure::MPa)) <<                  ", 15, "<<SEGMENT(segmentKey)<<std::endl;
//
//      // DUMMY VALUE... Fix Test!!!
//      resultsFile<<bridgeId<<", "<<pid<<", 50014, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(losses.RefinedLosses.GetDeltaFcdp(), WBFL::Units::Measure::MPa)) <<             ", 15, "<<SEGMENT(segmentKey)<<std::endl;

// loss in permanent strands
      resultsFile<<bridgeId<<", "<<pid<<", 55001, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetElasticShortening(poi,pgsTypes::Permanent),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55002, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Permanent,stressStrandsIntervalIdx,pgsTypes::End),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55003, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Permanent,releaseIntervalIdx,pgsTypes::Start),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55004, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Permanent,liftSegmentIntervalIdx,pgsTypes::Start),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55005, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Permanent,haulSegmentIntervalIdx,pgsTypes::Start),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55006, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Permanent,tsInstallIntervalIdx,pgsTypes::Start),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55007, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Permanent,erectSegmentIntervalIdx,pgsTypes::End),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55008, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Permanent,tsRemovalIntervalIdx,pgsTypes::Start),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55009, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Permanent,noncompositeIntervalIdx,pgsTypes::Start),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55009a,"<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Permanent, trafficBarrierIntervalIdx,pgsTypes::Start),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55010, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Permanent,liveLoadIntervalIdx,pgsTypes::Start),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
// loss in temporary strands
      resultsFile<<bridgeId<<", "<<pid<<", 55101, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetElasticShortening(poi,pgsTypes::Temporary),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55102, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Temporary,stressStrandsIntervalIdx,pgsTypes::End),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55103, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Temporary,releaseIntervalIdx,pgsTypes::Start),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55104, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Temporary,liftSegmentIntervalIdx,pgsTypes::Start),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55105, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Temporary,haulSegmentIntervalIdx,pgsTypes::Start),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55106, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::Start),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55107, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Temporary,erectSegmentIntervalIdx,pgsTypes::End),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55108, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Temporary,tsRemovalIntervalIdx,pgsTypes::Start),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55109, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Temporary, noncompositeIntervalIdx,pgsTypes::Start),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55109a,"<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Temporary, trafficBarrierIntervalIdx,pgsTypes::Start),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 55110, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pLosses->GetEffectivePrestressLoss(poi,pgsTypes::Temporary,liveLoadIntervalIdx,pgsTypes::Start),WBFL::Units::Measure::MPa)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;

      // eff flange width
      resultsFile<<bridgeId<<", "<<pid<<", 50001, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pSp2->GetEffectiveFlangeWidth(poi), WBFL::Units::Measure::Millimeter)) <<", 2, "<<SEGMENT(segmentKey)<<std::endl;

      // force and stress in prestressing strands
      resultsFile<<bridgeId<<", "<<pid<<", 50002, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,releaseIntervalIdx,pgsTypes::Start), WBFL::Units::Measure::MPa))    << ", 15, " << SEGMENT(segmentKey) << std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50003, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pPrestressForce->GetPrestressForce(    poi,pgsTypes::Permanent,releaseIntervalIdx,pgsTypes::Start, pgsTypes::tltMinimum), WBFL::Units::Measure::Newton)) << ", 15, " << SEGMENT(segmentKey) << std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50004, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,liveLoadIntervalIdx,pgsTypes::Middle), WBFL::Units::Measure::MPa))    << ", 15, " << SEGMENT(segmentKey) << std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50005, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pPrestressForce->GetPrestressForce(    poi,pgsTypes::Permanent,liveLoadIntervalIdx,pgsTypes::Middle, pgsTypes::tltMinimum), WBFL::Units::Measure::Newton)) << ", 15, " << SEGMENT(segmentKey) << std::endl;

      // stresses due to external loads
      // casting yards
      pLsForces->GetStress(releaseIntervalIdx, pgsTypes::ServiceI, poi, bat, true, pgsTypes::TopGirder, &min,&max);
      resultsFile<<bridgeId<<", "<<pid<<", 50018, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(min , WBFL::Units::Measure::MPa)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      pLsForces->GetStress(releaseIntervalIdx, pgsTypes::ServiceI, poi, bat, true, pgsTypes::BottomGirder,&min,&max);
      resultsFile<<bridgeId<<", "<<pid<<", 50019, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(min , WBFL::Units::Measure::MPa)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;

      // bridge site 2
      pLsForces->GetStress(trafficBarrierIntervalIdx, pgsTypes::ServiceI, poi, bat, true, pgsTypes::TopGirder, &min,&max);
      resultsFile<<bridgeId<<", "<<pid<<", 50020, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(min , WBFL::Units::Measure::MPa)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;

      // bridge site 3
      pLsForces->GetStress(liveLoadIntervalIdx, pgsTypes::ServiceI, poi, bat, true, pgsTypes::TopGirder, &min,&max);
      resultsFile<<bridgeId<<", "<<pid<<", 50021, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(min , WBFL::Units::Measure::MPa)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      pLsForces->GetStress(liveLoadIntervalIdx, pgsTypes::ServiceI, poi, bat, true, pgsTypes::BottomGirder, &min,&max);
      resultsFile<<bridgeId<<", "<<pid<<", 50022, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(max , WBFL::Units::Measure::MPa)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      pLsForces->GetStress(liveLoadIntervalIdx, pgsTypes::ServiceI, poi, bat, true, pgsTypes::TopDeck, &min,&max);
      resultsFile<<bridgeId<<", "<<pid<<", 50023, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(min , WBFL::Units::Measure::MPa)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;

      pLsForces->GetStress(liveLoadIntervalIdx, pgsTypes::ServiceIII, poi, bat, true, pgsTypes::TopGirder, &min,&max);
      resultsFile<<bridgeId<<", "<<pid<<", 50024, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(min , WBFL::Units::Measure::MPa)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      pLsForces->GetStress(liveLoadIntervalIdx, pgsTypes::ServiceIII, poi, bat, true, pgsTypes::BottomGirder, &min,&max);
      resultsFile<<bridgeId<<", "<<pid<<", 50025, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(max , WBFL::Units::Measure::MPa)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      pLsForces->GetStress(liveLoadIntervalIdx, pgsTypes::ServiceIII, poi, bat, true, pgsTypes::TopDeck, &min,&max);
      resultsFile<<bridgeId<<", "<<pid<<", 50026, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(min , WBFL::Units::Measure::MPa)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;

      if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
      {
         pLsForces->GetStress(liveLoadIntervalIdx, pgsTypes::ServiceIA, poi, bat, true, pgsTypes::TopDeck, &min,&max);
         resultsFile<<bridgeId<<", "<<pid<<", 50027, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(min , WBFL::Units::Measure::MPa)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      }
      else
      {
         pLsForces->GetStress(liveLoadIntervalIdx, pgsTypes::FatigueI, poi, bat, true, pgsTypes::TopDeck, &min,&max);
         resultsFile<<bridgeId<<", "<<pid<<", 50027, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(min , WBFL::Units::Measure::MPa)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      }

      // stresses due to prestress
      resultsFile<<bridgeId<<", "<<pid<<", 56018, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pPrestressStresses->GetStress(releaseIntervalIdx,poi,pgsTypes::TopGirder,true/*include live load if applicable*/,pgsTypes::ServiceIII,INVALID_INDEX/*controlling live load*/), WBFL::Units::Measure::MPa)) <<         ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 56019, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pPrestressStresses->GetStress(releaseIntervalIdx,poi,pgsTypes::BottomGirder,true/*include live load if applicable*/, pgsTypes::ServiceIII, INVALID_INDEX/*controlling live load*/), WBFL::Units::Measure::MPa)) <<         ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 56020, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pPrestressStresses->GetStress(noncompositeIntervalIdx,poi,pgsTypes::TopGirder,true/*include live load if applicable*/, pgsTypes::ServiceIII, INVALID_INDEX/*controlling live load*/), WBFL::Units::Measure::MPa)) <<         ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 56021, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pPrestressStresses->GetStress(noncompositeIntervalIdx,poi,pgsTypes::BottomGirder,true/*include live load if applicable*/, pgsTypes::ServiceIII, INVALID_INDEX/*controlling live load*/), WBFL::Units::Measure::MPa)) <<         ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 56022, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pPrestressStresses->GetStress(trafficBarrierIntervalIdx,poi,pgsTypes::TopGirder,true/*include live load if applicable*/, pgsTypes::ServiceIII, INVALID_INDEX/*controlling live load*/), WBFL::Units::Measure::MPa)) <<         ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 56023, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pPrestressStresses->GetStress(trafficBarrierIntervalIdx,poi,pgsTypes::BottomGirder,true/*include live load if applicable*/, pgsTypes::ServiceIII, INVALID_INDEX/*controlling live load*/), WBFL::Units::Measure::MPa)) <<         ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 56024, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pPrestressStresses->GetStress(liveLoadIntervalIdx,poi,pgsTypes::TopGirder,true/*include live load if applicable*/, pgsTypes::ServiceIII, INVALID_INDEX/*controlling live load*/), WBFL::Units::Measure::MPa)) <<         ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 56025, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pPrestressStresses->GetStress(liveLoadIntervalIdx,poi,pgsTypes::BottomGirder,true/*include live load if applicable*/, pgsTypes::ServiceIII, INVALID_INDEX/*controlling live load*/), WBFL::Units::Measure::MPa)) <<         ", 15, "<<SEGMENT(segmentKey)<<std::endl;

      // positive moment capacity
      const CRACKINGMOMENTDETAILS* pcmd = pMomentCapacity->GetCrackingMomentDetails(liveLoadIntervalIdx,poi,true);
      resultsFile<<bridgeId<<", "<<pid<<", 50028, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pcmd->Mcr , WBFL::Units::Measure::NewtonMillimeter)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;

      const MINMOMENTCAPDETAILS* pmmcd = pMomentCapacity->GetMinMomentCapacityDetails(liveLoadIntervalIdx,poi,true);
      resultsFile<<bridgeId<<", "<<pid<<", 50029, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pmmcd->Mr , WBFL::Units::Measure::NewtonMillimeter)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;

      const MOMENTCAPACITYDETAILS* pmcd = pMomentCapacity->GetMomentCapacityDetails(liveLoadIntervalIdx,poi,true);
      resultsFile<<bridgeId<<", "<<pid<<", 50030, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pmcd->Mn , WBFL::Units::Measure::NewtonMillimeter)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50035, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pmcd->c, WBFL::Units::Measure::Millimeter)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50036, "<<loc<<", "<< QUIET(IsZero(pmcd->de, 0.001) ? 99999 : pmcd->c/pmcd->de) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50039, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pmcd->de, WBFL::Units::Measure::Millimeter)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50040, "<<loc<<", "<< QUIET(IsZero(pmmcd->Mcr, 0.001) ? 99999 : pmmcd->Mr/pmmcd->Mcr)<<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50041, "<<loc<<", "<< QUIET(IsZero(pmmcd->Mu, 0.001) ? 99999 : pmmcd->Mr/pmmcd->Mu)<< ", 15, "<<SEGMENT(segmentKey)<<std::endl;

      const pgsFlexuralCapacityArtifact* pCompositeCap;
      pCompositeCap = pGdrArtifact->FindPositiveMomentFlexuralCapacityArtifact(liveLoadIntervalIdx,pgsTypes::StrengthI,poi);
      if ( pCompositeCap )
      {
         ATLASSERT(pCompositeCap->GetPointOfInterest() == poi);
         resultsFile<<bridgeId<<", "<<pid<<", 122016, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pCompositeCap->GetDemand() , WBFL::Units::Measure::NewtonMillimeter)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 122017, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pCompositeCap->GetCapacity() , WBFL::Units::Measure::NewtonMillimeter)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 122018, "<<loc<<", "<<(int)(pCompositeCap->Passed()?1:0)<<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      }

      // negative moment capacity
      pcmd = pMomentCapacity->GetCrackingMomentDetails(liveLoadIntervalIdx,poi,false);
      resultsFile<<bridgeId<<", "<<pid<<", 50128, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pcmd->Mcr , WBFL::Units::Measure::NewtonMillimeter)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;

      pmmcd = pMomentCapacity->GetMinMomentCapacityDetails(liveLoadIntervalIdx,poi,false);
      resultsFile<<bridgeId<<", "<<pid<<", 50129, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pmmcd->Mr , WBFL::Units::Measure::NewtonMillimeter)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;

      pmcd = pMomentCapacity->GetMomentCapacityDetails(liveLoadIntervalIdx,poi,false);
      resultsFile<<bridgeId<<", "<<pid<<", 50130, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pmcd->Mn , WBFL::Units::Measure::NewtonMillimeter)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50135, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pmcd->c, WBFL::Units::Measure::Millimeter)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50136, "<<loc<<", "<< QUIET(IsZero(pmcd->de, 0.001) ? 99999 : pmcd->c/pmcd->de) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50139, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pmcd->de, WBFL::Units::Measure::Millimeter)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50140, "<<loc<<", "<< QUIET(IsZero(pmmcd->Mcr, 0.001) ? 99999 : pmmcd->Mr/pmmcd->Mcr)<<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50141, "<<loc<<", "<< QUIET(IsZero(pmmcd->Mu, 0.001) ? 99999 : pmmcd->Mr/pmmcd->Mu)<< ", 15, "<<SEGMENT(segmentKey)<<std::endl;

      pCompositeCap = pGdrArtifact->FindNegativeMomentFlexuralCapacityArtifact(liveLoadIntervalIdx,pgsTypes::StrengthI,poi);
      if ( pCompositeCap )
      {
         ATLASSERT(pCompositeCap->GetPointOfInterest() == poi);
         // the bug is probably with the POIs
         resultsFile<<bridgeId<<", "<<pid<<", 122116, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pCompositeCap->GetDemand() , WBFL::Units::Measure::NewtonMillimeter)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 122117, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pCompositeCap->GetCapacity() , WBFL::Units::Measure::NewtonMillimeter)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
         resultsFile<<bridgeId<<", "<<pid<<", 122118, "<<loc<<", "<<(int)(pCompositeCap->Passed()?1:0)<<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      }


      // shear capacity
      SHEARCAPACITYDETAILS scd;
      pShearCapacity->GetShearCapacityDetails(pgsTypes::StrengthI, liveLoadIntervalIdx, poi, nullptr, &scd);
      resultsFile<<bridgeId<<", "<<pid<<", 50031, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(scd.Aps , WBFL::Units::Measure::Millimeter2)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50032, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(scd.As , WBFL::Units::Measure::Millimeter2)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50033, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pPrestressForce->GetEffectivePrestress(poi,pgsTypes::Permanent,erectSegmentIntervalIdx,pgsTypes::End), WBFL::Units::Measure::MPa)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50042, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(scd.pVn , WBFL::Units::Measure::Newton)) <<    ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50043, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(scd.Vn , WBFL::Units::Measure::Newton)) <<     ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50044, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(scd.Vc , WBFL::Units::Measure::Newton)) <<     ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50045, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(scd.Vs , WBFL::Units::Measure::Newton)) <<     ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      if (lrfdVersionMgr::NinthEdition2020 <= lrfdVersionMgr::GetVersion())
      {
         resultsFile << bridgeId << ", " << pid << ", 50045a, " << loc << ", " << QUIET(scd.lambda_duct) << ", 15, " << SEGMENT(segmentKey) << std::endl;
      }
      resultsFile<<bridgeId<<", "<<pid<<", 50046, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(scd.Vp , WBFL::Units::Measure::Newton)) <<     ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50047, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(scd.bv, WBFL::Units::Measure::Millimeter)) <<  ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50050, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(scd.Ac, WBFL::Units::Measure::Millimeter2)) << ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50051, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(scd.dv, WBFL::Units::Measure::Millimeter)) <<  ", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50053, "<<loc<<", "<< QUIET(scd.Beta) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50054, "<<loc<<", "<< QUIET(scd.ex) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50055, "<<loc<<", "<< QUIET(scd.Fe) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50056, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(scd.Theta, WBFL::Units::Measure::Degree)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50057, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(scd.fpops, WBFL::Units::Measure::MPa)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<", "<<pid<<", 50058, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(scd.fpeps, WBFL::Units::Measure::MPa)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
      resultsFile << bridgeId << ", " << pid << ", 50059s, " << loc << ", " << QUIET(WBFL::Units::ConvertFromSysUnits(scd.fpoptSegment, WBFL::Units::Measure::MPa)) << ", 15, " << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << ", " << pid << ", 50060s, " << loc << ", " << QUIET(WBFL::Units::ConvertFromSysUnits(scd.fpeptSegment, WBFL::Units::Measure::MPa)) << ", 15, " << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << ", " << pid << ", 50059g, " << loc << ", " << QUIET(WBFL::Units::ConvertFromSysUnits(scd.fpoptGirder, WBFL::Units::Measure::MPa)) << ", 15, " << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << ", " << pid << ", 50060g, " << loc << ", " << QUIET(WBFL::Units::ConvertFromSysUnits(scd.fpeptGirder, WBFL::Units::Measure::MPa)) << ", 15, " << SEGMENT(segmentKey) << std::endl;

      psArtifact = pstirrup_artifact->GetStirrupCheckAtPoisArtifactAtPOI( liveLoadIntervalIdx, pgsTypes::StrengthI, poi.GetID() );
      if ( psArtifact )
      {
         ATLASSERT(poi == psArtifact->GetPointOfInterest());
         pArtifact = psArtifact->GetLongReinfShearArtifact();
         if ( pArtifact )
         {
            ATLASSERT(IsEqual(WBFL::Units::ConvertFromSysUnits(psArtifact->GetPointOfInterest().GetDistFromStart(), WBFL::Units::Measure::Millimeter),loc));
            resultsFile<<bridgeId<<", "<<pid<<", 50061, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pArtifact->GetDemandForce(), WBFL::Units::Measure::Newton)) <<  ", 15, "<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<", "<<pid<<", 50062, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pArtifact->GetCapacityForce(), WBFL::Units::Measure::Newton)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;

            pSDArtifact = psArtifact->GetStirrupDetailArtifact();
            resultsFile<<bridgeId<<", "<<pid<<", 50063, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pSDArtifact->GetSMax(), WBFL::Units::Measure::Millimeter)) <<   ", 15, "<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<", "<<pid<<", 50064, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pSDArtifact->GetAvsMin(), WBFL::Units::Measure::Millimeter2)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<", "<<pid<<", 50065, "<<loc<<", "<< QUIET(IsZero(scd.Vu,0.001) ? 99999 : scd.Vn/scd.Vu) <<", 15 "<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100205, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSDArtifact->GetS(), WBFL::Units::Measure::Millimeter)) <<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
            if ( pSDArtifact->GetAfter1999() )
            {
               resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100206, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSDArtifact->GetVuLimit(), WBFL::Units::Measure::Newton)) <<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
            }
            else
            {
               resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100206, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSDArtifact->GetvuLimit(), WBFL::Units::Measure::MPa)) <<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
            }

            pAHsrtifact = psArtifact->GetHorizontalShearArtifact();
            resultsFile<<bridgeId<<", "<<pid<<", 50067, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pAHsrtifact->GetCapacity()*pAHsrtifact->GetPhi(), WBFL::Units::Measure::Newton)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<", "<<pid<<", 50068, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pAHsrtifact->GetCapacity(), WBFL::Units::Measure::Newton)) <<", 15, "<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100207, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pAHsrtifact->GetDemand(), WBFL::Units::Measure::Newton)) <<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;

            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50069, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pAHsrtifact->GetAcv(), WBFL::Units::Measure::Millimeter2)) <<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50070, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pAHsrtifact->GetAvOverS(), WBFL::Units::Measure::Millimeter2)) <<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 50071, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pAHsrtifact->GetNormalCompressionForce(), WBFL::Units::Measure::Newton)) <<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100208, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pAHsrtifact->GetSpacing(), WBFL::Units::Measure::Millimeter)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100209, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pAHsrtifact->GetAvOverSReqd(), WBFL::Units::Measure::Millimeter)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;

            const pgsVerticalShearArtifact* pVertical = psArtifact->GetVerticalShearArtifact();
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100210, ")<<loc<<_T(", ")<<(int)(pVertical->IsStrutAndTieRequired()?1:0)<<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100212, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pVertical->GetAvOverSReqd(), WBFL::Units::Measure::Millimeter)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100213, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pVertical->GetDemand(), WBFL::Units::Measure::Newton)) <<  _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
            resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100214, ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pVertical->GetCapacity(), WBFL::Units::Measure::Newton)) <<  _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
         }
      }

      // pass/fail cases
      if (pArtifact && pArtifact->IsApplicable())
         resultsFile<<bridgeId<<", "<<pid<<", 100202, "<<loc<<", "<<(int)(pArtifact->Passed()?1:0)<<", 15, "<<SEGMENT(segmentKey)<<std::endl;

      if (pSDArtifact)
         resultsFile<<bridgeId<<", "<<pid<<", 100203, "<<loc<<", "<<(int)(pSDArtifact->Passed()?1:0)<<", 15, "<<SEGMENT(segmentKey)<<std::endl;

      if (pAHsrtifact)
         resultsFile<<bridgeId<<", "<<pid<<", 100204, "<<loc<<", "<<(int)(pAHsrtifact->Passed()?1:0)<<", 15, "<<SEGMENT(segmentKey)<<std::endl;

#pragma Reminder("Update reg tests to use actual tasks done, rather than a guess at the tasks")
      // MUST DO THIS AFTER ALL OTHER CHANGES ARE DONE AND THE REG TESTS ARE CLEAN. CAN'T CHANGE REG TEST OUTPUT FORMAT
      // UNLESS TEST RESULTS ARE CLEAN FIRST, OTHERWISE YOU'LL NEVER KNOW IF CHANGES ARE BUGS OR A RESULT OF THE FORMAT CHANGE
      StressCheckTask task;
      task.intervalIdx = noncompositeIntervalIdx;
      task.limitState = pgsTypes::ServiceI;
      task.stressType = pgsTypes::Tension;
      const pgsFlexuralStressArtifact* pStressArtifact;
      pStressArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(task,poi.GetID());
      if ( pStressArtifact )
      {
         ATLASSERT(IsEqual(loc,WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart(), WBFL::Units::Measure::Millimeter)));
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122019, ")<<loc<<_T(", ")<<(int)(pStressArtifact->BeamPassed()?1:0)<<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      }

      task.intervalIdx = lastIntervalIdx;
      task.limitState = pgsTypes::ServiceIII;
      task.stressType = pgsTypes::Tension;
      task.bIncludeLiveLoad = true;
      pStressArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(task,poi.GetID());
      if ( pStressArtifact )
      {
         ATLASSERT(IsEqual(loc,WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart(), WBFL::Units::Measure::Millimeter)));
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122023, ")<<loc<<_T(", ")<<(int)(pStressArtifact->Passed(pgsTypes::TopGirder)?1:0)<<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      }

      task.intervalIdx = noncompositeIntervalIdx;
      task.limitState = pgsTypes::ServiceI;
      task.stressType = pgsTypes::Compression;
      task.bIncludeLiveLoad = true;
      pStressArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(task,poi.GetID());
      if ( pStressArtifact )
      {
         ATLASSERT(IsEqual(loc,WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart(), WBFL::Units::Measure::Millimeter)));
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122024, ")<<loc<<_T(", ")<<(int)(pStressArtifact->BeamPassed()?1:0)<<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      }

      task.intervalIdx = lastIntervalIdx;
      task.limitState = pgsTypes::ServiceI;
      task.stressType = pgsTypes::Compression;
      task.bIncludeLiveLoad = false;
      pStressArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(task,poi.GetID());
      if ( pStressArtifact )
      {
         ATLASSERT(IsEqual(loc,WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart(), WBFL::Units::Measure::Millimeter)));
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122025, ")<<loc<<_T(", ")<<(int)(pStressArtifact->BeamPassed()?1:0)<<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      }

      task.intervalIdx = lastIntervalIdx;
      task.limitState = pgsTypes::ServiceI;
      task.stressType = pgsTypes::Compression;
      task.bIncludeLiveLoad = true;
      pStressArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(task,poi.GetID());
      if ( pStressArtifact )
      {
         ATLASSERT(IsEqual(loc,WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart(), WBFL::Units::Measure::Millimeter)));
         resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 122026, ")<<loc<<_T(", ")<<(int)(pStressArtifact->BeamPassed()?1:0)<<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      }

      task.intervalIdx = lastIntervalIdx;
      task.limitState = (lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims ? pgsTypes::ServiceIA : pgsTypes::FatigueI);
      task.stressType = pgsTypes::Compression;
      task.bIncludeLiveLoad = true;
      pStressArtifact = pSegmentArtifact->GetFlexuralStressArtifactAtPoi(task, poi.GetID());
      if (pStressArtifact)
      {
         ATLASSERT(IsEqual(loc, WBFL::Units::ConvertFromSysUnits(poi.GetDistFromStart(), WBFL::Units::Measure::Millimeter)));
         resultsFile << bridgeId << _T(", ") << pid << _T(", 122027, ") << loc << _T(", ") << (int)(pStressArtifact->BeamPassed() ? 1 : 0) << _T(", 15, ") << SEGMENT(segmentKey) << std::endl;
      }
   } // next POI

   // confinement
   const pgsConfinementCheckArtifact& rconf = pstirrup_artifact->GetConfinementArtifact();
   if (rconf.IsApplicable())
   {
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100220, ")<<-1<<_T(", ")<<(int)(rconf.Passed()?1:0)<<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100221, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(rconf.GetSMax(), WBFL::Units::Measure::Millimeter)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100222, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(rconf.GetStartProvidedZoneLength(), WBFL::Units::Measure::Millimeter)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100223, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(rconf.GetStartRequiredZoneLength(), WBFL::Units::Measure::Millimeter)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100224, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(rconf.GetStartS(), WBFL::Units::Measure::Millimeter)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100225, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(rconf.GetEndProvidedZoneLength(), WBFL::Units::Measure::Millimeter)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100226, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(rconf.GetEndRequiredZoneLength(), WBFL::Units::Measure::Millimeter)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100227, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(rconf.GetEndS(), WBFL::Units::Measure::Millimeter)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
   }

   // splitting / bursting
   const auto pSplit = pstirrup_artifact->GetSplittingCheckArtifact();
   if(pSplit)
   {
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100230, ")<<-1<<_T(", ")<<(int)(pSplit->Passed()?1:0)<<_T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100231, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetAps(pgsTypes::metStart,pgsTypes::Permanent) + pSplit->GetAps(pgsTypes::metStart, pgsTypes::Temporary), WBFL::Units::Measure::Millimeter2)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100232, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetAvs(pgsTypes::metStart), WBFL::Units::Measure::Millimeter)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100233, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetFpj(pgsTypes::metStart,pgsTypes::Permanent), WBFL::Units::Measure::Newton)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100234, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetFs(pgsTypes::metStart), WBFL::Units::Measure::MPa)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100235, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetH(pgsTypes::metStart), WBFL::Units::Measure::Millimeter)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100236, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetLossesAfterTransfer(pgsTypes::metStart,pgsTypes::Permanent), WBFL::Units::Measure::MPa)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100237, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetSplittingForce(pgsTypes::metStart), WBFL::Units::Measure::Newton)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100238, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetSplittingResistance(pgsTypes::metStart), WBFL::Units::Measure::Newton)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100239, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetSplittingZoneLength(pgsTypes::metStart), WBFL::Units::Measure::Millimeter)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100241, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetAps(pgsTypes::metEnd, pgsTypes::Permanent) + pSplit->GetAps(pgsTypes::metEnd, pgsTypes::Temporary), WBFL::Units::Measure::Millimeter2)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100242, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetAvs(pgsTypes::metEnd), WBFL::Units::Measure::Millimeter)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100243, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetFpj(pgsTypes::metStart, pgsTypes::Permanent), WBFL::Units::Measure::Newton)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100244, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetFs(pgsTypes::metEnd), WBFL::Units::Measure::MPa)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100245, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetH(pgsTypes::metEnd), WBFL::Units::Measure::Millimeter)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100246, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetLossesAfterTransfer(pgsTypes::metEnd,pgsTypes::Permanent), WBFL::Units::Measure::MPa)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100247, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetSplittingForce(pgsTypes::metEnd), WBFL::Units::Measure::Newton)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100248, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetSplittingResistance(pgsTypes::metEnd), WBFL::Units::Measure::Newton)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 100249, ")<<-1<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(pSplit->GetSplittingZoneLength(pgsTypes::metEnd), WBFL::Units::Measure::Millimeter)) <<   _T(", 15, ")<<SEGMENT(segmentKey)<<std::endl;
   }

   const pgsPrincipalTensionStressArtifact* pWebStressArtifact = pGdrArtifact->GetSegmentArtifact(segmentKey.segmentIndex)->GetPrincipalTensionStressArtifact();
   if (pWebStressArtifact && pWebStressArtifact->IsApplicable())
   {
      GET_IFACE(IPrincipalWebStress, pPrincipalWebStress);
      const std::vector<pgsPrincipalTensionSectionArtifact>* pvWebStressSections = pWebStressArtifact->GetPrincipalTensionStressArtifacts();
      int i = 0;
      for (const auto& artifact : *pvWebStressSections)
      {
         Float64 loc = WBFL::Units::ConvertFromSysUnits(artifact.GetPointOfInterest().GetDistFromStart(), WBFL::Units::Measure::Millimeter);
         resultsFile << bridgeId << _T(", ") << pid << _T(", 100300, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(artifact.Getfmax(), WBFL::Units::Measure::MPa)) << _T(", ") << i << _T(", ") << SEGMENT(segmentKey) << std::endl;
         resultsFile << bridgeId << _T(", ") << pid << _T(", 100301, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(artifact.GetWebElevation(), WBFL::Units::Measure::Millimeter)) << _T(", ") << i << _T(", ") << SEGMENT(segmentKey) << std::endl;
         resultsFile << bridgeId << _T(", ") << pid << _T(", 100302, ") << loc << _T(", ") << artifact.Passed() << _T(", ") << i << _T(", ") << SEGMENT(segmentKey) << std::endl;

         const PRINCIPALSTRESSINWEBDETAILS* pDetails = pPrincipalWebStress->GetPrincipalWebStressDetails(artifact.GetPointOfInterest());

         for (const auto& section_details : pDetails->WebSections)
         {
            resultsFile << bridgeId << _T(", ") << pid << _T(", 100303, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(section_details.YwebSection, WBFL::Units::Measure::Millimeter)) << _T(", ") << i << _T(", ") << SEGMENT(segmentKey) << std::endl;
            resultsFile << bridgeId << _T(", ") << pid << _T(", 100304, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(section_details.fTop, WBFL::Units::Measure::MPa)) << _T(", ") << i << _T(", ") << SEGMENT(segmentKey) << std::endl;
            resultsFile << bridgeId << _T(", ") << pid << _T(", 100305, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(section_details.fBot, WBFL::Units::Measure::MPa)) << _T(", ") << i << _T(", ") << SEGMENT(segmentKey) << std::endl;
            resultsFile << bridgeId << _T(", ") << pid << _T(", 100306, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(section_details.fpcx, WBFL::Units::Measure::MPa)) << _T(", ") << i << _T(", ") << SEGMENT(segmentKey) << std::endl;
            resultsFile << bridgeId << _T(", ") << pid << _T(", 100307, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(section_details.bw, WBFL::Units::Measure::Millimeter)) << _T(", ") << i << _T(", ") << SEGMENT(segmentKey) << std::endl;
            resultsFile << bridgeId << _T(", ") << pid << _T(", 100308, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(section_details.Qnc, WBFL::Units::Measure::Millimeter3)) << _T(", ") << i << _T(", ") << SEGMENT(segmentKey) << std::endl;
            resultsFile << bridgeId << _T(", ") << pid << _T(", 100309, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(section_details.Qc, WBFL::Units::Measure::Millimeter3)) << _T(", ") << i << _T(", ") << SEGMENT(segmentKey) << std::endl;
            resultsFile << bridgeId << _T(", ") << pid << _T(", 100310, ") << loc << _T(", ") << QUIET(WBFL::Units::ConvertFromSysUnits(section_details.t, WBFL::Units::Measure::MPa)) << _T(", ") << i << _T(", ") << SEGMENT(segmentKey) << std::endl;
         }

         i++;
      }

   }

   return true;
}

bool CTestAgentImp::RunHandlingTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, const CSegmentKey& segmentKey)
{
   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   // write to poi file
   poiFile<<" 1, "<< bridgeId<< ", 3, 1, 0.0000, 2, -1, -1, -1,  0,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0"<<std::endl;

   GET_IFACE(IArtifact,pArtifacts);
   const pgsSegmentArtifact* pArtifact = pArtifacts->GetSegmentArtifact(segmentKey);

   // lifting
   const WBFL::Stability::LiftingCheckArtifact* pLiftArtifact = pArtifact->GetLiftingCheckArtifact();
   if ( pLiftArtifact != nullptr )
   {
      const WBFL::Stability::LiftingResults& liftingResults = pLiftArtifact->GetLiftingResults();
      if ( !liftingResults.bIsStable[+WBFL::Stability::ImpactDirection::NoImpact][+WBFL::Stability::WindDirection::Left]  || !liftingResults.bIsStable[+WBFL::Stability::ImpactDirection::ImpactUp][+WBFL::Stability::WindDirection::Left]  || !liftingResults.bIsStable[+WBFL::Stability::ImpactDirection::ImpactDown][+WBFL::Stability::WindDirection::Left] ||
           !liftingResults.bIsStable[+WBFL::Stability::ImpactDirection::NoImpact][+WBFL::Stability::WindDirection::Right] || !liftingResults.bIsStable[+WBFL::Stability::ImpactDirection::ImpactUp][+WBFL::Stability::WindDirection::Right] || !liftingResults.bIsStable[+WBFL::Stability::ImpactDirection::ImpactDown][+WBFL::Stability::WindDirection::Right])
      {
         resultsFile<<"Girder is unstable for lifting"<<std::endl;
         return true;
      }

      resultsFile << bridgeId << _T(", ") << pid << _T(", 100001a, ") << QUIET(WBFL::Units::ConvertFromSysUnits(liftingResults.MaxDirectStress, WBFL::Units::Measure::MPa)) << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100001b, ") << liftingResults.MaxDirectStressAnalysisPointIndex << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100001c, ") << +liftingResults.MaxDirectStressImpactDirection << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100001d, ") << +liftingResults.MaxDirectStressCorner << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;

      resultsFile << bridgeId << _T(", ") << pid << _T(", 100002a, ") << QUIET(WBFL::Units::ConvertFromSysUnits(liftingResults.MinDirectStress, WBFL::Units::Measure::MPa)) << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100002b, ") << liftingResults.MinDirectStressAnalysisPointIndex << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100002c, ") << +liftingResults.MinDirectStressImpactDirection << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100002d, ") << +liftingResults.MinDirectStressCorner << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;

      resultsFile << bridgeId << _T(", ") << pid << _T(", 100003a, ") << QUIET(WBFL::Units::ConvertFromSysUnits(liftingResults.MaxStress, WBFL::Units::Measure::MPa)) << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100003b, ") << liftingResults.MaxStressAnalysisPointIndex << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100003c, ") << +liftingResults.MaxStressImpactDirection << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100003d, ") << +liftingResults.MaxStressCorner << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;

      resultsFile << bridgeId << _T(", ") << pid << _T(", 100004a, ") << QUIET(WBFL::Units::ConvertFromSysUnits(liftingResults.MinStress, WBFL::Units::Measure::MPa)) << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100004b, ") << liftingResults.MinStressAnalysisPointIndex << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100004c, ") << +liftingResults.MinStressImpactDirection << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100004d, ") << +liftingResults.MinStressCorner << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;

      resultsFile << bridgeId << _T(", ") << pid << _T(", 100005a, ") << liftingResults.FScrMin << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100005b, ") << liftingResults.FScrMinAnalysisPointIndex << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100005c, ") << +liftingResults.FScrMinImpactDirection << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100005d, ") << +liftingResults.FScrMinWindDirection << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100005e, ") << +liftingResults.FScrMinCorner << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;

      resultsFile << bridgeId << _T(", ") << pid << _T(", 100006a, ") << liftingResults.MinFsFailure << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100006b, ") << liftingResults.MinAdjFsFailure << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100006c, ") << +liftingResults.FSfImpactDirection << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
      resultsFile << bridgeId << _T(", ") << pid << _T(", 100006d, ") << +liftingResults.FSfWindDirection << _T(", 50, ") << SEGMENT(segmentKey) << std::endl;
   }

   // hauling
   const pgsHaulingAnalysisArtifact* pHaulArtifact = pArtifact->GetHaulingAnalysisArtifact();
   if (pHaulArtifact != nullptr)
   {
      // Artifact writes its own data
      pHaulArtifact->Write1250Data(segmentKey, resultsFile, poiFile, m_pBroker, pid, bridgeId);
   }

   return true;
}

bool CTestAgentImp::RunWsdotGirderScheduleTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, const CSegmentKey& segmentKey)
{
   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   GET_IFACE(IArtifact,pIArtifact);
   const pgsSegmentArtifact* pArtifact = pIArtifact->GetSegmentArtifact(segmentKey);

   GET_IFACE(ICamber,pCamber);
   // create pois at the start of girder and mid-span
   pgsPointOfInterest pois(segmentKey,0.0);

   GET_IFACE(IPointOfInterest, pPointOfInterest );
   PoiList vPoi;
   pPointOfInterest->GetPointsOfInterest(segmentKey, POI_5L | POI_ERECTED_SEGMENT, &vPoi);
   ATLASSERT(vPoi.size()==1);
   const pgsPointOfInterest& pmid(vPoi.front());

   Float64 loc = pmid.GetDistFromStart();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   CSpanKey spanKey;
   Float64 Xspan;
   pPointOfInterest->ConvertPoiToSpanPoint(pmid,&spanKey,&Xspan);

   GET_IFACE(IBridge, pBridge );
   CComPtr<IAngle> as1;
   pBridge->GetPierSkew(spanKey.spanIndex,&as1);
   Float64 s1;
   as1->get_Value(&s1);
   Float64 t1 = s1 + M_PI/2.0;
   resultsFile<<bridgeId<<", "<<pid<<", 123001, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(t1 , WBFL::Units::Measure::Degree)) <<", 101, "<<SEGMENT(segmentKey)<<std::endl;

   CComPtr<IAngle> as2;
   pBridge->GetPierSkew(spanKey.spanIndex+1,&as2);
   Float64 s2;
   as2->get_Value(&s2);
   Float64 t2 = s2 + M_PI/2.0;

   resultsFile<<bridgeId<<", "<<pid<<", 123002, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(t2 , WBFL::Units::Measure::Degree)) <<", 101, "<<SEGMENT(segmentKey)<<std::endl;

   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   Float64 endDist;
   ConnectionLibraryEntry::EndDistanceMeasurementType mtEndDist;
   pIBridgeDesc->GetPier(spanKey.spanIndex)->GetGirderEndDistance(pgsTypes::Ahead,&endDist,&mtEndDist);

   Float64 N1 = endDist;

   resultsFile<<bridgeId<<", "<<pid<<", 123003, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(N1, WBFL::Units::Measure::Millimeter)) <<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;

   pIBridgeDesc->GetPier(spanKey.spanIndex+1)->GetGirderEndDistance(pgsTypes::Back,&endDist,&mtEndDist);
   Float64 N2 = endDist;

   resultsFile<<bridgeId<<", "<<pid<<", 123004, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(N2, WBFL::Units::Measure::Millimeter)) <<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;

   Float64 P1 = N1 / sin(t1);
   resultsFile<<bridgeId<<", "<<pid<<", 123005, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(P1, WBFL::Units::Measure::Millimeter)) <<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;

   Float64 P2 = N2 / sin(t2);
   resultsFile<<bridgeId<<", "<<pid<<", 123006, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(P2, WBFL::Units::Measure::Millimeter)) <<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;

   resultsFile<<bridgeId<<", "<<pid<<", 123007, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pBridge->GetSegmentLength(segmentKey), WBFL::Units::Measure::Millimeter)) <<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;

   GET_IFACE(IMaterials, pMaterial);
   resultsFile<<bridgeId<<", "<<pid<<", 123008, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pMaterial->GetSegmentFc(segmentKey,liveLoadIntervalIdx), WBFL::Units::Measure::MPa)) <<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 123009, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pMaterial->GetSegmentFc(segmentKey,releaseIntervalIdx), WBFL::Units::Measure::MPa)) <<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;

   GET_IFACE(IStrandGeometry, pStrandGeometry );
   
   StrandIndexType nh = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Harped);
   resultsFile<<bridgeId<<", "<<pid<<", 123010, "<<loc<<", "<<nh<<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;

   Float64 hj = pStrandGeometry->GetPjack(segmentKey,pgsTypes::Harped);
   resultsFile<<bridgeId<<", "<<pid<<", 123011, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(hj, WBFL::Units::Measure::Newton)) <<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;

   StrandIndexType ns = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Straight);
   resultsFile<<bridgeId<<", "<<pid<<", 123012, "<<loc<<", "<<ns<<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;

   Float64 sj = pStrandGeometry->GetPjack(segmentKey,pgsTypes::Straight);
   resultsFile<<bridgeId<<", "<<pid<<", 123013, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(sj, WBFL::Units::Measure::Newton)) <<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;

   StrandIndexType nt = pStrandGeometry->GetStrandCount(segmentKey,pgsTypes::Temporary);
   resultsFile<<bridgeId<<", "<<pid<<", 123014, "<<loc<<", "<<nt<<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;

   Float64 tj = pStrandGeometry->GetPjack(segmentKey,pgsTypes::Temporary);
   resultsFile<<bridgeId<<", "<<pid<<", 123015, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(tj, WBFL::Units::Measure::Newton)) <<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;

   GET_IFACE(ISectionProperties, pSectProp );
   Float64 ybg = pSectProp->GetY(releaseIntervalIdx,pois,pgsTypes::BottomGirder);
   Float64 sse = pStrandGeometry->GetEccentricity(releaseIntervalIdx,pois, pgsTypes::Straight).Y();
   if (0 < ns)
   {
      resultsFile<<bridgeId<<", "<<pid<<", 123016, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(ybg-sse, WBFL::Units::Measure::Millimeter)) <<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;
   }

   Float64 hse = pStrandGeometry->GetEccentricity(releaseIntervalIdx,pmid,pgsTypes::Harped).Y();
   if (0 < nh)
   {
      resultsFile<<bridgeId<<", "<<pid<<", 123017, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(ybg-hse, WBFL::Units::Measure::Millimeter)) <<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;
   }

   // get location of first harped strand
   if (0 < nh)
   {
      const auto* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
      auto strandDefType = pSegment->Strands.GetStrandDefinitionType();
      if (IsGridBasedStrandModel(strandDefType)) // GetNextNumStrands and ComputeStrandFill don't work with these strand def types
      {
         StrandIndexType nns = pStrandGeometry->GetNextNumStrands(segmentKey, pgsTypes::Harped, 0);
         ConfigStrandFillVector fillvec = pStrandGeometry->ComputeStrandFill(segmentKey, pgsTypes::Harped, nns);
         GDRCONFIG config = pBridge->GetSegmentConfiguration(segmentKey);
         config.PrestressConfig.SetStrandFill(pgsTypes::Harped, fillvec);

         Float64 eh2 = pStrandGeometry->GetEccentricity(releaseIntervalIdx, pmid, pgsTypes::Harped, &config).Y();
         Float64 Fb = pSectProp->GetY(releaseIntervalIdx, pois, pgsTypes::BottomGirder) - eh2;
         resultsFile << bridgeId << ", " << pid << ", 123018, " << loc << ", " << QUIET(WBFL::Units::ConvertFromSysUnits(Fb, WBFL::Units::Measure::Millimeter)) << ", 101, " << SEGMENT(segmentKey) << std::endl;
      }
   }

   Float64 ytg = pSectProp->GetY(releaseIntervalIdx,pois,pgsTypes::TopGirder);
   Float64 hss = pStrandGeometry->GetEccentricity(releaseIntervalIdx,pois,pgsTypes::Harped).Y();
   if (0 < nh)
   {
      resultsFile<<bridgeId<<", "<<pid<<", 123019, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(ytg+hss, WBFL::Units::Measure::Millimeter)) <<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;
   }

   resultsFile<<bridgeId<<", "<<pid<<", 123020, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pCamber->GetScreedCamber( pmid, CREEP_MAXTIME ), WBFL::Units::Measure::Millimeter)) <<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;

   // get # of days for creep
   GET_IFACE(ISpecification, pSpec );
   GET_IFACE(ILibrary, pLib);
   std::_tstring spec_name = pSpec->GetSpecification();
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( spec_name.c_str() );

   Float64 days =  WBFL::Units::ConvertFromSysUnits(pSpecEntry->GetCreepDuration1Min(), WBFL::Units::Measure::Day);
   resultsFile<<bridgeId<<", "<<pid<<", 123021, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pSpecEntry->GetCreepDuration1Min(), WBFL::Units::Measure::Day)) <<   ", 101, "<<SEGMENT(segmentKey)<<std::endl;

   CREEPCOEFFICIENTDETAILS details = pCamber->GetCreepCoefficientDetails(segmentKey, ICamber::cpReleaseToFinal, CREEP_MAXTIME);
   resultsFile << bridgeId << ", " << pid << ", 123060," << QUIET(details.Ct) << SEGMENT(segmentKey) << std::endl;

   details = pCamber->GetCreepCoefficientDetails(segmentKey, ICamber::cpReleaseToDeck, CREEP_MAXTIME);
   resultsFile << bridgeId << ", " << pid << ", 123061," << QUIET(details.Ct) << SEGMENT(segmentKey) << std::endl;

   details = pCamber->GetCreepCoefficientDetails(segmentKey, ICamber::cpReleaseToDiaphragm, CREEP_MAXTIME);
   resultsFile << bridgeId << ", " << pid << ", 123062," << QUIET(details.Ct) << SEGMENT(segmentKey) << std::endl;

   if (IsStructuralDeck(pIBridgeDesc->GetDeckDescription()->GetDeckType()))
   {
       details = pCamber->GetCreepCoefficientDetails(segmentKey, ICamber::cpDiaphragmToDeck, CREEP_MAXTIME);
       resultsFile << bridgeId << ", " << pid << ", 123063," << QUIET(details.Ct) << SEGMENT(segmentKey) << std::endl;

       details = pCamber->GetCreepCoefficientDetails(segmentKey, ICamber::cpDeckToFinal, CREEP_MAXTIME);
       resultsFile << bridgeId << ", " << pid << ", 123064," << QUIET(details.Ct) << SEGMENT(segmentKey) << std::endl;
   }

   // Stirrup data
   GET_IFACE(IStirrupGeometry,pStirrupGeometry);

   // Primary zones
   Int32 id = 123100;
   ZoneIndexType nz = pStirrupGeometry->GetPrimaryZoneCount(segmentKey);
   for (ZoneIndexType iz=0; iz<nz; iz++)
   {
      Float64 zoneStart, zoneEnd;
      pStirrupGeometry->GetPrimaryZoneBounds(segmentKey, iz, &zoneStart, &zoneEnd);

      WBFL::Materials::Rebar::Size barSize;
      Float64 spacing;
      Float64 nStirrups;
      pStirrupGeometry->GetPrimaryVertStirrupBarInfo(segmentKey,iz,&barSize,&nStirrups,&spacing);

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", Zone ")<< LABEL_STIRRUP_ZONE(iz) <<  _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(zoneStart, WBFL::Units::Measure::Millimeter)) <<   _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(zoneEnd, WBFL::Units::Measure::Millimeter)) <<   _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<< nStirrups <<   _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<< GetBarSize(barSize) <<   _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(spacing, WBFL::Units::Measure::Millimeter)) <<   _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;
   }

   // Additional horizontal shear interface zones
   id = 123300;
   nz = pStirrupGeometry->GetHorizInterfaceZoneCount(segmentKey);
   for (ZoneIndexType iz=0; iz<nz; iz++)
   {
      Float64 zoneStart, zoneEnd;
      pStirrupGeometry->GetHorizInterfaceZoneBounds(segmentKey, iz, &zoneStart, &zoneEnd);

      WBFL::Materials::Rebar::Size barSize;
      Float64 spacing;
      Float64 nStirrups;
      pStirrupGeometry->GetHorizInterfaceBarInfo(segmentKey,iz,&barSize,&nStirrups,&spacing);

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", Zone ")<< LABEL_STIRRUP_ZONE(iz) <<  _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(zoneStart, WBFL::Units::Measure::Millimeter)) <<   _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(zoneEnd, WBFL::Units::Measure::Millimeter)) <<   _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<< nStirrups <<   _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<< GetBarSize(barSize) <<   _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(spacing, WBFL::Units::Measure::Millimeter)) <<   _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;
   }

   return true;
}


bool CTestAgentImp::RunDesignTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, const CSegmentKey& segmentKey)
{
   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   Float64 loc = 0.0;

   GET_IFACE(IArtifact,pIArtifact);
   const pgsGirderDesignArtifact* pGirderDesignArtifact = nullptr;
   const pgsSegmentDesignArtifact* pArtifact = nullptr;

   // design is only applicable to PGSuper files... group index is the span index
   SpanIndexType spanIdx = segmentKey.groupIndex;
   GirderIndexType gdrIdx = segmentKey.girderIndex;
   ATLASSERT(segmentKey.segmentIndex == 0);

   try
   {
      pGirderDesignArtifact = pIArtifact->CreateDesignArtifact(segmentKey,true/*design for flexure*/, sodDefault/*use default haunch design method*/, cdDesignForMinStrength, sdtLayoutStirrups);
      if ( pGirderDesignArtifact )
      {
         pArtifact = pGirderDesignArtifact->GetSegmentDesignArtifact(segmentKey.segmentIndex);
      }
   }
   catch(...)
   {
      resultsFile << "Design Failed for span " << spanIdx << " girder " << gdrIdx << std::endl;
      return false;
   }

   if ( pArtifact == nullptr )
   {
      resultsFile << "Design was canceled for span " << spanIdx << " girder " << gdrIdx << std::endl;
      return false;
   }

   if ( pArtifact->GetOutcome() != pgsSegmentDesignArtifact::Success )
   {
      resultsFile << "Design not successful for span " << spanIdx << " girder " << gdrIdx << std::endl;
      return false;
   }

   resultsFile<<bridgeId<<", "<<pid<<", 124001, "<<loc<<", "<<pArtifact->GetOutcome()<<   ", 102, "<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124002, "<<loc<<", "<<pArtifact->GetNumStraightStrands()<<   ", 102, "<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124003, "<<loc<<", "<<pArtifact->GetNumTempStrands()<<   ", 102, "<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124004, "<<loc<<", "<<pArtifact->GetNumHarpedStrands()<<   ", 102, "<<SEGMENT(segmentKey)<<std::endl;

   resultsFile<<bridgeId<<", "<<pid<<", 124005, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pArtifact->GetPjackStraightStrands(), WBFL::Units::Measure::Newton)) <<   ", 102, "<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124006, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pArtifact->GetPjackTempStrands(), WBFL::Units::Measure::Newton)) <<   ", 102, "<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124007, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pArtifact->GetPjackHarpedStrands(), WBFL::Units::Measure::Newton)) <<   ", 102, "<<SEGMENT(segmentKey)<<std::endl;

   resultsFile<<bridgeId<<", "<<pid<<", 124008, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pArtifact->GetHarpStrandOffsetHp(pgsTypes::metStart), WBFL::Units::Measure::Millimeter)) <<   ", 102, "<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124009, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pArtifact->GetHarpStrandOffsetEnd(pgsTypes::metEnd), WBFL::Units::Measure::Millimeter)) <<   ", 102, "<<SEGMENT(segmentKey)<<std::endl;

   resultsFile<<bridgeId<<", "<<pid<<", 124010, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pArtifact->GetReleaseStrength(), WBFL::Units::Measure::MPa)) <<   ", 102, "<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124011, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pArtifact->GetConcrete().GetFc(), WBFL::Units::Measure::MPa)) <<   ", 102, "<<SEGMENT(segmentKey)<<std::endl;

   resultsFile<<bridgeId<<", "<<pid<<", 124012s, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pArtifact->GetSlabOffset(pgsTypes::metStart), WBFL::Units::Measure::Millimeter)) <<   ", 102, "<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124012e, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pArtifact->GetSlabOffset(pgsTypes::metEnd), WBFL::Units::Measure::Millimeter)) <<   ", 102, "<<SEGMENT(segmentKey)<<std::endl;

   resultsFile<<bridgeId<<", "<<pid<<", 124013, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pArtifact->GetLeftLiftingLocation(), WBFL::Units::Measure::Millimeter)) <<   ", 102, "<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<", "<<pid<<", 124014, "<<loc<<", "<< QUIET(WBFL::Units::ConvertFromSysUnits(pArtifact->GetLeadingOverhang(), WBFL::Units::Measure::Millimeter)) <<   ", 102, "<<SEGMENT(segmentKey)<<std::endl;

   const CShearData2* pShearData = pArtifact->GetShearData();

   int ncz = -1;
   WBFL::Materials::Rebar::Size czsize(WBFL::Materials::Rebar::Size::bsNone);
   for (CShearData2::ShearZoneConstIterator czit = pShearData->ShearZones.begin(); czit != pShearData->ShearZones.end(); czit++)
   {
      if (czit->ConfinementBarSize != WBFL::Materials::Rebar::Size::bsNone)
      {
         czsize = czit->ConfinementBarSize;
         ncz++;
      }
      else
      {
         break;
      }
   }

   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124015, ")<<loc<<_T(", ")<<GetBarSize(czsize)<<   _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;
   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124016, ")<<loc<<_T(", ")<< ncz <<   _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;

   resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", 124017, ")<<loc<<_T(", ")<<pArtifact->GetNumberOfStirrupZonesDesigned()<<   _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;

   Int32 id = 124018;
   for (CShearData2::ShearZoneConstIterator czit = pShearData->ShearZones.begin(); czit != pShearData->ShearZones.end(); czit++)
   {
      const CShearZoneData2& zd = *czit;

      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<<GetBarSize(zd.VertBarSize)<<   _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(zd.BarSpacing, WBFL::Units::Measure::Millimeter)) <<   _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<< QUIET(WBFL::Units::ConvertFromSysUnits(zd.ZoneLength, WBFL::Units::Measure::Millimeter)) <<   _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;
      resultsFile<<bridgeId<<_T(", ")<<pid<<_T(", ")<<id++<<_T(", ")<<loc<<_T(", ")<< zd.nVertBars <<   _T(", 102, ")<<SEGMENT(segmentKey)<<std::endl;
   }

   return true;
}

bool CTestAgentImp::RunCamberTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, const CSegmentKey& segmentKey)
{
   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   GET_IFACE(IPointOfInterest,pPointsOfInterest);
   PoiList vPoi;
   pPointsOfInterest->GetPointsOfInterest(segmentKey, POI_5L | POI_ERECTED_SEGMENT, &vPoi);
   ATLASSERT(vPoi.size()==1);
   const pgsPointOfInterest& poi_midspan = vPoi.front();

   GET_IFACE( ICamber, pCamber );
   Float64 D40  = pCamber->GetDCamberForGirderSchedule(poi_midspan,CREEP_MINTIME);
   Float64 D120 = pCamber->GetDCamberForGirderSchedule(poi_midspan,CREEP_MAXTIME);

   resultsFile << bridgeId << ", " << pid << ", 125000, " << QUIET(WBFL::Units::ConvertFromSysUnits(D40,  WBFL::Units::Measure::Millimeter)) << ", " << SEGMENT(segmentKey) << std::endl;
   resultsFile << bridgeId << ", " << pid << ", 125001, " << QUIET(WBFL::Units::ConvertFromSysUnits(D120, WBFL::Units::Measure::Millimeter)) << ", " << SEGMENT(segmentKey) << std::endl;

   return true;
}


bool CTestAgentImp::RunFabOptimizationTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, const CSegmentKey& segmentKey)
{
   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   GET_IFACE(ISegmentLiftingSpecCriteria,pSegmentLiftingSpecCriteria);
   GET_IFACE_NOCHECK(ISegmentHaulingSpecCriteria,pSegmentHaulingSpecCriteria);
   if (pSegmentLiftingSpecCriteria->IsLiftingAnalysisEnabled() && pSegmentHaulingSpecCriteria->IsHaulingAnalysisEnabled() && pSegmentHaulingSpecCriteria->GetHaulingAnalysisMethod() == pgsTypes::hmWSDOT)
   {
	   GET_IFACE(IFabricationOptimization,pFabOp);

	   FABRICATIONOPTIMIZATIONDETAILS details;
      if (pFabOp->GetFabricationOptimizationDetails(segmentKey, &details) == false)
         return true; // analysis could not be done so just return with true so the testing keeps going

	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155000, ") << WBFL::Units::ConvertFromSysUnits(details.Fci[NO_TTS],WBFL::Units::Measure::MPa) << _T(", ") << SEGMENT(segmentKey) << std::endl;
	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155001, ") << WBFL::Units::ConvertFromSysUnits(details.L[NO_TTS],WBFL::Units::Measure::Millimeter) << _T(", ") << SEGMENT(segmentKey) << std::endl;

	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155002, ") << WBFL::Units::ConvertFromSysUnits(details.Fci[PS_TTS],WBFL::Units::Measure::MPa) << _T(", ") << SEGMENT(segmentKey) << std::endl;
	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155003, ") << WBFL::Units::ConvertFromSysUnits(details.L[PS_TTS],WBFL::Units::Measure::Millimeter) << _T(", ") << SEGMENT(segmentKey) << std::endl;

	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155004, ") << WBFL::Units::ConvertFromSysUnits(details.Fci[PT_TTS_OPTIONAL],WBFL::Units::Measure::MPa) << _T(", ") << SEGMENT(segmentKey) << std::endl;
	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155005, ") << WBFL::Units::ConvertFromSysUnits(details.L[PT_TTS_OPTIONAL],WBFL::Units::Measure::Millimeter) << _T(", ") << SEGMENT(segmentKey) << std::endl;

	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155006, ") << WBFL::Units::ConvertFromSysUnits(details.Fci[PT_TTS_REQUIRED],WBFL::Units::Measure::MPa) << _T(", ") << SEGMENT(segmentKey) << std::endl;
	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155007, ") << WBFL::Units::ConvertFromSysUnits(details.L[PT_TTS_REQUIRED],WBFL::Units::Measure::Millimeter) << _T(", ") << SEGMENT(segmentKey) << std::endl;

	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155008, ") << WBFL::Units::ConvertFromSysUnits(details.Fci_FormStripping_WithoutTTS,WBFL::Units::Measure::MPa) << _T(", ") << SEGMENT(segmentKey) << std::endl;

	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155009, ") << WBFL::Units::ConvertFromSysUnits(details.Lmin,WBFL::Units::Measure::Millimeter) << _T(", ") << SEGMENT(segmentKey) << std::endl;
	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155010, ") << WBFL::Units::ConvertFromSysUnits(details.Lmax,WBFL::Units::Measure::Millimeter) << _T(", ") << SEGMENT(segmentKey) << std::endl;

	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155011, ") << WBFL::Units::ConvertFromSysUnits(details.LUmin,WBFL::Units::Measure::Millimeter) << _T(", ") << SEGMENT(segmentKey) << std::endl;
	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155012, ") << WBFL::Units::ConvertFromSysUnits(details.LUmax,WBFL::Units::Measure::Millimeter) << _T(", ") << SEGMENT(segmentKey) << std::endl;
	   resultsFile << bridgeId << _T(", ") << pid << _T(", 155013, ") << WBFL::Units::ConvertFromSysUnits(details.LUsum,WBFL::Units::Measure::Millimeter) << _T(", ") << SEGMENT(segmentKey) << std::endl;
   }

   return true;
}

bool CTestAgentImp::RunLoadRatingTest(std::_tofstream& resultsFile, std::_tofstream& poiFile, const CGirderKey& girderKey)
{
   std::_tstring pid      = GetProcessID();
   std::_tstring bridgeId = GetBridgeID();

   GET_IFACE(IProductLoads,pProductLoads);
   GET_IFACE(IArtifact,pArtifacts);

   GET_IFACE(IBridge,pBridge);
   bool bNegMoments = pBridge->ProcessNegativeMoments(ALL_SPANS);
   
   GET_IFACE(IRatingSpecification,pRatingSpec);
   int n = (int)pgsTypes::lrLoadRatingTypeCount;
   for ( int i = 0; i < n; i++ )
   {
      pgsTypes::LoadRatingType ratingType = (pgsTypes::LoadRatingType)i;
      if ( !pRatingSpec->IsRatingEnabled(ratingType) )
      {
         continue;
      }

      pgsTypes::LiveLoadType llType = ::GetLiveLoadType(ratingType);
      VehicleIndexType nVehicles = pProductLoads->GetVehicleCount(llType);
      for ( VehicleIndexType vehicleIdx = 0; vehicleIdx < nVehicles; vehicleIdx++ )
      {
         const pgsRatingArtifact* pArtifact = pArtifacts->GetRatingArtifact(girderKey,ratingType,vehicleIdx);
         if ( pArtifact == nullptr )
         {
            continue;
         }

         std::_tstring strRatingType = ::GetLiveLoadTypeName(ratingType);
         std::_tstring strTruckName = pProductLoads->GetLiveLoadName(llType,vehicleIdx);
         resultsFile << strRatingType << _T(", ") << strTruckName << std::endl;

         const pgsMomentRatingArtifact* pMomentArtifact = nullptr;
         Float64 RF = pArtifact->GetMomentRatingFactorEx(true,&pMomentArtifact);
         if ( pMomentArtifact )
         {
            resultsFile << bridgeId << _T(", ") << pid << _T(", 881001, ") << QUIET(RF) << _T(", ") << GIRDER(girderKey) << std::endl;
            resultsFile << bridgeId << _T(", ") << pid << _T(", 881001a, ") << QUIET(pMomentArtifact->GetPointOfInterest().GetDistFromStart()) << _T(", ") << GIRDER(girderKey) << std::endl;
         }

         if ( bNegMoments )
         {
            pMomentArtifact = nullptr;
            RF = pArtifact->GetMomentRatingFactorEx(false,&pMomentArtifact);
            if ( pMomentArtifact )
            {
	            resultsFile << bridgeId << _T(", ") << pid << _T(", 881002, ") << QUIET(RF) << _T(", ") << GIRDER(girderKey) << std::endl;
               resultsFile << bridgeId << _T(", ") << pid << _T(", 881002a, ") << QUIET(pMomentArtifact->GetPointOfInterest().GetDistFromStart()) << _T(", ") << GIRDER(girderKey) << std::endl;
            }
         }
         
         if ( pRatingSpec->RateForShear(ratingType) )
         {
            const pgsShearRatingArtifact* pShearArtifact;
            RF = pArtifact->GetShearRatingFactorEx(&pShearArtifact);
            if ( pShearArtifact )
            {
               resultsFile << bridgeId << _T(", ") << pid << _T(", 881003, ") << QUIET(RF) << _T(", ") << GIRDER(girderKey) << std::endl;
               resultsFile << bridgeId << _T(", ") << pid << _T(", 881003a, ") << QUIET(pShearArtifact->GetPointOfInterest().GetDistFromStart()) << _T(", ") << GIRDER(girderKey) << std::endl;
            }
         }

         if ( pRatingSpec->RateForStress(ratingType) )
         {
            if ( ratingType == pgsTypes::lrPermit_Routine || ratingType == pgsTypes::lrPermit_Special )
            {
               // Service I reinforcement yield check if permit rating
               RF = pArtifact->GetYieldStressRatio(true);
               resultsFile << bridgeId << _T(", ") << pid << _T(", 881004, ") << QUIET(RF) << _T(", ") << GIRDER(girderKey) << std::endl;

               if ( bNegMoments )
               {
                  RF = pArtifact->GetYieldStressRatio(false);
                  resultsFile << bridgeId << _T(", ") << pid << _T(", 881005, ") << QUIET(RF) << _T(", ") << GIRDER(girderKey) << std::endl;
               }
            }
            else
            {
               // Service III flexure if other rating type
               RF = pArtifact->GetStressRatingFactor();
               resultsFile << bridgeId << _T(", ") << pid << _T(", 881006, ") << QUIET(RF) << _T(", ") << GIRDER(girderKey) << std::endl;
            }
         }

         RF = pArtifact->GetRatingFactor();
         resultsFile << bridgeId << _T(", ") << pid << _T(", 881007, ") << QUIET(RF) << _T(", ") << GIRDER(girderKey) << std::endl;
      }
   }

   return true;
}

bool CTestAgentImp::RunAlignmentTest(std::_tofstream& resultsFile)
{
   GET_IFACE(IRoadwayData, pAlignment);
   GET_IFACE_NOCHECK(IRoadway, pRoadway);

   resultsFile << _T("Alignment Data") << std::endl;
   const AlignmentData2& alignment = pAlignment->GetAlignmentData2();
   resultsFile << _T("Direction : ") << alignment.Direction << std::endl;
   resultsFile << _T("Ref Point, X : ") << alignment.xRefPoint << _T(", Y : ") << alignment.yRefPoint << std::endl;
   resultsFile << _T("HCurveCount : ") << alignment.CompoundCurves.size() << std::endl;

   IndexType hcIdx = 0; // keeps track of actual curves in the model (curves with zero radious are not curves in the alignment model)
   for (const auto& hcData : alignment.CompoundCurves)
   {
      if (!IsZero(hcData.Radius))
      {
         resultsFile << _T("Curve ") << hcIdx << std::endl;
         CComPtr<ICompoundCurve> hc;
         pRoadway->GetCurve(hcIdx, pgsTypes::pcGlobal, &hc);
         HCURVESTATIONS stations = pRoadway->GetCurveStations(hcIdx);
         resultsFile << _T("TS : ") << stations.TSStation << std::endl;
         resultsFile << _T("SPI1 : ") << stations.SPI1Station << std::endl;
         resultsFile << _T("SC : ") << stations.SCStation << std::endl;
         resultsFile << _T("PI : ") << stations.PIStation << std::endl;
         resultsFile << _T("CS : ") << stations.CSStation << std::endl;
         resultsFile << _T("SPI2 : ") << stations.SPI2Station << std::endl;
         resultsFile << _T("ST : ") << stations.STStation << std::endl;

         CComPtr<IDirection> dirTangent;
         hc->get_BkTangentBrg(&dirTangent);
         Float64 value;
         dirTangent->get_Value(&value);
         resultsFile << _T("Back Tangent Direction : ") << value << std::endl;

         hc->get_BkTangentLength(&value);
         resultsFile << _T("Back Tangent Length : ") << value << std::endl;

         dirTangent.Release();
         hc->get_FwdTangentBrg(&dirTangent);
         dirTangent->get_Value(&value);
         resultsFile << _T("Forward Tangent Direction : ") << value << std::endl;

         hc->get_FwdTangentLength(&value);
         resultsFile << _T("Foward Tangent Length : ") << value << std::endl;

         CComPtr<IAngle> angle;
         SpiralType spiralType = spEntry;
         resultsFile << _T("Entry Spiral") << std::endl;
         hc->get_SpiralLength(spiralType, &value);
         resultsFile << _T("Length : ") << value << std::endl;
         angle.Release();
         hc->get_SpiralAngle(spiralType, &angle);
         angle->get_Value(&value);
         resultsFile << _T("Angle : ") << value << std::endl;
         angle.Release();
         hc->get_DE(spiralType, &angle);
         angle->get_Value(&value);
         resultsFile << _T("DE : ") << value << std::endl;
         angle.Release();
         hc->get_DF(spiralType, &angle);
         angle->get_Value(&value);
         resultsFile << _T("DF : ") << value << std::endl;
         angle.Release();
         hc->get_DH(spiralType, &angle);
         angle->get_Value(&value);
         resultsFile << _T("DH : ") << value << std::endl;
         hc->get_LongTangent(spiralType, &value);
         resultsFile << _T("LT : ") << value << std::endl;
         hc->get_ShortTangent(spiralType, &value);
         resultsFile << _T("ST : ") << value << std::endl;
         hc->get_SpiralChord(spiralType, &value);
         resultsFile << _T("Chord : ") << value << std::endl;
         hc->get_X(spiralType, &value);
         resultsFile << _T("X : ") << value << std::endl;
         hc->get_Y(spiralType, &value);
         resultsFile << _T("Y : ") << value << std::endl;
         hc->get_Q(spiralType, &value);
         resultsFile << _T("Q : ") << value << std::endl;
         hc->get_T(spiralType, &value);
         resultsFile << _T("T : ") << value << std::endl;

         resultsFile << _T("Circular Curve") << std::endl;
         angle.Release();
         hc->get_CircularCurveAngle(&angle);
         angle->get_Value(&value);
         CurveDirectionType direction;
         hc->get_Direction(&direction);
         value *= (direction == cdRight ? -1 : 1);
         resultsFile << _T("Delta : ") << value << std::endl;
         hc->get_Tangent(&value);
         resultsFile << _T("T : ") << value << std::endl;
         hc->get_CurveLength(&value);
         resultsFile << _T("L : ") << value << std::endl;
         hc->get_External(&value);
         resultsFile << _T("E : ") << value << std::endl;
         hc->get_Chord(&value);
         resultsFile << _T("LC : ") << value << std::endl;
         hc->get_MidOrdinate(&value);
         resultsFile << _T("MO : ") << value << std::endl;


         spiralType = spExit;
         resultsFile << _T("Exit Spiral") << std::endl;
         hc->get_SpiralLength(spiralType, &value);
         resultsFile << _T("Length : ") << value << std::endl;
         angle.Release();
         hc->get_SpiralAngle(spiralType, &angle);
         angle->get_Value(&value);
         resultsFile << _T("Angle : ") << value << std::endl;
         angle.Release();
         hc->get_DE(spiralType, &angle);
         angle->get_Value(&value);
         resultsFile << _T("DE : ") << value << std::endl;
         angle.Release();
         hc->get_DF(spiralType, &angle);
         angle->get_Value(&value);
         resultsFile << _T("DF : ") << value << std::endl;
         angle.Release();
         hc->get_DH(spiralType, &angle);
         angle->get_Value(&value);
         resultsFile << _T("DH : ") << value << std::endl;
         hc->get_LongTangent(spiralType, &value);
         resultsFile << _T("LT : ") << value << std::endl;
         hc->get_ShortTangent(spiralType, &value);
         resultsFile << _T("ST : ") << value << std::endl;
         hc->get_SpiralChord(spiralType, &value);
         resultsFile << _T("Chord : ") << value << std::endl;
         hc->get_X(spiralType, &value);
         resultsFile << _T("X : ") << value << std::endl;
         hc->get_Y(spiralType, &value);
         resultsFile << _T("Y : ") << value << std::endl;
         hc->get_Q(spiralType, &value);
         resultsFile << _T("Q : ") << value << std::endl;
         hc->get_T(spiralType, &value);
         resultsFile << _T("T : ") << value << std::endl;

         hcIdx++;
      }
   }

   resultsFile << _T("Profile Data") << std::endl;
   const ProfileData2& profile = pAlignment->GetProfileData2();
   resultsFile << _T("Station : ") << profile.Station << std::endl;
   resultsFile << _T("Elevation : ") << profile.Elevation << std::endl;
   resultsFile << _T("Grade : ") << profile.Grade << std::endl;
   resultsFile << _T("VCurveCount : ") << profile.VertCurves.size() << std::endl;

   IndexType vcIdx = 0;
   for (const auto& vcd : profile.VertCurves)
   {
      if (!IsZero(vcd.L1))
      {
         resultsFile << _T("Vert Curve ") << vcIdx << std::endl;
         CComPtr<IVerticalCurve> vc;
         pRoadway->GetVertCurve(vcIdx, &vc);

         Float64 sta, elev;
         CComPtr<IStation> station;
         CComPtr<IProfilePoint> point;

         station.Release();
         point.Release();
         vc->get_BVC(&point);
         point->get_Station(&station);
         station->get_Value(&sta);
         point->get_Elevation(&elev);
         resultsFile << _T("BVC Sta: ") << sta << _T(", Elev : ") << elev << std::endl;

         station.Release();
         point.Release();
         vc->get_PVI(&point);
         point->get_Station(&station);
         station->get_Value(&sta);
         point->get_Elevation(&elev);
         resultsFile << _T("PVI Sta: ") << sta << _T(", Elev : ") << elev << std::endl;

         station.Release();
         point.Release();
         vc->get_EVC(&point);
         point->get_Station(&station);
         station->get_Value(&sta);
         point->get_Elevation(&elev);
         resultsFile << _T("EVC Sta: ") << sta << _T(", Elev : ") << elev << std::endl;

         station.Release();
         point.Release();
         vc->get_HighPoint(&point);
         point->get_Station(&station);
         station->get_Value(&sta);
         point->get_Elevation(&elev);
         resultsFile << _T("High Point Sta: ") << sta << _T(", Elev : ") << elev << std::endl;

         station.Release();
         point.Release();
         vc->get_LowPoint(&point);
         point->get_Station(&station);
         station->get_Value(&sta);
         point->get_Elevation(&elev);
         resultsFile << _T("Low Point Sta: ") << sta << _T(", Elev : ") << elev << std::endl;

         Float64 g1, g2;
         vc->get_EntryGrade(&g1);
         vc->get_ExitGrade(&g2);
         resultsFile << _T("Grades g1 : ") << g1 << _T(", g2 : ") << g2 << std::endl;

         Float64 L1, L2, Length;
         vc->get_L1(&L1);
         vc->get_L2(&L2);
         CComQIPtr<IProfileElement> element(vc);
         element->GetLength(&Length);
         resultsFile << _T("Lengths L1 : ") << L1 << _T(", L2 : ") << L2 << _T(", L: ") << Length << std::endl;


         vcIdx++;
      }
   }

   return true;
}


void CTestAgentImp::ProcessTestReport(const CTestCommandLineInfo& rCmdInfo)
{
   USES_CONVERSION;

   ASSERT(rCmdInfo.m_DoTxCadReport);

   pgsAutoGirderLabel auto_label;
   pgsAutoPierLabel auto_plabel;


   if (rCmdInfo.m_TxGirder != TXALLGIRDERS && 
       rCmdInfo.m_TxGirder != TXEIGIRDERS && 
       (rCmdInfo.m_TxGirder == INVALID_INDEX || 27 < rCmdInfo.m_TxGirder))
   {
      ::AfxMessageBox(_T("Invalid girder specified on command line for test report"));
      return;
   }
/*
   if (rCmdInfo.m_TxSpan != ALL_SPANS && rCmdInfo.m_TxSpan < 0)
   {
      ::AfxMessageBox(_T("Invalid span specified on command line for test report"));
      return;
   }
*/
   CString errfile;
   if (CreateTestFileNames(rCmdInfo.m_TxOutputFile, &errfile))
   {
      try
      {
         if ( !DoTestReport(rCmdInfo.m_TxOutputFile, errfile, rCmdInfo) )
         {
            CString msg = CString(_T("Error - Running test on file"))+rCmdInfo.m_strFileName;
            ::AfxMessageBox(msg);
         }
      }
      catch(const WBFL::System::XBase& e)
      {
         std::_tstring msg = e.GetErrorMessage();
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running Test report for input file: ")<<rCmdInfo.m_strFileName<<std::endl<< msg;
      }
      catch(CException* pex)
      {
         TCHAR   szCause[255];
         CString strFormatted;
         pex->GetErrorMessage(szCause, 255);
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running Test report for input file: ")<<rCmdInfo.m_strFileName<<std::endl<< szCause;
         delete pex;
      }
      catch(CException& ex)
      {
         TCHAR   szCause[255];
         CString strFormatted;
         ex.GetErrorMessage(szCause, 255);
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running Test report for input file: ")<<rCmdInfo.m_strFileName<<std::endl<< szCause;
      }
      catch(const std::exception* pex)
      {
         std::_tstring strMsg(CA2T(pex->what()));
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running test report for input file: ")<<rCmdInfo.m_strFileName<<std::endl<<strMsg<< std::endl;
         delete pex;
      }
      catch(const std::exception& ex)
      {
         std::_tstring strMsg(CA2T(ex.what()));
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running Test report for input file: ")<<rCmdInfo.m_strFileName<<std::endl<<strMsg<< std::endl;
      }
      catch(...)
      {
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Unknown Error running Test report for input file: ")<<rCmdInfo.m_strFileName;
      }
   }
}

bool CTestAgentImp::CreateTestFileNames(const CString& output, CString* pErrFileName)
{
   CString tmp(output);
   tmp.MakeLower();
   int loc = tmp.Find(_T("."),0);
   if (loc>0)
   {
      CString basename = output.Left(loc);
      *pErrFileName = basename + _T(".err");
      return true;
   }
   else
   {
      *pErrFileName = output + _T(".err");

      return false;
   }
}

bool CTestAgentImp::DoTestReport(const CString& outputFileName, const CString& errorFileName, const CTestCommandLineInfo& txInfo)
{
   // Called from the command line processing in the Application object

   // open the error file
   std::_tofstream err_file(errorFileName);
   if ( !err_file )
   {
	   AfxMessageBox (_T("Could not Create error file"));
	   return false;
   }

   // Open/create the specified output file 
   FILE	*fp;
   errno_t result;
   if (txInfo.m_DoAppendToFile)
   {
      result = _tfopen_s(&fp, LPCTSTR (outputFileName), _T("a+"));
   }
   else
   {
      result = _tfopen_s(&fp, LPCTSTR (outputFileName), _T("w+"));
   }

   if (result != 0 || fp == nullptr)
   {
      err_file<<_T("Error: Output file could not be Created.")<<std::endl;
	   return false;
   }

   // Get starting and ending spans
   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();

   SpanIndexType start_span, end_span;
   if (txInfo.m_TxSpan==ALL_SPANS)
   {
      // looping over all spans
      start_span = 0;
      end_span = nSpans-1;
   }
   else
   {
      if (txInfo.m_TxSpan>=nSpans)
      {
         err_file<<_T("Span value is out of range for this bridge")<<std::endl;
	      return false;
      }
      else
      {
         start_span = txInfo.m_TxSpan;
         end_span   = txInfo.m_TxSpan;
      }
   }

   // Build list of span/girders to operate on (error out before we do anything of there are problems)
   std::vector<SpanGirderHashType> spn_grd_list;

   for (SpanIndexType is=start_span; is<=end_span; is++)
   {
      // we can have a different number of girders per span
      GirderIndexType nGirders = pBridge->GetGirderCount(is);

      if (txInfo.m_TxGirder==TXALLGIRDERS)
      {
         for (GirderIndexType ig=0; ig<nGirders; ig++)
         {
            SpanGirderHashType key = HashSpanGirder(is,ig);
            spn_grd_list.push_back(key);
         }
      }
      else if (txInfo.m_TxGirder==TXEIGIRDERS)
      {
         // Exterior/Interior option
         SpanGirderHashType key = HashSpanGirder(is,0); // left exterior
         spn_grd_list.push_back(key);

         if (nGirders>2)
         {
            SpanGirderHashType key = HashSpanGirder(is,1); // exterior-most interior
            spn_grd_list.push_back(key);

            if (nGirders>4)
            {
               SpanGirderHashType key = HashSpanGirder(is,nGirders/2); // middle-most interior
               spn_grd_list.push_back(key);
            }
         }

//         key = HashSpanGirder(is,nGirders-1); // right exterior
//         spn_grd_list.push_back(key);
      }

      else if (txInfo.m_TxGirder<nGirders)
      {
         SpanGirderHashType key = HashSpanGirder(is,txInfo.m_TxGirder);
         spn_grd_list.push_back(key);
      }
      else
      {
         err_file<<_T("Girder value is out of range for this bridge")<<std::endl;
	      return false;
      }
   }

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   if (txInfo.m_TxRunType == CTestCommandLineInfo::txrGeometry)
   {
      // Geometry test only - save some text to .test file to placate comparisons
      _ftprintf(fp, _T("Bridge Geometry Test. See .dbr file for results."));

      // file names
      CString resultsfile, poifile, errfile;
      CString strExt(_T(".pgs"));
      if (create_test_file_names(strExt, txInfo.m_strFileName, &resultsfile, &poifile, &errfile))
      {
         GET_IFACE(ITest1250, ptst);

         try
         {
            if (!ptst->RunTestEx(RUN_GEOMTEST, spn_grd_list, std::_tstring(resultsfile), std::_tstring(poifile)))
            {
               CString msg = CString(_T("Error - Running geometry test on file")) + txInfo.m_strFileName;
               ::AfxMessageBox(msg);
            }
         }
         catch (...)
         {
            CString msg = CString(_T("Error - running geometry test for input file:")) + txInfo.m_strFileName;
            ::AfxMessageBox(msg);
            return false;
         }
      }
      else
      {
         CString msg = CString(_T("Error - Determining 1250 test file names for")) + txInfo.m_strFileName;
         ::AfxMessageBox(msg);
         return false;
      }
   }
   else
   {
      // Loop over all span/girder combos and create results
      std::vector<SpanGirderHashType>::iterator it(spn_grd_list.begin());
      std::vector<SpanGirderHashType>::iterator end(spn_grd_list.end());
      for (; it != end; it++)
      {
         SpanGirderHashType key = *it;
         SpanIndexType span;
         GirderIndexType girder;
         UnhashSpanGirder(key, &span, &girder);

         CSegmentKey segmentKey(span, girder, 0);

         CString strMessage;
         strMessage.Format(_T("Creating Test report for Span %s, Girder %s"), LABEL_SPAN(span), LABEL_GIRDER(girder));
         pProgress->UpdateMessage(strMessage);

         // See if we need to run a design
         bool designSucceeded = true;
         if (txInfo.m_TxRunType == CTestCommandLineInfo::txrDesign || txInfo.m_TxRunType == CTestCommandLineInfo::txrDesignShear)
         {
            arShearDesignType shearDesignType;
            if (txInfo.m_TxRunType == CTestCommandLineInfo::txrDesignShear)
            {
               GET_IFACE(IStirrupGeometry, pStirrupGeom);
               shearDesignType = pStirrupGeom->AreStirrupZonesSymmetrical(segmentKey) ? sdtLayoutStirrups : sdtRetainExistingLayout;
            }
            else
            {
               shearDesignType = sdtNoDesign;
            }

            GET_IFACE(IArtifact, pIArtifact);
            const pgsGirderDesignArtifact* pGirderDesignArtifact;
            const pgsSegmentDesignArtifact* pArtifact;
            try
            {
               // Design the girder
               pGirderDesignArtifact = pIArtifact->CreateDesignArtifact(segmentKey, true/*design for flexure*/, sodDefault/*use default haunch design method*/, cdDesignForMinStrength, shearDesignType);
               pArtifact = pGirderDesignArtifact->GetSegmentDesignArtifact(segmentKey.segmentIndex);
               ATLASSERT(segmentKey.IsEqual(pArtifact->GetSegmentKey()));

               if (pArtifact->GetOutcome() != pgsSegmentDesignArtifact::Success)
               {
                  err_file << _T("Design was unsuccessful") << std::endl;
                  designSucceeded = false;
               }

               // Copy the design to the bridge
               SaveFlexureDesign(segmentKey, pArtifact);
            }
            catch (...)
            {
               err_file << _T("Design Failed for span") << span << _T(" girder ") << girder << std::endl;
               return false;
            }
         }

         GET_IFACE(ITestFileExport, pTxDOTCadExport);
         if (!pTxDOTCadExport)
         {
            AfxMessageBox(_T("The Test File Exporter is not currently installed"));
            return false;
         }

         if (txInfo.m_TxRunType == CTestCommandLineInfo::TxrDistributionFactors)
         {
            // Write distribution factor data to file
            try
            {
               if (CAD_SUCCESS != pTxDOTCadExport->WriteDistributionFactorsToFile(fp, this->m_pBroker, segmentKey))
               {
                  err_file << _T("Warning: An error occured while writing to File") << std::endl;
                  return false;
               }
            }
            catch (CXUnwind* pExc)
            {
               // Probable lldf out of range error
               std::_tstring sCause;
               pExc->GetErrorMessage(&sCause);
               _ftprintf(fp, sCause.c_str());
               _ftprintf(fp, _T("\n"));
               pExc->Delete();
            }
         }
         else
         {
            /* Write CAD data to text file */
            if (CAD_SUCCESS != pTxDOTCadExport->WriteCADDataToFile(fp, this->m_pBroker, segmentKey, designSucceeded))
            {
               err_file << _T("Warning: An error occured while writing to File") << std::endl;
               return false;
            }
         }
      }
   }

   /* Close the open text file */
   fclose (fp);

   // ---------------------------------------------
   // Run a 12-50 output if this is a test file
   if (txInfo.m_TxFType == CTestCommandLineInfo::txfTest)
   {
      // file names
      CString resultsfile, poifile, errfile;
      CString strExt(_T(".pgs"));
      if (create_test_file_names(strExt,txInfo.m_strFileName,&resultsfile,&poifile,&errfile))
      {
         GET_IFACE(ITest1250, ptst );

         try
         {
            if (!ptst->RunTestEx(RUN_CADTEST, spn_grd_list, std::_tstring(resultsfile), std::_tstring(poifile)))
            {
               CString msg = CString(_T("Error - Running test on file"))+txInfo.m_strFileName;
               ::AfxMessageBox(msg);
            }
         }
         catch(...)
         {
            CString msg = CString(_T("Error - running test for input file:"))+txInfo.m_strFileName;
            ::AfxMessageBox(msg);
            return false;
         }
      }
      else
      {
         CString msg = CString(_T("Error - Determining 1250 test file names for"))+txInfo.m_strFileName;
         ::AfxMessageBox(msg);
         return false;
      }
   }


   return true;
}

void CTestAgentImp::SaveFlexureDesign(const CSegmentKey& segmentKey,const pgsSegmentDesignArtifact* pArtifact)
{
   // Artifact does hard work of converting to girder data
   CPrecastSegmentData segmentData = pArtifact->GetSegmentData();
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE_NOCHECK(ISpecification,pSpec);
   pIBridgeDesc->SetPrecastSegmentData(segmentKey,segmentData);

   const arDesignOptions& design_options = pArtifact->GetDesignOptions();

   if (design_options.doDesignForFlexure != dtNoDesign && design_options.doDesignSlabOffset != sodPreserveHaunch)
   {
      pgsTypes::SlabOffsetType slabOffsetType = pIBridgeDesc->GetSlabOffsetType();

      if ( slabOffsetType == pgsTypes::sotBridge )
      {
         pIBridgeDesc->SetSlabOffset( pArtifact->GetSlabOffset(pgsTypes::metStart) );
      }
      else
      {
         const CGirderGroupData* pGroup = pIBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
         PierIndexType startPierIdx = pGroup->GetPierIndex(pgsTypes::metStart);
         PierIndexType endPierIdx   = pGroup->GetPierIndex(pgsTypes::metEnd);
         Float64 startOffset = pArtifact->GetSlabOffset(pgsTypes::metStart);
         Float64 endOffset   = pArtifact->GetSlabOffset(pgsTypes::metEnd);

         pIBridgeDesc->SetSlabOffset( segmentKey, startOffset, endOffset  );
      }

      if (pSpec->IsAssumedExcessCamberForLoad())
      {
         pgsTypes::AssumedExcessCamberType camberType = pIBridgeDesc->GetAssumedExcessCamberType();
         if (camberType == pgsTypes::aecBridge)
         {
            pIBridgeDesc->SetAssumedExcessCamber(pArtifact->GetAssumedExcessCamber());
         }
         else
         {
            pIBridgeDesc->SetAssumedExcessCamber(segmentKey.groupIndex, segmentKey.girderIndex, pArtifact->GetAssumedExcessCamber());
         }
      }
   }
}
