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

// TxDOTAgentImp.cpp : Implementation of CTxDOTAgentImp

#include "stdafx.h"
#include "TxDOTAgentImp.h"

#include <IFace\StatusCenter.h>
#include <IReportManager.h>
#include <IFace\Project.h>
#include <IFace\Artifact.h>
#include <IFace\Test1250.h>
#include <IFace\GirderHandling.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\DesignConfigUtil.h>

#include <EAF\EAFAutoProgress.h>
#include <EAF\EAFApp.h>
#include <EAF\EAFUtilities.h>

#include <MfcTools\XUnwind.h>

#include <Reporting\PGSuperTitlePageBuilder.h>
#include <Reporting\SpanGirderReportSpecificationBuilder.h>
#include <Reporting\SpecCheckSummaryChapterBuilder.h>
#include <Reporting\LoadingDetailsChapterBuilder.h>
#include <Reporting\LiveLoadDetailsChapterBuilder.h>
#include <Reporting\UserDefinedLoadsChapterBuilder.h>
#include <Reporting\SectPropChapterBuilder.h>
#include <Reporting\BearingSeatElevationsChapterBuilder2.h>

#include "TexasIBNSChapterBuilder.h"
#include "TexasGirderSummaryChapterBuilder.h"
#include "TexasPrestressSummaryChapterBuilder.h"
#include "TexasCamberAndDeflectionChapterBuilder.h"
#include "TexasStressChecksChapterBuilder.h"
#include "TexasMomentCapacityChapterBuilder.h"
#include "TexasShearChapterBuilder.h"
#include "TOGATitlePageBuilder.h"

#include "TxDOTOptionalDesignSummaryChapterBuilder.h"
#include "TogaStressChecksChapterBuilder.h"
#include "TogaSpecCheckSummaryChapterBuilder.h"
#include "TogaCamberAndDeflectionChapterBuilder.h"
#include "TogaLongSectionChapterBuilder.h"
#include "TexasHaunchChapterBuilder.h"

#include "TxDOTCadWriter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CTxDOTAgentImp

/////////////////////////////////////////////////////////////////////////////
// IAgentEx
//
STDMETHODIMP CTxDOTAgentImp::SetBroker(IBroker* pBroker)
{
   EAF_AGENT_SET_BROKER(pBroker);

   CComQIPtr<ICLSIDMap> clsidMap(pBroker);
   clsidMap->AddCLSID(_T("{360F7694-BE5B-4E97-864F-EF3575689C6E}"),_T("{3700B253-8489-457C-8A6D-D174F95C457C}"));

   return S_OK;
}

STDMETHODIMP CTxDOTAgentImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   pBrokerInit->RegInterface( IID_ITxDOTCadExport, this );
   return S_OK;
}

STDMETHODIMP CTxDOTAgentImp::Init()
{
   CREATE_LOGFILE("TxDOTAgent");

   EAF_AGENT_INIT;

   // We are going to add new reports to PGSuper. In order to do this, the agent that implements
   // IReportManager must be loaded. We have no way of knowing if that agent is loaded before
   // or after us. Request the broker call our Init2 function after all registered agents
   // are loaded
   return AGENT_S_SECONDPASSINIT;
}

STDMETHODIMP CTxDOTAgentImp::Init2()
{
   // Register our reports
   GET_IFACE(IReportManager,pRptMgr);

   //
   // Create report spec builders
   //

   std::shared_ptr<CReportSpecificationBuilder> pGirderRptSpecBuilder( std::make_shared<CGirderReportSpecificationBuilder>(m_pBroker,CGirderKey(0,0)) );
   std::shared_ptr<CReportSpecificationBuilder> pMultiGirderRptSpecBuilder( std::make_shared<CMultiGirderReportSpecificationBuilder>(m_pBroker) );
   std::shared_ptr<CReportSpecificationBuilder> pMultiViewRptSpecBuilder( std::make_shared<CMultiViewSpanGirderReportSpecificationBuilder>(m_pBroker) );


   // Texas Girder Schedule - use compacted title page
   std::unique_ptr<CReportBuilder> pRptBuilder(std::make_unique<CReportBuilder>(_T("TxDOT Girder Schedule Report")));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName(),false,false)) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTexasIBNSChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTexasCamberAndDeflectionChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTexasHaunchChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );

   // Texas Summary report - short
   pRptBuilder = std::make_unique<CReportBuilder>(_T("TxDOT Summary Report (Short Form)"));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName(), false,false)) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CSpecCheckSummaryChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTexasGirderSummaryChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLoadingDetailsChapterBuilder(true,true,false,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLiveLoadDetailsChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CUserDefinedLoadsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTexasCamberAndDeflectionChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTexasHaunchChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CBearingDeductChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );

   // Texas Summary report - long form
   pRptBuilder = std::make_unique<CReportBuilder>(_T("TxDOT Summary Report (Long Form)"));
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(new CPGSuperTitlePageBuilder(m_pBroker,pRptBuilder->GetName(),true,false)) );
   pRptBuilder->SetReportSpecificationBuilder( pMultiViewRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CSpecCheckSummaryChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTexasGirderSummaryChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLoadingDetailsChapterBuilder(true,true,false,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLiveLoadDetailsChapterBuilder(true,false)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CUserDefinedLoadsChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTexasCamberAndDeflectionChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTexasHaunchChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CBearingDeductChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTexasPrestressSummaryChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTexasStressChecksChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTexasMomentCapacityChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTexasShearChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );

   // TOGA Long Form
   pRptBuilder = std::make_unique<CReportBuilder>(_T("TxDOT Optional Girder Analysis (TOGA) - Long Report"),true);
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(new CTOGATitlePageBuilder(m_pBroker,pRptBuilder->GetName(),false)) );
   pRptBuilder->SetReportSpecificationBuilder( pGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTogaSpecCheckSummaryChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTxDOTOptionalDesignSummaryChapterBuilder()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTogaLongSectionChapterBuilder()) );
//   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CSectPropChapterBuilder()) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CSectPropChapterBuilder(true,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CLoadingDetailsChapterBuilder(true,true,false,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CUserDefinedLoadsChapterBuilder(true,true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTexasPrestressSummaryChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTogaCamberAndDeflectionChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTexasHaunchChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTogaStressChecksChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTexasMomentCapacityChapterBuilder) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTexasShearChapterBuilder) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );

   // TOGA Short Form
   pRptBuilder = std::make_unique<CReportBuilder>(_T("TxDOT Optional Girder Analysis (TOGA) - Short Report"),true);
#if defined _DEBUG || defined _BETA_VERSION
   pRptBuilder->IncludeTimingChapter();
#endif
   pRptBuilder->AddTitlePageBuilder( std::shared_ptr<CTitlePageBuilder>(new CTOGATitlePageBuilder(m_pBroker,pRptBuilder->GetName(),false)) );
   pRptBuilder->SetReportSpecificationBuilder( pGirderRptSpecBuilder );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTogaSpecCheckSummaryChapterBuilder(true)) );
   pRptBuilder->AddChapterBuilder( std::shared_ptr<CChapterBuilder>(new CTxDOTOptionalDesignSummaryChapterBuilder()) );
   pRptMgr->AddReportBuilder( pRptBuilder.release() );

   return S_OK;
}

STDMETHODIMP CTxDOTAgentImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_TxDOTAgent;
   return S_OK;
}

STDMETHODIMP CTxDOTAgentImp::Reset()
{
   return S_OK;
}

STDMETHODIMP CTxDOTAgentImp::ShutDown()
{
   EAF_AGENT_CLEAR_INTERFACE_CACHE;
   CLOSE_LOGFILE;
   return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// ITxDOTCadExport
int CTxDOTAgentImp::WriteCADDataToFile(FILE *fp, IBroker* pBroker, const CSegmentKey& segmentKey, TxDOTCadExportFormatType format, bool designSucceeded)
{
   return TxDOT_WriteCADDataToFile(fp,pBroker,segmentKey, format, designSucceeded);
}

int CTxDOTAgentImp::WriteDistributionFactorsToFile(FILE *fp, IBroker* pBroker, const CSegmentKey& segmentKey)
{
   return TxDOT_WriteDistributionFactorsToFile(fp,pBroker,segmentKey);
}

/////////////////////////////////////////////////////////////////////////////
// IEAFProcessCommandLine
BOOL CTxDOTAgentImp::ProcessCommandLineOptions(CEAFCommandLineInfo& cmdInfo)
{
   // cmdInfo is the command line information from the application. The application
   // doesn't know about this plug-in at the time the command line parameters are parsed
   //
   // Re-parse the parameters with our own command line information object
   CTxDOTCommandLineInfo txCmdInfo;

   EAFGetApp()->ParseCommandLine(txCmdInfo);
   cmdInfo = txCmdInfo;

   if (txCmdInfo.m_DoTxCadReport && !txCmdInfo.m_bError)
   {
      ProcessTxDotCad(txCmdInfo);
      return TRUE;
   }
   else if (txCmdInfo.m_DoTogaTest && !txCmdInfo.m_bError)
   {
      ProcessTOGAReport(txCmdInfo);
      return TRUE;
   }

   return (txCmdInfo.m_bError ? TRUE : FALSE);
}


void CTxDOTAgentImp::ProcessTxDotCad(const CTxDOTCommandLineInfo& rCmdInfo)
{
   USES_CONVERSION;

   ASSERT(rCmdInfo.m_DoTxCadReport);

   pgsAutoLabel auto_label;


   if (rCmdInfo.m_TxGirder != TXALLGIRDERS && 
       rCmdInfo.m_TxGirder != TXEIGIRDERS && 
       (rCmdInfo.m_TxGirder == INVALID_INDEX || 27 < rCmdInfo.m_TxGirder))
   {
      ::AfxMessageBox(_T("Invalid girder specified on command line for TxDOT CAD report"));
      return;
   }
/*
   if (rCmdInfo.m_TxSpan != ALL_SPANS && rCmdInfo.m_TxSpan < 0)
   {
      ::AfxMessageBox(_T("Invalid span specified on command line for TxDOT CAD report"));
      return;
   }
*/
   CString errfile;
   if (CreateTxDOTFileNames(rCmdInfo.m_TxOutputFile, &errfile))
   {
      try
      {
         if ( !DoTxDotCadReport(rCmdInfo.m_TxOutputFile, errfile, rCmdInfo) )
         {
            CString msg = CString(_T("Error - Running test on file"))+rCmdInfo.m_strFileName;
            ::AfxMessageBox(msg);
         }
      }
      catch(const sysXBase& e)
      {
         std::_tstring msg;
         e.GetErrorMessage(&msg);
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running TxDOT CAD report for input file: ")<<rCmdInfo.m_strFileName<<std::endl<< msg;
      }
      catch(CException* pex)
      {
         TCHAR   szCause[255];
         CString strFormatted;
         pex->GetErrorMessage(szCause, 255);
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running TxDOT CAD report for input file: ")<<rCmdInfo.m_strFileName<<std::endl<< szCause;
         delete pex;
      }
      catch(CException& ex)
      {
         TCHAR   szCause[255];
         CString strFormatted;
         ex.GetErrorMessage(szCause, 255);
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running TxDOT CAD report for input file: ")<<rCmdInfo.m_strFileName<<std::endl<< szCause;
      }
      catch(const std::exception* pex)
      {
         std::_tstring strMsg(CA2T(pex->what()));
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running TxDOT CAD report for input file: ")<<rCmdInfo.m_strFileName<<std::endl<<strMsg<< std::endl;
         delete pex;
      }
      catch(const std::exception& ex)
      {
         std::_tstring strMsg(CA2T(ex.what()));
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Error running TxDOT CAD report for input file: ")<<rCmdInfo.m_strFileName<<std::endl<<strMsg<< std::endl;
      }
      catch(...)
      {
         std::_tofstream os;
         os.open(errfile);
         os <<_T("Unknown Error running TxDOT CAD report for input file: ")<<rCmdInfo.m_strFileName;
      }
   }
}


bool CTxDOTAgentImp::CreateTxDOTFileNames(const CString& output, CString* pErrFileName)
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

bool CTxDOTAgentImp::DoTxDotCadReport(const CString& outputFileName, const CString& errorFileName, const CTxDOTCommandLineInfo& txInfo)
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

   // Loop over all span/girder combos and create results
   std::vector<SpanGirderHashType>::iterator it(spn_grd_list.begin());
   std::vector<SpanGirderHashType>::iterator end(spn_grd_list.end());
   for( ; it != end; it++)
   {
      SpanGirderHashType key = *it;
      SpanIndexType span;
      GirderIndexType girder;
      UnhashSpanGirder(key, &span, &girder);

      CSegmentKey segmentKey(span,girder,0);

      CString strMessage;
      strMessage.Format(_T("Creating TxDOT CAD report for Span %d, Girder %s"),LABEL_SPAN(span), LABEL_GIRDER(girder));
      pProgress->UpdateMessage(strMessage);

      // See if we need to run a design
      bool designSucceeded=true;
      if (txInfo.m_TxRunType == CTxDOTCommandLineInfo::txrDesign || txInfo.m_TxRunType == CTxDOTCommandLineInfo::txrDesignShear)
      {
         // get design options from library entry. 
         GET_IFACE(ISpecification,pSpecification);

         std::vector<arDesignOptions> des_options_coll = pSpecification->GetDesignOptions(segmentKey);
         IndexType do_cnt = des_options_coll.size();
         IndexType do_idx = 1;

         // Add command line settings to options
         for(std::vector<arDesignOptions>::iterator it = des_options_coll.begin(); it!=des_options_coll.end(); it++)
         {
            arDesignOptions& des_options = *it;

            // Set up for shear design. 
            des_options.doDesignForShear = txInfo.m_TxRunType == CTxDOTCommandLineInfo::txrDesignShear;
            if(des_options.doDesignForShear)
            {
               // If stirrup zones are not symmetrical in test file, design using existing layout
               GET_IFACE(IStirrupGeometry,pStirrupGeom);
               bool are_symm = pStirrupGeom->AreStirrupZonesSymmetrical(segmentKey);
               des_options.doDesignStirrupLayout = are_symm ? slLayoutStirrups : slRetainExistingLayout;
            }
         }

         GET_IFACE(IArtifact,pIArtifact);
         const pgsGirderDesignArtifact* pGirderDesignArtifact;
         const pgsSegmentDesignArtifact* pArtifact;
         try
         {
            // Design the girder
            pGirderDesignArtifact = pIArtifact->CreateDesignArtifact( segmentKey, des_options_coll);
            pArtifact = pGirderDesignArtifact->GetSegmentDesignArtifact(segmentKey.segmentIndex);
            ATLASSERT(segmentKey.IsEqual(pArtifact->GetSegmentKey()));
         
            if (pArtifact->GetOutcome() != pgsSegmentDesignArtifact::Success)
            {
               err_file <<_T("Design was unsuccessful")<<std::endl;
               designSucceeded=false;
            }

            // Copy the design to the bridge
            SaveFlexureDesign(segmentKey, pArtifact);
         }
         catch(...)
         {
           err_file <<_T("Design Failed for span")<<span<<_T(" girder ")<<girder<<std::endl;
            return false;
         }
      }

      GET_IFACE(ITxDOTCadExport,pTxDOTCadExport);
      if ( !pTxDOTCadExport )
      {
         AfxMessageBox(_T("The TxDOT Cad Exporter is not currently installed"));
         return false;
      }

      if (txInfo.m_TxRunType == CTxDOTCommandLineInfo::TxrDistributionFactors)
      {
         // Write distribution factor data to file
         try
         {
            if (CAD_SUCCESS != pTxDOTCadExport->WriteDistributionFactorsToFile (fp, this->m_pBroker, segmentKey))
            {
               err_file <<_T("Warning: An error occured while writing to File")<<std::endl;
	            return false;
            }
         }
         catch(CXUnwind* pExc)
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
         if (CAD_SUCCESS != pTxDOTCadExport->WriteCADDataToFile(fp, this->m_pBroker, segmentKey, (TxDOTCadExportFormatType)txInfo.m_TxFType, designSucceeded) )
         {
            err_file <<_T("Warning: An error occured while writing to File")<<std::endl;
	         return false;
         }
	  }
   }

   /* Close the open text file */
   fclose (fp);

   // ---------------------------------------------
   // Run a 12-50 output if this is a test file
   if (txInfo.m_TxFType == CTxDOTCommandLineInfo::txfTest)
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

void CTxDOTAgentImp::SaveFlexureDesign(const CSegmentKey& segmentKey,const pgsSegmentDesignArtifact* pArtifact)
{
   // Artifact does hard work of converting to girder data
   CPrecastSegmentData segmentData = pArtifact->GetSegmentData();
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   GET_IFACE_NOCHECK(ISpecification,pSpec);
   pIBridgeDesc->SetPrecastSegmentData(segmentKey,segmentData);

   const arDesignOptions& design_options = pArtifact->GetDesignOptions();

   if (design_options.doDesignForFlexure != dtNoDesign && design_options.doDesignSlabOffset != sodNoADesign)
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

         pIBridgeDesc->SetSlabOffset( segmentKey.groupIndex, startPierIdx, segmentKey.girderIndex, startOffset);
         pIBridgeDesc->SetSlabOffset( segmentKey.groupIndex, endPierIdx,   segmentKey.girderIndex, endOffset  );
      }

      if (pSpec->IsAssExcessCamberForLoad())
      {
         pgsTypes::AssExcessCamberType camberType = pIBridgeDesc->GetAssExcessCamberType();
         if (camberType == pgsTypes::aecBridge)
         {
            pIBridgeDesc->SetAssExcessCamber(pArtifact->GetAssExcessCamber());
         }
         else
         {
            pIBridgeDesc->SetAssExcessCamber(segmentKey.groupIndex, segmentKey.girderIndex, pArtifact->GetAssExcessCamber());
         }
      }
   }
}

void CTxDOTAgentImp::ProcessTOGAReport(const CTxDOTCommandLineInfo& rCmdInfo)
{
   USES_CONVERSION;

   ASSERT(rCmdInfo.m_DoTogaTest);

   CString errfile;
   CreateTxDOTFileNames(rCmdInfo.m_TxOutputFile, &errfile);

   try
   {
      if ( !DoTOGAReport(rCmdInfo.m_TxOutputFile, rCmdInfo) )
      {
         CString msg = CString(_T("Error - Running test on file"))+rCmdInfo.m_strFileName;
         ::AfxMessageBox(msg);
      }
   }
   catch(const sysXBase& e)
   {
      std::_tstring msg;
      e.GetErrorMessage(&msg);
      std::_tofstream os;
      os.open(errfile);
      os <<_T("Error running TOGA report for input file: ")<<rCmdInfo.m_strFileName<<std::endl<< msg;
   }
   catch(CException* pex)
   {
      TCHAR   szCause[255];
      CString strFormatted;
      pex->GetErrorMessage(szCause, 255);
      std::_tofstream os;
      os.open(errfile);
      os <<_T("Error running TOGA report for input file: ")<<rCmdInfo.m_strFileName<<std::endl<< szCause;
      delete pex;
   }
   catch(CException& ex)
   {
      TCHAR   szCause[255];
      CString strFormatted;
      ex.GetErrorMessage(szCause, 255);
      std::_tofstream os;
      os.open(errfile);
      os <<_T("Error running TOGA report for input file: ")<<rCmdInfo.m_strFileName<<std::endl<< szCause;
   }
   catch(const std::exception* pex)
   {
      std::_tstring strMsg(CA2T(pex->what()));
      std::_tofstream os;
      os.open(errfile);
      os <<_T("Error running TOGA report for input file: ")<<rCmdInfo.m_strFileName<<std::endl<<strMsg<< std::endl;
      delete pex;
   }
   catch(const std::exception& ex)
   {
      std::_tstring strMsg(CA2T(ex.what()));
      std::_tofstream os;
      os.open(errfile);
      os <<_T("Error running TOGA report for input file: ")<<rCmdInfo.m_strFileName<<std::endl<<strMsg<< std::endl;
   }
   catch(...)
   {
      std::_tofstream os;
      os.open(errfile);
      os <<_T("Unknown Error running TOGAreport for input file: ")<<rCmdInfo.m_strFileName;
   }
}

bool CTxDOTAgentImp::DoTOGAReport(const CString& outputFileName, const CTxDOTCommandLineInfo& rCmdInfo)
{
   // Called from the command line processing in the Application object

   // Open/create the specified output file 
   FILE	*fp;
   errno_t result;
   if (rCmdInfo.m_DoAppendToFile)
   {
      result = _tfopen_s(&fp, LPCTSTR (outputFileName), _T("a+"));
   }
   else
   {
      result = _tfopen_s(&fp, LPCTSTR (outputFileName), _T("w+"));
   }

   if (result != 0 || fp == nullptr)
   {
      CString errfile;
      CreateTxDOTFileNames(rCmdInfo.m_TxOutputFile, &errfile);
      std::_tofstream err_file(errfile);
      err_file<<_T("Error: Output file could not be Created.")<<std::endl;
	   return false;
   }

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   // Write data to file
   if (CAD_SUCCESS != TxDOT_WriteTOGAReportToFile(fp, this->m_pBroker))
   {
      CString errfile;
      CreateTxDOTFileNames(rCmdInfo.m_TxOutputFile, &errfile);
      std::_tofstream err_file(errfile);
      err_file <<_T("Warning: An error occured while writing to File")<<std::endl;
      return false;
   }

   /* Close the open text file */
   fclose (fp);

   return true;
}
